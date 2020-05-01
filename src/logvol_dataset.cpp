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
    hid_t sid = -1;
    void *ap;
    int ndim;

    sid = H5Screate(H5S_SCALAR); CHECK_ID(sid);

    dp = new H5VL_log_dset_t();
    if (loc_params->obj_type == H5I_FILE) dp->fp = (H5VL_log_file_t*)obj;
    else if (loc_params->obj_type == H5I_GROUP) dp->fp = ((H5VL_log_group_t*)obj)->fp;
    else RET_ERR("container not a file or group")
    (dp->fp->refcnt)++;
    dp->uo = H5VLdataset_create(op->uo, loc_params, op->uvlid, name, lcpl_id, type_id, sid, dcpl_id, dapl_id, dxpl_id, NULL); CHECK_NERR(dp->uo);
    dp->uvlid = op->uvlid;
    dp->type = H5I_DATASET;
    dp->dtype = H5Tcopy(type_id); CHECK_ID(dp->dtype)
    dp->esize = H5Tget_size(type_id); CHECK_ID(dp->esize)

    // NOTE: I don't know if it work for scalar dataset, can we create zero sized attr?
    ndim = H5Sget_simple_extent_dims(space_id, dp->dims, dp->mdims); CHECK_ID(ndim);
    dp->ndim = (hsize_t)ndim;
    dp->id = (dp->fp->ndset)++;
    /*
    if (dp->fp->mdc.size() < dp->id + 1){
        dp->fp->mdc.resize(dp->id + 1);
    }
    dp->fp->mdc[dp->id] = {ndim, dp->dtype, dp->esize};
    */
    dp->fp->idx.resize(dp->fp->ndset);

    // Atts
    err = H5VL_logi_add_att(dp, "_dims", H5T_STD_I64LE, H5T_NATIVE_INT64, dp->ndim, dp->dims, dxpl_id); CHECK_ERR
    err = H5VL_logi_add_att(dp, "_mdims", H5T_STD_I64LE, H5T_NATIVE_INT64, dp->ndim, dp->mdims, dxpl_id); CHECK_ERR
    err = H5VL_logi_add_att(dp, "_ID", H5T_STD_I32LE, H5T_NATIVE_INT32, 1, &(dp->id), dxpl_id); CHECK_ERR

    goto fn_exit;
err_out:;
    printf("%d\n",err);
    if (dp){
        (dp->fp->ndset)--;
        (dp->fp->refcnt)--;
        delete dp;
        dp = NULL;
    }
fn_exit:;

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
void *H5VL_log_dataset_open(void *obj, const H5VL_loc_params_t *loc_params,
                            const char *name, hid_t dapl_id, hid_t dxpl_id, void **req) {
    herr_t err;
    H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
    H5VL_log_dset_t *dp = NULL;
    H5VL_loc_params_t locp;
    va_list args;
    void *ap;
    int ndim;

    dp = new H5VL_log_dset_t();
    if (loc_params->obj_type == H5I_FILE) dp->fp = (H5VL_log_file_t*)obj;
    else if (loc_params->obj_type == H5I_GROUP) dp->fp = ((H5VL_log_group_t*)obj)->fp;
    else RET_ERR("container not a file or group")
    dp->uo = H5VLdataset_open(op->uo, loc_params, op->uvlid, name, dapl_id, dxpl_id, NULL); CHECK_NERR(dp->uo);
    dp->uvlid = op->uvlid;
    dp->type = H5I_DATASET;
    err = H5VLdataset_get_wrapper(dp->uo, dp->uvlid, H5VL_DATASET_GET_TYPE, dxpl_id, NULL, &(dp->dtype)); CHECK_ERR
    dp->esize = H5Tget_size(dp->type); CHECK_ID(dp->esize)

    // Atts
    err = H5VL_logi_get_att_ex(dp, "_dims", H5T_NATIVE_INT64, &(dp->ndim), dp->dims, dxpl_id); CHECK_ERR
    err = H5VL_logi_get_att(dp, "_mdims", H5T_NATIVE_INT64, dp->mdims, dxpl_id); CHECK_ERR
    err = H5VL_logi_get_att(dp, "_ID", H5T_NATIVE_INT32, &(dp->id), dxpl_id); CHECK_ERR

err_out:;
    printf("%d\n",err);
    if (dp) delete dp;
    dp = NULL;

    return (void *)dp;
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
herr_t H5VL_log_dataset_read(void *dset, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t plist_id, void *buf, void **req) {
    herr_t err = 0;
    int i, j;
    int n;
    int nb;
    MPI_Offset **starts, **counts;
    size_t esize, isize;
    htri_t eqtype;
    char *bufp = (char*)buf;
    H5VL_log_rreq_t r;
    std::vector<H5VL_log_rreq_t> local_reqs;
    std::vector<H5VL_log_rreq_t> &reqs = local_reqs;
    H5VL_log_dset_t *dp = (H5VL_log_dset_t*)dset;


    if (!(dp->fp->idxvalid)){
        err = H5VL_log_filei_metaupdate(dp->fp); CHECK_ERR
    }

    // Gather starts and counts   
    err = H5VL_logi_get_selection(file_space_id, n, starts, counts); CHECK_ERR

    // Get element size
    esize = H5Tget_size(mem_type_id);

    // Check if need convert
    eqtype = H5Tequal(dp->dtype, mem_type_id); CHECK_ID(eqtype);

    // Check for nonblocking flag
    err = H5Pget_nonblocking(plist_id, &nb); CHECK_ERR
    if (nb){
        reqs = dp->fp->rreqs;
    }

    // Put request in queue
    for(i = 0; i < n; i++){
        r.rsize = 1;
        for(j = 0; j < dp->ndim; j++){
            r.rsize *= counts[i][j];
            r.start[j] = starts[i][j];
            r.count[j] = counts[i][j];
        }
        r.ibuf = bufp;  
        bufp += r.rsize * esize;

        if (eqtype == 0){
            r.dtype = H5Tcopy(dp->dtype);
            r.mtype = H5Tcopy(mem_type_id);

            if (esize >= dp->esize){
                r.xbuf = r.ibuf; 
            }
            else{
                err = H5VL_log_filei_balloc(dp->fp, r.rsize * dp->esize, (void**)(&(r.xbuf))); CHECK_ERR
            }
        }
        else{
            r.dtype = -1;
            r.mtype = -1;
            r.xbuf = r.ibuf; 
        }

        r.esize = dp->esize;
        r.did = dp->id;
        r.ndim = dp->ndim;

        reqs.push_back(r);        
    }

    // Flush it immediately if blocking
    if (!nb){
        err = H5VL_log_nb_flush_read_reqs(dp->fp, reqs, plist_id); CHECK_ERR
    }

    /* Code to handle blocking separately
    else{
        int mpierr;
        MPI_Datatype ftype, mtype;
        std::vector<H5VL_log_search_ret_t> intersections;
        MPI_Status stat;
        hssize_t nelem;
        void *xbuf;

        if ((!eqtype) && (isize < dp->esize)){
            nelem = H5Sget_select_npoints(mem_space_id); CHECK_ID(nelem)

            err = H5VL_log_filei_balloc(dp->fp, nelem * dp->esize, (void**)(&(xbuf))); CHECK_ERR
        }
        else{
            xbuf = buf;
        }

        err = H5VL_log_dataset_readi_idx_search_ex(dp->fp, dp, xbuf, n, starts, counts, intersections); CHECK_ERR

        if (intersections.size() > 0){
            err = H5VL_log_dataset_readi_gen_rtypes(intersections, &ftype, &mtype); CHECK_ERR
            mpierr = MPI_Type_commit(&mtype); CHECK_MPIERR
            mpierr = MPI_Type_commit(&ftype); CHECK_MPIERR

            mpierr = MPI_File_set_view(dp->fp->fh, 0, MPI_BYTE, ftype, "native", MPI_INFO_NULL); CHECK_MPIERR

            mpierr = MPI_File_read_all(dp->fp->fh, MPI_BOTTOM, 1, mtype, &stat); CHECK_MPIERR
        }
        else{
            mpierr = MPI_File_set_view(dp->fp->fh, 0, MPI_BYTE, MPI_DATATYPE_NULL, "native", MPI_INFO_NULL); CHECK_MPIERR

            mpierr = MPI_File_read_all(dp->fp->fh, MPI_BOTTOM, 0, MPI_DATATYPE_NULL, &stat); CHECK_MPIERR
        }

        if(!eqtype){
            err = H5Tconvert(dp->dtype, mem_type_id, nelem, xbuf, NULL, plist_id); CHECK_ERR

            if (xbuf != buf){
                memcpy(buf, xbuf, nelem * isize);

                H5VL_log_filei_bfree(dp->fp, (void**)(&(xbuf)));
            }
        }
    }
    */

err_out:;
    if (starts != NULL){
        delete[] starts[0];
        delete[] starts;
    }

    return err;
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
herr_t H5VL_log_dataset_write(  void *dset, hid_t mem_type_id, hid_t mem_space_id,
                                hid_t file_space_id, hid_t plist_id, 
                                const void *buf, void **req) {
    herr_t err = 0;
    int i, j, k, l;
    int nb;
    size_t esize;
    int nblock;
    MPI_Offset **starts, **counts;
    char *bufp = (char*)buf;
    H5VL_log_dset_t *dp = (H5VL_log_dset_t*)dset;
    H5VL_log_wreq_t r;
    htri_t eqtype;

    // Gather starts and counts   
    err = H5VL_logi_get_selection(file_space_id, nblock, starts, counts); CHECK_ERR

    // Check for nonblocking flag
    err = H5Pget_nonblocking(plist_id, &nb); CHECK_ERR

    // Get element size
    esize = H5Tget_size(mem_type_id); CHECK_ID(esize)

    // Put request in queue
    for(i = 0; i < nblock; i++){
        r.rsize = 1;
        for(j = 0; j < dp->ndim; j++){
            r.rsize *= counts[i][j];
            r.start[j] = starts[i][j];
            r.count[j] = counts[i][j];
        }

        eqtype = H5Tequal(dp->dtype, mem_type_id); CHECK_ID(eqtype);
        if (eqtype > 0){
            if (nb){
                r.buf = bufp;
                r.buf_alloc = 0;
            }
            else{
                err = H5VL_log_filei_balloc(dp->fp, r.rsize, (void**)(&(r.buf))); CHECK_ERR
                r.buf_alloc = 1;
                memcpy(r.buf, bufp, r.rsize);
            }
        }
        else{
            err = H5VL_log_filei_balloc(dp->fp, r.rsize * std::max(esize, (size_t)(dp->esize)), (void**)(&(r.buf))); CHECK_ERR
            r.buf_alloc = 1;
            memcpy(r.buf, bufp, r.rsize);
        
            err = H5Tconvert(mem_type_id, dp->dtype, r.rsize, r.buf, NULL, plist_id);
        }

        bufp += r.rsize * esize;
        
        r.rsize *= dp->esize;

        r.did = dp->id;
        r.ndim = dp->ndim;

        r.ldid = -1;
        r.ldoff = 0;

        dp->fp->wreqs.push_back(r);        
    }

err_out:;
    if (starts != NULL){
        delete starts;
    }
    return err ;
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
herr_t H5VL_log_dataset_get(void *dset, H5VL_dataset_get_t get_type, hid_t dxpl_id, void **req, va_list arguments) {
    H5VL_log_dset_t *dp = (H5VL_log_dset_t *)dset;
    herr_t err = 0;

    switch(get_type) {
        /* H5Dget_space */
        case H5VL_DATASET_GET_SPACE:
            {
                hid_t *ret_id = va_arg(arguments, hid_t*);

                *ret_id = H5Screate_simple(dp->ndim, dp->dims, NULL);   

                break;
            }

        /* H5Dget_space_status */
        case H5VL_DATASET_GET_SPACE_STATUS:
            {
                break;
            }

        /* H5Dget_type */
        case H5VL_DATASET_GET_TYPE:
            {
                hid_t   *ret_id = va_arg(arguments, hid_t *);

                *ret_id = H5Tcopy(dp->dtype);

                break;
            }

        /* H5Dget_create_plist */
        case H5VL_DATASET_GET_DCPL:
            {
                RET_ERR("get_type not supported")
                break;
            }

        /* H5Dget_access_plist */
        case H5VL_DATASET_GET_DAPL:
            {
                RET_ERR("get_type not supported")
                break;
            }

        /* H5Dget_storage_size */
        case H5VL_DATASET_GET_STORAGE_SIZE:
            {
                hsize_t *ret = va_arg(arguments, hsize_t *);

                break;
            }
        default:
            RET_ERR("get_type not supported")
    } /* end switch */
    
err_out:;
    return err;
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
herr_t H5VL_log_dataset_specific(void *obj, H5VL_dataset_specific_t specific_type, hid_t dxpl_id, void **req, va_list arguments) {
    H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
    herr_t err;

    err = H5VLdataset_specific(op->uo, op->uvlid, specific_type, dxpl_id, req, arguments);

    return err;
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
herr_t H5VL_log_dataset_optional(void *obj, H5VL_dataset_optional_t optional_type, hid_t dxpl_id, void **req, va_list arguments) {
    H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
    herr_t err;

    err = H5VLdataset_optional(op->uo, op->uvlid, optional_type, dxpl_id, req, arguments);

    return err;
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

    H5Tclose(dp->dtype);

    delete dp;

err_out:;

    return err;
} /* end H5VL_log_dataset_close() */