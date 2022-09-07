/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#pragma once

#include <H5VLconnector.h>

#include "H5VL_log.h"

extern void *H5VL_log_info_copy (const void *_info);
extern herr_t H5VL_log_info_cmp (int *cmp_value, const void *_info1, const void *_info2);
extern herr_t H5VL_log_info_free (void *_info);
extern herr_t H5VL_log_info_to_str (const void *_info, char **str);
extern herr_t H5VL_log_str_to_info (const char *str, void **_info);
