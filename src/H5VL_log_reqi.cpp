#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <H5VLconnector.h>

#include <cassert>
#include <cstdlib>

#include "H5VL_log_obj.hpp"
#include "H5VL_log_req.hpp"
#include "H5VL_log_reqi.hpp"
#include "H5VL_logi.hpp"
#include "H5VL_logi_debug.hpp"

herr_t H5VL_log_reqi_notify_cb (void *ctx, H5VL_request_status_t status) {
	herr_t err = 0;
	int i;
	H5VL_log_req_notify_ctx_t *cp = (H5VL_log_req_notify_ctx_t *)ctx;

	if (status == H5VL_REQUEST_STATUS_FAIL) {
		cp->stat = H5VL_REQUEST_STATUS_FAIL;
	} else if (cp->stat == H5VL_REQUEST_STATUS_SUCCEED) {
		cp->stat = status;
	}

	if (--cp->cnt == 0) {
		err = cp->cb (cp->ctx, cp->stat);
		delete cp;
	}

	return err;
}

void H5VL_log_req_t::append (void *uo) {
	H5VL_log_ureq_t ureq;

	ureq.stat = H5VL_REQUEST_STATUS_IN_PROGRESS;
	ureq.req  = uo;
	this->ureqs.push_back (ureq);
}