#pragma once
#include <H5VLconnector.h>

#include <atomic>
#include <vector>

typedef struct H5VL_log_req_notify_ctx_t {
	H5VL_request_notify_t cb;
	void *ctx;
	std::atomic<int> cnt;
	H5VL_request_status_t stat;
} H5VL_log_req_notify_ctx_t;

typedef struct H5VL_log_ureq_t {
	void *req;
	H5VL_request_status_t stat;
} H5VL_log_ureq_t;

/* The log VOL dataset object */
typedef struct H5VL_log_req_t : H5VL_log_obj_t {
	std::vector<H5VL_log_ureq_t> ureqs;
	using H5VL_log_obj_t::H5VL_log_obj_t;
	void append (void *uo);
} H5VL_log_req_t;

herr_t H5VL_log_request_wait (void *obj, uint64_t timeout, H5VL_request_status_t *status);
herr_t H5VL_log_request_notify (void *obj, H5VL_request_notify_t cb, void *ctx);
herr_t H5VL_log_request_cancel (void *obj, H5VL_request_status_t *status);
herr_t H5VL_log_request_specific_reissue (void *obj,
										  hid_t connector_id,
										  H5VL_request_specific_args_t *args,
										  ...);
herr_t H5VL_log_request_specific (void *obj, H5VL_request_specific_args_t *args);
herr_t H5VL_log_request_optional (void *obj, H5VL_optional_args_t *args);
herr_t H5VL_log_request_free (void *obj);