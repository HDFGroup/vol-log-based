#pragma once

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <array>
#include <string>

#include "H5VL_log.h"
#include "H5VL_log_dataset.hpp"
#include "H5VL_log_obj.hpp"
#include "H5VL_logi.hpp"
#include "H5VL_logi_idx.hpp"
#include "H5VL_logi_nb.hpp"
#include "H5VL_logi_profiling.hpp"

#define LOG_GROUP_NAME "_LOG"

typedef struct H5VL_log_contig_buffer_t {
	char *begin, *end;
	char *cur;
} H5VL_log_contig_buffer_t;

typedef struct H5VL_log_cord_t {
	MPI_Offset cord[H5S_MAX_RANK];
} H5VL_log_cord_t;

/* The log VOL file object */
typedef struct H5VL_log_file_t : H5VL_log_obj_t {
	int rank;
	int np;
	MPI_Comm comm;

	/* Aligned data layout */
	int nnode;
	int nodeid;
	int noderank;
	int prev_rank;
	int next_rank;
	int target_ost;
	size_t ssize;
	int scount;
	MPI_Comm nodecomm;

	int refcnt;
	bool closing;
	unsigned flag;

	hid_t dxplid;

	void *lgp;
	int ndset;
	int nldset;
	int nmdset;

	MPI_File fh;
	int fd;

	std::vector<H5VL_log_wreq_t *> wreqs;
	int nflushed;
	std::vector<H5VL_log_rreq_t> rreqs;

	std::vector<H5VL_log_merged_wreq_t *> mreqs;
	std::vector<H5VL_log_dset_info_t> dsets;

	ssize_t bsize;
	size_t bused;

	ssize_t mbuf_size;

	std::string name;

	// H5VL_log_buffer_pool_t data_buf;
	H5VL_log_contig_buffer_t meta_buf;

	// std::vector<int> lut;
	std::vector<H5VL_logi_array_idx_t> idx;
	bool idxvalid;
	bool metadirty;

	// Configuration flag
	int config;

#ifdef LOGVOL_PROFILING
	//#pragma message ( "C Preprocessor got here!" )
	double tlocal[H5VL_LOG_NTIMER];
	double clocal[H5VL_LOG_NTIMER];
#endif

	H5VL_log_file_t ();
	H5VL_log_file_t (hid_t uvlid);
	H5VL_log_file_t (void *uo, hid_t uvlid);
	//~H5VL_log_file_t ();
} H5VL_log_file_t;

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

void *H5VL_log_file_create (
	const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id, hid_t dxpl_id, void **req);
void *H5VL_log_file_open (
	const char *name, unsigned flags, hid_t fapl_id, hid_t dxpl_id, void **req);
herr_t H5VL_log_file_get (void *file, H5VL_file_get_args_t *args, hid_t dxpl_id, void **req);
herr_t H5VL_log_file_specific (void *file,
							   H5VL_file_specific_args_t *args,
							   hid_t dxpl_id,
							   void **req);
herr_t H5VL_log_file_optional (void *file, H5VL_optional_args_t *args, hid_t dxpl_id, void **req);
herr_t H5VL_log_file_close (void *file, hid_t dxpl_id, void **req);