#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
//
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
//
#include <hdf5.h>
//
#include "H5VL_log_file.hpp"
#include "H5VL_logi_nb.hpp"
#include "h5replay.hpp"
#include "h5replay_copy.hpp"
#include "h5replay_data.hpp"
#include "h5replay_meta.hpp"
#include "unistd.h"

int main (int argc, char *argv[]) {
	herr_t ret;
	int rank, np;
	int opt;
	std::string inpath, outpath;

	MPI_Init (&argc, &argv);
	MPI_Comm_size (MPI_COMM_WORLD, &np);
	MPI_Comm_rank (MPI_COMM_WORLD, &rank);

	// Parse input
	while ((opt = getopt (argc, argv, "hi:o:")) != -1) {
		switch (opt) {
			case 'i':
				inpath = std::string (optarg);
				break;
			case 'o':
				outpath = std::string (optarg);
				break;
			case 'h':
			default:
				if (rank == 0) {
					std::cout << "Usage: h5reply -i <input file path> -o <output file path>"
							  << std::endl;
				}
				return 0;
		}
	}

	ret = h5replay_core (inpath, outpath, rank, np);

	return 0;
}

herr_t h5replay_core (std::string &inpath, std::string &outpath, int rank, int np) {
	herr_t err = 0;
	int mpierr;
	int i;
	hid_t finid = -1, foutid = -1;
	hid_t faplid = -1;
	hid_t lgid	 = -1;
	hid_t aid	 = -1;
	hid_t dxplid = -1;
	int ndset;
	int nldset;
	int nmdset;
	int config;
	int att_buf[4];
	char *metabuf = NULL;
	char *databuf = NULL;
	hid_t *dids	  = NULL;
	hid_t *mdids  = NULL;
	hid_t src_did = -1;
	std::vector<dset_info> dsets;
	std::vector<MPI_Offset> mstart;
	std::vector<MPI_Offset> mend;
	std::vector<MPI_Offset> mstride;
	h5replay_copy_handler_arg copy_arg;
	std::vector<h5replay_idx_t> reqs;
	MPI_File fin  = MPI_FILE_NULL;
	MPI_File fout = MPI_FILE_NULL;

	// Open the input and output file
	faplid = H5Pcreate (H5P_FILE_ACCESS);
	CHECK_ID (faplid)
	err = H5Pset_fapl_mpio (faplid, MPI_COMM_WORLD, MPI_INFO_NULL);
	CHECK_ERR

	finid = H5Fopen (inpath.c_str (), H5F_ACC_RDONLY, faplid);
	CHECK_ID (faplid)
	foutid = H5Fcreate (outpath.c_str (), H5F_ACC_TRUNC, H5P_DEFAULT, faplid);
	CHECK_ID (foutid)
	mpierr = MPI_File_open (MPI_COMM_WORLD, inpath.c_str (), MPI_MODE_RDONLY, MPI_INFO_NULL, &fin);
	CHECK_MPIERR
	mpierr =
		MPI_File_open (MPI_COMM_WORLD, outpath.c_str (), MPI_MODE_WRONLY, MPI_INFO_NULL, &fout);
	CHECK_MPIERR

	// Read file metadata
	aid = H5Aopen (finid, "_int_att", H5P_DEFAULT);
	CHECK_ID (aid)

	dxplid = H5Pcreate (H5P_DATASET_XFER);
	CHECK_ID (dxplid)
	err = H5Pset_dxpl_mpio (dxplid, H5FD_MPIO_COLLECTIVE);
	CHECK_ERR
	err = H5Aread (aid, H5T_NATIVE_INT, att_buf);
	CHECK_ERR
	ndset  = att_buf[0];
	nldset = att_buf[1];
	nmdset = att_buf[2];
	config = att_buf[3];

	// Copy data objects
	copy_arg.dsets.resize (ndset);
	copy_arg.fid = foutid;
	err = H5Ovisit3 (finid, H5_INDEX_CRT_ORDER, H5_ITER_INC, h5replay_copy_handler, &copy_arg,
					 H5O_INFO_ALL);
	CHECK_ERR

	// Open the log group
	lgid = H5Gopen2 (finid, LOG_GROUP_NAME, H5P_DEFAULT);
	CHECK_ID (lgid)

	// Read the metadata
	reqs.resize (ndset);
	err = h5replay_parse_meta (rank, np, lgid, nmdset, copy_arg.dsets, reqs);
	CHECK_ERR

	// Read the data
	err = h5replay_read_data (fin, copy_arg.dsets, reqs);
	CHECK_ERR

	// Write the data
	err = h5replay_write_data (foutid, copy_arg.dsets, reqs);
	CHECK_ERR

err_out:;
	if (dids) { free (dids); }
	if (faplid >= 0) { H5Pclose (faplid); }
	if (dxplid >= 0) { H5Pclose (dxplid); }
	if (aid >= 0) { H5Aclose (aid); }
	if (lgid >= 0) { H5Gclose (lgid); }
	if (finid >= 0) { H5Fclose (finid); }
	for (auto &dset : copy_arg.dsets) { H5Dclose (dset.id); }
	if (foutid >= 0) { H5Fclose (foutid); }
	if (fin != MPI_FILE_NULL) { MPI_File_close (&fin); }
	if (fout != MPI_FILE_NULL) { MPI_File_close (&fout); }
	return err;
}
