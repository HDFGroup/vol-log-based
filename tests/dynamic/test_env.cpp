/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * This program creates a file and then opens it. Before opening the file, the
 * environment variable for default VOL is changed to pass_through (built-in
 * pass through VOL) by the setenv function.
 *
 * HDF5 checks the default environment variable at library initialization time,
 * so changing the environment variables should have no effect on the default
 * VOL used on the second file. Reset all plugin paths and turn the VOL plugin
 * off and on does not make any difference.
 *
 * To change the VOL after HDF5 initialization, the user has to specify the VOL
 * to use in file access property programmatically.
 *
 * The compile and run commands are given below.
 *
 *    % mpicxx -g -o setenv setenv.cpp -lhdf5
 *
 *    % mpiexec -l -n 1 setenv setenv.nc
 *    % *** TESTING CXX    setenv: Change VOL env variable
 *    % Envinroment variables for file create:
 *    % HDF5_VOL_CONNECTOR = 
 *    % HDF5_PLUGIN_PATH = 
 *    % VOL used is native
 *    % Envinroment variables for file open:
 *    % HDF5_VOL_CONNECTOR = pass_through under_vol=0;under_info={}
 *    % HDF5_PLUGIN_PATH = 
 *    % VOL used is native
 *    % pass
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <libgen.h> /* basename() */
#include <hdf5.h>
#include <mpi.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "testutils.hpp"

int main (int argc, char **argv) {
    int rank, np, err, nerrs = 0, verbose=0;
    unsigned int i, nidx;
    const char *env_str, *file_name;
    char buf[1024];
    hid_t fid      = -1;  // File ID
    hid_t faplid   = -1;
    hid_t log_vlid = -1;  // Logvol ID
    char name1[512], name2[512];
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
    SHOW_TEST_INFO ("Change VOL env var")

    if (rank == 0) {
        if (verbose)
            std::cout << "\nEnvinroment variables for file create:" << std::endl;

        env_str = getenv ("HDF5_VOL_CONNECTOR");
        if (!env_str) env_str = const_cast<char *> ("");
        if (verbose)
            std::cout << "HDF5_VOL_CONNECTOR = " << env_str << std::endl;

        env_str = getenv ("HDF5_PLUGIN_PATH");
        if (!env_str) env_str = const_cast<char *> ("");
        if (verbose)
            std::cout << "HDF5_PLUGIN_PATH = " << env_str << std::endl;
    }

    faplid = H5Pcreate (H5P_FILE_ACCESS);
    assert (faplid >= 0);
    err = H5Pset_fapl_mpio (faplid, MPI_COMM_WORLD, MPI_INFO_NULL);
    assert (err >= 0);

    // Create file
    fid = H5Fcreate (file_name, H5F_ACC_TRUNC, H5P_DEFAULT, faplid);
    assert (fid >= 0);

    err = H5VLget_connector_name (fid, name1, 512);
    assert (err >= 0);
    if (rank == 0 && verbose) {
        std::cout << "Create file: " << file_name << std::endl;
        std::cout << "H5VLget_connector_name is " << name1 << std::endl;
    }

    err = H5Fclose (fid);
    assert (err >= 0);
    fid = -1;

    // Change env
    setenv ("HDF5_VOL_CONNECTOR", "pass_through under_vol=0;under_info={}", 1);

    if (rank == 0) {
        if (verbose)
            std::cout << "Envinroment variables for file open:" << std::endl;

        env_str = getenv ("HDF5_VOL_CONNECTOR");
        assert (env_str);
        if (verbose)
            std::cout << "HDF5_VOL_CONNECTOR = " << env_str << std::endl;

        env_str = getenv ("HDF5_PLUGIN_PATH");
        if (!env_str) env_str = const_cast<char *> ("");
        if (verbose)
            std::cout << "HDF5_PLUGIN_PATH = " << env_str << std::endl;
    }

    // Reset all plugin path
    err = H5PLsize (&nidx);
    assert (err >= 0);
    for (i = 0; i < nidx; i++) {
        err = H5PLget (i, buf, 1024);
        assert (err >= 0);
        err = H5PLreplace (buf, i);
        assert (err >= 0);
    }

    // Turn VOL plugin off and on
    err = H5PLget_loading_state (&nidx);
    assert (err >= 0);

    nidx &= ~(H5PL_VOL_PLUGIN);
    err = H5PLset_loading_state (nidx & (~(H5PL_VOL_PLUGIN)));
    assert (err >= 0);

    err = H5PLset_loading_state (nidx);
    assert (err >= 0);

    // Open file
    fid = H5Fopen (file_name, H5F_ACC_RDONLY, faplid);
    assert (fid >= 0);

    err = H5VLget_connector_name (fid, name2, 512);
    assert (err >= 0);
    if (rank == 0 && verbose) {
        std::cout << "Open file: " << file_name << std::endl;
        std::cout << "H5VLget_connector_name is " << name2 << std::endl;
    }

    err = H5Fclose (fid);
    assert (err >= 0);
    fid = -1;

    assert (std::string (name1) == std::string (name2));

    if (fid >= 0) H5Fclose (fid);
    if (faplid >= 0) H5Pclose (faplid);
    if (log_vlid >= 00) H5VLclose (log_vlid);

    SHOW_TEST_RESULT

    MPI_Finalize ();

    return (nerrs > 0);
}
