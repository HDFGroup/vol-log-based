/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#pragma once

#include <H5VLconnector.h>

/* Characteristics of the pass-through VOL connector */
#define H5VL_log_NAME   "LOG"
#define H5VL_log_REGID  514 /* VOL connector ID */
#define H5VL_log_APIVER 3   // Verion of VOL interface used by logvol
/********************* */
/* Function prototypes */
/********************* */
herr_t H5VL_log_init (hid_t vipl_id);
herr_t H5VL_log_obj_term (void);
herr_t H5VL_log_optional (void *obj, H5VL_optional_args_t *args, hid_t dxpl_id, void **req);
