/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
//
#include <exception>
#include <iostream>
#include <string>
//
#include <H5VL_logi_err.hpp>
//

H5VL_logi_exception::H5VL_logi_exception (const char *file,
                                          int line,
                                          const char *func,
                                          const char *msg)
    : file (file), line (line), func (func), msg (msg), is_hdf5 (false) {}

H5VL_logi_exception::H5VL_logi_exception (
    const char *file, int line, const char *func, std::string msg, bool is_hdf5)
    : file (file), line (line), func (func), msg (msg), is_hdf5 (is_hdf5) {}

std::string H5VL_logi_exception::what () {
    return "In " + func + " at " + file + ":" + std::to_string (line) + ": " + msg;
}
