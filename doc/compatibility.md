## The Log VOL connector API compatibility

Following tables contains list of the Log VOL connector's compatibility with HDF5 APIs.
APIs not listed below is assumed to be unsupported.

See [doc/vol_test_fails.md](./vol_test_fails.md) for a comprehensive test result of log-layout based VOL on all HDF5 features.

### Level of support
* Full
  + The API is supported as long as the underlying VOL supports it.
  + Applications can expect exactly the same behavior as using the underlying VOL. 
  + The request is usually passed directly to the underlying VOL. 
* High
  + The primary purpose of the API is supported by the Log VOL connector.
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
* All attribute APIs are Highy supported (Full).

### Dataset APIs (H5D*)

| API                  | Level of Support | Notes                                                                                                                                                                                                                                                                                               |
|----------------------|------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| H5Dcreate            | High             | A dataset can only have one opened instance at a time. Operating multiple   dataset handle to the same dataset will result in undefined behavier.                                                                                                                                                   |
| H5Dcreate_anon       | None             |                                                                                                                                                                                                                                                                                                     |
| H5Dopen              | High             | A dataset can only have one opened instance at a time. Operating multiple   dataset handle to the same dataset will result in undefined behavier.                                                                                                                                                   |
| H5Dclose             | High             |                                                                                                                                                                                                                                                                                                     |
| H5Dfill              | None             |                                                                                                                                                                                                                                                                                                     |
| H5Dread              | Partial          | The API stages read request in the Log VOL connector without actualy   performing read.                                                                                                                                                                                                                 |
| H5Dwrite             | Partial          | The API stages write request in the Log VOL connector without actualy writing   to the file. Application must call H5Fflush to ensure the data is written to   the file. The application should not overwrite unflushed write reqeusts   (writing to the same place twice), or the result is undefined. |
| H5Dflush             | Repurposed       | Same as calling H5Fflush on the file containing the dataset                                                                                                                                                                                                                                         |
| H5Drefresh           | None             |                                                                                                                                                                                                                                                                                                     |
| H5Dgather            | None             |                                                                                                                                                                                                                                                                                                     |
| H5Dscatter           | None             |                                                                                                                                                                                                                                                                                                     |
| H5Diterate           | None             |                                                                                                                                                                                                                                                                                                     |
| H5Dset_extent        | High             |                                                                                                                                                                                                                                                                                                     |
| H5Dvlen_get_buf_size | None             |                                                                                                                                                                                                                                                                                                     |
| H5Dvlen_reclaim      | None             |                                                                                                                                                                                                                                                                                                     |
| H5Dget_storage_size  | None             |                                                                                                                                                                                                                                                                                                     |
| H5Dget_space         | High             |                                                                                                                                                                                                                                                                                                     |
| H5Dget_space_status  | None             |                                                                                                                                                                                                                                                                                                     |
| H5Dget_type          | Full      |                                                                                                                                                                                                                                                                                                     |
| H5Dget_create_plist  | Full      |                                                                                                                                                                                                                                                                                                     |
| H5Dget_access_plist  | Full      |                                                                                                                                                                                                                                                                                                     |
| H5Dget_offset        | None             |                                                                                                                                                                                                                                                                                                     |

### Error APIs (H5E*)
* All error APIs are not supported (None).

### File APIs (H5F*)

| API                             | Level of Support | Notes                                                                                                                                    |
|---------------------------------|------------------|------------------------------------------------------------------------------------------------------------------------------------------|
| H5Fcreate                       | High             | A file can only have one opened instance at a time. Operating multiple   file handle to the same file will result in undefined behavier. |
| H5Fopen                         | High             | A file can only have one opened instance at a time. Operating multiple   file handle to the same file will result in undefined behavier. |
| H5Fget_file_image               | None             |                                                                                                                                          |
| H5Freopen                       | None             |                                                                                                                                          |
| H5Fclose                        | High             | All objects in the file must be closed before closing the file.                                                                          |
| H5Fflush                        | Repurposed       | The API is repurposed for flushing staged I/O requests.                                                                                  |
| H5Fmount                        | None             |                                                                                                                                          |
| H5Funmount                      | None             |                                                                                                                                          |
| H5Fget_vfd_handle               | None             |                                                                                                                                          |
| H5Fget_filesize                 | Full      |                                                                                                                                          |
| H5Fget_create_plist             | Full      |                                                                                                                                          |
| H5Fget_access_plist             | High             | the Log VOL connector may introduce additional file access property in the file   it creates                                                     |
| H5Fget_info                     | Full      |                                                                                                                                          |
| H5Fget_info1 *                  | Full      |                                                                                                                                          |
| H5Fget_info2                    | Full      |                                                                                                                                          |
| H5Fget_intent                   | Full      |                                                                                                                                          |
| H5Fget_name                     | Full      |                                                                                                                                          |
| H5Fget_obj_count                | None             |                                                                                                                                          |
| H5Fget_obj_ids                  | None             |                                                                                                                                          |
| H5Fget_free_sections            | None             |                                                                                                                                          |
| H5Fget_freespace                | None             |                                                                                                                                          |
| H5Fclear_elink_file_cache       | None             |                                                                                                                                          |
| H5Fset_mdc_config               | None             |                                                                                                                                          |
| H5Fget_mdc_config               | Full      |                                                                                                                                          |
| H5Fget_mdc_hit_rate             | Full      |                                                                                                                                          |
| H5Freset_mdc_hit_rate_stats     | None             |                                                                                                                                          |
| H5Fget_mdc_image_info           | Full      |                                                                                                                                          |
| H5Fget_mdc_size                 | Full      |                                                                                                                                          |
| H5Fget_metadata_read_retry_info | Full      |                                                                                                                                          |
| H5Fstart_mdc_logging            | None             |                                                                                                                                          |
| H5Fstop_mdc_logging             | None             |                                                                                                                                          |
| H5Fget_mdc_logging_status       | None             |                                                                                                                                          |
| H5Fstart_swmr_write             | None             |                                                                                                                                          |
| H5Fset_mpi_atomicity            | None             |                                                                                                                                          |
| H5Fget_mpi_atomicity            | Full      |                                                                                                                                          |
| H5Fget_page_buffering_stats     | Full      |                                                                                                                                          |
| H5Freset_page_buffering_stats   | None             |                                                                                                                                          |

### Group APIs (H5G*)
* All group APIs are Highy supported (Full).

### Link APIs (H5L*)

| API                 | Level of Support | Notes |
|---------------------|------------------|-------|
| H5Lcreate_hard      | None             |       |
| H5Lcreate_soft      | None             |       |
| H5Lcreate_external  | None             |       |
| H5Lexists           | High             |       |
| H5Lmove             | None             |       |
| H5Lcopy             | None             |       |
| H5Ldelete           | None             |       |
| H5Lget_info         | Full             |       |
| H5Lget_val          | Full             |       |
| H5Lunpack_elink_val | None             |       |
| H5Lcreate_ud        | None             |       |
| H5Lregister         | None             |       |
| H5Lunregister       | None             |       |
| H5Lis_registered    | None             |       |
| H5Literate          | High             |       |
| H5Literate_by_name  | High             |       |
| H5Lvisit            | High             |       |
| H5Lvisit_by_name    | High             |       |
| H5Lget_info_by_idx  | None             |       |
| H5Lget_name_by_idx  | None             |       |
| H5Lget_val_by_idx   | None             |       |
| H5Ldelete_by_idx    | None             |       |

### Object APIs (H5O*)

| API                         | Level of Support | Notes                                                                          |
|-----------------------------|------------------|--------------------------------------------------------------------------------|
| H5Oopen                     | High             | If the object is a file or a dataset, the object specific limitations   apply. |
| H5Oopen_by_idx              | None             |                                                                                |
| H5Oopen_by_addr             | None             |                                                                                |
| H5Olink                     | None             |                                                                                |
| H5Oclose                    | High             | If the object is a file, H5Fclose limitations apply.                           |
| H5Ocopy                     | None             |                                                                                |
| H5Oexists_by_name           | Full             |                                                                                |
| H5Ovisit                    | High             |                                                                                |
| H5Ovisit_by_name            | High             |                                                                                |
| H5Oset_comment *            | None             |                                                                                |
| H5Oset_comment_by_name *    | None             |                                                                                |
| H5Oget_comment              | None             |                                                                                |
| H5Oget_comment_by_name      | None             |                                                                                |
| H5Oget_info                 | High             |                                                                                |
| H5Oget_info_by_name         | High             |                                                                                |
| H5Oget_info_by_idx          | None             |                                                                                |
| H5Oincr_refcount            | None             |                                                                                |
| H5Odecr_refcount            | None             |                                                                                |
| H5Oflush                    | None             |                                                                                |
| H5Orefresh                  | None             |                                                                                |
| H5Oare_mdc_flushes_disabled | None             |                                                                                |
| H5Odisable_mdc_flushes      | None             |                                                                                |
| H5Oenable_mdc_flushes       | None             |                                                                                |

### Datatype APIs (H5T*)
* All datatype APIs are Highy supported (Full).



