#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "H5VL_log_dataseti.hpp"
#include "H5VL_log_filei.hpp"
#include "H5VL_log_obj.hpp"
#include "H5VL_logi.hpp"

/********************* */
/* Function prototypes */
/********************* */

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
	herr_t err = 0;
	H5VL_log_obj_t *new_obj;
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
	void *uo;
#ifdef LOGVOL_VERBOSE_DEBUG
	{
		char vname[128];
		ssize_t nsize;

		nsize = H5Iget_name (dxpl_id, vname, 128);
		if (nsize == 0) {
			sprintf (vname, "Unnamed_Object");
		} else if (nsize < 0) {
			sprintf (vname, "Unknown_Object");
		}

		printf ("H5VL_log_object_open(%p, %p, %p, %s, %p)\n", obj, loc_params, opened_type, vname,
				req);
	}
#endif
	uo = H5VLobject_open (op->uo, loc_params, op->uvlid, opened_type, dxpl_id, req);
	if (uo == NULL) { CHECK_PTR (uo); }

	if (*opened_type == H5I_DATASET) {
		return H5VL_log_dataseti_open_with_uo (obj, uo, loc_params, dxpl_id);
	} else {
		return H5VL_log_obj_open_with_uo (obj, uo, *opened_type, loc_params);
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

#ifdef LOGVOL_VERBOSE_DEBUG
	{
		char vname[3][128];
		ssize_t nsize;

		nsize = H5Iget_name (ocpypl_id, vname[0], 128);
		if (nsize == 0) {
			sprintf (vname[0], "Unnamed_Object");
		} else if (nsize < 0) {
			sprintf (vname[0], "Unknown_Object");
		}
		nsize = H5Iget_name (lcpl_id, vname[1], 128);
		if (nsize == 0) {
			sprintf (vname[1], "Unnamed_Object");
		} else if (nsize < 0) {
			sprintf (vname[1], "Unknown_Object");
		}
		nsize = H5Iget_name (dxpl_id, vname[2], 128);
		if (nsize == 0) {
			sprintf (vname[2], "Unnamed_Object");
		} else if (nsize < 0) {
			sprintf (vname[2], "Unknown_Object");
		}

		printf ("H5VL_log_object_copy(%p, %p, %s, %p,%p,%s,%s,%s,%s,%p)\n", src_obj, src_loc_params,
				src_name, dst_obj, dst_loc_params, dst_name, vname[0], vname[1], vname[2], req);
	}
#endif
	ERR_OUT ("H5VL_log_object_copy Not Supported")
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
							H5VL_object_get_args_t *args,
							hid_t dxpl_id,
							void **req) {
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
#ifdef LOGVOL_VERBOSE_DEBUG
	{
		char vname[2][128];
		ssize_t nsize;

		nsize = H5Iget_name (args->get_type, vname[0], 128);
		if (nsize == 0) {
			sprintf (vname[0], "Unnamed_Object");
		} else if (nsize < 0) {
			sprintf (vname[0], "Unknown_Object");
		}
		nsize = H5Iget_name (dxpl_id, vname[1], 128);
		if (nsize == 0) {
			sprintf (vname[1], "Unnamed_Object");
		} else if (nsize < 0) {
			sprintf (vname[1], "Unknown_Object");
		}

		printf ("H5VL_log_object_get(%p, %p, %s, %s,%p, ...)\n", obj, loc_params, vname[0],
				vname[1], req);
	}
#endif
	return H5VLobject_get (op->uo, loc_params, op->uvlid, args, dxpl_id, req);
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
								 H5VL_object_specific_args_t *args,
								 hid_t dxpl_id,
								 void **req) {
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
#ifdef LOGVOL_VERBOSE_DEBUG
	{
		char vname[2][128];
		ssize_t nsize;

		nsize = H5Iget_name (specific_type, vname[0], 128);
		if (nsize == 0) {
			sprintf (vname[0], "Unnamed_Object");
		} else if (nsize < 0) {
			sprintf (vname[0], "Unknown_Object");
		}
		nsize = H5Iget_name (dxpl_id, vname[1], 128);
		if (nsize == 0) {
			sprintf (vname[1], "Unnamed_Object");
		} else if (nsize < 0) {
			sprintf (vname[1], "Unknown_Object");
		}

		printf ("H5VL_log_object_specific(%p, %p, %s, %s,%p, ...)\n", obj, loc_params, vname[0],
				vname[1], req);
	}
#endif
	return H5VLobject_specific (op->uo, loc_params, op->uvlid, args, dxpl_id, req);
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
herr_t H5VL_log_object_optional (void *obj,
								 const H5VL_loc_params_t *loc_params,
								 H5VL_optional_args_t *args,
								 hid_t dxpl_id,
								 void **req) {
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
#ifdef LOGVOL_VERBOSE_DEBUG
	{
		char vname[2][128];
		ssize_t nsize;

		nsize = H5Iget_name (opt_type, vname[0], 128);
		if (nsize == 0) {
			sprintf (vname[0], "Unnamed_Object");
		} else if (nsize < 0) {
			sprintf (vname[0], "Unknown_Object");
		}
		nsize = H5Iget_name (dxpl_id, vname[1], 128);
		if (nsize == 0) {
			sprintf (vname[1], "Unnamed_Object");
		} else if (nsize < 0) {
			sprintf (vname[1], "Unknown_Object");
		}

		printf ("H5VL_log_object_optional(%p, %s, %s,%p, ...)\n", obj, vname[0], vname[1], req);
	}
#endif
	return H5VLobject_optional (op->uo, loc_params, op->uvlid, args, dxpl_id, req);
} /* end H5VL_log_object_optional() */