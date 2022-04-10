/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#include <hdf5.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#include "H5VL_log.h"
#include "testutils.hpp"

int main (int argc, char **argv) {
    herr_t err = 0;
    int nerrs  = 0;
    int rank, np;
    const char *file_name;
    hid_t fid      = -1;  // File ID
    hid_t did      = -1;  // Dataset ID
    hid_t sid      = -1;  // Dataset space ID
    hid_t faplid   = -1;
    hid_t log_vlid = -1;  // Logvol ID
    int buf;

    MPI_Init (&argc, &argv);
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
    SHOW_TEST_INFO ("Blocking write on datasets")

    // Register LOG VOL plugin
    log_vlid = H5VLregister_connector (&H5VL_log_g, H5P_DEFAULT);

    faplid = H5Pcreate (H5P_FILE_ACCESS);
    // MPI and collective metadata is required by LOG VOL
    H5Pset_fapl_mpio (faplid, MPI_COMM_WORLD, MPI_INFO_NULL);
    H5Pset_all_coll_metadata_ops (faplid, 1);
    H5Pset_vol (faplid, log_vlid, NULL);

    // Create file
    fid = H5Fcreate (file_name, H5F_ACC_TRUNC, H5P_DEFAULT, faplid);
    CHECK_ERR (fid)

    // Create datasets
    sid = H5Screate (H5S_SCALAR);
    CHECK_ERR (sid);
    did = H5Dcreate2 (fid, "D", H5T_STD_I32LE, sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    CHECK_ERR (did)

    // Rank 0 write to dataset
    if (!rank) {
        buf = 1;
        err = H5Dwrite (did, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &buf);
        CHECK_ERR (err)
    }

    err = H5Fflush (fid, H5F_SCOPE_LOCAL);
    CHECK_ERR (err)

    // All process read
    buf = 0;
    err = H5Dread (did, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &buf);
    CHECK_ERR (err)
    EXP_VAL (buf, 1)

    err = H5Sclose (sid);
    CHECK_ERR (err)
    sid = -1;
    err = H5Dclose (did);
    CHECK_ERR (err)
    did = -1;
    err = H5Fclose (fid);
    CHECK_ERR (err)
    fid = -1;

    err = H5Pclose (faplid);
    CHECK_ERR (err)
    faplid = -1;

err_out:;
    if (sid >= 0) H5Sclose (sid);
    if (did >= 0) H5Dclose (did);
    if (fid >= 0) H5Fclose (fid);
    if (faplid >= 0) H5Pclose (faplid);
    if (log_vlid >= 00) H5VLclose (log_vlid);

    SHOW_TEST_RESULT

    MPI_Finalize ();

    return (nerrs > 0);
}
