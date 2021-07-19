#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <hdf5.h>

#include <cassert>
#include <cstdlib>
#include <cstring>

#include "H5VL_log_info.hpp"
#include "H5VL_logi.hpp"

const H5VL_info_class_t H5VL_log_info_g {

	sizeof (H5VL_log_info_t), /* info size    */
	H5VL_log_info_copy,		  /* info copy    */
	H5VL_log_info_cmp,		  /* info compare */
	H5VL_log_info_free,		  /* info free    */
	H5VL_log_info_to_str,	  /* info to str  */
	H5VL_log_str_to_info,	  /* str to info  */
};

/*---------------------------------------------------------------------------
 * Function:    H5VL_log_info_copy
 *
 * Purpose:     Duplicate the connector's info object.
 *
 * Returns: Success:    New connector info object
 *      Failure:    NULL
 *
 *---------------------------------------------------------------------------
 */
void *H5VL_log_info_copy (const void *_info) {
	const H5VL_log_info_t *info = (const H5VL_log_info_t *)_info;
	H5VL_log_info_t *new_info;

	/* Allocate new VOL info struct for the PNC connector */
	new_info = (H5VL_log_info_t *)calloc (1, sizeof (H5VL_log_info_t));

	new_info->uvlid = info->uvlid;
	H5VL_logi_inc_ref (new_info->uvlid);

	if (info->under_vol_info)
		H5VLcopy_connector_info (new_info->uvlid, &(new_info->under_vol_info),
								 info->under_vol_info);

	return (new_info);
} /* end H5VL_log_info_copy() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_log_info_cmp
 *
 * Purpose:     Compare two of the connector's info objects, setting *cmp_value,
 *      following the same rules as strcmp().
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
herr_t H5VL_log_info_cmp (int *cmp_value, const void *_info1, const void *_info2) {
	const H5VL_log_info_t *info1 = (const H5VL_log_info_t *)_info1;
	const H5VL_log_info_t *info2 = (const H5VL_log_info_t *)_info2;

	/* Sanity checks */
	assert (info1);
	assert (info2);

	/* Compare under VOL connector classes */
	H5VLcmp_connector_cls (cmp_value, info1->uvlid, info2->uvlid);
	if (*cmp_value == 0) {
		/* Compare under VOL connector info objects */
		H5VLcmp_connector_info (cmp_value, info1->uvlid, info1->under_vol_info,
								info2->under_vol_info);
	}

	return (0);
} /* end H5VL_log_info_cmp() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_log_info_free
 *
 * Purpose:     Release an info object for the connector.
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
herr_t H5VL_log_info_free (void *_info) {
	H5VL_log_info_t *info = (H5VL_log_info_t *)_info;
	hid_t err_id;

#ifdef LOGVOL_VERBOSE_DEBUG
	printf ("H5VL_log_info_free(%p)\n", _info);
#endif

	/* Release MPI_Info */
	// MPI_Comm_free(&(info->comm));

	err_id = H5Eget_current_stack ();

	/* Release underlying VOL ID and info */
	if (info->uvlid) H5VLfree_connector_info (info->uvlid, info->under_vol_info);
	H5VL_logi_dec_ref (info->uvlid);

	H5Eset_current_stack (err_id);

	/* Free PNC info object itself */
	free (info);

	return (0);
} /* end H5VL_log_info_free() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_log_info_to_str
 *
 * Purpose:     Serialize an info object for this connector into a string
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
herr_t H5VL_log_info_to_str (const void *_info, char **str) {
	const H5VL_log_info_t *info	   = (const H5VL_log_info_t *)_info;
	H5VL_class_value_t under_value = (H5VL_class_value_t)-1;
	char *under_vol_string		   = NULL;
	size_t under_vol_str_len	   = 0;

#ifdef LOGVOL_VERBOSE_DEBUG
	printf ("H5VL_log_info_to_str(%p, %p)\n", _info, str);
#endif

	/* Get value and string for underlying VOL connector */
	H5VLget_value (info->uvlid, &under_value);
	H5VLconnector_info_to_str (info->under_vol_info, info->uvlid, &under_vol_string);

	/* Determine length of underlying VOL info string */
	if (under_vol_string) under_vol_str_len = strlen (under_vol_string);

	/* Allocate space for our info */
	*str = (char *)H5allocate_memory (32 + under_vol_str_len, (hbool_t)0);
	assert (*str);

	/* Encode our info
	 * Normally we'd use snprintf() here for a little extra safety, but that
	 * call had problems on Windows until recently. So, to be as platform-independent
	 * as we can, we're using sprintf() instead.
	 */
	sprintf (*str, "under_vol=%u;under_info={%s}", (unsigned)under_value,
			 (under_vol_string ? under_vol_string : ""));

	return (0);
} /* end H5VL_log_info_to_str() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_log_str_to_info
 *
 * Purpose:     Deserialize a string into an info object for this connector.
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
herr_t H5VL_log_str_to_info (const char *str, void **_info) {
	H5VL_log_info_t *info = (H5VL_log_info_t *)_info;
	unsigned under_vol_value;
	const char *under_vol_info_start, *under_vol_info_end;
	hid_t uvlid;
	void *under_vol_info = NULL;

#ifdef LOGVOL_VERBOSE_DEBUG
	printf ("H5VL_log_str_to_info(%s, %p)\n", str, _info);
#endif

	/* Retrieve the underlying VOL connector value and info */
	sscanf (str, "under_vol=%u;", &under_vol_value);
	uvlid				 = H5VLregister_connector_by_value ((H5VL_class_value_t)under_vol_value,
												H5P_VOL_INITIALIZE_DEFAULT);
	under_vol_info_start = strchr (str, '{');
	under_vol_info_end	 = strrchr (str, '}');
	assert (under_vol_info_end > under_vol_info_start);
	if (under_vol_info_end != (under_vol_info_start + 1)) {
		char *under_vol_info_str;

		under_vol_info_str = (char *)malloc ((size_t) (under_vol_info_end - under_vol_info_start));
		memcpy (under_vol_info_str, under_vol_info_start + 1,
				(size_t) ((under_vol_info_end - under_vol_info_start) - 1));
		*(under_vol_info_str + (under_vol_info_end - under_vol_info_start)) = '\0';

		H5VLconnector_str_to_info (under_vol_info_str, uvlid, &under_vol_info);

		free (under_vol_info_str);
	} /* end else */

	/* Allocate new pass-through VOL connector info and set its fields */
	info				 = (H5VL_log_info_t *)calloc (1, sizeof (H5VL_log_info_t));
	info->uvlid			 = uvlid;
	info->under_vol_info = under_vol_info;

	/* Set return value */
	*_info = info;

	return 0;
} /* end H5VL_log_str_to_info() */