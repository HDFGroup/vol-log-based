/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#pragma once

#include <mpi.h>

#include "h5lreplay_meta.hpp"

void h5lreplay_read_data (MPI_File fin,
                          std::vector<dset_info> &dsets,
                          std::vector<h5lreplay_idx_t> &reqs);

void h5lreplay_write_data (hid_t foutid,
                           std::vector<dset_info> &dsets,
                           std::vector<h5lreplay_idx_t> &reqs);
