#include "simulation/fbe_simulation_public_api.h"
#include "ktrace.h"
#include "sep_tests.h"
#include "physical_package_tests.h"
#include "esp_tests.h"

#include "fbe/fbe_api_sim_transport.h"
#include "fbe/fbe_api_sim_transport_packet_interface.h"
#include "fbe/fbe_api_terminator_interface.h"

#include "fbe/fbe_api_common.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"

#include "simulation_log.h"
#include "system_tests.h"

#include "fbe/fbe_types.h"
#include "sep_tests.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe_database.h"
#include "physical_package_dll.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "sep_utils.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "pp_utils.h"
#include "physical_package_tests.h"


#define	MF_ASSERT(exp)												\
	if (!(exp)) {													\
		KvPrintCopyPeer("Assertion Failed:  (%s) ", #exp);			\
		KvPrintCopyPeer("at %s:%d\n", catmergeFile(__FILE__), __LINE__);			\
		/* KdBreakPoint sets break exception in DBG builds only */  \
		fbe_debug_break();                                             \
	}


static void fbe_sp_sim_func(void * context);


static void suite_log_callback_func(const char *suite_name, const char *log_file_name);

static fbe_thread_t fbe_sp_sim_thread[2];
static fbe_semaphore_t fbe_sp_sim_thread_semaphore[2];
static fbe_thread_t simulated_drive_server_thread;
static EMCPAL_LARGE_INTEGER timeout;

#define CMD_ARGV_MAX_NUM  8
#define FBE_TEST_SPB    1

char* default_sim_exe_str_spa = " fbe_sp_sim.exe SPA";
char* default_sim_exe_str_spb = " fbe_sp_sim.exe SPB";
char start_sp_sim_cmd[2][100] = {"start fbe_sp_sim.exe SPA", "start fbe_sp_sim.exe SPB"};
char debugger_choice[2][100];
char sp_log_choice[32] = "\0";

/*
 * \fn void fbe_test_environment_active()
 * \details
 *
 *   Returns TRUE if/when fbe_test_public_api will start and control
 *   the fbe_sp_test simulation processes
 *
 */
TITANPUBLIC BOOL fbe_test_environment_active() {
    return TRUE;
}


/*
 * \fn void fbe_test_setup()
 * \param sp    - identifies the SP
 * \details
 *
 *   Setup routine launches the fbe_sp_sim process and establishes communication 
 *
 */
TITANPUBLIC void fbe_test_setup(fbe_test_sp_type_t sp) {
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t sp_mode = (sp == fbe_test_SPA) ? 0 : 1;
    fbe_sim_transport_connection_target_t connect_to_sp
                       = (sp == fbe_test_SPA) ? FBE_SIM_SP_A : FBE_SIM_SP_B;

    // configure a generic timeout for use when accessing mutex and semaphores
    timeout.QuadPart = -10000 * 100;

    fbe_semaphore_init(&fbe_sp_sim_thread_semaphore[sp_mode], 0, 1);
    fbe_thread_init(&fbe_sp_sim_thread[sp_mode], "fbe_sp_sim", fbe_sp_sim_func, (void *)sp);

    EmcutilSleep(2000);

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

    /*initialize the simulation side of fbe api*/
    status = fbe_api_common_init_sim();
    
   /*set function entries for commands that would go to the physical package*/
    fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_PHYSICAL,
                                                   fbe_api_sim_transport_send_io_packet,
                                                   fbe_api_sim_transport_send_client_control_packet);

    /*set function entries for commands that would go to the sep package*/
    fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_SEP_0,
                                                   fbe_api_sim_transport_send_io_packet,
                                                   fbe_api_sim_transport_send_client_control_packet);
    /*set function entries for commands that would go to the neit package*/
    fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_NEIT,
                                                   fbe_api_sim_transport_send_io_packet,
                                                   fbe_api_sim_transport_send_client_control_packet);

    fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_ESP,
                                                   fbe_api_sim_transport_send_io_packet,
                                                   fbe_api_sim_transport_send_client_control_packet);

    
    /*need to initialize the client to connect to the server*/
    fbe_api_sim_transport_reset_client(FBE_CLIENT_MODE_DEV_PC);

    /*need to configure transport to connect to the correct SP process*/
    fbe_api_sim_transport_set_target_server(connect_to_sp);



    status = fbe_api_sim_transport_init_client(connect_to_sp, FBE_TRUE);
    if (status != FBE_STATUS_OK) {
        printf("\nCan't connect to FBE API Server, make sure FBE is running !!!\n");
        //DbgBreakPoint();
        exit(-1);
    }

    /*
     * The termiator needs to be initialized before any commands can be sent to it
     */
    status = fbe_api_terminator_init();
    MF_ASSERT(status == FBE_STATUS_OK);
}

/*
 * \fn void fbe_test_cleanup()
 * \param sp    - identifies the SP
 * \details
 *
 *   cleanup routine shuts dow the fbe_sp_sim process
 *
 */
TITANPUBLIC void fbe_test_cleanup(fbe_test_sp_type_t sp) {
    fbe_status_t status;
    EMCPAL_STATUS emcpal_status;
    fbe_u32_t sp_mode = (sp == fbe_test_SPA) ? 0 : 1;

    //status = fbe_api_sim_server_unload_package(FBE_PACKAGE_ID_NEIT);
    //MF_ASSERT(FBE_STATUS_OK == status);

    status = fbe_api_sim_server_unload_package(FBE_PACKAGE_ID_SEP_0);
    MF_ASSERT(FBE_STATUS_OK == status);

    status = fbe_api_sim_server_unload_package(FBE_PACKAGE_ID_PHYSICAL);
    MF_ASSERT(FBE_STATUS_OK == status);

    status = fbe_api_terminator_destroy();
    MF_ASSERT(FBE_STATUS_OK == status);

    /* destroy */
    fbe_api_sim_transport_destroy_client(sp_mode);


    /* stop fbe_sp_sim.exe */
    /*  NOTE: this will kill both fbe_sp_sim processes... */
#ifdef ALAMOSA_WINDOWS_ENV
    system("taskkill /im fbe_sp_sim.exe /f");
#else
    system("killall -TERM fbe_sp_sim.exe");
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */

    /*wait for threads to end*/
    emcpal_status = fbe_semaphore_wait(&fbe_sp_sim_thread_semaphore[sp_mode],
                                       &timeout);

    if (EMCPAL_STATUS_SUCCESS == emcpal_status) {
        fbe_thread_wait(&fbe_sp_sim_thread[sp_mode]);
    } else {
        fbe_api_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                      "%s: sp %d thread (%p) did not exit!\n", __FUNCTION__,
                      (int)sp_mode, fbe_sp_sim_thread[sp_mode].thread);
    }
    fbe_semaphore_destroy(&fbe_sp_sim_thread_semaphore[sp_mode]);
    fbe_thread_destroy(&fbe_sp_sim_thread[sp_mode]);
}


/* Temporary hack - should be removed */
fbe_status_t 
fbe_get_package_id(fbe_package_id_t * package_id)
{
    *package_id = FBE_PACKAGE_ID_PHYSICAL;
    return FBE_STATUS_OK;
}



static void fbe_sp_sim_func(void * context)
{    
    int result;
    char *command;

    fbe_test_sp_type_t which_sp = (fbe_test_sp_type_t)context;
    
    fbe_api_trace(FBE_TRACE_LEVEL_INFO,"%s entry\n", __FUNCTION__);

    command = start_sp_sim_cmd[which_sp];
    
    fbe_api_trace(FBE_TRACE_LEVEL_INFO, "invoking %s\n", command);


    EmcutilSleep(200);

    result = system(command);

    fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s done\n", __FUNCTION__);

    EmcutilSleep(200);
    
    fbe_semaphore_release(&fbe_sp_sim_thread_semaphore[which_sp], 0, 1, FALSE);

    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

static void program_armada_system(fbe_u32_t sp_mode) {
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_board_info_t 	board_info;

    fbe_terminator_sas_port_info_t	sas_port;
    fbe_terminator_api_device_handle_t	port0_handle;    

//    fbe_terminator_sas_port_info_t	sas_port_unc;
//    fbe_terminator_api_device_handle_t	port_unc_handle;

    fbe_terminator_iscsi_port_info_t   iscsi_port;
    fbe_terminator_api_device_handle_t	port_iscsi_fe_handle = 0xFFFF;

    fbe_terminator_fcoe_port_info_t   fcoe_port;
    fbe_terminator_api_device_handle_t	port_fcoe_fe_handle = 0xFFFF;


    fbe_terminator_fc_port_info_t   fc_port_fe;
    fbe_terminator_api_device_handle_t	port_fc_fe_handle = 0xFFFF;

    fbe_cpd_shim_hardware_info_t                hardware_info;

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_DEFIANT_M1_HW_TYPE;
    status  = fbe_api_terminator_insert_board (&board_info);
    MF_ASSERT(FBE_STATUS_OK == status);

    /*insert a port*/
    sas_port.backend_number = 0;
    sas_port.io_port_number = 3;
    sas_port.portal_number = 5;
    sas_port.sas_address = 0x5000097A7800903F;
    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    sas_port.port_role = FBE_CPD_SHIM_PORT_ROLE_BE;
    sas_port.port_connect_class = FBE_CPD_SHIM_CONNECT_CLASS_SAS;
    status  = fbe_api_terminator_insert_sas_port (&sas_port, &port0_handle);
    MF_ASSERT(FBE_STATUS_OK == status);

    fbe_zero_memory(&hardware_info,sizeof(fbe_cpd_shim_hardware_info_t));
    hardware_info.vendor = 0x11F8;
    hardware_info.device = 0x8001;
    hardware_info.bus = 0x34;
    hardware_info.slot = 0;
    hardware_info.function = 0;

    status = fbe_api_terminator_set_hardware_info(port0_handle,&hardware_info);
    MF_ASSERT(status == FBE_STATUS_OK);


    /*insert a FC FE port*/
    fc_port_fe.backend_number = 1;
    fc_port_fe.io_port_number = 5;
    fc_port_fe.portal_number = 0;
    fc_port_fe.diplex_address = 0x5;
    fc_port_fe.port_type = FBE_PORT_TYPE_FC_PMC;
    fc_port_fe.port_role = FBE_CPD_SHIM_PORT_ROLE_FE;
    fc_port_fe.port_connect_class = FBE_CPD_SHIM_CONNECT_CLASS_FC;
    status  = fbe_api_terminator_insert_fc_port(&fc_port_fe, &port_fc_fe_handle);
    MF_ASSERT(FBE_STATUS_OK == status);

    /*insert a iScsi FE port*/
    iscsi_port.backend_number = 2;
    iscsi_port.io_port_number = 6;
    iscsi_port.portal_number = 0;    
    iscsi_port.port_type = FBE_PORT_TYPE_ISCSI;
    iscsi_port.port_role = FBE_CPD_SHIM_PORT_ROLE_FE;
    iscsi_port.port_connect_class = FBE_CPD_SHIM_CONNECT_CLASS_ISCSI;
    status  = fbe_api_terminator_insert_iscsi_port(&iscsi_port, &port_iscsi_fe_handle);
    MF_ASSERT(FBE_STATUS_OK == status);
}

static void program_enclosure(fbe_u32_t sp_mode, fbe_u32_t beIndex, fbe_u32_t encIndex) 
{
    fbe_terminator_api_device_handle_t	port_handle;
    fbe_terminator_sas_encl_info_t	sas_encl;
    fbe_terminator_sas_drive_info_t sas_drive;
    
    fbe_terminator_api_device_handle_t	enc_handle;
    fbe_terminator_api_device_handle_t	drive_handle;
    fbe_u32_t                       slot = 0;
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;


    status = fbe_api_terminator_get_port_handle(beIndex, &port_handle);
    MF_ASSERT(status == FBE_STATUS_OK);

    /*insert an enclosure to port 0*/
    sas_encl.backend_number = beIndex;
    sas_encl.encl_number = encIndex;
    sas_encl.uid[0] = beIndex; // some unique ID.
    sas_encl.sas_address = 0x5000097A79000000 + ((fbe_u64_t)(sas_encl.encl_number) << 16);
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    status  = fbe_api_terminator_insert_sas_enclosure (port_handle, &sas_encl, &enc_handle);
    MF_ASSERT(FBE_STATUS_OK == status);

    for (slot = 0; slot < 12; slot ++) {
	    /*insert a sas_drive to port 0 encl 0 slot */
	    sas_drive.backend_number = beIndex;
	    sas_drive.encl_number = encIndex;
	    sas_drive.capacity = TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY;
	    sas_drive.block_size = 520;
	    sas_drive.sas_address = 0x5000097A78000000 + ((fbe_u64_t)(sas_drive.encl_number) << 16) + slot;
	    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
	    status  = fbe_api_terminator_insert_sas_drive (enc_handle, slot, &sas_drive, &drive_handle);
	    MF_ASSERT(FBE_STATUS_OK == status);
    }
}

static void program_1enclosure_12drives(fbe_u32_t sp_mode) {
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;

    program_armada_system(sp_mode);
    program_enclosure(sp_mode, 0, 0);
}

static void program_2enclosures_24drives(fbe_u32_t sp_mode) 
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    
    program_armada_system(sp_mode);
    program_enclosure(sp_mode, 0, 0);
    program_enclosure(sp_mode, 0, 1);
}

static void program_4enclosures_48drives(fbe_u32_t sp_mode) 
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    
    program_armada_system(sp_mode);
    program_enclosure(sp_mode, 0, 0);
    program_enclosure(sp_mode, 0, 1);
    program_enclosure(sp_mode, 0, 2);
    program_enclosure(sp_mode, 0, 3);
}

/*
 * \fn void fbe_test_initialize_hw_config()
 * \param sp       - identifies the SP
 * \param config   - identifies the scenario to configure
 * \details
 *
 *   cleanup routine shuts dow the fbe_sp_sim process
 *
 */
TITANPUBLIC void fbe_test_initialize_hw_config(fbe_test_sp_type_t sp, fbe_test_sp_hw_config_t config)
{
    fbe_u32_t sp_mode = (sp == fbe_test_SPA) ? 0 : 1;
    fbe_status_t status;

    switch(config) {
        case hw_config_1enclosure_12drives:
            program_1enclosure_12drives(sp_mode);
            break;
        case hw_config_2enclosures_24drives:
            program_2enclosures_24drives(sp_mode);
            break;
        case hw_config_4enclosures_48drives:
            program_4enclosures_48drives(sp_mode);
            break;
        default:
            fbe_debug_break();
            break;
    }


    /*
     * After successfully loading the config into the Terminator, bring up the Physical package
     */
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
	MF_ASSERT(FBE_STATUS_OK == status);

    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
	MF_ASSERT(FBE_STATUS_OK == status);
}


/*
 * \fn void fbe_test_create_lun()
 * \param rgId      - index of the raid group in which the LUN is created
 * \param lunId     - index the LUN to create
 * \param capacity  - the number of blocks capacity of the LUN
 * \details
 *
 *   configure LUN lunID in raid Group rgId with capacity
 *
 */
TITANPUBLIC void fbe_test_create_lun(UINT_32 rgId, UINT_32 lunId, UINT_64 capacity) {
    fbe_status_t                                        status = FBE_STATUS_OK;
    fbe_api_job_service_lun_create_t                    fbe_lun_create_req;
    fbe_job_service_error_type_t                        job_error_code;
    fbe_status_t                                        job_status;


    fbe_lun_create_req.raid_type        = FBE_RAID_GROUP_TYPE_RAID5;
    fbe_lun_create_req.raid_group_id    = (fbe_raid_group_number_t) rgId;
    fbe_lun_create_req.lun_number       = (fbe_lun_number_t) lunId;
    fbe_lun_create_req.capacity         = (fbe_lba_t)capacity;
    fbe_lun_create_req.placement        = FBE_BLOCK_TRANSPORT_BEST_FIT;
    fbe_lun_create_req.ndb_b            = FBE_FALSE;
    fbe_lun_create_req.noinitialverify_b = FBE_FALSE;
    fbe_lun_create_req.addroffset       = FBE_LBA_INVALID; /* set to a valid offset for NDB */
    fbe_lun_create_req.context          = NULL;
    fbe_lun_create_req.job_number       = 0;
    fbe_lun_create_req.user_private     = FBE_FALSE;
    fbe_lun_create_req.wait_ready = FBE_FALSE;
    fbe_lun_create_req.ready_timeout_msec = 20000;

    /* create a Logical Unit using job service fbe api. */
    status = fbe_api_job_service_lun_create(&fbe_lun_create_req);
    MF_ASSERT(status == FBE_STATUS_OK);

    /* wait for raid group creation */
    status = fbe_api_common_wait_for_job(fbe_lun_create_req.job_number,
										 100000,        /*wait maximum of 10 seconds*/
										 &job_error_code,
										 &job_status);

	MF_ASSERT(status != FBE_STATUS_TIMEOUT);
	MF_ASSERT(job_status == FBE_STATUS_OK);
}

static void create_raidgroup(fbe_raid_group_number_t id, fbe_u32_t bus, fbe_u32_t enc, fbe_u32_t firstSlot, fbe_u32_t lastSlot) {
    fbe_api_job_service_raid_group_create_t     fbe_raid_group_create_req;
    fbe_u32_t                                   slot;
    fbe_u32_t                                   index;
    fbe_u32_t                                   driveCount;
    fbe_status_t                                status;
    fbe_job_service_error_type_t                job_error_code;
    fbe_status_t                                job_status;
    fbe_object_id_t                             rg_object_id;

    fbe_zero_memory(&fbe_raid_group_create_req,sizeof(fbe_api_job_service_raid_group_create_t));

    fbe_raid_group_create_req.b_bandwidth                       = 0;
    fbe_raid_group_create_req.capacity                          = FBE_LBA_INVALID; /* not specifying max number of LUNs for test */
    //fbe_raid_group_create_req.explicit_removal                  = 0;
    fbe_raid_group_create_req.power_saving_idle_time_in_seconds = FBE_LUN_DEFAULT_POWER_SAVE_IDLE_TIME;
    fbe_raid_group_create_req.is_private                        = FBE_TRI_FALSE;
    fbe_raid_group_create_req.max_raid_latency_time_is_sec      = FBE_LUN_MAX_LATENCY_TIME_DEFAULT;
    fbe_raid_group_create_req.raid_group_id                     = id;
    fbe_raid_group_create_req.raid_type                         = FBE_RAID_GROUP_TYPE_RAID5;
    fbe_raid_group_create_req.context                           = (void *)NULL;
    fbe_raid_group_create_req.user_private                      = FBE_FALSE;
    fbe_raid_group_create_req.wait_ready                        = FBE_FALSE;
    fbe_raid_group_create_req.ready_timeout_msec                = 20000;

    for(index = 0, slot = firstSlot, driveCount = 0; slot <= lastSlot; index++, slot++) {
        driveCount++;
        fbe_raid_group_create_req.disk_array[index].bus         = bus;
        fbe_raid_group_create_req.disk_array[index].enclosure   = enc;
        fbe_raid_group_create_req.disk_array[index].slot        = slot;
    }

    fbe_raid_group_create_req.drive_count = driveCount;

    /* dispatch raid group creation */
    status = fbe_api_job_service_raid_group_create(&fbe_raid_group_create_req);
    MF_ASSERT(status == FBE_STATUS_OK);

    /* wait for raid group creation */
    status = fbe_api_common_wait_for_job(fbe_raid_group_create_req.job_number,
										 100000,        /*wait maximum of 10 seconds*/
										 &job_error_code,
										 &job_status);

	MF_ASSERT(status != FBE_STATUS_TIMEOUT);
	MF_ASSERT(job_status == FBE_STATUS_OK);

    /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(
            fbe_raid_group_create_req.raid_group_id, &rg_object_id);

    MF_ASSERT(FBE_STATUS_OK == status);
    MF_ASSERT(FBE_OBJECT_ID_INVALID != rg_object_id);

#if 0
    /* Verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);
    MF_ASSERT(status == FBE_STATUS_OK);
#endif
}

static void program_1raidgroup_12drives(void) 
{
    /* The HW config should contain a single viper enclosure with 12 drives */
    /* create a single raid group with all 12 drivers */
    create_raidgroup(0, 0, 0, 0, 11);
}

static void program_4raidgroups_12drives(void) 
{
    /* The HW config should contain a single viper enclosure with 12 drives */
    /* create 4 raid group each containing 3 drivers */
    create_raidgroup(0, 0, 0, 0, 2);
    create_raidgroup(1, 0, 0, 3, 5);
    create_raidgroup(2, 0, 0, 6, 8);
    create_raidgroup(3, 0, 0, 9, 11);
}

static void program_4raidgroups_24drives(void) 
{
    /* The HW config should contain two viper enclosure each with 12 drives */
    /* create 4 raid groups each containing 6 drivers */
    create_raidgroup(0, 0, 0, 0, 5);
    create_raidgroup(1, 0, 0, 6, 11);

    create_raidgroup(2, 0, 1, 0, 5);
    create_raidgroup(3, 0, 1, 6, 11);
}

static void program_8raidgroups_24drives(void) 
{
    /* The HW config should contain two viper enclosure each with 12 drives */
    /* create 8 raid groups each containing 3 drivers */
    create_raidgroup(0, 0, 0, 0, 2);
    create_raidgroup(1, 0, 0, 3, 5);
    create_raidgroup(2, 0, 0, 6, 8);
    create_raidgroup(3, 0, 0, 9, 11);

    create_raidgroup(4, 0, 1, 0, 2);
    create_raidgroup(5, 0, 1, 3, 5);
    create_raidgroup(6, 0, 1, 6, 8);
    create_raidgroup(7, 0, 1, 9, 11);
}


static void program_16raidgroups_48drives(void) 
{
    /* The HW config should contain two viper enclosure each with 12 drives */
    /* create 16 raid groups each containing 3 drivers */
    create_raidgroup(0,  0, 0, 0, 2);
    create_raidgroup(1,  0, 0, 3, 5);
    create_raidgroup(2,  0, 0, 6, 8);
    create_raidgroup(3,  0, 0, 9, 11);
 
    create_raidgroup(4,  0, 1, 0, 2);
    create_raidgroup(5,  0, 1, 3, 5);
    create_raidgroup(6,  0, 1, 6, 8);
    create_raidgroup(7,  0, 1, 9, 11);

    create_raidgroup(8,  0, 2, 0, 2);
    create_raidgroup(9,  0, 2, 3, 5);
    create_raidgroup(10, 0, 2, 6, 8);
    create_raidgroup(11, 0, 2, 9, 11);

    create_raidgroup(12, 0, 3, 0, 2);
    create_raidgroup(13, 0, 3, 3, 5);
    create_raidgroup(14, 0, 3, 6, 8);
    create_raidgroup(15, 0, 3, 9, 11);

}

/*
 * \fn void fbe_test_initialize_logical_config()
 * \param config   - identifies the logical config to configure
 * \details
 *
 *   configure the SEP with the specified logical config
 *
 */
TITANPUBLIC void fbe_test_initialize_logical_config(fbe_test_logical_config_t config)
{
    fbe_status_t status;


    // need to load the SEP package first
	sep_config_load_sep();

    // Question:  Is package FBE_PACKAGE_ID_ESP, FBE_PACKAGE_ID_ADMIN_INTERFACE or FBE_PACKAGE_ID_NEIT required?

    switch(config) {
        case logical_config_1raidgroup_12drives:
            program_1raidgroup_12drives();
            break;
        case logical_config_4raidgroups_12drives:
            program_4raidgroups_12drives();
            break;
        case logical_config_4raidgroups_24drives:
            program_4raidgroups_24drives();
            break;
        case logical_config_8raidgroups_24drives:
            program_8raidgroups_24drives();
            break;
        case logical_config_16raidgroups_48drives:
            program_16raidgroups_48drives();
            break;
        default:
            break;
    }

}
