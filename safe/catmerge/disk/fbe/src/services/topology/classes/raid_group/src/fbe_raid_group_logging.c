/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_raid_group_logging.c
 ***************************************************************************
 *
 * @brief
 *   This file contains the code to log event log messages for the raid group 
 *   object.
 * 
 * @ingroup raid_group_class_files
 * 
 * @version
 *   10/25/2010:  Created. Jean Montes.
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/


#include "fbe_raid_group_rebuild.h"                     //  rebuild header file
#include "fbe_raid_group_logging.h"                     //  this file's .h file  
#include "fbe/fbe_event_log_api.h"                      //  for fbe_event_log_write
#include "fbe/fbe_event_log_utils.h"                    //  for message codes
#include "fbe/fbe_enclosure.h"                          //  for fbe_base_enclosure_mgmt_get_slot_number_t
#include "fbe_notification.h"


/*****************************************
 * LOCAL FUNCTION PROTOTYPES
 *****************************************/


//  Log a message to the event log that all rebuilds for a RG have completed 
static fbe_status_t fbe_raid_group_logging_log_all_rebuilds_start_or_complete(
                                    fbe_raid_group_t*                       in_raid_group_p,
                                    fbe_packet_t*                           in_packet_p, 
                                    fbe_u32_t                               in_position,
                                    fbe_u32_t                               in_bus,
                                    fbe_u32_t                               in_enclosure,
                                    fbe_u32_t                               in_slot,
                                    fbe_bool_t                              is_start);


//  Check if a rebuild/copy was for the start or end of a LUN  
static fbe_status_t fbe_raid_group_logging_get_lun_obj_id(
                                    fbe_raid_group_t*                       in_raid_group_p,
                                    fbe_lba_t                               in_start_lba,
                                    fbe_object_id_t*                        out_lun_object_id_p);


//  Get the raid group object id of the raid group object, adjusted for RAID-10 if needed
static fbe_status_t fbe_raid_group_logging_get_adjusted_raid_group_object_id(
                                    fbe_raid_group_t*                       in_raid_group_p,
                                    fbe_object_id_t*                        out_raid_group_object_id_p);


//  Get the disk position in the RAID group, adjusted for RAID-10 if needed 
static fbe_status_t fbe_raid_group_logging_get_adjusted_disk_position(
                                    fbe_raid_group_t*                       in_raid_group_p,
                                    fbe_u32_t                               in_position,
                                    fbe_u32_t*                              out_adjusted_position_p);

//  Send a control request to another component and get the response 
static fbe_status_t fbe_raid_group_logging_send_control_packet_and_get_response( 
                                    fbe_packet_t*                           in_packet_p, 
                                    fbe_service_id_t                        in_destination_service,
                                    fbe_object_id_t                         in_destination_object_id,
                                    fbe_payload_control_operation_opcode_t  in_control_code,
                                    void*                                   io_buffer_p,
                                    fbe_u32_t                               in_buffer_size);

static fbe_status_t fbe_raid_group_logging_send_control_packet(
                                        fbe_packet_t*                           in_packet_p, 
                                        fbe_service_id_t                        in_destination_service,
                                        fbe_object_id_t                         in_destination_object_id,
                                        fbe_payload_control_operation_opcode_t  in_control_code,
                                        void*                                   io_buffer_p,
                                        fbe_u32_t                               in_buffer_size);
static fbe_status_t fbe_raid_group_logging_send_control_packet_completion(fbe_packet_t * packet_p,
                                                                           fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_logging_send_event_log_event_to_lun(
                                                fbe_raid_group_t*               in_raid_group_p,
                                                fbe_packet_t*                   in_packet_p,
                                                fbe_event_event_log_request_t*  in_event_log_event_data_p,
                                                fbe_lba_t                       in_actual_lba);
static fbe_status_t fbe_raid_group_logging_send_event_log_event_to_lun_completion(fbe_event_t * in_event_p,
                                                                         fbe_event_completion_context_t in_context);


/***************
 * FUNCTIONS
 ***************/
 

/*!****************************************************************************
 * fbe_raid_group_logging_log_complete 
 ******************************************************************************
 * @brief
 *   This function will log a "rebuild complete" or "copy complete" message 
 *   when the rebuild/copy has finished. 
 *
 * @param in_raid_group_p           - pointer to the raid group
 * @param in_packet_p               - pointer to a packet 
 * @param in_position               - disk position in the RG 
 * @param in_bus                    - disk physical location, bus
 * @param in_encl                   - disk physical location, enclosure
 * @param in_slot                   - disk physical location, slot
 *
 * @return fbe_status_t           
 *
 * @author
 *   09/20/2010 - Created. Jean Montes. 
 *   12/14/2012 - changed parameter list,  Naizhong Chiu
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_logging_log_all_complete(
                                    fbe_raid_group_t*           in_raid_group_p,
                                    fbe_packet_t*               in_packet_p, 
                                    fbe_u32_t                   in_position,
                                    fbe_u32_t                   in_bus,
                                    fbe_u32_t                   in_encl,
                                    fbe_u32_t                   in_slot)
{

    //  Log the appropriate message for a rebuild or copy based on whether the raid group is a virtual drive 
    //  or not.  If it is, then this is a copy.  Otherwise, it is a rebuild. 
    if (in_raid_group_p->base_config.base_object.class_id == FBE_CLASS_ID_VIRTUAL_DRIVE)
    {
        fbe_base_object_trace((fbe_base_object_t *)in_raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Attempted to clear rebuild logging for a virtual drive. \n",
                              __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        fbe_raid_group_logging_log_all_rebuilds_start_or_complete(in_raid_group_p, in_packet_p, in_position, in_bus, in_encl, in_slot, FBE_FALSE);
    }

    //  Return success  
    return FBE_STATUS_OK;

} // End fbe_raid_group_logging_log_rebuild_complete()



/*!****************************************************************************
 * fbe_raid_group_logging_log_all_rebuilds_start_or_complete 
 ******************************************************************************
 * @brief
 *   This function will log a "rebuild start" or "rebuild complete" message. 
 *   It log a message "rebuild started" when drive has started rebuilding. 
 *   It log a message "rebuild complete" when all LUNs in the RAID group
 *   have finished rebuilding.  The message will include the RAID group number 
 *   and the RAID group object id. 
 *
 * @param in_raid_group_p           - pointer to the raid group
 * @param in_packet_p               - pointer to a packet 
 * @param in_position               - disk position in the RG 
 * @param in_bus                    - disk physical location, bus 
 * @param in_encl                   - disk physical location, enclosure 
 * @param in_slot                   - disk physical location, slot 
 * @param is_start                  - indicate which event message needs to log,
 *                                    Rebuild started or Completed in RG.
 *
 *
 * @return fbe_status_t           
 *
 * @author
 *   09/20/2010 - Created. Jean Montes. 
 *   08/13/2012 - Modified. Amit Dhaduk
 *   12/14/2012 - changed parameter list.  Naizhong Chiu
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_logging_log_all_rebuilds_start_or_complete(
                                    fbe_raid_group_t*               in_raid_group_p,
                                    fbe_packet_t*                   in_packet_p, 
                                    fbe_u32_t                       in_position,
                                    fbe_u32_t                       in_bus,
                                    fbe_u32_t                       in_enclosure,
                                    fbe_u32_t                       in_slot,
                                    fbe_bool_t                      is_start)
{
    fbe_status_t                    status;
    fbe_object_id_t                 raid_group_object_id;           // customer-visible raid group's object id 
    fbe_raid_group_number_t         raid_group_number;              // customer-visible raid group number
    fbe_u32_t                       adjusted_position;              // disk position adjusted for RAID-10 if needed

    //  Get the raid group number and the raid group object ID, adjusted for RAID-10 if needed
    fbe_raid_group_logging_get_raid_group_number_and_object_id(in_raid_group_p,&raid_group_number,
        &raid_group_object_id); 

    //  Get the disk position adjusted for RAID-10 if needed.  (This will only be displayed in the event log and 
    //  is not used anywhere else.)
    status = fbe_raid_group_logging_get_adjusted_disk_position(in_raid_group_p, in_position, &adjusted_position); 

    //  Log the event log message 
    if ((raid_group_number != FBE_RAID_GROUP_INVALID) &&
        (status == FBE_STATUS_OK)                        )
    {
        if(is_start)
        {
            fbe_event_log_write(SEP_INFO_RAID_OBJECT_RAID_GROUP_REBUILD_STARTED, NULL, 0, 
                                "%d 0x%x %d %d %d %d", raid_group_number, raid_group_object_id, adjusted_position, 
                                in_bus, 
                                in_enclosure, 
                                in_slot);
        }
        else
        {           
            fbe_event_log_write(SEP_INFO_RAID_OBJECT_RAID_GROUP_REBUILD_COMPLETED, NULL, 0, 
                                "%d 0x%x %d %d %d %d", raid_group_number, raid_group_object_id, adjusted_position, 
                                in_bus, 
                                in_enclosure, 
                                in_slot);
        }
    }

    //  Return success  
    return FBE_STATUS_OK;

} // End fbe_raid_group_logging_log_all_rebuilds_start_or_complete()


/*!****************************************************************************
 * fbe_raid_group_logging_log_lun_and_update_checkpoint_for_rebuild_op() 
 ******************************************************************************
 * @brief
 *   This function will log a message that a rebuild or copy of a LUN has 
 *   started or completed when needed.  It is specfic to rebuild operations 
 *   (rebuild and copy) as opposed to verify.
 * 
 * @param in_raid_group_p           - pointer to the raid group
 * @param in_packet_p               - pointer to a packet 
 *
 * @return  fbe_status_t           
 *
 * @author
 *   09/20/2010 - Created. Jean Montes. 
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_logging_log_lun_and_update_checkpoint_for_rebuild_op(fbe_packet_t *packet_p, 
                                                                                        fbe_packet_completion_context_t context)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_raid_group_t                   *raid_group_p;
    fbe_raid_group_rebuild_context_t   *rebuild_context_p;          // pointer to the rebuild context
    fbe_raid_position_bitmask_t         position_bitmask = 0;
    fbe_u32_t                           width;
    fbe_raid_position_t                 position;
    fbe_block_edge_t*                   downstream_edge_p;      // pointer to edge being rebuilt
    fbe_object_id_t                     downstream_object_id;   // object id of VD/PVD being rebuilt
    fbe_base_config_physical_drive_location_t  *location = NULL;

    /* Cast the context to a pointer to a raid group object.
     */ 
    raid_group_p = (fbe_raid_group_t *)context;

    //  Get a pointer to the rebuild context from the packet
    fbe_raid_group_rebuild_get_rebuild_context(raid_group_p, packet_p, &rebuild_context_p);
    fbe_raid_group_get_width(raid_group_p, &width);

    /* log an event at Raid Group level if Rebuild or Copy started */
    if(rebuild_context_p->start_lba == 0)
    {
        /* we've had issued the request to aquire the slot info */
        if (rebuild_context_p->event_log_position != FBE_RAID_POSITION_INVALID)
        {
            status = fbe_transport_get_status_code(packet_p);
            if (status == FBE_STATUS_OK)
            {
                //  Log the appropriate message(s) for a rebuild or copy based on whether the "raid group" is a virtual
                //  drive.  If it is, then this is a copy.  Otherwise, it is a rebuild. 
                if (raid_group_p->base_config.base_object.class_id == FBE_CLASS_ID_VIRTUAL_DRIVE)
                {
                    if (rebuild_context_p->event_log_source_position != FBE_RAID_POSITION_INVALID)
                    {
                        //  Log the message to the event log 
                        fbe_event_log_write(SEP_INFO_RAID_OBJECT_RAID_GROUP_COPY_STARTED, NULL, 0, 
                            "%d %d %d %d %d %d", 
                            rebuild_context_p->event_log_source_location.port_number,
                            rebuild_context_p->event_log_source_location.enclosure_number,
                            rebuild_context_p->event_log_source_location.slot_number,
                            rebuild_context_p->event_log_location.port_number,
                            rebuild_context_p->event_log_location.enclosure_number,
                            rebuild_context_p->event_log_location.slot_number);
                        position = rebuild_context_p->event_log_position + 1;
                    }
                    else
                    {
                        // come back to the same position for the source location info
                        position = rebuild_context_p->event_log_position;
                    }
                }
                else
                {
                    fbe_raid_group_logging_log_all_rebuilds_start_or_complete(raid_group_p, packet_p, rebuild_context_p->event_log_position, 
                                rebuild_context_p->event_log_location.port_number, 
                                rebuild_context_p->event_log_location.enclosure_number, 
                                rebuild_context_p->event_log_location.slot_number,
                                FBE_TRUE);
                    position = rebuild_context_p->event_log_position + 1;
                }
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_WARNING, 
                    FBE_TRACE_MESSAGE_ID_INFO, "%s: error %d getting location for index %d.\n", __FUNCTION__, status, rebuild_context_p->event_log_position);
                // move on to the next position
                position = rebuild_context_p->event_log_position + 1;
                fbe_raid_group_rebuild_context_set_event_log_position(rebuild_context_p, FBE_RAID_POSITION_INVALID);
            }
        }
        else
        {
            position = 0;
        }
        /* For all positions that require to log an event
         */
        for (; position < width; position++)
        {
            /* If this position needs to be updated, log a rebuild event.
             */
            position_bitmask = (1 << position);
            if (position_bitmask & rebuild_context_p->update_checkpoint_bitmask)
            {
                /* new position to log */
                if (rebuild_context_p->event_log_position != position)
                {
                    fbe_raid_group_rebuild_context_set_event_log_position(rebuild_context_p, position);
                    /* clear the source for now */
                    fbe_raid_group_rebuild_context_set_event_log_source_position(rebuild_context_p, FBE_RAID_POSITION_INVALID);
                    /* we need to go and get the slot info for this position */
                    //  Get the object id of the downstream object for this position.  If this is a rebuild, the downstream 
                    //  object will be the VD.  If this is a copy, the downstream object is the PVD. 
                    fbe_base_config_get_block_edge((fbe_base_config_t*) raid_group_p, &downstream_edge_p, position);
                    fbe_block_transport_get_server_id(downstream_edge_p, &downstream_object_id);
                    location = &rebuild_context_p->event_log_location;
                }
                else
                {
                    /* we may need to take care of the source location for virtual_drive */
                    if ((raid_group_p->base_config.base_object.class_id == FBE_CLASS_ID_VIRTUAL_DRIVE) &&
                        (rebuild_context_p->event_log_source_position == FBE_RAID_POSITION_INVALID))
                    {
                        //  The position that is passed in is for the destination/proactive spare drive.  Determine the position 
                        //  of the source/proactive candidate drive.  Since it is a mirror, one must be position 0 and the other
                        //  position 1. 
                        if (rebuild_context_p->event_log_position == 0)
                        {
                            fbe_raid_group_rebuild_context_set_event_log_source_position(rebuild_context_p, 1);
                        }
                        else
                        {
                            fbe_raid_group_rebuild_context_set_event_log_source_position(rebuild_context_p, 0);
                        }
                        /* we need to go and get the slot info for this position */
                        //  Get the object id of the downstream object for this position.  If this is a rebuild, the downstream 
                        //  object will be the VD.  If this is a copy, the downstream object is the PVD. 
                        fbe_base_config_get_block_edge((fbe_base_config_t*) raid_group_p, &downstream_edge_p, rebuild_context_p->event_log_source_position);
                        fbe_block_transport_get_server_id(downstream_edge_p, &downstream_object_id);
                        location = &rebuild_context_p->event_log_source_location;
                    }
                    else
                    {
                        /* nothing to do, move to the next */
                        continue;  
                    }
                }

                /* set the completion function for the signature init condition. */
                fbe_transport_set_completion_function(packet_p, fbe_raid_group_logging_log_lun_and_update_checkpoint_for_rebuild_op, context);

                //  Get the bus information for the drive.  We just send this to the downstream object and it will forward it 
                //  all the way down to the physical object which contains thie info. 
                status = fbe_raid_group_logging_send_control_packet(packet_p, FBE_SERVICE_ID_TOPOLOGY, 
                                    downstream_object_id, FBE_PROVISION_DRIVE_CONTROL_CODE_GET_DRIVE_LOCATION, 
                                    location, sizeof(fbe_base_config_physical_drive_location_t));

                if (status == FBE_STATUS_OK)
                {
                    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
                }
            }

        } /* end for all positions */

    }


    /* Now update the rebuild checkpoint(s)
     */
    status = fbe_raid_group_incr_rebuild_checkpoint(raid_group_p, packet_p, 
                                                    rebuild_context_p->update_checkpoint_lba,
                                                    rebuild_context_p->update_checkpoint_blocks,
                                                    rebuild_context_p->update_checkpoint_bitmask);

    /* Always return more processing.
     */
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

} // End fbe_raid_group_logging_log_lun_and_update_checkpoint_for_rebuild_op()



/*!****************************************************************************
 * fbe_raid_group_logging_get_lun_obj_id
 ******************************************************************************
 * @brief
 *   Note: This function is deprecated and should no longer be used.  The 
 *   LUN start/end information needs to be found by sending a permit request
 *   event to the LUN.  JMM 11/18/10.
 *
 *   This function will get the LUN object ID based on the given LBA and RAID 
 *   group.
 *
 * @param in_raid_group_p           - pointer to the raid group
 * @param in_start_lba              - starting LBA of the I/O. The LBA is relative 
 *                                    to the RAID extent on a single disk. 
 * @param in_block_count            - number of blocks 
 * @param out_lun_object_id_p       - LUN object id of the LUN which contains the LBA
 *                                    if it is the start or end of the LUN; or 
 *                                    FBE_OBJECT_ID_INVALID if no LUN is starting/finishing
 * 
 * @return fbe_status_t           
 *
 * @author
 *   09/20/2010 - Created. Jean Montes. 
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_logging_get_lun_obj_id(
                                        fbe_raid_group_t*           in_raid_group_p,
                                        fbe_lba_t                   in_start_lba,
                                        fbe_object_id_t*            out_lun_object_id_p)
{

    fbe_block_transport_server_t*       block_transport_server_p;   // pointer to the raid object's block transport
                                                                    // server 
    fbe_block_edge_t*                   block_edge_p;               // pointer to an edge 
    fbe_raid_geometry_t*                raid_geometry_p;            // pointer to raid geometry info 
    fbe_u16_t                           data_disks;                 // number of data disks in the rg
    fbe_lba_t                           logical_start_lba;          // LUN-based start LBA 


    //  Initialize the output parameters
    *out_lun_object_id_p    = FBE_OBJECT_ID_INVALID; 

    //  Convert the start LBA, which is disk-based, to a LUN-based LBA 
    raid_geometry_p     = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    logical_start_lba   = in_start_lba * data_disks; 

    //  Get the pointer to the RAID block transport server
    block_transport_server_p = &in_raid_group_p->base_config.block_transport_server;

    fbe_block_transport_find_edge_and_obj_id_by_lba(block_transport_server_p, logical_start_lba, out_lun_object_id_p, &block_edge_p);

    return FBE_STATUS_OK;

} // End fbe_raid_group_logging_get_lun_obj_id()


/*!****************************************************************************
 *  fbe_raid_group_logging_get_disk_info_for_position  
 ******************************************************************************
 *
 * @brief
 *   This function is used to get the port, enclosure, and slot information 
 *   for a specific drive position of the RAID group.
 *
 * @param in_raid_group_p       - pointer to the raid group
 * @param in_packet_p           - pointer to a packet 
 * @param in_position           - disk position in the RG 
 * @param out_bus_num_p         - pointer to data that gets populated with the bus 
 *                                number
 * @param out_encl_num_p        - pointer to data that gets populated with the 
 *                                enclosure number
 * @param out_slot_num_p        - pointer to data that gets populated with the slot 
 *                                number
 * 
 * @return  fbe_status_t 
 *
 * @author
 *   10/30/2010 - Created. Jean Montes. 
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_logging_get_disk_info_for_position(
                                                    fbe_raid_group_t*       in_raid_group_p,
                                                    fbe_packet_t*           in_packet_p, 
                                                    fbe_u32_t               in_position, 
                                                    fbe_u32_t*              out_bus_num_p,
                                                    fbe_u32_t*              out_encl_num_p,
                                                    fbe_u32_t*              out_slot_num_p)

{
    fbe_block_edge_t*                                   downstream_edge_p;      // pointer to edge being rebuilt
    fbe_object_id_t                                     downstream_object_id;   // object id of VD/PVD being rebuilt
    fbe_provision_drive_get_physical_drive_location_t   drive_location;         // drive location (bus, encl, slot)
    fbe_status_t                                        status;                 // fbe status 

    //  Initialize the output parameters 
    *out_bus_num_p      = FBE_PORT_NUMBER_INVALID;
    *out_encl_num_p     = FBE_ENCLOSURE_NUMBER_INVALID;
    *out_slot_num_p     = FBE_SLOT_NUMBER_INVALID;

    //  Get the object id of the downstream object for this position.  If this is a rebuild, the downstream 
    //  object will be the VD.  If this is a copy, the downstream object is the PVD. 
    fbe_base_config_get_block_edge((fbe_base_config_t*) in_raid_group_p, &downstream_edge_p, in_position);
    fbe_block_transport_get_server_id(downstream_edge_p, &downstream_object_id);

    //  Get the bus information for the drive.  We just send this to the downstream object and it will forward it 
    //  all the way down to the physical object which contains thie info. 
    status = fbe_raid_group_logging_send_control_packet_and_get_response(in_packet_p, FBE_SERVICE_ID_TOPOLOGY, 
        downstream_object_id, FBE_PROVISION_DRIVE_CONTROL_CODE_GET_DRIVE_LOCATION, &drive_location, sizeof(drive_location));

    //  If there was an error, then the data is not valid - just return here 
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    //  Set the bus, enclosure and slot in the output parameter
    *out_bus_num_p = drive_location.port_number;
    *out_encl_num_p = drive_location.enclosure_number;
    *out_slot_num_p = drive_location.slot_number;

    //  Return success 
    return FBE_STATUS_OK;

} //  End fbe_raid_group_logging_get_disk_info_for_position()


/*!****************************************************************************
 *  fbe_raid_group_logging_get_raid_group_number_and_object_id  
 ******************************************************************************
 *
 * @brief
 *   This function is used to get the raid group number and object ID for a 
 *   raid group.  The raid group number is the customer-visible RAID group
 *   identifier.  If this is a RAID-10 raid group, the RG number and object id 
 *   of the striper RG will be returned.  
 *
 * @param in_raid_group_p               - pointer to the raid group
 * @param out_raid_group_num_p          - pointer to data that gets populated with the  
 *                                        raid group number (customer-visible RG id)
 * @param out_raid_group_object_id_p    - pointer to data that gets populated with the  
 *                                        object id of the above raid group 
 * 
 * @return fbe_status_t 
 *
 * @author
 *   11/01/2010 - Created. Jean Montes. 
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_logging_get_raid_group_number_and_object_id(
                                                    fbe_raid_group_t *in_raid_group_p,
                                                    fbe_u32_t        *out_raid_group_num_p,
                                                    fbe_object_id_t  *out_raid_group_object_id_p)

{
    fbe_status_t    status = FBE_STATUS_OK;
    
    /* Determine the raid group object id of the "user visible" RAID group and
     * set it in the data used to request the RG number.
     */
    fbe_raid_group_logging_get_adjusted_raid_group_object_id(in_raid_group_p, out_raid_group_object_id_p);

    /* If the `user visable' raid group is valid (there are cases for RAID-10
     * where the parent (i.e. `user visable') raid group may have already
     * been destroyed).
     */
    if (*out_raid_group_object_id_p == FBE_OBJECT_ID_INVALID)
    {
        /* If the object id was invalid set the raid group to invalid.
         */
        *out_raid_group_num_p = FBE_RAID_GROUP_INVALID;
    }
    else
    {
        /* Else get rg number from database service.
         */
        status = fbe_database_get_rg_number(*out_raid_group_object_id_p,out_raid_group_num_p);
    }
     
    return status;

} //  End fbe_raid_group_logging_get_raid_group_number_and_object_id()


/*!****************************************************************************
 *  fbe_raid_group_logging_get_adjusted_raid_group_object_id  
 ******************************************************************************
 *
 * @brief
 *   This function is used to get the raid group object id of the "user visible"
 *   RAID group.  If the RAID group is a RAID-10 and this is the mirror under
 *   the striper, it will get the object id of the striper raid group.  In all
 *   other cases it simply gets the object id of the input raid group. 
 *
 * @param in_raid_group_p               - pointer to the raid group
 * @param out_raid_group_object_id_p    - pointer to data that gets populated with   
 *                                        the raid group object ID of the user visible
 *                                        raid group 
 * 
 * @return fbe_status_t 
 *
 * @author
 *   11/18/2010 - Created. Jean Montes. 
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_logging_get_adjusted_raid_group_object_id(
                                        fbe_raid_group_t*           in_raid_group_p,
                                        fbe_object_id_t*            out_raid_group_object_id_p)

{
    fbe_raid_geometry_t*                raid_geometry_p;            // pointer to raid geometry info 
    fbe_block_transport_server_t*       block_transport_server_p;   // pointer to the raid object's block transport
                                                                    // server 
    fbe_edge_index_t                    striper_client_index;       // index of this mirror within the striper 
   

    //  Initialize the output parameter 
    *out_raid_group_object_id_p = FBE_OBJECT_ID_INVALID;

    //  Get the object id of the raid group and set it in the output parameter 
    fbe_base_object_get_object_id((fbe_base_object_t*) in_raid_group_p, out_raid_group_object_id_p);

    //  Check if this is a mirror under a RAID-10 striper.  If not, we don't need to do anything more - return here.
    raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    if (fbe_raid_geometry_is_mirror_under_striper(raid_geometry_p) == FBE_FALSE)
    {
        return FBE_STATUS_OK;
    }

    // We have a RAID-10 and so need to get the object ID of the striper object above the mirror  

    //  Get the pointer to the RAID block transport server
    block_transport_server_p = &in_raid_group_p->base_config.block_transport_server;

    fbe_block_transport_find_first_upstream_edge_index_and_obj_id(block_transport_server_p, 
                                                                  &striper_client_index, 
                                                                  out_raid_group_object_id_p);
    //  Return success 
    return FBE_STATUS_OK;

} //  End fbe_raid_group_logging_get_adjusted_raid_group_object_id()

/*!****************************************************************************
 *  fbe_raid_group_logging_get_adjusted_disk_position 
 ******************************************************************************
 *
 * @brief
 *   This function will get the "adjusted disk position" corresponding to the 
 *   given disk position.  If the RAID group is a RAID-10 and this is the mirror 
 *   under the striper, it will calculate the disk position relative to the entire
 *   RAID-10 raid group.  In all other cases, it simply returns the disk position 
 *   passed in. 
 *
 * @param in_raid_group_p               - pointer to the raid group
 * @param in_position                   - disk position in the given RG
 * @param out_adjusted_position_p       - pointer to data that gets populated with   
 *                                        the adjusted disk position
 * 
 * @return fbe_status_t 
 *
 * @author
 *   11/18/2010 - Created. Jean Montes. 
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_logging_get_adjusted_disk_position(
                                        fbe_raid_group_t*           in_raid_group_p,
                                        fbe_u32_t                   in_position,
                                        fbe_u32_t*                  out_adjusted_position_p)

{
    fbe_raid_geometry_t*                raid_geometry_p;            // pointer to raid geometry info 
    fbe_block_transport_server_t*       block_transport_server_p;   // pointer to the raid object's block transport
                                                                    // server 
    fbe_edge_index_t                    striper_client_index;       // index of this mirror within the striper 
    fbe_object_id_t                     object_id;

    //  Initialize the output parameter to the position passed in 
    *out_adjusted_position_p = in_position;

    //  Check if this is a mirror under a RAID-10 striper.  If not, we don't need to do anything more - return here.
    raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    if (fbe_raid_geometry_is_mirror_under_striper(raid_geometry_p) == FBE_FALSE)
    {
        return FBE_STATUS_OK;
    }

    //  We have a RAID-10 and so need to get the position of this disk in the overall RAID-10, not just in the
    //  mirror

    //  Get the pointer to the RAID block transport server
    block_transport_server_p = &in_raid_group_p->base_config.block_transport_server;

    fbe_block_transport_find_first_upstream_edge_index_and_obj_id(block_transport_server_p, 
                                                                  &striper_client_index, 
                                                                  &object_id);

    // If the upstream raid groups is gone return error
    if (object_id == FBE_OBJECT_ID_INVALID)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //  The position in the overall RAID-10 raid group is the striper's client index times 2 (since each mirror has two 
    //  drives) plus the input position (0 or 1 - which indicates its position in the mirror)
    *out_adjusted_position_p = (striper_client_index * 2) + in_position;

    //  Return success 
    return FBE_STATUS_OK;

} //  End fbe_raid_group_logging_get_adjusted_disk_position()


/*!****************************************************************************
 *  fbe_raid_group_logging_send_control_packet_and_get_response   
 ******************************************************************************
 *
 * @brief
 *   This function will send a control operation to the given destination.  The
 *   caller will pass in the control operation type and a buffer for it that 
 *   has been pre-loaded with any input information.  The function will then 
 *   send the request and wait for the response to it, which gets set in the 
 *   buffer. 
 *
 *   Note, this function only sends packets to the SEP package. 
 * 
 * @param in_packet_p               - pointer to a packet 
 * @param in_destination_service    - service to send the request to 
 * @param in_destination_object_id  - object to send the request to 
 * @param in_control_code           - control code to send
 * @param io_buffer_p               - pointer to buffer with input/output data
 * @param in_buffer_size            - size of buffer
 * 
 * @return  fbe_status_t 
 *
 * @author
 *   09/20/2010 - Created. Jean Montes. 
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_logging_send_control_packet_and_get_response(
                                        fbe_packet_t*                           in_packet_p, 
                                        fbe_service_id_t                        in_destination_service,
                                        fbe_object_id_t                         in_destination_object_id,
                                        fbe_payload_control_operation_opcode_t  in_control_code,
                                        void*                                   io_buffer_p,
                                        fbe_u32_t                               in_buffer_size)
{
    fbe_payload_ex_t*                  sep_payload_p;
    fbe_payload_control_operation_t*    control_op_p;
    fbe_status_t                        packet_status;
    fbe_payload_control_status_t        control_status;


    //  Build a control operation with the control code and buffer pointer passed in 
    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);
    control_op_p  = fbe_payload_ex_allocate_control_operation(sep_payload_p);
    fbe_payload_control_build_operation(control_op_p, in_control_code, io_buffer_p, in_buffer_size);
        
    //  Set the address to send the packet to 
    fbe_transport_set_address(in_packet_p, FBE_PACKAGE_ID_SEP_0, in_destination_service, FBE_CLASS_ID_INVALID, 
            in_destination_object_id);

    //  Set the packet flags.  The traverse flag will cause the destination object to forward the packet if it
    //  does not handle it itself (in the cases where it is going to an object).
    fbe_transport_set_packet_attr(in_packet_p, FBE_PACKET_FLAG_TRAVERSE);
    fbe_transport_set_sync_completion_type(in_packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    //  Send the packet and wait for its completion 
    fbe_service_manager_send_control_packet(in_packet_p);
    fbe_transport_wait_completion(in_packet_p);

    //  Get the control operation status and packet status 
    fbe_payload_control_get_status(control_op_p, &control_status);
    packet_status = fbe_transport_get_status_code(in_packet_p);

    //  Release the control operation 
    fbe_payload_ex_release_control_operation(sep_payload_p, control_op_p);

    //  If there was a control error, return a generic failure status
    if (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //  Return the packet status
    return packet_status;

} //  End fbe_raid_group_logging_send_control_packet_and_get_response()


/*!****************************************************************************
 *  fbe_raid_group_logging_send_control_packet   
 ******************************************************************************
 *
 * @brief
 *   This function will send a control operation to the given destination.  The
 *   caller will pass in the control operation type and a buffer for it that 
 *   has been pre-loaded with any input information.  The function will then 
 *   send the request, which gets set in the 
 *   buffer. 
 *
 *   Note, this function only sends packets to the SEP package. 
 * 
 * @param in_packet_p               - pointer to a packet 
 * @param in_destination_service    - service to send the request to 
 * @param in_destination_object_id  - object to send the request to 
 * @param in_control_code           - control code to send
 * @param io_buffer_p               - pointer to buffer with input/output data
 * @param in_buffer_size            - size of buffer
 * 
 * @return  fbe_status_t 
 *
 * @author
 *   12/14/2012 NChiu, created from the sync function. 
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_logging_send_control_packet(
                                        fbe_packet_t*                           in_packet_p, 
                                        fbe_service_id_t                        in_destination_service,
                                        fbe_object_id_t                         in_destination_object_id,
                                        fbe_payload_control_operation_opcode_t  in_control_code,
                                        void*                                   io_buffer_p,
                                        fbe_u32_t                               in_buffer_size)
{
    fbe_payload_ex_t*                  sep_payload_p;
    fbe_payload_control_operation_t*    control_op_p;
    fbe_status_t                        packet_status;


    //  Build a control operation with the control code and buffer pointer passed in 
    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);
    control_op_p  = fbe_payload_ex_allocate_control_operation(sep_payload_p);
    fbe_payload_control_build_operation(control_op_p, in_control_code, io_buffer_p, in_buffer_size);
        
    //  Set the address to send the packet to 
    fbe_transport_set_address(in_packet_p, FBE_PACKAGE_ID_SEP_0, in_destination_service, FBE_CLASS_ID_INVALID, 
            in_destination_object_id);

    //  Set the packet flags.  The traverse flag will cause the destination object to forward the packet if it
    //  does not handle it itself (in the cases where it is going to an object).
    fbe_transport_set_packet_attr(in_packet_p, FBE_PACKET_FLAG_TRAVERSE);
    /* set the completion function for the signature init condition. */
    fbe_transport_set_completion_function(in_packet_p, fbe_raid_group_logging_send_control_packet_completion, io_buffer_p);

    //  Send the packet and wait for its completion 
    packet_status = fbe_service_manager_send_control_packet(in_packet_p);

    return packet_status;
}

static fbe_status_t fbe_raid_group_logging_send_control_packet_completion(fbe_packet_t * packet_p,
                                                                           fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t*                  sep_payload_p;
    fbe_payload_control_operation_t*    control_op_p;
    fbe_status_t                        packet_status;
    fbe_payload_control_status_t        control_status;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_op_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    //  Get the control operation status and packet status 
    fbe_payload_control_get_status(control_op_p, &control_status);
    packet_status = fbe_transport_get_status_code(packet_p);

    //  Release the control operation 
    fbe_payload_ex_release_control_operation(sep_payload_p, control_op_p);

    //  If there was a control error, return a generic failure status
    if (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
    }

    //  Return the packet status
    return FBE_STATUS_OK;

} //  End fbe_raid_group_logging_send_control_packet_and_get_response()


/*!****************************************************************************
 * fbe_raid_group_logging_log_lun_bv_start_or_complete 
 ******************************************************************************
 * @brief
 * This function sends request to LUN for Background Verify started or
 * completed if it is.
 * 
 * @param in_raid_group_p           - pointer to the raid group
 * @param in_packet_p               - pointer to a packet 
 * @param in_start_lba              - start lba for the verify block IO
 * 
 * @return  fbe_status_t           
 *
 * @author
 *   10/01/2012 - Created. Amit Dhaduk
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_logging_log_lun_bv_start_or_complete_if_needed(
                                    fbe_raid_group_t*           in_raid_group_p,
                                    fbe_packet_t*               in_packet_p, 
                                    fbe_lba_t                   in_start_lba)
{

    fbe_event_event_log_request_t   event_log_event_data = {0};
    fbe_raid_verify_tracking_t*     verify_ts_p;                // pointer to verify tracking data


    //  Get verify tracking data
    verify_ts_p = fbe_raid_get_verify_ts_ptr(in_raid_group_p);
    event_log_event_data.verify_flags = verify_ts_p->verify_flags;

    // check if BV started. This flag set by LUN during verify permit request
    if (verify_ts_p->is_lun_start_b == FBE_TRUE)
    {
        //  Send a event request to the lun that that Background Verify started
        event_log_event_data.event_log_type = FBE_EVENT_LOG_EVENT_TYPE_LUN_BV_STARTED;

        fbe_raid_group_logging_send_event_log_event_to_lun(in_raid_group_p,in_packet_p,
                                                           &event_log_event_data,
                                                           in_start_lba);
    }

    // check if BV completed. This flag set by LUN during verify permit request
    if (verify_ts_p->is_lun_end_b == FBE_TRUE)
    {
        //  Send a event request to the lun that that Background Verify completed    
        event_log_event_data.event_log_type = FBE_EVENT_LOG_EVENT_TYPE_LUN_BV_COMPLETED;

        fbe_raid_group_logging_send_event_log_event_to_lun(in_raid_group_p,in_packet_p,
                                                           &event_log_event_data,
                                                           in_start_lba);
    }
    
    //  Return success  
    return FBE_STATUS_OK;

} // End fbe_raid_group_logging_log_lun_bv_start_or_complete()







/*!****************************************************************************
 * fbe_raid_group_logging_log_rl_started
 ******************************************************************************
 * @brief
 *   This function will log a rebuild logging message when the disk position
 *   in the given RG is marked for rebuild logging.
 *
 * @param in_raid_group_p           - pointer to the raid group
 * @param in_packet_p               - pointer to a packet 
 * @param in_position               - disk position in the RG 
 *
 * @return fbe_status_t           
 *
 * @author
 *   11/2010 - Created. Susan Rundbaken (rundbs). 
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_logging_log_rl_started(
                                    fbe_raid_group_t*           in_raid_group_p,
                                    fbe_packet_t*               in_packet_p, 
                                    fbe_u32_t                   in_position)
{
    fbe_class_id_t                  class_id;                       // object class ID
    fbe_object_id_t                 raid_group_object_id;           // customer-visible raid group's object id 
    fbe_raid_group_number_t         raid_group_number;              // customer-visible raid group number
    fbe_u32_t                       adjusted_position;              // adjusted position rb logging disk


    //  Get the class ID of the incoming object
    class_id = fbe_raid_group_get_class_id(in_raid_group_p);

    //  If the object is a VD object, return immediately; we do not log RL messages for the VD
    if (FBE_CLASS_ID_VIRTUAL_DRIVE == class_id)
    {
        return FBE_STATUS_OK;
    }

    //  Get the raid group number and the raid group object ID, adjusted for RAID-10 if needed
    fbe_raid_group_logging_get_raid_group_number_and_object_id(in_raid_group_p,&raid_group_number,
                                                               &raid_group_object_id); 

    //  Get the disk position adjusted for RAID-10 if needed.  (This will only be displayed in the event log and 
    //  is not used anywhere else.)
    fbe_raid_group_logging_get_adjusted_disk_position(in_raid_group_p, in_position, &adjusted_position); 

    //  Log the event log message 
    if (raid_group_number != FBE_RAID_GROUP_INVALID)
    {
        fbe_event_log_write(SEP_INFO_RAID_OBJECT_DISK_RB_LOGGING_STARTED, NULL, 0, 
                            "%d %d 0x%x", adjusted_position, raid_group_number, raid_group_object_id);
    }

    //  Return success  
    return FBE_STATUS_OK;

} // End fbe_raid_group_logging_log_rl_started()


/*!****************************************************************************
 * fbe_raid_group_logging_log_rl_cleared 
 ******************************************************************************
 * @brief
 *   This function will log a RL message when the disk position
 *   in the given RG stops rebuild logging.
 *
 * @param in_raid_group_p           - pointer to the raid group
 * @param in_packet_p               - pointer to a packet 
 * @param in_position               - disk position in the RG 
 *
 * @return fbe_status_t           
 *
 * @author
 *   11/2010 - Created. Susan Rundbaken (rundbs). 
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_logging_log_rl_cleared(
                                    fbe_raid_group_t*           in_raid_group_p,
                                    fbe_packet_t*               in_packet_p, 
                                    fbe_u32_t                   in_position)
{
    fbe_class_id_t                  class_id;                       // object class ID
    fbe_object_id_t                 raid_group_object_id;           // customer-visible raid group's object id 
    fbe_raid_group_number_t         raid_group_number;              // customer-visible raid group number
    fbe_u32_t                       adjusted_position;              // adjusted position of the disk no longer RL


    //  Get the class ID of the incoming object
    class_id = fbe_raid_group_get_class_id(in_raid_group_p);

    //  If the object is a VD object, return immediately; we do not log RL event messages for the VD
    if (FBE_CLASS_ID_VIRTUAL_DRIVE == class_id)
    {
        return FBE_STATUS_OK;
    }

    //  Get the raid group number and the raid group object ID, adjusted for RAID-10 if needed
    fbe_raid_group_logging_get_raid_group_number_and_object_id(in_raid_group_p,&raid_group_number,
                                                               &raid_group_object_id); 

    //  Get the disk position adjusted for RAID-10 if needed.  (This will only be displayed in the event log and 
    //  is not used anywhere else.)
    fbe_raid_group_logging_get_adjusted_disk_position(in_raid_group_p, in_position, &adjusted_position); 

    //  Log the event log message 
    if (raid_group_number != FBE_RAID_GROUP_INVALID)
    {
        fbe_event_log_write(SEP_INFO_RAID_OBJECT_DISK_RB_LOGGING_STOPPED, NULL, 0, 
                            "%d %d 0x%x", adjusted_position, raid_group_number, raid_group_object_id);
    }
    //  Return success  
    return FBE_STATUS_OK;

} // End fbe_raid_group_logging_log_rl_cleared()

/*!****************************************************************************
 * fbe_raid_group_logging_send_event_log_event_to_lun()
 ******************************************************************************
 * @brief
 *  This function sends the event log event to LUN object to log the event log from LUN.
 *
 * @param in_raid_group_p           - pointer to the raid group.
 * @param in_packet_p               - control packet sent to us from the scheduler.
 * @param in_event_log_event_data_p - pointer to event data to send
 * @param in_actual_lba             - start lba
 * 
 * @return fbe_status_t
 *
 * @author
 *  9/30/2011 - Created. Vishnu Sharma
 *  
 ******************************************************************************/
static fbe_status_t fbe_raid_group_logging_send_event_log_event_to_lun(
                                                fbe_raid_group_t*               in_raid_group_p,
                                                fbe_packet_t*                   in_packet_p,
                                                fbe_event_event_log_request_t*  in_event_log_event_data_p,
                                                fbe_lba_t                       in_actual_lba)
{
    fbe_event_t *                   event_p = NULL;
    fbe_event_stack_t *             event_stack = NULL;    
    fbe_block_count_t               block_count; 
    fbe_raid_geometry_t*            raid_geometry_p = NULL;
    fbe_u16_t                       data_disks; 
    
        
    event_p = fbe_memory_ex_allocate(sizeof(fbe_event_t));

    if(event_p == NULL)
    { 
        /* Do not worry we will send it later */
        fbe_base_object_trace( (fbe_base_object_t *)in_raid_group_p,
            FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s unable to allocate event",__FUNCTION__);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_event_init(event_p);
    fbe_event_set_master_packet(event_p, in_packet_p);
    event_stack = fbe_event_allocate_stack(event_p);
    

    // Get the chunk size
    block_count = fbe_raid_group_get_chunk_size(in_raid_group_p);

    // Get the number of data disks in this raid object
    raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);

    block_count = block_count * data_disks;
    
    /* Fill LBA range */
    fbe_event_stack_set_extent(event_stack, in_actual_lba * data_disks , block_count);
    
    // Populate the event data and send the event
    fbe_event_set_event_log_request_data(event_p, in_event_log_event_data_p);

    fbe_event_stack_set_completion_function(event_stack, fbe_raid_group_logging_send_event_log_event_to_lun_completion, in_raid_group_p);

    fbe_base_config_send_event((fbe_base_config_t *)in_raid_group_p, FBE_EVENT_TYPE_EVENT_LOG, event_p);

    return FBE_STATUS_OK;

}  // End fbe_raid_group_logging_send_event_log_event_to_lun()

/*!****************************************************************************
 * fbe_raid_group_logging_send_event_log_event_to_lun_completion()
 ******************************************************************************
 * @brief
 *  This is the send event log event to lun completion function. Retrieve the packet
 *  and complete the packet 
 *
 * @param event_p   - Event pointer.
 * @param context_p - Context which was passed with event.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  9/30/2011 - Created. Vishnu Sharma
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_logging_send_event_log_event_to_lun_completion(fbe_event_t * in_event_p,
                                                                         fbe_event_completion_context_t in_context)
{

    fbe_event_status_t                      event_status = FBE_EVENT_STATUS_INVALID;
    fbe_event_stack_t *                     event_stack = NULL;
    fbe_packet_t *                          packet_p = NULL;
    fbe_raid_group_t *                      raid_group_p = NULL;

    raid_group_p = (fbe_raid_group_t *) in_context;

    // get the data out of the event before we free it.
    event_stack = fbe_event_get_current_stack(in_event_p);
    fbe_event_get_master_packet(in_event_p, &packet_p);
    fbe_event_get_status(in_event_p, &event_status);
    
    
    fbe_event_release_stack(in_event_p, event_stack);
    fbe_event_destroy(in_event_p);
    fbe_memory_ex_release(in_event_p);

        
    return FBE_STATUS_OK;

}  // End fbe_raid_group_logging_send_event_log_event_to_lun_completion()

/*!****************************************************************************
 * fbe_raid_group_logging_break_context_log_and_checkpoint_update()
 ******************************************************************************
 * @brief
 *   This function will log a message that a rebuild or copy of a LUN has 
 *   started or completed when needed.  It is specfic to rebuild operations 
 *   (rebuild and copy) as opposed to verify.
 * 
 * @param raid_group_p           - pointer to the raid group
 * @param packet_p               - pointer to a packet 
 *
 * @return  fbe_status_t           
 *
 * @author
 *  03/09/2012  Ron Proulx  - Created
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_logging_break_context_log_and_checkpoint_update(fbe_raid_group_t *raid_group_p,
                                                              fbe_packet_t *packet_p,
                                                              fbe_raid_position_bitmask_t checkpoint_change_bitmask,
                                                              fbe_lba_t checkpoint_update_lba,
                                                              fbe_block_count_t checkpoint_update_blocks)

{
    fbe_raid_group_rebuild_context_t   *rebuild_context_p;          // pointer to the rebuild context
    fbe_queue_head_t                    raid_group_packet_queue;

    //  Get a pointer to the rebuild context from the packet
    fbe_raid_group_rebuild_get_rebuild_context(raid_group_p, packet_p, &rebuild_context_p);

    // Populate the checkpoint update information
    fbe_raid_group_rebuild_context_set_update_checkpoint_bitmask(rebuild_context_p, checkpoint_change_bitmask);
    fbe_raid_group_rebuild_context_set_update_checkpoint_lba(rebuild_context_p, checkpoint_update_lba);
    fbe_raid_group_rebuild_context_set_update_checkpoint_blocks(rebuild_context_p, checkpoint_update_blocks);
    fbe_raid_group_rebuild_context_set_event_log_position(rebuild_context_p, FBE_RAID_POSITION_INVALID);
    fbe_raid_group_rebuild_context_set_event_log_source_position(rebuild_context_p, FBE_RAID_POSITION_INVALID);

    /* In most cases it is the event thread (since we have asked for permission)
     * that this method is invoked on.  Since we don't want to tie up the event
     * thread while the I/O is executed, we must queue the packet to the block
     * transport.
     */
    fbe_queue_init(&raid_group_packet_queue);
    if (fbe_queue_is_element_on_queue(&packet_p->queue_element)){
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s packet %p already on the queue.\n", __FUNCTION__, packet_p);
    }
    fbe_queue_push(&raid_group_packet_queue, &packet_p->queue_element);
    fbe_transport_run_queue_push(&raid_group_packet_queue, 
                                 fbe_raid_group_logging_log_lun_and_update_checkpoint_for_rebuild_op,
                                 (fbe_packet_completion_context_t)raid_group_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
// end fbe_raid_group_logging_break_context_log_and_checkpoint_update()


