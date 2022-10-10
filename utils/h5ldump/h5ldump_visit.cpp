/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
//
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
//
#include <hdf5.h>
//
#include "H5VL_log_dataseti.hpp"
#include "H5VL_log_filei.hpp"
#include "H5VL_logi_err.hpp"
#include "H5VL_logi_nb.hpp"
#include "h5ldump.hpp"

void h5ldump_visit (std::string path, std::vector<H5VL_log_dset_info_t> &dsets) {
    herr_t err       = 0;
    hid_t fid        = -1;              // File ID
    hid_t aid        = -1;              // ID of file attribute
    hid_t faplid     = -1;              // File access property ID
    hid_t nativevlid = -1;              // Native VOL ID
    int att_buf[H5VL_LOG_FILEI_NATTR];  // Temporary buffer for reading file attributes
    H5VL_logi_err_finally finally ([&] () -> void {
        if (aid >= 0) { H5Aclose (aid); }
        if (fid >= 0) { H5Fclose (fid); }
        if (faplid >= 0) { H5Pclose (faplid); }
    });

    // Always use native VOL
    nativevlid = H5VLpeek_connector_id_by_name ("native");
    faplid     = H5Pcreate (H5P_FILE_ACCESS);
    H5Pset_vol (faplid, nativevlid, NULL);

    // Open the input file
    fid = H5Fopen (path.c_str (), H5F_ACC_RDONLY, faplid);
    CHECK_ID (fid)

    // Read file metadata
    aid = H5Aopen (fid, H5VL_LOG_FILEI_ATTR, H5P_DEFAULT);
    CHECK_ID (aid)

    err = H5Aread (aid, H5T_NATIVE_INT, att_buf);
    CHECK_ERR
    dsets.resize (att_buf[0]);

    err = H5Ovisit3 (fid, H5_INDEX_CRT_ORDER, H5_ITER_INC, h5ldump_visit_handler, &dsets,
                     H5O_INFO_ALL);
    CHECK_ERR
}

herr_t h5ldump_visit_handler (hid_t o_id,
                              const char *name,
                              const H5O_info_t *object_info,
                              void *op_data) {
    herr_t err   = 0;
    hid_t did    = -1;          // Current dataset ID
    hid_t dcplid = -1;          // Current dataset creation property list ID
    hid_t aid    = -1;          // Dataset attribute ID
    hid_t sid    = -1;          // Dataset attribute space ID
    int id;                     // Log VOL dataset ID
    H5VL_log_dset_info_t dset;  // Current dataset info
    std::vector<H5VL_log_dset_info_t> *dsets = (std::vector<H5VL_log_dset_info_t> *)op_data;

    // Skip unnamed and hidden object
    if ((name == NULL) || (name[0] == '_' && name[1] != '_') ||
        (name[0] == '/' || (name[0] == '.'))) {
        goto err_out;
    }

    try {
        // Visit a dataset
        if (object_info->type == H5O_TYPE_DATASET) {
            int ndim;
            hsize_t hndim;

            // Open src dataset
            did = H5Dopen2 (o_id, name, H5P_DEFAULT);
            CHECK_ID (did)

            // Read ndim and dims
            aid = H5Aopen (did, H5VL_LOG_DATASETI_ATTR_DIMS, H5P_DEFAULT);
            CHECK_ID (aid)
            sid = H5Aget_space (aid);
            CHECK_ID (sid)
            ndim = H5Sget_simple_extent_dims (sid, &hndim, NULL);
            assert (ndim == 1);
            dset.ndim = (int)hndim;
            H5Sclose (sid);
            sid = -1;
            err = H5Aread (aid, H5T_NATIVE_INT64, dset.dims);
            CHECK_ERR
            H5Aclose (aid);
            aid = -1;

            // Read dataset ID
            aid = H5Aopen (did, H5VL_LOG_DATASETI_ATTR_ID, H5P_DEFAULT);
            CHECK_ID (aid)
            err = H5Aread (aid, H5T_NATIVE_INT, &id);
            CHECK_ERR
            H5Aclose (aid);
            aid = -1;

            // Datatype and esize
            dset.dtype = H5Dget_type (did);
            CHECK_ID (dset.dtype)
            dset.esize = H5Tget_size (dset.dtype);

            // Filters
            dcplid = H5Dget_create_plist (did);
            CHECK_ID (dcplid)
            H5VL_logi_get_filters (dcplid, dset.filters);

            ((*dsets)[id]) = dset;
        }
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    if (sid >= 0) { H5Sclose (sid); }
    if (aid >= 0) { H5Aclose (aid); }
    if (did >= 0) { H5Dclose (did); }
    if (dcplid >= 0) { H5Pclose (dcplid); }
    return err;
}
