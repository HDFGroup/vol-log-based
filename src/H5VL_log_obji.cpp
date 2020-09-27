#include "H5VL_log_obji.hpp"
#include "H5VL_log_filei.hpp"

void *H5VL_log_obj_open_with_uo (void *obj, void *uo, H5I_type_t type, const H5VL_loc_params_t *loc_params) {
	H5VL_log_obj_t *pp	 = (H5VL_log_obj_t *)obj;
	H5VL_log_obj_t *op = NULL;

	/* Check arguments */
	// if(loc_params->type != H5VL_OBJECT_BY_SELF) RET_ERR("loc_params->type is not
	// H5VL_OBJECT_BY_SELF")

	op = new H5VL_log_obj_t ();
	CHECK_NERR (op);
	if (loc_params->obj_type == H5I_FILE)
		op->fp = (H5VL_log_file_t *)obj;
	else if (loc_params->obj_type == H5I_GROUP)
		op->fp = ((H5VL_log_obj_t *)obj)->fp;
	else
		RET_ERR ("container not a file or group")
    H5VL_log_filei_inc_ref(op->fp);

	op->uo	  = uo;
	op->uvlid = pp->uvlid;
	H5Iinc_ref (op->uvlid);
	op->type = type;

	return (void *)op;

err_out:;
	delete op;

	return NULL;
} /* end H5VL_log_group_ppen() */
