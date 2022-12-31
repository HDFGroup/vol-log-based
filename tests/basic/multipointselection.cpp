/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <hdf5.h>

#include "H5VL_log.h"
#include "testutils.hpp"

#define N 10

int main (int argc, char **argv) {
    const char *file_name;
    int i, rank, np, nerrs=0, buf[N];
    herr_t err;
    hid_t fapl_id=-1, log_vlid=H5I_INVALID_HID;
    hid_t file_id=-1, dspace_id=-1, dset_id=-1, mspace_id=-1, dxpl_id=-1;
    hsize_t dims[3], start[2];
    vol_env env;

    int mpi_required;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &mpi_required);

    MPI_Comm_size (MPI_COMM_WORLD, &np);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);

    if (argc > 2) {
        if (!rank) printf ("Usage: %s [filename]\n", argv[0]);
        MPI_Finalize ();
        return 1;
    } else if (argc > 1) {
        file_name = argv[1];
    } else {
        file_name = "multipointselection.h5";
    }

    /* check VOL related environment variables */
    check_env(&env);
    SHOW_TEST_INFO ("H5Sselect_elements")

    // Set MPI-IO and parallel access proterty.
    fapl_id = H5Pcreate (H5P_FILE_ACCESS);
    CHECK_ERR (fapl_id)
    err = H5Pset_fapl_mpio (fapl_id, MPI_COMM_WORLD, MPI_INFO_NULL);
    CHECK_ERR (err)
    err = H5Pset_all_coll_metadata_ops (fapl_id, 1);
    CHECK_ERR (err)
    err = H5Pset_coll_metadata_write (fapl_id, 1);
    CHECK_ERR (err)

    // Collective I/O
    dxpl_id = H5Pcreate (H5P_DATASET_XFER);
    CHECK_ERR (dxpl_id)
    err = H5Pset_dxpl_mpio (dxpl_id, H5FD_MPIO_COLLECTIVE);
    CHECK_ERR (err)

    if (env.native_only == 0 && env.connector == 0) {
        // Register LOG VOL plugin
        log_vlid = H5VLregister_connector (&H5VL_log_g, H5P_DEFAULT);
        CHECK_ERR (log_vlid)
        err = H5Pset_vol (fapl_id, log_vlid, NULL);
        CHECK_ERR (err)
    }

    // Create file
    file_id = H5Fcreate (file_name, H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id);
    CHECK_ERR (file_id)

    // Define dataset
    dims[0]   = np;
    dims[1]   = N;
    dspace_id = H5Screate_simple (2, dims, NULL);  // Dataset space
    CHECK_ERR (dspace_id)
    dset_id = H5Dcreate (file_id, "M", H5T_NATIVE_INT, dspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    CHECK_ERR (dset_id)
    mspace_id = H5Screate_simple (1, dims + 1, NULL);  // Memory space for I/O
    CHECK_ERR (mspace_id)

    // Select region
    start[0] = rank;
    start[1] = N - 1;
    err = H5Sselect_elements (dspace_id, H5S_SELECT_SET, 1, start);
    CHECK_ERR (err)
    for (i = 1; i < N; i++) {
        start[1] = N - i - 1;
        err = H5Sselect_elements (dspace_id, H5S_SELECT_PREPEND, 1, start);
        CHECK_ERR (err)
    }

    for (i = 0; i < N; i++) { buf[i] = rank + 1 + i; }
    err = H5Dwrite (dset_id, H5T_NATIVE_INT, mspace_id, dspace_id, dxpl_id, buf);
    CHECK_ERR (err)
    err = H5Fflush (file_id, H5F_SCOPE_GLOBAL);
    CHECK_ERR (err)

    // Close file
    err = H5Dclose (dset_id);
    CHECK_ERR (err)
    err = H5Fclose (file_id);
    CHECK_ERR (err)

    // Open file
    file_id = H5Fopen (file_name, H5F_ACC_RDONLY, fapl_id);
    CHECK_ERR (file_id)

    // Open dataset
    dset_id = H5Dopen2 (file_id, "M", H5P_DEFAULT);
    CHECK_ERR (dset_id)

    for (i = 0; i < N; i++) { buf[i] = 0; }
    err = H5Dread (dset_id, H5T_NATIVE_INT, mspace_id, dspace_id, dxpl_id, buf);
    CHECK_ERR (err)
    err = H5Fflush (file_id, H5F_SCOPE_GLOBAL);
    CHECK_ERR (err)

    for (i = 0; i < N; i++) {
        if (buf[i] != rank + 1 + i) {
            printf ("Rank %d: Error. Expect buf[%d] = %d, but got %d\n",
                    rank, i, rank + 1 + i, buf[i]);
        }
    }

err_out:
    if (dspace_id != -1) {
        err = H5Sclose (dspace_id);
        CHECK_ERR (err)
    }
    if (mspace_id != -1) {
        err = H5Sclose (mspace_id);
        CHECK_ERR (err)
    }
    if (dset_id != -1) {
        err = H5Dclose (dset_id);
        CHECK_ERR (err)
    }
    if (file_id != -1) {
        err = H5Fclose (file_id);
        CHECK_ERR (err)
    }
    if (log_vlid != H5I_INVALID_HID) {
        err = H5VLclose (log_vlid);
        CHECK_ERR (err)
    }
    if (fapl_id != -1) {
        err = H5Pclose (fapl_id);
        CHECK_ERR (err)
    }
    if (dxpl_id != -1) {
        err = H5Pclose (dxpl_id);
        CHECK_ERR (err)
    }

    SHOW_TEST_RESULT

    MPI_Finalize ();

    return (nerrs > 0);
}

