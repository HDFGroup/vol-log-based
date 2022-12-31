/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */

#include <hdf5.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#include <cassert>

#include "H5VL_log.h"
#include "testutils.hpp"

#define N 10

int main (int argc, char **argv) {
    const char *file_name;
    int nerrs           = 0, rank, np, buf[N];
    herr_t err          = 0;
    hid_t file_id       = -1;  // File ID
    hid_t dataset_id    = -1;  // Dataset ID
    hid_t file_space_id = -1;  // Dataset space ID
    hid_t faplid        = -1;  // file access property list ID
    hid_t log_vlid      = -1;  // Logvol ID
    hsize_t dims[1]     = {N};
    vol_env env;

    int mpi_required;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &mpi_required);

    MPI_Comm_size (MPI_COMM_WORLD, &np);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);

    if (argc > 2) {
        if (!rank) printf ("Usage: %s [filename]\n", argv[0]);
        MPI_Finalize ();
        return 1;
    } else if (argc > 1) {
        file_name = argv[1];
    } else {
        file_name = "test.h5";
    }

    /* check VOL related environment variables */
    check_env(&env);
    SHOW_TEST_INFO ("H5Sselect_none 0")

    faplid = H5Pcreate (H5P_FILE_ACCESS);
    CHECK_ERR (faplid)

    // MPI and collective metadata is required by LOG VOL
    err = H5Pset_fapl_mpio (faplid, MPI_COMM_WORLD, MPI_INFO_NULL);
    CHECK_ERR (err)
    err = H5Pset_all_coll_metadata_ops (faplid, 1);
    CHECK_ERR (err)

    if (env.native_only == 0 && env.connector == 0) {
        // Register LOG VOL plugin
        log_vlid = H5VLregister_connector (&H5VL_log_g, H5P_DEFAULT);
        H5Pset_vol (faplid, log_vlid, NULL);
    }

    // Create file
    file_id = H5Fcreate (file_name, H5F_ACC_TRUNC, H5P_DEFAULT, faplid);
    CHECK_ERR (file_id)

    // Create dataset's dataspace (in file)
    dims[0]       = np;
    file_space_id = H5Screate_simple (1, dims, NULL);
    CHECK_ERR (file_space_id);

    // Create a new dataset
    dataset_id = H5Dcreate2 (file_id, "D", H5T_STD_I32LE, file_space_id,
                             H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    CHECK_ERR (dataset_id)

    // only rank 0 writes zero-sized data
    if (rank == 0) {
        /* In HDF5, the correct way to do zero-size write is to call select API
         * H5Sselect_none() using the dataset's dataspace, instead of
         * H5Screate(H5S_NULL). HDF5 reference manual says "Any dataspace
         * specified in file_space_id is ignored by the library and the
         * dataset's dataspace is always used." The dataset's dataspace is the
         * dataspace used when creating the dataset. Once the dataset is
         * created, its dataspace can be set to a selected area (e.g. subarray)
         * and passed into H5Dwrite() to write to an subbarray space of the
         * dataset occupied in the file. Note that the size of mem_type_id must
         * match file_space_id.
         */

        /* create a zero-sized memory space */
        hid_t mem_space_id = H5Screate (H5S_NULL);
        CHECK_ERR (mem_space_id)

        /* set the selection of dataset's file space to zero size */
        err = H5Sselect_none (file_space_id);
        CHECK_ERR (err)

        err = H5Dwrite (dataset_id, H5T_NATIVE_INT, mem_space_id, file_space_id, H5P_DEFAULT, NULL);
        CHECK_ERR (err)

        err = H5Sclose (mem_space_id);
        CHECK_ERR (err)

    } else {
        /* copy file_space_id into mem_space_id, so their sizes are equal */
        hid_t mem_space_id = H5Scopy (file_space_id);
        CHECK_ERR (mem_space_id)

        err = H5Dwrite (dataset_id, H5T_NATIVE_INT, mem_space_id, file_space_id, H5P_DEFAULT, buf);
        CHECK_ERR (err)

        err = H5Sclose (mem_space_id);
        CHECK_ERR (err)
    }

    err = H5Sclose (file_space_id);
    CHECK_ERR (err)
    file_space_id = -1;

    err = H5Dclose (dataset_id);
    CHECK_ERR (err)
    dataset_id = -1;

    err = H5Pclose (faplid);
    CHECK_ERR (err)
    faplid = -1;

    err = H5Fclose (file_id);
    CHECK_ERR (err)
    file_id = -1;

err_out:
    if (file_space_id >= 0) H5Sclose (file_space_id);
    if (dataset_id >= 0) H5Dclose (dataset_id);
    if (file_id >= 0) H5Fclose (file_id);
    if (faplid >= 0) H5Pclose (faplid);
    if (log_vlid >= 0) H5VLclose (log_vlid);

    SHOW_TEST_RESULT

    MPI_Finalize ();

    return (nerrs > 0);
}
