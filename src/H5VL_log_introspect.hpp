#pragma once

#include <H5VLpublic.h>

herr_t H5VL_log_introspect_get_conn_cls(void *obj, H5VL_get_conn_lvl_t lvl, const H5VL_class_t **conn_cls);
herr_t H5VL_log_introspect_opt_query(void *obj, H5VL_subclass_t cls, int opt_type, hbool_t *supported);
