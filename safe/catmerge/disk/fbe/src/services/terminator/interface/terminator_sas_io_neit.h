#ifndef TERMINATOR_SAS_NEIT_H
#define TERMINATOR_SAS_NEIT_H


#include "fbe/fbe_types.h"
#include "fbe_terminator.h"
#include "fbe_scsi.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_time.h"


/* This is an invalid number of seconds.
 */
#define NEIT_INVALID_SECONDS   CSX_CONST_U32(0xFFFFFFFF)

/* This is for invalid drive.
 */
#define NEIT_INVALID_POSITION 0xFFFFFFFF

/* This is for invalid range.
 */
#define NEIT_INVALID_RANGE  0xFFFFFFFF

/* When num_of_times_to_reactivate is set to this value, the error 
 * record will always be reactivated after the reset time is past.  
 * It's OK to use 0 because reactivation is turned off by setting 
 * secs_to_reactivate to NEIT_INVALID_SECONDS, rather than by 
 * setting num_of_times to 0.
 */
#define NEIT_ALWAYS_REACTIVATE  0

/* 0xFFFFFFFF is used for any range identification with in 
 * the NEIT module. Hex values are defaulted with 0xFFFFFFFF. 
 */
#define NEIT_ANY_RANGE  0xFFFFFFFF

#define NEIT_ANY_OPCODE 0xFF
#define NEIT_INVALID_SENSE 0xFF

#define NEIT_SIX_BYTE_CDB 6
#define NEIT_TEN_BYTE_CDB 10


 /**********************************************************************
 * The following is the type definition of the search parameters table. 
 * These values are used to search the error table for any existing
 * error records. 
 **********************************************************************/
typedef struct fbe_neit_search_params_s
{
    neit_drive_t drive;
    fbe_lba_t lba_start;
    fbe_lba_t lba_end;
    fbe_u8_t opcode;
}fbe_neit_search_params_t;

 /**********************************************************************
 * NEIT states are defined in the following enumeration type
 **********************************************************************/
typedef enum fbe_neit_state_e{    
    NEIT_NOT_INIT,   /* Not yet initialized. */
    NEIT_INIT,       /* Initialized but not started yet. */
    NEIT_START,      /* Error insertion is in progress. */
    NEIT_STOP,       /* Error insertion is stopped. */    
}fbe_neit_state_t;


 /**********************************************************************
 * This structure contains the parameters to monitor NEIT functionality.
 **********************************************************************/
typedef struct fbe_neit_state_monitor_t{
    fbe_neit_state_t current_state;
}fbe_neit_state_monitor_s;


 /********************************************************************
 * Structure for Error Rules in List for a specific drive. This is a 
 * linked  list implementation.
 *********************************************************************/
typedef struct fbe_neit_err_list_s
{

    fbe_queue_element_t  queue_element;
    fbe_terminator_neit_error_record_t*  error_record;    /* Error Record */

} fbe_neit_err_list_t;


/**********************************************************************
 * Structure for Error lookup table. This is a linked list 
 * implementation.
 **********************************************************************/
typedef struct fbe_neit_drive_error_table_s
{

    fbe_queue_element_t  queue_element;
    neit_drive_t drive;
    fbe_queue_head_t     neit_err_list_head;

} fbe_neit_drive_error_table_t;



 /**********************************************************************
 * Function Declarations
 **********************************************************************/

fbe_status_t fbe_terminator_sas_payload_insert_error(fbe_payload_cdb_operation_t  *payload_cdb_operation,
                                                     fbe_terminator_device_ptr_t drive_handle);
fbe_status_t  fbe_neit_init(void);
fbe_status_t  fbe_neit_close(void);
fbe_status_t  fbe_neit_error_injection_start(void);
fbe_status_t  fbe_neit_error_injection_stop(void); 
fbe_status_t  fbe_neit_insert_error_record(fbe_terminator_neit_error_record_t  error_record_to_add);

fbe_terminator_neit_error_record_t*  fbe_neit_search_error_rules(fbe_neit_search_params_t* search);
fbe_status_t  fbe_neit_validate_new_record(fbe_terminator_neit_error_record_t*  error_record);
fbe_neit_drive_error_table_t*  fbe_neit_search_drive_table(neit_drive_t drive_to_search_for);
fbe_status_t  fbe_neit_add_err_list(fbe_neit_drive_error_table_t*  error_table_addition_point,
                                fbe_terminator_neit_error_record_t*  error_record_to_add);
fbe_status_t  fbe_neit_add_drive_table(fbe_terminator_neit_error_record_t*  error_record_to_add);
fbe_status_t  fbe_error_record_init(fbe_terminator_neit_error_record_t*  record);
fbe_status_t  fbe_error_record_copy(fbe_terminator_neit_error_record_t*  to_record, fbe_terminator_neit_error_record_t*  from_record);
fbe_status_t  fbe_drive_init(neit_drive_t* drive);
fbe_bool_t    fbe_drive_match(neit_drive_t drive_1, neit_drive_t drive_2);
fbe_status_t  fbe_drive_copy(neit_drive_t* to_drive, neit_drive_t* from_drive);

#endif /*TERMINATOR_SAS_NEIT_H*/

