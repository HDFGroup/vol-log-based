#pragma once

#include <mpi.h>

#include "h5lreplay_meta.hpp"

typedef struct h5lreplay_write_req {
	char *buf;
	hsize_t *start;
	hsize_t *count;
	dset_info *info;
	bool operator< (const h5lreplay_write_req &rhs) const;
} h5lreplay_write_req;

herr_t h5lreplay_read_data (MPI_File fin,
							std::vector<dset_info> &dsets,
							std::vector<h5lreplay_idx_t> &reqs);

herr_t h5lreplay_write_data (MPI_File foutid,
							 std::vector<dset_info> &dsets,
							 std::vector<h5lreplay_idx_t> &reqs);

herr_t h5lreplay_write_data_gen_types (std::vector<h5lreplay_write_req> blocks,
									   MPI_Datatype *ftype,
									   MPI_Datatype *mtype);