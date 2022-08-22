/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "H5VL_log_att.hpp"
#include "H5VL_log_atti.hpp"
#include "H5VL_log_filei.hpp"
#include "H5VL_log_obj.hpp"
#include "H5VL_log_req.hpp"
#include "H5VL_logi.hpp"
#include "H5VL_logi_util.hpp"

/********************* */
/* Function prototypes */
/********************* */

const H5VL_attr_class_t H5VL_log_attr_g {
    H5VL_log_attr_create,   /* create       */
    H5VL_log_attr_open,     /* open         */
    H5VL_log_attr_read,     /* read         */
    H5VL_log_attr_write,    /* write        */
    H5VL_log_attr_get,      /* get          */
    H5VL_log_attr_specific, /* specific     */
    H5VL_log_attr_optional, /* optional     */
    H5VL_log_attr_close     /* close        */
};

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_attr_create
 *
 * Purpose:     Creates an attribute on an object.
 *
 * Return:      Success:    Pointer to attribute object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *H5VL_log_attr_create (void *obj,
                            const H5VL_loc_params_t *loc_params,
                            const char *name,
                            hid_t type_id,
                            hid_t space_id,
                            hid_t acpl_id,
                            hid_t aapl_id,
                            hid_t dxpl_id,
                            void **req) {
    H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
    H5VL_log_obj_t *ap = NULL;
    H5VL_log_req_t *rp;
    char *iname = NULL;  // Internal name of object
    void **ureqp, *ureq;

    try {
        H5VL_LOGI_PROFILING_TIMER_START;

        /* Check arguments */
        if (op->fp->is_log_based_file) {
            iname = H5VL_logi_name_remap (name);
        } else {
            iname = (char *)name;
        }

        ap = new H5VL_log_obj_t (op, H5I_ATTR);

        if (req) {
            rp    = new H5VL_log_req_t ();
            ureqp = &ureq;
        } else {
            ureqp = NULL;
        }

        H5VL_LOGI_PROFILING_TIMER_START;
        ap->uo = H5VLattr_create (op->uo, loc_params, op->uvlid, iname, type_id, space_id, acpl_id,
                                  aapl_id, dxpl_id, ureqp);
        H5VL_LOGI_PROFILING_TIMER_STOP (ap->fp, TIMER_H5VLATT_CREATE);
        CHECK_PTR (ap->uo);

        if (req) {
            rp->append (ureq);
            *req = rp;
        }

        H5VL_LOGI_PROFILING_TIMER_STOP (ap->fp, TIMER_H5VL_LOG_ATT_CREATE);
    }
    H5VL_LOGI_EXP_CATCH

    if (iname && iname != name) { free (iname); }
    return (void *)ap;

err_out:;
    if (ap) { delete ap; }
    if (iname && iname != name) { free (iname); }

    return NULL;
} /* end H5VL_log_attr_create() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_attr_open
 *
 * Purpose:     Opens an attribute on an object.
 *
 * Return:      Success:    Pointer to attribute object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *H5VL_log_attr_open (void *obj,
                          const H5VL_loc_params_t *loc_params,
                          const char *name,
                          hid_t aapl_id,
                          hid_t dxpl_id,
                          void **req) {
    H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
    H5VL_log_obj_t *ap = NULL;
    H5VL_log_req_t *rp;
    char *iname = NULL;  // Internal name of object
    void **ureqp, *ureq;

    try {
        H5VL_LOGI_PROFILING_TIMER_START;

        if (op->fp->is_log_based_file) {
            /* Rename user objects to avoid conflict with internal object */
            iname = H5VL_logi_name_remap (name);

            // Skip internal attributes
            if (loc_params->type == H5VL_OBJECT_BY_IDX) {
                if (op->type == H5I_FILE) {
                    ((H5VL_loc_params_t *)loc_params)->loc_data.loc_by_idx.n += 1;
                } else if (op->type == H5I_DATASET) {
                    ((H5VL_loc_params_t *)loc_params)->loc_data.loc_by_idx.n += 3;
                }
            }
        } else {
            iname = (char *)name;
        }

        ap = new H5VL_log_obj_t (op, H5I_ATTR);

        if (req) {
            rp    = new H5VL_log_req_t ();
            ureqp = &ureq;
        } else {
            ureqp = NULL;
        }

        H5VL_LOGI_PROFILING_TIMER_START;
        ap->uo = H5VLattr_open (op->uo, loc_params, op->uvlid, iname, aapl_id, dxpl_id, ureqp);
        H5VL_LOGI_PROFILING_TIMER_STOP (ap->fp, TIMER_H5VLATT_OPEN);
        CHECK_PTR (ap->uo);

        // Revert arg changes
        if (loc_params->type == H5VL_OBJECT_BY_IDX) {
            if (op->type == H5I_FILE) {
                ((H5VL_loc_params_t *)loc_params)->loc_data.loc_by_idx.n -= 1;
            } else if (op->type == H5I_DATASET) {
                ((H5VL_loc_params_t *)loc_params)->loc_data.loc_by_idx.n -= 3;
            }
        }

        if (req) {
            rp->append (ureq);
            *req = rp;
        }

        H5VL_LOGI_PROFILING_TIMER_STOP (ap->fp, TIMER_H5VL_LOG_ATT_OPEN);
    }
    H5VL_LOGI_EXP_CATCH

    if (iname && iname != name) { free (iname); }
    return (void *)ap;

err_out:;
    if (ap) { delete ap; }
    if (iname && iname != name) { free (iname); }

    return NULL;
} /* end H5VL_log_attr_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_attr_read
 *
 * Purpose:     Reads data from attribute.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_attr_read (void *attr, hid_t mem_type_id, void *buf, hid_t dxpl_id, void **req) {
    H5VL_log_obj_t *op = (H5VL_log_obj_t *)attr;
    herr_t err         = 0;
    H5VL_log_req_t *rp;
    void **ureqp, *ureq;

    try {
        H5VL_LOGI_PROFILING_TIMER_START;

        if (req) {
            rp    = new H5VL_log_req_t ();
            ureqp = &ureq;
        } else {
            ureqp = NULL;
        }

        H5VL_LOGI_PROFILING_TIMER_START;
        err = H5VLattr_read (op->uo, op->uvlid, mem_type_id, buf, dxpl_id, ureqp);
        H5VL_LOGI_PROFILING_TIMER_STOP (op->fp, TIMER_H5VLATT_READ);

        if (req) {
            rp->append (ureq);
            *req = rp;
        }

        H5VL_LOGI_PROFILING_TIMER_STOP (op->fp, TIMER_H5VL_LOG_ATT_READ);
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
} /* end H5VL_log_attr_read() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_attr_write
 *
 * Purpose:     Writes data to attribute.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_attr_write (
    void *attr, hid_t mem_type_id, const void *buf, hid_t dxpl_id, void **req) {
    H5VL_log_obj_t *op = (H5VL_log_obj_t *)attr;
    herr_t err         = 0;
    H5VL_log_req_t *rp;
    void **ureqp, *ureq;

    try {
        H5VL_LOGI_PROFILING_TIMER_START;

        if (req) {
            rp    = new H5VL_log_req_t ();
            ureqp = &ureq;
        } else {
            ureqp = NULL;
        }

        H5VL_LOGI_PROFILING_TIMER_START;
        err = H5VLattr_write (op->uo, op->uvlid, mem_type_id, buf, dxpl_id, ureqp);
        H5VL_LOGI_PROFILING_TIMER_STOP (op->fp, TIMER_H5VLATT_WRITE);

        if (req) {
            rp->append (ureq);
            *req = rp;
        }

        H5VL_LOGI_PROFILING_TIMER_STOP (op->fp, TIMER_H5VL_LOG_ATT_WRITE);
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
} /* end H5VL_log_attr_write() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_attr_get
 *
 * Purpose:     Gets information about an attribute
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_attr_get (void *obj, H5VL_attr_get_args_t *args, hid_t dxpl_id, void **req) {
    H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
    herr_t err         = 0;
    H5VL_log_req_t *rp;
    void **ureqp, *ureq;

    try {
        H5VL_LOGI_PROFILING_TIMER_START;

        if (req) {
            rp    = new H5VL_log_req_t ();
            ureqp = &ureq;
        } else {
            ureqp = NULL;
        }

        // Skip internal attributes
        if (args->op_type == H5VL_ATTR_GET_INFO) {
            if (args->args.get_info.loc_params.type == H5VL_OBJECT_BY_IDX) {
                if (op->type == H5I_FILE) {
                    args->args.get_info.loc_params.loc_data.loc_by_idx.n += 1;
                } else if (op->type == H5I_DATASET) {
                    args->args.get_info.loc_params.loc_data.loc_by_idx.n += 3;
                }
            }
        } else if (args->op_type == H5VL_ATTR_GET_NAME) {
            if (args->args.get_name.loc_params.type == H5VL_OBJECT_BY_IDX) {
                if (op->type == H5I_FILE) {
                    args->args.get_name.loc_params.loc_data.loc_by_idx.n += 1;
                } else if (op->type == H5I_DATASET) {
                    args->args.get_name.loc_params.loc_data.loc_by_idx.n += 3;
                }
            }
        }

        H5VL_LOGI_PROFILING_TIMER_START;
        err = H5VLattr_get (op->uo, op->uvlid, args, dxpl_id, ureqp);
        CHECK_ERR
        H5VL_LOGI_PROFILING_TIMER_STOP (op->fp, TIMER_H5VLATT_GET);

        // Revert changes to args
        if (args->op_type == H5VL_ATTR_GET_INFO) {
            if (args->args.get_info.loc_params.type == H5VL_OBJECT_BY_IDX) {
                if (op->type == H5I_FILE) {
                    args->args.get_info.loc_params.loc_data.loc_by_idx.n -= 1;
                } else if (op->type == H5I_DATASET) {
                    args->args.get_info.loc_params.loc_data.loc_by_idx.n -= 3;
                }
                // No need to increase n for dec order, previous run are for index bound check,
                // rerun with correct n
                if (args->args.get_info.loc_params.loc_data.loc_by_idx.order == H5_ITER_DEC) {
                    H5VL_LOGI_PROFILING_TIMER_START;
                    err = H5VLattr_get (op->uo, op->uvlid, args, dxpl_id, ureqp);
                    H5VL_LOGI_PROFILING_TIMER_STOP (op->fp, TIMER_H5VLATT_GET);
                }
            }
            if (op->type == H5I_FILE) {
                args->args.get_info.ainfo->corder -= 1;
            } else if (op->type == H5I_DATASET) {
                args->args.get_info.ainfo->corder -= 3;
            }
        } else if (args->op_type == H5VL_ATTR_GET_NAME) {
            if (args->args.get_name.loc_params.type == H5VL_OBJECT_BY_IDX) {
                if (op->type == H5I_FILE) {
                    args->args.get_name.loc_params.loc_data.loc_by_idx.n -= 1;
                } else if (op->type == H5I_DATASET) {
                    args->args.get_name.loc_params.loc_data.loc_by_idx.n -= 3;
                }
                // No need to increase n for dec order, previous run are for index bound check,
                // rerun with correct n
                if (args->args.get_info.loc_params.loc_data.loc_by_idx.order == H5_ITER_DEC) {
                    H5VL_LOGI_PROFILING_TIMER_START;
                    err = H5VLattr_get (op->uo, op->uvlid, args, dxpl_id, ureqp);
                    H5VL_LOGI_PROFILING_TIMER_STOP (op->fp, TIMER_H5VLATT_GET);
                }
            }
        }

        if (req) {
            rp->append (ureq);
            *req = rp;
        }

        H5VL_LOGI_PROFILING_TIMER_STOP (op->fp, TIMER_H5VL_LOG_ATT_GET);
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
} /* end H5VL_log_attr_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_attr_specific
 *
 * Purpose:     Specific operation on attribute
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_attr_specific (void *obj,
                               const H5VL_loc_params_t *loc_params,
                               H5VL_attr_specific_args_t *args,
                               hid_t dxpl_id,
                               void **req) {
    H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
    herr_t err         = 0;
    H5VL_log_req_t *rp;
    void **ureqp, *ureq;
    char *iname                   = NULL;  // Internal name of object
    const char *original_name     = NULL;  // Original value in loc_params before being remapped
    char *iname_arg               = NULL;  // Internal name of object in args
    const char *original_name_arg = NULL;  // Original value in args before being remapped
    H5VL_log_atti_iterate_op_data *ctx = NULL;

    try {
        H5VL_LOGI_PROFILING_TIMER_START;

        if (req) {
            rp    = new H5VL_log_req_t ();
            ureqp = &ureq;
        } else {
            ureqp = NULL;
        }

        if (!op->fp->is_log_based_file) {
            err = H5VLattr_specific (op->uo, loc_params, op->uvlid, args, dxpl_id, req);
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

        // Replace H5Aiterate callback with logvol wrpapper
        if (args->op_type == H5VL_ATTR_ITER) {
            ctx = (H5VL_log_atti_iterate_op_data *)malloc (sizeof (H5VL_log_atti_iterate_op_data));
            ctx->op                    = args->args.iterate.op;
            ctx->op_data               = args->args.iterate.op_data;
            args->args.iterate.op      = H5VL_log_atti_iterate_op;
            args->args.iterate.op_data = ctx;
        }

        switch (args->op_type) {
            case H5VL_ATTR_EXISTS:;
                /* Rename user objects to avoid conflict with internal object */
                original_name_arg      = args->args.exists.name;
                iname_arg              = H5VL_logi_name_remap (original_name_arg);
                args->args.exists.name = iname_arg;
                break;
            case H5VL_ATTR_DELETE:;
                /* Rename user objects to avoid conflict with internal object */
                original_name_arg   = args->args.del.name;
                iname_arg           = H5VL_logi_name_remap (original_name_arg);
                args->args.del.name = iname_arg;
                break;
            default:;
        }

        H5VL_LOGI_PROFILING_TIMER_START;
        err = H5VLattr_specific (op->uo, loc_params, op->uvlid, args, dxpl_id, ureqp);
        CHECK_ERR
        H5VL_LOGI_PROFILING_TIMER_STOP (op->fp, TIMER_H5VLATT_SPECIFIC);

        if (args->op_type == H5VL_ATTR_ITER) {
            args->args.iterate.op      = ctx->op;
            args->args.iterate.op_data = ctx->op_data;
        }

        if (req) {
            rp->append (ureq);
            *req = rp;
        }

        H5VL_LOGI_PROFILING_TIMER_STOP (op->fp, TIMER_H5VL_LOG_ATT_SPECIFIC);
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    if (ctx) { free (ctx); }
    if (iname && iname != original_name) { free (iname); }
    // Restore name in loc_param
    if (original_name) {
        ((H5VL_loc_params_t *)loc_params)->loc_data.loc_by_name.name = original_name;
    }
    if (iname_arg && iname_arg != original_name_arg) { free (iname_arg); }
    // Restore name in args
    if (original_name_arg) {
        switch (args->op_type) {
            case H5VL_ATTR_EXISTS:;
                args->args.exists.name = original_name_arg;
                break;
            case H5VL_ATTR_DELETE:;
                args->args.del.name = original_name_arg;
                break;
            default:;
#ifdef LOGVOL_DEBUG
                RET_ERR ("Shouldn't reach here")
#endif
        }
    }
    return err;
} /* end H5VL_log_attr_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_attr_optional
 *
 * Purpose:     Perform a connector-specific operation on an attribute
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_attr_optional (void *obj, H5VL_optional_args_t *args, hid_t dxpl_id, void **req) {
    H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
    herr_t err         = 0;
    H5VL_log_req_t *rp;
    void **ureqp, *ureq;

    try {
        H5VL_LOGI_PROFILING_TIMER_START;

        if (req) {
            rp    = new H5VL_log_req_t ();
            ureqp = &ureq;
        } else {
            ureqp = NULL;
        }

        H5VL_LOGI_PROFILING_TIMER_START;
        err = H5VLattr_optional (op->uo, op->uvlid, args, dxpl_id, ureqp);
        H5VL_LOGI_PROFILING_TIMER_STOP (op->fp, TIMER_H5VLATT_OPTIONAL);

        if (req) {
            rp->append (ureq);
            *req = rp;
        }

        H5VL_LOGI_PROFILING_TIMER_STOP (op->fp, TIMER_H5VL_LOG_ATT_OPTIONAL);
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
} /* end H5VL_log_attr_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_attr_close
 *
 * Purpose:     Closes an attribute.
 *
 * Return:      Success:    0
 *              Failure:    -1, attr not closed.
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_attr_close (void *attr, hid_t dxpl_id, void **req) {
    H5VL_log_obj_t *ap = (H5VL_log_obj_t *)attr;
    herr_t err         = 0;
    H5VL_log_req_t *rp;
    void **ureqp, *ureq;

    try {
        H5VL_LOGI_PROFILING_TIMER_START;

        if (req) {
            rp    = new H5VL_log_req_t ();
            ureqp = &ureq;
        } else {
            ureqp = NULL;
        }

        H5VL_LOGI_PROFILING_TIMER_START;
        err = H5VLattr_close (ap->uo, ap->uvlid, dxpl_id, ureqp);
        CHECK_ERR
        H5VL_LOGI_PROFILING_TIMER_STOP (ap->fp, TIMER_H5VLATT_CLOSE);

        if (req) {
            rp->append (ureq);
            *req = rp;
        }

        H5VL_LOGI_PROFILING_TIMER_STOP (ap->fp, TIMER_H5VL_LOG_ATT_CLOSE);

        delete ap;
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
} /* end H5VL_log_attr_close() */
