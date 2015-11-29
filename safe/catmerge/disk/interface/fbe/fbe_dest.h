#ifndef FBE_DEST_H
#define FBE_DEST_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_atomic.h"
#include "fbe/fbe_scsi_interface.h"
#include "fbe/fbe_sata_interface.h"
#include "fbe/fbe_payload_cdb_operation.h"
#include "fbe/fbe_payload_fis_operation.h"
#include "fbe/fbe_dest_service.h"
#include "fbe/fbe_atomic.h"

#define MAX_CMDS 12
#define DEST_MAX_STR_LEN_HERE 64

typedef void * fbe_dest_record_handle_t;

typedef enum fbe_dest_error_type_e {
    FBE_DEST_ERROR_TYPE_INVALID,
    FBE_DEST_ERROR_TYPE_NONE,
    FBE_DEST_ERROR_TYPE_SCSI,   
    FBE_DEST_ERROR_TYPE_PORT,
    FBE_DEST_ERROR_TYPE_FIS,
    FBE_DEST_ERROR_TYPE_GLITCH,
    FBE_DEST_ERROR_TYPE_FAIL,
    /* add new types here.  Change values above will break xml */

    FBE_DEST_ERROR_TYPE_LAST
}fbe_dest_error_type_t;

typedef enum fbe_dest_reactivation_gap_type_e {
    FBE_DEST_REACT_GAP_INVALID,
    FBE_DEST_REACT_GAP_NONE,
    FBE_DEST_REACT_GAP_TIME,
    FBE_DEST_REACT_GAP_IO_COUNT,

}fbe_dest_reactivation_gap_type_t;

typedef struct fbe_dest_scsi_error_s {
    /*TODO: dest has a bug which uses scsi_error struct when it's a port error.  To make sure these 
      two structs are compatible, the port_status must be first.  Eventually this needs to be fixed.  -wayne */
    fbe_port_request_status_t                   port_status;  
    fbe_u8_t                                    scsi_command[MAX_CMDS]; /* SCSI command for which the error should be inserted. */
    fbe_u32_t                                   scsi_command_count;
    fbe_scsi_sense_key_t                        scsi_sense_key;
    fbe_scsi_additional_sense_code_t            scsi_additional_sense_code;
    fbe_scsi_additional_sense_code_qualifier_t  scsi_additional_sense_code_qualifier;
    fbe_payload_cdb_scsi_status_t               scsi_status;

}fbe_dest_scsi_error_t;

typedef struct fbe_dest_sata_error_s {
    fbe_sata_command_t          sata_command; /* SATA command for which the error should be inserted. */
    fbe_u8_t                    response_buffer[FBE_PAYLOAD_FIS_RESPONSE_BUFFER_SIZE];
    fbe_port_request_status_t   port_status;     /* dmrb->status */
}fbe_dest_sata_error_t;

typedef struct fbe_dest_port_error_s {
    /* The status the port would return if it's a port error*/
    /* TODO, this must always be first member.  See TODO above in fbe_dest_scsi_error_t */
    fbe_port_request_status_t port_status; /* dmrb->status */

    /* fbe_payload_cdb_scsi_status_t port_force_scsi_status; */ /* dmrb->u.scsi.scsi_status */

}fbe_dest_port_error_t;


/* Flag used to control error injection, to be used as a bitmask */
typedef enum fbe_dest_injection_flag_e {
    FBE_DEST_INJECTION_ON_SEND_PATH = 0x1,   /*error is injected on send path and returned immediately. i.e. not sent to drive.
                                               If not set then injects on completion path, which is the default.*/

} fbe_dest_injection_flag_t;

// add more flags here.

/**********************************************************************
 * DEST_RECORD_LOG_TYPE
 **********************************************************************
 * This is the log structure for each and every error rule. At this 
 * point, this structure contains only one variable that identifies 
 * the number of times the error is inserted. 
 **********************************************************************/
typedef struct FBE_DEST_RECORD_LOG_TYPE
{
    /* Number of times the error is inserted.
     */
    fbe_u32_t times_inserted;

    /* Field that keeps track of how many times we reset the
     * times_inserted field due to a reactivation.
     */
    fbe_u32_t times_reset;
    
}FBE_DEST_RECORD_LOG;

/* Structure for DEST error parameters. */
typedef struct fbe_dest_error_record_s{
    /* Object id for which the error should be inserted. */
    fbe_object_id_t object_id;

    /* physical location */
    fbe_u32_t bus, enclosure, slot;

    /* LBA range within which the error should be inserted. */
    fbe_lba_t lba_start;
    fbe_lba_t lba_end;

 /* This is the opcode for which the error should be inserted. Default 
    * is set to be N/A. This is set as a string since the user can 
    * refer to it by symbolic name. Refer to dest_usr_intf.c for the 
    * SCSI Opcode lookup table. 
    */
    TEXT opcode[DEST_MAX_STR_LEN_HERE];
    /* The type of error SCSI, FIS or PORT */
    fbe_dest_error_type_t dest_error_type;

    union {                   
        fbe_dest_scsi_error_t dest_scsi_error;
        fbe_dest_sata_error_t dest_sata_error;
        fbe_dest_port_error_t dest_port_error;
    }dest_error;


    /* Additional sense data fields */
    fbe_u32_t valid_lba;
    fbe_u32_t deferred;

    /* Number of times the error should be inserted. */
    fbe_u32_t num_of_times_to_insert;

    /* Record reactivation settings */
    fbe_dest_reactivation_gap_type_t react_gap_type;

    /* Number of milliseconds that a record can remain inactive after getting injected num_of_times_to_insert times. */
    fbe_u32_t react_gap_msecs;

    /* Number of IOs that will be skipped before record is reactivated*/
    fbe_u32_t react_gap_io_count;

    /* Random gap between record reactivation */
    fbe_bool_t is_random_gap;

    /* Following specifies values if using random gap. */
    /* Max random milliseconds to wait before reactivation.
     */
    fbe_u32_t max_rand_msecs_to_reactivate;

    /* Max IOs to skip before reactivation.
     */
    fbe_u32_t max_rand_react_gap_io_count;


    /* These fields control the number of times that the record can be
     * reactivated. 
     */
    fbe_bool_t is_random_react_interations;
    fbe_u32_t num_of_times_to_reactivate;
	
	/* Allow to skip error insertion (we can inser error for every second IO for example) */
	fbe_u32_t frequency;

    /* Errors can be injected randomly based on max frequency.  IOs will be randomly injected from 0..frequency-1 */
    fbe_bool_t is_random_frequency;

    /* Flag that controls error injection */
    fbe_dest_injection_flag_t injection_flag;

    /* Delay IO.  In msec.  See FBE_DEST_TIMER_PERIODIC_WAKEUP for thread wakeup resolution, on order of 100ms*/
    fbe_u32_t delay_io_msec; 

    /* Error insertion tracker */
    fbe_atomic_32_t is_active_record;

    /* This is the logs structure. At this point, there is only one field
    * in this. This is pulled out as a structure to make it modular and
    * ease any future additions to log structures.
    */
    FBE_DEST_RECORD_LOG logs;
    /*******************************************/
    /* INTERNALLY USED - NOT TO BE SET BY USER */
    
    /* Timestamp when this request was made inactive by being hit num_of_times_to_insert times. */
    fbe_time_t max_times_hit_timestamp;

    /* How many times we reset the times_inserted field due to a reactivation. */
    fbe_u32_t times_reset;

    fbe_atomic_32_t times_inserting; /* Number of qualified IOs that will be inserted. Used to keep track
                                  outstanding IOs to be inserted to prevent us from injecting too
                                  many IOs */
    fbe_atomic_32_t times_inserted; /* Number of times the error is inserted. */

    fbe_u32_t react_gap_io_skipped;  /* number of IOs skipped between reactivation*/

    fbe_u32_t qualified_io_counter;  /* keeps track of number of qualified IOs to be inserted.  Used for calculating error 
                                     insertion based on frequency. */
	
	fbe_u64_t glitch_time; /* time for which the drive is in glitch mode. In msec.*/

    union
    {
        /* Number of seconds that a record can remain inactive after getting injected num_of_times_to_insert times. */
        fbe_u32_t rand_secs_to_reactivate;
    
        /* Number of IOs that will be skipped before record is reactivated*/
        fbe_u32_t rand_react_gap_io_count;
    };

    /* Does the object specificied in this record require descriptor format sense data */
    fbe_bool_t requires_descriptor_format_sense_data;   
    
    /* Unique record id for each record that's added. */
    fbe_u32_t record_id;

}fbe_dest_error_record_t;

/* FBE_DEST_SERVICE_CONTROL_CODE_ADD_RECORD */
typedef struct fbe_dest_control_add_record_s
{
    fbe_dest_error_record_t dest_error_record; /* INPUT */
    fbe_dest_record_handle_t dest_record_handle; /* OUTPUT */
}fbe_dest_control_add_record_t;

/* FBE_DEST_SERVICE_CONTROL_CODE_REMOVE_RECORD */
typedef struct fbe_dest_control_remove_record_s
{
    fbe_dest_record_handle_t dest_record_handle; /* INPUT */
}fbe_dest_control_remove_record_t;

enum {
    FBE_DEST_RECORD_HANDLE_INVALID = 0
};

fbe_status_t fbe_dest_init(void);
fbe_status_t fbe_dest_destroy(void);
fbe_status_t fbe_dest_add_record(fbe_dest_error_record_t * dest_error_record, fbe_dest_record_handle_t * dest_record_handle);
fbe_status_t fbe_dest_remove_record(fbe_dest_record_handle_t dest_record_handle);
fbe_status_t fbe_dest_remove_object(fbe_object_id_t object_id);
fbe_status_t fbe_dest_get_record(fbe_dest_record_handle_t dest_record_handle, fbe_dest_error_record_t * dest_error_record);
fbe_status_t fbe_dest_get_record_next(fbe_dest_record_handle_t * dest_record_handle, fbe_dest_error_record_t * dest_error_record);
fbe_status_t fbe_dest_get_record_handle(fbe_dest_record_handle_t * dest_record_handle, fbe_dest_error_record_t * dest_error_record);
fbe_status_t fbe_dest_search_record(fbe_dest_record_handle_t * dest_record_handle, fbe_dest_error_record_t * dest_error_record);
fbe_status_t fbe_dest_update_times_to_insert(fbe_dest_record_handle_t * dest_record_handle, fbe_dest_error_record_t * dest_error_record);
fbe_status_t fbe_dest_start(void);
fbe_status_t fbe_dest_stop(void);
fbe_status_t fbe_dest_cleanup_queue(void);

#endif /* FBE_DEST_H */
