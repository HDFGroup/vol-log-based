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

int buf[12];
int main (int argc, char **argv) {
    int i;
    int rank;
    hid_t logvol_id, file_id, fapl_id, dataset_id, dxpl_id, dset_space_id;

    MPI_Init (&argc, &argv);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);

    for(i = 0; i < 12; i++) buf[i] = rank;

    #include "H5VL_log.h"

    fapl_id   = H5Pcreate (H5P_FILE_ACCESS);
    H5Pset_fapl_mpio (fapl_id, MPI_COMM_WORLD, MPI_INFO_NULL);
    logvol_id = H5VL_log_register ();
    H5Pset_vol (fapl_id, logvol_id, NULL);
    file_id = H5Fcreate (argv[1], H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id);

    hsize_t dim[2] = {9, 12};
    dset_space_id  = H5Screate_simple (2, dim, dim);
    dataset_id = H5Dcreate2 (file_id, "D", H5T_STD_I32LE, dset_space_id, 
                             H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    hsize_t start[2] = {(hsize_t)rank / 3 * 3, ((hsize_t)rank % 3) * 4}, 
            block[2] = {3, 4}, count[2] = {1, 1};
    H5Sselect_hyperslab (dset_space_id, H5S_SELECT_SET, start, NULL, count, block);
    dxpl_id = H5Pcreate (H5P_DATASET_XFER);
    H5Pset_buffered (dxpl_id, false);
    H5Dwrite (dataset_id, H5T_NATIVE_INT, H5S_BLOCK, dset_space_id, dxpl_id, buf);

    // Close objects
    H5Sclose (dset_space_id);
    H5Dclose (dataset_id);
    H5Fclose (file_id);
    H5Pclose (fapl_id);
    MPI_Finalize ();
    return 0;
}
