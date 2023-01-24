/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#pragma once

#include <H5VLconnector.h>
#include <hdf5.h>
#include <mpi.h>

#include "H5VL_log_obj.hpp"
#ifdef HDF5_GE_1133
#define H5VL_log_under_dataset_read(uo, uvlid, mtid, msid, dsid, dxplid, buf, req) \
    H5VLdataset_read (1, &(uo), uvlid, &(mtid), &(msid), &(dsid), dxplid, (void**)&(buf), NULL)
#define H5VL_log_under_dataset_write(uo, uvlid, mtid, msid, dsid, dxplid, buf, req) \
    H5VLdataset_write (1, &(uo), uvlid, &(mtid), &(msid), &(dsid), dxplid, (const void**)&(buf), NULL)
#else
#define H5VL_log_under_dataset_read(uo, uvlid, mtid, msid, dsid, dxplid, buf, req) \
    H5VLdataset_read (uo, uvlid, mtid, msid, dsid, dxplid, (void*)buf, NULL)
#define H5VL_log_under_dataset_write(uo, uvlid, mtid, msid, dsid, dxplid, buf, req) \
    H5VLdataset_write (uo, uvlid, mtid, msid, dsid, dxplid, (void*)buf, NULL)
#endif

// Utils
extern MPI_Datatype h5t_to_mpi_type (hid_t type_id);
extern void sortreq (int ndim, hssize_t len, MPI_Offset **starts, MPI_Offset **counts);
extern int intersect (int ndim, MPI_Offset *sa, MPI_Offset *ca, MPI_Offset *sb);
extern void mergereq (int ndim, hssize_t *len, MPI_Offset **starts, MPI_Offset **counts);
extern void sortblock (int ndim, hssize_t len, hsize_t **starts);
extern bool hlessthan (int ndim, hsize_t *a, hsize_t *b);

template <class A, class B>
int H5VL_logi_vector_cmp (int ndim, A *l, B *r);

extern void H5VL_logi_add_att (H5VL_log_obj_t *op,
                               const char *name,
                               hid_t atype,
                               hid_t mtype,
                               hsize_t len,
                               void *buf,
                               hid_t dxpl_id,
                               void **req);
extern void H5VL_logi_put_att (
    H5VL_log_obj_t *op, const char *name, hid_t mtype, void *buf, hid_t dxpl_id);
extern void H5VL_logi_get_att (
    H5VL_log_obj_t *op, const char *name, hid_t mtype, void *buf, hid_t dxpl_id);

// Added by Zanhua: check att exists
// extern herr_t H5VL_logi_exists_att(
// 	void* uo, hid_t uvlid, H5I_type_t type, const char *name, hid_t dxpl_id, hbool_t
// *exists_ptr);
extern hbool_t H5VL_logi_exists_att (H5VL_log_obj_t *op, const char *name, hid_t dxpl_id);
extern hbool_t H5VL_logi_exists_link (H5VL_log_file_t *op, const char *name, hid_t dxpl_id);

extern void H5VL_logi_get_att_ex (
    H5VL_log_obj_t *op, const char *name, hid_t mtype, hsize_t *len, void *buf, hid_t dxpl_id);

extern void H5VL_logi_add_att (void *uo,
                               hid_t uvlid,
                               H5I_type_t type,
                               const char *name,
                               hid_t atype,
                               hid_t mtype,
                               hsize_t len,
                               void *buf,
                               hid_t dxpl_id,
                               void **req);
extern void H5VL_logi_put_att (void *uo,
                               hid_t uvlid,
                               H5I_type_t type,
                               const char *name,
                               hid_t mtype,
                               void *buf,
                               hid_t dxpl_id);
extern void H5VL_logi_get_att (void *uo,
                               hid_t uvlid,
                               H5I_type_t type,
                               const char *name,
                               hid_t mtype,
                               void *buf,
                               hid_t dxpl_id);

MPI_Datatype H5VL_logi_get_mpi_type_by_size (size_t size);

inline void H5VL_logi_sreverse (uint16_t *val) {
    *val = ((((*val) >> 8) & 0x00FF) | (((*val) << 8) & 0xFF00));
}
inline void H5VL_logi_lreverse (uint32_t *val) {
    *val = (((*val) >> 24) & 0x000000FF) | (((*val) >> 8) & 0x0000FF00) |
           (((*val) << 8) & 0x00FF0000) | (((*val) << 24) & 0xFF000000);
}
inline void H5VL_logi_llreverse (uint64_t *val) {
    *val = ((((*val) >> 56) & 0x00000000000000FF) | (((*val) >> 40) & 0x000000000000FF00) |
            (((*val) >> 24) & 0x0000000000FF0000) | (((*val) >> 8) & 0x00000000FF000000) |
            (((*val) << 8) & 0x000000FF00000000) | (((*val) << 24) & 0x0000FF0000000000) |
            (((*val) << 40) & 0x00FF000000000000) | (((*val) << 56) & 0xFF00000000000000));
}
inline void H5VL_logi_sreverse (uint16_t *start, uint16_t *end) {
    for (; start < end; start++) { H5VL_logi_sreverse (start); }
}
inline void H5VL_logi_lreverse (uint32_t *start, uint32_t *end) {
    for (; start < end; start++) { H5VL_logi_lreverse (start); }
}
inline void H5VL_logi_llreverse (uint64_t *start, uint64_t *end) {
    for (; start < end; start++) { H5VL_logi_llreverse (start); }
}

inline char *H5VL_logi_name_remap (const char *name) {
    int n;
    char *ret;

    if (!name) { return NULL; }

    n = strlen (name);

    if (n == 0) { ERR_OUT ("Object name cannot be empty") }
    if (name[0] == '_') {
        ret    = (char *)malloc (n + 2);
        ret[0] = '_';
        memcpy (ret + 1, name, n);
        ret[n + 1] = '\0';
    } else {
        ret = (char *)name;
    }

    return ret;
}

inline void H5VL_logi_reset_lib_stat (void *&stat) {
    herr_t err = 0;
    err        = H5VLretrieve_lib_state (&stat);
    CHECK_ERR
    err = H5VLstart_lib_state ();
    CHECK_ERR
    err = H5VLrestore_lib_state (stat);
    CHECK_ERR
}

inline void H5VL_logi_restore_lib_stat (void *&stat) {
    herr_t err = 0;
    if (stat) {
        err = H5VLfinish_lib_state ();
        CHECK_ERR
        err = H5VLrestore_lib_state (stat);
        CHECK_ERR
        err = H5VLfree_lib_state (stat);
        CHECK_ERR
    }
    stat = NULL;
}