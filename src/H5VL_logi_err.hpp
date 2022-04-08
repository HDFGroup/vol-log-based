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

#ifdef LOGVOL_DEBUG
#define H5VL_LOGI_EXP_CATCH                    \
	catch (H5VL_logi_exception & e) {          \
		std::cout << e.what () << std::endl;   \
		if (e.is_hdf5) { H5Eprint1 (stdout); } \
		goto err_out;                          \
	}                                          \
	catch (std::exception & e) {               \
		std::cout << e.what () << std::endl;   \
		goto err_out;                          \
	}
#define H5VL_LOGI_EXP_CATCH_ERR                \
	catch (H5VL_logi_exception & e) {          \
		std::cout << e.what () << std::endl;   \
		if (e.is_hdf5) { H5Eprint1 (stdout); } \
		err = -1;                              \
		goto err_out;                          \
	}                                          \
	catch (std::exception & e) {               \
		std::cout << e.what () << std::endl;   \
		err = -1;                              \
		goto err_out;                          \
	}
#else
#define H5VL_LOGI_EXP_CATCH      \
	catch (std::exception & e) { \
		goto err_out;            \
	}
#define H5VL_LOGI_EXP_CATCH_ERR  \
	catch (std::exception & e) { \
		err = -1;                \
		goto err_out;            \
	}
#endif

inline void H5VL_logi_print_err (int line, char *file, char *msg, bool h5err = false) {
#ifdef LOGVOL_DEBUG
	if (msg) {
		printf ("Error at line %d in %s: %s\n", line, file, msg);
	} else {
		printf ("Error at line %d in %s:\n", line, file);
	}
	if (h5err) { H5Eprint1 (stdout); }
#endif
}

inline bool H5VL_logi_debug_verbose () {
	char *val = getenv ("LOGVOL_VERBOSE_DEBUG");
	if (val) return strcmp (val, "1") == 0;
	return false;
}

#define CHECK_ERR                                                         \
	{                                                                     \
		if (err < 0) {                                                    \
			H5VL_logi_print_err (__LINE__, (char *)__FILE__, NULL, true); \
			goto err_out;                                                 \
		}                                                                 \
	}

#define CHECK_MPIERR                                                  \
	{                                                                 \
		if (mpierr != MPI_SUCCESS) {                                  \
			int el = 256;                                             \
			char errstr[256];                                         \
			MPI_Error_string (mpierr, errstr, &el);                   \
			H5VL_logi_print_err (__LINE__, (char *)__FILE__, errstr); \
			err = -1;                                                 \
			goto err_out;                                             \
		}                                                             \
	}

#define CHECK_ID(A)                                                       \
	{                                                                     \
		if (A < 0) {                                                      \
			H5VL_logi_print_err (__LINE__, (char *)__FILE__, NULL, true); \
			goto err_out;                                                 \
		}                                                                 \
	}

#define CHECK_PTR(A)                                                      \
	{                                                                     \
		if (A == NULL) {                                                  \
			H5VL_logi_print_err (__LINE__, (char *)__FILE__, NULL, true); \
			goto err_out;                                                 \
		}                                                                 \
	}

#define ERR_OUT(A)                                                   \
	{                                                                \
		H5VL_logi_print_err (__LINE__, (char *)__FILE__, (char *)A); \
		goto err_out;                                                \
	}

#define RET_ERR(A)  \
	{               \
		err = -1;   \
		ERR_OUT (A) \
	}

#define H5VL_LOGI_CHECK_NAME(name)                                                    \
	{                                                                                 \
		if (!name || (name[0] == '_' && name[1] == '_')) { ERR_OUT ("Invalid name") } \
	}

class H5VL_logi_exception : std::exception {
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