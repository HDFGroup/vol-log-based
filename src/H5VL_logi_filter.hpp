#pragma once

#include <H5VLconnector.h>
#include <hdf5.h>

#include <vector>

#define LOGVOL_FILTER_CD_MAX   0x08
#define LOGVOL_FILTER_NAME_MAX 0x40
typedef struct H5VL_log_filter_t {
	H5Z_filter_t id;
	unsigned int flags;
	size_t cd_nelmts;
	unsigned int cd_values[LOGVOL_FILTER_CD_MAX];
	char name[LOGVOL_FILTER_NAME_MAX];
	unsigned int filter_config;

	H5VL_log_filter_t ();
} H5VL_log_filter_t;

typedef std::vector<H5VL_log_filter_t> H5VL_log_filter_pipeline_t;

herr_t H5VL_logi_filter (
	H5VL_log_filter_pipeline_t &pipeline, void *in, int in_len, void **out, int *out_len);

herr_t H5VL_logi_unfilter (
	H5VL_log_filter_pipeline_t &pipeline, void *in, int in_len, void **out, int *out_len);