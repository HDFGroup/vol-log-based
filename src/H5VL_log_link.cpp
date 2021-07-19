#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cassert>

#include "H5VL_log.h"
#include "H5VL_log_link.hpp"
#include "H5VL_log_obj.hpp"
#include "H5VL_log_req.hpp"
#include "H5VL_logi.hpp"

/********************* */
/* Function prototypes */
/********************* */
const H5VL_link_class_t H5VL_log_link_g {
	//   H5VL_log_link_create_reissue,                       /* create_reissue */
	H5VL_log_link_create,	/* create */
	H5VL_log_link_copy,		/* copy */
	H5VL_log_link_move,		/* move */
	H5VL_log_link_get,		/* get */
	H5VL_log_link_specific, /* specific */
	H5VL_log_link_optional	/* optional */
};

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_link_create_reissue
 *
 * Purpose:     Re-wrap vararg arguments into a va_list and reissue the
 *              link create callback to the underlying VOL connector.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_link_create_reissue (H5VL_link_create_args_t *args,
									 void *obj,
									 const H5VL_loc_params_t *loc_params,
									 hid_t connector_id,
									 hid_t lcpl_id,
									 hid_t lapl_id,
									 hid_t dxpl_id,
									 void **req,
									 ...) {
	va_list arguments;
	herr_t err = 0;

	va_start (arguments, req);
	err = H5VLlink_create (args, obj, loc_params, connector_id, lcpl_id, lapl_id, dxpl_id, req);
	va_end (arguments);

	return err;
} /* end H5VL_log_link_create_reissue() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_link_create
 *
 * Purpose:     Creates a hard / soft / UD / external link.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_link_create (H5VL_link_create_args_t *args,
							 void *obj,
							 const H5VL_loc_params_t *loc_params,
							 hid_t lcpl_id,
							 hid_t lapl_id,
							 hid_t dxpl_id,
							 void **req) {
	H5VL_log_obj_t *o = (H5VL_log_obj_t *)obj;
	hid_t uvlid		  = -1;
	herr_t err		  = 0;

#ifdef LOGVOL_VERBOSE_DEBUG
	printf ("------- LOG VOL LINK Create\n");
#endif

	/* Try to retrieve the "under" VOL id */
	if (o) uvlid = o->uvlid;

	/* Fix up the link target object for hard link creation */
	if (H5VL_LINK_CREATE_HARD == args->op_type) {
		void *cur_obj;
		H5VL_loc_params_t cur_params;

		/* Retrieve the object & loc params for the link target */
		cur_obj	   = args->args.hard.curr_obj;
		cur_params = args->args.hard.curr_loc_params;

		/* If it's a non-NULL pointer, find the 'under object' and re-set the property */
		if (cur_obj) {
			/* Check if we still need the "under" VOL ID */
			if (uvlid < 0) uvlid = ((H5VL_log_obj_t *)cur_obj)->uvlid;

			/* Set the object for the link target */
			cur_obj = ((H5VL_log_obj_t *)cur_obj)->uo;
		} /* end if */

		/* Re-issue 'link create' call, using the unwrapped pieces */
		err = H5VL_log_link_create_reissue (args, (o ? o->uo : NULL), loc_params, uvlid, lcpl_id,
											lapl_id, dxpl_id, req, cur_obj, cur_params);
	} /* end if */
	else
		err = H5VLlink_create (args, (o ? o->uo : NULL), loc_params, uvlid, lcpl_id, lapl_id,
							   dxpl_id, req);

	return err;
} /* end H5VL_log_link_create() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_link_copy
 *
 * Purpose:     Renames an object within an HDF5 container and copies it to a new
 *              group.  The original name SRC is unlinked from the group graph
 *              and then inserted with the new name DST (which can specify a
 *              new path for the object) as an atomic operation. The names
 *              are interpreted relative to SRC_LOC_ID and
 *              DST_LOC_ID, which are either file IDs or group ID.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_link_copy (void *src_obj,
						   const H5VL_loc_params_t *loc_params1,
						   void *dst_obj,
						   const H5VL_loc_params_t *loc_params2,
						   hid_t lcpl_id,
						   hid_t lapl_id,
						   hid_t dxpl_id,
						   void **req) {
	H5VL_log_obj_t *o_src = (H5VL_log_obj_t *)src_obj;
	H5VL_log_obj_t *o_dst = (H5VL_log_obj_t *)dst_obj;
	hid_t uvlid			  = -1;
	herr_t err			  = 0;
	H5VL_log_req_t *rp;
	void **ureqp, *ureq;

#ifdef LOGVOL_VERBOSE_DEBUG
	printf ("------- LOG VOL LINK Copy\n");
#endif

	if (req) {
		rp	  = new H5VL_log_req_t ();
		ureqp = &ureq;
	} else {
		ureqp = NULL;
	}

	/* Retrieve the "under" VOL id */
	if (o_src)
		uvlid = o_src->uvlid;
	else if (o_dst)
		uvlid = o_dst->uvlid;
	assert (uvlid > 0);

	err = H5VLlink_copy ((o_src ? o_src->uo : NULL), loc_params1, (o_dst ? o_dst->uo : NULL),
						 loc_params2, uvlid, lcpl_id, lapl_id, dxpl_id, ureqp);
	CHECK_ERR

	if (req) {
		rp->append (ureq);
		*req = rp;
	}

err_out:;
	return err;
} /* end H5VL_log_link_copy() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_link_move
 *
 * Purpose:     Moves a link within an HDF5 file to a new group.  The original
 *              name SRC is unlinked from the group graph
 *              and then inserted with the new name DST (which can specify a
 *              new path for the object) as an atomic operation. The names
 *              are interpreted relative to SRC_LOC_ID and
 *              DST_LOC_ID, which are either file IDs or group ID.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_link_move (void *src_obj,
						   const H5VL_loc_params_t *loc_params1,
						   void *dst_obj,
						   const H5VL_loc_params_t *loc_params2,
						   hid_t lcpl_id,
						   hid_t lapl_id,
						   hid_t dxpl_id,
						   void **req) {
	H5VL_log_obj_t *o_src = (H5VL_log_obj_t *)src_obj;
	H5VL_log_obj_t *o_dst = (H5VL_log_obj_t *)dst_obj;
	hid_t uvlid			  = -1;
	herr_t err			  = 0;
	H5VL_log_req_t *rp;
	void **ureqp, *ureq;

#ifdef LOGVOL_VERBOSE_DEBUG
	printf ("------- LOG VOL LINK Move\n");
#endif

	if (req) {
		rp	  = new H5VL_log_req_t ();
		ureqp = &ureq;
	} else {
		ureqp = NULL;
	}

	/* Retrieve the "under" VOL id */
	if (o_src)
		uvlid = o_src->uvlid;
	else if (o_dst)
		uvlid = o_dst->uvlid;
	assert (uvlid > 0);

	err = H5VLlink_move ((o_src ? o_src->uo : NULL), loc_params1, (o_dst ? o_dst->uo : NULL),
						 loc_params2, uvlid, lcpl_id, lapl_id, dxpl_id, ureqp);
	CHECK_ERR

	if (req) {
		rp->append (ureq);
		*req = rp;
	}

err_out:;
	return err;
} /* end H5VL_log_link_move() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_link_get
 *
 * Purpose:     Get info about a link
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_link_get (void *obj,
						  const H5VL_loc_params_t *loc_params,
						  H5VL_link_get_args_t *args,
						  hid_t dxpl_id,
						  void **req) {
	H5VL_log_obj_t *o = (H5VL_log_obj_t *)obj;
	herr_t err		  = 0;
	H5VL_log_req_t *rp;
	void **ureqp, *ureq;

#ifdef LOGVOL_VERBOSE_DEBUG
	printf ("------- LOG VOL LINK Get\n");
#endif

	if (req) {
		rp	  = new H5VL_log_req_t ();
		ureqp = &ureq;
	} else {
		ureqp = NULL;
	}

	err = H5VLlink_get (o->uo, loc_params, o->uvlid, args, dxpl_id, ureqp);
	CHECK_ERR

	if (req) {
		rp->append (ureq);
		*req = rp;
	}
err_out:;
	return err;
} /* end H5VL_log_link_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_link_specific
 *
 * Purpose:     Specific operation on a link
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_link_specific (void *obj,
							   const H5VL_loc_params_t *loc_params,
							   H5VL_link_specific_args_t *args,
							   hid_t dxpl_id,
							   void **req) {
	H5VL_log_obj_t *o = (H5VL_log_obj_t *)obj;
	herr_t err		  = 0;
	H5VL_log_req_t *rp;
	void **ureqp, *ureq;

#ifdef LOGVOL_VERBOSE_DEBUG
	printf ("------- LOG VOL LINK Specific\n");
#endif

	if (req) {
		rp	  = new H5VL_log_req_t ();
		ureqp = &ureq;
	} else {
		ureqp = NULL;
	}

	err = H5VLlink_specific (o->uo, loc_params, o->uvlid, args, dxpl_id, ureqp);
	CHECK_ERR

	if (req) {
		rp->append (ureq);
		*req = rp;
	}
err_out:;
	return err;
} /* end H5VL_log_link_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_link_optional
 *
 * Purpose:     Perform a connector-specific operation on a link
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_link_optional (void *obj,
							   const H5VL_loc_params_t *loc_params,
							   H5VL_optional_args_t *args,
							   hid_t dxpl_id,
							   void **req) {
	H5VL_log_obj_t *o = (H5VL_log_obj_t *)obj;
	herr_t err		  = 0;
	H5VL_log_req_t *rp;
	void **ureqp, *ureq;

#ifdef LOGVOL_VERBOSE_DEBUG
	printf ("------- LOG VOL LINK Optional\n");
#endif

	if (req) {
		rp	  = new H5VL_log_req_t ();
		ureqp = &ureq;
	} else {
		ureqp = NULL;
	}

	err = H5VLlink_optional (o->uo, loc_params, o->uvlid, args, dxpl_id, ureqp);
	CHECK_ERR

	if (req) {
		rp->append (ureq);
		*req = rp;
	}

err_out:;
	return err;
} /* end H5VL_log_link_optional() */