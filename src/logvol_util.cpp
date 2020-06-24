#include "logvol_internal.hpp"

extern hid_t H5VL_LOG_g;

/*-------------------------------------------------------------------------
 * Function:    H5VL__H5VL_log_new_obj
 *
 * Purpose:     Create a new log object for an underlying object
 *
 * Return:      Success:    Pointer to the new log object
 *              Failure:    NULL
 *
 * Programmer:  Quincey Koziol
 *              Monday, December 3, 2018
 *
 *-------------------------------------------------------------------------
 */
H5VL_log_obj_t *H5VL_log_new_obj(void *under_obj, hid_t uvlid) {
    H5VL_log_obj_t *new_obj;

    new_obj = (H5VL_log_obj_t *)calloc(1, sizeof(H5VL_log_obj_t));
    new_obj->uo = under_obj;
    new_obj->uvlid = uvlid;
    H5Iinc_ref(new_obj->uvlid);

    return new_obj;
} /* end H5VL__H5VL_log_new_obj() */




/*-------------------------------------------------------------------------
 * Function:    H5VL__H5VL_log_free_obj
 *
 * Purpose:     Release a log object
 *
 * Note:	Take care to preserve the current HDF5 error stack
 *		when calling HDF5 API calls.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 * Programmer:  Quincey Koziol
 *              Monday, December 3, 2018
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_free_obj(H5VL_log_obj_t *obj) {
    //hid_t err_id;

    //err_id = H5Eget_current_stack();

    H5Idec_ref(obj->uvlid);

    //H5Eset_current_stack(err_id);

    free(obj);

    return 0;
} /* end H5VL__H5VL_log_free_obj() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_log_register
 *
 * Purpose:     Register the pass-through VOL connector and retrieve an ID
 *              for it.
 *
 * Return:      Success:    The ID for the pass-through VOL connector
 *              Failure:    -1
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, November 28, 2018
 *
 *-------------------------------------------------------------------------
 */
hid_t H5VL_log_register(void) {
    /* Singleton register the pass-through VOL connector ID */
    if(H5VL_LOG_g < 0)
        H5VL_LOG_g = H5VLregister_connector(&H5VL_log_g, H5P_DEFAULT);

    return H5VL_LOG_g;
} /* end H5VL_log_register() */

herr_t H5VLfile_optional_wrapper(void *obj, hid_t connector_id, H5VL_file_optional_t opt_type, hid_t dxpl_id, void **req, ...) {
    herr_t err;
    va_list args;

    va_start(args, req);
    err = H5VLfile_optional(obj, connector_id, opt_type, dxpl_id, req, args);
    va_end(args);

    return err;
}

herr_t H5VLattr_get_wrapper(void *obj, hid_t connector_id, H5VL_attr_get_t get_type, hid_t dxpl_id, void **req, ...) {
    herr_t err;
    va_list args;

    va_start(args, req);
    err = H5VLattr_get(obj, connector_id, get_type, dxpl_id, req, args);
    va_end(args);

    return err;
}

herr_t H5VL_logi_add_att(H5VL_log_obj_t *op, const char *name, hid_t atype, hid_t mtype, hsize_t len, void *buf, hid_t dxpl_id) {
    herr_t err = 0;
    H5VL_loc_params_t loc;
    hid_t asid = -1;
    void *ap;

    asid = H5Screate_simple(1, &len, &len); CHECK_ID(asid);

    loc.obj_type = op->type;
    loc.type = H5VL_OBJECT_BY_SELF;

    ap = H5VLattr_create(op->uo, &loc, op->uvlid, name, atype, asid, H5P_ATTRIBUTE_CREATE_DEFAULT, H5P_ATTRIBUTE_ACCESS_DEFAULT, dxpl_id, NULL); CHECK_NERR(ap);
    err = H5VLattr_write(ap, op->uvlid, mtype, buf, dxpl_id, NULL); CHECK_ERR;
    err = H5VLattr_close(ap, op->uvlid, dxpl_id, NULL); CHECK_ERR

    H5Sclose(asid);

err_out:;
    return err;
}

herr_t H5VL_logi_put_att(H5VL_log_obj_t *op, const char *name, hid_t mtype, void *buf, hid_t dxpl_id) {
    herr_t err = 0;
    H5VL_loc_params_t loc;
    void *ap;

    loc.obj_type = op->type;
    loc.type = H5VL_OBJECT_BY_SELF;

    ap = H5VLattr_open(op->uo, &loc, op->uvlid, name, H5P_ATTRIBUTE_ACCESS_DEFAULT, dxpl_id, NULL); CHECK_NERR(ap);
    err = H5VLattr_write(ap, op->uvlid, mtype, buf, dxpl_id, NULL); CHECK_ERR;
    err = H5VLattr_close(ap, op->uvlid, dxpl_id, NULL); CHECK_ERR

err_out:;
    return err;
}

herr_t H5VL_logi_get_att(H5VL_log_obj_t *op, const char *name, hid_t mtype, void *buf, hid_t dxpl_id) {
    herr_t err = 0;
    H5VL_loc_params_t loc;
    void *ap;

    loc.obj_type = op->type;
    loc.type = H5VL_OBJECT_BY_SELF;

    ap = H5VLattr_open(op->uo, &loc, op->uvlid, name, H5P_ATTRIBUTE_ACCESS_DEFAULT, dxpl_id, NULL); CHECK_NERR(ap);
    err = H5VLattr_read(ap, op->uvlid, mtype, buf, dxpl_id, NULL); CHECK_ERR;
    err = H5VLattr_close(ap, op->uvlid, dxpl_id, NULL); CHECK_ERR

err_out:;
    return err;
}

herr_t H5VL_logi_get_att_ex(H5VL_log_obj_t *op, const char *name, hid_t mtype, hsize_t *len, void *buf, hid_t dxpl_id) {
    herr_t err = 0;
    H5VL_loc_params_t loc;
    hid_t asid = -1;
    int ndim;
    void *ap;

    loc.obj_type = op->type;
    loc.type = H5VL_OBJECT_BY_SELF;

    ap = H5VLattr_open(op->uo, &loc, op->uvlid, name, H5P_ATTRIBUTE_ACCESS_DEFAULT, dxpl_id, NULL); CHECK_NERR(ap);
    err = H5VLattr_get_wrapper(ap, op->uvlid, H5VL_ATTR_GET_SPACE, dxpl_id, NULL, &asid); CHECK_ERR
    ndim = H5Sget_simple_extent_dims(asid, len, NULL); CHECK_ID(ndim)
    LOG_VOL_ASSERT(ndim == 1);
    err = H5VLattr_read(ap, op->uvlid, mtype, buf, dxpl_id, NULL); CHECK_ERR;
    err = H5VLattr_close(ap, op->uvlid, dxpl_id, NULL); CHECK_ERR

err_out:;
    H5Sclose(asid);

    return err;
}

herr_t H5VLdataset_optional_wrapper(void *obj, hid_t connector_id, H5VL_dataset_optional_t opt_type, hid_t dxpl_id, void **req, ...) {
    herr_t err;
    va_list args;

    va_start(args, req);
    err = H5VLdataset_optional(obj, connector_id, opt_type, dxpl_id, req, args);
    va_end(args);

    return err;
}

herr_t H5VLdataset_specific_wrapper(void *obj, hid_t connector_id, H5VL_dataset_specific_t specific_type, hid_t dxpl_id, void **req, ...) {
    herr_t err;
    va_list args;

    va_start(args, req);
    err = H5VLdataset_specific(obj, connector_id, specific_type, dxpl_id, req, args);
    va_end(args);

    return err;
}

herr_t H5VLdataset_get_wrapper(void *obj, hid_t connector_id, H5VL_dataset_get_t get_type, hid_t dxpl_id, void **req, ...) {
    herr_t err;
    va_list args;

    va_start(args, req);
    err = H5VLdataset_get(obj, connector_id, get_type, dxpl_id, req, args);
    va_end(args);

    return err;
}

herr_t H5VLlink_specific_wrapper(void *obj, const H5VL_loc_params_t *loc_params, hid_t connector_id, H5VL_link_specific_t specific_type, hid_t dxpl_id, void **req, ...) {
    herr_t err;
    va_list args;

    va_start(args, req);
    err = H5VLlink_specific(obj, loc_params, connector_id, specific_type, dxpl_id, req, args);
    va_end(args);

    return err;
}

herr_t H5Pset_nonblocking(hid_t plist, H5VL_log_req_type_t nonblocking) {
    herr_t err;
    htri_t isdxpl;

    isdxpl = H5Pisa_class(plist, H5P_DATASET_XFER); CHECK_ID(isdxpl)
    if (isdxpl == 0) RET_ERR("Not dxplid")

    err = H5Pset(plist, "nonblocking", &nonblocking); CHECK_ERR

err_out:;
    return err;
}

herr_t H5Pget_nonblocking(hid_t plist, H5VL_log_req_type_t *nonblocking) {
    herr_t err = 0;
    htri_t isdxpl;

    isdxpl = H5Pisa_class(plist, H5P_DATASET_XFER); CHECK_ID(isdxpl)
    if (isdxpl == 0) *nonblocking = H5VL_LOG_REQ_BLOCKING;  // Default property will not pass class check
    else {
        err = H5Pget(plist, "nonblocking", nonblocking); CHECK_ERR
    }

err_out:;
    return err;
}

herr_t H5Pset_nb_buffer_size(hid_t plist, size_t size) {
    herr_t err;
    htri_t isfapl;

    isfapl = H5Pisa_class(plist, H5P_FILE_ACCESS); CHECK_ID(isfapl)
    if (isfapl == 0) RET_ERR("Not faplid")

    err = H5Pset(plist, "nb_buffer_size", &size); CHECK_ERR

err_out:;
    return err;
}

herr_t H5Pget_nb_buffer_size(hid_t plist, ssize_t *size) {
    herr_t err = 0;
    htri_t isfapl;

    isfapl = H5Pisa_class(plist, H5P_FILE_ACCESS); CHECK_ID(isfapl)
    if (isfapl == 0) *size = LOG_VOL_BSIZE_UNLIMITED;  // Default property will not pass class check
    else {
        err = H5Pget(plist, "nb_buffer_size", size); CHECK_ERR
    }

err_out:;
    return err;
}
