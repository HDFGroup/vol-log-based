#include <hdf5.h>
#include <mpi.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "testutils.hpp"

#define N 10

int main (int argc, char **argv) {
	int err, nerrs = 0;
	int rank, np;
	const char *file_name;
	hid_t fid;
	hid_t faplid;
	char volname[128]={0};			  // Name of current VOL
	ssize_t volname_len;		  // Length of volname
	std::string vol_name;		  // Name of the VOL used
	std::string target_vol_name;  // Name of the specified VOL
	char *env;					  // HDF5_VOL_CONNECTOR environment variable

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
	SHOW_TEST_INFO ("Creating files")

	faplid = H5Pcreate (H5P_FILE_ACCESS);
	// MPI and collective metadata is required by LOG VOL
	H5Pset_fapl_mpio (faplid, MPI_COMM_WORLD, MPI_INFO_NULL);
	H5Pset_all_coll_metadata_ops (faplid, 1);

	// Create file
	fid = H5Fcreate (file_name, H5F_ACC_TRUNC, H5P_DEFAULT, faplid);
	CHECK_ERR (fid)

	// Check VOL name
	env = getenv ("HDF5_VOL_CONNECTOR");
	if (env) {
		target_vol_name = std::string (strtok (env, " "));
	} else {
		target_vol_name = "native";
	}
	volname_len			 = H5VLget_connector_name (fid, volname, 128);
	volname[volname_len] = '\0';
	vol_name			 = std::string (volname);
	EXP_VAL (vol_name, target_vol_name)

	err = H5Fclose (fid);
	CHECK_ERR (err)

	err = H5Pclose (faplid);
	CHECK_ERR (err)

err_out:
	SHOW_TEST_RESULT

	MPI_Finalize ();

	return (nerrs > 0);
}
