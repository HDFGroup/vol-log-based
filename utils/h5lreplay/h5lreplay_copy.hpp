/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#pragma once

#include <vector>
//
#include <hdf5.h>
//
#include "h5lreplay.hpp"

typedef struct h5lreplay_copy_handler_arg {
    hid_t fid;
    std::vector<dset_info> dsets;
} h5lreplay_copy_handler_arg;

herr_t h5lreplay_attr_copy_handler (hid_t location_id,
                                    const char *attr_name,
                                    const H5A_info_t *ainfo,
                                    void *op_data);

herr_t h5lreplay_copy_handler (hid_t o_id,
                               const char *name,
                               const H5O_info_t *object_info,
                               void *op_data);
