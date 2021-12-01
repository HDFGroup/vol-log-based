## Log-based VOL API compatibility

Following tables contains list of log-based VOL's compatibility with HDF5 APIs.
APIs not listed below is assumed to be unsupported.

### Level of support
* Full
  + The API is supported as long as the underlying VOL supports it.
  + Applications can expect exactly the same behavior as using the underlying VOL. 
  + The request is passed directly to the underlying VOL. 
* High
  + The primary purpose of the API is supported by the log-based VOL.
  + There may be some limitations that should not affect most applications.
  + The behavior may differ slightly from the underlying VOL.
* Partial
  + The API only work under certain condition or a limited set of parameters.
  + The API will return an error if the input argument is not supported.
* Repurposed
  + The API serves a different purpose than the origin HDF5 API.
* None
  + The API is not supported or never tested.
  + The API may return an error or incorrect result.

### Attribute APIs (H5A*)
* All attribute APIs are fully supported (Full).

### Dataset APIs (H5D*)

| API                  | Level of Support | Notes                                                                                                                                                                                                                                                                                               |
|----------------------|------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| H5Dcreate            | Full             | A dataset can only have one opened instance at a time. Operating multiple   dataset handle to the same dataset will result in undefined behavier.                                                                                                                                                   |
| H5Dcreate_anon       | None             |                                                                                                                                                                                                                                                                                                     |
| H5Dopen              | Full             | A dataset can only have one opened instance at a time. Operating multiple   dataset handle to the same dataset will result in undefined behavier.                                                                                                                                                   |
| H5Dclose             | Full             |                                                                                                                                                                                                                                                                                                     |
| H5Dfill              | None             |                                                                                                                                                                                                                                                                                                     |
| H5Dread              | Partial          | The API stages read request in the log-based VOL without actualy   performing read.                                                                                                                                                                                                                 |
| H5Dwrite             | Partial          | The API stages write request in the log-based VOL without actualy writing   to the file. Application must call H5Fflush to ensure the data is written to   the file. The application should not overwrite unflushed write reqeusts   (writing to the same place twice), or the result is undefined. |
| H5Dflush             | Repurposed       | Same as calling H5Fflush on the file containing the dataset                                                                                                                                                                                                                                         |
| H5Drefresh           | None             |                                                                                                                                                                                                                                                                                                     |
| H5Dgather            | None             |                                                                                                                                                                                                                                                                                                     |
| H5Dscatter           | None             |                                                                                                                                                                                                                                                                                                     |
| H5Diterate           | None             |                                                                                                                                                                                                                                                                                                     |
| H5Dset_extent        | Full             |                                                                                                                                                                                                                                                                                                     |
| H5Dvlen_get_buf_size | None             |                                                                                                                                                                                                                                                                                                     |
| H5Dvlen_reclaim      | None             |                                                                                                                                                                                                                                                                                                     |
| H5Dget_storage_size  | None             |                                                                                                                                                                                                                                                                                                     |
| H5Dget_space         | Full             |                                                                                                                                                                                                                                                                                                     |
| H5Dget_space_status  | None             |                                                                                                                                                                                                                                                                                                     |
| H5Dget_type          | Passthrough      |                                                                                                                                                                                                                                                                                                     |
| H5Dget_create_plist  | Passthrough      |                                                                                                                                                                                                                                                                                                     |
| H5Dget_access_plist  | Passthrough      |                                                                                                                                                                                                                                                                                                     |
| H5Dget_offset        | None             |                                                                                                                                                                                                                                                                                                     |

### Error APIs (H5E*)
* All error APIs are not supported (None).

### File APIs (H5F*)

| API                             | Level of Support | Notes                                                                                                                                    |
|---------------------------------|------------------|------------------------------------------------------------------------------------------------------------------------------------------|
| H5Fcreate                       | Full             | A file can only have one opened instance at a time. Operating multiple   file handle to the same file will result in undefined behavier. |
| H5Fopen                         | Full             | A file can only have one opened instance at a time. Operating multiple   file handle to the same file will result in undefined behavier. |
| H5Fget_file_image               | None             |                                                                                                                                          |
| H5Freopen                       | None             |                                                                                                                                          |
| H5Fclose                        | Full             | All objects in the file must be closed before closing the file.                                                                          |
| H5Fflush                        | Repurposed       | The API is repurposed for flushing staged I/O requests.                                                                                  |
| H5Fmount                        | None             |                                                                                                                                          |
| H5Funmount                      | None             |                                                                                                                                          |
| H5Fget_vfd_handle               | None             |                                                                                                                                          |
| H5Fget_filesize                 | Passthrough      |                                                                                                                                          |
| H5Fget_create_plist             | Passthrough      |                                                                                                                                          |
| H5Fget_access_plist             | Full             | Log-based VOL may introduce additional file access property in the file   it creates                                                     |
| H5Fget_info                     | Passthrough      |                                                                                                                                          |
| H5Fget_info1 *                  | Passthrough      |                                                                                                                                          |
| H5Fget_info2                    | Passthrough      |                                                                                                                                          |
| H5Fget_intent                   | Passthrough      |                                                                                                                                          |
| H5Fget_name                     | Passthrough      |                                                                                                                                          |
| H5Fget_obj_count                | None             |                                                                                                                                          |
| H5Fget_obj_ids                  | None             |                                                                                                                                          |
| H5Fget_free_sections            | None             |                                                                                                                                          |
| H5Fget_freespace                | None             |                                                                                                                                          |
| H5Fclear_elink_file_cache       | None             |                                                                                                                                          |
| H5Fset_mdc_config               | None             |                                                                                                                                          |
| H5Fget_mdc_config               | Passthrough      |                                                                                                                                          |
| H5Fget_mdc_hit_rate             | Passthrough      |                                                                                                                                          |
| H5Freset_mdc_hit_rate_stats     | None             |                                                                                                                                          |
| H5Fget_mdc_image_info           | Passthrough      |                                                                                                                                          |
| H5Fget_mdc_size                 | Passthrough      |                                                                                                                                          |
| H5Fget_metadata_read_retry_info | Passthrough      |                                                                                                                                          |
| H5Fstart_mdc_logging            | None             |                                                                                                                                          |
| H5Fstop_mdc_logging             | None             |                                                                                                                                          |
| H5Fget_mdc_logging_status       | None             |                                                                                                                                          |
| H5Fstart_swmr_write             | None             |                                                                                                                                          |
| H5Fset_mpi_atomicity            | None             |                                                                                                                                          |
| H5Fget_mpi_atomicity            | Passthrough      |                                                                                                                                          |
| H5Fget_page_buffering_stats     | Passthrough      |                                                                                                                                          |
| H5Freset_page_buffering_stats   | None             |                                                                                                                                          |

### Group APIs (H5G*)
* All group APIs are fully supported (Full).

### Object APIs (H5O*)

| API                         | Level of Support | Notes                                                                          |
|-----------------------------|------------------|--------------------------------------------------------------------------------|
| H5Oopen                     | Full             | If the object is a file or a dataset, the object specific limitations   apply. |
| H5Oopen_by_idx              | None             |                                                                                |
| H5Oopen_by_addr             | None             |                                                                                |
| H5Olink                     | None             |                                                                                |
| H5Oclose                    | Full             | If the object is a file, H5Fclose limitations apply.                           |
| H5Ocopy                     | None             |                                                                                |
| H5Ovisit                    | None             |                                                                                |
| H5Ovisit_by_name            | None             |                                                                                |
| H5Oset_comment *            | None             |                                                                                |
| H5Oset_comment_by_name *    | None             |                                                                                |
| H5Oget_comment              | None             |                                                                                |
| H5Oget_comment_by_name      | None             |                                                                                |
| H5Oget_info                 | Full             |                                                                                |
| H5Oget_info_by_name         | Full             |                                                                                |
| H5Oget_info_by_idx          | Full             |                                                                                |
| H5Oincr_refcount            | None             |                                                                                |
| H5Odecr_refcount            | None             |                                                                                |
| H5Oflush                    | None             |                                                                                |
| H5Orefresh                  | None             |                                                                                |
| H5Oare_mdc_flushes_disabled | None             |                                                                                |
| H5Odisable_mdc_flushes      | None             |                                                                                |
| H5Oenable_mdc_flushes       | None             |                                                                                |

### Datatype APIs (H5T*)
* All datatype APIs are fully supported (Full).



