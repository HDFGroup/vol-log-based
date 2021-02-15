#pragma once

#include <hdf5.h>

#include <vector>

typedef struct meta_sec {
	int start;
	int end;
	int off;
	int stride;
	char *buf;
} meta_sec;

typedef struct meta_sel {
	hsize_t start[H5S_MAX_RANK];
	hsize_t count[H5S_MAX_RANK];
	char *buf;
}meta_sel;

typedef struct meta_block {
	int did;
	std::vector<meta_sel> sels;
	MPI_Offset foff;
	size_t fsize;
	char *buf;
} meta_block;

herr_t h5replay_parse_meta (int rank, int np, hid_t lgid, int nmdset, std::vector<dset_info> &dsets, std::vector<std::vector<meta_block>> &reqs);