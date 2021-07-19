#pragma once

#include <H5VLconnector.h>

#include "H5VL_logi.hpp"

// Internal
extern void *H5VL_log_obj_open_with_uo (void *obj,
										void *uo,
										H5I_type_t type,
										const H5VL_loc_params_t *loc_params);
