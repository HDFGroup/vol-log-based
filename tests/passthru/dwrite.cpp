/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#include <hdf5.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define N 10

#define CHECK_ERR(A)                                                           \
  {                                                                            \
    if (A < 0) {                                                               \
      printf("Error at line %d: code %ld\n", __LINE__, (long) A);              \
      goto err_out;                                                            \
      nerrs++;                                                                 \
    }                                                                          \
  }

int main(int argc, char **argv) {
  herr_t err = 0;
  int nerrs = 0;
  int i;
  int rank, np;
  int mpi_required = 0;
  const char *file_name;

  hid_t fid = -1;          // File ID
  hid_t did = -1;          // Dataset ID
  hid_t filespace_id = -1; // File space ID
  hid_t memspace_id = -1;  // Memory space ID
  hid_t faplid = -1;       // File Access Property List

  int buf[N];
  hsize_t dims[2] = {0, N}; // dims[0] will be modified later
  hsize_t start[2], count[2];

  // init MPI
  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &mpi_required);
  MPI_Comm_size(MPI_COMM_WORLD, &np);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (argc > 2) {
        if (!rank) printf ("Usage: %s [filename]\n", argv[0]);
        MPI_Finalize ();
        return 1;
    } else if (argc > 1) {
        file_name = argv[1];
    } else {
        file_name = "test.h5";
    }

  /* Set faplid to use LOG VOL */
  faplid = H5Pcreate(H5P_FILE_ACCESS);
  CHECK_ERR(faplid);

  // MPI and collective metadata is required by LOG VOL
  err = H5Pset_fapl_mpio(faplid, MPI_COMM_WORLD, MPI_INFO_NULL);
  CHECK_ERR(err);
  err = H5Pset_all_coll_metadata_ops(faplid, 1);
  CHECK_ERR(err);

  /* Create file using LOG VOL. */
  fid = H5Fcreate(file_name, H5F_ACC_TRUNC, H5P_DEFAULT, faplid);
  CHECK_ERR(fid);

  /* Create 2D (num_process x N) datasets */
  dims[0] = np;
  filespace_id = H5Screate_simple(2, dims, dims);
  CHECK_ERR(filespace_id);
  did = H5Dcreate(fid, "D", H5T_STD_I32LE, filespace_id, H5P_DEFAULT,
                  H5P_DEFAULT, H5P_DEFAULT);
  CHECK_ERR(did);

  /* Prepare data */
  for (i = 0; i < N; i++) {
    buf[i] = rank * 100 + i;
  }
  start[0] = rank;
  start[1] = 0;
  count[0] = 1;
  count[1] = N;

  /* File space setting */
  err = H5Sselect_hyperslab(filespace_id, H5S_SELECT_SET, start, NULL, count,
                            NULL);
  CHECK_ERR(err);

  /* Mem space setting */
  memspace_id = H5Screate_simple(1, dims + 1, dims + 1);
  CHECK_ERR(memspace_id);

  /* Request to write data */
  err = H5Dwrite(did, H5T_NATIVE_INT, memspace_id, filespace_id, H5P_DEFAULT,
                 buf);
  CHECK_ERR(err);

  /* Close everything */
err_out:;
  if (memspace_id >= 0)
    H5Sclose(memspace_id);
  if (filespace_id >= 0)
    H5Sclose(filespace_id);
  if (did >= 0)
    H5Dclose(did);
  if (fid >= 0)
    H5Fclose(fid);
  if (faplid >= 0)
    H5Pclose(faplid);

  MPI_Finalize();

  return (nerrs > 0);
}
