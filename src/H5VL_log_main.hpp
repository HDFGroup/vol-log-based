#pragma once

#include <H5VLconnector.h>

/* Characteristics of the pass-through VOL connector */
#define H5VL_log_NAME	 "LOG"
#define H5VL_log_VALUE	 1026 /* VOL connector ID */
#define H5VL_log_VERSION 2

/********************* */
/* Function prototypes */
/********************* */
herr_t H5VL_log_init (hid_t vipl_id);
herr_t H5VL_log_obj_term (void);
herr_t H5VL_log_optional (void *obj, H5VL_optional_args_t *args, hid_t dxpl_id, void **req);
