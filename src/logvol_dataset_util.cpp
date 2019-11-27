#include "ncmpi_vol.h"
#include "pnetcdf.h"
#include <algorithm>

#define REQSWAP(A,B) {tmp = starts[A]; starts[A] = starts[B]; starts[B] = tmp; tmp = counts[A]; counts[A] = counts[B]; counts[B] = tmp;}
#define BLKSWAP(A,B) {tmp = starts[A]; starts[A] = starts[B]; starts[B] = tmp;}

static bool lessthan(int ndim, MPI_Offset *a, MPI_Offset *b){
    int i;

    for(i = 0; i < ndim; i++){
        if (*a < *b){
            return true;
        }
        else if (*b < *a){
            return false;
        }
        ++a;
        ++b;
    }

    return false;
}

void sortreq(int ndim, hssize_t len, MPI_Offset **starts, MPI_Offset **counts){
    int i, j, p;
    MPI_Offset *tmp;

    if (len < 16){
        j = 1;
        while(j){
            j = 0;
            for(i = 0; i < len - 1; i++){
                if (lessthan(ndim, starts[i + 1], starts[i])){
                    REQSWAP(i, i + 1)
                    j = 1;
                }
            }
        }
        return;
    }

    p = len / 2;
    REQSWAP(p, len - 1);
    p = len - 1;

    j = 0;
    for(i = 0; i < len; i++){
        if (lessthan(ndim, starts[i], starts[p])){
            if (i != j){
                REQSWAP(i, j)
            }
            j++;
        }
    }

    REQSWAP(p, j)

    sortreq(ndim, j, starts, counts);
    sortreq(ndim, len - j - 1, starts + j + 1, counts + j + 1);
}

bool hlessthan(int ndim, hsize_t *a, hsize_t *b){
    int i;

    for(i = 0; i < ndim; i++){
        if (*a < *b){
            return true;
        }
        else if (*b < *a){
            return false;
        }
        ++a;
        ++b;
    }

    return false;
}

void sortblock(int ndim, hssize_t len, hsize_t **starts){
    int i, j, p;
    hsize_t *tmp;

    if (len < 16){
        j = 1;
        while(j){
            j = 0;
            for(i = 0; i < len - 1; i++){
                if (hlessthan(ndim, starts[i + 1], starts[i])){
                    BLKSWAP(i, i + 1)
                    j = 1;
                }
            }
        }
        return;
    }

    p = len / 2;
    BLKSWAP(p, len - 1);
    p = len - 1;

    j = 0;
    for(i = 0; i < len; i++){
        if (hlessthan(ndim, starts[i], starts[p])){
            if (i != j){
                BLKSWAP(i, j)
            }
            j++;
        }
    }

    BLKSWAP(p, j)

    sortblock(ndim, j, starts);
    sortblock(ndim, len - j - 1, starts + j + 1);
}

int intersect(int ndim, MPI_Offset *sa, MPI_Offset *ca, MPI_Offset *sb){
    int i;

    for(i = 0; i < ndim - 1; i++){
        if (sa[i] != sb[i]){
            return 0;
        }
    }

    return (sa[ndim - 1] + ca[ndim - 1]) >= sb[ndim - 1];
}

void mergereq(int ndim, hssize_t *len, MPI_Offset **starts, MPI_Offset **counts){
    int i, j, p;

    p = 0;
    for(i = 1; i < *len; i++){
        if (intersect(ndim, starts[p], counts[p], starts[i])){
            counts[p][ndim - 1] = std::max(starts[p][ndim - 1] + counts[p][ndim - 1], starts[i][ndim - 1] + counts[i][ndim - 1]) - starts[p][ndim - 1];
        }
        else{
            if (p != i){
                ++p;
                starts[p] = starts[i];
                counts[p] = counts[i];
            }
        }
    }

    *len = p + 1;
}