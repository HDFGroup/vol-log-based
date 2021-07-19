#pragma once

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <H5VLconnector.h>

struct H5VL_log_file_t;
typedef struct H5VL_log_obj_t {
	H5I_type_t type;
	void *uo;					 // Under obj
	hid_t uvlid;				 // Under VolID
	struct H5VL_log_file_t *fp;	 // File object

	H5VL_log_obj_t ();
	H5VL_log_obj_t (struct H5VL_log_obj_t *pp);
	H5VL_log_obj_t (struct H5VL_log_obj_t *pp, H5I_type_t type);
	H5VL_log_obj_t (struct H5VL_log_obj_t *pp, H5I_type_t type, void *uo);

	~H5VL_log_obj_t ();

#ifdef LOGVOL_DEBUG
	int ext_ref;
#endif
} H5VL_log_obj_t;

extern const H5VL_object_class_t H5VL_log_object_g;

void *H5VL_log_object_open (void *obj,
							const H5VL_loc_params_t *loc_params,
							H5I_type_t *opened_type,
							hid_t dxpl_id,
							void **req);
herr_t H5VL_log_object_copy (void *src_obj,
							 const H5VL_loc_params_t *src_loc_params,
							 const char *src_name,
							 void *dst_obj,
							 const H5VL_loc_params_t *dst_loc_params,
							 const char *dst_name,
							 hid_t ocpypl_id,
							 hid_t lcpl_id,
							 hid_t dxpl_id,
							 void **req);
herr_t H5VL_log_object_get (void *obj,
							const H5VL_loc_params_t *loc_params,
							H5VL_object_get_args_t *args,
							hid_t dxpl_id,
							void **req);
herr_t H5VL_log_object_specific (void *obj,
								 const H5VL_loc_params_t *loc_params,
								 H5VL_object_specific_args_t *args,
								 hid_t dxpl_id,
								 void **req);
herr_t H5VL_log_object_optional (void *obj,
								 const H5VL_loc_params_t *loc_params,
								 H5VL_optional_args_t *args,
								 hid_t dxpl_id,
								 void **req);

// Internal
extern void *H5VL_log_obj_open_with_uo (void *obj,
										void *uo,
										H5I_type_t type,
										const H5VL_loc_params_t *loc_params);
