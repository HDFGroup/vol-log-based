/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#pragma once

#include <H5VLconnector.h>

#include "H5VL_logi.hpp"

typedef struct H5VL_log_obji_iterate_op_data {
    H5O_iterate2_t op;
    void *op_data;
} H5VL_log_obji_iterate_op_data;

herr_t H5VL_log_obji_iterate_op (hid_t obj,
                                 const char *name,
                                 const H5O_info2_t *info,
                                 void *op_data);

// Internal
extern void *H5VL_log_obj_open_with_uo (void *obj,
                                        void *uo,
                                        H5I_type_t type,
                                        const H5VL_loc_params_t *loc_params);
