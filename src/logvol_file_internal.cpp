#include "logvol_internal.hpp"


herr_t H5VL_log_filei_balloc(H5VL_log_file_t *fp, size_t size, void **buf) {
    herr_t err = 0;
    size_t *bp;

    if (fp->bsize >= 0){ // Negative means infinite
        if (fp->bused + size > fp->bsize){
            err = -1;
            RET_ERR("Out of buffer")
        }
    }

    fp->bused += size;
    bp = (size_t*)malloc(size + sizeof(size_t));
    if (bp == NULL){
        err = -1;
        RET_ERR("OOM")
    }
    *buf = bp + 1;

err_out:;
    return err;
}

herr_t H5VL_log_filei_bfree(H5VL_log_file_t *fp, void *buf) {
    herr_t err = 0;
    size_t *bp;

    bp = ((size_t*)buf) - 1;
    fp->bused -= bp[0];
    free(bp);

err_out:;
    return err;
}

herr_t H5VL_log_filei_flush(H5VL_log_file_t *fp, hid_t dxplid){
    herr_t err;

    if (fp->wreqs.size() > 0){
        err = H5VL_log_nb_flush_write_reqs(fp, dxplid); CHECK_ERR
    }

    if (fp->rreqs.size() > 0){
        err = H5VL_log_nb_flush_read_reqs(fp, fp->rreqs, dxplid); CHECK_ERR
        fp->rreqs.clear();
    }

err_out:;
    return err;
}

herr_t H5VL_log_filei_metaflush(H5VL_log_file_t *fp){
    herr_t err;
    int mpierr;
    int i;
    int cnt;
    MPI_Offset *mlens = NULL, *mlens_all;
    MPI_Offset *moffs = NULL, *doffs;
    char *buf = NULL;
    char **bufp = NULL;
    H5VL_loc_params_t loc;
    void *mdp, *ldp;
    hid_t mdsid = -1, ldsid = -1, mmsid = -1, lmsid = -1;
    hsize_t start, count;
    hsize_t dsize, msize;
    htri_t has_idx;

    // Calculate size and offset of the metadata per dataset
    mlens = new MPI_Offset[fp->ndset * 2];
    mlens_all = mlens + fp->ndset;
    moffs = new MPI_Offset[(fp->ndset + 1) * 2];
    doffs = moffs + fp->ndset + 1;
    memset(mlens, 0, sizeof(MPI_Offset) * fp->ndset);
    for(auto &rp : fp->wreqs){
        mlens[rp.did] += rp.ndim * sizeof(MPI_Offset) * 2 + sizeof(int) * 2 + sizeof(MPI_Offset) + sizeof(size_t);
    }
    moffs[0] = 0;
    for(i = 0; i < fp->ndset; i++){
        moffs[i + 1] = moffs[i] + mlens[i];
    }

    // Pack data
    buf = new char[moffs[fp->ndset]];
    bufp = new char*[fp->ndset];
    
    for(i = 0; i < fp->ndset; i++){
        bufp[i] = buf + moffs[i];
    }
    for(auto &rp : fp->wreqs){
        *((int*)bufp[rp.did]) = rp.did;
        bufp[rp.did] += sizeof(int);
        *((int*)bufp[rp.did]) = rp.ndim;
        bufp[rp.did] += sizeof(int);
        memcpy(bufp[rp.did], rp.start, rp.ndim * sizeof(MPI_Offset));
        bufp[rp.did] += rp.ndim * sizeof(MPI_Offset);
        memcpy(bufp[rp.did], rp.count, rp.ndim * sizeof(MPI_Offset));
        bufp[rp.did] += rp.ndim * sizeof(MPI_Offset);
        *((MPI_Offset*)bufp[rp.did]) = rp.ldoff;
        bufp[rp.did] += sizeof(MPI_Offset);
        *((size_t*)bufp[rp.did]) = rp.rsize;
        bufp[rp.did] += sizeof(size_t);
    }

    // Sync metadata size
    mpierr = MPI_Allreduce(mlens, mlens_all, fp->ndset, MPI_LONG_LONG, MPI_SUM, fp->comm); CHECK_MPIERR
    // NOTE: Some MPI implementation do not produce output for rank 0, moffs must ne initialized to 0
    memset(moffs, 0, sizeof(MPI_Offset) * fp->ndset);
    mpierr = MPI_Exscan(mlens, moffs, fp->ndset, MPI_LONG_LONG, MPI_SUM, fp->comm); CHECK_MPIERR
    doffs[0] = 0;
    for(i = 0; i < fp->ndset; i++){
        doffs[i + 1] = doffs[i] + mlens_all[i];
        moffs[i] += doffs[i];
    }

    // Create metadata dataset
    loc.obj_type = H5I_GROUP;
    loc.type = H5VL_OBJECT_BY_SELF;
    loc.loc_data.loc_by_name.name    = "_idx";
    loc.loc_data.loc_by_name.lapl_id = H5P_DATASET_ACCESS_DEFAULT;
    err = H5VLlink_specific_wrapper(fp->lgp, &loc, fp->uvlid, H5VL_LINK_EXISTS, H5P_DATASET_XFER_DEFAULT, NULL, &has_idx); CHECK_ERR
    if (has_idx){
        // If the exist, we expand them
        mdp = H5VLdataset_open(fp->lgp, &loc, fp->uvlid, "_idx", H5P_DATASET_ACCESS_DEFAULT, fp->dxplid, NULL);
        ldp = H5VLdataset_open(fp->lgp, &loc, fp->uvlid, "_lookup", H5P_DATASET_ACCESS_DEFAULT, fp->dxplid, NULL); CHECK_NERR(ldp)

        // Resize both dataset
        dsize = (hsize_t)doffs[fp->ndset];
        err = H5VLdataset_specific_wrapper(mdp, fp->uvlid, H5VL_DATASET_SET_EXTENT, fp->dxplid, NULL, &dsize);
        dsize = fp->ndset;
        err = H5VLdataset_specific_wrapper(ldp, fp->uvlid, H5VL_DATASET_SET_EXTENT, fp->dxplid, NULL, &dsize);

        // Get data space
        err = H5VLdataset_get_wrapper(mdp, fp->uvlid, H5VL_DATASET_GET_SPACE, fp->dxplid, NULL, &mdsid); CHECK_ERR
        err = H5VLdataset_get_wrapper(ldp, fp->uvlid, H5VL_DATASET_GET_SPACE, fp->dxplid, NULL, &ldsid); CHECK_ERR
    }
    else {  // Create the idx dataset and look up table dataset
        hid_t mdcplid, ldcplid;
        
        // Space
        dsize = (hsize_t)doffs[fp->ndset];
        msize = H5S_UNLIMITED;
        mdsid = H5Screate_simple(1, &dsize, &msize); CHECK_ID(mdsid)
        dsize = fp->ndset;
        msize = H5S_UNLIMITED;
        ldsid = H5Screate_simple(1, &dsize, &msize); CHECK_ID(ldsid)

        // Chunk
        mdcplid = H5Pcreate(H5P_DATASET_CREATE); CHECK_ID(mdcplid)
        dsize = 1048576;
        err = H5Pset_chunk(mdcplid, 1, &dsize); CHECK_ERR
        ldcplid = H5Pcreate(H5P_DATASET_CREATE); CHECK_ID(ldcplid)
        dsize = 128;
        err = H5Pset_chunk(ldcplid, 1, &dsize); CHECK_ERR

        // Create
        mdp = H5VLdataset_create(fp->lgp, &loc, fp->uvlid, "_idx", H5P_LINK_CREATE_DEFAULT, H5T_STD_B8LE, mdsid, mdcplid, H5P_DATASET_ACCESS_DEFAULT, fp->dxplid, NULL); CHECK_NERR(mdp);
        ldp = H5VLdataset_create(fp->lgp, &loc, fp->uvlid, "_lookup", H5P_LINK_CREATE_DEFAULT, H5T_STD_I64LE, ldsid, ldcplid, H5P_DATASET_ACCESS_DEFAULT, fp->dxplid, NULL); CHECK_NERR(ldp);
    }

    // Write metadata
    err = H5Sselect_none(mdsid); CHECK_ERR
    for(i = 0; i < fp->ndset; i++){
        start = moffs[i];
        count = mlens[i];
        err = H5Sselect_hyperslab(mdsid, H5S_SELECT_OR, &start, NULL, &count, NULL); CHECK_ERR
    }
    msize = moffs[fp->ndset];
    mmsid = H5Screate_simple(1, &msize, &msize); CHECK_ID(mmsid)
    msize = fp->ndset;
    lmsid = H5Screate_simple(1, &msize, &msize); CHECK_ID(lmsid)
    if (fp->rank == 0){
        start = 0;
        count = fp->ndset;
        err = H5Sselect_hyperslab(ldsid, H5S_SELECT_SET, &start, NULL, &count, NULL); CHECK_ERR
    }
    else{
        err = H5Sselect_none(ldsid); CHECK_ERR
        err = H5Sselect_none(lmsid); CHECK_ERR
    }
    err = H5VLdataset_write(mdp, fp->uvlid, H5T_NATIVE_B8, mmsid, mdsid, fp->dxplid, buf, NULL);    CHECK_ERR
    err = H5VLdataset_write(ldp, fp->uvlid, H5T_STD_I64LE, lmsid, ldsid, fp->dxplid, doffs, NULL);    CHECK_ERR

    // Close the dataset
    err = H5VLdataset_close(mdp, fp->uvlid, fp->dxplid, NULL); CHECK_ERR 
    err = H5VLdataset_close(ldp, fp->uvlid, fp->dxplid, NULL); CHECK_ERR 

    if (moffs[fp->ndset] > 0){
        fp->idxvalid = false;
    }

    fp->metadirty = false;

err_out:
    // Cleanup
    if (mlens != NULL) delete mlens;
    if (moffs != NULL) delete moffs;
    if (buf != NULL) delete buf;
    if (bufp != NULL) delete bufp;
    if (mdsid >= 0) H5Sclose(mdsid);
    if (ldsid >= 0) H5Sclose(ldsid);
    if (mmsid >= 0) H5Sclose(mmsid);
    if (lmsid >= 0) H5Sclose(lmsid);

    return err;
}

herr_t H5VL_log_filei_metaupdate(H5VL_log_file_t *fp){
    herr_t err;
    int i;
    H5VL_loc_params_t loc;
    htri_t mdexist;
    void *mdp = NULL, *ldp = NULL;
    hid_t mdsid = -1, ldsid = -1;
    hsize_t mdsize, ldsize;
    char *buf = NULL, *bufp;
    int ndim;
    H5VL_log_metaentry_t entry;

    if (fp->metadirty){
        H5VL_log_filei_metaflush(fp);
    }

    loc.obj_type = H5I_GROUP;
    loc.type = H5VL_OBJECT_BY_SELF;
    loc.loc_data.loc_by_name.name = "_idx";
    loc.loc_data.loc_by_name.lapl_id = H5P_LINK_ACCESS_DEFAULT;
    err = H5VLlink_specific_wrapper(fp->lgp, &loc, fp->uvlid, H5VL_LINK_EXISTS, fp->dxplid, NULL, &mdexist); CHECK_ERR

    if (mdexist > 0){
        mdp = H5VLdataset_open(fp->lgp, &loc, fp->uvlid, "_idx", H5P_DATASET_ACCESS_DEFAULT, fp->dxplid, NULL); CHECK_NERR(mdp)
        //ldp = H5VLdataset_open(fp->lgp, &loc, fp->uvlid, "_lookup", H5P_DATASET_ACCESS_DEFAULT, fp->dxplid, NULL); CHECK_NERR(ldp)

        // Get data space and size
        err = H5VLdataset_get_wrapper(mdp, fp->uvlid, H5VL_DATASET_GET_SPACE, fp->dxplid, NULL, &mdsid); CHECK_ERR
        //err = H5VLdataset_get_wrapper(ldp, fp->uvlid, H5VL_DATASET_GET_SPACE, fp->dxplid, NULL, &ldsid); CHECK_ERR
        ndim = H5Sget_simple_extent_dims(mdsid, &mdsize, NULL); assert(ndim == 1);
        //ndim = H5Sget_simple_extent_dims(ldsid, &ldsize, NULL); assert(ndim == 1);

        // Allocate buffer
        buf = new char[mdsize];
        //fp->lut.resize(ldsize);

        // Read metadata
        err = H5VLdataset_read(mdp, fp->uvlid, H5T_NATIVE_B8, mdsid, mdsid, fp->dxplid, buf, NULL);    CHECK_ERR
        //err = H5VLdataset_read(ldp, fp->uvlid, H5T_STD_I64LE, ldsid, ldsid, fp->dxplid, fp->lut.data(), NULL);    CHECK_ERR

        // Close the dataset
        err = H5VLdataset_close(mdp, fp->uvlid, fp->dxplid, NULL); CHECK_ERR 
        //err = H5VLdataset_close(ldp, fp->uvlid, fp->dxplid, NULL); CHECK_ERR 

        // Parse metadata
        bufp = buf;
        while(bufp < buf + mdsize) {
            entry.did = *((int*)bufp);
            bufp += sizeof(int);
            ndim = *((int*)bufp);
            bufp += sizeof(int);
            memcpy(entry.start, bufp, ndim * sizeof(MPI_Offset));
            bufp += ndim * sizeof(MPI_Offset);
            memcpy(entry.count, bufp, ndim * sizeof(MPI_Offset));
            bufp += ndim * sizeof(MPI_Offset);
            entry.ldoff = *((MPI_Offset*)bufp);
            bufp += sizeof(MPI_Offset);
            entry.rsize = *((size_t*)bufp);
            bufp += sizeof(size_t);

            fp->idx[entry.did].push_back(entry);
        }
    }
    else{
        for(i = 0; i < fp->ndset; i++){
            fp->idx[i].clear();
        }
    }
    
    fp->idxvalid = true;

err_out:;

    // Cleanup
    if (mdsid >= 0) H5Sclose(mdsid);
    if (ldsid >= 0) H5Sclose(ldsid);
    if (buf != NULL) delete buf;

    return err;
}