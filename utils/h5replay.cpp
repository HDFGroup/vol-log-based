#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <hdf5.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "H5VL_log_file.hpp"
#include "H5VL_logi_nb.hpp"
#include "h5replay.hpp"
#include "unistd.h"

int main (int argc, char *argv[]) {
	herr_t ret;
	int rank, np;
	int opt;
	std::string inpath, outpath;

	MPI_Init (&argc, &argv);
	MPI_Comm_size (MPI_COMM_WORLD, &np);
	MPI_Comm_rank (MPI_COMM_WORLD, &rank);

	// Parse input
	while ((opt = getopt (argc, argv, "hi:o:")) != -1) {
		switch (opt) {
			case 'i':
				inpath = std::string (optarg);
				break;
			case 'o':
				outpath = std::string (optarg);
				break;
			case 'h':
			default:
				std::cout << "Usage: h5reply -i <input file path> -o <output file path>"
						  << std::endl;
				return 0;
		}
	}

	ret = h5replay_core (inpath, outpath);

	return 0;
}

herr_t h5replay_core (std::string &inpath, std::string &outpath) {
	herr_t err	= 0;
	hid_t finid = -1, foutid = -1;
	hid_t faplid = -1;
	hid_t lgid	 = -1;
	hid_t aid	 = -1;
	hid_t dxplid = -1;
	int ndset;
	int nldset;
	int nmdset;
	int config;
	int att_buf[4];
	char *metabuf = NULL;
	char *databuf = NULL;
	hid_t *dids	  = NULL;
	hid_t src_did = -1;
	h5replay_copy_handler_arg copy_arg;

	// Open the input and output file
	faplid = H5Pcreate (H5P_FILE_ACCESS);
	CHECK_ID (faplid)
	err = H5Pset_fapl_mpio (faplid, MPI_COMM_WORLD, MPI_INFO_NULL);
	CHECK_ERR

	finid = H5Fopen (inpath.c_str (), H5F_ACC_RDONLY, faplid);
	CHECK_ID (faplid)
	foutid = H5Fcreate (outpath.c_str (), H5F_ACC_TRUNC, H5P_DEFAULT, faplid);
	CHECK_ID (foutid)

	// Rread file metadata
	aid = H5Aopen (finid, "_int_att", H5P_DEFAULT);
	CHECK_ID (aid)

	dxplid = H5Pcreate (H5P_DATASET_XFER);
	CHECK_ID (dxplid)
	err = H5Pset_dxpl_mpio (dxplid, H5FD_MPIO_COLLECTIVE);
	CHECK_ERR
	err = H5Aread (aid, H5T_NATIVE_INT, att_buf);
	CHECK_ERR
	ndset  = att_buf[0];
	nldset = att_buf[1];
	nmdset = att_buf[2];
	config = att_buf[3];

	// Copy data objects
	dids		  = (hid_t *)malloc (sizeof (hid_t) * ndset);
	copy_arg.dids = dids;
	copy_arg.fid  = foutid;
	err = H5Ovisit3 (finid, H5_INDEX_CRT_ORDER, H5_ITER_INC, h5replay_copy_handler, &copy_arg,
					 H5O_INFO_ALL);
	CHECK_ERR

    // Open the log group
	lgid = H5Gopen2 (finid, LOG_GROUP_NAME, H5P_DEFAULT);
	CHECK_ID (lgid)

err_out:;
	if (dids) { free (dids); }
	if (faplid >= 0) { H5Pclose (faplid); }
	if (dxplid >= 0) { H5Pclose (dxplid); }
	if (aid >= 0) { H5Fclose (aid); }
	if (lgid >= 0) { H5Fclose (lgid); }
	if (finid >= 0) { H5Fclose (finid); }
	if (foutid >= 0) { H5Fclose (foutid); }
	return err;
}

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
        argp->dids[id]=dst_did;
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