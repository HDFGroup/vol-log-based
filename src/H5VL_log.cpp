#include "H5VL_log.h"
#include "H5VL_log_info.hpp"
#include "H5VL_logi.hpp"

/* The connector identification number, initialized at runtime */
hid_t H5VL_LOG_g = H5I_INVALID_HID;

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_register
 *
 * Purpose:     Register the pass-through VOL connector and retrieve an ID
 *              for it.
 *
 * Return:      Success:    The ID for the pass-through VOL connector
 *              Failure:    -1
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, November 28, 2018
 *
 *-------------------------------------------------------------------------
 */
hid_t H5VL_log_register(void) {
    /* Singleton register the pass-through VOL connector ID */
    if(H5VL_LOG_g < 0)
        H5VL_LOG_g = H5VLregister_connector(&H5VL_log_g, H5P_VOL_INITIALIZE_DEFAULT);

    return H5VL_LOG_g;
} /* end H5VL_log_register() */

/*-------------------------------------------------------------------------
 * Function:    H5Pset_vol_log
 *
 * Purpose:     Set to use the log VOL in a file access property list
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5Pset_vol_log (hid_t fapl_id) {
	herr_t err = 0;
	hid_t logvlid;
	H5VL_log_info_t log_vol_info;

	// Register the VOL if not yet registered
	logvlid = H5VL_log_register ();
	CHECK_ID (logvlid)

	err = H5Pget_vol_id (fapl_id, &(log_vol_info.uvlid));
	CHECK_ERR
	err = H5Pget_vol_info (fapl_id, &(log_vol_info.under_vol_info));
	CHECK_ERR

	err = H5Pset_vol (fapl_id, logvlid, &log_vol_info);
	CHECK_ERR

err_out:
	return err;
}

#define NB_PROPERTY_NAME	"H5VL_log_nonblocking"
herr_t H5Pset_nonblocking (hid_t plist, H5VL_log_req_type_t nonblocking) {
	herr_t err = 0;
	htri_t isdxpl, pexist;

	isdxpl = H5Pisa_class (plist, H5P_DATASET_XFER);
	CHECK_ID (isdxpl)
	if (isdxpl == 0) RET_ERR ("Not dxplid")

	pexist = H5Pexist (plist, NB_PROPERTY_NAME);
	CHECK_ID (pexist)
	if (!pexist) {
		H5VL_log_req_type_t blocking = H5VL_LOG_REQ_BLOCKING;
		err = H5Pinsert2 (plist, NB_PROPERTY_NAME, sizeof (H5VL_log_req_type_t), &blocking, NULL,
						  NULL, NULL, NULL, NULL, NULL);
		CHECK_ERR
	}

	err = H5Pset (plist, NB_PROPERTY_NAME, &nonblocking);
	CHECK_ERR

err_out:;
	return err;
}

herr_t H5Pget_nonblocking (hid_t plist, H5VL_log_req_type_t *nonblocking) {
	herr_t err = 0;
	htri_t isdxpl, pexist;

	isdxpl = H5Pisa_class (plist, H5P_DATASET_XFER);
	CHECK_ID (isdxpl)
	if (isdxpl == 0)
		*nonblocking = H5VL_LOG_REQ_BLOCKING;  // Default property will not pass class check
	else {
		pexist = H5Pexist (plist, NB_PROPERTY_NAME);
		CHECK_ID (pexist)
		if (pexist) {
			err = H5Pget (plist, NB_PROPERTY_NAME, nonblocking);
			CHECK_ERR

		} else {
			*nonblocking = H5VL_LOG_REQ_BLOCKING;
		}
	}

err_out:;
	return err;
}

#define BSIZE_PROPERTY_NAME "H5VL_log_nb_buffer_size"
herr_t H5Pset_nb_buffer_size (hid_t plist, size_t size) {
	herr_t err = 0;
	htri_t isfapl;
	htri_t isdxpl, pexist;

	// TODO: Fix pclass problem
	return 0;

	isfapl = H5Pisa_class (plist, H5P_FILE_ACCESS);
	CHECK_ID (isfapl)
	if (isfapl == 0) RET_ERR ("Not faplid")

	pexist = H5Pexist (plist, BSIZE_PROPERTY_NAME);
	CHECK_ID (pexist)
	if (!pexist) {
		ssize_t infty = LOG_VOL_BSIZE_UNLIMITED;
		err = H5Pinsert2 (plist, BSIZE_PROPERTY_NAME, sizeof (size_t), &infty, NULL, NULL, NULL,
						  NULL, NULL, NULL);
		CHECK_ERR
	}

	err = H5Pset (plist, BSIZE_PROPERTY_NAME, &size);
	CHECK_ERR

err_out:;
	return err;
}

herr_t H5Pget_nb_buffer_size (hid_t plist, ssize_t *size) {
	herr_t err = 0;
	htri_t isdxpl, pexist;

	isdxpl = H5Pisa_class (plist, H5P_FILE_CREATE);
	CHECK_ID (isdxpl)
	if (isdxpl == 0)
		*size = LOG_VOL_BSIZE_UNLIMITED;  // Default property will not pass class check
	else {
		pexist = H5Pexist (plist, BSIZE_PROPERTY_NAME);
		CHECK_ID (pexist)
		if (pexist) {
			err = H5Pget (plist, BSIZE_PROPERTY_NAME, size);
			CHECK_ERR

		} else {
			*size = LOG_VOL_BSIZE_UNLIMITED;
		}
	}

err_out:;
	return err;
}

#define MERGE_META_NAME_PROPERTY_NAME "H5VL_log_metadata_merge"
herr_t H5Pset_meta_merge (hid_t plist, hbool_t merge) {
	herr_t err = 0;
	htri_t isfapl;
	htri_t isdxpl, pexist;

	// TODO: Fix pclass problem
	return 0;

	isfapl = H5Pisa_class (plist, H5P_FILE_ACCESS);
	CHECK_ID (isfapl)
	if (isfapl == 0) RET_ERR ("Not faplid")

	pexist = H5Pexist (plist, MERGE_META_NAME_PROPERTY_NAME);
	CHECK_ID (pexist)
	if (!pexist) {
		hbool_t f = false;
		err = H5Pinsert2 (plist, MERGE_META_NAME_PROPERTY_NAME, sizeof (hbool_t), &f, NULL, NULL, NULL,
						  NULL, NULL, NULL);
		CHECK_ERR
	}

	err = H5Pset (plist, MERGE_META_NAME_PROPERTY_NAME, &merge);
	CHECK_ERR

err_out:;
	return err;
}

herr_t H5Pget_meta_merge (hid_t plist, hbool_t *merge) {
	herr_t err = 0;
	htri_t isdxpl, pexist;

	isdxpl = H5Pisa_class (plist, H5P_FILE_CREATE);
	CHECK_ID (isdxpl)
	if (isdxpl == 0)
		*merge = false;  // Default property will not pass class check
	else {
		pexist = H5Pexist (plist, MERGE_META_NAME_PROPERTY_NAME);
		CHECK_ID (pexist)
		if (pexist) {
			err = H5Pget (plist, MERGE_META_NAME_PROPERTY_NAME, merge);
			CHECK_ERR

		} else {
			*merge = false;
		}
	}

err_out:;
	return err;
}

#define ZIP_META_NAME_PROPERTY_NAME "H5VL_log_metadata_zip"
herr_t H5Pset_meta_zip (hid_t plist, hbool_t zip) {
	herr_t err = 0;
	htri_t isfapl;
	htri_t isdxpl, pexist;

	// TODO: Fix pclass problem
	return 0;

	isfapl = H5Pisa_class (plist, H5P_FILE_ACCESS);
	CHECK_ID (isfapl)
	if (isfapl == 0) RET_ERR ("Not faplid")

	pexist = H5Pexist (plist, ZIP_META_NAME_PROPERTY_NAME);
	CHECK_ID (pexist)
	if (!pexist) {
		hbool_t f = false;
		err = H5Pinsert2 (plist, ZIP_META_NAME_PROPERTY_NAME, sizeof (hbool_t), &f, NULL, NULL, NULL,
						  NULL, NULL, NULL);
		CHECK_ERR
	}

	err = H5Pset (plist, ZIP_META_NAME_PROPERTY_NAME, &zip);
	CHECK_ERR

err_out:;
	return err;
}

herr_t H5Pget_meta_zip (hid_t plist, hbool_t *zip) {
	herr_t err = 0;
	htri_t isdxpl, pexist;

	isdxpl = H5Pisa_class (plist, H5P_FILE_ACCESS);
	CHECK_ID (isdxpl)
	if (isdxpl == 0)
		*zip = false;  // Default property will not pass class check
	else {
		pexist = H5Pexist (plist, ZIP_META_NAME_PROPERTY_NAME);
		CHECK_ID (pexist)
		if (pexist) {
			err = H5Pget (plist, ZIP_META_NAME_PROPERTY_NAME, zip);
			CHECK_ERR

		} else {
			*zip = false;
		}
	}

err_out:;
	return err;
}

#define SEL_ENCODING_PROPERTY_NAME "H5VL_log_sel_encoding"
herr_t H5Pset_sel_encoding (hid_t plist, H5VL_log_sel_encoding_t encoding) {
	herr_t err = 0;
	htri_t isfapl;
	htri_t isdxpl, pexist;

	// TODO: Fix pclass problem
	return 0;

	isfapl = H5Pisa_class (plist, H5P_FILE_ACCESS);
	CHECK_ID (isfapl)
	if (isfapl == 0) RET_ERR ("Not faplid")

	pexist = H5Pexist (plist, SEL_ENCODING_PROPERTY_NAME);
	CHECK_ID (pexist)
	if (!pexist) {
		H5VL_log_sel_encoding_t offset = H5VL_LOG_ENCODING_OFFSET;
		err = H5Pinsert2 (plist, SEL_ENCODING_PROPERTY_NAME, sizeof (size_t), &offset, NULL, NULL, NULL,
						  NULL, NULL, NULL);
		CHECK_ERR
	}

	err = H5Pset (plist, SEL_ENCODING_PROPERTY_NAME, &encoding);
	CHECK_ERR

err_out:;
	return err;
}

herr_t H5Pget_sel_encoding (hid_t plist, H5VL_log_sel_encoding_t *encoding) {
	herr_t err = 0;
	htri_t isdxpl, pexist;

	isdxpl = H5Pisa_class (plist, H5P_FILE_CREATE);
	CHECK_ID (isdxpl)
	if (isdxpl == 0)
		*encoding = H5VL_LOG_ENCODING_OFFSET;  // Default property will not pass class check
	else {
		pexist = H5Pexist (plist, SEL_ENCODING_PROPERTY_NAME);
		CHECK_ID (pexist)
		if (pexist) {
			err = H5Pget (plist, SEL_ENCODING_PROPERTY_NAME, encoding);
			CHECK_ERR

		} else {
			*encoding = H5VL_LOG_ENCODING_OFFSET;
		}
	}

err_out:;
	return err;
}