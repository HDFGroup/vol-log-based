#include "hdf5.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <chrono> 

#define N 1024

int buf[N];

using namespace std;

int main (void) {
    int i;
    hid_t fid, sid, msid, vsid, did, vdid, dcplid;
    hsize_t dims[2], start[2], count[2];

    MPI_Init(NULL, NULL);

    fid = H5Fcreate ("test.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    dims[0] = dims[1] = N;
    msid = H5Screate_simple (1, dims, NULL);
    sid = H5Screate_simple (2, dims, NULL);
    vsid = H5Screate_simple (2, dims, NULL);
    did = H5Dcreate2 (fid, "D", H5T_NATIVE_INT, sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    auto st = chrono::high_resolution_clock::now(); 
    dcplid = H5Pcreate (H5P_DATASET_CREATE);
    for (i = 0; i < N; i++) {
        start[0] = 0;
        count[0] = N;
        count[1] = 1;
        start[1] = i;
        H5Sselect_hyperslab (vsid, H5S_SELECT_SET, start, NULL, count, NULL);
        start[0] = i;
        count[0] = 1;
        count[1] = N;
        start[1] = 0;
        H5Sselect_hyperslab (sid, H5S_SELECT_SET, start, NULL, count, NULL);
        H5Pset_virtual (dcplid, vsid, "test.h5", "D", sid);
        H5Dwrite(did, H5T_NATIVE_INT, msid, sid, H5P_DEFAULT, buf);
    }
    vdid = H5Dcreate2 (fid, "VD", H5T_NATIVE_INT, sid, H5P_DEFAULT, dcplid, H5P_DEFAULT);
    auto et = chrono::high_resolution_clock::now(); 
    auto ms = chrono::duration_cast<chrono::milliseconds>(et - st); 
    cout << "Create VDS took " << ms.count() << " ms." << endl;

    st = chrono::high_resolution_clock::now(); 
    for (i = 0; i < N; i++) {
        start[0] = i;
        count[0] = 1;
        count[1] = N;
        start[1] = 0;
        H5Sselect_hyperslab (sid, H5S_SELECT_SET, start, NULL, count, NULL);
        H5Dwrite(did, H5T_NATIVE_INT, msid, sid, H5P_DEFAULT, buf);
    }
    et = chrono::high_resolution_clock::now(); 
    ms = chrono::duration_cast<chrono::milliseconds>(et - st); 
    cout << "Row write " << ms.count() << " ms." << endl;

    st = chrono::high_resolution_clock::now(); 
    for (i = 0; i < N; i++) {
        start[0] = 0;
        count[0] = N;
        count[1] = 1;
        start[1] = i;
        H5Sselect_hyperslab (sid, H5S_SELECT_SET, start, NULL, count, NULL);
        H5Dwrite(did, H5T_NATIVE_INT, msid, sid, H5P_DEFAULT, buf);
    }
    et = chrono::high_resolution_clock::now(); 
    ms = chrono::duration_cast<chrono::milliseconds>(et - st); 
    cout << "Col write " << ms.count() << " ms." << endl;

    st = chrono::high_resolution_clock::now(); 
    for (i = 0; i < N; i++) {
        start[0] = 0;
        count[0] = N;
        count[1] = 1;
        start[1] = i;
        H5Sselect_hyperslab (sid, H5S_SELECT_SET, start, NULL, count, NULL);
        H5Dwrite(vdid, H5T_NATIVE_INT, msid, sid, H5P_DEFAULT, buf);
    }
    et = chrono::high_resolution_clock::now(); 
    ms = chrono::duration_cast<chrono::milliseconds>(et - st); 
    cout << "VDS col write " << ms.count() << " ms." << endl;

    st = chrono::high_resolution_clock::now(); 
    for (i = 0; i < N; i++) {
        start[0] = i;
        count[0] = 1;
        count[1] = N;
        start[1] = 0;
        H5Sselect_hyperslab (sid, H5S_SELECT_SET, start, NULL, count, NULL);
        H5Dread(did, H5T_NATIVE_INT, msid, sid, H5P_DEFAULT, buf);
    }
    et = chrono::high_resolution_clock::now(); 
    ms = chrono::duration_cast<chrono::milliseconds>(et - st); 
    cout << "Row read " << ms.count() << " ms." << endl;

    st = chrono::high_resolution_clock::now(); 
    for (i = 0; i < N; i++) {
        start[0] = 0;
        count[0] = N;
        count[1] = 1;
        start[1] = i;
        H5Sselect_hyperslab (sid, H5S_SELECT_SET, start, NULL, count, NULL);
        H5Dread(did, H5T_NATIVE_INT, msid, sid, H5P_DEFAULT, buf);
    }
    et = chrono::high_resolution_clock::now(); 
    ms = chrono::duration_cast<chrono::milliseconds>(et - st); 
    cout << "Col read " << ms.count() << " ms." << endl;

    st = chrono::high_resolution_clock::now(); 
    for (i = 0; i < N; i++) {
        start[0] = 0;
        count[0] = N;
        count[1] = 1;
        start[1] = i;
        H5Sselect_hyperslab (sid, H5S_SELECT_SET, start, NULL, count, NULL);
        H5Dread(vdid, H5T_NATIVE_INT, msid, sid, H5P_DEFAULT, buf);
    }
    et = chrono::high_resolution_clock::now(); 
    ms = chrono::duration_cast<chrono::milliseconds>(et - st); 
    cout << "VDS col read " << ms.count() << " ms." << endl;

    H5Sclose(sid);
    H5Sclose(vsid);
    H5Sclose(msid);
    H5Dclose(did);
    H5Dclose(vdid);
    H5Pclose(dcplid);
    H5Fclose(fid);

    MPI_Finalize();

    return 0;
}
