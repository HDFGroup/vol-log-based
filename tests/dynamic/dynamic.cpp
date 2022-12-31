/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */

#include <hdf5.h>
#include <mpi.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "testutils.hpp"

#define N 10

int main (int argc, char **argv) {
    int err, nerrs = 0, rank, np, buf[10][10];
    const char *file_name;
    hid_t fid, faplid, space_id, dset, xfer_plist, fspace, mspace;
    char volname[512] = {0};     // Name of current VOL
    ssize_t volname_len;         // Length of volname
    std::string vol_name;        // Name of the VOL used
    std::string target_vol_name; // Name of the specified VOL
    char *env_str;               // HDF5_VOL_CONNECTOR environment variable
    hsize_t ones[2] = {1, 1}, dims[2] = {10, 10}; /* dataspace dim sizes */
    hsize_t start[2], count[2];
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
        file_name = "test.h5";
    }

    check_env(&env);
    SHOW_TEST_INFO ("Creating files")

    faplid = H5Pcreate (H5P_FILE_ACCESS);
    // MPI and collective metadata is required by LOG VOL
    H5Pset_fapl_mpio (faplid, MPI_COMM_WORLD, MPI_INFO_NULL);
    H5Pset_all_coll_metadata_ops (faplid, 1);

    // Create file
    fid = H5Fcreate (file_name, H5F_ACC_TRUNC, H5P_DEFAULT, faplid);
    CHECK_ERR (fid)

    // Check VOL name
    env_str = getenv ("HDF5_VOL_CONNECTOR");
    if (env_str) {
        target_vol_name = std::string (strtok (env_str, " "));
    } else {
        target_vol_name = "native";
    }
    volname_len          = H5VLget_connector_name (fid, volname, 512);
    volname[volname_len] = '\0';
    vol_name             = std::string (volname);
    EXP_VAL (vol_name, target_vol_name)

    space_id = H5Screate_simple (2, dims, NULL);

    /* create a dataset collectively */
    dset = H5Dcreate2 (fid, "variable", H5T_NATIVE_INT, space_id, H5P_DEFAULT, H5P_DEFAULT,
                       H5P_DEFAULT);
    CHECK_ERR (dset)

    start[0] = (rank == 2 || rank == 3) ? 5 : 0;
    start[1] = (rank == 1 || rank == 3) ? 5 : 0;
    count[0] = count[1] = 5;

    fspace = H5Dget_space (dset);
    CHECK_ERR (fspace)
    err = H5Sselect_hyperslab (fspace, H5S_SELECT_SET, start, NULL, ones, count);
    CHECK_ERR (fspace)

    mspace = H5Screate_simple (2, count, NULL);
    CHECK_ERR (mspace)

    xfer_plist = H5Pcreate (H5P_DATASET_XFER);
    CHECK_ERR (xfer_plist)
    err = H5Pset_dxpl_mpio (xfer_plist, H5FD_MPIO_COLLECTIVE);
    CHECK_ERR (err)

    err = H5Dwrite (dset, H5T_NATIVE_INT, mspace, fspace, xfer_plist, buf);
    CHECK_ERR (err)

    err = H5Pclose (xfer_plist);
    CHECK_ERR (err)
    err = H5Sclose (mspace);
    CHECK_ERR (err)
    err = H5Sclose (fspace);
    CHECK_ERR (err)
    err = H5Dclose (dset);
    CHECK_ERR (err)
    err = H5Sclose (space_id);
    CHECK_ERR (err)
    err = H5Fclose (fid);
    CHECK_ERR (err)
    err = H5Pclose (faplid);
    CHECK_ERR (err)

err_out:
    SHOW_TEST_RESULT

    MPI_Finalize ();

    return (nerrs > 0);
}
