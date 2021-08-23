#pragma once

#include <hdf5.h>

#include <vector>

class H5VL_log_selections {
   public:
	hsize_t **starts;  // Start of selection
	hsize_t **counts;  // Count of selection
	int nsel;			   // Size of the selection (bytes)
	int ndim;

	H5VL_log_selections(int ndim, int nsel);
	H5VL_log_selections(int ndim, int nsel, hsize_t **starts, hsize_t **counts);
	H5VL_log_selections (hid_t dsid);
	H5VL_log_selections (H5VL_log_selections &rhs);
	~H5VL_log_selections();

	herr_t get_mpi_type(size_t esize, MPI_Datatype *type); 
	hsize_t get_sel_size(); 
	hsize_t get_sel_size(int i); 

private:	
	hsize_t *starts_arr;	// starts[0] can be swapped in sorting, save the allocated array to free it later
	bool isdeep;	// Is a deep copy, (need free of starts and counts)

	void reserve(int nsel);	// Allocate space for nsel blocks
	void convert_to_deep();	// Coverts a shallow copy to deep copy
};

typedef struct H5VL_log_selection {
	hsize_t start[H5S_MAX_RANK];	 // Start of selection
	hsize_t count[H5S_MAX_RANK];	 // Count of selection
	size_t size;					 // Size of the selection (bytes)
} H5VL_log_selection;

typedef struct H5VL_log_selection2 {
	MPI_Offset start;  // Start of selection
	MPI_Offset end;	   // Count of selection
	size_t size;	   // Size of the selection (bytes)
} H5VL_log_selection2;

extern int H5VL_log_dataspace_contig_ref;
extern hid_t H5VL_log_dataspace_conti;
