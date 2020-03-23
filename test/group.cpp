#include <stdio.h>
#include <stdlib.h>

#include <hdf5.h>
#include "logvol.h"
#include "testutils.hpp"

#define N 10

int main(int argc, char **argv) {   
    int err, nerrs = 0;
    int rank, np;
    const char *file_name;  
    hid_t fid, gid, sgid;
    hid_t faplid, dxplid;
    hid_t log_vlid;  
  
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &np);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    printf("%d\n",rank);

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
    gid = H5Gcreate2(fid, "test", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT); CHECK_ERR(gid)
    // Create sub-group
    sgid = H5Gcreate2(gid, "test2", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT); CHECK_ERR(sgid)
    err = H5Gclose(sgid); CHECK_ERR(err)
    err = H5Gclose(gid); CHECK_ERR(err)

    // Open group
    gid = H5Gopen2(fid, "test", H5P_DEFAULT); CHECK_ERR(gid)
    // Open sub-group
    sgid = H5Gopen2(gid, "test2", H5P_DEFAULT); CHECK_ERR(sgid)
    err = H5Gclose(sgid); CHECK_ERR(err)
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

