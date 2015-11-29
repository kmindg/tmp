/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main entry points for the logical drive object.
 *  This includes the create and destroy methods for the logical drive, as
 *  well as the event entry point, load and unload methods.
 * 
 * @ingroup logical_drive_class_files
 * 
 * @version
 *   10/29/2007:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_logical_drive.h"
#include "base_object_private.h"
#include "fbe_logical_drive_private.h"
#include "fbe_logical_drive_io_command.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe_notification.h"
#include "fbe/fbe_enclosure.h"
#include "fbe/fbe_winddk.h"
/* Class methods forward declaration.
 */
fbe_status_t fbe_logical_drive_load(void);
fbe_status_t fbe_logical_drive_unload(void);
fbe_status_t fbe_logical_drive_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_logical_drive_destroy( fbe_object_handle_t object_handle);
fbe_status_t fbe_logical_drive_destroy_interface( fbe_object_handle_t object_handle);
fbe_status_t fbe_logical_drive_event_entry(fbe_object_handle_t object_handle, 
                                           fbe_event_type_t event,
                                           fbe_event_context_t event_context);
fbe_status_t fbe_logical_drive_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p);
fbe_status_t fbe_logical_drive_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p);
static fbe_status_t fbe_logical_drive_process_block_transport_event(fbe_block_transport_event_type_t event_type,
                                                                   fbe_block_trasnport_event_context_t context);

static fbe_status_t fbe_logical_drive_process_incoming_io_duing_hibernate(fbe_logical_drive_t * logical_drive);

/* Export class methods.
 */
fbe_class_methods_t fbe_logical_drive_class_methods = {FBE_CLASS_ID_LOGICAL_DRIVE,
                                                       fbe_logical_drive_load,
                                                       fbe_logical_drive_unload,
                                                       fbe_logical_drive_create_object,
                                                       fbe_logical_drive_destroy_interface,
                                                       fbe_logical_drive_control_entry,
                                                       fbe_logical_drive_event_entry,
                                                       fbe_logical_drive_io_entry,
                                                       fbe_logical_drive_monitor_entry};

/* This is our global transport constant, which we use to setup a portion of our block 
 * transport server. This is global since it is the same for all logical drives.
 */
fbe_block_transport_const_t fbe_logical_drive_block_transport_const = {fbe_logical_drive_block_transport_entry,
                                                                        fbe_logical_drive_process_block_transport_event,
                                                                        fbe_logical_drive_io_entry,
                                                                        NULL, NULL};

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/

static fbe_status_t logical_drive_get_capacity(fbe_logical_drive_t * logical_drive, 
                                               fbe_packet_t * packet);
static fbe_status_t logical_drive_send_packet(fbe_logical_drive_t * logical_drive);

static fbe_status_t fbe_logical_drive_get_location_info_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_ldo_process_traffic_load_change(fbe_logical_drive_t * const logical_drive_p);

/*!***************************************************************
 * fbe_logical_drive_load()
 ****************************************************************
 * @brief
 *  This function is called to construct any
 *  global data for the class.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  11/01/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_logical_drive_load(void)
{
    fbe_status_t status;

    /* At present we do not have any global data to construct.
     */
    FBE_ASSERT_AT_COMPILE_TIME((sizeof(fbe_logical_drive_attributes_t) * 8) > FBE_LOGICAL_DRIVE_FLAG_LAST);

    /* Init the small logical drive pool of sg lists.
     */
    status = fbe_ldo_io_cmd_pool_init();
    if (status != FBE_STATUS_OK)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_LOGICAL_DRIVE,
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "logical drive: unable to init sg pool. %s\n", __FUNCTION__);
        return status;
    }

    /* Init the bitbucket.
     */
    fbe_ldo_bitbucket_init();
    return FBE_STATUS_OK;
}
/* end fbe_logical_drive_load() */

/*!***************************************************************
 * fbe_logical_drive_unload()
 ****************************************************************
 * @brief
 *  This function is called to tear down any
 *  global data for the class.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  11/01/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_logical_drive_unload(void)
{
    /* Destroy any global data.
     */
    fbe_ldo_io_cmd_pool_destroy();
    return FBE_STATUS_OK;
}
/* end fbe_logical_drive_unload() */

/*!***************************************************************
 * fbe_logical_drive_create_object(fbe_packet_t * packet_p, 
 *                                 fbe_object_handle_t * object_handle)
 ****************************************************************
 * @brief
 *  This function is called to create an instance of this class.
 *
 * @param packet_p - The packet used to construct the object.  
 * @param object_handle - The object being created.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  11/01/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_logical_drive_create_object(fbe_packet_t * packet_p, 
                                fbe_object_handle_t * object_handle)
{
    fbe_logical_drive_t *logical_drive_p;
    fbe_status_t status;

    /* Call the base class create function.
     */
    status = fbe_base_discovered_create_object(packet_p, object_handle);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Set class id.
     */
    logical_drive_p = (fbe_logical_drive_t *)fbe_base_handle_to_pointer(*object_handle);

    fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                          "logical drive create: %s Object: 0x%x Ptr: 0x%p\n", 
                          __FUNCTION__,
                          logical_drive_p->base_discovered.base_object.object_id,
                          logical_drive_p);

    /* Set class id.
     */
    fbe_base_object_set_class_id((fbe_base_object_t *) logical_drive_p, FBE_CLASS_ID_LOGICAL_DRIVE);

    /* Simply call our static init function to init members that have no
     * dependencies. 
     */
    return fbe_logical_drive_init_members(logical_drive_p);
}
/* end fbe_logical_drive_create_object() */

/*!***************************************************************
 * fbe_logical_drive_init_members()
 ****************************************************************
 * @brief
 *  This function initializes the logical drive object.
 *
 *  Note that everything that is created here must be
 *  destroyed within fbe_logical_drive_destroy().
 *
 * @param logical_drive_p - The logical drive object.
 *
 * @return
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  11/28/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_logical_drive_init_members(fbe_logical_drive_t * const logical_drive_p)
{
    fbe_status_t status;
    /* Initialize our local members.
     */
    fbe_block_transport_server_init(&logical_drive_p->block_transport_server);

    /* Setup the constant portion of the block transport server.
     */
    status = fbe_block_transport_server_set_block_transport_const(&logical_drive_p->block_transport_server,
                                                                  &fbe_logical_drive_block_transport_const,
                                                                  logical_drive_p);

    status = fbe_block_transport_server_set_stack_limit(&logical_drive_p->block_transport_server);

    /* Set up this object's defaults.
     */
    /* Peter Puhov - This is an empty function 
    fbe_ldo_setup_defaults(logical_drive_p);
    */
    logical_drive_p->object_id = FBE_OBJECT_ID_INVALID;
    logical_drive_p->port_number = FBE_PORT_NUMBER_INVALID;
    logical_drive_p->enclosure_number = FBE_ENCLOSURE_NUMBER_INVALID;
    logical_drive_p->slot_number = FBE_SLOT_NUMBER_INVALID;

    return FBE_STATUS_OK;
}
/* end fbe_logical_drive_init_members() */

/*!***************************************************************
 * fbe_logical_drive_destroy_interface()
 ****************************************************************
 * @brief
 *  This function is the interface function, which is called
 *  by the topology to destroy an instance of this class.
 *
 * @param object_handle - The object being destroyed.
 *
 * @return fbe_status_t
 *  STATUS of the destroy operation.
 *
 * @author
 *  12/04/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_logical_drive_destroy_interface(fbe_object_handle_t object_handle)
{
    fbe_status_t status;

    fbe_base_object_trace((fbe_base_object_t*)fbe_base_handle_to_pointer(object_handle), 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                          "fbe_logical_drive_main: %s entry\n", __FUNCTION__);

    /* Destroy this object.
     */
    fbe_logical_drive_destroy(object_handle);
    
    /* Call parent destructor.
     */
    status = fbe_base_discovered_destroy_object(object_handle);

    return status;
}
/* end fbe_logical_drive_destroy_interface */

/*!***************************************************************
 * fbe_logical_drive_destroy()
 ****************************************************************
 * @brief
 *  This function is called to destroy the logical drive object.
 *
 *  This function tears down everything that was created in
 *  fbe_logical_drive_init().
 *
 * @param object_handle - The object being destroyed.
 *
 * @return fbe_status_t
 *  STATUS of the destroy operation.
 *
 * @author
 *  11/01/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_logical_drive_destroy(fbe_object_handle_t object_handle)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_drive_t * logical_drive_p;

    fbe_base_object_trace((fbe_base_object_t*)fbe_base_handle_to_pointer(object_handle), 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_CREATE_OBJECT, 
                          "fbe_logical_drive_main: %s entry\n", __FUNCTION__);
    
    logical_drive_p = (fbe_logical_drive_t *)fbe_base_handle_to_pointer(object_handle);

    /* Cleanup the server.
     */
    if (logical_drive_p->block_transport_server.base_transport_server.client_list != NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)fbe_base_handle_to_pointer(object_handle), 
                          FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_CREATE_OBJECT, 
                          "%s client list is not empty!!\n", __FUNCTION__);
    }
    fbe_block_transport_server_destroy(&logical_drive_p->block_transport_server);

    return status;
}
/* end fbe_logical_drive_destroy */

/*!**************************************************************
 * fbe_logical_drive_state_change_event_entry()
 ****************************************************************
 * @brief
 *  This is the handler for a change in the edge state.
 *
 * @param logical_drive_p - logical drive that is changing.
 * @param event_context - The context for the event.
 *
 * @return - Status of the handling.
 *
 * @author
 *  7/15/2008 - Created. RPF
 *
 ****************************************************************/

static fbe_status_t 
fbe_logical_drive_state_change_event_entry(fbe_logical_drive_t * const logical_drive_p, 
                                           fbe_event_context_t event_context)
{
    fbe_status_t            status;
    fbe_path_state_t        path_state;
    fbe_path_attr_t         path_attr = 0;
    fbe_lifecycle_state_t   lifecycle_state;

    /* Fetch the path state.  Note that while it returns a status, it currently
     * never fails. 
     */ 
    status = fbe_block_transport_get_path_state(&logical_drive_p->block_edge, &path_state);
    if (status != FBE_STATUS_OK)
    {
        /* Not able to get the path state.
         */
        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_CREATE_OBJECT, 
                              "%s cannot get block edge path state 0x%x!!\n", __FUNCTION__, status);
        return status;
    }

    /*let's see if we need to process the attributes*/
    fbe_block_transport_get_path_attributes (&logical_drive_p->block_edge, &path_attr);

    /* Now use the path state and attributes to determine what to due.
     */
    switch (path_state)
    {
        case FBE_PATH_STATE_INVALID:
            break;
        case FBE_PATH_STATE_ENABLED:
            /* If the logical drive object is in the failed state and the 
             * downstream health is now optimal, transition to activate.
             */
            fbe_base_object_get_lifecycle_state((fbe_base_object_t *)logical_drive_p, &lifecycle_state);
            if (lifecycle_state == FBE_LIFECYCLE_STATE_FAIL)
            {
                /* We were failed but now our downstream edge has become enabled.
                 * Transition to activate and attempt to come Ready.
                 */
                status = fbe_lifecycle_set_cond(&fbe_logical_drive_lifecycle_const, 
                                                (fbe_base_object_t*)logical_drive_p, 
                                                FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);
            }
            break;
        case FBE_PATH_STATE_DISABLED:
            /* If the path state is no longer enabled (but still valid), 
             * set block edge disabled condition 
             */
            status = fbe_lifecycle_set_cond(&fbe_logical_drive_lifecycle_const,
                                            (fbe_base_object_t*)logical_drive_p,
                                            FBE_LOGICAL_DRIVE_LIFECYCLE_COND_BLOCK_EDGE_DISABLED);
            #if 0/*sharel moved to the handling of FBE_EVENT_TYPE_ATTRIBUTE_CHANGED*/
            if(path_attr & (FBE_BLOCK_PATH_ATTR_FLAGS_CHECK_QUEUED_IO_TIMER |
                            FBE_BLOCK_PATH_ATTR_FLAGS_CALL_HOME_END_OF_LIFE) ){
                status = fbe_ldo_process_attributes (logical_drive_p, path_attr);
            }
            #endif
            break;
        case FBE_PATH_STATE_BROKEN:
            /* The object below us has transitioned to a failed state. 
             * We will simply transition to failed.
             */
            status = fbe_lifecycle_set_cond(&fbe_logical_drive_lifecycle_const,
                                            (fbe_base_object_t*)logical_drive_p,
                                            FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);
            break;
        case FBE_PATH_STATE_GONE:
            /* The path state is gone, so we will transition ourselves to 
             * destroy. 
             */
            status = fbe_lifecycle_set_cond(&fbe_logical_drive_lifecycle_const,
                                            (fbe_base_object_t*)logical_drive_p,
                                            FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY);
            break;
        case FBE_PATH_STATE_SLUMBER:
             /* The PDO went to hibernate state
             */
            status = fbe_lifecycle_set_cond(&fbe_logical_drive_lifecycle_const,
                                            (fbe_base_object_t*)logical_drive_p,
                                            FBE_BASE_OBJECT_LIFECYCLE_COND_HIBERNATE);
            break;
        default:
            /* This is a path state we do not expect.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                                  FBE_TRACE_LEVEL_WARNING, 
                                  FBE_TRACE_MESSAGE_ID_CREATE_OBJECT, 
                                  "%s path state unexpected %d\n", __FUNCTION__, path_state);
            break;

    }
    return status;
}
/******************************************
 * end fbe_logical_drive_state_change_event_entry()
 ******************************************/

/*!***************************************************************
 * fbe_logical_drive_event_entry()
 ****************************************************************
 * @brief
 *  This function is called to pass an event to a given instance
 *  of the logical drive class.
 *
 * @param object_handle - The object receiving the event.
 * @param event_type - Type of event that is arriving. e.g. state change.
 * @param event_context - Context that is associated with the event.
 *
 * @return
 *  fbe_status_t
 *
 * @author
 *  11/01/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_logical_drive_event_entry(fbe_object_handle_t object_handle, 
                              fbe_event_type_t event_type,
                              fbe_event_context_t event_context)
{
    fbe_logical_drive_t *logical_drive_p = NULL;
    fbe_status_t        status;
    fbe_path_attr_t     path_attr = 0;
    fbe_path_state_t    path_state = FBE_PATH_STATE_INVALID;

    logical_drive_p = (fbe_logical_drive_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)logical_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry event_type %d context 0x%p\n",
                          __FUNCTION__, event_type, event_context);

    /* First handle the event we have received.
     */
    switch (event_type)
    {
        case FBE_EVENT_TYPE_EDGE_STATE_CHANGE:
            status = fbe_logical_drive_state_change_event_entry(logical_drive_p, event_context);
            break;
        case FBE_EVENT_TYPE_EDGE_TRAFFIC_LOAD_CHANGE:
            status = fbe_ldo_process_traffic_load_change(logical_drive_p);
            break;
        case FBE_EVENT_TYPE_ATTRIBUTE_CHANGED:
            fbe_block_transport_get_path_attributes (&logical_drive_p->block_edge, &path_attr);
            
            /* We don't expect a `broken' edge unless one of the fault
             * attributes is set.
             */
            fbe_block_transport_get_path_state(&logical_drive_p->block_edge, &path_state);
            if ( (path_state == FBE_PATH_STATE_BROKEN)              &&
                !(path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_FAULT_MASK)    )
            {
                fbe_base_object_trace((fbe_base_object_t *)logical_drive_p, 
                                      FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s LINK/DRIVE FAULT not set state: %d attr 0x%x\n",
                                      __FUNCTION__, path_state, path_attr);
            }

            /* Process the attributes.
             */
            status = fbe_ldo_process_attributes (logical_drive_p, path_attr);
            break;
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_base_object_trace((fbe_base_object_t*)logical_drive_p,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                  "%s Uknown event event_type %d context 0x%p\n",
                                  __FUNCTION__, event_type, event_context);
            break;

    }
    /* We also need to let the base class know about the event.
     */
    status = fbe_base_discovered_event_entry(object_handle, event_type, event_context);

    return status;
}
/* end fbe_logical_drive_event_entry() */

/*!***************************************************************
 * fbe_ldo_setup_block_sizes()
 ****************************************************************
 * @brief
 *  This function translates an imported lba from
 *  imported block size to exported block size.
 *
 * @param logical_drive_p - The logical drive object.
 * @param imported_block_size - The block size exported by lower level.
 *
 * @return void
 *
 * @author
 *  11/01/07 - Created. RPF
 *
 ****************************************************************/
void
fbe_ldo_setup_block_sizes(fbe_logical_drive_t * const logical_drive_p,
                          fbe_block_size_t imported_block_size)
{
    /* If the block sizes have changed, we should refresh it now.
     */
    fbe_logical_drive_lock(logical_drive_p);

    if ( (fbe_ldo_get_imported_block_size(logical_drive_p) != imported_block_size))
    {
        fbe_block_edge_t * edge_p = &logical_drive_p->block_edge;
        fbe_block_edge_geometry_t block_edge_geometry;

        switch(imported_block_size)
        {
            case 512:
                block_edge_geometry = FBE_BLOCK_EDGE_GEOMETRY_512_512;
                break;
            case 520:
                block_edge_geometry = FBE_BLOCK_EDGE_GEOMETRY_520_520;
                break;
            case 4096:
                block_edge_geometry = FBE_BLOCK_EDGE_GEOMETRY_4096_4096;
                break;
            case 4160:
                block_edge_geometry = FBE_BLOCK_EDGE_GEOMETRY_4160_4160;
                break;
            default:
                fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                                      FBE_TRACE_LEVEL_ERROR, 
                                      FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                                      "%s Invalid imported size %d\n", __FUNCTION__, imported_block_size);
                block_edge_geometry = FBE_BLOCK_EDGE_GEOMETRY_INVALID;
                break;
        };
        fbe_block_transport_edge_set_geometry(edge_p, block_edge_geometry);
    }
    /* We're done with updating the sizes, unlock now.
     */
    fbe_logical_drive_unlock(logical_drive_p);
    return;
}
/*********************************************
 * fbe_ldo_setup_block_sizes()
 *********************************************/

/*!***************************************************************
 * fbe_ldo_process_attributes()
 ****************************************************************
 * @brief
 *  This function processes the new attributes we see on the edge
 *  with the physical drive
 *
 * @param logical_drive_p - The pointer of the sending logical drive.
 * @param attribute - The new attribute on the edge.
 *
 * @return fbe_status_t FBE_STATUS_OK
 *
 * @author
 *  12/11/07 - Created. Shay Harel.
 *  11/03/09 - Modified to inform the upstream edge. Dhaval Patel
 *  02/08/12 - Fixed so a notification sent for each attribute set.   Wayne Garrett
 *
 ****************************************************************/
fbe_status_t 
fbe_ldo_process_attributes(fbe_logical_drive_t * const logical_drive_p, fbe_path_attr_t attribute)
{
    fbe_notification_info_t     notification;
    fbe_object_id_t             my_object_id;
    fbe_u32_t                   clients = 0;
    fbe_u32_t                   mask = 0;

    notification.class_id = FBE_CLASS_ID_LOGICAL_DRIVE;
    notification.object_type = FBE_TOPOLOGY_OBJECT_TYPE_LOGICAL_DRIVE;

    fbe_base_object_get_object_id((fbe_base_object_t *)logical_drive_p, &my_object_id);

    fbe_block_transport_server_get_number_of_clients(&(logical_drive_p->block_transport_server),
                                                     &clients);

    if (0 == clients)
    {
        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s no client edges connected. attr:0x%x\n",__FUNCTION__, attribute);

        /* optimization to only set condition for attributes that need
           special handling when no client edges connected*/
        if (attribute & (FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ |
                         FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ_FAST_DL |
                         FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ_TRIAL_RUN))
        {
            fbe_lifecycle_set_cond(&fbe_logical_drive_lifecycle_const, (fbe_base_object_t*)logical_drive_p,
                                   FBE_LOGICAL_DRIVE_LIFECYCLE_COND_NO_CLIENT_EDGES_CONNECTED);
        }
    }
    else
    {
        /* Propagate download attribute changes.
         * Propagate health check request attribute changes.
         * Propagate drive/link fault attributes.
         */
        mask = (FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_MASK | 
                FBE_BLOCK_PATH_ATTR_FLAGS_HEALTH_CHECK_REQUEST |
                FBE_BLOCK_PATH_ATTR_FLAGS_FAULT_MASK);

        fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(&(logical_drive_p->block_transport_server), 
                (attribute & mask), mask);  
    }

    /* Check the edge attributes and send the appropriate notification.  Note that more than
       one notification may be sent if multiple attributes are set.
     */
    if (attribute & FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE)
    {

        /* drive is dying so send the notification event to registered users.
         */

        if (clients > 0)
        {
            /* Update the end of life attributes on the edge with upstream object.
             */
            fbe_block_transport_server_set_path_attr(&(logical_drive_p->block_transport_server), 
                                                     0, 
                                                     FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE);
        }

        notification.notification_type = FBE_NOTIFICATION_TYPE_END_OF_LIFE;
        fbe_notification_send(my_object_id, notification);
    }
    else
    {
        /* Else check if end-of-life needs to be cleared on the upstream edge.
         */
        fbe_path_attr_t upstream_path_attr = 0;

        fbe_block_transport_server_get_number_of_clients(&(logical_drive_p->block_transport_server),
                                                         &clients);
        if (clients > 0)
        {
            /* Update the end of life attributes on the edge with upstream object.
             */
            fbe_block_transport_server_get_path_attr(&(logical_drive_p->block_transport_server),
                                                     0, 
                                                     &upstream_path_attr);
            if (upstream_path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE)
            {
                fbe_block_transport_server_clear_path_attr(&(logical_drive_p->block_transport_server), 
                                                           0, 
                                                           FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE);
            }
        }
    }

    if (attribute & FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS)
    {
        /* Update the timeout errors attributes on the edge with upstream object.
         */
        fbe_block_transport_server_set_path_attr_all_servers(&(logical_drive_p->block_transport_server), 
                                                             FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS);
    }

    return FBE_STATUS_OK;
}
/*********************************************
 * fbe_ldo_process_attributes()
 *********************************************/

fbe_status_t
fbe_logical_drive_get_location_info(fbe_logical_drive_t * const logical_drive_p, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_ex_t  * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_semaphore_t sem;

    fbe_base_port_mgmt_get_port_number_t            get_port_number;
    fbe_base_enclosure_mgmt_get_enclosure_number_t  get_enclosure_number;
    fbe_base_enclosure_mgmt_get_slot_number_t       get_slot_number;

    fbe_semaphore_init(&sem, 0, 1);

    payload = fbe_transport_get_payload_ex(packet);

    /* Get port number */
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    get_port_number.port_number = FBE_PORT_NUMBER_INVALID;

    fbe_payload_control_build_operation(control_operation, 
                                        FBE_BASE_PORT_CONTROL_CODE_GET_PORT_NUMBER, 
                                        &get_port_number, 
                                        sizeof(fbe_base_port_mgmt_get_port_number_t));

    status = fbe_transport_set_completion_function( packet, fbe_logical_drive_get_location_info_completion, &sem);
    fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_TRAVERSE);

    fbe_base_discovered_send_control_packet((fbe_base_discovered_t *) logical_drive_p, packet);
    fbe_semaphore_wait(&sem, NULL);
    fbe_payload_ex_release_control_operation(payload, control_operation);
    
    logical_drive_p->port_number = get_port_number.port_number;

    /* Get enclosure number */
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    get_enclosure_number.enclosure_number = FBE_ENCLOSURE_NUMBER_INVALID;

    fbe_payload_control_build_operation(control_operation, 
                                        FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_NUMBER, 
                                        &get_enclosure_number, 
                                        sizeof(fbe_base_enclosure_mgmt_get_enclosure_number_t));

    status = fbe_transport_set_completion_function( packet, fbe_logical_drive_get_location_info_completion, &sem);
    fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_TRAVERSE);

    fbe_base_discovered_send_control_packet((fbe_base_discovered_t *) logical_drive_p, packet);
    fbe_semaphore_wait(&sem, NULL);
    fbe_payload_ex_release_control_operation(payload, control_operation);
    
    logical_drive_p->enclosure_number = get_enclosure_number.enclosure_number;
    
    /* Get slot number */
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    get_slot_number.enclosure_slot_number = FBE_SLOT_NUMBER_INVALID;

    fbe_payload_control_build_operation(control_operation, 
                                        FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SLOT_NUMBER, 
                                        &get_slot_number, 
                                        sizeof(fbe_base_enclosure_mgmt_get_slot_number_t));

    status = fbe_transport_set_completion_function( packet, fbe_logical_drive_get_location_info_completion, &sem);
    fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_TRAVERSE);

    fbe_base_discovered_send_control_packet((fbe_base_discovered_t *) logical_drive_p, packet);
    fbe_semaphore_wait(&sem, NULL);
    fbe_payload_ex_release_control_operation(payload, control_operation);
    
    logical_drive_p->slot_number = get_slot_number.enclosure_slot_number;

    fbe_semaphore_destroy(&sem);
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_logical_drive_get_location_info_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

static fbe_status_t fbe_ldo_process_traffic_load_change(fbe_logical_drive_t * const logical_drive_p)
{
    fbe_traffic_priority_t      traffic_load = FBE_TRAFFIC_PRIORITY_INVALID;
    fbe_traffic_priority_t      server_traffic_load = FBE_TRAFFIC_PRIORITY_INVALID;

    /*compare the below and above edge so we can make a deceision*/

    traffic_load = fbe_block_transport_edge_get_traffic_priority(&logical_drive_p->block_edge);
    fbe_block_transport_server_get_traffic_priority(&logical_drive_p->block_transport_server, &server_traffic_load);
    if (traffic_load != server_traffic_load) {
        /*let's generate an event to upper edges, since we have one edge only and we are the first one to receive,
        we still do it at the context of the PDO call. Later, the PVD will need to process it on the monitor context*/
        fbe_block_transport_server_set_traffic_priority(&logical_drive_p->block_transport_server, traffic_load);
    }

    return FBE_STATUS_OK;
}

fbe_status_t fbe_logical_drive_process_block_transport_event(fbe_block_transport_event_type_t event_type,
                                                                   fbe_block_trasnport_event_context_t context)
{
    fbe_logical_drive_t * logical_drive = (fbe_logical_drive_t *)context;

    switch(event_type) {
    case FBE_BLOCK_TRASPORT_EVENT_TYPE_IO_WAITING_ON_QUEUE:
        /*there is an incomming IO, we need to wake up the object and make sure once it's in ready, to send the IO*/
        fbe_logical_drive_process_incoming_io_duing_hibernate(logical_drive);
        break;
    default:
        fbe_base_object_trace((fbe_base_object_t*)logical_drive,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s can't process event:%d from block transport\n",__FUNCTION__, event_type);

    }
    

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_logical_drive_process_incoming_io_duing_hibernate(fbe_logical_drive_t * logical_drive)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;

    /*set a condition on the READY rotary once the object becomes alive*/
    status = fbe_lifecycle_set_cond(&fbe_logical_drive_lifecycle_const,
                                    (fbe_base_object_t*)logical_drive,
                                     FBE_LOGICAL_DRIVE_LIFECYCLE_COND_DEQUEUE_PENDING_IO);

    if (status != FBE_STATUS_OK) {
         fbe_base_object_trace((fbe_base_object_t *)logical_drive,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                                "%s can't set condition to process IO after drive wakeup!\n",__FUNCTION__ );

         return FBE_STATUS_GENERIC_FAILURE;

    }

    /*and move it to activate so it starts waking up*/
    status = fbe_lifecycle_set_cond(&fbe_logical_drive_lifecycle_const, 
                                    (fbe_base_object_t*)logical_drive, 
                                    FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);

    if (status != FBE_STATUS_OK) {
         fbe_base_object_trace((fbe_base_object_t *)logical_drive,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s can't set object to activate after drive wakeup!\n",__FUNCTION__ );

         return FBE_STATUS_GENERIC_FAILURE;

    }
    

    return FBE_STATUS_OK;
}

/*******************************
 * end fbe_logical_drive_main.c
 *******************************/
