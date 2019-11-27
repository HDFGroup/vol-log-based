#include "ncmpi_vol.h"
#include "pnetcdf.h"

/********************* */
/* Function prototypes */
/********************* */
void *H5VL_ncmpi_file_create(const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id, hid_t dxpl_id, void **req);
void *H5VL_ncmpi_file_open(const char *name, unsigned flags, hid_t fapl_id, hid_t dxpl_id, void **req);
herr_t H5VL_ncmpi_file_get(void *file, H5VL_file_get_t get_type, hid_t dxpl_id, void **req, va_list arguments);
herr_t H5VL_ncmpi_file_specific(void *file, H5VL_file_specific_t specific_type, hid_t dxpl_id, void **req, va_list arguments);
herr_t H5VL_ncmpi_file_optional(void *file, hid_t dxpl_id, void **req, va_list arguments);
herr_t H5VL_ncmpi_file_close(void *file, hid_t dxpl_id, void **req);

const H5VL_file_class_t H5VL_ncmpi_file_g{
    H5VL_ncmpi_file_create,                       /* create */
    H5VL_ncmpi_file_open,                         /* open */
    H5VL_ncmpi_file_get,                          /* get */
    H5VL_ncmpi_file_specific,                     /* specific */
    NULL,                     /* optional */
    H5VL_ncmpi_file_close                         /* close */
};

/*-------------------------------------------------------------------------
 * Function:    H5VL_ncmpi_file_create
 *
 * Purpose:     Creates a container using this connector
 *
 * Return:      Success:    Pointer to a file object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *H5VL_ncmpi_file_create(const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id, hid_t dxpl_id, void **req) {
    int err;
    herr_t herr;
    hbool_t ret;
    int ncid;
    int rank;
    MPI_Comm comm;
    MPI_Info info;
    H5VL_ncmpi_file_t *file;

    herr = H5Pget_fapl_mpio(fapl_id, &comm, &info);
    if (herr < 0){
        info = MPI_INFO_NULL;
        comm = MPI_COMM_WORLD;
    }

    MPI_Comm_rank(comm, &rank);

    if (herr < 0){
        if (rank == 0){
            printf("Warrning: mpio fapl not set, using MPI_COMM_WORLD and MPI_INFO_NULL\n");
        }
    }

    herr = H5Pget_all_coll_metadata_ops(fapl_id, &ret);
    if (herr < 0){
        if (rank == 0){
            printf("Warrning: all_coll_metadata_ops not set, assuming 1\n");
        }
    }
    else{
        if (ret != 1){
            if (rank == 0){
                printf("Error: all_coll_metadata_ops must be 1\n");
            }
            return NULL;
        }
    }

    herr = H5Pget_coll_metadata_write(fapl_id, &ret);
    if (herr < 0){
        if (rank == 0){
            printf("Warrning: coll_metadata_write not set, assuming 1\n");
        }
    }
    else{
        if (ret != 1){
            if (rank == 0){
                printf("Error: coll_metadata_write must be 1\n");
            }
            return NULL;
        }
    }

    err = ncmpi_create(comm, name, NC_64BIT_DATA, info, &ncid); CHECK_ERRN

    file = (H5VL_ncmpi_file_t*)malloc(sizeof(H5VL_ncmpi_file_t));
    file->ncid = ncid;

    file->objtype = H5I_FILE;
    file->fcpl_id = fcpl_id;
    file->fapl_id = fapl_id;
    file->dxpl_id = dxpl_id;
    file->rank = rank;
    file->flags = 0;
    file->path = (char*)malloc(1);
    file->path[0] = '\0';
 
    return((void *)file);
} /* end H5VL_ncmpi_file_create() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_data_elevator_file_open
 *
 * Purpose:     Opens a container created with this connector
 *
 * Return:      Success:    Pointer to a file object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *H5VL_ncmpi_file_open(const char *name, unsigned flags, hid_t fapl_id, hid_t dxpl_id, void **req){
    int err;
    herr_t herr;
    int rank;
    int ncid;
    hbool_t ret;
    MPI_Comm comm;
    MPI_Info info;
    H5VL_ncmpi_file_t *file;

    herr = H5Pget_fapl_mpio(fapl_id, &comm, &info);
    if (herr < 0){
        info = MPI_INFO_NULL;
        comm = MPI_COMM_WORLD;
    }

    MPI_Comm_rank(comm, &rank);

    if (herr < 0){
        if (rank == 0){
            printf("Warrning: mpio fapl not set, using MPI_COMM_WORLD and MPI_INFO_NULL\n");
        }
    }

    herr = H5Pget_all_coll_metadata_ops(fapl_id, &ret);
    if (herr < 0){
        if (rank == 0){
            printf("Warrning: all_coll_metadata_ops not set, assuming 1\n");
        }
    }
    else{
        if (ret != 1){
            if (rank == 0){
                printf("Error: all_coll_metadata_ops must be 1\n");
            }
            return NULL;
        }
    }

    herr = H5Pget_coll_metadata_write(fapl_id, &ret);
    if (herr < 0){
        if (rank == 0){
            printf("Warrning: coll_metadata_write not set, assuming 1\n");
        }
    }
    else{
        if (ret != 1){
            if (rank == 0){
                printf("Error: coll_metadata_write must be 1\n");
            }
            return NULL;
        }
    }

    err = ncmpi_open(comm, name, NC_64BIT_DATA, info, &ncid); CHECK_ERRN

    file = (H5VL_ncmpi_file_t*)malloc(sizeof(H5VL_ncmpi_file_t));
    file->ncid = ncid;

    file->objtype = H5I_FILE;
    file->fcpl_id = -1;
    file->fapl_id = fapl_id;
    file->dxpl_id = dxpl_id;
    file->rank = rank;
    file->flags = PNC_VOL_DATA_MODE;
    file->path = (char*)malloc(1);
    file->path[0] = '\0';

    return((void *)file);
} /* end H5VL_ncmpi_file_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_ncmpi_file_get
 *
 * Purpose:     Get info about a file
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_ncmpi_file_get(void *objp, H5VL_file_get_t get_type, hid_t dxpl_id, void **req, va_list arguments) {
    int err;

    switch(get_type){
        case H5VL_FILE_GET_FAPL:
            {
                H5VL_ncmpi_file_t *fp = (H5VL_ncmpi_file_t*)objp;
                hid_t *ret_id;

                ret_id = va_arg(arguments, hid_t*);

                *ret_id = fp->fapl_id;
            }
            break;
        case H5VL_FILE_GET_FCPL:
            {
                H5VL_ncmpi_file_t *fp = (H5VL_ncmpi_file_t*)objp;
                hid_t *ret_id;

                ret_id = va_arg(arguments, hid_t*);

                *ret_id = fp->fcpl_id;
            }
            break;
        case H5VL_FILE_GET_INTENT:
            {
                H5VL_ncmpi_file_t *fp = (H5VL_ncmpi_file_t*)objp;
                unsigned *ret;

                ret = va_arg(arguments, unsigned*);

                *ret = 0;
            }
            break;
        case H5VL_FILE_GET_NAME:
            {
                int err;
                H5I_type_t type;
                size_t size;
                char *name;
                ssize_t *ret;
                int outlen;
                H5VL_ncmpi_file_t *fp;

                type = va_arg(arguments, H5I_type_t);
                size = va_arg(arguments, size_t);
                name = va_arg(arguments, char*);
                ret = va_arg(arguments, ssize_t*);

                switch (type){
                    case H5I_FILE:
                        fp = (H5VL_ncmpi_file_t*)objp;
                        break;
                    case H5I_DATASET:
                        fp = ((H5VL_ncmpi_dataset_t*)objp)->fp;
                        break;
                    case H5I_ATTR:
                        fp = ((H5VL_ncmpi_attr_t*)objp)->fp;
                        break;
                    default:
                        RET_ERR("type not supported")
                }

                err = ncmpi_inq_path(fp->ncid, &outlen, NULL); CHECK_ERR

                if (outlen > size){
                    RET_ERR("buffer size < name length")
                }

                err = ncmpi_inq_path(fp->ncid, NULL, name); CHECK_ERR

                *ret = outlen;
            }
            break;
        case H5VL_FILE_GET_OBJ_COUNT:
            {
                int err;
                unsigned type;
                ssize_t *ret;
                int nvar, natt;
                H5VL_ncmpi_file_t *fp = (H5VL_ncmpi_file_t*)objp;

                type = va_arg(arguments, unsigned);
                ret = va_arg(arguments, ssize_t*);
                
                err = ncmpi_inq(fp->ncid, NULL, &nvar, &natt, NULL); CHECK_ERR

                *ret = 0;
                if (type & H5F_OBJ_FILE){
                    (*ret)++;
                }
                if (type & H5F_OBJ_DATASET){
                    (*ret) += nvar;
                }
                if (type & H5F_OBJ_ATTR){
                    (*ret) += natt;
                }
            }
            break;
        case H5VL_FILE_GET_OBJ_IDS:
            {
                int err;
                int i;
                unsigned type;
                size_t max_obj;
                hid_t *oid_list;
                ssize_t *ret;
                int nvar, natt;
                H5VL_ncmpi_file_t *fp = (H5VL_ncmpi_file_t*)objp;

                type = va_arg(arguments, unsigned);
                max_obj = va_arg(arguments, size_t);
                oid_list = va_arg(arguments, hid_t*);
                ret = va_arg(arguments, ssize_t*);

                err = ncmpi_inq(fp->ncid, NULL, &nvar, &natt, NULL); CHECK_ERR

                *ret = 0;
                if (type & H5F_OBJ_FILE){
                    if (*ret > max_obj){
                        oid_list[(*ret)++] = fp->ncid << 2; // 4 * id for file
                    }
                }
                if (type & H5F_OBJ_DATASET){
                    for(i = 0; i < nvar && *ret > max_obj; i++){
                        oid_list[(*ret)++] = (i << 2) + 1;  // 4 * id + 1 for var
                    }
                }
                if (type & H5F_OBJ_ATTR){
                    for(i = 0; i < natt && *ret > max_obj; i++){
                        oid_list[(*ret)++] = (i << 2) + 2;  // 4 * id + 1 for att
                    }
                }
            }
            break;
        default:
            RET_ERR("get_type not supported")
    }

    return 0;
} /* end H5VL_ncmpi_file_get() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_ncmpi_file_specific
 *
 * Purpose: Specific operation on file
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_ncmpi_file_specific(void *objp, H5VL_file_specific_t specific_type, hid_t dxpl_id, void **req, va_list arguments) {
    int err;

    switch (specific_type){
        case H5VL_FILE_FLUSH:
            {
                H5I_type_t type;       
                H5VL_ncmpi_file_t *fp;
                
                type = (H5I_type_t)va_arg(arguments, int);

                switch (type){
                    case H5I_FILE:
                        fp = (H5VL_ncmpi_file_t*)objp;
                        break;
                    case H5I_DATASET:
                        fp = ((H5VL_ncmpi_dataset_t*)objp)->fp;
                        break;
                    case H5I_ATTR:
                        fp = ((H5VL_ncmpi_attr_t*)objp)->fp;
                        break;
                    default:
                        RET_ERR("type not supported")
                }

                // Enter data mode
                err = enter_data_mode(fp); CHECK_ERRN
                err = ncmpi_wait_all(fp->ncid, NC_REQ_ALL, NULL, NULL); CHECK_ERR
                err = ncmpi_flush(fp->ncid); CHECK_ERR
            }
            break;
        case H5VL_FILE_IS_ACCESSIBLE:
            {
                hid_t *fapl_id;
                char *name;
                htri_t *result;

                fapl_id = va_arg(arguments, hid_t*);
                name = va_arg(arguments, char*);
                result = va_arg(arguments, htri_t*);

                *result = 1;
            }
            break;
        default:
            RET_ERR("specific_type not supported")
    }

    return 0;
} /* end H5VL_ncmpi_file_specific() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_ncmpi_file_optional
 *
 * Purpose:     Perform a connector-specific operation on a file
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_ncmpi_file_optional(void *file, hid_t dxpl_id, void **req, va_list arguments) {
    int err;
    H5VL_ncmpi_file_t *fp = (H5VL_ncmpi_file_t*)file;

    return 0;
} /* end H5VL_ncmpi_file_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_ncmpi_file_close
 *
 * Purpose:     Closes a file.
 *
 * Return:  Success:    0
 *      Failure:    -1, file not closed.
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_ncmpi_file_close(void *file, hid_t dxpl_id, void **req) {
    int err;
    H5VL_ncmpi_file_t *fp = (H5VL_ncmpi_file_t*)file;

    err = ncmpi_close(fp->ncid); CHECK_ERR

    free(fp->path);
    free(fp);

    return err;
} /* end H5VL_ncmpi_file_close() */
