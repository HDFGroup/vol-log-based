/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 *  This program illustrates the usage of the H5A Interface functions.
 *  It creates and writes a dataset, and then creates and writes array,
 *  scalar, and string attributes of the dataset.
 *  Program reopens the file, attaches to the scalar attribute using
 *  attribute name and reads and displays its value. Then index of the
 *  third attribute is used to read and display attribute values.
 *  The H5Aiterate function is used to iterate through the dataset attributes,
 *  and display their names. The function is also reads and displays the values
 *  of the array attribute.
 */

#include <stdlib.h>

#include "hdf5.h"
#include "H5VL_log.h"

#define H5FILE_NAME_BASE "regular.h5"
#define H5FILE_NAME_USE_LOG "regular_log_vol.h5"

#define RANK 1 /* Rank and size of the dataset  */
#define SIZE 7

#define ARANK  2 /* Rank and dimension sizes of the first dataset attribute */
#define ADIM1  2
#define ADIM2  3
#define ANAME  "Float attribute"     /* Name of the array attribute */
#define ANAMES "Character attribute" /* Name of the string attribute */

#define CHECKERR(A)\
do {    \
    if ((A) != 0) { \
        printf("%s: %d\n\tError check failed: %s\n", __FILE__, __LINE__, #A); \
        goto err_out; \
    } \
} while (0)

static herr_t attr_info(hid_t loc_id, const char *name, const H5A_info_t *ainfo, void *opdata);
/* Operator function */
int helper(int argc, char ** argv, const char *file_name, bool use_log_vol);

int main(int argc, char **argv) {
    herr_t err = 0;
    const char *file_name_base;
    const char *file_name_use_log;
    int rank;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);

    if (argc > 3) {
        if (!rank) printf ("Usage: %s [filename1] [filename2]\n", argv[0]);
        MPI_Finalize ();
        return 1;
    } else if (argc > 2) {
        file_name_base = argv[1];
        file_name_use_log = argv[2];
    } else {
        file_name_base = H5FILE_NAME_BASE;
        file_name_use_log = H5FILE_NAME_USE_LOG;
    }

    
    err = helper(argc, argv, file_name_base, false);
    if (err == 0) {
        err = helper(argc, argv, file_name_use_log, true);
    }
    MPI_Finalize();
    
   
    
    return err;
}

herr_t helper(int argc, char ** argv, const char *file_name, bool use_log_vol)
{

    hid_t file, dataset; /* File and dataset identifiers */

    hid_t       fid;                 /* Dataspace identifier */
    hid_t       attr1, attr2, attr3; /* Attribute identifiers */
    hid_t       attr;
    hid_t       aid1, aid2, aid3; /* Attribute dataspace identifiers */
    hid_t       atype, atype_mem; /* Attribute type */
    H5T_class_t type_class;
    hid_t fapl_id, log_vol_id = 0;

    hsize_t fdim[] = {SIZE};
    hsize_t adim[] = {ADIM1, ADIM2}; /* Dimensions of the first attribute  */

    float matrix[ADIM1][ADIM2]; /* Attribute data */

    herr_t      err;            /* Return value */
    H5O_info2_t oinfo;          /* Object info */
    unsigned    i, j;           /* Counters */
    char        string_out[80]; /* Buffer to read string attribute back */
    int         point_out;      /* Buffer to read scalar attribute back */

    /*
     * Data initialization.
     */
    int  vector[7] = {1, 2, 3, 4, 5, 6, 7}; /* Dataset data */
    int  point    = 1;                     /* Value of the scalar attribute */
    char string[] = "ABCD";                /* Value of the string attribute */

    for (i = 0; i < ADIM1; i++) { /* Values of the array attribute */
        for (j = 0; j < ADIM2; j++)
            matrix[i][j] = -1.;
    }
    
    // Log-based VOL VOL require MPI-IO and parallel access
    // Set MPI-IO and parallel access proterty.
    fapl_id = H5Pcreate(H5P_FILE_ACCESS); 
    err = H5Pset_fapl_mpio(fapl_id, MPI_COMM_WORLD, MPI_INFO_NULL);
    CHECKERR(err);
    err = H5Pset_all_coll_metadata_ops(fapl_id, 1);
    CHECKERR(err);
    err = H5Pset_coll_metadata_write(fapl_id, 1);
    CHECKERR(err);

    /*
     * Create a regular file.
     */
    file = H5Fcreate(file_name, H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id);
    err = H5Fclose(file);
    CHECKERR(err);

    // registering LOG VOL
    if (use_log_vol) {
        log_vol_id = H5VLregister_connector(&H5VL_log_g, H5P_DEFAULT); 
        err = H5Pset_vol(fapl_id, log_vol_id, NULL);
        CHECKERR(err);
    }
    

    // reopen regular file using LOG VOL
    file = H5Fopen(file_name, H5F_ACC_RDWR, fapl_id);

    /*
     * Create the dataspace for the dataset in the file.
     */
    fid = H5Screate(H5S_SIMPLE);
    err = H5Sset_extent_simple(fid, RANK, fdim, NULL);
    CHECKERR(err);
    /*
     * Create the dataset in the file.
     */
    dataset = H5Dcreate2(file, "Dataset", H5T_NATIVE_INT, fid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    /*
     * Write data to the dataset.
     */
    printf("Test Case succeeds till %d\n", __LINE__);
    err = H5Dwrite(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, vector);
    printf("Test Case succeeds till %d\n", __LINE__);
    CHECKERR(err);
    /*
     * Create dataspace for the first attribute.
     */
    aid1 = H5Screate(H5S_SIMPLE);
    err  = H5Sset_extent_simple(aid1, ARANK, adim, NULL);
    CHECKERR(err);
    /*
     * Create array attribute.
     */
    attr1 = H5Acreate2(dataset, ANAME, H5T_NATIVE_FLOAT, aid1, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Write array attribute.
     */
    err = H5Awrite(attr1, H5T_NATIVE_FLOAT, matrix);
    CHECKERR(err);

    /*
     * Create scalar attribute.
     */
    aid2  = H5Screate(H5S_SCALAR);
    attr2 = H5Acreate2(dataset, "Integer attribute", H5T_NATIVE_INT, aid2, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Write scalar attribute.
     */
    err = H5Awrite(attr2, H5T_NATIVE_INT, &point);
    CHECKERR(err);

    /*
     * Create string attribute.
     */
    aid3  = H5Screate(H5S_SCALAR);
    atype = H5Tcopy(H5T_C_S1);
    err = H5Tset_size(atype, 5);
    CHECKERR(err);
    err = H5Tset_strpad(atype, H5T_STR_NULLTERM);
    CHECKERR(err);
    attr3 = H5Acreate2(dataset, ANAMES, atype, aid3, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Write string attribute.
     */
    err = H5Awrite(attr3, atype, string);
    CHECKERR(err);

    /*
     * Close attribute and file dataspaces, and datatype.
     */
    err = H5Sclose(aid1);
    CHECKERR(err);
    err = H5Sclose(aid2);
    CHECKERR(err);
    err = H5Sclose(aid3);
    CHECKERR(err);
    err = H5Sclose(fid);
    CHECKERR(err);
    err = H5Tclose(atype);
    CHECKERR(err);

    /*
     * Close the attributes.
     */
    err = H5Aclose(attr1);
    CHECKERR(err);
    err = H5Aclose(attr2);
    CHECKERR(err);
    err = H5Aclose(attr3);
    CHECKERR(err);

    /*
     * Close the dataset.
     */
    err = H5Dclose(dataset);
    CHECKERR(err);

    /*
     * Close the file.
     */
    err = H5Fclose(file);
    CHECKERR(err);

    /*
     * Reopen the file.
     */
    file = H5Fopen(file_name, H5F_ACC_RDONLY, H5P_DEFAULT);

    /*
     * Open the dataset.
     */
    dataset = H5Dopen2(file, "Dataset", H5P_DEFAULT);

    /*
     * Attach to the scalar attribute using attribute name, then read and
     * display its value.
     */
    attr = H5Aopen(dataset, "Integer attribute", H5P_DEFAULT);
    err  = H5Aread(attr, H5T_NATIVE_INT, &point_out);
    CHECKERR(err);
    printf("The value of the attribute \"Integer attribute\" is %d \n", point_out);
    err = H5Aclose(attr);
    CHECKERR(err);

    //! [H5Oget_info3_snip]

    /*
     * Find string attribute by iterating through all attributes
     */
    err = H5Oget_info3(dataset, &oinfo, H5O_INFO_NUM_ATTRS);
    CHECKERR(err);
    for (i = 0; i < (unsigned)oinfo.num_attrs; i++) {
        attr       = H5Aopen_by_idx(dataset, ".", H5_INDEX_CRT_ORDER, H5_ITER_INC, (hsize_t)i, H5P_DEFAULT,
                              H5P_DEFAULT);
        atype      = H5Aget_type(attr);
        type_class = H5Tget_class(atype);
        if (type_class == H5T_STRING) {
            atype_mem = H5Tget_native_type(atype, H5T_DIR_ASCEND);
            err       = H5Aread(attr, atype_mem, string_out);
            CHECKERR(err);
            printf("Found string attribute; its index is %d , value =   %s \n", i, string_out);
            err = H5Tclose(atype_mem);
            CHECKERR(err);
        }
        err = H5Aclose(attr);
        CHECKERR(err);
        err = H5Tclose(atype);
        CHECKERR(err);
    }

    //! [H5Oget_info3_snip]
    /*
     * Get attribute info using iteration function.
     */
    err = H5Aiterate2(dataset, H5_INDEX_NAME, H5_ITER_INC, NULL, attr_info, NULL);
    CHECKERR(err);

    /*
     * Close the dataset and the file.
     */
    err = H5Dclose(dataset);
    CHECKERR(err);
    err = H5Fclose(file);
    CHECKERR(err);
    err = H5Pclose(fapl_id);
    CHECKERR(err);

err_out:;
    if (log_vol_id > 0) {
        H5VLclose(log_vol_id);
    } 
    return err;
}

/*
 * Operator function.
 */
static herr_t
attr_info(hid_t loc_id, const char *name, const H5A_info_t *ainfo, void *opdata)
{
    hid_t   attr, atype, aspace; /* Attribute, datatype and dataspace identifiers */
    int     rank;
    hsize_t sdim[64];
    herr_t  ret;
    int     i;
    size_t  npoints;     /* Number of elements in the array attribute. */
    float * float_array; /* Pointer to the array attribute. */

    /* avoid warnings */
    (void)opdata;

    /*
     * Open the attribute using its name.
     */
    attr = H5Aopen(loc_id, name, H5P_DEFAULT);

    /*
     * Display attribute name.
     */
    printf("\nName : %s\n", name);

    /*
     * Get attribute datatype, dataspace, rank, and dimensions.
     */
    atype  = H5Aget_type(attr);
    aspace = H5Aget_space(attr);
    rank   = H5Sget_simple_extent_ndims(aspace);
    ret    = H5Sget_simple_extent_dims(aspace, sdim, NULL);

    /*
     *  Display rank and dimension sizes for the array attribute.
     */

    if (rank > 0) {
        printf("Rank : %d \n", rank);
        printf("Dimension sizes : ");
        for (i = 0; i < rank; i++)
            printf("%d ", (int)sdim[i]);
        printf("\n");
    }

    /*
     * Read array attribute and display its type and values.
     */

    if (H5T_FLOAT == H5Tget_class(atype)) {
        printf("Type : FLOAT \n");
        npoints     = H5Sget_simple_extent_npoints(aspace);
        float_array = (float *)malloc(sizeof(float) * (int)npoints);
        ret         = H5Aread(attr, atype, float_array);
        printf("Values : ");
        for (i = 0; i < (int)npoints; i++)
            printf("%f ", float_array[i]);
        printf("\n");
        free(float_array);
    }

    /*
     * Release all identifiers.
     */
    H5Tclose(atype);
    H5Sclose(aspace);
    H5Aclose(attr);

    return 0;
}