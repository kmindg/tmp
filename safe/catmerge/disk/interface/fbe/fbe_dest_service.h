#ifndef FBE_DEST_SERVICE_H
#define FBE_DEST_SERVICE_H

#include "fbe/fbe_dest.h" 

typedef enum fbe_dest_service_control_code_e {
    FBE_DEST_SERVICE_CONTROL_CODE_INVALID = FBE_SERVICE_CONTROL_CODE_INVALID_DEF(FBE_SERVICE_ID_DEST),
    FBE_DEST_SERVICE_CONTROL_CODE_INIT_RECORD,
    FBE_DEST_SERVICE_CONTROL_CODE_ADD_RECORD,
    FBE_DEST_SERVICE_CONTROL_CODE_REMOVE_RECORD,
    FBE_DEST_SERVICE_CONTROL_CODE_REMOVE_ALL_RECORDS,
    FBE_DEST_SERVICE_CONTROL_CODE_START,
    FBE_DEST_SERVICE_CONTROL_CODE_STOP,
    FBE_DEST_SERVICE_CONTROL_CODE_GET_RECORD,
    FBE_DEST_SERVICE_CONTROL_CODE_GET_RECORD_HANDLE,
     FBE_DEST_SERVICE_CONTROL_CODE_SEARCH_RECORD,
    FBE_DEST_SERVICE_CONTROL_CODE_GET_RECORD_NEXT,
    FBE_DEST_SERVICE_CONTROL_CODE_RECORD_UPDATE_TIMES_TO_INSERT,
    FBE_DEST_SERVICE_CONTROL_CODE_GET_STATE,

    FBE_DEST_SERVICE_CONTROL_CODE_LAST
} fbe_dest_service_control_code_t;


/**********************************************************************
 * DEST_STATE
 **********************************************************************
 * DEST states are defined in the following enumeration type
 **********************************************************************/
typedef enum fbe_dest_state_type
{    
    FBE_DEST_NOT_INIT=0,    /* Not yet initialized. */
    FBE_DEST_INIT,          /* Initialized but not started yet. */
    FBE_DEST_START,         /* Error insertion is in progress. */
    FBE_DEST_STOP,          /* Error insertion is stopped. */        
}fbe_dest_state_type;

/**********************************************************************
 * DEST_STATE_MONITOR_TYPE
 **********************************************************************
 * This structure contains the parameters to monitor DEST functionality.
 **********************************************************************/
typedef struct fbe_dest_state_monitor_type
{
    fbe_dest_state_type current_state;

}fbe_dest_state_monitor;

#endif /* FBE_DEST_SERVICE_H */
