#pragma once
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <H5VLpublic.h>

#include "H5VL_log.h"

#include "H5VL_logi_err.hpp"

#ifdef LOGVOL_PROFILING
#include "H5VL_logi_profiling.hpp"
#define TIMER_START          \
	{                        \
		double tstart, tend; \
		tstart = MPI_Wtime ();
#define TIMER_STOP(A, B)                             \
	tend = MPI_Wtime ();                             \
	H5VL_log_profile_add_time (A, B, tend - tstart); \
	}
#else
#define TIMER_START \
	{}
#define TIMER_STOP(A, B) \
	{}
#endif

#define LOGVOL_SELCTION_TYPE_HYPERSLABS 0x01
#define LOGVOL_SELCTION_TYPE_POINTS 0x02
#define LOGVOL_SELCTION_TYPE_OFFSETS 0x04

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

