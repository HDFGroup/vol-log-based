#include "ncmpi_vol.h"

/* Characteristics of the pass-through VOL connector */
#define H5VL_NCMPI_NAME        "PnetCDF"
#define H5VL_NCMPI_VALUE        1026           /* VOL connector ID */
#define H5VL_NCMPI_VERSION      1111

/********************* */
/* Function prototypes */
/********************* */
herr_t H5VL_ncmpi_init(hid_t vipl_id);
herr_t H5VL_ncmpi_term(void);
void *H5VL_ncmpi_info_copy(const void *info);
herr_t H5VL_ncmpi_info_cmp(int *cmp_value, const void *info1, const void *info2);
herr_t H5VL_ncmpi_info_free(void *info);
herr_t H5VL_ncmpi_info_to_str(const void *info, char **str);
herr_t H5VL_ncmpi_str_to_info(const char *str, void **info);
void *H5VL_ncmpi_get_object(const void *obj);
herr_t H5VL_ncmpi_get_wrap_ctx(const void *obj, void **wrap_ctx);
herr_t H5VL_ncmpi_free_wrap_ctx(void *obj);
void *H5VL_ncmpi_wrap_object(void *obj, H5I_type_t obj_type, void *wrap_ctx);
void *H5VL_ncmpi_unwrap_object(void *wrap_ctx);

/*******************/
/* Local variables */
/*******************/

/* PNC VOL connector class struct */
const H5VL_class_t H5VL_ncmpi_g = {
    H5VL_NCMPI_VERSION,                          /* version      */
    (H5VL_class_value_t)H5VL_NCMPI_VALUE,        /* value        */
    H5VL_NCMPI_NAME,                             /* name         */
    0,                                           /* capability flags */
    H5VL_ncmpi_init,                         /* initialize   */
    H5VL_ncmpi_term,                         /* terminate    */
    {
        sizeof(H5VL_ncmpi_info_t),               /* info size    */
        H5VL_ncmpi_info_copy,                    /* info copy    */
        H5VL_ncmpi_info_cmp,                     /* info compare */
        H5VL_ncmpi_info_free,                    /* info free    */
        H5VL_ncmpi_info_to_str,                  /* info to str  */
        H5VL_ncmpi_str_to_info,                  /* str to info  */
    },
    {
        H5VL_ncmpi_get_object,                   /* get_object   */
        H5VL_ncmpi_get_wrap_ctx,                 /* get_wrap_ctx */
        H5VL_ncmpi_wrap_object,                  /* wrap_object  */
        H5VL_ncmpi_unwrap_object,                  /* wrap_object  */
        H5VL_ncmpi_free_wrap_ctx,                /* free_wrap_ctx */
    },
    H5VL_ncmpi_attr_g,
    H5VL_ncmpi_dataset_g,
    {                                               /* datatype_cls */
        NULL,                   /* commit */
        NULL,                     /* open */
        NULL,                      /* get_size */
        NULL,                 /* specific */
        NULL,                 /* optional */
        NULL                     /* close */
    },
    H5VL_ncmpi_file_g,                  /* file_cls */
    H5VL_ncmpi_group_g,                  /* group_cls */
    {                                           /* link_cls */
        NULL,                       /* create */
        NULL,                         /* copy */
        NULL,                         /* move */
        NULL,                          /* get */
        NULL,                     /* specific */
        NULL,                     /* optional */
    },
    {                                           /* object_cls */
        NULL,                       /* open */
        NULL,                       /* copy */
        NULL,                        /* get */
        NULL,                   /* specific */
        NULL,                   /* optional */
    },
    {                                           /* request_cls */
        NULL,                      /* wait */
        NULL,                    /* notify */
        NULL,                    /* cancel */
        NULL,                  /* specific */
        NULL,                  /* optional */
        NULL                       /* free */
    },
    NULL                                        /* optional */
};

/* The connector identification number, initialized at runtime */
hid_t H5VL_NCMPI_g = H5I_INVALID_HID;

/*-------------------------------------------------------------------------
 * Function:    H5VL_ncmpi_init
 *
 * Purpose:     Initialize this VOL connector, performing any necessary
 *      operations for the connector that will apply to all containers
 *              accessed with the connector.
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_ncmpi_init(hid_t vipl_id) {

    return(0);
} /* end H5VL_ncmpi_init() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_ncmpi_term
 *
 * Purpose:     Terminate this VOL connector, performing any necessary
 *      operations for the connector that release connector-wide
 *      resources (usually created / initialized with the 'init'
 *      callback).
 *
 * Return:  Success:    0
 *      Failure:    (Can't fail)
 *
 *---------------------------------------------------------------------------
 */
herr_t H5VL_ncmpi_term(void) {

    return(0);
} /* end H5VL_ncmpi_term() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_ncmpi_info_copy
 *
 * Purpose:     Duplicate the connector's info object.
 *
 * Returns: Success:    New connector info object
 *      Failure:    NULL
 *
 *---------------------------------------------------------------------------
 */
void* H5VL_ncmpi_info_copy(const void *_info) {
    const H5VL_ncmpi_info_t *info = (const H5VL_ncmpi_info_t *)_info;
    H5VL_ncmpi_info_t *new_info;

    /* Allocate new VOL info struct for the PNC connector */
    new_info = (H5VL_ncmpi_info_t *)calloc(1, sizeof(H5VL_ncmpi_info_t));

    MPI_Comm_dup(info->comm, &(new_info->comm));

    return(new_info);
} /* end H5VL_ncmpi_info_copy() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_ncmpi_info_cmp
 *
 * Purpose:     Compare two of the connector's info objects, setting *cmp_value,
 *      following the same rules as strcmp().
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
herr_t H5VL_ncmpi_info_cmp(int *cmp_value, const void *_info1, const void *_info2) {
    const H5VL_ncmpi_info_t *info1 = (const H5VL_ncmpi_info_t *)_info1;
    const H5VL_ncmpi_info_t *info2 = (const H5VL_ncmpi_info_t *)_info2;

    /* Sanity checks */
    assert(info1);
    assert(info2);

    /* Initialize comparison value */
    *cmp_value = 0;

    return(0);
} /* end H5VL_ncmpi_info_cmp() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_ncmpi_info_free
 *
 * Purpose:     Release an info object for the connector.
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
herr_t H5VL_ncmpi_info_free(void *_info) {
    H5VL_ncmpi_info_t *info = (H5VL_ncmpi_info_t *)_info;

    /* Release MPI_Info */
    MPI_Comm_free(&(info->comm));

    /* Free PNC info object itself */
    free(info);

    return(0);
} /* end H5VL_ncmpi_info_free() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_ncmpi_info_to_str
 *
 * Purpose:     Serialize an info object for this connector into a string
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
herr_t H5VL_ncmpi_info_to_str(const void *_info, char **str) {
    const H5VL_ncmpi_info_t *info = (const H5VL_ncmpi_info_t *)_info;
    H5VL_class_value_t under_value = (H5VL_class_value_t)-1;
    char *under_vol_string = NULL;
    size_t under_vol_str_len = 0;

    return(0);
} /* end H5VL_ncmpi_info_to_str() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_ncmpi_str_to_info
 *
 * Purpose:     Deserialize a string into an info object for this connector.
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
herr_t H5VL_ncmpi_str_to_info(const char *str, void **_info) {
    return(0);
} /* end H5VL_ncmpi_str_to_info() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_ncmpi_get_object
 *
 * Purpose:     Retrieve the 'data' for a VOL object.
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
void * H5VL_ncmpi_get_object(const void *obj) {
    return NULL;
} /* end H5VL_ncmpi_get_object() */


/*---------------------------------------------------------------------------
 * Function:    H5VL_ncmpi_get_wrap_ctx
 *
 * Purpose:     Retrieve a "wrapper context" for an object
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
herr_t H5VL_ncmpi_get_wrap_ctx(const void *obj, void **wrap_ctx) {
    return(0);
} /* end H5VL_ncmpi_get_wrap_ctx() */


/*---------------------------------------------------------------------------
 * Function:    H5VL_ncmpi_wrap_object
 *
 * Purpose:     Use a "wrapper context" to wrap a data object
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
void* H5VL_ncmpi_wrap_object(void *obj, H5I_type_t type, void *_wrap_ctx) {
    return NULL;
} /* end H5VL_ncmpi_wrap_object() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_ncmpi_wrap_object
 *
 * Purpose:     Use a "wrapper context" to wrap a data object
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
void* H5VL_ncmpi_unwrap_object(void *_wrap_ctx) {
    return NULL;
} /* end H5VL_ncmpi_wrap_object() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_ncmpi_free_wrap_ctx
 *
 * Purpose:     Release a "wrapper context" for an object
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
herr_t H5VL_ncmpi_free_wrap_ctx(void *_wrap_ctx) {
    return(0);
} /* end H5VL_ncmpi_free_wrap_ctx() */

