/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <hdf5.h>

#include "H5VL_log.h"
#include "testutils.hpp"

#define N 10

int main (int argc, char **argv) {
    int err, nerrs = 0;
    int rank, np;
    int buf = 1;
    const char *file_name;
    hid_t fid, gid, faid, gaid, sid;
    hid_t faplid;
    hid_t log_vlid=H5I_INVALID_HID;
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
        file_name = "attr.h5";
    }

    /* check VOL related environment variables */
    check_env(&env);
    SHOW_TEST_INFO ("Creating attributes")

    faplid = H5Pcreate (H5P_FILE_ACCESS);
    // MPI and collective metadata is required by LOG VOL
    err = H5Pset_fapl_mpio (faplid, MPI_COMM_WORLD, MPI_INFO_NULL);
    CHECK_ERR (err)
    err = H5Pset_all_coll_metadata_ops (faplid, 1);
    CHECK_ERR (err)

    if (env.native_only == 0 && env.connector == 0) {
        // Register LOG VOL plugin
        log_vlid = H5VLregister_connector (&H5VL_log_g, H5P_DEFAULT);
        CHECK_ERR (log_vlid)
        err = H5Pset_vol (faplid, log_vlid, NULL);
        CHECK_ERR (err)
    }

    // Create file
    fid = H5Fcreate (file_name, H5F_ACC_TRUNC, H5P_DEFAULT, faplid);
    CHECK_ERR (fid)
    // Create group
    gid = H5Gcreate2 (fid, "test", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    CHECK_ERR (gid)

    // Create attr
    sid  = H5Screate (H5S_SCALAR);
    faid = H5Acreate2 (fid, "test_attr", H5T_STD_I32LE, sid, H5P_DEFAULT, H5P_DEFAULT);
    CHECK_ERR (faid)
    gaid = H5Acreate2 (gid, "test_attr", H5T_STD_I32LE, sid, H5P_DEFAULT, H5P_DEFAULT);
    CHECK_ERR (gaid)

    // Write attr
    err = H5Awrite (faid, H5T_NATIVE_INT32, &buf);
    CHECK_ERR (err)
    err = H5Awrite (gaid, H5T_NATIVE_INT32, &buf);
    CHECK_ERR (err)

    err = H5Aclose (faid);
    CHECK_ERR (err)
    err = H5Aclose (gaid);
    CHECK_ERR (err)
    err = H5Gclose (gid);
    CHECK_ERR (err)
    err = H5Fclose (fid);
    CHECK_ERR (err)
    err = H5Pclose (faplid);
    CHECK_ERR (err)

err_out:
    if (log_vlid != H5I_INVALID_HID) H5VLclose (log_vlid);
    SHOW_TEST_RESULT

    MPI_Finalize ();

    return (nerrs > 0);
}
