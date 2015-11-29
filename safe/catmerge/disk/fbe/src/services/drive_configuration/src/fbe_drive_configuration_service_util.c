/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_drive_configuration_service_util.c
 ***************************************************************************
 *
 * @brief
 *  This file contains utility functions for the drive_configuration
 *  service.
 *
 * @version
 *   03/27/2012:  Wayne Garrett -- created.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_transport_memory.h"
#include "fbe/fbe_service_manager_interface.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_drive_configuration_service_private.h"



/*************************
 *   FUNCTION DEFINITIONS
 *************************/

void
drive_configuration_trace(fbe_trace_level_t trace_level,
                          fbe_trace_message_id_t message_id,
                          const char * fmt, ...)
{
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t service_trace_level;

    va_list args;

    service_trace_level = default_trace_level = fbe_trace_get_default_trace_level();
    if (fbe_base_service_is_initialized(&drive_configuration_service.base_service)) {
        service_trace_level = fbe_base_service_get_trace_level(&drive_configuration_service.base_service);
        if (default_trace_level > service_trace_level) {
            service_trace_level = default_trace_level;
        }
    }
    if (trace_level > service_trace_level) {
        return;
    }

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_SERVICE,
                     FBE_SERVICE_ID_DRIVE_CONFIGURATION,
                     trace_level,
                     message_id,
                     fmt, 
                     args);
    va_end(args);
}


 /*!**************************************************************
 * fbe_drive_configuration_get_physical_drive_objects()
 ****************************************************************
 * @brief
 *  Get the total number of objects of a given class.
 *
 * @param get_total_p - Ptr to get structure.               
 *
 * @return status - The status of the operation.
 *
 * @version
 *   11/16/2011:  chenl6 - Created.
 *   03/27/2012:  Wayne Garrett - Made this a utility function.
 *
 ****************************************************************/
fbe_status_t 
fbe_drive_configuration_get_physical_drive_objects(fbe_topology_control_get_physical_drive_objects_t * get_total_p)
{
    fbe_status_t                                            status = FBE_STATUS_OK;
    fbe_packet_t *                                          packet_p = NULL;
    fbe_payload_ex_t *                                      payload_p = NULL;
    fbe_payload_control_operation_t *                       control_p = NULL;

    /* Allocate packet.
     */
    packet_p = fbe_transport_allocate_packet();
    if (packet_p == NULL) 
    {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_transport_initialize_packet(packet_p);
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_allocate_control_operation(payload_p);

    fbe_payload_control_build_operation(control_p,
                                        FBE_TOPOLOGY_CONTROL_CODE_GET_PHYSICAL_DRIVE_OBJECTS,
                                        get_total_p,
                                        sizeof(fbe_topology_control_get_physical_drive_objects_t));
    /* Set packet address.
     */
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_PHYSICAL,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID);
    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_NO_ATTRIB);
    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    fbe_service_manager_send_control_packet(packet_p);

    /* Wait for completion.
     * The packet is always completed so the status above need not be checked.
     */
    fbe_transport_wait_completion(packet_p);

    /* Save status before we release packet.
     */
    status = fbe_transport_get_status_code(packet_p);

    fbe_transport_release_packet(packet_p);

    return status;
}
/******************************************
 * end fbe_drive_configuration_get_physical_drive_objects()
 ******************************************/

 /*!**************************************************************
 * dcs_get_active_table_index()
 ****************************************************************
 * @brief
 *  Get index for the active threshold table, which is currently
 *  being used by PDO instances.
 *
 * @param none.           
 *
 * @return index for active threshold table
 *
 * @version
 *   09/24/2012  Wayne Garrett - Created.
 *
 ****************************************************************/
fbe_u32_t dcs_get_active_table_index(void)
{
    return drive_configuration_service.active_table_index;
}

 /*!**************************************************************
 * dcs_get_updating_table_index()
 ****************************************************************
 * @brief
 *  Get index for the updating threshold table, which can be
 *  updated safely before being activated.
 *
 * @param none.           
 *
 * @return index for updating threshold table
 *
 * @version
 *   09/24/2012  Wayne Garrett - Created.
 *
 ****************************************************************/
fbe_u32_t dcs_get_updating_table_index(void)
{
    return (drive_configuration_service.active_table_index==0) ? 1 : 0;
}

 /*!**************************************************************
 * dcs_activate_new_table()
 ****************************************************************
 * @brief
 *  Activates a new table for PDOs to re-register with.
 *
 * @param none.           
 *
 * @return index for updating threshold table
 *
 * @version
 *   09/24/2012  Wayne Garrett - Created.
 *
 ****************************************************************/
void dcs_activate_new_table(void)
{
    drive_configuration_service.active_table_index = dcs_get_updating_table_index();
}

 /*!**************************************************************
 * dcs_handle_to_table_index()
 ****************************************************************
 * @brief
 *  Returns the embedded table index from the handle
 *
 * @param handle - handle to threshold record.           
 *
 * @return table index
 *
 * @version
 *   09/24/2012  Wayne Garrett - Created.
 *
 ****************************************************************/
fbe_u32_t dcs_handle_to_table_index(fbe_drive_configuration_handle_t handle)
{
    return handle >> DCS_TABLE_INDEX_SHIFT;
}

 /*!**************************************************************
 * dcs_handle_to_entry_index()
 ****************************************************************
 * @brief
 *  Returns the embedded entry index from the handle
 *
 * @param handle - handle to threshold record.           
 *
 * @return entry index
 *
 * @version
 *   09/24/2012  Wayne Garrett - Created.
 *
 ****************************************************************/
fbe_u32_t dcs_handle_to_entry_index(fbe_drive_configuration_handle_t handle)
{
    return handle & ~(0xFF << DCS_TABLE_INDEX_SHIFT);
}


/*!**************************************************************
 * @fn dcs_is_all_unregistered_from_old_table
 ****************************************************************
 * @brief
 *  Check if all clients have unregistered from the old table.
 * 
 * @return fbe_bool_t - FBE_TRUE if all unregistered.
 *
 * @version
 *   10/04/2012:  Wayne Garrett - Created.
 *
 ****************************************************************/
fbe_bool_t dcs_is_all_unregistered_from_old_table(void)
{
    fbe_bool_t is_all_unregistered = FBE_TRUE;
    fbe_u32_t  old_table_idx, entry_idx;

    /* The active table has been swapped in, so the updating table is old table which clients are unregistering from */
    old_table_idx = dcs_get_updating_table_index();  
    
    for (entry_idx = 0; entry_idx < MAX_THRESHOLD_TABLE_RECORDS; entry_idx++) 
    {
        if (drive_configuration_service.threshold_record_tables[old_table_idx][entry_idx].ref_count != 0) 
        {
            drive_configuration_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                                  "DIEH - not all unregistered. handle:0x%x count:%d \n", 
                                  (old_table_idx << DCS_TABLE_INDEX_SHIFT) | entry_idx,
                                  drive_configuration_service.threshold_record_tables[old_table_idx][entry_idx].ref_count);

            is_all_unregistered = FBE_FALSE;
            break;
        }
    }

    return is_all_unregistered;
}
