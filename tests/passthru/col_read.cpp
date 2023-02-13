/*
 *    Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *    See COPYRIGHT notice in top-level directory.
 */
#include <hdf5.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#include "H5VL_log.h"
#include "testutils.hpp"

#define ASSERT_EQUAL(A, B)   \
    {                        \
        CHECK_ERR ((A - B)); \
        CHECK_ERR ((B - A)); \
    }

int get_expected (int idx, int rank, int phase, int is_log, int np);

int main (int argc, char **argv) {
    herr_t err = 0;
    int nerrs  = 0;
    htri_t ret;
    int i, ii;
    int rank, np;
    const char *file_name;
    int mpi_required = 0;
    int expected     = 0;

    // VOL IDs
    hid_t log_vlid = -1;

    hid_t fid          = -1;  // File ID
    hid_t did          = -1;  // Dataset ID
    hid_t filespace_id = -1;  // File space ID
    hid_t memspace_id  = -1;  // Memory space ID
    hid_t faplid       = -1;  // File Access Property List
    hid_t dxplid       = -1;  // Data transfer Property List

    int *buf;
    hsize_t dims[2] = {0, 0};  // dims[0] will be modified later
    hsize_t start[2], count[2];
    vol_env env;

    // init MPI
    MPI_Init_thread (&argc, &argv, MPI_THREAD_MULTIPLE, &mpi_required);
    MPI_Comm_size (MPI_COMM_WORLD, &np);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);
    buf = (int *)malloc (2 * np * sizeof (int));

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
    check_env (&env);
    SHOW_TEST_INFO ("Checking column read")

    /* Set file access property list to MPI */
    faplid = H5Pcreate (H5P_FILE_ACCESS);
    CHECK_ERR (faplid);
    err = H5Pset_fapl_mpio (faplid, MPI_COMM_WORLD, MPI_INFO_NULL);
    CHECK_ERR (err);
    err = H5Pset_all_coll_metadata_ops (faplid, 1);
    CHECK_ERR (err);

    /* Create file */
    fid = H5Fcreate (file_name, H5F_ACC_TRUNC, H5P_DEFAULT, faplid);
    CHECK_ERR (fid);

    /* Create 2D (num_process x N) datasets */
    dims[0]      = np;
    dims[1]      = 2 * np;
    filespace_id = H5Screate_simple (2, dims, dims);
    CHECK_ERR (filespace_id);

    did =
        H5Dcreate2 (fid, "D", H5T_NATIVE_INT, filespace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    CHECK_ERR (did);

    /* Prepare data */
    for (i = 0; i < dims[1]; i++) { buf[i] = 100 * rank + i; }
    start[0] = rank;
    start[1] = 0;
    count[0] = 1;
    count[1] = dims[1];

    /* File space setting */
    err = H5Sselect_hyperslab (filespace_id, H5S_SELECT_SET, start, NULL, count, NULL);
    CHECK_ERR (err);

    /* Mem space setting */
    memspace_id = H5Screate_simple (1, dims + 1, dims + 1);
    CHECK_ERR (memspace_id);

    /* Request to write data */
    err = H5Dwrite (did, H5T_NATIVE_INT, memspace_id, filespace_id, H5P_DEFAULT, buf);
    CHECK_ERR (err);
    H5Fflush (fid, H5F_SCOPE_GLOBAL);
    H5Sclose (memspace_id);

    for (i = 0; i < dims[1]; i++) { buf[i] = -1; }

    start[0] = 0;
    start[1] = rank * 2;
    count[0] = np;
    count[1] = 1;

    /* File space setting */
    err = H5Sselect_hyperslab (filespace_id, H5S_SELECT_SET, start, NULL, count, NULL);
    CHECK_ERR (err);

    memspace_id = H5Screate_simple (1, dims, dims);
    CHECK_ERR (memspace_id);

    dxplid = H5Pcreate (H5P_DATASET_XFER);
    err    = H5Pset_buffered (dxplid, true);
    CHECK_ERR (err);

    err = H5Dread (did, H5T_NATIVE_INT, memspace_id, filespace_id, dxplid, buf);
    CHECK_ERR (err);

    for (i = 0; i < dims[1]; i++) {
        expected = get_expected (i, rank, 0, env.log_env, np);
        ASSERT_EQUAL (buf[i], expected);
    }

    MPI_Barrier (MPI_COMM_WORLD);
    start[0] = 0;
    start[1] = rank * 2 + 1;
    count[0] = np;
    count[1] = 1;

    /* File space setting */
    err = H5Sselect_hyperslab (filespace_id, H5S_SELECT_SET, start, NULL, count, NULL);
    CHECK_ERR (err);
    err = H5Dread (did, H5T_NATIVE_INT, memspace_id, filespace_id, dxplid, buf + np);

    for (i = 0; i < dims[1]; i++) {
        expected = get_expected (i, rank, 1, env.log_env, np);
        ASSERT_EQUAL (buf[i], expected);
    }

    H5Fflush (fid, H5F_SCOPE_GLOBAL);

    MPI_Barrier (MPI_COMM_WORLD);
    for (i = 0; i < dims[1]; i++) {
        expected = get_expected (i, rank, 2, env.log_env, np);
        ASSERT_EQUAL (buf[i], expected);
    }

    /* Close everything */
err_out:;
    if (dxplid > 0) {
        H5Pclose (dxplid);
        dxplid = -1;
    }
    if (memspace_id >= 0) H5Sclose (memspace_id);
    if (filespace_id >= 0) H5Sclose (filespace_id);
    if (did >= 0) H5Dclose (did);
    if (fid >= 0) H5Fclose (fid);
    if (faplid >= 0) H5Pclose (faplid);
    if (log_vlid >= 00) H5VLclose (log_vlid);
    if (buf) free (buf);

    MPI_Finalize ();

    return (nerrs > 0);
}

int get_expected (int idx, int rank, int phase, int is_log, int np) {
    int expected = -1;
    int temp_idx = idx % np;
    int region   = idx / np;
    if (np == 1) region = idx;

    if (phase == 0) {
        if (is_log == 1) {
            expected = -1;
        } else if (region == 0) {
            expected = temp_idx * 100 + 2 * rank + region;
        }

    } else if (phase == 1) {
        if (is_log == 1) {
            expected = -1;
        } else {
            expected = temp_idx * 100 + 2 * rank + region;
        }
    } else {
        expected = temp_idx * 100 + 2 * rank + region;
    }
    return expected;
}