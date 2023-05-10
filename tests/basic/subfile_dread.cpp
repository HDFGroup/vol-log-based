/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */

#include <hdf5.h>
#include <libgen.h> /* basename() */
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h> /* getenv() */
#include <string.h> /* strcpy() */

#include "H5VL_log.h"
#include "testutils.hpp"

#define INDEP_CHECK_ERR(A, COMM)                                             \
    {                                                                        \
        err_macro = (A);                                                     \
        MPI_Allreduce (MPI_IN_PLACE, &err_macro, 1, MPI_INT, MPI_MIN, COMM); \
        if (err_macro < 0) {                                                 \
            nerrs++;                                                         \
            printf ("Error at line %d in %s:\n", __LINE__, __FILE__);        \
            goto err_out;                                                    \
        }                                                                    \
    }

#define DEBUG_PRINT(COMM)                                                              \
    {                                                                                  \
        MPI_Barrier (COMM);                                                            \
        if (rank == 0) { printf ("DEBUG: %s:%s:%d\n", __FILE__, __func__, __LINE__); } \
    }
int create_subfile (const char *file_name, int rank, int np, vol_env *env_ptr);
int read_subfile (const char *file_name, int rank, int np, vol_env *env_ptr, MPI_Comm comm);
int expected_buf_val (int rank, int np, int i, int is_write, int is_columwise);

int main (int argc, char **argv) {
    int err_macro = 0;
    herr_t err    = 0;
    int i, rank, np, nerrs = 0, mpi_required;
    char file_name[256], *env_str;
    vol_env env;
    MPI_Comm comm1 = MPI_COMM_WORLD, comm2 = MPI_COMM_WORLD;

    MPI_Init_thread (&argc, &argv, MPI_THREAD_MULTIPLE, &mpi_required);
    MPI_Comm_size (MPI_COMM_WORLD, &np);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);

    sprintf (file_name, "%s.h5", basename (argv[0]));
    if (argc > 2) {
        if (!rank) printf ("Usage: %s [filename]\n", argv[0]);
        MPI_Finalize ();
        return 1;
    } else if (argc > 1)
        strcpy (file_name, argv[1]);

    // check VOL related environment variables
    check_env (&env);
    SHOW_TEST_INFO ("subfileing read")

    // pre-process: create a file with (np / 2) subfiles
    err = create_subfile (file_name, rank, np, &env);
    INDEP_CHECK_ERR (err, MPI_COMM_WORLD)
    DEBUG_PRINT (MPI_COMM_WORLD);

    {  // test reading with np processes (i.e. nproc > nsubfiles)
        MPI_Barrier (MPI_COMM_WORLD);
        err = read_subfile (file_name, rank, np, &env, MPI_COMM_WORLD);
        INDEP_CHECK_ERR (err, MPI_COMM_WORLD)
    }
    DEBUG_PRINT (MPI_COMM_WORLD);

    {  // test reading with np / 2 processes (i.e. nproc == nsubfiles)
        MPI_Barrier (MPI_COMM_WORLD);
        MPI_Comm_split (MPI_COMM_WORLD, rank * 2 / np, rank, &comm1);
        if (rank < (np + 1) / 2) { err = read_subfile (file_name, rank, np, &env, comm1); }
        INDEP_CHECK_ERR (err, MPI_COMM_WORLD)
    }
    DEBUG_PRINT (MPI_COMM_WORLD);

    {  // test reading with np / 4 processes (i.e. nproc < nsubfiles)
        MPI_Barrier (MPI_COMM_WORLD);
        MPI_Comm_split (MPI_COMM_WORLD, rank * 4 / np, rank, &comm2);
        if (rank < (np + 3) / 4) { err = read_subfile (file_name, rank, np, &env, comm2); }
        INDEP_CHECK_ERR (err, MPI_COMM_WORLD)
    }
    DEBUG_PRINT (MPI_COMM_WORLD);

    SHOW_TEST_RESULT
err_out:;
    if (comm1 != MPI_COMM_WORLD) MPI_Comm_free (&comm1);
    if (comm2 != MPI_COMM_WORLD) MPI_Comm_free (&comm2);
    MPI_Finalize ();

    return (nerrs > 0);
}

int expected_buf_val (int rank, int np, int i, int is_write, int is_columwise) {
    if (is_write) return rank * 100 + i;
    // below: is_read
    if (is_columwise) return i * 100 + (np - rank - 1);
    return (np - rank - 1) * 100 + i;
}

int read_subfile (const char *file_name, int rank, int np, vol_env *env_ptr, MPI_Comm comm) {
    herr_t err = 0;
    int i, nerrs = 0, nsubfiles;
    int err_macro = 0;
    hid_t fid = -1, did = -1, sid = -1, msid = -1, msid2 = -1;
    hid_t fapl_id = -1, fcpl_id = H5P_DEFAULT, log_vlid = H5I_INVALID_HID;
    hsize_t dims[2] = {0, 0}, start[2], count[2];
    int *buf        = NULL;
    int ii          = 0;

    CHECK_ERR (np);
    buf = (int *)malloc (sizeof (int) * np * np);

    fapl_id = H5Pcreate (H5P_FILE_ACCESS);
    CHECK_ERR (fapl_id)
    // MPI and collective metadata is required by LOG VOL
    err = H5Pset_fapl_mpio (fapl_id, comm, MPI_INFO_NULL);
    CHECK_ERR (err)
    err = H5Pset_all_coll_metadata_ops (fapl_id, 1);
    CHECK_ERR (err)

    if (env_ptr->native_only == 0 && env_ptr->connector == 0) {
        // Register LOG VOL plugin
        log_vlid = H5VLregister_connector (&H5VL_log_g, H5P_DEFAULT);
        CHECK_ERR (log_vlid)
        err = H5Pset_vol (fapl_id, log_vlid, NULL);
        CHECK_ERR (err)
    }
    DEBUG_PRINT (comm);

    // Open file
    fid = H5Fopen (file_name, H5F_ACC_RDONLY, fapl_id);
    CHECK_ERR (fid)

    // Open a dataset of 2D array of size np x np
    dims[0] = np;
    dims[1] = np;
    sid     = H5Screate_simple (2, dims, dims);
    CHECK_ERR (sid);
    did = H5Dopen2 (fid, "D", H5P_DEFAULT);
    CHECK_ERR (did)

    msid = H5Screate_simple (1, dims + 1, dims + 1);
    CHECK_ERR (msid);
    DEBUG_PRINT (comm);

    {  // test1: read pattern same as write pattern
        // reset buffer
        for (i = 0; i < np * np; i++) { buf[i] = 0; }

        // create a hyperslab of 1 x np
        start[0] = rank;
        start[1] = 0;
        count[0] = 1;
        count[1] = np;
        err      = H5Sselect_hyperslab (sid, H5S_SELECT_SET, start, NULL, count, NULL);
        CHECK_ERR (err)

        // read from dataset
        err = H5Dread (did, H5T_NATIVE_INT, msid, sid, H5P_DEFAULT, buf);
        CHECK_ERR (err)

        for (i = 0; i < np; i++) {
            if (buf[i] != expected_buf_val (rank, np, i, 1, 0)) { nerrs++; }
        }
        INDEP_CHECK_ERR ((-nerrs), comm);
        DEBUG_PRINT (comm);
    }

    {  // test2: read pattern different from write pattern, but still row-wise.
        // reset buffer
        for (i = 0; i < np * np; i++) { buf[i] = 0; }

        /* create a hyperslab of 1 x np */
        start[0] = np - rank - 1;
        start[1] = 0;
        count[0] = 1;
        count[1] = np;
        err      = H5Sselect_hyperslab (sid, H5S_SELECT_SET, start, NULL, count, NULL);
        CHECK_ERR (err)

        // read from dataset
        err = H5Dread (did, H5T_NATIVE_INT, msid, sid, H5P_DEFAULT, buf);
        CHECK_ERR (err)

        for (i = 0; i < np; i++) {
            if (buf[i] != expected_buf_val (rank, np, i, 0, 0)) { nerrs++; }
        }

        INDEP_CHECK_ERR ((-nerrs), comm);
        DEBUG_PRINT (comm);
    }

    {  // test3: read pattern different from write pattern, read column-wise.
        // reset buffer
        for (i = 0; i < np * np; i++) { buf[i] = 0; }

        // create a hyperslab of np x 1
        start[0] = 0;
        start[1] = np - rank - 1;
        count[0] = np;
        count[1] = 1;
        err      = H5Sselect_hyperslab (sid, H5S_SELECT_SET, start, NULL, count, NULL);
        CHECK_ERR (err)

        // read from dataset
        err = H5Dread (did, H5T_NATIVE_INT, msid, sid, H5P_DEFAULT, buf);
        CHECK_ERR (err)

        for (i = 0; i < np; i++) {
            if (buf[i] != expected_buf_val (rank, np, i, 0, 1)) { nerrs++; }
        }
        INDEP_CHECK_ERR ((-nerrs), comm);
        DEBUG_PRINT (comm);
    }

    {  // test4: each process read entire dataset
        // reset buffer
        for (i = 0; i < np * np; i++) { buf[i] = 0; }

        // create the entire region as hyperslab
        err = H5Sselect_all (sid);
        CHECK_ERR (err)

        dims[1] = np * np;
        msid2   = H5Screate_simple (1, dims + 1, dims + 1);
        CHECK_ERR (msid);

        // read from dataset
        err = H5Dread (did, H5T_NATIVE_INT, msid2, sid, H5P_DEFAULT, buf);
        CHECK_ERR (err)

        for (i = 0; i < np * np; i++) {
            if (buf[i] != ((i / np) * 100 + (i % np))) { nerrs++; }
        }
        INDEP_CHECK_ERR ((-nerrs), comm);
        DEBUG_PRINT (comm);
    }

err_out:;
    if (buf) free (buf);
    if (msid >= 0) H5Sclose (msid);
    if (msid2 >= 0) H5Sclose (msid2);
    if (sid >= 0) H5Sclose (sid);
    if (did >= 0) H5Dclose (did);
    if (fapl_id >= 0) H5Pclose (fapl_id);
    if (fcpl_id != H5P_DEFAULT) H5Pclose (fcpl_id);
    if (log_vlid != H5I_INVALID_HID) H5VLclose (log_vlid);
    if (fid >= 0) H5Fclose (fid);

    return -nerrs;
}

int create_subfile (const char *file_name, int rank, int np, vol_env *env_ptr) {
    herr_t err = 0;
    int i, nerrs = 0, nsubfiles;
    hid_t fid = -1, did = -1, sid = -1, msid = -1;
    hid_t fapl_id = -1, fcpl_id = H5P_DEFAULT, log_vlid = H5I_INVALID_HID;
    hsize_t dims[2] = {0, 0}, start[2], count[2];
    int *buf        = NULL;
    char *env_str;

    CHECK_ERR (np);
    buf = (int *)malloc (sizeof (int) * np);

    fapl_id = H5Pcreate (H5P_FILE_ACCESS);
    CHECK_ERR (fapl_id)
    // MPI and collective metadata is required by LOG VOL
    err = H5Pset_fapl_mpio (fapl_id, MPI_COMM_WORLD, MPI_INFO_NULL);
    CHECK_ERR (err)
    err = H5Pset_all_coll_metadata_ops (fapl_id, 1);
    CHECK_ERR (err)

    if (env_ptr->native_only == 0 && env_ptr->connector == 0) {
        // Register LOG VOL plugin
        log_vlid = H5VLregister_connector (&H5VL_log_g, H5P_DEFAULT);
        CHECK_ERR (log_vlid)
        err = H5Pset_vol (fapl_id, log_vlid, NULL);
        CHECK_ERR (err)
    }

    env_str = getenv ("H5VL_LOG_NSUBFILES");
    if (env_str == NULL) {
        /* set the number of subfiles */
        fcpl_id = H5Pcreate (H5P_FILE_CREATE);
        CHECK_ERR (fcpl_id)
        nsubfiles = np / 2;
        if (nsubfiles == 0) nsubfiles = -1;
        err = H5Pset_subfiling (fcpl_id, nsubfiles);
        CHECK_ERR (err)
    }

    // Create file
    fid = H5Fcreate (file_name, H5F_ACC_TRUNC, fcpl_id, fapl_id);
    CHECK_ERR (fid)

    // Create a dataset of 2D array of size np x np
    dims[0] = np;
    dims[1] = np;
    sid     = H5Screate_simple (2, dims, dims);
    CHECK_ERR (sid);
    did = H5Dcreate2 (fid, "D", H5T_STD_I32LE, sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    CHECK_ERR (did)

    for (i = 0; i < np; i++) { buf[i] = expected_buf_val (rank, np, i, 1, 0); }

    // create a hyperslab of 1 x np
    start[0] = rank;
    start[1] = 0;
    count[0] = 1;
    count[1] = np;
    err      = H5Sselect_hyperslab (sid, H5S_SELECT_SET, start, NULL, count, NULL);
    CHECK_ERR (err)

    msid = H5Screate_simple (1, dims + 1, dims + 1);
    CHECK_ERR (msid);

    // Write to dataset in parallel
    err = H5Dwrite (did, H5T_NATIVE_INT, msid, sid, H5P_DEFAULT, buf);
    CHECK_ERR (err)

err_out:;
    if (buf) free (buf);
    if (msid >= 0) H5Sclose (msid);
    if (sid >= 0) H5Sclose (sid);
    if (did >= 0) H5Dclose (did);
    if (fapl_id >= 0) H5Pclose (fapl_id);
    if (fcpl_id != H5P_DEFAULT) H5Pclose (fcpl_id);
    if (log_vlid != H5I_INVALID_HID) H5VLclose (log_vlid);
    if (fid >= 0) H5Fclose (fid);

    return -nerrs;
}