#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <hdf5.h>
#include <mpi.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "h5replay.hpp"
#include "h5replay_data.hpp"
#include "h5replay_meta.hpp"

herr_t h5replay_read_data (int rank, int np,std::vector<dset_info> &dsets, std::vector<req_block> &reqs) {
	herr_t err = 0;
	int i, j, k, l;
    MPI_Aint *offs;
    int *lens;
    size_t bsize;   // Data buffer size
    size_t esize; // Size of a block
    size_t min_bsize;   // Min data buffer size to store the compressed data (can be larger)
    size_t zbsize;  // Size of the decompression buffer
    size_t prev_bsize;
    char *buf;
    char *zbuf;
    std::vector<MPI_Aint> foffs, moffs;
    std::vector<int> lens;

    // Calculate memory buffer size for data
    bsize=0;
    zbsize=0;
    prev_bsize=0;
    for(auto &req:reqs){
        if(req.fsize>0){
            min_bsize=bsize+req.fsize;
            if(zbsize<bsize-prev_bsize){
                zbsize=bsize-prev_bsize;
            }
            prev_bsize=bsize;
        }

        esize=dsets[req.did].esize;
        for(i=0;i<dsets[req.did].ndim;i++){
esize*=req.count[i];
        }
        bsize+=esize;
    }

    

    for(auto &req:reqs){
        if(req.fsize>0){
            foffs.push_back(req.foff);
            moffs.push_back(req.buf);
            lens.push_back(req.fsize);            
        }
        else{

        }
    }
    
    err_out:;
    return err;
}