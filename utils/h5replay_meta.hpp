#pragma once

#include <vector>
//
#include <hdf5.h>
//
#include "H5VL_logi_meta.hpp"
#include "H5VL_logi_idx.hpp"

typedef struct meta_sec {
	int start;
	int end;
	int off;
	int stride;
	char *buf;
} meta_sec;

typedef struct meta_block : H5VL_logi_metablock_t{
	std::vector<char*>bufs;
} meta_block;

class h5replay_idx_t : public H5VL_logi_idx_t {
   public:
   	std::vector<meta_block> entries;
	herr_t clear ();
	herr_t insert (H5VL_logi_metablock_t &meta);
	herr_t search (H5VL_log_rreq_t &req, std::vector<H5VL_log_search_ret_t> &ret);
};

herr_t h5replay_parse_meta (int rank, int np, hid_t lgid, int nmdset, std::vector<dset_info> &dsets, std::vector<h5replay_idx_t> &reqs);