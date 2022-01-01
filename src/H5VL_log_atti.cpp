#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "H5VL_log_atti.hpp"
#include "H5VL_logi.hpp"

herr_t H5VL_log_atti_iterate_op (hid_t location_id,
								 const char *attr_name,
								 const H5A_info_t *ainfo,
								 void *op_data) {
	H5VL_log_atti_iterate_op_data *ctx = (H5VL_log_atti_iterate_op_data *)op_data;

	// Skip internal objects
	if (attr_name && (attr_name[0] != '_' || attr_name[1] != '_')) {
		return ctx->op (location_id, attr_name, ainfo, ctx->op_data);
	}

	return 0;
}
