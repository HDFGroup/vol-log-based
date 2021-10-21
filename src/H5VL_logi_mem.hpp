#pragma once
//
#include <vector>
//
#include <H5VLconnector.h>
//
#include "H5VL_log_obj.hpp"

#define H5VL_log_delete_arr(A) \
	{                          \
		delete[] A;            \
		A = NULL;              \
	}
#define H5VL_log_free(A) \
	{                    \
		free (A);        \
		A = NULL;        \
	}
#define H5VL_log_Sclose(A)         \
	{                              \
		if (A != -1) H5Sclose (A); \
	}
#define H5VL_log_Tclose(A)         \
	{                              \
		if (A != -1) H5Tclose (A); \
	}
#define H5VL_log_type_free(A)         \
	{                                 \
		if (A != MPI_DATATYPE_NULL) { \
			MPI_Type_free (&A);       \
			A = MPI_DATATYPE_NULL;    \
		}                             \
	}

extern H5VL_log_obj_t *H5VL_log_new_obj (void *under_obj, hid_t uvlid);
extern herr_t H5VL_log_free_obj (H5VL_log_obj_t *obj);

class H5VL_logi_buffer{
	typedef struct H5VL_logi_buffer_block {
		char *start;
		char *cur;
		char *end;
	} H5VL_logi_buffer_block;

	std::vector<H5VL_logi_buffer_block> blocks;
	H5VL_logi_buffer_block cur;
    size_t block_size;

	void new_block();

	public:
	H5VL_logi_buffer();
	H5VL_logi_buffer(size_t block_size);
	~H5VL_logi_buffer();

	void reset();
	char *reserve(size_t size);
	void reclaim(size_t size);
herr_t get_mem_type (MPI_Datatype &type);
};