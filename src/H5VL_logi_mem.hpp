#pragma once

#include <H5VLconnector.h>

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