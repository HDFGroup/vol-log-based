/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */

/* This program is to test Asynchronous I/O VOl connector whether can return a
 * a valid faplid from calling H5Fget_access_plist(). See the github issue in
 * https://github.com/hpc-io/vol-cache/issues/15
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <hdf5.h>

#include "H5VL_log.h"
#include "testutils.hpp"

#define N 10

int main(int argc, char **argv) {
    const char *file_name;
    int rank, np, mpi_required, nerrs=0;
    herr_t err = 0;
    hid_t fid = -1;          // File ID
    hid_t faplid = -1;       // File Access Property List
    hid_t plist_id = -1;
    hid_t faplid2 = -1;
    hid_t log_vlid = H5I_INVALID_HID;  // Logvol ID
    vol_env env;

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
        file_name = "h5p_get_driver.h5";
    }

    /* check VOL related environment variables */
    check_env(&env);
    SHOW_TEST_INFO ("Test H5Pget_driver")

    faplid = H5Pcreate(H5P_FILE_ACCESS);
    CHECK_ERR(faplid);
    err = H5Pset_fapl_mpio(faplid, MPI_COMM_WORLD, MPI_INFO_NULL);
    CHECK_ERR(err);

    if (env.native_only == 0 && env.connector == 0) {
        // Register LOG VOL plugin
        log_vlid = H5VLregister_connector (&H5VL_log_g, H5P_DEFAULT);
        CHECK_ERR(log_vlid)
        err = H5Pset_vol (faplid, log_vlid, NULL);
        CHECK_ERR(err)
    }

    // create file
    fid = H5Fcreate(file_name, H5F_ACC_TRUNC, H5P_DEFAULT, faplid);
    CHECK_ERR(fid);

    // get faplid
    faplid2 = H5Fget_access_plist(fid);
    CHECK_ERR(faplid2);

    plist_id = H5Pget_driver(faplid2);  // Error occurs here when using Async I/O VOL
    CHECK_ERR(plist_id);

err_out:
    if (fid >= 0) H5Fclose(fid);
    if (faplid >= 0) H5Pclose(faplid);
    if (log_vlid != H5I_INVALID_HID) H5VLclose (log_vlid);

    SHOW_TEST_RESULT

    MPI_Finalize();

    return 0;
}

