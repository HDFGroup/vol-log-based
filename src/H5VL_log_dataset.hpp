#pragma once

#include <H5VLpublic.h>
#include <mpi.h>

#include "H5VL_log_obj.hpp"
#include "H5VL_log_wrap.hpp"


#define LOGVOL_SELCTION_TYPE_HYPERSLABS 0x01
#define LOGVOL_SELCTION_TYPE_POINTS 0x02
#define LOGVOL_SELCTION_TYPE_OFFSETS 0x04


/* The log VOL dataset object */
typedef struct H5VL_log_dset_t : H5VL_log_obj_t {
	int id;
	hsize_t ndim;
	hsize_t dims[H5S_MAX_RANK];
	hsize_t mdims[H5S_MAX_RANK];
	hsize_t dsteps[H5S_MAX_RANK];

	hid_t dtype;
	hsize_t esize;
} H5VL_log_dset_t;