#include "logvol_internal.hpp"
#include <algorithm>
#include <vector>


template <class T>
static bool lessthan(int ndim, T *a, T *b){
    int i;

    for(i = 0; i < ndim; i++){
        if (*a < *b){
            return true;
        }
        else if (*b < *a){
            return false;
        }
        ++a;
        ++b;
    }

    return false;
}

#define ELEM_SWAP_EX(A,B) {tmp = starts[A]; starts[A] = starts[B]; starts[B] = tmp; tmp = counts[A]; counts[A] = counts[B]; counts[B] = tmp;}
template <class T>
static void sortvec_ex(int ndim, int len, T **starts, T **counts){
    int i, j, p;
    T *tmp;

    if (len < 16){
        j = 1;
        while(j){
            j = 0;
            for(i = 0; i < len - 1; i++){
                if (lessthan(ndim, starts[i + 1], starts[i])){
                    ELEM_SWAP_EX(i, i + 1)
                    j = 1;
                }
            }
        }
        return;
    }

    p = len / 2;
    ELEM_SWAP_EX(p, len - 1);
    p = len - 1;

    j = 0;
    for(i = 0; i < len; i++){
        if (lessthan(ndim, starts[i], starts[p])){
            if (i != j){
                ELEM_SWAP_EX(i, j)
            }
            j++;
        }
    }

    ELEM_SWAP_EX(p, j)

    sortvec_ex(ndim, j, starts, counts);
    sortvec_ex(ndim, len - j - 1, starts + j + 1, counts + j + 1);
}

herr_t H5VL_logi_get_selection(hid_t sid, int &n, MPI_Offset **&starts, MPI_Offset **&counts){
    herr_t err = 0;
    int i, j, k, l;
    int ndim;
    int nreq, old_nreq, nbreq;
    hssize_t nblock;
    H5S_sel_type stype;
    hsize_t **hstarts, **hends;
    int *group;

    ndim = H5Sget_simple_extent_ndims(sid); CHECK_ID(ndim)

    // Get selection type
    if (sid == H5S_ALL){
        stype = H5S_SEL_ALL;
    }
    else{
        stype =  H5Sget_select_type(sid);
    }

    switch (stype){
        case H5S_SEL_HYPERSLABS:
            {
                nblock = H5Sget_select_hyper_nblocks(sid); CHECK_ID(nblock)

                hstarts = new hsize_t*[nblock * 2];
                hends = hstarts + nblock;

                hstarts[0] = new hsize_t[ndim * 2 * nblock];
                hends[0] = hstarts[0] + ndim;
                for(i = 1; i < nblock; i++){
                    hstarts[i] = hstarts[i - 1] + ndim * 2;
                    hends[i] = hends[i - 1] + ndim * 2;
                }

                err = H5Sget_select_hyper_blocklist(sid, 0, nblock, hstarts[0]);

                sortvec_ex(ndim, nblock, hstarts, hends);

                // Check for interleving
                group = new int[nblock + 1];
                group[0] = 0;
                for(i = 0; i < nblock - 1; i++){
                    if (lessthan(ndim, hends[i], hstarts[i + 1])){  // The end cord is the max offset of the previous block. If less than the start offset of the next block, there won't be interleving
                        group[i + 1] = group[i] + 1;
                    }
                    else{
                        group[i + 1] = group[i];
                    }
                }
                group[i + 1] = group[i] + 1; // Dummy group to trigger process for final group

                j = 0;
                nreq = 0;
                for(i = j = 0; i <= nblock; i++){
                    if (group[i] != group[j]){  // Within 1 group
                        if (i - j == 1){    // Sinlge block, no need to breakdown
                            //offs[k] = nreq; // Record offset
                            nreq++;
                            j++;
                        }
                        else{
                            for(;j < i; j++){
                                //offs[i] = nreq; // Record offset
                                nbreq = 1;
                                for(k = 0; k < ndim - 1; k++){
                                    nbreq *= hends[j][k] - hstarts[j][k] + 1;
                                }
                                nreq += nbreq;
                            }
                        }
                    }
                }

                starts = new MPI_Offset*[nreq * 2];
                counts = starts + nreq;
                starts[0] = new MPI_Offset[ndim * 2 * nreq];
                counts[0] = starts[0] + ndim;
                for(i = 1; i < nblock; i++){
                    starts[i] = starts[i - 1] + ndim * 2;
                    counts[i] = counts[i - 1] + ndim * 2;
                }
                nreq = 0;
                for(i = j = 0; i <= nblock; i++){
                    if (group[i] != group[j]){  // Within 1 group
                        if (i - j == 1){    // Sinlge block, no need to breakdown
                            for(k = 0; k < ndim; k++){
                                starts[nreq][k] =  (MPI_Offset)hstarts[j][k];
                                counts[nreq][k] =  (MPI_Offset)(hends[j][k] - hstarts[j][k] + 1);
                            }
                            //offs[k] = nreq; // Record offset
                            nreq++;
                            j++;
                        }
                        else{
                            old_nreq = nreq;
                            for(;j < i; j++){    // Breakdown each block
                                for(k = 0; k < ndim; k++){
                                    starts[nreq][k] =  (MPI_Offset)hstarts[j][k];
                                    counts[nreq][k] =  1;
                                }
                                counts[nreq][ndim - 1] =  (MPI_Offset)(hends[j][ndim - 1] - hstarts[j][ndim - 1] + 1);

                                for(l = 0; l < ndim; l++){
                                    if (starts[nreq][l] < (MPI_Offset)hends[j][l]) break;
                                }
                                nreq++;
                                while(l < ndim - 1){    // For each row
                                    memcpy(starts[nreq], starts[nreq - 1], sizeof(MPI_Offset));
                                    starts[nreq][ndim - 2]++;
                                    for(k = ndim - 2; k > 0; k--){
                                        if (starts[nreq][k] > (MPI_Offset)(hends[j][k])){
                                            starts[nreq][k] = (MPI_Offset)(hstarts[j][k]);
                                            starts[nreq][k - 1]++;
                                        }
                                        else{
                                            break;
                                        }
                                    }
                                    for(l = 0; l < ndim; l++){
                                        if (starts[nreq][l] < (MPI_Offset)hends[j][l]) break;
                                    }
                                    nreq++;
                                }
                            }

                            sortvec_ex(ndim, nreq - old_nreq, starts + old_nreq, counts + old_nreq);
                        }
                    }
                }

                n = nreq;
            }
            break;
        case H5S_SEL_POINTS:
            {
                nblock = H5Sget_select_elem_npoints(sid); CHECK_ID(nblock)

                if (nblock){
                    hstarts = new hsize_t*[nblock];
                    hstarts[0] = new hsize_t[ndim * nblock];
                    for(i = 1; i < nblock; i++){
                        hstarts[i] = hstarts[i - 1] + ndim;
                    }

                    err = H5Sget_select_elem_pointlist(sid, 0, nblock, hstarts[0]); CHECK_ERR

                    starts = new MPI_Offset*[nblock * 2];
                    counts = starts + nblock;
                    starts[0] = new MPI_Offset[ndim * 2 * nblock];
                    counts[0] = starts[0] + ndim;
                    for(i = 1; i < nblock; i++){
                        starts[i] = starts[i - 1] + ndim * 2;
                        counts[i] = counts[i - 1] + ndim * 2;
                    }

                    for(i = 0; i < nblock; i++){
                        for(j = 0; j < ndim; j++){
                            starts[i][j] = (MPI_Offset)hstarts[i][j];
                            counts[i][j] = 1;
                        }
                    }
                }

                n = nblock;
            }
            break;
        case H5S_SEL_ALL:
            {
                starts = new MPI_Offset*[2];
                counts = starts + 1;
                starts[0] = new MPI_Offset[ndim * 2];
                counts[0] = starts[0] + ndim;

                n = 1;
            }
            break;
        default:
            RET_ERR("Unsupported selection type");
    }

err_out:;
    if (err != 0){
        if (starts != NULL){
            H5VL_log_delete(starts[0]);
            delete starts;
            starts = NULL;
            counts = NULL;
        }

        n = 0;
    }

    return err;
}

static bool intersect(int ndim, MPI_Offset *sa, MPI_Offset *ca, MPI_Offset *sb, MPI_Offset *cb, MPI_Offset *so, MPI_Offset *co){
    int i;

    for(i = 0; i < ndim; i++){
        so[i] = std::max(sa[i], sb[i]);
        co[i] = std::min(sa[i] + ca[i], sb[i] + cb[i]) - so[i];
        if (co[i] <= 0) false; 
    }

    return true;
}

herr_t H5VL_logi_idx_search(H5VL_log_file_t *fp, H5VL_log_dset_t *dp, int n, MPI_Offset **starts, MPI_Offset **counts, std::vector<H5VL_log_search_ret_t> &ret){
    herr_t err = 0;
    int i, j, k;
    MPI_Offset os[32], oc[32];
    MPI_Offset moff = 0, rsize;
    H5VL_log_search_ret_t cur;

    for(i = 0; i < n; i++){
        rsize = (MPI_Offset)dp->esize;
        for(j = 0; j < dp->ndim; j++){
            rsize *= counts[i][j];
        }
        for(auto &ent : fp->idx[dp->id]){
            if(intersect(dp->ndim, ent.start, ent.count, starts[i], counts[i], os, oc)){
                for(j = 0; j < dp->ndim; j++){
                    cur.fstart[j] = os[j] - ent.start[j];
                    cur.fsize[j] = ent.count[j];
                    cur.mstart[j] = os[j] - starts[i][j];
                    cur.msize[j] = counts[i][j];
                    cur.count[j] = oc[j];
                }
                cur.foff = ent.ldoff;
                cur.moff = moff;
                cur.ndim = dp->ndim;
                ret.push_back(cur);
            }
        }
        moff += rsize;
    }

    return err;
}

static bool interleve(int ndim, int *sa, int *ca, int *sb){
    int i;

    for(i = 0; i < ndim; i++){
        if (sa[i] + ca[i] < sb[i]){
            return false;
        }
        else if (sa[i] + ca[i] > sb[i]){
            return true;
        }
    }

    return true;
}

#define OSWAP(A,B) {to = oa[A]; oa[A] = oa[B]; oa[B] = to; to = ob[A]; ob[A] = ob[B]; ob[B] = to; tl = l[A]; l[A] = l[B]; l[B] = tl;}
void sortoffsets(int len, MPI_Aint *oa, MPI_Aint *ob, int *l){
    int i, j, p;
    MPI_Aint to;
    int tl;

    if (len < 16){
        j = 1;
        while(j){
            j = 0;
            for(i = 0; i < len - 1; i++){
                if (oa[i + 1] < oa[i]){
                    OSWAP(i, i + 1)
                    j = 1;
                }
            }
        }
        return;
    }

    p = len / 2;
    OSWAP(p, len - 1);
    p = len - 1;

    j = 0;
    for(i = 0; i < len; i++){
        if (oa[i] < oa[p]){
            if (i != j){
                OSWAP(i, j)
            }
            j++;
        }
    }

    OSWAP(p, j)

    sortoffsets(j, oa, ob, l);
    sortoffsets(len - j - 1, oa + j + 1, ob + j + 1, l + j + 1);
}
// Assume no overlaping read
herr_t H5VL_logi_generate_dtype(H5VL_log_dset_t *dp, std::vector<H5VL_log_search_ret_t> blocks, MPI_Datatype *ftype, MPI_Datatype *mtype){
    herr_t err = 0;
    int mpierr;
    int i, j, k;
    int nblock = blocks.size();
    std::vector<bool> newgroup(nblock, 0);
    int nt, nrow, old_nt;
    int *lens;
    MPI_Aint *foffs = NULL, *moffs = NULL;
    MPI_Datatype *ftypes = NULL, *mtypes = NULL, etype = MPI_DATATYPE_NULL;
    MPI_Offset fssize[32], mssize[32];
    MPI_Offset ctr[32];

    if (!nblock){
        *ftype = *mtype = MPI_DATATYPE_NULL;
        return 0;
    }

    mpierr = MPI_Type_contiguous(dp->esize, MPI_BYTE, &etype); CHECK_MPIERR
    mpierr = MPI_Type_commit(&etype); CHECK_MPIERR

    std::sort(blocks.begin(), blocks.end());

    for(i = 0; i < nblock - 1; i++){
        if ((blocks[i].foff == blocks[i + 1].foff) && interleve(dp->ndim, blocks[i].fstart, blocks[i].count, blocks[i + 1].fstart)){
            newgroup[i] = false;
        }
        else{
            newgroup[i] = true;
        }
    }
    newgroup[nblock - 1] = true;

    // Count total types after breakdown
    nt = 0;
    for(i = j = 0; i < nblock; i++){
        if (newgroup[i]){
            if (i == j){ // only 1
                nt++;
                j++;
            }
            else{
                for(; j <= i; j++){ // Breakdown
                    nrow = 1;
                    for(j = 0; j < dp->ndim - 1; j++){
                        nrow *= blocks[i].count[j];
                    }
                    nt += nrow;
                }
            }
        }
    }

    lens = new int[nt];
    foffs = new MPI_Aint[nt];
    moffs = new MPI_Aint[nt];
    ftypes = new MPI_Datatype[nt];
    mtypes = new MPI_Datatype[nt];

    // Construct the actual type
    nt = 0;
    for(i = j = 0; i < nblock; i++){
        if (newgroup[i]){
            if (i == j){ // only 1
                mpierr = MPI_Type_create_subarray(dp->ndim, blocks[j].fsize, blocks[j].count, blocks[j].fstart, MPI_ORDER_C, etype, ftypes + nt); CHECK_MPIERR
                mpierr = MPI_Type_create_subarray(dp->ndim, blocks[j].msize, blocks[j].count, blocks[j].mstart, MPI_ORDER_C, etype, mtypes + nt); CHECK_MPIERR
                mpierr = MPI_Type_commit(ftypes + nt); CHECK_MPIERR
                mpierr = MPI_Type_commit(mtypes + nt); CHECK_MPIERR
                foffs[nt] = blocks[j].foff;
                moffs[nt] = blocks[j].moff;
                lens[nt] = 1;
                j++;
                nt++;
            }
            else{
                old_nt = nt;
                for(; j <= i; j++){ // Breakdown each intersecting blocks
                    fssize[dp->ndim - 1] = 1;
                    mssize[dp->ndim - 1] = 1;
                    for(k = dp->ndim - 2; k > -1; k--){
                        fssize[k] = fssize[k + 1] * blocks[j].fsize[k + 1];
                        mssize[k] = mssize[k + 1] * blocks[j].msize[k + 1];
                    }
                    
                    memset(ctr, 0, sizeof(MPI_Offset) * dp->ndim);
                    while(ctr[0] < blocks[j].count[0]){ // Foreach row
                        lens[nt] = blocks[j].count[dp->ndim - 1] * dp->esize;
                        foffs[nt] = blocks[j].foff;
                        moffs[nt] = blocks[j].moff;
                        for(k = 0; k < dp->ndim; k++){  // Calculate offset
                            foffs[nt] += fssize[k] * (blocks[j].fstart[k] + ctr[k]);
                            moffs[nt] += mssize[k] * (blocks[j].mstart[k] + ctr[k]);
                        }
                        ftypes[nt] = MPI_BYTE;
                        mtypes[nt] = MPI_BYTE;
                        nt++;

                        ctr[dp->ndim - 2]++;    // Move to next position
                        for(k = dp->ndim - 2; k > 0; k--){
                            if (ctr[k] >= blocks[j].count[k]){
                                ctr[k] = 0;
                                ctr[k - 1]++;
                            }
                        }
                    }
                }
                
                // Sort into order
                sortoffsets(nt - old_nt, foffs + old_nt, moffs + old_nt, lens + old_nt);

                // Should there be overlapping read, we have to merge them as well
            }
        }
    }

    mpierr = MPI_Type_struct(nt, lens, foffs, ftypes, ftype); CHECK_MPIERR
    mpierr = MPI_Type_struct(nt, lens, moffs, mtypes, mtype); CHECK_MPIERR

err_out:

    if (ftypes != NULL){
        for(i = 0; i < nt; i++){
            if (ftypes[i] != MPI_BYTE){
                MPI_Type_free(ftypes + i);
            }
        }
        delete ftypes;
    }

    if (mtypes != NULL){
        for(i = 0; i < nt; i++){
            if (mtypes[i] != MPI_BYTE){
                MPI_Type_free(mtypes + i);
            }
        }
        delete mtypes;
    }

    if (etype != MPI_DATATYPE_NULL){
        MPI_Type_free(&etype);
    }

    H5VL_log_delete(foffs)
    H5VL_log_delete(moffs)
    H5VL_log_delete(lens)

    return err;
}