#include "ncmpi_vol.h"

MPI_Datatype h5t_to_mpi_type(hid_t type_id) {
    if (type_id == H5T_NATIVE_CHAR) return MPI_CHAR;
    else if (type_id == H5T_NATIVE_SHORT) return MPI_SHORT;
    else if (type_id == H5T_NATIVE_INT) return MPI_INT;
    else if (type_id == H5T_NATIVE_LONG) return MPI_LONG;
    else if (type_id == H5T_NATIVE_LLONG) return MPI_LONG_LONG;
    else if (type_id == H5T_NATIVE_UCHAR) return MPI_UNSIGNED_CHAR;
    else if (type_id == H5T_NATIVE_USHORT) return MPI_UNSIGNED_SHORT;
    else if (type_id == H5T_NATIVE_UINT) return MPI_UNSIGNED;
    else if (type_id == H5T_NATIVE_ULONG) return MPI_UNSIGNED_LONG;
    else if (type_id == H5T_NATIVE_ULLONG) return MPI_UNSIGNED_LONG_LONG;
    else if (type_id == H5T_NATIVE_FLOAT) return MPI_FLOAT;
    else if (type_id == H5T_NATIVE_DOUBLE) return MPI_DOUBLE;
    else if (type_id == H5T_NATIVE_LDOUBLE) return MPI_DOUBLE;
    return MPI_DATATYPE_NULL;
}

nc_type h5t_to_nc_type(hid_t type_id) {
    if (type_id == H5T_NATIVE_CHAR) return NC_CHAR;
    else if (type_id == H5T_NATIVE_SHORT) return NC_SHORT;
    else if (type_id == H5T_NATIVE_INT) return NC_INT;
    else if (type_id == H5T_NATIVE_LONG) return NC_INT;
    else if (type_id == H5T_NATIVE_LLONG) return NC_INT64;
    else if (type_id == H5T_NATIVE_UCHAR) return NC_CHAR;
    else if (type_id == H5T_NATIVE_USHORT) return NC_USHORT;
    else if (type_id == H5T_NATIVE_UINT) return NC_UINT;
    else if (type_id == H5T_NATIVE_ULONG) return NC_UINT;
    else if (type_id == H5T_NATIVE_ULLONG) return NC_UINT64;
    else if (type_id == H5T_NATIVE_FLOAT) return NC_FLOAT;
    else if (type_id == H5T_NATIVE_DOUBLE) return NC_DOUBLE;
    else if (type_id == H5T_NATIVE_LDOUBLE) return NC_DOUBLE;
    return NC_NAT;
}

hid_t nc_to_h5t_type(nc_type type_id) {
    if (type_id == NC_CHAR) return H5T_NATIVE_CHAR;
    else if (type_id == NC_SHORT) return H5T_NATIVE_SHORT;
    else if (type_id == NC_INT) return H5T_NATIVE_INT;
    else if (type_id == NC_INT64) return H5T_NATIVE_LLONG;
    else if (type_id == NC_USHORT) return H5T_NATIVE_USHORT;
    else if (type_id == NC_UINT) return H5T_NATIVE_UINT;
    else if (type_id == NC_UINT64) return H5T_NATIVE_ULLONG;
    else if (type_id == NC_FLOAT) return H5T_NATIVE_FLOAT;
    else if (type_id == NC_DOUBLE) return H5T_NATIVE_DOUBLE;
    return -1;
}

int nc_type_size(nc_type type_id) {
    if (type_id == NC_CHAR) return 1;
    else if (type_id == NC_SHORT) return 2;
    else if (type_id == NC_INT) return 4;
    else if (type_id == NC_INT64) return 8;
    else if (type_id == NC_USHORT) return 2;
    else if (type_id == NC_UINT) return 4;
    else if (type_id == NC_UINT64) return 8;
    else if (type_id == NC_FLOAT) return 4;
    else if (type_id == NC_DOUBLE) return 8;
    return 0;
}

int enter_data_mode(H5VL_ncmpi_file_t *fp){
    int err;

    if (fp->flags & PNC_VOL_DATA_MODE){
        return NC_NOERR;
    }

    err = ncmpi_enddef(fp->ncid); CHECK_ERR
    fp->flags |= PNC_VOL_DATA_MODE;

    return NC_NOERR;
}

int enter_define_mode(H5VL_ncmpi_file_t *fp){
    int err;

    if (!(fp->flags & PNC_VOL_DATA_MODE)){
        return NC_NOERR;
    }

    err = ncmpi_redef(fp->ncid); CHECK_ERR
    fp->flags ^= PNC_VOL_DATA_MODE;

    return NC_NOERR;
}

int enter_indep_mode(H5VL_ncmpi_file_t *fp){
    int err;

    if (fp->flags & PNC_VOL_INDEP_MODE){
        return NC_NOERR;
    }

    err = ncmpi_begin_indep_data(fp->ncid); CHECK_ERR
    fp->flags |= PNC_VOL_INDEP_MODE;

    return NC_NOERR;
}

int enter_coll_mode(H5VL_ncmpi_file_t *fp){
    int err;

    if (!(fp->flags & PNC_VOL_INDEP_MODE)){
        return NC_NOERR;
    }

    err = ncmpi_end_indep_data(fp->ncid); CHECK_ERR
    fp->flags ^= PNC_VOL_INDEP_MODE;

    return NC_NOERR;
}