#include <zlib.h>

#include "logvol_internal.hpp"

#ifdef ENABLE_ZLIB
herr_t logvol_zip_compress(void *in, int in_len, void *out, int *out_len);
herr_t logvol_zip_compress_alloc(void *in, int in_len, void **out, int *out_len);
herr_t logvol_zip_decompress(void *in, int in_len, void *out, int *out_len);
herr_t logvol_zip_decompress_alloc(void *in, int in_len, void **out, int *out_len);
#endif