/*@LB************************************************************************
 ****************************************************************************
 ****************************************************************************
 ****
 ****  Copyright (c) 2006 EMC Corporation.
 ****  All rights reserved.
 ****
 ****  ALL RIGHTS ARE RESERVED BY EMC CORPORATION.
 ****  ACCESS TO THIS SOURCE CODE IS STRICTLY RESTRICTED UNDER CONTRACT.
 ****  THIS CODE IS TO BE KEPT STRICTLY CONFIDENTIAL.
 ****
 ****************************************************************************
 ****************************************************************************
 *@LE************************************************************************/

/*@FB************************************************************************
 ****************************************************************************
 ****************************************************************************
 ****
 **** FILE: fbe_topology_main.c
 ****
 **** DESCRIPTION: 
 **** NOTES:
 ****    <TBS>
 ****
 ****************************************************************************
 ****************************************************************************
 *@FE************************************************************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_rwlock.h"

#include "fbe_base.h"
#include "fbe_base_object.h"
#include "base_object_private.h" /* we need it for get_class_id function */
#include "fbe_topology.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_base_service.h"
#include "fbe_transport_memory.h"
#include "fbe_notification.h"
#include "fbe_topology_private.h"
#include "fbe/fbe_provision_drive.h"
#include "fbe/fbe_base_config.h"
#include "fbe/fbe_port.h"
#include "fbe/fbe_enclosure.h"
#include "fbe_system_limits.h"
#include "fbe_database.h"


typedef struct fbe_topology_service_s{
    fbe_base_service_t  base_service;
}fbe_topology_service_t;

typedef struct fbe_topology_statistics_s {
    fbe_u32_t number_of_created_objects;
    fbe_u32_t number_of_destroyed_objects;
}fbe_topology_statistics_t;

static fbe_io_entry_function_t physical_package_io_entry = NULL;
static fbe_io_entry_function_t sep_io_entry = NULL;

static fbe_topology_service_t topology_service;
static fbe_topology_statistics_t topology_statistics;
static fbe_topology_send_event_function_t topology_event_table[FBE_PACKAGE_ID_LAST]; /* Events to different packages will ve redirected */

/* Declare our service methods */
static fbe_status_t fbe_topology_control_entry(fbe_packet_t * packet); /* Note: remove it from fbe_topology.h */
fbe_service_methods_t fbe_topology_service_methods = {FBE_SERVICE_ID_TOPOLOGY, fbe_topology_control_entry};

static const fbe_class_methods_t ** topology_class_table = NULL;

/* Forward declaration */
static fbe_status_t topology_clean_all (fbe_packet_t * packet);
/*static fbe_status_t topology_list_all_objects (fbe_packet_t * packet); */
static fbe_status_t topology_get_statistics (fbe_packet_t * packet);
static fbe_status_t topology_send_mgmt_packet(fbe_object_id_t object_id, fbe_packet_t * packet);
static fbe_status_t topology_enumerate_objects(fbe_packet_t *  packet);
static fbe_status_t topology_get_object_type(fbe_packet_t *  packet);
static fbe_status_t topology_get_statistics(fbe_packet_t *  packet);

static fbe_status_t topology_get_provision_drive_by_location(fbe_packet_t *  packet);
static fbe_status_t topology_get_enclosure_by_location(fbe_packet_t *  packet);
static fbe_status_t topology_get_physical_drive_by_location(fbe_packet_t *  packet);
static fbe_status_t topology_get_port_by_location(fbe_packet_t *  packet);
static fbe_status_t topology_get_port_by_location_and_role(fbe_packet_t *  packet);
static fbe_status_t topology_enumerate_class(fbe_packet_t *  packet);
static fbe_status_t topology_get_provision_drive_location(fbe_packet_t * packet,
        fbe_object_id_t object_id,
        fbe_physical_drive_get_location_t *drive_location_p);

static fbe_status_t topology_get_provision_drive_upstream_vd_object_id(fbe_packet_t * packet,
                                                                       fbe_object_id_t pvd_object_id,
                                                                       fbe_bool_t b_is_system_drive,
                                                                       fbe_object_id_t *vd_object_id_p);

static fbe_status_t topology_get_info_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t topology_get_port_number(fbe_packet_t * packet, fbe_object_id_t object_id, fbe_u32_t *port_number);
static fbe_status_t topology_get_port_role(fbe_packet_t * packet, fbe_object_id_t object_id, fbe_port_role_t *port_role);

static fbe_status_t topology_class_command(fbe_class_id_t class_id, fbe_packet_t *  packet);
static fbe_status_t topology_get_physical_drive_objects(fbe_packet_t *  packet);


static fbe_status_t topology_get_board(fbe_packet_t *  packet);
static fbe_status_t topology_enumerate_ports_by_role(fbe_packet_t *  packet);
static fbe_status_t topology_set_config_pvd_hs_flag(fbe_packet_t *  packet);
static fbe_status_t topology_get_config_pvd_hs_flag(fbe_packet_t *  packet);
static fbe_status_t topology_update_disable_spare_select_unconsumed(fbe_packet_t *  packet);
static fbe_status_t topology_get_disable_spare_select_unconsumed(fbe_packet_t *  packet);


static fbe_atomic_t fbe_topology_set_gate(fbe_object_id_t id);

/* Note: We should use object id as an index */
static fbe_u32_t max_supported_objects = 0;
static topology_object_table_entry_t *topology_object_table = NULL; 
/* static fbe_rwlock_t topology_object_table_lock; */
static fbe_spinlock_t topology_object_table_lock;

/*! @note For test purposes, we allow only drive configure as `test spare' to 
 *        be selected.  By default this flag is False.
 */
static fbe_bool_t topology_b_select_only_test_spare_drives = FBE_FALSE;     /*! @note The default must be `False'.*/

/*! @note For test purposes, we allow the selection of `unconsumed' spare 
 *        drives to be disabled.
 */
static fbe_bool_t topology_b_disable_spare_select_unconsumed = FBE_FALSE;    /*! @note The default must be `False'.*/

/* Forward declaration */
static const fbe_class_methods_t * topology_get_class(fbe_class_id_t class_id);
static fbe_status_t topology_allocate_object_id(fbe_object_id_t * object_id);
static fbe_status_t topology_send_mgmt_packet_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t topology_check_object_existent(fbe_packet_t *  packet);

static __forceinline void topology_set_control_handle(fbe_object_id_t id, fbe_object_handle_t control_handle);    
static __forceinline void topology_set_io_handle(fbe_object_id_t id, fbe_object_handle_t control_handle);    
static __forceinline void topology_set_object_ready(fbe_object_id_t id);
static __forceinline void topology_set_object_exist(fbe_object_id_t id);
static __forceinline fbe_status_t topology_set_object_reserved(fbe_object_id_t id);
static __forceinline void topology_set_object_not_exist(fbe_object_id_t id);

static __forceinline fbe_status_t topology_get_object_class_id(fbe_object_id_t id, fbe_class_id_t * class_id);
static __forceinline fbe_bool_t topology_is_object_ready(fbe_object_id_t id);
static __forceinline fbe_bool_t topology_is_object_exist(fbe_object_id_t id);
static __forceinline fbe_bool_t topology_is_object_reserved(fbe_object_id_t id);
static __forceinline fbe_bool_t topology_is_object_not_exist(fbe_object_id_t id);
static __forceinline fbe_u32_t topology_is_valid_object_id(fbe_object_id_t id);

static fbe_status_t fbe_topology_control_entry(fbe_packet_t * packet);

static fbe_status_t
topology_get_enclosure_info(fbe_packet_t * packet,
                            fbe_object_id_t object_id, 
                            fbe_u32_t *port,
                            fbe_u32_t *enclosure, 
                            fbe_u32_t *component_id);


static fbe_status_t
topology_get_physical_drive_info(fbe_packet_t * packet,
                                 fbe_object_id_t object_id, 
                                 fbe_u32_t *port,
                                 fbe_u32_t *enclosure,
                                 fbe_u32_t *slot);



static fbe_status_t topology_register_event_entry(void);
static fbe_status_t topology_register_event_entry_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t topology_unregister_event_entry(void);
static fbe_status_t topology_unregister_event_entry_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t topology_register_event_function(fbe_packet_t *  packet);
static fbe_status_t topology_unregister_event_function(fbe_packet_t *  packet);

static fbe_status_t topology_get_spare_drive_pool(fbe_packet_t *  packet);

static fbe_status_t topology_get_provision_drive_info(fbe_packet_t * packet,
                                                      fbe_object_id_t object_id,
                                                      fbe_provision_drive_info_t *provision_drive_info_p);


static fbe_status_t fbe_topology_get_total_objects(fbe_packet_t *  packet);

static fbe_status_t topology_get_physical_logical_drive_by_sn(fbe_packet_t *  packet);
static fbe_status_t topology_get_physical_drive_by_sn(fbe_packet_t *  packet);

static fbe_status_t fbe_topology_get_total_objects_of_class(fbe_packet_t *  packet);

static fbe_status_t topology_get_physical_drive_cached_info(fbe_packet_t *packet,
                                                            fbe_object_id_t obj_id,
                                                            fbe_physical_drive_mgmt_get_drive_information_t *get_info);

static fbe_status_t topology_get_bvd_id(fbe_packet_t *  packet);

static fbe_status_t topology_get_object_id_of_singleton_class(fbe_packet_t *  packet);

static fbe_status_t topology_get_pvd_by_location_from_non_paged(fbe_packet_t *  packet);

static fbe_status_t topology_get_provision_drive_location_from_non_paged(fbe_packet_t * packet,
        fbe_object_id_t object_id,
        fbe_provision_drive_get_physical_drive_location_t *drive_location_p);

static __forceinline fbe_u32_t topology_is_valid_object_id(fbe_object_id_t id)
{
    if(id < max_supported_objects){
        return 1;
    } else {
        return 0;
    }
}

static __forceinline fbe_status_t 
topology_get_object_class_id(fbe_object_id_t id, fbe_class_id_t * class_id)
{
    if(!topology_is_object_ready(id)){
        *class_id = FBE_CLASS_ID_INVALID;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(topology_object_table[id].control_handle == topology_object_table[id].io_handle){
        *class_id = fbe_base_object_get_class_id(topology_object_table[id].control_handle);
    } else {
        *class_id  = FBE_CLASS_ID_VERTEX;
    }

    if(*class_id != FBE_CLASS_ID_INVALID) {
        return FBE_STATUS_OK;
    } else {
        return FBE_STATUS_GENERIC_FAILURE;
    }
}

static __forceinline void topology_set_control_handle(fbe_object_id_t id, fbe_object_handle_t control_handle)
{
    topology_object_table[id].control_handle = control_handle; 
}

static __forceinline fbe_object_handle_t topology_get_control_handle(fbe_object_id_t id)
{
    return topology_object_table[id].control_handle; 
}

static __forceinline void topology_set_io_handle(fbe_object_id_t id, fbe_object_handle_t io_handle)
{
    topology_object_table[id].io_handle = io_handle; 
}

static __forceinline fbe_object_handle_t topology_get_io_handle(fbe_object_id_t id)
{
    return topology_object_table[id].io_handle; 
}

static __forceinline void topology_set_object_exist(fbe_object_id_t id)
{
    topology_object_table[id].object_status = FBE_TOPOLOGY_OBJECT_STATUS_EXIST;
}

static __forceinline void topology_set_object_ready(fbe_object_id_t id)
{
    topology_object_table[id].object_status = FBE_TOPOLOGY_OBJECT_STATUS_READY;
}

static __forceinline fbe_bool_t topology_is_object_exist(fbe_object_id_t id)
{
    return (topology_object_table[id].object_status == FBE_TOPOLOGY_OBJECT_STATUS_EXIST);
}

static __forceinline fbe_bool_t topology_is_object_ready(fbe_object_id_t id)
{
    return (topology_object_table[id].object_status == FBE_TOPOLOGY_OBJECT_STATUS_READY);
}

static __forceinline fbe_status_t topology_set_object_reserved(fbe_object_id_t id)    
{
    if(topology_object_table[id].object_status == FBE_TOPOLOGY_OBJECT_STATUS_NOT_EXIST)
    {
        topology_object_table[id].object_status = FBE_TOPOLOGY_OBJECT_STATUS_RESERVED;
        return FBE_STATUS_OK;
    }else {
        return FBE_STATUS_GENERIC_FAILURE;
    }
}

static __forceinline fbe_bool_t topology_is_object_reserved(fbe_object_id_t id)
{
    return (topology_object_table[id].object_status == FBE_TOPOLOGY_OBJECT_STATUS_RESERVED);
}

static __forceinline void topology_set_object_not_exist(fbe_object_id_t id)
{
    topology_object_table[id].object_status = FBE_TOPOLOGY_OBJECT_STATUS_NOT_EXIST;
}

static __forceinline fbe_bool_t topology_is_object_not_exist(fbe_object_id_t id)
{
    return (topology_object_table[id].object_status == FBE_TOPOLOGY_OBJECT_STATUS_NOT_EXIST);
}

static const fbe_class_methods_t * 
topology_get_class(fbe_class_id_t class_id)
{   
    const fbe_class_methods_t * pclass = NULL;
    fbe_bool_t b_class_found = FBE_FALSE;
    fbe_u32_t i;

    for(i = 0; topology_class_table[i] != NULL; i++)
    {
        pclass = topology_class_table[i];
        if(pclass->class_id == class_id) {
            b_class_found = FBE_TRUE;
            break;
        }
    }
    return ((b_class_found == FBE_TRUE) ? pclass : NULL);
}

static fbe_status_t 
fbe_topology_init(fbe_packet_t * packet)
{
    fbe_status_t status;
    const fbe_class_methods_t * pclass; /* pointer to cbe class */
    fbe_u32_t i;
    fbe_object_id_t id;
    fbe_package_id_t my_package_id;

    fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s entry\n", __FUNCTION__);

    /*figure out how many we need*/
    fbe_get_package_id(&my_package_id);
    status = fbe_system_limits_get_max_objects(my_package_id, &max_supported_objects);
    if (status != FBE_STATUS_OK) {
         fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                               "%s unable to allocate memory for objects table\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    topology_object_table = (topology_object_table_entry_t *)fbe_memory_ex_allocate(max_supported_objects * sizeof(topology_object_table_entry_t));

    if(topology_class_table == NULL){
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                               "%s topology_class_table not initialized\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Mark the service as initialized. */
    fbe_base_service_init((fbe_base_service_t *)&topology_service);

    /* Set the service id.*/
     fbe_base_service_set_service_id((fbe_base_service_t *)&topology_service, FBE_SERVICE_ID_TOPOLOGY);

    /* Set the trace level to the default. */
    fbe_base_service_set_trace_level(&topology_service.base_service, 
                                     fbe_trace_get_default_trace_level());

    /* Zero topology_object_table */
    fbe_zero_memory(topology_object_table, max_supported_objects * sizeof(topology_object_table_entry_t));

    /* Set status of all objects to FBE_TOPOLOGY_OBJECT_STATUS_NOT_EXIST */
    for(id = 0; id < max_supported_objects ; id ++) {
        topology_set_object_not_exist(id);

        /* The default state of the gate is ON so that I/Os cannot arrive.
         * We clear it when we are created and set it when we are destroyed.  
         */
        fbe_topology_set_gate(id);
    }

    /* Clear topology statistics */
    topology_statistics.number_of_created_objects = 0;
    topology_statistics.number_of_destroyed_objects = 0;

    /* fbe_rwlock_init(&topology_object_table_lock); */
    fbe_spinlock_init(&topology_object_table_lock);

    /* We probably should test here if mem_pool is ready */

    /* Load all classes */
    /* We are running over all topology_class_table and loading all classes */
    for(i = 0; topology_class_table[i] != NULL; i++)
    {
        pclass = topology_class_table[i];
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_DEBUG_LOW, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s load class %X\n",
                               __FUNCTION__, pclass->class_id);

        status = pclass->load();
        if (status != FBE_STATUS_OK) {
            fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                                   FBE_TRACE_LEVEL_ERROR, 
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s load failed, class: %X, status: %X\n",
                                   __FUNCTION__, pclass->class_id, status);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }        
    }
 
    /* Zero event table */
    fbe_zero_memory(topology_event_table, sizeof(topology_event_table));

    if(my_package_id == FBE_PACKAGE_ID_SEP_0){
        topology_register_event_entry();
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_topology_destroy(fbe_packet_t * packet)
{
    fbe_status_t        status;
    const               fbe_class_methods_t * pclass = NULL; /* pointer to class */
    fbe_object_id_t     id;
    fbe_u32_t           i;
    fbe_package_id_t    my_package_id;
 
    fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s entry\n", __FUNCTION__);

    fbe_base_service_destroy((fbe_base_service_t *) &topology_service);
       
    /* Check if there is no object left behind */
    for(id = 0; id < max_supported_objects ; id++) {
        if(topology_is_object_ready(id)) {
            fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                                   FBE_TRACE_LEVEL_ERROR, 
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s object_id %X is not destroyed \n",
                                   __FUNCTION__, id);
            fbe_transport_set_status(packet, FBE_STATUS_BUSY, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_BUSY;
        }
    }    

    /* fbe_rwlock_destroy(&topology_object_table_lock); */
    fbe_spinlock_destroy(&topology_object_table_lock);

    /* It is time to die */
    for(i = 0; topology_class_table[i] != NULL; i++)
    {
        pclass = topology_class_table[i];
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_DEBUG_LOW, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s unload class %X\n",
                               __FUNCTION__, pclass->class_id);
        status = pclass->unload();
        if (status != FBE_STATUS_OK) {
            fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                                   FBE_TRACE_LEVEL_ERROR, 
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s unload failed, class: %X, status: %X\n",
                                   __FUNCTION__, pclass->class_id, status);
        }        
    } 

    fbe_get_package_id(&my_package_id);
    if(my_package_id == FBE_PACKAGE_ID_SEP_0){
        topology_unregister_event_entry();
    }


    fbe_memory_ex_release(topology_object_table);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

void
fbe_topology_class_trace(fbe_u32_t class_id,
                         fbe_trace_level_t trace_level,
                         fbe_u32_t message_id,
                         const fbe_char_t * fmt, ...)
{
    fbe_trace_level_t default_trace_level;
    va_list argList;

    default_trace_level = fbe_trace_get_default_trace_level();
    if (trace_level > default_trace_level) {
        return;
    }

    va_start(argList, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_CLASS,
                     class_id,
                     trace_level,
                     message_id,
                     fmt, 
                     argList);
    va_end(argList);
}

fbe_status_t 
fbe_topology_init_class_table(const fbe_class_methods_t * class_table[])
{
    const fbe_class_methods_t *** topology_class_table_ptr = &topology_class_table;

    fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s entry\n", __FUNCTION__);

    *topology_class_table_ptr = class_table;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_topology_get_class_table(const fbe_class_methods_t ** class_table[])
{
    const fbe_class_methods_t *** topology_class_table_ptr = &topology_class_table; 

    fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s entry\n", __FUNCTION__);

    *class_table = *topology_class_table_ptr;

    return FBE_STATUS_OK;
}

enum fbe_topology_gate_e{
    FBE_TOPOLOGY_GATE_BIT   = 0x10000000,
    FBE_TOPOLOGY_GATE_MASK  = 0x0FFFFFFF,
};
static fbe_atomic_t fbe_topology_set_gate(fbe_object_id_t id)
{
    return fbe_atomic_or(&topology_object_table[id].reference_count, FBE_TOPOLOGY_GATE_BIT);
}
fbe_bool_t fbe_topology_is_reference_count_zero(fbe_object_id_t id)
{
    fbe_atomic_t ref_count;
    ref_count = fbe_topology_set_gate(id);
    return((ref_count & FBE_TOPOLOGY_GATE_MASK) == 0);
}
void fbe_topology_clear_gate(fbe_object_id_t id)
{
    fbe_atomic_and(&topology_object_table[id].reference_count, ~FBE_TOPOLOGY_GATE_BIT);
}
fbe_status_t 
fbe_topology_release_control_handle(fbe_object_id_t id)
{
    fbe_atomic_decrement(&topology_object_table[id].reference_count);
    return FBE_STATUS_OK;
}
fbe_object_handle_t 
fbe_topology_get_control_handle_atomic(fbe_object_id_t id)
{
    fbe_atomic_t io_gate;
   if(!topology_is_valid_object_id(id)){
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_WARNING, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid id 0x%X\n", __FUNCTION__, id);
        return NULL;
    }
    
    if(!topology_is_object_ready(id)) {
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_WARNING, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s object not ready, id: 0x%X\n", __FUNCTION__, id);
        return NULL;
    }
    io_gate = fbe_atomic_increment(&topology_object_table[id].reference_count);
    if(!(io_gate & FBE_TOPOLOGY_GATE_BIT)){
        return topology_object_table[id].control_handle;
    } else {
        /* We do have a gate, so decrement the I/O count and go to the old code */
        fbe_atomic_decrement(&topology_object_table[id].reference_count);
        return NULL;
    }
}

fbe_object_handle_t 
fbe_topology_get_control_handle(fbe_object_id_t id)
{
   if(!topology_is_valid_object_id(id)){
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid id %X\n", __FUNCTION__, id);
        return NULL;
    }
    
    if(!topology_is_object_ready(id)) {
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s object not ready, id: %X\n", __FUNCTION__, id);
        return NULL;
    }
    return topology_object_table[id].control_handle;
}

fbe_object_handle_t 
fbe_topology_get_io_handle(fbe_object_id_t id)
{
   if(!topology_is_valid_object_id(id)){
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid id %X\n", __FUNCTION__, id);
        return NULL;
    }
    
    if(!topology_is_object_ready(id)) {
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s object not ready, id: %X\n", __FUNCTION__, id);
        return NULL;
    }
    
    return topology_object_table[id].io_handle;
}

static fbe_status_t 
topology_allocate_object_id(fbe_object_id_t * object_id)
{
    fbe_object_id_t id;
    fbe_package_id_t my_package_id;

    fbe_get_package_id(&my_package_id);

    /* Find the id to start searching from */
    if(my_package_id == FBE_PACKAGE_ID_SEP_0)
    {
        id = FBE_RESERVED_OBJECT_IDS;
    }
    else
    {
        id = 0;
    }
    
    for(; id < max_supported_objects ; id ++) {
        if(topology_is_object_not_exist(id)) {
           /*topology_set_object_reserved(id);*/
            *object_id = id;
           return FBE_STATUS_OK;
        }
    }
    return FBE_STATUS_GENERIC_FAILURE;
}

fbe_status_t
topology_initialize_object(fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_object_mgmt_create_object_t * object_attr = NULL;
    fbe_object_handle_t  control_handle;
    const fbe_class_methods_t * pclass; /* pointer to cbe class */
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &object_attr); 

    fbe_spinlock_lock(&topology_object_table_lock);
    if(!topology_is_valid_object_id(object_attr->object_id)){
        fbe_spinlock_unlock(&topology_object_table_lock);

        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s invalid object id %X\n",
                               __FUNCTION__, object_attr->object_id);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if(! topology_is_object_not_exist(object_attr->object_id)) {
        fbe_spinlock_unlock(&topology_object_table_lock);

        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s invalid object id %X, already exists\n",
                               __FUNCTION__, object_attr->object_id);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    topology_set_object_reserved(object_attr->object_id);
    pclass = topology_get_class(object_attr->class_id);
    if(pclass == NULL){
        topology_set_object_not_exist(object_attr->object_id);
        fbe_spinlock_unlock(&topology_object_table_lock);

        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_DEBUG_LOW, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s invalid class id %X\n",
                               __FUNCTION__, object_attr->class_id);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_unlock(&topology_object_table_lock);
    /* fbe_rwlock_write_lock(&topology_object_table_lock); */
    pclass->create_object(packet, &control_handle); 
        
    fbe_spinlock_lock(&topology_object_table_lock);
        
    status = fbe_transport_get_status_code(packet);
    if(status == FBE_STATUS_OK) {        
        topology_set_control_handle(object_attr->object_id, control_handle);
        topology_set_io_handle(object_attr->object_id, control_handle);
        if(object_attr->class_id == FBE_CLASS_ID_VERTEX) {
            topology_set_io_handle(object_attr->object_id, NULL);
        }
        topology_set_object_ready(object_attr->object_id);
        topology_statistics.number_of_created_objects++;
        /* fbe_notification_send(object_attr->object_id, FBE_NOTIFICATION_TYPE_OBJECT_CREATED); */
    } else {
        topology_set_object_not_exist(object_attr->object_id);
    }
    /* fbe_rwlock_write_unlock(&topology_object_table_lock); */
    fbe_spinlock_unlock(&topology_object_table_lock);

    fbe_transport_complete_packet(packet);
    return status;
}

fbe_status_t
topology_create_object(fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_object_mgmt_create_object_t * object_attr = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    const fbe_class_methods_t * pclass; /* pointer to cbe class */
    fbe_object_handle_t  control_handle;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &object_attr); 
    
    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len < sizeof(fbe_base_object_mgmt_create_object_t)){
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid io_out_size_requested %X\n",
                               __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock(&topology_object_table_lock);
    if(object_attr->object_id == FBE_OBJECT_ID_INVALID){
    status = topology_allocate_object_id(&object_attr->object_id);
        if(status == FBE_STATUS_GENERIC_FAILURE){
            fbe_spinlock_unlock(&topology_object_table_lock);
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s topology_allocate_object_id fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else
    {
        if(!topology_is_valid_object_id(object_attr->object_id)){
            fbe_spinlock_unlock(&topology_object_table_lock);

            fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                                   FBE_TRACE_LEVEL_ERROR, 
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s invalid object id %X\n",
                                   __FUNCTION__, object_attr->object_id);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        
        if((topology_is_object_exist(object_attr->object_id)) ||
           (topology_is_object_ready(object_attr->object_id))) {
            fbe_spinlock_unlock(&topology_object_table_lock);

            fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                                   FBE_TRACE_LEVEL_ERROR, 
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s invalid object id %X, already exists\n",
                                   __FUNCTION__, object_attr->object_id);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    topology_set_object_reserved(object_attr->object_id);
    pclass = topology_get_class(object_attr->class_id);
    if(pclass == NULL){
        topology_set_object_not_exist(object_attr->object_id);
        fbe_spinlock_unlock(&topology_object_table_lock);

        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_DEBUG_LOW, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s invalid class id %X\n",
                               __FUNCTION__, object_attr->class_id);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_unlock(&topology_object_table_lock);

    /* fbe_rwlock_write_lock(&topology_object_table_lock); */
    pclass->create_object(packet, &control_handle); 
        
    fbe_spinlock_lock(&topology_object_table_lock);
    status = fbe_transport_get_status_code(packet);
    if(status == FBE_STATUS_OK) {        
        topology_set_control_handle(object_attr->object_id, control_handle);
        topology_set_io_handle(object_attr->object_id, control_handle);
        if(object_attr->class_id == FBE_CLASS_ID_VERTEX) {
            topology_set_io_handle(object_attr->object_id, NULL);
        }
        topology_set_object_ready(object_attr->object_id);
        topology_statistics.number_of_created_objects++;
        /* fbe_notification_send(object_attr->object_id, FBE_NOTIFICATION_TYPE_OBJECT_CREATED); */
    } else {
        topology_set_object_not_exist(object_attr->object_id);
    }
    /* fbe_rwlock_write_unlock(&topology_object_table_lock); */
    fbe_spinlock_unlock(&topology_object_table_lock);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

fbe_status_t 
cbe_topology_get_object_class_id(fbe_object_id_t id, fbe_class_id_t * class_id)
{ 
    if(!topology_is_valid_object_id(id)){
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid object id %X\n",
                               __FUNCTION__, id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    return topology_get_object_class_id(id, class_id);
}

fbe_status_t 
topology_destroy_object(fbe_packet_t * packet)
{
    fbe_object_id_t object_id;
    const fbe_class_methods_t * pclass; /* pointer to cbe class */
    fbe_class_id_t class_id;
    fbe_object_handle_t control_handle;
    fbe_status_t mgmt_status;
    fbe_topology_mgmt_destroy_object_t * topology_mgmt_destroy_object = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &topology_mgmt_destroy_object); 
    object_id = topology_mgmt_destroy_object->object_id;

    fbe_spinlock_lock(&topology_object_table_lock);
    if(!topology_is_valid_object_id(object_id)){
        fbe_spinlock_unlock(&topology_object_table_lock);
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid object id %X\n", __FUNCTION__, object_id);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
  
    mgmt_status = topology_get_object_class_id(object_id, &class_id);
    if(mgmt_status != FBE_STATUS_OK){
        fbe_spinlock_unlock(&topology_object_table_lock);
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s can't get object class id %X\n", __FUNCTION__, object_id);
        fbe_transport_set_status(packet, mgmt_status, 0);
        fbe_transport_complete_packet(packet);
        return mgmt_status;
    }

    pclass = topology_get_class(class_id);
    if(pclass == NULL){
        fbe_spinlock_unlock(&topology_object_table_lock);
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s topology_get_class fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* This is for the cases where we did not set gate already like physical package. 
     * SEP has already set the gate by this point. 
     */
    fbe_topology_set_gate(object_id);
    control_handle = fbe_topology_get_control_handle(object_id);
    /* We have to make sure that object IS NOT connected */

    topology_set_object_not_exist(object_id);
    
    topology_statistics.number_of_destroyed_objects++;
    fbe_spinlock_unlock(&topology_object_table_lock);

    /* fbe_rwlock_write_lock(&topology_object_table_lock); */

    mgmt_status =  pclass->destroy_object(control_handle); 
    if(mgmt_status != FBE_STATUS_OK){

        int i;

        for (i = 1; i <= 10; i++) {
            fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                                   FBE_TRACE_LEVEL_ERROR, 
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s destroy object (%p) failure %d, retry %d\n",
                                   __FUNCTION__, (void *)control_handle,
                                   (int)mgmt_status, i);
            fbe_thread_delay(200);

            mgmt_status =  pclass->destroy_object(control_handle); 
            if (FBE_STATUS_OK == mgmt_status){
                break;
            }
        }
        if (FBE_STATUS_OK != mgmt_status){
            fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                                   FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s destroy object (%p) failed, giving up\n",
                                   __FUNCTION__, (void *)control_handle);
            /* We failed to destroy object - it means he exist */
            /* topology_set_object_ready(object_id); */
            fbe_transport_set_status(packet, mgmt_status, 0);
            fbe_transport_complete_packet(packet);
            return mgmt_status;
        }
    }

    /* fbe_rwlock_write_unlock(&topology_object_table_lock); */

    /* fbe_notification_send(object_id, FBE_NOTIFICATION_TYPE_OBJECT_DESTROYED); */

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_topology_control_entry(fbe_packet_t * packet)
{    
    fbe_status_t status;
    fbe_object_id_t object_id;
    fbe_class_id_t class_id;
    fbe_payload_control_operation_opcode_t control_code;

    control_code = fbe_transport_get_control_code(packet);
    if(control_code == FBE_BASE_SERVICE_CONTROL_CODE_INIT) {
        status = fbe_topology_init(packet);
        return status;
    }

    if(!fbe_base_service_is_initialized((fbe_base_service_t *) &topology_service)){
        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_NOT_INITIALIZED;
    }

    /* Get object_id */
    status = fbe_transport_get_object_id(packet, &object_id);
    /* If object_id is valid - route packet to the object */
    if(object_id != FBE_OBJECT_ID_INVALID){
        return topology_send_mgmt_packet(object_id, packet);
    }

    /* Get class_id */
    status = fbe_transport_get_class_id(packet, &class_id);
    /* If class_id is valid - route packet to the class */
    if(class_id != FBE_CLASS_ID_INVALID){
        return topology_class_command(class_id, packet);
    }

    switch(control_code) {
        case FBE_TOPOLOGY_CONTROL_CODE_CREATE_OBJECT:
            status = topology_create_object( packet);
            break;

        case FBE_TOPOLOGY_CONTROL_CODE_DESTROY_OBJECT:
            status = topology_destroy_object( packet);
            break;

        case FBE_TOPOLOGY_CONTROL_CODE_ENUMERATE_OBJECTS:
            status = topology_enumerate_objects( packet);
            break;
        case FBE_TOPOLOGY_CONTROL_CODE_GET_OBJECT_TYPE:
            status = topology_get_object_type( packet);
            break;
        case FBE_TOPOLOGY_CONTROL_CODE_GET_STATISTICS:
            status = topology_get_statistics(packet);
            break;
        case FBE_TOPOLOGY_CONTROL_CODE_GET_DRIVE_BY_LOCATION:
            fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                                   FBE_TRACE_LEVEL_WARNING, 
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s obsolete control code 0x%x (get_drive_by_location) object id 0x%x class id 0x%x\n", 
                                   __FUNCTION__, control_code, object_id, class_id);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_transport_complete_packet(packet);
            break;
        case FBE_TOPOLOGY_CONTROL_CODE_GET_PROVISION_DRIVE_BY_LOCATION:
            status = topology_get_provision_drive_by_location(packet);
            break;
        case FBE_TOPOLOGY_CONTROL_CODE_GET_ENCLOSURE_BY_LOCATION:
            status = topology_get_enclosure_by_location(packet);
            break;
        case FBE_TOPOLOGY_CONTROL_CODE_GET_PHYSICAL_DRIVE_BY_LOCATION:
            status = topology_get_physical_drive_by_location(packet);
            break;

        case FBE_TOPOLOGY_CONTROL_CODE_REGISTER_EVENT_FUNCTION:
            status = topology_register_event_function(packet);
            break;

        case FBE_TOPOLOGY_CONTROL_CODE_UNREGISTER_EVENT_FUNCTION:
            status = topology_unregister_event_function(packet);
            break;

        case FBE_TOPOLOGY_CONTROL_CODE_GET_PORT_BY_LOCATION:
            status = topology_get_port_by_location(packet);
            break;

        case FBE_TOPOLOGY_CONTROL_CODE_GET_PORT_BY_LOCATION_AND_ROLE:
            status = topology_get_port_by_location_and_role(packet);
            break;

        case FBE_TOPOLOGY_CONTROL_CODE_ENUMERATE_CLASS:
            status = topology_enumerate_class( packet);
            break;

        case FBE_TOPOLOGY_CONTROL_CODE_GET_SPARE_DRIVE_POOL:
            status = topology_get_spare_drive_pool(packet);
            break;

        case FBE_BASE_SERVICE_CONTROL_CODE_DESTROY:
            status = fbe_topology_destroy( packet);
            break;

        case FBE_TOPOLOGY_CONTROL_CODE_GET_TOTAL_OBJECTS:
            status = fbe_topology_get_total_objects( packet);
            break;

        case FBE_TOPOLOGY_CONTROL_CODE_GET_TOTAL_OBJECTS_OF_CLASS:
            status = fbe_topology_get_total_objects_of_class(packet);
            break;

        case FBE_TOPOLOGY_CONTROL_CODE_GET_LOGICAL_DRIVE_BY_SERIAL_NUMBER:
            fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                                   FBE_TRACE_LEVEL_WARNING, 
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s obsolete control code 0x%x (get_logical_drv_by_SN) object id 0x%x class id 0x%x\n", 
                                   __FUNCTION__, control_code, object_id, class_id);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_transport_complete_packet(packet);
            break;

        case FBE_TOPOLOGY_CONTROL_CODE_GET_PHYSICAL_DRIVE_BY_SERIAL_NUMBER:
            status = topology_get_physical_drive_by_sn(packet);
            break;

        case FBE_TOPOLOGY_CONTROL_CODE_GET_BOARD:
            status = topology_get_board(packet);
            break;

        case FBE_TOPOLOGY_CONTROL_CODE_ENUMERATE_PORTS_BY_ROLE:
            status = topology_enumerate_ports_by_role( packet );
            break;

        case FBE_TOPOLOGY_CONTROL_CODE_GET_BVD_OBJECT_ID:
            status = topology_get_bvd_id(packet);
            break;

        case FBE_TOPOLOGY_CONTROL_CODE_GET_OBJECT_ID_OF_SINGLETON_CLASS:
            status = topology_get_object_id_of_singleton_class(packet);
            break;

        case FBE_TOPOLOGY_CONTROL_CODE_CHECK_OBJECT_EXISTENT:
            status = topology_check_object_existent( packet);
            break;

        case FBE_TOPOLOGY_CONTROL_CODE_GET_PHYSICAL_DRIVE_OBJECTS:
            status = topology_get_physical_drive_objects( packet);
            break;

        case FBE_TOPOLOGY_CONTROL_CODE_SET_PVD_HS_FLAG_CONTROL:
            status = topology_set_config_pvd_hs_flag(packet);
            break;

        case FBE_TOPOLOGY_CONTROL_CODE_GET_PVD_HS_FLAG_CONTROL:
            status = topology_get_config_pvd_hs_flag(packet);
            break;

        case FBE_TOPOLOGY_CONTROL_CODE_UPDATE_DISABLE_SPARE_SELECT_UNCONSUMED:
            status = topology_update_disable_spare_select_unconsumed(packet);
            break;

        case FBE_TOPOLOGY_CONTROL_CODE_GET_DISABLE_SPARE_SELECT_UNCONSUMED:
            status = topology_get_disable_spare_select_unconsumed(packet);
            break;
		case FBE_TOPOLOGY_CONTROL_CODE_GET_PROVISION_DRIVE_BY_LOCATION_FROM_NON_PAGED:
			status = topology_get_pvd_by_location_from_non_paged(packet);
			break;

        default:
            /*! @todo DE200 - There are at least (3) control codes that are 
             *        incorrectly sent to the topology service.
             *          o   FBE_LOGICAL_DRIVE_CONTROL_GET_DRIVE_INFO
             *        So for now the trace level is `warning'.  It should be
             *        `error'.
             */
            fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                                   FBE_TRACE_LEVEL_WARNING, 
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s Unknown control code 0x%x object id 0x%x class id 0x%x\n", 
                                   __FUNCTION__, control_code, object_id, class_id);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_transport_complete_packet(packet);
            break;
    }

    return status;
}

static fbe_status_t
topology_send_mgmt_packet(fbe_object_id_t object_id, fbe_packet_t * packet)
{
    const fbe_class_methods_t * pclass; /* pointer to cbe class */
    fbe_class_id_t class_id;
    fbe_object_handle_t control_handle;
    fbe_status_t mgmt_status;
    fbe_lifecycle_state_t lifecycle_state;
    fbe_packet_attr_t packet_attr;

    /* Since the object could have been destroyed, this is an info.*/
    if(!topology_is_valid_object_id(object_id)){
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_DEBUG_LOW, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid object id 0x%x\n", __FUNCTION__, object_id);
        fbe_transport_set_status(packet, FBE_STATUS_NO_OBJECT, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_NO_OBJECT;
    }

    mgmt_status = topology_get_object_class_id(object_id, &class_id);
    if(mgmt_status != FBE_STATUS_OK){
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_DEBUG_LOW, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid class id 0x%x for object id 0x%x \n", __FUNCTION__, class_id, object_id);
        fbe_transport_set_status(packet, FBE_STATUS_NO_OBJECT, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_NO_OBJECT;
    }

    pclass = topology_get_class(class_id);
    if(pclass == NULL){
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s topology_get_class fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_NO_OBJECT, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    control_handle = fbe_topology_get_control_handle(object_id);
    if(control_handle == NULL){
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s topology_get_control_handle failed for object: 0x%x\n", 
                               __FUNCTION__, object_id);
        fbe_transport_set_status(packet, FBE_STATUS_NO_OBJECT, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)fbe_base_handle_to_pointer(control_handle), &lifecycle_state);
    fbe_transport_get_packet_attr(packet, &packet_attr);

    if((lifecycle_state == FBE_LIFECYCLE_STATE_DESTROY) && !(packet_attr & FBE_PACKET_FLAG_DESTROY_ENABLED)){
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_WARNING, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s object: 0x%x in destroy state\n", 
                               __FUNCTION__, object_id);
        fbe_transport_set_status(packet, FBE_STATUS_NO_OBJECT, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_NO_OBJECT;
    }

    if((lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE) && (packet_attr & FBE_PACKET_FLAG_EXTERNAL)){
        /*! @todo Why do we allow an object id of 0!??*/
        if (object_id == 0)
        {
            fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                                   FBE_TRACE_LEVEL_WARNING, 
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s object: 0x%x in specialize state\n", 
                                   __FUNCTION__, object_id);
        }
        else
        {
            fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                                   FBE_TRACE_LEVEL_DEBUG_LOW, 
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s object: 0x%x in specialize state\n", 
                                   __FUNCTION__, object_id);
        }
        fbe_transport_set_status(packet, FBE_STATUS_BUSY, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_BUSY;
    }

    fbe_base_object_increment_userper_counter(control_handle);
    fbe_transport_set_completion_function(packet, topology_send_mgmt_packet_completion, control_handle);

    mgmt_status =  pclass->control_entry(control_handle, packet); 

    return mgmt_status;
}

static fbe_status_t
topology_enumerate_objects(fbe_packet_t *  packet)
{
    fbe_topology_mgmt_enumerate_objects_t * topology_mgmt_enumerate_objects = NULL;
    fbe_object_id_t                         id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                               i  =0;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_sg_element_t *                      sg_element = NULL;
    fbe_u32_t                               sg_elements = 0;
    fbe_object_id_t *                       obj_id_list = NULL;
    fbe_u32_t                               total_objects_copied = 0;

    fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s entry\n", __FUNCTION__);
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len != sizeof(fbe_topology_mgmt_enumerate_objects_t)){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Error Invalid control_buffer length", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &topology_mgmt_enumerate_objects); 
    if(topology_mgmt_enumerate_objects == NULL){
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*we need the sg list in order to fill this memory with the object IDs*/
    fbe_payload_ex_get_sg_list(payload, &sg_element, &sg_elements);

    obj_id_list = (fbe_object_id_t *)sg_element->address;

    /*sanity check*/
    if ((sg_element->count / sizeof(fbe_object_id_t)) != topology_mgmt_enumerate_objects->number_of_objects) {
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s User allcated memory is not the same as objects to copy\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;

    }

    /* Clean up the array */
    for (i = 0 ; i < topology_mgmt_enumerate_objects->number_of_objects; i++, obj_id_list ++){
        *obj_id_list = FBE_OBJECT_ID_INVALID;
    }

    /* Scan the object_table */
    obj_id_list = (fbe_object_id_t *)sg_element->address;
    for(id = 0; id < max_supported_objects ; id++) {
        if(topology_is_object_ready(id)) {
            *obj_id_list = id;
            obj_id_list ++;/*get to the next memory location*/
            total_objects_copied ++;

            /*check if we can't do any more object. It's possible that since the user asked how many we had, some more objects were added
            and we don't want to overwrite memory*/
            if (total_objects_copied == topology_mgmt_enumerate_objects->number_of_objects) {
                break;
            }
        }
    }

    topology_mgmt_enumerate_objects->number_of_objects_copied = total_objects_copied;

    fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                           FBE_TRACE_LEVEL_DEBUG_LOW,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s number_of_objects 0x%X \n",
                           __FUNCTION__, total_objects_copied);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t
topology_get_object_type(fbe_packet_t *  packet)
{
    fbe_topology_mgmt_get_object_type_t * topology_mgmt_get_object_type = NULL;
    fbe_class_id_t class_id;
    fbe_status_t status;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_topology_mgmt_get_object_type_t)){
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Invalid buffer_len \n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &topology_mgmt_get_object_type); 
    if(topology_mgmt_get_object_type == NULL){
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (topology_mgmt_get_object_type->object_id > max_supported_objects)
    {
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_WARNING, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s object_id %d > max supported %d fail\n", 
                               __FUNCTION__, 
                               topology_mgmt_get_object_type->object_id, max_supported_objects);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if(!topology_is_object_ready(topology_mgmt_get_object_type->object_id)) {
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_WARNING, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Invalid object_id %X\n",
                               __FUNCTION__, topology_mgmt_get_object_type->object_id);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = topology_get_object_class_id(topology_mgmt_get_object_type->object_id, &class_id);
    if(status != FBE_STATUS_OK) {
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s topology_get_object_class_id fail %X\n", __FUNCTION__, status);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_topology_class_id_to_object_type(class_id, &topology_mgmt_get_object_type->topology_object_type);
    if(status != FBE_STATUS_OK) {
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s fbe_topology_map_class_id_to_object_type failed %X\n", __FUNCTION__, status);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_topology_send_io_packet(fbe_packet_t * packet)
{
    const fbe_class_methods_t * pclass; /* pointer to fbe class */
    fbe_class_id_t class_id;

    fbe_object_handle_t io_handle;
    fbe_status_t status;
    fbe_object_id_t object_id;
    fbe_package_id_t destination_package_id;
    fbe_package_id_t my_package_id;

    fbe_get_package_id(&my_package_id);
    fbe_transport_get_package_id(packet,&destination_package_id);

    if(my_package_id != destination_package_id){
        if((destination_package_id == FBE_PACKAGE_ID_PHYSICAL) && (physical_package_io_entry != NULL)){
            status = physical_package_io_entry(packet);
            return status;
        } else if((destination_package_id == FBE_PACKAGE_ID_SEP_0) && (sep_io_entry != NULL)){
            status = sep_io_entry(packet);
            return status;
        } else {
            fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s: Invalid destination package id %d \n", __FUNCTION__, destination_package_id);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }


     status = fbe_transport_get_object_id(packet, &object_id);

    /* fbe_rwlock_read_lock(&topology_object_table_lock); */

    if(!topology_is_valid_object_id(object_id)){
        /* fbe_rwlock_read_unlock(&topology_object_table_lock); */ 

        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_DEBUG_LOW, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s invalid object id %X\n", __FUNCTION__, object_id);
        /* FBE30
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
        */
        fbe_transport_set_status(packet, FBE_STATUS_NO_OBJECT, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_NO_OBJECT;
    }

    status = topology_get_object_class_id(object_id, &class_id);
    if(status != FBE_STATUS_OK){
        /* fbe_rwlock_read_unlock(&topology_object_table_lock); */
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid object id %X\n", __FUNCTION__, object_id);
        fbe_transport_set_status(packet, FBE_STATUS_NO_OBJECT, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    pclass = topology_get_class(class_id);
    if(pclass == NULL){
        /* fbe_rwlock_read_unlock(&topology_object_table_lock); */
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s topology_get_class fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(pclass->io_entry == NULL){
        /* fbe_rwlock_read_unlock(&topology_object_table_lock); */
        /*! @todo DE200 - Need to root cause the reason for this failure and 
         *        then change the trace level back to `error'.
         */
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_WARNING, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s io_entry NULL\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    io_handle = fbe_topology_get_io_handle(object_id);
    status =  pclass->io_entry(io_handle, packet); 

    /* fbe_rwlock_read_unlock(&topology_object_table_lock); */
    return status;
}

fbe_status_t 
fbe_topology_send_event(fbe_object_id_t object_id, fbe_event_type_t event_type, fbe_event_context_t event_context)
{
    fbe_status_t status;
    const fbe_class_methods_t * pclass; /* pointer to fbe class */
    fbe_class_id_t class_id;
    fbe_object_handle_t control_handle;
    fbe_lifecycle_state_t lifecycle_state;

    /* fbe_rwlock_read_lock(&topology_object_table_lock); */

    /* Since the object object might have been destroyed this is an info. */
    if(!topology_is_valid_object_id(object_id)){
        /* fbe_rwlock_read_unlock(&topology_object_table_lock); */ 
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_DEBUG_LOW, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid object id 0x%x, event %d\n", __FUNCTION__, object_id, event_type);
        return FBE_STATUS_NO_OBJECT;
    }


    status = topology_get_object_class_id(object_id, &class_id);
    if(status != FBE_STATUS_OK){
        /* fbe_rwlock_read_unlock(&topology_object_table_lock); */
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_DEBUG_LOW, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid class id 0x%x for object id 0x%x, event %d\n", __FUNCTION__, class_id, object_id, event_type);
        return FBE_STATUS_NO_OBJECT;
    }

    pclass = topology_get_class(class_id);
    if(pclass == NULL){
        /* fbe_rwlock_read_unlock(&topology_object_table_lock); */
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s topology_get_class fail\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(pclass->event_entry == NULL){
        /* fbe_rwlock_read_unlock(&topology_object_table_lock); */
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s event_entry is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_handle = fbe_topology_get_control_handle_atomic(object_id);
    if(control_handle == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_WARNING, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s object %d does not exist, event %d\n", __FUNCTION__, object_id, event_type);
        return FBE_STATUS_NO_OBJECT;
    }

    /*! @todo AR 501373 - Currently a concern about getting the lifecycle state.*/
    /* Since the object object might have been destroyed this is an info. */
    status = fbe_base_object_get_lifecycle_state((fbe_base_object_t *)control_handle, &lifecycle_state);
    if((status != FBE_STATUS_OK)                        ||
       (lifecycle_state == FBE_LIFECYCLE_STATE_DESTROY)    ){

        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_WARNING, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s object id 0x%x is destroyed fail event %d\n", 
                               __FUNCTION__, object_id, event_type);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Call the event entry.*/
    status =  pclass->event_entry(control_handle, event_type, event_context); 

    fbe_topology_release_control_handle(object_id);

    /* fbe_rwlock_read_unlock(&topology_object_table_lock); */
    return status;
}

/* This function may be call by scheduler ONLY */
fbe_status_t 
fbe_topology_send_monitor_packet(fbe_packet_t * packet)
{
    const fbe_class_methods_t * pclass; /* pointer to fbe class */
    fbe_class_id_t class_id;
    fbe_object_handle_t control_handle;
    fbe_status_t status;
    fbe_object_id_t object_id;
    fbe_payload_ex_t * payload = NULL;

    status = fbe_transport_get_object_id(packet, &object_id);

    /* fbe_rwlock_read_lock(&topology_object_table_lock); */

    /* Since the object could have been destroyed this is an info. */
    if(!topology_is_valid_object_id(object_id)){
        /* fbe_rwlock_read_unlock(&topology_object_table_lock); */ 

        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_DEBUG_LOW, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid object id 0x%x\n", __FUNCTION__, object_id);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = topology_get_object_class_id(object_id, &class_id);
    if(status != FBE_STATUS_OK){
        /* fbe_rwlock_read_unlock(&topology_object_table_lock); */
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_DEBUG_LOW, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid class id 0x%x object id 0x%x\n", __FUNCTION__, class_id, object_id);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    pclass = topology_get_class(class_id);
    if(pclass == NULL){
        /* fbe_rwlock_read_unlock(&topology_object_table_lock); */
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s topology_get_class fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(pclass->monitor_entry == NULL){
        /* fbe_rwlock_read_unlock(&topology_object_table_lock); */
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s class_id = %d monitor_entry NULL\n", __FUNCTION__, class_id);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_handle = fbe_topology_get_control_handle(object_id);
    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_increment_control_operation_level(payload);
    status =  pclass->monitor_entry(control_handle, packet); 

    /* fbe_rwlock_read_unlock(&topology_object_table_lock); */
    return status;
}

static fbe_status_t
topology_get_statistics(fbe_packet_t *  packet)
{
    fbe_topology_control_get_statistics_t * topology_control_get_statistics = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_length_t buffer_length;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &topology_control_get_statistics);
    if(topology_control_get_statistics == NULL){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length < sizeof(fbe_topology_control_get_statistics_t)){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid buffer_length %d", __FUNCTION__, buffer_length);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    topology_control_get_statistics->number_of_created_objects = topology_statistics.number_of_created_objects;
    topology_control_get_statistics->number_of_destroyed_objects = topology_statistics.number_of_destroyed_objects;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}


static fbe_status_t
topology_get_provision_drive_by_location(fbe_packet_t *  packet)
{
    fbe_topology_control_get_provision_drive_id_by_location_t * provision_drive_location = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_length_t buffer_length;
    fbe_object_id_t id;
    fbe_class_id_t class_id;
    fbe_physical_drive_get_location_t drive_location;
    fbe_packet_t *  new_packet = NULL;
    fbe_status_t status = FBE_STATUS_OK;
	fbe_packet_attr_t caller_attr;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &provision_drive_location);
    if(provision_drive_location == NULL){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s fbe_payload_control_get_buffer failed", __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length < sizeof(fbe_topology_control_get_drive_by_location_t)){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s Invalid buffer_length %d", __FUNCTION__, buffer_length);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Scan topology table for pvd drive objects */
    new_packet = fbe_transport_allocate_packet();
    fbe_transport_initialize_packet(new_packet);
	fbe_transport_get_packet_attr(packet, &caller_attr);
	fbe_transport_set_packet_attr(new_packet, caller_attr);
	fbe_transport_clear_packet_attr(new_packet,FBE_PACKET_FLAG_COMPLETION_SEMAPHORE_INITIALIZED);/*we don't want to carry that*/

    for(id = 0; id < max_supported_objects ; id ++) {
        topology_get_object_class_id(id, &class_id);
        if (class_id == FBE_CLASS_ID_PROVISION_DRIVE)
        {
            fbe_transport_packet_clear_callstack_depth(new_packet);/* reset this since we reuse the packet */
            status = topology_get_provision_drive_location(new_packet,
                    id,
                    &drive_location);

            if (status == FBE_STATUS_NO_OBJECT) {
                    /*the object was there and was removed, let's go to the next one*/
                    continue;
            }

            if (status != FBE_STATUS_OK)
            {
                /* The PVD might not be connected - keep trying though */
                continue;
            }

            if (drive_location.port_number == provision_drive_location->port_number &&
                    drive_location.enclosure_number == provision_drive_location->enclosure_number &&
                    drive_location.slot_number == provision_drive_location->slot_number)
            {
                provision_drive_location->object_id = id;
                fbe_transport_release_packet(new_packet);
                fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
                fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_OK;
            }
        }
    }

    fbe_transport_release_packet(new_packet);
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    fbe_payload_control_set_status_qualifier(control_operation, FBE_OBJECT_ID_INVALID);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}


static fbe_status_t 
topology_get_info_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
    return FBE_STATUS_OK;
}

static fbe_status_t
topology_get_enclosure_by_location(fbe_packet_t *  packet)
{
    fbe_topology_control_get_enclosure_by_location_t * location = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_length_t buffer_length;
    fbe_object_id_t id;
    fbe_class_id_t class_id;
    fbe_packet_t *  new_packet = NULL;
    fbe_u32_t   encl = 0, port = 0,component_id = 0;
    fbe_u32_t comp_id;
    fbe_object_handle_t control_handle;
    fbe_lifecycle_state_t lifecycle_state;
	fbe_packet_attr_t caller_attr;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &location);
    if(location == NULL){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed", __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length < sizeof(fbe_topology_control_get_enclosure_by_location_t)){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid buffer_length %d", __FUNCTION__, buffer_length);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Scan topology table for logical drive objects */
    new_packet = fbe_transport_allocate_packet();
    fbe_transport_initialize_packet(new_packet);

	fbe_transport_get_packet_attr(packet, &caller_attr);
	fbe_transport_set_packet_attr(new_packet, caller_attr);
	fbe_transport_clear_packet_attr(new_packet,FBE_PACKET_FLAG_COMPLETION_SEMAPHORE_INITIALIZED);/*we don't want to carry that*/

    location->component_count = 0;
    location->enclosure_object_id = FBE_OBJECT_ID_INVALID;
    for(comp_id = 0;comp_id<FBE_API_MAX_ENCL_COMPONENTS;comp_id++)
    {
        location->comp_object_id[comp_id] = FBE_OBJECT_ID_INVALID;
    }
    
    for(id = 0; id < FBE_MAX_OBJECTS ; id ++) 
    {
        if(!topology_is_object_ready(id))
        {
            continue;        
        }        
        control_handle = fbe_topology_get_control_handle(id);
        if(control_handle == NULL)
        {
            fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                                   FBE_TRACE_LEVEL_ERROR, 
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s topology_get_control_handle failed for object: 0x%x\n", 
                                   __FUNCTION__, id);
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        fbe_base_object_get_lifecycle_state((fbe_base_object_t *)fbe_base_handle_to_pointer(control_handle), &lifecycle_state);
    
        if(lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE)
        {
            /* The object is still in FBE_LIFECYCLE_STATE_SPECIALIZE. The class id is not finalized yet. 
             * For example: If the ICM object is out of SPECIALIZE state but its edge expanders are still
             * in the SPECIALIZE state, we could get the class id for the edge expander as 
             * FBE_CLASS_ID_SAS_ENCLOSURE and satisfied the check 
             * if((class_id > FBE_CLASS_ID_ENCLOSURE_FIRST) && (class_id < FBE_CLASS_ID_ENCLOSURE_LAST))
             * which could cause the edge expander object id to overwrite the ICM object id
             * to be saved as location->enclosure_object_id.
             * That is why I added the check here.
             * - PINGHE
             */
            continue;
        }        

        topology_get_object_class_id(id, &class_id);
        if((class_id > FBE_CLASS_ID_ENCLOSURE_FIRST) && (class_id < FBE_CLASS_ID_ENCLOSURE_LAST))
        {
            fbe_transport_packet_clear_callstack_depth(new_packet);/* reset this since we reuse the packet */
            topology_get_enclosure_info(new_packet, id, &port, &encl, &component_id);
            if((port == location->port_number) && (encl == location->enclosure_number))
            {
                location->enclosure_object_id = id;
                /* Removed the line location->component_count = 0; here 
                 * I have seen some cases the Voyager ICM object id is larger than its EE LCCs.
                 * It means it is possible that we already found two EE LCCs when we get here.
                 * Therefore, we can not set location->component_count back to 0
                 * - PINGHE.
                 */
                 
                if((class_id != FBE_CLASS_ID_SAS_VOYAGER_ICM_ENCLOSURE &&
                    class_id != FBE_CLASS_ID_SAS_VIKING_IOSXP_ENCLOSURE &&
                    class_id != FBE_CLASS_ID_SAS_CAYENNE_IOSXP_ENCLOSURE &&
                    class_id != FBE_CLASS_ID_SAS_NAGA_IOSXP_ENCLOSURE) ||
                   ((class_id == FBE_CLASS_ID_SAS_VOYAGER_ICM_ENCLOSURE) &&
                    (location->component_count == 2)) ||
                   ((class_id == FBE_CLASS_ID_SAS_VIKING_IOSXP_ENCLOSURE) &&
                    (location->component_count == 4)) ||
                   ((class_id == FBE_CLASS_ID_SAS_CAYENNE_IOSXP_ENCLOSURE) &&
                     (location->component_count == 1)) ||
                    ((class_id == FBE_CLASS_ID_SAS_NAGA_IOSXP_ENCLOSURE) &&
                    (location->component_count == 2)))
                {
                        fbe_transport_release_packet(new_packet);
                        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
                        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
                        fbe_transport_complete_packet(packet);
                        return FBE_STATUS_OK;
                }
                else
                {
                    continue;
                }
            }
        }
        else if(class_id == FBE_CLASS_ID_SAS_VOYAGER_EE_LCC)  /* For voyager expander objects */
        {
            fbe_transport_packet_clear_callstack_depth(new_packet);/* reset this since we reuse the packet */
            topology_get_enclosure_info(new_packet, id, &port, &encl, &component_id);
            if((port == location->port_number) && (encl == location->enclosure_number))
            {
                location->component_count++;
                location->comp_object_id[component_id] = id;
                /* I have seen some cases the Voyager ICM object id is larger than its EE LCCs.
                 * So I added the check (location->enclosure_object_id != FBE_OBJECT_ID_INVALID). 
                 * - PINGHE 
                 */
                if((location->component_count == 2) && 
                   (location->enclosure_object_id != FBE_OBJECT_ID_INVALID))   
                {
                    fbe_transport_release_packet(new_packet);
                    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
                    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
                    fbe_transport_complete_packet(packet);
                    return FBE_STATUS_OK;     
                }
                else
                {
                    continue;
                }
            }
        }
        else if( (class_id == FBE_CLASS_ID_SAS_VIKING_DRVSXP_LCC)||
                 (class_id == FBE_CLASS_ID_SAS_NAGA_DRVSXP_LCC) )  
        {
            fbe_transport_packet_clear_callstack_depth(new_packet);/* reset this since we reuse the packet */
            topology_get_enclosure_info(new_packet, id, &port, &encl, &component_id);
            if((port == location->port_number) && (encl == location->enclosure_number))
            {
                location->component_count++;
                location->comp_object_id[component_id] = id;
                /* I have seen some cases the Voyager ICM object id is larger than its EE LCCs.
                 * So I added the check (location->enclosure_object_id != FBE_OBJECT_ID_INVALID). 
                 * - PINGHE 
                 */
                if((location->component_count == 4) && 
                   (location->enclosure_object_id != FBE_OBJECT_ID_INVALID))   
                {
                    fbe_transport_release_packet(new_packet);
                    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
                    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
                    fbe_transport_complete_packet(packet);
                    return FBE_STATUS_OK;     
                }
                else
                {
                    continue;
                }
            }
        }
        else if(class_id == FBE_CLASS_ID_SAS_CAYENNE_DRVSXP_LCC)  /* For voyager expander objects */
        {
            fbe_transport_packet_clear_callstack_depth(new_packet);/* reset this since we reuse the packet */
            topology_get_enclosure_info(new_packet, id, &port, &encl, &component_id);
            if((port == location->port_number) && (encl == location->enclosure_number))
            {
                location->component_count++;
                location->comp_object_id[component_id] = id;
                /* I have seen some cases the Voyager ICM object id is larger than its EE LCCs.
                 * So I added the check (location->enclosure_object_id != FBE_OBJECT_ID_INVALID). 
                 * - PINGHE 
                 */
                if((location->component_count == 1) && 
                   (location->enclosure_object_id != FBE_OBJECT_ID_INVALID))   
                {
                    fbe_transport_release_packet(new_packet);
                    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
                    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
                    fbe_transport_complete_packet(packet);
                    return FBE_STATUS_OK;     
                }
                else
                {
                    continue;
                }
            }
        }
    }

    fbe_transport_release_packet(new_packet);
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    fbe_payload_control_set_status_qualifier(control_operation, FBE_OBJECT_ID_INVALID);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t
topology_get_enclosure_info(fbe_packet_t * packet,
                            fbe_object_id_t object_id, 
                            fbe_u32_t *port,
                            fbe_u32_t *enclosure,
                            fbe_u32_t *component_id)
{
    fbe_status_t status;
    fbe_payload_ex_t  * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_semaphore_t sem;
    fbe_base_enclosure_mgmt_get_enclosure_number_t  base_enclosure_mgmt_get_enclosure_number;
    fbe_base_port_mgmt_get_port_number_t            base_port_mgmt_get_port_number;
    fbe_base_enclosure_mgmt_get_component_id_t  base_enclosure_mgmt_get_component_id;

    fbe_semaphore_init(&sem, 0, 1);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    fbe_payload_control_build_operation(control_operation, 
                                        FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_NUMBER, 
                                        &base_enclosure_mgmt_get_enclosure_number, 
                                        sizeof(fbe_base_enclosure_mgmt_get_enclosure_number_t));

    status = fbe_transport_set_completion_function( packet, topology_get_info_completion, &sem);

    status = fbe_transport_set_address( packet,
                                        FBE_PACKAGE_ID_PHYSICAL,
                                        FBE_SERVICE_ID_TOPOLOGY,
                                        FBE_CLASS_ID_INVALID,
                                        object_id);

    fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_TRAVERSE);

    fbe_payload_ex_increment_control_operation_level(payload);

    topology_send_mgmt_packet(object_id, packet);

    fbe_semaphore_wait(&sem, NULL);

    *enclosure = base_enclosure_mgmt_get_enclosure_number.enclosure_number;

    /*now for the port of the enclosure*/
    fbe_payload_ex_release_control_operation(payload, control_operation);

    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    fbe_payload_control_build_operation(control_operation, 
                                        FBE_BASE_PORT_CONTROL_CODE_GET_PORT_NUMBER, 
                                        &base_port_mgmt_get_port_number, 
                                        sizeof(fbe_base_port_mgmt_get_port_number_t));

    status = fbe_transport_set_completion_function( packet, topology_get_info_completion, &sem);

    status = fbe_transport_set_address( packet,
                                        FBE_PACKAGE_ID_PHYSICAL,
                                        FBE_SERVICE_ID_TOPOLOGY,
                                        FBE_CLASS_ID_INVALID,
                                        object_id);

    fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_TRAVERSE);

    fbe_payload_ex_increment_control_operation_level(payload);

    topology_send_mgmt_packet(object_id, packet);

    fbe_semaphore_wait(&sem, NULL);

    *port = base_port_mgmt_get_port_number.port_number;

    /*now for the component_id of the enclosure*/
    fbe_payload_ex_release_control_operation(payload, control_operation);

    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    fbe_payload_control_build_operation(control_operation, 
                                        FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_COMPONENT_ID, 
                                        &base_enclosure_mgmt_get_component_id, 
                                        sizeof(fbe_base_enclosure_mgmt_get_component_id_t));

    status = fbe_transport_set_completion_function( packet, topology_get_info_completion, &sem);

    status = fbe_transport_set_address( packet,
                                        FBE_PACKAGE_ID_PHYSICAL,
                                        FBE_SERVICE_ID_TOPOLOGY,
                                        FBE_CLASS_ID_INVALID,
                                        object_id);

    fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_TRAVERSE);

    fbe_payload_ex_increment_control_operation_level(payload);

    topology_send_mgmt_packet(object_id, packet);

    fbe_semaphore_wait(&sem, NULL);

    *component_id = base_enclosure_mgmt_get_component_id.component_id;

    fbe_semaphore_destroy(&sem);
    
    fbe_payload_ex_release_control_operation(payload, control_operation);
    
    return FBE_STATUS_OK;
}

static fbe_status_t topology_get_physical_drive_by_location(fbe_packet_t *  packet)
{
    fbe_topology_control_get_physical_drive_by_location_t * location = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_length_t buffer_length;
    fbe_object_id_t id;
    fbe_class_id_t class_id;
    fbe_packet_t *  new_packet = NULL;
    fbe_u32_t       port  =0, encl = 0, slot = 0;
	fbe_packet_attr_t caller_attr;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &location);
    if(location == NULL){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed", __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length < sizeof(fbe_topology_control_get_physical_drive_by_location_t)){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid buffer_length %d", __FUNCTION__, buffer_length);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Scan topology table for physical drive objects */
    new_packet = fbe_transport_allocate_packet();
    fbe_transport_initialize_packet(new_packet);

	fbe_transport_get_packet_attr(packet, &caller_attr);
	fbe_transport_set_packet_attr(new_packet, caller_attr);
	fbe_transport_clear_packet_attr(new_packet,FBE_PACKET_FLAG_COMPLETION_SEMAPHORE_INITIALIZED);/*we don't want to carry that*/

    for(id = 0; id < max_supported_objects ; id ++) {
        topology_get_object_class_id(id, &class_id);
        if ((class_id > FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST) && (class_id < FBE_CLASS_ID_PHYSICAL_DRIVE_LAST)) {
            fbe_transport_packet_clear_callstack_depth(new_packet);/* reset this since we reuse the packet */
            topology_get_physical_drive_info(new_packet, id, &port, &encl, &slot);
            if(port== location->port_number && encl == location->enclosure_number && slot == location->slot_number){
                /* found a match*/
                location->physical_drive_object_id = id;
                fbe_transport_release_packet(new_packet);
                fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
                fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_OK;
            }
        }
    }
    /* no match found */
    fbe_transport_release_packet(new_packet);
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    fbe_payload_control_set_status_qualifier(control_operation, FBE_OBJECT_ID_INVALID);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t
topology_get_physical_drive_info(fbe_packet_t * packet,
                                 fbe_object_id_t object_id, 
                                 fbe_u32_t *port,
                                 fbe_u32_t *enclosure,
                                 fbe_u32_t *slot)
{
    fbe_status_t status;
    fbe_payload_ex_t  * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_physical_drive_get_location_t  get_location;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    fbe_payload_control_build_operation(control_operation, 
                                        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_LOCATION, 
                                        &get_location, 
                                        sizeof(fbe_physical_drive_get_location_t));
    status = fbe_transport_set_address( packet,
                                        FBE_PACKAGE_ID_PHYSICAL,
                                        FBE_SERVICE_ID_TOPOLOGY,
                                        FBE_CLASS_ID_INVALID,
                                        object_id);

    fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_NO_ATTRIB);
    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    fbe_payload_ex_increment_control_operation_level(payload);

    topology_send_mgmt_packet(object_id, packet);
    fbe_transport_wait_completion(packet);
    fbe_payload_ex_release_control_operation(payload, control_operation);

    *enclosure = get_location.enclosure_number;
    *port = get_location.port_number;
    *slot = get_location.slot_number;
    
    return FBE_STATUS_OK;
}

static fbe_status_t
topology_register_event_entry(void)
{
    fbe_packet_t *  packet = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_topology_control_register_event_function_t topology_control_register_event_function;
    fbe_package_id_t my_package_id;
    fbe_semaphore_t sem;
    fbe_status_t status;

    fbe_semaphore_init(&sem, 0, 1);

    packet = fbe_transport_allocate_packet();
    fbe_transport_initialize_packet(packet);
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    fbe_get_package_id(&my_package_id);
    topology_control_register_event_function.package_id = my_package_id;
    topology_control_register_event_function.event_function = fbe_topology_send_event;

    fbe_payload_control_build_operation(control_operation, 
                                        FBE_TOPOLOGY_CONTROL_CODE_REGISTER_EVENT_FUNCTION, 
                                        &topology_control_register_event_function, 
                                        sizeof(fbe_topology_control_register_event_function_t));

    status = fbe_transport_set_completion_function( packet, topology_register_event_entry_completion, &sem);

    status = fbe_transport_set_address( packet,
                                        FBE_PACKAGE_ID_PHYSICAL,
                                        FBE_SERVICE_ID_TOPOLOGY,
                                        FBE_CLASS_ID_INVALID,
                                        FBE_OBJECT_ID_INVALID);

    fbe_service_manager_send_control_packet(packet);
    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);
    fbe_payload_ex_release_control_operation(payload, control_operation);
    fbe_transport_release_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t 
topology_register_event_entry_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
    return FBE_STATUS_OK;
}

static fbe_status_t
topology_unregister_event_entry(void)
{
    fbe_packet_t *  packet = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_topology_control_unregister_event_function_t topology_control_unregister_event_function;
    fbe_package_id_t my_package_id;
    fbe_semaphore_t sem;
    fbe_status_t status;

    fbe_semaphore_init(&sem, 0, 1);

    packet = fbe_transport_allocate_packet();
    fbe_transport_initialize_packet(packet);
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    fbe_get_package_id(&my_package_id);
    topology_control_unregister_event_function.package_id = my_package_id;

    fbe_payload_control_build_operation(control_operation, 
                                        FBE_TOPOLOGY_CONTROL_CODE_UNREGISTER_EVENT_FUNCTION, 
                                        &topology_control_unregister_event_function, 
                                        sizeof(fbe_topology_control_unregister_event_function_t));

    status = fbe_transport_set_completion_function( packet, topology_unregister_event_entry_completion, &sem);

    status = fbe_transport_set_address( packet,
                                        FBE_PACKAGE_ID_PHYSICAL,
                                        FBE_SERVICE_ID_TOPOLOGY,
                                        FBE_CLASS_ID_INVALID,
                                        FBE_OBJECT_ID_INVALID);

    fbe_service_manager_send_control_packet(packet);
    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);
    fbe_payload_ex_release_control_operation(payload, control_operation);
    fbe_transport_release_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t 
topology_unregister_event_entry_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
    return FBE_STATUS_OK;
}


static fbe_status_t
topology_register_event_function(fbe_packet_t *  packet)
{
    fbe_topology_control_register_event_function_t * topology_control_register_event_function = NULL;   
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_topology_control_register_event_function_t)){
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Invalid buffer_len \n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &topology_control_register_event_function); 
    if(topology_control_register_event_function == NULL){
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
        
    if(topology_control_register_event_function->package_id >= FBE_PACKAGE_ID_LAST){
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Invalid package_id %d\n", __FUNCTION__, topology_control_register_event_function->package_id);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    topology_event_table[topology_control_register_event_function->package_id] = topology_control_register_event_function->event_function;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t
topology_unregister_event_function(fbe_packet_t *  packet)
{
    fbe_topology_control_unregister_event_function_t * topology_control_unregister_event_function = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_topology_control_unregister_event_function_t)){
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Invalid buffer_len \n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &topology_control_unregister_event_function); 
    if(topology_control_unregister_event_function == NULL){
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
        
    if(topology_control_unregister_event_function->package_id >= FBE_PACKAGE_ID_LAST){
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Invalid package_id %d\n", __FUNCTION__, topology_control_unregister_event_function->package_id);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    topology_event_table[topology_control_unregister_event_function->package_id] = NULL;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_topology_send_event_to_another_package(fbe_object_id_t object_id, 
                                           fbe_package_id_t package_id, 
                                           fbe_event_type_t event_type, 
                                           fbe_event_context_t event_context)
{
    fbe_topology_send_event_function_t event_function;

    if(package_id >= FBE_PACKAGE_ID_LAST){
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Invalid package_id %d\n", __FUNCTION__, package_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(topology_event_table[package_id] == NULL){
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Unregistered package_id %d\n", __FUNCTION__, package_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    event_function = topology_event_table[package_id];

    event_function(object_id, event_type, event_context);

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_topology_set_physical_package_io_entry(fbe_io_entry_function_t io_entry)
{
    physical_package_io_entry = io_entry;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_topology_set_sep_io_entry(fbe_io_entry_function_t io_entry)
{
    sep_io_entry = io_entry;
    return FBE_STATUS_OK;
}

static fbe_status_t topology_get_port_by_location(fbe_packet_t *  packet)
{
    fbe_topology_control_get_port_by_location_t * location = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_length_t buffer_length;
    fbe_object_id_t id;
    fbe_class_id_t class_id;
    fbe_packet_t *  new_packet = NULL;
    fbe_u32_t       port = 0;
	fbe_packet_attr_t caller_attr;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &location);
    if(location == NULL){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed", __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length < sizeof(fbe_topology_control_get_port_by_location_t)){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid buffer_length %d", __FUNCTION__, buffer_length);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Scan topology table for port objects objects */
    new_packet = fbe_transport_allocate_packet();
    fbe_transport_initialize_packet(new_packet);

	fbe_transport_get_packet_attr(packet, &caller_attr);
	fbe_transport_set_packet_attr(new_packet, caller_attr);
	fbe_transport_clear_packet_attr(new_packet,FBE_PACKET_FLAG_COMPLETION_SEMAPHORE_INITIALIZED);/*we don't want to carry that*/

    for(id = 0; id < max_supported_objects ; id ++) {
        topology_get_object_class_id(id, &class_id);
        if((class_id > FBE_CLASS_ID_PORT_FIRST) && (class_id < FBE_CLASS_ID_PORT_LAST)){
                fbe_transport_packet_clear_callstack_depth(new_packet);/* reset this since we reuse the packet */
                topology_get_port_number(new_packet, id, &port);
                if(port == location->port_number){
                        location->port_object_id = id;
                        fbe_transport_release_packet(new_packet);
                        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
                        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
                        fbe_transport_complete_packet(packet);
                        return FBE_STATUS_OK;
                    }
        }

    }

    fbe_transport_release_packet(new_packet);
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    fbe_payload_control_set_status_qualifier(control_operation, FBE_OBJECT_ID_INVALID);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t topology_get_port_by_location_and_role(fbe_packet_t *  packet)
{
    fbe_topology_control_get_port_by_location_and_role_t * get_port = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_length_t buffer_length;
    fbe_object_id_t id;
    fbe_class_id_t  class_id;
    fbe_packet_t *  new_packet = NULL;
    fbe_u32_t       port = 0;
    fbe_port_role_t port_role;
	fbe_packet_attr_t caller_attr;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &get_port);
    if(get_port == NULL){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed", __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length < sizeof(fbe_topology_control_get_port_by_location_and_role_t)){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid buffer_length %d", __FUNCTION__, buffer_length);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Scan topology table for port objects objects */
    new_packet = fbe_transport_allocate_packet();
    fbe_transport_initialize_packet(new_packet);

	fbe_transport_get_packet_attr(packet, &caller_attr);
	fbe_transport_set_packet_attr(new_packet, caller_attr);
	fbe_transport_clear_packet_attr(new_packet,FBE_PACKET_FLAG_COMPLETION_SEMAPHORE_INITIALIZED);/*we don't want to carry that*/

    for(id = 0; id < max_supported_objects ; id ++) {
        topology_get_object_class_id(id, &class_id);
        if((class_id > FBE_CLASS_ID_PORT_FIRST) && (class_id < FBE_CLASS_ID_PORT_LAST)){
            topology_get_port_number(new_packet, id, &port);
            topology_get_port_role(new_packet, id, &port_role);
            if(port == get_port->port_number && port_role == get_port->port_role){
                get_port->port_object_id = id;
                fbe_transport_release_packet(new_packet);
                fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
                fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_OK;
            }
        }
    }

    fbe_transport_release_packet(new_packet);
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    fbe_payload_control_set_status_qualifier(control_operation, FBE_OBJECT_ID_INVALID);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t
topology_get_port_number(fbe_packet_t * packet, fbe_object_id_t object_id, fbe_u32_t *port)
{
    fbe_status_t status;
    fbe_payload_ex_t  * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_semaphore_t sem;
    fbe_base_port_mgmt_get_port_number_t            base_port_mgmt_get_port_number;
    
    fbe_semaphore_init(&sem, 0, 1);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    fbe_payload_control_build_operation(control_operation, 
                                        FBE_BASE_PORT_CONTROL_CODE_GET_PORT_NUMBER, 
                                        &base_port_mgmt_get_port_number, 
                                        sizeof(fbe_base_port_mgmt_get_port_number_t));

    status = fbe_transport_set_completion_function( packet, topology_get_info_completion, &sem);

    status = fbe_transport_set_address( packet,
                                        FBE_PACKAGE_ID_PHYSICAL,
                                        FBE_SERVICE_ID_TOPOLOGY,
                                        FBE_CLASS_ID_INVALID,
                                        object_id);

    fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_TRAVERSE);
    fbe_payload_ex_increment_control_operation_level(payload);
    topology_send_mgmt_packet(object_id, packet);
    fbe_semaphore_wait(&sem, NULL);

    *port = base_port_mgmt_get_port_number.port_number;

    fbe_payload_ex_release_control_operation(payload, control_operation);

    fbe_semaphore_destroy(&sem);
    
    return FBE_STATUS_OK;
}
static fbe_status_t
topology_get_port_role(fbe_packet_t * packet, fbe_object_id_t object_id, fbe_port_role_t *port_role)
{
    fbe_status_t status;
    fbe_payload_ex_t  * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_semaphore_t sem;
    fbe_base_port_mgmt_get_port_role_t base_port_mgmt_get_port_role;
    
    fbe_semaphore_init(&sem, 0, 1);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    fbe_payload_control_build_operation(control_operation, 
                                        FBE_BASE_PORT_CONTROL_CODE_GET_PORT_ROLE, 
                                        &base_port_mgmt_get_port_role, 
                                        sizeof(fbe_base_port_mgmt_get_port_role_t));

    status = fbe_transport_set_completion_function( packet, topology_get_info_completion, &sem);

    status = fbe_transport_set_address( packet,
                                        FBE_PACKAGE_ID_PHYSICAL,
                                        FBE_SERVICE_ID_TOPOLOGY,
                                        FBE_CLASS_ID_INVALID,
                                        object_id);

    fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_TRAVERSE);
    fbe_payload_ex_increment_control_operation_level(payload);
    topology_send_mgmt_packet(object_id, packet);
    fbe_semaphore_wait(&sem, NULL);

    *port_role = base_port_mgmt_get_port_role.port_role;

    fbe_payload_ex_release_control_operation(payload, control_operation);

    fbe_semaphore_destroy(&sem);
    
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * topology_get_spare_drive_pool()
 ****************************************************************
 * @brief
 * This function sends the get provision drive info control 
 * packet to all the provision drive object id's in topology object
 * table and select the provision drive object id's which are
 * configured as spare. It updates the buffer in control packet
 * with this list of object id's which are configured as spare.
 *
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  08/03/2009 - Created. Dhaval Patel
 ****************************************************************/
static fbe_status_t topology_get_spare_drive_pool(fbe_packet_t *  packet)
{
    fbe_topology_control_get_spare_drive_pool_t *   spare_drive_pool_p;
    fbe_payload_ex_t *                             payload = NULL;
    fbe_payload_control_operation_t *               control_operation = NULL;
    fbe_payload_control_buffer_length_t             buffer_length;
    fbe_object_id_t                                 object_id;
    fbe_class_id_t                                  class_id;
    fbe_status_t                                    status;
    fbe_packet_t *                                  new_packet_p = NULL;
    fbe_object_id_t                                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_provision_drive_info_t                      provision_drive_info;
	fbe_packet_attr_t								caller_attr;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &spare_drive_pool_p);
    if(spare_drive_pool_p == NULL)
    {
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length < sizeof(fbe_topology_control_get_spare_drive_pool_t))
    {
        fbe_base_service_trace((fbe_base_service_t *) &topology_service,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid buffer_length %d",
                                __FUNCTION__, buffer_length);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    /* initialize the number of spare as zero. */
    spare_drive_pool_p->number_of_spare = 0;

    /* Validate the desired spare drive type.
     */
    switch(spare_drive_pool_p->desired_spare_drive_type)
    {
        case FBE_TOPOLOGY_GET_SPARE_DRIVE_TYPE_UNCONSUMED:
            /* The `disable spare select unconsumed' is True, then no drives 
             * will be selected.
             */
            if (topology_b_disable_spare_select_unconsumed == FBE_TRUE)
            {
                /* Generate a warning since no drives will be selected.
                 */
                fbe_base_service_trace((fbe_base_service_t *)&topology_service,
                                       FBE_TRACE_LEVEL_WARNING,
                                       FBE_TRACE_MESSAGE_ID_INFO,
                                       "%s Disable select `unconsumed' is True for desired type: `unconsumed'.\n",
                                       __FUNCTION__);
                fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
                fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_OK;
            }
            break;

        case FBE_TOPOLOGY_GET_SPARE_DRIVE_TYPE_TEST_SPARE:
            /* If `select only test spare' is False, then no drives will be selected.
             */
            if (topology_b_select_only_test_spare_drives == FBE_FALSE) 
            {
                /* Generate a warning since no drives will be selected.
                 */
                fbe_base_service_trace((fbe_base_service_t *)&topology_service,
                                       FBE_TRACE_LEVEL_WARNING,
                                       FBE_TRACE_MESSAGE_ID_INFO,
                                       "%s Select only `test spare' is False for desired type: `test spare'.\n",
                                       __FUNCTION__);
                fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
                fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_OK;
            }
            break;

        default:
            /* Invalidate desired spare type.
             */
            fbe_base_service_trace((fbe_base_service_t *) &topology_service,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid desired spare type: %d",
                                __FUNCTION__, spare_drive_pool_p->desired_spare_drive_type);
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* allocate the new packet to query the provision virtual drive object to
     * get the drive information.
     */
    new_packet_p = fbe_transport_allocate_packet();
    if(new_packet_p == NULL)
    {
        fbe_base_service_trace((fbe_base_service_t *) &topology_service,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Packet allocation failed.",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_transport_initialize_sep_packet(new_packet_p);

	fbe_transport_get_packet_attr(packet, &caller_attr);
	fbe_transport_set_packet_attr(new_packet_p, caller_attr);
	fbe_transport_clear_packet_attr(new_packet_p,FBE_PACKET_FLAG_COMPLETION_SEMAPHORE_INITIALIZED);/*we don't want to carry that*/

    /* scan through the topology table for provision drive objects. */
    for(object_id = 0; object_id < max_supported_objects ; object_id ++)
    {
        topology_get_object_class_id(object_id, &class_id);
        if(class_id == FBE_CLASS_ID_PROVISION_DRIVE)
        {
            fbe_transport_packet_clear_callstack_depth(new_packet_p);/* reset this since we reuse the packet */
            /* send the get drive info to get the provision drive object information. */
            status = topology_get_provision_drive_info(new_packet_p, object_id, &provision_drive_info);
            if (status != FBE_STATUS_OK)
            {
                /*the object was there and was removed, let's go to the next one*/
                continue;
            }

            /* Based on the `desired spare type' determine if the drive is a
             * valid spare or not.
             */
            if ((spare_drive_pool_p->desired_spare_drive_type == FBE_TOPOLOGY_GET_SPARE_DRIVE_TYPE_UNCONSUMED) &&
                (provision_drive_info.config_type != FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED)                  )
            {
                /* Desired spare type was `unconsumed' but configured pvd type 
                 * was not.
                 */
                continue;
            }
            else if ((spare_drive_pool_p->desired_spare_drive_type == FBE_TOPOLOGY_GET_SPARE_DRIVE_TYPE_TEST_SPARE) &&
                     (provision_drive_info.config_type != FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE)         )
            {
                /* Desired spare type was `test spare' but configured pvd type 
                 * was not.
                 */
                continue;
            }

            /*! @note We must allow system drives to be used as spares, but this 
             *        is an edge case so print an informational message.
             */
            if (provision_drive_info.is_system_drive == FBE_TRUE)
            {
                /* Log an informational trace and continue.
                 */
                fbe_base_service_trace((fbe_base_service_t *)&topology_service,
                                       FBE_TRACE_LEVEL_INFO,
                                       FBE_TRACE_MESSAGE_ID_INFO,
                                       "%s system pvd: 0x%x(%d_%d_%d) type: %d offset: 0x%llx \n",
                                       __FUNCTION__, object_id, 
                                       provision_drive_info.port_number, provision_drive_info.enclosure_number, provision_drive_info.slot_number,
                                       provision_drive_info.config_type, (unsigned long long)provision_drive_info.default_offset);
            }

            /* send the usurpur command to get the upstream object list. */
            vd_object_id = FBE_OBJECT_ID_INVALID;
            fbe_transport_packet_clear_callstack_depth(new_packet_p);/* reset this since we reuse the packet */
            status = topology_get_provision_drive_upstream_vd_object_id(new_packet_p, object_id, provision_drive_info.is_system_drive,
                                                                        &vd_object_id);
            if (status != FBE_STATUS_OK)
            {
                /* the object was there and it might be removed, let's go to 
                 * the next one. 
                 */
                fbe_base_service_trace((fbe_base_service_t *)&topology_service,
                                       FBE_TRACE_LEVEL_WARNING,
                                       FBE_TRACE_MESSAGE_ID_INFO,
                                       "%s Get upstream pvd: 0x%x(%d_%d_%d) type: %d failed. status: 0x%x\n",
                                       __FUNCTION__, object_id, 
                                       provision_drive_info.port_number, provision_drive_info.enclosure_number, provision_drive_info.slot_number,
                                       provision_drive_info.config_type, status);
                continue;
            }

            /* If an upstream virtual drive is detected it is an error (since 
             * we have already validated that the configuration type is either):
             *  o Unconsumed
             *      OR
             *  o Test Spare
             */
            if (vd_object_id != FBE_OBJECT_ID_INVALID)
            {
                /* Log an error trace and continue.
                 */
                fbe_base_service_trace((fbe_base_service_t *)&topology_service,
                                       FBE_TRACE_LEVEL_INFO,
                                       FBE_TRACE_MESSAGE_ID_INFO,
                                       "%s Unexpected vd obj: 0x%x pvd: 0x%x(%d_%d_%d) type: %d \n",
                                       __FUNCTION__, vd_object_id, object_id, 
                                       provision_drive_info.port_number, provision_drive_info.enclosure_number, provision_drive_info.slot_number,
                                       provision_drive_info.config_type);
                continue;
            }

            /* select this drive for the spare since all the validation passed. */
            spare_drive_pool_p->spare_object_list[spare_drive_pool_p->number_of_spare] = object_id;
            spare_drive_pool_p->number_of_spare++;                    

            if (spare_drive_pool_p->number_of_spare == FBE_MAX_SPARE_OBJECTS)
            {
                /* Break the loop if we have found enough spare. */
                break;
            }
        }
    }

    fbe_transport_release_packet(new_packet_p);

    /* complete the packet with good status. */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * topology_get_provision_drive_info()
 ****************************************************************
 * @brief
 * This function sends the get drive info control packet to 
 * provision virtual.drive object and in return it gets drive 
 * information from provision drive object.
 *
 * @param packet_p - The packet requesting this operation.
 * @param object_id - Provision virtual drive object id.
 * @param provision_drive_info_p - Provision drive info returned 
 * by PVD object.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  08/03/2009 - Created. Dhaval Patel
 ****************************************************************/
static fbe_status_t
topology_get_provision_drive_info(fbe_packet_t * packet,
                                  fbe_object_id_t object_id,
                                  fbe_provision_drive_info_t *provision_drive_info_p)
{
    fbe_status_t status;
    fbe_payload_ex_t  * payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_payload_control_status_t  control_status;
    fbe_semaphore_t sem;

    fbe_semaphore_init(&sem, 0, 1);


    payload_p = fbe_transport_get_payload_ex(packet);
    control_operation_p = fbe_payload_ex_allocate_control_operation(payload_p);
    if(control_operation_p == NULL) 
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Build the payload control operation with GET drive info control packet.
     */
    fbe_payload_control_build_operation(control_operation_p, 
                                        FBE_PROVISION_DRIVE_CONTROL_CODE_GET_PROVISION_DRIVE_INFO,
                                        provision_drive_info_p,
                                        sizeof(fbe_provision_drive_info_t));

    status = fbe_transport_set_completion_function(packet, 
                                                   topology_get_info_completion,
                                                   &sem);
    if(status != FBE_STATUS_OK) 
    {
        fbe_payload_ex_release_control_operation(payload_p, control_operation_p);
        return status;
    }

    /* Send the control packet to specific provision virtual drive object.
     */
    status = fbe_transport_set_address( packet,
                                        FBE_PACKAGE_ID_SEP_0,
                                        FBE_SERVICE_ID_TOPOLOGY,
                                        FBE_CLASS_ID_INVALID,
                                        object_id);
    if(status  != FBE_STATUS_OK) 
    {
        fbe_payload_ex_release_control_operation(payload_p, control_operation_p);
        return status;
    }

    fbe_payload_ex_increment_control_operation_level(payload_p);

    status = topology_send_mgmt_packet(object_id, packet);
    if(status  != FBE_STATUS_OK) 
    {
        fbe_payload_ex_release_control_operation(payload_p, control_operation_p);
        fbe_semaphore_destroy(&sem); /* SAFEBUG - needs destroy */
        return status;
    }

    /* Wait for the operation to complete.
     */
    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    /* Save the status for returning.
     */
    status = fbe_transport_get_status_code(packet);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_ex_release_control_operation(payload_p, control_operation_p);

    /* Return the status code based on the status of the packet and payload status.
     */
    return ((status == FBE_STATUS_OK && control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) ? FBE_STATUS_OK :FBE_STATUS_GENERIC_FAILURE) ;
}

/*!**************************************************************
 * topology_get_provision_drive_location()
 ****************************************************************
 * @brief
 * This function sends the get drive info control packet to 
 * topology service and in return it gets drive locatio 
 * information from provision drive object.
 *
 * @param packet_p - The packet requesting this operation.
 * @param object_id - Provision virtual drive object id.
 * @param provision_drive_info_p - Provision drive info returned 
 * by PVD object.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/26/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t
topology_get_provision_drive_location(fbe_packet_t * packet,
                                      fbe_object_id_t object_id,
                                      fbe_physical_drive_get_location_t *drive_location_p)
{
    fbe_status_t status;
    fbe_payload_ex_t  * payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_payload_control_status_t  control_status;
    fbe_semaphore_t sem;

    payload_p = fbe_transport_get_payload_ex(packet);
    control_operation_p = fbe_payload_ex_allocate_control_operation(payload_p);
    if(control_operation_p == NULL) 
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now issue the get drive info to the proper pdo */
    fbe_semaphore_init(&sem, 0, 1);

    /* Build the payload control operation with GET drive info control packet.
     */
    fbe_payload_control_build_operation(control_operation_p, 
                                        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_LOCATION,
                                        drive_location_p,
                                        sizeof(fbe_physical_drive_get_location_t));

    status = fbe_transport_set_completion_function(packet, 
                                                   topology_get_info_completion,
                                                   &sem);
    if(status != FBE_STATUS_OK) 
    {
        fbe_payload_ex_release_control_operation(payload_p, control_operation_p);
        fbe_semaphore_destroy(&sem);
        return status;
    }

    /*! @todo DE200 - This should not be sent to the topology service since this control 
     *                code should be going to an object (the logical drive object).
     */
    status = fbe_transport_set_address( packet,
                                        FBE_PACKAGE_ID_SEP_0,
                                        FBE_SERVICE_ID_TOPOLOGY,
                                        FBE_CLASS_ID_INVALID,
                                        object_id);
    if(status  != FBE_STATUS_OK) 
    {
        fbe_payload_ex_release_control_operation(payload_p, control_operation_p);
        fbe_semaphore_destroy(&sem);
        return status;
    }

    /* Mark packet for traversal. */
    fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_TRAVERSE);

    fbe_payload_ex_increment_control_operation_level(payload_p);

    status = topology_send_mgmt_packet(object_id, packet);
    if(status  != FBE_STATUS_OK) 
    {
        fbe_payload_ex_release_control_operation(payload_p, control_operation_p);
        fbe_semaphore_destroy(&sem); /* SAFEBUG - needs destroy */
        return status;
    }

    /* Wait for the operation to complete.
     */
    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    /* Save the status for returning.
     */
    status = fbe_transport_get_status_code(packet);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_ex_release_control_operation(payload_p, control_operation_p);

    /* Return the status code based on the status of the packet and payload status.
     */
    return ((status == FBE_STATUS_OK && control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) ? FBE_STATUS_OK :FBE_STATUS_GENERIC_FAILURE) ;
}

/*!***************************************************************************
 *          topology_get_provision_drive_upstream_vd_object_id()
 *****************************************************************************
 * @brief
 * This function sends the get drive info control packet to 
 * provision virtual.drive object and in return it gets drive 
 * information from provision drive object.
 *
 * @param packet_p       - The packet requesting this operation.
 * @param pvd_object_id  - Provision virtual drive object id.
 * @param b_is_system_drive - FBE_TRUE - Is system drive (validate that none of
 *          the upstream objects are vds)
 *                          - FBE_FALSE - We don't expect ANY upstream objects
 * @param vd_object_id_p - Pointer to virtual drive object id.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  04/05/2010 - Created. Dhaval Patel
 *****************************************************************************/
static fbe_status_t topology_get_provision_drive_upstream_vd_object_id(fbe_packet_t *packet,
                                                                       fbe_object_id_t pvd_object_id,
                                                                       fbe_bool_t b_is_system_drive,
                                                                       fbe_object_id_t *vd_object_id_p)
{
    fbe_status_t                            status;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_base_config_upstream_object_list_t  pvd_upstream_object_list;
    fbe_payload_control_operation_t        *control_operation_p = NULL;
    fbe_payload_control_status_t            control_status;
    fbe_semaphore_t                         sem;
    fbe_class_id_t                          class_id = FBE_CLASS_ID_INVALID;
    fbe_u32_t                               upstream_index = 0;

    /* Initialize the upstream virtual drive object as invalid. */
    fbe_semaphore_init(&sem, 0, 1);
    *vd_object_id_p = FBE_OBJECT_ID_INVALID;
    pvd_upstream_object_list.number_of_upstream_objects = 0;
    pvd_upstream_object_list.upstream_object_list[0] = FBE_OBJECT_ID_INVALID;
    
    payload_p = fbe_transport_get_payload_ex(packet);
    control_operation_p = fbe_payload_ex_allocate_control_operation(payload_p);
    if(control_operation_p == NULL) 
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Build the payload control operation with GET drive info control packet. */
    fbe_payload_control_build_operation(control_operation_p, 
                                        FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_OBJECT_LIST,
                                        &pvd_upstream_object_list,
                                        sizeof(fbe_base_config_upstream_object_list_t));

    status = fbe_transport_set_completion_function(packet, 
                                                   topology_get_info_completion,
                                                   &sem);
    if (status != FBE_STATUS_OK) 
    {
        fbe_payload_ex_release_control_operation(payload_p, control_operation_p);
        return status;
    }

    /* Send the control packet to specific provision virtual drive object. */
    status = fbe_transport_set_address( packet,
                                        FBE_PACKAGE_ID_SEP_0,
                                        FBE_SERVICE_ID_TOPOLOGY,
                                        FBE_CLASS_ID_INVALID,
                                        pvd_object_id);
    if(status  != FBE_STATUS_OK) 
    {
        fbe_payload_ex_release_control_operation(payload_p, control_operation_p);
        return status;
    }

    /* Mark packet for traversal. */
    fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_TRAVERSE);

    fbe_payload_ex_increment_control_operation_level(payload_p);

    status = topology_send_mgmt_packet(pvd_object_id, packet);
    if(status  != FBE_STATUS_OK) 
    {
        fbe_payload_ex_release_control_operation(payload_p, control_operation_p);
        fbe_semaphore_destroy(&sem); /* SAFEBUG - needs destroy */
        return status;
    }

    /* Wait for the operation to complete. */
    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    /* Save the status for returning. */
    status = fbe_transport_get_status_code(packet);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_ex_release_control_operation(payload_p, control_operation_p);

    /* Return the status code based on the status of the packet and payload status. */
    if((status == FBE_STATUS_OK) && (control_status == FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        /* If there are upstream objects first check if this is a system disk
         * or not.
         */
        if (pvd_upstream_object_list.number_of_upstream_objects > 0)
        {
            /*! @note If this is a system disk we don't expect ANY upstream objects. 
             */
            if (b_is_system_drive == FBE_FALSE)
            {
                /* No need to get the class id of the upstream object, return the
                 * object id of the first item.
                 */
                *vd_object_id_p = pvd_upstream_object_list.upstream_object_list[0];
                return FBE_STATUS_OK;
            }
            else
            {
                /* Else walk thru the list and validate that none of the upstream
                 * objects is a virtual drive.
                 */
                for (upstream_index = 0; 
                     upstream_index < pvd_upstream_object_list.number_of_upstream_objects; 
                     upstream_index++)
                {
                    /*! @note Get the class id of the upstream object. 
                     *
                     *  @note Currently we allow any upstream object as long as
                     *        the class is NOT:
                     *          o Virtual Drive
                     */
                    status = topology_get_object_class_id(pvd_upstream_object_list.upstream_object_list[upstream_index],
                                                          &class_id);
                    if (status != FBE_STATUS_OK)
                    {
                        return status;
                    }
                    if (class_id == FBE_CLASS_ID_VIRTUAL_DRIVE)
                    {
                        /* We found a virtual drive, populate the vd object id
                         * and return Success.
                         */
                        *vd_object_id_p = pvd_upstream_object_list.upstream_object_list[upstream_index];
                        return FBE_STATUS_OK;
                    }
                }

                /* There were not upstream virtual drives.  Return success.
                 */
                return FBE_STATUS_OK;
            }
        }
        else
        {
            /* Else no upstream objects (typically case).  Return success.
             */
            return FBE_STATUS_OK;
        }
    }
    else
    {
        *vd_object_id_p = FBE_OBJECT_ID_INVALID;
        return FBE_STATUS_GENERIC_FAILURE;
    }
}

static fbe_status_t
topology_enumerate_class(fbe_packet_t *  packet)
{
    fbe_topology_control_enumerate_class_t * topology_mgmt_enumerate_class = NULL;
    fbe_object_id_t                         id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                               i  =0;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t *                         payload = NULL;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_sg_element_t *                      sg_element = NULL;
    fbe_u32_t                               sg_elements = 0;
    fbe_object_id_t *                       obj_id_list = NULL;
    fbe_u32_t                               total_objects_copied = 0;

    fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_topology_control_enumerate_class_t))
    {
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Error Invalid control_buffer length", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &topology_mgmt_enumerate_class); 
    if (topology_mgmt_enumerate_class == NULL)
    {
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*we need the sg list in order to fill this memory with the object IDs*/
    fbe_payload_ex_get_sg_list(payload, &sg_element, &sg_elements);

    obj_id_list = (fbe_object_id_t *)sg_element->address;

    /*sanity check*/
    if ((sg_element->count / sizeof(fbe_object_id_t)) != topology_mgmt_enumerate_class->number_of_objects)
    {
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s User allocated memory is not the same as objects to copy\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;

    }

    /* Clean up the array */
    for (i = 0 ; i < topology_mgmt_enumerate_class->number_of_objects; i++, obj_id_list ++)
    {
        *obj_id_list = FBE_OBJECT_ID_INVALID;
    }

    /* Scan the object_table */
    obj_id_list = (fbe_object_id_t *)sg_element->address;
    for (id = 0; id < max_supported_objects ; id++)
    {
        fbe_class_id_t class_id;
        topology_get_object_class_id(id, &class_id);

        /* Include this object in the enumeration if its class id matches the one 
         * we are looking for. 
         * FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST implies all physical drives
         */
        if (((topology_mgmt_enumerate_class->class_id == class_id ) ||
             ((topology_mgmt_enumerate_class->class_id == FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST) &&
              (class_id > FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST) &&
              (class_id < FBE_CLASS_ID_PHYSICAL_DRIVE_LAST )))&& 
            topology_is_object_ready(id))
        {
            if (total_objects_copied >= topology_mgmt_enumerate_class->number_of_objects)
            {
                /* We found more objects than expected.
                 */
                total_objects_copied++;
                break;
            }
            obj_id_list[total_objects_copied] = id;
            total_objects_copied++;
        }
    }

    topology_mgmt_enumerate_class->number_of_objects_copied = total_objects_copied;

    fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                           FBE_TRACE_LEVEL_DEBUG_LOW,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s number_of_objects 0x%X \n",
                           __FUNCTION__, total_objects_copied);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
static fbe_status_t
topology_enumerate_ports_by_role(fbe_packet_t *  packet)
{
    fbe_topology_control_enumerate_ports_by_role_t * topology_mgmt_enumerate_ports = NULL;
    fbe_object_id_t                         id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                               i  =0;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t *                         payload = NULL;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_sg_element_t *                      sg_element = NULL;
    fbe_u32_t                               sg_elements = 0;
    fbe_object_id_t *                       obj_id_list = NULL;
    fbe_u32_t                               total_objects_copied = 0;
    fbe_packet_t *                          new_packet = NULL;
	fbe_packet_attr_t						caller_attr;

    fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_topology_control_enumerate_ports_by_role_t))
    {
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Error Invalid control_buffer length", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &topology_mgmt_enumerate_ports); 
    if (topology_mgmt_enumerate_ports == NULL)
    {
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*we need the sg list in order to fill this memory with the object IDs*/
    fbe_payload_ex_get_sg_list(payload, &sg_element, &sg_elements);

    obj_id_list = (fbe_object_id_t *)sg_element->address;

    /*sanity check*/
    if ((sg_element->count / sizeof(fbe_object_id_t)) != topology_mgmt_enumerate_ports->number_of_objects)
    {
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s User allocated memory is not the same as objects to copy\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;

    }

    /* Clean up the array */
    for (i = 0 ; i < topology_mgmt_enumerate_ports->number_of_objects; i++, obj_id_list ++)
    {
        *obj_id_list = FBE_OBJECT_ID_INVALID;
    }

    /* Scan the object_table for first ports then `port_role' */
    obj_id_list = (fbe_object_id_t *)sg_element->address;
    new_packet = fbe_transport_allocate_packet();
    fbe_transport_initialize_packet(new_packet);

	fbe_transport_get_packet_attr(packet, &caller_attr);
	fbe_transport_set_packet_attr(new_packet, caller_attr);
	fbe_transport_clear_packet_attr(new_packet,FBE_PACKET_FLAG_COMPLETION_SEMAPHORE_INITIALIZED);/*we don't want to carry that*/

    for (id = 0; id < max_supported_objects ; id++)
    {
        fbe_class_id_t  class_id;

        topology_get_object_class_id(id, &class_id);
        if((class_id > FBE_CLASS_ID_PORT_FIRST) && (class_id < FBE_CLASS_ID_PORT_LAST))
        {
            fbe_port_role_t port_role;

            topology_get_port_role(new_packet, id, &port_role);

            /* Include this object in the enumeration if its class id matches
             * the one we are looking for. 
             */
            if ((port_role == topology_mgmt_enumerate_ports->port_role) && 
                topology_is_object_ready(id))
            {
                if (total_objects_copied >= topology_mgmt_enumerate_ports->number_of_objects)
                {
                    /* We found more objects than expected.
                     */
                    total_objects_copied++;
                    break;
                }
                obj_id_list[total_objects_copied] = id;
                total_objects_copied++;
            }
        }
    }

    fbe_transport_release_packet(new_packet);

    topology_mgmt_enumerate_ports->number_of_objects_copied = total_objects_copied;

    fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                           FBE_TRACE_LEVEL_DEBUG_LOW,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s number_of_objects 0x%X \n",
                           __FUNCTION__, total_objects_copied);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t
topology_class_command(fbe_class_id_t class_id, fbe_packet_t *  packet)
{
    const fbe_class_methods_t * pclass = NULL; 
    fbe_status_t status;

    fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s entry\n", __FUNCTION__);

    pclass = topology_get_class(class_id);
    if (pclass == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s class id invalid 0x%x\n", __FUNCTION__, class_id);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = pclass->control_entry(NULL, packet); 
    
    return status;
}

static fbe_status_t fbe_topology_get_total_objects(fbe_packet_t *  packet)
{
    fbe_topology_control_get_total_objects_t *  fbe_topology_control_get_total_objects = NULL;
    fbe_object_id_t                             id = FBE_OBJECT_ID_INVALID;
    fbe_payload_control_buffer_length_t         len = 0;
    fbe_payload_ex_t *                          payload = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL;

    fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s entry\n", __FUNCTION__);
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len != sizeof(fbe_topology_control_get_total_objects_t)){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Error Invalid control_buffer length", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_topology_control_get_total_objects); 
    if(fbe_topology_control_get_total_objects == NULL){
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Scan the object_table */
    fbe_topology_control_get_total_objects->total_objects = 0;
    for(id = 0; id < max_supported_objects ; id++) {
        if(topology_is_object_ready(id)) {
            fbe_topology_control_get_total_objects->total_objects++;
        }
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}
static fbe_status_t fbe_topology_get_total_objects_of_class(fbe_packet_t *  packet)
{
    fbe_topology_control_get_total_objects_of_class_t *     fbe_topology_control_get_total_objects_of_class = NULL;
    fbe_object_id_t                             id = FBE_OBJECT_ID_INVALID;
    fbe_payload_control_buffer_length_t         len = 0;
    fbe_payload_ex_t *                          payload = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL;

    fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s entry\n", __FUNCTION__);
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len != sizeof(fbe_topology_control_get_total_objects_of_class_t)){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Error Invalid control_buffer length", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_topology_control_get_total_objects_of_class); 
    if(fbe_topology_control_get_total_objects_of_class == NULL){
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Scan the object_table */
    fbe_topology_control_get_total_objects_of_class->total_objects = 0;
    for(id = 0; id < max_supported_objects ; id++) {
        fbe_class_id_t class_id;
        topology_get_object_class_id(id, &class_id);

        /* Include this object in the enumeration if its class id matches the one 
         * we are looking for. 
         * FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST implies all physical drives.
         */
        if (((class_id == fbe_topology_control_get_total_objects_of_class->class_id)||
             ((fbe_topology_control_get_total_objects_of_class->class_id == FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST) &&
              (class_id > FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST) &&
              (class_id < FBE_CLASS_ID_PHYSICAL_DRIVE_LAST )) ||
             ((fbe_topology_control_get_total_objects_of_class->class_id == FBE_CLASS_ID_LUN) &&
              (class_id == FBE_CLASS_ID_LUN || class_id == FBE_CLASS_ID_EXTENT_POOL_LUN)) )&& 
            topology_is_object_ready(id)) {
            fbe_topology_control_get_total_objects_of_class->total_objects++;
        }
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}

static fbe_status_t topology_get_physical_logical_drive_by_sn(fbe_packet_t *  packet)
{
    fbe_topology_control_get_logical_drive_by_sn_t *    get_drive_by_sn_ptr = NULL;
    fbe_physical_drive_mgmt_get_drive_information_t *   physical_drive_mgmt_get_drive_information = NULL;
    fbe_object_id_t                                     id = FBE_OBJECT_ID_INVALID;
    fbe_payload_control_buffer_length_t                 len = 0;
    fbe_payload_ex_t *                                  payload = NULL;
    fbe_payload_control_operation_t *                   control_operation = NULL;
    fbe_class_id_t                                      class_id = FBE_CLASS_ID_INVALID;
    fbe_s32_t                                           compare_result = 0;
    fbe_status_t                                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                                      new_packet = NULL;
	fbe_packet_attr_t									caller_attr;

    fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s entry\n", __FUNCTION__);
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len != sizeof(fbe_topology_control_get_logical_drive_by_sn_t)){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Error Invalid control_buffer length", __FUNCTION__);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &get_drive_by_sn_ptr); 
    if(get_drive_by_sn_ptr == NULL){
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Scan topology table for physical drive objects */
    new_packet = fbe_transport_allocate_packet();
    fbe_transport_initialize_packet(new_packet);

	fbe_transport_get_packet_attr(packet, &caller_attr);
	fbe_transport_set_packet_attr(new_packet, caller_attr);
	fbe_transport_clear_packet_attr(new_packet,FBE_PACKET_FLAG_COMPLETION_SEMAPHORE_INITIALIZED);/*we don't want to carry that*/

    /*allocate memory for getting the drive info*/
    physical_drive_mgmt_get_drive_information = (fbe_physical_drive_mgmt_get_drive_information_t *)fbe_memory_native_allocate(sizeof(fbe_physical_drive_mgmt_get_drive_information_t));

    for(id = 0; id < max_supported_objects ; id ++) {
        topology_get_object_class_id(id, &class_id);
        if(class_id == FBE_CLASS_ID_LOGICAL_DRIVE){
                fbe_transport_packet_clear_callstack_depth(new_packet);/* reset this since we reuse the packet */
                status = topology_get_physical_drive_cached_info(new_packet, id, physical_drive_mgmt_get_drive_information);

                if (status == FBE_STATUS_NO_OBJECT) {
                    /*the object was there and was removed, let's go to the next one*/
                    fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                                           FBE_TRACE_LEVEL_INFO,
                                           FBE_TRACE_MESSAGE_ID_INFO,
                                           "%s pdo: 0x%x has been removed.\n", 
                                           __FUNCTION__, id);
                    continue;
                }

                if(status != FBE_STATUS_OK){
                    fbe_transport_release_packet(new_packet);
                    fbe_memory_native_release(physical_drive_mgmt_get_drive_information);
                    fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                                           FBE_TRACE_LEVEL_ERROR, 
                                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                           "%s failed to get cached drive info\n", __FUNCTION__);
                    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
                    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                    fbe_transport_complete_packet(packet);
                    return FBE_STATUS_GENERIC_FAILURE;
                }

                /*compare sn*/
                compare_result = fbe_compare_string(physical_drive_mgmt_get_drive_information->get_drive_info.product_serial_num,
                                                    get_drive_by_sn_ptr->sn_size,
                                                    get_drive_by_sn_ptr->product_serial_num,
                                                    get_drive_by_sn_ptr->sn_size,
                                                    FBE_FALSE);

                /*and see if there is a match*/
                if(compare_result == 0){
                        get_drive_by_sn_ptr->object_id = id;

                        fbe_transport_release_packet(new_packet);
                        fbe_memory_native_release(physical_drive_mgmt_get_drive_information);
                        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
                        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
                        fbe_transport_complete_packet(packet);
                        return FBE_STATUS_OK;
                    }
        }

    }

    /* We failed to locate the request drive.  This can happen since the drive
     * can be removed at any time.  Simply report an warning. 
     */ 
    fbe_transport_release_packet(new_packet);
    fbe_memory_native_release(physical_drive_mgmt_get_drive_information);

    get_drive_by_sn_ptr->object_id = FBE_OBJECT_ID_INVALID;
    fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                           FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s Failed to find a match for SN:%s \n", 
                           __FUNCTION__, get_drive_by_sn_ptr->product_serial_num);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    fbe_payload_control_set_status_qualifier(control_operation, FBE_OBJECT_ID_INVALID);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}


static fbe_status_t topology_get_physical_drive_by_sn(fbe_packet_t *  packet)
{
    fbe_topology_control_get_physical_drive_by_sn_t *   get_drive_by_sn_ptr = NULL;
    fbe_physical_drive_mgmt_get_drive_information_t *   physical_drive_mgmt_get_drive_information = NULL;
    fbe_object_id_t                                     id = FBE_OBJECT_ID_INVALID;
    fbe_payload_control_buffer_length_t                 len = 0;
    fbe_payload_ex_t *                                  payload = NULL;
    fbe_payload_control_operation_t *                   control_operation = NULL;
    fbe_class_id_t                                      class_id = FBE_CLASS_ID_INVALID;
    fbe_s32_t                                           compare_result = 0;
    fbe_status_t                                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                                      new_packet = NULL;
	fbe_packet_attr_t									caller_attr;

    fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s entry\n", __FUNCTION__);
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len != sizeof(fbe_topology_control_get_physical_drive_by_sn_t)){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Error Invalid control_buffer length", __FUNCTION__);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &get_drive_by_sn_ptr); 
    if(get_drive_by_sn_ptr == NULL){
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Scan topology table for physical drive objects */
    new_packet = fbe_transport_allocate_packet();
    fbe_transport_initialize_packet(new_packet);

	fbe_transport_get_packet_attr(packet, &caller_attr);
	fbe_transport_set_packet_attr(new_packet, caller_attr);
	fbe_transport_clear_packet_attr(new_packet,FBE_PACKET_FLAG_COMPLETION_SEMAPHORE_INITIALIZED);/*we don't want to carry that*/

    /*allocate memory for getting the drive info*/
    physical_drive_mgmt_get_drive_information = (fbe_physical_drive_mgmt_get_drive_information_t *)fbe_memory_native_allocate(sizeof(fbe_physical_drive_mgmt_get_drive_information_t));

    for(id = 0; id < max_supported_objects ; id ++) {
        topology_get_object_class_id(id, &class_id);
        if ((class_id > FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST) && (class_id < FBE_CLASS_ID_PHYSICAL_DRIVE_LAST)) {
                fbe_transport_packet_clear_callstack_depth(new_packet);/* reset this since we reuse the packet */
                status = topology_get_physical_drive_cached_info(new_packet, id, physical_drive_mgmt_get_drive_information);

                if (status == FBE_STATUS_NO_OBJECT) {
                    /*the object was there and was removed, let's go to the next one*/
                    fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                                           FBE_TRACE_LEVEL_INFO,
                                           FBE_TRACE_MESSAGE_ID_INFO,
                                           "%s pdo: 0x%x has been removed.\n", 
                                           __FUNCTION__, id);
                    continue;
                }

                if (status == FBE_STATUS_BUSY) {
                    /*the object was just created, let's go to the next one*/
                    fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                                           FBE_TRACE_LEVEL_INFO,
                                           FBE_TRACE_MESSAGE_ID_INFO,
                                           "%s pdo: 0x%x is busy.\n", 
                                           __FUNCTION__, id);
                    continue;
                }

                if(status != FBE_STATUS_OK){
                    fbe_transport_release_packet(new_packet);
                    fbe_memory_native_release(physical_drive_mgmt_get_drive_information);
                    fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                                           FBE_TRACE_LEVEL_ERROR, 
                                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                           "%s failed to get cached drive info, status 0x%x, obj 0x%x\n", __FUNCTION__, status, id);
                    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
                    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                    fbe_transport_complete_packet(packet);
                    return FBE_STATUS_GENERIC_FAILURE;
                }

                /*compare sn*/
                compare_result = fbe_compare_string(physical_drive_mgmt_get_drive_information->get_drive_info.product_serial_num,
                                                    get_drive_by_sn_ptr->sn_size,
                                                    get_drive_by_sn_ptr->product_serial_num,
                                                    get_drive_by_sn_ptr->sn_size,
                                                    FBE_FALSE);

                /*and see if there is a match*/
                if(compare_result == 0){
                        get_drive_by_sn_ptr->object_id = id;

                        fbe_transport_release_packet(new_packet);
                        fbe_memory_native_release(physical_drive_mgmt_get_drive_information);
                        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
                        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
                        fbe_transport_complete_packet(packet);
                        return FBE_STATUS_OK;
                    }
        }

    }

    /* We failed to locate the request drive.  This can happen since the drive
     * can be removed at any time.  Simply report an warning. 
     */ 
    fbe_transport_release_packet(new_packet);
    fbe_memory_native_release(physical_drive_mgmt_get_drive_information);

    get_drive_by_sn_ptr->object_id = FBE_OBJECT_ID_INVALID;
    fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                           FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s Failed to find a match for SN:%s \n", 
                           __FUNCTION__, get_drive_by_sn_ptr->product_serial_num);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    fbe_payload_control_set_status_qualifier(control_operation, FBE_OBJECT_ID_INVALID);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t topology_get_physical_drive_cached_info(fbe_packet_t *packet,
                                                            fbe_object_id_t obj_id,
                                                            fbe_physical_drive_mgmt_get_drive_information_t *get_info)
{
    fbe_status_t                        status;
    fbe_payload_ex_t                   *payload = NULL;
    fbe_payload_control_operation_t    *control_operation = NULL;

    /* We don't allow an object id of 0*/
    if (obj_id == 0) {        
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s unsupported object id: %d\n", 
                               __FUNCTION__, obj_id);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    fbe_payload_control_build_operation(control_operation, 
                                        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_CACHED_DRIVE_INFO, 
                                        get_info, 
                                        sizeof(fbe_physical_drive_mgmt_get_drive_information_t));

    
    status = fbe_transport_set_address( packet,
                                        FBE_PACKAGE_ID_PHYSICAL,
                                        FBE_SERVICE_ID_TOPOLOGY,
                                        FBE_CLASS_ID_INVALID,
                                        obj_id);

    fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_TRAVERSE);/*traverse it to the physical drive*/
    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    fbe_payload_ex_increment_control_operation_level(payload);

    topology_send_mgmt_packet(obj_id, packet);

    fbe_transport_wait_completion(packet);
  
    /* get the packet status. */
    status = fbe_transport_get_status_code (packet);

    fbe_payload_ex_release_control_operation(payload, control_operation);

    return status;
}


static fbe_status_t
topology_get_board(fbe_packet_t *  packet)
{
    fbe_topology_control_get_board_t * boardInfo = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_length_t buffer_length;
    fbe_object_id_t id;
    fbe_class_id_t class_id;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &boardInfo);
    if(boardInfo == NULL){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed", __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length < sizeof(fbe_topology_control_get_board_t)){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid buffer_length %d", __FUNCTION__, buffer_length);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Scan topology table for logical drive objects */
    for(id = 0; id < max_supported_objects ; id ++) {
        topology_get_object_class_id(id, &class_id);
        if((class_id > FBE_CLASS_ID_BOARD_FIRST) && (class_id < FBE_CLASS_ID_BOARD_LAST)){
            // only one Board Object, so return ID
            boardInfo->board_object_id = id;
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
            fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
        }
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    fbe_payload_control_set_status_qualifier(control_operation, FBE_OBJECT_ID_INVALID);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_topology_class_id_to_object_type(fbe_class_id_t class_id, fbe_topology_object_type_t *object_type)
{
    if((class_id >= FBE_CLASS_ID_BASE_OBJECT) && (class_id <= FBE_CLASS_ID_BASE_DISCOVERING)){
        /* Object hasn't completed specializing. */
        *object_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
    }else if((class_id > FBE_CLASS_ID_BOARD_FIRST) && (class_id < FBE_CLASS_ID_BOARD_LAST)){
        *object_type = FBE_TOPOLOGY_OBJECT_TYPE_BOARD;
    }else if((class_id > FBE_CLASS_ID_PORT_FIRST) && (class_id < FBE_CLASS_ID_PORT_LAST)){
        *object_type = FBE_TOPOLOGY_OBJECT_TYPE_PORT;
    }else if((class_id > FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST) && (class_id < FBE_CLASS_ID_PHYSICAL_DRIVE_LAST)){
        *object_type = FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE;
    }else if ((class_id > FBE_CLASS_ID_ENCLOSURE_FIRST) && (class_id < FBE_CLASS_ID_ENCLOSURE_LAST)){
        *object_type = FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE;
    }else if ((class_id > FBE_CLASS_ID_LCC_FIRST) && (class_id < FBE_CLASS_ID_LCC_LAST)){ 
        *object_type = FBE_TOPOLOGY_OBJECT_TYPE_LCC;   
    }else if ((class_id > FBE_CLASS_ID_LOGICAL_DRIVE_FIRST) && (class_id < FBE_CLASS_ID_LOGICAL_DRIVE_LAST)){
        *object_type = FBE_TOPOLOGY_OBJECT_TYPE_LOGICAL_DRIVE;
    }else if ((class_id > FBE_CLASS_ID_RAID_FIRST) && (class_id < FBE_CLASS_ID_RAID_LAST)){
        *object_type = FBE_TOPOLOGY_OBJECT_TYPE_RAID_GROUP;
    }else if (class_id == FBE_CLASS_ID_VIRTUAL_DRIVE){
        *object_type = FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_DRIVE;
    }else if (class_id == FBE_CLASS_ID_PROVISION_DRIVE){
        *object_type = FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE;
    }else if (class_id == FBE_CLASS_ID_LUN){
        *object_type = FBE_TOPOLOGY_OBJECT_TYPE_LUN;
    }else if ((class_id > FBE_CLASS_ID_ENVIRONMENT_MGMT_FIRST) && (class_id < FBE_CLASS_ID_ENVIRONMENT_MGMT_LAST)){
        *object_type = FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT;
    }else if ((class_id > FBE_CLASS_ID_LCC_FIRST) && (class_id < FBE_CLASS_ID_LCC_LAST)){
        *object_type = FBE_TOPOLOGY_OBJECT_TYPE_LCC;
    }else if (class_id == FBE_CLASS_ID_BVD_INTERFACE){
        *object_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
    }else if (class_id == FBE_CLASS_ID_EXTENT_POOL){
        //*object_type = FBE_TOPOLOGY_OBJECT_TYPE_EXTENT_POOL;
#ifndef NO_EXT_POOL_ALIAS
        *object_type = FBE_TOPOLOGY_OBJECT_TYPE_RAID_GROUP;
#endif
    }else if (class_id == FBE_CLASS_ID_EXTENT_POOL_LUN){
        //*object_type = FBE_TOPOLOGY_OBJECT_TYPE_EXT_POOL_LUN;
#ifndef NO_EXT_POOL_ALIAS
        *object_type = FBE_TOPOLOGY_OBJECT_TYPE_LUN;
#endif
    }else if (class_id == FBE_CLASS_ID_EXTENT_POOL_METADATA_LUN){
        *object_type = FBE_TOPOLOGY_OBJECT_TYPE_EXT_POOL_LUN;
    }else {
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s Unknown class_id %d\n", __FUNCTION__, class_id);
        *object_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;

}

static fbe_status_t topology_get_bvd_id(fbe_packet_t *  packet)
{
    fbe_topology_control_get_bvd_id_t * bvd_info = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_length_t buffer_length;
    fbe_object_id_t id;
    fbe_class_id_t class_id;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &bvd_info);
    if(bvd_info == NULL){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed", __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length < sizeof(fbe_topology_control_get_bvd_id_t)){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid buffer_length %d", __FUNCTION__, buffer_length);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Scan topology table for bvd object */
    for(id = 0; id < max_supported_objects ; id ++) {
        topology_get_object_class_id(id, &class_id);
        if(class_id == FBE_CLASS_ID_BVD_INTERFACE){
            // only one Board Object, so return ID
            bvd_info->bvd_object_id = id;
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
            fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
        }
    }

    fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Failed to get BVD object ID", __FUNCTION__);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    fbe_payload_control_set_status_qualifier(control_operation, FBE_OBJECT_ID_INVALID);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}

static fbe_status_t 
topology_send_mgmt_packet_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_object_handle_t control_handle;

    control_handle = (fbe_object_handle_t)context;
    fbe_base_object_decrement_userper_counter(control_handle);
    return FBE_STATUS_OK;
}

static fbe_status_t topology_get_object_id_of_singleton_class(fbe_packet_t *  packet)
{
    fbe_topology_control_get_object_id_of_singleton_t *     fbe_topology_control_get_object_id_of_singleton = NULL;
    fbe_object_id_t                             id = FBE_OBJECT_ID_INVALID;
    fbe_payload_control_buffer_length_t         len = 0;
    fbe_payload_ex_t *                          payload = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL;

    fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s entry\n", __FUNCTION__);
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len != sizeof(fbe_topology_control_get_object_id_of_singleton_t)){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Error Invalid control_buffer length", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_topology_control_get_object_id_of_singleton); 
    if(fbe_topology_control_get_object_id_of_singleton == NULL){
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Scan the object_table */
    for(id = 0; id < max_supported_objects ; id++) {
        fbe_class_id_t class_id;
        topology_get_object_class_id(id, &class_id);

        /* Include this object in the enumeration if its class id matches the one 
         * we are looking for. 
         */
        if ((class_id == fbe_topology_control_get_object_id_of_singleton->class_id) && 
            topology_is_object_ready(id)) {
            fbe_topology_control_get_object_id_of_singleton->object_id = id;
             fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
             fbe_transport_complete_packet(packet);
             return FBE_STATUS_OK;
            
        }
    }

    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}

static fbe_status_t topology_check_object_existent(fbe_packet_t *  packet)
{
    fbe_topology_mgmt_check_object_existent_t * object_exists_ptr = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer_length(control_operation, &len);
    
    if(len != sizeof(fbe_topology_mgmt_check_object_existent_t))
    {
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Invalid buffer_len \n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &object_exists_ptr);

    if(object_exists_ptr == NULL)
    {
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);

        return FBE_STATUS_GENERIC_FAILURE;
    }
       
    if (object_exists_ptr->object_id == FBE_OBJECT_ID_INVALID)
    {
        fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Object id is invalid\n", __FUNCTION__);
        object_exists_ptr->exists_b = FBE_FALSE;
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);

        return FBE_STATUS_GENERIC_FAILURE;
    }
     
    if (topology_is_object_not_exist(object_exists_ptr->object_id))
    {
        object_exists_ptr->exists_b = FBE_FALSE;
    }
    else
    {
        object_exists_ptr->exists_b = FBE_TRUE;
    }


    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}   /* end of topology_check_object_existent() */

static fbe_status_t topology_get_physical_drive_objects(fbe_packet_t *  packet)
{
    fbe_topology_control_get_physical_drive_objects_t * get_drive_objects = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_length_t buffer_length;
    fbe_class_id_t  class_id = FBE_CLASS_ID_INVALID;
    fbe_u32_t       id = 0;
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &get_drive_objects);
    if(get_drive_objects == NULL){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length < sizeof(fbe_topology_control_get_physical_drive_objects_t)){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid buffer_length %d", __FUNCTION__, buffer_length);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Clean up the array */
    for (id = 0 ; id < FBE_MAX_OBJECTS; id++){
        get_drive_objects->pdo_list[id] = FBE_OBJECT_ID_INVALID;
    }

    /* Scan the object_table */
    get_drive_objects->number_of_objects = 0;
    for(id = 0; id < FBE_MAX_OBJECTS ; id++) {

        if (topology_is_object_ready(id)) {
            status = topology_get_object_class_id(id, &class_id);
            if(status != FBE_STATUS_OK) {
                fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                                       FBE_TRACE_LEVEL_ERROR, 
                                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                       "%s topology_get_object_class_id fail %X\n", __FUNCTION__, status);
    
                fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
    
            /*we look only for drives*/
            if ((class_id > FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST) && (class_id < FBE_CLASS_ID_PHYSICAL_DRIVE_LAST)) {
                get_drive_objects->pdo_list[get_drive_objects->number_of_objects] = id;
                get_drive_objects->number_of_objects++;
            }
        }
    }

    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t
topology_set_config_pvd_hs_flag(fbe_packet_t *  packet)
{
    fbe_topology_mgmt_set_pvd_hs_flag_t * set_hs_flag = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_length_t buffer_length;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &set_hs_flag);
    if(set_hs_flag == NULL){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length != sizeof(fbe_topology_mgmt_set_pvd_hs_flag_t)){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid buffer_length %d", __FUNCTION__, buffer_length);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    topology_b_select_only_test_spare_drives = set_hs_flag->enable;

    fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                               FBE_TRACE_LEVEL_INFO, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s Set config_pvd_as_hs_flag as %s. \n", __FUNCTION__, set_hs_flag->enable ? "Enable" : "Disable");

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t
topology_get_config_pvd_hs_flag(fbe_packet_t *  packet)
{
    fbe_topology_mgmt_get_pvd_hs_flag_t * get_hs_flag = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_length_t buffer_length;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &get_hs_flag);
    if(get_hs_flag == NULL){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length != sizeof(fbe_topology_mgmt_get_pvd_hs_flag_t)){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid buffer_length %d", __FUNCTION__, buffer_length);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    get_hs_flag->b_is_test_spare_enabled = topology_b_select_only_test_spare_drives;

    fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                           FBE_TRACE_LEVEL_INFO, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s Get config_pvd_as_hs_flag as %s. \n", 
                            __FUNCTION__, 
                           get_hs_flag->b_is_test_spare_enabled ? "Enable" : "Disable");

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t
topology_update_disable_spare_select_unconsumed(fbe_packet_t *  packet)
{
    fbe_topology_mgmt_update_disable_spare_select_unconsumed_t *update_disable_unconsumed_p = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_length_t buffer_length;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &update_disable_unconsumed_p);
    if(update_disable_unconsumed_p == NULL){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length != sizeof(fbe_topology_mgmt_update_disable_spare_select_unconsumed_t)){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid buffer_length %d", __FUNCTION__, buffer_length);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    topology_b_disable_spare_select_unconsumed = update_disable_unconsumed_p->b_disable_spare_select_unconsumed;

    fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                           FBE_TRACE_LEVEL_INFO, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s Set disable unconsumed spare: %d\n", 
                           __FUNCTION__, update_disable_unconsumed_p->b_disable_spare_select_unconsumed);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t
topology_get_disable_spare_select_unconsumed(fbe_packet_t *  packet)
{
    fbe_topology_mgmt_get_disable_spare_select_unconsumed_t *get_disable_unconsumed_p = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_length_t buffer_length;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &get_disable_unconsumed_p);
    if(get_disable_unconsumed_p == NULL){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length != sizeof(fbe_topology_mgmt_get_disable_spare_select_unconsumed_t)){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid buffer_length %d", __FUNCTION__, buffer_length);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    get_disable_unconsumed_p->b_disable_spare_select_unconsumed = topology_b_disable_spare_select_unconsumed;

    fbe_base_service_trace((fbe_base_service_t*)&topology_service,
                           FBE_TRACE_LEVEL_INFO, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s disable unconsumed as spare: %d\n", 
                            __FUNCTION__, get_disable_unconsumed_p->b_disable_spare_select_unconsumed);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

/*this function can get the b/e/s information of a PVD even if it's in SLF mode w/o an LDO under it*/
static fbe_status_t topology_get_pvd_by_location_from_non_paged(fbe_packet_t *  packet)
{
    fbe_topology_control_get_provision_drive_id_by_location_t * provision_drive_location = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_length_t buffer_length;
    fbe_object_id_t id;
    fbe_class_id_t class_id;
    fbe_provision_drive_get_physical_drive_location_t drive_location;
    fbe_packet_t *  new_packet = NULL;
    fbe_status_t status = FBE_STATUS_OK;
	fbe_packet_attr_t caller_attr;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &provision_drive_location);
    if(provision_drive_location == NULL){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s fbe_payload_control_get_buffer failed", __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length < sizeof(fbe_topology_control_get_drive_by_location_t)){
        fbe_base_service_trace((fbe_base_service_t *) &topology_service, 
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s Invalid buffer_length %d", __FUNCTION__, buffer_length);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Scan topology table for pvd drive objects */
    new_packet = fbe_transport_allocate_packet();
    fbe_transport_initialize_packet(new_packet);
	fbe_transport_get_packet_attr(packet, &caller_attr);
	fbe_transport_set_packet_attr(new_packet, caller_attr);
	fbe_transport_clear_packet_attr(new_packet,FBE_PACKET_FLAG_COMPLETION_SEMAPHORE_INITIALIZED);/*we don't want to carry that*/

    for(id = 0; id < max_supported_objects ; id ++) {
        topology_get_object_class_id(id, &class_id);
        if (class_id == FBE_CLASS_ID_PROVISION_DRIVE)
        {
            fbe_transport_packet_clear_callstack_depth(new_packet);/* reset this since we reuse the packet */
            status = topology_get_provision_drive_location_from_non_paged(new_packet,
                    id,
                    &drive_location);

            if (status == FBE_STATUS_NO_OBJECT) {
                    /*the object was there and was removed, let's go to the next one*/
                    continue;
            }

            if (status != FBE_STATUS_OK)
            {
                /* The PVD might not be connected - keep trying though */
                continue;
            }

            if (drive_location.port_number == provision_drive_location->port_number &&
                    drive_location.enclosure_number == provision_drive_location->enclosure_number &&
                    drive_location.slot_number == provision_drive_location->slot_number)
            {
                provision_drive_location->object_id = id;
                fbe_transport_release_packet(new_packet);
                fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
                fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_OK;
            }
        }
    }

    fbe_transport_release_packet(new_packet);
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    fbe_payload_control_set_status_qualifier(control_operation, FBE_OBJECT_ID_INVALID);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t topology_get_provision_drive_location_from_non_paged(fbe_packet_t * packet,
        fbe_object_id_t object_id,
        fbe_provision_drive_get_physical_drive_location_t *drive_location_p)
{
	fbe_status_t status;
    fbe_payload_ex_t  * payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_payload_control_status_t  control_status;
    fbe_semaphore_t sem;

    payload_p = fbe_transport_get_payload_ex(packet);
    control_operation_p = fbe_payload_ex_allocate_control_operation(payload_p);
    if(control_operation_p == NULL) 
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

	drive_location_p->port_number = FBE_PORT_NUMBER_INVALID;
	drive_location_p->enclosure_number = FBE_ENCLOSURE_NUMBER_INVALID;
	drive_location_p->slot_number = FBE_SLOT_NUMBER_INVALID;

    /* Now issue the get drive info to the proper pdo */
    fbe_semaphore_init(&sem, 0, 1);

    /* Build the payload control operation with GET drive info control packet.
     */
    fbe_payload_control_build_operation(control_operation_p, 
                                        FBE_PROVISION_DRIVE_CONTROL_CODE_GET_DRIVE_LOCATION,
                                        drive_location_p,
                                        sizeof(fbe_provision_drive_get_physical_drive_location_t));

    status = fbe_transport_set_completion_function(packet, 
                                                   topology_get_info_completion,
                                                   &sem);
    if(status != FBE_STATUS_OK) 
    {
        fbe_payload_ex_release_control_operation(payload_p, control_operation_p);
        fbe_semaphore_destroy(&sem);
        return status;
    }
   
    status = fbe_transport_set_address( packet,
                                        FBE_PACKAGE_ID_SEP_0,
                                        FBE_SERVICE_ID_TOPOLOGY,
                                        FBE_CLASS_ID_INVALID,
                                        object_id);
    if(status  != FBE_STATUS_OK) 
    {
        fbe_payload_ex_release_control_operation(payload_p, control_operation_p);
        fbe_semaphore_destroy(&sem);
        return status;
    }

    fbe_payload_ex_increment_control_operation_level(payload_p);

    status = topology_send_mgmt_packet(object_id, packet);
    if(status  != FBE_STATUS_OK) 
    {
        fbe_payload_ex_release_control_operation(payload_p, control_operation_p);
        fbe_semaphore_destroy(&sem); /* SAFEBUG - needs destroy */
        return status;
    }

    /* Wait for the operation to complete.
     */
    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    /* Save the status for returning.
     */
    status = fbe_transport_get_status_code(packet);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_ex_release_control_operation(payload_p, control_operation_p);

    /* Return the status code based on the status of the packet and payload status.
     */
    return ((status == FBE_STATUS_OK && control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) ? FBE_STATUS_OK :FBE_STATUS_GENERIC_FAILURE) ;
}
