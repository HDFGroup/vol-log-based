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

typedef struct h5replay_copy_handler_arg {
    hid_t fid;
    hid_t *dids;
}h5replay_copy_handler_arg;

herr_t h5replay_core (std::string &inpath, std::string &outpath) ;

herr_t h5replay_copy_handler (hid_t o_id,
								   const char *name,
								   const H5O_info_t *object_info,
								   void *op_data);

herr_t h5replay_attr_copy_handler( hid_t location_id, const char *attr_name, const H5A_info_t *ainfo, void *op_data);