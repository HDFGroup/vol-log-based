/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
//
#include <mpi.h>
//
#include "H5VL_log_dataset.hpp"
#include "H5VL_logi_idx.hpp"
//
// Order of the first element
bool H5VL_log_idx_search_ret_t::operator< (const H5VL_log_idx_search_ret_t &rhs) const {
    int i;

    if (foff + doff < rhs.foff + rhs.doff)
        return true;
    else if (foff + doff > rhs.foff + rhs.doff)
        return false;

    for (i = 0; i < (int)(info->ndim); i++) {
        if (dstart[i] < rhs.dstart[i])
            return true;
        else if (dstart[i] > rhs.dstart[i])
            return false;
    }

    return false;
}

// Order of the last element
bool H5VL_log_idx_search_ret_t::operator> (const H5VL_log_idx_search_ret_t &rhs) const {
    int i;

    // NOTE: > operator is only correct when foff and doff is the same
    /*
    if (foff + doff > rhs.foff + rhs.doff)
        return true;
    else if (foff + doff > rhs.foff + rhs.doff)
        return false;
    */

    for (i = 0; i < (int)(info->ndim); i++) {
        if (dstart[i] + count[i] > rhs.dstart[i] + rhs.count[i])
            return true;
        else if (dstart[i] + count[i] > rhs.dstart[i] + rhs.count[i])
            return false;
    }

    return false;
}

H5VL_logi_idx_t::H5VL_logi_idx_t (H5VL_log_file_t *fp) : fp (fp) {}
