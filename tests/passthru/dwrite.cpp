/*
 *    Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *    See COPYRIGHT notice in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <hdf5.h>
#include <mpi.h>

#include "testutils.hpp"

#define N 10

int main(int argc, char **argv) {
    herr_t err = 0;
    int nerrs = 0;
    int i;
    int rank, np;
    int mpi_required = 0;
    const char *file_name;

    hid_t fid = -1;          // File ID
    hid_t did = -1;          // Dataset ID
    hid_t filespace_id = -1; // File space ID
    hid_t memspace_id = -1;  // Memory space ID
    hid_t fcplid = -1;       // File Access Property List

    int buf[N];
    hsize_t dims[2] = {0, N}; // dims[0] will be modified later
    hsize_t start[2], count[2];
    vol_env env;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &mpi_required);
    MPI_Comm_size(MPI_COMM_WORLD, &np);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

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
    SHOW_TEST_INFO ("Writing hyperslab")

    /* Set file create property list to MPI */
    fcplid = H5Pcreate(H5P_FILE_ACCESS);
    CHECK_ERR(fcplid);
    err = H5Pset_fapl_mpio(fcplid, MPI_COMM_WORLD, MPI_INFO_NULL);
    CHECK_ERR(err);
    err = H5Pset_all_coll_metadata_ops(fcplid, 1);
    CHECK_ERR(err);

    /* Create file */
    fid = H5Fcreate(file_name, H5F_ACC_TRUNC, H5P_DEFAULT, fcplid);
    CHECK_ERR(fid);

    /* Create 2D (num_process x N) datasets */
    dims[0] = np;
    filespace_id = H5Screate_simple(2, dims, dims);
    CHECK_ERR(filespace_id);
    did = H5Dcreate(fid, "D", H5T_STD_I32LE, filespace_id, H5P_DEFAULT,
                    H5P_DEFAULT, H5P_DEFAULT);
    CHECK_ERR(did);

    /* Prepare data */
    for (i = 0; i < N; i++) {
        buf[i] = rank * 100 + i;
    }
    start[0] = rank;
    start[1] = 0;
    count[0] = 1;
    count[1] = N;

    /* File space setting */
    err = H5Sselect_hyperslab(filespace_id, H5S_SELECT_SET, start, NULL, count,
                              NULL);
    CHECK_ERR(err);

    /* Mem space setting */
    memspace_id = H5Screate_simple(1, dims + 1, dims + 1);
    CHECK_ERR(memspace_id);

    /* write data */
    err = H5Dwrite(did, H5T_NATIVE_INT, memspace_id, filespace_id, H5P_DEFAULT,
                   buf);
    CHECK_ERR(err);

    /* Close everything */
err_out:;
    if (memspace_id >= 0)
        H5Sclose(memspace_id);
    if (filespace_id >= 0)
        H5Sclose(filespace_id);
    if (did >= 0)
        H5Dclose(did);
    if (fid >= 0)
        H5Fclose(fid);
    if (fcplid >= 0)
        H5Pclose(fcplid);

    SHOW_TEST_RESULT

    MPI_Finalize();

    return (nerrs > 0);
}
