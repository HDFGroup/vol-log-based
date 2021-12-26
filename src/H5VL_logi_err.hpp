#pragma once

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <cstdlib>
#include <cstring>
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

inline void H5VL_logi_print_err (int line, char *file, char *msg) {
#ifdef LOGVOL_DEBUG
	if (msg) {
		printf ("Error at line %d in %s: %s\n", line, file, msg);
	} else {
		printf ("Error at line %d in %s:\n", line, file);
	}
#endif
}

inline bool H5VL_logi_debug_verbose () {
	char *val = getenv ("LOGVOL_VERBOSE_DEBUG");
	if (val) return strcmp (val, "1") == 0;
	return false;
}

#define CHECK_ERR                                                   \
	{                                                               \
		if (err < 0) {                                              \
			H5VL_logi_print_err (__LINE__, (char *)__FILE__, NULL); \
			H5Eprint1 (stdout);                                     \
			goto err_out;                                           \
		}                                                           \
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

#define CHECK_ID(A)                                                 \
	{                                                               \
		if (A < 0) {                                                \
			H5VL_logi_print_err (__LINE__, (char *)__FILE__, NULL); \
			H5Eprint1 (stdout);                                     \
			goto err_out;                                           \
		}                                                           \
	}

#define CHECK_PTR(A)                                                \
	{                                                               \
		if (A == NULL) {                                            \
			H5VL_logi_print_err (__LINE__, (char *)__FILE__, NULL); \
			H5Eprint1 (stdout);                                     \
			goto err_out;                                           \
		}                                                           \
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

#define H5VL_LOGI_CHECK_NAME(name)                                \
	{                                                             \
		if (!name || name[0] == '_') { ERR_OUT ("Invalid name") } \
	}
