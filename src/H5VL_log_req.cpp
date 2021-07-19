#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <H5VLconnector.h>

#include <cassert>
#include <cstdlib>

#include "H5VL_log.h"
#include "H5VL_log_obj.hpp"
#include "H5VL_log_req.hpp"
#include "H5VL_log_reqi.hpp"
#include "H5VL_logi.hpp"
#include "H5VL_logi_debug.hpp"

/********************* */
/* Function prototypes */
/********************* */
const H5VL_request_class_t H5VL_log_request_g {
	H5VL_log_request_wait,	 /* wait */
	H5VL_log_request_notify, /* notify */
	H5VL_log_request_cancel, /* cancel */
	//    H5VL_log_request_specific_reissue,                     /* specific_reissue */
	H5VL_log_request_specific, /* specific */
	H5VL_log_request_optional, /* optional */
	H5VL_log_request_free	   /* free */
};

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_request_wait
 *
 * Purpose:     Wait (with a timeout) for an async operation to complete
 *
 * Note:        Releases the request if the operation has completed and the
 *              connector callback succeeds
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_request_wait (void *obj, uint64_t timeout, H5VL_request_status_t *status) {
	herr_t err = 0;
	uint64_t t1, t2;
	H5VL_log_req_t *rp = (H5VL_log_req_t *)obj;

	*status = H5VL_REQUEST_STATUS_SUCCEED;

	for (auto &ureq : rp->ureqs) {
		if (ureq.stat == H5VL_REQUEST_STATUS_IN_PROGRESS) {
			err = H5VLrequest_wait (ureq.req, rp->uvlid, timeout, &(ureq.stat));
			CHECK_ERR
		}

		if (ureq.stat == H5VL_REQUEST_STATUS_FAIL) {
			*status = H5VL_REQUEST_STATUS_FAIL;
		} else if (*status == H5VL_REQUEST_STATUS_SUCCEED) {
			*status = ureq.stat;
		}
	}

	if (*status != H5VL_REQUEST_STATUS_IN_PROGRESS) { delete rp; }

err_out:;
	return err;
} /* end H5VL_log_request_wait() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_request_notify
 *
 * Purpose:     Registers a user callback to be invoked when an asynchronous
 *              operation completes
 *
 * Note:        Releases the request, if connector callback succeeds
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_request_notify (void *obj, H5VL_request_notify_t cb, void *ctx) {
	herr_t err		   = 0;
	H5VL_log_req_t *rp = (H5VL_log_req_t *)obj;
	H5VL_log_req_notify_ctx_t *cp;

	cp		 = new H5VL_log_req_notify_ctx_t;
	cp->cb	 = cb;
	cp->cnt	 = rp->ureqs.size ();
	cp->ctx	 = ctx;
	cp->stat = H5VL_REQUEST_STATUS_SUCCEED;

	for (auto &ureq : rp->ureqs) {
		err = H5VLrequest_notify (ureq.req, rp->uvlid, H5VL_log_reqi_notify_cb, cp);
		CHECK_ERR
	}

	delete rp;

err_out:;
	return err;
} /* end H5VL_log_request_notify() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_request_cancel
 *
 * Purpose:     Cancels an asynchronous operation
 *
 * Note:        Releases the request, if connector callback succeeds
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_request_cancel (void *obj, H5VL_request_status_t *status) {
	herr_t err		   = 0;
	H5VL_log_req_t *rp = (H5VL_log_req_t *)obj;

	for (auto &ureq : rp->ureqs) {
		err = H5VLrequest_cancel (ureq.req, rp->uvlid, status);
		CHECK_ERR
	}

	delete rp;

err_out:;
	return err;
} /* end H5VL_log_request_cancel() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_request_specific_reissue
 *
 * Purpose:     Re-wrap vararg arguments into a va_list and reissue the
 *              request specific callback to the underlying VOL connector.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_request_specific_reissue (void *obj,
										  hid_t connector_id,
										  H5VL_request_specific_args_t *args) {
	herr_t ret_value;

	ret_value = H5VLrequest_specific (obj, connector_id, args);

	return ret_value;
} /* end H5VL_log_request_specific_reissue() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_request_specific
 *
 * Purpose:     Specific operation on a request
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_request_specific (void *obj, H5VL_request_specific_args_t *args) {
	herr_t err = 0;

	switch (args->op_type) {
		case H5VL_REQUEST_GET_ERR_STACK:
		// break;
		case H5VL_REQUEST_GET_EXEC_TIME:
		// break;
		default:
			RET_ERR ("specific_type not supported")
	}

err_out:;
	return err;
} /* end H5VL_log_request_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_request_optional
 *
 * Purpose:     Perform a connector-specific operation for a request
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_request_optional (void *obj, H5VL_optional_args_t *args) {
	herr_t err		   = 0;
	H5VL_log_req_t *rp = (H5VL_log_req_t *)obj;

	for (auto &ureq : rp->ureqs) {
		err = H5VLrequest_optional (ureq.req, rp->uvlid, args);
		CHECK_ERR
	}

err_out:;
	return err;
} /* end H5VL_log_request_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_request_free
 *
 * Purpose:     Releases a request, allowing the operation to complete without
 *              application tracking
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_request_free (void *obj) {
	herr_t err		   = 0;
	H5VL_log_req_t *rp = (H5VL_log_req_t *)obj;

	for (auto &ureq : rp->ureqs) {
		err = H5VLrequest_free (ureq.req, rp->uvlid);
		CHECK_ERR
	}

	delete rp;

err_out:;
	return err;
} /* end H5VL_log_request_free() */
