/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <assert.h>

#include <cstdint>

#include "H5VL_log.h"
#include "H5VL_log_dataset.hpp"
#include "H5VL_log_dataseti.hpp"
#include "H5VL_log_filei.hpp"
#include "H5VL_log_req.hpp"
#include "H5VL_logi.hpp"
#include "H5VL_logi_dataspace.hpp"
#include "H5VL_logi_filter.hpp"
#include "H5VL_logi_util.hpp"
#include "H5VL_logi_wrapper.hpp"
#include "H5VL_logi_zip.hpp"

/********************* */
/* Function prototypes */
/********************* */

const H5VL_dataset_class_t H5VL_log_dataset_g {
    H5VL_log_dataset_create,   /* create       */
    H5VL_log_dataset_open,     /* open         */
    H5VL_log_dataset_read,     /* read         */
    H5VL_log_dataset_write,    /* write        */
    H5VL_log_dataset_get,      /* get          */
    H5VL_log_dataset_specific, /* specific     */
    H5VL_log_dataset_optional, /* optional     */
    H5VL_log_dataset_close     /* close        */
};

int H5Dwrite_n_op_val = 0;
int H5Dread_n_op_val  = 0;

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_dataset_create
 *
 * Purpose:     Creates a dataset in a container
 *
 * Return:      Success:    Pointer to a dataset object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *H5VL_log_dataset_create (void *obj,
                               const H5VL_loc_params_t *loc_params,
                               const char *name,
                               hid_t lcpl_id,
                               hid_t type_id,
                               hid_t space_id,
                               hid_t dcpl_id,
                               hid_t dapl_id,
                               hid_t dxpl_id,
                               void **req) {
    herr_t err = 0;
    int i;
    H5VL_log_obj_t *op        = (H5VL_log_obj_t *)obj;
    H5VL_log_dset_t *dp       = NULL;
    H5VL_log_dset_info_t *dip = NULL;  // Dataset info
    // H5VL_link_create_args_t args;
    // H5VL_loc_params_t loc;
    hid_t zero_space = -1;
    int ndim;
    htri_t is_var_type;
    char *iname = NULL;  // Internal name of object
    H5VL_log_req_t *rp;
    void **ureqp, *ureq;
    H5D_fill_value_t stat;
    void *lib_state = NULL;
    // char lname[1024];
    H5VL_logi_err_finally finally (
        [&dcpl_id, &lib_state] () -> void { H5VL_logi_restore_lib_stat (lib_state); });

    try {
        H5VL_LOGI_PROFILING_TIMER_START;
        /* Check arguments */
        if (op->fp->is_log_based_file) { iname = H5VL_logi_name_remap (name); }

        // Logvol doesn't support variable len type
        is_var_type = H5Tis_variable_str (type_id);
        if (is_var_type == true) { RET_ERR ("Variable length types are not supproted") }

        // Unused
        // Create request handle
        if (req) {
            rp    = new H5VL_log_req_t ();
            ureqp = &ureq;
        } else {
            ureqp = NULL;
        }

        dp = new H5VL_log_dset_t (op, H5I_DATASET);
        if (!op->fp->is_log_based_file) {
            dp->uo = H5VLdataset_create (op->uo, loc_params, op->uvlid, name, lcpl_id, type_id,
                                         space_id, dcpl_id, dapl_id, dxpl_id, ureqp);
            CHECK_PTR (dp->uo)
            if (req) { rp->append (ureq); }
            return dp;
        }

        // Create anchor dataset
        // TODO: native VOL will not save filter information when layout is contiguous
        err = H5Pset_layout (dcpl_id, H5D_CONTIGUOUS);
        CHECK_ERR

        dp->id = dp->fp->ndset;  // ID nees to be set before writing to attribute

        H5VL_LOGI_PROFILING_TIMER_START;
        zero_space = H5Screate (H5S_NULL);

        dp->uo = H5VLdataset_create (op->uo, loc_params, op->uvlid, iname, lcpl_id, type_id,
                                     zero_space, dcpl_id, dapl_id, dxpl_id, ureqp);
        H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VLDATASET_CREATE);
        CHECK_PTR (dp->uo)
        if (req) { rp->append (ureq); }
        H5Sclose (zero_space);

        // Construct new dataset info
        dip = new H5VL_log_dset_info_t ();
        CHECK_PTR (dip)

        dip->dtype = H5Tcopy (type_id);
        CHECK_ID (dip->dtype)
        dip->esize = H5Tget_size (type_id);
        CHECK_ID (dip->esize)

        err = H5Pfill_value_defined (dcpl_id, &stat);
        if (stat == H5D_FILL_VALUE_DEFAULT || stat == H5D_FILL_VALUE_USER_DEFINED) {
            dip->fill = (char *)malloc (dip->esize);
            CHECK_PTR (dip->fill)
            err = H5Pget_fill_value (dcpl_id, type_id, dip->fill);
            CHECK_ERR
        } else {
            dip->fill = NULL;
        }

        // NOTE: I don't know if it work for scalar dataset, can we create zero sized attr?
        ndim = H5Sget_simple_extent_dims (space_id, dip->dims, dip->mdims);
        CHECK_ID (ndim)
        dip->ndim = (hsize_t)ndim;

        // Dstep for encoding selection
        if (dp->fp->config & H5VL_FILEI_CONFIG_SEL_ENCODE) {
            dip->dsteps[dip->ndim - 1] = 1;
            for (i = dip->ndim - 2; i > -1; i--) {
                dip->dsteps[i] = dip->dsteps[i + 1] * dip->dims[i + 1];
            }
        }

        // Filters
        H5VL_logi_get_filters (dcpl_id, dip->filters);

        // Reset hdf5 context to allow attr operations within a dataset operation
        H5VL_logi_reset_lib_stat (lib_state);

        // Record dataset metadata as attributes
        H5VL_logi_add_att (dp, H5VL_LOG_DATASETI_ATTR_DIMS, H5T_STD_I64LE, H5T_NATIVE_INT64,
                           dip->ndim, dip->dims, dxpl_id, ureqp);
        if (req) { rp->append (ureq); }
        H5VL_logi_add_att (dp, H5VL_LOG_DATASETI_ATTR_MDIMS, H5T_STD_I64LE, H5T_NATIVE_INT64,
                           dip->ndim, dip->mdims, dxpl_id, ureqp);
        if (req) { rp->append (ureq); }
        H5VL_logi_add_att (dp, H5VL_LOG_DATASETI_ATTR_ID, H5T_STD_I32LE, H5T_NATIVE_INT32, 1,
                           &(dp->id), dxpl_id, ureqp);
        if (req) {
            rp->append (ureq);
            *req = rp;
        }

        // Append dataset to the file
        LOG_VOL_ASSERT (dp->fp->ndset == (int)(dp->fp->dsets_info.size ()))
        LOG_VOL_ASSERT (dp->fp->ndset == (int)(dp->fp->mreqs.size ()))
        dp->fp->ndset++;
        dp->fp->dsets_info.push_back (dip);          // Dataset info
        dp->fp->idx->reserve (dp->fp->ndset);        // Index for H5Dread
        dp->fp->mreqs.resize (dp->fp->ndset, NULL);  // Merged requests

        // Create soft link to aid dataset visiting on file opening
        // Broken, not used anymore
        /*
        sprintf (lname, "_dset%d", dp->id);
        args.op_type					 = H5VL_LINK_CREATE_SOFT;
        args.args.soft.target			 = name;
        loc.type						 = H5VL_OBJECT_BY_NAME;
        loc.loc_data.loc_by_name.lapl_id = H5P_LINK_ACCESS_DEFAULT;
        loc.loc_data.loc_by_name.name	 = lname;
        loc.obj_type					 = op->type;
        err = H5VLlink_create (&args, op->fp->uo, &loc, op->uvlid, H5P_LINK_CREATE_DEFAULT,
                                                   H5P_LINK_ACCESS_DEFAULT, dp->fp->dxplid, req);
        CHECK_ERR
        */
        H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_CREATE);

#ifdef LOGVOL_DEBUG
        if (H5VL_logi_debug_verbose ()) {
            printf ("H5VL_log_dataset_create(%p, %s, %ld)\n", obj, name, dip->ndim);
        }
#endif
    }
    H5VL_LOGI_EXP_CATCH

    goto fn_exit;
err_out:;
    if (dp) {
        if (dip) { delete dip; }
        delete dp;
        dp = NULL;
    }
fn_exit:;
    if (iname && iname != name) { free (iname); }
    return (void *)dp;
} /* end H5VL_log_dataset_create() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_dataset_open
 *
 * Purpose:     Opens a dataset in a container
 *
 * Return:      Success:    Pointer to a dataset object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *H5VL_log_dataset_open (void *obj,
                             const H5VL_loc_params_t *loc_params,
                             const char *name,
                             hid_t dapl_id,
                             hid_t dxpl_id,
                             void **req) {
    // herr_t err		   = 0;
    char *iname        = NULL;  // Internal name of object
    H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
    void *uo           = NULL;

    try {
#ifdef LOGVOL_DEBUG
        if (H5VL_logi_debug_verbose ()) { printf ("H5VL_log_dataset_open(%p, %s)\n", obj, name); }
#endif
        /* Check arguments */

        if (op->fp->is_log_based_file) {
            /* Rename user objects to avoid conflict with internal object */
            iname = H5VL_logi_name_remap (name);
        } else {
            H5VL_log_dset_t *dp = new H5VL_log_dset_t (op, H5I_DATASET);
            dp->uo = H5VLdataset_open (op->uo, loc_params, op->uvlid, name, dapl_id, dxpl_id, NULL);
            CHECK_PTR (dp->uo);
            return (void *)dp;
        }

        H5VL_LOGI_PROFILING_TIMER_START;
        uo = H5VLdataset_open (op->uo, loc_params, op->uvlid, iname, dapl_id, dxpl_id, NULL);
        CHECK_PTR (uo);
        H5VL_LOGI_PROFILING_TIMER_STOP (op->fp, TIMER_H5VLDATASET_OPEN);

        if (iname && iname != name) { free (iname); }
        return H5VL_log_dataseti_open (op, uo, dxpl_id);
    }
    H5VL_LOGI_EXP_CATCH

err_out:;
    if (iname && iname != name) { free (iname); }
    return NULL;
} /* end H5VL_log_dataset_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_dataset_read
 *
 * Purpose:     Reads data elements from a dataset into a buffer.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_dataset_read_1 (void *dset,
                                hid_t mem_type_id,
                                hid_t mem_space_id,
                                hid_t file_space_id,
                                hid_t plist_id,
                                void *buf,
                                void **req) {
    herr_t err                = 0;
    H5VL_log_dset_t *dp       = (H5VL_log_dset_t *)dset;
    H5VL_log_dset_info_t *dip = NULL;  // Dataset info
    hid_t dsid;                        // Dataset space id
    H5VL_log_selections *dsel = NULL;  // Selection blocks

    try {
        if (!dp->fp->is_log_based_file) {
            return H5VL_log_under_dataset_read (dp->uo, dp->uvlid, mem_type_id, mem_space_id, file_space_id,
                                     plist_id, buf, NULL);
        }
        dip = dp->fp->dsets_info[dp->id];
        H5VL_LOGI_PROFILING_TIMER_START;
        if (file_space_id == H5S_ALL) {
            dsid = H5Screate_simple (dip->ndim, dip->dims, dip->mdims);
            CHECK_ID (dsid);
        } else {
            dsid = file_space_id;
        }
        dsel = new H5VL_log_selections (dsid);
        H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOGI_GET_DATASPACE_SELECTION);

        // H5S_All means using file space
        if (mem_space_id == H5S_ALL) mem_space_id = dsid;

        H5VL_log_dataseti_read (dp, mem_type_id, mem_space_id, dsel, plist_id, buf, req);
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    // Note: dsel should be freed when the read request is deleted
    if (dsid != file_space_id) { H5Sclose (dsid); }
    return err;
} /* end H5VL_log_dataset_read() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_dataset_write
 *
 * Purpose:     Writes data elements from a buffer into a dataset.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_dataset_write_1 (void *dset,
                                 hid_t mem_type_id,
                                 hid_t mem_space_id,
                                 hid_t file_space_id,
                                 hid_t plist_id,
                                 const void *buf,
                                 void **req) {
    herr_t err          = 0;
    H5VL_log_dset_t *dp = (H5VL_log_dset_t *)dset;

    H5VL_log_dset_info_t *dip = NULL;  // Dataset info
    hid_t dsid;                        // Dataset space id
    H5VL_log_selections *dsel = NULL;  // Selection blocks

    try {
        if (!dp->fp->is_log_based_file) {
            return H5VL_log_under_dataset_write (dp->uo, dp->uvlid, mem_type_id, mem_space_id, file_space_id,
                                      plist_id, buf, NULL);
        }
        dip = dp->fp->dsets_info[dp->id];
        H5VL_LOGI_PROFILING_TIMER_START;
        if (file_space_id == H5S_ALL) {
            dsid = H5Screate_simple (dip->ndim, dip->dims, dip->mdims);
            CHECK_ID (dsid);
        } else {
            dsid = file_space_id;
        }
        dsel = new H5VL_log_selections (dsid);
        H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOGI_GET_DATASPACE_SELECTION);
        CHECK_PTR (dsel)

        // H5S_All means using file space
        if (mem_space_id == H5S_ALL) mem_space_id = dsid;

        H5VL_log_dataseti_write (dp, mem_type_id, mem_space_id, dsel, plist_id, buf, req);
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    if (dsel) { delete dsel; }
    if (dsid != file_space_id) { H5Sclose (dsid); }
    return err;
} /* end H5VL_log_dataset_write() */

herr_t H5VL_log_dataset_read_2 (size_t count,
                                void *dset[],
                                hid_t mem_type_id[],
                                hid_t mem_space_id[],
                                hid_t file_space_id[],
                                hid_t plist_id,
                                void *buf[],
                                void **req) {
    herr_t err = 0;
    size_t i;

    for (i = 0; i < count; i++) {
        err = H5VL_log_dataset_read_1 (dset[i], mem_type_id[i], mem_space_id[i], file_space_id[i],
                                       plist_id, buf[i], NULL);
        CHECK_ERR
    }

    return err;
}

herr_t H5VL_log_dataset_write_2 (size_t count,
                                 void *dset[],
                                 hid_t mem_type_id[],
                                 hid_t mem_space_id[],
                                 hid_t file_space_id[],
                                 hid_t plist_id,
                                 const void *buf[],
                                 void **req) {
    herr_t err = 0;
    size_t i;

    for (i = 0; i < count; i++) {
        err = H5VL_log_dataset_write_1 (dset[i], mem_type_id[i], mem_space_id[i], file_space_id[i],
                                        plist_id, buf[i], NULL);
        CHECK_ERR
    }

    return err;
}

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_dataset_get
 *
 * Purpose:     Gets information about a dataset
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_dataset_get (void *dset, H5VL_dataset_get_args_t *args, hid_t dxpl_id, void **req) {
    herr_t err                = 0;
    H5VL_log_dset_t *dp       = (H5VL_log_dset_t *)dset;
    H5VL_log_dset_info_t *dip = NULL;  // Dataset info
    // H5VL_log_req_t *rp;
    // void **ureqp, *ureq;

    try {
        H5VL_LOGI_PROFILING_TIMER_START;

        if (!dp->fp->is_log_based_file) {
            err = H5VLdataset_get (dp->uo, dp->uvlid, args, dxpl_id, req);
            return err;
        }

        dip = dp->fp->dsets_info[dp->id];

        switch (args->op_type) {
            /* H5Dget_space */
            case H5VL_DATASET_GET_SPACE: {
                args->args.get_space.space_id = H5Screate_simple (dip->ndim, dip->dims, dip->mdims);
                break;
            }

            /* H5Dget_space_status */
            case H5VL_DATASET_GET_SPACE_STATUS: {
                err = -1;
                ERR_OUT ("H5VL_DATASET_GET_SPACE_STATUS not supported")
                break;
            }

            /* H5Dargs->get_type */
            case H5VL_DATASET_GET_TYPE:
            /* H5Dget_create_plist */
            case H5VL_DATASET_GET_DCPL:
            /* H5Dget_access_plist */
            case H5VL_DATASET_GET_DAPL: {
                H5VL_LOGI_PROFILING_TIMER_START;
                err = H5VLdataset_get (dp->uo, dp->uvlid, args, dxpl_id, req);
                H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VLDATASET_GET);
                break;
            }

            /* H5Dget_storage_size */
            case H5VL_DATASET_GET_STORAGE_SIZE: {
                err = -1;
                ERR_OUT ("H5VL_DATASET_GET_STORAGE_SIZE not supported")
                break;
            }
            default:
                ERR_OUT ("Unrecognized op type")
        } /* end switch */

        H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_GET);
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
} /* end H5VL_log_dataset_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_dataset_specific
 *
 * Purpose:     Specific operation on a dataset
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_dataset_specific (void *obj,
                                  H5VL_dataset_specific_args_t *args,
                                  hid_t dxpl_id,
                                  void **req) {
    herr_t err                = 0;
    H5VL_log_dset_t *dp       = (H5VL_log_dset_t *)obj;
    H5VL_log_dset_info_t *dip = NULL;  // Dataset info
    // H5VL_log_req_t *rp;
    // void **ureqp, *ureq;

    try {
        H5VL_LOGI_PROFILING_TIMER_START;
        if (!dp->fp->is_log_based_file) {
            err = H5VLdataset_specific (dp->uo, dp->uvlid, args, dxpl_id, req);
            return err;
        }

        dip = dp->fp->dsets_info[dp->id];

        switch (args->op_type) {
            case H5VL_DATASET_SET_EXTENT: { /* H5Dset_extent */
                int32_t i;
                const hsize_t *new_sizes = args->args.set_extent.size;

                // Adjust dim
                for (i = 0; i < (int32_t)(dip->ndim); i++) {
                    if (dip->mdims[i] != H5S_UNLIMITED && new_sizes[i] > dip->mdims[i]) {
                        err = -1;
                        ERR_OUT ("size cannot exceed max size")
                    }
                    dip->dims[i] = new_sizes[i];
                }

                // Record new size
                H5VL_logi_put_att (dp, H5VL_LOG_DATASETI_ATTR_DIMS, H5T_NATIVE_INT64, dip->dims,
                                   dxpl_id);

                // Recalculate dsteps if needed
                if (dp->fp->config & H5VL_FILEI_CONFIG_SEL_ENCODE) {
                    dip->dsteps[dip->ndim - 1] = 1;
                    for (i = dip->ndim - 2; i > -1; i--) {
                        dip->dsteps[i] = dip->dsteps[i + 1] * dip->dims[i + 1];
                    }

                    // Flush merged request as dstep may be changed
                    if (dp->fp->mreqs[dp->id] && (dp->fp->mreqs[dp->id]->nsel > 0)) {
                        dp->fp->wreqs.push_back (dp->fp->mreqs[dp->id]);
                        // Update total metadata size in wreqs
                        dp->fp->mdsize += dp->fp->mreqs[dp->id]->hdr->meta_size;
                        dp->fp->mreqs[dp->id] = NULL;
                    }
                }

                break;
            }
            default:
                H5VL_LOGI_PROFILING_TIMER_START;
                err = H5VLdataset_specific (dp->uo, dp->uvlid, args, dxpl_id, req);
                H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VLDATASET_SPECIFIC);
        }

        H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_SPECIFIC);
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
} /* end H5VL_log_dataset_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_dataset_optional
 *
 * Purpose:     Perform a connector-specific operation on a dataset
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_dataset_optional (void *obj,
                                  H5VL_optional_args_t *args,
                                  hid_t dxpl_id,
                                  void **req) {
    herr_t err          = 0;
    H5VL_log_obj_t *op  = (H5VL_log_obj_t *)obj;
    H5VL_log_dset_t *dp = (H5VL_log_dset_t *)op;

    if (!dp->fp->is_log_based_file) {
        err = H5VLdataset_optional (op->uo, op->uvlid, args, dxpl_id, req);
        return err;
    }

    H5VL_log_dset_info_t *dip = dp->fp->dsets_info[dp->id];  // Dataset info
    // H5VL_log_req_t *rp;
    // void **ureqp, *ureq;
    H5VL_log_selections *dsel     = NULL;                                  // Selection blocks
    H5VL_log_dio_n_arg_t *varnarg = (H5VL_log_dio_n_arg_t *)(args->args);  // H5Dwrite_n args

    try {
        H5VL_LOGI_PROFILING_TIMER_START;

        if (args->op_type == H5Dwrite_n_op_val) {
            H5VL_LOGI_PROFILING_TIMER_START;
            dsel = new H5VL_log_selections (dip->ndim, dip->dims, varnarg->n, varnarg->starts,
                                            varnarg->counts);
            H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOGI_GET_DATASPACE_SELECTION);
            CHECK_PTR (dsel)

            H5VL_log_dataseti_write (dp, varnarg->mem_type_id, H5S_BLOCK, dsel, dxpl_id,
                                     varnarg->buf, req);
        } else if (args->op_type == H5Dread_n_op_val) {
            H5VL_LOGI_PROFILING_TIMER_START;
            dsel = new H5VL_log_selections (dip->ndim, dip->dims, varnarg->n, varnarg->starts,
                                            varnarg->counts);
            H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOGI_GET_DATASPACE_SELECTION);
            CHECK_PTR (dsel)

            H5VL_log_dataseti_read (dp, varnarg->mem_type_id, H5S_BLOCK, dsel, dxpl_id,
                                    varnarg->buf, req);
        } else {
            /*
            if (req) {
                    rp	  = new H5VL_log_req_t ();
                    ureqp = &ureq;
            } else {
                    ureqp = NULL;
            }
            */

            H5VL_LOGI_PROFILING_TIMER_START;
            err = H5VLdataset_optional (op->uo, op->uvlid, args, dxpl_id, req);
            H5VL_LOGI_PROFILING_TIMER_STOP (op->fp, TIMER_H5VLDATASET_OPTIONAL);

            /*
            if (req) {
                    rp->append (ureq);
                    *req = rp;
            }
            */

            H5VL_LOGI_PROFILING_TIMER_STOP (op->fp, TIMER_H5VL_LOG_DATASET_OPTIONAL);
        }
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    if (args->op_type == H5Dwrite_n_op_val) {  // Dsel for read will be freed when req is deleted
        if (dsel) { delete dsel; }
    }

    return err;
} /* end H5VL_log_dataset_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_dataset_close
 *
 * Purpose:     Closes a dataset.
 *
 * Return:      Success:    0
 *              Failure:    -1, dataset not closed.
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_dataset_close (void *dset, hid_t dxpl_id, void **req) {
    herr_t err          = 0;
    H5VL_log_dset_t *dp = (H5VL_log_dset_t *)dset;
    // H5VL_log_dset_info_t *dip = dp->fp->dsets_info[dp->id];	 // Dataset info

    try {
#ifdef LOGVOL_DEBUG
        if (H5VL_logi_debug_verbose ()) { printf ("H5VL_log_dataset_close(%p)\n", dset); }
#endif

        H5VL_LOGI_PROFILING_TIMER_START;

        H5VL_LOGI_PROFILING_TIMER_START;
        err = H5VLdataset_close (dp->uo, dp->uvlid, dxpl_id, NULL);
        CHECK_ERR
        H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VLDATASET_CLOSE);

        // Flush and free merged reqeusts
        // Dataset info is shared by all dataset instances
        // Deallocate them only at file close
        /*
        if (dp->fp->mreqs[dp->id]->dbufs.size ()) {
                dp->fp->wreqs.push_back (dp->fp->mreqs[dp->id]);
                // Update total metadata size in wreqs
                dp->fp->mdsize += dp->fp->mreqs[dp->id]->hdr->meta_size;
        } else {
                delete dp->fp->mreqs[dp->id];
                dp->fp->mreqs[dp->id] = NULL;
        }
        H5Tclose (dip->dtype);
        */

        H5VL_LOGI_PROFILING_TIMER_STOP (dp->fp, TIMER_H5VL_LOG_DATASET_CLOSE);

        delete dp;
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;

    return err;
} /* end H5VL_log_dataset_close() */
