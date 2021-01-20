#include <hdf5.h>

#include "h5replay.hpp"
#include "h5replay_copy.hpp"

herr_t h5replay_copy_handler (hid_t o_id,
							  const char *name,
							  const H5O_info_t *object_info,
							  void *op_data) {
	herr_t err						= 0;
	hid_t src_did					= -1;
	hid_t dst_did					= -1;
	hid_t aid						= -1;
	hid_t sid						= -1;
	hid_t tid						= -1;
	hid_t dcplid					= -1;
	h5replay_copy_handler_arg *argp = (h5replay_copy_handler_arg *)op_data;

#ifdef LOGVOL_DEBUG
	cout << "Copying " << name << endl;
#endif

	// Skip unnamed and hidden object
	if ((name == NULL) || (name[0] == '_')) { goto err_out; }

	// Copy a dataset
	if (object_info->type == H5O_TYPE_DATASET) {
		int id;
		int ndim;
        hsize_t zero=0;
		hsize_t dims[H5S_MAX_RANK], mdims[H5S_MAX_RANK];

		// Open src dataset
		src_did = H5Dopen2 (o_id, name, H5P_DEFAULT);
		CHECK_ID (src_did)
		dcplid = H5Dget_create_plist (src_did);
		// Read ndim and dims
		aid = H5Aopen (src_did, "_dims", H5P_DEFAULT);
		CHECK_ID (aid)
		sid = H5Aget_space (aid);
		CHECK_ID (sid)
		ndim = H5Sget_simple_extent_ndims (sid);
		CHECK_ID (ndim)
		H5Sclose (sid);
		sid = -1;
		err = H5Aread (aid, H5T_NATIVE_INT64, dims);
		CHECK_ERR
		H5Aclose (aid);
		aid = -1;
		// Read max dims
		aid = H5Aopen (src_did, "_mdims", H5P_DEFAULT);
		CHECK_ID (aid)
		err = H5Aread (aid, H5T_NATIVE_INT64, mdims);
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

		// Create dst dataset
		err = H5Pset_layout (dcplid, H5D_CONTIGUOUS);
		CHECK_ERR
		sid = H5Screate_simple (ndim, dims, mdims);
		CHECK_ID (sid)
		dst_did = H5Dcreate2 (argp->fid, name, tid, sid, H5P_DEFAULT, dcplid, H5P_DEFAULT);
		CHECK_ID (dst_did)

        // Copy all attributes
        err= H5Aiterate2( src_did, H5_INDEX_CRT_ORDER, H5_ITER_INC, &zero, h5replay_attr_copy_handler, &dst_did);
        CHECK_ERR

        // Record did for replaying data
        // Do not close dst_did
        argp->dsets[id].id=dst_did;
		argp->dsets[id].ndim=ndim;
	} else {  // Copy anything else as is
		err = H5Ocopy (o_id, name, argp->fid, name, H5P_DEFAULT, H5P_DEFAULT);
		CHECK_ERR
	}

err_out:;
	if (sid >= 0) { H5Sclose (sid); }
	if (aid >= 0) { H5Aclose (aid); }
	if (src_did >= 0) { H5Sclose (src_did); }
	return err;
}

herr_t h5replay_attr_copy_handler( hid_t location_id, const char *attr_name, const H5A_info_t *ainfo, void *op_data){
    herr_t err=0;
    hid_t dst_did=*((hid_t*)(op_data));
    
    err=H5Ocopy(location_id,attr_name,dst_did,attr_name,H5P_DEFAULT,H5P_DEFAULT);
    CHECK_ERR

    err_out:;
	return err;
}