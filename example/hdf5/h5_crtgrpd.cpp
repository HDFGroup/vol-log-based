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
 *  This example illustrates how to create a dataset in a group.
 *  It is used in the HDF5 Tutorial.
 */

#include "hdf5.h"
#include "H5VL_log.h"
#include <cassert>
#define FILE "groups.h5"

int main()
{

	hid_t file_id, group_id, dataset_id, dataspace_id; /* identifiers */
	hid_t log_vlid, faplid;
	hsize_t dims[2];
	herr_t status;
	int i, j, dset1_data[3][3], dset2_data[2][10];

	MPI_Init(NULL, NULL);

	/* Initialize the first dataset. */
	for (i = 0; i < 3; i++)
		for (j = 0; j < 3; j++)
			dset1_data[i][j] = j + 1;

	/* Initialize the second dataset. */
	for (i = 0; i < 2; i++)
		for (j = 0; j < 10; j++)
			dset2_data[i][j] = j + 1;

	faplid = H5Pcreate(H5P_FILE_ACCESS);
	// MPI and collective metadata is required by LOG VOL
	H5Pset_fapl_mpio(faplid, MPI_COMM_WORLD, MPI_INFO_NULL);
	H5Pset_all_coll_metadata_ops(faplid, 1);
	H5Pset_vol(faplid, log_vlid, NULL);

	/* Open an existing file. */
	file_id = H5Fopen(FILE, H5F_ACC_RDWR, H5P_DEFAULT);
	assert(file_id >= 0);

	/* Create the data space for the first dataset. */
	dims[0] = 3;
	dims[1] = 3;
	dataspace_id = H5Screate_simple(2, dims, NULL);

	/* Create a dataset in group "MyGroup". */
	dataset_id = H5Dcreate2(file_id, "/MyGroup/dset1", H5T_STD_I32BE, dataspace_id,
							H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	assert(dataset_id >= 0);

	/* Write the first dataset. */
	status = H5Dwrite(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT,
					  dset1_data);
	assert(status == 0);

	/* Close the data space for the first dataset. */
	status = H5Sclose(dataspace_id);

	/* Close the first dataset. */
	status = H5Dclose(dataset_id);

	/* Open an existing group of the specified file. */
	group_id = H5Gopen2(file_id, "/MyGroup/Group_A", H5P_DEFAULT);
	assert(group_id >= 0);

	/* Create the data space for the second dataset. */
	dims[0] = 2;
	dims[1] = 10;
	dataspace_id = H5Screate_simple(2, dims, NULL);

	/* Create the second dataset in group "Group_A". */
	dataset_id = H5Dcreate2(group_id, "dset2", H5T_STD_I32BE, dataspace_id,
							H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	assert(dataset_id >= 0);

	/* Write the second dataset. */
	status = H5Dwrite(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT,
					  dset2_data);
	assert(status == 0);

	/* Close the data space for the second dataset. */
	status = H5Sclose(dataspace_id);

	/* Close the second dataset */
	status = H5Dclose(dataset_id);

	/* Close the group. */
	status = H5Gclose(group_id);

	/* Close the file. */
	status = H5Fclose(file_id);

	status = H5Pclose(faplid);

	MPI_Finalize();

	return 0;
}
