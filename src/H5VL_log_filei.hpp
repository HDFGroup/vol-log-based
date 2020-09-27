#pragma once

#include "H5VL_log_int.hpp"
#include "H5VL_log_file.hpp"

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