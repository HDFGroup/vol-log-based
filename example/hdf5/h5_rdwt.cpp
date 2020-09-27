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
 *  This example illustrates how to write and read data in an existing
 *  dataset.  It is used in the HDF5 Tutorial.
 */

#include "hdf5.h"
#include "H5VL_log.h"
#include <cassert>
#define FILE "dset.h5"

int main()
{

   hid_t file_id, dataset_id; /* identifiers */
   hid_t log_vlid, faplid;
   herr_t status;
   int i, j, dset_data[4][6];

   MPI_Init(NULL, NULL);

   /* Initialize the dataset. */
   for (i = 0; i < 4; i++)
      for (j = 0; j < 6; j++)
         dset_data[i][j] = i * 6 + j + 1;

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

   /* Write the dataset. */
   status = H5Dwrite(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                     dset_data);
   assert(status == 0);

   status = H5Dread(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                    dset_data);
   assert(status == 0);

   /* Close the dataset. */
   status = H5Dclose(dataset_id);
   assert(status == 0);

   /* Close the file. */
   status = H5Fclose(file_id);
   assert(status == 0);

   status = H5Pclose(faplid);
   assert(status == 0);

   MPI_Finalize();

   return 0;
}
