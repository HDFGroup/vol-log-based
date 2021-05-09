#pragma once

#include <vector>
//
#include <hdf5.h>
//
#include "H5VL_logi_meta.hpp"

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

herr_t h5replay_parse_meta (int rank, int np, hid_t lgid, int nmdset, std::vector<dset_info> &dsets, std::vector<std::vector<meta_block>> &reqs);