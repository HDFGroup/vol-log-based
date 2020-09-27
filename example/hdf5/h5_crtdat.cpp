/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 *  This example illustrates how to create a dataset that is a 4 x 6 
 *  array.  It is used in the HDF5 Tutorial.
 */

#include "hdf5.h"
#include "H5VL_log.h"
#include <cassert>
#define FILE "dset.h5"

int main()
{

	hid_t file_id, dataset_id, dataspace_id; /* identifiers */
	hid_t log_vlid, faplid;
	hsize_t dims[2];
	herr_t status;

	MPI_Init(NULL, NULL);

	//Register LOG VOL plugin
	log_vlid = H5VLregister_connector(&H5VL_log_g, H5P_DEFAULT);
	assert(log_vlid >= 0);

	faplid = H5Pcreate(H5P_FILE_ACCESS);
	// MPI and collective metadata is required by LOG VOL
	H5Pset_fapl_mpio(faplid, MPI_COMM_WORLD, MPI_INFO_NULL);
	H5Pset_all_coll_metadata_ops(faplid, 1);
	H5Pset_vol(faplid, log_vlid, NULL);

	/* Create a new file using default properties. */
	file_id = H5Fcreate(FILE, H5F_ACC_TRUNC, H5P_DEFAULT, faplid);
	assert(file_id >= 0);

	/* Create the data space for the dataset. */
	dims[0] = 4;
	dims[1] = 6;
	dataspace_id = H5Screate_simple(2, dims, NULL);

	/* Create the dataset. */
	dataset_id = H5Dcreate2(file_id, "/dset", H5T_STD_I32BE, dataspace_id,
							H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	assert(dataset_id >= 0);

	/* End access to the dataset and release resources used by it. */
	status = H5Dclose(dataset_id);

	/* Terminate access to the data space. */
	status = H5Sclose(dataspace_id);

	/* Close the file. */
	status = H5Fclose(file_id);

	status = H5Pclose(faplid);

	MPI_Finalize();

	return 0;
}
