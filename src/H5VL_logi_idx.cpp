#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "H5VL_logi_idx.hpp"
#include "H5VL_log_file.hpp"
#include "H5VL_logi_nb.hpp"
#include "H5VL_logi.hpp"
#include <algorithm>
#include <vector>
#include <mpi.h>

static bool intersect(int ndim, hsize_t *sa, hsize_t *ca, hsize_t *sb, hsize_t *cb, hsize_t *so, hsize_t *co){
    int i;

    for(i = 0; i < ndim; i++){
        so[i] = std::max(sa[i], sb[i]);
        co[i] = std::min(sa[i] + ca[i], sb[i] + cb[i]) - so[i];
        if (co[i] <= 0) return false; 
    }

    return true;
}

herr_t H5VL_logi_idx_search(void *file, H5VL_log_rreq_t &req, std::vector<H5VL_log_search_ret_t> &ret){
    herr_t err = 0;
    int j, k;
    size_t soff;
    hsize_t os[H5S_MAX_RANK], oc[H5S_MAX_RANK];
    H5VL_log_search_ret_t cur;
    H5VL_log_file_t *fp=(H5VL_log_file_t *)file;

    soff = 0;
    for(auto sel : req.sels){
        for(auto &ent : fp->idx[req.hdr.did]){
            if(intersect(req.ndim, ent.start, ent.count, sel.start, sel.count, os, oc)){
                for(j = 0; j < req.ndim; j++){
                    cur.fstart[j] = os[j] - ent.start[j];
                    cur.fsize[j] = ent.count[j];
                    cur.mstart[j] = os[j] - sel.start[j];
                    cur.msize[j] = sel.count[j];
                    cur.count[j] = oc[j];
                }
                cur.foff = ent.ldoff;
                cur.moff = (MPI_Offset)req.xbuf + soff;
                cur.ndim = req.ndim;
                cur.esize = req.esize;
                ret.push_back(cur);

            }
        }
        soff += sel.size;
    }

    return err;
}