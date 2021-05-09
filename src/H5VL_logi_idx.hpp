#pragma once

#include "H5VL_logi_nb.hpp"
#include <hdf5.h>

typedef struct H5VL_log_search_ret_t {
	int ndim;
	int fsize[H5S_MAX_RANK];
	int fstart[H5S_MAX_RANK];
	int msize[H5S_MAX_RANK];
	int mstart[H5S_MAX_RANK];
	int count[H5S_MAX_RANK];
	size_t esize;
	MPI_Offset foff, moff;

	bool operator< (const H5VL_log_search_ret_t &rhs) const {
		int i;

		if (foff < rhs.foff)
			return true;
		else if (foff > rhs.foff)
			return false;

		for (i = 0; i < ndim; i++) {
			if (fstart[i] < rhs.fstart[i])
				return true;
			else if (fstart[i] > rhs.fstart[i])
				return false;
		}

		return false;
	}
} H5VL_log_search_ret_t;


typedef struct H5VL_log_metaentry_t {
	int did;
	hsize_t start[H5S_MAX_RANK];
	hsize_t count[H5S_MAX_RANK];
	MPI_Offset ldoff;
	size_t rsize;
} H5VL_log_metaentry_t;

// Index
extern herr_t H5VL_logi_idx_search (void *file,
									H5VL_log_rreq_t &req,
									std::vector<H5VL_log_search_ret_t> &ret);