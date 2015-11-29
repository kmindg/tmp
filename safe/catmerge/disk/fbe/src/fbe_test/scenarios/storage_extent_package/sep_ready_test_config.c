#include "mut.h"
#include "fbe_test_package_config.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_sim_server.h"
#include "sep_utils.h"
#include "sep_tests.h"
#include "pp_utils.h"
#include "fbe_test.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_registry.h"
#include "fbe_test_esp_utils.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_database_interface.h"


/*!*******************************************************************
 * @def SEP_LOAD_MAX_CONFIG_WAIT_MSEC
 *********************************************************************
 * @brief Max time we will wait for config service to come ready.
 *
 *********************************************************************/
#define SEP_LOAD_MAX_CONFIG_WAIT_MSEC 30000
#define SEP_LOAD_DEFAULT_LOCAL_EMV_REGISTRY 4 /*default local emv registry value*/

#define EspBootFromFlashRegPath    "\\SYSTEM\\CurrentControlSet\\Services\\"

static fbe_status_t sep_config_load_esp_and_sep_packages_completion(fbe_packet_t * packet, fbe_packet_completion_context_t completion_context);

void sep_config_00(void)
{
#define NUMBER_OF_OBJECTS 5

    fbe_status_t                        status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_board_info_t         board_info;
    fbe_terminator_sas_port_info_t      sas_port;
    fbe_terminator_sas_encl_info_t      sas_encl;
    fbe_terminator_sas_drive_info_t     sas_drive;

    fbe_terminator_api_device_handle_t  port0_handle;
    fbe_terminator_api_device_handle_t  encl0_0_handle;
    fbe_terminator_api_device_handle_t  drive_handle;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: using new creation API and Terminator Class Management\n", __FUNCTION__);
    mut_printf(MUT_LOG_LOW, "=== configuring terminator ===\n");
    /*before loading the physical package we initialize the terminator*/
    status = fbe_api_terminator_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_DEFIANT_M1_HW_TYPE;
    status  = fbe_api_terminator_insert_board (&board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a port*/
    sas_port.sas_address = (fbe_u64_t)0x0000000000000001;
    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    sas_port.io_port_number = 1;
    sas_port.portal_number = 2;
    sas_port.backend_number = 0;
    status  = fbe_api_terminator_insert_sas_port (&sas_port, &port0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert an enclosure to port 0*/
    sas_encl.backend_number = 0;
    sas_encl.encl_number = 0;
    sas_encl.uid[0] = 1; // some unique ID.
    sas_encl.sas_address = (fbe_u64_t)0x0000000000000002;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    status  = fbe_api_terminator_insert_sas_enclosure (port0_handle, &sas_encl, &encl0_0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a sas_drive to port 0 encl 0 slot 0*/
    sas_drive.backend_number = 0;
    sas_drive.encl_number = 0;
    sas_drive.sas_address = (fbe_u64_t)0x987654321;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.capacity = TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY;
    sas_drive.block_size = 520;

    status  = fbe_api_terminator_insert_sas_drive (encl0_0_handle, 0, &sas_drive, &drive_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a sas_drive to port 0 encl 0 slot 1*/
    sas_drive.backend_number = 0;
    sas_drive.encl_number = 0;
    sas_drive.sas_address = (fbe_u64_t)0x987654322;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.capacity = TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY;
    sas_drive.block_size = 520;
    status  = fbe_api_terminator_insert_sas_drive (encl0_0_handle, 1, &sas_drive, &drive_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Init fbe api on server */
    mut_printf(MUT_LOG_LOW, "=== starting Server FBE_API ===\n");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== starting physical package===\n");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== set Physical Package entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== starting system discovery ===\n");
    /* wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(NUMBER_OF_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    /* Load Storage Extent Package */
	sep_config_load_sep();

    return;

#undef NUMBER_OF_OBJECTS
}

void sep_config_01(void)
{
#define NUMBER_OF_OBJECTS 7

    fbe_terminator_board_info_t     board_info;
    fbe_terminator_sas_port_info_t  sas_port_info;
    fbe_terminator_sas_encl_info_t  sas_encl_info;
    fbe_terminator_sas_drive_info_t sas_drive_info;

    fbe_terminator_api_device_handle_t  port_handle;
    fbe_terminator_api_device_handle_t  encl_handle;
    fbe_terminator_api_device_handle_t  drive_handle;

    fbe_status_t status;
    fbe_u32_t i;
    
    mut_printf(MUT_LOG_LOW, "=== configuring terminator ===\n");
    status = fbe_api_terminator_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
   
    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_DEFIANT_M1_HW_TYPE;
    status  = fbe_api_terminator_insert_board (&board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Insert port (0). */
    sas_port_info.sas_address = (fbe_u64_t)0x0000000000000001;
    sas_port_info.port_type = FBE_PORT_TYPE_SAS_PMC;
    sas_port_info.io_port_number = 1;
    sas_port_info.portal_number = 2;
    sas_port_info.backend_number = 0;
    status  = fbe_api_terminator_insert_sas_port(&sas_port_info, &port_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Insert an enclosure (0,0) . */
    sas_encl_info.backend_number = 0;
    sas_encl_info.encl_number = 0;
    sas_encl_info.uid[0] = 1; // some unique ID.
    sas_encl_info.sas_address = (fbe_u64_t)0x0000000000000002;
    sas_encl_info.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    status  = fbe_api_terminator_insert_sas_enclosure(port_handle, &sas_encl_info, &encl_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Insert a SAS drives (0,0,0..2). */
    sas_drive_info.backend_number = 0;
    sas_drive_info.encl_number = 0;
    sas_drive_info.sas_address = (fbe_u64_t)0x987654321;
    sas_drive_info.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive_info.capacity = TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY;
    sas_drive_info.block_size = 520;

    for ( i = 0; i < 4; ++i )
    {
        status  = fbe_api_terminator_insert_sas_drive (encl_handle, i, &sas_drive_info, &drive_handle);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        ++sas_drive_info.sas_address;
    }

    /* Init fbe api on server */
    mut_printf(MUT_LOG_LOW, "=== starting Server FBE_API ===\n");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== starting physical package===\n");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== starting system discovery ===\n");
    /* wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(NUMBER_OF_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    /* Load Storage Extent Package */
    sep_config_load_sep();
    return;

#undef NUMBER_OF_OBJECTS
}

void sep_config_load_packages(fbe_packet_t *packet, fbe_semaphore_t *sem, 
                              fbe_sep_package_load_params_t *sep_params, const fbe_test_packages_t *load_packages)
{
    fbe_status_t status;
    fbe_u32_t   default_local_emv = SEP_LOAD_DEFAULT_LOCAL_EMV_REGISTRY;
    fbe_u32_t   bootFromFlash = FBE_FALSE;   


    sep_config_fill_load_params(sep_params);

    fbe_test_util_create_registry_file();

    

    /*If the SP is booting, we should create the local emv registry for it*/
    status = fbe_test_esp_registry_write(K10DriverConfigRegPath,
                                    K10EXPECTEDMEMORYVALUEKEY, 
                                    FBE_REGISTRY_VALUE_DWORD,
                                    &default_local_emv,
                                    sizeof(fbe_u32_t));
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sep_config_load_packages: set lemv on SPA failed");

    /* ESP and SEP use different registry path for the BootFlashMode key
     * Set BOOT_FROM_FLASH_KEY to FALSE here to avoid ESP boot in BootFlash mode
     */
    status = fbe_test_esp_registry_write(EspBootFromFlashRegPath,
                                         BOOT_FROM_FLASH_KEY, 
                                         FBE_REGISTRY_VALUE_DWORD,
                                         &bootFromFlash,
                                         sizeof(fbe_u32_t));
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sep_config_load_packages: set ESP boot flash mode failed");


    mut_printf(MUT_LOG_LOW, "%s === starting Storage Extent Package(SEP) ===", __FUNCTION__);


    mut_printf(MUT_LOG_LOW, "%s: Semaphore=0x%p", __FUNCTION__, sem);
    mut_printf(MUT_LOG_LOW, "%s: Packet=0x%p", __FUNCTION__, packet);

    status = fbe_api_common_send_control_packet_to_service_asynch(packet, 
                                                           FBE_SIM_SERVER_CONTROL_CODE_LOAD_PACKAGE,
                                                           sep_params, 
                                                           sizeof(fbe_sep_package_load_params_t),
                                                           FBE_SERVICE_ID_SIM_SERVER,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           sep_config_load_esp_and_sep_packages_completion,
                                                           sem, 
                                                           FBE_PACKAGE_ID_SEP_0);
    if (status == FBE_STATUS_PENDING) {
       status = FBE_STATUS_OK;
    }

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


    if (load_packages->esp == FBE_TRUE)
    {
        mut_printf(MUT_LOG_LOW, "%s === starting Environmental Services Package(ESP) ===", __FUNCTION__);
        status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_ESP);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        is_esp_loaded = FBE_TRUE;
    }

    return;
}

void sep_config_load_esp_and_sep_packages(fbe_packet_t *packet, fbe_semaphore_t *sem, fbe_sep_package_load_params_t *sep_params)
{
    fbe_test_packages_t load_packages;
    fbe_zero_memory(&load_packages, sizeof(fbe_test_packages_t));

    load_packages.esp = FBE_TRUE;

    sep_config_load_packages(packet,sem,sep_params,&load_packages);

    return;
}

void sep_config_load_sep_packages(fbe_packet_t *packet, fbe_semaphore_t *sem, fbe_sep_package_load_params_t *sep_params)
{
    fbe_test_packages_t load_packages;
    fbe_zero_memory(&load_packages, sizeof(fbe_test_packages_t));

    sep_config_load_packages(packet,sem,sep_params,&load_packages);

    return;
}

void sep_config_load_neit_package(void)
{
    fbe_status_t status;
    fbe_neit_package_load_params_t neit_params;
    neit_config_fill_load_params(&neit_params);

    mut_printf(MUT_LOG_LOW, "%s: loading neit package", __FUNCTION__);
    status = fbe_api_sim_server_load_package_params(FBE_PACKAGE_ID_NEIT, &neit_params, sizeof(fbe_neit_package_load_params_t));
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;
}

static fbe_status_t 
sep_config_load_esp_and_sep_packages_completion(fbe_packet_t * packet, fbe_packet_completion_context_t completion_context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t *)completion_context;

    mut_printf(MUT_LOG_LOW, "%s: Semaphore=0x%p", __FUNCTION__, sem);
    mut_printf(MUT_LOG_LOW, "%s: Packet=0x%p", __FUNCTION__, packet);
    fbe_semaphore_release(sem, 0, 1, FALSE);
    return FBE_STATUS_OK;
}

void sep_config_set_neit_trace_flags(void)
{
    fbe_u32_t trace_level;
    if (fbe_test_sep_util_get_cmd_option_int("-rdgen_trace_level", &trace_level))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "set rdgen trace level to %d", trace_level);
        fbe_test_sep_util_set_service_trace_flags(FBE_TRACE_TYPE_SERVICE, FBE_SERVICE_ID_RDGEN,
                                                  FBE_PACKAGE_ID_NEIT, trace_level);
    }
    if (fbe_test_sep_util_get_cmd_option_int("-lei_trace_level", &trace_level))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "set logical error injection trace level to %d", trace_level);
        fbe_test_sep_util_set_service_trace_flags(FBE_TRACE_TYPE_SERVICE, FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION,
                                                  FBE_PACKAGE_ID_NEIT, trace_level);
    }
    if (fbe_test_sep_util_get_cmd_option_int("-pei_trace_level", &trace_level))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "set protocol error injection trace level to %d", trace_level);
        fbe_test_sep_util_set_service_trace_flags(FBE_TRACE_TYPE_SERVICE, FBE_SERVICE_ID_PROTOCOL_ERROR_INJECTION,
                                                  FBE_PACKAGE_ID_NEIT, trace_level);
    }
}

void sep_config_load_setup_package_entries(const fbe_test_packages_t *set_packages)
{
    fbe_status_t status;

    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (set_packages->esp == FBE_TRUE)
    {
        status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_ESP); 
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        is_esp_loaded = FBE_TRUE;
    }

    /* Set the trace limits so we stop the system when an
     * error or critical error is hit in the sep and neit packages. 
     * The test will fail anyway at the end so it is easier to debug 
     * if we stop as soon as an error is hit.
     */ 
    fbe_test_sep_util_enable_trace_limits(FBE_FALSE);
    fbe_test_pp_util_enable_trace_limits();
    sep_config_set_neit_trace_flags();

    /* Setup some default I/O flags, which includes checking to see if they were 
     * included on the command line. 
     */
    fbe_test_sep_util_set_default_terminator_and_rba_flags();
    fbe_test_sep_util_set_default_io_flags();
    return;
}

void sep_config_load_esp_sep_and_neit_setup_package_entries(void)
{
    fbe_test_packages_t set_packages;
    fbe_zero_memory(&set_packages, sizeof(fbe_test_packages_t));

    set_packages.esp = FBE_TRUE;

    sep_config_load_setup_package_entries(&set_packages);
    return;
}

void sep_config_load_sep_and_neit_setup_package_entries(void)
{
    fbe_test_packages_t set_packages;
    fbe_zero_memory(&set_packages, sizeof(fbe_test_packages_t));

    sep_config_load_setup_package_entries(&set_packages);
    return;
}

void sep_config_load_package_params(fbe_sep_package_load_params_t *sep_params_p,
                                         fbe_neit_package_load_params_t *neit_params_p, const fbe_test_packages_t *load_packages)
{
    fbe_status_t status;
    fbe_u32_t length;
    fbe_u32_t   default_local_emv = SEP_LOAD_DEFAULT_LOCAL_EMV_REGISTRY;
    fbe_u32_t   bootFromFlash = FBE_FALSE;   
    

    fbe_test_mark_sp_up(fbe_test_conn_target_to_sp(fbe_api_sim_transport_get_target_server()));
    
    fbe_test_sep_util_set_terminator_drive_debug_flags(0 /* no default */);

    status = fbe_test_util_create_registry_file();
    if (status != FBE_STATUS_OK) {
        mut_printf(MUT_LOG_TEST_STATUS, "Create of registry failed with status: 0x%x", status);
    }

    /*If one of the SP is first boot, we should create the local emv registry for it*/
    /*If the SP is booting, we should create the local emv registry for it*/
    status = fbe_test_esp_registry_write(K10DriverConfigRegPath,
                                    K10EXPECTEDMEMORYVALUEKEY, 
                                    FBE_REGISTRY_VALUE_DWORD,
                                    &default_local_emv,
                                    sizeof(fbe_u32_t));
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sep_config_load_sep_and_neit_params: set lemv on SPA failed");

    /* ESP and SEP use different registry path for the BootFlashMode key
     * Set BOOT_FROM_FLASH_KEY to FALSE here to avoid ESP boot in BootFlash mode
     */
    status = fbe_test_esp_registry_write(EspBootFromFlashRegPath,
                                         BOOT_FROM_FLASH_KEY, 
                                         FBE_REGISTRY_VALUE_DWORD,
                                         &bootFromFlash,
                                         sizeof(fbe_u32_t));
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sep_config_load_sep_and_neit_params: set ESP boot flash mode failed");
    

    mut_printf(MUT_LOG_LOW, "%s === starting Storage Extent Package(SEP) ===", __FUNCTION__);

    if (sep_params_p == NULL)
    {
        length = 0;
    }
    else
    {
        length = sizeof(fbe_sep_package_load_params_t);
    }
    status = fbe_api_sim_server_load_package_params(FBE_PACKAGE_ID_SEP_0, sep_params_p, length);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "%s: load neit package", __FUNCTION__);
    if (neit_params_p == NULL)
    {
        length = 0;
    }
    else
    {
        length = sizeof(fbe_neit_package_load_params_t);
    }
    status = fbe_api_sim_server_load_package_params(FBE_PACKAGE_ID_NEIT, neit_params_p, length);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


    if (load_packages->esp == FBE_TRUE)
    {
        mut_printf(MUT_LOG_LOW, "%s === starting Environmental Services Package(ESP) ===", __FUNCTION__); 
        status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_ESP);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        is_esp_loaded = FBE_TRUE;
    }
    if (load_packages->esp == FBE_TRUE)
    {
        sep_config_load_esp_sep_and_neit_setup_package_entries();
    }
    else
    {
        sep_config_load_sep_and_neit_setup_package_entries();
    }

	status = fbe_test_sep_util_wait_for_database_service(SEP_LOAD_MAX_CONFIG_WAIT_MSEC);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}

void sep_config_load_esp_sep_and_neit_params(fbe_sep_package_load_params_t *sep_params_p,
                                         fbe_neit_package_load_params_t *neit_params_p)
{
    fbe_test_packages_t load_packages;
    fbe_zero_memory(&load_packages, sizeof(fbe_test_packages_t));

    load_packages.esp = FBE_TRUE;

    sep_config_load_package_params(sep_params_p, neit_params_p, &load_packages);
    return;
}

void sep_config_load_sep_and_neit_params(fbe_sep_package_load_params_t *sep_params_p,
                                         fbe_neit_package_load_params_t *neit_params_p)
{
    fbe_test_packages_t load_packages;
    fbe_zero_memory(&load_packages, sizeof(fbe_test_packages_t));

    sep_config_load_package_params(sep_params_p, neit_params_p, &load_packages);
    return;
}

void sep_config_fill_load_params(fbe_sep_package_load_params_t *sep_params_p)
{
    fbe_u32_t cmd_line_flags = 0;
    fbe_u32_t index;
    fbe_zero_memory(sep_params_p, sizeof(fbe_sep_package_load_params_t));

    if (fbe_test_sep_util_get_cmd_option_int("-fbe_default_trace_level", &cmd_line_flags))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "SEP using default trace level of: 0x%x", cmd_line_flags);
        sep_params_p->default_trace_level = cmd_line_flags;
    }
    else
    {
        sep_params_p->default_trace_level = FBE_TRACE_LEVEL_INFO;
    }

    if (fbe_test_sep_util_get_cmd_option_int("-raid_library_default_debug_flags", &cmd_line_flags))
    {
        sep_params_p->raid_library_debug_flags = cmd_line_flags;
        mut_printf(MUT_LOG_TEST_STATUS, "using raid library default debug flags of: 0x%x", sep_params_p->raid_library_debug_flags);
    }
    else
    {
        sep_params_p->raid_library_debug_flags = FBE_RAID_LIBRARY_DEBUG_FLAG_NONE;
    }

    if (fbe_test_sep_util_get_cmd_option("-stop_sep_neit_on_error"))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "-stop_sep_neit_on_error set.  Enable error limits. ");
        sep_params_p->error_limit.action = FBE_TRACE_ERROR_LIMIT_ACTION_STOP_SYSTEM;
        sep_params_p->error_limit.num_errors = 1;
        sep_params_p->error_limit.trace_level = FBE_TRACE_LEVEL_ERROR;
        sep_params_p->cerr_limit.action = FBE_TRACE_ERROR_LIMIT_ACTION_STOP_SYSTEM;
        sep_params_p->cerr_limit.num_errors = 1;
        sep_params_p->cerr_limit.trace_level = FBE_TRACE_LEVEL_CRITICAL_ERROR;
    }
    else
    {
        sep_params_p->error_limit.action = FBE_TRACE_ERROR_LIMIT_ACTION_INVALID;
        sep_params_p->error_limit.num_errors = 0;
        sep_params_p->error_limit.trace_level = FBE_TRACE_LEVEL_INVALID;
        sep_params_p->cerr_limit.action = FBE_TRACE_ERROR_LIMIT_ACTION_INVALID;
        sep_params_p->cerr_limit.num_errors = 1;
        sep_params_p->cerr_limit.trace_level = FBE_TRACE_LEVEL_INVALID; 
    }

    if (fbe_test_sep_util_get_cmd_option_int("-raid_group_debug_flags", &cmd_line_flags))
    {
        sep_params_p->raid_group_debug_flags = cmd_line_flags;
        mut_printf(MUT_LOG_TEST_STATUS, "using raid group debug flags of: 0x%x", sep_params_p->raid_group_debug_flags);
    }
    else
    {
        sep_params_p->raid_group_debug_flags = FBE_RAID_GROUP_DEBUG_FLAG_NONE;
    }

    if (fbe_test_sep_util_get_cmd_option_int("-virtual_drive_debug_flags", &cmd_line_flags))
    {
        sep_params_p->virtual_drive_debug_flags = cmd_line_flags;
        mut_printf(MUT_LOG_TEST_STATUS, "using virtual drive debug flags of: 0x%x", sep_params_p->virtual_drive_debug_flags);
    }
    else
    {
        sep_params_p->virtual_drive_debug_flags = FBE_VIRTUAL_DRIVE_DEBUG_FLAG_DEFAULT;
    }

    if (fbe_test_sep_util_get_cmd_option_int("-pvd_class_debug_flags", &cmd_line_flags))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "using pvd class debug flags of: 0x%x", cmd_line_flags);
        sep_params_p->pvd_class_debug_flags = cmd_line_flags;
    }
    else
    {
        sep_params_p->pvd_class_debug_flags = FBE_PROVISION_DRIVE_DEBUG_FLAG_NONE;
    }

	if (fbe_test_sep_util_get_cmd_option_int("-lifecycle_state_debug_flags", &cmd_line_flags))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "using lifecycle debug flags of: 0x%x", cmd_line_flags);
        sep_params_p->lifecycle_class_debug_flags = cmd_line_flags;
    }
    else
    {
        sep_params_p->lifecycle_class_debug_flags = FBE_LIFECYCLE_DEBUG_FLAG_NONE;
    }

	if (fbe_test_sep_util_get_cmd_option_int("-fbe_default_trace_level", &cmd_line_flags))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "using default trace level of: 0x%x", cmd_line_flags);
        sep_params_p->default_trace_level = cmd_line_flags;
    }
    else
    {
        sep_params_p->default_trace_level = FBE_TRACE_LEVEL_INFO;
    }
    for (index = 0; index < MAX_SCHEDULER_DEBUG_HOOKS; index++)
    {
        sep_params_p->scheduler_hooks[index].object_id = FBE_OBJECT_ID_INVALID;
    }
    sep_params_p->init_data_params.number_of_main_chunks = 0;
    fbe_test_common_reboot_restore_sep_params(sep_params_p);

    if (fbe_test_should_use_sim_psl()){
        sep_params_p->flags |= FBE_SEP_LOAD_FLAG_USE_SIM_PSL;
    }

    sep_params_p->ext_pool_slice_blocks = fbe_test_get_ext_pool_slice_blocks();
}

void neit_config_fill_load_params(fbe_neit_package_load_params_t *neit_params_p)
{
    fbe_u32_t cmd_line_flags;
    fbe_zero_memory(neit_params_p, sizeof(fbe_neit_package_load_params_t));

    /* The random seed allows us to make tests more random. 
     * We seed the random number generator with the time. 
     * We also allow the user to provide the random seed so that they can 
     * be able to reproduce failures more easily. 
     * This is commented out for now. 
     */
    neit_params_p->random_seed = (fbe_u32_t)fbe_get_time();
    fbe_test_sep_util_get_cmd_option_int("-neit_random_seed", &neit_params_p->random_seed);

    mut_printf(MUT_LOG_TEST_STATUS, "neit random seed is: 0x%x", neit_params_p->random_seed);

    if (fbe_test_sep_util_get_cmd_option("-stop_load_raw_mirror"))
    {
        neit_params_p->load_raw_mirror = FBE_FALSE;
        mut_printf(MUT_LOG_TEST_STATUS, "NEIT stop load raw mirror\n");
    }else
    {
        neit_params_p->load_raw_mirror = FBE_TRUE;
    }

    if (fbe_test_sep_util_get_cmd_option_int("-fbe_default_trace_level", &cmd_line_flags))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "NEIT using default trace level of: 0x%x", cmd_line_flags);
        neit_params_p->default_trace_level = cmd_line_flags;
    }
    else
    {
        neit_params_p->default_trace_level = FBE_TRACE_LEVEL_INFO;
    }
    if (fbe_test_sep_util_get_cmd_option("-stop_sep_neit_on_error"))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "-stop_sep_neit_on_error set.  Enable error limits. ");
        neit_params_p->error_limit.action = FBE_TRACE_ERROR_LIMIT_ACTION_STOP_SYSTEM;
        neit_params_p->error_limit.num_errors = 1;
        neit_params_p->error_limit.trace_level = FBE_TRACE_LEVEL_ERROR;
        neit_params_p->cerr_limit.action = FBE_TRACE_ERROR_LIMIT_ACTION_STOP_SYSTEM;
        neit_params_p->cerr_limit.num_errors = 1;
        neit_params_p->cerr_limit.trace_level = FBE_TRACE_LEVEL_CRITICAL_ERROR;
    }
    else
    {
        neit_params_p->error_limit.action = FBE_TRACE_ERROR_LIMIT_ACTION_INVALID;
        neit_params_p->error_limit.num_errors = 0;
        neit_params_p->error_limit.trace_level = FBE_TRACE_LEVEL_INVALID;
        neit_params_p->cerr_limit.action = FBE_TRACE_ERROR_LIMIT_ACTION_INVALID;
        neit_params_p->cerr_limit.num_errors = 1;
        neit_params_p->cerr_limit.trace_level = FBE_TRACE_LEVEL_INVALID; 
    }
}

void sep_config_load(const fbe_test_packages_t *load_packages)
{
    fbe_sep_package_load_params_t sep_params; 
    fbe_neit_package_load_params_t neit_params; 

    sep_config_fill_load_params(&sep_params);
    neit_config_fill_load_params(&neit_params);
    fbe_test_mark_sp_up(fbe_test_conn_target_to_sp(fbe_api_sim_transport_get_target_server()));
    
    if (load_packages->esp == FBE_TRUE && load_packages->sep == FBE_TRUE && load_packages->neit == FBE_TRUE)
    {
        sep_config_load_esp_sep_and_neit_params(&sep_params, &neit_params); 
    }
    else if (load_packages->sep == FBE_TRUE && load_packages->neit == FBE_TRUE)
    {
        sep_config_load_sep_and_neit_params(&sep_params, &neit_params);
    }

    /* Setup some default I/O flags, which includes checking to see if they were 
     * included on the command line. 
     */
    fbe_test_sep_util_set_default_io_flags();

    /* Setup some default I/O flags, which includes checking to see if they were 
     * included on the command line. 
     */
    fbe_test_sep_util_set_default_terminator_and_rba_flags();
    return;
}

void sep_config_load_esp_sep_and_neit(void)
{
    fbe_test_packages_t load_packages;
    fbe_zero_memory(&load_packages, sizeof(fbe_test_packages_t));

    load_packages.esp = FBE_TRUE;
    load_packages.neit = FBE_TRUE;
    load_packages.sep = FBE_TRUE;
      
    mut_printf(MUT_LOG_TEST_STATUS, "%s Starting", __FUNCTION__);
    sep_config_load(&load_packages);
    mut_printf(MUT_LOG_TEST_STATUS, "%s Complete", __FUNCTION__);
    return;
}

void sep_config_load_sep_and_neit(void)
{
    fbe_test_packages_t load_packages;
    fbe_zero_memory(&load_packages, sizeof(fbe_test_packages_t));

    load_packages.neit = FBE_TRUE;
    load_packages.sep = FBE_TRUE;

    mut_printf(MUT_LOG_TEST_STATUS, "%s Starting", __FUNCTION__);
    sep_config_load(&load_packages);
    mut_printf(MUT_LOG_TEST_STATUS, "%s Complete", __FUNCTION__);
    return;
}

void sep_config_load_sep_and_neit_no_esp(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "%s Starting", __FUNCTION__);
    sep_config_load_sep_and_neit();
    mut_printf(MUT_LOG_TEST_STATUS, "%s Complete", __FUNCTION__);
    return;
}

void sep_config_load_neit(void)
{
    fbe_status_t status;
    mut_printf(MUT_LOG_TEST_STATUS, "%s Starting", __FUNCTION__);

    mut_printf(MUT_LOG_LOW, "%s: load neit package", __FUNCTION__);
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "%s Complete", __FUNCTION__);
    return;
}

void sep_config_load_sep_setup_package_entries(void)
{
	fbe_status_t status;

	//status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_ESP);
    //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	fbe_test_sep_util_enable_trace_limits(FBE_TRUE);
	//fbe_test_sep_util_enable_pkg_trace_limits(FBE_PACKAGE_ID_SEP_0);

    /* Setup some default I/O flags, which includes checking to see if they were 
     * included on the command line. 
     */
    fbe_test_sep_util_set_default_terminator_and_rba_flags();

}

void sep_config_load_sep_and_neit_both_sps(void)
{
    fbe_test_packages_t load_packages;
    fbe_bool_t load_balance = FBE_FALSE;

    fbe_zero_memory(&load_packages, sizeof(fbe_test_packages_t));

    load_packages.neit = FBE_TRUE;
    load_packages.sep = FBE_TRUE;

    sep_config_load_packages_both_sps(&load_packages,load_balance);

    return;
}

void sep_config_load_esp_sep_and_neit_both_sps(void)
{
    fbe_test_packages_t load_packages;
    fbe_bool_t load_balance = FBE_FALSE;

    fbe_zero_memory(&load_packages, sizeof(fbe_test_packages_t));

    load_packages.esp = FBE_TRUE;
    load_packages.neit = FBE_TRUE;
    load_packages.sep = FBE_TRUE;

    sep_config_load_packages_both_sps(&load_packages,load_balance);
    return;
}

void sep_config_load_packages_both_sps(const fbe_test_packages_t *load_packages, fbe_bool_t load_balance)
{

    fbe_status_t status;
    fbe_packet_t * packet[2];
    fbe_semaphore_t sem[2];
    fbe_sep_package_load_params_t *sep_params[2]; 
    fbe_u32_t i;

    mut_printf(MUT_LOG_LOW, "***************%s: BEGIN***************!", __FUNCTION__);

    for(i = 0; i < 2; i++)
    {
       packet[i] = fbe_api_get_contiguous_packet();
       if (packet[i] == NULL) 
       {
          fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__);
          return;
       }
       fbe_semaphore_init(&sem[i], 0, 1);
       sep_params[i] = malloc(sizeof(fbe_sep_package_load_params_t));
    }

    /* Start loading ESP and SEP on SP A.
     */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_test_mark_sp_up(fbe_test_conn_target_to_sp(fbe_api_sim_transport_get_target_server()));
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    mut_printf(MUT_LOG_LOW, "%s: Semaphore=0x%p", __FUNCTION__, &sem[0]);
    mut_printf(MUT_LOG_LOW, "%s: Packet=0x%p", __FUNCTION__, packet[0]);
    if (load_packages->esp == FBE_TRUE)
    {
        sep_config_load_esp_and_sep_packages(packet[0], &sem[0], sep_params[0]);
    }
    else
    {
        sep_config_load_sep_packages(packet[0], &sem[0], sep_params[0]);
    }

    status = fbe_test_sep_util_wait_for_ica_stamp_generation(SEP_LOAD_MAX_CONFIG_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    /* Start loading ESP and SEP on SP B in parallel.
     */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_mark_sp_up(fbe_test_conn_target_to_sp(fbe_api_sim_transport_get_target_server()));
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    mut_printf(MUT_LOG_LOW, "%s: Semaphore=0x%p", __FUNCTION__, &sem[1]);
    mut_printf(MUT_LOG_LOW, "%s: Packet=0x%p", __FUNCTION__, packet[1]);
    if (load_packages->esp == FBE_TRUE)
    {
        sep_config_load_esp_and_sep_packages(packet[1], &sem[1], sep_params[1]);
    }
    else
    {
        sep_config_load_sep_packages(packet[1], &sem[1], sep_params[1]);
    }

    /* Start loading neit on SP A and SP B.
     */
    if (load_packages->neit == FBE_TRUE) 
    {
       status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
       MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
       sep_config_load_neit_package(); 

       status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
       MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
       sep_config_load_neit_package(); 
    }
    
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    if (load_packages->sep == FBE_TRUE && load_packages->neit == FBE_TRUE && load_packages->esp == FBE_TRUE) 
    {
        sep_config_load_esp_sep_and_neit_setup_package_entries(); 
    }
    else if (load_packages->sep == FBE_TRUE && load_packages->neit == FBE_TRUE)
    {
        sep_config_load_sep_and_neit_setup_package_entries();
    }
    else 
    {
       sep_config_load_sep_setup_package_entries();
    }
    status = fbe_test_sep_util_wait_for_database_service(SEP_LOAD_MAX_CONFIG_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    if (load_balance == FBE_TRUE)
    {
        fbe_api_database_set_load_balance(load_balance);
    }
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    if (load_packages->sep == FBE_TRUE && load_packages->neit == FBE_TRUE && load_packages->esp == FBE_TRUE) 
    {
        sep_config_load_esp_sep_and_neit_setup_package_entries(); 
    }
    else if (load_packages->sep == FBE_TRUE && load_packages->neit == FBE_TRUE)
    {
        sep_config_load_sep_and_neit_setup_package_entries();
    }
    else 
    {
       sep_config_load_sep_setup_package_entries();
    }
    status = fbe_test_sep_util_wait_for_database_service(SEP_LOAD_MAX_CONFIG_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    if (load_balance == FBE_TRUE)
    {
        fbe_api_database_set_load_balance(load_balance);
    }

    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_semaphore_wait_ms(&sem[0], 0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_semaphore_wait_ms(&sem[1], 0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    for (i = 0; i < 2; i++) {
       fbe_api_return_contiguous_packet(packet[i]);/*no need to destory !*/
       fbe_semaphore_destroy(&sem[i]);
       free(sep_params[i]);
    }
    
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "***************%s: END***************!", __FUNCTION__);
    return;
}

void sep_config_load_sep_and_neit_load_balance_both_sps_with_sep_params(fbe_sep_package_load_params_t *spa_sep_params_p, 
                                                                        fbe_sep_package_load_params_t *spb_sep_params_p)
{
    fbe_status_t status;
    fbe_packet_t * packet[2];
    fbe_semaphore_t sem[2];
    fbe_u32_t i;
   
    for(i = 0; i < 2; i++){
       packet[i] = fbe_api_get_contiguous_packet();
       if (packet[i] == NULL) {
          fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__);
          return;
       }
       fbe_semaphore_init(&sem[i], 0, 1);
    }
    
    /* Start loading esp and sep on SP A.
     */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_test_mark_sp_up(fbe_test_conn_target_to_sp(fbe_api_sim_transport_get_target_server()));
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    sep_config_load_sep_packages(packet[0], &sem[0], spa_sep_params_p);

    /* Start loading sep and sep on SP B in parallel.
     */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_mark_sp_up(fbe_test_conn_target_to_sp(fbe_api_sim_transport_get_target_server()));
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    sep_config_load_sep_packages(packet[1], &sem[1], spb_sep_params_p);

    /* Start loading neit on SP A.
     */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    sep_config_load_neit_package(); 

    /* Start loading neit on SP B.
     */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    sep_config_load_neit_package(); 


    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    sep_config_load_sep_and_neit_setup_package_entries();
    status = fbe_test_sep_util_wait_for_database_service(SEP_LOAD_MAX_CONFIG_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_api_database_set_load_balance(FBE_TRUE);


    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    sep_config_load_sep_and_neit_setup_package_entries();
    status = fbe_test_sep_util_wait_for_database_service(SEP_LOAD_MAX_CONFIG_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_api_database_set_load_balance(FBE_TRUE);

    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_semaphore_wait_ms(&sem[0], 0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_semaphore_wait_ms(&sem[1], 0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    for (i = 0; i < 2; i++) {
       fbe_api_return_contiguous_packet(packet[i]);/*no need to destory !*/
       fbe_semaphore_destroy(&sem[i]);
    }


    /* We will work primarily with SPA.  The configuration is automatically
     * instantiated on both SPs.  We no longer create the raid groups during 
     * the setup.
     */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
void sep_config_load_sep_and_neit_load_balance_both_sps(void)
{
    fbe_test_packages_t load_packages;
    fbe_bool_t load_balance = FBE_TRUE;

    fbe_zero_memory(&load_packages, sizeof(fbe_test_packages_t));

    load_packages.neit = FBE_TRUE;
    load_packages.sep = FBE_TRUE;

    sep_config_load_packages_both_sps(&load_packages, load_balance);

    return;
}

void sep_config_load_esp_sep_and_neit_load_balance_both_sps(void)
{
    fbe_test_packages_t load_packages;
    fbe_bool_t load_balance = FBE_TRUE;

    fbe_zero_memory(&load_packages, sizeof(fbe_test_packages_t));

    load_packages.esp = FBE_TRUE;
    load_packages.neit = FBE_TRUE;
    load_packages.sep = FBE_TRUE;

    sep_config_load_packages_both_sps(&load_packages, load_balance);

    return;
}

void sep_config_load_sep_package(void)
{
	fbe_status_t status;
    fbe_u32_t   default_local_emv = SEP_LOAD_DEFAULT_LOCAL_EMV_REGISTRY;
    fbe_sep_package_load_params_t sep_params; 

    sep_config_fill_load_params(&sep_params);
    fbe_test_mark_sp_up(fbe_test_conn_target_to_sp(fbe_api_sim_transport_get_target_server()));
        
	fbe_test_util_create_registry_file();

    /*If the SP is booting, we should create the local emv registry for it*/
    status = fbe_test_esp_registry_write(K10DriverConfigRegPath,
                                    K10EXPECTEDMEMORYVALUEKEY, 
                                    FBE_REGISTRY_VALUE_DWORD,
                                    &default_local_emv,
                                    sizeof(fbe_u32_t));
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sep_config_load_sep_package: set lemv on SPA failed");

    mut_printf(MUT_LOG_LOW, "%s === starting Storage Extent Package(SEP) ===", __FUNCTION__);
    status = fbe_api_sim_server_load_package_params(FBE_PACKAGE_ID_SEP_0, &sep_params, sizeof(fbe_sep_package_load_params_t));
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}


void sep_config_load_sep(void)
{
    fbe_bool_t dualsp = FBE_FALSE;
    sep_config_load_sep_with_time_save(dualsp);
}

void sep_config_load_sep_with_time_save(fbe_bool_t dualsp)
{
    fbe_status_t status;
    fbe_test_packages_t load_packages;
    fbe_bool_t load_balance = FBE_FALSE;

    fbe_zero_memory(&load_packages, sizeof(fbe_test_packages_t));

    load_packages.sep = FBE_TRUE;

    mut_printf(MUT_LOG_TEST_STATUS, "%s Starting", __FUNCTION__);
    if (dualsp == FBE_TRUE) {
       sep_config_load_packages_both_sps(&load_packages, load_balance);
    }
    else {
       sep_config_load_sep_package();
       sep_config_load_sep_setup_package_entries();
           status = fbe_test_sep_util_wait_for_database_service(SEP_LOAD_MAX_CONFIG_WAIT_MSEC);
           MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "%s Complete", __FUNCTION__);
    return;
}


void sep_config_load_balance_load_sep(void)
{
    fbe_bool_t dualsp = FBE_FALSE;
    sep_config_load_balance_load_sep_with_time_save(dualsp);
    return;    
}

void sep_config_load_balance_load_sep_with_time_save(fbe_bool_t dualsp)
{
    fbe_status_t status;
    fbe_test_packages_t load_packages;
    fbe_bool_t load_balance = FBE_FALSE;

    fbe_zero_memory(&load_packages, sizeof(fbe_test_packages_t));

    load_packages.sep = FBE_TRUE;

    mut_printf(MUT_LOG_TEST_STATUS, "%s Starting", __FUNCTION__);
    if (dualsp == FBE_TRUE) {
       sep_config_load_packages_both_sps(&load_packages, load_balance);
    }
    else {
       sep_config_load_sep_package();
       sep_config_load_sep_setup_package_entries();
       status = fbe_test_sep_util_wait_for_database_service(SEP_LOAD_MAX_CONFIG_WAIT_MSEC);
       MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
       fbe_api_database_set_load_balance(FBE_TRUE);
    }
    
    mut_printf(MUT_LOG_TEST_STATUS, "%s Complete", __FUNCTION__);
    return;    
}

void sep_config_load_kms(fbe_kms_package_load_params_t *kms_params_p)
{
    fbe_status_t status;
    fbe_u32_t length;

    mut_printf(MUT_LOG_TEST_STATUS, "%s Starting", __FUNCTION__);

    if (kms_params_p == NULL)
    {
        length = 0;
    }
    else
    {
        length = sizeof(fbe_kms_package_load_params_t);
    }
    status = fbe_api_sim_server_load_package_params(FBE_PACKAGE_ID_KMS, kms_params_p, length);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_KMS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "%s Complete", __FUNCTION__);
    return;
}

void sep_config_load_kms_both_sps(fbe_kms_package_load_params_t *kms_params_p)
{
    fbe_status_t status;

    /* Start loading SP A. */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sep_config_load_kms(kms_params_p);

    /* Start loading SP B. */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sep_config_load_kms(kms_params_p);

    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
}

