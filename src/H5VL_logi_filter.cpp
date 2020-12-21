#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
#include <vector>

#include "H5VL_logi_err.hpp"
#include "H5VL_logi_filter.hpp"
#include "H5VL_logi_filter_deflate.hpp"
#include "H5VL_logi_mem.hpp"
#include "hdf5.h"

herr_t H5VL_logi_filter (
	H5VL_log_filter_pipeline_t &pipeline, void *in, int in_len, void **out, int *out_len) {
	herr_t err = 0;
	int i;
	int bsize[2], size_in, size_out;
	char *buf[2], *bin, *bout;

	bsize[0] = bsize[1] = 0;
	buf[0] = buf[1] = NULL;

	for (i = 0; i < pipeline.size (); i++) {
		// Alternate input and output buffer
		bin		 = buf[i & 1];
		bout	 = buf[(i + 1) & 1];
		size_out = bsize[(i + 1) & 1];

		// First iteration read from user buffer
		if (i == 0) {
			bin		= (char *)in;
			size_in = in_len;
		}
		// Last iteration write to user buffer
		if (i == pipeline.size () - 1) {
			bout	 = (char *)*out;
			size_out = *out_len;
		}

		switch (pipeline[i].id) {
			case H5Z_FILTER_DEFLATE: {
				err = H5VL_logi_filter_deflate_alloc (pipeline[i], bin, size_in, (void **)&bout,
													  &size_out);
				CHECK_ERR
			} break;
			default:
				ERR_OUT ("Filter not supported")
				break;
		}

		if (i == pipeline.size () - 1) {
			*out_len = size_out;
			*out	 = bout;
		} else {
			if (bsize[(i + 1) & 1] < size_out) { bsize[(i + 1) & 1] = size_out; }
			buf[(i + 1) & 1] = bout;
			size_in			 = size_out;
		}
	}

err_out:;
	H5VL_log_free (buf[0]) H5VL_log_free (buf[1]) return err;
}

herr_t H5VL_logi_unfilter (
	H5VL_log_filter_pipeline_t &pipeline, void *in, int in_len, void **out, int *out_len) {
	herr_t err = 0;
	int i;
	int bsize[2], size_in, size_out;
	char *buf[2], *bin, *bout;

	bsize[0] = bsize[1] = 0;
	buf[0] = buf[1] = NULL;

	for (i = pipeline.size () - 1; i > -1; i--) {
		// Alternate input and output buffer
		bin		 = buf[i & 1];
		bout	 = buf[(i + 1) & 1];
		size_out = bsize[(i + 1) & 1];

		// First iteration read from user buffer
		if (i == 0) {
			bout	 = (char *)*out;
			size_out = *out_len;
		}
		// Last iteration write to user buffer
		if (i == pipeline.size () - 1) {
			bin		= (char *)in;
			size_in = in_len;
		}

		switch (pipeline[i].id) {
			case H5Z_FILTER_DEFLATE: {
				err = H5VL_logi_filter_deflate_alloc (pipeline[i], bin, size_in, (void **)&bout,
													  &size_out);
				CHECK_ERR
			} break;
			default:
				ERR_OUT ("Filter not supported")
				break;
		}

		if (i == 0) {
			*out	 = bout;
			*out_len = size_out;
		} else {
			buf[(i + 1) & 1] = bout;
			if (bsize[(i + 1) & 1] < size_out) { bsize[(i + 1) & 1] = size_out; }
			size_in = size_out;
		}
	}

err_out:;
	H5VL_log_free (buf[0]) H5VL_log_free (buf[1]) return err;
}

H5VL_log_filter_t::H5VL_log_filter_t () { this->cd_nelmts = LOGVOL_FILTER_CD_MAX; }