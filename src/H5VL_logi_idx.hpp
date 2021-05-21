#pragma once

#include <hdf5.h>

#include "H5VL_logi_nb.hpp"

struct H5VL_log_dset_info_t;
typedef struct H5VL_log_idx_search_ret_t {
	H5VL_log_dset_info_t *info;
	int dsize[H5S_MAX_RANK];
	int dstart[H5S_MAX_RANK];
	int msize[H5S_MAX_RANK];
	int mstart[H5S_MAX_RANK];
	int count[H5S_MAX_RANK];
	MPI_Offset xsize;	// Data size after unfiltering
	char *zbuf = NULL;	// Buffer to the decompressed buf
	MPI_Offset doff;	// Relative offset of the target data in the unfiltered data block
	char *xbuf;			// Where the data should go
	MPI_Offset foff;	// File offset of the data block containing the data
	MPI_Offset fsize;	// File offset nad size of the filtered data

	bool operator< (const H5VL_log_idx_search_ret_t &rhs) const;
} H5VL_log_idx_search_ret_t;

typedef struct H5VL_log_metaentry_t {
	int did;
	hsize_t start[H5S_MAX_RANK];
	hsize_t count[H5S_MAX_RANK];
	MPI_Offset foff;
	size_t fsize;
} H5VL_log_metaentry_t;

class H5VL_logi_idx_t {
   public:
	virtual herr_t clear ()																	  = 0;
	virtual herr_t insert (H5VL_logi_metablock_t &meta)										  = 0;
	virtual herr_t search (H5VL_log_rreq_t &req, std::vector<H5VL_log_idx_search_ret_t> &ret) = 0;
};

class H5VL_logi_array_idx_t : public H5VL_logi_idx_t {
	std::vector<H5VL_logi_metablock_t> entries;

   public:
	herr_t clear ();
	herr_t insert (H5VL_logi_metablock_t &meta);
	herr_t search (H5VL_log_rreq_t &req, std::vector<H5VL_log_idx_search_ret_t> &ret);
};