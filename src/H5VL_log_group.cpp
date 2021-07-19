#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <hdf5.h>

#include "H5VL_log_filei.hpp"
#include "H5VL_log_group.hpp"
#include "H5VL_log_obj.hpp"
#include "H5VL_log_req.hpp"
#include "H5VL_logi.hpp"
#include "H5VL_logi_err.hpp"

/********************* */
/* Function prototypes */
/********************* */
const H5VL_group_class_t H5VL_log_group_g {
	H5VL_log_group_create,	 /* create       */
	H5VL_log_group_open,	 /* open       */
	H5VL_log_group_get,		 /* get          */
	H5VL_log_group_specific, /* specific     */
	H5VL_log_group_optional, /* optional     */
	H5VL_log_group_close	 /* close        */
};

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_group_create
 *
 * Purpose:     Creates a group inside a container
 *
 * Return:      Success:    Pointer to a group object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *H5VL_log_group_create (void *obj,
							 const H5VL_loc_params_t *loc_params,
							 const char *name,
							 hid_t lcpl_id,
							 hid_t gcpl_id,
							 hid_t gapl_id,
							 hid_t dxpl_id,
							 void **req) {
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
	H5VL_log_obj_t *gp;
	H5VL_log_req_t *rp;
	void **ureqp, *ureq;
	H5VL_LOGI_PROFILING_TIMER_START;

	/* Check arguments */
	if (loc_params->type != H5VL_OBJECT_BY_SELF)
		ERR_OUT ("loc_params->type is not H5VL_OBJECT_BY_SELF")

	gp = new H5VL_log_obj_t (op, H5I_GROUP);

	if (req) {
		rp	  = new H5VL_log_req_t ();
		ureqp = &ureq;
	} else {
		ureqp = NULL;
	}

	H5VL_LOGI_PROFILING_TIMER_START;
	gp->uo = H5VLgroup_create (op->uo, loc_params, op->uvlid, name, lcpl_id, gcpl_id, gapl_id,
							   dxpl_id, ureqp);
	CHECK_PTR (gp->uo)
	H5VL_LOGI_PROFILING_TIMER_STOP (op->fp, TIMER_H5VLGROUP_CREATE);

	if (req) {
		rp->append (ureq);
		*req = rp;
	}

	H5VL_LOGI_PROFILING_TIMER_STOP (gp->fp, TIMER_H5VL_LOG_GROUP_CREATE);

	return (void *)gp;

err_out:;
	delete gp;

	return NULL;
} /* end H5VL_log_group_create() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_group_open
 *
 * Purpose:     Opens a group inside a container
 *
 * Return:      Success:    Pointer to a group object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *H5VL_log_group_open (void *obj,
						   const H5VL_loc_params_t *loc_params,
						   const char *name,
						   hid_t gapl_id,
						   hid_t dxpl_id,
						   void **req) {
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
	H5VL_log_obj_t *gp;
	H5VL_log_req_t *rp;
	void **ureqp, *ureq;
	H5VL_LOGI_PROFILING_TIMER_START;

	/* Check arguments */
	if (loc_params->type != H5VL_OBJECT_BY_SELF)
		ERR_OUT ("loc_params->type is not H5VL_OBJECT_BY_SELF")

	gp = new H5VL_log_obj_t (op, H5I_GROUP);

	if (req) {
		rp	  = new H5VL_log_req_t ();
		ureqp = &ureq;
	} else {
		ureqp = NULL;
	}

	H5VL_LOGI_PROFILING_TIMER_START;
	gp->uo = H5VLgroup_open (op->uo, loc_params, op->uvlid, name, gapl_id, dxpl_id, ureqp);
	CHECK_PTR (gp->uo)
	H5VL_LOGI_PROFILING_TIMER_STOP (op->fp, TIMER_H5VLGROUP_OPEN);

	if (req) {
		rp->append (ureq);
		*req = rp;
	}

	H5VL_LOGI_PROFILING_TIMER_STOP (gp->fp, TIMER_H5VL_LOG_GROUP_OPEN);

	return (void *)gp;
err_out:;
	delete gp;
	return NULL;
} /* end H5VL_log_group_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_group_get
 *
 * Purpose:     Get info about a group
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_group_get (void *obj, H5VL_group_get_args_t *args, hid_t dxpl_id, void **req) {
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
	herr_t err		   = 0;
	H5VL_log_req_t *rp;
	void **ureqp, *ureq;
	H5VL_LOGI_PROFILING_TIMER_START;

	if (req) {
		rp	  = new H5VL_log_req_t ();
		ureqp = &ureq;
	} else {
		ureqp = NULL;
	}

	H5VL_LOGI_PROFILING_TIMER_START;
	err = H5VLgroup_get (op->uo, op->uvlid, args, dxpl_id, ureqp);
	H5VL_LOGI_PROFILING_TIMER_STOP (op->fp, TIMER_H5VLGROUP_GET);

	if (req) {
		rp->append (ureq);
		*req = rp;
	}

	H5VL_LOGI_PROFILING_TIMER_STOP (op->fp, TIMER_H5VL_LOG_GROUP_GET);
	return err;
} /* end H5VL_log_group_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_group_specific
 *
 * Purpose:     Specific operation on a group
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_group_specific (void *obj,
								H5VL_group_specific_args_t *args,
								hid_t dxpl_id,
								void **req) {
	herr_t err		   = 0;
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
	H5VL_log_req_t *rp;
	void **ureqp, *ureq;
	H5VL_LOGI_PROFILING_TIMER_START;

	if (req) {
		rp	  = new H5VL_log_req_t ();
		ureqp = &ureq;
	} else {
		ureqp = NULL;
	}

	H5VL_LOGI_PROFILING_TIMER_START;
	err = H5VLgroup_specific (op->uo, op->uvlid, args, dxpl_id, ureqp);
	H5VL_LOGI_PROFILING_TIMER_STOP (op->fp, TIMER_H5VLGROUP_SPECIFIC);

	if (req) {
		rp->append (ureq);
		*req = rp;
	}

	H5VL_LOGI_PROFILING_TIMER_STOP (op->fp, TIMER_H5VL_LOG_GROUP_SPECIFIC);
	return err;
} /* end H5VL_log_group_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_group_optional
 *
 * Purpose:     Perform a connector-specific operation on a group
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_group_optional (void *obj, H5VL_optional_args_t *args, hid_t dxpl_id, void **req) {
	H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
	herr_t err		   = 0;
	H5VL_log_req_t *rp;
	void **ureqp, *ureq;
	H5VL_LOGI_PROFILING_TIMER_START;

	if (req) {
		rp	  = new H5VL_log_req_t ();
		ureqp = &ureq;
	} else {
		ureqp = NULL;
	}

	H5VL_LOGI_PROFILING_TIMER_START;
	err = H5VLgroup_optional (op->uo, op->uvlid, args, dxpl_id, ureqp);
	H5VL_LOGI_PROFILING_TIMER_STOP (op->fp, TIMER_H5VLGROUP_OPTIONAL);

	if (req) {
		rp->append (ureq);
		*req = rp;
	}

	H5VL_LOGI_PROFILING_TIMER_STOP (op->fp, TIMER_H5VL_LOG_GROUP_OPTIONAL);
	return err;
} /* end H5VL_log_group_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_group_close
 *
 * Purpose:     Closes a group.
 *
 * Return:      Success:    0
 *              Failure:    -1, group not closed.
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_group_close (void *grp, hid_t dxpl_id, void **req) {
	H5VL_log_obj_t *gp = (H5VL_log_obj_t *)grp;
	herr_t err		   = 0;
	H5VL_log_req_t *rp;
	void **ureqp, *ureq;
	H5VL_LOGI_PROFILING_TIMER_START;

	if (req) {
		rp	  = new H5VL_log_req_t ();
		ureqp = &ureq;
	} else {
		ureqp = NULL;
	}

	H5VL_LOGI_PROFILING_TIMER_START;
	err = H5VLgroup_close (gp->uo, gp->uvlid, dxpl_id, ureqp);
	CHECK_ERR
	H5VL_LOGI_PROFILING_TIMER_STOP (gp->fp, TIMER_H5VLGROUP_CLOSE);

	if (req) {
		rp->append (ureq);
		*req = rp;
	}

	H5VL_LOGI_PROFILING_TIMER_STOP (gp->fp, TIMER_H5VL_LOG_GROUP_CLOSE);

	delete gp;

err_out:;
	return err;
} /* end H5VL_log_group_close() */