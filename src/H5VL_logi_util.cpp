#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cassert>
#include <cstdlib>

#include "H5VL_logi.hpp"
#include "H5VL_logi_util.hpp"
#include "H5VL_logi_wrapper.hpp"
#include "hdf5.h"

herr_t H5VL_logi_add_att (H5VL_log_obj_t *op,
						  const char *name,
						  hid_t atype,
						  hid_t mtype,
						  hsize_t len,
						  void *buf,
						  hid_t dxpl_id,
						  void **req) {
	herr_t err = 0;
	H5VL_loc_params_t loc;
	hid_t asid = -1;
	void *ap;

	asid = H5Screate_simple (1, &len, &len);
	CHECK_ID (asid);

	loc.obj_type = op->type;
	loc.type	 = H5VL_OBJECT_BY_SELF;

	ap = H5VLattr_create (op->uo, &loc, op->uvlid, name, atype, asid, H5P_ATTRIBUTE_CREATE_DEFAULT,
						  H5P_ATTRIBUTE_ACCESS_DEFAULT, dxpl_id, NULL);
	CHECK_PTR (ap);
	err = H5VLattr_write (ap, op->uvlid, mtype, buf, dxpl_id, NULL);
	CHECK_ERR;
	err = H5VLattr_close (ap, op->uvlid, dxpl_id, NULL);
	CHECK_ERR

	H5Sclose (asid);

err_out:;
	return err;
}

herr_t H5VL_logi_put_att (
	H5VL_log_obj_t *op, const char *name, hid_t mtype, void *buf, hid_t dxpl_id) {
	herr_t err = 0;
	H5VL_loc_params_t loc;
	void *ap;

	loc.obj_type = op->type;
	loc.type	 = H5VL_OBJECT_BY_SELF;

	ap = H5VLattr_open (op->uo, &loc, op->uvlid, name, H5P_ATTRIBUTE_ACCESS_DEFAULT, dxpl_id, NULL);
	CHECK_PTR (ap);
	err = H5VLattr_write (ap, op->uvlid, mtype, buf, dxpl_id, NULL);
	CHECK_ERR;
	err = H5VLattr_close (ap, op->uvlid, dxpl_id, NULL);
	CHECK_ERR

err_out:;
	return err;
}

herr_t H5VL_logi_get_att (
	H5VL_log_obj_t *op, const char *name, hid_t mtype, void *buf, hid_t dxpl_id) {
	herr_t err = 0;
	H5VL_loc_params_t loc;
	void *ap;

	loc.obj_type = op->type;
	loc.type	 = H5VL_OBJECT_BY_SELF;

	ap = H5VLattr_open (op->uo, &loc, op->uvlid, name, H5P_ATTRIBUTE_ACCESS_DEFAULT, dxpl_id, NULL);
	CHECK_PTR (ap);
	err = H5VLattr_read (ap, op->uvlid, mtype, buf, dxpl_id, NULL);
	CHECK_ERR;
	err = H5VLattr_close (ap, op->uvlid, dxpl_id, NULL);
	CHECK_ERR

err_out:;
	return err;
}

herr_t H5VL_logi_get_att_ex (
	H5VL_log_obj_t *op, const char *name, hid_t mtype, hsize_t *len, void *buf, hid_t dxpl_id) {
	herr_t err = 0;
	H5VL_loc_params_t loc;
	hid_t asid = -1;
	int ndim;
	void *ap;

	loc.obj_type = op->type;
	loc.type	 = H5VL_OBJECT_BY_SELF;

	ap = H5VLattr_open (op->uo, &loc, op->uvlid, name, H5P_ATTRIBUTE_ACCESS_DEFAULT, dxpl_id, NULL);
	CHECK_PTR (ap);
	asid = H5VL_logi_attr_get_space (op->fp, ap, op->uvlid, dxpl_id);
	CHECK_ID (asid);
	ndim = H5Sget_simple_extent_dims (asid, len, NULL);
	CHECK_ID (ndim)
	LOG_VOL_ASSERT (ndim == 1);
	err = H5VLattr_read (ap, op->uvlid, mtype, buf, dxpl_id, NULL);
	CHECK_ERR;
	err = H5VLattr_close (ap, op->uvlid, dxpl_id, NULL);
	CHECK_ERR

err_out:;
	H5Sclose (asid);

	return err;
}

MPI_Datatype H5VL_logi_get_mpi_type_by_size (size_t size) {
	switch (size) {
		case 1:
			return MPI_BYTE;
		case 2:
			return MPI_SHORT;
		case 4:
			return MPI_INT;
		case 8:
			return MPI_LONG_LONG;
	}

	return MPI_DATATYPE_NULL;
}