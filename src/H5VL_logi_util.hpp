#pragma once

#include <H5VLconnector.h>

#include "H5VL_log_obj.hpp"

// Utils
extern MPI_Datatype h5t_to_mpi_type (hid_t type_id);
extern void sortreq (int ndim, hssize_t len, MPI_Offset **starts, MPI_Offset **counts);
extern int intersect (int ndim, MPI_Offset *sa, MPI_Offset *ca, MPI_Offset *sb);
extern void mergereq (int ndim, hssize_t *len, MPI_Offset **starts, MPI_Offset **counts);
extern void sortblock (int ndim, hssize_t len, hsize_t **starts);
extern bool hlessthan (int ndim, hsize_t *a, hsize_t *b);

template <class A, class B>
int H5VL_logi_vector_cmp (int ndim, A *l, B *r);

extern herr_t H5VL_logi_add_att (H5VL_log_obj_t *op,
								 const char *name,
								 hid_t atype,
								 hid_t mtype,
								 hsize_t len,
								 void *buf,
								 hid_t dxpl_id,
								 void **req);
extern herr_t H5VL_logi_put_att (
	H5VL_log_obj_t *op, const char *name, hid_t mtype, void *buf, hid_t dxpl_id);
extern herr_t H5VL_logi_get_att (
	H5VL_log_obj_t *op, const char *name, hid_t mtype, void *buf, hid_t dxpl_id);
extern herr_t H5VL_logi_get_att_ex (
	H5VL_log_obj_t *op, const char *name, hid_t mtype, hsize_t *len, void *buf, hid_t dxpl_id);

MPI_Datatype H5VL_logi_get_mpi_type_by_size (size_t size);