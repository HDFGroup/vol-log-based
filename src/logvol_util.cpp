#include "logvol.h"

/*-------------------------------------------------------------------------
 * Function:    H5VL__H5VL_log_new_obj
 *
 * Purpose:     Create a new pass through object for an underlying object
 *
 * Return:      Success:    Pointer to the new pass through object
 *              Failure:    NULL
 *
 * Programmer:  Quincey Koziol
 *              Monday, December 3, 2018
 *
 *-------------------------------------------------------------------------
 */
H5VL_log_t *
H5VL_log_new_obj(void *under_obj, hid_t under_vol_id)
{
    H5VL_log_t *new_obj;

    new_obj = (H5VL_log_t *)calloc(1, sizeof(H5VL_log_t));
    new_obj->under_object = under_obj;
    new_obj->under_vol_id = under_vol_id;
    H5Iinc_ref(new_obj->under_vol_id);

    return new_obj;
} /* end H5VL__H5VL_log_new_obj() */


/*-------------------------------------------------------------------------
 * Function:    H5VL__H5VL_log_free_obj
 *
 * Purpose:     Release a pass through object
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
herr_t
H5VL_log_free_obj(H5VL_log_t *obj)
{
    hid_t err_id;

    err_id = H5Eget_current_stack();

    H5Idec_ref(obj->under_vol_id);

    H5Eset_current_stack(err_id);

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
hid_t
H5VL_log_register(void)
{
    /* Singleton register the pass-through VOL connector ID */
    if(H5VL_PASSTHRU_g < 0)
        H5VL_PASSTHRU_g = H5VLregister_connector(&H5VL_log_g, H5P_DEFAULT);

    return H5VL_PASSTHRU_g;
} /* end H5VL_log_register() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_log_init
 *
 * Purpose:     Initialize this VOL connector, performing any necessary
 *              operations for the connector that will apply to all containers
 *              accessed with the connector.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_log_init(hid_t vipl_id)
{
#ifdef ENABLE_PASSTHRU_LOGGING
    printf("------- PASS THROUGH VOL INIT\n");
#endif

    /* Shut compiler up about unused parameter */
    vipl_id = vipl_id;

    return 0;
} /* end H5VL_log_init() */


/*---------------------------------------------------------------------------
 * Function:    H5VL_log_term
 *
 * Purpose:     Terminate this VOL connector, performing any necessary
 *              operations for the connector that release connector-wide
 *              resources (usually created / initialized with the 'init'
 *              callback).
 *
 * Return:      Success:    0
 *              Failure:    (Can't fail)
 *
 *---------------------------------------------------------------------------
 */
herr_t
H5VL_log_term(void)
{
#ifdef ENABLE_PASSTHRU_LOGGING
    printf("------- PASS THROUGH VOL TERM\n");
#endif

    /* Reset VOL ID */
    H5VL_PASSTHRU_g = H5I_INVALID_HID;

    return 0;
} /* end H5VL_log_term() */


/*---------------------------------------------------------------------------
 * Function:    H5VL_log_info_copy
 *
 * Purpose:     Duplicate the connector's info object.
 *
 * Returns:     Success:    New connector info object
 *              Failure:    NULL
 *
 *---------------------------------------------------------------------------
 */
void *
H5VL_log_info_copy(const void *_info)
{
    const H5VL_log_info_t *info = (const H5VL_log_info_t *)_info;
    H5VL_log_info_t *new_info;

#ifdef ENABLE_PASSTHRU_LOGGING
    printf("------- PASS THROUGH VOL INFO Copy\n");
#endif

    /* Allocate new VOL info struct for the pass through connector */
    new_info = (H5VL_log_info_t *)calloc(1, sizeof(H5VL_log_info_t));

    /* Increment reference count on underlying VOL ID, and copy the VOL info */
    new_info->under_vol_id = info->under_vol_id;
    H5Iinc_ref(new_info->under_vol_id);
    if(info->under_vol_info)
        H5VLcopy_connector_info(new_info->under_vol_id, &(new_info->under_vol_info), info->under_vol_info);

    return new_info;
} /* end H5VL_log_info_copy() */


/*---------------------------------------------------------------------------
 * Function:    H5VL_log_info_cmp
 *
 * Purpose:     Compare two of the connector's info objects, setting *cmp_value,
 *              following the same rules as strcmp().
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
herr_t
H5VL_log_info_cmp(int *cmp_value, const void *_info1, const void *_info2)
{
    const H5VL_log_info_t *info1 = (const H5VL_log_info_t *)_info1;
    const H5VL_log_info_t *info2 = (const H5VL_log_info_t *)_info2;

#ifdef ENABLE_PASSTHRU_LOGGING
    printf("------- PASS THROUGH VOL INFO Compare\n");
#endif

    /* Sanity checks */
    assert(info1);
    assert(info2);

    /* Initialize comparison value */
    *cmp_value = 0;
    
    /* Compare under VOL connector classes */
    H5VLcmp_connector_cls(cmp_value, info1->under_vol_id, info2->under_vol_id);
    if(*cmp_value != 0)
        return 0;

    /* Compare under VOL connector info objects */
    H5VLcmp_connector_info(cmp_value, info1->under_vol_id, info1->under_vol_info, info2->under_vol_info);
    if(*cmp_value != 0)
        return 0;

    return 0;
} /* end H5VL_log_info_cmp() */


/*---------------------------------------------------------------------------
 * Function:    H5VL_log_info_free
 *
 * Purpose:     Release an info object for the connector.
 *
 * Note:	Take care to preserve the current HDF5 error stack
 *		when calling HDF5 API calls.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
herr_t
H5VL_log_info_free(void *_info)
{
    H5VL_log_info_t *info = (H5VL_log_info_t *)_info;
    hid_t err_id;

#ifdef ENABLE_PASSTHRU_LOGGING
    printf("------- PASS THROUGH VOL INFO Free\n");
#endif

    err_id = H5Eget_current_stack();

    /* Release underlying VOL ID and info */
    if(info->under_vol_info)
        H5VLfree_connector_info(info->under_vol_id, info->under_vol_info);
    H5Idec_ref(info->under_vol_id);

    H5Eset_current_stack(err_id);

    /* Free pass through info object itself */
    free(info);

    return 0;
} /* end H5VL_log_info_free() */


/*---------------------------------------------------------------------------
 * Function:    H5VL_log_info_to_str
 *
 * Purpose:     Serialize an info object for this connector into a string
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
herr_t
H5VL_log_info_to_str(const void *_info, char **str)
{
    const H5VL_log_info_t *info = (const H5VL_log_info_t *)_info;
    H5VL_class_value_t under_value = (H5VL_class_value_t)-1;
    char *under_vol_string = NULL;
    size_t under_vol_str_len = 0;

#ifdef ENABLE_PASSTHRU_LOGGING
    printf("------- PASS THROUGH VOL INFO To String\n");
#endif

    /* Get value and string for underlying VOL connector */
    H5VLget_value(info->under_vol_id, &under_value);
    H5VLconnector_info_to_str(info->under_vol_info, info->under_vol_id, &under_vol_string);

    /* Determine length of underlying VOL info string */
    if(under_vol_string)
        under_vol_str_len = strlen(under_vol_string);

    /* Allocate space for our info */
    *str = (char *)H5allocate_memory(32 + under_vol_str_len, (hbool_t)0);
    assert(*str);

    /* Encode our info
     * Normally we'd use snprintf() here for a little extra safety, but that
     * call had problems on Windows until recently. So, to be as platform-independent
     * as we can, we're using sprintf() instead.
     */
    sprintf(*str, "under_vol=%u;under_info={%s}", (unsigned)under_value, (under_vol_string ? under_vol_string : ""));

    return 0;
} /* end H5VL_log_info_to_str() */


/*---------------------------------------------------------------------------
 * Function:    H5VL_log_str_to_info
 *
 * Purpose:     Deserialize a string into an info object for this connector.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
herr_t
H5VL_log_str_to_info(const char *str, void **_info)
{
    H5VL_log_info_t *info;
    unsigned under_vol_value;
    const char *under_vol_info_start, *under_vol_info_end;
    hid_t under_vol_id;
    void *under_vol_info = NULL;
    
#ifdef ENABLE_PASSTHRU_LOGGING
    printf("------- PASS THROUGH VOL INFO String To Info\n");
#endif

    /* Retrieve the underlying VOL connector value and info */
    sscanf(str, "under_vol=%u;", &under_vol_value);
    under_vol_id = H5VLregister_connector_by_value((H5VL_class_value_t)under_vol_value, H5P_DEFAULT);
    under_vol_info_start = strchr(str, '{');
    under_vol_info_end = strrchr(str, '}');
    assert(under_vol_info_end > under_vol_info_start);
    if(under_vol_info_end != (under_vol_info_start + 1)) {
        char *under_vol_info_str;

        under_vol_info_str = (char *)malloc((size_t)(under_vol_info_end - under_vol_info_start));
        memcpy(under_vol_info_str, under_vol_info_start + 1, (size_t)((under_vol_info_end - under_vol_info_start) - 1));
        *(under_vol_info_str + (under_vol_info_end - under_vol_info_start)) = '\0';

        H5VLconnector_str_to_info(under_vol_info_str, under_vol_id, &under_vol_info);

        free(under_vol_info_str);
    } /* end else */

    /* Allocate new pass-through VOL connector info and set its fields */
    info = (H5VL_log_info_t *)calloc(1, sizeof(H5VL_log_info_t));
    info->under_vol_id = under_vol_id;
    info->under_vol_info = under_vol_info;

    /* Set return value */
    *_info = info;

    return 0;
} /* end H5VL_log_str_to_info() */


/*---------------------------------------------------------------------------
 * Function:    H5VL_log_get_object
 *
 * Purpose:     Retrieve the 'data' for a VOL object.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
void *
H5VL_log_get_object(const void *obj)
{
    const H5VL_log_t *o = (const H5VL_log_t *)obj;

#ifdef ENABLE_PASSTHRU_LOGGING
    printf("------- PASS THROUGH VOL Get object\n");
#endif

    return H5VLget_object(o->under_object, o->under_vol_id);
} /* end H5VL_log_get_object() */


/*---------------------------------------------------------------------------
 * Function:    H5VL_log_get_wrap_ctx
 *
 * Purpose:     Retrieve a "wrapper context" for an object
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
herr_t
H5VL_log_get_wrap_ctx(const void *obj, void **wrap_ctx)
{
    const H5VL_log_t *o = (const H5VL_log_t *)obj;
    H5VL_log_wrap_ctx_t *new_wrap_ctx;

#ifdef ENABLE_PASSTHRU_LOGGING
    printf("------- PASS THROUGH VOL WRAP CTX Get\n");
#endif

    /* Allocate new VOL object wrapping context for the pass through connector */
    new_wrap_ctx = (H5VL_log_wrap_ctx_t *)calloc(1, sizeof(H5VL_log_wrap_ctx_t));

    /* Increment reference count on underlying VOL ID, and copy the VOL info */
    new_wrap_ctx->under_vol_id = o->under_vol_id;
    H5Iinc_ref(new_wrap_ctx->under_vol_id);
    H5VLget_wrap_ctx(o->under_object, o->under_vol_id, &new_wrap_ctx->under_wrap_ctx);

    /* Set wrap context to return */
    *wrap_ctx = new_wrap_ctx;

    return 0;
} /* end H5VL_log_get_wrap_ctx() */


/*---------------------------------------------------------------------------
 * Function:    H5VL_log_wrap_object
 *
 * Purpose:     Use a "wrapper context" to wrap a data object
 *
 * Return:      Success:    Pointer to wrapped object
 *              Failure:    NULL
 *
 *---------------------------------------------------------------------------
 */
void *
H5VL_log_wrap_object(void *obj, H5I_type_t obj_type, void *_wrap_ctx)
{
    H5VL_log_wrap_ctx_t *wrap_ctx = (H5VL_log_wrap_ctx_t *)_wrap_ctx;
    H5VL_log_t *new_obj;
    void *under;

#ifdef ENABLE_PASSTHRU_LOGGING
    printf("------- PASS THROUGH VOL WRAP Object\n");
#endif

    /* Wrap the object with the underlying VOL */
    under = H5VLwrap_object(obj, obj_type, wrap_ctx->under_vol_id, wrap_ctx->under_wrap_ctx);
    if(under)
        new_obj = H5VL_log_new_obj(under, wrap_ctx->under_vol_id);
    else
        new_obj = NULL;

    return new_obj;
} /* end H5VL_log_wrap_object() */


/*---------------------------------------------------------------------------
 * Function:    H5VL_log_unwrap_object
 *
 * Purpose:     Unwrap a wrapped object, discarding the wrapper, but returning
 *		underlying object.
 *
 * Return:      Success:    Pointer to unwrapped object
 *              Failure:    NULL
 *
 *---------------------------------------------------------------------------
 */
void *
H5VL_log_unwrap_object(void *obj)
{
    H5VL_log_t *o = (H5VL_log_t *)obj;
    void *under;

#ifdef ENABLE_PASSTHRU_LOGGING
    printf("------- PASS THROUGH VOL UNWRAP Object\n");
#endif

    /* Unrap the object with the underlying VOL */
    under = H5VLunwrap_object(o->under_object, o->under_vol_id);

    if(under)
        H5VL_log_free_obj(o);

    return under;
} /* end H5VL_log_unwrap_object() */


/*---------------------------------------------------------------------------
 * Function:    H5VL_log_free_wrap_ctx
 *
 * Purpose:     Release a "wrapper context" for an object
 *
 * Note:	Take care to preserve the current HDF5 error stack
 *		when calling HDF5 API calls.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
herr_t
H5VL_log_free_wrap_ctx(void *_wrap_ctx)
{
    H5VL_log_wrap_ctx_t *wrap_ctx = (H5VL_log_wrap_ctx_t *)_wrap_ctx;
    hid_t err_id;

#ifdef ENABLE_PASSTHRU_LOGGING
    printf("------- PASS THROUGH VOL WRAP CTX Free\n");
#endif

    err_id = H5Eget_current_stack();

    /* Release underlying VOL ID and wrap context */
    if(wrap_ctx->under_wrap_ctx)
        H5VLfree_wrap_ctx(wrap_ctx->under_wrap_ctx, wrap_ctx->under_vol_id);
    H5Idec_ref(wrap_ctx->under_vol_id);

    H5Eset_current_stack(err_id);

    /* Free pass through wrap context object itself */
    free(wrap_ctx);

    return 0;
} /* end H5VL_log_free_wrap_ctx() */