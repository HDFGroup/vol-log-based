#pragma once

#include <hdf5.h>
#include <mpi.h>

void *H5VL_log_attr_create (void *obj,
							const H5VL_loc_params_t *loc_params,
							const char *attr_name,
							hid_t type_id,
							hid_t space_id,
							hid_t acpl_id,
							hid_t aapl_id,
							hid_t dxpl_id,
							void **req);
void *H5VL_log_attr_open (void *obj,
						  const H5VL_loc_params_t *loc_params,
						  const char *attr_name,
						  hid_t aapl_id,
						  hid_t dxpl_id,
						  void **req);
herr_t H5VL_log_attr_read (void *attr, hid_t dtype_id, void *buf, hid_t dxpl_id, void **req);
herr_t H5VL_log_attr_write (void *attr, hid_t dtype_id, const void *buf, hid_t dxpl_id, void **req);
herr_t H5VL_log_attr_get (void *obj, H5VL_attr_get_args_t *args, hid_t dxpl_id, void **req);
herr_t H5VL_log_attr_specific (void *obj,
							   const H5VL_loc_params_t *loc_params,
							   H5VL_attr_specific_args_t *args,
							   hid_t dxpl_id,
							   void **req);
herr_t H5VL_log_attr_optional (void *obj, H5VL_optional_args_t *args, hid_t dxpl_id, void **req);
herr_t H5VL_log_attr_close (void *attr, hid_t dxpl_id, void **req);