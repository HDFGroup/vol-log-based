#include "logvol_internal.hpp"

/********************* */
/* Function prototypes */
/********************* */
void *H5VL_log_datatype_commit(void *obj, const H5VL_loc_params_t *loc_params,
    const char *name, hid_t type_id, hid_t lcpl_id, hid_t tcpl_id, hid_t tapl_id,
    hid_t dxpl_id, void **req);
void *H5VL_log_datatype_open(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t tapl_id, hid_t dxpl_id, void **req);
herr_t H5VL_log_datatype_get(void *dt, H5VL_datatype_get_t get_type, hid_t dxpl_id, void **req, va_list arguments);
herr_t H5VL_log_datatype_specific(void *obj, H5VL_datatype_specific_t specific_type, hid_t dxpl_id, void **req, va_list arguments);
herr_t H5VL_log_datatype_optional(void *obj, H5VL_datatype_optional_t opt_type, hid_t dxpl_id, void **req, va_list arguments);
herr_t H5VL_log_datatype_close(void *dt, hid_t dxpl_id, void **req);

const H5VL_datatype_class_t H5VL_log_datatype_g{
    H5VL_log_datatype_commit,                       /* commit */
    H5VL_log_datatype_open,                         /* open */
    H5VL_log_datatype_get,                          /* get */
    H5VL_log_datatype_specific,                     /* specific */
    H5VL_log_datatype_optional,                     /* optional */
    H5VL_log_datatype_close                         /* close */
};

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_datatype_commit
 *
 * Purpose:     Commits a datatype inside a container.
 *
 * Return:      Success:    Pointer to datatype object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL_log_datatype_commit(void *obj, const H5VL_loc_params_t *loc_params,
    const char *name, hid_t type_id, hid_t lcpl_id, hid_t tcpl_id, hid_t tapl_id,
    hid_t dxpl_id, void **req)
{
    H5VL_log_obj_t *dt;
    H5VL_log_obj_t *o = (H5VL_log_obj_t *)obj;
    void *under;

#ifdef ENABLE_PASSTHRU_LOGGING 
    printf("------- LOG VOL DATATYPE Commit\n");
#endif

    under = H5VLdatatype_commit(o->uo, loc_params, o->uvlid, name, type_id, lcpl_id, tcpl_id, tapl_id, dxpl_id, req);
    if(under) {
        dt = H5VL_log_new_obj(under, o->uvlid);

        /* Check for async request */
        if(req && *req)
            *req = H5VL_log_new_obj(*req, o->uvlid);
    } /* end if */
    else
        dt = NULL;

    return (void *)dt;
} /* end H5VL_log_datatype_commit() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_log_datatype_open
 *
 * Purpose:     Opens a named datatype inside a container.
 *
 * Return:      Success:    Pointer to datatype object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL_log_datatype_open(void *obj, const H5VL_loc_params_t *loc_params,
    const char *name, hid_t tapl_id, hid_t dxpl_id, void **req)
{
    H5VL_log_obj_t *dt;
    H5VL_log_obj_t *o = (H5VL_log_obj_t *)obj;  
    void *under;

#ifdef ENABLE_PASSTHRU_LOGGING 
    printf("------- LOG VOL DATATYPE Open\n");
#endif

    under = H5VLdatatype_open(o->uo, loc_params, o->uvlid, name, tapl_id, dxpl_id, req);
    if(under) {
        dt = H5VL_log_new_obj(under, o->uvlid);

        /* Check for async request */
        if(req && *req)
            *req = H5VL_log_new_obj(*req, o->uvlid);
    } /* end if */
    else
        dt = NULL;

    return (void *)dt;
} /* end H5VL_log_datatype_open() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_log_datatype_get
 *
 * Purpose:     Get information about a datatype
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t 
H5VL_log_datatype_get(void *dt, H5VL_datatype_get_t get_type,
    hid_t dxpl_id, void **req, va_list arguments)
{
    H5VL_log_obj_t *o = (H5VL_log_obj_t *)dt;
    herr_t ret_value;

#ifdef ENABLE_PASSTHRU_LOGGING 
    printf("------- LOG VOL DATATYPE Get\n");
#endif

    ret_value = H5VLdatatype_get(o->uo, o->uvlid, get_type, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_log_new_obj(*req, o->uvlid);

    return ret_value;
} /* end H5VL_log_datatype_get() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_log_datatype_specific
 *
 * Purpose:     Specific operations for datatypes
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t 
H5VL_log_datatype_specific(void *obj, H5VL_datatype_specific_t specific_type,
    hid_t dxpl_id, void **req, va_list arguments)
{
    H5VL_log_obj_t *o = (H5VL_log_obj_t *)obj;
    hid_t uvlid;
    herr_t ret_value;

#ifdef ENABLE_PASSTHRU_LOGGING 
    printf("------- LOG VOL DATATYPE Specific\n");
#endif

    // Save copy of underlying VOL connector ID and prov helper, in case of
    // refresh destroying the current object
    uvlid = o->uvlid;

    ret_value = H5VLdatatype_specific(o->uo, o->uvlid, specific_type, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_log_new_obj(*req, uvlid);

    return ret_value;
} /* end H5VL_log_datatype_specific() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_log_datatype_optional
 *
 * Purpose:     Perform a connector-specific operation on a datatype
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t 
H5VL_log_datatype_optional(void *obj, H5VL_datatype_optional_t opt_type, hid_t dxpl_id, void **req,
    va_list arguments)
{
    H5VL_log_obj_t *o = (H5VL_log_obj_t *)obj;
    herr_t ret_value;

#ifdef ENABLE_PASSTHRU_LOGGING 
    printf("------- LOG VOL DATATYPE Optional\n");
#endif

    ret_value = H5VLdatatype_optional(o->uo, o->uvlid, opt_type, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_log_new_obj(*req, o->uvlid);

    return ret_value;
} /* end H5VL_log_datatype_optional() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_log_datatype_close
 *
 * Purpose:     Closes a datatype.
 *
 * Return:      Success:    0
 *              Failure:    -1, datatype not closed.
 *
 *-------------------------------------------------------------------------
 */
herr_t 
H5VL_log_datatype_close(void *dt, hid_t dxpl_id, void **req)
{
    H5VL_log_obj_t *o = (H5VL_log_obj_t *)dt;
    herr_t ret_value;

#ifdef ENABLE_PASSTHRU_LOGGING 
    printf("------- LOG VOL DATATYPE Close\n");
#endif

    assert(o->uo);

    ret_value = H5VLdatatype_close(o->uo, o->uvlid, dxpl_id, req);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_log_new_obj(*req, o->uvlid);

    /* Release our wrapper, if underlying datatype was closed */
    if(ret_value >= 0)
        H5VL_log_free_obj(o);

    return ret_value;
} /* end H5VL_log_datatype_close() */