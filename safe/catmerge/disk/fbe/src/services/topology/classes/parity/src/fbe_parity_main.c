/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_parity_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main entry points for the parity object.
 *  This includes the create and destroy methods for the parity, as
 *  well as the event entry point, load and unload methods.
 * 
 * @ingroup parity_class_files
 * 
 * @version
 *   07/21/2009:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_parity.h"
#include "base_object_private.h"
#include "fbe_parity_private.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe_notification.h"
#include "fbe_raid_group_object.h"
#include "fbe_raid_group_needs_rebuild.h"
#include "fbe_private_space_layout.h"
/**************************
 *   FORWARD DECLARATIONS
 **************************/

/* Class methods forward declaration.
 */
fbe_status_t fbe_parity_load(void);
fbe_status_t fbe_parity_unload(void);
fbe_status_t fbe_parity_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_parity_destroy( fbe_object_handle_t object_handle);
fbe_status_t fbe_parity_destroy_interface( fbe_object_handle_t object_handle);
fbe_status_t fbe_parity_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p);
fbe_status_t fbe_parity_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p);


/* Export class methods.
 */
fbe_class_methods_t fbe_parity_class_methods = {FBE_CLASS_ID_PARITY,
                                                fbe_parity_load,
                                                fbe_parity_unload,
                                                fbe_parity_create_object,
                                                fbe_parity_destroy_interface,
                                                fbe_parity_control_entry,
                                                fbe_parity_event_entry,
                                                fbe_parity_io_entry,
                                                fbe_parity_monitor_entry};




/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/

/*!***************************************************************
 * fbe_parity_load()
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
fbe_parity_load(void)
{
    fbe_status_t status;
    /* Make sure our parity object fits inside a memory chunk.
     */
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_parity_t) <= FBE_MEMORY_CHUNK_SIZE);

    /* Make sure the monitor is sane. 
     * No sense trying to bring up any object lifecycle that is not valid.
     */ 
    status = fbe_parity_monitor_load_verify();

    if (status != FBE_STATUS_OK)
    {
        /* Panic - Indicates something wrong with the conditions and rotaries.
         * Logging CRITICAL_ERROR will take care of PANIC
         */
        fbe_topology_class_trace(FBE_CLASS_ID_PARITY,
                                 FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                                 "%s fbe_parity_monitor_load_verify failed with status 0x%x\n", 
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
    return status;
}
/* end fbe_parity_load() */

/*!***************************************************************
 * fbe_parity_unload()
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
fbe_parity_unload(void)
{
    fbe_raid_group_unload();
    return FBE_STATUS_OK;
}
/* end fbe_parity_unload() */

/*!***************************************************************
 * fbe_parity_create_object()
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
fbe_parity_create_object(fbe_packet_t * packet_p, 
                         fbe_object_handle_t * object_handle)
{
    fbe_parity_t *parity_p;
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
    parity_p = (fbe_parity_t *)fbe_base_handle_to_pointer(*object_handle);

    fbe_base_object_set_class_id((fbe_base_object_t *) parity_p, FBE_CLASS_ID_PARITY);

	fbe_base_config_send_specialize_notification((fbe_base_config_t *) parity_p);

    fbe_base_object_trace((fbe_base_object_t*)parity_p, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                          "parity create: %s Object: 0x%x Ptr: 0x%p\n", 
                          __FUNCTION__,
                          parity_p->raid_group.base_config.base_object.object_id,
                          parity_p);

    /* Force verify to make sure the object is properly initialized before scheduling it.
     * This avoids race condition between scheduler thread and usurper command trying to 
     * initialize the object rotary preset at the same time.
     */
    fbe_lifecycle_verify((fbe_lifecycle_const_t *)&fbe_parity_lifecycle_const, (fbe_base_object_t*)parity_p);

	/* Enable lifecycle tracing if requested */
	status = fbe_base_object_enable_lifecycle_debug_trace((fbe_base_object_t *)parity_p);

    /* Simply call our static init function to init members that have no
     * dependencies. 
     */
    return fbe_parity_init_members(parity_p);
}
/* end fbe_parity_create_object() */
/*!***************************************************************
 * fbe_parity_init_members()
 ****************************************************************
 * @brief
 *  This function initializes the parity object.
 *
 *  Note that everything that is created here must be
 *  destroyed within fbe_parity_destroy().
 *
 * @param parity_p - The parity object.
 *
 * @return
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  07/21/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_parity_init_members(fbe_parity_t * const parity_p)
{
    fbe_object_id_t object_id;
    fbe_base_object_get_object_id((fbe_base_object_t*)parity_p, &object_id);

    /* Make sure our edge array fits in a memory chunk.
     */
    if ((sizeof(fbe_block_edge_t) * FBE_PARITY_MAX_EDGES) > FBE_MEMORY_CHUNK_SIZE)
    {
        fbe_base_object_trace((fbe_base_object_t*)parity_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                              "%s parity block edge array will not fit inside a memory chunk! %d %llu\n", 
                              __FUNCTION__,
                              FBE_MEMORY_CHUNK_SIZE,
                              (unsigned long long)((sizeof(fbe_block_edge_t) * FBE_PARITY_MAX_EDGES)));
    }
    parity_p->block_edge_p = fbe_memory_native_allocate(sizeof(fbe_block_edge_t) * 16);

    if (parity_p->block_edge_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)parity_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                              "%s unable to allocate edge memory Object: 0x%x Ptr: 0x%p\n", 
                              __FUNCTION__,
                              parity_p->raid_group.base_config.base_object.object_id,
                              parity_p);
    }

    /* Let the raid group initialize. */
    fbe_raid_group_init(&parity_p->raid_group,
                        parity_p->block_edge_p,
                        (fbe_raid_common_state_t)fbe_parity_generate_start);

    /* Write_logging related */
    /* Make sure our write log info fits in a memory chunk.
     */
    if (sizeof(fbe_parity_write_log_info_t) > FBE_MEMORY_CHUNK_SIZE)
    {
        fbe_base_object_trace((fbe_base_object_t*)parity_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                              "%s parity write log info will not fit inside a memory chunk! %d %llu\n", 
                              __FUNCTION__,
                              FBE_MEMORY_CHUNK_SIZE,
                              (unsigned long long)(sizeof(fbe_parity_write_log_info_t)));
    }
    parity_p->write_log_info_p = fbe_memory_native_allocate(sizeof(fbe_parity_write_log_info_t));

    if (parity_p->write_log_info_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)parity_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                              "%s unable to allocate write log info Object: 0x%x Ptr: 0x%p\n", 
                              __FUNCTION__,
                              parity_p->raid_group.base_config.base_object.object_id,
                              parity_p);
    }


    /* Initialize write_logging info */
    fbe_parity_write_log_init(parity_p->write_log_info_p);

    /* Give pointer to raid_geomentry such that Raid state machines can access write_log */
    fbe_raid_geometry_set_write_logging_blob_p(&parity_p->raid_group.geo, parity_p->write_log_info_p);

    if (object_id == FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG)
    {
        fbe_raid_geometry_set_vault(&parity_p->raid_group.geo);

        /* Note that write_logging is not enabled for Vault RG */
        fbe_base_object_trace((fbe_base_object_t*)parity_p, 
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                              "%s Write_logging disabled for RG object: 0x%x Ptr: 0x%p\n", 
                              __FUNCTION__,
                              parity_p->raid_group.base_config.base_object.object_id,
                              parity_p);

    }
    else
    {
        /* Clear vault flag, which also enables write_logging for this RG */
        fbe_raid_geometry_clear_vault(&parity_p->raid_group.geo);
    }

    return FBE_STATUS_OK;
}
/* end fbe_parity_init_members() */

/*!***************************************************************
 * fbe_parity_destroy_interface()
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
fbe_parity_destroy_interface(fbe_object_handle_t object_handle)
{
    fbe_status_t status;
    //fbe_parity_t *parity_p = (fbe_parity_t*)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)fbe_base_handle_to_pointer(object_handle), 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                          "fbe_parity_main: %s entry\n", __FUNCTION__);

    /* Destroy this object.
     */
    fbe_parity_destroy(object_handle);
    
    /* Call parent destructor.
     */
    status = fbe_raid_group_destroy_interface(object_handle);
    return status;
}
/* end fbe_parity_destroy_interface */

/*!***************************************************************
 * fbe_parity_destroy()
 ****************************************************************
 * @brief
 *  This function is called to destroy the parity object.
 *
 *  This function tears down everything that was created in
 *  fbe_parity_init().
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
fbe_parity_destroy(fbe_object_handle_t object_handle)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_parity_t * parity_p;

    fbe_base_object_trace((fbe_base_object_t*)fbe_base_handle_to_pointer(object_handle), 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_CREATE_OBJECT, 
                          "fbe_parity_main: %s entry\n", __FUNCTION__);
    
    parity_p = (fbe_parity_t *)fbe_base_handle_to_pointer(object_handle);

    if (parity_p->block_edge_p != NULL)
    {
        fbe_memory_native_release(parity_p->block_edge_p);
        parity_p->block_edge_p = NULL;
    }
    if (parity_p->write_log_info_p != NULL)
    {
        fbe_parity_write_log_destroy(parity_p->write_log_info_p);
        fbe_memory_native_release(parity_p->write_log_info_p);
        parity_p->write_log_info_p = NULL;
    }

    return status;
}
/* end fbe_parity_destroy */

/*******************************
 * end fbe_parity_main.c
 *******************************/
