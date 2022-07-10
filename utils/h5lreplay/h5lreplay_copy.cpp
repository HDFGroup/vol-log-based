/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
//
#include <cassert>
#include <cstdio>
#include <iostream>
//
#include <hdf5.h>
//
#include "H5VL_log_dataset.hpp"
#include "H5VL_log_dataseti.hpp"
#include "h5lreplay.hpp"
#include "h5lreplay_copy.hpp"

herr_t h5lreplay_copy_handler (hid_t o_id,
                               const char *name,
                               const H5O_info_t *object_info,
                               void *op_data) {
    herr_t err   = 0;
    hid_t src_id = -1;
    hid_t dst_id = -1;
    hid_t aid    = -1;
    hid_t sid    = -1;
    hid_t tid    = -1;
    hid_t dcplid = -1;
    hsize_t n;  // Attr iterate idx position
    dset_info dset;
    h5lreplay_copy_handler_arg *argp = (h5lreplay_copy_handler_arg *)op_data;

    // Skip unnamed and hidden object
    if ((name == NULL) || (name[0] == '_' && name[1] != '_') ||
        (name[0] == '/' || (name[0] == '.'))) {
#ifdef LOGVOL_DEBUG
        if (name[0] == '_') {
            std::cout << "Skip log-based VOL data objects " << name << std::endl;
        }
#endif
        goto err_out;
    }
    if (name[0] == '_') name++;

    try {
        // Copy a dataset
        if (object_info->type == H5O_TYPE_DATASET) {
            int id;
            int ndim;
            hsize_t hndim;
            hsize_t dims[H5S_MAX_RANK], mdims[H5S_MAX_RANK];

#ifdef LOGVOL_DEBUG
            std::cout << "Reconstructing user dataset " << name << std::endl;
#endif

            // Open src dataset
            src_id = H5Dopen2 (o_id, name, H5P_DEFAULT);
            CHECK_ID (src_id)
            dcplid = H5Dget_create_plist (src_id);

            // Read ndim and dims
            aid = H5Aopen (src_id, H5VL_LOG_DATASETI_ATTR_DIMS, H5P_DEFAULT);
            CHECK_ID (aid)
            sid = H5Aget_space (aid);
            CHECK_ID (sid)
            ndim = H5Sget_simple_extent_dims (sid, &hndim, NULL);
            assert (ndim == 1);
            ndim = (int)hndim;
            H5Sclose (sid);
            sid = -1;
            err = H5Aread (aid, H5T_NATIVE_INT64, dims);
            CHECK_ERR
            H5Aclose (aid);
            aid = -1;
            // Read max dims
            aid = H5Aopen (src_id, H5VL_LOG_DATASETI_ATTR_MDIMS, H5P_DEFAULT);
            CHECK_ID (aid)
            err = H5Aread (aid, H5T_NATIVE_INT64, mdims);
            CHECK_ERR
            H5Aclose (aid);
            aid = -1;
            // Read dataset ID
            aid = H5Aopen (src_id, H5VL_LOG_DATASETI_ATTR_ID, H5P_DEFAULT);
            CHECK_ID (aid)
            err = H5Aread (aid, H5T_NATIVE_INT, &id);
            CHECK_ERR
            H5Aclose (aid);
            aid = -1;
            // Datatype and esize
            tid = H5Dget_type (src_id);
            CHECK_ID (tid)

            // Create dst dataset
            err = H5Pset_layout (dcplid, H5D_CONTIGUOUS);
            CHECK_ERR
            sid = H5Screate_simple (ndim, dims, mdims);
            CHECK_ID (sid)
            dst_id = H5Dcreate2 (argp->fid, name, tid, sid, H5P_DEFAULT, dcplid, H5P_DEFAULT);
            CHECK_ID (dst_id)

            // Filters
            H5VL_logi_get_filters (dcplid, dset.filters);

            // Copy all attributes
            // err = H5Aiterate2 (src_did, H5_INDEX_CRT_ORDER, H5_ITER_INC, &zero,
            //				   h5lreplay_attr_copy_handler, &dst_did);
            // CHECK_ERR

            dset.id    = dst_id;
            dset.dtype = tid;
            dset.esize = H5Tget_size (tid);
            dset.ndim  = ndim;

            // Record did for replaying data
            // Do not close dst_did
            argp->dsets[id] = dset;

            // Copy attributes
            n   = 0;
            err = H5Aiterate2 (src_id, H5_INDEX_NAME, H5_ITER_INC, &n, h5lreplay_attr_copy_handler,
                               (void *)dst_id);
            CHECK_ERR
        } else if (object_info->type == H5O_TYPE_GROUP) {  // Copy a group
            // Open source group
            src_id = H5Gopen2 (o_id, name, H5P_DEFAULT);
            CHECK_ID (src_id)

            // Create dst group
            dst_id = H5Gcreate2 (argp->fid, name, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            CHECK_ID (dst_id)

            // Copy attributes
            n   = 0;
            err = H5Aiterate2 (src_id, H5_INDEX_NAME, H5_ITER_INC, &n, h5lreplay_attr_copy_handler,
                               (void *)dst_id);
            CHECK_ERR
        } else {  // Copy anything else as is
            err = H5Ocopy (o_id, name, argp->fid, name, H5P_DEFAULT, H5P_DEFAULT);
            CHECK_ERR
        }
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    if (sid >= 0) { H5Sclose (sid); }
    if (aid >= 0) { H5Aclose (aid); }
    if (dcplid >= 0) { H5Pclose (dcplid); }
    if (object_info->type == H5O_TYPE_DATASET) {
        if (src_id >= 0) { H5Dclose (src_id); }
    } else if (object_info->type == H5O_TYPE_GROUP) {
        if (src_id >= 0) { H5Gclose (src_id); }
        if (dst_id >= 0) { H5Gclose (dst_id); }
    }

    return err;
}

herr_t h5lreplay_attr_copy_handler (hid_t location_id,
                                    const char *attr_name,
                                    const H5A_info_t *ainfo,
                                    void *op_data) {
    int err = 0;
    int i;
    hid_t pid     = (hid_t)op_data;
    hid_t src_aid = -1;
    hid_t dst_aid = -1;
    hid_t sid     = -1;
    hid_t tid     = -1;
    int ndim;
    hsize_t dims[H5S_MAX_RANK], mdims[H5S_MAX_RANK];
    size_t bsize;  // Buffer size
    size_t esize;  // size of element tid
    void *buf = NULL;

    // Skip unnamed and hidden object
    if ((attr_name == NULL) || (attr_name[0] == '_' && attr_name[1] != '_') ||
        (attr_name[0] == '/' || (attr_name[0] == '.'))) {
#ifdef LOGVOL_DEBUG
        if (attr_name[0] == '_') {
            std::cout << "Skip log-based VOL data objects " << attr_name << std::endl;
        }
#endif
        goto err_out;
    }
    if (attr_name[0] == '_') attr_name++;

    try {
#ifdef LOGVOL_DEBUG
        std::cout << "Copying user attribute " << attr_name << std::endl;
#endif

        // Open src attr
        src_aid = H5Aopen (location_id, attr_name, H5P_DEFAULT);
        CHECK_ID (src_aid)

        // Get space
        sid = H5Aget_space (src_aid);
        CHECK_ID (sid)

        // Allocate buffer
        ndim = H5Sget_simple_extent_dims (sid, dims, mdims);
        CHECK_ID (ndim)
        bsize = 1;
        for (i = 0; i < ndim; i++) { bsize *= dims[i]; }
        tid = H5Aget_type (src_aid);
        CHECK_ID (tid)
        esize = H5Tget_size (tid);
        if (esize <= 0) { ERR_OUT ("Attribute element size unknown") }
        bsize *= esize;
        buf = malloc (bsize);
        CHECK_PTR (buf)

        // Read attr
        err = H5Aread (src_aid, tid, buf);
        CHECK_ERR

        // Create dst attr
        dst_aid = H5Acreate2 (pid, attr_name, tid, sid, H5P_DEFAULT, H5P_DEFAULT);
        CHECK_ID (dst_aid)

        // Write attr
        err = H5Awrite (dst_aid, tid, buf);
        CHECK_ERR
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    if (buf) { free (buf); }
    if (sid >= 0) { H5Sclose (sid); }
    if (tid >= 0) { H5Tclose (tid); }
    if (src_aid >= 0) { H5Aclose (src_aid); }
    if (dst_aid >= 0) { H5Aclose (dst_aid); }

    return err;
}
