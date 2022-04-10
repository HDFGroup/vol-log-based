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

#define N 10

int main (int argc, char **argv) {
    int err, nerrs = 0;
    int rank, np;
    const char *file_name;
    hid_t fid      = -1;  // File ID
    hid_t gid      = -1;  // Group ID
    hid_t sgid     = -1;  // Subgroup ID
    hid_t faplid   = -1;
    hid_t log_vlid = -1;  // Logvol ID

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
    SHOW_TEST_INFO ("Creating groups")

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

    // Create group
    gid = H5Gcreate2 (fid, "test", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    CHECK_ERR (gid)
    // Create sub-group
    sgid = H5Gcreate2 (gid, "test2", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    CHECK_ERR (sgid)
    err = H5Gclose (sgid);
    CHECK_ERR (err)
    err = H5Gclose (gid);
    CHECK_ERR (err)

    // Open group
    gid = H5Gopen2 (fid, "test", H5P_DEFAULT);
    CHECK_ERR (gid)
    // Open sub-group
    sgid = H5Gopen2 (gid, "test2", H5P_DEFAULT);
    CHECK_ERR (sgid)

    err = H5Gclose (sgid);
    CHECK_ERR (err)
    sgid = -1;
    err  = H5Gclose (gid);
    CHECK_ERR (err)
    gid = -1;

    err = H5Fclose (fid);
    CHECK_ERR (err)
    fid = -1;

    err = H5Pclose (faplid);
    CHECK_ERR (err)
    faplid = -1;

err_out:
    if (sgid >= 0) H5Gclose (sgid);
    if (gid >= 0) H5Gclose (gid);
    if (fid >= 0) H5Fclose (fid);
    if (faplid >= 0) H5Pclose (faplid);
    if (log_vlid >= 00) H5VLclose (log_vlid);

    SHOW_TEST_RESULT

    MPI_Finalize ();

    return (nerrs > 0);
}
