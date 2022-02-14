#pragma once

#include <hdf5.h>

#include <string>

#include "H5VL_log_dataset.hpp"
#include "H5VL_logi_err.hpp"
#include "H5VL_logi_filter.hpp"
#include "H5VL_logi_meta.hpp"

typedef struct dset_info : H5VL_log_dset_info_t {
	hid_t did;
	haddr_t foff;
} dset_info;

herr_t h5lreplay_core (std::string &inpath, std::string &outpath, int rank, int np);
