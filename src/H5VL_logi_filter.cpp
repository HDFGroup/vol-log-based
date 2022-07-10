/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
#include <vector>

#include "H5VL_logi_err.hpp"
#include "H5VL_logi_filter.hpp"
#include "H5VL_logi_filter_deflate.hpp"
#include "H5VL_logi_mem.hpp"
#include "hdf5.h"

void H5VL_logi_filter (
    H5VL_log_filter_pipeline_t &pipeline, void *in, int in_len, void **out, int *out_len) {
    int i;
    int bsize[2];  // Size of buf
    int size_in;   // Size of bin
    int size_out;  // Size of bout
    char *buf[2];  // Buffer for intermediate results in the filter pipeline
    char *bin;     // Current input
    char *bout;    // Current output
    H5VL_logi_err_finally finally ([&buf] () -> void {
        H5VL_log_free (buf[0]);
        H5VL_log_free (buf[1]);
    });

    bsize[0] = bsize[1] = 0;
    buf[0] = buf[1] = NULL;

    for (i = 0; i < (int)(pipeline.size ()); i++) {
        // Alternate input and output buffer
        bin      = buf[i & 1];
        bout     = buf[(i + 1) & 1];
        size_out = bsize[(i + 1) & 1];

        // First iteration read from user buffer
        if (i == 0) {
            bin     = (char *)in;
            size_in = in_len;
        }
        // Last iteration write to user buffer
        if (i == (int)(pipeline.size ()) - 1) {
            bout     = (char *)*out;
            size_out = *out_len;
        }

        switch (pipeline[i].id) {
            case H5Z_FILTER_DEFLATE: {
                H5VL_logi_filter_deflate_alloc (pipeline[i], bin, size_in, (void **)&bout,
                                                &size_out);
            } break;
            default:
                ERR_OUT ("Filter not supported")
                break;
        }

        if (i == (int)(pipeline.size ()) - 1) {
            *out_len = size_out;
            *out     = bout;
        } else {
            if (bsize[(i + 1) & 1] < size_out) { bsize[(i + 1) & 1] = size_out; }
            buf[(i + 1) & 1] = bout;
            size_in          = size_out;
        }
    }
}

void H5VL_logi_unfilter (
    H5VL_log_filter_pipeline_t &pipeline, void *in, int in_len, void **out, int *out_len) {
    int i;
    int bsize[2];  // Size of buf
    int size_in;   // Size of bin
    int size_out;  // Size of bout
    char *buf[2];  // Buffer for intermediate results in the filter pipeline
    char *bin;     // Current input
    char *bout;    // Current output
    H5VL_logi_err_finally finally ([&buf] () -> void {
        H5VL_log_free (buf[0]);
        H5VL_log_free (buf[1]);
    });

    bsize[0] = bsize[1] = 0;
    buf[0] = buf[1] = NULL;

    for (i = pipeline.size () - 1; i > -1; i--) {
        // Alternate input and output buffer
        bin      = buf[i & 1];
        bout     = buf[(i + 1) & 1];
        size_out = bsize[(i + 1) & 1];

        // First iteration read from user buffer
        if (i == 0) {
            bout     = (char *)*out;
            size_out = *out_len;
        }
        // Last iteration write to user buffer
        if (i == (int)(pipeline.size ()) - 1) {
            bin     = (char *)in;
            size_in = in_len;
        }

        switch (pipeline[i].id) {
            case H5Z_FILTER_DEFLATE: {
                H5VL_logi_filter_deflate_alloc (pipeline[i], bin, size_in, (void **)&bout,
                                                &size_out);
            } break;
            default:
                ERR_OUT ("Filter not supported")
                break;
        }

        if (i == 0) {
            *out     = bout;
            *out_len = size_out;
        } else {
            buf[(i + 1) & 1] = bout;
            if (bsize[(i + 1) & 1] < size_out) { bsize[(i + 1) & 1] = size_out; }
            size_in = size_out;
        }
    }
}

H5VL_log_filter_t::H5VL_log_filter_t () { this->cd_nelmts = 0; }
