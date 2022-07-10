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
#include "H5VL_logi_idx.hpp"
#include "H5VL_logi_meta.hpp"

typedef struct meta_sec {
    int start;
    int end;
    int off;
    int stride;
    char *buf;
} meta_sec;

typedef struct meta_block : H5VL_logi_metaentry_t {
    std::vector<char *> bufs;
} meta_block;

class h5lreplay_idx_t : public H5VL_logi_idx_t {
   public:
    h5lreplay_idx_t ();
    ~h5lreplay_idx_t () = default;
    std::vector<meta_block> entries;
    void clear ();                              // Remove all entries
    void reserve (size_t size);                 // Make space for at least size datasets
    void insert (H5VL_logi_metaentry_t &meta);  // Add an entry
    void parse_block (char *block,
                      size_t size);  // Parse a block of encoded metadata and insert all entries
    void search (H5VL_log_rreq_t *req,
                 std::vector<H5VL_log_idx_search_ret_t> &ret);  // Search for matchings
};

void h5lreplay_parse_meta (int rank,
                           int np,
                           hid_t lgid,
                           int nmdset,
                           std::vector<dset_info> &dsets,
                           std::vector<h5lreplay_idx_t> &reqs,
                           int config);
