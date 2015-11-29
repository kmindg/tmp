#ifndef FBE_PROTOCOL_ERROR_INJECTION_SERVICE_H
#define FBE_PROTOCOL_ERROR_INJECTION_SERVICE_H

#include "fbe/fbe_service.h"

#include "fbe/fbe_scsi_interface.h"
#include "fbe/fbe_sata_interface.h"
#include "fbe/fbe_payload_cdb_operation.h"
#include "fbe/fbe_payload_fis_operation.h"

#define MAX_CMDS 12

/*!*******************************************************************
 * @def PROTOCOL_ERROR_INJECTION_MAX_ERRORS
 *********************************************************************
 * @brief Number of maximum errors inserted using the single protocol error 
 *           injection record. This number has been derived from the 
 *           FBE_MEDIA_ERROR_EDGE_TAP_MAX_RECORDS 
 *
 *********************************************************************/
#define PROTOCOL_ERROR_INJECTION_MAX_ERRORS 256

/*!*******************************************************************
 * @def PROTOCOL_ERROR_INJECTION_MAX_REASSIGN_BLOCKS
 *********************************************************************
 * @brief Number of maximum blocks that can be reassigned.
 *
 *********************************************************************/
#define PROTOCOL_ERROR_INJECTION_MAX_REASSIGN_BLOCKS 8

typedef fbe_u32_t fbe_protocol_error_injection_magic_number_t;
enum {
    FBE_PROTOCOL_ERROR_INJECTION_MAGIC_NUMBER = 0x4E454954,  /* ASCII for PROTOCOL_ERROR_INJECTION */
};

typedef enum fbe_protocol_error_injection_service_control_code_e {
	FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_INVALID = FBE_SERVICE_CONTROL_CODE_INVALID_DEF(FBE_SERVICE_ID_PROTOCOL_ERROR_INJECTION),
	FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_INIT_RECORD,
	FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_ADD_RECORD,
	FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_REMOVE_RECORD,
	FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_REMOVE_OBJECT,
	FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_START,
	FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_STOP,
	FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_GET_RECORD,
	FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_GET_RECORD_HANDLE,
    FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_GET_RECORD_NEXT,
    FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_RECORD_UPDATE_TIMES_TO_INSERT,
	FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_REMOVE_ALL_RECORDS,

	FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_LAST
} fbe_protocol_error_injection_service_control_code_t;

typedef enum fbe_protocol_error_injection_error_type_e {
    FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_INVALID,
    FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI,
    FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_FIS,
    FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_PORT,

    FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_LAST
}fbe_protocol_error_injection_error_type_t;

typedef struct fbe_protocol_error_injection_scsi_error_s {
    fbe_u8_t                                    scsi_command[MAX_CMDS]; /* SCSI command for which the error should be inserted. */
    fbe_scsi_sense_key_t                        scsi_sense_key;
    fbe_scsi_additional_sense_code_t            scsi_additional_sense_code;
    fbe_scsi_additional_sense_code_qualifier_t  scsi_additional_sense_code_qualifier;
    fbe_port_request_status_t                   port_status; 
}fbe_protocol_error_injection_scsi_error_t;

typedef struct fbe_protocol_error_injection_sata_error_s {
    fbe_sata_command_t          sata_command; /* SATA command for which the error should be inserted. */
    fbe_u8_t                    response_buffer[FBE_PAYLOAD_FIS_RESPONSE_BUFFER_SIZE];
    fbe_port_request_status_t   port_status;     /* dmrb->status */
}fbe_protocol_error_injection_sata_error_t;

typedef struct fbe_protocol_error_injection_port_error_s {
    /* The status the port would return if it's a port error*/
    fbe_bool_t      check_ranges;
    fbe_u8_t        scsi_command[MAX_CMDS];
    fbe_port_request_status_t port_status; /* dmrb->status */
    /* fbe_payload_cdb_scsi_status_t port_force_scsi_status; */ /* dmrb->u.scsi.scsi_status */
}fbe_protocol_error_injection_port_error_t;

/* Structure for PROTOCOL_ERROR_INJECTION error parameters. */
typedef struct fbe_protocol_error_injection_error_record_s{
    /* Object id for which the error should be inserted. */
    fbe_object_id_t object_id;

    /* LBA range within which the error should be inserted. */
    fbe_lba_t lba_start;
    fbe_lba_t lba_end;

    /* The type of error SCSI, FIS or PORT */
    fbe_protocol_error_injection_error_type_t protocol_error_injection_error_type;

    union {
        fbe_protocol_error_injection_scsi_error_t protocol_error_injection_scsi_error;
        fbe_protocol_error_injection_sata_error_t protocol_error_injection_sata_error;
        fbe_protocol_error_injection_port_error_t protocol_error_injection_port_error;
    }protocol_error_injection_error;

    /* Number of times the error should be inserted. */
    fbe_u32_t num_of_times_to_insert;

    /* Timestamp when this request was made inactive by being hit num_of_times_to_insert times. */
    fbe_time_t max_times_hit_timestamp;

    /* Number of seconds that a record can remain inactive after getting injected num_of_times_to_insert times. */
    fbe_u32_t secs_to_reactivate;

    /* This field controls the number of times that the record can be reactivated. */
    fbe_u32_t num_of_times_to_reactivate;

    /* Number of times the error is inserted. */
    fbe_u32_t times_inserted;

    /* How many times we reset the times_inserted field due to a reactivation. */
    fbe_u32_t times_reset;

    /* Allow to skip error insertion (we can inser error for every second IO for example) */
    fbe_u32_t frequency;

    /*! Number of blocks reassigned.
      * For a write verify IO request, if we hit a media error, we reassign 
      * the block with media error. This field tracks such reassigned blocks 
      * those are due to inserted media errors as a part of the protocol 
      * error insertion.
      */
     fbe_u32_t reassign_count;

    /*! TRUE means we are injecting this error 
     * FALSE means we are not injecting this error. 
     * Basically to decide to keep this record active or not.
     */
    fbe_bool_t b_active[PROTOCOL_ERROR_INJECTION_MAX_ERRORS];

    /*! TRUE means this is test which involves the media error insertion.
      * FALSE means this is test which does not involve the media error insertion.
      */
    fbe_bool_t b_test_for_media_error;
    /*! TRUE means this is test which involves incrementing the error count, on other disk position too.
      * FALSE means no incrementing of the error count
      */
    fbe_bool_t b_inc_error_on_all_pos;
    /*! Tag value would be common to all the error records for which we are incrementing error on other postion
      *  It is significant only when b_inc_error_on_all_pos is true
      */
    fbe_u32_t tag;

     /*! This field specifies how blks in the Write operation should be skipped (incomplete write).
      * blk_cnt in the CDB will be reduced by the amount of skip_blks. 
      */
    fbe_u32_t skip_blks;
    /*! FBE_TRUE to simply drop this request on the floor.
     */
	fbe_u32_t b_drop;

}fbe_protocol_error_injection_error_record_t;

typedef void * fbe_protocol_error_injection_record_handle_t;

/* FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_ADD_RECORD */
typedef struct fbe_protocol_error_injection_control_add_record_s
{
    fbe_protocol_error_injection_error_record_t protocol_error_injection_error_record; /* INPUT */
    fbe_protocol_error_injection_record_handle_t protocol_error_injection_record_handle; /* OUTPUT */
}fbe_protocol_error_injection_control_add_record_t;

/* FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_REMOVE_RECORD */
typedef struct fbe_protocol_error_injection_control_remove_record_s
{
    fbe_protocol_error_injection_record_handle_t protocol_error_injection_record_handle; /* INPUT */
}fbe_protocol_error_injection_control_remove_record_t;

/* FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_REMOVE_OBJECT */
typedef struct fbe_protocol_error_injection_control_remove_object_s
{
    fbe_object_id_t object_id; /* INPUT */
}fbe_protocol_error_injection_control_remove_object_t;

#endif /* FBE_PROTOCOL_ERROR_INJECTION_SERVICE_H */

