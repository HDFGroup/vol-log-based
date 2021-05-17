#pragma once

#include <hdf5.h>

#include "H5VL_logi_nb.hpp"

typedef struct H5VL_log_search_ret_t {
	int ndim;
	int did;
	int dsize[H5S_MAX_RANK];
	int dstart[H5S_MAX_RANK];
	int msize[H5S_MAX_RANK];
	int mstart[H5S_MAX_RANK];
	int count[H5S_MAX_RANK];
	size_t esize;
	size_t zbsize;
	char *zbuf = NULL;
	MPI_Offset moff, boff;
	MPI_Offset foff, fsize;

	bool operator< (const H5VL_log_search_ret_t &rhs) const {
		int i;

		if (foff < rhs.foff)
			return true;
		else if (foff > rhs.foff)
			return false;

		for (i = 0; i < ndim; i++) {
			if (dstart[i] < rhs.dstart[i])
				return true;
			else if (dstart[i] > rhs.dstart[i])
				return false;
		}

		return false;
	}
} H5VL_log_search_ret_t;

typedef struct H5VL_log_metaentry_t {
	int did;
	hsize_t start[H5S_MAX_RANK];
	hsize_t count[H5S_MAX_RANK];
	MPI_Offset foff;
	size_t fsize;
} H5VL_log_metaentry_t;

class H5VL_logi_idx_t {
   public:
	virtual herr_t clear ()									  = 0;
	virtual herr_t insert (H5VL_logi_metablock_t &meta)									  = 0;
	virtual herr_t search (H5VL_log_rreq_t &req, std::vector<H5VL_log_search_ret_t> &ret) = 0;
};

class H5VL_logi_array_idx_t : public H5VL_logi_idx_t {
	std::vector<H5VL_logi_metablock_t> entries;

   public:
	herr_t clear ();
	herr_t insert (H5VL_logi_metablock_t &meta);
	herr_t search (H5VL_log_rreq_t &req, std::vector<H5VL_log_search_ret_t> &ret);
};