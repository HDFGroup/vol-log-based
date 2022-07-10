/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#pragma once

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <cstdlib>
#include <cstring>
#include <exception>
#include <functional>
#include <iostream>
#include <string>
//
#include <H5Epublic.h>
//
#ifdef LOGVOL_DEBUG
#include <mpi.h>

#include <cassert>
#define LOG_VOL_ASSERT(A) assert (A);
#else
#define LOG_VOL_ASSERT(A) \
    {}
#endif

#define H5VL_LOGI_EXP_CATCH           \
    catch (H5VL_logi_exception & e) { \
        H5VL_logi_print_err (e);      \
        goto err_out;                 \
    }                                 \
    catch (std::exception & e) {      \
        H5VL_logi_print_err (e);      \
        goto err_out;                 \
    }
#define H5VL_LOGI_EXP_CATCH_ERR       \
    catch (H5VL_logi_exception & e) { \
        H5VL_logi_print_err (e);      \
        err = -1;                     \
        goto err_out;                 \
    }                                 \
    catch (std::exception & e) {      \
        H5VL_logi_print_err (e);      \
        err = -1;                     \
        goto err_out;                 \
    }

inline bool H5VL_logi_debug_verbose () {
    char *val = getenv ("LOGVOL_VERBOSE_DEBUG");
    if (val) return strcmp (val, "1") == 0;
    return false;
}

#define CHECK_ERR                                                          \
    {                                                                      \
        if (err < 0) {                                                     \
            std::string msg = "function returns " + std::to_string (err);  \
            throw H5VL_logi_exception (__FILE__, __LINE__, __func__, msg); \
        }                                                                  \
    }

#define CHECK_MPIERR                                                          \
    {                                                                         \
        if (mpierr != MPI_SUCCESS) {                                          \
            int el = 256;                                                     \
            char errstr[256];                                                 \
            MPI_Error_string (mpierr, errstr, &el);                           \
            throw H5VL_logi_exception (__FILE__, __LINE__, __func__, errstr); \
        }                                                                     \
    }

#define CHECK_PTR(A)                                                                            \
    {                                                                                           \
        if (A == NULL) {                                                                        \
            throw H5VL_logi_exception (__FILE__, __LINE__, __func__, "#A initialization fail"); \
        }                                                                                       \
    }

#define CHECK_ID(A)                                                           \
    {                                                                         \
        if (A < 0) {                                                          \
            throw H5VL_logi_exception (__FILE__, __LINE__, __func__,          \
                                       "HDF5 object #A ID is invalid", true); \
        }                                                                     \
    }

#define ERR_OUT(A) \
    { throw H5VL_logi_exception (__FILE__, __LINE__, __func__, (char *)A); }

#define RET_ERR(A) ERR_OUT (A)

#define H5VL_LOGI_CHECK_NAME(name)                                                          \
    {                                                                                       \
        if (!name || (name[0] == '_' && name[1] == '_')) {                                  \
            ERR_OUT (                                                                       \
                "Object (link) name starting wiht \"__\" are reserved for log-based VOL's " \
                "internal objects")                                                         \
        }                                                                                   \
    }

class H5VL_logi_exception : public std::exception {
    std::string file;
    int line;
    std::string func;
    std::string msg;

   public:
    bool is_hdf5;
    H5VL_logi_exception (const char *file, int line, const char *func, const char *msg);
    H5VL_logi_exception (
        const char *file, int line, const char *func, std::string msg, bool is_hdf5 = false);
    std::string what ();
};

class H5VL_logi_err_finally {
   public:
    H5VL_logi_err_finally (std::function<void ()> f) : func (f) {}
    ~H5VL_logi_err_finally (void) { func (); }

   private:
    std::function<void ()> func;
};

inline void H5VL_logi_print_err (H5VL_logi_exception &e) {
#ifdef LOGVOL_DEBUG
    std::cout << e.what () << std::endl;
    if (e.is_hdf5) { H5Eprint1 (stdout); }
#endif
}

inline void H5VL_logi_print_err (std::exception &e) {
#ifdef LOGVOL_DEBUG
    std::cout << e.what () << std::endl;
#endif
}
