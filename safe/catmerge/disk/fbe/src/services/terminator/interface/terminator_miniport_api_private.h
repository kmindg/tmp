#ifndef TERMINATOR_MINIPORT_API_PRIVATE_H
#define TERMINATOR_MINIPORT_API_PRIVATE_H

#include "fbe_terminator_miniport_interface.h"

typedef enum pmc_port_state_e {
    PMC_PORT_STATE_INVALID,
    PMC_PORT_STATE_UNINITIALIZED,
    PMC_PORT_STATE_INITIALIZED,

    PMC_PORT_STATE_LAST
}pmc_port_state_t; /* FIXME delete this! */

typedef enum cpd_port_state_e {
    CPD_PORT_STATE_INVALID,
    CPD_PORT_STATE_UNINITIALIZED,
    CPD_PORT_STATE_INITIALIZED,
    CPD_PORT_STATE_LAST
} cpd_port_state_t;

typedef enum device_table_record_state_e{
    RECORD_INVALID,
    RECORD_VALID,
    RECORD_RESERVED
}device_table_record_state_t;

typedef struct terminator_miniport_sas_device_table_record_s{
    device_table_record_state_t	record_state;
    fbe_u32_t                   generation_id;
    fbe_terminator_device_ptr_t device_handle;
}terminator_miniport_sas_device_table_record_t;

typedef struct terminator_miniport_sas_device_table_s{
	fbe_u32_t record_count;
    terminator_miniport_sas_device_table_record_t *records;
}terminator_miniport_sas_device_table_t;

typedef struct terminator_pmc_port_s{
    fbe_u32_t port_index;
    fbe_terminator_device_ptr_t port_handle;
    pmc_port_state_t state;
    fbe_cpd_shim_callback_function_t    callback_function; /* Async events from miniport */
    fbe_cpd_shim_callback_context_t     callback_context;    

    fbe_payload_ex_completion_function_t payload_completion_function; /* port completion function */
    fbe_payload_ex_completion_context_t  payload_completion_context; /* port completion context */

    fbe_cpd_shim_callback_function_t sfp_event_callback_function; /* callback function for sfp event */
    fbe_cpd_shim_callback_context_t sfp_event_callback_context;

#ifdef C4_INTEGRATED
    fbe_thread_t reset_thread;
#endif /* C4_INTEGRATED - C4BUG - possible dead code */

    terminator_miniport_sas_device_table_t device_table;
}terminator_pmc_port_t; /* FIXME delete this! */

typedef struct terminator_cpd_port_s{
    fbe_u32_t port_index;
    fbe_terminator_device_ptr_t port_handle;
    cpd_port_state_t state;
    fbe_cpd_shim_callback_function_t    callback_function; /* Async events from miniport */
    fbe_cpd_shim_callback_context_t     callback_context;    
    fbe_payload_ex_completion_function_t payload_completion_function; /* port completion function */
    fbe_payload_ex_completion_context_t  payload_completion_context; /* port completion context */
    fbe_cpd_shim_callback_function_t sfp_event_callback_function; /* callback function for sfp event */
    fbe_cpd_shim_callback_context_t sfp_event_callback_context;
    terminator_miniport_sas_device_table_t device_table;
}terminator_cpd_port_t;
//}terminator_pmc_port_t; /* FIXME delete this! */

//typedef struct terminator_cpd_port_s{
//    fbe_u32_t port_index;
//    fbe_terminator_device_ptr_t port_handle;
//    cpd_port_state_t state;
//    fbe_cpd_shim_callback_function_t    callback_function; /* Async events from miniport */
//    fbe_cpd_shim_callback_context_t     callback_context;    
//    fbe_payload_ex_completion_function_t payload_completion_function; /* port completion function */
//    fbe_payload_ex_completion_context_t  payload_completion_context; /* port completion context */
//    fbe_cpd_shim_callback_function_t sfp_event_callback_function; /* callback function for sfp event */
//    fbe_cpd_shim_callback_context_t sfp_event_callback_context;
//    terminator_miniport_sas_device_table_t device_table;
//}terminator_cpd_port_t;

fbe_status_t fbe_terminator_miniport_api_complete_io (fbe_u32_t port_index, fbe_terminator_io_t *terminator_io);
fbe_status_t fbe_terminator_miniport_api_all_devices_on_port_logged_in (fbe_terminator_device_ptr_t handle);
fbe_status_t fbe_terminator_miniport_api_all_devices_on_port_logged_out (fbe_terminator_device_ptr_t handle);

fbe_status_t fbe_terminator_miniport_api_device_logged_out (fbe_u32_t port_index, fbe_terminator_device_ptr_t device_id);


fbe_status_t terminator_miniport_device_reset_common_action(fbe_terminator_device_ptr_t device_handle);

fbe_status_t terminator_miniport_drive_power_cycle_action(fbe_terminator_device_ptr_t device_handle);
fbe_status_t fbe_terminator_miniport_api_device_table_add (fbe_u32_t port_index, 
							   fbe_terminator_device_ptr_t device_handle, 
							   fbe_u32_t *device_index);
fbe_status_t fbe_terminator_miniport_api_device_table_remove (fbe_u32_t port_index, fbe_u32_t device_index);
fbe_status_t fbe_terminator_miniport_api_device_table_reserve (fbe_u32_t port_index, fbe_miniport_device_id_t cpd_device_id);
fbe_status_t fbe_terminator_miniport_api_get_cpd_device_id(fbe_u32_t port_index, 
							   fbe_u32_t device_table_index, 
							   fbe_miniport_device_id_t *cpd_device_id);
fbe_status_t fbe_terminator_miniport_api_device_table_get_device_handle (fbe_u32_t port_index, 
							       fbe_miniport_device_id_t cpd_device_id, 
							       fbe_terminator_device_ptr_t *device_handle);
fbe_status_t fbe_terminator_miniport_api_clean_sempahores(fbe_u32_t port_index);



#endif /*TERMINATOR_MINIPORT_API_PRIVATE_H*/
