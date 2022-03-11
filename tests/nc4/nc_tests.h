#ifndef _H_NC_TESTS
#define _H_NC_TESTS

#include <netcdf.h>

#define MILLION 1000000
#define TEST_VAL_42 42
#define BAD_NAME "dd//d/  "

#define NC_DIMID_ATT_NAME NC_ATT_DIMID_NAME /*See nc4internal.h*/
#define NC3_STRICT_ATT_NAME NC_ATT_NC3_STRICT_NAME
#define NCPROPS "_NCProperties"

/* Generic reserved Attributes */
#define NC_ATT_REFERENCE_LIST "REFERENCE_LIST"
#define NC_ATT_CLASS "CLASS"
#define NC_ATT_DIMENSION_LIST "DIMENSION_LIST"
#define NC_ATT_NAME "NAME"
#define NC_ATT_COORDINATES "_Netcdf4Coordinates" /*see hdf5internal.h:COORDINATES*/
#define NC_ATT_FORMAT "_Format"
#define NC_ATT_DIMID_NAME "_Netcdf4Dimid"
#define NC_ATT_NC3_STRICT_NAME "_nc3_strict"
#define NC_XARRAY_DIMS "_ARRAY_DIMENSIONS"
#define NC_NCZARR_ATTR "_NCZARR_ATTR"

/* Other hidden attributes */
#define ISNETCDF4ATT "_IsNetcdf4"
#define SUPERBLOCKATT "_SuperblockVersion"

#endif

