#pragma once

#include <hdf5.h>
#include <mpi.h>

#include "H5VL_logi_nb.hpp"

struct H5VL_log_dset_info_t;
typedef struct H5VL_log_idx_search_ret_t {
	H5VL_log_dset_info_t *info;
	int dsize[H5S_MAX_RANK];   // Size of the data block in the log entry
	int dstart[H5S_MAX_RANK];  // Starting coordinate of the intersection in the log entry
	int msize[H5S_MAX_RANK];   // Size of the selection block in the memory space
	int mstart[H5S_MAX_RANK];  // Starting coordinate of the intersection in the selected block of
							   // the memory space
	int count[H5S_MAX_RANK];   // Size of the intersection
	MPI_Offset xsize;		   // Data size after unfiltering
	char *zbuf = NULL;		   // Buffer to the decompressed buf
	MPI_Offset doff;		   // Relative offset of the target data in the unfiltered data block
	char *xbuf;				   // Where the data should go
	MPI_Offset foff;		   // File offset of the data block containing the data
	MPI_Offset fsize;		   // File offset nad size of the filtered data

	bool operator< (const H5VL_log_idx_search_ret_t &rhs) const;
} H5VL_log_idx_search_ret_t;

typedef struct H5VL_log_metaentry_t {
	int did;					  // Dataset ID
	hsize_t start[H5S_MAX_RANK];  // Start of the selected block
	hsize_t count[H5S_MAX_RANK];  // Size of the selected block
	MPI_Offset foff;			  // Offset of data in file
	size_t fsize;				  // Size of data in file
} H5VL_log_metaentry_t;

class H5VL_logi_idx_t {
   public:
	virtual herr_t clear ()																	  = 0;
	virtual herr_t insert (H5VL_logi_metablock_t &meta)										  = 0;
	virtual herr_t search (H5VL_log_rreq_t *req, std::vector<H5VL_log_idx_search_ret_t> &ret) = 0;
};

class H5VL_logi_array_idx_t : public H5VL_logi_idx_t {
	std::vector<H5VL_logi_metablock_t> entries;

   public:
	herr_t clear ();
	herr_t insert (H5VL_logi_metablock_t &meta);
	herr_t search (H5VL_log_rreq_t *req, std::vector<H5VL_log_idx_search_ret_t> &ret);
};