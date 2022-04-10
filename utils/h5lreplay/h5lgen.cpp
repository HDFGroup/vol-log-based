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
#define M 10
int buf[N * M];

int main (int argc, char **argv) {
    int err, nerrs = 0;
    int rank, np;
    const char *file_name;
    int i;
    hid_t fid       = -1;  // File ID
    hid_t faid      = -1;  // File attribute ID
    hid_t gid       = -1;  // Group ID
    hid_t gaid      = -1;  // Group attribute ID
    hid_t did       = -1;  // ID of dataset in file
    hid_t daid      = -1;  // Data attribute ID
    hid_t gdid      = -1;  // ID of dataset in group
    hid_t sid       = -1;  // Dataset space ID
    hid_t asid      = -1;  // Attribute dataset space ID
    hid_t msid      = -1;  // Memory space ID
    hid_t faplid    = -1;
    hid_t log_vlid  = -1;  // Logvol ID
    hsize_t dims[2] = {N, M};

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

    for (i = 0; i < N * M; i++) { buf[i] = i + 1; }

    faplid = H5Pcreate (H5P_FILE_ACCESS);
    CHECK_ID (faplid)
    // MPI and collective metadata is required by LOG VOL
    err = H5Pset_fapl_mpio (faplid, MPI_COMM_WORLD, MPI_INFO_NULL);
    CHECK_ERR (err)
    err = H5Pset_all_coll_metadata_ops (faplid, 1);
    CHECK_ERR (err)

    // Create file
    fid = H5Fcreate (file_name, H5F_ACC_TRUNC, H5P_DEFAULT, faplid);
    CHECK_ID (fid)

    // Create file dataset
    sid = H5Screate_simple (2, dims, dims);
    CHECK_ID (sid);
    did = H5Dcreate2 (fid, "D", H5T_STD_I32LE, sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    CHECK_ID (did)

    // Create dataset attr
    asid = H5Screate (H5S_SCALAR);
    CHECK_ID (asid);
    daid = H5Acreate2 (did, "A", H5T_STD_I32LE, asid, H5P_DEFAULT, H5P_DEFAULT);
    CHECK_ID (daid);

    // Create file attr
    faid = H5Acreate2 (fid, "A", H5T_STD_I32LE, asid, H5P_DEFAULT, H5P_DEFAULT);
    CHECK_ID (faid);

    // Create group
    gid = H5Gcreate2 (fid, "G", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    CHECK_ID (gid)

    // Create group dataset
    gdid = H5Dcreate2 (gid, "D", H5T_STD_I32LE, sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    CHECK_ID (gdid)

    // Create group attr
    gaid = H5Acreate2 (gid, "A", H5T_STD_I32LE, asid, H5P_DEFAULT, H5P_DEFAULT);
    CHECK_ID (gaid);

    // Write attributes
    err = H5Awrite (faid, H5T_NATIVE_INT32, buf);
    CHECK_ERR (err)
    err = H5Awrite (gaid, H5T_NATIVE_INT32, buf + 1);
    CHECK_ERR (err)
    err = H5Awrite (daid, H5T_NATIVE_INT32, buf + 2);
    CHECK_ERR (err)

    // Write datasets
    err = H5Dwrite (did, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf);
    CHECK_ERR (err)
    err = H5Dwrite (gdid, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf);
    CHECK_ERR (err)

err_out:
    if (sid >= 0) H5Sclose (sid);
    if (asid >= 0) H5Sclose (asid);
    if (msid >= 0) H5Sclose (msid);
    if (did >= 0) H5Dclose (did);
    if (gdid >= 0) H5Dclose (gdid);
    if (gid >= 0) H5Gclose (gid);
    if (faid >= 0) H5Aclose (faid);
    if (daid >= 0) H5Aclose (daid);
    if (gaid >= 0) H5Aclose (gaid);
    if (fid >= 0) H5Fclose (fid);
    if (faplid >= 0) H5Pclose (faplid);
    if (log_vlid >= 00) H5VLclose (log_vlid);

    MPI_Finalize ();

    return nerrs == 0 ? 0 : -1;
}
