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
#include <dirent.h>
#include <hdf5.h>
#include <unistd.h>
//
#include "H5VL_log_file.hpp"
#include "H5VL_log_filei.hpp"
#include "H5VL_logi_nb.hpp"
#include "h5replay.hpp"
#include "h5replay_copy.hpp"
#include "h5replay_data.hpp"
#include "h5replay_meta.hpp"

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
	hid_t finid	 = -1;	// ID of the input file
	hid_t foutid = -1;	// ID of the output file
	hid_t fsubid = -1;	// ID of the subfile
	hid_t faplid = -1;
	hid_t lgid	 = -1;	// ID of the log group
	hid_t aid	 = -1;	// ID of file attribute
	hid_t dxplid = -1;
	int ndset;		 // # dataset in the input file
	int nldset;		 // # data dataset in the current file (main file| subfile)
	int nmdset;		 // # metadata dataset in the current file (main file| subfile)
	int config;		 // Config flags of the input file
	int att_buf[4];	 // Temporary buffer for reading file attributes
	h5replay_copy_handler_arg copy_arg;	 // File structure
	std::vector<h5replay_idx_t> reqs;  // Requests recorded in the current file (main file| subfile)
	MPI_File fin  = MPI_FILE_NULL;	   // file handle of the input file
	MPI_File fout = MPI_FILE_NULL;	   // file handle of the output file
	MPI_File fsub = MPI_FILE_NULL;	   // file handle of the subfile
	DIR *subdir	  = NULL;			   // subfile dir
	struct dirent *subfile;			   // subfile entry in dir

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

	reqs.resize (ndset);
	if (config & H5VL_FILEI_CONFIG_SUBFILING) {
		std::string subpath;

		// Close attr in main file
		H5Aclose (aid);
		aid = -1;

		// Iterate subdir
		subdir = opendir ((inpath + ".subfiles").c_str ());
		CHECK_PTR (subdir)
		while (subfile = readdir (subdir)) {
			subpath = std::string (subfile->d_name);
			if (subpath.substr (subpath.size () - 2) == ".h5") {
#ifdef LOGVOL_DEBUG
				if (subfile->d_type != DT_REG) {  // Not all FS support d_type
					std::cout << "Warning: file type of " << subpath << " is unknown" << std::endl;
				}
#endif
				// Open the subfile
				fsubid = H5Fopen (subpath.c_str (), H5F_ACC_RDONLY, faplid);
				if (fsubid >= 0) {
					mpierr = MPI_File_open (MPI_COMM_WORLD, subpath.c_str (), MPI_MODE_RDONLY,
											MPI_INFO_NULL, &fsub);
					CHECK_MPIERR

					// Read file metadata
					aid = H5Aopen (fsubid, "_int_att", H5P_DEFAULT);  // Open attr in subfile
					CHECK_ID (aid)
					err = H5Aread (aid, H5T_NATIVE_INT, att_buf);
					CHECK_ERR
					nldset = att_buf[1];
					nmdset = att_buf[2];

					// Open the log group
					lgid = H5Gopen2 (fsubid, LOG_GROUP_NAME, H5P_DEFAULT);
					CHECK_ID (lgid)

					// Clean up the index
					for (auto &r : reqs) { r.clear (); }

					// Read the metadata
					err =
						h5replay_parse_meta (rank, np, lgid, nmdset, copy_arg.dsets, reqs, config);
					CHECK_ERR

					// Read the data
					err = h5replay_read_data (fsub, copy_arg.dsets, reqs);
					CHECK_ERR

					// Write the data
					err = h5replay_write_data (foutid, copy_arg.dsets, reqs);
					CHECK_ERR

					// Close the subfile
					MPI_File_close (&fsub);
					fsub = MPI_FILE_NULL;
					H5Fclose (fsubid);
					fsubid = -1;
					H5Gclose (lgid);
					lgid = -1;
					H5Aclose (aid);
					aid = -1;
				}
#ifdef LOGVOL_DEBUG
				else
					std::cout << "Warning: cannot open subfile" << subpath << std::endl;
			}
#endif
		}
	} else {
		// Open the log group
		lgid = H5Gopen2 (finid, LOG_GROUP_NAME, H5P_DEFAULT);
		CHECK_ID (lgid)

		// Read the metadata
		err = h5replay_parse_meta (rank, np, lgid, nmdset, copy_arg.dsets, reqs, config);
		CHECK_ERR

		// Read the data
		err = h5replay_read_data (fin, copy_arg.dsets, reqs);
		CHECK_ERR

		// Write the data
		err = h5replay_write_data (foutid, copy_arg.dsets, reqs);
		CHECK_ERR
	}

err_out:;
	if (faplid >= 0) { H5Pclose (faplid); }
	if (dxplid >= 0) { H5Pclose (dxplid); }
	if (aid >= 0) { H5Aclose (aid); }
	if (lgid >= 0) { H5Gclose (lgid); }
	if (finid >= 0) { H5Fclose (finid); }
	if (fsubid >= 0) { H5Fclose (fsubid); }
	for (auto &dset : copy_arg.dsets) { H5Dclose (dset.id); }
	if (foutid >= 0) { H5Fclose (foutid); }
	if (fin != MPI_FILE_NULL) { MPI_File_close (&fin); }
	if (fout != MPI_FILE_NULL) { MPI_File_close (&fout); }
	if (subdir) { closedir (subdir); }
	return err;
}
