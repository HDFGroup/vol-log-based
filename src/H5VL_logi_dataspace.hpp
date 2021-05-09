#pragma once

#include <hdf5.h>
#include <vector>

typedef struct H5VL_log_selection {
	hsize_t start[H5S_MAX_RANK];	 // Start of selection
	hsize_t count[H5S_MAX_RANK];	 // Count of selection
	size_t size;					 // Size of the selection (bytes)
} H5VL_log_selection;

typedef struct H5VL_log_selection2 {
	MPI_Offset start;	 // Start of selection
	MPI_Offset end;	 // Count of selection
	size_t size;					 // Size of the selection (bytes)
} H5VL_log_selection2;

extern int H5VL_log_dataspace_contig_ref;
extern hid_t H5VL_log_dataspace_conti;

// Dataspace internals
extern herr_t H5VL_logi_get_dataspace_selection (hid_t sid, std::vector<H5VL_log_selection> &sels);
extern herr_t H5VL_logi_get_dataspace_sel_type (hid_t sid, size_t esize, MPI_Datatype *type);