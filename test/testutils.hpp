#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <hdf5.h>

#define CHECK_ERR(A) { \
    if (A < 0) { \
        nerrs++; \
        printf("Error at line %d in %s:\n", \
        __LINE__,__FILE__); \
        H5Eprint1(stdout); \
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
