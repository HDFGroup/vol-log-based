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
 *  This example illustrates how to create an attribute attached to a
 *  dataset.  It is used in the HDF5 Tutorial.
 */

#include "hdf5.h"
#include "H5VL_log.h"
#include <cassert>
#define FILE "dset.h5"

int main()
{

	hid_t file_id, dataset_id, attribute_id, dataspace_id; /* identifiers */
	hid_t log_vlid, faplid;
	hsize_t dims;
	int attr_data[2];
	herr_t status;

	/* Initialize the attribute data. */
	attr_data[0] = 100;
	attr_data[1] = 200;

	MPI_Init(NULL, NULL);

	//Register LOG VOL plugin
	log_vlid = H5VLregister_connector(&H5VL_log_g, H5P_DEFAULT);
	assert(log_vlid >= 0);

	faplid = H5Pcreate(H5P_FILE_ACCESS);
	// MPI and collective metadata is required by LOG VOL
	H5Pset_fapl_mpio(faplid, MPI_COMM_WORLD, MPI_INFO_NULL);
	H5Pset_all_coll_metadata_ops(faplid, 1);
	H5Pset_vol(faplid, log_vlid, NULL);

	/* Open an existing file. */
	file_id = H5Fopen(FILE, H5F_ACC_RDWR, faplid);
	assert(file_id >= 0);

	/* Open an existing dataset. */
	dataset_id = H5Dopen2(file_id, "/dset", H5P_DEFAULT);
	assert(dataset_id >= 0);

	/* Create the data space for the attribute. */
	dims = 2;
	dataspace_id = H5Screate_simple(1, &dims, NULL);

	/* Create a dataset attribute. */
	attribute_id = H5Acreate2(dataset_id, "Units", H5T_STD_I32BE, dataspace_id,
							  H5P_DEFAULT, H5P_DEFAULT);
	assert(attribute_id >= 0);

	/* Write the attribute data. */
	status = H5Awrite(attribute_id, H5T_NATIVE_INT, attr_data);
	assert(status == 0);

	/* Close the attribute. */
	status = H5Aclose(attribute_id);

	/* Close the dataspace. */
	status = H5Sclose(dataspace_id);

	/* Close to the dataset. */
	status = H5Dclose(dataset_id);

	/* Close the file. */
	status = H5Fclose(file_id);

	status = H5Pclose(faplid);

	MPI_Finalize();

	return 0;
}
