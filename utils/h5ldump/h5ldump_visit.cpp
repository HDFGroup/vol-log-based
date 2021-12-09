#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
//
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
//
#include <hdf5.h>
//
#include "H5VL_log_filei.hpp"
#include "H5VL_logi_err.hpp"
#include "H5VL_logi_nb.hpp"
#include "h5ldump.hpp"

herr_t h5ldump_visit (std::string path, std::vector<H5VL_log_dset_info_t> &dsets) {
	herr_t err = 0;
	hid_t fid  = -1;  // File ID

	// Open the input file
	fid = H5Fopen (path.c_str (), H5F_ACC_RDONLY, H5P_DEFAULT);
	CHECK_ID (fid)

	err = H5Ovisit3 (fid, H5_INDEX_CRT_ORDER, H5_ITER_INC, h5ldump_visit_handler, &dsets,
					 H5O_INFO_ALL);
	CHECK_ERR

err_out:;
	if (fid >= 0) { H5Fclose (fid); }
	return err;
}

herr_t h5ldump_visit_handler (hid_t o_id,
							  const char *name,
							  const H5O_info_t *object_info,
							  void *op_data) {
	herr_t err = 0;
	hid_t did  = -1;			// Current dataset ID
	hid_t aid  = -1;			// Dataset attribute ID
	hid_t sid  = -1;			// Dataset attribute space ID
	hid_t tid  = -1;			// Dataset type ID
	int id;						// Log VOL dataset ID
	H5VL_log_dset_info_t dset;	// Current dataset info
	std::vector<H5VL_log_dset_info_t> *dsets = (std::vector<H5VL_log_dset_info_t> *)op_data;

	// Skip unnamed and hidden object
	if ((name == NULL) || (name[0] == '_') || (name[0] == '/' || (name[0] == '.'))) {
		goto err_out;
	}

	// Visit a dataset
	if (object_info->type == H5O_TYPE_DATASET) {
		int ndim;
		hsize_t hndim;

		// Open src dataset
		did = H5Dopen2 (o_id, name, H5P_DEFAULT);
		CHECK_ID (did)

		// Read ndim and dims
		aid = H5Aopen (did, "_dims", H5P_DEFAULT);
		CHECK_ID (aid)
		sid = H5Aget_space (aid);
		CHECK_ID (sid)
		ndim = H5Sget_simple_extent_dims (sid, &hndim, NULL);
		assert (ndim == 1);
		dset.ndim = (int)hndim;
		H5Sclose (sid);
		sid = -1;
		err = H5Aread (aid, H5T_NATIVE_INT64, dset.dims);
		CHECK_ERR
		H5Aclose (aid);
		aid = -1;

		// Read dataset ID
		aid = H5Aopen (did, "_ID", H5P_DEFAULT);
		CHECK_ID (aid)
		err = H5Aread (aid, H5T_NATIVE_INT, &id);
		CHECK_ERR
		H5Aclose (aid);
		aid = -1;

		// Datatype and esize
		tid = H5Dget_type (did);
		CHECK_ID (tid)
		dset.esize = H5Tget_size (tid);

		dsets->push_back (dset);
	}
err_out:;
	if (sid >= 0) { H5Sclose (sid); }
	if (aid >= 0) { H5Aclose (aid); }
	if (did >= 0) { H5Dclose (did); }
	return err;
}