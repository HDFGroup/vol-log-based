#pragma once

#include <H5VLpublic.h>

extern void *H5VL_log_datatype_open_with_uo (void *obj, void *uo, const H5VL_loc_params_t *loc_params);
extern MPI_Datatype H5VL_logi_get_mpi_type_by_size (size_t size);