#include <H5VLconnector.h>

#include "H5VL_logi_filter.hpp"

herr_t H5VL_logi_filter_deflate (
	H5VL_log_filter_t &fp, void *in, int in_len, void *out, int *out_len);
herr_t H5VL_logi_filter_deflate_alloc (
	H5VL_log_filter_t &fp, void *in, int in_len, void **out, int *out_len);
herr_t H5VL_logi_filter_inflate (
	H5VL_log_filter_t &fp, void *in, int in_len, void *out, int *out_len);
herr_t H5VL_logi_filter_inflate_alloc (
	H5VL_log_filter_t &fp, void *in, int in_len, void **out, int *out_len);