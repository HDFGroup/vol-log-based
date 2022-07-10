/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#pragma once

#include <H5VLconnector.h>
#include <mpi.h>

#include <vector>

#include "H5VL_logi_dataspace.hpp"
#include "H5VL_logi_meta.hpp"

#define H5VL_LOGI_META_FLAG_MUL_SEL     0x01
#define H5VL_LOGI_META_FLAG_MUL_SELX    0x02
#define H5VL_LOGI_META_FLAG_SEL_ENCODE  0x04
#define H5VL_LOGI_META_FLAG_SEL_DEFLATE 0x08
#define H5VL_LOGI_META_FLAG_SEL_REF     0x10
#define H5VL_LOGI_META_FLAG_REC         0x20

typedef struct H5VL_log_req_data_block_t {
    char *ubuf;   // User buffer
    char *xbuf;   // I/O buffer, always continguous and the same format as the dataset
    size_t size;  // size of xbuf
} H5VL_log_req_data_block_t;

class H5VL_log_wreq_t {
   public:
    H5VL_logi_meta_hdr *hdr;  // Location of header in meta_buf
    // int ndim;  // Dim of the target dataset
    // MPI_Offset start[H5S_MAX_RANK];
    // MPI_Offset count[H5S_MAX_RANK];
    // int flag;
    // std::vector<H5VL_log_selection> sels;  // Selections within the dataset

    // H5VL_logi_meta_hdr *meta_hdr;
    char *meta_buf = NULL;  // Encoded metadata
    char *sel_buf  = NULL;  // Location of selection in meta_buf
    int nsel;               // Number of selections
    // size_t meta_size;

    MPI_Offset
        meta_off;  // Offset of the metadata related to the starting metadata block of the process

    std::vector<H5VL_log_req_data_block_t> dbufs;  // Data buffers <xbuf, ubuf, size>

    size_t operator() () const;
    bool operator== (H5VL_log_wreq_t &rhs) const;
    bool operator== (const H5VL_log_wreq_t rhs) const;

    void resize (size_t size);

    H5VL_log_wreq_t ();
    H5VL_log_wreq_t (const H5VL_log_wreq_t &obj);
    H5VL_log_wreq_t (void *dp, H5VL_log_selections *sels);
    ~H5VL_log_wreq_t ();
};
template <>
struct std::hash<H5VL_log_wreq_t> {
    size_t operator() (H5VL_log_wreq_t const &s) const noexcept;
};

struct H5VL_log_dset_t;
class H5VL_log_selections;
class H5VL_log_merged_wreq_t : public H5VL_log_wreq_t {
   public:
    char *mbufp = NULL;      // Next empty byte in the metadata buffer
    char *mbufe = NULL;      // End of metadata buffer
    size_t meta_size_alloc;  // Size of allocated meta_buf

    H5VL_log_merged_wreq_t ();
    H5VL_log_merged_wreq_t (H5VL_log_file_t *fp, int id, int nsel);
    H5VL_log_merged_wreq_t (H5VL_log_dset_t *dp, int nsel);
    ~H5VL_log_merged_wreq_t ();

    void append (H5VL_log_dset_t *dp,
                 H5VL_log_req_data_block_t &db,
                 H5VL_log_selections *sels);  // Append new requests into this reqeust
    void reset (H5VL_log_dset_info_t &dset);

   private:
    void init (H5VL_log_dset_t *dp, int nsel);
    void init (H5VL_log_file_t *fp, int id, int nsel);
    void reserve (size_t size);  // Allocate meta_buf of size at least size
};
class H5VL_log_rreq_t {
   public:
    H5VL_logi_meta_hdr hdr;
    H5VL_log_dset_info_t *info;
    // int did;							   // Source dataset ID
    int ndim;                          // Dim of the source dataset
    H5VL_log_selections *sels = NULL;  // Selections within the dataset
    // MPI_Offset start[H5S_MAX_RANK];
    // MPI_Offset count[H5S_MAX_RANK];

    hid_t dtype;  // Dataset type
    hid_t mtype;  // Memory type

    size_t rsize;  // Number of data elements in xbuf
    size_t esize;  // Size of a data element in xbuf

    char *ubuf;  // I/O buffer, always continguous and the same format as the dataset
    char *xbuf;  // User buffer

    MPI_Datatype ptype;  // Datatype that represetn memory space selection

    H5VL_log_rreq_t ();
    ~H5VL_log_rreq_t ();
};

void H5VL_log_nb_flush_read_reqs (void *file, std::vector<H5VL_log_rreq_t *> &reqs, hid_t dxplid);
void H5VL_log_nb_perform_read (H5VL_log_file_t *fp,
                               std::vector<H5VL_log_rreq_t *> &reqs,
                               hid_t dxplid);
void H5VL_log_nb_flush_write_reqs (void *file, hid_t dxplid);
void H5VL_log_nb_ost_write (void *file, off_t doff, off_t off, int cnt, int *mlens, off_t *moffs);
void H5VL_log_nb_flush_write_reqs_align (void *file, hid_t dxplid);
