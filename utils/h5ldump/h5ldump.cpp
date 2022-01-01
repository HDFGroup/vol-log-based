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
#include <mpi.h>
#include <unistd.h>
//
#include "H5VL_log_filei.hpp"
#include "H5VL_logi_nb.hpp"
#include "h5ldump.hpp"

int main (int argc, char *argv[]) {
	herr_t err = 0;
	int rank, np;
	int opt;
	bool dumpdata = false;					  // Dump data along with metadata
	std::string inpath;						  // Input file path
	std::vector<H5VL_log_dset_info_t> dsets;  // Dataset info

	MPI_Init (&argc, &argv);
	MPI_Comm_size (MPI_COMM_WORLD, &np);
	MPI_Comm_rank (MPI_COMM_WORLD, &rank);

	if (np > 1) {
		if (rank == 0) { std::cout << "Warning: h5ldump is sequential" << std::endl; }
		return 0;
	}

	// Parse input
	while ((opt = getopt (argc, argv, "hi:o:d")) != -1) {
		switch (opt) {
			case 'd':
				dumpdata = true;
				break;
			case 'h':
			default:
				if (rank == 0) { std::cout << "Usage: h5ldump <input file path>" << std::endl; }
				return 0;
		}
	}

	if (optind >= argc || argv[optind] == NULL) { /* input file is mandatory */
		if (rank == 0) { std::cout << "Usage: h5ldump <input file path>" << std::endl; }
		return 0;
	}
	inpath = std::string (argv[optind]);

	// Get dataaset metadata
	err = h5ldump_visit (inpath, dsets);
	CHECK_ERR

	// Dump the logs
	err = h5ldump_file (inpath, dsets, dumpdata, 0);
	CHECK_ERR

	// Cleanup dataset contec
	for (auto &d : dsets) {
		if (d.dtype != -1) { H5Tclose (d.dtype); }
	}

err_out:;
	return 0;
}

herr_t h5ldump_file (std::string path,
					 std::vector<H5VL_log_dset_info_t> &dsets,
					 bool dumpdata,
					 int indent) {
	herr_t err = 0;
	int mpierr;
	int i;
	MPI_File fh		 = MPI_FILE_NULL;
	hid_t fid		 = -1;	// File ID
	hid_t faplid	 = -1;	// File access property ID
	hid_t nativevlid = -1;	// Native VOL ID
	hid_t lgid		 = -1;	// Log group ID
	hid_t aid		 = -1;	// File attribute ID
	int ndset;				// Number of user datasets
	int nldset;				// Number of data datasets
	int nmdset;				// Number of metadata datasets
	int nsubfile;			// Number of subfiles
	int config;				// File config flags
	int att_buf[5];			// attirbute buffer

	// Always use native VOL
	nativevlid = H5VLpeek_connector_id_by_name ("native");
	faplid	   = H5Pcreate (H5P_FILE_ACCESS);
	H5Pset_vol (faplid, nativevlid, NULL);

	// Open the input file
	fid = H5Fopen (path.c_str (), H5F_ACC_RDONLY, faplid);
	CHECK_ID (fid)

	if (dumpdata) {
		mpierr =
			MPI_File_open (MPI_COMM_SELF, path.c_str (), MPI_MODE_RDONLY, MPI_INFO_NULL, &(fh));
		CHECK_MPIERR
	}

	// Read file metadata
	aid = H5Aopen (fid, "_int_att", H5P_DEFAULT);
	if (!aid) {
		std::cout << "Error: " << path << " is not a valid log-based VOL file." << std::endl;
		std::cout << "Use h5ldump in HDF5 utilities to read traditional HDF5 files." << std::endl;
		ERR_OUT ("File not recognized")
	}
	err = H5Aread (aid, H5T_NATIVE_INT, att_buf);
	CHECK_ERR
	ndset	 = att_buf[0];
	nldset	 = att_buf[1];
	nmdset	 = att_buf[2];
	config	 = att_buf[3];
	nsubfile = att_buf[4];

	std::cout << std::string (indent, ' ') << "File: " << path << std::endl;
	indent += 4;
	std::cout << std::string (indent, ' ') << "File properties: " << std::endl;
	indent += 4;
	std::cout << std::string (indent, ' ') << "Automatic write reqeusts merging: "
			  << ((config & H5VL_FILEI_CONFIG_METADATA_MERGE) ? "on" : "off") << std::endl;
	std::cout << std::string (indent, ' ')
			  << "Metadata encoding: " << ((config & H5VL_FILEI_CONFIG_SEL_ENCODE) ? "on" : "off")
			  << std::endl;
	std::cout << std::string (indent, ' ') << "Metadata compression: "
			  << ((config & H5VL_FILEI_CONFIG_SEL_DEFLATE) ? "on" : "off") << std::endl;
	std::cout << std::string (indent, ' ') << "Metadata deduplication: "
			  << ((config & H5VL_FILEI_CONFIG_METADATA_SHARE) ? "on" : "off") << std::endl;
	std::cout << std::string (indent, ' ')
			  << "Subfiling: " << ((config & H5VL_FILEI_CONFIG_SUBFILING) ? "on" : "off")
			  << std::endl;
	std::cout << std::string (indent, ' ') << "Number of user datasets: " << ndset << std::endl;
	std::cout << std::string (indent, ' ') << "Number of data datasets: " << nldset << std::endl;
	std::cout << std::string (indent, ' ') << "Number of metadata datasets: " << nmdset
			  << std::endl;
	std::cout << std::string (indent, ' ') << "Number of metadata datasets: " << nmdset
			  << std::endl;
	if (config & H5VL_FILEI_CONFIG_SUBFILING) {
		std::cout << std::string (indent, ' ') << "Number of subfiles: " << nsubfile << std::endl;
	}
	indent -= 4;

	if (config & H5VL_FILEI_CONFIG_SUBFILING) {
		for (i = 0; i < nsubfile; i++) {
			std::cout << std::string (indent, ' ') << "Subfile " << i << std::endl;
			err = h5ldump_file (path + ".subfiles/" + std::to_string (i) + ".h5", dsets, dumpdata,
								indent + 4);
			CHECK_ERR
			// std::cout << std::string (indent, ' ') << "End subfile " << i << std::endl;
		}
	} else {
		// Open the log group
		lgid = H5Gopen2 (fid, "_LOG", H5P_DEFAULT);
		CHECK_ID (lgid)
		if (lgid < 0) {
			std::cout << "Error: " << path << " is not a valid log-based VOL file." << std::endl;
			std::cout << "Use h5ldump in HDF5 utilities to read traditional HDF5 files."
					  << std::endl;
			ERR_OUT ("File not recognized")
		}
		// Iterate through metadata datasets
		for (i = 0; i < nmdset; i++) {
			std::cout << std::string (indent, ' ') << "Metadata dataset " << i << std::endl;
			err = h5ldump_mdset (lgid, "__md_" + std::to_string (i), dsets, fh, indent + 4);
			CHECK_ERR
			// std::cout << std::string (indent, ' ') << "End metadata dataset " << i << std::endl;
		}
	}
	indent -= 4;
	;
	// std::cout << std::string (indent, ' ') << "End file: " << path << std::endl;

err_out:;
	if (fh != MPI_FILE_NULL) { MPI_File_close (&fh); }
	if (aid >= 0) { H5Aclose (aid); }
	if (lgid >= 0) { H5Gclose (lgid); }
	if (fid >= 0) { H5Fclose (fid); }
	if (faplid >= 0) { H5Pclose (faplid); }
	return err;
}
