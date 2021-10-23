#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>

#include "H5VL_log_obj.hpp"
#include "H5VL_logi.hpp"
#include "H5VL_logi_mem.hpp"

/*-------------------------------------------------------------------------
 * Function:    H5VL__H5VL_log_new_obj
 *
 * Purpose:     Create a new log object for an underlying object
 *
 * Return:      Success:    Pointer to the new log object
 *              Failure:    NULL
 *
 * Programmer:  Quincey Koziol
 *              Monday, December 3, 2018
 *
 *-------------------------------------------------------------------------
 */
H5VL_log_obj_t *H5VL_log_new_obj (void *under_obj, hid_t uvlid) {
	H5VL_log_obj_t *new_obj;

	new_obj		   = (H5VL_log_obj_t *)calloc (1, sizeof (H5VL_log_obj_t));
	new_obj->uo	   = under_obj;
	new_obj->uvlid = uvlid;
	H5VL_logi_inc_ref (new_obj->uvlid);

	return new_obj;
} /* end H5VL__H5VL_log_new_obj() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__H5VL_log_free_obj
 *
 * Purpose:     Release a log object
 *
 * Note:	Take care to preserve the current HDF5 error stack
 *		when calling HDF5 API calls.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 * Programmer:  Quincey Koziol
 *              Monday, December 3, 2018
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_free_obj (H5VL_log_obj_t *obj) {
	// hid_t err_id;

	// err_id = H5Eget_current_stack();

	H5VL_logi_dec_ref (obj->uvlid);

	// H5Eset_current_stack(err_id);

	free (obj);

	return 0;
} /* end H5VL__H5VL_log_free_obj() */