#ifndef FBE_PAYLOAD_CDB_FIS_H
#define FBE_PAYLOAD_CDB_FIS_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_payload_cdb_operation.h"

typedef enum fbe_payload_cdb_fis_io_status_e{
    FBE_PAYLOAD_CDB_FIS_IO_STATUS_INVALID,
    FBE_PAYLOAD_CDB_FIS_IO_STATUS_OK,
    FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY,
    FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_NO_RETRY
}fbe_payload_cdb_fis_io_status_t;

typedef enum fbe_payload_cdb_fis_error_flags_e{
    FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR             = 0x00000000,
    FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_RECOVERED            = 0x00000001,
    FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_MEDIA                = 0x00000002,
    FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE             = 0x00000004,
    FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_LINK                 = 0x00000008,
    FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_DATA                 = 0x00000010,
    FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HEALTHCHECK          = 0x00000020,

    FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_REMAPPED             = 0x00000100, 

    FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NOT_SPINNING         = 0x00010000,

    FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_UKNOWN               = 0x01000000,
    FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_END_OF_LIFE          = 0x02000000,
    FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_END_OF_LIFE_CALLHOME = 0x04000000,
    FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_FATAL                = 0x08000000,
    FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_FATAL_CALLHOME       = 0x10000000,

    FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_PORT                 = 0x20000000,
    FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_INVALID			    = 0x80000000,
}fbe_payload_cdb_fis_error_flags_t;

typedef fbe_u32_t fbe_payload_cdb_fis_error_t;

/* forward declarations */
struct fbe_payload_fis_operation_s;
struct fbe_drive_configuration_record_s;

fbe_status_t fbe_payload_cdb_completion(struct fbe_payload_cdb_operation_s * cdb_operation, /* IN */
                                        struct fbe_drive_configuration_record_s * threshold_rec, /* IN */
                                        fbe_payload_cdb_fis_io_status_t * io_status, /* OUT */
                                        fbe_payload_cdb_fis_error_t * error, /* OUT */
                                        fbe_lba_t * bad_lba); /* OUT */

fbe_status_t fbe_payload_fis_completion(struct fbe_payload_fis_operation_s * fis_operation, /* IN */
                                        fbe_drive_configuration_handle_t drive_configuration_handle, /* IN */
                                        fbe_payload_cdb_fis_io_status_t * io_status, /* OUT */
                                        fbe_payload_cdb_fis_error_t * error, /* OUT */
                                        fbe_lba_t * bad_lba); /* OUT */

#endif /* FBE_PAYLOAD_CDB_FIS_H */