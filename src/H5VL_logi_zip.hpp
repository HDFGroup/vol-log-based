#pragma once

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <H5VLconnector.h>

#ifdef ENABLE_ZLIB
herr_t H5VL_log_zip_compress (void *in, int in_len, void *out, int *out_len);
herr_t H5VL_log_zip_compress_alloc (void *in, int in_len, void **out, int *out_len);
herr_t H5VL_log_zip_decompress (void *in, int in_len, void *out, int *out_len);
herr_t H5VL_log_zip_decompress_alloc (void *in, int in_len, void **out, int *out_len);
#endif