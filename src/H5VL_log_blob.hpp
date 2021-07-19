#pragma once

#include <H5VLconnector.h>

/* Blob callbacks */
static herr_t H5VL_log_blob_put (void *obj, const void *buf, size_t size, void *blob_id, void *ctx);
static herr_t H5VL_log_blob_get (void *obj, const void *blob_id, void *buf, size_t size, void *ctx);
static herr_t H5VL_log_blob_specific (void *obj, void *blob_id, H5VL_blob_specific_args_t *args);
static herr_t H5VL_log_blob_optional (void *obj, void *blob_id, H5VL_optional_args_t *args);
