#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
//
#include <cassert>
#include <cstdio>
#include <iostream>
//
#include <hdf5.h>
//
#include "H5VL_log_dataset.hpp"
#include "h5lreplay.hpp"
#include "h5lreplay_copy.hpp"

herr_t h5lreplay_copy_handler (hid_t o_id,
							   const char *name,
							   const H5O_info_t *object_info,
							   void *op_data) {
	herr_t err = 0;
	int i;
	hid_t src_did = -1;
	hid_t dst_did = -1;
	hid_t aid	  = -1;
	hid_t sid	  = -1;
	hid_t tid	  = -1;
	hid_t dcplid  = -1;
	dset_info dset;
	h5lreplay_copy_handler_arg *argp = (h5lreplay_copy_handler_arg *)op_data;

	// Skip unnamed and hidden object
	if ((name == NULL) || (name[0] == '_') || (name[0] == '/' || (name[0] == '.'))) {
#ifdef LOGVOL_DEBUG
		if (name[0] == '_') {
			std::cout << "Skip log-based VOL data objects " << name << std::endl;
		}
#endif
		goto err_out;
	}

	// Copy a dataset
	if (object_info->type == H5O_TYPE_DATASET) {
		int id;
		int ndim;
		hsize_t hndim;

#ifdef LOGVOL_DEBUG
		std::cout << "Reconstructing user dataset " << name << std::endl;
#endif

		// Open src dataset
		src_did = H5Dopen2 (o_id, name, H5P_DEFAULT);
		CHECK_ID (src_did)
		dcplid = H5Dget_create_plist (src_did);

		// Read ndim and dims
		aid = H5Aopen (src_did, "_dims", H5P_DEFAULT);
		CHECK_ID (aid)
		sid = H5Aget_space (aid);
		CHECK_ID (sid)
		ndim = H5Sget_simple_extent_dims (sid, &hndim, NULL);
		assert (ndim == 1);
		ndim = (int)hndim;
		H5Sclose (sid);
		sid = -1;
		err = H5Aread (aid, H5T_NATIVE_INT64, dset.dims);
		CHECK_ERR
		H5Aclose (aid);
		aid = -1;
		// Read max dims
		aid = H5Aopen (src_did, "_mdims", H5P_DEFAULT);
		CHECK_ID (aid)
		err = H5Aread (aid, H5T_NATIVE_INT64, dset.mdims);
		CHECK_ERR
		H5Aclose (aid);
		aid = -1;
		// Read dataset ID
		aid = H5Aopen (src_did, "_ID", H5P_DEFAULT);
		CHECK_ID (aid)
		err = H5Aread (aid, H5T_NATIVE_INT, &id);
		CHECK_ERR
		H5Aclose (aid);
		aid = -1;
		// Datatype and esize
		tid = H5Dget_type (src_did);
		CHECK_ID (tid)

		// Create dst dataset
		err = H5Pset_layout (dcplid, H5D_CONTIGUOUS);
		CHECK_ERR
		sid = H5Screate_simple (ndim, dset.dims, dset.mdims);
		CHECK_ID (sid)
		dst_did = H5Dcreate2 (argp->fid, name, tid, sid, H5P_DEFAULT, dcplid, H5P_DEFAULT);
		CHECK_ID (dst_did)

		// Filters
		err = H5VL_logi_get_filters (dcplid, dset.filters);
		CHECK_ERR

		// Copy all attributes
		// err = H5Aiterate2 (src_did, H5_INDEX_CRT_ORDER, H5_ITER_INC, &zero,
		//				   h5lreplay_attr_copy_handler, &dst_did);
		// CHECK_ERR

		// File offset
		err = H5Dclose (dst_did);
		CHECK_ERR
		dst_did = -1;
		err		= H5Fflush (argp->fid, H5F_SCOPE_LOCAL);
		CHECK_ERR
		dst_did = H5Dopen2 (argp->fid, name, H5P_DEFAULT);
		CHECK_ID (dst_did)
		dset.foff = H5Dget_offset (dst_did);
		CHECK_ID (dset.foff)

		dset.dsteps[dset.ndim - 1] = 1;
		for (i = dset.ndim - 2; i > -1; i--) {
			dset.dsteps[i] = dset.dsteps[i + 1] * dset.dims[i + 1];
		}
		dset.did   = dst_did;
		dset.dtype = tid;
		dset.esize = H5Tget_size (tid);
		dset.ndim  = ndim;

		// Record did for replaying data
		// Do not close dst_did
		argp->dsets[id] = dset;
	} else {  // Copy anything else as is
		err = H5Ocopy (o_id, name, argp->fid, name, H5P_DEFAULT, H5P_DEFAULT);
		CHECK_ERR
	}

err_out:;
	if (sid >= 0) { H5Sclose (sid); }
	if (aid >= 0) { H5Aclose (aid); }
	if (dcplid >= 0) { H5Pclose (dcplid); }
	if (src_did >= 0) { H5Dclose (src_did); }
	return err;
}

herr_t h5lreplay_attr_copy_handler (hid_t location_id,
									const char *attr_name,
									const H5A_info_t *ainfo,
									void *op_data) {
	herr_t err = 0;
	// hid_t dst_did = *((hid_t *)(op_data));

	// err=H5Ocopy(location_id,attr_name,dst_did,attr_name,H5P_DEFAULT,H5P_DEFAULT);
	// CHECK_ERR

	// err_out:;
	return err;
}