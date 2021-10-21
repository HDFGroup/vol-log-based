#pragma once

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <H5Epublic.h>
#include <cassert>

#ifdef LOGVOL_DEBUG
#include <cstdlib>
#include <cstring>
#define DEBUG_ABORT                                        \
	{                                                      \
		char *val = getenv ("LOGVOL_DEBUG_ABORT_ON_ERR");  \
		if (val && (strcmp (val, "1") == 0)) { abort (); } \
	}
//#define LOGVOL_VERBOSE_DEBUG 1
#else
#define DEBUG_ABORT
#endif

#ifdef LOGVOL_DEBUG
#define LOG_VOL_ASSERT(A) assert (A);
#else
#define LOG_VOL_ASSERT(A) \
	{}
#endif

#define CHECK_ERR                                                     \
	{                                                                 \
		if (err < 0) {                                                \
			printf ("Error at line %d in %s:\n", __LINE__, __FILE__); \
			H5Eprint1 (stdout);                                       \
			DEBUG_ABORT                                               \
			goto err_out;                                             \
		}                                                             \
	}

#define CHECK_MPIERR                                                             \
	{                                                                            \
		if (mpierr != MPI_SUCCESS) {                                             \
			int el = 256;                                                        \
			char errstr[256];                                                    \
			MPI_Error_string (mpierr, errstr, &el);                              \
			printf ("Error at line %d in %s: %s\n", __LINE__, __FILE__, errstr); \
			err = -1;                                                            \
			DEBUG_ABORT                                                          \
			goto err_out;                                                        \
		}                                                                        \
	}

#define CHECK_ID(A)                                                   \
	{                                                                 \
		if (A < 0) {                                                  \
			printf ("Error at line %d in %s:\n", __LINE__, __FILE__); \
			H5Eprint1 (stdout);                                       \
			DEBUG_ABORT                                               \
			goto err_out;                                             \
		}                                                             \
	}

#define CHECK_PTR(A)                                                  \
	{                                                                 \
		if (A == NULL) {                                              \
			printf ("Error at line %d in %s:\n", __LINE__, __FILE__); \
			H5Eprint1 (stdout);                                       \
			DEBUG_ABORT                                               \
			goto err_out;                                             \
		}                                                             \
	}

#define ERR_OUT(A)                                                      \
	{                                                                   \
		printf ("Error at line %d in %s: %s\n", __LINE__, __FILE__, A); \
		DEBUG_ABORT                                                     \
		goto err_out;                                                   \
	}

#define RET_ERR(A)  \
	{               \
		err = -1;   \
		ERR_OUT (A) \
	}
