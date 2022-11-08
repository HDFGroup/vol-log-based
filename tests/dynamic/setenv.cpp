/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * This program creates two files. Before creating the second file, the 
 * environment variable for default VOL is changed to pass_through (built-in 
 * pass through VOL) by the setenv function.
 * 
 * HDF5 checks the default environment variable at library initialization time,
 * so changing the environment variables should have no effect on the default 
 * VOL used on the second file. Reset all plugin paths and turn the VOL plugin 
 * off and on does not make any difference.
 *
 * The compile and run commands are given below.
 *
 *    % mpicxx -g -o setenv setenv.cpp -I../ -lhdf5 
 *
 *    % mpiexec -l -n 1 setenv setenv.nc
 *    % *** TESTING CXX    setenv: Changing VOL environment variable ---- pass
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <hdf5.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#include <cstring>

#include "H5VL_log.h"
#include "testutils.hpp"

#define N 10

int main (int argc, char **argv) {
	int err, nerrs = 0;
	int rank, np;
	unsigned int nidx;
	int i;
	char buf[1024];
	const char *file_name;
	hid_t fid	   = -1;  // File ID
	hid_t faplid   = -1;
	hid_t log_vlid = -1;  // Logvol ID
	char name1[64];
	char name2[64];

	MPI_Init (&argc, &argv);
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
	SHOW_TEST_INFO ("Changing VOL environment variable")

	faplid = H5Pcreate (H5P_FILE_ACCESS);
	CHECK_ERR (faplid)
	err = H5Pset_fapl_mpio (faplid, MPI_COMM_WORLD, MPI_INFO_NULL);
	CHECK_ERR (err)

	// Create file
	fid = H5Fcreate (file_name, H5F_ACC_TRUNC, H5P_DEFAULT, faplid);
	CHECK_ERR (fid)

	err = H5VLget_connector_name (fid, name1, 64);
	CHECK_ERR (err)

	err = H5Fclose (fid);
	CHECK_ERR (err)
	fid = -1;

	// Change env
	setenv ("HDF5_VOL_CONNECTOR", "pass_through under_vol=0;under_info={}", 1);

	// Reset all plugin path
	err = H5PLsize (&nidx);
	CHECK_ERR (err)
	for (i = 0; i < nidx; i++) {
		err = H5PLget (i, buf, 1024);
		CHECK_ERR (err)
		err = H5PLreplace (buf, i);
		CHECK_ERR (err)
	}

	// Turn VOL plugin off and on
	err= H5PLget_loading_state(&nidx);
	CHECK_ERR(err)

	nidx &=~(H5PL_VOL_PLUGIN);
	err= H5PLset_loading_state(nidx&(~(H5PL_VOL_PLUGIN)));
	CHECK_ERR(err)

	err= H5PLset_loading_state(nidx);
	CHECK_ERR(err)

	// Open file
	fid = H5Fopen (file_name, H5F_ACC_RDONLY, faplid);
	CHECK_ERR (fid)

	err = H5VLget_connector_name (fid, name2, 64);
	CHECK_ERR (err)

	err = H5Fclose (fid);
	CHECK_ERR (err)
	fid = -1;

	EXP_VAL (std::string (name1), std::string (name2))

err_out:
	if (fid >= 0) H5Fclose (fid);
	if (faplid >= 0) H5Pclose (faplid);
	if (log_vlid >= 00) H5VLclose (log_vlid);

	SHOW_TEST_RESULT

	MPI_Finalize ();

	return (nerrs > 0);
}
