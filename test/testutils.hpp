#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <hdf5.h>

#define CHECK_ERR(A) { \
    if (A < 0) { \
        nerrs++; \
        printf("Error at line %d in %s:\n", \
        __LINE__,__FILE__); \
        goto err_out; \
    } \
}

#define EXP_ERR(A,B) { \
    if (A != B) { \
        nerrs++; \
        printf("Error at line %d in %s: expecting %d but got %d\n", \
        __LINE__,__FILE__, A, B); \
        goto err_out; \
    } \
}

#define EXP_VAL(A,B) { \
    if (A != B) { \
        nerrs++; \
        std::cout << "Error at line " << __LINE__ << " in " << __FILE__ << ": expecting " << (B) << " but got " << (A) << std::endl; \
        goto err_out; \
    } \
}

#define EXP_VAL_EX(A,B,C) { \
    if (A != B) { \
        nerrs++; \
        std::cout << "Error at line " << __LINE__ << " in " << __FILE__ << ": expecting " << C << " = " << (B) << " but got " << (A) << std::endl; \
        goto err_out; \
    } \
}

#define PASS_STR "pass\n"
#define SKIP_STR "skip\n"
#define FAIL_STR "fail with %d mismatches\n"