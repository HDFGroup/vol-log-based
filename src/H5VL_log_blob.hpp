#pragma once

#include <H5VLpublic.h>

/* Blob callbacks */
static herr_t H5VL_log_blob_put (void *obj, const void *buf, size_t size, void *blob_id, void *ctx);
static herr_t H5VL_log_blob_get (void *obj, const void *blob_id, void *buf, size_t size, void *ctx);
static herr_t H5VL_log_blob_specific (void *obj,
									  void *blob_id,
									  H5VL_blob_specific_t specific_type,
									  va_list arguments);
static herr_t H5VL_log_blob_optional (void *obj,
									  void *blob_id,
									  H5VL_blob_optional_t opt_type,
									  va_list arguments);
