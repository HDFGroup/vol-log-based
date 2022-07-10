/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#pragma once

#include <hdf5.h>
#include <mpi.h>

#include "H5VL_logi_nb.hpp"

enum H5VL_log_idx_type_t { list = 0, compact = 1 };

struct H5VL_log_dset_info_t;
typedef struct H5VL_log_idx_search_ret_t {
    H5VL_log_dset_info_t *info;
    int dsize[H5S_MAX_RANK];   // Size of the data block in the log entry
    int dstart[H5S_MAX_RANK];  // Starting coordinate of the intersection in the log entry
    int msize[H5S_MAX_RANK];   // Size of the selection block in the memory space
    int mstart[H5S_MAX_RANK];  // Starting coordinate of the intersection in the selected block of
                               // the memory space
    int count[H5S_MAX_RANK];   // Size of the intersection
    MPI_Offset xsize;          // Data size after unfiltering
    char *zbuf = NULL;         // Buffer to the decompressed buf
    MPI_Offset doff;           // Relative offset of the target data in the unfiltered data block
    char *xbuf;                // Where the data should go
    MPI_Offset foff;           // File offset of the data block containing the data
    MPI_Offset fsize;          // File offset nad size of the filtered data

    bool operator< (const H5VL_log_idx_search_ret_t &rhs) const;
    bool operator> (const H5VL_log_idx_search_ret_t &rhs) const;
} H5VL_log_idx_search_ret_t;

typedef struct H5VL_log_metaentry_t {
    int did;                      // Dataset ID
    hsize_t start[H5S_MAX_RANK];  // Start of the selected block
    hsize_t count[H5S_MAX_RANK];  // Size of the selected block
    MPI_Offset foff;              // Offset of data in file
    size_t fsize;                 // Size of data in file
} H5VL_log_metaentry_t;

class H5VL_logi_idx_t {
   protected:
    H5VL_log_file_t *fp;

   public:
    H5VL_logi_idx_t (H5VL_log_file_t *fp);
    virtual ~H5VL_logi_idx_t ()                       = default;
    virtual void clear ()                             = 0;  // Remove all entries
    virtual void reserve (size_t size)                = 0;  // Make space for at least size datasets
    virtual void insert (H5VL_logi_metaentry_t &meta) = 0;  // Add an entry
    virtual void parse_block (
        char *block,
        size_t size) = 0;  // Parse a block of encoded metadata and insert all entries
    virtual void search (H5VL_log_rreq_t *req,
                         std::vector<H5VL_log_idx_search_ret_t> &ret) = 0;  // Search for matchings
};

class H5VL_logi_array_idx_t : public H5VL_logi_idx_t {
    std::vector<std::vector<H5VL_logi_metaentry_t>> idxs;

   public:
    H5VL_logi_array_idx_t (H5VL_log_file_t *fp);
    H5VL_logi_array_idx_t (H5VL_log_file_t *fp, size_t size);
    ~H5VL_logi_array_idx_t () = default;
    void clear ();                              // Remove all entries
    void reserve (size_t size);                 // Make space for at least size datasets
    void insert (H5VL_logi_metaentry_t &meta);  // Add an entry
    void parse_block (char *block,
                      size_t size);  // Parse a block of encoded metadata and insert all entries
    void search (H5VL_log_rreq_t *req,
                 std::vector<H5VL_log_idx_search_ret_t> &ret);  // Search for matchings
};

class H5VL_logi_compact_idx_t : public H5VL_logi_idx_t {
    class H5VL_logi_compact_idx_entry_t {
       public:
        hssize_t rec = -1;  // record number, -1 for non-record
        void
            *blocks;  // Start and count pairs of the selected blocks, or reference to other entries
        int nsel;     // # selections, -1 for ref entry
        MPI_Offset foff;  // Offset of data in file
        size_t fsize;     // Size of data in file
        size_t dsize;

        // H5VL_logi_compact_idx_entry_t (MPI_Offset foff, size_t fsize, int ndim, int nsel);
        H5VL_logi_compact_idx_entry_t (MPI_Offset foff,
                                       size_t fsize,
                                       H5VL_logi_compact_idx_entry_t *ref);
        H5VL_logi_compact_idx_entry_t (int ndim, H5VL_logi_metaentry_t &meta);
        ~H5VL_logi_compact_idx_entry_t ();
    };

    std::vector<std::vector<H5VL_logi_compact_idx_entry_t *>> idxs;

   public:
    H5VL_logi_compact_idx_t (H5VL_log_file_t *fp);
    H5VL_logi_compact_idx_t (H5VL_log_file_t *fp, size_t size);
    ~H5VL_logi_compact_idx_t ();
    void clear ();                              // Remove all entries
    void reserve (size_t size);                 // Make space for at least size datasets
    void insert (H5VL_logi_metaentry_t &meta);  // Add an entry
    void parse_block (char *block,
                      size_t size);  // Parse a block of encoded metadata and insert all entries
    void search (H5VL_log_rreq_t *req,
                 std::vector<H5VL_log_idx_search_ret_t> &ret);  // Search for matchings
};
