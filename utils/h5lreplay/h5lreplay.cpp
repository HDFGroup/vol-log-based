/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
//
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
//
#include <dirent.h>
#include <hdf5.h>
#include <mpi.h>
#include <unistd.h>
#include <libgen.h>
//
#include "H5VL_log_file.hpp"
#include "H5VL_log_filei.hpp"
#include "H5VL_logi_nb.hpp"
#include "h5lreplay.hpp"
#include "h5lreplay_copy.hpp"
#include "h5lreplay_data.hpp"
#include "h5lreplay_meta.hpp"

const char hdf5sig[] = {(char)0x89, (char)0x48, (char)0x44, (char)0x46,
                        (char)0x0d, (char)0x0a, (char)0x1a, (char)0x0a};

int main (int argc, char *argv[]) {
    herr_t err = 0;
    int rank, np;
    int opt;
    std::string inpath, outpath;
    std::ifstream fin;  // File stream for reading file signature
    char sig[8];        // File signature

    MPI_Init (&argc, &argv);
    MPI_Comm_size (MPI_COMM_WORLD, &np);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);

    // Parse input
    while ((opt = getopt (argc, argv, "hi:o:")) != -1) {
        switch (opt) {
            case 'i':
                inpath = std::string (optarg);
                break;
            case 'o':
                outpath = std::string (optarg);
                break;
            case 'h':
            default:
                if (rank == 0) {
                    std::cout << "Usage: h5lreplay -i <input file path> -o <output file path>"
                              << std::endl;
                }
                return 0;
        }
    }

    // Make sure input file is HDF5 file
    if (!rank) {
        fin.open (inpath, std::ios_base::in);
        if (fin.is_open ()) {
            memset (sig, 0, sizeof (sig));
            fin.read (sig, 8);
            if (memcmp (hdf5sig, sig, 8)) {
                std::cout << "Error: " << inpath << " is not a valid HDF5 file." << std::endl;
                err = -1;
            }
        } else {
            std::cout << "Error: cannot open " << inpath << std::endl;
            err = -1;
        }
    }
    MPI_Bcast (&err, 1, MPI_INT, 0, MPI_COMM_WORLD);

    try {
        h5lreplay_core (inpath, outpath, rank, np);
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err == 0 ? 0 : -1;
}

/*----< split_comm() >-------------------------------------------------------*/
static int split_comm (MPI_Comm orig_comm, MPI_Comm *sub_comm, int *num_subfiles, int *subfile_ID) {
    int mpierr, color, orig_rank, sub_nprocs, sub_rank, int_msg[2];
    MPI_Comm comm_roots;

    MPI_Comm_rank (orig_comm, &orig_rank);

    /* split communicator to create one sub-communicator per compute node */
    mpierr =
        MPI_Comm_split_type (orig_comm, MPI_COMM_TYPE_SHARED, orig_rank, MPI_INFO_NULL, sub_comm);
    CHECK_MPIERR

    /* calculate subfile ID and take care of both process rank assignments:
     * block-based (MPICH_RANK_REORDER_METHOD=1) or
     * round-robin (MPICH_RANK_REORDER_METHOD=0)
     */
    MPI_Comm_size (*sub_comm, &sub_nprocs);
    MPI_Comm_rank (*sub_comm, &sub_rank);
    color  = (sub_rank == 0) ? 1 : 0;
    mpierr = MPI_Comm_split (orig_comm, color, orig_rank, &comm_roots);
    CHECK_MPIERR

    MPI_Comm_size (comm_roots, num_subfiles);
    MPI_Comm_rank (comm_roots, subfile_ID);
    MPI_Comm_free (&comm_roots);

    int_msg[0] = *num_subfiles;
    int_msg[1] = *subfile_ID;
    mpierr     = MPI_Bcast (int_msg, 2, MPI_INT, 0, *sub_comm);
    CHECK_MPIERR
    *num_subfiles = int_msg[0];
    *subfile_ID   = int_msg[1];

    return (mpierr != MPI_SUCCESS);
}

void h5lreplay_core (std::string &inpath, std::string &outpath, int rank, int np) {
    herr_t err = 0;
    int mpierr;
    int i;
    hid_t nativevlid;   // Native VOL ID
    hid_t finid  = -1;  // ID of the input file
    hid_t foutid = -1;  // ID of the output file
    hid_t fsubid = -1;  // ID of the subfile
    hid_t faplid = -1;
    hid_t lgid   = -1;  // ID of the log group
    hid_t aid    = -1;  // ID of file attribute
    hid_t dxplid = -1;
    hsize_t n;  // Attr iterate idx position
    int ndset;  // # dataset in the input file
    // int nldset;		 // # data dataset in the current file (main file| subfile)
    int nmdset;      // # metadata dataset in the current file (main file| subfile)
    int config;      // Config flags of the input file
    int att_buf[5];  // Temporary buffer for reading file attributes
    h5lreplay_copy_handler_arg copy_arg;  // File structure
    std::vector<h5lreplay_idx_t>
        reqs;                          // Requests recorded in the current file (main file| subfile)
    MPI_File fin  = MPI_FILE_NULL;     // file handle of the input file
    MPI_File fout = MPI_FILE_NULL;     // file handle of the output file
    MPI_File fsub = MPI_FILE_NULL;     // file handle of the subfile
    int nsubfiles;                     // number of subfiles
    int nnode;                         // number of nodes replaying the file
    int subid;                         // id of the node
    int subrank;                       // rank within the node
    int subnp;                         // number of processes within the node
    MPI_Comm subcomm = MPI_COMM_NULL;  // communicator within the node
    H5VL_logi_err_finally finally ([&] () -> void {
        if (faplid >= 0) { H5Pclose (faplid); }
        if (dxplid >= 0) { H5Pclose (dxplid); }
        if (aid >= 0) { H5Aclose (aid); }
        if (lgid >= 0) { H5Gclose (lgid); }
        if (finid >= 0) { H5Fclose (finid); }
        if (fsubid >= 0) { H5Fclose (fsubid); }
        for (auto &dset : copy_arg.dsets) { H5Dclose (dset.id); }
        if (foutid >= 0) { H5Fclose (foutid); }
        if (fin != MPI_FILE_NULL) { MPI_File_close (&fin); }
        if (fout != MPI_FILE_NULL) { MPI_File_close (&fout); }
        if (subcomm != MPI_COMM_NULL) { MPI_Comm_free (&subcomm); }
    });

    // Open the input and output file
    nativevlid = H5VLpeek_connector_id_by_name ("native");
    faplid     = H5Pcreate (H5P_FILE_ACCESS);
    CHECK_ID (faplid)
    err = H5Pset_fapl_mpio (faplid, MPI_COMM_WORLD, MPI_INFO_NULL);
    CHECK_ERR
    err = H5Pset_vol (faplid, nativevlid, NULL);
    CHECK_ERR

    finid = H5Fopen (inpath.c_str (), H5F_ACC_RDONLY, faplid);
    CHECK_ID (faplid)
    foutid = H5Fcreate (outpath.c_str (), H5F_ACC_TRUNC, H5P_DEFAULT, faplid);
    CHECK_ID (foutid)
    mpierr = MPI_File_open (MPI_COMM_WORLD, inpath.c_str (), MPI_MODE_RDONLY, MPI_INFO_NULL, &fin);
    CHECK_MPIERR
    mpierr =
        MPI_File_open (MPI_COMM_WORLD, outpath.c_str (), MPI_MODE_WRONLY, MPI_INFO_NULL, &fout);
    CHECK_MPIERR

    // Read file metadata
    aid = H5Aopen (finid, H5VL_LOG_FILEI_ATTR_INT, H5P_DEFAULT);
    CHECK_ID (aid)

    dxplid = H5Pcreate (H5P_DATASET_XFER);
    CHECK_ID (dxplid)
    err = H5Pset_dxpl_mpio (dxplid, H5FD_MPIO_COLLECTIVE);
    CHECK_ERR
    err = H5Aread (aid, H5T_NATIVE_INT, att_buf);
    CHECK_ERR
    ndset = att_buf[0];
    // nldset = att_buf[1];
    nmdset = att_buf[2];
    config = att_buf[3];

    // Copy attributes
    n   = 0;
    err = H5Aiterate2 (finid, H5_INDEX_NAME, H5_ITER_INC, &n, h5lreplay_attr_copy_handler,
                       (void *)foutid);
    CHECK_ERR

    // Copy data objects
    copy_arg.dsets.resize (ndset);
    copy_arg.fid = foutid;
    err = H5Ovisit3 (finid, H5_INDEX_CRT_ORDER, H5_ITER_INC, h5lreplay_copy_handler, &copy_arg,
                     H5O_INFO_ALL);
    CHECK_ERR

    reqs.resize (ndset);
    if (config & H5VL_FILEI_CONFIG_SUBFILING) {
        std::string subpath;

        // Split the communicator according to nodes
        nsubfiles = att_buf[4];
        err       = split_comm (MPI_COMM_WORLD, &subcomm, &nnode, &subrank);
        CHECK_ERR
        MPI_Comm_size (subcomm, &subnp);

        // Close attr in main file
        H5Aclose (aid);
        aid = -1;

        // Iterate subdir
        for (i = 0; i < nsubfiles; i++) {
            subpath = inpath + ".subfiles" + std::string (basename ((char *)(inpath.c_str ()))) +
                      "." + std::to_string (i + subid);

            // Clean up the index
            for (auto &r : reqs) { r.clear (); }

            // Open the subfile
            if (i + subid < nsubfiles) {
                fsubid = H5Fopen (subpath.c_str (), H5F_ACC_RDONLY, faplid);
                CHECK_ID (fsubid)

                mpierr = MPI_File_open (subcomm, subpath.c_str (), MPI_MODE_RDONLY, MPI_INFO_NULL,
                                        &fsub);
                CHECK_MPIERR

                // Read file metadata
                aid = H5Aopen (fsubid, H5VL_LOG_FILEI_ATTR_INT,
                               H5P_DEFAULT);  // Open attr in subfile
                CHECK_ID (aid)
                err = H5Aread (aid, H5T_NATIVE_INT, att_buf);
                CHECK_ERR
                // nldset = att_buf[1];
                nmdset = att_buf[2];

                // Open the log group
                lgid = H5Gopen2 (fsubid, H5VL_LOG_FILEI_GROUP_LOG, H5P_DEFAULT);
                CHECK_ID (lgid)

                // Read the metadata
                h5lreplay_parse_meta (subrank, subnp, lgid, nmdset, copy_arg.dsets, reqs, config);

                // Read the data
                h5lreplay_read_data (fsub, copy_arg.dsets, reqs);
            }

            // Write the data
            h5lreplay_write_data (foutid, copy_arg.dsets, reqs);

            if (i + subid < nsubfiles) {
                // Close the subfile
                MPI_File_close (&fsub);
                fsub = MPI_FILE_NULL;
                H5Fclose (fsubid);
                fsubid = -1;
                H5Gclose (lgid);
                lgid = -1;
                H5Aclose (aid);
                aid = -1;
            }
        }
    } else {
        // Open the log group
        lgid = H5Gopen2 (finid, H5VL_LOG_FILEI_GROUP_LOG, H5P_DEFAULT);
        CHECK_ID (lgid)

        // Read the metadata
        h5lreplay_parse_meta (rank, np, lgid, nmdset, copy_arg.dsets, reqs, config);

        // Read the data
        h5lreplay_read_data (fin, copy_arg.dsets, reqs);

        // Write the data
        h5lreplay_write_data (foutid, copy_arg.dsets, reqs);
    }
}
