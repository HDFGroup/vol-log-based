/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 *
 *
 * This program is to test Async I/O VOL version 1.4, when using H5S_BLOCK as
 * the memory space ID. Encountered error messages are given below.
 *
 * To compile:
 *     mpicc h5s_block.c -o h5s_block -I HDF5/1.13.3/include -L HDF5/1.13.3/lib -lhdf5
 *
 * To run
 *     ./h5s_block
 *
 * Output on screen:
 * HDF5-DIAG: Error detected in HDF5 (1.13.3) MPI-process 0:
 *   #000: ../../hdf5-1.13.3/src/H5S.c line 487 in H5Scopy(): not a dataspace
 *     major: Invalid arguments to routine
 *     minor: Inappropriate type
 *   [ASYNC ABT LOG] Argobots execute async_dataset_write_fn failed
 * free(): invalid pointer
 * Abort (core dumped)
 *
 */

#include <stdio.h>
#include <assert.h>
#include <mpi.h>
#include <hdf5.h>

#include "H5VL_log.h"
#include "testutils.hpp"

#define N 10

int main (int argc, char **argv) {
    const char *file_name;
    int rank, np, nerrs=0, mpi_required, buf[N];
    herr_t err;
    hid_t file_id=-1, dspace_id=-1, dset_id=-1, mspace_id=-1, faplid=-1;
    hid_t log_vlid = H5I_INVALID_HID;  // Logvol ID
    vol_env env;
    hsize_t dims[2];

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
        file_name = "h5s_block.h5";
    }

    /* check VOL related environment variables */
    check_env(&env);
    SHOW_TEST_INFO ("Test H5S_BLOCK")

    faplid = H5Pcreate(H5P_FILE_ACCESS);
    CHECK_ERR(faplid)

    if (env.native_only == 0 && env.connector == 0) {
        // Register LOG VOL plugin
        log_vlid = H5VLregister_connector (&H5VL_log_g, H5P_DEFAULT);
        CHECK_ERR(log_vlid)
        err = H5Pset_vol(faplid, log_vlid, NULL);
        CHECK_ERR(err)
    }

    file_id = H5Fcreate(file_name, H5F_ACC_TRUNC, H5P_DEFAULT, faplid);
    CHECK_ERR(file_id)

    dims[0] = N;
    dspace_id = H5Screate_simple(1, dims, NULL);
    CHECK_ERR(dspace_id)
    dset_id = H5Dcreate(file_id, "var", H5T_NATIVE_INT, dspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    CHECK_ERR(dset_id)

    mspace_id = H5S_BLOCK;
    /* change H5S_BLOCK to H5S_ALL will avoid the error */

    err = H5Dwrite(dset_id, H5T_NATIVE_INT, mspace_id, H5S_ALL, H5P_DEFAULT, buf);
    CHECK_ERR(err)

err_out:
    if (dset_id != -1) H5Dclose(dset_id);
    if (dspace_id != -1) H5Sclose(dspace_id);
    if (file_id != -1) H5Fclose(file_id);
    if (faplid != -1) H5Pclose(faplid);
    if (log_vlid != H5I_INVALID_HID) H5VLclose (log_vlid);

    SHOW_TEST_RESULT

    MPI_Finalize ();
    return 0;
}

