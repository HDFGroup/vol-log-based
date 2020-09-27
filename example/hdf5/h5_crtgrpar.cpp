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
 *  This example illustrates the creation of groups using absolute and 
 *  relative names.  It is used in the HDF5 Tutorial.
 */

#include "hdf5.h"
#include "H5VL_log.h"
#include <cassert>
#define FILE "groups.h5"

int main()
{

	hid_t file_id, group1_id, group2_id, group3_id; /* identifiers */
	hid_t log_vlid, faplid;
	herr_t status;

	MPI_Init(NULL, NULL);

	faplid = H5Pcreate(H5P_FILE_ACCESS);
	// MPI and collective metadata is required by LOG VOL
	H5Pset_fapl_mpio(faplid, MPI_COMM_WORLD, MPI_INFO_NULL);
	H5Pset_all_coll_metadata_ops(faplid, 1);
	H5Pset_vol(faplid, log_vlid, NULL);

	/* Create a new file using default properties. */
	file_id = H5Fcreate(FILE, H5F_ACC_TRUNC, H5P_DEFAULT, faplid);
	assert(file_id >= 0);

	/* Create group "MyGroup" in the root group using absolute name. */
	group1_id = H5Gcreate2(file_id, "/MyGroup", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	assert(group1_id >= 0);

	/* Create group "Group_A" in group "MyGroup" using absolute name. */
	group2_id = H5Gcreate2(file_id, "/MyGroup/Group_A", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	assert(group2_id >= 0);

	/* Create group "Group_B" in group "MyGroup" using relative name. */
	group3_id = H5Gcreate2(group1_id, "Group_B", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	assert(group3_id >= 0);

	/* Close groups. */
	status = H5Gclose(group1_id);
	status = H5Gclose(group2_id);
	status = H5Gclose(group3_id);

	/* Close the file. */
	status = H5Fclose(file_id);

	status = H5Pclose(faplid);

	MPI_Finalize();

	return 0;
}
