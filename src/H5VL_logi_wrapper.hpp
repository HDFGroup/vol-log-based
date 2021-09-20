#pragma once

#include "H5VL_log_dataset.hpp"
#include "H5VL_log_obj.hpp"
#include "H5VL_logi.hpp"
#include "hdf5.h"

// Wraper
/*
extern herr_t H5VL_logi_dataset_specific_wrapper (void *obj,
												  hid_t connector_id,
												  H5VL_dataset_specific_args_t *args,
												  hid_t dxpl_id,
												  void **req,
												  ...);
extern herr_t H5VL_logi_dataset_get_wrapper (
	void *obj, hid_t connector_id, H5VL_dataset_get_args_t *args, hid_t dxpl_id, void **req, ...);
extern herr_t H5VL_logi_dataset_optional_wrapper (
	void *obj, hid_t connector_id, H5VL_optional_args_t *args, hid_t dxpl_id, void **req, ...);
extern herr_t H5VL_logi_file_optional_wrapper (
	void *obj, hid_t connector_id, H5VL_optional_args_t *args, hid_t dxpl_id, void **req, ...);

extern herr_t H5VL_logi_attr_get_wrapper (
	void *obj, hid_t connector_id, H5VL_attr_get_args_t *args, hid_t dxpl_id, void **req, ...);

extern herr_t H5VL_logi_link_specific_wrapper (void *obj,
											   const H5VL_loc_params_t *loc_params,
											   hid_t connector_id,
											   H5VL_link_specific_args_t *args,
											   hid_t dxpl_id,
											   void **req,
											   ...);
extern herr_t H5VL_logi_object_get_wrapper (void *obj,
											const H5VL_loc_params_t *loc_params,
											hid_t connector_id,
											H5VL_object_get_args_t *args,
											hid_t dxpl_id,
											void **req,
											...);
*/
inline hid_t H5VL_logi_dataset_get_type (H5VL_log_file_t *fp,
										 void *obj,
										 hid_t connector_id,
										 hid_t dxpl_id) {
	herr_t err = 0;
	H5VL_dataset_get_args_t args;

	args.op_type = H5VL_DATASET_GET_TYPE;

	H5VL_LOGI_PROFILING_TIMER_START;
	err = H5VLdataset_get (obj, connector_id, &args, dxpl_id, NULL);
	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VLDATASET_GET);
	CHECK_ERR

err_out:;
	if (err) { args.args.get_type.type_id = -1; }
	return args.args.get_type.type_id;
}

inline hid_t H5VL_logi_dataset_get_space (H5VL_log_file_t *fp,
										  void *obj,
										  hid_t connector_id,
										  hid_t dxpl_id) {
	herr_t err = 0;
	H5VL_dataset_get_args_t args;

	args.op_type = H5VL_DATASET_GET_SPACE;

	H5VL_LOGI_PROFILING_TIMER_START;
	err = H5VLdataset_get (obj, connector_id, &args, dxpl_id, NULL);
	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VLDATASET_GET);
	CHECK_ERR

err_out:;
	if (err) { args.args.get_space.space_id = -1; }
	return args.args.get_space.space_id;
}

inline hid_t H5VL_logi_dataset_get_dcpl (H5VL_log_file_t *fp,
										 void *obj,
										 hid_t connector_id,
										 hid_t dxpl_id) {
	herr_t err = 0;
	H5VL_dataset_get_args_t args;

	args.op_type = H5VL_DATASET_GET_DCPL;

	H5VL_LOGI_PROFILING_TIMER_START;
	err = H5VLdataset_get (obj, connector_id, &args, dxpl_id, NULL);
	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VLDATASET_GET);
	CHECK_ERR

err_out:;
	if (err) {
		if (args.args.get_dcpl.dcpl_id >= 0) {
			H5Pclose (args.args.get_dcpl.dcpl_id);
			args.args.get_dcpl.dcpl_id = -1;
		}
	}
	return args.args.get_dcpl.dcpl_id;
}

inline herr_t H5VL_logi_dataset_get_foff (
	H5VL_log_file_t *fp, void *obj, hid_t connector_id, hid_t dxpl_id, haddr_t *off) {
	herr_t err = 0;
	H5VL_optional_args_t args;
	H5VL_native_dataset_optional_args_t optarg;

	optarg.get_offset.offset = off;

	args.op_type = H5VL_NATIVE_DATASET_GET_OFFSET;
	args.args	 = &optarg;

	H5VL_LOGI_PROFILING_TIMER_START;
	err = H5VLdataset_optional (obj, connector_id, &args, dxpl_id, NULL);
	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VLDATASET_OPTIONAL);
	CHECK_ERR

err_out:;
	return err;
}

inline hid_t H5VL_logi_attr_get_space (H5VL_log_file_t *fp,
									   void *obj,
									   hid_t connector_id,
									   hid_t dxpl_id) {
	herr_t err = 0;
	H5VL_attr_get_args_t args;

	args.op_type = H5VL_ATTR_GET_SPACE;

	H5VL_LOGI_PROFILING_TIMER_START;
	err = H5VLattr_get (obj, connector_id, &args, dxpl_id, NULL);
	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VLATT_GET);
	CHECK_ERR

err_out:;
	if (err) {
		if (args.args.get_space.space_id >= 0) {
			H5Pclose (args.args.get_space.space_id);
			args.args.get_space.space_id = -1;
		}
	}
	return args.args.get_space.space_id;
}

inline herr_t H5VL_logi_file_flush (H5VL_log_file_t *fp,
								   void *obj,
								   hid_t connector_id,
								   hid_t dxpl_id) {
	herr_t err = 0;
	H5VL_file_specific_args_t args;

	args.op_type			 = H5VL_FILE_FLUSH;
	args.args.flush.scope	 = H5F_SCOPE_GLOBAL;
	args.args.flush.obj_type = H5I_FILE;

	H5VL_LOGI_PROFILING_TIMER_START;
	err = H5VLfile_specific (obj, connector_id, &args, dxpl_id, NULL);
	H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILE_SPECIFIC);
	CHECK_ERR

err_out:;

	return err;
}