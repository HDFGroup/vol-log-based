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
inline hid_t H5VL_logi_dataset_get_type (void *obj, hid_t connector_id, hid_t dxpl_id) {
	herr_t err			= 0;
	H5VL_log_dset_t *dp = (H5VL_log_dset_t *)obj;
	H5VL_dataset_get_args_t args;

	args.op_type = H5VL_DATASET_GET_TYPE;

	H5VL_LOGI_PROFILING_TIMER_START;
	err = H5VLdataset_get (dp, connector_id, &args, dxpl_id, NULL);
	H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VLDATASET_GET);
	CHECK_ERR

err_out:;
	if (err) { args.args.get_type.type_id = -1; }
	return args.args.get_type.type_id;
}

inline hid_t H5VL_logi_dataset_get_space (void *obj, hid_t connector_id, hid_t dxpl_id) {
	herr_t err			= 0;
	H5VL_log_dset_t *dp = (H5VL_log_dset_t *)obj;
	H5VL_dataset_get_args_t args;

	args.op_type = H5VL_DATASET_GET_SPACE;

	H5VL_LOGI_PROFILING_TIMER_START;
	err = H5VLdataset_get (dp, connector_id, &args, dxpl_id, NULL);
	H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VLDATASET_GET);
	CHECK_ERR

err_out:;
	if (err) { args.args.get_space.space_id = -1; }
	return args.args.get_space.space_id;
}

inline hid_t H5VL_logi_dataset_get_dcpl (void *obj, hid_t connector_id, hid_t dxpl_id) {
	herr_t err			= 0;
	H5VL_log_dset_t *dp = (H5VL_log_dset_t *)obj;
	H5VL_dataset_get_args_t args;

	args.op_type = H5VL_DATASET_GET_DCPL;

	H5VL_LOGI_PROFILING_TIMER_START;
	err = H5VLdataset_get (dp, connector_id, &args, dxpl_id, NULL);
	H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VLDATASET_GET);
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

inline herr_t H5VL_logi_dataset_get_foff (void *obj,
										  hid_t connector_id,
										  hid_t dxpl_id,
										  haddr_t *off) {
	herr_t err			= 0;
	H5VL_log_dset_t *dp = (H5VL_log_dset_t *)obj;
	H5VL_optional_args_t args;
	H5VL_native_dataset_optional_args_t optarg;

	optarg.get_offset.offset = off;

	args.op_type = H5VL_NATIVE_DATASET_GET_OFFSET;
	args.args = &optarg;

	H5VL_LOGI_PROFILING_TIMER_START;
	err = H5VLdataset_optional (dp, connector_id, &args, dxpl_id, NULL);
	H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VLDATASET_OPTIONAL);
	CHECK_ERR

	*off = *((haddr_t *)(args.args));

err_out:;
	return err;
}

inline hid_t H5VL_logi_attr_get_space (void *obj, hid_t connector_id, hid_t dxpl_id) {
	herr_t err		   = 0;
	H5VL_log_obj_t *op = (H5VL_log_dset_t *)obj;
	H5VL_attr_get_args_t args;

	args.op_type = H5VL_ATTR_GET_SPACE;

	H5VL_LOGI_PROFILING_TIMER_START;
	err = H5VLattr_get (op, connector_id, &args, dxpl_id, NULL);
	H5VL_LOGI_PROFILING_TIMER_STOP (op->fp, TIMER_H5VLATT_GET);
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