/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#pragma once

#include <hdf5.h>

#include <string>

#include "H5VL_log_dataset.hpp"
#include "H5VL_logi_err.hpp"
#include "H5VL_logi_filter.hpp"
#include "H5VL_logi_meta.hpp"

typedef struct dset_info : H5VL_log_dset_info_t {
    hid_t id;
} dset_info;

void h5lreplay_core (std::string &inpath, std::string &outpath, int rank, int np);
