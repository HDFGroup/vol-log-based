#include "logvol_internal.hpp"


MPI_Datatype H5VL_log_dtypei_mpitype_by_size(size_t size) {
    switch (size) {
        case 1:
            return MPI_BYTE;
        case 2:
            return MPI_SHORT;
        case 4:
            return MPI_INT;
        case 8:
            return MPI_LONG_LONG;
    }

    return MPI_DATATYPE_NULL
}