#include "ncmpi_vol.h"
#include "pnetcdf.h"

/********************* */
/* Function prototypes */
/********************* */
void *H5VL_ncmpi_dataset_create(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t lcpl_id, hid_t type_id, hid_t space_id, hid_t dcpl_id, hid_t dapl_id, hid_t dxpl_id, void **req);
void *H5VL_ncmpi_dataset_open(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t dapl_id, hid_t dxpl_id, void **req);
herr_t H5VL_ncmpi_dataset_read(void *dset, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t plist_id, void *buf, void **req);
herr_t H5VL_ncmpi_dataset_write(void *dset, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t plist_id, const void *buf, void **req);
herr_t H5VL_ncmpi_dataset_get(void *dset, H5VL_dataset_get_t get_type, hid_t dxpl_id, void **req, va_list arguments);
herr_t H5VL_ncmpi_dataset_specific(void *obj, H5VL_dataset_specific_t specific_type, hid_t dxpl_id, void **req, va_list arguments);
herr_t H5VL_ncmpi_dataset_optional(void *obj, hid_t dxpl_id, void **req, va_list arguments);
herr_t H5VL_ncmpi_dataset_close(void *dset, hid_t dxpl_id, void **req);

const H5VL_dataset_class_t H5VL_ncmpi_dataset_g{
    H5VL_ncmpi_dataset_create,                /* create       */
    H5VL_ncmpi_dataset_open,                  /* open         */
    H5VL_ncmpi_dataset_read,                  /* read         */
    H5VL_ncmpi_dataset_write,                 /* write        */
    H5VL_ncmpi_dataset_get,                   /* get          */
    H5VL_ncmpi_dataset_specific,              /* specific     */
    H5VL_ncmpi_dataset_optional,              /* optional     */
    H5VL_ncmpi_dataset_close                  /* close        */
};

/*-------------------------------------------------------------------------
 * Function:    H5VL_ncmpi_dataset_create
 *
 * Purpose:     Handles the dataset create callback
 *
 * Return:      Success:    dataset pointer
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void* H5VL_ncmpi_dataset_create(void *obj, const H5VL_loc_params_t *loc_params,
                                const char *name, hid_t lcpl_id, hid_t type_id, hid_t space_id,
                                hid_t dcpl_id, hid_t dapl_id, hid_t  dxpl_id,
                                void  **req) {
    int err;
    int i;
    hsize_t *dims;
    MPI_Offset dlen;
    nc_type type;
    char tmp[1024];
    char *ppath;
    H5VL_ncmpi_dataset_t *varp;        /* New dataset's info */
    H5VL_ncmpi_file_t *fp;
    H5VL_ncmpi_group_t *gp;

    /* Check arguments */
    if((loc_params->obj_type != H5I_FILE) && (loc_params->obj_type != H5I_GROUP))   RET_ERRN("container not a file or group")
    if(loc_params->type != H5VL_OBJECT_BY_SELF) RET_ERRN("loc_params->type is not H5VL_OBJECT_BY_SELF")
    if(H5I_DATATYPE != H5Iget_type(type_id))    RET_ERRN("invalid datatype ID")
    if(H5I_DATASPACE != H5Iget_type(space_id))   RET_ERRN("invalid dataspace ID")

    if (loc_params->obj_type == H5I_FILE){
        fp = (H5VL_ncmpi_file_t*)obj;
        gp = NULL;
        ppath = NULL;
    }
    else{
        gp = (H5VL_ncmpi_group_t*)obj;
        fp = gp->fp;
        ppath = gp->path;
    }

    // Convert to NC type
    type = h5t_to_nc_type(type_id);
    if (type == NC_NAT) RET_ERRN("only native type is supported")

    // Enter define mode
    err = enter_define_mode(fp); CHECK_ERRN

    varp = (H5VL_ncmpi_dataset_t*)malloc(sizeof(H5VL_ncmpi_dataset_t));

    varp->objtype = H5I_DATASET;
    varp->dcpl_id = dcpl_id;
    varp->dapl_id = dapl_id;
    varp->dxpl_id = dxpl_id;
    varp->fp = fp;
    varp->gp = gp;
    if (ppath == NULL){
        varp->path = (char*)malloc(strlen(name) + 1);
        sprintf(varp->path, "%s", name);
        varp->name = varp->path;
    }
    else{
        varp->path = (char*)malloc(strlen(ppath) + strlen(name) + 2);
        sprintf(varp->path, "%s_%s", ppath, name);
        varp->name = varp->path + strlen(ppath) + 1;
    }
    
    varp->ndim = H5Sget_simple_extent_ndims(space_id);
    if (varp->ndim < 0)   RET_ERRN("ndim < 0")

    dims = (hsize_t*)malloc(sizeof(hsize_t) * varp->ndim );
    varp->ndim = H5Sget_simple_extent_dims(space_id, dims, NULL);
    if (varp->ndim < 0)   RET_ERRN("ndim < 0")

    varp->dimids = (int*)malloc(sizeof(int) * varp->ndim );
    for(i = 0; i < varp->ndim; i++){
        dlen = dims[i];
        sprintf(tmp, "%s_dim_%d", name, i);
        err = ncmpi_def_dim(fp->ncid, tmp, dlen, varp->dimids + i); CHECK_ERRN
    }

    err = ncmpi_def_var(fp->ncid, varp->path, type, varp->ndim, varp->dimids, &(varp->varid)); CHECK_ERRJ

    free(dims);

    return (void *)varp;

errout:
    free(varp->path);
    free(varp);

    return NULL;
} /* end H5VL_ncmpi_dataset_create() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_ncmpi_dataset_open
 *
 * Purpose:     Handles the dataset open callback
 *
 * Return:      Success:    dataset pointer
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void* H5VL_ncmpi_dataset_open(void *obj, const H5VL_loc_params_t *loc_params, const char *name, 
                            hid_t dapl_id, hid_t  dxpl_id, void  **req) {
    int err;
    int i;
    hsize_t *dims;
    MPI_Offset dlen;
    nc_type type;
    char tmp[1024];
    char *ppath;
    H5VL_ncmpi_dataset_t *varp;        /* New dataset's info */
    H5VL_ncmpi_group_t *gp;
    H5VL_ncmpi_file_t *fp;

    /* Check arguments */
    if((loc_params->obj_type != H5I_FILE) && (loc_params->obj_type != H5I_GROUP))   RET_ERRN("container not a file or group")
    if(loc_params->type != H5VL_OBJECT_BY_SELF) RET_ERRN("loc_params->type is not H5VL_OBJECT_BY_SELF")

    if (loc_params->obj_type == H5I_FILE){
        fp = (H5VL_ncmpi_file_t*)obj;
        gp = NULL;
        ppath = NULL;
    }
    else{
        gp = (H5VL_ncmpi_group_t*)obj;
        fp = gp->fp;
        ppath = gp->path;
    }

    varp = (H5VL_ncmpi_dataset_t*)malloc(sizeof(H5VL_ncmpi_dataset_t));

    varp->objtype = H5I_DATASET;
    varp->dcpl_id = -1;
    varp->dapl_id = dapl_id;
    varp->dxpl_id = dxpl_id;
    varp->fp = fp;
    varp->gp = gp;
    if (ppath == NULL){
        varp->path = (char*)malloc(strlen(name) + 1);
        sprintf(varp->path, "%s", name);
        varp->name = varp->path;
    }
    else{
        varp->path = (char*)malloc(strlen(ppath) + strlen(name) + 2);
        sprintf(varp->path, "%s_%s", ppath, name);
        varp->name = varp->path + strlen(ppath) + 1;
    }

    err = ncmpi_inq_varid(fp->ncid, varp->path, &(varp->varid)); CHECK_ERRJ

    err = ncmpi_inq_varndims(fp->ncid, varp->varid, &(varp->ndim)); CHECK_ERRJ

    varp->dimids = (int*)malloc(sizeof(int) * varp->ndim );
    err = ncmpi_inq_vardimid(fp->ncid, varp->varid, varp->dimids);  CHECK_ERRJ

    return (void *)varp;

errout:
    free(varp->path);
    free(varp);

    return NULL;
} /* end H5VL_ncmpi_dataset_open() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_ncmpi_dataset_read
 *
 * Purpose:     Handles the dataset read callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_ncmpi_dataset_read( void *obj, hid_t mem_type_id, hid_t mem_space_id,
                                hid_t file_space_id, hid_t  dxpl_id, void *buf,
                                void  **req) {
        int err = 0;
    herr_t herr = 0;
    int esize;
    int *offs = NULL;
    MPI_Datatype type;
    H5S_sel_type stype;
    H5VL_ncmpi_dataset_t *varp = (H5VL_ncmpi_dataset_t*)obj;

    // Convert to MPI type
    type = h5t_to_mpi_type(mem_type_id);
    if (type == MPI_DATATYPE_NULL) RET_ERR("only native type is supported")
    esize = nc_type_size(h5t_to_nc_type(mem_type_id));

    if (file_space_id == H5S_ALL){
        stype = H5S_SEL_ALL;
    }
    else{
        stype =  H5Sget_select_type(file_space_id);
    }

    switch (stype){
        case H5S_SEL_POINTS:
            {
                int i;
                hssize_t npoint;
                hsize_t *hstart;
                MPI_Offset *start, *count;
                MPI_Offset **starts, **counts;

                npoint = H5Sget_select_elem_npoints(file_space_id);

                hstart = (hsize_t*)malloc(sizeof(hsize_t) * varp->ndim * npoint);

                start = (MPI_Offset*)malloc(sizeof(MPI_Offset) * varp->ndim * (npoint + 1));
                count = start + varp->ndim * npoint;
                starts = (MPI_Offset**)malloc(sizeof(MPI_Offset*) * npoint * 2);
                counts = starts + npoint;

                herr = H5Sget_select_elem_pointlist(file_space_id, 0, npoint, hstart); 

                for(i = 0; i < varp->ndim; i++){
                    count[i] = 1;
                }
                for(i = 0; i < npoint * varp->ndim; i++){
                    start[i] = hstart[i];
                }
                for(i = 0; i < npoint; i++){
                    starts[i] = start + (i * varp->ndim);
                    counts[i] = count;
                }

                sortreq(varp->ndim, npoint, starts, counts);   // Sort reqs

                err = ncmpi_iget_varn(varp->fp->ncid, varp->varid, npoint, starts, counts, buf, npoint, type, NULL); 

                free(hstart);
                free(start);
                free(starts);
            }
            break;
        case H5S_SEL_HYPERSLABS:
            {
                int i, j, k, l;
                int *offs;
                int *group;
                hssize_t nblock;
                hssize_t nreq, nbreq, ngreq;
                hsize_t *hstart, **hstarts;
                MPI_Offset nelems, nelemsreq;
                MPI_Offset *start, *count;
                MPI_Offset **starts, **counts;
                char *bufp = (char*)buf;

                nblock = H5Sget_select_hyper_nblocks(file_space_id);

                hstart = (hsize_t*)malloc(sizeof(hsize_t) * varp->ndim * 2 * nblock);
                hstarts = (hsize_t**)malloc(sizeof(hsize_t*) * varp->ndim * 2 * nblock);
                for(i = 0; i < nblock; i++){
                    hstarts[i] = hstart + i * varp->ndim * 2;
                }

                herr = H5Sget_select_hyper_blocklist(file_space_id, 0, nblock, hstart);

                // Block level sorting
                sortblock(varp->ndim, nblock, hstarts); 

                // Check for interleving
                group = (int*)malloc(sizeof(int) * (nblock + 1));
                memset(group, 0, sizeof(int) * nblock);
                for(i = 0; i < nblock - 1; i++){
                    if (hlessthan(varp->ndim, hstarts[i] + varp->ndim, hstarts[i + 1])){
                        group[i + 1] = group[i] + 1;
                    }
                    else{
                        group[i + 1] = group[i];
                    }
                }
                group[i + 1] = group[i] + 1; // Dummy group to trigger process for final group

                // Count number of breaked down requests
                offs = (int*)malloc(sizeof(int) * (nblock + 1));
                nreq = 0;
                for(k = l = 0; l <= nblock; l++){
                    if (group[k] != group[l]){  // Within 1 group
                        if (l - k == 1){    // Sinlge block, no need to breakdown
                            offs[k] = nreq; // Record offset
                            nreq++;
                        }
                        else{
                            for(i = k; i < l; i++){
                                offs[i] = nreq; // Record offset
                                nbreq = 1;
                                for(j = 0; j < varp->ndim - 1; j++){
                                    nbreq *= hstarts[i][j + varp->ndim] - hstarts[i][j] + 1;
                                }
                                nreq += nbreq;
                            }
                        }
                        k = l;
                    }
                }

                // Generate varn req
                start = (MPI_Offset*)malloc(sizeof(MPI_Offset) * varp->ndim * (nreq + 1) * 2);
                count = start + varp->ndim * (nreq + 1);
                starts = (MPI_Offset**)malloc(sizeof(MPI_Offset*) * varp->ndim * 2 * (nreq + 1));
                counts = starts + varp->ndim * (nreq + 1);
                for(i = 0; i < nreq + 1; i++){
                    starts[i] = start + i * varp->ndim;
                    counts[i] = count + i * varp->ndim;
                }
                nreq = 0;
                for(k = l = 0; l <= nblock; l++){
                    if (group[k] != group[l]){  // Within 1 group
                        if (l - k == 1){    // Sinlge block, no need to breakdown
                            for(j = 0; j < varp->ndim; j++){
                                starts[nreq][j] = hstarts[k][j];
                                counts[nreq][j] = hstarts[k][varp->ndim + j] - hstarts[k][j] + 1;
                            }
                            nreq++;
                        }
                        else{
                            nbreq = nreq;
                            for(i = k; i < l; i++){ // Breakdown each reqs
                                for(j = 0; j < varp->ndim; j++){
                                    starts[nreq][j] = hstarts[i][j];
                                    counts[nreq][j] = 1;
                                }
                                counts[nreq][varp->ndim - 1] = hstarts[i][varp->ndim * 2 - 1] - hstarts[i][varp->ndim - 1] + 1;
                                nreq++;

                                while(1){
                                    memcpy(starts[nreq], starts[nreq - 1], sizeof(MPI_Offset) * varp->ndim);
                                    memcpy(counts[nreq], counts[nreq - 1], sizeof(MPI_Offset) * varp->ndim);
                                    starts[nreq][varp->ndim - 2]++;
                                    for(j = varp->ndim - 2; j > 0; j--){
                                        if (starts[nreq][j] > hstarts[i][varp->ndim + j]){
                                            starts[nreq][j] = hstarts[i][j];
                                            starts[nreq][j - 1]++;
                                        }
                                        else{
                                            break;
                                        }
                                    }
                                    if (starts[nreq][0] <= hstarts[i][varp->ndim]){
                                        nreq++;
                                    }
                                    else{
                                        break;
                                    }
                                }
                            }
                            ngreq = nreq - nbreq;
                            nreq = nbreq;

                            sortreq(varp->ndim, ngreq, starts + nreq, counts + nreq);   // Sort breaked down reqs
                            mergereq(varp->ndim, &ngreq, starts + nreq, counts + nreq); // Merge insecting reqs

                            nreq += ngreq;
                        }
                        k = l;
                    }
                }

                // Count number of elements
                nelems = 0;
                for(i = 0; i < nreq; i++){
                    nelemsreq = 1;
                    for(j = 0; j < varp->ndim; j++){
                        nelemsreq *= counts[i][j];
                    }
                    nelems += nelemsreq;
                }

                // Post request
                err = ncmpi_iget_varn(varp->fp->ncid, varp->varid, nreq, starts, counts, buf, nelems, type, NULL);

                free(hstart);
                free(hstarts);
                free(group);
                free(start);
                free(starts);
            }
            break;
        case H5S_SEL_ALL:
            {
                int i;
                MPI_Offset dlen, nelems;

                nelems = 1;
                for(i = 0; i < varp->ndim; i++){
                    err = ncmpi_inq_dimlen(varp->fp->ncid, varp->dimids[i], &dlen); CHECK_ERR
                    nelems *= dlen;
                }

                err = ncmpi_iget_var(varp->fp->ncid, varp->varid, buf, nelems, type, NULL); 
            }
            break;
        default:
            RET_ERR("Select type not supported");
    }

    return err | herr;
} /* end H5VL_ncmpi_dataset_read() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_ncmpi_dataset_write
 *
 * Purpose:     Handles the dataset write callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_ncmpi_dataset_write(void *obj, hid_t mem_type_id, hid_t mem_space_id,
                                hid_t file_space_id, hid_t  dxpl_id, const void *buf,
                                void  **req) {
    int err = 0;
    herr_t herr = 0;
    int esize;
    int *offs = NULL;
    MPI_Datatype type;
    H5S_sel_type stype;
    H5VL_ncmpi_dataset_t *varp = (H5VL_ncmpi_dataset_t*)obj;

    // Convert to MPI type
    type = h5t_to_mpi_type(mem_type_id);
    if (type == MPI_DATATYPE_NULL) RET_ERR("only native type is supported")
    esize = nc_type_size(h5t_to_nc_type(mem_type_id));

    if (file_space_id == H5S_ALL){
        stype = H5S_SEL_ALL;
    }
    else{
        stype =  H5Sget_select_type(file_space_id);
    }

    switch (stype){
        case H5S_SEL_POINTS:
            {
                int i;
                hssize_t npoint;
                hsize_t *hstart;
                MPI_Offset *start, *count;
                MPI_Offset **starts, **counts;

                npoint = H5Sget_select_elem_npoints(file_space_id);

                hstart = (hsize_t*)malloc(sizeof(hsize_t) * varp->ndim * npoint);

                start = (MPI_Offset*)malloc(sizeof(MPI_Offset) * varp->ndim * (npoint + 1));
                count = start + varp->ndim * npoint;
                starts = (MPI_Offset**)malloc(sizeof(MPI_Offset*) * npoint * 2);
                counts = starts + npoint;

                herr = H5Sget_select_elem_pointlist(file_space_id, 0, npoint, hstart); 

                for(i = 0; i < varp->ndim; i++){
                    count[i] = 1;
                }
                for(i = 0; i < npoint * varp->ndim; i++){
                    start[i] = hstart[i];
                }
                for(i = 0; i < npoint; i++){
                    starts[i] = start + (i * varp->ndim);
                    counts[i] = count;
                }

                sortreq(varp->ndim, npoint, starts, counts);   // Sort reqs

                err = ncmpi_iput_varn(varp->fp->ncid, varp->varid, npoint, starts, counts, buf, npoint, type, NULL); 

                free(hstart);
                free(start);
                free(starts);
            }
            break;
        case H5S_SEL_HYPERSLABS:
            {
                int i, j, k, l;
                int *offs;
                int *group;
                hssize_t nblock;
                hssize_t nreq, nbreq, ngreq;
                hsize_t *hstart, **hstarts;
                MPI_Offset nelems, nelemsreq;
                MPI_Offset *start, *count;
                MPI_Offset **starts, **counts;
                char *bufp = (char*)buf;

                nblock = H5Sget_select_hyper_nblocks(file_space_id);

                hstart = (hsize_t*)malloc(sizeof(hsize_t) * varp->ndim * 2 * nblock);
                hstarts = (hsize_t**)malloc(sizeof(hsize_t*) * varp->ndim * 2 * nblock);
                for(i = 0; i < nblock; i++){
                    hstarts[i] = hstart + i * varp->ndim * 2;
                }

                herr = H5Sget_select_hyper_blocklist(file_space_id, 0, nblock, hstart);

                // Block level sorting
                sortblock(varp->ndim, nblock, hstarts); 

                // Check for interleving
                group = (int*)malloc(sizeof(int) * (nblock + 1));
                memset(group, 0, sizeof(int) * nblock);
                for(i = 0; i < nblock - 1; i++){
                    if (hlessthan(varp->ndim, hstarts[i] + varp->ndim, hstarts[i + 1])){
                        group[i + 1] = group[i] + 1;
                    }
                    else{
                        group[i + 1] = group[i];
                    }
                }
                group[i + 1] = group[i] + 1; // Dummy group to trigger process for final group

                // Count number of breaked down requests
                offs = (int*)malloc(sizeof(int) * (nblock + 1));
                nreq = 0;
                for(k = l = 0; l <= nblock; l++){
                    if (group[k] != group[l]){  // Within 1 group
                        if (l - k == 1){    // Sinlge block, no need to breakdown
                            offs[k] = nreq; // Record offset
                            nreq++;
                        }
                        else{
                            for(i = k; i < l; i++){
                                offs[i] = nreq; // Record offset
                                nbreq = 1;
                                for(j = 0; j < varp->ndim - 1; j++){
                                    nbreq *= hstarts[i][j + varp->ndim] - hstarts[i][j] + 1;
                                }
                                nreq += nbreq;
                            }
                        }
                        k = l;
                    }
                }

                // Generate varn req
                start = (MPI_Offset*)malloc(sizeof(MPI_Offset) * varp->ndim * (nreq + 1) * 2);
                count = start + varp->ndim * (nreq + 1);
                starts = (MPI_Offset**)malloc(sizeof(MPI_Offset*) * varp->ndim * 2 * (nreq + 1));
                counts = starts + varp->ndim * (nreq + 1);
                for(i = 0; i < nreq + 1; i++){
                    starts[i] = start + i * varp->ndim;
                    counts[i] = count + i * varp->ndim;
                }
                nreq = 0;
                for(k = l = 0; l <= nblock; l++){
                    if (group[k] != group[l]){  // Within 1 group
                        if (l - k == 1){    // Sinlge block, no need to breakdown
                            for(j = 0; j < varp->ndim; j++){
                                starts[nreq][j] = hstarts[k][j];
                                counts[nreq][j] = hstarts[k][varp->ndim + j] - hstarts[k][j] + 1;
                            }
                            nreq++;
                        }
                        else{
                            nbreq = nreq;
                            for(i = k; i < l; i++){ // Breakdown each reqs
                                for(j = 0; j < varp->ndim; j++){
                                    starts[nreq][j] = hstarts[i][j];
                                    counts[nreq][j] = 1;
                                }
                                counts[nreq][varp->ndim - 1] = hstarts[i][varp->ndim * 2 - 1] - hstarts[i][varp->ndim - 1] + 1;
                                nreq++;

                                while(1){
                                    memcpy(starts[nreq], starts[nreq - 1], sizeof(MPI_Offset) * varp->ndim);
                                    memcpy(counts[nreq], counts[nreq - 1], sizeof(MPI_Offset) * varp->ndim);
                                    starts[nreq][varp->ndim - 2]++;
                                    for(j = varp->ndim - 2; j > 0; j--){
                                        if (starts[nreq][j] > hstarts[i][varp->ndim + j]){
                                            starts[nreq][j] = hstarts[i][j];
                                            starts[nreq][j - 1]++;
                                        }
                                        else{
                                            break;
                                        }
                                    }
                                    if (starts[nreq][0] <= hstarts[i][varp->ndim]){
                                        nreq++;
                                    }
                                    else{
                                        break;
                                    }
                                }
                            }
                            ngreq = nreq - nbreq;
                            nreq = nbreq;

                            sortreq(varp->ndim, ngreq, starts + nreq, counts + nreq);   // Sort breaked down reqs
                            mergereq(varp->ndim, &ngreq, starts + nreq, counts + nreq); // Merge insecting reqs

                            nreq += ngreq;
                        }
                        k = l;
                    }
                }

                // Count number of elements
                nelems = 0;
                for(i = 0; i < nreq; i++){
                    nelemsreq = 1;
                    for(j = 0; j < varp->ndim; j++){
                        nelemsreq *= counts[i][j];
                    }
                    nelems += nelemsreq;
                }

                // Post request
                err = ncmpi_iput_varn(varp->fp->ncid, varp->varid, nreq, starts, counts, buf, nelems, type, NULL);

                free(hstart);
                free(hstarts);
                free(group);
                free(start);
                free(starts);
            }
            break;
        case H5S_SEL_ALL:
            {
                int i;
                MPI_Offset dlen, nelems;

                nelems = 1;
                for(i = 0; i < varp->ndim; i++){
                    err = ncmpi_inq_dimlen(varp->fp->ncid, varp->dimids[i], &dlen); CHECK_ERR
                    nelems *= dlen;
                }

                err = ncmpi_iput_var(varp->fp->ncid, varp->varid, buf, nelems, type, NULL); 
            }
            break;
        default:
            RET_ERR("Select type not supported");
    }

    return err | herr;
} /* end H5VL_ncmpi_dataset_write() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_ncmpi_dataset_get
 *
 * Purpose:     Handles the dataset get callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_ncmpi_dataset_get(void *obj, H5VL_dataset_get_t get_type, hid_t  dxpl_id, void  **req, va_list arguments) {
    int err;
    H5VL_ncmpi_dataset_t *varp = (H5VL_ncmpi_dataset_t*)obj;

    switch(get_type) {
        /* H5Dget_space */
        case H5VL_DATASET_GET_SPACE:
            {
                int i;
                MPI_Offset dlen;
                hsize_t *dims;
                hid_t *ret_id = va_arg(arguments, hid_t*);

                // Get dim size
                dims = (hsize_t*)malloc(sizeof(hsize_t) * varp->ndim);
                for(i = 0; i < varp->ndim; i++){
                    err = ncmpi_inq_dimlen(varp->fp->ncid, varp->dimids[i], &dlen); CHECK_ERR
                    dims[i] = dlen;
                }

                *ret_id = H5Screate_simple(varp->ndim, dims, NULL);   

                free(dims);

                break;
            }

        /* H5Dget_space_statuc */
        case H5VL_DATASET_GET_SPACE_STATUS:
            {

                break;
            }

        /* H5Dget_type */
        case H5VL_DATASET_GET_TYPE:
            {
                hid_t   *ret_id = va_arg(arguments, hid_t *);
                nc_type xtype;

                err = ncmpi_inq_vartype(varp->fp->ncid, varp->varid, &xtype); CHECK_ERR

                *ret_id = nc_to_h5t_type(xtype);

                break;
            }

        /* H5Dget_create_plist */
        case H5VL_DATASET_GET_DCPL:
            {
                hid_t   *ret_id = va_arg(arguments, hid_t *);

                *ret_id = varp->dcpl_id;

                break;
            }

        /* H5Dget_access_plist */
        case H5VL_DATASET_GET_DAPL:
            {
                hid_t   *ret_id = va_arg(arguments, hid_t *);

                *ret_id = varp->dapl_id;

                break;
            }

        /* H5Dget_storage_size */
        case H5VL_DATASET_GET_STORAGE_SIZE:
            {
                hsize_t *ret = va_arg(arguments, hsize_t *);

                break;
            }

        /* H5Dget_offset */
        case H5VL_DATASET_GET_OFFSET:
            {
                haddr_t *ret = va_arg(arguments, haddr_t *);
                MPI_Offset off;

                err = ncmpi_inq_varoffset(varp->fp->ncid, varp->varid, &off); CHECK_ERR

                *ret = off;

                break;
            }

        default:
            RET_ERR("get_type not supported")
    } /* end switch */
    
    return 0;
} /* end H5VL_ncmpi_dataset_get() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_ncmpi_dataset_specific
 *
 * Purpose:     Handles the dataset specific callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_ncmpi_dataset_specific(void *obj, H5VL_dataset_specific_t specific_type, hid_t  dxpl_id, void  **req, va_list arguments) {
    int err;
    H5VL_ncmpi_dataset_t *varp = (H5VL_ncmpi_dataset_t*)obj;

    switch(specific_type) {
        /* H5Dspecific_space */
        case H5VL_DATASET_SET_EXTENT:
            {
                const hsize_t *size = va_arg(arguments, const hsize_t *); 

                RET_ERR("can not change dataset size")

                break;
            }

        case H5VL_DATASET_FLUSH:
            {
                hid_t dset_id = va_arg(arguments, hid_t);

                err = ncmpi_flush(varp->fp->ncid); CHECK_ERR

                break;
            }

        case H5VL_DATASET_REFRESH:
            {
                hid_t dset_id = va_arg(arguments, hid_t);

                break;
            }

        default:
            RET_ERR("specific_type not supported")
    } /* end switch */
    return 0;
} /* end H5VL_ncmpi_dataset_specific() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_ncmpi_dataset_optional
 *
 * Purpose:     Handles the dataset optional callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_ncmpi_dataset_optional(void *obj, hid_t  dxpl_id, void  **req, va_list arguments) {

    return 0;
} /* end H5VL_ncmpi_dataset_optional() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_ncmpi_dataset_close
 *
 * Purpose:     Handles the dataset close callback
 *
 * Return:      Success:    SUCCEED
 *              Failure:    FAIL (dataset will not be closed)
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_ncmpi_dataset_close(void *dset, hid_t  dxpl_id, void  **req) {
    H5VL_ncmpi_dataset_t *varp = (H5VL_ncmpi_dataset_t*)dset;
    
    free(varp->dimids);
    free(varp->path);
    free(varp);

    return 0;
} /* end H5VL_ncmpi_dataset_close() */

