#pragma once

#include <hdf5.h>

#define CHECK_ERR                                                     \
	{                                                                 \
		if (err < 0) {                                                \
			printf ("Error at line %d in %s:\n", __LINE__, __FILE__); \
			H5Eprint1 (stdout);                                       \
			goto err_out;                                             \
		}                                                             \
	}

#define CHECK_MPIERR                                                             \
	{                                                                            \
		if (mpierr != MPI_SUCCESS) {                                             \
			int el = 256;                                                        \
			char errstr[256];                                                    \
			MPI_Error_string (mpierr, errstr, &el);                              \
			printf ("Error at line %d in %s: %s\n", __LINE__, __FILE__, errstr); \
			err = -1;                                                            \
			goto err_out;                                                        \
		}                                                                        \
	}

#define CHECK_ID(A)                                                   \
	{                                                                 \
		if (A < 0) {                                                  \
			printf ("Error at line %d in %s:\n", __LINE__, __FILE__); \
			H5Eprint1 (stdout);                                       \
			goto err_out;                                             \
		}                                                             \
	}


typedef struct dset_info {
	int ndim;
	hid_t id;
	hsize_t esize;
} dset_info;

herr_t h5replay_core (std::string &inpath, std::string &outpath) ;

