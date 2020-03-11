#include "logvol_internal.hpp"

/********************* */
/* Function prototypes */
/********************* */
void *H5VL_log_file_create(const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id, hid_t dxpl_id, void **req);
void *H5VL_log_file_open(const char *name, unsigned flags, hid_t fapl_id, hid_t dxpl_id, void **req);
herr_t H5VL_log_file_get(void *file, H5VL_file_get_t get_type, hid_t dxpl_id, void **req, va_list arguments);
herr_t H5VL_log_file_specific(void *file, H5VL_file_specific_t specific_type, hid_t dxpl_id, void **req, va_list arguments);
herr_t H5VL_log_file_optional(void *file, hid_t dxpl_id, void **req, va_list arguments);
herr_t H5VL_log_file_close(void *file, hid_t dxpl_id, void **req);

const H5VL_file_class_t H5VL_log_file_g{
    H5VL_log_file_create,                       /* create */
    H5VL_log_file_open,                         /* open */
    H5VL_log_file_get,                          /* get */
    H5VL_log_file_specific,                     /* specific */
    NULL,                     /* optional */
    H5VL_log_file_close                         /* close */
};

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_file_create
 *
 * Purpose:     Creates a container using this connector
 *
 * Return:      Success:    Pointer to a file object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *H5VL_log_file_create(const char *name, unsigned flags, hid_t fcpl_id,
                            hid_t fapl_id, hid_t dxpl_id, void **req) {
    herr_t err;
    H5VL_log_info_t *info = NULL;
    H5VL_log_file_t *fp = NULL;
    H5VL_loc_params_t loc_params;
    hid_t uvlid, under_fapl_id;
    void *under_vol_info;
    MPI_Comm comm;

    // Try get info about under VOL
    H5Pget_vol_info(fapl_id, (void **)&info);

    if (info){
        uvlid = info->uvlid;
        under_vol_info = info->under_vol_info;
    }
    else{   // If no under VOL specified, use the native VOL
        assert(H5VLis_connector_registered("native") == 1);
        uvlid = H5VLget_connector_id_by_name("native");
        assert(uvlid > 0);
        under_vol_info = NULL;
    }

    // Make sure we have mpi enabled
    err = H5Pget_fapl_mpio(fapl_id, &comm, NULL); CHECK_ERR

    // Init file obj
    fp = new H5VL_log_file_t();
    fp->closing = false;
    fp->refcnt = 0;
    MPI_Comm_dup(comm, &(fp->comm));
    MPI_Comm_rank(comm, &(fp->rank));
    fp->uvlid = uvlid;

    // Create the file with underlying VOL
    under_fapl_id = H5Pcopy(fapl_id);
    H5Pset_vol(under_fapl_id, uvlid, under_vol_info);
    fp->uo = H5VLfile_create(name, flags, fcpl_id, under_fapl_id, dxpl_id, NULL); CHECK_NERR(fp->uo)
    H5Pclose(under_fapl_id);
    
    // Create LOG group
    loc_params.obj_type = H5I_FILE;
    loc_params.type = H5VL_OBJECT_BY_SELF;
    fp->lgp = H5VLgroup_create(fp->uo, &loc_params, fp->uvlid, LOG_GROUP_NAME, H5P_LINK_CREATE_DEFAULT, H5P_GROUP_CREATE_DEFAULT,  H5P_DEFAULT, dxpl_id, NULL); CHECK_NERR(fp->lgp)

    goto fn_exit;
err_out:;
    if (fp){
        delete fp;
    }
    fp = NULL;
fn_exit:;
    MPI_Comm_free(&comm);

    if (info){
        free(info);
    }

    return (void *)fp;
} /* end H5VL_log_file_create() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_log_file_open
 *
 * Purpose:     Opens a container created with this connector
 *
 * Return:      Success:    Pointer to a file object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *H5VL_log_file_open(const char *name, unsigned flags, hid_t fapl_id,
                            hid_t dxpl_id, void **req){
    herr_t err;
    H5VL_log_info_t *info = NULL;
    H5VL_log_file_t *fp = NULL;
    H5VL_loc_params_t loc_params;
    hid_t uvlid, under_fapl_id;
    void *under_vol_info;
    MPI_Comm comm;

    // Try get info about under VOL
    H5Pget_vol_info(fapl_id, (void **)&info);

    if (info){
        uvlid = info->uvlid;
        under_vol_info = info->under_vol_info;
    }
    else{   // If no under VOL specified, use the native VOL
        assert(H5VLis_connector_registered("native") == 1);
        uvlid = H5VLget_connector_id_by_name("native");
        assert(uvlid > 0);
        under_vol_info = NULL;
    }

    // Make sure we have mpi enabled
    err = H5Pget_fapl_mpio(fapl_id, &comm, NULL); CHECK_ERR

    // Init file obj
    fp = new H5VL_log_file_t();
    fp->closing = false;
    fp->refcnt = 0;
    MPI_Comm_dup(comm, &(fp->comm));
    MPI_Comm_rank(comm, &(fp->rank));
    fp->uvlid = uvlid;

    // Create the file with underlying VOL
    under_fapl_id = H5Pcopy(fapl_id);
    H5Pset_vol(under_fapl_id, uvlid, under_vol_info);
    fp->uo = H5VLfile_open(name, flags, under_fapl_id, dxpl_id, NULL); CHECK_NERR(fp->uo)
    H5Pclose(under_fapl_id);
    
    // Create LOG group
    loc_params.obj_type = H5I_FILE;
    loc_params.type = H5VL_OBJECT_BY_SELF;
    fp->lgp = H5VLgroup_open(fp->uo, &loc_params, fp->uvlid, LOG_GROUP_NAME, H5P_DEFAULT, dxpl_id, NULL); CHECK_NERR(fp->lgp)

    goto fn_exit;
err_out:;
    if (fp){
        delete fp;
    }
    fp = NULL;
fn_exit:;
    MPI_Comm_free(&comm);

    if (info){
        free(info);
    }

    return (void *)fp;
} /* end H5VL_log_file_open() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_log_file_get
 *
 * Purpose:     Get info about a file
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_file_get(void *file, H5VL_file_get_t get_type, hid_t dxpl_id,
                            void **req, va_list arguments){
    herr_t err;
    H5VL_log_file_t *fp = (H5VL_log_file_t*)file;
    
#ifdef ENABLE_PASSTHRU_LOGGING
    printf("------- LOG VOL FILE Get\n");
#endif

    err = H5VLfile_get(fp->uo, fp->uvlid, get_type, dxpl_id, req, arguments); CHECK_ERR

err_out:;
    return err;
} /* end H5VL_log_file_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_file_specific
 *
 * Purpose:     Specific operation on file
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_file_specific(  void *file, H5VL_file_specific_t specific_type,
                                hid_t dxpl_id, void **req, va_list arguments) {
    herr_t err;
    H5VL_log_file_t *fp = (H5VL_log_file_t*)file;
    

#ifdef ENABLE_PASSTHRU_LOGGING
    printf("------- LOG VOL FILE Specific\n");
#endif

    
    if(specific_type == H5VL_FILE_IS_ACCESSIBLE || specific_type == H5VL_FILE_DELETE) {
        hid_t uvlid, under_fapl_id, fapl_id;
        void *under_vol_info;
        H5VL_log_info_t *info = NULL;

        // Try get info about under VOL
        fapl_id = va_arg(arguments, hid_t);
        H5Pget_vol_info(fapl_id, (void **)&info);
        if (info){
            uvlid = info->uvlid;
            under_vol_info = info->under_vol_info;
            free(info);
        }
        else{   // If no under VOL specified, use the native VOL
            assert(H5VLis_connector_registered("native") == 1);
            uvlid = H5VLget_connector_id_by_name("native");
            assert(uvlid > 0);
            under_vol_info = NULL;
        }

        /* Call specific of under VOL */
        under_fapl_id = H5Pcopy(fapl_id);
        H5Pset_vol(under_fapl_id, uvlid, under_vol_info);
        err = H5VLfile_specific(NULL, uvlid, specific_type, dxpl_id, req, arguments); CHECK_ERR
        H5Pclose(under_fapl_id);
    } /* end else-if */
    else{
        RET_ERR("Unsupported specific_type")
    }

err_out:;
    return err;
} /* end H5VL_log_file_specific() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_log_file_optional
 *
 * Purpose:     Perform a connector-specific operation on a file
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_file_optional(void *file, H5VL_file_optional_t opt_type, hid_t dxpl_id, void **req, va_list arguments) {
    herr_t err;
    H5VL_log_file_t *fp = (H5VL_log_file_t*)file;

#ifdef ENABLE_PASSTHRU_LOGGING
    printf("------- LOG VOL File Optional\n");
#endif

    err = H5VLfile_optional(fp->uo, fp->uvlid, opt_type, dxpl_id, req, arguments); CHECK_ERR

err_out:;
    return err;
} /* end H5VL_log_file_optional() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_log_file_close
 *
 * Purpose:     Closes a file.
 *
 * Return:      Success:    0
 *              Failure:    -1, file not closed.
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_file_close(void *file, hid_t dxpl_id, void **req) {
    herr_t err;
    H5VL_log_file_t *fp = (H5VL_log_file_t*)file;

#ifdef ENABLE_PASSTHRU_LOGGING
    printf("------- LOG VOL FILE Close\n");
#endif

    err = H5VLgroup_close(fp->lgp, fp->uvlid, dxpl_id, req); CHECK_ERR

    err = H5VLfile_close(fp->uo, fp->uvlid, dxpl_id, req); CHECK_ERR

    MPI_Comm_free(&(fp->comm));
    delete fp;

err_out:
    return err;
} /* end H5VL_log_file_close() */
