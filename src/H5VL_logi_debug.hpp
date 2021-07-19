#pragma once

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <H5VLconnector.h>
#include <mpi.h>

#include <cstdio>

#ifdef LOGVOL_DEBUG
extern int H5VL_log_debug_MPI_Type_create_subarray (int ndims,
													const int array_of_sizes[],
													const int array_of_subsizes[],
													const int array_of_starts[],
													int order,
													MPI_Datatype oldtype,
													MPI_Datatype *newtype);
extern void hexDump (char *desc, void *addr, size_t len, char *fname);
extern void hexDump (char *desc, void *addr, size_t len);
extern void hexDump (char *desc, void *addr, size_t len, FILE *fp);
extern int H5VL_logi_inc_ref (hid_t);
extern int H5VL_logi_dec_ref (hid_t);
#else
#define H5VL_log_debug_MPI_Type_create_subarray MPI_Type_create_subarray
#define H5VL_logi_inc_ref						H5Iinc_ref
#define H5VL_logi_dec_ref						H5Idec_ref
#endif
