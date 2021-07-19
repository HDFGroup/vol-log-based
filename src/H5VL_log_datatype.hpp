#pragma once

#include <H5VLconnector.h>

static void *H5VL_log_datatype_commit (void *obj,
									   const H5VL_loc_params_t *loc_params,
									   const char *name,
									   hid_t type_id,
									   hid_t lcpl_id,
									   hid_t tcpl_id,
									   hid_t tapl_id,
									   hid_t dxpl_id,
									   void **req);
static void *H5VL_log_datatype_open (void *obj,
									 const H5VL_loc_params_t *loc_params,
									 const char *name,
									 hid_t tapl_id,
									 hid_t dxpl_id,
									 void **req);
static herr_t H5VL_log_datatype_get (void *dt,
									 H5VL_datatype_get_args_t *args,
									 hid_t dxpl_id,
									 void **req);
static herr_t H5VL_log_datatype_specific (void *obj,
										  H5VL_datatype_specific_args_t *args,
										  hid_t dxpl_id,
										  void **req);
static herr_t H5VL_log_datatype_optional (void *obj,
										  H5VL_optional_args_t *args,
										  hid_t dxpl_id,
										  void **req);
static herr_t H5VL_log_datatype_close (void *dt, hid_t dxpl_id, void **req);

extern void *H5VL_log_datatype_open_with_uo (void *obj,
											 void *uo,
											 const H5VL_loc_params_t *loc_params);
extern MPI_Datatype H5VL_logi_get_mpi_type_by_size (size_t size);