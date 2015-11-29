/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

 /*!*************************************************************************
 * @file fbe_environment_limit_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains main control entry  functions for environment
 *  limit service.
 *  This file contains platform independent code.
 *
 * @ingroup environment_limit_service_files
 * 
 * @version
 *   19-August-2010:  Created. Vishnu Sharma
 *
 ***************************************************************************/

 #include "fbe_environment_limit_private.h"
 #include "fbe/fbe_transport.h"
 #include "fbe/fbe_pe_types.h"
 #include "fbe/fbe_board.h"
 #include "fbe/fbe_service_manager_interface.h"
 #include "fbe_base_service.h"
 #include "fbe_transport_memory.h"
 #include "fbe/fbe_registry.h"

/* Declare our service methods */
fbe_status_t fbe_environment_limit_control_entry(fbe_packet_t * packet); 
fbe_service_methods_t fbe_environment_limit_service_methods = {FBE_SERVICE_ID_ENVIRONMENT_LIMIT, fbe_environment_limit_control_entry};

typedef struct fbe_environment_limit_service_s{
    fbe_base_service_t  base_service;
    fbe_spinlock_t      environment_limits_lock;
    fbe_environment_limits_t environment_limits;
}fbe_environment_limit_service_t;

static fbe_environment_limit_service_t environment_limit_service;

static fbe_status_t fbe_environment_limit_get_limits(fbe_packet_t * packet);
static fbe_status_t fbe_environment_limit_set_limits(fbe_packet_t * packet);
static fbe_status_t fbe_initialize_environment_limit_service(void);

/*!****************************************************************************
 * @fn fbe_environment_limit_trace(fbe_trace_level_t trace_level,
 *                         fbe_trace_message_id_t message_id,
 *                         const char * fmt, ...)
 ******************************************************************************
 * @brief
 *      This function is the interface used by the environment limit
 *      service to trace info
 *
 * @param trace_level - Trace level value that message needs to be
 *                      logged at
 * @param message_id - Message Id
 * @param fmt - Insertion Strings
 * 
 * @return
 *
 * @author:
 *      19-August-2010    Vishnu Sharma  Created.
 *
 ******************************************************************************/
void fbe_environment_limit_trace(fbe_trace_level_t trace_level,
                         fbe_trace_message_id_t message_id,
                         const char * fmt, ...)
{
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t service_trace_level;

    va_list args;

    service_trace_level = default_trace_level = fbe_trace_get_default_trace_level();
    if (fbe_base_service_is_initialized((fbe_base_service_t *) &environment_limit_service)) {
        service_trace_level = fbe_base_service_get_trace_level((fbe_base_service_t *) &environment_limit_service);
        if (default_trace_level > service_trace_level) {
            service_trace_level = default_trace_level;
        }
    }
    if (trace_level > service_trace_level) {
        return;
    }

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_SERVICE,
                     FBE_SERVICE_ID_ENVIRONMENT_LIMIT,
                     trace_level,
                     message_id,
                     fmt, 
                     args);
    va_end(args);
}
/***************************************************************************
    end fbe_environment_limit_trace()     
****************************************************************************/

/*!****************************************************************************
 * @fn fbe_environment_limit_init(fbe_packet_t * packet)
 ******************************************************************************
 *
 * @brief
 *      This function initializes the environment limit service and calls
 *      any platform specific initialization routines
 *
 * @param packet - Pointer to the packet received by the environment limit
 *                 service
 * 
 * @return fbe_status_t
 *
 * @author
 *      19-August-2010    Vishnu Sharma  Created.
 *
 ******************************************************************************/
static fbe_status_t 
fbe_environment_limit_init(fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_environment_limit_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s: entry\n", __FUNCTION__);

    fbe_base_service_set_service_id((fbe_base_service_t *) &environment_limit_service, FBE_SERVICE_ID_ENVIRONMENT_LIMIT);

    fbe_base_service_init((fbe_base_service_t *) &environment_limit_service);
    fbe_spinlock_init(&environment_limit_service.environment_limits_lock);

    status = fbe_initialize_environment_limit_service();
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_NOT_INITIALIZED;
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}
/****************************************************************************
    end fbe_environment_limit_init()     
*****************************************************************************/

/*!****************************************************************************
 * @fn fbe_initialize_environment_limit_service()
 ******************************************************************************
 *
 * @brief
 *      This function initializes the environment limit service and calls
 *      any platform specific initialization routines
 *
 * @return fbe_status_t
 *
 * @author
 *      26-August-2010    Vishnu Sharma  Created.
 *
 ******************************************************************************/
static fbe_status_t
fbe_initialize_environment_limit_service(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    /* Call the implementation specific init */
    status = fbe_environment_limit_init_limits(&environment_limit_service.environment_limits);
    return status;
}
/****************************************************************************
    end fbe_initialize_environment_limit_service()     
*****************************************************************************/

/*!****************************************************************************
 * @fn fbe_environment_limit_destroy(fbe_packet_t * packet)
 ******************************************************************************
 *
 * @brief
 *      This function performs the necessary clean up in
 *      preparation to stop the environment limit service
 *
 * @param  packet - Pointer to the packet received by the environment_limit
 *                  service
 * 
 * @return  fbe_status_t
 *
 * @author
 *      19-August-2010    Vishnu Sharma  Created.
 *
 ******************************************************************************/
static fbe_status_t 
fbe_environment_limit_destroy(fbe_packet_t * packet)
{
    fbe_environment_limit_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s: entry\n", __FUNCTION__);

    /* Call the implementation specific cleanup */
    fbe_environment_limit_clear_limits();

    fbe_spinlock_destroy(&environment_limit_service.environment_limits_lock);
    fbe_base_service_destroy((fbe_base_service_t *) &environment_limit_service);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}
/***********************************************************************
    end fbe_environment_limit_destroy() 
************************************************************************/

/*!**********************************************************************
 * @fn fbe_environment_limit_control_entry(fbe_packet_t * packet)
 ************************************************************************
 *
 * @brief
 *      This function dispatches the control code to the
 *      appropriate handler functions
 *
 * @param  packet - Pointer to the packet received by the environment limit
 *                  service
 * 
 * @return  fbe_status_t
 *
 * @author
 *      19-August-2010    Vishnu Sharma  Created.
 *
 *************************************************************************/
fbe_status_t 
fbe_environment_limit_control_entry(fbe_packet_t * packet)
{    
    fbe_status_t status;
    fbe_payload_control_operation_opcode_t control_code; 

    control_code = fbe_transport_get_control_code(packet);
    if(control_code == FBE_BASE_SERVICE_CONTROL_CODE_INIT) {
        status = fbe_environment_limit_init(packet);
        return status;
    }

    /* No operation is allowed until the environment limit service is
     *  initialized. Return the status immediately
     */
    if(!fbe_base_service_is_initialized((fbe_base_service_t *) &environment_limit_service)){
        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_NOT_INITIALIZED;
    }

    switch(control_code)
    {
        case FBE_BASE_SERVICE_CONTROL_CODE_DESTROY:
            status = fbe_environment_limit_destroy( packet);
            break;
        case FBE_ENVIRONMENT_LIMIT_CONTROL_CODE_GET_LIMITS:
            status = fbe_environment_limit_get_limits(packet);
            break;
        case FBE_ENVIRONMENT_LIMIT_CONTROL_CODE_SET_LIMITS:
            status = fbe_environment_limit_set_limits(packet);
            break;
        default:
            status = fbe_base_service_control_entry((fbe_base_service_t*)&environment_limit_service, packet);
            break;
    }
    return status;
}
/****************************************************************************
    end fbe_environment_limit_control_entry()     
*****************************************************************************/

/*!**********************************************************************
 * @fn fbe_environment_limit_get_limits(fbe_packet_t * packet)
 ************************************************************************
 *
 * @brief
 *      This function get data from 
 *      environment limit servise
 *
 * @param  packet - Pointer to the packet received by the environment limit
 *                  service
 * 
 * @return  fbe_status_t
 *
 * @author
 *      26-August-2010    Vishnu Sharma  Created.
 *
 *************************************************************************/
static fbe_status_t
fbe_environment_limit_get_limits(fbe_packet_t * packet)
{
    fbe_environment_limits_t * environment_limits = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_length_t buffer_length = 0;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &environment_limits);
    if(environment_limits == NULL){
        fbe_base_service_trace((fbe_base_service_t *) &environment_limit_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length != sizeof(fbe_environment_limits_t)){
        fbe_base_service_trace((fbe_base_service_t *) &environment_limit_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid buffer_length %d", __FUNCTION__, buffer_length);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Need to get the lock first then copy the data to the buffer
     * passed in
     */
    fbe_spinlock_lock(&environment_limit_service.environment_limits_lock);
    fbe_copy_memory(environment_limits, &environment_limit_service.environment_limits, buffer_length);
    fbe_spinlock_unlock(&environment_limit_service.environment_limits_lock);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/****************************************************************************
    end fbe_environment_limit_get_limits()     
*****************************************************************************/

/*!**********************************************************************
 * @fn fbe_environment_limit_set_limits(fbe_packet_t * packet)
 ************************************************************************
 *
 * @brief
 *      This function set data from 
 *      environment limit servise
 *
 * @param  packet - Pointer to the packet received by the environment limit
 *                  service
 * 
 * @return  fbe_status_t
 *
 * @author
 *      26-August-2010    Vishnu Sharma  Created.
 *
 *************************************************************************/
static fbe_status_t 
fbe_environment_limit_set_limits(fbe_packet_t * packet)
{
    fbe_environment_limits_t * environment_limits = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_length_t buffer_length = 0;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &environment_limits);
    if(environment_limits == NULL){
        fbe_base_service_trace((fbe_base_service_t *) &environment_limit_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length != sizeof(fbe_environment_limits_t)){
        fbe_base_service_trace((fbe_base_service_t *) &environment_limit_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid buffer_length %d", __FUNCTION__, buffer_length);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Need to get the lock first then set the data to the service
     * passed in
     */
    fbe_spinlock_lock(&environment_limit_service.environment_limits_lock);
    fbe_copy_memory(&environment_limit_service.environment_limits,environment_limits,buffer_length);
    fbe_spinlock_unlock(&environment_limit_service.environment_limits_lock);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/****************************************************************************
    end fbe_environment_limit_set_limits()     
*****************************************************************************/


fbe_u32_t fbe_environment_limit_get_platform_fru_count()
{
    return environment_limit_service.environment_limits.platform_fru_count;
}
fbe_u32_t fbe_environment_limit_get_platform_max_be_count()
{
    return environment_limit_service.environment_limits.platform_max_be_count;
}
fbe_u32_t fbe_environment_limit_get_platform_max_encl_count_per_bus()
{
    return environment_limit_service.environment_limits.platform_max_encl_count_per_bus;
}
fbe_status_t fbe_environment_limit_get_platform_max_memory_mb(fbe_u32_t * platform_max_memory_mb)
{
    fbe_status_t                  		status = FBE_STATUS_GENERIC_FAILURE;
    fbe_board_mgmt_platform_info_t 		platform_info;
    fbe_packet_t *                  	packet_p = NULL;
    fbe_payload_ex_t *             		payload = NULL;
    fbe_payload_control_operation_t *	control_operation = NULL;
    fbe_payload_control_status_t    	payload_control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_object_id_t board_object_id;
    
    *platform_max_memory_mb = 0;

    fbe_environment_limit_get_board_object_id(&board_object_id);

    /* The memory service is not available yet */
    packet_p = fbe_allocate_nonpaged_pool_with_tag ( sizeof (fbe_packet_t), 'pfbe');

    if (packet_p == NULL) {
        fbe_environment_limit_trace (FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s:can't allocate packet\n",__FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;

    }

    fbe_transport_initialize_packet(packet_p);
    payload = fbe_transport_get_payload_ex (packet_p);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    fbe_payload_control_build_operation (control_operation,
                                         FBE_BASE_BOARD_CONTROL_CODE_GET_PLATFORM_INFO,
                                         &platform_info,
                                         sizeof(fbe_board_mgmt_platform_info_t));

    /* Set packet address.*/
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_PHYSICAL,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              board_object_id); 

    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    /* Control packets should be sent via service manager. */
    status = fbe_service_manager_send_control_packet(packet_p);
    if (status != FBE_STATUS_OK){
            fbe_environment_limit_trace (FBE_TRACE_LEVEL_ERROR,
                                         FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s:failed to send control packet\n",__FUNCTION__);
        
    }

    fbe_transport_wait_completion(packet_p);

    fbe_payload_control_get_status(control_operation, &payload_control_status);
    if(payload_control_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_environment_limit_trace (FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s:bad return status\n",__FUNCTION__);
    }

    status  = fbe_transport_get_status_code(packet_p);
    if(status != FBE_STATUS_OK){
        fbe_environment_limit_trace (FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s:bad return status\n",__FUNCTION__);
    }

    fbe_payload_ex_release_control_operation(payload, control_operation);
    fbe_transport_destroy_packet(packet_p);
    fbe_release_nonpaged_pool(packet_p);

    if(platform_info.is_simulation){
        *platform_max_memory_mb = 120 + 11; // 11MB is reserved for database tables.
        fbe_environment_limit_trace (FBE_TRACE_LEVEL_INFO,
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "Simulation platform_max_memory_mb: %d\n", *platform_max_memory_mb);

        return FBE_STATUS_OK;
    }

    /* We need to allocate 1MB per core for 1 block DCA request */
    //*platform_max_memory_mb += fbe_get_cpu_count();

    fbe_environment_limit_get_max_memory_mb(platform_info.hw_platform_info.platformType,
                                            platform_max_memory_mb);

    fbe_environment_limit_trace (FBE_TRACE_LEVEL_INFO,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "platform_max_memory_mb: %d\n", *platform_max_memory_mb);

    return FBE_STATUS_OK;
}

void fbe_environment_limit_get_max_memory_mb(SPID_HW_TYPE hw_type,
                                             fbe_u32_t * platform_max_memory_mb)
{
     EMCUTIL_STATUS status;
     fbe_u32_t MemValueFromRegistry;
     fbe_u32_t DefaultRegMemValue;
     

    // 05-01: Until the memory rework for MCR takes place, for now to get pass the out-of-memory issue,
    // we added 80 mb to each of the platform
    // ******* PLEASE SEE THE MEMORY RESERVED AFTER THE SWITCH STATEMENT FOR DCA 
    // REQUESTS BASED ON THE CORE COUNT  *************************

    // SEP database tables will allocate its memory from CMM control pool.
    // The increasing memoy budge for different platform are as below: 
    // Platform The total control memory (MB) 
    // SPID_PROMETHEUS_M1_HW_TYPE 20 
    // SPID_DEFIANT_M1_HW_TYPE 17 
    // SPID_DEFIANT_M2_HW_TYPE 10 
    // SPID_DEFIANT_M3_HW_TYPE 9 
    // SPID_DEFIANT_M4_HW_TYPE 5 
    // SPID_DEFIANT_M5_HW_TYPE 5
    // When database table size enlarges, please update this memory budget.
    
    switch(hw_type){
    // For now assume Flare Hemi memory needs same as Nautilus for Hellcat.
    case SPID_PROMETHEUS_S1_HW_TYPE:
    case SPID_DEFIANT_S1_HW_TYPE:
        *platform_max_memory_mb = 8 + 5 + 120 + 2 + 80;
        //KERNEL_MEM_IN_MEGABYTE = 120;
        //EMALLOC_MEM_SIZE=(8 * MEGABYTE);
        //CTL_RAM_DATA_SIZE=((5 + KERNEL_MEM_IN_MEGABYTE + 2) * MEGABYTE);
        break;

    case SPID_ENTERPRISE_HW_TYPE:
        *platform_max_memory_mb = 443 + 256;
    break;

// For now assume Flare Hemi memory needs same as Dreadnought for Prometheus M1.
    case SPID_PROMETHEUS_M1_HW_TYPE:
		*platform_max_memory_mb = 443 + 256 + 20 + 6;
        //*platform_max_memory_mb = 15 + 40 + 5 + 246 + 2 + 80;
        //KERNEL_MEM_IN_MEGABYTE = 246;
        //EMALLOC_MEM_SIZE=(15 * MEGABYTE);
        //CTL_RAM_DATA_SIZE=((40 + (5 + KERNEL_MEM_IN_MEGABYTE + 2)) * MEGABYTE);
        // database_table_size = 20 MB
        // Every RG consumes one chunk for stripe lock hash. 5/15/2014
        break;

    case SPID_DEFIANT_M1_HW_TYPE:
		*platform_max_memory_mb = 309 + 256 + 17 + 2 + 1 + 4 + 16;
        //*platform_max_memory_mb = 15 + 40 + 5 + 153 + 2 + 80;
        //KERNEL_MEM_IN_MEGABYTE = 153;
        //EMALLOC_MEM_SIZE=(15 * MEGABYTE);
        //CTL_RAM_DATA_SIZE=((40 + (5 + KERNEL_MEM_IN_MEGABYTE + 2)) * MEGABYTE);
        // database_table_size = 17 MB
        // VNX7600 supports 1000 RGs, so 2MB is increased for objects 
        // and 1MB is increased for database tables
        // Every RG consumes one chunk for stripe lock hash. 5/15/2014
        // F3011 FLUs 4096->8192, which needs 16MB more for objects
        break;

    case SPID_DEFIANT_M2_HW_TYPE:
		*platform_max_memory_mb = 276 + 128 + 10 + 3;
        //*platform_max_memory_mb = 8 + 5 + 120 + 2 + 80;
        //KERNEL_MEM_IN_MEGABYTE = 120;
        //EMALLOC_MEM_SIZE=(8 * MEGABYTE);
        //CTL_RAM_DATA_SIZE=((5 + KERNEL_MEM_IN_MEGABYTE + 2) * MEGABYTE);
        // database_table_size = 10 MB
        // Every RG consumes one chunk for stripe lock hash. 5/15/2014
        break;

    case SPID_DEFIANT_M3_HW_TYPE:
		*platform_max_memory_mb = 233 + 128 + 9 + 2 + 8;
        //*platform_max_memory_mb = 8 + 5 + 120 + 2 + 80;
        //KERNEL_MEM_IN_MEGABYTE = 120;
        //EMALLOC_MEM_SIZE=(8 * MEGABYTE);
        //CTL_RAM_DATA_SIZE=((5 + KERNEL_MEM_IN_MEGABYTE + 2) * MEGABYTE);
        // database_table_size = 9 MB
        // Every RG consumes one chunk for stripe lock hash. 5/15/2014
        // F3011 FLUs 2048 -> 4096, which needs 8MB more for objects
        break;

    case SPID_DEFIANT_M4_HW_TYPE:
    case SPID_DEFIANT_S4_HW_TYPE:
		*platform_max_memory_mb = 224 + 64 + 5 + 1;
        //*platform_max_memory_mb = 8 + 5 + 88 + 2 + 80;
        //KERNEL_MEM_IN_MEGABYTE = 88;
        //EMALLOC_MEM_SIZE=(8 * MEGABYTE);
        //CTL_RAM_DATA_SIZE=((5 + KERNEL_MEM_IN_MEGABYTE + 2) * MEGABYTE);
        // database_table_size = 5 MB
        // Every RG consumes one chunk for stripe lock hash. 5/15/2014
        break;

    case SPID_DEFIANT_M5_HW_TYPE:
        *platform_max_memory_mb = 8 + 5 + 88 + 2 + 80 + 64 + 5 + 1;
        //KERNEL_MEM_IN_MEGABYTE = 88;
        //EMALLOC_MEM_SIZE=(8 * MEGABYTE);
        //CTL_RAM_DATA_SIZE=((5 + KERNEL_MEM_IN_MEGABYTE + 2) * MEGABYTE);
        //  database_table_size = 5 MB
        // Every RG consumes one chunk for stripe lock hash. 5/15/2014
        break;

    case SPID_DEFIANT_K2_HW_TYPE:
    //@KHP_TODO: As per Peter Puhov, this should be same as DEFIANT_M1.
    //           Ideally it should be the "MCR (FBE memory)" field from the limits-spreadsheet.
        *platform_max_memory_mb = 309 + 256;
        break;

    case SPID_DEFIANT_K3_HW_TYPE:
    //@KHP_TODO: As per Peter Puhov, this should be same as DEFIANT_M3.
    //           Ideally it should be the "MCR (FBE memory)" field from the limits-spreadsheet.
        *platform_max_memory_mb = 233 + 128;
        break;

    case SPID_NOVA1_HW_TYPE:
    case SPID_NOVA3_HW_TYPE:
    case SPID_NOVA3_XM_HW_TYPE:
    case SPID_NOVA_S1_HW_TYPE:
        *platform_max_memory_mb = 239;
        break;

    case SPID_BEARCAT_HW_TYPE:
        *platform_max_memory_mb = 239; /* FIXME */
        break;

    case SPID_OBERON_S1_HW_TYPE:
        *platform_max_memory_mb = 239;
        break;

    case SPID_OBERON_1_HW_TYPE:
    case SPID_OBERON_2_HW_TYPE:
    case SPID_OBERON_3_HW_TYPE:
    case SPID_OBERON_4_HW_TYPE:
    case SPID_HYPERION_1_HW_TYPE:
    case SPID_HYPERION_2_HW_TYPE:
    case SPID_HYPERION_3_HW_TYPE:
 
        //Note: At this time we have not called core config init so read this value from the registry directly
        DefaultRegMemValue = 239;
        status = fbe_registry_read(NULL,
                                   K10_CORE_CONFIG_KEY,
                                   MAXMEMORYKEYNAME,
                                   &MemValueFromRegistry,
                                   sizeof(fbe_u32_t),
                                   FBE_REGISTRY_VALUE_DWORD,
                                   &DefaultRegMemValue,
                                   sizeof(fbe_u32_t),
                                   FALSE);

        *platform_max_memory_mb = MemValueFromRegistry;
        break;

    case SPID_TRITON_1_HW_TYPE:
        *platform_max_memory_mb = 443 + 256;
        break;

    //Revisit it; Same as KH sim
    case SPID_MERIDIAN_ESX_HW_TYPE:
    case SPID_TUNGSTEN_HW_TYPE:
        *platform_max_memory_mb = 239;
        break;

    default:
         /* This is the default value if platform specific SPID type is not listed above */
        *platform_max_memory_mb = 160 + 80;
        //KERNEL_MEM_IN_MEGABYTE = 35;
        //EMALLOC_MEM_SIZE=(3 * MEGABYTE);
        //CTL_RAM_DATA_SIZE=((3 + KERNEL_MEM_IN_MEGABYTE + 2) * MEGABYTE);
        break;
    }
}
fbe_status_t 
fbe_environment_limit_get_board_object_id(fbe_object_id_t *board_id)
{
    fbe_status_t                  		status = FBE_STATUS_GENERIC_FAILURE;
    fbe_topology_control_get_board_t 	board_info;
    fbe_packet_t *                  	packet_p = NULL;
    fbe_payload_ex_t *             		payload = NULL;
    fbe_payload_control_operation_t *	control_operation = NULL;
    fbe_payload_control_status_t    	payload_control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    
    packet_p = fbe_allocate_nonpaged_pool_with_tag ( sizeof (fbe_packet_t), 'pfbe');
    if (packet_p == NULL) {
        fbe_environment_limit_trace (FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s:can't allocate packet\n",__FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;

    }

    fbe_transport_initialize_packet(packet_p);
    payload = fbe_transport_get_payload_ex (packet_p);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    fbe_payload_control_build_operation (control_operation,
                                         FBE_TOPOLOGY_CONTROL_CODE_GET_BOARD,
                                         &board_info,
                                         sizeof(fbe_topology_control_get_board_t));

    /* Set packet address.*/
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_PHYSICAL,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID); 

    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    /* Control packets should be sent via service manager. */
    status = fbe_service_manager_send_control_packet(packet_p);
    if (status != FBE_STATUS_OK){
            fbe_environment_limit_trace (FBE_TRACE_LEVEL_ERROR,
                                         FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s:failed to send control packet\n",__FUNCTION__);
        
    }

    fbe_transport_wait_completion(packet_p);

    fbe_payload_control_get_status(control_operation, &payload_control_status);
    if(payload_control_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_environment_limit_trace (FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s:bad return status\n",__FUNCTION__);
    }

    status  = fbe_transport_get_status_code(packet_p);
    if(status != FBE_STATUS_OK){
        fbe_environment_limit_trace (FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s:bad return status\n",__FUNCTION__);
    }

    fbe_payload_ex_release_control_operation(payload, control_operation);
    fbe_transport_destroy_packet(packet_p);
    fbe_release_nonpaged_pool(packet_p);

    *board_id = board_info.board_object_id;

    return status;

}

void fbe_environment_limit_get_platform_hardware_limits(fbe_environment_limits_platform_hardware_limits_t * hardware_limits)
{
    fbe_environment_limit_trace (FBE_TRACE_LEVEL_INFO,
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s:max SLICs %d Mezzanines %d\n",__FUNCTION__,
                                 environment_limit_service.environment_limits.platform_hardware_limits.max_slics,
                                 environment_limit_service.environment_limits.platform_hardware_limits.max_mezzanines);
    *hardware_limits = environment_limit_service.environment_limits.platform_hardware_limits;
    return;
}

void fbe_environment_limit_get_platform_port_limits(fbe_environment_limits_platform_port_limits_t * port_limits)
{
    *port_limits = environment_limit_service.environment_limits.platform_port_limits;
    return;
}

//end fbe_environment_limit_main.c
