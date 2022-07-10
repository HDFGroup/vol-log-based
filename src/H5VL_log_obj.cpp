/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "H5VL_log_dataseti.hpp"
#include "H5VL_log_filei.hpp"
#include "H5VL_log_obj.hpp"
#include "H5VL_log_obji.hpp"
#include "H5VL_logi.hpp"
#include "H5VL_logi_util.hpp"

/********************* */
/* Function prototypes */
/********************* */

const H5VL_object_class_t H5VL_log_object_g {
    H5VL_log_object_open,     /* open */
    H5VL_log_object_copy,     /* copy */
    H5VL_log_object_get,      /* get */
    H5VL_log_object_specific, /* specific */
    H5VL_log_object_optional, /* optional */
};

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_object_open
 *
 * Purpose:     Opens an object inside a container.
 *
 * Return:      Success:    Pointer to object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *H5VL_log_object_open (void *obj,
                            const H5VL_loc_params_t *loc_params,
                            H5I_type_t *opened_type,
                            hid_t dxpl_id,
                            void **req) {
    // herr_t err = 0;
    H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
    void *ret          = NULL;
    void *uo;
    char *iname               = NULL;  // Internal name of object
    const char *original_name = NULL;  // Original value in loc_params before being remapped

    try {
#ifdef LOGVOL_DEBUG
        if (H5VL_logi_debug_verbose ()) {
            printf ("H5VL_log_object_open(%p, %p, %p, dxplid, %p)\n", obj, loc_params, opened_type,
                    req);
        }
#endif
        /* Rename user objects to avoid conflict with internal object */
        if (loc_params->type == H5VL_OBJECT_BY_NAME) {
            original_name = loc_params->loc_data.loc_by_name.name;
            if (op->fp->is_log_based_file) {
                iname         = H5VL_logi_name_remap (original_name);
                ((H5VL_loc_params_t *)loc_params)->loc_data.loc_by_name.name = iname;
            }
        }

        uo = H5VLobject_open (op->uo, loc_params, op->uvlid, opened_type, dxpl_id, req);
        CHECK_PTR (uo);

        if (op->fp->is_log_based_file) {
            if (*opened_type == H5I_DATASET) {
                ret = H5VL_log_dataseti_open (obj, uo, dxpl_id);
            } else {
                ret = H5VL_log_obj_open_with_uo (obj, uo, *opened_type, loc_params);
            }
        } else {
            ret = uo;
        }
        
    }
    H5VL_LOGI_EXP_CATCH

err_out:;
    if (iname && iname != original_name) { free (iname); }
    // Restore name in loc_param
    if (original_name) {
        ((H5VL_loc_params_t *)loc_params)->loc_data.loc_by_name.name = original_name;
    }
    return ret;
} /* end H5VL_log_object_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_object_copy
 *
 * Purpose:     Copies an object inside a container.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_object_copy (void *src_obj,
                             const H5VL_loc_params_t *src_loc_params,
                             const char *src_name,
                             void *dst_obj,
                             const H5VL_loc_params_t *dst_loc_params,
                             const char *dst_name,
                             hid_t ocpypl_id,
                             hid_t lcpl_id,
                             hid_t dxpl_id,
                             void **req) {
    herr_t err                  = 0;
    H5VL_log_obj_t *o_src       = (H5VL_log_obj_t *)src_obj;
    H5VL_log_obj_t *o_dst       = (H5VL_log_obj_t *)dst_obj;
    char *iname_s               = NULL;  // Internal name of object
    const char *original_name_s = NULL;  // Original value in loc_params before being remapped
    char *iname_d               = NULL;  // Internal name of object
    const char *original_name_d = NULL;  // Original value in loc_params before being remapped

    try {
#ifdef LOGVOL_DEBUG
        if (H5VL_logi_debug_verbose ()) {
            printf (
                "H5VL_log_object_copy(%p, %p, %s, %p, %p, %s, ocpypl_id, lcpl_id, dxplid, %p)\n",
                src_obj, src_loc_params, src_name, dst_obj, dst_loc_params, dst_name, req);
        }
#endif  
        if (o_src->fp->is_log_based_file || o_dst->fp->is_log_based_file) {
            ERR_OUT ("H5VL_log_object_copy Not Supported")
        }
        

        /* Rename user objects to avoid conflict with internal object */
        if (src_loc_params->type == H5VL_OBJECT_BY_NAME && o_src->fp->is_log_based_file) {
            original_name_s = src_loc_params->loc_data.loc_by_name.name;
            iname_s         = H5VL_logi_name_remap (original_name_s);
            ((H5VL_loc_params_t *)src_loc_params)->loc_data.loc_by_name.name = iname_s;
        }
        if (dst_loc_params->type == H5VL_OBJECT_BY_NAME && o_dst->fp->is_log_based_file) {
            original_name_d = dst_loc_params->loc_data.loc_by_name.name;
            iname_d         = H5VL_logi_name_remap (original_name_d);
            ((H5VL_loc_params_t *)dst_loc_params)->loc_data.loc_by_name.name = iname_d;
        }

        err = H5VLobject_copy (o_src->uo, src_loc_params, src_name, o_dst->uo, dst_loc_params,
                               dst_name, o_src->uvlid, ocpypl_id, lcpl_id, dxpl_id, req);
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    if (iname_s && iname_s != original_name_s) { free (iname_s); }
    // Restore name in loc_param
    if (original_name_s) {
        ((H5VL_loc_params_t *)src_loc_params)->loc_data.loc_by_name.name = original_name_s;
    }
    if (iname_d && iname_d != original_name_d) { free (iname_d); }
    // Restore name in loc_param
    if (original_name_d) {
        ((H5VL_loc_params_t *)dst_loc_params)->loc_data.loc_by_name.name = original_name_d;
    }
    return err;
} /* end H5VL_log_object_copy() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_object_get
 *
 * Purpose:     Get info about an object
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_object_get (void *obj,
                            const H5VL_loc_params_t *loc_params,
                            H5VL_object_get_args_t *args,
                            hid_t dxpl_id,
                            void **req) {
    herr_t err                = 0;
    H5VL_log_obj_t *op        = (H5VL_log_obj_t *)obj;
    char *iname               = NULL;  // Internal name of object
    const char *original_name = NULL;  // Original value in loc_params before being remapped

    try {
#ifdef LOGVOL_DEBUG
        if (H5VL_logi_debug_verbose ()) {
            printf ("H5VL_log_object_get(%p, %p, args, dxplid, %p, ...)\n", obj, loc_params, req);
        }
#endif

        // Block access to internal objects
        if (op->fp->is_log_based_file) {
            switch (loc_params->type) {
                case H5VL_OBJECT_BY_NAME:
                    /* Rename user objects to avoid conflict with internal object */
                    original_name = loc_params->loc_data.loc_by_name.name;
                    iname         = H5VL_logi_name_remap (original_name);
                    ((H5VL_loc_params_t *)loc_params)->loc_data.loc_by_name.name = iname;
                    break;
                case H5VL_OBJECT_BY_SELF:
                    break;
                case H5VL_OBJECT_BY_IDX:
                case H5VL_OBJECT_BY_TOKEN:
                    RET_ERR ("Access by idx annd token is not supported")
                    break;
            }
        }

        err = H5VLobject_get (op->uo, loc_params, op->uvlid, args, dxpl_id, req);
        CHECK_ERR

        // Deduct internal attirbutes from num_attrs
        if (args->op_type == H5VL_OBJECT_GET_INFO) {
            if (op->type == H5I_FILE) {
                args->args.get_info.oinfo->num_attrs -= 1;
            } else if (op->type == H5I_DATASET) {
                args->args.get_info.oinfo->num_attrs -= 3;
            }
        }
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    if (iname && iname != original_name) { free (iname); }
    // Restore name in loc_param
    if (original_name) {
        ((H5VL_loc_params_t *)loc_params)->loc_data.loc_by_name.name = original_name;
    }
    return err;
} /* end H5VL_log_object_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_object_specific
 *
 * Purpose:     Specific operation on an object
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_object_specific (void *obj,
                                 const H5VL_loc_params_t *loc_params,
                                 H5VL_object_specific_args_t *args,
                                 hid_t dxpl_id,
                                 void **req) {
    herr_t err                         = 0;
    H5VL_log_obj_t *op                 = (H5VL_log_obj_t *)obj;
    H5VL_log_obji_iterate_op_data *ctx = NULL;
    char *iname                        = NULL;  // Internal name of object
    const char *original_name = NULL;  // Original value in loc_params before being remapped

    try {
#ifdef LOGVOL_DEBUG
        if (H5VL_logi_debug_verbose ()) {
            printf ("H5VL_log_object_specific(%p, %p, args, dxplid ,%p, ...)\n", obj, loc_params,
                    req);
        }
#endif
        if (!op->fp->is_log_based_file) {
            // TODO: Save copy of underlying VOL connector ID, in case of
            // 'refresh' operation destroying the current object
            err = H5VLobject_specific (op->uo, loc_params, op->uvlid, args, dxpl_id, req);
            CHECK_ERR;
            return err;
        }
        // Block access to internal objects
        switch (loc_params->type) {
            case H5VL_OBJECT_BY_NAME:
                /* Rename user objects to avoid conflict with internal object */
                original_name = loc_params->loc_data.loc_by_name.name;
                iname         = H5VL_logi_name_remap (original_name);
                ((H5VL_loc_params_t *)loc_params)->loc_data.loc_by_name.name = iname;
                break;
            case H5VL_OBJECT_BY_SELF:
                break;
            case H5VL_OBJECT_BY_IDX:
            case H5VL_OBJECT_BY_TOKEN:
                RET_ERR ("Access by idx annd token is not supported")
                break;
        }

        // Replace H5Oiterate/visit callback with logvol wrpapper
        if (args->op_type == H5VL_OBJECT_VISIT) {
            ctx = (H5VL_log_obji_iterate_op_data *)malloc (sizeof (H5VL_log_obji_iterate_op_data));
            ctx->op                  = args->args.visit.op;
            ctx->op_data             = args->args.visit.op_data;
            args->args.visit.op      = H5VL_log_obji_iterate_op;
            args->args.visit.op_data = ctx;
        }

        err = H5VLobject_specific (op->uo, loc_params, op->uvlid, args, dxpl_id, req);
        CHECK_ERR

        if (args->op_type == H5VL_OBJECT_VISIT) {
            args->args.visit.op      = ctx->op;
            args->args.visit.op_data = ctx->op_data;
        }
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    if (ctx) { free (ctx); }
    if (iname && iname != original_name) { free (iname); }
    // Restore name in loc_param
    if (original_name) {
        ((H5VL_loc_params_t *)loc_params)->loc_data.loc_by_name.name = original_name;
    }
    return err;
} /* end H5VL_log_object_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_object_optional
 *
 * Purpose:     Perform a connector-specific operation for an object
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_object_optional (void *obj,
                                 const H5VL_loc_params_t *loc_params,
                                 H5VL_optional_args_t *args,
                                 hid_t dxpl_id,
                                 void **req) {
    herr_t err                = 0;
    H5VL_log_obj_t *op        = (H5VL_log_obj_t *)obj;
    char *iname               = NULL;  // Internal name of object
    const char *original_name = NULL;  // Original value in loc_params before being remapped

    try {
#ifdef LOGVOL_DEBUG
        if (H5VL_logi_debug_verbose ()) {
            printf ("H5VL_log_object_optional(%p, args, dxplid, %p, ...)\n", obj, req);
        }
#endif

        if (op->fp->is_log_based_file) {
            // Block access to internal objects
            switch (loc_params->type) {
                case H5VL_OBJECT_BY_NAME:
                    /* Rename user objects to avoid conflict with internal object */
                    original_name = loc_params->loc_data.loc_by_name.name;
                    iname         = H5VL_logi_name_remap (original_name);
                    ((H5VL_loc_params_t *)loc_params)->loc_data.loc_by_name.name = iname;
                    break;
                case H5VL_OBJECT_BY_SELF:
                    break;
                case H5VL_OBJECT_BY_IDX:
                case H5VL_OBJECT_BY_TOKEN:
                    RET_ERR ("Access by idx annd token is not supported")
                    break;
            }
        }
        

        err = H5VLobject_optional (op->uo, loc_params, op->uvlid, args, dxpl_id, req);
        CHECK_ERR
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    if (iname && iname != original_name) { free (iname); }
    // Restore name in loc_param
    if (original_name) {
        ((H5VL_loc_params_t *)loc_params)->loc_data.loc_by_name.name = original_name;
    }
    return err;
} /* end H5VL_log_object_optional() */
