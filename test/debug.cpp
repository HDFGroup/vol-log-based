#include <stdio.h>
#include <stdlib.h>

#include <hdf5.h>

#define N 10

int main(int argc, char **argv) {  
    int i, j;  
    int rank, np;
    const char *file_name;  
    char dataset_name[]="data";  
    hid_t file_id, datasetId, dataspaceId, file_space, memspace_id, attid_f, attid_d, attid_g;
    hid_t pnc_fapl, pnc_vol_id, dxplid;  
    hsize_t dims[2], start[2], count[2];
    int buf[100];

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &np);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (argc > 1){
        file_name = argv[1];
    }
    else{
        file_name = "test.hdf";
    }
    if(rank == 0) printf("Writing file_name = %s at rank 0 \n", file_name);

    //Register DataElevator plugin 
    pnc_fapl = H5Pcreate(H5P_FILE_ACCESS); 
    H5Pset_fapl_mpio(pnc_fapl, MPI_COMM_WORLD, MPI_INFO_NULL);
    H5Pset_all_coll_metadata_ops(pnc_fapl, 1);
    H5Pset_coll_metadata_write(pnc_fapl, 1);

    // Create file
    file_id  = H5Fcreate(file_name, H5F_ACC_TRUNC, H5P_DEFAULT, pnc_fapl);     
    
    dims [0]    = 10;
    dims [1]    = 10;
    dims [2]    = 100;
    dataspaceId = H5Screate_simple(2, dims, NULL);   
    datasetId   = H5Dcreate(file_id,dataset_name,H5T_NATIVE_INT,dataspaceId,H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT);  
    memspace_id = H5Screate_simple(1, dims + 2, NULL);   
    file_space = H5Dget_space(datasetId);

    start[0] = 0;
    start[1] = 0;
    count[0] = 10;
    count[1] = 10;
    H5Sselect_hyperslab(file_space, H5S_SELECT_SET, start, NULL, count, NULL);

    dxplid = H5Pcreate (H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(dxplid, H5FD_MPIO_COLLECTIVE);

    for(i = 0; i < N * N; i++){
        buf[i] = i;
    }
    H5Dwrite(datasetId, H5T_NATIVE_INT, memspace_id, file_space, dxplid, buf);  

    for(i = 0; i < N * N; i++){
        buf[i] = 0;
    }

    start[0] = 0;
    start[1] = 0;
    count[0] = 3;
    count[1] = 2;
    H5Sselect_hyperslab(file_space, H5S_SELECT_SET, start, NULL, count, NULL);

    start[0] = 1;
    start[1] = 2;
    count[0] = 3;
    count[1] = 2;
    H5Sselect_hyperslab(file_space, H5S_SELECT_OR , start, NULL, count, NULL);

    start[0] = 0;
    count[0] = 12;
    H5Sselect_hyperslab(memspace_id, H5S_SELECT_SET, start, NULL, count, NULL);

    H5Dread(datasetId, H5T_NATIVE_INT, memspace_id, file_space, dxplid, buf);  

    for(i = 0; i < 12; i++){
        printf("%d, ", buf[i]);
    }
    printf("\n");

    H5Sclose(file_space);
    H5Sclose(memspace_id);
    H5Sclose(dataspaceId); 
    H5Pclose(dxplid);  
    H5Dclose(datasetId);  
    H5Fclose(file_id);
    
    MPI_Finalize();

    return 0;  
}  


