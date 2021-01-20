#pragma once

#include <hdf5.h>

typedef struct meta_sec {
	int start;
	int end;
	int off;
	int stride;
	char *buf;
} meta_sec;

typedef struct req_block {
	int did;
	hsize_t start[H5S_MAX_RANK];
	hsize_t count[H5S_MAX_RANK];
	MPI_Offset foff;
	size_t fsize;
	char *buf;
} req_block;

herr_t h5replay_parse_meta (int rank, int np, hid_t lgid, int nmdset, std::vector<dset_info> &dsets, std::vector<req_block> &reqs);