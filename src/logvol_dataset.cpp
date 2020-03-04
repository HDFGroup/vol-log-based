#include "logvol_internal.hpp"

/********************* */
/* Function prototypes */
/********************* */
void *H5VL_log_dataset_create(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t lcpl_id, hid_t type_id, hid_t space_id, hid_t dcpl_id, hid_t dapl_id, hid_t dxpl_id, void **req);
void *H5VL_log_dataset_open(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t dapl_id, hid_t dxpl_id, void **req);
herr_t H5VL_log_dataset_read(void *dset, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t plist_id, void *buf, void **req);
herr_t H5VL_log_dataset_write(void *dset, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t plist_id, const void *buf, void **req);
herr_t H5VL_log_dataset_get(void *dset, H5VL_dataset_get_t get_type, hid_t dxpl_id, void **req, va_list arguments);
herr_t H5VL_log_dataset_specific(void *obj, H5VL_dataset_specific_t specific_type, hid_t dxpl_id, void **req, va_list arguments);
herr_t H5VL_log_dataset_optional(void *obj, H5VL_dataset_optional_t optional_type, hid_t dxpl_id, void **req, va_list arguments);
herr_t H5VL_log_dataset_close(void *dset, hid_t dxpl_id, void **req);

const H5VL_dataset_class_t H5VL_log_dataset_g{
    H5VL_log_dataset_create,                /* create       */
    H5VL_log_dataset_open,                  /* open         */
    H5VL_log_dataset_read,                  /* read         */
    H5VL_log_dataset_write,                 /* write        */
    H5VL_log_dataset_get,                   /* get          */
    H5VL_log_dataset_specific,              /* specific     */
    H5VL_log_dataset_optional,              /* optional     */
    H5VL_log_dataset_close                  /* close        */
};

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
void *H5VL_log_dataset_create(  void *obj, const H5VL_loc_params_t *loc_params,
                                const char *name, hid_t lcpl_id, hid_t type_id, hid_t space_id,
                                hid_t dcpl_id, hid_t dapl_id, hid_t dxpl_id, void **req) {
    herr_t err;
    H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
    H5VL_log_dset_t *dp = NULL;
    H5VL_loc_params_t locp;
    hid_t sid = -1, asid = -1;
    void *ap;
    int ndim;
    hsize_t nd, dims[32], mdims[32];

    sid = H5Screate(H5S_SCALAR); CHECK_ID(sid);

    dp = new H5VL_log_dset_t();
    dp->uo = H5VLdataset_create(op->uo, loc_params, op->uvlid, name, lcpl_id, type_id, sid, dcpl_id, dapl_id, dxpl_id, NULL); CHECK_NERR(dp->uo);
    dp->uvlid = op->uvlid;

    // NOTE: I don't know if it work for scalar dataset, can we create zero sized attr?
    ndim = H5Sget_simple_extent_dims(space_id, dims, mdims); CHECK_ID(ndim);
    nd = (hsize_t)ndim;
    asid = H5Screate_simple(1, &nd, &nd); CHECK_ID(asid);
    locp.obj_type = H5I_DATASET;
    locp.type = H5VL_OBJECT_BY_SELF;
    ap = H5VLattr_create(dp->uo, &locp, dp->uvlid, "_dim", H5T_STD_I64LE, asid, H5P_ATTRIBUTE_CREATE_DEFAULT, H5P_ATTRIBUTE_ACCESS_DEFAULT, dxpl_id, NULL); CHECK_NERR(ap);
    err = H5VLattr_write(ap, dp->uvlid, H5T_NATIVE_INT64, dims, dxpl_id, NULL); CHECK_ERR;
    err = H5VLattr_close(ap, dp->uvlid, dxpl_id, NULL); CHECK_ERR

    goto fn_exit;
err_out:;
    printf("%d\n",err);
    if (dp) delete dp;
    dp = NULL;
fn_exit:;
    H5Sclose(asid);
    H5Sclose(sid);

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
void *
H5VL_log_dataset_open(void *obj, const H5VL_loc_params_t *loc_params,
    const char *name, hid_t dapl_id, hid_t dxpl_id, void **req)
{
    H5VL_log_obj_t *dset;
    H5VL_log_obj_t *o = (H5VL_log_obj_t *)obj;
    void *under;

#ifdef ENABLE_PASSTHRU_LOGGING 
    printf("------- LOG VOL DATASET Open\n");
#endif

    under = H5VLdataset_open(o->uo, loc_params, o->uvlid, name, dapl_id, dxpl_id, req);
    if(under) {
        dset = H5VL_log_new_obj(under, o->uvlid);

        /* Check for async request */
        if(req && *req)
            *req = H5VL_log_new_obj(*req, o->uvlid);
    } /* end if */
    else
        dset = NULL;

    return (void *)dset;
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
herr_t 
H5VL_log_dataset_read(void *dset, hid_t mem_type_id, hid_t mem_space_id,
    hid_t file_space_id, hid_t plist_id, void *buf, void **req)
{
    H5VL_log_obj_t *o = (H5VL_log_obj_t *)dset;
    herr_t ret_value;

#ifdef ENABLE_PASSTHRU_LOGGING 
    printf("------- LOG VOL DATASET Read\n");
#endif

    ret_value = H5VLdataset_read(o->uo, o->uvlid, mem_type_id, mem_space_id, file_space_id, plist_id, buf, req);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_log_new_obj(*req, o->uvlid);

    return ret_value;
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
herr_t 
H5VL_log_dataset_write(void *dset, hid_t mem_type_id, hid_t mem_space_id,
    hid_t file_space_id, hid_t plist_id, const void *buf, void **req)
{
    H5VL_log_obj_t *o = (H5VL_log_obj_t *)dset;
    herr_t ret_value;

#ifdef ENABLE_PASSTHRU_LOGGING 
    printf("------- LOG VOL DATASET Write\n");
#endif

    ret_value = H5VLdataset_write(o->uo, o->uvlid, mem_type_id, mem_space_id, file_space_id, plist_id, buf, req);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_log_new_obj(*req, o->uvlid);

    return ret_value;
} /* end H5VL_log_dataset_write() */


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
herr_t 
H5VL_log_dataset_get(void *dset, H5VL_dataset_get_t get_type,
    hid_t dxpl_id, void **req, va_list arguments)
{
    H5VL_log_obj_t *o = (H5VL_log_obj_t *)dset;
    herr_t ret_value;

#ifdef ENABLE_PASSTHRU_LOGGING 
    printf("------- LOG VOL DATASET Get\n");
#endif

    ret_value = H5VLdataset_get(o->uo, o->uvlid, get_type, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_log_new_obj(*req, o->uvlid);

    return ret_value;
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
herr_t 
H5VL_log_dataset_specific(void *obj, H5VL_dataset_specific_t specific_type, hid_t dxpl_id, void **req, va_list arguments)
{
    H5VL_log_obj_t *o = (H5VL_log_obj_t *)obj;
    hid_t uvlid;
    herr_t ret_value;

#ifdef ENABLE_PASSTHRU_LOGGING 
    printf("------- LOG VOL H5Dspecific\n");
#endif

    // Save copy of underlying VOL connector ID and prov helper, in case of
    // refresh destroying the current object
    uvlid = o->uvlid;

    ret_value = H5VLdataset_specific(o->uo, o->uvlid, specific_type, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_log_new_obj(*req, uvlid);

    return ret_value;
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
herr_t 
H5VL_log_dataset_optional(void *obj, H5VL_dataset_optional_t optional_type,
    hid_t dxpl_id, void **req, va_list arguments)
{
    H5VL_log_obj_t *o = (H5VL_log_obj_t *)obj;
    herr_t ret_value;

#ifdef ENABLE_PASSTHRU_LOGGING 
    printf("------- LOG VOL DATASET Optional\n");
#endif

    ret_value = H5VLdataset_optional(o->uo, o->uvlid, optional_type, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_log_new_obj(*req, o->uvlid);

    return ret_value;
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
herr_t H5VL_log_dataset_close(void *dset, hid_t dxpl_id, void **req){
    herr_t err;
    H5VL_log_dset_t *dp = (H5VL_log_dset_t*)dset;

    err = H5VLdataset_close(dp->uo, dp->uvlid, dxpl_id, req); CHECK_ERR

    delete dp;

err_out:;

    return err;
} /* end H5VL_log_dataset_close() */