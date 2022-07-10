/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#include <H5VLconnector.h>

#include "H5VL_logi_filter.hpp"

void H5VL_logi_filter_deflate (
    H5VL_log_filter_t &fp, void *in, int in_len, void *out, int *out_len);
void H5VL_logi_filter_deflate_alloc (
    H5VL_log_filter_t &fp, void *in, int in_len, void **out, int *out_len);
void H5VL_logi_filter_inflate (
    H5VL_log_filter_t &fp, void *in, int in_len, void *out, int *out_len);
void H5VL_logi_filter_inflate_alloc (
    H5VL_log_filter_t &fp, void *in, int in_len, void **out, int *out_len);
