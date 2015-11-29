#include "fbe_terminator.h"
#include "fbe_terminator_device_registry.h"
#include "fbe_terminator_class_management.h"
#include "terminator_sas_io_neit.h"
#include "terminator_drive.h"

/*
 * Just the wrapper for the corresponding terminator function
 * fbe_terminator_remove_drive().
 */
fbe_status_t fbe_terminator_api_remove_sas_drive (fbe_terminator_api_device_handle_t drive_handle)
{
    tdr_device_ptr_t device_ptr;
    tdr_status_t status = TDR_STATUS_GENERIC_FAILURE;
    fbe_status_t fbe_status = FBE_STATUS_GENERIC_FAILURE;
    status = fbe_terminator_device_registry_get_device_ptr(drive_handle, &device_ptr);
    if(status != TDR_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_status = fbe_terminator_remove_drive(device_ptr);
    if (fbe_status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, 
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: fbe_terminator_api_remove_sas_drive failed\n", __FUNCTION__);
        return fbe_status;
    }

    return fbe_status;
}

/*
 * Just the wrapper for the corresponding terminator function
 * fbe_terminator_pull_drive().
 */
fbe_status_t fbe_terminator_api_pull_drive (fbe_terminator_api_device_handle_t drive_handle)
{
    tdr_device_ptr_t device_ptr;
    tdr_status_t status = TDR_STATUS_GENERIC_FAILURE;
    fbe_status_t fbe_status = FBE_STATUS_GENERIC_FAILURE;
    status = fbe_terminator_device_registry_get_device_ptr(drive_handle, &device_ptr);
    if(status != TDR_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_status = fbe_terminator_pull_drive(device_ptr);
    if (fbe_status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: fbe_terminator_api_pull_drive failed\n", __FUNCTION__);
        return fbe_status;
    }

    return fbe_status;
}

/*
 * Just the wrapper for the corresponding terminator function
 * fbe_terminator_reinsert_drive().
 */
fbe_status_t fbe_terminator_api_reinsert_drive (fbe_terminator_api_device_handle_t enclosure_handle,
												  fbe_u32_t slot_number,
												  fbe_terminator_api_device_handle_t drive_handle)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_terminator_reinsert_drive(enclosure_handle, slot_number, drive_handle);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: fbe_terminator_api_reinsert_drive failed\n", __FUNCTION__);
        return status;
    }

    return status;
}

/* Should be replaced with attr getter */
fbe_status_t fbe_terminator_api_get_sas_drive_info(fbe_terminator_api_device_handle_t drive_handle, fbe_terminator_sas_drive_info_t *drive_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    tdr_status_t tdr_status = TDR_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t device_ptr;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(drive_handle, &device_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    status = terminator_get_sas_drive_info(device_ptr, drive_info);
    return status;
}

/* Should be replaced with attr setter */
fbe_status_t fbe_terminator_api_set_sas_drive_info(fbe_terminator_api_device_handle_t drive_handle, fbe_terminator_sas_drive_info_t *drive_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    tdr_status_t tdr_status = TDR_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t device_ptr;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(drive_handle, &device_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    status = terminator_set_sas_drive_info(device_ptr, drive_info);
    return status;
}

fbe_status_t fbe_terminator_api_get_drive_error_count(fbe_terminator_api_device_handle_t device_handle, fbe_u32_t  *const error_count_p)
{
    tdr_status_t status = TDR_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t device_ptr;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    status = fbe_terminator_device_registry_get_device_ptr(device_handle, &device_ptr);
    if (status == TDR_STATUS_GENERIC_FAILURE)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *error_count_p = drive_get_error_count((terminator_drive_t*)device_ptr);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_api_clear_drive_error_count(fbe_terminator_api_device_handle_t device_handle)
{
    tdr_status_t status = TDR_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t device_ptr;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    status = fbe_terminator_device_registry_get_device_ptr(device_handle, &device_ptr);
    if (status == TDR_STATUS_GENERIC_FAILURE)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    drive_clear_error_count((terminator_drive_t*)device_ptr);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_api_remove_sata_drive (fbe_terminator_api_device_handle_t drive_handle)
{
    tdr_status_t status = TDR_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t device_ptr;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    status = fbe_terminator_device_registry_get_device_ptr(drive_handle, &device_ptr);
    if (status == TDR_STATUS_GENERIC_FAILURE)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return fbe_terminator_remove_drive(device_ptr);
}

/* FIXME: should be replaced with  fbe_terminator_api_get_device_attribute */
fbe_status_t fbe_terminator_api_get_sata_drive_info(fbe_terminator_api_device_handle_t drive_handle, fbe_terminator_sata_drive_info_t *drive_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    tdr_status_t tdr_status = TDR_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t device_ptr;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(drive_handle, &device_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    status = terminator_get_sata_drive_info(device_ptr, drive_info);
    return status;
}

fbe_status_t fbe_terminator_api_set_sata_drive_info(fbe_terminator_api_device_handle_t drive_handle, fbe_terminator_sata_drive_info_t *drive_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    tdr_status_t tdr_status = TDR_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t device_ptr;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(drive_handle, &device_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    status = terminator_set_sata_drive_info(device_ptr, drive_info);
    return status;
}

fbe_status_t fbe_terminator_api_set_drive_product_id(fbe_terminator_api_device_handle_t drive_handle, const fbe_u8_t * product_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    tdr_status_t tdr_status = TDR_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t device_ptr;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(drive_handle, &device_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    status = terminator_set_drive_product_id(device_ptr, product_id);
    return status;
}

fbe_status_t fbe_terminator_api_get_drive_product_id(fbe_terminator_api_device_handle_t drive_handle, fbe_u8_t * product_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    tdr_status_t tdr_status = TDR_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t device_ptr;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(drive_handle, &device_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    status = terminator_get_drive_product_id(device_ptr, product_id);
    return status;
}

fbe_status_t fbe_terminator_api_drive_get_state (fbe_terminator_api_device_handle_t device_handle,
												 terminator_sas_drive_state_t * drive_state)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    tdr_status_t tdr_status = TDR_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t device_ptr;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(device_handle, &device_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    status = terminator_drive_get_state(device_ptr, drive_state);
	return status;
}

fbe_status_t fbe_terminator_api_drive_set_state (fbe_terminator_api_device_handle_t device_handle,
                                                 terminator_sas_drive_state_t  drive_state)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    tdr_status_t tdr_status = TDR_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t device_ptr;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(device_handle, &device_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    status = terminator_drive_set_state(device_ptr, drive_state);
	return status;
}

fbe_status_t fbe_terminator_api_drive_get_default_page_info(fbe_sas_drive_type_t drive_type, fbe_terminator_sas_drive_type_default_info_t *default_info_p)
{
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    return terminator_sas_drive_get_default_page_info(drive_type, default_info_p);
}

fbe_status_t fbe_terminator_api_drive_set_default_page_info(fbe_sas_drive_type_t drive_type, const fbe_terminator_sas_drive_type_default_info_t *default_info_p)
{
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    return terminator_sas_drive_set_default_page_info(drive_type, default_info_p);
}

fbe_status_t fbe_terminator_api_sas_drive_set_default_field(fbe_sas_drive_type_t drive_type, fbe_terminator_drive_default_field_t field, fbe_u8_t *data, fbe_u32_t size)
{
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    return terminator_sas_drive_set_default_field(drive_type, field, data, size);
}

/* NEIT interface */
fbe_status_t fbe_terminator_api_drive_error_injection_init(void)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    status = fbe_neit_init();
    return status;
}

fbe_status_t fbe_terminator_api_drive_error_injection_destroy(void)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    status = fbe_neit_close();
    return status;
}

fbe_status_t fbe_terminator_api_drive_error_injection_start(void)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    status = fbe_neit_error_injection_start();
    return status;
}

fbe_status_t fbe_terminator_api_drive_error_injection_stop(void)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    status = fbe_neit_error_injection_stop();
    return status;
}

fbe_status_t fbe_terminator_api_drive_error_injection_add_error(fbe_terminator_neit_error_record_t record)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    status = fbe_neit_insert_error_record(record);
    return status;
}

fbe_status_t fbe_terminator_api_drive_error_injection_init_error(fbe_terminator_neit_error_record_t *record)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    status = fbe_error_record_init(record);
    return status;
}

fbe_status_t fbe_terminator_api_drive_payload_insert_error(fbe_payload_cdb_operation_t  *payload_cdb_operation,
                                                           fbe_terminator_api_device_handle_t device_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    tdr_status_t tdr_status = TDR_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t device_ptr;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(device_handle, &device_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }

    status = fbe_terminator_sas_payload_insert_error(payload_cdb_operation, device_ptr);
    return status;
}

/*!***************************************************************************
 *          fbe_terminator_api_set_simulated_drive_debug_flags ()
 *****************************************************************************
 *
 * @brief   Set the server port of simulated drive deug flags.
 * 
 * @param   drive_select_type - Method to select drive type (all, fru_index for b/e/s)
 * @param   first_term_drive_index - FBE_TERMINATOR_DRIVE_SELECT_ALL_DRIVES -
 *              Select all drives. Otherwise select type defines this value.
 * @param   last_term_drive_index - Last index of terminator drive array to select
 * @param   backend_bus_number - Backend bus number
 * @param   encl_number - encl_number
 * @param   slot_number - slot number
 * @param   terminator_drive_debug_flags - Terminator debug flags to set
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  10/18/2011  Ron Proulx  - Created
 *
 *********************************************************************/
fbe_status_t fbe_terminator_api_set_simulated_drive_debug_flags(fbe_terminator_drive_select_type_t drive_select_type,
                                                                fbe_u32_t first_term_drive_index,
                                                                fbe_u32_t last_term_drive_index,
                                                                fbe_u32_t backend_bus_number,
                                                                fbe_u32_t encl_number,
                                                                fbe_u32_t slot_number,
                                                                fbe_terminator_drive_debug_flags_t terminator_drive_debug_flags)
{
    fbe_status_t fbe_status = FBE_STATUS_GENERIC_FAILURE;

    fbe_status = terminator_simulated_drive_set_debug_flags(drive_select_type,
                                                            first_term_drive_index,
                                                            last_term_drive_index,
                                                            backend_bus_number,
                                                            encl_number,
                                                            slot_number,
                                                            terminator_drive_debug_flags);
    if (fbe_status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, 
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: Failed with status: 0x%x\n", 
                         __FUNCTION__, fbe_status);
        return fbe_status;
    }

    return fbe_status;
}

/*!***************************************************************************
 *          fbe_terminator_api_set_log_page_31 ()
 *****************************************************************************
 *
 * @brief   Set the log page 31 data for the specified drive 
 * 
 * @param   drive_handle - pointer to the drive
 * @param   log_page_31 - pointer to the log page 31 data 
 * @param   buffer_length - size of the log_page_31 buffer
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  07/23/2015  Deanna Heng - Created
 *
 *********************************************************************/
fbe_status_t fbe_terminator_api_set_log_page_31(fbe_terminator_api_device_handle_t drive_handle, 
                                                fbe_u8_t * log_page_31,
                                                fbe_u32_t buffer_length)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    tdr_status_t tdr_status = TDR_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t device_ptr;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(drive_handle, &device_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = sas_drive_set_log_page_31((terminator_drive_t *)device_ptr, log_page_31, buffer_length);
    return status;
}

/*!***************************************************************************
 *          fbe_terminator_api_get_log_page_31 ()
 *****************************************************************************
 *
 * @brief   Get the log page 31 data for the specified drive 
 * 
 * @param   drive_handle - pointer to the drive
 * @param   log_page_31 - pointer to the log page 31 data 
 * @param   buffer_length - pointer to size of log page 31 buffer size
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  07/23/2015  Deanna Heng - Created
 *
 *********************************************************************/
fbe_status_t fbe_terminator_api_get_log_page_31(fbe_terminator_api_device_handle_t drive_handle, 
                                                fbe_u8_t * log_page_31,
                                                fbe_u32_t * buffer_length)
{
    tdr_status_t tdr_status = TDR_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t device_ptr;
    fbe_u8_t * log_page = NULL;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    if (log_page_31 == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    tdr_status = fbe_terminator_device_registry_get_device_ptr(drive_handle, &device_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    log_page = sas_drive_get_log_page_31((terminator_drive_t *)device_ptr, buffer_length);
    fbe_copy_memory(log_page_31, log_page, *buffer_length);
    return FBE_STATUS_OK;
}
