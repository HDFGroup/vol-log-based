/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#pragma once

#include <H5VLconnector.h>

herr_t H5VL_log_introspect_get_conn_cls (void *obj,
                                         H5VL_get_conn_lvl_t lvl,
                                         const H5VL_class_t **conn_cls);
herr_t H5VL_log_introspect_get_cap_flags (const void *info, uint64_t *cap_flags);
herr_t H5VL_log_introspect_opt_query (void *obj,
                                      H5VL_subclass_t cls,
                                      int opt_type,
                                      uint64_t *flags);
