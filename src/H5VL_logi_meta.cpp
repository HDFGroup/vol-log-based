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
#include "H5VL_logi_err.hpp"
#include "H5VL_logi_meta.hpp"
#include "H5VL_logi_zip.hpp"


herr_t H5VL_logi_metaentry_decode (H5VL_log_dset_info_t &dset, void *ent, H5VL_logi_idx_t &idx) {
	herr_t err = 0;
	int i, j;
	int nsel;
	MPI_Offset *bp;
	MPI_Offset dsteps[H5S_MAX_RANK];
	char *zbuf	   = NULL;
	bool zbufalloc = false;
	H5VL_logi_metablock_t block;
	char *bufp = (char *)ent;

	block.hdr = *((H5VL_logi_meta_hdr *)bufp);
	bufp += sizeof (H5VL_logi_meta_hdr);

	// Extract the metadata
	if (block.hdr.flag & H5VL_LOGI_META_FLAG_MUL_SEL) {
		nsel = *((int *)bufp);
		bufp += sizeof (int);

		if (block.hdr.flag & H5VL_LOGI_META_FLAG_SEL_DEFLATE) {
#ifdef ENABLE_ZLIB
			int inlen, clen;
			MPI_Offset bsize;  // Size of all zbuf
			MPI_Offset esize;  // Size of a block
							   // Calculate buffer size;
			esize = 0;
			bsize = 0;
			if (block.hdr.flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {	// Logical location
				esize += nsel * sizeof (MPI_Offset) * 2;
				bsize += sizeof (MPI_Offset) * (dset.ndim - 1);
			} else {
				esize += nsel * sizeof (MPI_Offset) * 2 * dset.ndim;
			}
			if (block.hdr.flag & H5VL_LOGI_META_FLAG_MUL_SELX) {  // Physical location
				esize += sizeof (MPI_Offset) * 2;
			} else {
				bsize += sizeof (MPI_Offset) * 2;
			}
			bsize += esize * nsel;

			zbuf	  = (char *)malloc (bsize);
			zbufalloc = true;

			inlen = block.hdr.meta_size - sizeof (H5VL_logi_meta_hdr) - sizeof (int);
			clen  = bsize;
			err	  = H5VL_log_zip_compress (bufp, inlen, zbuf, &clen);
			CHECK_ERR
#else
			RET_ERR ("Comrpessed Metadata Support Not Enabled")
#endif
		} else {
			zbuf	  = bufp;
			zbufalloc = false;
		}
	} else {
		nsel	  = 1;
		zbuf	  = bufp;
		zbufalloc = false;
	}

	// Convert to req
	bp = (MPI_Offset *)zbuf;
	if (block.hdr.flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {
		memcpy (dsteps, bp, sizeof (MPI_Offset) * (dset.ndim - 1));
		dsteps[dset.ndim - 1] = 1;
		bp += dset.ndim - 1;
	}

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
			block.sels[0].boff = 0;
			block.dsize		   = dset.esize;
			for (j = 0; j < dset.ndim; j++) { block.dsize *= block.sels[0].count[j]; }
			idx.insert (block);
		}
	} else {
		block.foff = *((MPI_Offset *)bp);
		bp++;
		block.fsize = *((MPI_Offset *)bp);
		bp++;

		block.sels.resize (nsel);

		// Decode start
		for (i = 0; i < nsel; i++) {
			if (block.hdr.flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {
				H5VL_logi_sel_decode (dset.ndim, dsteps, *((MPI_Offset *)bp), block.sels[i].start);
				bp++;
			} else {
				memcpy (block.sels[i].start, bp, sizeof (MPI_Offset) * dset.ndim);
				bp += dset.ndim;
			}
		}
		// Decode count
		for (i = 0; i < nsel; i++) {
			if (block.hdr.flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {
				H5VL_logi_sel_decode (dset.ndim, dsteps, *((MPI_Offset *)bp), block.sels[i].count);
				bp++;
			} else {
				memcpy (block.sels[i].count, bp, sizeof (MPI_Offset) * dset.ndim);
				bp += dset.ndim;
			}
			if (i > 0) {
				block.sels[i].boff = dset.esize;
				for (j = 0; j < dset.ndim; j++) {
					block.sels[i].boff *= block.sels[i - 1].count[j];
				}
				block.sels[i].boff += block.sels[i - 1].boff;
			} else {
				block.sels[i].boff = 0;
			}
		}

		block.dsize = dset.esize;
		for (j = 0; j < dset.ndim; j++) { block.dsize *= block.sels[i - 1].count[j]; }
		block.dsize += block.sels[i - 1].boff;
		idx.insert (block);
	}
err_out:;
	if (block.hdr.flag & H5VL_LOGI_META_FLAG_SEL_DEFLATE) { free (zbuf); }

	return err;
}

herr_t H5VL_logi_metaentry_encode (H5VL_log_dset_info_t &dset,
								   H5VL_logi_meta_hdr &hdr,
								   int nsel,
								   hsize_t **starts,
								   hsize_t **counts,
								   void *meta) {
	herr_t err = 0;
	int i;
	char *mbuf;

	// Jump to blocks
	mbuf = (char *)meta + sizeof (H5VL_logi_meta_hdr);	// Header

	// Add nreq field if more than 1 blocks
	if (hdr.flag & H5VL_LOGI_META_FLAG_MUL_SEL) {
		*((int *)mbuf) = nsel;
		mbuf += sizeof (int);
	}
#ifdef LOGVOL_DEBUG
	else {
		if (nsel > 1) { RET_ERR ("Meta flag mismatch") }
	}
#endif
	mbuf += sizeof (MPI_Offset) * 2;  // Skip through file location, we don't know yet

	if (hdr.flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {
		// Dsteps
		memcpy (mbuf, dset.dsteps, sizeof (MPI_Offset) * (dset.ndim - 1));
		mbuf += sizeof (MPI_Offset) * (dset.ndim - 1);

		for (i = 0; i < nsel; i++) {
			H5VL_logi_sel_encode (dset.ndim, dset.dsteps, starts[i], (MPI_Offset *)mbuf);
			mbuf += sizeof (MPI_Offset);
		}
		for (i = 0; i < nsel; i++) {
			H5VL_logi_sel_encode (dset.ndim, dset.dsteps, counts[i], (MPI_Offset *)mbuf);
			mbuf += sizeof (MPI_Offset);
		}
	} else {
		for (i = 0; i < nsel; i++) {
			memcpy (mbuf, starts[i], sizeof (hsize_t) * dset.ndim);
			mbuf += sizeof (hsize_t) * dset.ndim;
		}
		for (i = 0; i < nsel; i++) {
			memcpy (mbuf, counts[i], sizeof (hsize_t) * dset.ndim);
			mbuf += sizeof (hsize_t) * dset.ndim;
		}
	}

err_out:;
	return err;
}

herr_t H5VL_logi_metaentry_encode (H5VL_log_dset_info_t &dset,
								   H5VL_logi_meta_hdr &hdr,
								   std::vector<H5VL_log_selection> sels,
								   void *meta) {
	herr_t err = 0;
	int i;
	char *mbuf;

	// Jump to blocks
	mbuf = (char *)meta + sizeof (H5VL_logi_meta_hdr);	// Header

	// Add nreq field if more than 1 blocks
	if (hdr.flag & H5VL_LOGI_META_FLAG_MUL_SEL) {
		*((int *)mbuf) = sels.size ();
		mbuf += sizeof (int);
	}
#ifdef LOGVOL_DEBUG
	else {
		if (sels.size () > 1) { RET_ERR ("Meta flag mismatch") }
	}
#endif
	mbuf += sizeof (MPI_Offset) * 2;  // Skip through file location, we don't know yet

	if (hdr.flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {
		// Dsteps
		memcpy (mbuf, dset.dsteps, sizeof (MPI_Offset) * (dset.ndim - 1));
		mbuf += sizeof (MPI_Offset) * (dset.ndim - 1);

		for (auto &sel : sels) {
			H5VL_logi_sel_encode (dset.ndim, dset.dsteps, sel.start, (MPI_Offset *)mbuf);
			mbuf += sizeof (MPI_Offset);
		}
		for (auto &sel : sels) {
			H5VL_logi_sel_encode (dset.ndim, dset.dsteps, sel.count, (MPI_Offset *)mbuf);
			mbuf += sizeof (MPI_Offset);
		}
	} else {
		for (auto &sel : sels) {
			memcpy (mbuf, sel.start, sizeof (hsize_t) * dset.ndim);
			mbuf += sizeof (hsize_t) * dset.ndim;
		}
		for (auto &sel : sels) {
			memcpy (mbuf, sel.count, sizeof (hsize_t) * dset.ndim);
			mbuf += sizeof (hsize_t) * dset.ndim;
		}
	}

err_out:;
	return err;
}