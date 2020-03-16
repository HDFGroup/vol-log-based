#pragma once
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <hdf5.h>
#include <H5VLpublic.h>
#include "logvol.h"

#define LOG_GROUP_NAME "_LOG"

#define CHECK_ERR { \
    if (err < 0) { \
        printf("Error at line %d in %s:\n", \
        __LINE__,__FILE__); \
        H5Eprint1(stdout); \
        goto err_out; \
    } \
}

#define CHECK_ID(A) { \
    if (A < 0) { \
        printf("Error at line %d in %s:\n", \
        __LINE__,__FILE__); \
        H5Eprint1(stdout); \
        goto err_out; \
    } \
}

#define CHECK_NERR(A) { \
    if (A == NULL) { \
        printf("Error at line %d in %s:\n", \
        __LINE__,__FILE__); \
        H5Eprint1(stdout); \
        goto err_out; \
    } \
}

#define RET_ERR(A) { \
    printf("Error at line %d in %s: %s\n", \
    __LINE__,__FILE__, A); \
    goto err_out; \
}

typedef struct H5VL_log_obj_t {
    int type;
    void *uo;   // Under obj
    hid_t uvlid; // Under VolID
} H5VL_log_obj_t;

/* The log VOL file object */
typedef struct H5VL_log_file_t : H5VL_log_obj_t {
    int rank;
    MPI_Comm comm;

    int refcnt;
    bool closing;

    void *lgp;
} H5VL_log_file_t;

/* The log VOL group object */
typedef struct H5VL_log_group_t : H5VL_log_obj_t {
    H5VL_log_file_t *fp;
} H5VL_log_group_t;

/* The log VOL dataset object */
typedef struct H5VL_log_dset_t : H5VL_log_obj_t {
    H5VL_log_file_t *fp;
    hsize_t ndim;
    hsize_t dims[32];
    hsize_t mdims[32];
} H5VL_log_dset_t;

/* The log VOL wrapper context */
typedef struct H5VL_log_wrap_ctx_t {
    hid_t uvlid;         /* VOL ID for under VOL */
    void *under_wrap_ctx;       /* Object wrapping context for under VOL */
} H5VL_log_wrap_ctx_t;

extern H5VL_log_obj_t* H5VL_log_new_obj(void *under_obj, hid_t uvlid);
extern herr_t H5VL_log_free_obj(H5VL_log_obj_t *obj);
extern herr_t H5VL_log_init(hid_t vipl_id);
extern herr_t H5VL_log_obj_term(void);
extern void* H5VL_log_info_copy(const void *_info);
extern herr_t H5VL_log_info_cmp(int *cmp_value, const void *_info1, const void *_info2);
extern herr_t H5VL_log_info_free(void *_info);
extern herr_t H5VL_log_info_to_str(const void *_info, char **str);
extern herr_t H5VL_log_str_to_info(const char *str, void **_info);
extern void* H5VL_log_get_object(const void *obj);
extern herr_t H5VL_log_get_wrap_ctx(const void *obj, void **wrap_ctx);
extern void* H5VL_log_wrap_object(void *obj, H5I_type_t obj_type, void *_wrap_ctx);
extern void* H5VL_log_unwrap_object(void *obj);
extern herr_t H5VL_log_free_wrap_ctx(void *_wrap_ctx);

// APIs
extern const H5VL_file_class_t H5VL_log_file_g;
extern const H5VL_dataset_class_t H5VL_log_dataset_g;
extern const H5VL_attr_class_t H5VL_log_attr_g;
extern const H5VL_group_class_t H5VL_log_group_g;
extern const H5VL_introspect_class_t H5VL_log_introspect_g;

// Utils
extern MPI_Datatype h5t_to_mpi_type(hid_t type_id);
extern void sortreq(int ndim, hssize_t len, MPI_Offset **starts, MPI_Offset **counts);
extern int intersect(int ndim, MPI_Offset *sa, MPI_Offset *ca, MPI_Offset *sb);
extern void mergereq(int ndim, hssize_t *len, MPI_Offset **starts, MPI_Offset **counts);
extern void sortblock(int ndim, hssize_t len, hsize_t **starts);
extern bool hlessthan(int ndim, hsize_t *a, hsize_t *b);

extern herr_t H5VLattr_get_wrapper(void *obj, hid_t connector_id, H5VL_attr_get_t get_type, hid_t dxpl_id, void **req, ...);