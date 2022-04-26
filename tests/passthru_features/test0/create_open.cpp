#include <stdio.h>
#include <stdlib.h>

#include <hdf5.h>
#include "H5VL_log.h"


int main(int argc, char **argv) {  
    int rank, np;
    const char *file_name;  
    hid_t fapl_id, log_vol_id;
    hid_t file_id, dset_id, dspace_id;
    hsize_t dims[1];

    int data[7] = {99, 999, 9999, 99999, 3, 33, 333};
    dims[0] = 7;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &np);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    file_name = "log-based.h5";

    // Log-based VOL VOL require MPI-IO and parallel access
    // Set MPI-IO and parallel access proterty.
    fapl_id = H5Pcreate(H5P_FILE_ACCESS); 
    H5Pset_fapl_mpio(fapl_id, MPI_COMM_WORLD, MPI_INFO_NULL);
    H5Pset_all_coll_metadata_ops(fapl_id, 1);
    H5Pset_coll_metadata_write(fapl_id, 1);

    // Resgiter Log-based VOL VOL
    log_vol_id = H5VLregister_connector(&H5VL_log_g, H5P_DEFAULT); 
    H5Pset_vol(fapl_id, log_vol_id, NULL);

    // Create log based file: log-based.h5
    file_id = H5Fcreate(file_name, H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id);     
    H5Fclose(file_id);

    printf("======================== processing log based file\n");
    // Open file: log-based.h5
    file_id = H5Fopen("log-based.h5", H5F_ACC_RDWR, fapl_id); 
    // // create a dataset "/dset"
    dspace_id = H5Screate_simple(1, dims, dims);
    dset_id = H5Dcreate2(file_id, "/dset", H5T_STD_I32LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Sclose(dspace_id);
    H5Dclose(dset_id);
    // // open the created dataset "/dset"
    dset_id = H5Dopen2(file_id, "/dset", H5P_DEFAULT);
    // // write to "/dset"
    H5Dwrite(dset_id, H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    H5Dclose(dset_id);
    H5Fclose(file_id);

    // htri_t is_accessibile = H5Fis_accessible(file_name, fapl_id);
    // printf("log-based.h5 is_accessible: %d; fapl_id is %d\n", is_accessibile, fapl_id);

    printf("======================== processing regular file\n");
    // Open a regular HDF5 file, repeat the same for regular.h5
    file_id = H5Fopen("regular.h5", H5F_ACC_RDWR, fapl_id);
    dspace_id = H5Screate_simple(1, dims, dims);
    // // create a dataset "/dset", and close
    dset_id = H5Dcreate2(file_id, "/dset", H5T_STD_I32LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Sclose(dspace_id);
    H5Dclose(dset_id);
    // // open the data set again
    dset_id = H5Dopen2(file_id, "/dset", H5P_DEFAULT);
    // // write
    H5Dwrite(dset_id, H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    // // close
    H5Dclose(dset_id); 
    H5Fclose(file_id);


    H5VLclose(log_vol_id);
    H5Pclose(fapl_id);
    MPI_Finalize();
    return 0;  
}  
