/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_mirror_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main entry points for the mirror object.
 *  This includes the create and destroy methods for the mirror, as
 *  well as the event entry point, load and unload methods.
 * 
 * @ingroup mirror_class_files
 * 
 * @version
 *   05/20/2009:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_mirror.h"
#include "base_object_private.h"
#include "fbe_mirror_private.h"
#include "fbe_raid_library.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe_notification.h"
#include "fbe_raid_group_needs_rebuild.h"
#include "fbe_private_space_layout.h"


/* Class methods forward declaration.
 */
fbe_status_t fbe_mirror_load(void);
fbe_status_t fbe_mirror_unload(void);
fbe_status_t fbe_mirror_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_mirror_destroy( fbe_object_handle_t object_handle);
fbe_status_t fbe_mirror_destroy_interface( fbe_object_handle_t object_handle);

fbe_status_t fbe_mirror_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p);
fbe_status_t fbe_mirror_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p);

/* Export class methods.
 */
fbe_class_methods_t fbe_mirror_class_methods = {FBE_CLASS_ID_MIRROR,
                                                fbe_mirror_load,
                                                fbe_mirror_unload,
                                                fbe_mirror_create_object,
                                                fbe_mirror_destroy_interface,
                                                fbe_mirror_control_entry,
                                                fbe_mirror_event_entry,
                                                fbe_mirror_io_entry,
                                                fbe_mirror_monitor_entry};

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/

/*!***************************************************************
 * fbe_mirror_load()
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
 *  05/20/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_mirror_load(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    /* At present we do not have any global data to construct.
     */
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_mirror_t) <= FBE_MEMORY_CHUNK_SIZE);
    /* Make sure the monitor is sane. 
     * No sense trying to bring up any object lifecycle that is not valid.
     */ 
    status = fbe_mirror_monitor_load_verify();

    if (status != FBE_STATUS_OK)
    {
        /* Panic - Indicates something wrong with the conditions and rotaries.
         * Logging CRITICAL_ERROR will take care of PANIC
         */
        fbe_topology_class_trace(FBE_CLASS_ID_MIRROR,
                                 FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                                 "%s fbe_mirror_monitor_load_verify failed with status 0x%x\n", 
                                 __FUNCTION__, status);
    }
    else
    {
        /* This currently just does compile time checks.  If that changes we should check for status here.
         */
        fbe_raid_group_load();
    }

    return FBE_STATUS_OK;
}
/* end fbe_mirror_load() */

/*!***************************************************************
 * fbe_mirror_unload()
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
 *  05/20/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_mirror_unload(void)
{
    /* Destroy any global data.
     */
    return FBE_STATUS_OK;
}
/* end fbe_mirror_unload() */

/*!***************************************************************
 * fbe_mirror_create_object()
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
 *  05/20/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_mirror_create_object(fbe_packet_t * packet_p, 
                         fbe_object_handle_t * object_handle)
{
    fbe_mirror_t *mirror_p;
    fbe_status_t status;

    /* Call the base class create function.
     */
    status = fbe_base_object_create_object(packet_p, object_handle);
    if(status != FBE_STATUS_OK) {
        return status;
    }

    /* Set class id.
     */
    mirror_p = (fbe_mirror_t *)fbe_base_handle_to_pointer(*object_handle);

    fbe_base_object_set_class_id((fbe_base_object_t *) mirror_p, FBE_CLASS_ID_MIRROR);

	fbe_base_config_send_specialize_notification((fbe_base_config_t *) mirror_p);

    fbe_base_object_trace((fbe_base_object_t*)mirror_p, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                          "mirror create: %s Object: 0x%x Ptr: 0x%p\n", 
                          __FUNCTION__,
                          mirror_p->raid_group.base_config.base_object.object_id,
                          mirror_p);

    /* Simply call our static init function to initialize the members.
     */
    status = fbe_mirror_init_members(mirror_p); 

    /* Force verify to make sure the object is properly initialized before scheduling it.
     * This avoids race condition between scheduler thread and usurper command trying to 
     * initialize the object rotary preset at the same time.
     */
    fbe_lifecycle_verify((fbe_lifecycle_const_t *)&fbe_mirror_lifecycle_const, (fbe_base_object_t*)mirror_p);

	/* Enable lifecycle tracing if requested */
	status = fbe_base_object_enable_lifecycle_debug_trace((fbe_base_object_t *)mirror_p);

    /* Now return the status.
     */
    return(status);
}
/* end fbe_mirror_create_object() */

/*!***************************************************************
 * fbe_mirror_init_members()
 ****************************************************************
 * @brief
 *  This function initializes the mirror object.
 *
 *  Note that everything that is created here must be
 *  destroyed within fbe_mirror_destroy().
 *
 * @param mirror_p - The mirror object.
 *
 * @return
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  05/20/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_mirror_init_members(fbe_mirror_t * const mirror_p)
{
    fbe_object_id_t object_id;
    fbe_raid_geometry_t *raid_geometry_p = NULL;

    fbe_base_object_get_object_id((fbe_base_object_t*)mirror_p, &object_id);
    
    raid_geometry_p = fbe_raid_group_get_raid_geometry((fbe_raid_group_t*)mirror_p);

    /* Storage for the block edge information is in the mirror object
     * itself.
     */
	//mirror_p->block_edge_p = mirror_p->block_edge_array;

    /* Let the raid group initialize.
     */
    fbe_raid_group_init(&mirror_p->raid_group,
                        mirror_p->block_edge_array, /* mirror_p->block_edge_p, */
                        (fbe_raid_common_state_t)fbe_mirror_generate_start);
    /* Let the mirror optimization structure initialize.
     */
    fbe_mirror_optimization_init( &(mirror_p->mirror_opt_db));

    ((fbe_raid_group_t *) mirror_p)->geo.mirror_optimization_p =
        (fbe_mirror_optimization_t *)&(mirror_p->mirror_opt_db);

    /* A raw mirror library needs the offset since its downstream edge does not 
     * have an edge offset.  The library has the offset on the downstream edge 
     * and it does not need an offset to be applied by the library. 
     */
    fbe_raid_geometry_set_raw_mirror_offset(raid_geometry_p, 0);
    fbe_raid_geometry_set_raw_mirror_rg_offset(raid_geometry_p, 0);

    if (object_id >= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_UTILITY_BOOT_VOLUME_SPA &&
        object_id <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_PRIMARY_BOOT_VOLUME_SPB)
    {
        fbe_raid_geometry_set_c4_mirror(raid_geometry_p);
    }
    
    return FBE_STATUS_OK;
}
/* end fbe_mirror_init_members() */

/*!***************************************************************
 * fbe_mirror_destroy_interface()
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
fbe_mirror_destroy_interface(fbe_object_handle_t object_handle)
{
    fbe_status_t status;

    fbe_base_object_trace((fbe_base_object_t*)fbe_base_handle_to_pointer(object_handle), 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                          "fbe_mirror_main: %s entry\n", __FUNCTION__);

    /* Destroy this object.
     */
    fbe_mirror_destroy(object_handle);
    
    /* Call parent destructor.
     */
    status = fbe_raid_group_destroy_interface(object_handle);

    return status;
}
/* end fbe_mirror_destroy_interface */

/*!***************************************************************
 * fbe_mirror_destroy()
 ****************************************************************
 * @brief
 *  This function is called to destroy the mirror object.
 *
 *  This function tears down everything that was created in
 *  fbe_mirror_init().
 *
 * @param object_handle - The object being destroyed.
 *
 * @return fbe_status_t
 *  STATUS of the destroy operation.
 *
 * @author
 *  05/20/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_mirror_destroy(fbe_object_handle_t object_handle)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_mirror_t * mirror_p;
    fbe_mirror_optimization_t * mirror_opt_p;

    fbe_base_object_trace((fbe_base_object_t*)fbe_base_handle_to_pointer(object_handle), 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_DESTROY_OBJECT, 
                          "fbe_mirror_main: %s entry\n", __FUNCTION__);
    
    mirror_p = (fbe_mirror_t *)fbe_base_handle_to_pointer(object_handle);
    mirror_opt_p = &mirror_p->mirror_opt_db;
    fbe_spinlock_destroy(&mirror_opt_p->spinlock);

    return status;
}
/* end fbe_mirror_destroy */

/*******************************
 * end fbe_mirror_main.c
 *******************************/
