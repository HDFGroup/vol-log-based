#pragma once
#include <H5VLconnector.h>

herr_t H5VL_log_token_cmp (void *obj,
							   const H5O_token_t *token1,
							   const H5O_token_t *token2,
							   int *cmp_value);
herr_t H5VL_log_token_to_str (void *obj,
								  H5I_type_t obj_type,
								  const H5O_token_t *token,
								  char **token_str);
herr_t H5VL_log_token_from_str (void *obj,
									H5I_type_t obj_type,
									const char *token_str,
									H5O_token_t *token);
