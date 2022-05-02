#include <stdio.h>
#include <stdlib.h>

#include <hdf5.h>
#include "H5VL_log.h"

#define DATA_SIZE 7
#define ASSERT(A) \
do {    \
    if (!(A)) { \
        printf("%s: %d\n\tAssertion failed: %s\n", __FILE__, __LINE__, #A); \
        exit(-1); \
    } \
} while (0)

#define CHECKERR(A)\
do {    \
    if ((A) != 0) { \
        printf("%s: %d\n\tError check failed: %s\n", __FILE__, __LINE__, #A); \
        goto err_out; \
    } \
} while (0)

const int org_data[DATA_SIZE] = {5, 7, 11, 13, 17, 19, 23};

void init_data(int* data, int size);
void print_data(int* data, int size);
void reset_data(int* data, int size);
bool cmp_data(int* data, int size);


int main(int argc, char **argv) {  
    const char *log_file_name, *regular_file_name;  
    hid_t fapl_id, log_vol_id = 0;
    hid_t log_file_id, regular_file_id, dset_id, dspace_id;
    hsize_t dims[1];
    herr_t err = 0;

    int data[7];
    dims[0] = DATA_SIZE;
    init_data(data, DATA_SIZE);

    MPI_Init(&argc, &argv);

    log_file_name = "log-based.h5";
    regular_file_name = "regular.h5";

    // Log-based VOL VOL require MPI-IO and parallel access
    // Set MPI-IO and parallel access proterty.
    fapl_id = H5Pcreate(H5P_FILE_ACCESS); 
    err = H5Pset_fapl_mpio(fapl_id, MPI_COMM_WORLD, MPI_INFO_NULL);
    CHECKERR(err);
    err = H5Pset_all_coll_metadata_ops(fapl_id, 1);
    CHECKERR(err);
    err = H5Pset_coll_metadata_write(fapl_id, 1);
    CHECKERR(err);

    // create a regular file and write data to it. (using native VOL)
    regular_file_id = H5Fcreate(regular_file_name, H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id);
    dspace_id = H5Screate_simple(1, dims, dims);
    dset_id = H5Dcreate2(regular_file_id, "/dset", H5T_STD_I32LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    err = H5Dwrite(dset_id, H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    CHECKERR(err);

    err = H5Dclose(dset_id);
    CHECKERR(err);
    err = H5Sclose(dspace_id);
    CHECKERR(err);
    err = H5Fclose(regular_file_id);
    CHECKERR(err);

    // registering LOG VOL
    log_vol_id = H5VLregister_connector(&H5VL_log_g, H5P_DEFAULT); 
    err = H5Pset_vol(fapl_id, log_vol_id, NULL);
    CHECKERR(err);

    //reset data
    reset_data(data, DATA_SIZE);

    // read from regular file using LOG VOL
    regular_file_id = H5Fopen(regular_file_name, H5F_ACC_RDWR, fapl_id);
    dset_id = H5Dopen(regular_file_id, "/dset", H5P_DEFAULT);
    err = H5Dread(dset_id, H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    CHECKERR(err);
    ASSERT(cmp_data(data, DATA_SIZE));
    err = H5Dclose(dset_id);
    CHECKERR(err);
    err = H5Fclose(regular_file_id);
    CHECKERR(err);
    
    // create and write to log based file.
    log_file_id = H5Fcreate(log_file_name, H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id);
    dspace_id = H5Screate_simple(1, dims, dims);
    dset_id = H5Dcreate2(log_file_id, "/dset", H5T_STD_I32LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    err = H5Dwrite(dset_id, H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    CHECKERR(err);
    
    err = H5Dclose(dset_id);
    CHECKERR(err);
    err = H5Sclose(dspace_id);
    CHECKERR(err);
    err = H5Fclose(log_file_id);
    CHECKERR(err);

    //reset data
    reset_data(data, DATA_SIZE);

    // read from log based file:
    log_file_id = H5Fopen(log_file_name, H5F_ACC_RDWR, fapl_id);
    dset_id = H5Dopen(log_file_id, "/dset", H5P_DEFAULT);
    err = H5Dread(dset_id, H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    CHECKERR(err);
    ASSERT(cmp_data(data, DATA_SIZE));

    err = H5Dclose(dset_id);
    CHECKERR(err);
    err = H5Fclose(log_file_id);
    CHECKERR(err);

    err = H5Pclose(fapl_id);
    CHECKERR(err);
err_out:;
    if (log_vol_id > 0) {
        H5VLclose(log_vol_id);
    } 
    // H5VLclose(log_vol_id);
    
    MPI_Finalize();
    return err;  
}  

void init_data(int* data, int size){
    for (int i = 0 ; i < size; i++) {
        data[i] = org_data[i];
    }
}

void print_data(int* data, int size){
    for (int i = 0 ; i < size; i++) {
        printf("%d ", data[i]);
    }
    printf("\n");
}
void reset_data(int* data, int size){
    for (int i = 0 ; i < size; i++) {
        data[i] = 0;
    }
}

bool cmp_data(int* data, int size) {
    for (int i = 0 ; i < size; i++) {
        if (data[i] != org_data[i]) {
            return false;
        }
    }
    return true;
}