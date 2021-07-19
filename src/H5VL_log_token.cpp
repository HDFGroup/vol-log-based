#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <H5VLconnector.h>
#include <H5VLconnector_passthru.h>

#include <cassert>

#include "H5VL_log_obj.hpp"
#include "H5VL_log_token.hpp"
#include "H5VL_logi.hpp"

const H5VL_token_class_t H5VL_log_token_g {
	H5VL_log_token_cmp,					   /* cmp */
	H5VL_log_token_to_str,				   /* to_str */
	H5VL_log_token_from_str /* from_str */ /* optional */
};

/*---------------------------------------------------------------------------
 * Function:    H5VL_log_token_cmp
 *
 * Purpose:     Compare two of the connector's object tokens, setting
 *              *cmp_value, following the same rules as strcmp().
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
herr_t H5VL_log_token_cmp (void *obj,
						   const H5O_token_t *token1,
						   const H5O_token_t *token2,
						   int *cmp_value) {
	H5VL_log_obj_t *o = (H5VL_log_obj_t *)obj;
	herr_t ret_value;

#ifdef LOGVOL_VERBOSE_DEBUG
	printf ("H5VL_log_token_cmp(%p, %p, %p, %p)\n", obj, token1, token2, cmp_value);
#endif

	/* Sanity checks */
	assert (obj);
	assert (token1);
	assert (token2);
	assert (cmp_value);

	ret_value = H5VLtoken_cmp (o->uo, o->uvlid, token1, token2, cmp_value);

	return ret_value;
} /* end H5VL_log_token_cmp() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_log_token_to_str
 *
 * Purpose:     Serialize the connector's object token into a string.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
herr_t H5VL_log_token_to_str (void *obj,
							  H5I_type_t obj_type,
							  const H5O_token_t *token,
							  char **token_str) {
	H5VL_log_obj_t *o = (H5VL_log_obj_t *)obj;
	herr_t ret_value;

#ifdef LOGVOL_VERBOSE_DEBUG
	{
		char objname[128];
		ssize_t nsize;

		nsize = H5Iget_name (obj_type, objname, 128);
		if (nsize == 0) {
			sprintf (objname, "Unnamed_Object");
		} else if (nsize < 0) {
			sprintf (objname, "Unknown_Object");
		}

		printf ("H5VL_log_token_to_str(%p, %s, %p, %p)\n", obj, objname, token, token_str);
	}
#endif

	/* Sanity checks */
	assert (obj);
	assert (token);
	assert (token_str);

	ret_value = H5VLtoken_to_str (o->uo, obj_type, o->uvlid, token, token_str);

	return ret_value;
} /* end H5VL_log_token_to_str() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_log_token_from_str
 *
 * Purpose:     Deserialize the connector's object token from a string.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
herr_t H5VL_log_token_from_str (void *obj,
								H5I_type_t obj_type,
								const char *token_str,
								H5O_token_t *token) {
	H5VL_log_obj_t *o = (H5VL_log_obj_t *)obj;
	herr_t ret_value;

#ifdef LOGVOL_VERBOSE_DEBUG
	{
		char objname[128];
		ssize_t nsize;

		nsize = H5Iget_name (obj_type, objname, 128);
		if (nsize == 0) {
			sprintf (objname, "Unnamed_Object");
		} else if (nsize < 0) {
			sprintf (objname, "Unknown_Object");
		}

		printf ("H5VL_log_token_from_str(%p, %s, %s, %p)\n", obj, objname, token_str, token);
	}
#endif

	/* Sanity checks */
	assert (obj);
	assert (token);
	assert (token_str);

	ret_value = H5VLtoken_from_str (o->uo, obj_type, o->uvlid, token_str, token);

	return ret_value;
} /* end H5VL_log_token_from_str() */