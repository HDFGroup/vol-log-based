# Note for Developers

### Table of contents
- [Logvol file format proposals](#characteristics-and-structure-of-neutrino-experimental-data)

---

## HDF5 native VOL dynamic dataype init

The predefined datatype are not defined as constants in HDF5.
They are global variables initialized by the native VOL when the VOL get initialized.
As a result, we need to initialize the native VOL before using any derived datatype.
H5Fcreate will call the initialization callback function if the VOL supports it.
It requires harnessing the introspect interface so the dispatcher knows whether the VOL support the init call back.
Also, the file optional interface, which includes the init callback, must be implemented as well.

---