#pragma once

#include "h5lreplay_meta.hpp"
#include <mpi.h>

herr_t h5lreplay_read_data (MPI_File fin,
						   std::vector<dset_info> &dsets,
						   std::vector<h5lreplay_idx_t> &reqs);

herr_t h5lreplay_write_data (hid_t foutid,
							std::vector<dset_info> &dsets,
							std::vector<h5lreplay_idx_t> &reqs);