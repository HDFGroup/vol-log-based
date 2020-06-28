#include "logvol_internal.hpp"

void *H5VL_log_group_open_with_uo(void *obj, void *uo, const H5VL_loc_params_t *loc_params) {
    H5VL_log_obj_t *op = (H5VL_log_obj_t*)obj;
    H5VL_log_group_t *gp;

    /* Check arguments */
    if(loc_params->type != H5VL_OBJECT_BY_SELF) RET_ERR("loc_params->type is not H5VL_OBJECT_BY_SELF")

    gp = new H5VL_log_group_t();
    if (loc_params->obj_type == H5I_FILE) gp->fp = (H5VL_log_file_t*)obj;
    else if (loc_params->obj_type == H5I_GROUP) gp->fp = ((H5VL_log_group_t*)obj)->fp;
    else RET_ERR("container not a file or group")

    gp->uo = uo;
    gp->uvlid = op->uvlid;
    H5Iinc_ref (gp->uvlid);
    gp->type = H5I_GROUP;
    
    return (void *)gp;

err_out:;
    delete gp;

    return NULL;
} /* end H5VL_log_group_open() */