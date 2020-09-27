#pragma once

#include "H5VL_log_int.hpp"

// Wraper
extern herr_t H5VL_logi_dataset_specific_wrapper (void *obj,
											hid_t connector_id,
											H5VL_dataset_specific_t specific_type,
											hid_t dxpl_id,
											void **req,
											...);
extern herr_t H5VL_logi_dataset_get_wrapper (
	void *obj, hid_t connector_id, H5VL_dataset_get_t get_type, hid_t dxpl_id, void **req, ...);
extern herr_t H5VL_logi_dataset_optional_wrapper (void *obj,
											hid_t connector_id,
											H5VL_dataset_optional_t opt_type,
											hid_t dxpl_id,
											void **req,
											...);
extern herr_t H5VL_logi_file_optional_wrapper (
	void *obj, hid_t connector_id, H5VL_file_optional_t opt_type, hid_t dxpl_id, void **req, ...);

extern herr_t H5VL_logi_attr_get_wrapper(void *obj, hid_t connector_id, H5VL_attr_get_t get_type, hid_t dxpl_id, void **req, ...);

extern herr_t H5VL_logi_link_specific_wrapper (void *obj,
										 const H5VL_loc_params_t *loc_params,
										 hid_t connector_id,
										 H5VL_link_specific_t specific_type,
										 hid_t dxpl_id,
										 void **req,
										 ...);
extern herr_t H5VL_logi_object_get_wrapper (void *obj,
									  const H5VL_loc_params_t *loc_params,
									  hid_t connector_id,
									  H5VL_object_get_t get_type,
									  hid_t dxpl_id,
									  void **req,
									  ...);