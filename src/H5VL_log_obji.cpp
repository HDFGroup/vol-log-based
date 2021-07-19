#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "H5VL_log_filei.hpp"
#include "H5VL_log_obj.hpp"
#include "H5VL_log_obji.hpp"
#include "H5VL_log_wrap.hpp"
#include "H5VL_logi.hpp"

void *H5VL_log_obj_open_with_uo (void *obj,
								 void *uo,
								 H5I_type_t type,
								 const H5VL_loc_params_t *loc_params) {
	H5VL_log_obj_t *pp = (H5VL_log_obj_t *)obj;
	H5VL_log_obj_t *op = NULL;

	/* Check arguments */
	// if(loc_params->type != H5VL_OBJECT_BY_SELF) ERR_OUT("loc_params->type is not
	// H5VL_OBJECT_BY_SELF")

	op = new H5VL_log_obj_t (pp, type, uo);
	CHECK_PTR (op);

	return (void *)op;

err_out:;
	delete op;

	return NULL;
} /* end H5VL_log_group_ppen() */

H5VL_log_obj_t::H5VL_log_obj_t () {
	this->fp	= NULL;
	this->uvlid = -1;
	this->uo	= NULL;
	this->type	= H5I_UNINIT;
#ifdef LOGVOL_DEBUG
	this->ext_ref = 0;
#endif
}
H5VL_log_obj_t::H5VL_log_obj_t (struct H5VL_log_obj_t *pp) {
	this->fp	= pp->fp;
	this->uvlid = pp->uvlid;
	H5VL_log_filei_inc_ref (this->fp);
#ifdef LOGVOL_DEBUG
	this->ext_ref = 1;
#endif
	H5VL_logi_inc_ref (this->uvlid);
}
H5VL_log_obj_t::H5VL_log_obj_t (struct H5VL_log_obj_t *pp, H5I_type_t type) : H5VL_log_obj_t (pp) {
	this->type = type;
}
H5VL_log_obj_t::H5VL_log_obj_t (struct H5VL_log_obj_t *pp, H5I_type_t type, void *uo)
	: H5VL_log_obj_t (pp, type) {
	this->uo = uo;
}
H5VL_log_obj_t::~H5VL_log_obj_t () {
	if (this->fp && (this->fp != this)) { H5VL_log_filei_dec_ref (this->fp); }
	if (this->uvlid >= 0) {
#ifdef LOGVOL_DEBUG
		this->ext_ref--;
		assert (this->ext_ref >= 0);
#endif
		H5VL_logi_dec_ref (this->uvlid);
	}
}