#include "fbe/fbe_types.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe_terminator_device_registry.h"
#include "fbe_terminator_class_management.h"
#include "fbe_terminator_device_classes.h"
#include "fbe_terminator.h"
#include "fbe_terminator_file_api.h"
#include "fbe_simulated_drive.h"
#include "terminator_drive.h"
#include "terminator_simulated_disk.h"

extern terminator_simulated_drive_type_t terminator_simulated_drive_type;
extern fbe_terminator_io_mode_t          terminator_io_mode;

terminator_drive_io_func terminator_drive_read;
terminator_drive_io_func terminator_drive_write;
terminator_drive_io_func terminator_drive_write_zero_pattern;

static fbe_bool_t terminator_api_initialized = FBE_FALSE;

fbe_status_t fbe_terminator_api_set_simulated_drive_type(terminator_simulated_drive_type_t simulated_drive_type)
{
    terminator_simulated_drive_type = simulated_drive_type;

    terminator_trace(FBE_TRACE_LEVEL_INFO,
                     FBE_TRACE_MESSAGE_ID_INFO,
                     "%s Set terminator drive type to: %d \n", 
                     __FUNCTION__,
                     terminator_simulated_drive_type);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_api_get_simulated_drive_type(terminator_simulated_drive_type_t *simulated_drive_type)
{
    *simulated_drive_type = terminator_simulated_drive_type;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_api_init(void)
{
    fbe_status_t     status = FBE_STATUS_GENERIC_FAILURE;
    tdr_status_t tdr_status = TDR_STATUS_GENERIC_FAILURE;
    tcm_status_t tcm_status = TCM_STATUS_GENERIC_FAILURE;

#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
    fbe_u32_t          sp;
    terminator_sp_id_t sp_id;
    fbe_char_t        *simulated_drive_server_name = NULL;
    fbe_u16_t          simulated_drive_server_port = (fbe_u16_t)-1;
#endif // UMODE_ENV || SIMMODE_ENV

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n",
                     __FUNCTION__);

    fbe_transport_init();

    /* initialize the Terminator device registry */
    if ((tdr_status = fbe_terminator_device_registry_init(TERMINATOR_MAX_NUMBER_OF_DEVICES)) != TDR_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* initialize the Terminator Class Management subsystem */
    if ((tcm_status = fbe_terminator_class_management_init()) != TCM_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* populate TCM with classes and their setters and getters */
    if ((status = terminator_device_classes_initialize()) != FBE_STATUS_OK)
    {
        return status;
    }

    /* initialize the Terminator IO function pointers */
    switch (terminator_simulated_drive_type)
    {
    case TERMINATOR_SIMULATED_DRIVE_TYPE_LOCAL_MEMORY:
        terminator_drive_read               = terminator_simulated_disk_memory_read;
        terminator_drive_write              = terminator_simulated_disk_memory_write;
        terminator_drive_write_zero_pattern = terminator_simulated_disk_memory_write_zero_pattern;
        break;
#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
    case TERMINATOR_SIMULATED_DRIVE_TYPE_REMOTE_MEMORY:
        fbe_terminator_api_get_sp_id(&sp_id);
        sp = (sp_id == TERMINATOR_SP_A ? 0 : 1);

        fbe_terminator_api_get_simulated_drive_server_name(&simulated_drive_server_name);
        fbe_terminator_api_get_simulated_drive_server_port(&simulated_drive_server_port);

        terminator_trace(FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s Initializing simulated drive server: %s port: %d sp: %d\n",
                         __FUNCTION__,
                         simulated_drive_server_name,
                         simulated_drive_server_port,
                         sp);

        if (fbe_simulated_drive_init(simulated_drive_server_name, simulated_drive_server_port, sp))
        {
            terminator_drive_read               = terminator_simulated_drive_client_read;
            terminator_drive_write              = terminator_simulated_drive_client_write;
            terminator_drive_write_zero_pattern = terminator_simulated_drive_client_write_zero_pattern;
            status = FBE_STATUS_OK;
        }
        else
        {
            terminator_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                             "%s Failed to initialize simulated drive server: %s port: %d sp: %d\n", 
                             __FUNCTION__, simulated_drive_server_name, simulated_drive_server_port, sp);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        break;
#endif // UMODE_ENV || SIMMODE_ENV
    case TERMINATOR_SIMULATED_DRIVE_TYPE_LOCAL_FILE:
        terminator_drive_read               = terminator_simulated_disk_local_file_read;
        terminator_drive_write              = terminator_simulated_disk_local_file_write;
        terminator_drive_write_zero_pattern = terminator_simulated_disk_local_file_write_zero_pattern;
        if ((status = terminator_simulated_disk_local_file_init()) != FBE_STATUS_OK)
        {
            return status;
        }
        break;
    case TERMINATOR_SIMULATED_DRIVE_TYPE_VMWARE_REMOTE_FILE:
        terminator_drive_read               = terminator_simulated_disk_remote_file_simple_read;
        terminator_drive_write              = terminator_simulated_disk_remote_file_simple_write;
        terminator_drive_write_zero_pattern = terminator_simulated_disk_remote_file_simple_write_zero_pattern;
        break;
    default:
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s Invalid terminator drive type: %d \n", 
                         __FUNCTION__,
                         terminator_simulated_drive_type);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* initialize the terminator data structures */
    if ((status = terminator_init()) != FBE_STATUS_OK)
    {
        return status;
    }

    if ((status = fbe_terminator_miniport_api_init()) != FBE_STATUS_OK)
    {
        return status;
    }

    /* this is a good place to issue a sequnace of INSERT functions in order
     *  to build a toplogy, we can do it from a file as well
     */

    terminator_api_initialized = FBE_TRUE;

    return status;
}

fbe_status_t fbe_terminator_api_destroy(void)
{
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n",
                     __FUNCTION__);

    if (!terminator_api_initialized)
    {
        return FBE_STATUS_OK;
    }

    terminator_api_initialized = FBE_FALSE;

    /* wait for enclosure firmware activate thread to avoid incorrect object attributes */
    terminator_wait_for_enclosure_firmware_activate_thread();

    fbe_terminator_class_management_destroy();
    fbe_terminator_device_registry_destroy();
    fbe_terminator_miniport_api_destroy();

    fbe_terminator_api_set_specl_sfi_entry_func(NULL);

    terminator_destroy();

    switch (terminator_simulated_drive_type)
    {
#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
    case TERMINATOR_SIMULATED_DRIVE_TYPE_REMOTE_MEMORY:
        if (!fbe_simulated_drive_cleanup())
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s fbe_simulated_drive_cleanup() failed!\n",
                             __FUNCTION__);
        }
        break;
#endif // UMODE_ENV || SIMMODE_ENV
    case TERMINATOR_SIMULATED_DRIVE_TYPE_LOCAL_FILE:
        terminator_simulated_disk_local_file_destroy();
        break;
    case TERMINATOR_SIMULATED_DRIVE_TYPE_VMWARE_REMOTE_FILE:
        terminator_simulated_disk_remote_file_simple_destroy();
        break;
    }

    fbe_transport_destroy(); /* SAFEBUG - missing cleanup */

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                     FBE_TRACE_MESSAGE_ID_INFO,
                     "%s: Done - Terminator API exit\n",
                     __FUNCTION__);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_api_set_io_mode(fbe_terminator_io_mode_t io_mode)
{

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    return fbe_terminator_set_io_mode(io_mode);
}

fbe_status_t fbe_terminator_api_get_io_mode(fbe_terminator_io_mode_t *io_mode)
{
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    return fbe_terminator_get_io_mode(io_mode);
}

fbe_status_t fbe_terminator_api_set_io_completion_irql(fbe_bool_t b_is_io_completion_at_dpc)
{

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    return fbe_terminator_set_io_completion_at_dpc(b_is_io_completion_at_dpc);
}

fbe_status_t fbe_terminator_api_set_need_update_enclosure_firmware_rev(fbe_bool_t  update_rev)
{
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    return terminator_set_need_update_enclosure_firmware_rev(update_rev);
}

fbe_status_t fbe_terminator_api_get_need_update_enclosure_firmware_rev(fbe_bool_t  *update_rev)
{
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    return terminator_get_need_update_enclosure_firmware_rev(update_rev);
}

fbe_status_t fbe_terminator_api_set_need_update_enclosure_resume_prom_checksum(fbe_bool_t  need_update_checksum)
{
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    return terminator_set_need_update_enclosure_resume_prom_checksum(need_update_checksum);
}

fbe_status_t fbe_terminator_api_get_need_update_enclosure_resume_prom_checksum(fbe_bool_t  *need_update_checksum)
{
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    return terminator_get_need_update_enclosure_resume_prom_checksum(need_update_checksum);
}

fbe_status_t fbe_terminator_api_set_io_global_completion_delay(fbe_u32_t global_completion_delay)
{
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    return fbe_terminator_set_io_global_completion_delay(global_completion_delay);
}

fbe_status_t fbe_terminator_api_find_device_class(
                        const char * device_class_name,
                        fbe_terminator_api_device_class_handle_t * device_class_handle)
{
    tcm_status_t tcm_status;
    tcm_reference_t tcm_class;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tcm_status = fbe_terminator_class_management_class_find(device_class_name, &tcm_class);
    if(tcm_status != TCM_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: could not find TCM class for %s\n", __FUNCTION__, device_class_name);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        /*  Since fbe_terminator_api_device_class_handle_t and tcm_reference_t
            are both void* pointers, we can use simple type casting to convert
            one to another. */
        *device_class_handle = (fbe_terminator_api_device_class_handle_t)tcm_class;
        return FBE_STATUS_OK;
    }
}

fbe_status_t fbe_terminator_api_create_device_class_instance(
                        fbe_terminator_api_device_class_handle_t device_class_handle,
                        const char * device_type,
                        fbe_terminator_api_device_handle_t * device_handle)
{
    tcm_status_t tcm_status;
    tcm_reference_t tcm_instance;
    tdr_status_t tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tcm_status = fbe_terminator_class_management_create_instance(
        (tcm_reference_t)device_class_handle, /* simple type casting can be used here */
        device_type,
        &tcm_instance);
    if(tcm_status != TCM_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: could not instantiate via TCM.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    tdr_status = fbe_terminator_device_registry_add_device((tdr_device_type_t)(csx_ptrhld_t) device_class_handle,
                                                               (tdr_device_ptr_t) tcm_instance,
                                                               device_handle);
    if(tdr_status != TDR_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: could not add device to TDR.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_api_set_device_attribute(fbe_terminator_api_device_handle_t device_handle,
                                                    const char * attribute_name,
                                                    const char * attribute_value)
{
    tdr_device_ptr_t device_ptr;
    tdr_status_t status;
    tcm_status_t tcm_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    status = fbe_terminator_device_registry_get_device_ptr(device_handle, &device_ptr);
    if(status != TDR_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    tcm_status = terminator_set_device_attribute(device_ptr, attribute_name, attribute_value);
    return (tcm_status == TCM_STATUS_OK) ? FBE_STATUS_OK : FBE_STATUS_GENERIC_FAILURE;
}

fbe_status_t fbe_terminator_api_get_device_attribute(fbe_terminator_api_device_handle_t device_handle,
                                                    const char * attribute_name,
                                                    char * attribute_value_buffer,
                                                    fbe_u32_t buffer_length)
{
    tdr_device_ptr_t device_ptr;
    tdr_status_t status;
    tcm_status_t tcm_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    status = fbe_terminator_device_registry_get_device_ptr(device_handle, &device_ptr);
    if(status != TDR_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    tcm_status = terminator_get_device_attribute(device_ptr, attribute_name, attribute_value_buffer, buffer_length);
    return (tcm_status == TCM_STATUS_OK) ? FBE_STATUS_OK : FBE_STATUS_GENERIC_FAILURE;
}

fbe_status_t fbe_terminator_api_force_login_device (fbe_terminator_api_device_handle_t device_handle)
{
    tdr_device_ptr_t    device_ptr;
    tdr_status_t        status;
    fbe_status_t        return_status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t           port_index;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    status = fbe_terminator_device_registry_get_device_ptr(device_handle, &device_ptr);
    if(status != TDR_STATUS_OK)
    {
        return return_status;
    }
    /* add the devices to the logout queue and let the thread do the rest*/
    return_status = terminator_set_device_login_pending (device_ptr);
    if (return_status != FBE_STATUS_OK) {
        return return_status;
    }

    /*once it's add to logout queue, we need to generate a logout*/
    return_status = terminator_get_port_index(device_ptr, &port_index);
    return_status = fbe_terminator_miniport_api_device_state_change_notify(port_index);
    return return_status;
}

fbe_status_t fbe_terminator_api_force_logout_device (fbe_terminator_api_device_handle_t device_handle)
{
    /* Logout a device should generate logout to port shim for the device and all it's children */
    /* before logout any device, the device io queue should be emptied.
       If the io queue is not empty, each io should be aborted */
    /* here are the steps:
     * 1) find the devices to logout and add them to logout queue, but not remove them from the tree
     * 2) miniport_api event thread will check the logout queue
     * 3) miniport_api pops a device, abort the io_queue, sends logout to port and mark the device logout_complete.
     * And we should be done with the logout
     */
    tdr_device_ptr_t    device_ptr;
    tdr_status_t        status;
    fbe_status_t        return_status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t           port_index;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    status = fbe_terminator_device_registry_get_device_ptr(device_handle, &device_ptr);
    if(status != TDR_STATUS_OK)
    {
        return return_status;
    }
    /* add the devices to the logout queue and let the thread do the rest*/
    return_status = terminator_set_device_logout_pending (device_ptr);
    if (return_status != FBE_STATUS_OK) {
        return return_status;
    }

    /*once it's add to logout queue, we need to generate a logout*/
    return_status = terminator_get_port_index(device_ptr, &port_index);
    return_status = fbe_terminator_miniport_api_device_state_change_notify(port_index);
    return return_status;
}

fbe_status_t fbe_terminator_api_remove_device (fbe_terminator_api_device_handle_t device_handle)
{
    tdr_device_ptr_t device_ptr;
    tdr_status_t status;
    fbe_status_t return_status = FBE_STATUS_GENERIC_FAILURE;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    status = fbe_terminator_device_registry_get_device_ptr(device_handle, &device_ptr);
    if(status != TDR_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return_status = fbe_terminator_remove_device(device_ptr);
    return return_status;
}

fbe_status_t fbe_terminator_api_unmount_device (fbe_terminator_api_device_handle_t device_handle)
{
    tdr_device_ptr_t device_ptr;
    tdr_status_t status;
    fbe_status_t return_status = FBE_STATUS_GENERIC_FAILURE;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    status = fbe_terminator_device_registry_get_device_ptr(device_handle, &device_ptr);
    if(status != TDR_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return_status = fbe_terminator_unmount_device(device_ptr);
    return return_status;
}

/* Device reset interface */
fbe_status_t fbe_terminator_api_register_device_reset_function (fbe_terminator_api_device_handle_t device_handle,
                                                                fbe_terminator_device_reset_function_t reset_function)
{
    tdr_device_ptr_t device_ptr;
    tdr_status_t status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    status = fbe_terminator_device_registry_get_device_ptr(device_handle, &device_ptr);
    if(status != TDR_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return terminator_register_device_reset_function(device_ptr, reset_function);
}

fbe_status_t fbe_terminator_api_set_device_reset_delay (fbe_terminator_api_device_handle_t device_handle, fbe_u32_t delay_in_ms)
{
    tdr_device_ptr_t device_ptr;
    tdr_status_t status;
    status = fbe_terminator_device_registry_get_device_ptr(device_handle, &device_ptr);
    if(status != TDR_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return terminator_set_device_reset_delay(device_ptr, delay_in_ms);
}

fbe_status_t fbe_terminator_api_get_device_cpd_device_id(const fbe_terminator_api_device_handle_t device_handle, fbe_miniport_device_id_t *cpd_device_id)
{
    tdr_device_ptr_t device_ptr;
    tdr_status_t status;

    status = fbe_terminator_device_registry_get_device_ptr(device_handle, &device_ptr);
    if(status != TDR_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return terminator_get_cpd_device_id_from_miniport(device_ptr, cpd_device_id);
}

fbe_status_t fbe_terminator_api_reserve_miniport_sas_device_table_index(const fbe_u32_t port_number, const fbe_miniport_device_id_t cpd_device_id)
{
    return terminator_reserve_cpd_device_id (port_number, cpd_device_id);
}

fbe_status_t fbe_terminator_api_miniport_sas_device_table_force_add(const fbe_terminator_api_device_handle_t device_handle, const fbe_miniport_device_id_t cpd_device_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    tdr_device_ptr_t device_ptr;
    tdr_status_t tdr_status;
    tdr_status = fbe_terminator_device_registry_get_device_ptr(device_handle, &device_ptr);
    if(tdr_status != TDR_STATUS_OK)
    {
        return status;
    }
    status = terminator_set_miniport_sas_device_table_index(device_ptr, cpd_device_id);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }
    status = terminator_add_device_to_miniport_sas_device_table(device_ptr);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }
    status = terminator_update_device_cpd_device_id(device_ptr);
    return status;
}
fbe_status_t fbe_terminator_api_miniport_sas_device_table_force_remove(const fbe_terminator_api_device_handle_t device_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t port_index;
    tdr_device_ptr_t device_ptr;
    tdr_status_t tdr_status;
    tdr_status = fbe_terminator_device_registry_get_device_ptr(device_handle, &device_ptr);
    if(tdr_status != TDR_STATUS_OK)
    {
        return status;
    }
    status = terminator_get_port_index(device_ptr, &port_index);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }
    status = terminator_remove_device_from_miniport_api_device_table(port_index, device_ptr);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    status = terminator_set_miniport_sas_device_table_index(device_ptr, INVALID_TMSDT_INDEX);
    return status;
}

fbe_status_t fbe_terminator_api_activate_device(fbe_terminator_api_device_handle_t device_handle)
{
    tdr_device_ptr_t device_ptr;
    tdr_status_t status;
    status = fbe_terminator_device_registry_get_device_ptr(device_handle, &device_ptr);
    if(status != TDR_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return terminator_activate_device(device_ptr);
}

fbe_u32_t fbe_terminator_api_get_devices_count()
{
    /* wrap Terminator device registry function call */
    return (fbe_u32_t)fbe_terminator_device_registry_get_device_count();
}

fbe_status_t fbe_terminator_api_get_devices_count_by_type_name(const fbe_u8_t *device_type_name, fbe_u32_t  *const device_count)
{
    /* wrap Terminator device registry function call */
    tcm_reference_t tcm_class;
    tcm_status_t tcm_status = fbe_terminator_class_management_class_find(device_type_name, &tcm_class);
    if(tcm_status != TCM_STATUS_OK)
    {
      *device_count = 0;
      return FBE_STATUS_GENERIC_FAILURE;
    }else {

    *device_count =  fbe_terminator_device_registry_get_device_count_by_type((tdr_device_type_t)(fbe_ptrhld_t)tcm_class);

    return  FBE_STATUS_OK ;
    }

}
fbe_status_t fbe_terminator_api_enumerate_devices(fbe_terminator_api_device_handle_t device_handle_array[],
                                                  fbe_u32_t number_of_devices)
{
    /* wrap Terminator device registry function call */
    tdr_status_t tdr_status =  fbe_terminator_device_registry_enumerate_all_devices(
                                    (tdr_device_handle_t *)device_handle_array,
                                    (tdr_u32_t)number_of_devices);
    return ( tdr_status == TDR_STATUS_OK ) ? FBE_STATUS_OK : FBE_STATUS_GENERIC_FAILURE;
}

fbe_status_t fbe_terminator_api_insert_device(fbe_terminator_api_device_handle_t parent_device,
                                              fbe_terminator_api_device_handle_t child_device)
{
    tdr_device_ptr_t p_device_ptr, c_device_ptr;
    tdr_status_t status;
    status = fbe_terminator_device_registry_get_device_ptr(parent_device, &p_device_ptr);
    if(status != TDR_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_terminator_device_registry_get_device_ptr(child_device, &c_device_ptr);
    if(status != TDR_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return terminator_insert_device(p_device_ptr, c_device_ptr);
}

