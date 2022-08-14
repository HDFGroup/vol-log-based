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
#include <map>
#include <unordered_map>
#include <vector>
//
#include <mpi.h>
#include <sys/types.h>
#include <unistd.h>
//
#include "H5VL_log_dataset.hpp"
#include "H5VL_log_dataseti.hpp"
#include "H5VL_log_filei.hpp"
#include "H5VL_logi.hpp"
#include "H5VL_logi_idx.hpp"
#include "H5VL_logi_meta.hpp"
#include "H5VL_logi_nb.hpp"
#include "H5VL_logi_util.hpp"
#include "H5VL_logi_wrapper.hpp"

#define H5VL_LOGI_MERGED_REQ_SEL_RESERVE 32
#define H5VL_LOGI_MERGED_REQ_SEL_MUL     3

H5VL_log_wreq_t::H5VL_log_wreq_t () {}
H5VL_log_wreq_t::H5VL_log_wreq_t (const H5VL_log_wreq_t &rhs) {
    meta_buf = (char *)malloc (rhs.hdr->meta_size);
    if (!meta_buf) throw "OOM";
    hdr     = (H5VL_logi_meta_hdr *)meta_buf;
    sel_buf = meta_buf + (rhs.sel_buf - rhs.meta_buf);
    memcpy (meta_buf, rhs.meta_buf, rhs.hdr->meta_size);
}

H5VL_log_wreq_t::H5VL_log_wreq_t (void *dset, H5VL_log_selections *sels) {
    H5VL_log_dset_t *dp       = (H5VL_log_dset_t *)dset;
    H5VL_log_dset_info_t *dip = dp->fp->dsets_info[dp->id];  // Dataset info
    size_t mbsize;
    hsize_t recnum;  // Record number
    int i;
    int encdim;  // number of dim encoded (ndim or ndim - 1)
    int flag;
    char *bufp;

    // Anticipated offset in the metadata dataset
    meta_off = dp->fp->mdsize;

    // Flags
    flag   = 0;
    encdim = dip->ndim;
    // Check if it is a record write
    if (dip->ndim && dip->mdims[0] == H5S_UNLIMITED) {
        if (sels->nsel > 0) {
            recnum = sels->starts[0][0];
            for (i = 0; i < sels->nsel; i++) {
                if (sels->counts[i][0] != 1) break;
                if (sels->starts[i][0] != recnum) break;
            }
            if (i == sels->nsel) {
                flag |= H5VL_LOGI_META_FLAG_REC;
                encdim--;
            }
        }
    }
    nsel = sels->nsel;
    if (nsel > 1) {
        flag |= H5VL_LOGI_META_FLAG_MUL_SEL;
        if ((encdim > 1) && (dp->fp->config & H5VL_FILEI_CONFIG_SEL_ENCODE)) {
            flag |= H5VL_LOGI_META_FLAG_SEL_ENCODE;
        }
        if (dp->fp->config & H5VL_FILEI_CONFIG_SEL_DEFLATE) {
            flag |= H5VL_LOGI_META_FLAG_SEL_DEFLATE;
        }
    }

    // Allocate metadata buffer
    mbsize = sizeof (H5VL_logi_meta_hdr);
    if (flag & H5VL_LOGI_META_FLAG_REC) {
        mbsize += sizeof (MPI_Offset);  // Record
    }
    if (flag & H5VL_LOGI_META_FLAG_MUL_SEL) {
        mbsize += sizeof (int);  // N
    }
    if (flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {
        mbsize += sizeof (MPI_Offset) * (encdim - 1 + nsel * 2);
    } else {
        mbsize += sizeof (MPI_Offset) * (encdim * nsel * 2);
    }
    this->meta_buf = (char *)malloc (mbsize);
    CHECK_PTR (this->meta_buf)
    this->sel_buf = this->meta_buf + sizeof (H5VL_logi_meta_hdr);

    // Fill up the header
    this->hdr            = (H5VL_logi_meta_hdr *)(this->meta_buf);
    this->hdr->did       = dp->id;
    this->hdr->flag      = flag;
    this->hdr->meta_size = mbsize;

    // Encoding selection

    // Add record number
    if (flag & H5VL_LOGI_META_FLAG_REC) {
#ifdef WORDS_BIGENDIAN
        H5VL_logi_llreverse ((uint64_t *)(&recnum));
#endif
        *((MPI_Offset *)this->sel_buf) = recnum;
        this->sel_buf += sizeof (MPI_Offset);
    }
    bufp = this->sel_buf;

    // Add nreq field if more than 1 blocks
    if (flag & H5VL_LOGI_META_FLAG_MUL_SEL) {
#ifdef WORDS_BIGENDIAN
        H5VL_logi_lreverse ((uint32_t *)(&nsel));
#endif
        *((int *)bufp) = nsel;
        bufp += sizeof (int);
    }
#ifdef LOGVOL_DEBUG
    else {
        if (nsel > 1) { RET_ERR ("Meta flag mismatch") }
    }
#endif

#ifdef WORDS_BIGENDIAN
    {
        uint64_t *rstart = (uint64_t *)bufp;  // Start addr to do endian reverse
#endif

        // Dsteps
        if (flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {
            memcpy (bufp, dip->dsteps, sizeof (MPI_Offset) * (encdim - 1));
            bufp += sizeof (MPI_Offset) * (encdim - 1);
        }

        if (flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {
            sels->encode (bufp, dip->dsteps, flag & H5VL_LOGI_META_FLAG_REC ? 1 : 0);
        } else {
            sels->encode (bufp, NULL, flag & H5VL_LOGI_META_FLAG_REC ? 1 : 0);
        }
#ifdef WORDS_BIGENDIAN
        H5VL_logi_llreverse (rstart, (uint64_t *)(meta_buf + mbsize));
    }
#endif
}

H5VL_log_wreq_t::~H5VL_log_wreq_t () {
    if (meta_buf) { free (meta_buf); }
}

void H5VL_log_wreq_t::resize (size_t size) {
    LOG_VOL_ASSERT (size >= sizeof (H5VL_logi_meta_hdr))

    // Resize metadata buffer
    sel_buf -= (size_t)meta_buf;
    meta_buf = (char *)realloc (meta_buf, size);
    CHECK_PTR (meta_buf)
    sel_buf += (size_t)meta_buf;

    // Update header
    hdr            = (H5VL_logi_meta_hdr *)meta_buf;
    hdr->meta_size = size;
}

size_t std::hash<H5VL_log_wreq_t>::operator() (H5VL_log_wreq_t const &r) const noexcept {
    size_t ret = 0;
    size_t *val;
    size_t *end;
    size_t selsize = r.hdr->meta_size - (r.sel_buf - r.meta_buf);

    end = (size_t *)((char *)(r.sel_buf) + selsize - selsize % sizeof (size_t));
    for (val = (size_t *)(r.sel_buf); val < end; val++) { ret ^= *val; }

    return ret;
}

bool H5VL_log_wreq_t::operator== (const H5VL_log_wreq_t rhs) const {
    if (hdr->meta_size != rhs.hdr->meta_size) { return false; }
    return memcmp (sel_buf, rhs.sel_buf, hdr->meta_size - (sel_buf - meta_buf)) == 0;
}

bool H5VL_log_wreq_t::operator== (H5VL_log_wreq_t &rhs) const {
    if (hdr->meta_size != rhs.hdr->meta_size) { return false; }
    return memcmp (sel_buf, rhs.sel_buf, hdr->meta_size - (sel_buf - meta_buf)) == 0;
}

H5VL_log_merged_wreq_t::H5VL_log_merged_wreq_t () {
    this->meta_buf   = NULL;
    this->hdr->fsize = 0;
    this->nsel       = 0;
}

H5VL_log_merged_wreq_t::H5VL_log_merged_wreq_t (H5VL_log_dset_t *dp, int nsel) {
    this->init (dp, nsel);
}

H5VL_log_merged_wreq_t::H5VL_log_merged_wreq_t (H5VL_log_file_t *fp, int id, int nsel) {
    this->init (fp, id, nsel);
}

H5VL_log_merged_wreq_t::~H5VL_log_merged_wreq_t () {}

void H5VL_log_merged_wreq_t::init (H5VL_log_file_t *fp, int id, int nsel) {
    int flag = 0;

    if (nsel < H5VL_LOGI_MERGED_REQ_SEL_RESERVE) { nsel = H5VL_LOGI_MERGED_REQ_SEL_RESERVE; }

    this->meta_size_alloc = sizeof (H5VL_logi_meta_hdr) + sizeof (int);

    flag = H5VL_LOGI_META_FLAG_MUL_SEL;
    if ((fp->dsets_info[id]->ndim > 1) && (fp->config & H5VL_FILEI_CONFIG_SEL_ENCODE)) {
        flag |= H5VL_LOGI_META_FLAG_SEL_ENCODE;
        this->meta_size_alloc += sizeof (MPI_Offset) * (2 * nsel + fp->dsets_info[id]->ndim);
    } else {
        this->meta_size_alloc += sizeof (MPI_Offset) * fp->dsets_info[id]->ndim * 2 * nsel;
    }

    if (fp->config & H5VL_FILEI_CONFIG_SEL_DEFLATE) { flag |= H5VL_LOGI_META_FLAG_SEL_DEFLATE; }

    this->meta_buf = (char *)malloc (this->meta_size_alloc);
    CHECK_PTR (this->meta_buf);
    this->sel_buf = this->meta_buf + sizeof (H5VL_logi_meta_hdr);
    this->mbufe   = this->meta_buf + this->meta_size_alloc;
    // Skip headers and nreq (unknown at this time)
    this->mbufp = this->meta_buf + sizeof (H5VL_logi_meta_hdr) + sizeof (int);

    // Pack dsteps
    if (flag & H5VL_FILEI_CONFIG_SEL_ENCODE) {
        memcpy (this->mbufp, fp->dsets_info[id]->dsteps,
                sizeof (MPI_Offset) * (fp->dsets_info[id]->ndim - 1));
        this->mbufp += sizeof (MPI_Offset) * (fp->dsets_info[id]->ndim - 1);
    }

    // Fill up the header
    this->hdr            = (H5VL_logi_meta_hdr *)(this->meta_buf);
    this->hdr->did       = id;
    this->hdr->flag      = flag;
    this->hdr->meta_size = this->mbufp - this->meta_buf;

    // There is no aggreagated blocks yet.
    // The input nsel means the number reserved, not the current number
    this->nsel = 0;

    // No aggregated requests, data size set to 0
    this->hdr->fsize = 0;
}

void H5VL_log_merged_wreq_t::init (H5VL_log_dset_t *dp, int nsel) {
    this->init (dp->fp, dp->id, nsel);
}

void H5VL_log_merged_wreq_t::reserve (size_t size) {
    if (this->mbufp + size > this->mbufe) {
        while (this->meta_size_alloc < this->hdr->meta_size + size) {
            this->meta_size_alloc <<= H5VL_LOGI_MERGED_REQ_SEL_MUL;
        }

        this->sel_buf -= (size_t) (this->meta_buf);
        this->meta_buf = (char *)realloc (this->meta_buf, this->meta_size_alloc);
        CHECK_PTR (this->meta_buf);
        this->sel_buf += (size_t) (this->meta_buf);
        this->hdr   = (H5VL_logi_meta_hdr *)(this->meta_buf);
        this->mbufp = this->meta_buf + this->hdr->meta_size;
        this->mbufe = this->meta_buf + this->meta_size_alloc;
    }
}

void H5VL_log_merged_wreq_t::append (H5VL_log_dset_t *dp,
                                     H5VL_log_req_data_block_t &db,
                                     H5VL_log_selections *sels) {
    H5VL_log_dset_info_t *dip = dp->fp->dsets_info[dp->id];  // Dataset info
    size_t msize;

    // Init the meta buffer if not yet inited
    if (this->meta_buf == NULL) { this->init (dp, sels->nsel); }

    // Reserve space in the metadata buffer
    if (this->hdr->flag & H5VL_FILEI_CONFIG_SEL_ENCODE) {
        msize = sels->nsel * 2 * sizeof (MPI_Offset);
    } else {
        msize = sels->nsel * 2 * sizeof (hsize_t) * dip->ndim;
    }
    this->reserve (msize);

    // Pack metadata
    if (this->hdr->flag & H5VL_FILEI_CONFIG_SEL_ENCODE) {
        sels->encode (this->mbufp, dip->dsteps);
    } else {
        sels->encode (this->mbufp);
    }
    this->mbufp += msize;

    // Record nsel
    nsel += sels->nsel;
    *((int *)(this->sel_buf)) = nsel;

    // Append data
    this->dbufs.push_back ({db.xbuf, db.ubuf, db.size});
    this->hdr->fsize += db.size;

    // Update metadata size
    // Header will be updated before flushing
    this->hdr->meta_size = this->mbufp - this->meta_buf;
}

inline void H5VL_log_read_idx_search (H5VL_log_file_t *fp,
                                      std::vector<H5VL_log_rreq_t *> &reqs,
                                      std::vector<H5VL_log_idx_search_ret_t> &intersecs) {
    int md, sec;  // Current metadata dataset and vurrent section

    // Flush metadata if dirty
    if (fp->metadirty) { H5VL_log_filei_metaflush (fp); }

    // If there is no metadata size limit, we load all the metadata at once
    if (fp->mbuf_size == LOG_VOL_BSIZE_UNLIMITED) {
        // Load metadata
        if (!(fp->idxvalid)) { H5VL_log_filei_metaupdate (fp); }

        // Search index
        for (auto r : reqs) { fp->idx->search (r, intersecs); }
    } else {
        md = sec = 0;
        while (md != -1) {  // Until we iterated all metadata datasets
                            // Load partial metadata
            H5VL_log_filei_metaupdate_part (fp, md, sec);
            // Search index
            for (auto &r : reqs) { fp->idx->search (r, intersecs); }
        }
    }
}

void H5VL_log_nb_perform_read (H5VL_log_file_t *fp,
                               std::vector<H5VL_log_rreq_t *> &reqs,
                               hid_t dxplid) {
    herr_t err = 0;
    int mpierr;
    int i;
    size_t esize;                             // Element size of the user buffer type
    MPI_Datatype ftype  = MPI_DATATYPE_NULL;  // File type for reading the raw data blocks
    MPI_Datatype mtype  = MPI_DATATYPE_NULL;  // Memory type for reading the raw data blocks
    MPI_Datatype zftype = MPI_DATATYPE_NULL;  // File type for packing the data
    MPI_Datatype zmtype = MPI_DATATYPE_NULL;  // Memory type for packing the data
    MPI_Datatype zetype = MPI_DATATYPE_NULL;  // Element type for packing the data
    bool compound_zetype =
        false;  // If element type for packing the data is compound (need to be freed)
    std::vector<H5VL_log_idx_search_ret_t>
        intersecs;  // Any intersection between selections in reqeusts and the metadata entries
    std::vector<H5VL_log_copy_ctx> overlaps;  // Any overlapping read regions
    std::map<MPI_Offset, char *> bufs;  // Temporary buffers for unfiltering filtered data blocks
    char *tbuf =
        NULL;  // Temporary buffers for packing data from unfiltered data block into request buffer
    size_t tbsize = 0;  // size of tbuf
    MPI_Status stat;
    H5VL_logi_err_finally finally (
        [&tbuf, &bufs, &mtype, &ftype, &zftype, &zmtype, &compound_zetype, &zetype] () -> void {
            free (tbuf);
            for (auto const &buf : bufs) { free (buf.second); }
            if (mtype != MPI_DATATYPE_NULL) MPI_Type_free (&mtype);
            if (ftype != MPI_DATATYPE_NULL) MPI_Type_free (&ftype);
            if (zftype != MPI_DATATYPE_NULL) MPI_Type_free (&zftype);
            if (zmtype != MPI_DATATYPE_NULL) MPI_Type_free (&zmtype);
            if (compound_zetype && zetype != MPI_DATATYPE_NULL) MPI_Type_free (&zetype);
        });

    H5VL_LOGI_PROFILING_TIMER_START;

    // Search index
    H5VL_log_read_idx_search (fp, reqs, intersecs);

    // Allocate zbuf for filtered data
    for (auto &block : intersecs) {
        if (block.info->filters.size () > 0) {
            if (bufs.find (block.foff) == bufs.end ()) {
                block.zbuf = (char *)malloc (std::max (block.xsize, block.fsize));
                CHECK_PTR (block.zbuf)
                bufs[block.foff] = block.zbuf;
                if (tbsize < (hsize_t)block.xsize) { tbsize = block.xsize; }
            } else {
                block.zbuf  = bufs[block.foff];
                block.fsize = 0;  // Don't need to read
            }
        }
    }

    // Read data
    if (intersecs.size () > 0) {
        H5VL_LOGI_PROFILING_TIMER_START;
        H5VL_log_dataset_readi_gen_rtypes (intersecs, &ftype, &mtype, overlaps);

        H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_DATASETI_READI_GEN_RTYPES);
        mpierr = MPI_Type_commit (&mtype);
        CHECK_MPIERR
        mpierr = MPI_Type_commit (&ftype);
        CHECK_MPIERR

        mpierr = MPI_File_set_view (fp->fh, 0, MPI_BYTE, ftype, "native", MPI_INFO_NULL);
        CHECK_MPIERR

        mpierr = MPI_File_read_at_all (fp->fh, 0, MPI_BOTTOM, 1, mtype, &stat);
        CHECK_MPIERR

        // Restore the file view
        mpierr = MPI_File_set_view (fp->fh, 0, MPI_BYTE, MPI_BYTE, "native", MPI_INFO_NULL);
        CHECK_MPIERR
    } else {
        // File view guaranteed to be contiguous
        // mpierr = MPI_File_set_view (fp->fh, 0, MPI_BYTE, MPI_BYTE, "native", MPI_INFO_NULL);
        // CHECK_MPIERR

        mpierr = MPI_File_read_at_all (fp->fh, 0, MPI_BOTTOM, 0, MPI_BYTE, &stat);
        CHECK_MPIERR
    }

    // In case there is overlapping read, copy the overlapping part
    for (auto &o : overlaps) { memcpy (o.dst, o.src, o.size); }

    // Unfilter all data
    if (tbsize > 0) { tbuf = (char *)malloc (tbsize); }
    for (auto &block : intersecs) {
        if (block.zbuf) {
            if (block.fsize > 0) {
                char *buf = NULL;
                int csize = 0;

                H5VL_logi_unfilter (block.info->filters, block.zbuf, block.fsize, (void **)&buf,
                                    &csize);

                memcpy (block.zbuf, buf, csize);
                free (buf);
            }

            // Pack from zbuf to xbuf
            zetype = H5VL_logi_get_mpi_type_by_size (block.info->esize);
            if (zetype == MPI_DATATYPE_NULL) {
                mpierr = MPI_Type_contiguous (block.info->esize, MPI_BYTE, &zetype);
                CHECK_MPIERR
                mpierr = MPI_Type_commit (&zetype);
                CHECK_MPIERR
                compound_zetype = true;
            } else {
                compound_zetype = false;
            }

            mpierr = H5VL_log_debug_MPI_Type_create_subarray (block.info->ndim, block.dsize,
                                                              block.count, block.dstart,
                                                              MPI_ORDER_C, zetype, &zftype);
            CHECK_MPIERR
            mpierr = H5VL_log_debug_MPI_Type_create_subarray (block.info->ndim, block.msize,
                                                              block.count, block.mstart,
                                                              MPI_ORDER_C, zetype, &zmtype);

            CHECK_MPIERR
            mpierr = MPI_Type_commit (&zftype);
            CHECK_MPIERR
            mpierr = MPI_Type_commit (&zmtype);
            CHECK_MPIERR

            if (compound_zetype) {
                mpierr = MPI_Type_free (&zetype);
                CHECK_MPIERR
            }

            i = 0;
            MPI_Pack (block.zbuf + block.doff, 1, zftype, tbuf, 1, &i, fp->comm);

            i = 0;
            MPI_Unpack (tbuf, 1, &i, block.xbuf, 1, zmtype, fp->comm);

            MPI_Type_free (&zftype);
            MPI_Type_free (&zmtype);
        }
    }

    // Post processing
    for (auto &r : reqs) {
        // Type convertion
        if (r->dtype != r->mtype) {
            // void *bg = NULL;

            esize = H5Tget_size (r->mtype);
            CHECK_ID (esize)

            // if (H5Tget_class (r->mtype) == H5T_COMPOUND) bg = malloc (r->rsize * esize);
            err = H5Tconvert (r->dtype, r->mtype, r->rsize, r->xbuf, NULL, dxplid);
            CHECK_ERR
            // free (bg);

            H5Tclose (r->dtype);
            H5Tclose (r->mtype);
        } else {
            esize = r->esize;
        }

        // Packing if memory space is not contiguous
        if (r->xbuf != r->ubuf) {
            if (r->ptype != MPI_DATATYPE_NULL) {
                i = 0;
                MPI_Unpack (r->xbuf, r->rsize * r->esize, &i, r->ubuf, 1, r->ptype, fp->comm);
                MPI_Type_free (&(r->ptype));
            } else {
                memcpy (r->ubuf, r->xbuf, r->rsize * esize);
            }

            H5VL_log_filei_bfree (fp, r->xbuf);
        }
    }

    H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_NB_PERFORM_READ);
}

void H5VL_log_nb_flush_read_reqs (void *file, std::vector<H5VL_log_rreq_t *> &reqs, hid_t dxplid) {
    herr_t err = 0;
    int mpierr;
    int i;
    int group_id;  // Original group ID (subfile to access)
    H5FD_mpio_xfer_t xfer_mode;
    H5VL_loc_params_t loc;
    H5VL_log_file_t *fp = (H5VL_log_file_t *)file;

    // Collective ?
    err = H5Pget_dxpl_mpio (dxplid, &xfer_mode);
    CHECK_ERR
    if ((xfer_mode == H5FD_MPIO_COLLECTIVE) || fp->np == 1) {
        H5VL_log_nb_flush_write_reqs (fp, dxplid);
    }

    H5VL_LOGI_PROFILING_TIMER_START;

    // Iterate through all subfiles
    if ((!(fp->config & H5VL_FILEI_CONFIG_SUBFILING)) || (fp->config & H5VL_FILEI_CONFIG_SINGLE_SUBFILE_READ)) {
        H5VL_log_nb_perform_read (fp, reqs, dxplid);
    } else {
        group_id = fp->group_id;  // Backup group ID
        // Process our own subfile last so wew don't need to reopen it
        for (i = 1; i <= fp->ngroup; i++) {
            H5VL_LOGI_PROFILING_TIMER_START;
            // Close the log group
            err = H5VLgroup_close (fp->lgp, fp->uvlid, fp->dxplid, NULL);
            CHECK_ERR
            // Close previous subfile with MPI
            mpierr = MPI_File_close (&(fp->fh));
            CHECK_MPIERR
            // Close previous subfile
            err = H5VLfile_close (fp->sfp, fp->uvlid, H5P_DATASET_XFER_DEFAULT, NULL);
            CHECK_ERR
            // Erase the index table of previous subfile
            fp->idx->clear ();
            fp->idxvalid = false;

            // Open the current subfile
            fp->group_id = (group_id + i) % fp->ngroup;
            H5VL_log_filei_open_subfile (fp, fp->flag, fp->ufaplid, fp->dxplid);

            // Open the LOG group
            loc.obj_type = H5I_FILE;
            loc.type     = H5VL_OBJECT_BY_SELF;
            fp->lgp      = H5VLgroup_open (fp->sfp, &loc, fp->uvlid, H5VL_LOG_FILEI_GROUP_LOG,
                                      H5P_GROUP_ACCESS_DEFAULT, fp->dxplid, NULL);
            CHECK_PTR (fp->lgp)
            // Open the file with MPI
            mpierr = MPI_File_open (fp->group_comm, fp->subname.c_str (), MPI_MODE_RDWR, fp->info,
                                    &(fp->fh));
            CHECK_MPIERR

            H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_NB_FLUSH_READ_REQS_SWITCH_SUBFILE);
        }
    }

    // Clear the request queue
    for (auto rp : reqs) { delete rp; }
    reqs.clear ();

    H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_NB_FLUSH_READ_REQS);
}

void H5VL_log_nb_flush_write_reqs (void *file, hid_t dxplid) {
    herr_t err = 0;
    int mpierr;
    int i, j;
    int cnt;                                 // # blocks in mtype
    int *mlens         = NULL;               // Lens of mtype for writing data
    MPI_Aint *moffs    = NULL;               // Offs of mtype for writing data
    MPI_Datatype mtype = MPI_DATATYPE_NULL;  // Mtype for writing data
    MPI_Status stat;
    MPI_Offset fsize_local;  // Total data size on this process
    MPI_Offset fsize_all;    // Total data size across all process
    MPI_Offset fsize_group;  // Total data size across all process sharing the same file
    MPI_Offset foff_all;     // File offsset of the data block of current process globally
    MPI_Offset foff_group;   // File offsset of the data block in the current group
    void *ldp;               // Handle to the log dataset
    // void *vldp;				 // Handle to the log dataset
    hid_t ldsid  = -1;  // Space of the log dataset
    hid_t dcplid = -1;  // log dataset creation property ID
    // hid_t vldsid  = -1;			  // Space of the virtual log dataset in the main
    // file hid_t vdcplid = -1;	 // Virtual log dataset creation property list
    hsize_t start;  // Size for dataspace selection
    // const hsize_t one = 1;		  // Constant 1 for dataspace selection
    haddr_t doff;  // File offset of the log dataset
    H5VL_loc_params_t loc;
    char dname[16];  // Name of the log dataset
    H5VL_log_file_t *fp = (H5VL_log_file_t *)file;
    H5VL_logi_err_finally finally ([&mtype, &ldsid, &dcplid, &mlens, &moffs] () -> void {
        if (mtype != MPI_DATATYPE_NULL) MPI_Type_free (&mtype);
        H5VL_log_Sclose (ldsid);
        // H5VL_log_Sclose (vldsid);
        H5VL_log_Pclose (dcplid);

        H5VL_log_free (mlens);
        H5VL_log_free (moffs);
    });

    H5VL_LOGI_PROFILING_TIMER_START;
    H5VL_LOGI_PROFILING_TIMER_START;

    // Flush all merged requests
    for (i = 0; i < (int)(fp->mreqs.size ()); i++) {
        if (fp->mreqs[i] && (fp->mreqs[i]->nsel > 0)) {
            fp->wreqs.push_back (fp->mreqs[i]);
            // Update total metadata size in wreqs
            fp->mdsize += fp->mreqs[i]->hdr->meta_size;
            fp->mreqs[i] = NULL;
        }
    }

    // Calculate number of blocks in mtype
    cnt = 0;
    for (i = fp->nflushed; i < (int)(fp->wreqs.size ()); i++) {
        cnt += fp->wreqs[i]->dbufs.size ();
    }

    // Construct memory type
    fsize_local = 0;
    if (cnt) {
        mlens = (int *)malloc (sizeof (int) * cnt);
        moffs = (MPI_Aint *)malloc (sizeof (MPI_Aint) * cnt);
        j     = 0;
        // Gather offset and lens requests
        for (i = fp->nflushed; i < (int)(fp->wreqs.size ()); i++) {
            for (auto &d : fp->wreqs[i]->dbufs) {
                moffs[j]   = (MPI_Aint)d.xbuf;
                mlens[j++] = d.size;
            }
            fp->wreqs[i]->hdr->foff = fsize_local;
            fsize_local += fp->wreqs[i]->hdr->fsize;
        }

        mpierr = MPI_Type_create_hindexed (cnt, mlens, moffs, MPI_BYTE, &mtype);
        CHECK_MPIERR
        mpierr = MPI_Type_commit (&mtype);
        CHECK_MPIERR
    } else {
        mtype = MPI_DATATYPE_NULL;
    }
#ifdef LOGVOL_PROFILING
    H5VL_log_profile_add_time (fp, TIMER_H5VL_LOG_NB_FLUSH_WRITE_REQS_SIZE,
                               (double)(fsize_local) / 1048576);
#endif

    H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_NB_FLUSH_WRITE_REQS_INIT);
    H5VL_LOGI_PROFILING_TIMER_START;

    loc.type     = H5VL_OBJECT_BY_SELF;
    loc.obj_type = H5I_GROUP;

    // Get file offset and total size globally
    mpierr = MPI_Allreduce (&fsize_local, &fsize_all, 1, MPI_LONG_LONG, MPI_SUM, fp->comm);
    CHECK_MPIERR
    // NOTE: Some MPI implementation do not produce output for rank 0, foff must be initialized to 0
    foff_all = 0;
    mpierr   = MPI_Exscan (&fsize_local, &foff_all, 1, MPI_LONG_LONG, MPI_SUM, fp->comm);
    CHECK_MPIERR

    if (fp->config & H5VL_FILEI_CONFIG_SUBFILING) {
        // Get file offset and total size in group
        mpierr =
            MPI_Allreduce (&fsize_local, &fsize_group, 1, MPI_LONG_LONG, MPI_SUM, fp->group_comm);
        CHECK_MPIERR
        // NOTE: Some MPI implementation do not produce output for rank 0, foff must be initialized
        // to 0
        foff_group = 0;
        mpierr = MPI_Exscan (&fsize_local, &foff_group, 1, MPI_LONG_LONG, MPI_SUM, fp->group_comm);
        CHECK_MPIERR
    } else {
        fsize_group = fsize_all;
        foff_group  = foff_all;
    }

    H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_NB_FLUSH_WRITE_REQS_SYNC);

    // Write out the data
    if (fsize_all) {
        sprintf (dname, "%s_%d", H5VL_LOG_FILEI_DSET_DATA, fp->nldset);

        // Create log dataset
        if (fsize_group) {
            H5VL_LOGI_PROFILING_TIMER_START;
            // Create the group data dataset
            start = (hsize_t)fsize_group;
            ldsid = H5Screate_simple (1, &start, &start);
            CHECK_ID (ldsid)

            // Allocate file space at creation time
            dcplid = H5Pcreate (H5P_DATASET_CREATE);
            CHECK_ID (dcplid)
            err = H5Pset_alloc_time (dcplid, H5D_ALLOC_TIME_EARLY);

            // Create dataset with under VOL
            H5VL_LOGI_PROFILING_TIMER_START;
            ldp = H5VLdataset_create (fp->lgp, &loc, fp->uvlid, dname, H5P_LINK_CREATE_DEFAULT,
                                      H5T_STD_B8LE, ldsid, dcplid, H5P_DATASET_ACCESS_DEFAULT,
                                      dxplid, NULL);
            H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VLDATASET_CREATE);
            CHECK_PTR (ldp);

            H5VL_LOGI_PROFILING_TIMER_START;
            // Get dataset file offset
            H5VL_logi_dataset_get_foff (fp, ldp, fp->uvlid, dxplid, &doff);
            // If not allocated, flush the file and reopen the dataset
            if (doff == HADDR_UNDEF) {
                H5VL_file_specific_args_t arg;

                // Close the dataset
                err = H5VLdataset_close (ldp, fp->uvlid, dxplid, NULL);
                CHECK_ERR

                // Flush the file
                arg.op_type             = H5VL_FILE_FLUSH;
                arg.args.flush.scope    = H5F_SCOPE_GLOBAL;
                arg.args.flush.obj_type = H5I_FILE;
                err                     = H5VLfile_specific (fp->uo, fp->uvlid, &arg, dxplid, NULL);
                CHECK_ERR

                // Reopen the dataset
                ldp = H5VLdataset_open (fp->lgp, &loc, fp->uvlid, dname, H5P_DATASET_ACCESS_DEFAULT,
                                        dxplid, NULL);
                CHECK_PTR (ldp);

                // Get dataset file offset
                H5VL_logi_dataset_get_foff (fp, ldp, fp->uvlid, dxplid, &doff);

                // Still don't work, discard the data
                if (doff == HADDR_UNDEF) {
                    printf ("WARNING: Log dataset creation failed, data is not recorded\n");
                    fflush (stdout);

                    if (mtype != MPI_DATATYPE_NULL) MPI_Type_free (&mtype);
                    mtype = MPI_DATATYPE_NULL;
                    doff  = 0;
                }
            }
            H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VLDATASET_OPTIONAL);

            // Close the dataset
            err = H5VLdataset_close (ldp, fp->uvlid, dxplid, NULL);
            CHECK_ERR

            H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_NB_FLUSH_WRITE_REQS_CREATE);

            H5VL_LOGI_PROFILING_TIMER_START;
            // Write the data
            if (mtype == MPI_DATATYPE_NULL) {
                mpierr = MPI_File_write_at_all (fp->fh, foff_group + doff, MPI_BOTTOM, 0, MPI_INT,
                                                &stat);
                CHECK_MPIERR
            } else {
                mpierr =
                    MPI_File_write_at_all (fp->fh, foff_group + doff, MPI_BOTTOM, 1, mtype, &stat);
                CHECK_MPIERR
            }
            H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_NB_FLUSH_WRITE_REQS_WR);

            // Update metadata in requests
            for (i = fp->nflushed; i < (int)(fp->wreqs.size ()); i++) {
                fp->wreqs[i]->hdr->foff += foff_group + doff;
                for (auto &d : fp->wreqs[i]->dbufs) {
                    if (d.ubuf != d.xbuf) { H5VL_log_filei_bfree (fp, (void *)(d.xbuf)); }
                }
            }
            fp->nflushed = fp->wreqs.size ();

            // Increase number of log dataset
            (fp->nldset)++;
        }

        // Create virtaul log dataset in the main file
        // Virtual dataset property must be consistant across all processes
        // Expensive communication and HDF5 operations
        /*
        if (fp->config & H5VL_FILEI_CONFIG_SUBFILING) {
                H5VL_LOGI_PROFILING_TIMER_START;

                start  = (hsize_t)fsize_all;
                vldsid = H5Screate_simple (1, &start, &start);
                CHECK_ID (ldsid)

                vdcplid = H5Pcreate (H5P_DATASET_CREATE);
                CHECK_ID (vdcplid)
                if (fsize_group && (fp->group_rank == 0)) {
                        start = foff_all;
                        count = fsize_group;
                        err	  = H5Sselect_hyperslab (vldsid, H5S_SELECT_SET, &start, NULL, &one,
        &count); CHECK_ERR err = H5Sselect_all (ldsid); CHECK_ERR err = H5Pset_virtual (vdcplid,
        vldsid, fp->subname.c_str (), dname, ldsid); CHECK_ERR
                        // TODO: handle error and don't hang other processes
                }

                loc.obj_type = H5I_GROUP;
                H5VL_LOGI_PROFILING_TIMER_START;
                vldp = H5VLdataset_create (fp->lgp, &loc, fp->uvlid, dname, H5P_LINK_CREATE_DEFAULT,
                                                                   H5T_STD_B8LE, vldsid, vdcplid,
        H5P_DATASET_ACCESS_DEFAULT, dxplid, NULL); CHECK_PTR (vldp); H5VL_LOGI_PROFILING_TIMER_STOP
        (fp, TIMER_H5VLDATASET_CREATE);

                err = H5VLdataset_close (vldp, fp->uvlid, dxplid, NULL);
                CHECK_ERR  // Close the virtual dataset

                H5VL_LOGI_PROFILING_TIMER_STOP (fp,
        TIMER_H5VL_LOG_NB_FLUSH_WRITE_REQS_CREATE_VIRTUAL);
        }
        */

        // Mark the metadata flag to dirty (unflushed metadata)
        if (fsize_all) { fp->metadirty = true; }
    }

    H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_NB_FLUSH_WRITE_REQS);
}

inline void H5VL_log_nb_flush_posix_write (int fd, char *buf, size_t count) {
    ssize_t wsize;

    while (count > 0) {
        wsize = write (fd, buf, count);
        if (wsize < 0) { ERR_OUT ("write fail") }
        buf += wsize;
        count -= wsize;
    }
}

// Deprecated
void H5VL_log_nb_ost_write (void *file, off_t doff, off_t off, int cnt, int *mlens, off_t *moffs) {
    int i;
    char *bs = NULL, *be, *bp;
    char *mbuf;
    off_t ost_base;  // File offset of the first byte on the target ost
    off_t cur_off;   // Offset of current stripe
    off_t seek_off;
    size_t skip_size;  // First chunk can be partial
    size_t mlen;
    size_t stride;
    H5VL_log_file_t *fp = (H5VL_log_file_t *)file;
    H5VL_logi_err_finally finally ([&bs] () -> void { H5VL_log_free (bs); });

    H5VL_LOGI_PROFILING_TIMER_START;

    // Buffer of a stripe
    bs = (char *)malloc (fp->ssize);
    be = bs + fp->ssize;

    stride   = fp->ssize * fp->scount;  // How many bytes to the next stripe on the same ost
    ost_base = doff + fp->ssize * fp->target_ost;  // Skip fp->target_ost stripes to get to the
                                                   // first byte of the target ost

    cur_off   = ost_base + (off / fp->ssize) * stride;
    skip_size = off % fp->ssize;
    seek_off  = lseek (fp->fd, cur_off + skip_size, SEEK_SET);
    if (seek_off < 0) { ERR_OUT ("lseek fail"); }
    bp = bs + skip_size;

    for (i = 0; i < cnt; i++) {
        mlen = mlens[i];
        mbuf = (char *)(moffs[i]);
        while (mlen > 0) {
            if ((size_t) (be - bp) <= mlen) {
                memcpy (bp, mbuf, be - bp);

                // flush
                if (skip_size) {
                    H5VL_log_nb_flush_posix_write (fp->fd, bs + skip_size, fp->ssize - skip_size);
                    skip_size = 0;
                } else {
                    H5VL_log_nb_flush_posix_write (fp->fd, bs, fp->ssize);
                }

                // Move to next stride
                seek_off = lseek (fp->fd, stride, SEEK_CUR);
                if (seek_off < 0) { ERR_OUT ("lseek fail"); }

                // Bytes written
                mlen -= be - bp;
                mbuf += be - bp;

                // Reset chunk buffer
                bp = bs;
            } else {
                memcpy (bp, mbuf, mlen);
                bp += mlen;
                mlen = 0;
            }
        }
    }
    // Last stripe
    if (bp > bs) { H5VL_log_nb_flush_posix_write (fp->fd, bs + skip_size, bp - bs - skip_size); }

    H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_NB_WRITE_REQS_ALIGNED);
}

// Deprecated
void H5VL_log_nb_flush_write_reqs_align (void *file, hid_t dxplid) {
    herr_t err = 0;
    int mpierr;
    int i, j;
    int cnt;
    int *mlens         = NULL;
    off_t *moffs       = NULL;
    MPI_Datatype mtype = MPI_DATATYPE_NULL;
    MPI_Status stat;
    MPI_Offset *fsize_local, *fsize_all, foff, fbase;
    void *ldp;
    hid_t ldsid = -1;
    hsize_t dsize, remain;
    haddr_t doff;
    H5VL_loc_params_t loc;
    char dname[16];
    H5VL_log_file_t *fp = (H5VL_log_file_t *)file;
    H5VL_logi_err_finally finally ([&] () -> void {
        if (mtype != MPI_DATATYPE_NULL) MPI_Type_free (&mtype);
        H5VL_log_Sclose (ldsid);

        H5VL_log_free (mlens);
        H5VL_log_free (moffs);
    });

    H5VL_LOGI_PROFILING_TIMER_START;

    cnt = fp->wreqs.size () - fp->nflushed;

    fsize_local = (MPI_Offset *)malloc (sizeof (MPI_Offset) * fp->scount * 2);
    fsize_all   = fsize_local + fp->scount;

    memset (fsize_local, 0, sizeof (MPI_Offset) * fp->scount);

    // Construct memory type
    mlens = (int *)malloc (sizeof (int) * cnt);
    moffs = (off_t *)malloc (sizeof (off_t) * cnt);
    j     = 0;
    for (i = fp->nflushed; i < (int)(fp->wreqs.size ()); i++) {
        for (auto &d : fp->wreqs[i]->dbufs) {
            moffs[j]   = (MPI_Aint)d.xbuf;
            mlens[j++] = d.size;
        }
        fp->wreqs[i]->hdr->foff = fsize_local[fp->target_ost];
        fsize_local[fp->target_ost] += fp->wreqs[i]->hdr->fsize;
    }

#ifdef LOGVOL_PROFILING
    H5VL_log_profile_add_time (fp, TIMER_H5VL_LOG_NB_FLUSH_WRITE_REQS_SIZE,
                               (double)(fsize_local[fp->target_ost]) / 1048576);
#endif

    // Get file offset and total size per ost
    mpierr = MPI_Allreduce (fsize_local, fsize_all, fp->scount, MPI_LONG_LONG, MPI_SUM, fp->comm);
    CHECK_MPIERR

    // The max size among OSTs
    dsize = 0;
    for (i = 0; i < fp->scount; i++) {
        if ((hsize_t) (fsize_all[i]) > dsize) dsize = fsize_all[i];
    }

    // Create log dataset
    if (dsize) {
        // Align to the next stripe boundary
        remain = dsize % fp->ssize;
        if (remain) { dsize += fp->ssize - remain; }
        dsize *= fp->scount;  // Total size on all osts
        dsize += fp->ssize;   // Add 1 stripe for alignment alignment

        ldsid = H5Screate_simple (1, &dsize, &dsize);
        CHECK_ID (ldsid)
        sprintf (dname, "%s_%d", H5VL_LOG_FILEI_DSET_DATA, fp->nldset);
        loc.obj_type = H5I_GROUP;
        loc.type     = H5VL_OBJECT_BY_SELF;
        ldp          = H5VLdataset_create (fp->lgp, &loc, fp->uvlid, dname, H5P_LINK_CREATE_DEFAULT,
                                  H5T_STD_B8LE, ldsid, H5P_DATASET_CREATE_DEFAULT,
                                  H5P_DATASET_ACCESS_DEFAULT, dxplid, NULL);
        CHECK_PTR (ldp);

        H5VL_LOGI_PROFILING_TIMER_START;
        // Get dataset file offset
        H5VL_logi_dataset_get_foff (fp, ldp, fp->uvlid, dxplid, &doff);
        if (remain) {  // Align to the next stripe
            doff += fp->ssize - remain;
        }
        H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VLDATASET_OPTIONAL);

        err = H5VLdataset_close (ldp, fp->uvlid, dxplid, NULL);
        CHECK_ERR  // Close the dataset

        // Receive offset form prev node
        fbase = 0;
        if (fp->group_rank == 0) {
            if (fp->prev_rank >= 0) {
                mpierr = MPI_Recv (&fbase, 1, MPI_LONG_LONG, fp->prev_rank, 0, fp->comm, &stat);
                CHECK_MPIERR
            }
        }

        // Offset for each process
        foff = fbase;
        fsize_local[fp->target_ost] +=
            fbase;  // Embed node offset in process offset to save a bcast
        MPI_Exscan (fsize_local + fp->target_ost, &foff, 1, MPI_LONG_LONG, MPI_SUM, fp->group_comm);
        fsize_local[fp->target_ost] -= fbase;

        // Write the data
        H5VL_log_nb_ost_write (fp, doff, foff, cnt, mlens, moffs);

        // Notify next node
        MPI_Reduce (fsize_local + fp->target_ost, &fbase, 1, MPI_LONG_LONG, MPI_SUM, 0,
                    fp->group_comm);
        if (fp->group_rank == 0) {
            if (fp->next_rank >= 0) {
                mpierr = MPI_Send (&fbase, 1, MPI_LONG_LONG, fp->next_rank, 0, fp->comm);
                CHECK_MPIERR
            }
        }

        // Update metadata
        for (i = fp->nflushed; i < (int)(fp->wreqs.size ()); i++) {
            fp->wreqs[i]->hdr->foff += foff;
            for (auto &db : fp->wreqs[i]->dbufs) {
                if (db.xbuf != db.ubuf) { H5VL_log_filei_bfree (fp, (void *)(db.xbuf)); }
            }
        }
        (fp->nldset)++;
        fp->nflushed = fp->wreqs.size ();

        if (fsize_all) { fp->metadirty = true; }

        // err = H5VL_log_filei_pool_free (&(fp->data_buf));
        // CHECK_ERR
    }

    H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_NB_FLUSH_WRITE_REQS);
}

H5VL_log_rreq_t::H5VL_log_rreq_t () {}
H5VL_log_rreq_t::~H5VL_log_rreq_t () {
    if (sels) {
        delete sels;
        sels = NULL;
    }
}
