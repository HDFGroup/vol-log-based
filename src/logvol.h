#ifndef _logVOL_H_
#define _logVOL_H_

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
 * Purpose: This is a "log" VOL connector, which forwards each
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
#include "H5VLpublic.h"


/* Identifier for the pass-through VOL connector */
#define H5VL_log  (H5VL_log_register())
#ifdef __cplusplus
extern "C" {
#endif
hid_t H5VL_log_register(void);
#ifdef __cplusplus
}
#endif

/************/
/* Typedefs */
/************/

/* Pass-through VOL connector info */
typedef struct H5VL_log_info_t {
    hid_t uvlid;         /* VOL ID for under VOL */
    void *under_vol_info;       /* VOL info for under VOL */
} H5VL_log_info_t;

extern const H5VL_class_t H5VL_log_g;

// Helper functions
herr_t H5Pset_nonblocking(hid_t plist, int nonblocking);
herr_t H5Pget_nonblocking(hid_t plist, int *nonblocking);

#endif