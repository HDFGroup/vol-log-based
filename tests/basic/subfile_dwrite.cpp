/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h> /* getenv() */
#include <string.h> /* strcpy() */
#include <libgen.h> /* basename() */
#include <mpi.h>
#include <hdf5.h>

#include "H5VL_log.h"
#include "testutils.hpp"

#define N 10

int main(int argc, char **argv) {
    herr_t err = 0;
    int i, rank, np, nerrs=0, mpi_required, nsubfiles, buf[N];
    char file_name[256], *env_str;
    hid_t fid=-1, did=-1, sid=-1, msid=-1;
    hid_t fapl_id=-1, fcpl_id=H5P_DEFAULT, log_vlid=H5I_INVALID_HID;
    hsize_t dims[2] = {0, N}, start[2], count[2];
    vol_env env;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &mpi_required);
    MPI_Comm_size(MPI_COMM_WORLD, &np);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    sprintf(file_name, "%s.h5", basename(argv[0]));
    if (argc > 2) {
        if (!rank) printf("Usage: %s [filename]\n", argv[0]);
        MPI_Finalize();
        return 1;
    }
    else if (argc > 1)
        strcpy(file_name, argv[1]);

    /* check VOL related environment variables */
    check_env(&env);
    SHOW_TEST_INFO("subfileing write")

    fapl_id = H5Pcreate(H5P_FILE_ACCESS);
    CHECK_ERR(fapl_id)
    // MPI and collective metadata is required by LOG VOL
    err = H5Pset_fapl_mpio(fapl_id, MPI_COMM_WORLD, MPI_INFO_NULL);
    CHECK_ERR(err)
    err = H5Pset_all_coll_metadata_ops(fapl_id, 1);
    CHECK_ERR(err)

    if (env.native_only == 0 && env.connector == 0) {
        // Register LOG VOL plugin
        log_vlid = H5VLregister_connector(&H5VL_log_g, H5P_DEFAULT);
        CHECK_ERR(log_vlid)
        err = H5Pset_vol(fapl_id, log_vlid, NULL);
        CHECK_ERR(err)
    }

    env_str = getenv("H5VL_LOG_NSUBFILES");
    if (env_str == NULL) {
        /* set the number of subfiles */
        fcpl_id = H5Pcreate(H5P_FILE_CREATE);
        CHECK_ERR(fcpl_id)
        nsubfiles = np / 2;
        if (nsubfiles == 0) nsubfiles = -1;
        err = H5Pset_subfiling(fcpl_id, nsubfiles);
        CHECK_ERR(err)
    }

    // Create file
    fid = H5Fcreate(file_name, H5F_ACC_TRUNC, fcpl_id, fapl_id);
    CHECK_ERR(fid)

    // Create a dataset of 2D array of size np x N
    dims[0] = np;
    sid = H5Screate_simple(2, dims, dims);
    CHECK_ERR(sid);
    did = H5Dcreate2(fid, "D", H5T_STD_I32LE, sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    CHECK_ERR(did)

    for (i = 0; i < N; i++) { buf[i] = rank + i; }

    /* create a hyperslab of 1 x N */
    start[0] = rank;
    start[1] = 0;
    count[0] = 1;
    count[1] = N;
    err = H5Sselect_hyperslab(sid, H5S_SELECT_SET, start, NULL, count, NULL);
    CHECK_ERR(err)

    msid = H5Screate_simple(1, dims + 1, dims + 1);
    CHECK_ERR(msid);

    // Write to dataset in parallel
    err = H5Dwrite(did, H5T_NATIVE_INT, msid, sid, H5P_DEFAULT, buf);
    CHECK_ERR(err)

    err = H5Fflush(fid, H5F_SCOPE_LOCAL);
    CHECK_ERR(err)

err_out:;
    if (msid >= 0) H5Sclose(msid);
    if (sid >= 0) H5Sclose(sid);
    if (did >= 0) H5Dclose(did);
    if (fapl_id >= 0) H5Pclose(fapl_id);
    if (fcpl_id != H5P_DEFAULT) H5Pclose(fcpl_id);
    if (log_vlid != H5I_INVALID_HID) H5VLclose(log_vlid);
    if (fid >= 0) H5Fclose(fid);

    SHOW_TEST_RESULT

    MPI_Finalize();

    return (nerrs > 0);
}

