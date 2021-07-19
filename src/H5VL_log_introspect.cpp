#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "H5VL_log.h"
#include "H5VL_log_info.hpp"
#include "H5VL_log_introspect.hpp"
#include "H5VL_logi.hpp"

const H5VL_introspect_class_t H5VL_log_introspect_g {
	H5VL_log_introspect_get_conn_cls,
	H5VL_log_introspect_get_cap_flags,
	H5VL_log_introspect_opt_query,
};

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_introspect_get_conn_clss
 *
 * Purpose:     Query the connector class.
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_introspect_get_conn_cls (void *obj,
										 H5VL_get_conn_lvl_t lvl,
										 const H5VL_class_t **conn_cls) {
	herr_t err		   = 0;
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;

	/* Check for querying this connector's class */
	if (lvl == H5VL_GET_CONN_LVL_CURR) {
		*conn_cls = &H5VL_log_g;
		err		  = 0;
	} /* end if */
	else
		err = H5VLintrospect_get_conn_cls (op->uo, op->uvlid, lvl, conn_cls);

	return err;
} /* end H5VL_log_introspect_get_conn_cls() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_introspect_get_cap_flags
 *
 * Purpose:
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_introspect_get_cap_flags (const void *info, unsigned *cap_flags) {
	herr_t err			= 0;
	H5VL_log_info_t *ip = (H5VL_log_info_t *)info;

	//*supported = 0;
	// return 0;

	err = H5VLintrospect_get_cap_flags (ip->under_vol_info, ip->uvlid, cap_flags);

	return err;
} /* end H5VL_log_introspect_get_cap_flags() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_introspect_opt_query
 *
 * Purpose:     Query if an optional operation is supported by this connector
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_introspect_opt_query (void *obj,
									  H5VL_subclass_t cls,
									  int opt_type,
									  uint64_t *flags) {
	herr_t err		   = 0;
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;

	//*supported = 0;
	// return 0;

	err = H5VLintrospect_opt_query (op->uo, op->uvlid, cls, opt_type, flags);

	return err;
} /* end H5VL_log_introspect_opt_query() */
