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
#define N 100000
int buf[N];
int main (int argc, char **argv) {
    int i;
    hid_t fid, did, sid;
    hid_t log_vlid, faplid;
    hsize_t dim = N;
    hsize_t **starts, **counts;
    MPI_Init (&argc, &argv);
    // Register LOG VOL plugin
    log_vlid = H5VL_log_register ();
    // Set log-based VOL in file access property
    faplid = H5Pcreate (H5P_FILE_ACCESS);
    H5Pset_fapl_mpio (faplid, MPI_COMM_WORLD, MPI_INFO_NULL);  // MPI required
    H5Pset_all_coll_metadata_ops (faplid, 1);
    H5Pset_vol (faplid, log_vlid, NULL);
    // Create file
    fid = H5Fcreate (argv[1], H5F_ACC_TRUNC, H5P_DEFAULT, faplid);
    // Create dataset
    sid = H5Screate_simple (1, &dim, &dim);
    did = H5Dcreate2 (fid, "D", H5T_STD_I32LE, sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    // Write to dataset
    starts    = (hsize_t **)malloc (sizeof (hsize_t *) * (N + 1) * 2);
    starts[0] = (hsize_t *)malloc (sizeof (hsize_t) * (N + 1) * 2);
    counts    = starts + N + 1; counts[0] = starts[0] + N + 1;
    for (i = 0; i <= N; i++) {
        starts[i + 1] = starts[i] + 1; starts[i][0] = i;
        counts[i + 1] = counts[i] + 1; counts[i][0] = 1;
    }
    H5Dwrite_n (did, H5T_NATIVE_INT, N, starts, counts, H5P_DEFAULT, buf);
    // Close objects
    H5Sclose (sid); H5Dclose (did); H5Fclose (fid); H5Pclose (faplid);
    free(starts); MPI_Finalize ();
    return 0;
}
