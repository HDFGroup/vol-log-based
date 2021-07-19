#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <zlib.h>

#include <cstdlib>

#include "H5VL_logi_err.hpp"
#include "H5VL_logi_filter.hpp"
#include "H5VL_logi_filter_deflate.hpp"

herr_t H5VL_logi_filter_deflate (
	H5VL_log_filter_t &fp, void *in, int in_len, void *out, int *out_len) {
	herr_t err = 0;
	int zerr;

	// zlib struct
	z_stream defstream;
	defstream.zalloc	= Z_NULL;
	defstream.zfree		= Z_NULL;
	defstream.opaque	= Z_NULL;
	defstream.avail_in	= (uInt) (in_len);	  // input size
	defstream.next_in	= (Bytef *)in;		  // input
	defstream.avail_out = (uInt) (*out_len);  // output buffer size
	defstream.next_out	= (Bytef *)out;		  // output buffer

	// the actual compression work.
	zerr = deflateInit (&defstream, Z_DEFAULT_COMPRESSION);
	if (zerr != Z_OK) { RET_ERR ("deflateInit fail") }
	zerr = deflate (&defstream, Z_FINISH);
	if (zerr != Z_STREAM_END) {
		if (zerr < 0) { RET_ERR ("deflate fail") }
	}
	zerr = deflateEnd (&defstream);
	if (zerr != Z_OK) { RET_ERR ("deflateEnd fail") }

	// If buffer not large enough
	if (defstream.avail_in > 0) { RET_ERR ("buffer too small") }

	// Size of comrpessed data
	*out_len = defstream.total_out;

err_out:;
	return err;
}

/* Compress the data at in and save it to a newly allocated buffer at out. out_len is set to actual
 * compressed data size The caller is responsible to free the buffer If out_len is not NULL, it will
 * be set to buffer size allocated
 */
herr_t H5VL_logi_filter_deflate_alloc (
	H5VL_log_filter_t &fp, void *in, int in_len, void **out, int *out_len) {
	herr_t err = 0;
	int zerr;
	unsigned long bsize;  // Start by 1/8 of the in_len
	char *buf;
	z_stream defstream;

	// zlib struct
	defstream.zalloc = Z_NULL;
	defstream.zfree	 = Z_NULL;
	defstream.opaque = Z_NULL;

	// Initialize deflat stream
	zerr = deflateInit (&defstream, Z_DEFAULT_COMPRESSION);
	if (zerr != Z_OK) { ERR_OUT ("deflateInit fail") }

	// Prepare the buffer
	bsize = deflateBound (&defstream, (unsigned long)in_len);
	if (bsize > *out_len) {
		buf = (char *)malloc (bsize);
		CHECK_PTR (buf)
		*out = buf;
	} else {
		buf = (char *)*out;
	}

	// The actual compression work
	defstream.avail_in	= (uInt) (in_len);	// input size
	defstream.next_in	= (Bytef *)in;		// input
	defstream.avail_out = (uInt) (bsize);	// output buffer size
	defstream.next_out	= (Bytef *)buf;		// output buffer

	zerr = deflate (&defstream, Z_FINISH);
	if (zerr != Z_STREAM_END) { ERR_OUT ("deflate fail") }

	// Finalize deflat stream
	zerr = deflateEnd (&defstream);
	if (zerr != Z_OK) { ERR_OUT ("deflateEnd fail") }

	// Size and data
	*out_len = defstream.total_out;
	*out	 = buf;

err_out:;
	return err;
}

/* If out_len is large enough, decompress the data at in and save it to out. out_len is set to
 * actual decompressed size If out_len is NULL, we assume out is large enough for decompressed data
 */
herr_t H5VL_logi_filter_inflate (
	H5VL_log_filter_t &fp, void *in, int in_len, void *out, int *out_len) {
	herr_t err = 0;
	int zerr;

	// zlib struct
	z_stream infstream;
	infstream.zalloc	= Z_NULL;
	infstream.zfree		= Z_NULL;
	infstream.opaque	= Z_NULL;
	infstream.avail_in	= (unsigned long)in_len;  // input size
	infstream.next_in	= (Bytef *)in;			  // input
	infstream.avail_out = (uInt) (*out_len);	  // output buffer size
	infstream.next_out	= (Bytef *)out;			  // buffer size

	// the actual decompression work.
	zerr = inflateInit (&infstream);
	if (zerr != Z_OK) { ERR_OUT ("inflateInit fail") }
	zerr = inflate (&infstream, Z_FINISH);
	if (zerr != Z_STREAM_END) { ERR_OUT ("inflate fail") }
	zerr = inflateEnd (&infstream);
	if (zerr != Z_OK) { ERR_OUT ("inflateEnd fail") }

	// If buffer not large enough
	if (infstream.avail_in > 0) { ERR_OUT ("buffer too small") }

	// Size of decomrpessed data
	*out_len = infstream.total_out;

err_out:;
	return err;
}

/* Decompress the data at in and save it to a newly allocated buffer at out. out_len is set to
 * actual decompressed data size The caller is responsible to free the buffer If out_len is not
 * NULL, it will be set to buffer size allocated
 */
herr_t H5VL_logi_filter_inflate_alloc (
	H5VL_log_filter_t &fp, void *in, int in_len, void **out, int *out_len) {
	herr_t err = 0;
	int zerr;
	int bsize;
	char *buf;

	// Prepare buf
	bsize = *out_len;
	if (bsize) {
		buf = (char *)*out;
	} else {
		// zlib does not provide upper bound of inflation size, guess input size * 2
		bsize = in_len << 1;
		buf	  = (char *)malloc (bsize);
	}

	// zlib struct
	z_stream infstream;
	infstream.zalloc	= Z_NULL;
	infstream.zfree		= Z_NULL;
	infstream.opaque	= Z_NULL;
	infstream.avail_in	= (uInt) (in_len);	// input size
	infstream.next_in	= (Bytef *)in;		// input
	infstream.avail_out = (uInt) (bsize);	// output buffer size
	infstream.next_out	= (Bytef *)buf;		// output buffer

	// Initialize deflat stream
	zerr = inflateInit (&infstream);
	if (zerr != Z_OK) { ERR_OUT ("inflateInit fail") }

	// The actual decompression work
	zerr = Z_OK;
	while (zerr != Z_STREAM_END) {
		// Compress data
		zerr = inflate (&infstream, Z_NO_FLUSH | Z_FINISH);
		// Check if buffer is lage enough
		if (zerr != Z_STREAM_END) {
			// Enlarge buffer if not enough
			buf = (char *)realloc (buf, bsize << 1);
			CHECK_PTR (buf)

			// Reset buffer info in stream
			infstream.next_out	= (Bytef *)(buf + bsize);
			infstream.avail_out = bsize;

			// Reocrd new buffer size
			bsize <<= 1;
		}
	}

	// Finalize deflat stream
	zerr = inflateEnd (&infstream);
	if (zerr != Z_OK) { ERR_OUT ("inflateEnd fail") }

	// Size and data
	*out_len = infstream.total_out;
	*out	 = buf;

err_out:;

	return err;
}
