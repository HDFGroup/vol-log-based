#include "logvol_internal.hpp"

#define NB_PROPERTY_NAME "logvol_nonblocking"
#define BSIZE_PROPERTY_NAME "logvol_nb_buffer_size"

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
		if (pexist) {			err = H5Pget (plist, NB_PROPERTY_NAME, nonblocking);
			CHECK_ERR

		} else {
			*nonblocking = H5VL_LOG_REQ_BLOCKING;
		}
	}

err_out:;
	return err;
}

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
		err = H5Pinsert2 (plist, BSIZE_PROPERTY_NAME, sizeof (size_t), &infty, NULL,
						  NULL, NULL, NULL, NULL, NULL);
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
		if (pexist) {			err = H5Pget (plist, BSIZE_PROPERTY_NAME, size);
			CHECK_ERR

		} else {
			*size = LOG_VOL_BSIZE_UNLIMITED;
		}
	}

err_out:;
	return err;
}