/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_virtual_drive_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main entry points for the virtual_drive object.
 *  This includes the create and destroy methods for the virtual_drive, as
 *  well as the event entry point, load and unload methods.
 * 
 * @ingroup virtual_drive_class_files
 * 
 * @version
 *   05/20/2009:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_virtual_drive.h"
#include "base_object_private.h"
#include "fbe_virtual_drive_private.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe_transport_memory.h"
#include "fbe_notification.h"
#include "fbe_spare.h"
#include "fbe_raid_library.h"
#include "fbe_raid_geometry.h"
#include "fbe_cmi.h"

/*****************************************
 * GLOBAL DEFINITIONS
 *****************************************/
static fbe_virtual_drive_debug_flags_t fbe_virtual_drive_default_debug_flags = FBE_VIRTUAL_DRIVE_DEBUG_FLAG_DEFAULT;
static int fbe_virtual_drive_default_raid_group_debug_flags = 0;
static int fbe_virtual_drive_default_raid_library_debug_flags = 0;

/* Class methods forward declaration.
 */
fbe_status_t fbe_virtual_drive_load(void);
fbe_status_t fbe_virtual_drive_unload(void);
fbe_status_t fbe_virtual_drive_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_virtual_drive_destroy( fbe_object_handle_t object_handle);

fbe_status_t fbe_virtual_drive_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p);
fbe_status_t fbe_virtual_drive_metadata_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p);
fbe_status_t fbe_virtual_drive_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p);

fbe_status_t fbe_virtual_drive_optimal_set_condition_based_on_configuration_mode(fbe_virtual_drive_t * virtual_drive_p);
fbe_status_t fbe_virtual_drive_degraded_set_condition_based_on_configuration_mode(fbe_virtual_drive_t * virtual_drive_p);

fbe_status_t fbe_virtual_drive_degraded_mirror_set_condition_based_on_path_state(fbe_virtual_drive_t * virtual_drive_p);
fbe_status_t fbe_virtual_drive_degraded_pass_thru_set_condition_based_on_path_state(fbe_virtual_drive_t * virtual_drive_p);

/* Export class methods.
 */
fbe_class_methods_t fbe_virtual_drive_class_methods = {FBE_CLASS_ID_VIRTUAL_DRIVE,
                                                       fbe_virtual_drive_load,
                                                       fbe_virtual_drive_unload,
                                                       fbe_virtual_drive_create_object,
                                                       fbe_virtual_drive_destroy,
                                                       fbe_virtual_drive_control_entry,
                                                       fbe_virtual_drive_event_entry,
                                                       fbe_virtual_drive_io_entry,
                                                       fbe_virtual_drive_monitor_entry};

fbe_block_transport_const_t fbe_virtual_drive_block_transport_const = {fbe_virtual_drive_block_transport_entry,
                                                                       fbe_base_config_process_block_transport_event,
                                                                       fbe_virtual_drive_io_entry,
                                                                       NULL, NULL};

/* By default, the unused_drive_as_hot_spare init value is always true. 
*/
static fbe_bool_t init_unused_drive_as_spare_flag = FBE_TRUE;

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/

static fbe_status_t fbe_virtual_drive_init_members(fbe_virtual_drive_t * const virtual_drive_p);

fbe_status_t
fbe_virtual_drive_set_cond_based_on_downstream_edge_attributes(fbe_virtual_drive_t * virtual_drive_p);


/*!****************************************************************************
 * fbe_virtual_drive_load()
 ******************************************************************************
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
 ******************************************************************************/
fbe_status_t 
fbe_virtual_drive_load(void)
{
    /* Call the base class create function. */
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_virtual_drive_t) <= FBE_MEMORY_CHUNK_SIZE);
    fbe_virtual_drive_monitor_load_verify();

    /*set some default system spare configuration information. */
    fbe_virtual_drive_initialize_default_system_spare_config();
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_load()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_unload()
 ******************************************************************************
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
 ******************************************************************************/
fbe_status_t 
fbe_virtual_drive_unload(void)
{
    /* Destroy any global data.
     */
    fbe_virtual_drive_teardown_default_system_spare_config();

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_unload()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_create_object()
 ******************************************************************************
 * @brief
 *  This function is called to create an instance of this class.
 *
 * @param packet_p - The packet used to construct the object.  
 * @param object_handle - The object being created.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @note    This method only initializes the virtual drive object.
 *          Configuration of the virtual drive object is done at a
 *          later time.
 *
 * @author
 *  05/20/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_virtual_drive_create_object(fbe_packet_t * packet_p, 
                                fbe_object_handle_t * object_handle)
{
    fbe_virtual_drive_t *   virtual_drive_p;
    fbe_status_t            status;

    status = fbe_base_object_create_object(packet_p, object_handle);    
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Set class id.
     */
    virtual_drive_p = (fbe_virtual_drive_t *)fbe_base_handle_to_pointer(*object_handle);

    fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                          "virtual_drive create: %s Object: 0x%x Ptr: 0x%p\n", 
                          __FUNCTION__,
                          virtual_drive_p->spare.raid_group.base_config.base_object.object_id,
                          virtual_drive_p);

    /* Set class id.
     */
    fbe_base_object_set_class_id((fbe_base_object_t *) virtual_drive_p, FBE_CLASS_ID_VIRTUAL_DRIVE);

	fbe_base_config_send_specialize_notification((fbe_base_config_t *) virtual_drive_p);

    /* Simply call our static init function to init members that have no
     * dependencies. 
     */
    status = fbe_virtual_drive_init_members(virtual_drive_p);

    /* Force verify to make sure the object is properly initialized before scheduling it.
     * This avoids race condition between scheduler thread and usurper command trying to 
     * initialize the object rotary preset at the same time.
     */
    fbe_lifecycle_verify((fbe_lifecycle_const_t *)&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t *) virtual_drive_p);

    /* Enable lifecycle tracing if requested */
    status = fbe_base_object_enable_lifecycle_debug_trace((fbe_base_object_t *)virtual_drive_p);

    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_create_object()
 ******************************************************************************/

/*****************************************************************************
 *          fbe_virtual_drive_get_default_debug_flags()
 *****************************************************************************
 *
 * @brief   Return the setting of the default raid group debug flags that
 *          will be used for all newly created raid groups.
 *
 * @param   debug_flags_p - Pointer to value of default raid group debug flags 
 *
 * @return  None (always succeeds)
 *
 * @author
 *  03/15/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
void fbe_virtual_drive_get_default_debug_flags(fbe_virtual_drive_debug_flags_t *debug_flags_p)
{
    *debug_flags_p = fbe_virtual_drive_default_debug_flags;
    return;
}
/* end of fbe_virtual_drive_get_default_raid_group_debug_flags() */

/*****************************************************************************
 *          fbe_virtual_drive_set_default_debug_flags()
 *****************************************************************************
 *
 * @brief   Set the value to be used for the raid group debug flags for any
 *          newly created raid group.
 *
 * @param   new_default_raid_group_debug_flags - New raid group debug flags
 *
 * @return  status - Always FBE_STATUS_OK
 *
 * @author
 *  03/15/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_virtual_drive_set_default_debug_flags(fbe_virtual_drive_debug_flags_t new_default_debug_flags)
{
    fbe_virtual_drive_default_debug_flags = new_default_debug_flags;
    return(FBE_STATUS_OK);
}
/* end of fbe_virtual_drive_set_default_debug_flags() */

/*****************************************************************************
 *          fbe_virtual_drive_get_default_raid_group_debug_flags()
 *****************************************************************************
 *
 * @brief   Return the setting of the default raid group debug flags that
 *          will be used for all newly created raid groups.
 *
 * @param   debug_flags_p - Pointer to value of default raid group debug flags 
 *
 * @return  None (always succeeds)
 *
 * @author
 *  03/15/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
void fbe_virtual_drive_get_default_raid_group_debug_flags(fbe_raid_group_debug_flags_t *debug_flags_p)
{
    *debug_flags_p = fbe_virtual_drive_default_raid_group_debug_flags;
    return;
}
/* end of fbe_virtual_drive_get_default_raid_group_debug_flags() 

/*****************************************************************************
 *          fbe_virtual_drive_set_default_raid_group_debug_flags()
 *****************************************************************************
 *
 * @brief   Set the value to be used for the raid group debug flags for any
 *          newly created raid group.
 *
 * @param   new_default_raid_group_debug_flags - New raid group debug flags
 *
 * @return  status - Always FBE_STATUS_OK
 *
 * @author
 *  03/15/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_virtual_drive_set_default_raid_group_debug_flags(fbe_raid_group_debug_flags_t new_default_raid_group_debug_flags)
{
    fbe_virtual_drive_default_raid_group_debug_flags = new_default_raid_group_debug_flags;
    return(FBE_STATUS_OK);
}
/* end of fbe_virtual_drive_set_default_raid_group_debug_flags() */

/*****************************************************************************
 *          fbe_virtual_drive_get_default_raid_library_debug_flags()
 *****************************************************************************
 *
 * @brief   Return the setting of the default raid group debug flags that
 *          will be used for all newly created raid groups.
 *
 * @param   debug_flags_p - Pointer to value of default raid group debug flags 
 *
 * @return  None (always succeeds)
 *
 * @author
 *  03/15/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
void fbe_virtual_drive_get_default_raid_library_debug_flags(fbe_raid_library_debug_flags_t *debug_flags_p)
{
    *debug_flags_p = fbe_virtual_drive_default_raid_library_debug_flags;
    return;
}
/* end of fbe_virtual_drive_get_default_raid_library_debug_flags() 

/*****************************************************************************
 *          fbe_virtual_drive_set_default_raid_library_debug_flags()
 *****************************************************************************
 *
 * @brief   Set the value to be used for the raid group debug flags for any
 *          newly created raid group.
 *
 * @param   new_default_raid_group_debug_flags - New raid group debug flags
 *
 * @return  status - Always FBE_STATUS_OK
 *
 * @author
 *  03/15/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_virtual_drive_set_default_raid_library_debug_flags(fbe_raid_library_debug_flags_t new_default_raid_library_debug_flags)
{
    fbe_virtual_drive_default_raid_library_debug_flags = new_default_raid_library_debug_flags;
    return(FBE_STATUS_OK);
}
/* end of fbe_virtual_drive_set_default_raid_library_debug_flags() */

/*!****************************************************************************
 * fbe_virtual_drive_init_members()
 ******************************************************************************
 * @brief
 *  This function initializes the virtual_drive object.
 *
 *  Note that everything that is created here must be destroyed within 
 *  fbe_virtual_drive_destroy().
 *
 * @param virtual_drive_p - The virtual_drive object.
 *
 * @return
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @note  This method only initializes the virtual drive object. Configuration
 *  of the virtual drive object is done at a later time.
 * @author
 *  05/20/2009 - Created. RPF
 *
 ******************************************************************************/
static fbe_status_t 
fbe_virtual_drive_init_members(fbe_virtual_drive_t * const virtual_drive_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_virtual_drive_debug_flags_t debug_flags = 0;

    /* Simply use the mirror initialization to initialize the mirror and RAID
     * group class members, virtual drive object is derived from the mirror so
     * first we should initialize the parent class members.
     */
    fbe_mirror_init_members((fbe_mirror_t *) virtual_drive_p); 

    /* Initialze the virtual drive flags.
     */
    fbe_virtual_drive_init_flags(virtual_drive_p);

    /* Initialize the debug flags to the default value.
     */
    fbe_virtual_drive_get_default_debug_flags(&debug_flags);
    fbe_virtual_drive_init_debug_flags(virtual_drive_p, debug_flags);

    /* Initialize the `need replacement drive' start time.
     */
    fbe_virtual_drive_swap_init_need_replacement_drive_start_time(virtual_drive_p);

    /* Initialize the `drive fault' start time.
     */
    fbe_virtual_drive_swap_init_drive_fault_start_time(virtual_drive_p);

    /* Initialize the `need proactive copy' start time.
     */
    fbe_virtual_drive_swap_init_need_proactive_copy_start_time(virtual_drive_p);

    /* Initialize the health to max.
     */
    virtual_drive_p->proactive_copy_health = FBE_SPARE_PS_MAX_HEALTH;

    /* Virtual drive object is created first time and so we still do not know
     * the configuration parameters provided by user.
     */
    fbe_virtual_drive_set_configuration_mode(virtual_drive_p,
                                             FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN);
    fbe_virtual_drive_set_new_configuration_mode(virtual_drive_p,
                                                 FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN);

    /* Initialize the user copy request as invalid, this request type will be 
     * set externally user wants to start proactive copy operation.
     */
    fbe_virtual_drive_set_copy_request_type(virtual_drive_p, FBE_SPARE_SWAP_INVALID_COMMAND);

    /* Initialize the swap in and out edge index to invalid.
     */
    fbe_virtual_drive_clear_swap_in_edge_index(virtual_drive_p);
    fbe_virtual_drive_clear_swap_out_edge_index(virtual_drive_p);

    /* Initialize the pvd object id    
     */
    fbe_virtual_drive_init_orig_pvd_object_id(virtual_drive_p);

    /* Initialize the unused_drive_as_spare flag    
    */
    fbe_virtual_drive_init_unused_drive_as_spare_flag(virtual_drive_p, init_unused_drive_as_spare_flag);
    

    fbe_base_config_set_block_transport_const((fbe_base_config_t *)virtual_drive_p,
                                              &fbe_virtual_drive_block_transport_const);

    fbe_base_config_set_outstanding_io_max((fbe_base_config_t *) virtual_drive_p, 0);

    /* Set the default offset */
    status = fbe_base_config_set_default_offset((fbe_base_config_t *) virtual_drive_p);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /*power saving related stuff*/
    base_config_disable_object_power_save((fbe_base_config_t *) virtual_drive_p);/*disabled by default*/
    fbe_base_config_set_power_saving_idle_time((fbe_base_config_t *) virtual_drive_p, FBE_BASE_CONFIG_DEFAULT_IDLE_TIME_IN_SECONDS);/*act like PVD*/

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_init_members()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_destroy()
 ******************************************************************************
 * @brief
 *  This function is called to destroy the virtual_drive object.
 *
 *  This function tears down everything that was created in
 *  fbe_virtual_drive_init().
 *
 * @param object_handle - The object being destroyed.
 *
 * @return fbe_status_t
 *  STATUS of the destroy operation.
 *
 * @author
 *  05/20/2009 - Created. RPF
 *
 ******************************************************************************/
fbe_status_t
fbe_virtual_drive_destroy(fbe_object_handle_t object_handle)
{
    fbe_status_t            status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t*)fbe_base_handle_to_pointer(object_handle), 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_CREATE_OBJECT, 
                          "fbe_virtual_drive_main: %s entry\n", __FUNCTION__);
    
    /*! Invoke the method to destroy the mirror
     */
    status = fbe_mirror_destroy_interface(object_handle);

    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_destroy()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_main_determine_edge_health() 
/*!****************************************************************************
 * @brief
 *  This function will determine the health of all the edges.  First it will 
 *  get the path state counters contained in the object.  Then it will adjust
 *  them for any edge that is up (enabled) but marked NR or rb logging. Such
 *  an edge is technically "unavailable" when determining the raid object's state.
 *
 *  The edge counters will be adjusted if needed and then returned in the
 *  output parameter. 
 *
 * @param virtual_drive_p               - pointer to a virtual drive object
 * @param out_edge_state_counters_p     - pointer to structure that gets populated
 *                                        with counters indicating the states of the
 *                                        edges 
 *
 * @return fbe_status_t   
 *
 * @author
 *   02/04/2011 - Created. Dhaval Patel (Derived from RAID object).
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_main_determine_edge_health(fbe_virtual_drive_t * virtual_drive_p,
                                                          fbe_base_config_path_state_counters_t *  edge_state_counters_p)
{

    fbe_base_config_t*                      base_config_p;              // pointer to a base config object
    fbe_block_edge_t*                       edge_p;                     // pointer to an edge 
    fbe_u32_t                               edge_index;                 // index to the current edge
    fbe_path_state_t                        path_state;                 // path state of the edge 
    fbe_bool_t                              b_is_rb_logging;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;         // configuration mode  
    fbe_bool_t                              b_is_copy_complete = FBE_FALSE;

    /* Trace function entry  */
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY((fbe_raid_group_t *) virtual_drive_p);

    /* Set up a pointer to the object as a base config object */
    base_config_p = (fbe_base_config_t *) virtual_drive_p; 

    /* Get the path counters, which contain the number of edges enabled, disabled, and broken */
    fbe_base_config_get_path_state_counters(base_config_p, edge_state_counters_p);

    /* Get the configuration mode of the virtual drive object. */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    /* Invoke method that determines if copy is complete or not.*/
    if ((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)  ||
        (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE)    )
    {
        b_is_copy_complete = fbe_virtual_drive_is_copy_complete(virtual_drive_p); 
    }

    /* Loop through all the edges to find if any are enabled but marked NR, an edge that 
     * is enabled but marked NR is "unhealthy" and need to be considered broken when 
     * determining the raid object's health.
     */
    for (edge_index = 0; edge_index < base_config_p->width; edge_index++)
    {
        /* Get a pointer to the next edge */
        fbe_base_config_get_block_edge(base_config_p, &edge_p, edge_index);

        /* Get the edge's path state */
         fbe_block_transport_get_path_state(edge_p, &path_state);

        /* If the edge is anything other than enabled, we don't need to do anything more - 
         *  move on to the next edge
         */
        if (path_state != FBE_PATH_STATE_ENABLED) 
        {
            continue;
        }

        /* If configuration mode is pass thru then we do not need to consider the rb logging or
         * checkpoint of the enabled edge.
         */
        if ((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE)  ||
            (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE)    )
        {
            continue;
        }

        /* The edge is rebuilding logging and not copy complete.
         */
        fbe_raid_group_get_rb_logging((fbe_raid_group_t *) virtual_drive_p, edge_index, &b_is_rb_logging);
        if ((b_is_copy_complete == FBE_FALSE) &&
            (b_is_rb_logging == FBE_TRUE)        )
        {
            /* The edge is marked NR/rb logging so it is unavailable.  Change the 
             * edge state counters accordingly. 
             */
            edge_state_counters_p->broken_counter++;
            edge_state_counters_p->enabled_counter--;
            continue; 
        }
    }        

    return FBE_STATUS_OK;

}
/******************************************************************************
 * end fbe_virtual_drive_main_determine_edge_health()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_verify_downstream_health()
 ******************************************************************************
 * @brief
 *  This function is used to verify the downstream health of the virtual drive
 *  object.
 *
 * @param virtual_drive_p - Virtual Drive object.
 *
 * @return fbe_base_config_downstream_health_state_t - It will return any of 
 *  following downstream health state.
 * 
 *  1) FBE_DOWNSTREAM_HEALTH_OPTIMAL:  both downstream edges are in ENABLE
 *                                     state.
 *  2) FBE_DOWNSTREAM_HEALTH_DISABLED: One of the downstream edge is in DISABLE
 *                                     state.
 *  3) FBE_DOWNSTREAM_HEALTH_DEGRADED: One of the downstream edge is in BRK/INV
 *                                     state.
 *  4) FBE_DOWNSTREAM_HEALTH_BROKEN:   Both the downstream edges are in BRK/INV
 *                                     state.
 *
 * @author
 *  8/14/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_base_config_downstream_health_state_t fbe_virtual_drive_verify_downstream_health(fbe_virtual_drive_t * virtual_drive_p)
{

    fbe_base_config_downstream_health_state_t   downstream_health_state;
    fbe_base_config_path_state_counters_t       path_state_counters;
    fbe_u32_t                                   width;
    fbe_virtual_drive_configuration_mode_t      configuration_mode;

    fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    fbe_base_config_get_width((fbe_base_config_t *) virtual_drive_p, &width);

    /* Get the configuration mode of the virtual drive object. */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    /* Determine health state of the downstream edges. */
    fbe_virtual_drive_main_determine_edge_health(virtual_drive_p, &path_state_counters);

    /* Virtual drive downstream health state is same as mirror downstream health state except
     * disabled counters, virtual drive waits for the disabled state transition to any other
     * permanent state before it sets any other condition (other than disable condition).
     */
    switch(configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            /* If both edges are enabled then we are optimal.  Otherwise wait
             * for other edge to either be invalid or broken.
             */
            if (path_state_counters.enabled_counter == width)
            {
                /* We are `optimal'.
                 */
                downstream_health_state = FBE_DOWNSTREAM_HEALTH_OPTIMAL;
            }
            else if (path_state_counters.enabled_counter > 0)
            {
                /* Else we have at least (1) edge that is enabled so we are
                 * degraded.
                 */
                downstream_health_state = FBE_DOWNSTREAM_HEALTH_DEGRADED;
            }
            else if (path_state_counters.disabled_counter > 0)
            { 
                /* else one of the downstream edge is in disable state, 
                 * set the health as disabled. 
                 */
                downstream_health_state = FBE_DOWNSTREAM_HEALTH_DISABLED;
            }
            else  
            {
                /* Else all other cases consider has broken health. 
                 */
                downstream_health_state = FBE_DOWNSTREAM_HEALTH_BROKEN;
            }
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            /* In pass-thru mode we should never be `optimal'
             * We need to evaluate enabled first since it is possible to be 
             * in a pass through state with multiple edges before config mode is set. 
             */
            if (path_state_counters.enabled_counter > 0)
            {  
                /* else one of the downstream edge is in brk/invalid state, 
                 * set the health as degraded. 
                 */
                if (fbe_virtual_drive_swap_is_primary_edge_broken(virtual_drive_p))
                {
                    downstream_health_state = FBE_DOWNSTREAM_HEALTH_BROKEN;
                }
                else
                {
                    downstream_health_state = FBE_DOWNSTREAM_HEALTH_DEGRADED;
                }
            }
            else if (path_state_counters.disabled_counter > 0)
            { 
                /* one of the downstream edge is in disable state, 
                 * set the health as disabled. 
                 */
                downstream_health_state = FBE_DOWNSTREAM_HEALTH_DISABLED;
            }
            else  
            {
                /* else all other cases consider has broken health. 
                 */
                downstream_health_state = FBE_DOWNSTREAM_HEALTH_BROKEN;
            }
            break;

        default:
            /* Unsupported configuration mode.  Mark health broken.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Unsupported mode: %d \n",
                                  __FUNCTION__, configuration_mode);
            downstream_health_state = FBE_DOWNSTREAM_HEALTH_BROKEN;
            break;

    } /* end set downstream health based on confiiguration mode and path sate counters */

    /* Trace some information if enabled.
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_HEALTH_TRACING,
                            "virtual_drive: verify downstream health mode: %d health: %d counts enb: %d dis: %d broke: %d inv: %d \n",
                            configuration_mode, downstream_health_state, 
                            path_state_counters.enabled_counter, path_state_counters.disabled_counter, 
                            path_state_counters.broken_counter, path_state_counters.invalid_counter);

    return downstream_health_state;
}
/******************************************************************************
 * end fbe_virtual_drive_verify_downstream_health()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_set_condition_based_on_downstream_health()
 ******************************************************************************
 * @brief
 *  This function will set the different condition based on the health of the
 *  communication path with its downstream obhects.
 *
 * @param virtual_drive_p - Virtual Drive object.
 * @param downstream_health_state - Health state of the dowstream object
 *                                  communication path.
 * @param caller - Routine which invoked this method
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  8/14/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_virtual_drive_set_condition_based_on_downstream_health(fbe_virtual_drive_t *virtual_drive_p, 
                                                           fbe_base_config_downstream_health_state_t downstream_health_state,
                                                           const char *caller)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_bool_t                              b_is_active_sp;
    fbe_bool_t                              b_is_peer_alive;

    /* Get the configuration mode
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    /* Trace some information if enabled
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_SWAP_TRACING,
                            "virtual_drive: set cond by health: %d mode: %d caller: %-50s\n",
                            downstream_health_state, configuration_mode, caller);

    /* Set any required conditions based on:
     *  o Configuration mode
     *  o Downstream health
     */
    switch (downstream_health_state)
    {
        case FBE_DOWNSTREAM_HEALTH_OPTIMAL:
             /* If downstream health is in optimal state then set the different condition based on
              * current configuration mode of the virtual drive object.
              */
            status = fbe_virtual_drive_optimal_set_condition_based_on_configuration_mode(virtual_drive_p);
            break;
        case FBE_DOWNSTREAM_HEALTH_DEGRADED:
             /* if downstream health is in degraded state then set the different condition based on
              * current configuration mode of the virtual drive object.
              */
            status = fbe_virtual_drive_degraded_set_condition_based_on_configuration_mode(virtual_drive_p);
            break;
        case FBE_DOWNSTREAM_HEALTH_BROKEN:
             /*! @note If the source drive is removed we will enter the failed
              *        condition.  But we will wait for it ti come back.
              */
            status = fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                            (fbe_base_object_t *) virtual_drive_p,
                                            FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);
            break;
        case FBE_DOWNSTREAM_HEALTH_DISABLED:
            /* If any of the downstream edge with VD object is in DISABLED state then set 
             * the health disabled condition (drain I/O).
             */
            status = fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                            (fbe_base_object_t*) virtual_drive_p,
                                            FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_DISABLED);
            break;
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Invalid downstream health state: 0x%x!!\n", 
                                  __FUNCTION__,
                                  downstream_health_state);
            break;
    }

    /* If we are healthy and we are not active and the peer is down
     * reschedule.  This is needed for the case where an event occurs
     * when the active SP dies but the object has not yet been marked
     * active on this SP.  We must reschedule until we become active.
     */
    b_is_active_sp = fbe_base_config_is_active((fbe_base_config_t *)virtual_drive_p);
    b_is_peer_alive = fbe_cmi_is_peer_alive();
    if (((downstream_health_state == FBE_DOWNSTREAM_HEALTH_OPTIMAL)  ||
         (downstream_health_state == FBE_DOWNSTREAM_HEALTH_DEGRADED)    ) &&
        (b_is_active_sp == FBE_FALSE)                                     &&
        (b_is_peer_alive == FBE_FALSE)                                       )
    {
        /* Reevaluate the downstream health after a short delay.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: set cond by health: %d mode: %d active: %d peer alive: %d reschedule\n",
                              downstream_health_state, configuration_mode,
                              b_is_active_sp, b_is_peer_alive);

        /* Wait the minimum interval (1 second) an check again.
         */ 
        fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t *)virtual_drive_p,
                               FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_EVALUATE_DOWNSTREAM_HEALTH);
        fbe_lifecycle_reschedule(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t *)virtual_drive_p, 1000);
    }

    /* Return the status.
     */
    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_set_condition_based_on_downstream_health()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_optimal_set_condition_based_on_configuration_mode()
 ******************************************************************************
 * @brief
 *  This function is used to set the different condition in optmial mode based
 *  on the current configuration mode of the virtual drive object.
 * 
 * @param virtual_drive_p - Virtual Drive object.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  10/22/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_virtual_drive_optimal_set_condition_based_on_configuration_mode(fbe_virtual_drive_t * virtual_drive_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_lifecycle_state_t                   my_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_lifecycle_cond_id_t                 cond_id = 0;
    fbe_bool_t                              b_is_copy_complete = FBE_FALSE;
    fbe_bool_t                              b_is_active = FBE_TRUE;
    fbe_bool_t                              b_destination_healthy = FBE_TRUE;

    fbe_base_object_trace((fbe_base_object_t *) virtual_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /*! @note Most of the conditions below are only valid when in the Ready state.
     */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *) virtual_drive_p, &my_state);
    b_is_copy_complete = fbe_virtual_drive_is_copy_complete(virtual_drive_p);
    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)virtual_drive_p);

    /* Get the path attributes to determine if the destination is healthy
     * (i.e. does not have EOL set) or not.  If the destination drive
     * is not healthy AND we have not completed the copy, abort the
     * copy operation (swap out the destination drive).
     */
    b_destination_healthy = fbe_virtual_drive_swap_is_secondary_edge_healthy(virtual_drive_p);

    /*! @note Most of the conditions below are only valid when in the Ready state.
     */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *) virtual_drive_p, &my_state);

    /* Get the configuration mode of the virtual drive object. 
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    switch(configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            /*! @note If the `needs replacement drive' flag is set, set the 
             *        condition that will check the health and clear the needs 
             *        replacement flag.
             *
             *  @note We cannot modify the flag directly since this method
             *        is invoked in a different context (the event context vs
             *        the monitor context where it is normally modified). 
             */
            if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_NEED_REPLACEMENT_DRIVE))
            {
                status = fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                            (fbe_base_object_t *)virtual_drive_p,
                                            FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_NEED_REPLACEMENT_DRIVE);
                cond_id = FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_NEED_REPLACEMENT_DRIVE;
            }
            
            /* If the copy is complete set the condition to complete the copy
             * operation.
             */
            if ( (b_is_active == FBE_TRUE)                                                                       &&
                 (b_is_copy_complete == FBE_TRUE)                                                                &&
                !fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_OPTIMAL_COMPLETE_COPY)    &&
                !fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS)    )
            {
                /* Swap-out an appropriate edge if virtual drive object is in 
                 * fully rebuilt mirror state. 
                 */
                fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_OPTIMAL_COMPLETE_COPY);
                status = fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                            (fbe_base_object_t *) virtual_drive_p,
                                            FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_IS_COPY_COMPLETE);
                cond_id = FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_IS_COPY_COMPLETE;
            }

            /* If EOL is set on the destination drive and the copy is not
             * complete abort the copy operation and swap-out the destination
             * drive.
             */
            if ((b_is_active == FBE_TRUE)            &&
                (b_is_copy_complete == FBE_FALSE)    &&
                (b_destination_healthy == FBE_FALSE)    )            
            {
                /* Swap-out an appropriate edge if virtual drive is not healthy.
                 */
                status = fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                            (fbe_base_object_t *) virtual_drive_p,
                                            FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_IS_COPY_COMPLETE);
                cond_id = FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_IS_COPY_COMPLETE;
            }
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            /*! @note If the `needs replacement drive' flag is set, set the 
             *        condition that will check the health and clear the needs 
             *        replacement flag.
             *
             *  @note We cannot modify the flag directly since this method
             *        is invoked in a different context (the event context vs
             *        the monitor context where it is normally modified). 
             */
            if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_NEED_REPLACEMENT_DRIVE))
            {
                status = fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                            (fbe_base_object_t *)virtual_drive_p,
                                            FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_NEED_REPLACEMENT_DRIVE);
                cond_id = FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_NEED_REPLACEMENT_DRIVE;
            }

            /* We can have optimal downstream health only if both downstream edges are in ENABLE state
             * and both have good data on it, if we are in pass thru mode then we need to set the 
             * condition to swap-out the appropriate edge.
             */
            if ( (b_is_active == FBE_TRUE)                                                                       &&
                !fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS)    )
            {
                /*! @todo We need a `swap-out' edge sparing command.
                 */
                status = fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                            (fbe_base_object_t *) virtual_drive_p,
                                            FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_SWAP_OUT_EDGE);
                cond_id = FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_SWAP_OUT_EDGE;
            }
            break;

        default:
            /* return error. */
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If enabled print some debug information.
     */
    if (cond_id != 0)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: optimal mode: %d life: %d active: %d copy complete: %d cond: 0x%x \n",
                              configuration_mode, my_state, b_is_active, b_is_copy_complete, cond_id);
    }
    else
    {   
        fbe_virtual_drive_trace(virtual_drive_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_VIRTUAL_DRIVE_DEBUG_FLAG_SWAP_TRACING,
                                "virtual_drive: optimal mode: %d life: %d active: %d copy complete: %d cond: 0x%x \n",
                                configuration_mode, my_state, b_is_active, b_is_copy_complete, cond_id);
    }

    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_optimal_set_condition_based_on_configuration_mode()
*******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_degraded_set_condition_based_on_configuration_mode()
 ******************************************************************************
 * @brief
 *  This function is used to set the different condition in degraded mode based
 *  on the current configuration mode of the virtual drive object.
 * 
 * @param virtual_drive_p - Virtual Drive object.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  10/22/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_virtual_drive_degraded_set_condition_based_on_configuration_mode(fbe_virtual_drive_t * virtual_drive_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;

    fbe_base_object_trace((fbe_base_object_t *) virtual_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Get the configuration mode of the virtual drive object. */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    switch(configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            /* set the appropriate condition for the degraded mirror state. */
            status = fbe_virtual_drive_degraded_mirror_set_condition_based_on_path_state(virtual_drive_p);
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            /* set the different condition based on path state of downstream object. */
            status = fbe_virtual_drive_degraded_pass_thru_set_condition_based_on_path_state(virtual_drive_p);
            break;            
        default:
            return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_degraded_set_condition_based_on_configuration_mode()
*******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_degraded_mirror_set_condition_based_on_path_state()
 ******************************************************************************
 * @brief
 *  This function is used to set the different condition in degraded mode based
 *  on the current configuration mode of the virtual drive object.
 * 
 * @param virtual_drive_p - Virtual Drive object.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  10/22/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_degraded_mirror_set_condition_based_on_path_state(fbe_virtual_drive_t * virtual_drive_p)
{
    fbe_path_state_t        first_edge_path_state;
    fbe_path_state_t        second_edge_path_state;
    fbe_path_attr_t         first_edge_path_attr;
    fbe_path_attr_t         second_edge_path_attr;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_lifecycle_state_t   my_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_lifecycle_cond_id_t cond_id = 0;
    fbe_virtual_drive_configuration_mode_t configuration_mode;
    fbe_bool_t              b_is_destination_failed = FBE_FALSE;

    /*! @note Most of the conditions below are only valid when in the Ready state.
     */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *) virtual_drive_p, &my_state);

    /* Get the path state and path attributes of both the downstream edges. */
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                       &first_edge_path_state);
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                       &second_edge_path_state);
    fbe_block_transport_get_path_attributes(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                            &first_edge_path_attr);
    fbe_block_transport_get_path_attributes(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                            &second_edge_path_attr);

    /* Determine our configuration mode.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    switch (configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
            if (second_edge_path_state != FBE_PATH_STATE_ENABLED)
            {
                b_is_destination_failed = FBE_TRUE;
            }
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            if (first_edge_path_state != FBE_PATH_STATE_ENABLED)
            {
                b_is_destination_failed = FBE_TRUE;
            }
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
        default:
            /* This method should only be invoke if we are in mirror mode.
             */
            fbe_base_object_trace((fbe_base_object_t *) virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Unexpected mode: %d\n", 
                                  __FUNCTION__, configuration_mode);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* if both downstream edges are enabled then resume the rebuild since we are in degraded mode. 
     */
    if ((first_edge_path_state == FBE_PATH_STATE_ENABLED)  &&
        (second_edge_path_state == FBE_PATH_STATE_ENABLED)    )
    {
        /* If both edges are marked End-Of-Life we need to fail the copy
         * operation.
         */
        if ((first_edge_path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE)  &&
            (second_edge_path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE)    )
        {
            /* Set the condition to abort the copy operation.
             */
            status = fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                            (fbe_base_object_t *) virtual_drive_p,
                                            FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_ABORT_COPY);
            cond_id = FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_ABORT_COPY;
        }
        else
        {    
            /* Set the condition to `mark needs rebuild' (if required).
            */
            status = fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                            (fbe_base_object_t *) virtual_drive_p,
                                            FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_EVAL_MARK_NR_IF_NEEDED_COND);
            cond_id = FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_EVAL_MARK_NR_IF_NEEDED_COND;
        }
    }
    else
    {
        /* Else the virtual drive is in mirror mode which means that we could
         * I/Os waiting in the mirror library for shutdown/continue.  Set the
         * eval rebuild logging condition which will:
         *  o Quiesce I/Os  
         *  o Evaluate the downstream health/needs rebuild
         *  o Unquiesce I/Os
         *  o Restart I/Os
         */
        /*! @note If the `needs replacement drive' flag is set, set the 
         *        condition that will check the health and clear the needs 
         *        replacement flag.
         *
         *  @note We cannot modify the flag directly since this method
         *        is invoked in a different context (the event context vs
         *        the monitor context where it is normally modified). 
         */
        if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_NEED_REPLACEMENT_DRIVE))
        {
            status = fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                        (fbe_base_object_t *)virtual_drive_p,
                                        FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_NEED_REPLACEMENT_DRIVE);
            cond_id = FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_NEED_REPLACEMENT_DRIVE;
        }

        /* If the destination drive failed set the condition to check needs 
         * replacement drive.
         */
        if (b_is_destination_failed == FBE_TRUE)
        {
            /* Set the condition that will determine if we need to wait before
             * swapping out the failed drive (and thus changing the mode to pass-thru).
             */
            status = fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                            (fbe_base_object_t *)virtual_drive_p,
                                            FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_NEED_REPLACEMENT_DRIVE);
            cond_id = FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_NEED_REPLACEMENT_DRIVE;
        }

        /* Set the condition to evaluate rebuild logging which will:
         *  o Source drive failed - Transition the virtual drive to failed.
         *  o Destination drive failed - Stay in Ready and eval rl.
         *  o Both drives - Transition the virtual drive to failed.
         */
        status = fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                        (fbe_base_object_t *)virtual_drive_p,
                                        FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_RB_LOGGING);
        cond_id = FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_RB_LOGGING;
        
        /* We are immediately rescheduled since we set at least (1) condition.
         */
    }

    /* When in mirror mode we always display the information.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: degraded mirror life: %d cond: 0x%x path state:[%d-%d] attr:[0x%x-0x%x]\n",
                          my_state, cond_id, first_edge_path_state, second_edge_path_state,
                          first_edge_path_attr, second_edge_path_attr);

    /* print the error message if set condition failed. */
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s set condition failed with status 0x%x, \n", __FUNCTION__, status);
    }

    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_degraded_mirror_set_condition_based_on_path_state()
*******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_degraded_pass_thru_set_condition_based_on_path_state()
 ******************************************************************************
 * @brief
 *  This function is used to set the different condition with pass thru 
 *  configuration mode and degraded downstream health.
 * 
 * @param virtual_drive_p - Virtual Drive object.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  10/22/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_degraded_pass_thru_set_condition_based_on_path_state(fbe_virtual_drive_t *virtual_drive_p)
{
    fbe_path_state_t        first_edge_path_state;
    fbe_path_state_t        second_edge_path_state;
    fbe_path_attr_t         first_edge_path_attr;
    fbe_path_attr_t         second_edge_path_attr;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_bool_t              b_does_virtual_drive_require_proactive_sparing = FBE_FALSE;
    fbe_edge_index_t        swap_out_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_lifecycle_state_t   my_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_lifecycle_cond_id_t cond_id = 0;
    fbe_bool_t              b_no_spare_reported = FBE_FALSE;
    fbe_virtual_drive_configuration_mode_t  configuration_mode; 

    /* Get the configuration mode.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    /* Get the path state and path attributes of both the downstream edges. */
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                       &first_edge_path_state);
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                       &second_edge_path_state);
    fbe_block_transport_get_path_attributes(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                            &first_edge_path_attr);
    fbe_block_transport_get_path_attributes(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                            &second_edge_path_attr);

    /*! @note Re-evaludate the health when we are in the Ready state.
     */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *) virtual_drive_p, &my_state);
    if (my_state != FBE_LIFECYCLE_STATE_READY) 
    {
        /* Set the condition to evaluate downstream health when the virtual 
         * drive becomes ready.
         */
        status = fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                        (fbe_base_object_t *)virtual_drive_p,
                                        FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_EVALUATE_DOWNSTREAM_HEALTH);
        cond_id = FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_EVALUATE_DOWNSTREAM_HEALTH;

        /* Trace some information if enabled.
         */
        fbe_virtual_drive_trace(virtual_drive_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_VIRTUAL_DRIVE_DEBUG_FLAG_CONFIG_TRACING,
                                "virtual_drive: degraded pass-thru not ready: %d cond: 0x%x path state:[%d-%d] attr:[0x%x-0x%x]\n",
                                my_state, cond_id,
                                first_edge_path_state, second_edge_path_state,
                                first_edge_path_attr, second_edge_path_attr);

        return FBE_STATUS_OK;
    }

    /* If we are pass-thru mode set the neccessary conditions.
     */
    if (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE)
    {
        /* Pass-thru first edge.  If the edge is `healthy' check if proactive
         * copy is required (we don't care about the second edge state).
         */
        if (first_edge_path_state == FBE_PATH_STATE_ENABLED)
        {
            /* set the swap-in proactive spare condition if needed. */
            fbe_virtual_drive_swap_check_if_proactive_spare_needed(virtual_drive_p, 
                                                                   &b_does_virtual_drive_require_proactive_sparing,
                                                                   &swap_out_edge_index);
            if (b_does_virtual_drive_require_proactive_sparing)
            {
                /* set the condition to swap-in proactive spare. */
                status = fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                            (fbe_base_object_t *) virtual_drive_p,
                                            FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_NEED_PROACTIVE_SPARE);
                cond_id = FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_NEED_PROACTIVE_SPARE;
            }
        }
        else
        {
            /* Set the condition to evaluate the extent health (evaluate 
             * rebuild logging).
             */
            status = fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                        (fbe_base_object_t *) virtual_drive_p,
                                        FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_RB_LOGGING);
            cond_id = FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_RB_LOGGING;
        }
    }
    else if (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE)
    {
        /* Pass-thru second edge.  If the edge is `healthy' check if proactive
         * copy is required (we don't care about the second edge state).
         */
        if (second_edge_path_state == FBE_PATH_STATE_ENABLED)
        {
            /* set the swap-in proactive spare condition if needed. */
            fbe_virtual_drive_swap_check_if_proactive_spare_needed(virtual_drive_p, 
                                                                   &b_does_virtual_drive_require_proactive_sparing,
                                                                   &swap_out_edge_index);
            if (b_does_virtual_drive_require_proactive_sparing)
            {
                /* set the condition to swap-in proactive spare. */
                status = fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                            (fbe_base_object_t *)virtual_drive_p,
                                            FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_NEED_PROACTIVE_SPARE);
                cond_id = FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_NEED_PROACTIVE_SPARE;
            }
        }
        else
        {
            /* Set the condition to evaluate the extent health (evaluate 
             * rebuild logging).
             */
            status = fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                        (fbe_base_object_t *) virtual_drive_p,
                                        FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_RB_LOGGING);
            cond_id = FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_RB_LOGGING;
        }
    }

    /* If the `no spare reported' flag is set, we should clear it now.
     */
    fbe_virtual_drive_metadata_has_no_spare_been_reported(virtual_drive_p, &b_no_spare_reported);
    if (b_no_spare_reported == FBE_TRUE)
    {
        /* We need to clear the `no spare reported' non-page flag.
         */
        status = fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                        (fbe_base_object_t *)virtual_drive_p,
                                        FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_CLEAR_NO_SPARE_REPORTED);
        if (status != FBE_STATUS_OK)
        {
            /* Report an error but continue.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s set condition failed with status 0x%x, \n", __FUNCTION__, status);
        }
        if (cond_id == 0)
        {
            cond_id = FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_CLEAR_NO_SPARE_REPORTED;
        }
    }

    /* Print some debug information.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: degraded pass-thru mode: %d life: %d cond: 0x%x path state:[%d-%d] attr:[0x%x-0x%x]\n",
                          configuration_mode, my_state, cond_id,
                          first_edge_path_state, second_edge_path_state,
                          first_edge_path_attr, second_edge_path_attr);

    /* print the error message if set condition failed. */
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s set condition failed with status 0x%x, \n", __FUNCTION__, status);
    }
    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_optimal_pass_thru_set_condition_based_on_path_state()
*******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_set_raid_debug_flags()
 ******************************************************************************
 * @brief
 *  This function is used to either set or clear the raid group and raid 
 *  library flags.
 *
 * @note (ATTENTION)
 *  Never call this routine without quiescing i/o on both the SPs.
 *
 * @param virtual_drive_p - Pointer to virtual drive to set debug flags for.
 * @param b_set_flags - FBE_TRUE - Set the raid group/library debug flags
 *                      FBE_FALSE - Clear the raid group/library debug flags
 *
 * @note    Assumes that the virtual drive lock is taken already.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_set_raid_debug_flags(fbe_virtual_drive_t *virtual_drive_p,
                                                           fbe_bool_t b_set_flags)
{
    fbe_raid_group_debug_flags_t    default_raid_group_debug_flags = 0;
    fbe_raid_library_debug_flags_t  default_raid_library_debug_flags = 0;
    fbe_raid_group_t               *raid_group_p = (fbe_raid_group_t *)virtual_drive_p;

    /* First get the raid group debug before bothering to change them.
     */
    fbe_virtual_drive_get_default_raid_group_debug_flags(&default_raid_group_debug_flags);
    if (default_raid_group_debug_flags != 0)
    {
        /* Now either set or clear the flags in the raid object.
         */
        if (b_set_flags == FBE_TRUE)
        {
            fbe_raid_group_set_debug_flags(raid_group_p, default_raid_group_debug_flags);
        }
        else
        {
            fbe_raid_group_init_debug_flags(raid_group_p, 0);
        }
    }
    fbe_virtual_drive_get_default_raid_library_debug_flags(&default_raid_library_debug_flags);
    if (default_raid_library_debug_flags != 0)
    {
        fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

        /* Now either set or clear the flags in the raid object.
         */
        if (b_set_flags == FBE_TRUE)
        {
            fbe_raid_geometry_set_debug_flags(raid_geometry_p, default_raid_library_debug_flags); 
        }
        else
        {
            fbe_raid_geometry_init_debug_flags(raid_geometry_p, 0);
        }
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_set_raid_debug_flags()
*******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_set_configuration_mode()
 ******************************************************************************
 * @brief
 *  This function is used to set the configuration mode of the virtual drive
 *  object.
 *
 * @note (ATTENTION)
 *  Never call this routine without quiescing i/o on both the SPs.

 * @param virtual_drive_p    - Virtual drive object.
 * @param configuration_mode - Configuration mode of the virtual.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 ******************************************************************************/
fbe_status_t
fbe_virtual_drive_set_configuration_mode(fbe_virtual_drive_t *virtual_drive_p,
                                         fbe_virtual_drive_configuration_mode_t configuration_mode)
{
    fbe_raid_geometry_t * raid_geometry_p = NULL;
    fbe_virtual_drive_debug_flags_t virtual_drive_debug_flags = 0;

    /* update the configuration mode in memory.*/
    virtual_drive_p->configuration_mode = configuration_mode;
    fbe_virtual_drive_get_default_debug_flags(&virtual_drive_debug_flags);

    /* get the geometry pointer. */
    raid_geometry_p = fbe_raid_group_get_raid_geometry((fbe_raid_group_t *) virtual_drive_p);

    /*!@note Whenever we set the configuration mode to mirror update the prefered
     * position in raid geometry information appropriately.
     */
    if(configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)
    {
        /* incase of sparing from edge 1 - edge 2, prefered position for read will be two. */
        fbe_raid_geometry_set_mirror_prefered_position(raid_geometry_p, FBE_MIRROR_PREFERED_POSITIOM_TWO);
    }
    else if(configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE)
    {
        /* incase of sparing from edge 2 - edge 1, prefered position for read will be one. */
        fbe_raid_geometry_set_mirror_prefered_position(raid_geometry_p, FBE_MIRROR_PREFERED_POSITION_ONE);
    }

    /* If the virtual drive debug flags are set adjust the raid group and
     * raid library debug flags based on the new configuration mode.
     */
    if (virtual_drive_debug_flags != 0)
    {
        /* Update the raid group and raid library debug flags based on 
         * configuration mode.
         */
        switch(configuration_mode)
        {
            case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
            case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
                /* Invoke the method that uses the default raid group debug
                 * flags.
                 */
                fbe_virtual_drive_set_raid_debug_flags(virtual_drive_p, FBE_TRUE);
                break;

            case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
            case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            default:
                /* Invoke the method to clear the raid group debug flags.
                 */
                fbe_virtual_drive_set_raid_debug_flags(virtual_drive_p, FBE_FALSE);
                break;
        }
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_set_configuration_mode()
*******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_get_configuration_mode()
 ******************************************************************************
 * @brief
 *  This function is used to get the configuration mode of the virtual drive
 *  object.
 *
 * @param virtual_drive_p      - Virtual drive object.
 * @param configuration_mode_p - Pointer to the configuration mode.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 ******************************************************************************/
fbe_status_t
fbe_virtual_drive_get_configuration_mode(fbe_virtual_drive_t * virtual_drive_p,
                                         fbe_virtual_drive_configuration_mode_t * configuration_mode_p)
{
    *configuration_mode_p = virtual_drive_p->configuration_mode;
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_get_configuration_mode()
*******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_is_mirror_configuration_mode_set()
 ******************************************************************************
 * @brief
 *  This function is used to verify whether virtual drive is currently
 *  configured as one of the mirror configuration mode.
 *
 * @param virtual_drive_p    - Virtual drive object.
 *
 * @return fbe_bool_t        - Returns TRUE if mirror mode set.
 *  
 *
 ******************************************************************************/
fbe_bool_t fbe_virtual_drive_is_mirror_configuration_mode_set(fbe_virtual_drive_t * virtual_drive_p)
{
    fbe_virtual_drive_configuration_mode_t configuration_mode;

    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    /* if one of the mirror configuration mode is set then return TRUE else return FALSE. */
    if((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE) ||
       (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE))
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}
/******************************************************************************
 * end fbe_virtual_drive_is_mirror_configuration_mode_set()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_is_pass_thru_configuration_mode_set()
 ******************************************************************************
 * @brief
 *  This function is used to verify whether virtual drive is currently
 *  configured as one of the pass thru configuration mode.
 *
 * @param virtual_drive_p   - Virtual drive object.
 *
 * @return fbe_bool_t       - Returns TRUE if mirror mode set.
 * 
 *
 ******************************************************************************/
fbe_bool_t fbe_virtual_drive_is_pass_thru_configuration_mode_set(fbe_virtual_drive_t * virtual_drive_p)
{
    fbe_virtual_drive_configuration_mode_t configuration_mode;

    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    /* if one of the mirror configuration mode is set then return TRUE else return FALSE. */
    if((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE) ||
       (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE))
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}
/******************************************************************************
 * end fbe_virtual_drive_is_pass_thru_configuration_mode_set()
*******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_set_new_configuration_mode()
 ******************************************************************************
 * @brief
 *  This function is used to set the new configuration mode of the virtual drive
 *  object.
 *
 * @param virtual_drive_p        - Virtual drive object.
 * @param new_configuration_mode - New configuration mode.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 ******************************************************************************/
fbe_status_t
fbe_virtual_drive_set_new_configuration_mode(fbe_virtual_drive_t * virtual_drive_p,
                                             fbe_virtual_drive_configuration_mode_t new_configuration_mode)
{
    virtual_drive_p->new_configuration_mode = new_configuration_mode;
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_set_new_configuration_mode()
*******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_set_default_offset()
 ******************************************************************************
 * @brief
 *  This function is used to set the new configuration mode of the virtual drive
 *  object.
 *
 * @param virtual_drive_p        - Virtual drive object.
 * @param new_configuration_mode - New configuration mode.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_set_default_offset(fbe_virtual_drive_t * virtual_drive_p,
                                                  fbe_lba_t  new_default_offset)
{
    fbe_base_config_t  *base_config_p = (fbe_base_config_t *)virtual_drive_p;

    fbe_block_transport_server_set_default_offset(&base_config_p->block_transport_server, new_default_offset);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_set_default_offset()
*******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_get_new_configuration_mode()
 ******************************************************************************
 * @brief
 *  This function is used to get the new configuration mode of the virtual
 *  drive object.
 *
 * @param virtual_drive_p      - Virtual drive object.
 * @param configuration_mode_p - Pointer to the configuration mode.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 ******************************************************************************/
fbe_status_t
fbe_virtual_drive_get_new_configuration_mode(fbe_virtual_drive_t * virtual_drive_p,
                                             fbe_virtual_drive_configuration_mode_t * new_configuration_mode_p)
{
    *new_configuration_mode_p = virtual_drive_p->new_configuration_mode;
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_get_new_configuration_mode()
*******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_set_unused_as_spare_flag()
 ******************************************************************************
 * @brief
 *  This function is used to set the init_unused_drive_as_spare_flag.
 *  When it's TRUE we select unused drives as hot spare.
 *  When it's FALSE, we are not allowed to do so, this is used for hot sparing.
 *
 * @param  fbe_bool_t  flag.

 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 ******************************************************************************/
void fbe_virtual_drive_set_unused_as_spare_flag(fbe_bool_t flag)
{
    init_unused_drive_as_spare_flag = flag;
}
/******************************************************************************
 * end fbe_virtual_drive_set_unused_as_spare_flag()
*******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_get_primary_edge_index()
 ******************************************************************************
 * @brief
 *  This function is used to get the primary edge index for the mirror vd object,
 *  it returns  edge index 0 if configuration mode is either pass through 
 *  first or mirror first edge else it will return the edge index 1.
 *
 * @param virtual_drive_p           - virtual dirve object.
 * @param primary_edge_index_p      - pointer to primary edge index.
 * 
 * @return fbe_status_t             - status of the operation.
 *
 * @author
 *  10/13/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_get_primary_edge_index(fbe_virtual_drive_t * virtual_drive_p,
                                                      fbe_edge_index_t * primary_edge_index_p)
{
    fbe_virtual_drive_configuration_mode_t configuration_mode;

    *primary_edge_index_p = FBE_EDGE_INDEX_INVALID;

    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    if((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE) ||
       (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE))
    {
        /* swap-in permanent spare with first edge. */
        *primary_edge_index_p = FIRST_EDGE_INDEX;
    }
    else if((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE) ||
            (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE))
    {
        /* swap-in permanent spare with second edge. */
        *primary_edge_index_p = SECOND_EDGE_INDEX;
    }
    else
    {
        *primary_edge_index_p = FBE_EDGE_INDEX_INVALID;
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_get_primary_edge_index()
*******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_get_secondary_edge_index()
 ******************************************************************************
 * @brief
 *  This function is used to get the secondary edge index for the mirror vd object.
 *
 * @param virtual_drive_p           - virtual dirve object.
 * @param secondary_edge_index_p    - pointer to secondary edge index.
 * 
 * @return fbe_status_t             - status of the operation.
 *
 * @author
 *  10/13/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_get_secondary_edge_index(fbe_virtual_drive_t *virtual_drive_p,
                                                        fbe_edge_index_t *secondary_edge_index_p)
{
    fbe_virtual_drive_configuration_mode_t configuration_mode;

    *secondary_edge_index_p = FBE_EDGE_INDEX_INVALID;

    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    if((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE) ||
       (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE))
    {
        /* secondary edge is the second edge */
        *secondary_edge_index_p = SECOND_EDGE_INDEX;
    }
    else if((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE) ||
            (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE))
    {
        /* secondary edge is the first edge */
        *secondary_edge_index_p = FIRST_EDGE_INDEX;
    }
    else
    {
        *secondary_edge_index_p = FBE_EDGE_INDEX_INVALID;
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_get_secondary_edge_index()
*******************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_is_downstream_drive_broken()
 ***************************************************************************** 
 * 
 * @brief   This method determines if the downstream drive is `permanently'
 *          broken.  For instance if the drive have been taken offline due to
 *          to many CRC errors or timeouts etc, then there is no point in
 *          waiting for the drive to come back online since it will never
 *          come back.
 *
 * @param virtual_drive_p - Virtual Drive object.
 * @param downstream_health_state - Health state of the dowstream object
 *                                  communication path.
 * @param caller - Routine which invoked this method
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  08/29/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_bool_t fbe_virtual_drive_is_downstream_drive_broken(fbe_virtual_drive_t *virtual_drive_p) 
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_bool_t          b_is_active;
    fbe_edge_index_t    edge_index_to_check = FBE_EDGE_INDEX_INVALID;
    fbe_path_state_t    first_edge_path_state;
    fbe_path_state_t    second_edge_path_state;
    fbe_path_attr_t     first_edge_path_attr;
    fbe_path_attr_t     second_edge_path_attr;

    /* determine if we are passive or active? */
    b_is_active = fbe_base_config_is_active((fbe_base_config_t *)virtual_drive_p);

    /* Get the path state and path attributes of both the downstream edges. */
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                       &first_edge_path_state);
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                       &second_edge_path_state);
    fbe_block_transport_get_path_attributes(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                            &first_edge_path_attr);
    fbe_block_transport_get_path_attributes(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                            &second_edge_path_attr);

    /* First determine which edge we should check.
     */
    status = fbe_virtual_drive_swap_find_broken_edge(virtual_drive_p, &edge_index_to_check);
    if (status != FBE_STATUS_OK)
    {
        /* If there is no position that needs a replacement generate an error
         * and return False.
         */
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s find swap out index failed. status :0x%x\n", __FUNCTION__, status);
        return FBE_FALSE;
    }

    /* If would could not locate a `broken' edge it simply means (1) of (2) things:
     *  1. The edge is transitioning from `disabled' to `broken'
     *                      OR
     *  2. The edge went `disabled' but is now back (`enabled')
     *
     */
    if (edge_index_to_check == FBE_EDGE_INDEX_INVALID)
    {
        /* If there is no position that needs a replacement and we are the active
         * SP generate an information message.
         */
        if (b_is_active == FBE_TRUE)
        {
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: drive index: %d is broken: %d path state:[%d-%d] attr:[0x%x-0x%x] wait\n",
                              edge_index_to_check, FBE_FALSE, first_edge_path_state, second_edge_path_state,
                              first_edge_path_attr, second_edge_path_attr);
        }
        return FBE_FALSE;
    }

    /* Now simply check the path attributes to determine if `Drive Fault' is set.
     */
    if (((edge_index_to_check == FIRST_EDGE_INDEX)                             &&
         ((first_edge_path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_DRIVE_FAULT) != 0)     ) ||
        ((edge_index_to_check == SECOND_EDGE_INDEX)                             &&
         ((second_edge_path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_DRIVE_FAULT) != 0)    )    )
    {
        /* Trace the fact that we will not wait the `need replacement time'
         * and return True (the drive is broken).
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: drive index: %d is broken: %d path state:[%d-%d] attr:[0x%x-0x%x]\n",
                              edge_index_to_check, FBE_TRUE, first_edge_path_state, second_edge_path_state,
                              first_edge_path_attr, second_edge_path_attr);
        return FBE_TRUE;
    }

    /* Else the drive is not broken so return false.
     */
    return FBE_FALSE;
}
/******************************************************************************
 * end fbe_virtual_drive_is_downstream_drive_broken()
 ******************************************************************************/

/*!**************************************************************
 * fbe_virtual_drive_check_hook()
 ****************************************************************
 * @brief
 *  This is the function that calls the hook for the pvd.
 *
 * @param virtual_drive_p - Object.
 * @param state - Current state of the monitor for the caller.
 * @param substate - Sub state of this monitor op for the caller.
 * @param status - status of the hook to return.
 * 
 * @return fbe_status_t Overall status of the operation.
 *
 * @author
 *  10/6/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_virtual_drive_check_hook(fbe_virtual_drive_t *virtual_drive_p,
                                          fbe_u32_t state,
                                          fbe_u32_t substate,
                                          fbe_u64_t val2,
                                          fbe_scheduler_hook_status_t *status)
{

	fbe_status_t ret_status;
    ret_status = fbe_base_object_check_hook((fbe_base_object_t *)virtual_drive_p,
                                            state,
                                            substate,
                                            val2,
                                            status);
    return ret_status;
}
/******************************************
 * end fbe_virtual_drive_check_hook()
 ******************************************/

/*!*************************************************************************************
 * fbe_virtual_drive_parent_mark_needs_rebuild_done_get_edge_to_set_to_end_marker()
 ***************************************************************************************
 *
 * @brief   This function uses the virtual drive configuration mode to determine
 *          which position in the virtual drive needs it's rebuild checkpoint set
 *          to the end marker.  There are (2) uses cases:
 *              o A drive swapped in to replace a failed drive
 *              o A copy operation failed and the destination need to be set to end
 *              o The source drive `glitched' (failed and came back before the abort
 *                copy timer trigger)
 * 
 * @param   virtual_drive_p - Pointer to virtual drive object
 * @param   edge_index_to_set_p - Pointer to edge index to set
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  04/17/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_parent_mark_needs_rebuild_done_get_edge_to_set_to_end_marker(fbe_virtual_drive_t *virtual_drive_p,
                                                                                            fbe_edge_index_t *edge_index_to_set_p)

{
    fbe_virtual_drive_configuration_mode_t configuration_mode;
    fbe_lba_t                           first_edge_checkpoint = 0;
    fbe_lba_t                           second_edge_checkpoint = 0;
    fbe_raid_position_bitmask_t         rl_bitmask = 0;
    fbe_path_state_t                    first_edge_path_state;
    fbe_path_state_t                    second_edge_path_state;

    /* Default value is invalid
     */
    *edge_index_to_set_p = FBE_EDGE_INDEX_INVALID;

    /* Get the virtual drive configuration mode and other values.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, FIRST_EDGE_INDEX, &first_edge_checkpoint);        
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, SECOND_EDGE_INDEX, &second_edge_checkpoint);
    fbe_raid_group_get_rb_logging_bitmask((fbe_raid_group_t *)virtual_drive_p, &rl_bitmask);
    fbe_block_transport_get_path_state(&((fbe_base_config_t *)virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                       &first_edge_path_state);
    fbe_block_transport_get_path_state(&((fbe_base_config_t *)virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                       &second_edge_path_state);

    /* Based on configuration mode determine which position needs the rebuild
     * checkpoint set to the end marker.
     */
    switch(configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
            /* If the first edge is not healthy trace a warning but continue.
             */
            if ((((1 << FIRST_EDGE_INDEX) & rl_bitmask) != 0)     ||
                (first_edge_path_state != FBE_PATH_STATE_ENABLED)    )
            {
                /* Trace a warning but continue.
                 */
                fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "virtual_drive: parent mark done get mode: %d path state: [%d-%d] rl: 0x%x chkpts: [0x%llx-0x%llx] \n",
                                      configuration_mode, first_edge_path_state, second_edge_path_state,
                                      rl_bitmask, first_edge_checkpoint, second_edge_checkpoint);
            }

            /* Return the first edge.
             */
            *edge_index_to_set_p = FIRST_EDGE_INDEX;
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            /* If the second edge is not healthy trace a warning but continue.
             */
            if ((((1 << SECOND_EDGE_INDEX) & rl_bitmask) != 0)     ||
                (second_edge_path_state != FBE_PATH_STATE_ENABLED)    )
            {
                /* Trace a warning but continue.
                 */
                fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "virtual_drive: parent mark done get mode: %d path state: [%d-%d] rl: 0x%x chkpts: [0x%llx-0x%llx] \n",
                                      configuration_mode, first_edge_path_state, second_edge_path_state,
                                      rl_bitmask, first_edge_checkpoint, second_edge_checkpoint);
            }

            /* Return the second edge.
             */
            *edge_index_to_set_p = SECOND_EDGE_INDEX;
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
            /* The source drive `glitched' (failed and returned before the
             * abort copy timer expired).  Validate that the source edge rebuild
             * checkpoint is at the end marker.  If it is not then something
             * went wrong.
             */
            if (first_edge_checkpoint != FBE_LBA_INVALID)
            {
                /*! @note Currently we do not support an degraded source drive.
                 */
                fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "virtual_drive: parent mark done get mode: %d path state: [%d-%d] rl: 0x%x chkpts: [0x%llx-0x%llx] bad\n",
                                      configuration_mode, first_edge_path_state, second_edge_path_state,
                                      rl_bitmask, first_edge_checkpoint, second_edge_checkpoint);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            else if ((((1 << FIRST_EDGE_INDEX) & rl_bitmask) != 0)     ||
                     (first_edge_path_state != FBE_PATH_STATE_ENABLED)    )
            {
                /* Trace a warning but continue.
                 */
                fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "virtual_drive: parent mark done get mode: %d path state: [%d-%d] rl: 0x%x chkpts: [0x%llx-0x%llx] \n",
                                      configuration_mode, first_edge_path_state, second_edge_path_state,
                                      rl_bitmask, first_edge_checkpoint, second_edge_checkpoint);
            }

            /* Return the first edge.
             */
            *edge_index_to_set_p = FIRST_EDGE_INDEX;
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            /* The source drive `glitched' (failed and returned before the
             * abort copy timer expired).  Validate that the source edge rebuild
             * checkpoint is at the end marker.  If it is not then something
             * went wrong.
             */
            if (second_edge_checkpoint != FBE_LBA_INVALID)
            {
                /*! @note Currently we do not support an degraded source drive.
                 */
                fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "virtual_drive: parent mark done get mode: %d path state: [%d-%d] rl: 0x%x chkpts: [0x%llx-0x%llx] bad\n",
                                      configuration_mode, first_edge_path_state, second_edge_path_state,
                                      rl_bitmask, first_edge_checkpoint, second_edge_checkpoint);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            else if ((((1 << SECOND_EDGE_INDEX) & rl_bitmask) != 0)     ||
                     (second_edge_path_state != FBE_PATH_STATE_ENABLED)    )
            {
                /* Trace a warning but continue.
                 */
                fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "virtual_drive: parent mark done get mode: %d path state: [%d-%d] rl: 0x%x chkpts: [0x%llx-0x%llx] \n",
                                      configuration_mode, first_edge_path_state, second_edge_path_state,
                                      rl_bitmask, first_edge_checkpoint, second_edge_checkpoint);
            }

            /* Return the first edge.
             */
            *edge_index_to_set_p = FIRST_EDGE_INDEX;
            break;

        default:
            /* Unexpected configuration mode
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "virtual_drive: parent mark done get mode: %d bad path state: [%d-%d] rl: 0x%x chkpts: [0x%llx-0x%llx] \n",
                                  configuration_mode, first_edge_path_state, second_edge_path_state,
                                  rl_bitmask, first_edge_checkpoint, second_edge_checkpoint);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Return success
     */
    return FBE_STATUS_OK;
}
/**************************************************************************************
 * end fbe_virtual_drive_parent_mark_needs_rebuild_done_get_edge_to_set_to_end_marker()
 **************************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_parent_mark_needs_rebuild_done_completion()
 ******************************************************************************
 * @brief
 *   This function is called when the non-paged metadata write to set the 
 *   rebuild checkpoint(s) to the logical end marker has completed.  It will 
 *   check the packet status and set any neccessary conditions.
 * 
 * @param in_packet_p   - pointer to a control packet from the scheduler
 * @param in_context    - completion context, which is a pointer to the raid 
 *                        group object
 * 
 * @return  fbe_status_t  
 *
 * @note    Completion - Return status
 *
 * @author
 *  10/01/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_parent_mark_needs_rebuild_done_completion(fbe_packet_t *packet_p,
                                                     fbe_packet_completion_context_t in_context)

{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_status_t                        packet_status;
    fbe_virtual_drive_t                *virtual_drive_p;
    fbe_raid_group_t                   *raid_group_p;
    fbe_virtual_drive_configuration_mode_t configuration_mode;
    fbe_bool_t                          b_is_edge_swapped = FBE_TRUE;
    fbe_bool_t                          b_is_mark_nr_required = FBE_FALSE;
    fbe_bool_t                          b_is_source_failed = FBE_FALSE;
    fbe_bool_t                          b_is_degraded_needs_rebuild = FBE_TRUE;
    fbe_bool_t                          b_is_mirror_mode = FBE_FALSE;
    fbe_edge_index_t                    upstream_edge_index;
    fbe_object_id_t                     upstream_object_id;
    fbe_block_edge_t                   *upstream_edge_p = NULL;
    fbe_path_attr_t                     upstream_edge_path_attr = 0;
    fbe_path_attr_t                     degraded_path_attr = 0;
    fbe_raid_group_np_metadata_flags_t  raid_group_np_flags_to_clear;

    /* Get the virtual drive object.
     */
    virtual_drive_p = (fbe_virtual_drive_t *)in_context;
    raid_group_p = (fbe_raid_group_t *)virtual_drive_p;
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    b_is_mirror_mode = fbe_virtual_drive_is_mirror_configuration_mode_set(virtual_drive_p);
    fbe_virtual_drive_metadata_is_edge_swapped(virtual_drive_p, &b_is_edge_swapped);
    fbe_virtual_drive_metadata_is_mark_nr_required(virtual_drive_p, &b_is_mark_nr_required);
    fbe_virtual_drive_metadata_is_source_failed(virtual_drive_p, &b_is_source_failed);
    fbe_virtual_drive_metadata_is_degraded_needs_rebuild(virtual_drive_p, &b_is_degraded_needs_rebuild);

    /* If success return `more processing'
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: parent mark needs rebuild compl mode: %d swapped: %d source failed: %d degraded: %d\n",
                          configuration_mode, b_is_edge_swapped, b_is_source_failed, b_is_degraded_needs_rebuild);

    /* Check the packet status.  If the write had an error, then we'll leave 
     * the condition set so that we can try again.  Return success so that the 
     * next completion function on the stack gets called.
     */ 
    packet_status = fbe_transport_get_status_code(packet_p);
    if (packet_status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s failed - status: 0x%x\n", 
                              __FUNCTION__, packet_status);
        return FBE_STATUS_OK;
    }

    /* Get the upstream edge information (sending this reques to this object).
     */
    fbe_block_transport_find_first_upstream_edge_index_and_obj_id(&raid_group_p->base_config.block_transport_server,
                                                                  &upstream_edge_index, &upstream_object_id);
    if (upstream_object_id != FBE_OBJECT_ID_INVALID)
    {
        upstream_edge_p = (fbe_block_edge_t *)fbe_base_transport_server_get_client_edge_by_client_id((fbe_base_transport_server_t *)&raid_group_p->base_config.block_transport_server,
                                                                             upstream_object_id);
        fbe_block_transport_get_path_attributes(upstream_edge_p, &upstream_edge_path_attr);
    }

    /* If the `degraded needs rebuild' is set, clear it now.
     */
    degraded_path_attr = upstream_edge_path_attr & (FBE_BLOCK_PATH_ATTR_FLAGS_DEGRADED | FBE_BLOCK_PATH_ATTR_FLAGS_DEGRADED_NEEDS_REBUILD);
    if (degraded_path_attr != 0)
    {
        /* Trace an informational message and clear the upstream path attribute.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: parent mark needs rebuild compl upstream attr: 0x%x obj: 0x%x clear: 0x%x\n", 
                              upstream_edge_path_attr, upstream_object_id, degraded_path_attr);

        /* Now clear the `degraded' in the upstream edge.
         */
        fbe_block_transport_server_clear_path_attr_all_servers(&((fbe_base_config_t *)virtual_drive_p)->block_transport_server,
                                                               degraded_path_attr);
    }

    /* Do not clear swapped or source failed if we are in mirror mode.
     */
    if (b_is_mirror_mode)
    {
        b_is_edge_swapped = FBE_FALSE;
        b_is_source_failed = FBE_FALSE;
    }

    /* Determine if we need to clear the swapped bit or not.
     */
    fbe_virtual_drive_metadata_determine_nonpaged_flags(virtual_drive_p,
                                                        &raid_group_np_flags_to_clear,
                                                        b_is_edge_swapped,  /* If edge swapped is set we want to clear it */
                                                        FBE_FALSE,          /* If edge swapped is set we want to clear it */
                                                        b_is_mark_nr_required,  /* If mark nr required is set we want to clear it */
                                                        FBE_FALSE,          /* If mark nr required is set we want to clear it */
                                                        b_is_source_failed, /* If source failed is set we want to clear it */
                                                        FBE_FALSE,          /* If source failed is set we want to clear it */
                                                        b_is_degraded_needs_rebuild, /* Clear degraded needs rebuild if set*/
                                                        FBE_FALSE           /* Clear degraded needs rebuild if set*/);
    /* Now write the updated non-paged flags.
     */
    status = fbe_virtual_drive_metadata_write_nonpaged_flags(virtual_drive_p,
                                                             packet_p,
                                                             raid_group_np_flags_to_clear);
    if (status != FBE_STATUS_MORE_PROCESSING_REQUIRED)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s edge swapped: %d source failed: %d write failed - status: 0x%x\n",
                              __FUNCTION__, b_is_edge_swapped, b_is_source_failed, status);
    
        /* Fail the request.
         */
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    /* Return more processing required.
     */
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/************************************************************************************
 * end fbe_virtual_drive_parent_mark_needs_rebuild_done_completion()
 ************************************************************************************/
 
/*!****************************************************************************
 *      fbe_virtual_drive_parent_mark_needs_rebuild_done_write_checkpoints()
 ******************************************************************************
 * @brief
 *   This function will set the rebuild checkpoint to end marker
 * 
 * @param packet_p
 * @param context
 * 
 * @return  status - FBE_STATUS_OK - Previous request failed - complete packet
 *                   FBE_STATUS_MORE_PROCESSING_REQUIRED - Continue 
 *
 * @note    Completion - Just return status
 *
 * @author
 *  04/13/2013  Ron Proulx  - Updated to clear unused edge.
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_parent_mark_needs_rebuild_done_write_checkpoints(fbe_packet_t *packet_p,                                    
                                                                                       fbe_packet_completion_context_t context)

{
    fbe_status_t                            status = FBE_STATUS_OK;                 
    fbe_virtual_drive_t                    *virtual_drive_p = NULL;
    fbe_edge_index_t                        edge_index_to_set = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                        edge_index_to_clear = FBE_EDGE_INDEX_INVALID;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;    
    fbe_lba_t                               first_edge_checkpoint = 0;
    fbe_lba_t                               second_edge_checkpoint = 0;
    fbe_raid_position_bitmask_t             rl_bitmask;
    fbe_u64_t                               metadata_offset;
    fbe_u32_t                               metadata_write_size;               
    fbe_raid_group_nonpaged_metadata_t     *nonpaged_metadata_p = NULL;
    fbe_raid_group_rebuild_nonpaged_info_t  rebuild_nonpaged_info;
    fbe_raid_group_rebuild_nonpaged_info_t *rebuild_nonpaged_info_p = NULL;
    fbe_lba_t                               dest_drive_checkpoint = 0;
    fbe_lba_t                               source_drive_checkpoint = 0;
    fbe_path_state_t                        dest_edge_path_state;
    fbe_path_state_t                        source_edge_path_state;
    fbe_lifecycle_state_t                   my_state = FBE_LIFECYCLE_STATE_INVALID;
    
    /* Get the virtual drive object.
     */
    virtual_drive_p = (fbe_virtual_drive_t *)context;

    /* If the status is not ok, that means we didn't get the 
     * lock. Just return. we are already in the completion routine
     */
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get the NP lock, status: 0x%x", 
                              __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    /* Set the completion function to release the NP lock.
     * Once we set the completion we must:
     *  o Complete the packet
     *  o Return `more processing required'
     */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_release_NP_lock, virtual_drive_p);

    /* Use the virtual drive specific method to locate the disk that needs it
     * rebuild checkpoint set.
     */
    fbe_virtual_drive_parent_mark_needs_rebuild_done_get_edge_to_set_to_end_marker(virtual_drive_p, &edge_index_to_set);

    /* Trace some information if enabled.
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_SWAP_TRACING,
                            "virtual_drive: parent write chkpts index to set: %d\n",
                            edge_index_to_set);

    /* If we could not locate the edge that needs to have the checkpoint set
     * it is an error.
     */
    if (edge_index_to_set == FBE_EDGE_INDEX_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s could not locate edge index to set checkpoint for.", __FUNCTION__); 

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Generate the index to clear.
     */
    edge_index_to_clear = (edge_index_to_set == FIRST_EDGE_INDEX) ? SECOND_EDGE_INDEX : FIRST_EDGE_INDEX;

    /*! @note Invoke virtual drive specific method that:
     *          o Sets the primary position checkpoint to the end marker
     *          o Sets the alternate position checkpoint to zero (0)
     *          o Sets the `rebuild logging' flag for the alternate position
     */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)virtual_drive_p, &my_state);
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, FIRST_EDGE_INDEX, &first_edge_checkpoint);        
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, SECOND_EDGE_INDEX, &second_edge_checkpoint);
    fbe_raid_group_get_rb_logging_bitmask((fbe_raid_group_t *)virtual_drive_p, &rl_bitmask);

    /* Trace some information if enabled
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_SWAP_TRACING,
                            "virtual_drive: parent write chkpts mode: %d set index: %d mask: 0x%x chkpts: [0x%llx-0x%llx] \n",
                            configuration_mode, edge_index_to_set, rl_bitmask, 
                            (unsigned long long)first_edge_checkpoint, 
                            (unsigned long long)second_edge_checkpoint);

    /* Get a pointer to the non-paged metadata.
     */ 
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)virtual_drive_p, (void **)&nonpaged_metadata_p);

    /* Copy the existing data to the structure we will write.
     */
    rebuild_nonpaged_info_p = &rebuild_nonpaged_info;
    *rebuild_nonpaged_info_p = *(&(nonpaged_metadata_p->rebuild_info));

    /* Get the checkpoints.
     */

    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, edge_index_to_set, &dest_drive_checkpoint);        
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, edge_index_to_clear, &source_drive_checkpoint);

    /* Get the path state of both the downstream edges. 
     */
    fbe_block_transport_get_path_state(&((fbe_base_config_t *)virtual_drive_p)->block_edge_p[edge_index_to_set],
                                       &dest_edge_path_state);
    fbe_block_transport_get_path_state(&((fbe_base_config_t *)virtual_drive_p)->block_edge_p[edge_index_to_clear],
                                       &source_edge_path_state);

    /* The parent raid group has marked needs rebuild as required.  The use
     * cases are:
     *  1. Permanent spare (typical) - Parent has set marked needs rebuild and
     *          now we need to set the rebuild checkpoint to the end marker.
     *
     *  2. Copy operation failed before the destination was fully rebuilt -
     *          Set the destination edge rebuild checkpoint to the end marker.
     *
     *  3. Source drive `glitched' (failed and returned) during a copy operation -
     *          Nothing to do (we validated that the source edge checkpoint is
     *          still at the end marker).
     */
    switch(configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            /* If the virtual drive or edge is no longer healthy something
             * went wrong.
             */
            if ((my_state == FBE_LIFECYCLE_STATE_READY)          &&
                (dest_edge_path_state == FBE_PATH_STATE_ENABLED)    )
            {
                /* Clear rebuild logging for the destination (position to set end marker
                 * for) and set it for the source.
                 */
                rebuild_nonpaged_info_p->rebuild_logging_bitmask &= ~(1 << edge_index_to_set);
                rebuild_nonpaged_info_p->rebuild_logging_bitmask |= (1 << edge_index_to_clear);

                /* Set the destination end marker to the end and set the source to zero.
                 */
                rebuild_nonpaged_info_p->rebuild_checkpoint_info[0].checkpoint = 0;
                rebuild_nonpaged_info_p->rebuild_checkpoint_info[0].position = edge_index_to_clear;
                rebuild_nonpaged_info_p->rebuild_checkpoint_info[1].checkpoint = FBE_LBA_INVALID;
                rebuild_nonpaged_info_p->rebuild_checkpoint_info[1].position = FBE_RAID_INVALID_DISK_POSITION;

                /* Trace some information if enabled.
                 */
                fbe_virtual_drive_trace(virtual_drive_p,
                                        FBE_TRACE_LEVEL_INFO,
                                        FBE_VIRTUAL_DRIVE_DEBUG_FLAG_SWAP_TRACING,
                                        "virtual_drive: parent chkpts mode: %d life: %d rl_mask: 0x%x chkpts:[0x%llx-0x%llx] pass-thru\n",
                                        configuration_mode, my_state,
                                        rebuild_nonpaged_info_p->rebuild_logging_bitmask,
                                        rebuild_nonpaged_info_p->rebuild_checkpoint_info[0].checkpoint,
                                        rebuild_nonpaged_info_p->rebuild_checkpoint_info[1].checkpoint);
            }
            else
            {
                /* Else the state has changed so return an error.
                 */
                fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "virtual_drive: parent chkpts mode: %d life: %d set: %d path state: %d chkpt: 0x%llx unexpected\n",
                                      configuration_mode, my_state, edge_index_to_set, dest_edge_path_state,
                                      dest_drive_checkpoint);

                fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet_p);
                return FBE_STATUS_MORE_PROCESSING_REQUIRED;
            }
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            /* Nothing to set packet status to ok and return.
             */
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;

        default:
            /* Unsupported configuration mode */
			/* I confirmed with Ron that the parent raid group has a local state and when the monitor runs again it will retry the usurper */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "virtual_drive: parent chkpts mode: %d life: %d set: %d path state: %d chkpt: 0x%llx mode bad\n",
                                  configuration_mode, my_state, edge_index_to_set, dest_edge_path_state,
                                  dest_drive_checkpoint);

            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Now generate the write size and offset
     */
    metadata_write_size = sizeof(*rebuild_nonpaged_info_p);
    metadata_offset = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->rebuild_info);

    /* Now send the write for the nonpaged.
     */ 
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*)virtual_drive_p, 
                                                             packet_p, 
                                                             metadata_offset, 
                                                             (fbe_u8_t *)rebuild_nonpaged_info_p,
                                                             metadata_write_size);

    /* Return more processing since a packet is outstanding.
     */
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/***************************************************************************
 * end fbe_virtual_drive_parent_mark_needs_rebuild_done_write_checkpoints()
 ***************************************************************************/ 

/*!****************************************************************************
 *          fbe_virtual_drive_parent_mark_needs_rebuild_done_set_checkpoint_to_end_marker()
 ******************************************************************************
 *
 * @brief   This method is called to handle the `clear swapped' request.  This
 *          request is sent from the parent raid group when it needs to mark the
 *          virtual drive as `ready for business'.  The parent raid group does
 *          this after it has marked NR for any degraded regions.  This is done
 *          to a drive that permanently replaced a failed disk.  When the drive
 *          is swapped in the rebuild checkpoint will be 0.  This method will
 *          advance the rebuild checkpoint to the end marker.
 *
 * @param   virtual_drive_p - Pointer to virtual drive object to set checkpoint for
 * @param   packet_p               - pointer to a control packet from the scheduler
 *
 * @return  status - Typically FBE_STATUS_MORE_PROCESSING_REQUIRED
 *
 * @note    Called - Must complete packet
 *
 * @author
 *  03/21/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_parent_mark_needs_rebuild_done_set_checkpoint_to_end_marker(fbe_virtual_drive_t *virtual_drive_p,
                                                                     fbe_packet_t *packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_edge_index_t                        edge_index_to_set = FBE_EDGE_INDEX_INVALID;
    fbe_bool_t                              b_is_active;
    fbe_bool_t                              b_is_edge_swapped;
    fbe_bool_t                              b_is_degraded_needs_rebuild;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;

    /* Trace the function entry
     */
    fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p,     
                          FBE_TRACE_LEVEL_DEBUG_HIGH,          
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                          "%s entry\n", __FUNCTION__); 

    /* Get the configuration mode for debug purposes.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    /* We we are not active (alternate could have taken over?)
     */
    fbe_raid_group_monitor_check_active_state((fbe_raid_group_t *)virtual_drive_p, &b_is_active);
   
    /* Determine if we need to clear the swapped bit or not.
     */
    fbe_virtual_drive_metadata_is_edge_swapped(virtual_drive_p, &b_is_edge_swapped);
    
    /* Determine if degraded needs rebuild is set
     */
    fbe_virtual_drive_metadata_is_degraded_needs_rebuild(virtual_drive_p, &b_is_degraded_needs_rebuild);
      
    /* Locate the position that need to have the checkpoint set to the end
     * marker.
     */
    fbe_virtual_drive_parent_mark_needs_rebuild_done_get_edge_to_set_to_end_marker(virtual_drive_p, &edge_index_to_set);

    /* Always trace some information
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_SWAP_TRACING,
                            "virtual_drive: parent mark needs rebuild done active: %d mode: %d swapped: %d degraded: %d edge to set: %d\n",
                            b_is_active, configuration_mode, b_is_edge_swapped, b_is_degraded_needs_rebuild, edge_index_to_set);

    /*! @note There is no guarantee that the parent raid group active will
     *        agree with the active virtual drive.  In addition either the
     *        active or passive side can modify the non-paged since the
     *        it is protected by the non-paged lock.
     */

    /* If the edge is not swapped and we could not locate the edge to set the
     * the end marker it is an error.
     */
    if ((edge_index_to_set == FBE_EDGE_INDEX_INVALID) &&
        (b_is_edge_swapped == FBE_FALSE)              &&
        (b_is_degraded_needs_rebuild == FBE_FALSE)       )
    {
        /* This is an error. Fail the request.
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s neither edge index or swapped is set.\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }
    else if (edge_index_to_set == FBE_EDGE_INDEX_INVALID)
    {
        /* Else if the set checkpoint index is invalid but the swapped bit is
         * set, simply clear the swapped flag.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: parent mark needs rebuild done no edge to set mode: %d swapped: %d degraded: %d\n",
                              configuration_mode, b_is_edge_swapped, b_is_degraded_needs_rebuild);

        /* Just complete the packet with success.  The completion method
         * will clear the swapped flag.
         */
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    /*! @note There are currently (2) conditions that will be executing I/O:
     *          o Re-zero the paged metadata on the now permanent spare
     *          o Write the default metadata on the now permanent spare
     *
     *  @todo Is it Ok to not wait for quiesce?
     */
    status = fbe_virtual_drive_quiesce_io_if_needed(virtual_drive_p);
    if (status == FBE_STATUS_PENDING)
    {
        /*! @todo Currently we trace in informational message.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: parent set checkpoint to end mode: %d swapped: %d edge to set: %d quiesce\n",
                              configuration_mode, b_is_edge_swapped, edge_index_to_set);
        //return FBE_STATUS_GENERIC_FAILURE; 
    }
    
    /* Set the rebuild checkpoint to the end marker on the permanent spare.
     */
    fbe_transport_set_completion_function(packet_p, 
                                          fbe_virtual_drive_metadata_set_checkpoint_to_end_marker_completion, 
                                          virtual_drive_p);    
    status = fbe_raid_group_get_NP_lock((fbe_raid_group_t *)virtual_drive_p, 
                                        packet_p, 
                                        fbe_virtual_drive_parent_mark_needs_rebuild_done_write_checkpoints);
        
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

}
/*************************************************************************************
 * end fbe_virtual_drive_parent_mark_needs_rebuild_done_set_checkpoint_to_end_marker()
 *************************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_get_checkpoint_for_parent_raid_group()
 *****************************************************************************
 *
 * @brief   This method determines the checkpoint when the caller is the
 *          raid group class of a raid group:
 *              o Parity raid group
 *                  or
 *              o Mirror raid group
 *                  or
 *              o Striper raid group
 *          It is used to determine the relative health of the raid group 
 *          (i.e. should it go broken etc).
 * 
 * @param   virtual_drive_p - The virtual drive.
 * @param   rg_server_obj_info_p - Pointer to control operation to populate
 * @param   configuration_mode - The configuration mode of the virtual drive
 * @param   b_is_edge_swapped - FBE_TRUE - The edge is swapped and therefore must
 *              be fully rebuilt.
 * @param   b_is_copy_complete - FBE_TRUE - The copy operation has completed.
 *              This means that the source edge is marked rebuild logging and
 *              the source checkpoint is 0.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/02/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_virtual_drive_get_checkpoint_for_parent_raid_group(fbe_virtual_drive_t *virtual_drive_p, 
                                                                    fbe_raid_group_get_checkpoint_info_t *rg_server_obj_info_p,
                                                                    fbe_virtual_drive_configuration_mode_t configuration_mode,
                                                                    fbe_bool_t b_is_edge_swapped,
                                                                    fbe_bool_t b_is_copy_complete)

{
    fbe_lba_t       first_edge_checkpoint = 0;
    fbe_lba_t       second_edge_checkpoint = 0;
    fbe_bool_t      b_is_source_failed_set = FBE_FALSE;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry((fbe_raid_group_t *)virtual_drive_p);
    fbe_lba_t       metadata_lba;

    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Get the checkpoints for both of the virtual drive edges.
     */
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, FIRST_EDGE_INDEX, &first_edge_checkpoint);        
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, SECOND_EDGE_INDEX, &second_edge_checkpoint);
    fbe_virtual_drive_metadata_is_source_failed(virtual_drive_p, &b_is_source_failed_set);
    fbe_raid_geometry_get_metadata_start_lba(raid_geometry_p, &metadata_lba);   
            
    /*! @note There are (3) possible values for the checkpoint returned:
     *          1.  0              - Position is completely degraded or swapped
     *          2. -1 (end marker) - Position is completely rebuilt (i.e. not degraded)
     *          3.  Other          - The source drive of a copy operation failed.
     *
     *  @note It is up to the parent raid group to subtract the downstream offset
     *        if the checkpoint returned is not 0 or -1.
     */

    /*! @note The purpose of this method is to let the parent raid group know
     *        the `health' of the virtual drive extent associated with a raid
     *        group position.  Here are the (4) cases based on configuration
     *        mode:
     *          o The virtual drive is in pass-thru mode first edge:
     *              + Return the first edge checkpoint:
     *                  1. Checkpoint is less than or equal paged metadata: return checkpoint
     *                  2. Checkpoint is in paged metadata or swapped: return 0
     *                  3. Else return the checkpoint which is at the end marker
     *          o The virtual drive is in pass-thru mode second edge:
     *              + Return the second edge checkpoint:
     *                  1. Checkpoint is less than or equal paged metadata: return checkpoint
     *                  2. Checkpoint is in paged metadata or swapped: return 0
     *                  3. Else return the checkpoint which is at the end marker
     *          o The virtual drive is in mirror mode first edge:
     *              + If this is a `mark NR' request and he source drive was
     *                removed, return the destination checkpoint, otherwise
     *                return the source checkpoint.  If this is a `clear rl'
     *                request and the copy is complete return the destination
     *                checkpoint, otherwise return the source checkpoint. 
     *          o The virtual drive is in mirror mode second edge:
     *              + If this is a `mark NR' request and he source drive was
     *                removed, return the destination checkpoint, otherwise
     *                return the source checkpoint.  If this is a `clear rl'
     *                request and the copy is complete return the destination
     *                checkpoint, otherwise return the source checkpoint. 
     */

    /* Based on the configuration mode populate the virtual drive checkpoint.
     */
    if (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE)
    {
        /* Set the return value to checkpoint of the first edge.
         */
        if (first_edge_checkpoint <= metadata_lba) 
        {
            /* If the checkpoint is less than or equal to the paged metadata
             * start it means that we are rebuilding the user area.
             */
            rg_server_obj_info_p->downstream_checkpoint = first_edge_checkpoint;
        }
        else if ((first_edge_checkpoint != FBE_LBA_INVALID) ||
                 (b_is_edge_swapped)                           )
        {
            /* Return 0 since we haven't completed rebuilding the paged.
             * Or if the this is a permanent spare return 0.
             */
            rg_server_obj_info_p->downstream_checkpoint = 0;
        }
        else
        {
            /* Else virtual drive is completely rebuilt.
             */
            rg_server_obj_info_p->downstream_checkpoint = first_edge_checkpoint;
        }
    }

    else if (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE)
    {
        /* Set the return value to checkpoint of the second edge.
         */
        if (second_edge_checkpoint <= metadata_lba) 
        {
            /* If the checkpoint is less than or equal to the paged metadata
             * start it means that we are rebuilding the user area.
             */
            rg_server_obj_info_p->downstream_checkpoint = second_edge_checkpoint;
        }
        else if ((second_edge_checkpoint != FBE_LBA_INVALID) ||
                 (b_is_edge_swapped)                            )
        {
            /* Return 0 since we haven't completed rebuilding the paged.
             * Or if the this is a permanent spare return 0.
             */
            rg_server_obj_info_p->downstream_checkpoint = 0;
        }
        else
        {
            /* Else virtual drive is completely rebuilt.
             */
            rg_server_obj_info_p->downstream_checkpoint = second_edge_checkpoint;
        }
    }

    else if (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)
    {
        /*! @note If this is a request to determine the `mark NR' checkpoint and
         *        the source edge was failed, return the checkpoint of
         *        the destination drive.
         */
        if (rg_server_obj_info_p->request_type == FBE_RAID_GROUP_GET_CHECKPOINT_REQUEST_TYPE_EVAL_MARK_NR)
        {
            /* If the source drive was failed, return the checkpoint of the
             * destination drive.
             */
            if (b_is_source_failed_set == FBE_TRUE)
            {
                rg_server_obj_info_p->downstream_checkpoint = second_edge_checkpoint;
            }
            else
            {
                /* Else the source drive was never missing so return the 
                 * checkpoint of the source drive.
                 */
                rg_server_obj_info_p->downstream_checkpoint = first_edge_checkpoint;
            }
        }
        else
        {
            /* Else this is a request to clear rebuild logging.  If copy is
             * complete return the destination checkpoint, otherwise return the
             * source check point.
             */
            if (b_is_copy_complete == FBE_TRUE)
            {
                rg_server_obj_info_p->downstream_checkpoint = second_edge_checkpoint;
            }
            else
            {
                /* Else the checkpoint is the source edge checkpoint.
                 */
                rg_server_obj_info_p->downstream_checkpoint = first_edge_checkpoint;
            }
        }
    }

    else if (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE)
    {
        /*! @note If this is a request to determine the `mark NR' checkpoint and
         *        the source edge was failed, return the checkpoint of
         *        the destination drive.
         */
        if (rg_server_obj_info_p->request_type == FBE_RAID_GROUP_GET_CHECKPOINT_REQUEST_TYPE_EVAL_MARK_NR)
        {
            /* If the source drive was failed, return the checkpoint of the
             * destination drive.
             */
            if (b_is_source_failed_set == FBE_TRUE)
            {
                rg_server_obj_info_p->downstream_checkpoint = first_edge_checkpoint;
            }
            else
            {
                /* Else the source drive was never missing so return the 
                 * checkpoint of the source drive.
                 */
                rg_server_obj_info_p->downstream_checkpoint = second_edge_checkpoint;
            }
        }
        else
        {
            /* Else this is a request to clear rebuild logging.  If copy is
             * complete return the destination checkpoint, otherwise return the
             * source check point.
             */
            if (b_is_copy_complete == FBE_TRUE)
            {
                rg_server_obj_info_p->downstream_checkpoint = first_edge_checkpoint;
            }
            else
            {
                /* Else the checkpoint is the source edge checkpoint.
                 */
                rg_server_obj_info_p->downstream_checkpoint = second_edge_checkpoint;
            }
        }
    }

    /* If the virtual drive is in mirror mode always trace the information.
     * Otherwise only trace the information if enabled.
     */
    if ((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)  ||
        (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE)    )
    {
        /* Always trace the information when the virtual drive is in mirror mode.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: get chkpt for parent vd chkpt: 0x%llx first: 0x%llx second: 0x%llx\n",
                              (unsigned long long)rg_server_obj_info_p->downstream_checkpoint, (unsigned long long)first_edge_checkpoint, (unsigned long long)second_edge_checkpoint);
    }
    else
    {
        /* Else only trace if enabled.
         */
        fbe_virtual_drive_trace(virtual_drive_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_VIRTUAL_DRIVE_DEBUG_FLAG_RL_TRACING,
                                "virtual_drive: get chkpt for parent vd chkpt: 0x%llx first: 0x%llx second: 0x%llx\n",
                                (unsigned long long)rg_server_obj_info_p->downstream_checkpoint, (unsigned long long)first_edge_checkpoint, (unsigned long long)second_edge_checkpoint);
    }

    return FBE_STATUS_OK;
}
/**************************************************************
 * end fbe_virtual_drive_get_checkpoint_for_parent_raid_group()
 **************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_get_checkpoint_for_virtual_drive()
 ***************************************************************************** 
 * 
 * @brief   This method determines the checkpoint when the caller is the
 *          raid group class of the virtual drive object.  it is used to
 *          determine the relative health of the virtual drive (i.e. should it
 *          go broken etc).
 * 
 * @param   virtual_drive_p - The virtual drive.
 * @param   rg_server_obj_info_p - Pointer to control operation to populate
 * @param   configuration_mode - The configuration mode of the virtual drive
 * @param   b_is_edge_swapped - FBE_TRUE - The edge is swapped and therefore must
 *              be fully rebuilt.
 * @param   b_is_copy_complete - FBE_TRUE - The copy operation has completed.
 *              This means that the source edge is marked rebuild logging and
 *              the source checkpoint is 0.
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  10/02/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_virtual_drive_get_checkpoint_for_virtual_drive(fbe_virtual_drive_t *virtual_drive_p, 
                                                                fbe_raid_group_get_checkpoint_info_t *rg_server_obj_info_p,
                                                                fbe_virtual_drive_configuration_mode_t configuration_mode,
                                                                fbe_bool_t b_is_edge_swapped,
                                                                fbe_bool_t b_is_copy_complete)
{
    fbe_lba_t                   first_edge_checkpoint = 0;
    fbe_lba_t                   second_edge_checkpoint = 0;
    fbe_lba_t                   edge_index_checkpoint = 0;
    fbe_raid_position_bitmask_t rl_bitmask = 0;
    fbe_bool_t                  b_is_source_failed_set = FBE_FALSE;
    fbe_path_state_t            first_edge_path_state;
    fbe_path_state_t            second_edge_path_state;
    fbe_raid_group_get_checkpoint_request_type_t request_type = rg_server_obj_info_p->request_type;

    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Get the checkpoints for both of the virtual drive edges.
     */
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, FIRST_EDGE_INDEX, &first_edge_checkpoint);        
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, SECOND_EDGE_INDEX, &second_edge_checkpoint);
    fbe_raid_group_get_rb_logging_bitmask((fbe_raid_group_t *)virtual_drive_p, &rl_bitmask);
    fbe_virtual_drive_metadata_is_source_failed(virtual_drive_p, &b_is_source_failed_set);   

    /* Get the path state of both the downstream edges.
     */
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                       &first_edge_path_state);
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                       &second_edge_path_state);

    /* Validate the edge index of the request.
     */
    if ((rg_server_obj_info_p->edge_index != (fbe_u32_t)FIRST_EDGE_INDEX)  &&
        (rg_server_obj_info_p->edge_index != (fbe_u32_t)SECOND_EDGE_INDEX)    )
    {
        /* Trace and error and fail the request.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s unsupported edge index: %d \n",
                              __FUNCTION__, rg_server_obj_info_p->edge_index);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, rg_server_obj_info_p->edge_index, &edge_index_checkpoint); 

    /*! @note The purpose of this method is to get the virtual drive checkpoint 
     *        for the raid group superclass of the virtual drive.  The raid
     *        group class will use the checkpoint to:
     *          1. Evaluate if rebuild logging can be cleared for the position
     *             specified by edge index.  In this case the result is treated
     *             as a binary:
     *              o -1 - The position is fully rebuilt and therefore rebuild
     *                     logging can be cleared.
     *              o Other - The position is not fully rebuilt and therefore
     *                        rebuild logging should not be cleared.
     *                          OR
     *          2. Evaluate which chunks need to be marked `Needs Rebuild' (NR).
     *             The checkpoint returned is used to determine which areas
     *             need to be marked.
     *        Here are the (4) cases based on configuration mode:
     *          o The virtual drive is in pass-thru mode first edge:
     *              + A request type of `mark NR' is unexpected.  Return an
     *                error and a checkpoint of 0.
     *              + If the `source drive failed' flag is set and this is a
     *                request to clear rebuild logging (which it must be)
     *                and the path state is enabled, return the end marker
     *                so that we clear rebuild logging and continue.
     *              + Else return the first edge checkpoint unless the drive is
     *                swapped.  If the drive is swapped return 0.
     *          o The virtual drive is in pass-thru mode second edge:
     *              + A request type of `mark NR' is unexpected.  Return an
     *                error and a checkpoint of 0.
     *              + If the `source drive failed' flag is set and this is a
     *                request to clear rebuild logging (which it must be)
     *                and the path state is enabled, return the end marker
     *                so that we clear rebuild logging and continue.
     *              + Else return the second edge checkpoint unless the drive is
     *                swapped.  If the drive is swapped return 0.
     *          o The virtual drive is in mirror mode first edge:
     *              + Simply return the checkpoint for the edge index supplied.
     *                If the drive is swapped return 0.
     *          o The virtual drive is in mirror mode second edge:
     *              + Simply return the checkpoint for the edge index supplied.
     *                If the drive is swapped return 0.
     */

    /* Based on the configuration mode populate the virtual drive checkpoint.
     */
    if (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE)
    {
        /* A request type of FBE_RAID_GROUP_GET_CHECKPOINT_REQUEST_TYPE_EVAL_MARK_NR
         * is not allowed in pass-thru mode.
         */
        if (request_type == FBE_RAID_GROUP_GET_CHECKPOINT_REQUEST_TYPE_EVAL_MARK_NR)
        {
            /* Can't mark NR if the virtual drive is in pass-thru mode.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s mode: %d request type: %d not supported in pass-thru\n",
                                  __FUNCTION__, configuration_mode, request_type);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        else if ((b_is_source_failed_set == FBE_TRUE)                                  &&
                 (request_type == FBE_RAID_GROUP_GET_CHECKPOINT_REQUEST_TYPE_CLEAR_RL) &&
                 (first_edge_path_state == FBE_PATH_STATE_ENABLED)                        )
        {
            /* This is the cleanup after the source drive failed.  Return the
             * end marker so that we clear rebuild logging for this edge.
             */
            rg_server_obj_info_p->downstream_checkpoint = FBE_LBA_INVALID;
             
        }
        else
        {
            /* Else (typical case) check if the drive is swapped and return the
             * checkpoint for the first edge.
             */
            rg_server_obj_info_p->downstream_checkpoint = first_edge_checkpoint;

            /*! @note The assumption is that if we are in first edge pass-thru and
             *        at least (1) edge has been swapped and the checkpoint indicates
             *        that we are fully rebuilt, we must return a checkpoint of 0 so
             *        that the parent raid group rebuilds the entire position.
             */
            if(b_is_edge_swapped && (first_edge_checkpoint == FBE_LBA_INVALID))
            {
                rg_server_obj_info_p->downstream_checkpoint = 0;
            }
        }
    }

    else if (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE)
    {
        /* A request type of FBE_RAID_GROUP_GET_CHECKPOINT_REQUEST_TYPE_EVAL_MARK_NR
         * is not allowed in pass-thru mode.
         */
        if (request_type == FBE_RAID_GROUP_GET_CHECKPOINT_REQUEST_TYPE_EVAL_MARK_NR)
        {
            /* Can't mark NR if the virtual drive is in pass-thru mode.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s mode: %d request type: %d not supported in pass-thru\n",
                                  __FUNCTION__, configuration_mode, request_type);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        else if ((b_is_source_failed_set == FBE_TRUE)                                  &&
                 (request_type == FBE_RAID_GROUP_GET_CHECKPOINT_REQUEST_TYPE_CLEAR_RL) &&
                 (second_edge_path_state == FBE_PATH_STATE_ENABLED)                       )
        {
            /* This is the cleanup after the source drive failed.  Return the
             * end marker so that we clear rebuild logging for this edge.
             */
            rg_server_obj_info_p->downstream_checkpoint = FBE_LBA_INVALID;
             
        }
        else
        {
            /* Else (typical case) check if the drive is swapped and return the
             * checkpoint for the second edge.
             */
            rg_server_obj_info_p->downstream_checkpoint = second_edge_checkpoint;

            /*! @note The assumption is that if we are in second edge pass-thru and
             *        at least (1) edge has been swapped and the checkpoint indicates
             *        that we are fully rebuilt, we must return a checkpoint of 0 so
             *        that the parent raid group rebuilds the entire position.
             */
            if (b_is_edge_swapped && (second_edge_checkpoint == FBE_LBA_INVALID))
            {
                rg_server_obj_info_p->downstream_checkpoint = 0;
            }
        }
    }

    else if (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)
    {
        /*! @note If the virtual drive is in mirror mode and the requesting class 
         *        is  the virtual drive start with the source checkpoint.  We need to
         *        return this so that the copy proceeds
         */
        if (request_type == FBE_RAID_GROUP_GET_CHECKPOINT_REQUEST_TYPE_CLEAR_RL)
        {
            /* If the request is clear rebuild logging and the the copy is
             * complete return the checkpoint for teh index requested.
             */
            if (b_is_copy_complete == FBE_TRUE)
            {
                /* If the requested edge is rebuild logging then return that
                 * checkpoint.
                 */
                rg_server_obj_info_p->downstream_checkpoint = edge_index_checkpoint;
            }
            else
            {
                /* Else return the source drive checkpoint.
                 */
                rg_server_obj_info_p->downstream_checkpoint = first_edge_checkpoint;
            }
        }
        else
        {
            /* Else if the request type is FBE_RAID_GROUP_GET_CHECKPOINT_REQUEST_TYPE_EVAL_MARK_NR
             * return the destination checkpoint.
             */
            rg_server_obj_info_p->downstream_checkpoint = second_edge_checkpoint;
        }
    }

    else if(configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE)
    {
        /*! @note If the virtual drive is in mirror mode and the requesting class 
         *        is  the virtual drive start with the source checkpoint.  We need to
         *        return this so that the copy proceeds
         */
        if (request_type == FBE_RAID_GROUP_GET_CHECKPOINT_REQUEST_TYPE_CLEAR_RL)
        {
            /* If the request is clear rebuild logging and the the copy is
             * complete return the checkpoint for teh index requested.
             */
            if (b_is_copy_complete == FBE_TRUE)
            {
                /* If the requested edge is rebuild logging then return that
                 * checkpoint.
                 */
                rg_server_obj_info_p->downstream_checkpoint = edge_index_checkpoint;
            }
            else
            {
                /* Else return the source drive checkpoint.
                 */
                rg_server_obj_info_p->downstream_checkpoint = second_edge_checkpoint;
            }
        }
        else
        {
            /* Else if the request type is FBE_RAID_GROUP_GET_CHECKPOINT_REQUEST_TYPE_EVAL_MARK_NR
             * return the destination checkpoint.
             */
            rg_server_obj_info_p->downstream_checkpoint = first_edge_checkpoint;
        }
    }

    /* If the virtual drive is in mirror mode always trace the information.
     * Otherwise only trace the information if enabled.
     */
    if ((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)  ||
        (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE) ||
        (b_is_source_failed_set == FBE_TRUE)                                        )
    {
        /* Always trace the information when the virtual drive is in mirror mode.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: get chkpt for vd - rl mask: 0x%x vd chkpt: 0x%llx first: 0x%llx second: 0x%llx\n",
                              rl_bitmask, (unsigned long long)rg_server_obj_info_p->downstream_checkpoint, (unsigned long long)first_edge_checkpoint, (unsigned long long)second_edge_checkpoint);
    }
    else
    {
        /* Else only trace if enabled.
         */
        fbe_virtual_drive_trace(virtual_drive_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_VIRTUAL_DRIVE_DEBUG_FLAG_RL_TRACING,
                                "virtual_drive: get chkpt for vd - rl mask: 0x%x vd chkpt: 0x%llx first: 0x%llx second: 0x%llx\n",
                                rl_bitmask, (unsigned long long)rg_server_obj_info_p->downstream_checkpoint, (unsigned long long)first_edge_checkpoint, (unsigned long long)second_edge_checkpoint);
    }

    return FBE_STATUS_OK;
}
/**********************************************************
 * end fbe_virtual_drive_get_checkpoint_for_virtual_drive()
 **********************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_set_swapped_bit_completion()
 ******************************************************************************
 * @brief
 *   This function is called when the non-paged metadata write to set the 
 *   rebuild checkpoint(s) to the logical end marker has completed.  It will 
 *   check the packet status and set the swapped bit in the non-paged.
 * 
 * @param in_packet_p   - pointer to a control packet from the scheduler
 * @param in_context    - completion context, which is a pointer to the raid 
 *                        group object
 * 
 * @return  fbe_status_t  
 *
 * @note    Completion - Return status
 *
 * @author
 *  05/01/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_set_swapped_bit_completion(fbe_packet_t *packet_p,
                                                          fbe_packet_completion_context_t in_context)

{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_status_t                        packet_status;
    fbe_virtual_drive_t                *virtual_drive_p;
    fbe_virtual_drive_configuration_mode_t configuration_mode;
    fbe_edge_index_t                    swap_in_edge_index;

    /* Get the virtual drive object.
     */
    virtual_drive_p = (fbe_virtual_drive_t *)in_context;
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    swap_in_edge_index = fbe_virtual_drive_get_swap_in_edge_index(virtual_drive_p);

    /* If success return `more processing'
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: set swapped bit compl mode: %d index: %d set checkpoint to end done\n",
                          configuration_mode, swap_in_edge_index);

    /* Check the packet status.  If the write had an error, then we'll leave 
     * the condition set so that we can try again.  Return success so that the 
     * next completion function on the stack gets called.
     */ 
    packet_status = fbe_transport_get_status_code(packet_p);
    if (packet_status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s failed - status: 0x%x\n", 
                              __FUNCTION__, packet_status);
        return FBE_STATUS_OK;
    }

    /* Now set the swapped flags
     */
    status = fbe_virtual_drive_metadata_set_edge_swapped(virtual_drive_p, packet_p);
    if (status != FBE_STATUS_MORE_PROCESSING_REQUIRED)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s set edge swapped failed - status: 0x%x\n",
                              __FUNCTION__, status);
    
        /* Fail the request.
         */
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    /* Return more processing required.
     */
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/************************************************************************************
 * end fbe_virtual_drive_set_swapped_bit_completion()
 ************************************************************************************/

/*!****************************************************************************
 *      fbe_virtual_drive_set_swapped_bit_write_checkpoint()
 ******************************************************************************
 * @brief
 *   This function will set the rebuild checkpoint to end marker
 * 
 * @param packet_p
 * @param context
 * 
 * @return  status - FBE_STATUS_OK - Previous request failed - complete packet
 *                   FBE_STATUS_MORE_PROCESSING_REQUIRED - Continue 
 *
 * @note    Completion - Just return status
 *
 * @author
 *  05/01/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_set_swapped_bit_write_checkpoint(fbe_packet_t *packet_p,                                    
                                                                       fbe_packet_completion_context_t context)

{
    fbe_status_t                            status = FBE_STATUS_OK;                 
    fbe_virtual_drive_t                    *virtual_drive_p = NULL;
    fbe_edge_index_t                        edge_index_to_set = FBE_EDGE_INDEX_INVALID;
    fbe_virtual_drive_configuration_mode_t  configuration_mode; 
    fbe_u32_t                               rebuild_index;
    fbe_u32_t                               rebuild_index_to_set = FBE_RAID_GROUP_INVALID_INDEX;   
    fbe_u64_t                               metadata_offset;
    fbe_u32_t                               metadata_write_size;               
    fbe_raid_group_nonpaged_metadata_t     *nonpaged_metadata_p = NULL;
    fbe_raid_group_rebuild_nonpaged_info_t  rebuild_nonpaged_info;
    fbe_raid_group_rebuild_nonpaged_info_t *rebuild_nonpaged_info_p = NULL;
    fbe_raid_group_rebuild_checkpoint_info_t *rebuild_checkpoint_info_p = NULL;
    
    /* Get the virtual drive object.
     */
    virtual_drive_p = (fbe_virtual_drive_t *)context;

    /* If the status is not ok, that means we didn't get the 
     * lock. Just return. we are already in the completion routine
     */
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get the NP lock, status: 0x%x", 
                              __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    /* Set the completion function to release the NP lock.
     * Once we set the completion we must:
     *  o Complete the packet
     *  o Return `more processing required'
     */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_release_NP_lock, virtual_drive_p);

    /* Use the virtual drive specific method to locate the disk that needs it
     * rebuild checkpoint set.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    edge_index_to_set = fbe_virtual_drive_get_swap_in_edge_index(virtual_drive_p);

    /* Trace some information if enabled.
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_SWAP_TRACING,
                            "virtual_drive: set swapped write chkpts mode: %d index to set: %d\n",
                            configuration_mode, edge_index_to_set);

    /* If we could not locate the edge that needs to have the checkpoint set
     * it is an error.
     */
    if (edge_index_to_set == FBE_EDGE_INDEX_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s could not locate edge index to set checkpoint for.", __FUNCTION__); 

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Get a pointer to the non-paged metadata.
     */ 
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)virtual_drive_p, (void **)&nonpaged_metadata_p);

    /* Copy the existing data to the structure we will write.
     */
    rebuild_nonpaged_info_p = &rebuild_nonpaged_info;
    *rebuild_nonpaged_info_p = *(&(nonpaged_metadata_p->rebuild_info));

    /* Locate the rebuild index for the edge (position)
     */
    for (rebuild_index = 0; rebuild_index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; rebuild_index++)
    {
        if (rebuild_nonpaged_info_p->rebuild_checkpoint_info[rebuild_index].position == edge_index_to_set)
        {
            rebuild_index_to_set = rebuild_index;
            break;
        }
    }

    /* If we could not locate the index something went wrong.
     */
    if (rebuild_index_to_set == FBE_RAID_GROUP_INVALID_INDEX)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s could not locate rebuild index to set checkpoint for.", __FUNCTION__); 

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Set the destination end marker
     */
    rebuild_checkpoint_info_p = &rebuild_nonpaged_info_p->rebuild_checkpoint_info[rebuild_index_to_set];
    rebuild_checkpoint_info_p->checkpoint = FBE_LBA_INVALID;
    rebuild_checkpoint_info_p->position = FBE_RAID_INVALID_DISK_POSITION;

    /* Now generate the write size and offset
     */
    metadata_write_size = sizeof(fbe_raid_group_rebuild_checkpoint_info_t); 
    metadata_offset = (fbe_u64_t) 
            (&((fbe_raid_group_nonpaged_metadata_t*)0)->rebuild_info.rebuild_checkpoint_info[rebuild_index_to_set]);

    /* Now send the write for the nonpaged.
     */ 
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *)virtual_drive_p, 
                                                             packet_p, 
                                                             metadata_offset, 
                                                             (fbe_u8_t *)rebuild_checkpoint_info_p,
                                                             metadata_write_size);

    /* Return more processing since a packet is outstanding.
     */
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/***************************************************************************
 * end fbe_virtual_drive_set_swapped_bit_write_checkpoint()
 ***************************************************************************/ 

/*!****************************************************************************
 *          fbe_virtual_drive_set_swapped_bit_set_checkpoint_to_end_marker()
 ******************************************************************************
 *
 * @brief   This method is called to handle the `set swapped' request AND a
 *          previous copy operation both the source and destination drive
 *          failed.  We must set the checkpoint to the end marker.
 *
 * @param   virtual_drive_p - Pointer to virtual drive object to set checkpoint for
 * @param   packet_p               - pointer to a control packet from the scheduler
 *
 * @return  status - Typically FBE_STATUS_MORE_PROCESSING_REQUIRED
 *
 * @note    Called - Must complete packet
 *
 * @author
 *  05/01/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_set_swapped_bit_set_checkpoint_to_end_marker(fbe_virtual_drive_t *virtual_drive_p,
                                                                            fbe_packet_t *packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_edge_index_t                        edge_index_to_set = FBE_EDGE_INDEX_INVALID;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;

    /* Trace the function entry
     */
    fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p,     
                          FBE_TRACE_LEVEL_DEBUG_HIGH,          
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                          "%s entry\n", __FUNCTION__); 

    /* Get the configuration mode for debug purposes.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    edge_index_to_set = fbe_virtual_drive_get_swap_in_edge_index(virtual_drive_p);

    /* Always trace some information
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_SWAP_TRACING,
                            "virtual_drive: set swapped bit mode: %d edge to set: %d\n",
                            configuration_mode, edge_index_to_set);

    /*! @note There should be no I/O going thru the failed virtual drive so 
     *        we do not need and should not quiesce.
     */
    /* Set the rebuild checkpoint to the end marker on the permanent spare.
     */
    fbe_transport_set_completion_function(packet_p, 
                                          fbe_virtual_drive_metadata_set_checkpoint_to_end_marker_completion, 
                                          virtual_drive_p);    
    status = fbe_raid_group_get_NP_lock((fbe_raid_group_t *)virtual_drive_p, 
                                        packet_p, 
                                        fbe_virtual_drive_set_swapped_bit_write_checkpoint);
        
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

}
/*************************************************************************************
 * end fbe_virtual_drive_set_swapped_bit_set_checkpoint_to_end_marker()
 *************************************************************************************/


/*******************************
 * end fbe_virtual_drive_main.c
 *******************************/
