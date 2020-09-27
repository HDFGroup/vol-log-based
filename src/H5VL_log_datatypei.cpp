#include "H5VL_log_datatype.hpp"
#include "H5VL_log_obj.hpp"
#include "H5VL_log_filei.hpp"
#include "H5VL_logi.hpp"

void *H5VL_log_datatype_open_with_uo (void *obj, void *uo, const H5VL_loc_params_t *loc_params) {
	H5VL_log_obj_t *op	 = (H5VL_log_obj_t *)obj;
	H5VL_log_obj_t *tp = NULL;

	/* Check arguments */
	// if(loc_params->type != H5VL_OBJECT_BY_SELF) RET_ERR("loc_params->type is not
	// H5VL_OBJECT_BY_SELF")

	tp = new H5VL_log_obj_t ();
	CHECK_NERR (tp);
	if (loc_params->obj_type == H5I_FILE)
		tp->fp = (H5VL_log_file_t *)obj;
	else if (loc_params->obj_type == H5I_GROUP)
		tp->fp = ((H5VL_log_obj_t *)obj)->fp;
	else
		RET_ERR ("container not a file or group")
    H5VL_log_filei_inc_ref(tp->fp);

	tp->uo	  = uo;
	tp->uvlid = op->uvlid;
	H5Iinc_ref (tp->uvlid);
	tp->type = H5I_DATATYPE;

	return (void *)tp;

err_out:;
	delete tp;

	return NULL;
} /* end H5VL_log_group_open() */