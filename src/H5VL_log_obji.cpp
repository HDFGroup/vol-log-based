/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
//
#include <memory>
//
#include "H5VL_log_filei.hpp"
#include "H5VL_log_obj.hpp"
#include "H5VL_log_obji.hpp"
#include "H5VL_log_wrap.hpp"
#include "H5VL_logi.hpp"

herr_t H5VL_log_obji_iterate_op (hid_t obj,
                                 const char *name,
                                 const H5O_info2_t *info,
                                 void *op_data) {
    H5VL_log_obji_iterate_op_data *ctx = (H5VL_log_obji_iterate_op_data *)op_data;

    // Skip internal objects
    if (name) {
        if (name[0] == '_') {
            if (name[1] == '_')
                name++;
            else {
                return 0;
            }
        }
        return ctx->op (obj, name, info, ctx->op_data);
    }

    return 0;
}

void *H5VL_log_obj_open_with_uo (void *obj,
                                 void *uo,
                                 H5I_type_t type,
                                 const H5VL_loc_params_t *loc_params) {
    H5VL_log_obj_t *pp = (H5VL_log_obj_t *)obj;
    std::unique_ptr<H5VL_log_obj_t> op;

    /* Check arguments */
    // if(loc_params->type != H5VL_OBJECT_BY_SELF) ERR_OUT("loc_params->type is not
    // H5VL_OBJECT_BY_SELF")

    op = std::make_unique<H5VL_log_obj_t> (pp, type, uo);
    CHECK_PTR (op);

    return (void *)(op.release ());
} /* end H5VL_log_group_ppen() */

H5VL_log_obj_t::H5VL_log_obj_t () {
    this->fp    = NULL;
    this->uvlid = -1;
    this->uo    = NULL;
    this->type  = H5I_UNINIT;
#ifdef LOGVOL_DEBUG
    this->ext_ref = 0;
#endif
}
H5VL_log_obj_t::H5VL_log_obj_t (struct H5VL_log_obj_t *pp) {
    this->fp    = pp->fp;
    this->uvlid = pp->uvlid;
    H5VL_log_filei_inc_ref (this->fp);
#ifdef LOGVOL_DEBUG
    this->ext_ref = 1;
#endif
    H5VL_logi_inc_ref (this->uvlid);
}
H5VL_log_obj_t::H5VL_log_obj_t (struct H5VL_log_obj_t *pp, H5I_type_t type) : H5VL_log_obj_t (pp) {
    this->type = type;
}
H5VL_log_obj_t::H5VL_log_obj_t (struct H5VL_log_obj_t *pp, H5I_type_t type, void *uo)
    : H5VL_log_obj_t (pp, type) {
    this->uo = uo;
}
H5VL_log_obj_t::~H5VL_log_obj_t () {
    hid_t err_id;

    if (this->fp && (this->fp != this)) { H5VL_log_filei_dec_ref (this->fp); }
    if (this->uvlid >= 0) {
#ifdef LOGVOL_DEBUG
        this->ext_ref--;
        assert (this->ext_ref >= 0);
#endif
        err_id = H5Eget_current_stack ();
        H5VL_logi_dec_ref (this->uvlid);
        H5Eset_current_stack (err_id);
    }
}
