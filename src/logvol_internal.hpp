#pragma once
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <H5VLpublic.h>
#include <assert.h>
#include <hdf5.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <vector>
#include <array>

#include "logvol.h"

#ifdef LOGVOL_PROFILING
#include "logvol_profiling.hpp"
#define TIMER_START          \
	{                        \
		double tstart, tend; \
		tstart = MPI_Wtime ();
#define TIMER_STOP(A, B)                             \
	tend = MPI_Wtime ();                             \
	H5VL_log_profile_add_time (A, B, tend - tstart); \
	}
#else
#define TIMER_START \
	{}
#define TIMER_STOP(A, B) \
	{}
#endif

#ifdef LOGVOL_DEBUG
#include <iostream>
#define DEBUG_ABORT                                        \
	{                                                      \
		char *val = getenv ("LOGVOL_DEBUG_ABORT_ON_ERR");  \
		if (val && (strcmp (val, "1") == 0)) { abort (); } \
	}
//#define LOGVOL_VERBOSE_DEBUG 1
#else
#define DEBUG_ABORT
#endif

#define LOG_GROUP_NAME "_LOG"
#ifdef LOGVOL_DEBUG
#define LOG_VOL_ASSERT(A) assert (A);
#else
#define LOG_VOL_ASSERT(A) \
	{}
#endif

#define LOGVOL_SELCTION_TYPE_HYPERSLABS 0x01
#define LOGVOL_SELCTION_TYPE_POINTS 0x02
#define LOGVOL_SELCTION_TYPE_OFFSETS 0x04

#define CHECK_ERR                                                     \
	{                                                                 \
		if (err < 0) {                                                \
			printf ("Error at line %d in %s:\n", __LINE__, __FILE__); \
			H5Eprint1 (stdout);                                       \
			DEBUG_ABORT                                               \
			goto err_out;                                             \
		}                                                             \
	}

#define CHECK_MPIERR                                                             \
	{                                                                            \
		if (mpierr != MPI_SUCCESS) {                                             \
			int el = 256;                                                        \
			char errstr[256];                                                    \
			MPI_Error_string (mpierr, errstr, &el);                              \
			printf ("Error at line %d in %s: %s\n", __LINE__, __FILE__, errstr); \
			err = -1;                                                            \
			DEBUG_ABORT                                                          \
			goto err_out;                                                        \
		}                                                                        \
	}

#define CHECK_ID(A)                                                   \
	{                                                                 \
		if (A < 0) {                                                  \
			printf ("Error at line %d in %s:\n", __LINE__, __FILE__); \
			H5Eprint1 (stdout);                                       \
			DEBUG_ABORT                                               \
			goto err_out;                                             \
		}                                                             \
	}

#define CHECK_NERR(A)                                                 \
	{                                                                 \
		if (A == NULL) {                                              \
			printf ("Error at line %d in %s:\n", __LINE__, __FILE__); \
			H5Eprint1 (stdout);                                       \
			DEBUG_ABORT                                               \
			goto err_out;                                             \
		}                                                             \
	}

#define RET_ERR(A)                                                      \
	{                                                                   \
		printf ("Error at line %d in %s: %s\n", __LINE__, __FILE__, A); \
		DEBUG_ABORT                                                     \
		goto err_out;                                                   \
	}

#define H5VL_log_delete_arr(A) \
	{                          \
		delete[] A;            \
		A = NULL;              \
	}
#define H5VL_log_free(A) \
	{                    \
		free (A);        \
		A = NULL;        \
	}
#define H5VL_log_Sclose(A)         \
	{                              \
		if (A != -1) H5Sclose (A); \
	}
#define H5VL_log_Tclose(A)         \
	{                              \
		if (A != -1) H5Tclose (A); \
	}
#define H5VL_log_type_free(A)         \
	{                                 \
		if (A != MPI_DATATYPE_NULL) { \
			MPI_Type_free (&A);       \
			A = MPI_DATATYPE_NULL;    \
		}                             \
	}

typedef struct H5VL_log_search_ret_t {
	int ndim;
	int fsize[H5S_MAX_RANK];
	int fstart[H5S_MAX_RANK];
	int msize[H5S_MAX_RANK];
	int mstart[H5S_MAX_RANK];
	int count[H5S_MAX_RANK];
	size_t esize;
	MPI_Offset foff, moff;

	bool operator< (const H5VL_log_search_ret_t &rhs) const {
		int i;

		if (foff < rhs.foff)
			return true;
		else if (foff > rhs.foff)
			return false;

		for (i = 0; i < ndim; i++) {
			if (fstart[i] < rhs.fstart[i])
				return true;
			else if (fstart[i] > rhs.fstart[i])
				return false;
		}

		return false;
	}
} H5VL_log_search_ret_t;

typedef struct H5VL_log_copy_ctx {
	char *src;
	char *dst;
	size_t size;
} H5VL_log_copy_ctx;

typedef struct H5VL_log_metaentry_t {
	int did;
	MPI_Offset start[H5S_MAX_RANK];
	MPI_Offset count[H5S_MAX_RANK];
	MPI_Offset ldoff;
	size_t rsize;
} H5VL_log_metaentry_t;

typedef struct H5VL_log_selection {
	MPI_Offset start[H5S_MAX_RANK];	 // Start of selection
	MPI_Offset count[H5S_MAX_RANK];	 // Count of selection
	size_t size;					 // Size of the selection (bytes)
} H5VL_log_selection;

typedef struct H5VL_log_selection2 {
	MPI_Offset start;	 // Start of selection
	MPI_Offset end;	 // Count of selection
	size_t size;					 // Size of the selection (bytes)
} H5VL_log_selection2;

typedef struct H5VL_log_wreq_t {
	int did;   // Target dataset ID
	//int ndim;  // Dim of the target dataset
	// MPI_Offset start[H5S_MAX_RANK];
	// MPI_Offset count[H5S_MAX_RANK];
	int flag;
	std::vector<H5VL_log_selection> sels;  // Selections within the dataset

	// int nsel;

	int ldid;		   // Log dataset ID
	MPI_Offset ldoff;  // Offset in log dataset

	size_t rsize;  // Size of data in xbuf (bytes)
	char *xbuf;	   // I/O buffer, always continguous and the same format as the dataset
	char *ubuf;	   // User buffer
				   // int buf_alloc;  // Whether the buffer is allocated or
} H5VL_log_wreq_t;

typedef struct H5VL_log_rreq_t {
	int did;							   // Source dataset ID
	int ndim;							   // Dim of the source dataset
	std::vector<H5VL_log_selection> sels;  // Selections within the dataset
	// MPI_Offset start[H5S_MAX_RANK];
	// MPI_Offset count[H5S_MAX_RANK];

	hid_t dtype;  // Dataset type
	hid_t mtype;  // Memory type

	size_t rsize;  // Number of data elements in xbuf
	size_t esize;  // Size of a data element in xbuf

	char *ubuf;	 // I/O buffer, always continguous and the same format as the dataset
	char *xbuf;	 // User buffer

	MPI_Datatype ptype;	 // Datatype that represetn memory space selection
} H5VL_log_rreq_t;

typedef struct H5VL_log_buffer_block_t {
	char *begin, *end;
	char *cur;
	H5VL_log_buffer_block_t *next;
} H5VL_log_buffer_block_t;

typedef struct H5VL_log_buffer_pool_t {
	ssize_t bsize;
	int inf;
	H5VL_log_buffer_block_t *head;
	H5VL_log_buffer_block_t *free_blocks;
} H5VL_log_buffer_pool_t;

typedef struct H5VL_log_contig_buffer_t {
	char *begin, *end;
	char *cur;
} H5VL_log_contig_buffer_t;

struct H5VL_log_file_t;
typedef struct H5VL_log_obj_t {
	H5I_type_t type;
	void *uo;					 // Under obj
	hid_t uvlid;				 // Under VolID
	struct H5VL_log_file_t *fp;	 // File object
} H5VL_log_obj_t;

typedef struct H5VL_log_dset_meta_t {
	// H5VL_log_file_t *fp;
	// int id;
	hsize_t ndim;
	// hsize_t dims[H5S_MAX_RANK];
	// hsize_t mdims[H5S_MAX_RANK];
	hid_t dtype;
	// hsize_t esize;
} H5VL_log_dset_meta_t;

typedef struct H5VL_log_cord_t {
	MPI_Offset cord[H5S_MAX_RANK];
}H5VL_log_cord_t;

/* The log VOL file object */
typedef struct H5VL_log_file_t : H5VL_log_obj_t {
	int rank;
	MPI_Comm comm;

	int refcnt;
	bool closing;
	unsigned flag;

	hid_t dxplid;

	void *lgp;
	int ndset;
	int nldset;

	MPI_File fh;

	std::vector<H5VL_log_wreq_t> wreqs;
	int nflushed;
	std::vector<H5VL_log_rreq_t> rreqs;

	// Should we do metadata caching?
	std::vector<int> ndim;
	std::vector<std::array<MPI_Offset,H5S_MAX_RANK>> dsizes;
	// std::vector<H5VL_log_dset_meta_t> mdc;

	ssize_t bsize;
	size_t bused;

	// H5VL_log_buffer_pool_t data_buf;
	H5VL_log_contig_buffer_t meta_buf;

	// std::vector<int> lut;
	std::vector<std::vector<H5VL_log_metaentry_t>> idx;
	bool idxvalid;
	bool metadirty;

#ifdef LOGVOL_PROFILING
	double tlocal[NTIMER];
	double clocal[NTIMER];
#endif
} H5VL_log_file_t;

/* The log VOL group object */
//typedef struct H5VL_log_obj_t H5VL_log_group_t;

/* The log VOL dataset object */
typedef struct H5VL_log_dset_t : H5VL_log_obj_t {
	int id;
	hsize_t ndim;
	hsize_t dims[H5S_MAX_RANK];
	hsize_t mdims[H5S_MAX_RANK];
	hsize_t dsteps[H5S_MAX_RANK];

	hid_t dtype;
	hsize_t esize;
} H5VL_log_dset_t;

/* The log VOL wrapper context */
typedef struct H5VL_log_wrap_ctx_t {
	void *uctx;	  // Under context
	hid_t uvlid;  // Under VolID
	H5VL_log_file_t *fp;
} H5VL_log_wrap_ctx_t;

extern int H5VL_log_dataspace_contig_ref;
extern hid_t H5VL_log_dataspace_conti;

extern H5VL_log_obj_t *H5VL_log_new_obj (void *under_obj, hid_t uvlid);
extern herr_t H5VL_log_free_obj (H5VL_log_obj_t *obj);
extern herr_t H5VL_log_init (hid_t vipl_id);
extern herr_t H5VL_log_obj_term (void);
extern void *H5VL_log_info_copy (const void *_info);
extern herr_t H5VL_log_info_cmp (int *cmp_value, const void *_info1, const void *_info2);
extern herr_t H5VL_log_info_free (void *_info);
extern herr_t H5VL_log_info_to_str (const void *_info, char **str);
extern herr_t H5VL_log_str_to_info (const char *str, void **_info);
extern void *H5VL_log_get_object (const void *obj);
extern herr_t H5VL_log_get_wrap_ctx (const void *obj, void **wrap_ctx);
extern void *H5VL_log_wrap_object (void *obj, H5I_type_t obj_type, void *_wrap_ctx);
extern void *H5VL_log_unwrap_object (void *obj);
extern herr_t H5VL_log_free_wrap_ctx (void *_wrap_ctx);

// APIs
extern const H5VL_file_class_t H5VL_log_file_g;
extern const H5VL_dataset_class_t H5VL_log_dataset_g;
extern const H5VL_attr_class_t H5VL_log_attr_g;
extern const H5VL_group_class_t H5VL_log_group_g;
extern const H5VL_introspect_class_t H5VL_log_introspect_g;
extern const H5VL_object_class_t H5VL_log_object_g;
extern const H5VL_datatype_class_t H5VL_log_datatype_g;
extern const H5VL_blob_class_t H5VL_log_blob_g;
extern const H5VL_link_class_t H5VL_log_link_g;
extern const H5VL_token_class_t H5VL_log_token_g;
extern const H5VL_wrap_class_t H5VL_log_wrap_g;

// Utils
extern MPI_Datatype h5t_to_mpi_type (hid_t type_id);
extern void sortreq (int ndim, hssize_t len, MPI_Offset **starts, MPI_Offset **counts);
extern int intersect (int ndim, MPI_Offset *sa, MPI_Offset *ca, MPI_Offset *sb);
extern void mergereq (int ndim, hssize_t *len, MPI_Offset **starts, MPI_Offset **counts);
extern void sortblock (int ndim, hssize_t len, hsize_t **starts);
extern bool hlessthan (int ndim, hsize_t *a, hsize_t *b);

template <class A, class B>
int H5VL_logi_vector_cmp (int ndim, A *l, B *r);

extern herr_t H5VLattr_get_wrapper (
	void *obj, hid_t connector_id, H5VL_attr_get_t get_type, hid_t dxpl_id, void **req, ...);
extern herr_t H5VL_logi_add_att (H5VL_log_obj_t *op,
								 const char *name,
								 hid_t atype,
								 hid_t mtype,
								 hsize_t len,
								 void *buf,
								 hid_t dxpl_id);
extern herr_t H5VL_logi_put_att (
	H5VL_log_obj_t *op, const char *name, hid_t mtype, void *buf, hid_t dxpl_id);
extern herr_t H5VL_logi_get_att (
	H5VL_log_obj_t *op, const char *name, hid_t mtype, void *buf, hid_t dxpl_id);
extern herr_t H5VL_logi_get_att_ex (
	H5VL_log_obj_t *op, const char *name, hid_t mtype, hsize_t *len, void *buf, hid_t dxpl_id);

// File internals
extern herr_t H5VL_log_filei_flush (H5VL_log_file_t *fp, hid_t dxplid);
extern herr_t H5VL_log_filei_metaflush (H5VL_log_file_t *fp);
extern herr_t H5VL_log_filei_metaupdate (H5VL_log_file_t *fp);
extern herr_t H5VL_log_filei_balloc (H5VL_log_file_t *fp, size_t size, void **buf);
extern herr_t H5VL_log_filei_bfree (H5VL_log_file_t *fp, void *buf);

extern herr_t H5VL_log_filei_pool_alloc (H5VL_log_buffer_pool_t *p, size_t bsize, void **buf);
extern herr_t H5VL_log_filei_pool_init (H5VL_log_buffer_pool_t *p, ssize_t bsize);
extern herr_t H5VL_log_filei_pool_free (H5VL_log_buffer_pool_t *p);
extern herr_t H5VL_log_filei_pool_finalize (H5VL_log_buffer_pool_t *p);

extern herr_t H5VL_log_filei_contig_buffer_init (H5VL_log_contig_buffer_t *bp, size_t init_size);
extern void H5VL_log_filei_contig_buffer_free (H5VL_log_contig_buffer_t *bp);
extern void *H5VL_log_filei_contig_buffer_alloc (H5VL_log_contig_buffer_t *bp, size_t size);

extern herr_t H5VL_log_filei_close (H5VL_log_file_t *fp);
extern void H5VL_log_filei_inc_ref (H5VL_log_file_t *fp);
extern herr_t H5VL_log_filei_dec_ref (H5VL_log_file_t *fp);

// Dataspace internals
extern herr_t H5VL_log_dataspacei_get_selection (hid_t sid, std::vector<H5VL_log_selection> &sels);
extern herr_t H5VL_log_dataspacei_get_sel_type (hid_t sid, size_t esize, MPI_Datatype *type);

// Datatype internals
extern void *H5VL_log_datatype_open_with_uo (void *obj, void *uo, const H5VL_loc_params_t *loc_params);

// Wraper
extern herr_t H5VLdataset_specific_wrapper (void *obj,
											hid_t connector_id,
											H5VL_dataset_specific_t specific_type,
											hid_t dxpl_id,
											void **req,
											...);
extern herr_t H5VLdataset_get_wrapper (
	void *obj, hid_t connector_id, H5VL_dataset_get_t get_type, hid_t dxpl_id, void **req, ...);
extern herr_t H5VLdataset_optional_wrapper (void *obj,
											hid_t connector_id,
											H5VL_dataset_optional_t opt_type,
											hid_t dxpl_id,
											void **req,
											...);
extern herr_t H5VLfile_optional_wrapper (
	void *obj, hid_t connector_id, H5VL_file_optional_t opt_type, hid_t dxpl_id, void **req, ...);
extern herr_t H5VLlink_specific_wrapper (void *obj,
										 const H5VL_loc_params_t *loc_params,
										 hid_t connector_id,
										 H5VL_link_specific_t specific_type,
										 hid_t dxpl_id,
										 void **req,
										 ...);
extern herr_t H5VLobject_get_wrapper (void *obj,
									  const H5VL_loc_params_t *loc_params,
									  hid_t connector_id,
									  H5VL_object_get_t get_type,
									  hid_t dxpl_id,
									  void **req,
									  ...);

// Dataset util
extern herr_t H5VL_log_dataset_readi_gen_rtypes (std::vector<H5VL_log_search_ret_t> blocks,
												 MPI_Datatype *ftype,
												 MPI_Datatype *mtype,
												 std::vector<H5VL_log_copy_ctx> &overlaps);

// Dataset internal
void *H5VL_log_dataset_open_with_uo (void *obj,
									 void *uo,
									 const H5VL_loc_params_t *loc_params,
									 hid_t dxpl_id);
void *H5VL_log_dataset_wrap (void *uo, H5VL_log_wrap_ctx_t *cp);

// Nonblocking
extern herr_t H5VL_log_nb_flush_read_reqs (H5VL_log_file_t *fp,
										   std::vector<H5VL_log_rreq_t> reqs,
										   hid_t dxplid);
extern herr_t H5VL_log_nb_flush_write_reqs (H5VL_log_file_t *fp, hid_t dxplid);

// Datatype
// extern herr_t H5VL_log_dtypei_convert_core(void *inbuf, void *outbuf, hid_t intype, hid_t
// outtype, int N); extern herr_t H5VL_log_dtypei_convert(void *inbuf, void *outbuf, hid_t intype,
// hid_t outtype, int N);
extern MPI_Datatype H5VL_log_dtypei_mpitype_by_size (size_t size);

// Index
extern herr_t H5VL_logi_idx_search (H5VL_log_file_t *fp,
									H5VL_log_rreq_t &req,
									std::vector<H5VL_log_search_ret_t> &ret);

// Property
extern herr_t H5Pset_nb_buffer_size (hid_t plist, size_t size);
extern herr_t H5Pget_nb_buffer_size (hid_t plist, ssize_t *size);
extern herr_t H5Pset_nonblocking (hid_t plist, int nonblocking);
extern herr_t H5Pget_nonblocking (hid_t plist, int *nonblocking);

// Internal
extern void *H5VL_log_obj_open_with_uo (void *obj, void *uo, H5I_type_t type, const H5VL_loc_params_t *loc_params);

#ifdef LOGVOL_DEBUG
extern int H5VL_log_debug_MPI_Type_create_subarray (int ndims,
													const int array_of_sizes[],
													const int array_of_subsizes[],
													const int array_of_starts[],
													int order,
													MPI_Datatype oldtype,
													MPI_Datatype *newtype);
extern void hexDump (char *desc, void *addr, size_t len, char *fname);
extern void hexDump (char *desc, void *addr, size_t len);
extern void hexDump (char *desc, void *addr, size_t len, FILE *fp);
#else
#define H5VL_log_debug_MPI_Type_create_subarray MPI_Type_create_subarray
#endif

#ifdef LOGVOL_PROFILING
void H5VL_log_profile_add_time (H5VL_log_file_t *fp, int id, double t);
void H5VL_log_profile_sub_time (H5VL_log_file_t *fp, int id, double t);
void H5VL_log_profile_print (H5VL_log_file_t *fp);
void H5VL_log_profile_reset (H5VL_log_file_t *fp);
#endif