#include "csx_ext.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_memory.h"
#include "fbe_transport_memory.h"
#include "fbe_topology.h"
#include "fbe_notification.h"
#include "fbe_scheduler.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_sep.h"
#include "sep_class_table.h"
#include "sep_service_table.h"
#include "fbe/fbe_event_log_api.h"
#include "fbe_cmi.h"
#include "fbe_metadata.h"
#include "fbe_raid_library.h"
#include "fbe/fbe_sep_shim.h"
#include "fbe/fbe_environment_limit.h"
#include "fbe_cmm.h"
#include "fbe_database.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_board.h"
#include "fbe/fbe_perfstats_interface.h"
#include "fbe_scheduler_encryption_plugin.h"
#include "fbe/fbe_extent_pool.h"
 

static CMM_POOL_ID				cmm_control_pool_id;
static CMM_CLIENT_ID			cmm_client_id;

static CMM_POOL_ID		cmm_data_pool_id;
static CMM_CLIENT_ID	cmm_data_client_id;
static PCMM_SGL			cmm_data_sgl;

#if defined(SIMMODE_ENV)
#define MINIMUM_SEP_DPS_DATA_MB 120
#else
#define MINIMUM_SEP_DPS_DATA_MB 120
#endif

static fbe_status_t sep_destroy_all_objects(void);
static fbe_status_t sep_destroy_object(fbe_object_id_t object_id);
static fbe_status_t sep_set_object_destroy_condition(fbe_object_id_t object_id);
static fbe_status_t sep_get_total_objects(fbe_u32_t *total);
static fbe_status_t sep_init_libraries(fbe_sep_package_load_params_t *params_p);
static fbe_status_t sep_destroy_libraries(void);

static fbe_status_t fbe_sep_memory_init(void);
static fbe_status_t fbe_sep_memory_destroy(void);
static void * fbe_sep_memory_allocate(fbe_u32_t allocation_size_in_bytes);
static void fbe_sep_memory_release(void * ptr);

static fbe_status_t sep_init_wait_for_db_service(void);
static fbe_status_t sep_init_dps_params(fbe_u64_t platform_max_memory_mb, fbe_memory_dps_init_parameters_t * params);
static fbe_status_t sep_init_get_default_dps_data_chunks(fbe_u64_t platform_max_memory_mb, fbe_memory_dps_init_parameters_t *init_dps_data_params);
static fbe_status_t sep_init_dps_data_params(fbe_u64_t platform_max_memory_mb, fbe_memory_dps_init_parameters_t *params, fbe_bool_t b_use_leftover);

static fbe_status_t sep_init_set_pvd_class_debug_flags(fbe_u32_t debug_flags);
static fbe_status_t sep_init_set_lifecycle_class_debug_flags(fbe_u32_t debug_flags);
extern fbe_status_t fbe_base_config_enable_system_background_zeroing(void);

static fbe_memory_dps_init_parameters_t sep_dps_init_parameters;
static fbe_memory_dps_init_parameters_t sep_dps_init_data_parameters;

extern fbe_status_t fbe_database_cmi_send_mailbomb(void);
static fbe_status_t sep_init_set_virtual_drive_class_debug_flags(fbe_u32_t debug_flags);
static fbe_status_t sep_init_set_raid_group_class_debug_flags(fbe_u32_t debug_flags);

static fbe_status_t fbe_sep_init_psl(fbe_sep_package_load_params_t *params_p);

static fbe_status_t fbe_sep_init_data_pool(fbe_memory_dps_init_parameters_t *init_data_parameters, fbe_bool_t b_use_leftover);
static fbe_status_t fbe_sep_destroy_data_pool(void);

static fbe_status_t sep_init_set_dps_data_chunks_from_params(fbe_memory_dps_init_parameters_t *test_dps_data_params, fbe_memory_dps_init_parameters_t *init_dps_data_params);

static fbe_status_t fbe_sep_init_get_platform_info(fbe_board_mgmt_platform_info_t *platform_info);
static fbe_status_t sep_init_set_metadata_nonpaged_blocks_per_object(fbe_u32_t blocks_per_object);

static void
sep_log_error(const char * fmt, ...) __attribute__((format(__printf_func__,1,2)));

static void
sep_log_error(const char * fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_PACKAGE,
                     FBE_PACKAGE_ID_SEP_0,
                     FBE_TRACE_LEVEL_ERROR,
                     0, /* message_id */
                     fmt, 
                     args);
    va_end(args);
}

static void
sep_log_critical_error(const char * fmt, ...) __attribute__((format(__printf_func__,1,2)));

static void
sep_log_critical_error(const char * fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_PACKAGE,
                     FBE_PACKAGE_ID_SEP_0,
                     FBE_TRACE_LEVEL_CRITICAL_ERROR,
                     0, /* message_id */
                     fmt, 
                     args);
    va_end(args);
}

static void
sep_log_info(const char * fmt, ...) __attribute__((format(__printf_func__,1,2)));

static void
sep_log_info(const char * fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_PACKAGE,
                     FBE_PACKAGE_ID_SEP_0,
                     FBE_TRACE_LEVEL_INFO,
                     0, /* message_id */
                     fmt, 
                     args);
    va_end(args);
}

//void fbe_ddk_csx_spinlock_enable_logging(void);
fbe_bool_t sep_is_service_mode(void)
{
    fbe_database_state_t db_state;
    fbe_database_get_state(&db_state);
    if (db_state == FBE_DATABASE_STATE_SERVICE_MODE)
        return FBE_TRUE;
    else
        return FBE_FALSE;
}

// 3/1/2013 Peter will change it in next check in !!!
extern void fbe_bvd_interface_usurper_enable_pp_group_priority(void);
extern void fbe_bvd_interface_usurper_disable_pp_group_priority(void);
#if !(defined(UMODE_ENV) || defined(SIMMODE_ENV))
extern void ktrace_set_cmm_proc_address(PVOID pcmmGetRBADataPoolID, 
                                 PVOID pcmmRegisterClient,
                                 PVOID pcmmGetMemorySGL,
                                 PVOID pcmmFreeMemorySGL,
                                 PVOID pcmmMapMemorySVA,
                                 PVOID pcmmUnMapMemorySVA);
                                 
#endif


fbe_status_t 
sep_init(fbe_sep_package_load_params_t *params_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t wait_count = 0;
    fbe_database_state_t db_state;
    
    sep_log_info("%s starting...\n", __FUNCTION__);
    
    //fbe_ddk_csx_spinlock_enable_logging();
    
    fbe_transport_init();

    status = sep_init_libraries(params_p);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: sep_init_libraries() failed, status: %X\n",
                      __FUNCTION__, status);
        return status;
    }

    status = sep_init_services(params_p);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: sep_init_services failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_sep_shim_init();
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: fbe_sep_shim_init failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

	// 3/1/2013 Peter will change it in next check in !!!
	fbe_bvd_interface_usurper_enable_pp_group_priority();
    /*
     * Wait for database status OK, or enter service mode
     * wait up to 10 minutes
     */
    fbe_database_get_state(&db_state);

    #if DBG  /*Debug only*/
    while (wait_count <= 600){
    #else
    while (wait_count <= 3600){
    #endif /*Debug only*/
        if (db_state == FBE_DATABASE_STATE_READY ||
            db_state == FBE_DATABASE_STATE_SERVICE_MODE || 
            db_state == FBE_DATABASE_STATE_CORRUPT) {

            if (!fbe_cmi_service_disabled())
            {
                /* Enable bg ops in PVD now as database loading done.. 
                 */
                status = fbe_database_enable_bg_operations(FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_SNIFF_VERIFY |
                                                           FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_ZEROING );
                if (status != FBE_STATUS_OK) {
                    sep_log_error("%s: failed to enable bg operations, status: %X\n",
                            __FUNCTION__, status);                
                    return status;
                }
            }
            sep_log_info("%s background operation in PVD enabled.\n", __FUNCTION__);
            return FBE_STATUS_OK;
        }

		if (db_state == FBE_DATABASE_STATE_BOOTFLASH_SERVICE_MODE)
		{
			sep_log_error("%s: db_state is bootflash service mode: %d\n",
					__FUNCTION__, db_state);                
#if defined(SIMMODE_ENV)
			return FBE_STATUS_OK;
#else
			return FBE_STATUS_GENERIC_FAILURE;
#endif
		}

        fbe_thread_delay(1000);
        wait_count++;

        //In normal case, db will be ready in 10 minutes. 
        //If free build and db wait for more than 10 minutes,print below error.
        if((wait_count > 600) && (!(wait_count % 60)))
        {
            sep_log_info("%s: Failed to wait for DB service in %d minutes, state=0x%X, wait_count=0x%x\n",
                          __FUNCTION__, wait_count/60, db_state, wait_count);
        }

        fbe_database_get_state(&db_state);
        if ((db_state == FBE_DATABASE_STATE_WAITING_FOR_CONFIG) && (wait_count >= 600))
        {
            sep_log_info("%s: FBE_DATABASE_STATE_WAITING_FOR_CONFIG, passive clear wait count\n",
                          __FUNCTION__);
            wait_count = 0;
        }
    }

    /*
     * Wait timeout
     */
    sep_log_critical_error("%s: Failed to wait for DB service, state=0x%X, wait_count=0x%x\n",
                           __FUNCTION__, db_state, wait_count);
   
    return FBE_STATUS_GENERIC_FAILURE;
}

fbe_status_t 
sep_destroy(void)
{
    fbe_status_t status;
//    EMCPAL_KIRQL starting_irql = EmcpalGetCurrentLevel();
    
    sep_log_info("%s: starting...\n", __FUNCTION__);

    // 11/1/2014 follow Peter's change to prevent physicalpackage panic after sep is unloaded !!!
    fbe_bvd_interface_usurper_disable_pp_group_priority();

    status = fbe_sep_shim_destroy();
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: fbe_sep_shim_destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }
    
    //if(EmcpalGetCurrentLevel() != starting_irql) { DbgBreakPoint(); }

    /* Destroy all objects */
    /* Destroying the job service. */
    status =  fbe_base_service_send_destroy_command(FBE_SERVICE_ID_JOB_SERVICE);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: Job service destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }
    #if 0 /*CMS DISBALED*/
    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_CMS);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: persistent memory service destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    } 
    #endif
    //if(EmcpalGetCurrentLevel() != starting_irql) { DbgBreakPoint(); }

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_DATABASE);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: fbe_database_service destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_PERFSTATS);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: fbe_database_service destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    //if(EmcpalGetCurrentLevel() != starting_irql) { DbgBreakPoint(); }

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_ENVIRONMENT_LIMIT);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: fbe_env_limit_service destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    //if(EmcpalGetCurrentLevel() != starting_irql) { DbgBreakPoint(); }
    
    /* Destroying the NEW persistence service. */
    status =  fbe_base_service_send_destroy_command(FBE_SERVICE_ID_PERSIST);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: Persist service destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    //if(EmcpalGetCurrentLevel() != starting_irql) { DbgBreakPoint(); }

    sep_destroy_all_objects();
    status =  fbe_base_service_send_destroy_command(FBE_SERVICE_ID_TRACE);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: fbe_trace_destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    //if(EmcpalGetCurrentLevel() != starting_irql) { DbgBreakPoint(); }

    status =  fbe_base_service_send_destroy_command(FBE_SERVICE_ID_TRAFFIC_TRACE);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: fbe_traffic trace_destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    //if(EmcpalGetCurrentLevel() != starting_irql) { DbgBreakPoint(); }

    status =  fbe_base_service_send_destroy_command(FBE_SERVICE_ID_SECTOR_TRACE);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: sector trace destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    //if(EmcpalGetCurrentLevel() != starting_irql) { DbgBreakPoint(); }

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_SCHEDULER);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: fbe_scheduler_destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    //if(EmcpalGetCurrentLevel() != starting_irql) { DbgBreakPoint(); }
    
    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_METADATA);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: fbe metadata service destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    //if(EmcpalGetCurrentLevel() != starting_irql) { DbgBreakPoint(); }

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_TOPOLOGY);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: sep_destroy_service failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    //if(EmcpalGetCurrentLevel() != starting_irql) { DbgBreakPoint(); }

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_NOTIFICATION);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: fbe_notification_destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    //if(EmcpalGetCurrentLevel() != starting_irql) { DbgBreakPoint(); }

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_CMI);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: CMI service destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    //if(EmcpalGetCurrentLevel() != starting_irql) { DbgBreakPoint(); }
    
    /* SAFEBUG - need to destroy event log so we can destroy event log lock */	
    status =  fbe_base_service_send_destroy_command(FBE_SERVICE_ID_EVENT_LOG);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: fbe_memory_destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    status =  fbe_base_service_send_destroy_command(FBE_SERVICE_ID_MEMORY);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: fbe_memory_destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    //if(EmcpalGetCurrentLevel() != starting_irql) { DbgBreakPoint(); }

	fbe_sep_destroy_data_pool();


    fbe_sep_memory_destroy();

    //if(EmcpalGetCurrentLevel() != starting_irql) { DbgBreakPoint(); }
    
    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_SERVICE_MANAGER);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: sep_destroy_service failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    //if(EmcpalGetCurrentLevel() != starting_irql) { DbgBreakPoint(); }

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_EVENT);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: fbe_event_destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    //if(EmcpalGetCurrentLevel() != starting_irql) { DbgBreakPoint(); }

    status = sep_destroy_libraries();
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: sep_destroy_libraries() failed, status: %X\n",
                      __FUNCTION__, status);
        return status;
    }

    //if(EmcpalGetCurrentLevel() != starting_irql) { DbgBreakPoint(); }

    fbe_transport_destroy();

    //if(EmcpalGetCurrentLevel() != starting_irql) { DbgBreakPoint(); }

    return FBE_STATUS_OK;
}

static fbe_status_t 
sep_sem_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;

    fbe_semaphore_release(sem, 0, 1, FALSE);

    return FBE_STATUS_OK;
}

static fbe_status_t 
sep_enumerate_objects(fbe_object_id_t *obj_list, fbe_u32_t total, fbe_u32_t *actual)
{
    fbe_packet_t * packet = NULL;
    fbe_semaphore_t sem;
    fbe_sg_element_t *	sg_list = NULL;
    fbe_sg_element_t *	temp_sg_list = NULL;
    fbe_payload_ex_t	*	payload = NULL;
    fbe_topology_mgmt_enumerate_objects_t 	p_topology_mgmt_enumerate_objects;/*small enough to be on the stack*/

    sg_list = (fbe_sg_element_t *)fbe_allocate_contiguous_memory(sizeof(fbe_sg_element_t) * 2);/*one for the entry and one for the NULL termination*/
    if (sg_list == NULL) {
        sep_log_error("%s: fbe_allocate_contiguous_memory failed to allocate sg list\n", __FUNCTION__);
        *actual = 0;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    temp_sg_list = sg_list;
    temp_sg_list->address = (fbe_u8_t *)obj_list;
    temp_sg_list->count = total * sizeof(fbe_object_id_t);
    temp_sg_list ++;
    temp_sg_list->address = NULL;
    temp_sg_list->count = 0;

    p_topology_mgmt_enumerate_objects.number_of_objects = total;
    
    fbe_semaphore_init(&sem, 0, 1);

    /* Allocate packet */
    packet = fbe_transport_allocate_packet();
    if(packet == NULL) {
        sep_log_error("%s: fbe_transport_allocate_packet failed\n", __FUNCTION__);
        fbe_release_contiguous_memory(sg_list);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    fbe_transport_initialize_packet(packet);

    fbe_transport_build_control_packet(packet, 
                                FBE_TOPOLOGY_CONTROL_CODE_ENUMERATE_OBJECTS,
                                &p_topology_mgmt_enumerate_objects,
                                0, /* no input data */
                                sizeof(fbe_topology_mgmt_enumerate_objects_t),
                                sep_sem_completion,
                                &sem);

    /* Set packet address */
    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_SEP_0,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                FBE_OBJECT_ID_INVALID);


    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_set_sg_list(payload, sg_list, 2);

    fbe_service_manager_send_control_packet(packet);
    fbe_semaphore_wait(&sem, NULL);

    *actual = p_topology_mgmt_enumerate_objects.number_of_objects_copied;

    fbe_semaphore_destroy(&sem);
    fbe_transport_release_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t 
sep_destroy_all_objects(void)
{
    fbe_object_id_t *						object_id_list = NULL;
    fbe_u32_t 								milliseconds_waited = 0;
    fbe_status_t 							status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t 								i = 0;
    fbe_u32_t								total_objects = 0;
    fbe_u32_t								actual_objects = 0;

    /*lets see if there are some leftovers*/
    status = sep_get_total_objects(&total_objects);
    if (status != FBE_STATUS_OK) {
        sep_log_error("%s: can't get total objects!\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (total_objects == 0) { goto skip_stuff; } /* SAFEBUG - prevent allocation of 0 bytes */
    object_id_list = (fbe_object_id_t *) fbe_allocate_contiguous_memory((sizeof(fbe_object_id_t) * total_objects));
    if (object_id_list == NULL) {
        sep_log_error("%s: can't allocate memory for object list!\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = sep_enumerate_objects(object_id_list, total_objects, &actual_objects);
    if (status != FBE_STATUS_OK) {
        sep_log_error("%s: can't enumerate objects!\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    while ((actual_objects != 0) &&
           (milliseconds_waited  < 120000)) { /* Wait 2 min delay (at most) */
        /* Wait 200 msec. */
        fbe_thread_delay(200);
        milliseconds_waited += 200;
        status = sep_enumerate_objects(object_id_list, total_objects, &actual_objects);
        if (status != FBE_STATUS_OK) {
            sep_log_error("%s: can't enumerate objects!\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* We have a race problem in VD object when set_destroy fight with set FAIL.
            This should be fixed in lifecycle - meantime the temporary fix may be: */
        for(i = 0; i < actual_objects; i++) {
            sep_set_object_destroy_condition(object_id_list[i]);
        }


    }

    /* print out object_id's */ 
    sep_log_info("%s: Found %X objects\n", __FUNCTION__, actual_objects);
    for(i = 0; i < actual_objects; i++) {
        /*
        sep_log_error("%s: It should never happen \n", __FUNCTION__);
        fbe_thread_delay(0xFFFFFFFF);
        */
        /* We want to stop since we have an object that is stuck. 
         * If we continue we will likely hit other strange issues as an object we are trying to use 
         * destroys itself. 
         */
        sep_log_critical_error("%s: destroy object_id %X \n", __FUNCTION__, object_id_list[i]);

        sep_destroy_object(object_id_list[i]);
    }
    fbe_release_contiguous_memory(object_id_list); /* SAFEBUG - needs to free memory */
    skip_stuff:;

    return FBE_STATUS_OK;
}

static fbe_status_t 
sep_destroy_object(fbe_object_id_t object_id)
{
    fbe_packet_t * packet = NULL;
    fbe_semaphore_t sem;
    fbe_topology_mgmt_destroy_object_t topology_mgmt_destroy_object;

    topology_mgmt_destroy_object.object_id = object_id;
    fbe_semaphore_init(&sem, 0, 1);

    /* Allocate packet */
    packet = fbe_transport_allocate_packet();
    if(packet == NULL) {
        sep_log_error("%s: fbe_transport_allocate_packet failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    fbe_transport_initialize_packet(packet);

    fbe_transport_build_control_packet(packet, 
                                FBE_TOPOLOGY_CONTROL_CODE_DESTROY_OBJECT,
                                &topology_mgmt_destroy_object,
                                sizeof(fbe_topology_mgmt_destroy_object_t),
                                0, /* no output data */
                                sep_sem_completion,
                                &sem);

    /* Set packet address */
    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_SEP_0,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                FBE_OBJECT_ID_INVALID);


    fbe_service_manager_send_control_packet(packet);
    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);
    fbe_transport_release_packet(packet);
    
    return FBE_STATUS_OK;
}

void sep_get_dps_memory_params(fbe_memory_dps_init_parameters_t *dps_params_p)
{
    *dps_params_p = sep_dps_init_parameters;
}
void sep_get_memory_parameters(  fbe_u32_t *sep_number_of_chunks,
                                 fbe_u32_t *platform_max_memory_mb,
                                 fbe_environment_limits_t *env_limits_p)
{
    fbe_u32_t   max_configurable_objects = 0;
    fbe_u32_t   total_database_tables_size = 0;

    /* Calculate the number of chunks using the system environment limits to allocate memory. 
       Number of chunks includes provision_drives(platform_fru_count), virtual_drive(platform_fru_count), max_rg and max_user_lun. 
       We also need to add 100 chunks for packet, object etc.. */
    (*sep_number_of_chunks) = (fbe_u32_t)(env_limits_p->platform_fru_count*2 + env_limits_p->platform_max_rg*2 + env_limits_p->platform_max_user_lun + 100);

	(*platform_max_memory_mb) -= ( (*sep_number_of_chunks) * FBE_MEMORY_CHUNK_SIZE) / (1024 * 1024);

    /* Reserve the memory budget from CMM for database tables, including user entry table, object entry table and edge table.
     * These part of memory will be allocated when initialize database service later.
     * The algorithm to calculate the database table size comes from fbe_database_allocate_config_tables()
     * */
    max_configurable_objects = env_limits_p->platform_fru_count * 2 + env_limits_p->platform_max_rg * 2 + FBE_RESERVED_OBJECT_IDS;
    max_configurable_objects += max_configurable_objects / 50;  // increase the max_configurable_objects by 2%
    total_database_tables_size = max_configurable_objects * (sizeof(database_object_entry_t) + sizeof(database_user_entry_t) + DATABASE_MAX_EDGE_PER_OBJECT * sizeof(database_edge_entry_t));
    (*platform_max_memory_mb) -= (total_database_tables_size + 1024 * 1024 - 1) / (1024 * 1024);

    sep_init_dps_params(*platform_max_memory_mb, &sep_dps_init_parameters);
}
fbe_status_t 
sep_init_services(fbe_sep_package_load_params_t *params_p)
{
    fbe_status_t status;
    fbe_u32_t platform_max_memory_mb;
    fbe_environment_limits_t env_limits;
    fbe_u32_t   sep_number_of_chunks;
    fbe_bool_t b_use_leftover;
    fbe_board_mgmt_platform_info_t 	platform_info;

    sep_log_info("%s starting...\n", __FUNCTION__);

    /* Initialize service manager */
    status = fbe_service_manager_init_service_table(sep_service_table);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: fbe_service_manager_init_service_table failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_SERVICE_MANAGER);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: sep_init_service failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }
    /*initialize the fbe trace service*/
    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_TRACE);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: trace service init failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }
    if (params_p)
    {
        fbe_trace_set_default_trace_level(params_p->default_trace_level);
        if (params_p->cerr_limit.action != FBE_TRACE_ERROR_LIMIT_ACTION_INVALID)
        {
            fbe_trace_set_error_limit(&params_p->cerr_limit);
        }
        if (params_p->error_limit.action != FBE_TRACE_ERROR_LIMIT_ACTION_INVALID)
        {
            fbe_trace_set_error_limit(&params_p->error_limit);
        }
    }
    /* Initialize the memory */
    fbe_sep_memory_init();
    fbe_memory_set_memory_functions(fbe_sep_memory_allocate, fbe_sep_memory_release);
    fbe_memory_dps_set_memory_functions(fbe_sep_memory_allocate, fbe_sep_memory_release);
    fbe_environment_limit_get_platform_max_memory_mb(&platform_max_memory_mb);

#if !(defined(UMODE_ENV) || defined(SIMMODE_ENV))
#ifdef FBE_KH_MISC_DEPEND
    ktrace_set_cmm_proc_address(cmmGetRBADataPoolID, cmmRegisterClient, cmmGetMemorySGL, cmmFreeMemorySGL, cmmMapMemorySVA, cmmUnMapMemorySVA);
#endif
#endif

    /* If this breaks, need to increase FBE_MAX_UPSTREAM_OBJECTS at least equal to PLATFORM_MAX_LUN_PER_RG(env_limits.platform_max_user_lun).*/
    FBE_ASSERT_AT_COMPILE_TIME(PLATFORM_MAX_LUN_PER_RG <= FBE_MAX_UPSTREAM_OBJECTS);

    status = fbe_environment_limit_get_initiated_limits(&env_limits);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: fbe_environment_limit_get_initiated_limits failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    /* We call this function to set the memory_env_limits in memory 
       for other clients use later. */
    fbe_memory_set_env_limits(&env_limits);

    sep_log_info("platform_fru_count: %d, max_rg: %d, max_user_lun: %d, max_memory_mb: %d \n", 
                 env_limits.platform_fru_count, env_limits.platform_max_rg, env_limits.platform_max_user_lun, platform_max_memory_mb);
    
    sep_get_memory_parameters(&sep_number_of_chunks, &platform_max_memory_mb, &env_limits);

    /* Initialize the memory */
    status =  fbe_memory_dps_init_number_of_chunks(&sep_dps_init_parameters);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: fbe_memory_dps_init_number_of_chunks failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    /* Initialize dps data pool*/
    if ((params_p != NULL) && (params_p->init_data_params.number_of_main_chunks != 0)) {
        sep_log_info("using supplied dps data params chunks - main: %d 64: %d rsvd 64: %d\n", 
                     params_p->init_data_params.number_of_main_chunks,
                     params_p->init_data_params.block_64_number_of_chunks,
                     params_p->init_data_params.reserved_block_64_number_of_chunks);
        sep_init_set_dps_data_chunks_from_params(&params_p->init_data_params, &sep_dps_init_data_parameters);
        b_use_leftover = FBE_FALSE;
    } else {
        sep_init_get_default_dps_data_chunks(platform_max_memory_mb, &sep_dps_init_data_parameters);
        b_use_leftover = FBE_TRUE;
    }

    fbe_sep_init_data_pool(&sep_dps_init_data_parameters, b_use_leftover);

    status =  fbe_memory_init_number_of_chunks(sep_number_of_chunks);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: fbe_memory_init_number_of_chunks failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }
    status =  fbe_base_service_send_init_command(FBE_SERVICE_ID_MEMORY);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: fbe_memory_init failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    status =  fbe_base_service_send_init_command(FBE_SERVICE_ID_EVENT_LOG);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: Event Log Serivce Init failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    /* Initialize topology */
    status = fbe_topology_init_class_table(sep_class_table);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: fbe_topology_init_class_table failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_TOPOLOGY);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: sep_init_service failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    /* If we have pvd debug flags passed in, initialize them.
     */
    if ((params_p != NULL ) && (params_p->pvd_class_debug_flags != 0))
    {
        status = sep_init_set_pvd_class_debug_flags(params_p->pvd_class_debug_flags);
        if(status != FBE_STATUS_OK) {
            sep_log_error("%s: set pvd debug flags failed, status: %X\n",
                    __FUNCTION__, status);
            return status;
        }
    }

    /* If we have virtual drive debug flags passed in, initialize them.
     */
    if ((params_p != NULL ) && (params_p->virtual_drive_debug_flags != 0))
    {
        status = sep_init_set_virtual_drive_class_debug_flags(params_p->virtual_drive_debug_flags);
        if(status != FBE_STATUS_OK) {
            sep_log_error("%s: set virtual drive debug flags failed, status: %X\n",
                    __FUNCTION__, status);
            return status;
        }
    }

    /* If we have rg debug flags passed in, initialize them.
     */
    if ((params_p != NULL ) && (params_p->raid_group_debug_flags != 0))
    {
        status = sep_init_set_raid_group_class_debug_flags(params_p->raid_group_debug_flags);
        if(status != FBE_STATUS_OK) {
            sep_log_error("%s: set RG debug flags failed, status: %X\n", __FUNCTION__, status);
            return status;
        }
    }

    /* Initialize notification service */
    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_NOTIFICATION);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: fbe_notification_init failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_CMI);
    if(status != FBE_STATUS_OK) {
		sep_log_critical_error("%s: CMI init failed, status: 0x%X\n",
                __FUNCTION__, status);
        return status;
    }

    
    fbe_sep_init_get_platform_info(&platform_info);

    /* Initialize metadata service */
    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_METADATA);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: fbe_notification_init failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    /* Set the nonpaged metadata to 8 blocks per object for MOONS platfrom.
     * Set the nonpaged metadata to 1 blocks per object for TRANSFORMERS and BEARCAT platform.
     */
    if (platform_info.hw_platform_info.cpuModule == OBERON_CPU_MODULE) {
       sep_init_set_metadata_nonpaged_blocks_per_object(8); /* 8 blocks per object */
    }
    else {
       sep_init_set_metadata_nonpaged_blocks_per_object(1); /* 1 blocks per object */
    }
    

    /* Initialize scheduler */
	/* Register plug ins first */
	fbe_scheduler_register_encryption_plugin(&encryption_plugin);

    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_SCHEDULER);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: fbe_scheduler_init failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    /* Insert the hooks in the scheduler */
    if (params_p != NULL) {      
        fbe_scheduler_set_startup_hooks(params_p->scheduler_hooks);
    }

    /* Initialize event service */
    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_EVENT);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: fbe_event_init failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    /* initialize the job service. */
    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_JOB_SERVICE);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: Job service init failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    /* initialize the NEW persistence service. */
     status = fbe_base_service_send_init_command(FBE_SERVICE_ID_PERSIST);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: Persistence service init failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_ENVIRONMENT_LIMIT);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: fbe_environment_limit_init failed, status 0x%x.\n",
                __FUNCTION__, status);
        return status;
    }
    /* If we have lifecycle debug flags passed in, initialize them.
     */
    if ((params_p != NULL ) && (params_p->lifecycle_class_debug_flags != 0))
    {
        status = sep_init_set_lifecycle_class_debug_flags(params_p->lifecycle_class_debug_flags);
        if(status != FBE_STATUS_OK)
        {
            sep_log_error("%s: set lifecycle debug flags failed, status: %X\n",
                    __FUNCTION__, status);
            return status;
        }
    }
    /* If we have ext pool params passed in, set them.
     */
    if ((params_p != NULL ) && (params_p->ext_pool_slice_blocks != FBE_LBA_INVALID)) {

        status = fbe_extent_pool_class_set_blocks_per_slice(params_p->ext_pool_slice_blocks);
        if(status != FBE_STATUS_OK) {
            sep_log_error("%s: set lifecycle debug flags failed, status: %X\n",
                    __FUNCTION__, status);
            return status;
        }
    }

    /*Initialize FBE perfstats service*/
    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_PERFSTATS);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: fbe_perfstats_service_init failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    /* Initialize logical configuration service */
    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_DATABASE);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: fbe_database_service_init failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

	#if 0 /*CMS DISABLED*/
    status =  fbe_base_service_send_init_command(FBE_SERVICE_ID_CMS);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: failed to init persistent memory service, status: %X\n",
                __FUNCTION__, status);
        return status;
    } 

    #endif

    /*initialize the fbe traffic trace service*/
    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_TRAFFIC_TRACE);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: traffic trace service init failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    /*initialize the sector trace service*/
    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_SECTOR_TRACE);
    if(status != FBE_STATUS_OK) {
        sep_log_error("%s: sector trace service init failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }
    return FBE_STATUS_OK;
}

fbe_status_t
sep_get_control_entry(fbe_service_control_entry_t * service_control_entry)
{
    *service_control_entry = fbe_service_manager_send_control_packet;
    return FBE_STATUS_OK;
}

static fbe_status_t 
sep_set_object_destroy_condition(fbe_object_id_t object_id)
{
    fbe_packet_t * packet;
    fbe_semaphore_t sem;

    fbe_semaphore_init(&sem, 0, 1);
    packet = fbe_transport_allocate_packet();
    if (packet == NULL) {
        sep_log_error("%s: fbe_transport_allocate_packet fail\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_transport_initialize_packet(packet);
    fbe_transport_build_control_packet(packet, 
                                       FBE_BASE_OBJECT_CONTROL_CODE_SET_DESTROY_COND,
                                       NULL,
                                       0,
                                       0,
                                       sep_sem_completion,
                                       &sem);
    fbe_transport_set_address(packet,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id);

    fbe_service_manager_send_control_packet(packet);
    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);
    fbe_transport_release_packet(packet);
    return FBE_STATUS_OK;
}

fbe_status_t
sep_get_io_entry(fbe_io_entry_function_t * io_entry)
{
    * io_entry = fbe_topology_send_io_packet;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_get_package_id(fbe_package_id_t * package_id)
{
    *package_id = FBE_PACKAGE_ID_SEP_0;
    return FBE_STATUS_OK;
}

static fbe_status_t sep_get_total_objects(fbe_u32_t *total)
{
    fbe_packet_t * 								packet = NULL;
    fbe_semaphore_t 							sem;
    fbe_topology_control_get_total_objects_t	get_total;

    fbe_semaphore_init(&sem, 0, 1);

    /* Allocate packet */
    packet = fbe_transport_allocate_packet();
    if(packet == NULL) {
        sep_log_error("%s: fbe_transport_allocate_packet failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    fbe_transport_initialize_packet(packet);

    fbe_transport_build_control_packet(packet, 
                                       FBE_TOPOLOGY_CONTROL_CODE_GET_TOTAL_OBJECTS,
                                       &get_total,
                                       sizeof(fbe_topology_control_get_total_objects_t),
                                       sizeof(fbe_topology_control_get_total_objects_t),
                                       sep_sem_completion,
                                       &sem);

    /* Set packet address */
    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_SEP_0,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                FBE_OBJECT_ID_INVALID);


    fbe_service_manager_send_control_packet(packet);
    fbe_semaphore_wait(&sem, NULL);

    *total = get_total.total_objects;

    fbe_semaphore_destroy(&sem);
    fbe_transport_release_packet(packet);

    return FBE_STATUS_OK;

}
static fbe_status_t sep_init_libraries(fbe_sep_package_load_params_t *params_p)
{
    fbe_status_t status;

    status = fbe_sep_init_psl(params_p);
    if (status != FBE_STATUS_OK)
    {
        sep_log_error("%s: fbe_sep_init_psl failed, status %d\n", __FUNCTION__, status);
        return status;
    }

    if (params_p != NULL)
    {
        status = fbe_raid_library_initialize(params_p->raid_library_debug_flags);
    }
    else
    {
        status = fbe_raid_library_initialize(FBE_RAID_LIBRARY_DEBUG_FLAG_NONE);
    }

    if (status != FBE_STATUS_OK)
    {
        sep_log_error("%s: fbe_raid_library_initialize failed, status %d\n", __FUNCTION__, status);
        return status;
    }
    status = fbe_xor_library_init();

    if (status != FBE_STATUS_OK)
    {
        sep_log_error("%s: fbe_xor_library_init failed, status %d\n", __FUNCTION__, status);
        return status;
    }
    return FBE_STATUS_OK;
}
static fbe_status_t sep_destroy_libraries(void)
{
    fbe_status_t status;
    status = fbe_raid_library_destroy();

    if (status != FBE_STATUS_OK)
    {
        sep_log_error("%s: fbe_raid_library_destroy failed, status %d\n", __FUNCTION__, status);
        return status;
    }
    status = fbe_xor_library_destroy();

    if (status != FBE_STATUS_OK)
    {
        sep_log_error("%s: fbe_xor_library_destroy failed, status %d\n", __FUNCTION__, status);
        return status;
    }
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_sep_memory_init(void)
{
   /* Register usage with the available pools in the CMM 
    * and get a small seed pool for use by Flare. 
    */
   cmm_control_pool_id = cmmGetLongTermControlPoolID();
   cmmRegisterClient(cmm_control_pool_id, 
                        NULL, 
                        0,  /* Minimum size allocation */  
                        CMM_MAXIMUM_MEMORY,   
                        "FBE memory", 
                        &cmm_client_id);

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_sep_memory_destroy(void)
{
    cmmDeregisterClient(cmm_client_id);
    return FBE_STATUS_OK;
}

static void * fbe_sep_memory_allocate(fbe_u32_t allocation_size_in_bytes)
{
	void * ptr = NULL;
    CMM_ERROR cmm_error;

	sep_log_info("MCRMEM: %d \n", allocation_size_in_bytes);

	cmm_error = cmmGetMemoryVA(cmm_client_id, allocation_size_in_bytes, &ptr);
   if (cmm_error != CMM_STATUS_SUCCESS) {
        sep_log_error("%s entry 1 cmmGetMemoryVA fail %X\n", __FUNCTION__, cmm_error);
        return NULL;
    }

    return ptr;
}

static void fbe_sep_memory_release(void * ptr)
{
    cmmFreeMemoryVA(cmm_client_id, ptr);
}

static fbe_status_t sep_init_wait_for_db_service(void)
{
    fbe_payload_ex_t *                 sep_payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_database_control_get_state_t 	control_state;
    fbe_packet_t *						packet = NULL;
    fbe_status_t 						status;

    /* Allocate packet.*/
    packet = fbe_transport_allocate_packet();
    if (packet == NULL) {
        sep_log_error ("%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_sep_packet(packet);
    sep_payload = fbe_transport_get_payload_ex (packet);
    //If the payload is NULL, exit and return generic failure.
    if (sep_payload == NULL) {
        fbe_transport_release_packet(packet);/*no need to destory !*/
        sep_log_error ("%s: sep_payload is NULL\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_allocate_control_operation(sep_payload);

    // If the control operation cannot be allocated, exit and return generic failure.
    if (control_operation == NULL) {
        fbe_transport_release_packet(packet);/*no need to destory !*/
        sep_log_error ("%s: control_operation is NULL\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_payload_control_build_operation (control_operation,
                                         FBE_DATABASE_CONTROL_CODE_GET_STATE,
                                         &control_state,
                                         sizeof(control_state));


    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_DATABASE,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID); 
    
    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    status = fbe_service_manager_send_control_packet(packet);
    
    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING && status != FBE_STATUS_BUSY){
        /* Only trace when it is an error we do not expect.*/
        sep_log_error("%s error in sending packet, status:%d\n", __FUNCTION__, status);
    }

    fbe_transport_wait_completion(packet);
    fbe_payload_ex_release_control_operation(sep_payload, control_operation);
    
    /* Save the status for returning.*/
    status = fbe_transport_get_status_code(packet);

    fbe_transport_release_packet(packet);

    if (status != FBE_STATUS_OK || (FBE_DATABASE_STATE_READY != control_state.state)) {
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}
static fbe_status_t sep_init_set_pvd_class_debug_flags(fbe_u32_t debug_flags)
{
    fbe_provision_drive_set_debug_trace_flag_t     debug_trace_flag;
    fbe_payload_ex_t *                 sep_payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_packet_t *						packet = NULL;
    fbe_status_t 						status;

    debug_trace_flag.trace_flag = debug_flags;

    /* Allocate packet.*/
    packet = fbe_transport_allocate_packet();
    if (packet == NULL) {
        sep_log_error ("%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_sep_packet(packet);
    sep_payload = fbe_transport_get_payload_ex (packet);
    //If the payload is NULL, exit and return generic failure.
    if (sep_payload == NULL) {
        fbe_transport_release_packet(packet);/*no need to destory !*/
        sep_log_error ("%s: sep_payload is NULL\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_allocate_control_operation(sep_payload);

    // If the control operation cannot be allocated, exit and return generic failure.
    if (control_operation == NULL) {
        fbe_transport_release_packet(packet);/*no need to destory !*/
        sep_log_error ("%s: control_operation is NULL\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_payload_control_build_operation(control_operation,
                                        FBE_PROVISION_DRIVE_CONTROL_CODE_SET_CLASS_DEBUG_FLAGS,
                                        &debug_trace_flag,
                                        sizeof(fbe_provision_drive_set_debug_trace_flag_t));

    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_PROVISION_DRIVE,
                              FBE_OBJECT_ID_INVALID); 
    
    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    status = fbe_service_manager_send_control_packet(packet);
    
    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING && status != FBE_STATUS_BUSY){
        /* Only trace when it is an error we do not expect.*/
        sep_log_error("%s error in sending packet to PVD class, status:%d\n", __FUNCTION__, status);
    }

    fbe_transport_wait_completion(packet);
    fbe_payload_ex_release_control_operation(sep_payload, control_operation);
    
    /* Save the status for returning.*/
    status = fbe_transport_get_status_code(packet);

    fbe_transport_release_packet(packet);

    return status;

}
static fbe_status_t sep_init_set_virtual_drive_class_debug_flags(fbe_u32_t debug_flags)
{
    fbe_payload_ex_t *                 sep_payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_packet_t *						packet = NULL;
    fbe_status_t 						status;
    fbe_u32_t                           set_debug_flags = debug_flags;

    /* Allocate packet.*/
    packet = fbe_transport_allocate_packet();
    if (packet == NULL) {
        sep_log_error ("%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_sep_packet(packet);
    sep_payload = fbe_transport_get_payload_ex (packet);
    //If the payload is NULL, exit and return generic failure.
    if (sep_payload == NULL) {
        fbe_transport_release_packet(packet);/*no need to destory !*/
        sep_log_error ("%s: sep_payload is NULL\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_allocate_control_operation(sep_payload);

    // If the control operation cannot be allocated, exit and return generic failure.
    if (control_operation == NULL) {
        fbe_transport_release_packet(packet);/*no need to destory !*/
        sep_log_error ("%s: control_operation is NULL\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_payload_control_build_operation(control_operation,
                                        FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_SET_DEBUG_FLAGS,
                                        &set_debug_flags,
                                        sizeof(fbe_u32_t));

    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_VIRTUAL_DRIVE,
                              FBE_OBJECT_ID_INVALID); 
    
    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    status = fbe_service_manager_send_control_packet(packet);
    
    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING && status != FBE_STATUS_BUSY){
        /* Only trace when it is an error we do not expect.*/
        sep_log_error("%s error in sending packet to vd class, status:%d\n", __FUNCTION__, status);
    }

    fbe_transport_wait_completion(packet);
    fbe_payload_ex_release_control_operation(sep_payload, control_operation);
    
    /* Save the status for returning.*/
    status = fbe_transport_get_status_code(packet);

    fbe_transport_release_packet(packet);

    return status;

}
static fbe_status_t sep_init_set_raid_group_class_debug_flags(fbe_u32_t debug_flags)
{
    fbe_raid_group_raid_group_debug_payload_t    debug_trace_flag;
    fbe_payload_ex_t *                 sep_payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_packet_t *						packet = NULL;
    fbe_status_t 						status;

    debug_trace_flag.raid_group_debug_flags = debug_flags;

    /* Allocate packet.*/
    packet = fbe_transport_allocate_packet();
    if (packet == NULL) {
        sep_log_error ("%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_sep_packet(packet);
    sep_payload = fbe_transport_get_payload_ex (packet);
    //If the payload is NULL, exit and return generic failure.
    if (sep_payload == NULL) {
        fbe_transport_release_packet(packet);/*no need to destory !*/
        sep_log_error ("%s: sep_payload is NULL\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_allocate_control_operation(sep_payload);

    // If the control operation cannot be allocated, exit and return generic failure.
    if (control_operation == NULL) {
        fbe_transport_release_packet(packet);/*no need to destory !*/
        sep_log_error ("%s: control_operation is NULL\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_payload_control_build_operation(control_operation,
                                        FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_RAID_GROUP_DEBUG_FLAGS,
                                        &debug_trace_flag,
                                        sizeof(fbe_raid_group_raid_group_debug_payload_t));

    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_PARITY,
                              FBE_OBJECT_ID_INVALID); 
    
    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    status = fbe_service_manager_send_control_packet(packet);
    
    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING && status != FBE_STATUS_BUSY){
        /* Only trace when it is an error we do not expect.*/
        sep_log_error("%s error in sending packet to RG class, status:%d\n", __FUNCTION__, status);
    }

    fbe_transport_wait_completion(packet);
    fbe_payload_ex_release_control_operation(sep_payload, control_operation);
    
    /* Save the status for returning.*/
    status = fbe_transport_get_status_code(packet);

    fbe_transport_release_packet(packet);

    return status;

}

/*!***************************************************************
 * sep_init_set_lifecycle_class_debug_flags()
 ****************************************************************
 * @brief
 *  This function is used to set the class level lifecycle debug
 *  tracing. This function is used to enable the lifecycle 
 *  tracing for all sep objects or any class of sep objects..
 *
 * @param fbe_u32_t - Debug flag to set.  
 * 
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  02/16/2012 - Created. Omer Miranda
 *
 ****************************************************************/
static fbe_status_t sep_init_set_lifecycle_class_debug_flags(fbe_u32_t debug_flags)
{
    fbe_lifecycle_debug_trace_control_t     debug_trace_flag;
    fbe_payload_ex_t *                 sep_payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_packet_t *						packet = NULL;
    fbe_status_t 						status;

    debug_trace_flag.trace_flag = debug_flags;

    /* Allocate packet.*/
    packet = fbe_transport_allocate_packet();
    if (packet == NULL) {
        sep_log_error ("%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_sep_packet(packet);
    sep_payload = fbe_transport_get_payload_ex (packet);
    //If the payload is NULL, exit and return generic failure.
    if (sep_payload == NULL) {
        fbe_transport_release_packet(packet);/*no need to destory !*/
        sep_log_error ("%s: sep_payload is NULL\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_allocate_control_operation(sep_payload);

    // If the control operation cannot be allocated, exit and return generic failure.
    if (control_operation == NULL) {
        fbe_transport_release_packet(packet);/*no need to destory !*/
        sep_log_error ("%s: control_operation is NULL\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_payload_control_build_operation(control_operation,
                                        FBE_TRACE_CONTROL_CODE_SET_LIFECYCLE_DEBUG_TRACE_FLAG,
                                        &debug_trace_flag,
                                        sizeof(fbe_lifecycle_debug_trace_control_t));

    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TRACE,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID); 
    
    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    status = fbe_service_manager_send_control_packet(packet);
    
    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING && status != FBE_STATUS_BUSY){
        /* Only trace when it is an error we do not expect.*/
        sep_log_error("%s error in sending packet to PVD class, status:%d\n", __FUNCTION__, status);
    }

    fbe_transport_wait_completion(packet);
    fbe_payload_ex_release_control_operation(sep_payload, control_operation);
    
    /* Save the status for returning.*/
    status = fbe_transport_get_status_code(packet);

    fbe_transport_release_packet(packet);

    return status;

}

#define FBE_SEP_DPS_FAST_PACKET_PERCENT     6.0		/*   percent for fast packets */   
#define FBE_SEP_DPS_FAST_64_BLOCK_PERCENT	91.0	/*  percent for fast 64-blks */
                                              /*  3 percent in reserve  */

static fbe_u32_t sep_init_core_count = 0;
void fbe_sep_init_set_simulated_core_count(fbe_u32_t core_count)
{
    sep_init_core_count = core_count;
}
static fbe_status_t 
sep_init_dps_params(fbe_u64_t platform_max_memory_mb, fbe_memory_dps_init_parameters_t * params)
{
    fbe_u32_t leftover;
    fbe_u32_t core_count;
    /* Allows us to simulate various core counts to get memory sizes.
     */
    if (sep_init_core_count != 0)
    {
        core_count = sep_init_core_count;
    }
    else
    {
        core_count= fbe_get_cpu_count();
    }
	cmm_data_pool_id = cmmGetSEPDataPoolID();

	if(cmm_data_pool_id != NULL){
		sep_log_info("CMM Data pool enabled\n");
#if defined(SIMMODE_ENV)
		/* For simulation only */
		params->packet_number_of_chunks = 2;
		params->block_64_number_of_chunks = 20;
#else
		/* It is new platform with MCR data memory 
			We will reduce the max memory size by 62 MB wich is used by NP, Paged, SL, Scheduler etc. 
		*/
		//platform_max_memory_mb -= 62;
		params->packet_number_of_chunks = 4;
		params->block_64_number_of_chunks = 32;
#endif

		params->reserved_packet_number_of_chunks = 4;
		params->reserved_block_64_number_of_chunks = 32;

	} else { /* Old platforms. Mustang etc. */ 
		params->packet_number_of_chunks = 4;
		params->block_64_number_of_chunks = 56;

		params->reserved_packet_number_of_chunks = 4;
		params->reserved_block_64_number_of_chunks = 56;
	}

    params->block_1_number_of_chunks = 0;

    params->reserved_block_1_number_of_chunks = 0;

	params->memory_ptr = NULL;

    /* We need 1 chunk per core primarily for DCA requests */
    params->fast_block_1_number_of_chunks = fbe_get_cpu_count();

#if defined(SIMMODE_ENV)
    params->fast_packet_number_of_chunks = (fbe_u32_t)(core_count * 2); /* 1500 packets per core */
    params->fast_block_64_number_of_chunks = (fbe_u32_t)(core_count * 4); 
#else
    /* This gives us half (512 I/Os per core), the max we could see.
     */
    params->fast_packet_number_of_chunks = (fbe_u32_t)(core_count * 6); /* 1500 packets per core */
    params->fast_block_64_number_of_chunks = (fbe_u32_t)(core_count * 16); 
    //params->fast_packet_number_of_chunks = (fbe_u32_t)(((leftover * FBE_SEP_DPS_FAST_PACKET_PERCENT) / 100.0) + 1);
    //params->fast_block_64_number_of_chunks = (fbe_u32_t)(((leftover * FBE_SEP_DPS_FAST_64_BLOCK_PERCENT) / 100.0) + 
    //1); 
#endif
    params->number_of_main_chunks =  params->packet_number_of_chunks + 
                                        params->block_1_number_of_chunks +
                                        params->block_64_number_of_chunks +
                                        params->fast_packet_number_of_chunks + 
                                        params->fast_block_1_number_of_chunks +
                                        params->fast_block_64_number_of_chunks +
                                        params->reserved_packet_number_of_chunks + 
                                        params->reserved_block_1_number_of_chunks +
                                        params->reserved_block_64_number_of_chunks + 1;

    /* If we have any leftover memory, we will put it into the fast 64 block pool.
     * Since this is used when we run out of the fast pools. 
     */
    if (params->number_of_main_chunks > (fbe_u32_t)platform_max_memory_mb)
    {
        sep_log_info("SEP:  We took %d MB of memory for dps > expected platform max memory %d MB.\n",
                     params->number_of_main_chunks,
                     (fbe_u32_t)platform_max_memory_mb);
    }
    else
    {
        leftover = (fbe_u32_t)platform_max_memory_mb - params->number_of_main_chunks;
        params->fast_block_64_number_of_chunks += leftover;
        params->number_of_main_chunks += leftover;
    }

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sep_init_get_board_object_id(fbe_object_id_t *board_id)
{
    fbe_status_t                  		status = FBE_STATUS_GENERIC_FAILURE;
    fbe_topology_control_get_board_t 	board_info;
    fbe_packet_t *                  	packet_p = NULL;
    fbe_payload_ex_t *             		payload = NULL;
    fbe_payload_control_operation_t *	control_operation = NULL;
    fbe_payload_control_status_t    	payload_control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    
    packet_p = fbe_allocate_nonpaged_pool_with_tag ( sizeof (fbe_packet_t), 'pfbe');
    if (packet_p == NULL) {
        sep_log_error("%s:can't allocate packet\n",__FUNCTION__);

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
            sep_log_error("%s:failed to send control packet\n",__FUNCTION__);
        
    }

    fbe_transport_wait_completion(packet_p);

    fbe_payload_control_get_status(control_operation, &payload_control_status);
    if(payload_control_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        sep_log_error("%s:bad return status\n",__FUNCTION__);
    }

    status  = fbe_transport_get_status_code(packet_p);
    if(status != FBE_STATUS_OK){
        sep_log_error("%s:bad return status\n",__FUNCTION__);
    }

    fbe_payload_ex_release_control_operation(payload, control_operation);
    fbe_transport_destroy_packet(packet_p);
    fbe_release_nonpaged_pool(packet_p);

    *board_id = board_info.board_object_id;

    return status;

}

static fbe_status_t fbe_sep_init_psl(fbe_sep_package_load_params_t *params_p)
{
    fbe_board_mgmt_platform_info_t 	platform_info;
    
    fbe_sep_init_get_platform_info(&platform_info);

    if ( (params_p != NULL) &&
         (params_p->flags & FBE_SEP_LOAD_FLAG_USE_SIM_PSL)) {
        fbe_private_space_layout_initialize_fbe_sim();
    } else {
        fbe_private_space_layout_initialize_library(platform_info.hw_platform_info.platformType);
    }

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_sep_init_data_pool(fbe_memory_dps_init_parameters_t *init_data_parameters, fbe_bool_t b_use_leftover)
{
	fbe_u32_t i;
	void * virtual_addr = NULL;
    CMM_ERROR cmm_error;
	csx_p_io_map_t  io_map_obj;
	fbe_u64_t total_mem_mb;
    fbe_u32_t cmi_data_length_mb = 6;
    fbe_u32_t cmi_data_length = cmi_data_length_mb * 1024 * 1024;
    void * cmi_data_p = NULL;
    fbe_u32_t db_data_length_mb = 5;
    fbe_u32_t db_data_length = db_data_length_mb * 1024 * 1024;
    void * db_data_p = NULL;

	cmm_data_pool_id = cmmGetSEPDataPoolID();

	if(cmm_data_pool_id == NULL){
		sep_log_info("CMM Data pool disabled\n");
        cmi_data_p = fbe_allocate_contiguous_memory(cmi_data_length);
		fbe_cmi_io_init_data_memory(cmi_data_length_mb, cmi_data_p);

        db_data_p = fbe_allocate_contiguous_memory(db_data_length);
		fbe_db_init_data_memory(db_data_length_mb, db_data_p);
		return FBE_STATUS_OK; 
	}

	cmmRegisterClient(cmm_data_pool_id, 
                        NULL, 
                        0,  /* Minimum size allocation */  
                        CMM_MAXIMUM_MEMORY,   
                        "SEP data memory", 
                        &cmm_data_client_id);

	cmm_data_sgl = cmmGetPoolSGL(cmm_data_pool_id);
	if(cmm_data_sgl != NULL){
		for(i = 0; i < 10; i++){
			sep_log_info("CMM Data SGL [%d] Addr: %llx Len: %lld\n", i, cmm_data_sgl[i].MemSegAddress, (long long)cmm_data_sgl[i].MemSegLength);
			if(cmm_data_sgl[i].MemSegLength == 0){
				break;
			}
		}
	}

	/* Allocate cmm data memory */
	cmm_error = cmmMapMemorySVA(cmm_data_sgl[0].MemSegAddress, cmm_data_sgl[0].MemSegLength, &virtual_addr, (void *)&io_map_obj);
	if (cmm_error != CMM_STATUS_SUCCESS){
		sep_log_info("CMM Data Can't map memory\n");
	} else {
	   sep_log_info("CMM Data Virtual address: %p \n", virtual_addr);
	}

	total_mem_mb = cmm_data_sgl[0].MemSegLength / (1024 * 1024);

    /* Carve Data memory for DB service */
    if (total_mem_mb > (MINIMUM_SEP_DPS_DATA_MB + TOTAL_SEP_IO_MEM_MB + SEP_DB_DATA_MEM_MB)) {
		fbe_db_init_data_memory(SEP_DB_DATA_MEM_MB, virtual_addr);
		total_mem_mb -= SEP_DB_DATA_MEM_MB;
		virtual_addr = (void *)((fbe_u64_t)virtual_addr + (SEP_DB_DATA_MEM_MB * 1024 * 1024));
	}
    else
    {
		sep_log_info("CMM Data pool is too small: %lld MB\n", (long long)cmm_data_sgl[0].MemSegLength / (1024 * 1024));
        db_data_p = fbe_allocate_contiguous_memory(db_data_length);
		fbe_db_init_data_memory(db_data_length_mb, db_data_p);
    }

    /* Allocate 100 MB for FBE CMI SEP IOs */
    if (total_mem_mb > (MINIMUM_SEP_DPS_DATA_MB + TOTAL_SEP_IO_MEM_MB)) {
		fbe_cmi_io_init_data_memory(TOTAL_SEP_IO_MEM_MB, virtual_addr);
		total_mem_mb -= TOTAL_SEP_IO_MEM_MB;
		virtual_addr = (void *)((fbe_u64_t)virtual_addr + (TOTAL_SEP_IO_MEM_MB * 1024 * 1024));
	}
    else
    {
		sep_log_info("CMM Data pool is too small: %lld MB\n", (long long)cmm_data_sgl[0].MemSegLength / (1024 * 1024));
        cmi_data_p = fbe_allocate_contiguous_memory(cmi_data_length);
		fbe_cmi_io_init_data_memory(cmi_data_length_mb, cmi_data_p);
    }

    if(total_mem_mb > MINIMUM_SEP_DPS_DATA_MB){
		init_data_parameters->memory_ptr = virtual_addr;
		sep_init_dps_data_params(total_mem_mb, init_data_parameters, b_use_leftover);
		fbe_memory_dps_init_number_of_data_chunks(init_data_parameters);
	} else {
		sep_log_info("CMM Data pool is too small: %lld MB\n", (long long)cmm_data_sgl[0].MemSegLength / (1024 * 1024));
		return FBE_STATUS_OK; 
	}

	return FBE_STATUS_OK;
}

static fbe_status_t fbe_sep_destroy_data_pool(void)
{
	cmmDeregisterClient(cmm_data_client_id);
	return FBE_STATUS_OK;
}

static fbe_status_t 
sep_init_get_default_dps_data_chunks(fbe_u64_t platform_max_memory_mb, fbe_memory_dps_init_parameters_t *init_dps_data_params)
{
#if defined(SIMMODE_ENV)
    init_dps_data_params->packet_number_of_chunks = 2;
    init_dps_data_params->block_64_number_of_chunks = 28;
#else
    init_dps_data_params->packet_number_of_chunks = 4;
    init_dps_data_params->block_64_number_of_chunks = 56;
#endif
    init_dps_data_params->block_1_number_of_chunks = 0;

    init_dps_data_params->reserved_packet_number_of_chunks = 4;
    init_dps_data_params->reserved_block_64_number_of_chunks = 70;
    init_dps_data_params->reserved_block_1_number_of_chunks = 0;

    init_dps_data_params->fast_packet_number_of_chunks = 0;
    init_dps_data_params->fast_block_64_number_of_chunks = 0;

    init_dps_data_params->number_of_main_chunks = (fbe_u32_t)platform_max_memory_mb;

    return FBE_STATUS_OK;
}

static fbe_status_t 
sep_init_dps_data_params(fbe_u64_t platform_max_memory_mb, fbe_memory_dps_init_parameters_t *params, fbe_bool_t b_use_leftover)
{
    fbe_u64_t leftover = 0;

    /* We need 1 chunk per core primarily for DCA requests */
    //params->fast_block_1_number_of_chunks = fbe_get_cpu_count();
    if (b_use_leftover) {
        params->number_of_main_chunks = (fbe_u32_t)platform_max_memory_mb;

        leftover = platform_max_memory_mb;
        leftover -= params->packet_number_of_chunks;
        leftover -= params->block_64_number_of_chunks;
        leftover -= params->reserved_packet_number_of_chunks;
        leftover -= params->reserved_block_64_number_of_chunks;
        leftover -= params->fast_block_1_number_of_chunks;
        
        params->fast_packet_number_of_chunks = (fbe_u32_t)(((leftover * FBE_SEP_DPS_FAST_PACKET_PERCENT) / 100.0) + 1);
        params->fast_block_64_number_of_chunks = (fbe_u32_t)(((leftover * FBE_SEP_DPS_FAST_64_BLOCK_PERCENT) / 100.0) + 1);
        params->fast_block_64_number_of_chunks = FBE_MIN(((fbe_u32_t)leftover - params->fast_packet_number_of_chunks), params->fast_block_64_number_of_chunks);
    }

    if(params->number_of_main_chunks < params->packet_number_of_chunks + 
                                        params->block_64_number_of_chunks +
                                        params->fast_packet_number_of_chunks + 
                                        params->fast_block_64_number_of_chunks + 
                                        params->reserved_packet_number_of_chunks + 
                                        params->reserved_block_64_number_of_chunks) 
    {
        sep_log_info("%s: platform_max_mem:%d < %d + %d + %d + %d + %d + %d\n", 
            __FUNCTION__, params->number_of_main_chunks, params->packet_number_of_chunks, params->block_64_number_of_chunks,
            params->fast_packet_number_of_chunks, params->fast_block_64_number_of_chunks,
			params->reserved_packet_number_of_chunks, params->reserved_block_64_number_of_chunks);

       params->number_of_main_chunks =  params->packet_number_of_chunks + 
                                        params->block_64_number_of_chunks +
                                        params->fast_packet_number_of_chunks + 
                                        params->fast_block_64_number_of_chunks +
                                        params->reserved_packet_number_of_chunks + 
                                        params->reserved_block_64_number_of_chunks + 1;

    }
                                    
    return FBE_STATUS_OK;
}

static fbe_status_t 
sep_init_set_dps_data_chunks_from_params(fbe_memory_dps_init_parameters_t *test_dps_data_params, fbe_memory_dps_init_parameters_t *init_dps_data_params)
{
    init_dps_data_params->packet_number_of_chunks = test_dps_data_params->packet_number_of_chunks;
    init_dps_data_params->block_64_number_of_chunks = test_dps_data_params->block_64_number_of_chunks;
    init_dps_data_params->block_1_number_of_chunks = test_dps_data_params->block_1_number_of_chunks;

    init_dps_data_params->reserved_packet_number_of_chunks = test_dps_data_params->reserved_packet_number_of_chunks;
    init_dps_data_params->reserved_block_64_number_of_chunks = test_dps_data_params->reserved_block_64_number_of_chunks;
    init_dps_data_params->reserved_block_1_number_of_chunks = test_dps_data_params->reserved_block_1_number_of_chunks;

    init_dps_data_params->fast_packet_number_of_chunks = test_dps_data_params->fast_packet_number_of_chunks;
    init_dps_data_params->fast_block_64_number_of_chunks = test_dps_data_params->fast_block_64_number_of_chunks;

    init_dps_data_params->number_of_main_chunks = test_dps_data_params->number_of_main_chunks;

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_sep_init_get_platform_info(fbe_board_mgmt_platform_info_t *platform_info) 
{
    fbe_status_t                  	status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                  	packet_p = NULL;
    fbe_payload_ex_t *             	payload = NULL;
    fbe_payload_control_operation_t *	control_operation = NULL;
    fbe_payload_control_status_t    	payload_control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_object_id_t 			board_object_id;

    fbe_sep_init_get_board_object_id(&board_object_id);

    /* The memory service is not available yet */
    packet_p = fbe_allocate_nonpaged_pool_with_tag ( sizeof (fbe_packet_t), 'pfbe');

    if (packet_p == NULL) {
        sep_log_error("%s:can't allocate packet\n",__FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;

    }

    fbe_transport_initialize_packet(packet_p);
    payload = fbe_transport_get_payload_ex (packet_p);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    fbe_payload_control_build_operation (control_operation,
                                         FBE_BASE_BOARD_CONTROL_CODE_GET_PLATFORM_INFO,
                                         platform_info,
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
            sep_log_error("%s:failed to send control packet\n",__FUNCTION__);
        
    }

    fbe_transport_wait_completion(packet_p);

    fbe_payload_control_get_status(control_operation, &payload_control_status);
    if(payload_control_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        sep_log_error("%s:bad return status\n",__FUNCTION__);
    }

    status  = fbe_transport_get_status_code(packet_p);
    if(status != FBE_STATUS_OK){
        sep_log_error("%s:bad return status\n",__FUNCTION__);
    }

    fbe_payload_ex_release_control_operation(payload, control_operation);
    fbe_transport_destroy_packet(packet_p);
    fbe_release_nonpaged_pool(packet_p);

    return status;
}

static fbe_status_t
sep_init_set_metadata_nonpaged_blocks_per_object(fbe_u32_t blocks_per_object)
{
    fbe_payload_ex_t *                 sep_payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_packet_t *						packet = NULL;
    fbe_status_t 						status;

    /* Allocate packet.*/
    packet = fbe_transport_allocate_packet();
    if (packet == NULL) {
        sep_log_error ("%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_sep_packet(packet);
    sep_payload = fbe_transport_get_payload_ex (packet);
    //If the payload is NULL, exit and return generic failure.
    if (sep_payload == NULL) {
        fbe_transport_release_packet(packet);/*no need to destory !*/
        sep_log_error ("%s: sep_payload is NULL\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_allocate_control_operation(sep_payload);

    // If the control operation cannot be allocated, exit and return generic failure.
    if (control_operation == NULL) {
        fbe_transport_release_packet(packet);/*no need to destory !*/
        sep_log_error ("%s: control_operation is NULL\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_payload_control_build_operation(control_operation,
                                        FBE_METADATA_CONTROL_CODE_NONPAGED_SET_BLOCKS_PER_OBJECT,
                                        &blocks_per_object,
                                        sizeof(blocks_per_object));

    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_METADATA,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID); 
    
    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    status = fbe_service_manager_send_control_packet(packet);
    
    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING && status != FBE_STATUS_BUSY){
        /* Only trace when it is an error we do not expect.*/
        sep_log_error("%s error in sending packet to metadata service, status:%d\n", __FUNCTION__, status);
    }

    fbe_transport_wait_completion(packet);
    fbe_payload_ex_release_control_operation(sep_payload, control_operation);
    
    /* Save the status for returning.*/
    status = fbe_transport_get_status_code(packet);

    fbe_transport_release_packet(packet);

    return status;
}

