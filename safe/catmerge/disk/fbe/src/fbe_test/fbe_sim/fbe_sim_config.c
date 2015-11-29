#include "fbe/fbe_winddk.h"
#include "fbe/fbe_physical_package.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_service_manager_interface.h"
#include "fbe_trace.h"
#include "sep_dll.h"
#include "physical_package_dll.h"
#include "esp_dll.h"
#include "fbe_terminator_miniport_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_sim_transport_packet_interface.h"
#include "fbe/fbe_api_sim_transport.h"
#include "fbe_test_package_config.h"
#include "sep_utils.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "simulation_log.h"
#include "mut.h"
#include "pp_utils.h"   
#include "fbe/fbe_api_transport.h"
#include "fbe_private_space_layout.h"
#include "EmcUTIL_DllLoader_Interface.h"

static fbe_sas_enclosure_type_t fbe_sim_get_encl_type(char * value);
static void fbe_sim_populate_terminator(fbe_sas_enclosure_type_t encl_type);

//  Configuration parameters 
#define FBE_SIM_DUMMY_LUNS_PER_RAID_GROUP       1       // number of LUNs in the RG 
#define FBE_SIM_DUMMY_ELEMENT_SIZE              128     // element size 
#define FBE_SIM_DUMMY_RAID_GROUP_CHUNK_SIZE     (8 * FBE_RAID_SECTORS_PER_ELEMENT)
#define FBE_SIM_DUMMY_RAID_GROUP_COUNT          1

                                                    // number of blocks in a raid group bitmap chunk
//  Define the bus-enclosure-disk for the disks in the RG 
#define FBE_SIM_DUMMY_PORT                      0       // port (bus) 0 
#define FBE_SIM_DUMMY_ENCLOSURE                 0       // enclosure 0 
#define FBE_SIM_DUMMY_SLOT_1                    4       // slot for first disk in the RG: VD1  - 0-0-4 
#define FBE_SIM_DUMMY_SLOT_2                    5       // slot for second disk in the RG: VD2 - 0-0-5 
#define FBE_SIM_DUMMY_SLOT_3                    6       // slot for third disk in the RG: VD3  - 0-0-6 

#define FBE_SIM_MAX_PORTS          2
#define FBE_SIM_MAX_ENCLS          8
#define FBE_SIM_MAX_DRIVES         15

//  Set of disks to be used - disks 0-0-4 through 0-0-6
fbe_test_raid_group_disk_set_t fbe_sim_disk_set[] = 
{
    {FBE_SIM_DUMMY_PORT, FBE_SIM_DUMMY_ENCLOSURE, FBE_SIM_DUMMY_SLOT_1},     // 0-0-4
    {FBE_SIM_DUMMY_PORT, FBE_SIM_DUMMY_ENCLOSURE, FBE_SIM_DUMMY_SLOT_2},     // 0-0-5
    {FBE_SIM_DUMMY_PORT, FBE_SIM_DUMMY_ENCLOSURE, FBE_SIM_DUMMY_SLOT_3},     // 0-0-6
    {FBE_U32_MAX,        FBE_U32_MAX,             FBE_U32_MAX         }      // terminator.
};


// This is the array of configurations we will loop over.
 fbe_test_rg_configuration_t fbe_sim_raid_group_config[FBE_SIM_DUMMY_RAID_GROUP_COUNT] = 
{
    /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    {3,         0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            0,          0},
};

static EMCUTIL_DLL_HANDLE simulation_log_dll_handle;

static void fill_sep_with_objects(void)
{    
    fbe_lun_number_t    lun_number = 0;

    // Fill up the disk and LU information to raid group configuration. */
    fbe_test_sep_util_fill_raid_group_disk_set(&fbe_sim_raid_group_config[0], FBE_SIM_DUMMY_RAID_GROUP_COUNT, fbe_sim_disk_set, NULL);
    fbe_test_sep_util_fill_raid_group_lun_configuration(&fbe_sim_raid_group_config[0],
                                                        FBE_SIM_DUMMY_RAID_GROUP_COUNT,
                                                        lun_number,         // first lun number for this RAID group.
                                                        FBE_SIM_DUMMY_LUNS_PER_RAID_GROUP,
                                                        0x7000);

    // Kick of the creation of all the raid group with Logical unit configuration. */
    fbe_test_sep_util_create_raid_group_configuration(&fbe_sim_raid_group_config[0], FBE_SIM_DUMMY_RAID_GROUP_COUNT);
                                    
    return;
    
}

fbe_status_t fbe_sim_config_init(fbe_u32_t sp_mode)
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_cmi_service_get_info_t      get_cmi_info;
    fbe_u32_t  sim_drive_type = 0;
    char *value;
    char *encl_type_str;
    simulation_log_init_func_t simulation_log_init_func = NULL;
    fbe_database_state_t db_state;
    fbe_sas_enclosure_type_t encl_type;

    simulation_log_dll_handle = emcutil_simple_dll_load("simulation_log");
    if(simulation_log_dll_handle == NULL)
    {
        printf("Cannot load simulation_log.dll!\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    simulation_log_init_func = (simulation_log_init_func_t)emcutil_simple_dll_lookup(simulation_log_dll_handle, "simulation_log_init");

    /* init log splitter */
    if(simulation_log_init_func != NULL)
    {
        simulation_log_init_func();
    }

    status = fbe_terminator_api_init_with_sp_id((sp_mode == FBE_SIM_SP_A) ? TERMINATOR_SP_A :TERMINATOR_SP_B);
    
    if (status != FBE_STATUS_OK) {
            printf("%s, error calling fbe_terminator_api_init\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    set_global_timeout(0xfffffffe);/*make sure we always run*/

    value = mut_get_user_option_value("-sim_drive_type");
    if (value != NULL) {
        sim_drive_type = atoi(value);
        status = fbe_terminator_api_set_simulated_drive_type(sim_drive_type);
        if (status != FBE_STATUS_OK) {
                printf("%s, invalid drive_type %d.  Select one of the following:\n\t 0:remote, 1:memory, 2:local\n",
                       __FUNCTION__, sim_drive_type);
                return FBE_STATUS_GENERIC_FAILURE;
        }
        printf("%s, Using sim_drive_type %d\n", __FUNCTION__,sim_drive_type);
    }

    encl_type_str = mut_get_user_option_value("-encl_type");
    
    /* Enable the IO mode.  This will cause the terminator to create
     * disk files as drive objects are created.
     */ 
    status = fbe_terminator_api_set_io_mode(FBE_TERMINATOR_IO_MODE_ENABLED);
    if (status != FBE_STATUS_OK) {
            printf("%s, error calling fbe_terminator_api_set_io_mode\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    encl_type = fbe_sim_get_encl_type(encl_type_str);

    /*configure the terminator manually here, in the future, we need to configure it remotely via the test application*/
    fbe_sim_populate_terminator(encl_type);
    
    fbe_test_load_physical_package();

        /*afer loading the physical packet, we want to wait and make sure at least the first 3 LDOs are ready
        so when SEP comes up, it can find the database drives*/

        /*initialize the fbe api*/
    status  = fbe_api_common_init_sim();
    if (status != FBE_STATUS_OK) {
            printf("%s, error calling fbe_api_common_init_sim\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_test_load_esp();

        /*explicit call to the init function of the simulation portion*/
    fbe_api_set_simulation_io_and_control_entries(FBE_PACKAGE_ID_PHYSICAL, physical_package_io_entry, physical_package_control_entry);
    fbe_api_sim_transport_set_control_entry(physical_package_control_entry, FBE_PACKAGE_ID_PHYSICAL);
    /*explicit call to the init function of the simulation portion*/
    fbe_api_set_simulation_io_and_control_entries(FBE_PACKAGE_ID_ESP, NULL, esp_control_entry);
    fbe_api_sim_transport_set_control_entry(esp_control_entry, FBE_PACKAGE_ID_ESP);

    /* Load Storage Extent Package */
    fbe_test_load_sep(NULL);

        /*explicit call to the init function of the simulation portion*/
    fbe_api_set_simulation_io_and_control_entries(FBE_PACKAGE_ID_SEP_0, sep_io_entry, sep_control_entry);
    fbe_api_sim_transport_set_control_entry(sep_control_entry, FBE_PACKAGE_ID_SEP_0);
    
    /*start the packet server*/
    fbe_api_sim_transport_init_server(sp_mode);

    /*we use this so the server will be able to get notifications registration reuqets and handle them*/
    fbe_api_sim_transport_control_init_server_control();
    fbe_api_sim_transport_register_notifications(FBE_PACKAGE_NOTIFICATION_ID_SEP_0 |
                                                 FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL|
                                                 FBE_PACKAGE_NOTIFICATION_ID_ESP);

    fbe_api_common_init_job_notification();/*Make sure jobs will notify us, we need it for the part that creates the configuration*/

    /*Load NEIT Package*/
    status = fbe_test_load_neit_package(NULL);
    if (status != FBE_STATUS_OK) {
        printf("Failed to load NEIT DLL\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_api_set_simulation_io_and_control_entries(FBE_PACKAGE_ID_NEIT, NULL, neit_package_control_entry);
    fbe_api_sim_transport_set_control_entry(neit_package_control_entry, FBE_PACKAGE_ID_NEIT);

    /*Load KMS */
    status = fbe_test_load_kms_package(NULL);
    if (status != FBE_STATUS_OK) {
        printf("Failed to load KMS DLL\n");
        //return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_api_set_simulation_io_and_control_entries(FBE_PACKAGE_ID_KMS, NULL, kms_control_entry);
    fbe_api_sim_transport_set_control_entry(kms_control_entry, FBE_PACKAGE_ID_KMS);

    /* let SEP finish initialization*/
    status = fbe_api_sim_server_wait_for_database_service();
    if (status != FBE_STATUS_OK) {
        printf("Timed out waiting for config servce\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * also return OK if db is in mantenance mode 
     */
    status = fbe_api_database_get_state(&db_state);
    if (status == FBE_STATUS_OK
        && (db_state == FBE_DATABASE_STATE_SERVICE_MODE
        || db_state == FBE_DATABASE_STATE_CORRUPT)) {
        return FBE_STATUS_OK;
    }

    /*we need to check if we are the passive or active sp and based on that decice if we need to load configurations*/
    status = fbe_api_cmi_service_get_info(&get_cmi_info);
    if (status != FBE_STATUS_OK) {
            printf("Failed to get CMI info, loading config by default\n");
            fill_sep_with_objects();
            return FBE_STATUS_OK;
    }

    if (get_cmi_info.sp_state == FBE_CMI_STATE_ACTIVE) {
            printf("We are the active SP and will load static config\n");
            fill_sep_with_objects();
    }else{
            printf("We are the passive SP no config will be loaded\n");
    }

    
    
    return FBE_STATUS_OK;
}

void fbe_sim_config_destroy(void)
{
    simulation_log_destroy_func_t simulation_log_destroy_func = NULL;

    fbe_api_transport_destroy_server();
    
    fbe_api_common_destroy_sim();
    
    /* This ordering of destroy is necessary.*/
    fbe_test_unload_kms();
        fbe_test_unload_neit_package();
    fbe_test_unload_esp();
    fbe_test_unload_sep();
    fbe_test_unload_physical_package();
    
    /* Destroy the terminator*/
    fbe_terminator_api_destroy();
    simulation_log_destroy_func = (simulation_log_destroy_func_t)emcutil_simple_dll_lookup(simulation_log_dll_handle, "simulation_log_destroy");
    if(simulation_log_destroy_func != NULL)
    {
        simulation_log_destroy_func();
    }
}

static void fbe_sim_populate_terminator(fbe_sas_enclosure_type_t encl_type)
{
    fbe_status_t                        status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_board_info_t         board_info;
    fbe_terminator_sas_port_info_t      sas_port;
    fbe_terminator_sas_encl_info_t      sas_encl;
    fbe_terminator_sas_drive_info_t     sas_drive;
    fbe_u64_t                           sas_addr = (fbe_u64_t)0x987654321;
    fbe_u32_t                           slot = 0;
    
    fbe_terminator_api_device_handle_t  port0_handle;
    fbe_terminator_api_device_handle_t  encl_handle;
    fbe_terminator_api_device_handle_t  drive_handle;

    fbe_terminator_api_device_handle_t  drvsxp0_encl_handle;
    fbe_terminator_api_device_handle_t  drvsxp1_encl_handle;
    fbe_terminator_api_device_handle_t  drvsxp2_encl_handle;
    fbe_terminator_api_device_handle_t  drvsxp3_encl_handle;


    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    //board_info.platform_type = SPID_LIGHTNING_HW_TYPE;
    if((encl_type == FBE_SAS_ENCLOSURE_TYPE_MIRANDA) || (encl_type == FBE_SAS_ENCLOSURE_TYPE_RHEA))
    {
        //board_info.platform_type = SPID_OBERON_1_HW_TYPE;
        board_info.platform_type = SPID_OBERON_S1_HW_TYPE; 
    }
    else if(encl_type == FBE_SAS_ENCLOSURE_TYPE_CALYPSO)
    {
        /* board_info.platform_type = SPID_HYPERION_1_HW_TYPE;
         * SPID_HYPERION_S1_HW_TYPE is not defined yet. 
         * We need to change to SPID_HYPERION_S1_HW_TYPE once it gets defined.
         */
        board_info.platform_type = SPID_OBERON_S1_HW_TYPE;
    }
    else
    {
        board_info.platform_type = SPID_PROMETHEUS_M1_HW_TYPE;
    }
    status = fbe_terminator_api_insert_board (&board_info);
    status = fbe_private_space_layout_initialize_library(board_info.platform_type);
    
    /*insert a port*/
    sas_port.sas_address = (fbe_u64_t)0x0000000000000001;
    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    sas_port.io_port_number = 1;
    sas_port.portal_number = 2;
    sas_port.backend_number = 0;
    status  = fbe_terminator_api_insert_sas_port (&sas_port, &port0_handle);

    switch(encl_type)
    {
    case FBE_SAS_ENCLOSURE_TYPE_VIKING:
        // insert viking to port IOSXP
        sas_encl.backend_number = 0;
        sas_encl.encl_number = 0;
        sas_encl.uid[0] = 1; // some unique ID.
        sas_encl.sas_address = (fbe_u64_t)0x0000000000000002;
        sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP;
        status  = fbe_terminator_api_insert_sas_enclosure (port0_handle, &sas_encl, &encl_handle);
    
        // Insert the 4 Viking DRVSXP into the IOSXP enclosure
        sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP;
    
        // Insert DRVSXP0
        sas_encl.connector_id = 2;
        sas_encl.sas_address = 0x50EE097A00000EE0 + (sas_encl.connector_id<<12) + sas_encl.connector_id; //FBE_TEST_BUILD_LCC_SAS_ADDRESS(0, 0, sas_encl.connector_id);
        status = fbe_terminator_api_insert_sas_enclosure (encl_handle, &sas_encl, &drvsxp0_encl_handle);
    
        // Insert DRVSXP1
        sas_encl.connector_id = 3;
        sas_encl.sas_address = 0x50EE097A00000EE0 + (sas_encl.connector_id<<12) + sas_encl.connector_id; //FBE_TEST_BUILD_LCC_SAS_ADDRESS(0, 0, sas_encl.connector_id);
        status = fbe_terminator_api_insert_sas_enclosure (encl_handle, &sas_encl, &drvsxp1_encl_handle);
    
        // Insert DRVSXP2
        sas_encl.connector_id = 4;
        sas_encl.sas_address = 0x50EE097A00000EE0 + (sas_encl.connector_id<<12) + sas_encl.connector_id; //FBE_TEST_BUILD_LCC_SAS_ADDRESS(0,0, sas_encl.connector_id);
        status = fbe_terminator_api_insert_sas_enclosure (encl_handle, &sas_encl, &drvsxp2_encl_handle);
    
        // Insert DRVSXP3
        sas_encl.connector_id = 5;
        sas_encl.sas_address = 0x50EE097A00000EE0 + (sas_encl.connector_id<<12) + sas_encl.connector_id; //FBE_TEST_BUILD_LCC_SAS_ADDRESS(0,0, sas_encl.connector_id);
        status = fbe_terminator_api_insert_sas_enclosure (encl_handle, &sas_encl, &drvsxp3_encl_handle);
        
        // set the handle to insert drives to first drvsxp
        encl_handle = drvsxp0_encl_handle;
        break;

    case FBE_SAS_ENCLOSURE_TYPE_ANCHO:
        /*insert an enclosure to port 0*/
        sas_encl.backend_number = 0;
        sas_encl.encl_number = 0;
        sas_encl.uid[0] = 1; // some unique ID.
        sas_encl.sas_address = (fbe_u64_t)0x0000000000000002;
        sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_ANCHO;
        status  = fbe_terminator_api_insert_sas_enclosure (port0_handle, &sas_encl, &encl_handle);
        break;

    case FBE_SAS_ENCLOSURE_TYPE_TABASCO:
        /*insert an enclosure to port 0*/
        sas_encl.backend_number = 0;
        sas_encl.encl_number = 0;
        sas_encl.uid[0] = 1; // some unique ID.
        sas_encl.sas_address = (fbe_u64_t)0x0000000000000002;
        sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_TABASCO;
        status  = fbe_terminator_api_insert_sas_enclosure (port0_handle, &sas_encl, &encl_handle);
        break;

    case FBE_SAS_ENCLOSURE_TYPE_CAYENNE: 
        // insert Cayenne IOSXP to port 0
        sas_encl.backend_number = 0;
        sas_encl.encl_number = 0;
        sas_encl.uid[0] = 1; // some unique ID.
        sas_encl.sas_address = (fbe_u64_t)0x0000000000000002;
        sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP;
        status  = fbe_terminator_api_insert_sas_enclosure (port0_handle, &sas_encl, &encl_handle);

        // Insert Cayenne DRVSXP
        sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP;
        sas_encl.connector_id = 4;
        sas_encl.sas_address = 0x50EE097A00000EE0 + (sas_encl.connector_id<<12) + sas_encl.connector_id; //FBE_TEST_BUILD_LCC_SAS_ADDRESS(0, 0, sas_encl.connector_id);
        status = fbe_terminator_api_insert_sas_enclosure (encl_handle, &sas_encl, &drvsxp0_encl_handle);

        // set the handle to insert drives to first drvsxp
        encl_handle = drvsxp0_encl_handle;
        break;

    case FBE_SAS_ENCLOSURE_TYPE_NAGA:
        // insert NAGA IOSXP to port 0
        sas_encl.backend_number = 0;
        sas_encl.encl_number = 0;
        sas_encl.uid[0] = 1; // some unique ID.
        sas_encl.sas_address = (fbe_u64_t)0x0000000000000002;
        sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP;
        status  = fbe_terminator_api_insert_sas_enclosure (port0_handle, &sas_encl, &encl_handle);

        // Insert NAGA DRVSXP0
        sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP;
        sas_encl.connector_id = 4;
        sas_encl.sas_address = 0x50EE097A00000EE0 + (sas_encl.connector_id<<12) + sas_encl.connector_id; //FBE_TEST_BUILD_LCC_SAS_ADDRESS(0, 0, sas_encl.connector_id);
        status = fbe_terminator_api_insert_sas_enclosure (encl_handle, &sas_encl, &drvsxp0_encl_handle);

        // Insert NAGA DRVSXP1
        sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP;
        sas_encl.connector_id = 5;
        sas_encl.sas_address = 0x50EE097A00000EE0 + (sas_encl.connector_id<<12) + sas_encl.connector_id; //FBE_TEST_BUILD_LCC_SAS_ADDRESS(0, 0, sas_encl.connector_id);
        status = fbe_terminator_api_insert_sas_enclosure (encl_handle, &sas_encl, &drvsxp1_encl_handle);

        // set the handle to insert drives to first drvsxp
        encl_handle = drvsxp0_encl_handle;
        break;

    case FBE_SAS_ENCLOSURE_TYPE_MIRANDA:
        /*insert miranda (25 drives oberon) to port 0*/
        sas_encl.backend_number = 0;
        sas_encl.encl_number = 0;
        sas_encl.uid[0] = 1; // some unique ID.
        sas_encl.sas_address = (fbe_u64_t)0x0000000000000002;
        sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_MIRANDA;
        status  = fbe_terminator_api_insert_sas_enclosure (port0_handle, &sas_encl, &encl_handle);
        break;

    case FBE_SAS_ENCLOSURE_TYPE_RHEA:
        /*insert miranda (12 drives oberon) to port 0*/
        sas_encl.backend_number = 0;
        sas_encl.encl_number = 0;
        sas_encl.uid[0] = 1; // some unique ID.
        sas_encl.sas_address = (fbe_u64_t)0x0000000000000002;
        sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_RHEA;
        status  = fbe_terminator_api_insert_sas_enclosure (port0_handle, &sas_encl, &encl_handle);
        break;

    case FBE_SAS_ENCLOSURE_TYPE_CALYPSO:
        /*insert calypso (25 drives hyperion) to port 0*/
        sas_encl.backend_number = 0;
        sas_encl.encl_number = 0;
        sas_encl.uid[0] = 1; // some unique ID.
        sas_encl.sas_address = (fbe_u64_t)0x0000000000000002;
        sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_CALYPSO;
        status  = fbe_terminator_api_insert_sas_enclosure (port0_handle, &sas_encl, &encl_handle);
        break;

    default:
        /*insert Derringer to port 0*/
        sas_encl.backend_number = 0;
        sas_encl.encl_number = 0;
        sas_encl.uid[0] = 1; // some unique ID.
        sas_encl.sas_address = (fbe_u64_t)0x0000000000000002;
        sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_DERRINGER;
        status  = fbe_terminator_api_insert_sas_enclosure (port0_handle, &sas_encl, &encl_handle);
        break;
    }
    
    for (slot = 0; slot < 25; slot++) {
        /*insert a sas_drive to port 0 encl 0 slot 0*/
        sas_drive.backend_number = 0;
        sas_drive.encl_number = 0;
        sas_drive.sas_address = ++sas_addr;
        sas_drive.drive_type = FBE_SAS_DRIVE_SIM_520;
        /* create drives with different capacities*/
        switch(slot)
        {
            case 0:
            case 1:
                sas_drive.capacity = (TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY + (TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY));
                break;
            case 2:
            case 3:
                sas_drive.capacity = (TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY + (TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY * 2));
                break;
            case 4:
                sas_drive.capacity = (TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY + (TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY * 3));
                break;
            case 5:
                sas_drive.capacity = (TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY + (TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY * 4));
                break;
            case 17:
            case 18:
            case 19:
                sas_drive.drive_type = FBE_SAS_DRIVE_SIM_520_FLASH_HE;
                sas_drive.capacity = (TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY + TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY);
                break;
            case 20:
            case 21:
            case 22:
            case 23:
            case 24:
                sas_drive.drive_type = FBE_SAS_DRIVE_COBRA_E_10K;
                sas_drive.capacity = (TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY + TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY);
                break;
            default:
                sas_drive.capacity = (TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY + TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY);
                break;
        }
        sas_drive.block_size = 520;
        fbe_sprintf(sas_drive.drive_serial_number, 17, "%llX", (unsigned long long)sas_drive.sas_address);
        printf("inserting drive with SN:%s to terminator location 0_0_%d\n", sas_drive.drive_serial_number, slot);
        status  = fbe_terminator_api_insert_sas_drive (encl_handle, slot, &sas_drive, &drive_handle);
    }
}

static fbe_sas_enclosure_type_t fbe_sim_get_encl_type(char * value)
{
    fbe_sas_enclosure_type_t encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;

    if (value != NULL) 
    {
        if(strncmp(value, "viking", 7) == 0)
        {
            encl_type = FBE_SAS_ENCLOSURE_TYPE_TABASCO;
        }
        else if(strncmp(value, "ancho", 5) == 0) 
        {
            encl_type = FBE_SAS_ENCLOSURE_TYPE_ANCHO;
        }
        else if(strncmp(value, "tabasco", 7) == 0)
        {
            encl_type = FBE_SAS_ENCLOSURE_TYPE_TABASCO;
        }
        else if(strncmp(value, "cayenne", 7) == 0)
        {
            encl_type = FBE_SAS_ENCLOSURE_TYPE_CAYENNE;
        }
        else if(strncmp(value, "naga", 4) == 0)
        {
            encl_type = FBE_SAS_ENCLOSURE_TYPE_NAGA;
        }
        else if(strncmp(value, "miranda", 7) == 0)
        {
            encl_type = FBE_SAS_ENCLOSURE_TYPE_MIRANDA;
        }
        else if(strncmp(value, "rhea", 4) == 0)
        {
            encl_type = FBE_SAS_ENCLOSURE_TYPE_RHEA;
        }
        else if(strncmp(value, "calypso", 7) == 0)
        {
            encl_type = FBE_SAS_ENCLOSURE_TYPE_CALYPSO;
        }
        else
        {
            encl_type = FBE_SAS_ENCLOSURE_TYPE_DERRINGER;
        }
    }

    if(encl_type == FBE_SAS_ENCLOSURE_TYPE_INVALID) 
    {
        encl_type = FBE_SAS_ENCLOSURE_TYPE_DERRINGER;
        printf("%s, Using encl_type derringer.\n", __FUNCTION__);
    }
    else
    {
        printf("%s, Using encl_type %s.\n", __FUNCTION__, value);
    }

    return encl_type;
}
