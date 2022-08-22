/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <mpi.h>

#include <cassert>
#include <cstdlib>

#include "H5VL_logi.hpp"
#include "H5VL_logi_util.hpp"
#include "H5VL_logi_wrapper.hpp"
#include "hdf5.h"

void H5VL_logi_add_att (void *uo,
                        hid_t uvlid,
                        H5I_type_t type,
                        const char *name,
                        hid_t atype,
                        hid_t mtype,
                        hsize_t len,
                        void *buf,
                        hid_t dxpl_id,
                        void **req) {
    herr_t err = 0;
    H5VL_loc_params_t loc;
    hid_t asid = -1;
    void *ap;

    asid = H5Screate_simple (1, &len, &len);
    CHECK_ID (asid);

    loc.obj_type = type;
    loc.type     = H5VL_OBJECT_BY_SELF;

    ap = H5VLattr_create (uo, &loc, uvlid, name, atype, asid, H5P_ATTRIBUTE_CREATE_DEFAULT,
                          H5P_ATTRIBUTE_ACCESS_DEFAULT, dxpl_id, NULL);
    CHECK_PTR (ap);
    err = H5VLattr_write (ap, uvlid, mtype, buf, dxpl_id, NULL);
    CHECK_ERR;
    err = H5VLattr_close (ap, uvlid, dxpl_id, NULL);
    CHECK_ERR
    H5Sclose (asid);
}
void H5VL_logi_add_att (H5VL_log_obj_t *op,
                        const char *name,
                        hid_t atype,
                        hid_t mtype,
                        hsize_t len,
                        void *buf,
                        hid_t dxpl_id,
                        void **req) {
    H5VL_logi_add_att (op->uo, op->uvlid, op->type, name, atype, mtype, len, buf, dxpl_id, req);
}

void H5VL_logi_put_att (void *uo,
                        hid_t uvlid,
                        H5I_type_t type,
                        const char *name,
                        hid_t mtype,
                        void *buf,
                        hid_t dxpl_id) {
    herr_t err = 0;
    H5VL_loc_params_t loc;
    void *ap;

    loc.obj_type = type;
    loc.type     = H5VL_OBJECT_BY_SELF;

    ap = H5VLattr_open (uo, &loc, uvlid, name, H5P_ATTRIBUTE_ACCESS_DEFAULT, dxpl_id, NULL);
    CHECK_PTR (ap);
    err = H5VLattr_write (ap, uvlid, mtype, buf, dxpl_id, NULL);
    CHECK_ERR;
    err = H5VLattr_close (ap, uvlid, dxpl_id, NULL);
    CHECK_ERR
}
void H5VL_logi_put_att (
    H5VL_log_obj_t *op, const char *name, hid_t mtype, void *buf, hid_t dxpl_id) {
    H5VL_logi_put_att (op->uo, op->uvlid, op->type, name, mtype, buf, dxpl_id);
}
void H5VL_logi_get_att (void *uo,
                        hid_t uvlid,
                        H5I_type_t type,
                        const char *name,
                        hid_t mtype,
                        void *buf,
                        hid_t dxpl_id) {
    herr_t err = 0;
    H5VL_loc_params_t loc;
    void *ap;

    loc.obj_type = type;
    loc.type     = H5VL_OBJECT_BY_SELF;

    ap = H5VLattr_open (uo, &loc, uvlid, name, H5P_ATTRIBUTE_ACCESS_DEFAULT, dxpl_id, NULL);
    CHECK_PTR (ap);
    err = H5VLattr_read (ap, uvlid, mtype, buf, dxpl_id, NULL);
    CHECK_ERR;
    err = H5VLattr_close (ap, uvlid, dxpl_id, NULL);
    CHECK_ERR
}
void H5VL_logi_get_att (
    H5VL_log_obj_t *op, const char *name, hid_t mtype, void *buf, hid_t dxpl_id) {
    H5VL_logi_get_att (op->uo, op->uvlid, op->type, name, mtype, buf, dxpl_id);
}

// This method checks if an attribute exists
hbool_t H5VL_logi_exists_att (H5VL_log_obj_t *op, const char *name, hid_t dxpl_id) {
    H5VL_loc_params_t loc;
    H5VL_attr_specific_args_t attr_check_exists;
    hbool_t exist = 0;

    loc.obj_type = op->type;
    loc.type     = H5VL_OBJECT_BY_SELF;

    attr_check_exists.args.exists.name   = name;
    attr_check_exists.args.exists.exists = &exist;
    attr_check_exists.op_type            = H5VL_ATTR_EXISTS;
    H5VLattr_specific (op->uo, &loc, op->uvlid, &attr_check_exists, dxpl_id, NULL);
    return exist;
}
// This methods checks if a link (group or dataset) exists in file
hbool_t H5VL_logi_exists_link (H5VL_log_file_t *fp, const char *name, hid_t dxpl_id) {
    H5VL_link_specific_args_t arg;
    hbool_t exists = 0;
    H5VL_loc_params_t loc;
    arg.op_type            = H5VL_LINK_EXISTS;
    arg.args.exists.exists = &exists;

    loc.type                         = H5VL_OBJECT_BY_NAME;
    loc.obj_type                     = H5I_FILE;
    loc.loc_data.loc_by_name.name    = name;
    loc.loc_data.loc_by_name.lapl_id = H5P_DEFAULT;

    H5VLlink_specific (fp->uo, &loc, fp->uvlid, &arg, fp->dxplid, NULL);
    return exists;
}

void H5VL_logi_get_att_ex (
    H5VL_log_obj_t *op, const char *name, hid_t mtype, hsize_t *len, void *buf, hid_t dxpl_id) {
    herr_t err = 0;
    H5VL_loc_params_t loc;
    hid_t asid = -1;
    int ndim;
    void *ap;
    H5VL_logi_err_finally finally ([&] () -> void { H5Sclose (asid); });

    loc.obj_type = op->type;
    loc.type     = H5VL_OBJECT_BY_SELF;

    ap = H5VLattr_open (op->uo, &loc, op->uvlid, name, H5P_ATTRIBUTE_ACCESS_DEFAULT, dxpl_id, NULL);
    CHECK_PTR (ap);
    asid = H5VL_logi_attr_get_space (op->fp, ap, op->uvlid, dxpl_id);
    CHECK_ID (asid);
    ndim = H5Sget_simple_extent_dims (asid, len, NULL);
    CHECK_ID (ndim)
    LOG_VOL_ASSERT (ndim == 1);
    err = H5VLattr_read (ap, op->uvlid, mtype, buf, dxpl_id, NULL);
    CHECK_ERR;
    err = H5VLattr_close (ap, op->uvlid, dxpl_id, NULL);
    CHECK_ERR
}

MPI_Datatype H5VL_logi_get_mpi_type_by_size (size_t size) {
    switch (size) {
        case 1:
            return MPI_BYTE;
        case 2:
            return MPI_SHORT;
        case 4:
            return MPI_INT;
        case 8:
            return MPI_LONG_LONG;
    }

    return MPI_DATATYPE_NULL;
}
