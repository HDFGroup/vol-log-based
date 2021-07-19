#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <zlib.h>

#include <cstdlib>

#include "H5VL_logi.hpp"
#include "H5VL_logi_zip.hpp"

/* If out_len is large enough, compress the data at in and save it to out. out_len is set to actual
 * compressed size If out_len is NULL, we assume out is large enough for compressed data
 */
herr_t H5VL_log_zip_compress (void *in, int in_len, void *out, int *out_len) {
	herr_t err = 0;
	int zerr;

	// zlib struct
	z_stream defstream;
	defstream.zalloc   = Z_NULL;
	defstream.zfree	   = Z_NULL;
	defstream.opaque   = Z_NULL;
	defstream.avail_in = (uInt) (in_len);  // input size
	defstream.next_in  = (Bytef *)in;	   // input
	if (out_len != NULL) {
		defstream.avail_out = (uInt) (*out_len);  // output buffer size
	} else {
		defstream.avail_out = (uInt)1000000000;	 // Assume it is large enough
	}
	defstream.next_out = (Bytef *)out;	// output buffer

	// the actual compression work.
	zerr = deflateInit (&defstream, Z_DEFAULT_COMPRESSION);
	if (zerr != Z_OK) { ERR_OUT ("deflateInit fail") }
	zerr = deflate (&defstream, Z_FINISH);
	if (zerr != Z_STREAM_END) {
		if (zerr < 0) { ERR_OUT ("deflate fail") }
		return -1;
	}
	zerr = deflateEnd (&defstream);
	if (zerr != Z_OK) { ERR_OUT ("deflateEnd fail") }

	// If buffer not large enough
	if (defstream.avail_in > 0) { return -1; }

	// Size of comrpessed data
	if (out_len != NULL) { *out_len = defstream.total_out; }

err_out:;

	return err;
}

/* Compress the data at in and save it to a newly allocated buffer at out. out_len is set to actual
 * compressed data size The caller is responsible to free the buffer If out_len is not NULL, it will
 * be set to buffer size allocated
 */
herr_t H5VL_log_zip_compress_alloc (void *in, int in_len, void **out, int *out_len) {
	herr_t err = 0;
	int zerr;
	int bsize;	// Start by 1/8 of the in_len
	char *buf;

	bsize = in_len >> 3;
	if (bsize < 6) { bsize = 6; }
	buf = (char *)malloc (bsize);

	// zlib struct
	z_stream defstream;
	defstream.zalloc	= Z_NULL;
	defstream.zfree		= Z_NULL;
	defstream.opaque	= Z_NULL;
	defstream.avail_in	= (uInt) (in_len);	// input size
	defstream.next_in	= (Bytef *)in;		// input
	defstream.avail_out = (uInt) (bsize);	// output buffer size
	defstream.next_out	= (Bytef *)buf;		// output buffer

	// Initialize deflat stream
	zerr = deflateInit (&defstream, Z_DEFAULT_COMPRESSION);
	if (zerr != Z_OK) { ERR_OUT ("deflateInit fail") }

	// The actual compression work
	zerr = Z_OK;
	while (zerr != Z_STREAM_END) {
		// Compress data
		zerr = deflate (&defstream, Z_NO_FLUSH | Z_FINISH);
		// Check if buffer is lage enough
		if (zerr != Z_STREAM_END) {
			// Enlarge buffer
			buf = (char *)realloc (buf, bsize << 1);

			// Reset buffer info in stream
			defstream.next_out	= (Bytef *)(buf + bsize);
			defstream.avail_out = bsize;

			// Reocrd new buffer size
			bsize <<= 1;
		}
	}

	// Finalize deflat stream
	zerr = deflateEnd (&defstream);
	if (zerr != Z_OK) { ERR_OUT ("deflateEnd fail") }

	// Size of comrpessed data
	if (out_len != NULL) { *out_len = defstream.total_out; }

	// Compressed data
	*out = buf;

err_out:;

	return err;
}

/* If out_len is large enough, decompress the data at in and save it to out. out_len is set to
 * actual decompressed size If out_len is NULL, we assume out is large enough for decompressed data
 */
herr_t H5VL_log_zip_decompress (void *in, int in_len, void *out, int *out_len) {
	herr_t err = 0;
	int zerr;

	// zlib struct
	z_stream infstream;
	infstream.zalloc   = Z_NULL;
	infstream.zfree	   = Z_NULL;
	infstream.opaque   = Z_NULL;
	infstream.avail_in = (unsigned long)in_len;	 // input size
	infstream.next_in  = (Bytef *)in;			 // input
	if (out_len != NULL) {
		infstream.avail_out = (uInt) (*out_len);  // output buffer size
	} else {
		infstream.avail_out = (uInt)1000000000;	 // Assume it is large enough
	}
	infstream.next_out = (Bytef *)out;	// buffer size

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
	if (out_len != NULL) { *out_len = infstream.total_out; }

err_out:;

	return err;
}

/* Decompress the data at in and save it to a newly allocated buffer at out. out_len is set to
 * actual decompressed data size The caller is responsible to free the buffer If out_len is not
 * NULL, it will be set to buffer size allocated
 */
herr_t H5VL_log_zip_decompress_alloc (void *in, int in_len, void **out, int *out_len) {
	herr_t err = 0;
	int zerr;
	int bsize = in_len << 1;  // Start by 2 times of the in_len
	char *buf;

	buf = (char *)malloc (bsize);

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
			// Enlarge buffer
			buf = (char *)realloc (buf, bsize << 1);

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

	// Size of comrpessed data
	if (out_len != NULL) { *out_len = infstream.total_out; }

	// Compressed data
	*out = buf;

err_out:;

	return err;
}
