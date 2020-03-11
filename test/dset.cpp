#include <stdio.h>
#include <stdlib.h>

#include <hdf5.h>
#include "logvol.h"
#include "testutils.hpp"

#define N 10
#define M 10

int main(int argc, char **argv) {   
    int err, nerrs = 0;
    int rank, np;
    const char *file_name;  
    int ndim;
    hid_t fid, gid, did, gdid, sid;
    hid_t faplid, dxplid;
    hid_t log_vlid;  
    hsize_t dims[2] = {N, M};
    hsize_t mdims[2];
  
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &np);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (argc > 1){
        file_name = argv[1];
    }
    else{
        file_name = "test.h5";
    }
    // if(rank == 0) printf("Writing file_name = %s at rank 0 \n", file_name);

    //Register LOG VOL plugin 
    log_vlid = H5VLregister_connector(&H5VL_log_g, H5P_DEFAULT); 

    faplid = H5Pcreate(H5P_FILE_ACCESS); 
    // MPI and collective metadata is required by LOG VOL
    H5Pset_fapl_mpio(faplid, MPI_COMM_WORLD, MPI_INFO_NULL);
    H5Pset_all_coll_metadata_ops(faplid, 1);   
    H5Pset_vol(faplid, log_vlid, NULL);

    // Create file
    fid = H5Fcreate(file_name, H5F_ACC_TRUNC, H5P_DEFAULT, faplid);    CHECK_ERR(fid)
    // Create group
    gid = H5Gcreate2(fid, "G", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT); CHECK_ERR(gid)

    // Create datasets
    sid = H5Screate_simple(2, dims, dims); CHECK_ERR(sid);
    did = H5Dcreate2(fid, "D", H5T_STD_I32LE, sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT); CHECK_ERR(did)
    gdid = H5Dcreate2(gid, "D", H5T_STD_I32LE, sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT); CHECK_ERR(gdid)
    err = H5Dclose(did); CHECK_ERR(err)
    err = H5Dclose(gdid); CHECK_ERR(err)
    err = H5Sclose(sid); CHECK_ERR(err)

    // Open datasets
    did = H5Dopen2(fid, "D", H5P_DEFAULT); CHECK_ERR(did)
    gdid = H5Dopen2(gid, "D", H5P_DEFAULT); CHECK_ERR(gdid)

    // Get dataspace
    sid = H5Dget_space(did); CHECK_ERR(sid)
    ndim = H5Sget_simple_extent_dims(sid, dims, mdims); CHECK_ERR(ndim);
    if (ndim != 2) {
        printf("Error at line %d in %s: expect ndim = %d, but got %d\n",
               __LINE__,__FILE__, 2, ndim);
        nerrs++;
    }
    if (dims[0] != N) {
        printf("Error at line %d in %s: expect dims[0] = %llu, but got %llu\n",
               __LINE__,__FILE__, N, dims[0]);
        nerrs++;
    }
    if (dims[1] != N) {
        printf("Error at line %d in %s: expect dims[1] = %llu, but got %llu\n",
               __LINE__,__FILE__, M, dims[1]);
        nerrs++;
    }
    if (mdims[0] != N) {
        printf("Error at line %d in %s: expect mdims[0] = %llu, but got %llu\n",
               __LINE__,__FILE__, N, mdims[0]);
        nerrs++;
    }
    if (mdims[1] != N) {
        printf("Error at line %d in %s: expect mdims[1] = %llu, but got %llu\n",
               __LINE__,__FILE__, M, mdims[1]);
        nerrs++;
    }
    err = H5Sclose(sid); CHECK_ERR(err)

    err = H5Dclose(did); CHECK_ERR(err)
    err = H5Dclose(gdid); CHECK_ERR(err)

    err = H5Gclose(gid); CHECK_ERR(err)
    err = H5Fclose(fid); CHECK_ERR(err)

    err = H5Pclose(faplid); CHECK_ERR(err)

err_out:  
    MPI_Allreduce(MPI_IN_PLACE, &nerrs, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    if (rank == 0) {
        if (nerrs) printf(FAIL_STR,nerrs);
        else       printf(PASS_STR);
    }

    MPI_Finalize();

    return (nerrs > 0);
}  


