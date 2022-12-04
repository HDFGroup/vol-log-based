/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#pragma once

#include <H5VLconnector.h>
#include <mpi.h>

#include "H5VL_log_obj.hpp"
#include "H5VL_log_wrap.hpp"
#include "H5VL_logi_filter.hpp"

#define LOGVOL_SELCTION_TYPE_HYPERSLABS 0x01
#define LOGVOL_SELCTION_TYPE_POINTS     0x02
#define LOGVOL_SELCTION_TYPE_OFFSETS    0x04

#ifdef HDF5_GE_1133
#define H5VL_log_dataset_read  H5VL_log_dataset_read_2
#define H5VL_log_dataset_write H5VL_log_dataset_write_2
#else
#define H5VL_log_dataset_read  H5VL_log_dataset_read_1
#define H5VL_log_dataset_write H5VL_log_dataset_write_1
#endif

/* The log VOL dataset object */
typedef struct H5VL_log_dset_info_t {
    hsize_t ndim;                     // Number of dimensions
    hsize_t esize;                    // Size of dtype
    hid_t dtype = -1;                 // External data type
    hsize_t dims[H5S_MAX_RANK];       // Current size
    hsize_t mdims[H5S_MAX_RANK];      // Max size along each dimension
    MPI_Offset dsteps[H5S_MAX_RANK];  // Number of elements in the subspace below each dimension
    std::vector<H5VL_log_filter_t> filters;  // Declared filters
    char *fill;                              // Fill value
} H5VL_log_dset_info_t;

/* The log VOL dataset object */
typedef struct H5VL_log_dset_t : H5VL_log_obj_t {
    int id;  // Dataset ID
    using H5VL_log_obj_t::H5VL_log_obj_t;
} H5VL_log_dset_t;

extern int H5Dwrite_n_op_val;
extern int H5Dread_n_op_val;

void *H5VL_log_dataset_create (void *obj,
                               const H5VL_loc_params_t *loc_params,
                               const char *name,
                               hid_t lcpl_id,
                               hid_t type_id,
                               hid_t space_id,
                               hid_t dcpl_id,
                               hid_t dapl_id,
                               hid_t dxpl_id,
                               void **req);
void *H5VL_log_dataset_open (void *obj,
                             const H5VL_loc_params_t *loc_params,
                             const char *name,
                             hid_t dapl_id,
                             hid_t dxpl_id,
                             void **req);
herr_t H5VL_log_dataset_read_1 (void *dset,
                                hid_t mem_type_id,
                                hid_t mem_space_id,
                                hid_t file_space_id,
                                hid_t plist_id,
                                void *buf,
                                void **req);
herr_t H5VL_log_dataset_write_1 (void *dset,
                                 hid_t mem_type_id,
                                 hid_t mem_space_id,
                                 hid_t file_space_id,
                                 hid_t plist_id,
                                 const void *buf,
                                 void **req);
herr_t H5VL_log_dataset_read_2 (size_t count,
                                void *dset[],
                                hid_t mem_type_id[],
                                hid_t mem_space_id[],
                                hid_t file_space_id[],
                                hid_t plist_id,
                                void *buf[],
                                void **req);
herr_t H5VL_log_dataset_write_2 (size_t count,
                                 void *dset[],
                                 hid_t mem_type_id[],
                                 hid_t mem_space_id[],
                                 hid_t file_space_id[],
                                 hid_t plist_id,
                                 const void *buf[],
                                 void **req);
herr_t H5VL_log_dataset_get (void *dset, H5VL_dataset_get_args_t *args, hid_t dxpl_id, void **req);
herr_t H5VL_log_dataset_specific (void *obj,
                                  H5VL_dataset_specific_args_t *args,
                                  hid_t dxpl_id,
                                  void **req);
herr_t H5VL_log_dataset_optional (void *obj, H5VL_optional_args_t *args, hid_t dxpl_id, void **req);
herr_t H5VL_log_dataset_close (void *dset, hid_t dxpl_id, void **req);
