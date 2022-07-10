/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#pragma once

#include <H5VLconnector.h>

typedef struct H5VL_log_linki_iterate_op_data {
    H5L_iterate2_t op;
    void *op_data;
} H5VL_log_linki_iterate_op_data;

herr_t H5VL_log_linki_iterate_op (hid_t group,
                                  const char *name,
                                  const H5L_info2_t *info,
                                  void *op_data);
