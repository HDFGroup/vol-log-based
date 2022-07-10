/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
//
#include <cstring>
#include <functional>
#include <map>
#include <vector>
//
#include <hdf5.h>
#include <mpi.h>
//
#include "H5VL_log_dataset.hpp"
#include "H5VL_logi_dataspace.hpp"
#include "H5VL_logi_err.hpp"
#include "H5VL_logi_meta.hpp"
#include "H5VL_logi_util.hpp"
#include "H5VL_logi_zip.hpp"

void H5VL_logi_metaentry_ref_decode (H5VL_log_dset_info_t &dset,
                                     void *ent,
                                     H5VL_logi_metaentry_t &block,
                                     std::map<char *, std::vector<H5VL_logi_metasel_t>> &bcache) {
    int i;
    char *bufp = (char *)ent;  // Next byte to process in ent
    size_t bsize;              // Size of a selection block
    hsize_t recnum;            // Record number
    MPI_Offset roff;           // Related offset of the referenced entry

    // Get the header
    block.hdr = *((H5VL_logi_meta_hdr *)bufp);
    bufp += sizeof (H5VL_logi_meta_hdr);

    // Entry size must be > 0
    if (block.hdr.meta_size <= 0) { RET_ERR ("Invalid metadata entry") }

    // Check if it is a record entry
    if (block.hdr.flag & H5VL_LOGI_META_FLAG_REC) {
        // Get record number
#ifdef WORDS_BIGENDIAN
        H5VL_logi_llreverse ((uint64_t *)bufp);
#endif
        recnum = *((MPI_Offset *)bufp);
        bufp += sizeof (MPI_Offset);
    }

    // Get referenced selections
#ifdef WORDS_BIGENDIAN
    H5VL_logi_llreverse ((uint64_t *)bufp);
#endif
    roff       = *((MPI_Offset *)bufp);
    block.sels = bcache[(char *)ent + roff];

    // Overwrite first dim if it is rec entry
    if (block.hdr.flag & H5VL_LOGI_META_FLAG_REC) {
        for (auto &sel : block.sels) {
            sel.start[0] = recnum;
            sel.count[0] = 1;
        }
    }

    // Calculate the unfiltered size of the data block
    block.dsize = 0;
    for (auto &s : block.sels) {
        bsize = dset.esize;
        for (i = 0; i < (int)(dset.ndim); i++) { block.dsize *= s.count[i]; }
        block.dsize += bsize;
    }

    // Calculate the unfiltered size of the data block
    // Data size = data offset of the last block + size of the alst block
    block.dsize = dset.esize;
    for (i = 0; i < (int)(dset.ndim); i++) {
        block.dsize *= block.sels[block.sels.size () - 1].count[i];
    }
    block.dsize += block.sels[block.sels.size () - 1].doff;
}

void H5VL_logi_metaentry_decode (H5VL_log_dset_info_t &dset,
                                 void *ent,
                                 H5VL_logi_metaentry_t &block) {
    MPI_Offset dsteps[H5S_MAX_RANK];  // corrdinate to offset encoding info in ent
    H5VL_logi_metaentry_decode (dset, ent, block, dsteps);
}

void H5VL_logi_metaentry_decode (H5VL_log_dset_info_t &dset,
                                 void *ent,
                                 H5VL_logi_metaentry_t &block,
                                 MPI_Offset *dsteps) {
    int i, j;
    int nsel;                  // Nunmber of selections in ent
    char *zbuf = NULL;         // Buffer for decompressing metadata
    char *bufp = (char *)ent;  // Next byte to process in ent
    MPI_Offset *bp;            // Next 8 byte selection to process
    hsize_t recnum;            // Record number
    int encdim;                // number of dim encoded (ndim or ndim - 1)
    int isrec;                 // Is a record entry
    MPI_Offset bsize;          // Size of decomrpessed selection
    MPI_Offset esize;          // Size of a decomrpessed selection block
#ifdef ENABLE_ZLIB
    bool zbufalloc = false;  // Should we free zbuf
    H5VL_logi_err_finally finally ([&block, &zbufalloc, &zbuf] () -> void {
        if (block.hdr.flag & H5VL_LOGI_META_FLAG_SEL_DEFLATE) {
            if (zbufalloc) { free (zbuf); }
        }
    });
#endif

    // Get the header
    block.hdr = *((H5VL_logi_meta_hdr *)bufp);
    bufp += sizeof (H5VL_logi_meta_hdr);

    // Entry size must be > 0
    if (block.hdr.meta_size <= 0) { RET_ERR ("Invalid metadata entry") }

    // Check if it is a record entry
    if (block.hdr.flag & H5VL_LOGI_META_FLAG_REC) {
        encdim = dset.ndim - 1;
        isrec  = 1;
#ifdef WORDS_BIGENDIAN
        H5VL_logi_llreverse ((uint64_t *)(bufp));
#endif
        // Get record number
        recnum = *((MPI_Offset *)bufp);
        bufp += sizeof (MPI_Offset);
    } else {
        isrec  = 0;
        encdim = dset.ndim;
    }

    // If there is more than 1 selection
    if (block.hdr.flag & H5VL_LOGI_META_FLAG_MUL_SEL) {
        // Get number of selection (right after the header)
#ifdef WORDS_BIGENDIAN
        H5VL_logi_lreverse ((uint32_t *)(bufp));
#endif
        nsel = *((int *)bufp);
        bufp += sizeof (int);

        // Calculate buffer size;
        esize = 0;
        bsize = 0;
        // Count selections
        if (block.hdr.flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {
            // Encoded start and count
            esize = sizeof (MPI_Offset) * 2;
            bsize += sizeof (MPI_Offset) * (encdim - 1);
        } else {
            // Start and count as coordinate
            esize = sizeof (MPI_Offset) * 2 * encdim;
        }
        bsize += esize * nsel;

        // If the rest of the entry is comrpessed, decomrpess it
        if (block.hdr.flag & H5VL_LOGI_META_FLAG_SEL_DEFLATE) {
#ifdef ENABLE_ZLIB
            int inlen;  // Size of comrpessed metadata
            int clen;   // Size of decompressed metadata

            // Allocate decompression buffer
            zbuf      = (char *)malloc (bsize);
            zbufalloc = true;

            // Decompress the metadata
            inlen = block.hdr.meta_size - sizeof (H5VL_logi_meta_hdr) - sizeof (int);
            clen  = bsize;
            H5VL_log_zip_decompress (bufp, inlen, zbuf, &clen);
#else
            RET_ERR ("Comrpessed Metadata Support Not Enabled")
#endif
        } else {
            zbuf = bufp;
#ifdef ENABLE_ZLIB
            zbufalloc = false;
#endif
        }
    } else {
        // Entries with single selection will never be comrpessed
        nsel  = 1;
        bsize = sizeof (MPI_Offset) * 2 * encdim * nsel;
        zbuf  = bufp;
#ifdef ENABLE_ZLIB
        zbufalloc = false;
#endif
    }

    bp = (MPI_Offset *)zbuf;
#ifdef WORDS_BIGENDIAN
    H5VL_logi_llreverse ((uint64_t *)(bp), (uint64_t *)(bp + bsize));
#endif
    // Retrieve the selection encoding info
    if (block.hdr.flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {
        memcpy (dsteps, bp, sizeof (MPI_Offset) * (encdim - 1));
        dsteps[encdim - 1] = 1;
        bp += encdim - 1;
    }

    /* Old code for merged metadata
     * H5VL_LOGI_META_FLAG_MUL_SELX is not used anymore
    if (block.hdr.flag & H5VL_LOGI_META_FLAG_MUL_SELX) {
            for (i = 0; i < nsel; i++) {
                    block.foff = *((MPI_Offset *)bp);
                    bp++;
                    block.fsize = *((MPI_Offset *)bp);
                    bp++;

                    block.sels.resize (1);

                    if (block.hdr.flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {
                            MPI_Offset off;

                            // Decode start
                            H5VL_logi_sel_decode (dset.ndim, dsteps, *((MPI_Offset *)bp),
    block.sels[0].start); bp++;

                            // Decode count
                            H5VL_logi_sel_decode (dset.ndim, dsteps, *((MPI_Offset *)bp),
    block.sels[0].count); bp++; } else { memcpy (block.sels[0].start, bp, sizeof (MPI_Offset) *
    dset.ndim); bp += dset.ndim; memcpy (block.sels[0].count, bp, sizeof (MPI_Offset) * dset.ndim);
                            bp += dset.ndim;
                    }
                    block.sels[0].doff = 0;
                    block.dsize		   = dset.esize;
                    for (j = 0; j < dset.ndim; j++) { block.dsize *= block.sels[0].count[j]; }
                    idx.insert (block);
            }
    } else
    */

    block.sels.resize (nsel);

    // Fill up the first dim if it is rec entry
    if (isrec) {
        for (auto &sel : block.sels) {
            sel.start[0] = recnum;
            sel.count[0] = 1;
        }
    }

    // Retrieve starts of selections
    for (i = 0; i < nsel; i++) {
        if (block.hdr.flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {
            H5VL_logi_sel_decode (encdim, dsteps, *((MPI_Offset *)bp), block.sels[i].start + isrec);
            bp++;
        } else {
            memcpy (block.sels[i].start + isrec, bp, sizeof (MPI_Offset) * encdim);
            bp += encdim;
        }
    }
    // Retrieve counts of selections
    for (i = 0; i < nsel; i++) {
        if (block.hdr.flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {
            H5VL_logi_sel_decode (encdim, dsteps, *((MPI_Offset *)bp), block.sels[i].count + isrec);
            bp++;
        } else {
            memcpy (block.sels[i].count + isrec, bp, sizeof (MPI_Offset) * encdim);
            bp += encdim;
        }
        // Calculate the offset of data of the selection within the unfiltered data block
        if (i > 0) {
            block.sels[i].doff = dset.esize;
            for (j = 0; j < (int)(dset.ndim); j++) {
                block.sels[i].doff *= block.sels[i - 1].count[j];
            }
            block.sels[i].doff += block.sels[i - 1].doff;
        } else {
            block.sels[i].doff = 0;
        }
    }

    // Calculate the unfiltered size of the data block
    // Data size = data offset of the last block + size of the last block
    block.dsize = dset.esize;
    for (j = 0; j < encdim; j++) { block.dsize *= block.sels[nsel - 1].count[j]; }
    block.dsize += block.sels[nsel - 1].doff;
}

/*
H5VL_logi_metacache::H5VL_logi_metacache(){

}

H5VL_logi_metacache::~H5VL_logi_metacache(){
        clear();
}

int H5VL_logi_metacache::add(char *buf, size_t size) {
        char *tmp;

        tmp=malloc(size);
        CHECK_PTR(tmp);
        memcpy(tmp,buf,size);
}

int H5VL_logi_metacache::find(char *buf, size_t size) {

}

void H5VL_logi_metacache::clear() {
        for(auto &t:table){
                free(t.first.first);
        }
        table.clear();
}
*/
