#ifndef FBE_NEIT_H
#define FBE_NEIT_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_scsi_interface.h"
#include "fbe/fbe_sata_interface.h"
#include "fbe/fbe_payload_cdb_operation.h"
#include "fbe/fbe_payload_fis_operation.h"

#define MAX_CMDS 12

typedef enum fbe_neit_error_type_e {
    FBE_NEIT_ERROR_TYPE_INVALID,
    FBE_NEIT_ERROR_TYPE_SCSI,
    FBE_NEIT_ERROR_TYPE_FIS,
    FBE_NEIT_ERROR_TYPE_PORT,

    FBE_NEIT_ERROR_TYPE_LAST
}fbe_neit_error_type_t;

typedef struct fbe_neit_scsi_error_s {
    fbe_u8_t                                    scsi_command[MAX_CMDS]; /* SCSI command for which the error should be inserted. */
    fbe_scsi_sense_key_t                        scsi_sense_key;
    fbe_scsi_additional_sense_code_t            scsi_additional_sense_code;
    fbe_scsi_additional_sense_code_qualifier_t  scsi_additional_sense_code_qualifier;
    fbe_port_request_status_t                   port_status; 

}fbe_neit_scsi_error_t;

typedef struct fbe_neit_sata_error_s {
    fbe_sata_command_t          sata_command; /* SATA command for which the error should be inserted. */
    fbe_u8_t                    response_buffer[FBE_PAYLOAD_FIS_RESPONSE_BUFFER_SIZE];
    fbe_port_request_status_t   port_status;     /* dmrb->status */
}fbe_neit_sata_error_t;

typedef struct fbe_neit_port_error_s {
    /* The status the port would return if it's a port error*/
    fbe_port_request_status_t port_status; /* dmrb->status */

    /* fbe_payload_cdb_scsi_status_t port_force_scsi_status; */ /* dmrb->u.scsi.scsi_status */

}fbe_neit_port_error_t;

/* Structure for NEIT error parameters. */
typedef struct fbe_neit_error_record_s{
    /* Object id for which the error should be inserted. */
    fbe_object_id_t object_id;

    /* LBA range within which the error should be inserted. */
    fbe_lba_t lba_start;
    fbe_lba_t lba_end;

    /* The type of error SCSI, FIS or PORT */
    fbe_neit_error_type_t neit_error_type;

    union {
        fbe_neit_scsi_error_t neit_scsi_error;
        fbe_neit_sata_error_t neit_sata_error;
        fbe_neit_port_error_t neit_port_error;
    }neit_error;

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
}fbe_neit_error_record_t;

typedef void * fbe_neit_record_handle_t;

enum {
    FBE_NEIT_RECORD_HANDLE_INVALID = 0
};

/*!*******************************************************************
 * @struct fbe_neit_package_load_params_t
 *********************************************************************
 * @brief The load parameters for our neit package.
 *
 *********************************************************************/
typedef struct fbe_neit_package_load_params_s
{
    fbe_u32_t random_seed;
    fbe_trace_level_t default_trace_level; /*!< Default trace level to set. */

    fbe_trace_error_limit_t error_limit;
    fbe_trace_error_limit_t cerr_limit;

	fbe_bool_t load_raw_mirror;
}
fbe_neit_package_load_params_t;

/*!*******************************************************************
 * @def FBE_NEIT_PACKAGE_NUMBER_OF_OUTSTANDING_PACKETS
 *********************************************************************
 * @brief This is the number of packets that can be outstanding until
 *        we run out of packets.
 *
 *********************************************************************/
#define FBE_NEIT_PACKAGE_NUMBER_OF_OUTSTANDING_PACKETS 250

fbe_status_t neit_package_init(fbe_neit_package_load_params_t *params_p);
fbe_status_t neit_package_destroy(void);
fbe_status_t neit_package_add_record(fbe_neit_error_record_t * neit_error_record, fbe_neit_record_handle_t * neit_record_handle);
fbe_status_t neit_package_remove_record(fbe_neit_record_handle_t neit_record_handle);
fbe_status_t neit_package_remove_object(fbe_object_id_t object_id);
fbe_status_t neit_package_get_record(fbe_neit_record_handle_t neit_record_handle, fbe_neit_error_record_t * neit_error_record);
fbe_status_t neit_package_start(void);
fbe_status_t neit_package_stop(void);
fbe_status_t neit_package_cleanup_queue(void);

#endif /* FBE_NEIT_H */
