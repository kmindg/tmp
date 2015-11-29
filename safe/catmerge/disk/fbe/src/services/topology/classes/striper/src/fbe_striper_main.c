/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_striper_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main entry points for the striper object.
 *  This includes the create and destroy methods for the striper, as
 *  well as the event entry point, load and unload methods.
 * 
 * @ingroup striper_class_files
 * 
 * @version
 *   07/21/2009:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_striper.h"
#include "base_object_private.h"
#include "fbe_striper_private.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe_notification.h"
#include "fbe_raid_group_object.h"

/* Class methods forward declaration.
 */
fbe_status_t fbe_striper_load(void);
fbe_status_t fbe_striper_unload(void);
fbe_status_t fbe_striper_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_striper_destroy( fbe_object_handle_t object_handle);
fbe_status_t fbe_striper_destroy_interface( fbe_object_handle_t object_handle);
fbe_status_t fbe_striper_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p);
fbe_status_t fbe_striper_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p);

/* Export class methods.
 */
fbe_class_methods_t fbe_striper_class_methods = {FBE_CLASS_ID_STRIPER,
                                                fbe_striper_load,
                                                fbe_striper_unload,
                                                fbe_striper_create_object,
                                                fbe_striper_destroy_interface,
                                                fbe_striper_control_entry,
                                                fbe_striper_event_entry,
                                                fbe_striper_io_entry,
                                                fbe_striper_monitor_entry};

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/

/*!***************************************************************
 * fbe_striper_load()
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
 *  07/21/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_striper_load(void)
{
    fbe_status_t status;

    /* Make sure our striper object fits inside a memory chunk.
     */
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_striper_t) <= FBE_MEMORY_CHUNK_SIZE);

    /* Make sure the monitor is sane. 
     * No sense trying to bring up any object lifecycle that is not valid.
     */ 
    status = fbe_striper_monitor_load_verify();

    if (status != FBE_STATUS_OK)
    {
        /* Panic - Indicates something wrong with the conditions and rotaries.
         * Logging CRITICAL_ERROR will take care of PANIC
         */
        fbe_topology_class_trace(FBE_CLASS_ID_STRIPER,
                                 FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                                 "%s fbe_striper_monitor_load_verify failed with status 0x%x\n", 
                                 __FUNCTION__, status);
    }
    else
    {
        /* This currently just does compile time checks.  If that changes we should check for status here.
         */
        fbe_raid_group_load();
    }
    /* At present we do not have any global data to construct.
     */
    return FBE_STATUS_OK;
}
/* end fbe_striper_load() */

/*!***************************************************************
 * fbe_striper_unload()
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
 *  07/21/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_striper_unload(void)
{
    /* Destroy any global data.
     */
    return FBE_STATUS_OK;
}
/* end fbe_striper_unload() */

/*!***************************************************************
 * fbe_striper_create_object()
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
 *  07/21/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_striper_create_object(fbe_packet_t * packet_p, 
                         fbe_object_handle_t * object_handle)
{
    fbe_striper_t *striper_p;
    fbe_status_t status;

    /* Call the base class create function.
     */
    status = fbe_base_object_create_object(packet_p, object_handle);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Set class id.
     */
    striper_p = (fbe_striper_t *)fbe_base_handle_to_pointer(*object_handle);

    fbe_base_object_set_class_id((fbe_base_object_t *) striper_p, FBE_CLASS_ID_STRIPER);

	fbe_base_config_send_specialize_notification((fbe_base_config_t *) striper_p);

    fbe_base_object_trace((fbe_base_object_t*)striper_p, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                          "striper create: %s Object: 0x%x Ptr: 0x%p\n", 
                          __FUNCTION__,
                          striper_p->raid_group.base_config.base_object.object_id,
                          striper_p);

    /* Force verify to make sure the object is properly initialized before scheduling it.
     * This avoids race condition between scheduler thread and usurper command trying to 
     * initialize the object rotary preset at the same time.
     */
    fbe_lifecycle_verify((fbe_lifecycle_const_t *)&fbe_striper_lifecycle_const, (fbe_base_object_t*)striper_p);

	/* Enable lifecycle tracing if requested */
	status = fbe_base_object_enable_lifecycle_debug_trace((fbe_base_object_t *)striper_p);


    /* Simply call our static init function to init members that have no
     * dependencies. 
     */
    return fbe_striper_init_members(striper_p);
}
/* end fbe_striper_create_object() */

/*!***************************************************************
 * fbe_striper_init_members()
 ****************************************************************
 * @brief
 *  This function initializes the striper object.
 *
 *  Note that everything that is created here must be
 *  destroyed within fbe_striper_destroy().
 *
 * @param striper_p - The striper object.
 *
 * @return
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  07/21/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_striper_init_members(fbe_striper_t * const striper_p)
{
    striper_p->block_edge_p = fbe_memory_native_allocate(sizeof(fbe_block_edge_t) * FBE_STRIPER_MAX_EDGES);
    /* striper_p->block_edge_p = striper_p->block_edge_array; */

    if (striper_p->block_edge_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)striper_p, 
                          FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                          "%s unable to allocate edge memory Object: 0x%x Ptr: 0x%p\n", 
                          __FUNCTION__,
                          striper_p->raid_group.base_config.base_object.object_id,
                          striper_p);
    }
    /* Let the raid group initialize.
     */
    fbe_raid_group_init(&striper_p->raid_group,
                        striper_p->block_edge_p,
                        (fbe_raid_common_state_t)fbe_striper_generate_start);

    /* Initialize our members.
     */
    return FBE_STATUS_OK;
}
/* end fbe_striper_init_members() */

/*!***************************************************************
 * fbe_striper_destroy_interface()
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
 *  05/21/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_striper_destroy_interface(fbe_object_handle_t object_handle)
{
    fbe_status_t status;

    fbe_base_object_trace((fbe_base_object_t*)fbe_base_handle_to_pointer(object_handle), 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                          "fbe_striper_main: %s entry\n", __FUNCTION__);

    /* Destroy this object.
     */
    fbe_striper_destroy(object_handle);
    
    /* Call parent destructor.
     */
    status = fbe_raid_group_destroy_interface(object_handle);

    return status;
}
/* end fbe_striper_destroy_interface */

/*!***************************************************************
 * fbe_striper_destroy()
 ****************************************************************
 * @brief
 *  This function is called to destroy the striper object.
 *
 *  This function tears down everything that was created in
 *  fbe_striper_init().
 *
 * @param object_handle - The object being destroyed.
 *
 * @return fbe_status_t
 *  STATUS of the destroy operation.
 *
 * @author
 *  07/21/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_striper_destroy(fbe_object_handle_t object_handle)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_striper_t * striper_p;

    fbe_base_object_trace((fbe_base_object_t*)fbe_base_handle_to_pointer(object_handle), 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_CREATE_OBJECT, 
                          "fbe_striper_main: %s entry\n", __FUNCTION__);
    
    striper_p = (fbe_striper_t *)fbe_base_handle_to_pointer(object_handle);

    if (striper_p->block_edge_p != NULL)
    {
        fbe_memory_native_release(striper_p->block_edge_p);
    }
    return status;
}
/* end fbe_striper_destroy */

/*******************************
 * end fbe_striper_main.c
 *******************************/
