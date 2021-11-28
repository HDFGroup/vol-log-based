#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
//
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
//
#include <hdf5.h>
//
#include "H5VL_log_dataset.hpp"

herr_t h5ldump_file (std::string path, std::vector<H5VL_log_dset_info_t> &dsets, int indent);
herr_t h5ldump_mdset (hid_t lgid, std::string name, std::vector<H5VL_log_dset_info_t> &dsets, int indent);
herr_t h5ldump_visit (std::string path, std::vector<H5VL_log_dset_info_t> &dsets);
herr_t h5ldump_mdsec (uint8_t *buf, size_t len, std::vector<H5VL_log_dset_info_t> &dsets, int indent);