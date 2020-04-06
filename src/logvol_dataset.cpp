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
    dp->fp->ndim.push_back(ndim);
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
void *H5VL_log_dataset_open(void *obj, const H5VL_loc_params_t *loc_params,
                            const char *name, hid_t dapl_id, hid_t dxpl_id, void **req) {
    herr_t err;
    H5VL_log_obj_t *op = (H5VL_log_obj_t *)obj;
    H5VL_log_dset_t *dp = NULL;
    H5VL_loc_params_t locp;
    hid_t sid = -1, asid = -1;
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
    err = H5VLdataset_get_wrapper(dp->uo, dp->uvlid, H5VL_DATASET_GET_TYPE, dxpl_id, NULL, &(dp->type)); CHECK_ERR
    dp->esize = H5Tget_size(dp->type); CHECK_ID(dp->esize)

    // Atts
    err = H5VL_logi_get_att(dp, "_dims", H5T_NATIVE_INT64, dp->dims, dxpl_id); CHECK_ERR
    err = H5VL_logi_get_att(dp, "_mdims", H5T_NATIVE_INT64, dp->mdims, dxpl_id); CHECK_ERR
    err = H5VL_logi_get_att(dp, "_ID", H5T_NATIVE_INT32, &(dp->id), dxpl_id); CHECK_ERR

    goto fn_exit;
err_out:;
    printf("%d\n",err);
    if (dp) delete dp;
    dp = NULL;
fn_exit:;
    H5Sclose(asid);

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
    int mpierr;
    int i, j;
    int n;
    MPI_Offset **starts, **counts;
    MPI_Datatype ftype, mtype;
    H5VL_log_dset_t *dp = (H5VL_log_dset_t*)dset;
    std::vector<H5VL_log_search_ret_t> intersections;
    MPI_Status stat;

    if (!(dp->fp->idxvalid)){
        err = H5VL_logi_file_metaupdate(dp->fp); CHECK_ERR
    }

    err = H5VL_logi_get_selection(file_space_id, n, starts, counts); CHECK_ERR

    err = H5VL_logi_idx_search(dp->fp, dp, n, starts, counts, intersections); CHECK_ERR

    if (intersections.size() > 0){
        err = H5VL_logi_generate_dtype(dp, intersections, &ftype, &mtype); CHECK_ERR
        mpierr = MPI_Type_commit(&mtype); CHECK_MPIERR
        mpierr = MPI_Type_commit(&ftype); CHECK_MPIERR

        mpierr = MPI_File_set_view(dp->fp->fh, 0, MPI_BYTE, ftype, "native", MPI_INFO_NULL); CHECK_MPIERR

        mpierr = MPI_File_read_all(dp->fp->fh, buf, 1, mtype, &stat); CHECK_MPIERR
    }
    else{
        mpierr = MPI_File_set_view(dp->fp->fh, 0, MPI_BYTE, MPI_DATATYPE_NULL, "native", MPI_INFO_NULL); CHECK_MPIERR

        mpierr = MPI_File_read_all(dp->fp->fh, buf, 0, MPI_DATATYPE_NULL, &stat); CHECK_MPIERR
    }

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
    H5S_sel_type stype;
    size_t esize;
    hssize_t nblock;
    hsize_t *start, **starts, **counts;
    char *bufp = (char*)buf;
    H5VL_log_dset_t *dp = (H5VL_log_dset_t*)dset;
    H5VL_log_req_t r;

    // Gather starts and counts
    stype =  H5Sget_select_type(file_space_id);
    switch (stype){
        case H5S_SEL_POINTS:
            {
                // Not supported
                RET_ERR("Point selection not supported")
            }
            break;
        case H5S_SEL_HYPERSLABS:
            {
                nblock = H5Sget_select_hyper_nblocks(file_space_id); CHECK_ID(nblock)

                start = new hsize_t[dp->ndim * 2 * nblock];
                starts = new hsize_t*[nblock * 2];
                counts = starts + nblock;

                err = H5Sget_select_hyper_blocklist(file_space_id, 0, nblock, start); CHECK_ERR

                for(i = 0; i < nblock; i++){
                    starts[i] = start + i * dp->ndim * 2;
                    counts[i] = starts[i] + dp->ndim;
                    for(j = 0; j < dp->ndim; j++){
                        counts[i][j] = counts[i][j] - starts[i][j] + 1;
                    }
                }
            }
            break;
        case H5S_SEL_ALL:
            {
                nblock = 1;

                start = new hsize_t[dp->ndim * 2 * nblock];
                starts = new hsize_t*[nblock * 2];
                counts = starts + nblock;
                starts[0] = start;
                counts[0] = starts[0] + dp->ndim;

                memset(starts[0], 0, sizeof(hsize_t) * dp->ndim);
                memcpy(counts[0], dp->dims, sizeof(hsize_t) * dp->ndim);
            }
            break;
        default:
            RET_ERR("Select type not supported");
    }

    // Get element size
    esize = H5Tget_size(mem_type_id);

    // Put request in queue
    for(i = 0; i < nblock; i++){
        r.rsize = esize;
        for(j = 0; j < dp->ndim; j++){
            r.rsize *= counts[i][j];
            r.start[j] = starts[i][j];
            r.count[j] = counts[i][j];
        }

        // TODO: Convert type, check cache size
        r.buf_alloc = 1;
        r.buf = new char[r.rsize];
        memcpy(r.buf, bufp, r.rsize);

        r.did = dp->id;
        r.ndim = dp->ndim;

        r.ldid = -1;
        r.ldoff = 0;

        dp->fp->wreqs.push_back(r);        

        bufp += r.rsize;
    }

err_out:;
    if (start != NULL){
        delete start;
    }
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
    herr_t err;

    RET_ERR("Not supported");

    goto fn_exit;
err_out:;
    err = -1;
fn_exit:;
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

    delete dp;

err_out:;

    return err;
} /* end H5VL_log_dataset_close() */