/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#pragma once
//
#include <vector>
//
#include <H5VLconnector.h>
#include <hdf5.h>
//
#include "H5VL_logi_err.hpp"

#define LOGVOL_FILTER_NAME_MAX 0x40
typedef struct H5VL_log_filter_t {
    H5Z_filter_t id;
    unsigned int flags;
    size_t cd_nelmts;
    std::vector<unsigned int> cd_values;
    char name[LOGVOL_FILTER_NAME_MAX];
    unsigned int filter_config;

    H5VL_log_filter_t ();
} H5VL_log_filter_t;

typedef std::vector<H5VL_log_filter_t> H5VL_log_filter_pipeline_t;

void H5VL_logi_filter (
    H5VL_log_filter_pipeline_t &pipeline, void *in, int in_len, void **out, int *out_len);

void H5VL_logi_unfilter (
    H5VL_log_filter_pipeline_t &pipeline, void *in, int in_len, void **out, int *out_len);

inline void H5VL_logi_get_filters (hid_t dcplid, std::vector<H5VL_log_filter_t> &filters) {
    int i;
    int nfilter;  // Number of filters in dcplid

    nfilter = H5Pget_nfilters (dcplid);
    CHECK_ID (nfilter);
    filters.resize (nfilter);
    for (i = 0; i < nfilter; i++) {
        filters[i].id =
            H5Pget_filter2 (dcplid, (unsigned int)i, &(filters[i].flags), &(filters[i].cd_nelmts),
                            filters[i].cd_values.data (), LOGVOL_FILTER_NAME_MAX, filters[i].name,
                            &(filters[i].filter_config));
        CHECK_ID (filters[i].id);

        // In case there are more cd_values, enlarge the array and get filter again
        if (filters[i].cd_nelmts > filters[i].cd_values.size ()) {
            filters[i].cd_values.resize (filters[i].cd_nelmts);
            filters[i].id = H5Pget_filter2 (dcplid, (unsigned int)i, &(filters[i].flags),
                                            &(filters[i].cd_nelmts), filters[i].cd_values.data (),
                                            LOGVOL_FILTER_NAME_MAX, filters[i].name,
                                            &(filters[i].filter_config));
            CHECK_ID (filters[i].id);
        }

        LOG_VOL_ASSERT (filters[i].cd_nelmts <= filters[i].cd_values.size ())
    }
}
