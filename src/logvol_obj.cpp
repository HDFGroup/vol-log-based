#include "logvol_internal.hpp"

/********************* */
/* Function prototypes */
/********************* */
void *H5VL_log_object_open (void *obj,
							const H5VL_loc_params_t *loc_params,
							H5I_type_t *opened_type,
							hid_t dxpl_id,
							void **req);
herr_t H5VL_log_object_copy (void *src_obj,
							 const H5VL_loc_params_t *src_loc_params,
							 const char *src_name,
							 void *dst_obj,
							 const H5VL_loc_params_t *dst_loc_params,
							 const char *dst_name,
							 hid_t ocpypl_id,
							 hid_t lcpl_id,
							 hid_t dxpl_id,
							 void **req);
herr_t H5VL_log_object_get (void *obj,
							const H5VL_loc_params_t *loc_params,
							H5VL_object_get_t get_type,
							hid_t dxpl_id,
							void **req,
							va_list arguments);
herr_t H5VL_log_object_specific (void *obj,
								 const H5VL_loc_params_t *loc_params,
								 H5VL_object_specific_t specific_type,
								 hid_t dxpl_id,
								 void **req,
								 va_list arguments);
herr_t H5VL_log_object_optional (
	void *obj, H5VL_object_optional_t opt_type, hid_t dxpl_id, void **req, va_list arguments);

const H5VL_object_class_t H5VL_log_object_g {
	H5VL_log_object_open,	  /* open */
	H5VL_log_object_copy,	  /* copy */
	H5VL_log_object_get,	  /* get */
	H5VL_log_object_specific, /* specific */
	H5VL_log_object_optional, /* optional */
};

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_object_open
 *
 * Purpose:     Opens an object inside a container.
 *
 * Return:      Success:    Pointer to object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *H5VL_log_object_open (void *obj,
							const H5VL_loc_params_t *loc_params,
							H5I_type_t *opened_type,
							hid_t dxpl_id,
							void **req) {
	herr_t err;
	H5VL_log_obj_t *new_obj;
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
	H5O_type_t otype;
	void *uo;

	uo = H5VLobject_open(op->uo, loc_params, op->uvlid, opened_type, dxpl_id, req);
	if (uo == NULL){
		CHECK_NERR(uo);
	}

	if (otype == H5O_TYPE_DATASET)
	{
		return H5VL_log_dataset_open_with_uo(obj, uo, loc_params, dxpl_id);
	}
	else if (otype == H5O_TYPE_GROUP)
	{
		return H5VL_log_group_open_with_uo(obj, uo, loc_params);
	}
	else{
		return uo;
	}

err_out:;
	return NULL;
} /* end H5VL_log_object_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_object_copy
 *
 * Purpose:     Copies an object inside a container.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_object_copy (void *src_obj,
							 const H5VL_loc_params_t *src_loc_params,
							 const char *src_name,
							 void *dst_obj,
							 const H5VL_loc_params_t *dst_loc_params,
							 const char *dst_name,
							 hid_t ocpypl_id,
							 hid_t lcpl_id,
							 hid_t dxpl_id,
							 void **req) {
	H5VL_log_obj_t *o_src = (H5VL_log_obj_t *)src_obj;
	H5VL_log_obj_t *o_dst = (H5VL_log_obj_t *)dst_obj;

	RET_ERR ("H5VL_log_object_copy Not Supported")
err_out:;
	return -1;

	return H5VLobject_copy (o_src->uo, src_loc_params, src_name, o_dst->uo, dst_loc_params,
							dst_name, o_src->uvlid, ocpypl_id, lcpl_id, dxpl_id, req);
} /* end H5VL_log_object_copy() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_object_get
 *
 * Purpose:     Get info about an object
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_object_get (void *obj,
							const H5VL_loc_params_t *loc_params,
							H5VL_object_get_t get_type,
							hid_t dxpl_id,
							void **req,
							va_list arguments) {
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;

	return H5VLobject_get (op->uo, loc_params, op->uvlid, get_type, dxpl_id, req, arguments);
} /* end H5VL_log_object_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_object_specific
 *
 * Purpose:     Specific operation on an object
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_object_specific (void *obj,
								 const H5VL_loc_params_t *loc_params,
								 H5VL_object_specific_t specific_type,
								 hid_t dxpl_id,
								 void **req,
								 va_list arguments) {
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;

	return H5VLobject_specific (op->uo, loc_params, op->uvlid, specific_type, dxpl_id, req,
								arguments);
} /* end H5VL_log_object_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_object_optional
 *
 * Purpose:     Perform a connector-specific operation for an object
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_object_optional (
	void *obj, H5VL_object_optional_t opt_type, hid_t dxpl_id, void **req, va_list arguments) {
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;

	return H5VLobject_optional (op->uo, op->uvlid, opt_type, dxpl_id, req, arguments);
} /* end H5VL_log_object_optional() */