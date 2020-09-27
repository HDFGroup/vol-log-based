#pragma once

#include <H5VLpublic.h>

#ifdef ENABLE_ZLIB
#include <zlib.h>
herr_t H5VL_log_zip_compress(void *in, int in_len, void *out, int *out_len);
herr_t H5VL_log_zip_compress_alloc(void *in, int in_len, void **out, int *out_len);
herr_t H5VL_log_zip_decompress(void *in, int in_len, void *out, int *out_len);
herr_t H5VL_log_zip_decompress_alloc(void *in, int in_len, void **out, int *out_len);
#endif