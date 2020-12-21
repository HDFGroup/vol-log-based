#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "H5VL_log_att.hpp"

#include "H5VL_log_filei.hpp"
#include "H5VL_log_obj.hpp"
#include "H5VL_logi.hpp"

/********************* */
/* Function prototypes */
/********************* */

const H5VL_attr_class_t H5VL_log_attr_g {
	H5VL_log_attr_create,	/* create       */
	H5VL_log_attr_open,		/* open         */
	H5VL_log_attr_read,		/* read         */
	H5VL_log_attr_write,	/* write        */
	H5VL_log_attr_get,		/* get          */
	H5VL_log_attr_specific, /* specific     */
	H5VL_log_attr_optional, /* optional     */
	H5VL_log_attr_close		/* close        */
};

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_attr_create
 *
 * Purpose:     Creates an attribute on an object.
 *
 * Return:      Success:    Pointer to attribute object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *H5VL_log_attr_create (void *obj,
							const H5VL_loc_params_t *loc_params,
							const char *name,
							hid_t type_id,
							hid_t space_id,
							hid_t acpl_id,
							hid_t aapl_id,
							hid_t dxpl_id,
							void **req) {
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
	H5VL_log_obj_t *ap;
	TIMER_START;

	ap = new H5VL_log_obj_t (op, H5I_ATTR);

	TIMER_START;
	ap->uo = H5VLattr_create (op->uo, loc_params, op->uvlid, name, type_id, space_id, acpl_id,
							  aapl_id, dxpl_id, NULL);
	CHECK_PTR (ap->uo);
	TIMER_STOP (ap->fp, TIMER_H5VL_ATT_CREATE);

	TIMER_STOP (ap->fp, TIMER_ATT_CREATE);
	return (void *)ap;

err_out:;
	delete ap;

	return NULL;
} /* end H5VL_log_attr_create() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_attr_open
 *
 * Purpose:     Opens an attribute on an object.
 *
 * Return:      Success:    Pointer to attribute object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *H5VL_log_attr_open (void *obj,
						  const H5VL_loc_params_t *loc_params,
						  const char *name,
						  hid_t aapl_id,
						  hid_t dxpl_id,
						  void **req) {
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
	H5VL_log_obj_t *ap;
	TIMER_START;

	ap = new H5VL_log_obj_t (op, H5I_ATTR);

	TIMER_START;
	ap->uo = H5VLattr_open (op->uo, loc_params, op->uvlid, name, aapl_id, dxpl_id, req);
	CHECK_PTR (ap->uo);
	TIMER_STOP (ap->fp, TIMER_H5VL_ATT_OPEN);

	TIMER_STOP (ap->fp, TIMER_ATT_OPEN);
	return (void *)ap;

err_out:;
	delete ap;

	return NULL;
} /* end H5VL_log_attr_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_attr_read
 *
 * Purpose:     Reads data from attribute.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_attr_read (void *attr, hid_t mem_type_id, void *buf, hid_t dxpl_id, void **req) {
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)attr;
	herr_t err		   = 0;
	TIMER_START;

	TIMER_START;
	err = H5VLattr_read (op->uo, op->uvlid, mem_type_id, buf, dxpl_id, req);
	TIMER_STOP (op->fp, TIMER_H5VL_ATT_READ);

	TIMER_STOP (op->fp, TIMER_ATT_READ);
	return err;
} /* end H5VL_log_attr_read() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_attr_write
 *
 * Purpose:     Writes data to attribute.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_attr_write (
	void *attr, hid_t mem_type_id, const void *buf, hid_t dxpl_id, void **req) {
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)attr;
	herr_t err		   = 0;
	TIMER_START;

	TIMER_START;
	err = H5VLattr_write (op->uo, op->uvlid, mem_type_id, buf, dxpl_id, req);
	TIMER_STOP (op->fp, TIMER_H5VL_ATT_WRITE);

	TIMER_STOP (op->fp, TIMER_ATT_WRITE);
	return err;
} /* end H5VL_log_attr_write() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_attr_get
 *
 * Purpose:     Gets information about an attribute
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_attr_get (
	void *obj, H5VL_attr_get_t get_type, hid_t dxpl_id, void **req, va_list arguments) {
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
	herr_t err		   = 0;
	TIMER_START;

	TIMER_START;
	err = H5VLattr_get (op->uo, op->uvlid, get_type, dxpl_id, req, arguments);
	TIMER_STOP (op->fp, TIMER_H5VL_ATT_GET);

	TIMER_STOP (op->fp, TIMER_ATT_GET);
	return err;
} /* end H5VL_log_attr_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_attr_specific
 *
 * Purpose:     Specific operation on attribute
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_attr_specific (void *obj,
							   const H5VL_loc_params_t *loc_params,
							   H5VL_attr_specific_t specific_type,
							   hid_t dxpl_id,
							   void **req,
							   va_list arguments) {
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
	herr_t err		   = 0;
	TIMER_START;

	TIMER_START;
	err = H5VLattr_specific (op->uo, loc_params, op->uvlid, specific_type, dxpl_id, req, arguments);
	CHECK_ERR
	TIMER_STOP (op->fp, TIMER_H5VL_ATT_SPECIFIC);

	TIMER_STOP (op->fp, TIMER_ATT_SPECIFIC);
err_out:;
	return err;
} /* end H5VL_log_attr_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_attr_optional
 *
 * Purpose:     Perform a connector-specific operation on an attribute
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_attr_optional (
	void *obj, H5VL_attr_optional_t opt_type, hid_t dxpl_id, void **req, va_list arguments) {
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
	herr_t err		   = 0;
	TIMER_START;

	TIMER_START;
	err = H5VLattr_optional (op->uo, op->uvlid, opt_type, dxpl_id, req, arguments);
	TIMER_STOP (op->fp, TIMER_H5VL_ATT_OPTIONAL);

	TIMER_STOP (op->fp, TIMER_ATT_OPTIONAL);
	return err;
} /* end H5VL_log_attr_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_attr_close
 *
 * Purpose:     Closes an attribute.
 *
 * Return:      Success:    0
 *              Failure:    -1, attr not closed.
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_attr_close (void *attr, hid_t dxpl_id, void **req) {
	H5VL_log_obj_t *ap = (H5VL_log_obj_t *)attr;
	herr_t err		   = 0;
	TIMER_START;

	TIMER_START;
	err = H5VLattr_close (ap->uo, ap->uvlid, dxpl_id, req);
	CHECK_ERR
	TIMER_STOP (ap->fp, TIMER_H5VL_ATT_CLOSE);

	TIMER_STOP (ap->fp, TIMER_ATT_CLOSE);

	delete ap;

err_out:;
	return err;
} /* end H5VL_log_attr_close() */