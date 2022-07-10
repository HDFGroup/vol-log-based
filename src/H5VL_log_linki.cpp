/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "H5VL_log_linki.hpp"

herr_t H5VL_log_linki_iterate_op (hid_t group,
                                  const char *name,
                                  const H5L_info2_t *info,
                                  void *op_data) {
    H5VL_log_linki_iterate_op_data *ctx = (H5VL_log_linki_iterate_op_data *)op_data;

    // Skip internal objects
    if (name) {
        if (name[0] == '_') {
            if (name[1] == '_')
                name++;
            else {
                return 0;
            }
        }
        return ctx->op (group, name, info, ctx->op_data);
    }

    return 0;
}
