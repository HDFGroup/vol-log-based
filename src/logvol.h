#ifndef _ncmpi_VOL_H_
#define _ncmpi_VOL_H_

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Purpose: This is a "pass through" VOL connector, which forwards each
 *      VOL callback to an underlying connector.
 *
 *      It is designed as an example VOL connector for developers to
 *      use when creating new connectors, especially connectors that
 *      are outside of the HDF5 library.  As such, it should _NOT_
 *      include _any_ private HDF5 header files.  This connector should
 *      therefore only make public HDF5 API calls and use standard C /
 *              POSIX calls.
 */

/* Header files needed */
/* (Public HDF5 and standard C / POSIX only) */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include "hdf5.h"
#include "pnetcdf.h"

/* Identifier for the pass-through VOL connector */
#define H5VL_NCMPI  (H5VL_ncmpi_register())
#ifdef __cplusplus
extern "C" {
#endif
H5_DLL hid_t H5VL_ncmpi_register(void);
#ifdef __cplusplus
}
#endif

#define CHECK_ERR { \
    if (err != NC_NOERR) { \
        printf("Error at line %d in %s: (%s)\n", \
        __LINE__,__FILE__,ncmpi_strerrno(err)); \
        return -1; \
    } \
}

#define CHECK_ERRN { \
    if (err != NC_NOERR) { \
        printf("Error at line %d in %s: (%s)\n", \
        __LINE__,__FILE__,ncmpi_strerrno(err)); \
        return NULL; \
    } \
}

#define CHECK_ERRJ { \
    if (err != NC_NOERR) { \
        printf("Error at line %d in %s: (%s)\n", \
        __LINE__,__FILE__,ncmpi_strerrno(err)); \
        goto errout; \
    } \
}

#define CHECK_HERR { \
    if (herr < 0) { \
        printf("Error at line %d in %s:\n", \
        __LINE__,__FILE__); \
        H5Eprint1(stdout); \
        return -1; \
    } \
}

#define CHECK_HERRN { \
    if (herr < 0) { \
        printf("Error at line %d in %s:\n", \
        __LINE__,__FILE__); \
        H5Eprint1(stdout); \
        return NULL; \
    } \
}

#define RET_ERR(A) { \
    printf("Error at line %d in %s: %s\n", \
    __LINE__,__FILE__, A); \
    return -1; \
}

#define RET_ERRN(A) { \
    printf("Error at line %d in %s: %s\n", \
    __LINE__,__FILE__, A); \
    return NULL; \
}

#define PNC_VOL_DATA_MODE 0x1
#define PNC_VOL_INDEP_MODE 0x2

/************/
/* Typedefs */
/************/

/* Pass-through VOL connector info */
typedef struct H5VL_ncmpi_info_t {
    MPI_Comm comm;
} H5VL_ncmpi_info_t;

/* The pass through VOL info object */
typedef struct H5VL_ncmpi_file_t {
    int objtype;

    hid_t fcpl_id;
    hid_t fapl_id;
    hid_t dxpl_id;

    char *path;

    unsigned int flags;

    int rank;
    int ncid;
} H5VL_ncmpi_file_t;

/* The pass through VOL info object */
typedef struct H5VL_ncmpi_group_t {
    int objtype;

    hid_t lcpl_id;
    hid_t gcpl_id;
    hid_t gapl_id;
    hid_t dxpl_id;

    char *path;
    char *name;

    struct H5VL_ncmpi_group_t *gp;
    H5VL_ncmpi_file_t *fp;
} H5VL_ncmpi_group_t;

/* The pass through VOL info object */
typedef struct H5VL_ncmpi_dataset_t {
    int objtype;

    hid_t dcpl_id;
    hid_t dapl_id;
    hid_t dxpl_id;

    int varid;
    int ndim;
    int *dimids;

    char *path;
    char *name;

    H5VL_ncmpi_group_t *gp;
    H5VL_ncmpi_file_t *fp;
} H5VL_ncmpi_dataset_t;

/* The pass through VOL info object */
typedef struct H5VL_ncmpi_attr_t {
    int objtype;
    
    hid_t acpl_id;
    hid_t aapl_id;
    hid_t dxpl_id;

    int attid;
    int varid;
    nc_type type;
    MPI_Offset size;

    char *path;
    char *name;

    H5VL_ncmpi_dataset_t *dp;
    H5VL_ncmpi_group_t *gp;
    H5VL_ncmpi_file_t *fp;
} H5VL_ncmpi_attr_t;

extern MPI_Datatype h5t_to_mpi_type(hid_t type_id);
extern nc_type h5t_to_nc_type(hid_t type_id);
extern hid_t nc_to_h5t_type(nc_type type_id);
extern int nc_type_size(nc_type type_id);
extern void sortreq(int ndim, hssize_t len, MPI_Offset **starts, MPI_Offset **counts);
extern int intersect(int ndim, MPI_Offset *sa, MPI_Offset *ca, MPI_Offset *sb);
extern void mergereq(int ndim, hssize_t *len, MPI_Offset **starts, MPI_Offset **counts);
extern void sortblock(int ndim, hssize_t len, hsize_t **starts);
extern bool hlessthan(int ndim, hsize_t *a, hsize_t *b);

extern int enter_data_mode(H5VL_ncmpi_file_t *fp);
extern int enter_define_mode(H5VL_ncmpi_file_t *fp);
extern int enter_indep_mode(H5VL_ncmpi_file_t *fp);
extern int enter_coll_mode(H5VL_ncmpi_file_t *fp);

extern const H5VL_file_class_t H5VL_ncmpi_file_g;
extern const H5VL_dataset_class_t H5VL_ncmpi_dataset_g;
extern const H5VL_attr_class_t H5VL_ncmpi_attr_g;
extern const H5VL_group_class_t H5VL_ncmpi_group_g;
extern const H5VL_class_t H5VL_ncmpi_g;

#endif