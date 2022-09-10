/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "H5VL_log.h"
#include "H5VL_log_dataset.hpp"
#include "H5VL_log_info.hpp"
#include "H5VL_logi.hpp"

/* The connector identification number, initialized at runtime */
hid_t H5VL_LOG_g = H5I_INVALID_HID;

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_register
 *
 * Purpose:     Register the pass-through VOL connector and retrieve an ID
 *              for it.
 *
 * Return:      Success:    The ID for the pass-through VOL connector
 *              Failure:    -1
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, November 28, 2018
 *
 *-------------------------------------------------------------------------
 */
hid_t H5VL_log_register (void) {
    /* Singleton register the pass-through VOL connector ID */
    if (H5VL_LOG_g < 0)
        H5VL_LOG_g = H5VLregister_connector (&H5VL_log_g, H5P_VOL_INITIALIZE_DEFAULT);

    return H5VL_LOG_g;
} /* end H5VL_log_register() */

/*-------------------------------------------------------------------------
 * Function:    H5Pset_vol_log
 *
 * Purpose:     Set to use the log VOL in a file access property list
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5Pset_vol_log (hid_t fapl_id) {
    herr_t err = 0;
    hid_t logvlid;
    H5VL_log_info_t log_vol_info;

    try {
        // Register the VOL if not yet registered
        logvlid = H5VL_log_register ();
        CHECK_ID (logvlid)

        err = H5Pget_vol_id (fapl_id, &(log_vol_info.uvlid));
        CHECK_ERR
        err = H5Pget_vol_info (fapl_id, &(log_vol_info.under_vol_info));
        CHECK_ERR

        err = H5Pset_vol (fapl_id, logvlid, &log_vol_info);
        CHECK_ERR
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
}

herr_t H5Dwrite_n (hid_t did,
                   hid_t mem_type_id,
                   int n,
                   hsize_t **starts,
                   hsize_t **counts,
                   hid_t dxplid,
                   void *buf) {
    herr_t err = 0;
    H5VL_log_dio_n_arg_t varnarg;
    H5VL_optional_args_t arg;

    try {
        if (dxplid == H5P_DEFAULT) { dxplid = H5P_DATASET_XFER_DEFAULT; }

        varnarg.mem_type_id = mem_type_id;
        varnarg.n           = n;
        varnarg.starts      = starts;
        varnarg.counts      = counts;
        varnarg.buf         = buf;

        arg.op_type = H5Dwrite_n_op_val;
        arg.args    = &varnarg;
        err         = H5VLdataset_optional_op (did, &arg, dxplid, H5ES_NONE);
        CHECK_ERR
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
}

herr_t H5Dread_n (hid_t did,
                  hid_t mem_type_id,
                  int n,
                  hsize_t **starts,
                  hsize_t **counts,
                  hid_t dxplid,
                  void *buf) {
    herr_t err = 0;
    H5VL_log_dio_n_arg_t varnarg;
    H5VL_optional_args_t arg;

    try {
        if (dxplid == H5P_DEFAULT) { dxplid = H5P_DATASET_XFER_DEFAULT; }

        varnarg.mem_type_id = mem_type_id;
        varnarg.n           = n;
        varnarg.starts      = starts;
        varnarg.counts      = counts;
        varnarg.buf         = buf;

        arg.op_type = H5Dread_n_op_val;
        arg.args    = &varnarg;
        err         = H5VLdataset_optional_op (did, &arg, dxplid, H5ES_NONE);
        CHECK_ERR
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
}

#define NB_PROPERTY_NAME "H5VL_log_nonblocking"
herr_t H5Pset_buffered (hid_t plist, hbool_t nonblocking) {
    herr_t err = 0;
    htri_t isdxpl, pexist;

    try {
        isdxpl = H5Pisa_class (plist, H5P_DATASET_XFER);
        CHECK_ID (isdxpl)
        if (isdxpl == 0) ERR_OUT ("Not dxplid")

        pexist = H5Pexist (plist, NB_PROPERTY_NAME);
        CHECK_ID (pexist)
        if (!pexist) {
            hbool_t blocking = false;
            err = H5Pinsert2 (plist, NB_PROPERTY_NAME, sizeof (hbool_t), &blocking, NULL, NULL,
                              NULL, NULL, NULL, NULL);
            CHECK_ERR
        }

        err = H5Pset (plist, NB_PROPERTY_NAME, &nonblocking);
        CHECK_ERR
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
}

herr_t H5Pget_buffered (hid_t plist, hbool_t *nonblocking) {
    herr_t err = 0;
    htri_t isdxpl, pexist;

    try {
        isdxpl = H5Pisa_class (plist, H5P_DATASET_XFER);
        CHECK_ID (isdxpl)
        if (isdxpl == 0)
            *nonblocking = false;  // Default property will not pass class check
        else {
            pexist = H5Pexist (plist, NB_PROPERTY_NAME);
            CHECK_ID (pexist)
            if (pexist) {
                err = H5Pget (plist, NB_PROPERTY_NAME, nonblocking);
                CHECK_ERR

            } else {
                *nonblocking = false;
            }
        }
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
}

#define BSIZE_PROPERTY_NAME "H5VL_log_nb_buffer_size"
herr_t H5Pset_nb_buffer_size (hid_t plist, size_t size) {
    herr_t err = 0;
    htri_t isfapl;
    htri_t pexist;

    try {
        // TODO: Fix pclass problem
        return 0;

        isfapl = H5Pisa_class (plist, H5P_FILE_ACCESS);
        CHECK_ID (isfapl)
        if (isfapl == 0) ERR_OUT ("Not faplid")

        pexist = H5Pexist (plist, BSIZE_PROPERTY_NAME);
        CHECK_ID (pexist)
        if (!pexist) {
            ssize_t infty = LOG_VOL_BSIZE_UNLIMITED;
            err = H5Pinsert2 (plist, BSIZE_PROPERTY_NAME, sizeof (size_t), &infty, NULL, NULL, NULL,
                              NULL, NULL, NULL);
            CHECK_ERR
        }

        err = H5Pset (plist, BSIZE_PROPERTY_NAME, &size);
        CHECK_ERR
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
}

herr_t H5Pget_nb_buffer_size (hid_t plist, ssize_t *size) {
    herr_t err = 0;
    htri_t isdxpl, pexist;

    try {
        isdxpl = H5Pisa_class (plist, H5P_FILE_ACCESS);
        CHECK_ID (isdxpl)
        if (isdxpl == 0)
            *size = LOG_VOL_BSIZE_UNLIMITED;  // Default property will not pass class check
        else {
            pexist = H5Pexist (plist, BSIZE_PROPERTY_NAME);
            CHECK_ID (pexist)
            if (pexist) {
                err = H5Pget (plist, BSIZE_PROPERTY_NAME, size);
                CHECK_ERR

            } else {
                *size = LOG_VOL_BSIZE_UNLIMITED;
            }
        }
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
}

#define IDXSIZE_PROPERTY_NAME "H5VL_log_idx_buffer_size"
herr_t H5Pset_idx_buffer_size (hid_t plist, size_t size) {
    herr_t err = 0;
    htri_t isfapl;
    htri_t pexist;

    try {
        // TODO: Fix pclass problem
        return 0;

        isfapl = H5Pisa_class (plist, H5P_FILE_ACCESS);
        CHECK_ID (isfapl)
        if (isfapl == 0) ERR_OUT ("Not faplid")

        pexist = H5Pexist (plist, IDXSIZE_PROPERTY_NAME);
        CHECK_ID (pexist)
        if (!pexist) {
            ssize_t infty = LOG_VOL_BSIZE_UNLIMITED;
            err = H5Pinsert2 (plist, IDXSIZE_PROPERTY_NAME, sizeof (size_t), &infty, NULL, NULL,
                              NULL, NULL, NULL, NULL);
            CHECK_ERR
        }

        err = H5Pset (plist, IDXSIZE_PROPERTY_NAME, &size);
        CHECK_ERR
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
}

herr_t H5Pget_idx_buffer_size (hid_t plist, ssize_t *size) {
    herr_t err = 0;
    htri_t isdxpl, pexist;

    try {
        isdxpl = H5Pisa_class (plist, H5P_FILE_ACCESS);
        CHECK_ID (isdxpl)
        if (isdxpl == 0)
            *size = LOG_VOL_BSIZE_UNLIMITED;  // Default property will not pass class check
        else {
            pexist = H5Pexist (plist, IDXSIZE_PROPERTY_NAME);
            CHECK_ID (pexist)
            if (pexist) {
                err = H5Pget (plist, IDXSIZE_PROPERTY_NAME, size);
                CHECK_ERR

            } else {
                *size = LOG_VOL_BSIZE_UNLIMITED;
            }
        }
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
}

#define MERGE_META_NAME_PROPERTY_NAME "H5VL_log_metadata_merge"
herr_t H5Pset_meta_merge (hid_t plist, hbool_t merge) {
    herr_t err = 0;
    htri_t isfapl;
    htri_t pexist;

    try {
        // TODO: Fix pclass problem
        return 0;

        isfapl = H5Pisa_class (plist, H5P_FILE_ACCESS);
        CHECK_ID (isfapl)
        if (isfapl == 0) ERR_OUT ("Not faplid")

        pexist = H5Pexist (plist, MERGE_META_NAME_PROPERTY_NAME);
        CHECK_ID (pexist)
        if (!pexist) {
            hbool_t f = false;
            err = H5Pinsert2 (plist, MERGE_META_NAME_PROPERTY_NAME, sizeof (hbool_t), &f, NULL,
                              NULL, NULL, NULL, NULL, NULL);
            CHECK_ERR
        }

        err = H5Pset (plist, MERGE_META_NAME_PROPERTY_NAME, &merge);
        CHECK_ERR
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
}

herr_t H5Pget_meta_merge (hid_t plist, hbool_t *merge) {
    herr_t err = 0;
    htri_t isdxpl, pexist;

    try {
        isdxpl = H5Pisa_class (plist, H5P_FILE_ACCESS);
        CHECK_ID (isdxpl)
        if (isdxpl == 0)
            *merge = false;  // Default property will not pass class check
        else {
            pexist = H5Pexist (plist, MERGE_META_NAME_PROPERTY_NAME);
            CHECK_ID (pexist)
            if (pexist) {
                err = H5Pget (plist, MERGE_META_NAME_PROPERTY_NAME, merge);
                CHECK_ERR

            } else {
                *merge = false;
            }
        }
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
}

#define SHARE_META_NAME_PROPERTY_NAME "H5VL_log_metadata_share"
herr_t H5Pset_meta_share (hid_t plist, hbool_t share) {
    herr_t err = 0;
    htri_t isfapl;
    htri_t pexist;

    try {
        // TODO: Fix pclass problem
        // return 0;

        isfapl = H5Pisa_class (plist, H5P_FILE_ACCESS);
        CHECK_ID (isfapl)
        if (isfapl == 0) ERR_OUT ("Not faplid")

        pexist = H5Pexist (plist, SHARE_META_NAME_PROPERTY_NAME);
        CHECK_ID (pexist)
        if (!pexist) {
            hbool_t f = false;
            err = H5Pinsert2 (plist, SHARE_META_NAME_PROPERTY_NAME, sizeof (hbool_t), &f, NULL,
                              NULL, NULL, NULL, NULL, NULL);
            CHECK_ERR
        }

        err = H5Pset (plist, SHARE_META_NAME_PROPERTY_NAME, &share);
        CHECK_ERR
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
}
herr_t H5Pget_meta_share (hid_t plist, hbool_t *share) {
    herr_t err = 0;
    htri_t isdxpl, pexist;

    try {
        isdxpl = H5Pisa_class (plist, H5P_FILE_ACCESS);
        CHECK_ID (isdxpl)
        if (isdxpl == 0)
            *share = true;  // Default property will not pass class check
        else {
            pexist = H5Pexist (plist, SHARE_META_NAME_PROPERTY_NAME);
            CHECK_ID (pexist)
            if (pexist) {
                err = H5Pget (plist, SHARE_META_NAME_PROPERTY_NAME, share);
                CHECK_ERR

            } else {
                *share = true;
            }
        }
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
}

#define ZIP_META_NAME_PROPERTY_NAME "H5VL_log_metadata_zip"
herr_t H5Pset_meta_zip (hid_t plist, hbool_t zip) {
    herr_t err = 0;
    htri_t isfapl;
    htri_t pexist;

    try {
        // TODO: Fix pclass problem
        // return 0;

        isfapl = H5Pisa_class (plist, H5P_FILE_ACCESS);
        CHECK_ID (isfapl)
        if (isfapl == 0) ERR_OUT ("Not faplid")

        pexist = H5Pexist (plist, ZIP_META_NAME_PROPERTY_NAME);
        CHECK_ID (pexist)
        if (!pexist) {
            hbool_t f = false;
            err = H5Pinsert2 (plist, ZIP_META_NAME_PROPERTY_NAME, sizeof (hbool_t), &f, NULL, NULL,
                              NULL, NULL, NULL, NULL);
            CHECK_ERR
        }

        err = H5Pset (plist, ZIP_META_NAME_PROPERTY_NAME, &zip);
        CHECK_ERR
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
}

herr_t H5Pget_meta_zip (hid_t plist, hbool_t *zip) {
    herr_t err = 0;
    htri_t isdxpl, pexist;

    try {
        isdxpl = H5Pisa_class (plist, H5P_FILE_ACCESS);
        CHECK_ID (isdxpl)
        if (isdxpl == 0)
            *zip = false;  // Default property will not pass class check
        else {
            pexist = H5Pexist (plist, ZIP_META_NAME_PROPERTY_NAME);
            CHECK_ID (pexist)
            if (pexist) {
                err = H5Pget (plist, ZIP_META_NAME_PROPERTY_NAME, zip);
                CHECK_ERR

            } else {
                *zip = false;
            }
        }
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
}

#define SEL_ENCODING_PROPERTY_NAME "H5VL_log_sel_encoding"
herr_t H5Pset_sel_encoding (hid_t plist, H5VL_log_sel_encoding_t encoding) {
    herr_t err = 0;
    htri_t isfapl;
    htri_t pexist;

    try {
        // TODO: Fix pclass problem
        // return 0;

        isfapl = H5Pisa_class (plist, H5P_FILE_ACCESS);
        CHECK_ID (isfapl)
        if (isfapl == 0) ERR_OUT ("Not faplid")

        pexist = H5Pexist (plist, SEL_ENCODING_PROPERTY_NAME);
        CHECK_ID (pexist)
        if (!pexist) {
            H5VL_log_sel_encoding_t offset = H5VL_LOG_ENCODING_OFFSET;
            err = H5Pinsert2 (plist, SEL_ENCODING_PROPERTY_NAME, sizeof (size_t), &offset, NULL,
                              NULL, NULL, NULL, NULL, NULL);
            CHECK_ERR
        }

        err = H5Pset (plist, SEL_ENCODING_PROPERTY_NAME, &encoding);
        CHECK_ERR
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
}

herr_t H5Pget_sel_encoding (hid_t plist, H5VL_log_sel_encoding_t *encoding) {
    herr_t err = 0;
    htri_t isdxpl, pexist;

    try {
        isdxpl = H5Pisa_class (plist, H5P_FILE_ACCESS);
        CHECK_ID (isdxpl)
        if (isdxpl == 0)
            *encoding = H5VL_LOG_ENCODING_OFFSET;  // Default property will not pass class check
        else {
            pexist = H5Pexist (plist, SEL_ENCODING_PROPERTY_NAME);
            CHECK_ID (pexist)
            if (pexist) {
                err = H5Pget (plist, SEL_ENCODING_PROPERTY_NAME, encoding);
                CHECK_ERR

            } else {
                *encoding = H5VL_LOG_ENCODING_OFFSET;
            }
        }
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
}

#define DATA_LAYOUT_PROPERTY_NAME "H5VL_log_data_layout"
herr_t H5Pset_data_layout (hid_t plist, H5VL_log_data_layout_t layout) {
    herr_t err = 0;
    htri_t isfapl;
    htri_t pexist;

    try {
        isfapl = H5Pisa_class (plist, H5P_FILE_CREATE);
        CHECK_ID (isfapl)
        if (isfapl == 0) ERR_OUT ("Not faplid")

        pexist = H5Pexist (plist, DATA_LAYOUT_PROPERTY_NAME);
        CHECK_ID (pexist)
        if (!pexist) {
            H5VL_log_data_layout_t contig = H5VL_LOG_DATA_LAYOUT_CONTIG;
            err = H5Pinsert2 (plist, DATA_LAYOUT_PROPERTY_NAME, sizeof (H5VL_log_data_layout_t),
                              &contig, NULL, NULL, NULL, NULL, NULL, NULL);
            CHECK_ERR
        }

        err = H5Pset (plist, DATA_LAYOUT_PROPERTY_NAME, &layout);
        CHECK_ERR
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
}

herr_t H5Pget_data_layout (hid_t plist, H5VL_log_data_layout_t *layout) {
    herr_t err = 0;
    htri_t isdxpl, pexist;

    try {
        isdxpl = H5Pisa_class (plist, H5P_FILE_CREATE);
        CHECK_ID (isdxpl)
        if (isdxpl == 0)
            *layout = H5VL_LOG_DATA_LAYOUT_CONTIG;  // Default property will not pass class check
        else {
            pexist = H5Pexist (plist, DATA_LAYOUT_PROPERTY_NAME);
            CHECK_ID (pexist)
            if (pexist) {
                err = H5Pget (plist, DATA_LAYOUT_PROPERTY_NAME, layout);
                CHECK_ERR

            } else {
                *layout = H5VL_LOG_DATA_LAYOUT_CONTIG;
            }
        }
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
}

#define SUBFILING_PROPERTY_NAME "H5VL_log_subfiling"
herr_t H5Pset_subfiling (hid_t plist, int nsubfiles) {
    herr_t err = 0;
    htri_t isfapl;
    htri_t pexist;

    try {
        isfapl = H5Pisa_class (plist, H5P_FILE_CREATE);
        CHECK_ID (isfapl)
        if (isfapl == 0) ERR_OUT ("Not fcplid")

        pexist = H5Pexist (plist, SUBFILING_PROPERTY_NAME);
        CHECK_ID (pexist)
        if (!pexist) {
            int n = H5VL_LOG_SUBFILING_OFF;
            err   = H5Pinsert2 (plist, SUBFILING_PROPERTY_NAME, sizeof (int), &n, NULL, NULL, NULL,
                                NULL, NULL, NULL);
            CHECK_ERR
        }

        err = H5Pset (plist, SUBFILING_PROPERTY_NAME, &nsubfiles);
        CHECK_ERR
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
}

herr_t H5Pget_subfiling (hid_t plist, int *nsubfiles) {
    herr_t err = 0;
    htri_t isfapl, pexist;

    try {
        isfapl = H5Pisa_class (plist, H5P_FILE_CREATE);
        CHECK_ID (isfapl)
        if (isfapl == 0)
            *nsubfiles = H5VL_LOG_SUBFILING_OFF;  // Default property will not pass class check
        else {
            pexist = H5Pexist (plist, SUBFILING_PROPERTY_NAME);
            CHECK_ID (pexist)
            if (pexist) {
                err = H5Pget (plist, SUBFILING_PROPERTY_NAME, nsubfiles);
                CHECK_ERR
            } else {
                *nsubfiles = H5VL_LOG_SUBFILING_OFF;
            }
        }
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
}

#define SINGLE_SUBFILE_READ_PROPERTY_NAME "H5VL_log_single_subfile_read"
herr_t H5Pset_single_subfile_read (hid_t plist, hbool_t single_subfile_read) {
    herr_t err = 0;
    htri_t isfapl;
    htri_t pexist;

    try {
        isfapl = H5Pisa_class (plist, H5P_FILE_CREATE);
        CHECK_ID (isfapl)
        if (isfapl == 0) ERR_OUT ("Not fcplid")

        pexist = H5Pexist (plist, SINGLE_SUBFILE_READ_PROPERTY_NAME);
        CHECK_ID (pexist)
        if (!pexist) {
            hbool_t f = false;
            err = H5Pinsert2 (plist, SINGLE_SUBFILE_READ_PROPERTY_NAME, sizeof (hbool_t), &f, NULL,
                              NULL, NULL, NULL, NULL, NULL);
            CHECK_ERR
        }

        err = H5Pset (plist, SINGLE_SUBFILE_READ_PROPERTY_NAME, &single_subfile_read);
        CHECK_ERR
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
}

herr_t H5Pget_single_subfile_read (hid_t plist, hbool_t *single_subfile_read) {
    herr_t err = 0;
    htri_t isfapl, pexist;

    try {
        isfapl = H5Pisa_class (plist, H5P_FILE_CREATE);
        CHECK_ID (isfapl)
        if (isfapl == 0)
            *single_subfile_read = false;  // Default property will not pass class check
        else {
            pexist = H5Pexist (plist, SINGLE_SUBFILE_READ_PROPERTY_NAME);
            CHECK_ID (pexist)
            if (pexist) {
                err = H5Pget (plist, SINGLE_SUBFILE_READ_PROPERTY_NAME, single_subfile_read);
                CHECK_ERR

            } else {
                *single_subfile_read = false;
            }
        }
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
}

#define PASSTHRU_READ_WRITE_PROPERTY_NAME "H5VL_log_passthru_read_write"
herr_t H5Pset_passthru_read_write (hid_t faplid, hbool_t enable) {
    herr_t err = 0;
    htri_t isfapl;
    htri_t pexist;

    try {
        isfapl = H5Pisa_class (faplid, H5P_FILE_ACCESS);
        CHECK_ID (isfapl);
        if (isfapl == 0) { ERR_OUT ("Not fcplid"); }

        pexist = H5Pexist (faplid, PASSTHRU_READ_WRITE_PROPERTY_NAME);
        CHECK_ID (pexist);
        if (!pexist) {
            hbool_t f = false;
            err = H5Pinsert2 (faplid, PASSTHRU_READ_WRITE_PROPERTY_NAME, sizeof (hbool_t), &f, NULL,
                              NULL, NULL, NULL, NULL, NULL);
            CHECK_ERR;
        }

        err = H5Pset (faplid, PASSTHRU_READ_WRITE_PROPERTY_NAME, &enable);
        CHECK_ERR;
    }
    H5VL_LOGI_EXP_CATCH_ERR;

err_out:;
    return err;
}
herr_t H5Pget_passthru_read_write (hid_t faplid, hbool_t *enable) {
    herr_t err = 0;
    htri_t isfapl, pexist;

    try {
        isfapl = H5Pisa_class (faplid, H5P_FILE_ACCESS);
        CHECK_ID (isfapl);
        if (isfapl == 0) {
            ERR_OUT ("Not faplid");
        } else {
            pexist = H5Pexist (faplid, PASSTHRU_READ_WRITE_PROPERTY_NAME);
            CHECK_ID (pexist);
            if (pexist) {
                err = H5Pget (faplid, PASSTHRU_READ_WRITE_PROPERTY_NAME, enable);
                CHECK_ERR;
            } else {
                *enable = false;
            }
        }
    }
    H5VL_LOGI_EXP_CATCH_ERR;

err_out:;
    return err;
}
