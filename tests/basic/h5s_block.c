/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 *
 *
 * This program is to test Async I/O VOL version 1.4, when using H5S_BLOCK as
 * the memory space ID. Encountered error messages are given below.
 *
 * To compile:
 *     mpicc h5s_block.c -o h5s_block -I HDF5/1.13.3/include -L HDF5/1.13.3/lib -lhdf5
 *
 * To run
 *     ./h5s_block
 *
 * Output on screen:
 * HDF5-DIAG: Error detected in HDF5 (1.13.3) MPI-process 0:
 *   #000: ../../hdf5-1.13.3/src/H5S.c line 487 in H5Scopy(): not a dataspace
 *     major: Invalid arguments to routine
 *     minor: Inappropriate type
 *   [ASYNC ABT LOG] Argobots execute async_dataset_write_fn failed
 * free(): invalid pointer
 * Abort (core dumped)
 *
 */

#include <stdio.h>
#include <assert.h>
#include <mpi.h>
#include <hdf5.h>

#define N 10

int main (int argc, char **argv) {
    int mpi_required, buf[N];
    herr_t err;
    hid_t file_id, dspace_id, dset_id, mspace_id;
    hsize_t dims[2];

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &mpi_required);

    file_id = H5Fcreate ("h5s_block.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    assert(file_id != -1);

    dims[0] = N;
    dspace_id = H5Screate_simple(1, dims, NULL);
    assert(dspace_id != -1);
    dset_id = H5Dcreate(file_id, "var", H5T_NATIVE_INT, dspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    assert(dset_id != -1);

    mspace_id = H5S_BLOCK;
    /* change H5S_BLOCK to H5S_ALL will avoid the error */

    err = H5Dwrite(dset_id, H5T_NATIVE_INT, mspace_id, H5S_ALL, H5P_DEFAULT, buf);
    assert(err != -1);

    H5Dclose(dset_id);
    H5Sclose(dspace_id);
    H5Fclose(file_id);

    MPI_Finalize ();
    return 0;
}

