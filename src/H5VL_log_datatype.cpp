#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "H5VL_log_datatype.hpp"

#include "H5VL_log_filei.hpp"
#include "H5VL_log_obj.hpp"
#include "H5VL_logi.hpp"

/* Datatype callbacks */
const H5VL_datatype_class_t H5VL_log_datatype_g {
	H5VL_log_datatype_commit,	/* commit       */
	H5VL_log_datatype_open,		/* open         */
	H5VL_log_datatype_get,		/* get          */
	H5VL_log_datatype_specific, /* specific     */
	H5VL_log_datatype_optional, /* optional     */
	H5VL_log_datatype_close		/* close        */
};

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_datatype_commit
 *
 * Purpose:     Commits a datatype inside a container.
 *
 * Return:      Success:    Pointer to datatype object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *H5VL_log_datatype_commit (void *obj,
									   const H5VL_loc_params_t *loc_params,
									   const char *name,
									   hid_t type_id,
									   hid_t lcpl_id,
									   hid_t tcpl_id,
									   hid_t tapl_id,
									   hid_t dxpl_id,
									   void **req) {
	H5VL_log_obj_t *tp;
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;

	tp = new H5VL_log_obj_t (op,H5I_DATATYPE);

	tp->uo = H5VLdatatype_commit (op->uo, loc_params, op->uvlid, name, type_id, lcpl_id, tcpl_id,
								  tapl_id, dxpl_id, req);
	CHECK_NERR (tp->uo);

	return (void *)tp;
err_out:;
	delete tp;
	return NULL;
} /* end H5VL_log_datatype_commit() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_datatype_open
 *
 * Purpose:     Opens a named datatype inside a container.
 *
 * Return:      Success:    Pointer to datatype object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *H5VL_log_datatype_open (void *obj,
									 const H5VL_loc_params_t *loc_params,
									 const char *name,
									 hid_t tapl_id,
									 hid_t dxpl_id,
									 void **req) {
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
	H5VL_log_obj_t *tp;

	tp = new H5VL_log_obj_t (op, H5I_DATATYPE);

	tp->uo = H5VLdatatype_open (op->uo, loc_params, op->uvlid, name, tapl_id, dxpl_id, req);
	CHECK_NERR (tp->uo);

	return (void *)tp;

err_out:;
	delete tp;

	return NULL;
} /* end H5VL_log_datatype_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_datatype_get
 *
 * Purpose:     Get information about a datatype
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_log_datatype_get (
	void *dt, H5VL_datatype_get_t get_type, hid_t dxpl_id, void **req, va_list arguments) {
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)dt;

	return H5VLdatatype_get (op->uo, op->uvlid, get_type, dxpl_id, req, arguments);

} /* end H5VL_log_datatype_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_datatype_specific
 *
 * Purpose:     Specific operations for datatypes
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_log_datatype_specific (void *obj,
										  H5VL_datatype_specific_t specific_type,
										  hid_t dxpl_id,
										  void **req,
										  va_list arguments) {
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;

	return H5VLdatatype_specific (op->uo, op->uvlid, specific_type, dxpl_id, req, arguments);
} /* end H5VL_log_datatype_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_datatype_optional
 *
 * Purpose:     Perform a connector-specific operation on a datatype
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_log_datatype_optional (
	void *obj, H5VL_datatype_optional_t opt_type, hid_t dxpl_id, void **req, va_list arguments) {
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;

	return H5VLdatatype_optional (op->uo, op->uvlid, opt_type, dxpl_id, req, arguments);
} /* end H5VL_log_datatype_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_datatype_close
 *
 * Purpose:     Closes a datatype.
 *
 * Return:      Success:    0
 *              Failure:    -1, datatype not closed.
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_log_datatype_close (void *dt, hid_t dxpl_id, void **req) {
	herr_t err		   = 0;
	H5VL_log_obj_t *tp = (H5VL_log_obj_t *)dt;

	err = H5VLdatatype_close (tp->uo, tp->uvlid, dxpl_id, req);
	CHECK_ERR

	delete tp;

err_out:;
	return err;
} /* end H5VL_log_datatype_close() */
