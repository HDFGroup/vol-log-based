#pragma once

#include <H5VLconnector.h>

typedef struct H5VL_log_info_t {
	hid_t uvlid;		  /* VOL ID for under VOL */
	void *under_vol_info; /* VOL info for under VOL */
} H5VL_log_info_t;

extern void *H5VL_log_info_copy (const void *_info);
extern herr_t H5VL_log_info_cmp (int *cmp_value, const void *_info1, const void *_info2);
extern herr_t H5VL_log_info_free (void *_info);
extern herr_t H5VL_log_info_to_str (const void *_info, char **str);
extern herr_t H5VL_log_str_to_info (const char *str, void **_info);