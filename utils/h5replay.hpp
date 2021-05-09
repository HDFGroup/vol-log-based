#pragma once

#include <hdf5.h>
#include <string>
#include "H5VL_logi_meta.hpp"
#include "H5VL_logi_filter.hpp"
#include "H5VL_logi_err.hpp"

typedef struct dset_info {
	int ndim;
	hid_t id;
	hsize_t esize;
	hid_t type;
	std::vector<H5VL_log_filter_t> filters;
} dset_info;

herr_t h5replay_core (std::string &inpath, std::string &outpath, int rank, int np) ;

