/*********************************************************************
 *
 *  Copyright (C) 2021, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 *
 *********************************************************************/
/* $Id$ */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * This example shows how to use the H5Dread_n API wrapper comes with the
 * log-vased VOL.
 * The H5Dread_n API is analogous to the ncmpi_get_varn* APIs in PnetCDF.
 *
 *    To compile:
 *        mpicxx -O2 dwrite_n.cpp -o dwrite_n -lH5VL_log -lhdf5
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <H5VL_log.h>
#include <hdf5.h>
#include <mpi.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>

#define N 10  // Size to write per process

int main (int argc, char **argv) {
    herr_t err = 0;
    int i;
    int rank, np;
    const char *file_name;
    hid_t fid;   // File ID
    hid_t did;   // Dataset ID
    hid_t sid;   // Dataset space ID
    hid_t msid;  // Memory space ID
    hid_t faplid;
    hid_t log_vlid;                               // ID of log-based VOL
    hsize_t dims[2] = {0, N};                     // Size of the dataset
    hsize_t start[2], count[2], one[2] = {1, 1};  // Buffer for setting selection
    hsize_t **starts, **counts;                   // Buffer for setting selection in H5Dwrite_n
    int buf[N];                                   // Data buffer

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

    // Register LOG VOL plugin
    log_vlid = H5VLregister_connector (&H5VL_log_g, H5P_DEFAULT);

    faplid = H5Pcreate (H5P_FILE_ACCESS);
    // MPI and collective metadata is required by LOG VOL
    H5Pset_fapl_mpio (faplid, MPI_COMM_WORLD, MPI_INFO_NULL);
    H5Pset_all_coll_metadata_ops (faplid, 1);
    H5Pset_vol (faplid, log_vlid, NULL);

    // Create file
    fid = H5Fcreate (file_name, H5F_ACC_TRUNC, H5P_DEFAULT, faplid);
    assert (fid >= 0);

    // Create datasets
    dims[0] = np;
    sid     = H5Screate_simple (2, dims, dims);
    assert (sid >= 0);
    did = H5Dcreate2 (fid, "D", H5T_STD_I32LE, sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    assert (did >= 0);

    // Initialize data buffer
    for (i = 0; i < N; i++) { buf[i] = rank + i; }

    // Write to the dataset using H5Dwrite
    msid = H5Screate_simple (1, dims + 1, dims + 1);
    assert (msid >= 0);
    start[0] = rank;
    start[1] = 0;
    count[0] = 1;
    count[1] = N;
    err      = H5Sselect_hyperslab (sid, H5S_SELECT_SET, start, NULL, count, one);
    assert (err == 0);
    err = H5Dwrite (did, H5T_NATIVE_INT, msid, sid, H5P_DEFAULT, buf);
    assert (err == 0);

    // Read from the dataset using H5Dread
    start[0] = 0;
    count[0] = 1;
    err      = H5Sselect_hyperslab (msid, H5S_SELECT_SET, start, NULL, count, one);
    assert (err == 0);
    for (i = 0; i < N; i++) {
        start[0] = rank;
        start[1] = i;
        count[0] = 1;
        count[1] = 1;
        err      = H5Sselect_hyperslab (sid, H5S_SELECT_SET, start, NULL, count, one);
        assert (err == 0);
        err = H5Dread (did, H5T_NATIVE_INT, msid, sid, H5P_DEFAULT, buf + i);
        assert (err == 0);
    }

    // Equivalent operation using H5Dread_n
    dims[0] = 1;
    msid    = H5Screate_simple (1, dims, dims);
    assert (msid >= 0);
    starts = (hsize_t **)malloc (sizeof (hsize_t *) * N * 2);
    assert (starts != NULL);
    counts    = starts + N;
    starts[0] = (hsize_t *)malloc (sizeof (hsize_t) * N * 2 * 2);
    assert (starts[0] != NULL);
    counts[0] = starts[0] + N * 2;
    for (i = 1; i < N; i++) {
        starts[i] = starts[i - 1] + 2;
        counts[i] = counts[i - 1] + 2;
    }
    for (i = 0; i < N; i++) {
        starts[i][0] = rank;
        starts[i][1] = i;
        counts[i][0] = 1;
        counts[i][1] = 1;
    }
    H5Dread_n (did, H5T_NATIVE_INT, N, starts, counts, H5P_DEFAULT, buf);
    free (starts[0]);
    free (starts);

    err = H5Sclose (sid);
    assert (err == 0);
    err = H5Dclose (did);
    assert (err == 0);
    err = H5Fclose (fid);
    assert (err == 0);

    err = H5Pclose (faplid);
    assert (err == 0);

    err = H5VLclose (log_vlid);
    assert (err == 0);

    MPI_Finalize ();

    return 0;
}
