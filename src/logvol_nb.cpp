#include "logvol_internal.hpp"

herr_t H5VL_log_nb_flush_read_reqs(H5VL_log_file_t *fp, std::vector<H5VL_log_rreq_t> reqs, hid_t dxplid){
    herr_t err = 0;
    int i, j;
    int n;
    int nb;
    MPI_Offset **starts, **counts;
    int mpierr;
    MPI_Datatype ftype, mtype;
    std::vector<H5VL_log_search_ret_t> intersections;
    std::vector<H5VL_log_copy_ctx> overlaps;
    MPI_Status stat;

    if (!(fp->idxvalid)){
        err = H5VL_log_filei_metaupdate(fp); CHECK_ERR
    }

    for(auto &r: reqs){
        H5VL_log_dataset_readi_idx_search(fp, r.did, r.ndim, r.esize, r.xbuf, r.start, r.count, intersections);
    }

    if (intersections.size() > 0){
        err = H5VL_log_dataset_readi_gen_rtypes(intersections, &ftype, &mtype, overlaps); CHECK_ERR
        mpierr = MPI_Type_commit(&mtype); CHECK_MPIERR
        mpierr = MPI_Type_commit(&ftype); CHECK_MPIERR

        mpierr = MPI_File_set_view(fp->fh, 0, MPI_BYTE, ftype, "native", MPI_INFO_NULL); CHECK_MPIERR

        mpierr = MPI_File_read_all(fp->fh, MPI_BOTTOM, 1, mtype, &stat); CHECK_MPIERR
    }
    else{
        mpierr = MPI_File_set_view(fp->fh, 0, MPI_BYTE, MPI_DATATYPE_NULL, "native", MPI_INFO_NULL); CHECK_MPIERR

        mpierr = MPI_File_read_all(fp->fh, MPI_BOTTOM, 0, MPI_DATATYPE_NULL, &stat); CHECK_MPIERR
    }

    for(auto &o: overlaps){
        memcpy(o.dst, o.src, o.size);
    }

    // Type convertion
    for(auto &r: fp->rreqs){
        if (r.dtype != r.mtype){
            err = H5Tconvert(r.dtype, r.mtype, r.rsize, r.xbuf, NULL, dxplid); CHECK_ERR
            
            if (r.xbuf != r.ibuf){
                size_t esize;

                esize = H5Tget_size(r.mtype); CHECK_ID(esize)
                memcpy(r.ibuf, r.xbuf, r.rsize * esize);
            
                H5VL_log_filei_bfree(fp, (void**)(&(r.xbuf)));
            }
        }
    }

err_out:;

    return err;
}

herr_t H5VL_log_nb_flush_write_reqs(H5VL_log_file_t *fp, hid_t dxplid){
    herr_t err;
    int mpierr;
    int i;
    int cnt;
    int *mlens = NULL;
    MPI_Aint *moffs = NULL;
    MPI_Datatype mtype = MPI_DATATYPE_NULL;
    MPI_Status stat;
    MPI_Offset fsize_local, fsize_all, foff;
    void *ldp;
    hid_t ldsid = -1;
    hsize_t dsize;
    haddr_t doff;
    H5VL_loc_params_t loc;
    char dname[16];

    cnt = fp->wreqs.size() - fp->nflushed;

    // Construct memory type
    mlens = (int*)malloc(sizeof(int) * cnt);
    moffs = (MPI_Aint*)malloc(sizeof(MPI_Aint) * cnt);
    fsize_local = 0;
    for(i = fp->nflushed; i < fp->wreqs.size(); i++){
        moffs[i - fp->nflushed] = (MPI_Aint)fp->wreqs[i].xbuf;
        mlens[i - fp->nflushed] = fp->wreqs[i].rsize;
        fp->wreqs[i].ldoff = fsize_local;
        fsize_local += fp->wreqs[i].rsize;
    }
    mpierr = MPI_Type_hindexed(cnt, mlens, moffs, MPI_BYTE, &mtype); CHECK_MPIERR
    mpierr = MPI_Type_commit(&mtype); CHECK_MPIERR

    // Get file offset and total size
    mpierr = MPI_Allreduce(&fsize_local, &fsize_all, 1, MPI_LONG_LONG, MPI_SUM, fp->comm); CHECK_MPIERR
    // NOTE: Some MPI implementation do not produce output for rank 0, foff must ne initialized to 0
    foff = 0;
    mpierr = MPI_Exscan(&fsize_local, &foff, 1, MPI_LONG_LONG, MPI_SUM, fp->comm); CHECK_MPIERR

    // Create log dataset
    dsize = (hsize_t)fsize_all;
    ldsid = H5Screate_simple(1, &dsize, &dsize); CHECK_ID(ldsid)
    sprintf(dname, "_ld_%d", fp->nldset);
    loc.obj_type = H5I_GROUP;
    loc.type = H5VL_OBJECT_BY_SELF;
    ldp = H5VLdataset_create(fp->lgp, &loc, fp->uvlid, dname, H5P_LINK_CREATE_DEFAULT, H5T_STD_B8LE, ldsid, H5P_DATASET_CREATE_DEFAULT, H5P_DATASET_ACCESS_DEFAULT, dxplid, NULL); CHECK_NERR(ldp);
    err = H5VLdataset_optional_wrapper(ldp, fp->uvlid, H5VL_NATIVE_DATASET_GET_OFFSET, dxplid, NULL, &doff); CHECK_ERR // Get dataset file offset
    err = H5VLdataset_close(ldp, fp->uvlid, dxplid, NULL); CHECK_ERR // Close the dataset

    // Write the data
    foff += doff;
    mpierr = MPI_File_write_at_all(fp->fh, foff, MPI_BOTTOM, 1, mtype, &stat); CHECK_MPIERR

    // Update metadata
    for(i = fp->nflushed; i < fp->wreqs.size(); i++){
        fp->wreqs[i].ldid = fp->nldset;
        fp->wreqs[i].ldoff += foff;
        if (fp->wreqs[i].xbuf != fp->wreqs[i].ubuf){
            H5VL_log_filei_bfree(fp, (void*)(fp->wreqs[i].xbuf));
        }
    }
    (fp->nldset)++;
    fp->nflushed = fp->wreqs.size();

    if (fsize_all){
        fp->metadirty = true;
    }
    
err_out:
    // Cleanup
    if (mtype != MPI_DATATYPE_NULL) MPI_Type_free(&mtype);
    H5VL_log_Sclose(ldsid);  

    H5VL_log_free(mlens);
    H5VL_log_free(moffs);
    
    return err;
}