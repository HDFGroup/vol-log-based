/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#include <hdf5.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define N 1000000
int buf;
int main (int argc, char **argv) {
    int i;
    hid_t fid, did, sid, msid;
    hsize_t dim = N, start, count, one = 1;
    MPI_Init (&argc, &argv);
    // Create file
    fid = H5Fcreate (argv[1], H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    // Create dataset
    sid = H5Screate_simple (1, &dim, &dim);
    did = H5Dcreate2 (fid, "D", H5T_STD_I32LE, sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    // Write to dataset
    msid = H5Screate_simple(1, &one, &one);
    for (i = 0; i < N; i++) {
        buf = i + 1; start  = i; count  = 1;
        H5Sselect_hyperslab (sid, H5S_SELECT_SET, &start, NULL, &one, &count);
        H5Dwrite (did, H5T_NATIVE_INT, msid, sid, H5P_DEFAULT, &buf);
    }
    // Close objects
    H5Sclose (sid); H5Sclose (msid); H5Dclose (did); H5Fclose (fid);
    MPI_Finalize ();
    return 0;
}
