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
	mbuf = (char*)meta + sizeof (H5VL_logi_meta_hdr);	// Header

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
	mbuf = (char*)meta + sizeof (H5VL_logi_meta_hdr);	// Header

	// Add nreq field if more than 1 blocks
	if (hdr.flag & H5VL_LOGI_META_FLAG_MUL_SEL) {
		*((int *)mbuf) = sels.size();
		mbuf += sizeof (int);
	}
#ifdef LOGVOL_DEBUG
	else {
		if (sels.size() > 1) { RET_ERR ("Meta flag mismatch") }
	}
#endif
	mbuf += sizeof (MPI_Offset) * 2;  // Skip through file location, we don't know yet

	if (hdr.flag & H5VL_LOGI_META_FLAG_SEL_ENCODE) {
		// Dsteps
		memcpy (mbuf, dset.dsteps, sizeof (MPI_Offset) * (dset.ndim - 1));
		mbuf += sizeof (MPI_Offset) * (dset.ndim - 1);

		for (i = 0; i < sels.size(); i++) {
			H5VL_logi_sel_encode (dset.ndim, dset.dsteps, sels[i].start, (MPI_Offset *)mbuf);
			mbuf += sizeof (MPI_Offset);
		}
		for (i = 0; i < sels.size(); i++) {
			H5VL_logi_sel_encode (dset.ndim, dset.dsteps, sels[i].count, (MPI_Offset *)mbuf);
			mbuf += sizeof (MPI_Offset);
		}
	} else {
		for (i = 0; i < sels.size(); i++) {
			memcpy (mbuf, sels[i].start, sizeof (hsize_t) * dset.ndim);
			mbuf += sizeof (hsize_t) * dset.ndim;
			memcpy (mbuf, sels[i].count, sizeof (hsize_t) * dset.ndim);
			mbuf += sizeof (hsize_t) * dset.ndim;
		}
	}

err_out:;
	return err;
}