#include <stdio.h>
#include <stdlib.h>

#include <hdf5.h>
#include "H5VL_log.h"
#include "testutils.hpp"

#define N 10

int main(int argc, char **argv) {   
    int err, nerrs = 0;
    int rank, np;
    const char *file_name;  
    hid_t fid;
    hid_t faplid, dxplid;
    hid_t log_vlid;  
  
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &np);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (argc > 2) {
        if (!rank)
            printf("Usage: %s [filename]\n", argv[0]);
        MPI_Finalize();
        return 1;
    }
    else if (argc > 1){
        file_name = argv[1];
    }
    else{
        file_name = "test.h5";
    }
    SHOW_TEST_INFO("Creating files")

    //Register LOG VOL plugin 
    log_vlid = H5VLregister_connector(&H5VL_log_g, H5P_DEFAULT); 

    faplid = H5Pcreate(H5P_FILE_ACCESS); 
    // MPI and collective metadata is required by LOG VOL
    H5Pset_fapl_mpio(faplid, MPI_COMM_WORLD, MPI_INFO_NULL);
    H5Pset_all_coll_metadata_ops(faplid, 1);   
    H5Pset_vol(faplid, log_vlid, NULL);

    // Create file
    fid = H5Fcreate(file_name, H5F_ACC_TRUNC, H5P_DEFAULT, faplid);    CHECK_ERR(fid)
    err = H5Fclose(fid); CHECK_ERR(err)

    // Open file
    //fid = H5Fopen(file_name, H5F_ACC_RDONLY, faplid);    CHECK_ERR(fid)
    //err = H5Fclose(fid); CHECK_ERR(err)

    err = H5Pclose(faplid); CHECK_ERR(err)

err_out:    
    SHOW_TEST_RESULT

    MPI_Finalize();

    return (nerrs > 0);
}  


