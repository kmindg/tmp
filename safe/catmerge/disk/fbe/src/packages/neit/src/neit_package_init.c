#include "csx_ext.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_queue.h"
#include "fbe_transport_memory.h"
#include "fbe_topology.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_neit_package.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe_neit_private.h"
#include "fbe/fbe_time.h"
#include "neit_package_service_table.h"
#include "fbe_cmm.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_random.h"
#include "fbe/fbe_board.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_api_physical_drive_interface.h"

static fbe_trace_level_t neit_trace_level = FBE_TRACE_LEVEL_INFO;   /* Set INFO by default for debugging purposes */
static fbe_bool_t neit_b_disable_raw_mirror = FBE_FALSE;            /* By default load the neit raw mirror service */
#define FBE_NEIT_PACKAGE_NUMBER_OF_CHUNKS FBE_NEIT_PACKAGE_NUMBER_OF_OUTSTANDING_PACKETS

typedef fbe_u32_t fbe_neit_magic_number_t;
enum {
    FBE_NEIT_MAGIC_NUMBER = 0x4E454954,  /* ASCII for NEIT */
};

typedef struct fbe_neit_error_element_s {
    fbe_queue_element_t queue_element;
    fbe_neit_magic_number_t magic_number;
    fbe_neit_error_record_t neit_error_record;
}fbe_neit_error_element_t;

#define FBE_NEIT_NUMBER_OF_CHUNKS 2048
#define FBE_NEIT_NUMBER_OF_DPS_CHUNKS 90
#define FBE_NEIT_NUMBER_OF_DPS_PACKET_CHUNKS 20
#define FBE_NEIT_NUMBER_OF_DPS_64_BLOCK_CHUNKS 70

static fbe_memory_dps_init_parameters_t neit_dps_init_parameters = 
{
    0, //FBE_NEIT_NUMBER_OF_DPS_CHUNKS,
    0, //FBE_NEIT_NUMBER_OF_DPS_PACKET_CHUNKS,
    0, //FBE_NEIT_NUMBER_OF_DPS_64_BLOCK_CHUNKS,
	0,
	0
};

static fbe_status_t neit_package_init_services(fbe_neit_package_load_params_t *params_p);
static void neit_package_log_error(const char * fmt, ...);
static void neit_package_log_info(const char * fmt, ...);
static fbe_status_t neit_package_init_libraries(void);
static fbe_status_t neit_package_destroy_libraries(void);

static fbe_status_t fbe_neit_memory_init(void);
static fbe_status_t fbe_neit_memory_destroy(void);
static void * fbe_neit_memory_allocate(fbe_u32_t allocation_size_in_bytes);
static void fbe_neit_memory_release(void * ptr);
fbe_status_t neit_package_init_psl(void);

fbe_status_t neit_package_init(fbe_neit_package_load_params_t *params_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    neit_package_log_info("%s starting...\n", __FUNCTION__);

    fbe_transport_init();
    status = fbe_api_neit_package_common_init();
    if (status != FBE_STATUS_OK) {
		neit_package_log_error("%s: fbe_api_neit_common_init failed, status 0x%x.\n",
							   __FUNCTION__, status);
        return status;
    }

    if (params_p != NULL)
    {
        /* Setup any values that need to be there when the packages are loaded.
         */
        neit_package_log_info("%s set random seed to 0x%x\n", __FUNCTION__, params_p->random_seed);
        fbe_random_set_seed(params_p->random_seed);
    }

    status = neit_package_init_libraries();
    if(status != FBE_STATUS_OK) {
        neit_package_log_error("%s: neit_package_init_libraries  failed, status: %X\n",
                               __FUNCTION__, status);
    }

    status = neit_package_init_services(params_p);
    if(status != FBE_STATUS_OK) {
        neit_package_log_error("%s: neit_package_init_services failed, status: %X\n",
                __FUNCTION__, status);
    }

    return status;
}
fbe_status_t neit_package_destroy(void)
{   
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    neit_package_log_info("%s entry\n", __FUNCTION__);


    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_DEST);
    if(status != FBE_STATUS_OK) {
        neit_package_log_error("%s: FBE_SERVICE_ID_DEST failed, status: %X\n",
                               __FUNCTION__, status);
        return status;
    }

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_PROTOCOL_ERROR_INJECTION);
    if(status != FBE_STATUS_OK) {
        neit_package_log_error("%s: PROTOCOL_ERROR_INJECTION failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_RDGEN);
    if(status != FBE_STATUS_OK) {
        neit_package_log_error("%s: FBE_SERVICE_ID_RDGEN failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION);
    if(status != FBE_STATUS_OK) {
        neit_package_log_error("%s: LOGICAL_ERROR_INJECTION failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    if (neit_b_disable_raw_mirror == FBE_FALSE) {
        status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_RAW_MIRROR);
        if(status != FBE_STATUS_OK) {
            neit_package_log_error("%s: FBE_SERVICE_ID_RAW_MIRROR failed, status: %X\n",
                    __FUNCTION__, status);
            return status;
        }
    }
    else {
        neit_package_log_info("%s: Skip destroy RAW MIRROR\n", __FUNCTION__);
        neit_b_disable_raw_mirror = FBE_FALSE;
    }

	#if 0 /*CMS DISABLED*/
	status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_CMS_EXERCISER);
    if(status != FBE_STATUS_OK) {
        neit_package_log_error("%s: cms exerciser destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }
	#endif

	status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_CMI);
	if(status != FBE_STATUS_OK) {
		neit_package_log_error("%s: CMI service destroy failed, status: %X\n",
				__FUNCTION__, status);
		return status;
	}

    status =  fbe_base_service_send_destroy_command(FBE_SERVICE_ID_MEMORY);
    if(status != FBE_STATUS_OK) {
        neit_package_log_error("%s: fbe_memory_destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

	status =  fbe_base_service_send_destroy_command(FBE_SERVICE_ID_NOTIFICATION);
    if(status != FBE_STATUS_OK) {
        neit_package_log_error("%s: notification destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

	status = fbe_api_neit_package_common_destroy();
    if(status != FBE_STATUS_OK) {
        neit_package_log_error("%s: fbe_api_neit_common_destroy failed, status 0x%x.\n",
					  __FUNCTION__, status);
        return status;
    }
    
    fbe_neit_memory_destroy();

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_SERVICE_MANAGER);
    if(status != FBE_STATUS_OK) {
        neit_package_log_error("%s: fbe_service_manager destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    status = neit_package_destroy_libraries();
    if(status != FBE_STATUS_OK) {
        neit_package_log_error("%s: neit_package_destroy_libraries() failed, status: %X\n",
                      __FUNCTION__, status);
        return status;
    }

    fbe_transport_destroy();
   
    return FBE_STATUS_OK;
}

static fbe_status_t neit_package_init_services(fbe_neit_package_load_params_t *params_p)
{
    fbe_status_t status;
    neit_package_log_info("%s starting...\n", __FUNCTION__);

    /* Initialize service manager */
    status = fbe_service_manager_init_service_table(neit_package_service_table);
    if(status != FBE_STATUS_OK) {
        neit_package_log_error("%s: fbe_service_manager_init_service_table failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_SERVICE_MANAGER);
    if(status != FBE_STATUS_OK) {
        neit_package_log_error("%s: sep_init_service failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    /* Initialize the memory */
    fbe_neit_memory_init();
    fbe_memory_set_memory_functions(fbe_neit_memory_allocate, fbe_neit_memory_release);

    /* Initialize the memory */
    status =  fbe_memory_dps_init_number_of_chunks(&neit_dps_init_parameters);
    if(status != FBE_STATUS_OK) {
        neit_package_log_error("%s: fbe_memory_dps_init_number_of_chunks failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }
    /* Initialize the memory */
    status =  fbe_memory_init_number_of_chunks(FBE_NEIT_PACKAGE_NUMBER_OF_CHUNKS);
    if(status != FBE_STATUS_OK) {
        neit_package_log_error("%s: fbe_memory_init_number_of_chunks failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

	/*PAY ATTENTION- In NEIT, the DPS memory will not be initialized unless explicitly called form RDGEN*/
    status =  fbe_base_service_send_init_command(FBE_SERVICE_ID_MEMORY);
    if(status != FBE_STATUS_OK) {
        neit_package_log_error("%s: fbe_memory_init failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    /*initialize the drive trace service*/
    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_TRACE);
    if(status != FBE_STATUS_OK) {
        neit_package_log_error("%s: trace service init failed, status: %X\n",
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

    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_CMI);
    if(status != FBE_STATUS_OK) {
        neit_package_log_error("%s: CMI init failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }
    
    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_RDGEN);
    if (status != FBE_STATUS_OK) {
        neit_package_log_error("%s: init of rdgen service failed, status: %x\n",
                __FUNCTION__, status);
        return status;
    }

    if (params_p)
    {
        if (params_p->load_raw_mirror!=FBE_FALSE) 
        {
            /* Initialize the raw mirror service */
            status = fbe_base_service_send_init_command(FBE_SERVICE_ID_RAW_MIRROR);
            if(status != FBE_STATUS_OK) {
                neit_package_log_error("%s: raw_mirror_service_init failed, status: %X\n",
                                         __FUNCTION__, status);
                return status;
            }
        }else
        {
            neit_package_log_info("%s: Skip init RAW MIRROR\n", __FUNCTION__);
            neit_b_disable_raw_mirror = FBE_TRUE;
        }
    }else
    {
        /* Initialize the raw mirror service */
        status = fbe_base_service_send_init_command(FBE_SERVICE_ID_RAW_MIRROR);
        if(status != FBE_STATUS_OK) {
            neit_package_log_error("%s: raw_mirror_service_init failed, status: %X\n",
                                     __FUNCTION__, status);
            return status;
        }
    }

	/* Initialize dest service */
    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_DEST);
    if(status != FBE_STATUS_OK) {
        neit_package_log_error("%s: dest init failed, status: %X\n",
                               __FUNCTION__, status);
        return status;
    }
	
    /* Initialize protocol error injection service */
    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_PROTOCOL_ERROR_INJECTION);
    if(status != FBE_STATUS_OK) {
        neit_package_log_error("%s: protocol_error_injectio_init failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    /* Initialize protocol error injection service */
    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION);
    if(status != FBE_STATUS_OK) {
        neit_package_log_error("%s: logical_error_injection_init failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

	#if 0 /*CMS disabled*/
	status = fbe_base_service_send_init_command(FBE_SERVICE_ID_CMS_EXERCISER);
    if(status != FBE_STATUS_OK) {
        neit_package_log_error("%s: cms exerciser init failed: %X\n",
                __FUNCTION__, status);
        return status;
    }
	#endif

	 status =  fbe_base_service_send_init_command(FBE_SERVICE_ID_NOTIFICATION);
    if(status != FBE_STATUS_OK) {
        neit_package_log_error("%s: notification init failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    return FBE_STATUS_OK;
}

fbe_status_t 
neit_package_get_record(fbe_neit_record_handle_t neit_record_handle, fbe_neit_error_record_t * neit_error_record)
{
    fbe_neit_error_element_t * neit_error_element = NULL;
   
    neit_error_element = (fbe_neit_error_element_t *)neit_record_handle;

    if((neit_error_element == NULL) || (neit_error_element->magic_number != FBE_NEIT_MAGIC_NUMBER)) {
        neit_package_log_error("%s Invalid handle\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(neit_error_record, &neit_error_element->neit_error_record, sizeof(fbe_neit_error_record_t));

    return FBE_STATUS_OK;
}

static void
neit_package_log_error(const char * fmt, ...)
{
    va_list args;
    if(FBE_TRACE_LEVEL_ERROR <= neit_trace_level) {
        va_start(args, fmt);
        fbe_trace_report(FBE_COMPONENT_TYPE_PACKAGE,
                        FBE_PACKAGE_ID_NEIT,
                        FBE_TRACE_LEVEL_ERROR,
                        0, /* message_id */
                        fmt, 
                        args);
        va_end(args);
    }
}

static void
neit_package_log_info(const char * fmt, ...)
{
    va_list args;

    if(FBE_TRACE_LEVEL_INFO <= neit_trace_level)  {
        va_start(args, fmt);
        fbe_trace_report(FBE_COMPONENT_TYPE_PACKAGE,
                        FBE_PACKAGE_ID_NEIT,
                        FBE_TRACE_LEVEL_INFO,
                        0, /* message_id */
                        fmt, 
                        args);
        va_end(args);
    }
}
static fbe_bool_t neit_package_control_entry_disabled = FBE_FALSE;

fbe_status_t
neit_package_send_control_packet(fbe_packet_t * packet)
{
    /* If we shut the door, then deny any new requests.
     */
    if (neit_package_control_entry_disabled) {
        neit_package_log_info("%s: packet: %p denied since neit is disabled for unload.\n", __FUNCTION__, packet);
        fbe_transport_set_status(packet, FBE_STATUS_FAILED, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    return fbe_service_manager_send_control_packet(packet);
}

fbe_status_t 
neit_package_get_control_entry(fbe_service_control_entry_t * service_control_entry)
{
    *service_control_entry = neit_package_send_control_packet;
    return FBE_STATUS_OK;
}

fbe_status_t 
neit_package_disable(void)
{
    neit_package_control_entry_disabled = FBE_TRUE;
    return FBE_STATUS_OK;
}

static fbe_status_t neit_package_init_libraries(void)
{
    fbe_status_t status;

    status = neit_package_init_psl();
    if (status != FBE_STATUS_OK)
    {
        neit_package_log_error("%s: neit_package_init_psl failed\n", __FUNCTION__, status);
        return status;
    }

    status = fbe_xor_library_init();

    if (status != FBE_STATUS_OK)
    {
        neit_package_log_error("%s: fbe_xor_library_init failed %d\n", __FUNCTION__, status);
        return status;
    }
    return FBE_STATUS_OK;
}

static fbe_status_t neit_package_destroy_libraries(void)
{
    fbe_status_t status;
    status = fbe_xor_library_destroy();

    if (status != FBE_STATUS_OK)
    {
        neit_package_log_error("%s: fbe_xor_library_destroy failed %d\n", __FUNCTION__, status);
        return status;
    }
    return FBE_STATUS_OK;
}


static CMM_POOL_ID              cmm_control_pool_id = 0;
static CMM_CLIENT_ID            cmm_client_id = 0;

static fbe_status_t fbe_neit_memory_init(void)
{
   /* Register usage with the available pools in the CMM 
    * and get a small seed pool for use by NEIT. 
    */
   cmm_control_pool_id = cmmGetLongTermControlPoolID();
   cmmRegisterClient(cmm_control_pool_id, 
                        NULL, 
                        0,  /* Minimum size allocation */  
                        CMM_MAXIMUM_MEMORY,   
                        "FBE NEIT memory", 
                        &cmm_client_id);

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_neit_memory_destroy(void)
{
    cmmDeregisterClient(cmm_client_id);
    return FBE_STATUS_OK;
}

static void * fbe_neit_memory_allocate(fbe_u32_t allocation_size_in_bytes)
{
    void * ptr = NULL;
    CMM_ERROR cmm_error;

    cmm_error = cmmGetMemoryVA(cmm_client_id, allocation_size_in_bytes, &ptr);
    if (cmm_error != CMM_STATUS_SUCCESS)
    {

        neit_package_log_error("%s entry 1 cmmGetMemoryVA fail %X\n", __FUNCTION__, cmm_error);
	}
    return ptr;
}

static void fbe_neit_memory_release(void * ptr)
{
    cmmFreeMemoryVA(cmm_client_id, ptr);
    return;
}

fbe_status_t 
neit_package_init_get_board_object_id(fbe_object_id_t *board_id)
{
    fbe_status_t                  		status = FBE_STATUS_GENERIC_FAILURE;
    fbe_topology_control_get_board_t 	board_info;
    fbe_packet_t *                  	packet_p = NULL;
    fbe_payload_ex_t *             		payload = NULL;
    fbe_payload_control_operation_t *	control_operation = NULL;
    fbe_payload_control_status_t    	payload_control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    
    packet_p = fbe_allocate_nonpaged_pool_with_tag ( sizeof (fbe_packet_t), 'pfbe');
    if (packet_p == NULL) {
        neit_package_log_error("%s:can't allocate packet\n",__FUNCTION__);

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
            neit_package_log_error("%s:failed to send control packet\n",__FUNCTION__);
        
    }

    fbe_transport_wait_completion(packet_p);

    fbe_payload_control_get_status(control_operation, &payload_control_status);
    if(payload_control_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        neit_package_log_error("%s:bad return status\n",__FUNCTION__);
    }

    status  = fbe_transport_get_status_code(packet_p);
    if(status != FBE_STATUS_OK){
        neit_package_log_error("%s:bad return status\n",__FUNCTION__);
    }

    fbe_payload_ex_release_control_operation(payload, control_operation);
    fbe_transport_destroy_packet(packet_p);
    fbe_release_nonpaged_pool(packet_p);

    *board_id = board_info.board_object_id;

    return status;

}

fbe_status_t neit_package_init_psl(void)
{
    fbe_status_t                  	status = FBE_STATUS_GENERIC_FAILURE;
    fbe_board_mgmt_platform_info_t 	platform_info;
    fbe_packet_t *                  	packet_p = NULL;
    fbe_payload_ex_t *             	payload = NULL;
    fbe_payload_control_operation_t *	control_operation = NULL;
    fbe_payload_control_status_t    	payload_control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_object_id_t 			board_object_id;

    neit_package_init_get_board_object_id(&board_object_id);

    /* The memory service is not available yet */
    packet_p = fbe_allocate_nonpaged_pool_with_tag ( sizeof (fbe_packet_t), 'pfbe');

    if (packet_p == NULL) {
        neit_package_log_error("%s:can't allocate packet\n",__FUNCTION__);

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
            neit_package_log_error("%s:failed to send control packet\n",__FUNCTION__);
        
    }

    fbe_transport_wait_completion(packet_p);

    fbe_payload_control_get_status(control_operation, &payload_control_status);
    if(payload_control_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        neit_package_log_error("%s:bad return status\n",__FUNCTION__);
    }

    status  = fbe_transport_get_status_code(packet_p);
    if(status != FBE_STATUS_OK){
        neit_package_log_error("%s:bad return status\n",__FUNCTION__);
    }

    fbe_payload_ex_release_control_operation(payload, control_operation);
    fbe_transport_destroy_packet(packet_p);
    fbe_release_nonpaged_pool(packet_p);

    fbe_private_space_layout_initialize_library(platform_info.hw_platform_info.platformType);

    return FBE_STATUS_OK;
}

