/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#pragma once

#include <hdf5.h>
#include <mpi.h>

typedef struct H5VL_log_atti_iterate_op_data {
    H5A_operator2_t op;
    void *op_data;
} H5VL_log_atti_iterate_op_data;

herr_t H5VL_log_atti_iterate_op (hid_t location_id,
                                 const char *attr_name,
                                 const H5A_info_t *ainfo,
                                 void *op_data);
