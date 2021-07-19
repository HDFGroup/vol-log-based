#pragma once

#include <H5VLconnector.h>

#include "H5VL_log_file.hpp"

/* The log VOL wrapper context */
/*
typedef struct H5VL_log_obj_t {
	void *uctx;	  // Under context
	hid_t uvlid;  // Under VolID
	H5VL_log_file_t *fp;
} H5VL_log_obj_t;
*/

extern void *H5VL_log_get_object (const void *obj);
extern herr_t H5VL_log_get_wrap_ctx (const void *obj, void **wrap_ctx);
extern void *H5VL_log_wrap_object (void *obj, H5I_type_t obj_type, void *_wrap_ctx);
extern void *H5VL_log_unwrap_object (void *obj);
extern herr_t H5VL_log_free_wrap_ctx (void *_wrap_ctx);
