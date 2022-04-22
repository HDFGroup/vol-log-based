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
#define N 1000000
int buf[N];
int main (int argc, char **argv) {
    int i;
    hid_t fid, did, sid;
    hid_t log_vlid, faplid, dxplid;
    hsize_t dim = N, start, count, one = 1;
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
    dxplid = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_nonblocking(dxplid, H5VL_LOG_REQ_NONBLOCKING);
    for (i = 0; i < N; i++) {
        buf[i] = i + 1; start  = i; count  = 1;
        H5Sselect_hyperslab (sid, H5S_SELECT_SET, &start, NULL, &one, &count);
        H5Dwrite (did, H5T_NATIVE_INT, H5S_CONTIG, sid, dxplid, buf + i);
    }
    // Close objects
    H5Sclose (sid); H5Dclose (did); H5Fclose (fid); H5Pclose(faplid); H5Pclose(dxplid);
    MPI_Finalize ();
    return 0;
}
