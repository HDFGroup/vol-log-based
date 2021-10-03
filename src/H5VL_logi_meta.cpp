#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
//
#include <cstring>
#include <functional>
#include <vector>
//
#include <hdf5.h>
//
#include "H5VL_log_dataset.hpp"
#include "H5VL_logi_dataspace.hpp"
#include "H5VL_logi_err.hpp"
#include "H5VL_logi_meta.hpp"
#include "H5VL_logi_zip.hpp"

herr_t H5VL_logi_metaentry_ref_decode (H5VL_log_dset_info_t &dset,
									   void *ent,
									   H5VL_logi_metablock_t &block,
									   std::vector<H5VL_logi_metasel_t> &sels) {
	herr_t err = 0;
	int i;
	MPI_Offset *bp;			   // Next 8 byte selection to process
	char *bufp = (char *)ent;  // Next byte to process in ent
	size_t bsize;			   // Size of a selection block

	// Get the header
	block.hdr = *((H5VL_logi_meta_hdr *)bufp);
	bufp += sizeof (H5VL_logi_meta_hdr);
	// Data location and size in the file
	block.foff = *((MPI_Offset *)bufp);
	bufp += sizeof (MPI_Offset);
	block.fsize = *((MPI_Offset *)bufp);
	bufp += sizeof (MPI_Offset);

	// Get referenced selections
	block.sels = sels;

	// Calculate the unfiltered size of the data block
	block.dsize = 0;
	for (auto &s : block.sels) {
		bsize = dset.esize;
		for (i = 0; i < dset.ndim; i++) { block.dsize *= s.count[i]; }
		block.dsize += bsize;
	}

	// Calculate the unfiltered size of the data block
	// Data size = data offset of the last block + size of the alst block
	block.dsize = dset.esize;
	for (i = 0; i < dset.ndim; i++) { block.dsize *= block.sels[sels.size() - 1].count[i]; }
	block.dsize += block.sels[sels.size() - 1].doff;

err_out:;
	return err;
}

herr_t H5VL_logi_metaentry_decode (H5VL_log_dset_info_t &dset,
								   void *ent,
								   H5VL_logi_metablock_t &block) {
	herr_t err = 0;
	int i, j;
	int nsel;						  // Nunmber of selections in ent
	MPI_Offset dsteps[H5S_MAX_RANK];  // corrdinate to offset encoding info in ent
	char *zbuf	   = NULL;			  // Buffer for decompressing metadata
	bool zbufalloc = false;			  // Should we free zbuf
	char *bufp	   = (char *)ent;	  // Next byte to process in ent
	MPI_Offset *bp;					  // Next 8 byte selection to process

	// Get the header
	block.hdr = *((H5VL_logi_meta_hdr *)bufp);
	bufp += sizeof (H5VL_logi_meta_hdr);
	// Data location and size in the file
	block.foff = *((MPI_Offset *)bufp);
	bufp += sizeof (MPI_Offset);
	block.fsize = *((MPI_Offset *)bufp);
	bufp += sizeof (MPI_Offset);

	// If there is more than 1 selection
	if (block.hdr.flag & H5VL_LOGI_META_FLAG_MUL_SEL) {
		// Get number of selection (right after the header)
		nsel = *((int *)bufp);
		bufp += sizeof (int);

		// If the rest of the entry is comrpessed, decomrpess it
		if (block.hdr.flag & H5VL_LOGI_META_FLAG_SEL_DEFLATE) {
#ifdef ENABLE_ZLIB
			int inlen;		   // Size of comrpessed metadata
			int clen;		   // Size of decompressed metadata
			MPI_Offset bsize;  // Size of zbuf
			MPI_Offset esize;  // Size of a selection block

			// Calculate buffer size;
			esize = 0;
			bsize = 0;
			// Count selections
			if (block.hdr.flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {
				// Encoded start and count
				esize += nsel * sizeof (MPI_Offset) * 2;
				bsize += sizeof (MPI_Offset) * (dset.ndim - 1);
			} else {
				// Start and count as coordinate
				esize += nsel * sizeof (MPI_Offset) * 2 * dset.ndim;
			}
			/*
			if (block.hdr.flag & H5VL_LOGI_META_FLAG_MUL_SELX) {  // Physical location
				esize += sizeof (MPI_Offset) * 2;
			} else {
				bsize += sizeof (MPI_Offset) * 2;
			}
			*/
			bsize += esize * nsel;

			// Allocate decompression buffer
			zbuf	  = (char *)malloc (bsize);
			zbufalloc = true;

			// Decompress the metadata
			inlen = block.hdr.meta_size - sizeof (H5VL_logi_meta_hdr) - sizeof (int) -
					sizeof (MPI_Offset) * 2;
			clen = bsize;
			err	 = H5VL_log_zip_decompress (bufp, inlen, zbuf, &clen);
			CHECK_ERR
#else
			RET_ERR ("Comrpessed Metadata Support Not Enabled")
#endif
		} else {
			zbuf	  = bufp;
			zbufalloc = false;
		}
	} else {
		// Entries with single selection will never be comrpessed
		nsel	  = 1;
		zbuf	  = bufp;
		zbufalloc = false;
	}

	bp = (MPI_Offset *)zbuf;
	// Retrieve the selection encoding info
	if (block.hdr.flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {
		memcpy (dsteps, bp, sizeof (MPI_Offset) * (dset.ndim - 1));
		dsteps[dset.ndim - 1] = 1;
		bp += dset.ndim - 1;
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
				H5VL_logi_sel_decode (dset.ndim, dsteps, *((MPI_Offset *)bp), block.sels[0].start);
				bp++;

				// Decode count
				H5VL_logi_sel_decode (dset.ndim, dsteps, *((MPI_Offset *)bp), block.sels[0].count);
				bp++;
			} else {
				memcpy (block.sels[0].start, bp, sizeof (MPI_Offset) * dset.ndim);
				bp += dset.ndim;
				memcpy (block.sels[0].count, bp, sizeof (MPI_Offset) * dset.ndim);
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

	// Retrieve starts of selections
	for (i = 0; i < nsel; i++) {
		if (block.hdr.flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {
			H5VL_logi_sel_decode (dset.ndim, dsteps, *((MPI_Offset *)bp), block.sels[i].start);
			bp++;
		} else {
			memcpy (block.sels[i].start, bp, sizeof (MPI_Offset) * dset.ndim);
			bp += dset.ndim;
		}
	}
	// Retrieve counts of selections
	for (i = 0; i < nsel; i++) {
		if (block.hdr.flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {
			H5VL_logi_sel_decode (dset.ndim, dsteps, *((MPI_Offset *)bp), block.sels[i].count);
			bp++;
		} else {
			memcpy (block.sels[i].count, bp, sizeof (MPI_Offset) * dset.ndim);
			bp += dset.ndim;
		}
		// Calculate the offset of data of the selection within the unfiltered data block
		if (i > 0) {
			block.sels[i].doff = dset.esize;
			for (j = 0; j < dset.ndim; j++) { block.sels[i].doff *= block.sels[i - 1].count[j]; }
			block.sels[i].doff += block.sels[i - 1].doff;
		} else {
			block.sels[i].doff = 0;
		}
	}

	// Calculate the unfiltered size of the data block
	// Data size = data offset of the last block + size of the alst block
	block.dsize = dset.esize;
	for (j = 0; j < dset.ndim; j++) { block.dsize *= block.sels[i - 1].count[j]; }
	block.dsize += block.sels[i - 1].doff;

err_out:;
	if (block.hdr.flag & H5VL_LOGI_META_FLAG_SEL_DEFLATE) { free (zbuf); }

	return err;
}
