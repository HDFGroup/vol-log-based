#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "H5VL_log.h"
#include "H5VL_log_blob.hpp"
#include "H5VL_log_obj.hpp"
#include "H5VL_logi.hpp"

const H5VL_blob_class_t H5VL_log_blob_g {
	H5VL_log_blob_put,		/* put       */
	H5VL_log_blob_get,		/* get         */
	H5VL_log_blob_specific, /* specific         */
	H5VL_log_blob_optional, /* optional         */
};

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_blob_put
 *
 * Purpose:     Handles the blob 'put' callback
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_blob_put (void *obj, const void *buf, size_t size, void *blob_id, void *ctx) {
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
#ifdef LOGVOL_VERBOSE_DEBUG
	printf ("H5VL_log_blob_put(%p, %p, %llu, %p, %p)\n", obj, buf, size, blob_id, ctx);
#endif
	return H5VLblob_put (op->uo, op->uvlid, buf, size, blob_id, ctx);
} /* end H5VL_log_blob_put() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_blob_get
 *
 * Purpose:     Handles the blob 'get' callback
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_blob_get (void *obj, const void *blob_id, void *buf, size_t size, void *ctx) {
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
#ifdef LOGVOL_VERBOSE_DEBUG
	printf ("H5VL_log_blob_get(%p, %p, %p, %llu, %p)\n", obj, blob_id, buf, size, ctx);
#endif
	return H5VLblob_get (op->uo, op->uvlid, blob_id, buf, size, ctx);
} /* end H5VL_log_blob_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_blob_specific
 *
 * Purpose:     Handles the blob 'specific' callback
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_blob_specific (void *obj, void *blob_id, H5VL_blob_specific_args_t *args) {
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
#ifdef LOGVOL_VERBOSE_DEBUG
	char sname[32];
	switch (specific_type) {
		case H5VL_BLOB_DELETE:
			sprintf (sname, "H5VL_BLOB_DELETE");
			break;
		case H5VL_BLOB_GETSIZE:
			sprintf (sname, "H5VL_BLOB_GETSIZE");
			break;
		case H5VL_BLOB_ISNULL:
			sprintf (sname, "H5VL_BLOB_ISNULL");
			break;
		case H5VL_BLOB_SETNULL:
			sprintf (sname, "H5VL_BLOB_SETNULL");
			break;
	}
	printf ("H5VL_log_blob_specific(%p, %p, %s, ...)\n", obj, blob_id, sname);
#endif
	return H5VLblob_specific (op->uo, op->uvlid, blob_id, args);
} /* end H5VL_log_blob_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_blob_optional
 *
 * Purpose:     Handles the blob 'optional' callback
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_blob_optional (void *obj, void *blob_id, H5VL_optional_args_t *args) {
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
#ifdef LOGVOL_VERBOSE_DEBUG
	printf ("H5VL_log_blob_optional(%p, %p, %d, ...)\n", obj, blob_id, opt_type);
#endif
	return H5VLblob_optional (op->uo, op->uvlid, blob_id, args);
} /* end H5VL_log_blob_optional() */