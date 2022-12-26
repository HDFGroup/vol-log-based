/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#pragma once

#include "H5VL_log_file.hpp"
#include "H5VL_logi.hpp"

#define H5VL_FILEI_CONFIG_METADATA_MERGE      0x01
#define H5VL_FILEI_CONFIG_SEL_ENCODE          0x02
#define H5VL_FILEI_CONFIG_SEL_DEFLATE         0x04
#define H5VL_FILEI_CONFIG_METADATA_SHARE      0x08
#define H5VL_FILEI_CONFIG_PASSTHRU            0x10

#define H5VL_FILEI_CONFIG_DATA_ALIGN 0x100
#define H5VL_FILEI_CONFIG_SUBFILING  0x200
// Read only the subfile matching the rank of the process
#define H5VL_FILEI_CONFIG_SINGLE_SUBFILE_READ 0x400

#define H5VL_LOG_FILEI_GROUP_LOG "_LOG"
#define H5VL_LOG_FILEI_ATTR      "_int_att"
#define H5VL_LOG_FILEI_NATTR     5
#define H5VL_LOG_FILEI_DSET_META "_md"
#define H5VL_LOG_FILEI_DSET_DATA "_ld"

// File internals

inline void H5VL_log_filei_init_idx (H5VL_log_file_t *fp) {
    switch (fp->index_type) {
        case list:
            fp->idx = new H5VL_logi_array_idx_t (fp);  // Array index
            break;
        case compact:
            fp->idx = new H5VL_logi_compact_idx_t (fp);  // Compact index
            break;
        default:
            fp->idx = NULL;
            break;
    }
    CHECK_PTR (fp->idx)
}

extern void H5VL_log_filei_post_open (H5VL_log_file_t *fp);
extern void H5VL_log_filei_post_create (H5VL_log_file_t *fp);
extern herr_t H5VL_log_filei_dset_visit (hid_t o_id,
                                         const char *name,
                                         const H5O_info_t *object_info,
                                         void *op_data);
extern void H5VL_log_filei_flush (H5VL_log_file_t *fp, hid_t dxplid);
extern void H5VL_log_filei_metaflush (H5VL_log_file_t *fp);
extern void H5VL_log_filei_metaupdate (H5VL_log_file_t *fp);
extern void H5VL_log_filei_metaupdate_part (H5VL_log_file_t *fp, int &md, int &sec);
extern void H5VL_log_filei_balloc (H5VL_log_file_t *fp, size_t size, void **buf);
extern void H5VL_log_filei_bfree (H5VL_log_file_t *fp, void *buf);

extern void H5VL_log_filei_pool_alloc (H5VL_log_buffer_pool_t *p, ssize_t bsize, void **buf);
extern void H5VL_log_filei_pool_init (H5VL_log_buffer_pool_t *p, ssize_t bsize);
extern void H5VL_log_filei_pool_free (H5VL_log_buffer_pool_t *p);
extern void H5VL_log_filei_pool_finalize (H5VL_log_buffer_pool_t *p);

extern void H5VL_log_filei_parse_fapl (H5VL_log_file_t *fp, hid_t faplid);
extern void H5VL_log_filei_parse_fcpl (H5VL_log_file_t *fp, hid_t fcplid);
extern hid_t H5VL_log_filei_get_under_plist (hid_t faplid);

extern void H5VL_log_filei_contig_buffer_init (H5VL_log_contig_buffer_t *bp, size_t init_size);
extern void H5VL_log_filei_contig_buffer_free (H5VL_log_contig_buffer_t *bp);
extern void *H5VL_log_filei_contig_buffer_alloc (H5VL_log_contig_buffer_t *bp, size_t size);
extern void H5VL_log_filei_create_subfile (H5VL_log_file_t *fp,
                                           unsigned flags,
                                           hid_t fapl_id,
                                           hid_t dxpl_id);
extern void H5VL_log_filei_open_subfile (H5VL_log_file_t *fp,
                                         unsigned flags,
                                         hid_t fapl_id,
                                         hid_t dxpl_id);
extern void H5VL_log_filei_parse_strip_info (H5VL_log_file_t *fp);
extern void H5VL_log_filei_calc_node_rank (H5VL_log_file_t *fp);

extern void H5VL_log_filei_close (H5VL_log_file_t *fp);
extern void H5VL_log_filei_inc_ref (H5VL_log_file_t *fp);
extern void H5VL_log_filei_dec_ref (H5VL_log_file_t *fp);

extern void *H5VL_log_filei_wrap (void *uo, H5VL_log_obj_t *cp);
H5VL_log_file_t *H5VL_log_filei_search (const char *path);
void H5VL_log_filei_register (H5VL_log_file_t *fp);
