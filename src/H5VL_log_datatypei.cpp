#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "H5VL_log_datatype.hpp"
#include "H5VL_log_filei.hpp"
#include "H5VL_log_obj.hpp"
#include "H5VL_logi.hpp"

void *H5VL_log_datatype_open_with_uo (void *obj, void *uo, const H5VL_loc_params_t *loc_params) {
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
	H5VL_log_obj_t *tp = NULL;

	/* Check arguments */
	// if(loc_params->type != H5VL_OBJECT_BY_SELF) ERR_OUT("loc_params->type is not
	// H5VL_OBJECT_BY_SELF")

	tp = new H5VL_log_obj_t (op, H5I_DATATYPE, uo);
	CHECK_PTR (tp);

	return (void *)tp;

err_out:;
	delete tp;

	return NULL;
} /* end H5VL_log_group_open() */