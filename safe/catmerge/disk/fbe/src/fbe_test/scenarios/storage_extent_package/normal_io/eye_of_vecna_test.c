

/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file eye_of_vecna_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains a test for the FBE Perfstats service
 *
 * @version
 *   04/05/2012:  Created. Ryan Gadsby
 *   06/22/2012:  Added support for testing PDO stats. Darren Insko
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe/fbe_api_rdgen_interface.h"
#include "sep_tests.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_database.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "sep_test_io.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_perfstats_sim.h"
#include "fbe/fbe_api_perfstats_interface.h"
#include "fbe/fbe_perfstats_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "pp_utils.h"
#include "fbe/fbe_random.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/
char * eye_of_vecna_short_desc = "Perfstats service functionality testing";
char * eye_of_vecna_long_desc = "\
\n\
This description is a placeholder stolen from King Minos\n\
STEP 1: Get info about various aspects of hardware information:\n\
        - Discrete CPU count, drive capacity and speed, occupied backend buses,\n\
          number of enclosures, drives per enclosure, etc. \n\
\n\
STEP 2: Configure all raid groups and LUNs.\n\
        - Bind as many LUNs as possible while maintaining the following pattern:\n\
          don't bind anything to bus 0 enclosure 0, and bind an equal number of LUNs to each backend port.\n\
        - All raid groups have 1 LUN each, size defined by stroking factor constant.\n\
\n\
STEP 3: Generate list of IO profiles to run. \n\
        - 256K sequential reads\n\
        - 256K sequential writes\n\
        - 8K random reads\n\
        - 8K random writes\n\
\n\
STEP 4: Run IO profiles.\n\
        - Run to number of LUN objects equal to GCD(ports,cores) at first,\n\
          then increase by factor of 4 until we don't have any more LUNs to run on.\n\
\n\
STEP 5: Compare results to baseline file on machine running fbe_test.\n\
        - If no baseline exists, create a new one and pass the test.\n\
        - If performance for any workload falls outside tolerance, fail the test.\n\
\n\
STEP 6: Write performance logs.\n\
        - Default path is C:\\FBE_Performance_Logs\\\n\
        - Log writing is in append mode, to keep historical data. \n\
Test Specific Switches:\n\
          \n\
\n\
Description last updated: 04/05/2012.\n";

/*!*******************************************************************
 * @def EYE_OF_VECNA_MAXIMUM_PORTS
 *********************************************************************
 * @brief Maximum number of ports we'll search for enclosures on for this test
 *********************************************************************/
#define EYE_OF_VECNA_MAXIMUM_PORTS 36

/*!*******************************************************************
 * @def EYE_OF_VECNA_DRIVES_PER_RAID_GROUP
 *********************************************************************
 * @brief Number of drives used to bind RGs (increase to nearest number
 * divisible by 2 for RAID 6.
 *********************************************************************/
#define EYE_OF_VECNA_DRIVES_PER_RAID_GROUP 5

/*!*******************************************************************
 * @def EYE_OF_VECNA_IO_COUNT
 *********************************************************************
 * @brief Number of IOs to do per LUN in the test
 *********************************************************************/
#define EYE_OF_VECNA_IO_COUNT 25

/*!*******************************************************************
 * @def EYE_OF_VECNA_THREAD_COUNT
 *********************************************************************
 * @brief Number of threads per RDGEN context
 * *********************************************************************/
#define EYE_OF_VECNA_THREAD_COUNT 5

/*!*******************************************************************
 * @def EYE_OF_VECNA_STRIPE_SIZE
 *********************************************************************
 * @brief Stripe size for our RAID groups (default is 256K but I can't find where that's defined)
 * *********************************************************************/
#define EYE_OF_VECNA_STRIPE_SIZE 512

/*!*******************************************************************
 * @def EYE_OF_VECNA_MAX_LUNS_PER_RG_SIM
 *********************************************************************
 * @brief Number of luns to bind per SP in simulation
 *********************************************************************/
#define EYE_OF_VECNA_MAX_LUNS_PER_RG_SIM 2

/*!*******************************************************************
 * @def EYE_OF_VECNA_MAX_LUNS_PER_RG_HARDWARE
 *********************************************************************
 * @brief Number of luns to bind per SP on hardware
 *********************************************************************/
#define EYE_OF_VECNA_MAX_LUNS_PER_RG_HARDWARE 100

/*!*******************************************************************
 * @def EYE_OF_VECNA_MAX_LUNS_TO_TEST_SIM
 *********************************************************************
 * @brief Number of luns to run IO to in simulation
 *********************************************************************/
#define EYE_OF_VECNA_MAX_LUNS_TO_TEST_SIM 24

/*!*******************************************************************
 * @def EYE_OF_VECNA_MAX_LUNS_TO_TEST_HARDWARE
 *********************************************************************
 * @brief Number of luns to run IO to on hardware
 *********************************************************************/
#define EYE_OF_VECNA_MAX_LUNS_TO_TEST_HARDWARE 8192

/*!*******************************************************************
 * @def eye_of_vecna_physical_config_t
 *********************************************************************
 * @brief Holds various hardware details needed to configure the array correctly,
 * specify the correct workloads, and determine what directory to store
 * logs in.
 *********************************************************************/
typedef struct eye_of_vecna_physical_config_e {
    fbe_u32_t cores;
    fbe_u32_t bus_count;
    fbe_u32_t occupied_backend_buses[PERFSTATS_CORES_SUPPORTED];
    fbe_u32_t enclosures_per_bus;
    fbe_u32_t drives_per_enclosure;
    fbe_lba_t drive_capacity;
}eye_of_vecna_physical_config_t ;

/*!*******************************************************************
 * @def eye_of_vecna_logical_config_t
 *********************************************************************
 * @brief Populated after configuring raid groups and luns, this struct
 * holds any relevant data for the logical configuration at any given point
 * in the test. 
 *********************************************************************/
typedef struct eye_of_vecna_logical_config_e{
    fbe_u32_t luns_bound;
    fbe_lba_t lun_capacity;
    fbe_u32_t drives_per_raidgroup;
} eye_of_vecna_logical_config_t;

/*!**************************************************************
 * eye_of_vecna_detect_configuration()
 ****************************************************************
 * @brief
 *  Populates a struct containing the relevant information to create
 *  performance workloads
 * 
 *  This is a single SP test.
 *
 * @param  eye_of_vecna_config_t containing the number of cores,
 * the number of buses (occupied ports), the number of enclosures
 * per bus and number of drives per enclosure, as well as a code to
 * denote the pattern of port occupation.        
 *
 * @return - None
 *
 ***************************************************************/
void eye_of_vecna_detect_configuration(eye_of_vecna_physical_config_t *physical_config){
    fbe_status_t status;
    fbe_board_mgmt_platform_info_t platform_info;
    fbe_device_physical_location_t enc_location;
    fbe_physical_drive_information_t drive_info;

    fbe_object_id_t *pvd_list;
    fbe_object_id_t enclosure_object_id;
    fbe_lifecycle_state_t pvd_state;
    fbe_u32_t pvd_count;
    fbe_u32_t pvd_i;
    fbe_u32_t bus_enc_map[EYE_OF_VECNA_MAXIMUM_PORTS];
    fbe_u32_t bus_i, enc_i, drive_i;
    fbe_u32_t port_count;
    fbe_u32_t drive_slot_count;
    fbe_u32_t backend_bus_count = 0;
    fbe_u32_t min_enclosures_per_bus = 256; //is there a constant for the maximum number of enclosures on a given bus?

    fbe_object_id_t object_id;

    //get core count
    mut_printf(MUT_LOG_TEST_STATUS, "Getting board object ID.");
    status = fbe_api_get_board_object_id(&object_id);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Could not get board object ID!");

    mut_printf(MUT_LOG_TEST_STATUS,"Getting platform information.");
    status = fbe_api_board_get_platform_info(object_id, &platform_info);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Could not get platform info!");
    
    //get total number of ports, then figure out which ones are actually occupied backend ports
    mut_printf(MUT_LOG_TEST_STATUS, "Getting total port count.");
    status = fbe_api_board_get_io_port_count(object_id, &port_count);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Could not get port count!");
    mut_printf(MUT_LOG_TEST_STATUS, "Total port count: %d", port_count);

    for (bus_i = 0; bus_i < EYE_OF_VECNA_MAXIMUM_PORTS; bus_i++) {//initialize array
        bus_enc_map[bus_i] = 0;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "Getting PVD object list");
    status = fbe_api_enumerate_class(FBE_CLASS_ID_PROVISION_DRIVE,FBE_PACKAGE_ID_SEP_0,&pvd_list,&pvd_count);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Could not get PVD object list!");

    for (pvd_i = 0; pvd_i < pvd_count; pvd_i++) {
        mut_printf(MUT_LOG_TEST_STATUS, "Getting PVD info for objectID: 0x%x", *(pvd_list+pvd_i));
        status = fbe_api_provision_drive_get_location(*(pvd_list+pvd_i), &bus_i, &enc_i, &drive_i);
        if (status != FBE_STATUS_BUSY) {
            MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Could not get PVD location!");

            //check for fail states
            status = fbe_api_get_object_lifecycle_state(*(pvd_list+pvd_i), &pvd_state, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_NOT_EQUAL_MSG(FBE_LIFECYCLE_STATE_FAIL,pvd_state,"Test failed due to failed drives. Can't test performance with degraded array");

            if (!drive_i) {//we only care about checking one drive per enclosure, so drive 0 should be fine.
                mut_printf(MUT_LOG_TEST_STATUS, "Updating bus_enc_map: index[%d] = %d",bus_i, enc_i);
                bus_enc_map[bus_i] = enc_i + 1; 
            }
        }
    }

    //fill config's bus map, determine min enclosures
    //assume that at least bus 0 enc 1 is occupied.
    mut_printf(MUT_LOG_TEST_STATUS, "Populating config bus map.");
    if (bus_enc_map[0] > 1) {
        min_enclosures_per_bus = bus_enc_map[0] - 1;
        mut_printf(MUT_LOG_TEST_STATUS, "Adding bus %d to map at index %d", bus_i, backend_bus_count);
        physical_config->occupied_backend_buses[backend_bus_count++] = bus_i;
    }
    for (bus_i = 1; bus_i < EYE_OF_VECNA_MAXIMUM_PORTS; bus_i++) {
        mut_printf(MUT_LOG_TEST_STATUS, "Checking bus existence: bus %d", bus_i);
        if (bus_enc_map[bus_i]){
            mut_printf(MUT_LOG_TEST_STATUS, "Adding bus %d to map at index %d", bus_i, backend_bus_count);
            physical_config->occupied_backend_buses[backend_bus_count++] = bus_i;
            min_enclosures_per_bus = CSX_MIN(bus_enc_map[bus_i], min_enclosures_per_bus);
        }
    }
    fbe_api_free_memory(pvd_list);

    /*get drive slot per enclosure. Assumption is that all enclosures will be fully populated with similar drives,*/
    mut_printf(MUT_LOG_TEST_STATUS, "Getting drive slots per enclosure from bus 0 enclosure 0");
    enc_location.bus = 0;
    enc_location.enclosure = 0;
    status = fbe_api_get_enclosure_object_id_by_location(enc_location.bus, enc_location.enclosure, &enclosure_object_id);
    if ((enclosure_object_id == FBE_OBJECT_ID_INVALID) || (status != FBE_STATUS_OK))
    {	
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't get object ID for bus 0 enclosure 0");
    }

    /* Figure out how many slots this enclosure has */
    status = fbe_api_enclosure_get_slot_count(enclosure_object_id, &drive_slot_count);
    //status = fbe_api_esp_encl_mgmt_get_drive_slot_count(&enc_location, &drive_slot_count);
    if (status != FBE_STATUS_OK)
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't get drive slot count on bus 0 enclosure 0");

    //get drive speed and type the same way, assuming 0_0_0 is representative.
    mut_printf(MUT_LOG_TEST_STATUS, "Getting drive object ID from drive 0_0_0");
    status = fbe_api_get_physical_drive_object_id_by_location(0,0,0, &object_id);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't get drive object ID from drive 0_0_0!");

    mut_printf(MUT_LOG_TEST_STATUS, "Getting drive info for drive object ID: 0x%x", object_id);
    status = fbe_api_physical_drive_get_drive_information(object_id, &drive_info, 0); //not sure what this third argument is for
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't get drive info!");

    //populate and return config struct
    if (FBE_IS_TRUE(platform_info.is_simulation)){
        physical_config->cores = 6;
    }
    else{
        physical_config->cores = (fbe_u32_t) platform_info.hw_platform_info.processorCount;
    }
    physical_config->bus_count = backend_bus_count;
    physical_config->enclosures_per_bus = min_enclosures_per_bus;
    physical_config->drives_per_enclosure = drive_slot_count;
    physical_config->drive_capacity = drive_info.gross_capacity;

    mut_printf(MUT_LOG_TEST_STATUS, "configuration information successfully gathered - cores: %d buses %d enclosures_per_bus: %d slot count: %d capacity: 0x%x",
                                        physical_config->cores,
                                        physical_config->bus_count,
                                        physical_config->enclosures_per_bus,
                                        physical_config->drives_per_enclosure,
                                        (unsigned int)physical_config->drive_capacity);


    return;
}

/*!**************************************************************
 * eye_of_vecna_configure_raid_groups()
 ****************************************************************
 * @brief
 *  This function creates as many raidgroups as possible on the array,
 *  according to the specified raid type and physical location pattern
 *  specifications. It will not attempt to span raid groups across enclosures.
 *
 * @param  eye_of_vecna_physical_config_t *config: the configuration struct generated
 * by king_minos_detect_configuration().
 * 
 * @param eye_of_vecna_logical_config_t *logical_config: the configuration struct modified
 * by king_minos_configure_raid_groups to include information about the logical configuration
 * elements of the array.
 *
 * @return - None.
 *
 ****************************************************************/
void eye_of_vecna_configure_raid_groups(eye_of_vecna_physical_config_t *physical_config,
                                        eye_of_vecna_logical_config_t *logical_config,
                                        fbe_raid_group_type_t raid_type){
    //fbe_u32_t parity_drives;
    fbe_u32_t raidgroups_per_enclosure;
    fbe_u32_t maximum_raidgroup_count;
    fbe_u32_t drives_per_raidgroup;
    fbe_u32_t rg_id = 0;
    fbe_u32_t bus_i = 0;
    fbe_u32_t enclosure_i = 0;
    fbe_u32_t slot_i = 0;
    fbe_u32_t di = 0;
    fbe_u16_t zero_percent = 0;
    fbe_u32_t total_lun_count = 0;
    fbe_u32_t lun_i;

    fbe_object_id_t *pvd_list;
    fbe_u32_t pvd_count;
    fbe_u32_t pvd_i;

    fbe_status_t status;
    fbe_object_id_t object_id;

    fbe_api_raid_group_create_t rg_create_job;
    fbe_api_lun_create_t lun_create_job;

    fbe_job_service_error_type_t job_error;

    fbe_zero_memory(&lun_create_job, sizeof(fbe_api_lun_create_t));
    fbe_zero_memory(&rg_create_job, sizeof(fbe_api_raid_group_create_t));

    drives_per_raidgroup = (raid_type == FBE_RAID_GROUP_TYPE_RAID6) ? //ensure RAID6 multiple of 2
        EYE_OF_VECNA_DRIVES_PER_RAID_GROUP + (EYE_OF_VECNA_DRIVES_PER_RAID_GROUP % 2) : EYE_OF_VECNA_DRIVES_PER_RAID_GROUP;
    raidgroups_per_enclosure = physical_config->drives_per_enclosure/drives_per_raidgroup;
 
    maximum_raidgroup_count = raidgroups_per_enclosure * physical_config->enclosures_per_bus * physical_config->bus_count;

    while (rg_id < maximum_raidgroup_count) {
        //populate disk_array
        mut_printf(MUT_LOG_TEST_STATUS, "Preparing to create RG with rg_id: %d", rg_id);
        for (di = 0; di < drives_per_raidgroup; di++) {
            rg_create_job.disk_array[di].bus = physical_config->occupied_backend_buses[bus_i];
            /*Since we're skipping bus 0 enclosure 0, any time we're on bus 0 we want to go one enclosure ahead of 
              the other buses. This has a performance impact due to adding an additional SAS expander to that port
              as we scale out, but it should be trivial*/
            rg_create_job.disk_array[di].enclosure = (physical_config->occupied_backend_buses[bus_i] == 0) ? (enclosure_i + 1) : enclosure_i;
            rg_create_job.disk_array[di].slot = slot_i + di; //add disk index
            mut_printf(MUT_LOG_TEST_STATUS, "Adding drive %d to disk array for RG: %d - BUS: %d ENCLOSURE: %d DRIVE: %d",
                                                                    di,
                                                                    rg_id,
                                                                    rg_create_job.disk_array[di].bus,
                                                                    rg_create_job.disk_array[di].enclosure,
                                                                    rg_create_job.disk_array[di].slot);
        }
        //populate RG creation struct
        mut_printf(MUT_LOG_TEST_STATUS, "Populating RG creation job struct for RG: %d", rg_id);
        rg_create_job.raid_group_id = rg_id;
        rg_create_job.drive_count = drives_per_raidgroup;
        rg_create_job.power_saving_idle_time_in_seconds = 0xFFFFFFFF;
        rg_create_job.max_raid_latency_time_is_sec = 0xFFFFFFFF;
        rg_create_job.raid_type = raid_type;
        rg_create_job.is_private = FBE_TRI_FALSE;
        rg_create_job.b_bandwidth = FBE_TRUE;
        rg_create_job.capacity = (fbe_block_count_t) physical_config->drive_capacity;         
        //actually create the rg
        mut_printf(MUT_LOG_TEST_STATUS, "Creating RG: %d ...", rg_id);
        status = fbe_api_create_rg(&rg_create_job,
                                           FBE_TRUE, //wait for creation
                                           RG_READY_TIMEOUT, //60 seconds
                                           &object_id,
                                           &job_error); //not needed in this case
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "RAID Group creation failed!");

        //bind lun to that rg based on stroke percentage parameter
        mut_printf(MUT_LOG_TEST_STATUS, "Populating LUN create job structs for RG: %d ...", rg_id);
        for (lun_i = 0; lun_i < EYE_OF_VECNA_MAX_LUNS_PER_RG_SIM - 1; lun_i++)
        {
            if (total_lun_count < EYE_OF_VECNA_MAX_LUNS_TO_TEST_SIM) {
                lun_create_job.raid_type = raid_type;
                lun_create_job.raid_group_id = rg_id;
                lun_create_job.lun_number = (fbe_lun_number_t) total_lun_count;
                lun_create_job.capacity = physical_config->drive_capacity / EYE_OF_VECNA_MAX_LUNS_PER_RG_SIM; //half stroke the drives, not important
                lun_create_job.addroffset = 0x0;
                lun_create_job.ndb_b = FBE_FALSE;
                lun_create_job.placement = FBE_BLOCK_TRANSPORT_FIRST_FIT;
                lun_create_job.noinitialverify_b = FBE_TRUE;
                lun_create_job.world_wide_name.bytes[0] = fbe_random() & 0xf;
                lun_create_job.world_wide_name.bytes[1] = fbe_random() & 0xf;
                
                mut_printf(MUT_LOG_TEST_STATUS, "Creating LUNs for RG: %d COUNT: %u ...", rg_id, total_lun_count);
                status = fbe_api_create_lun(&lun_create_job, 
                                            FBE_TRUE,  //wait for creation
                                            10000, //10 seconds
                                            &object_id,
                                            &job_error);
                MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "LUN creation failed!");
                total_lun_count++;
            }
        }
        
        rg_id++;
        
        //Pick next bus, enclosure, and slot indices based on striping booleans.
        
        //select next drive set based on vertical striping
        bus_i = rg_id % physical_config->bus_count; //always move to the next one
        enclosure_i = (rg_id/physical_config->bus_count) % physical_config->enclosures_per_bus; //move to the next one once we've rotated through each bus
        slot_i = ((rg_id/physical_config->bus_count/physical_config->enclosures_per_bus) % raidgroups_per_enclosure) * drives_per_raidgroup; //increment only once we've visted each bus and enclosure once
        
    }
    //wait for zero to complete on all PVD objects
    mut_printf(MUT_LOG_TEST_STATUS, "Getting PVD object list");
    status = fbe_api_enumerate_class(FBE_CLASS_ID_PROVISION_DRIVE,FBE_PACKAGE_ID_SEP_0,&pvd_list,&pvd_count);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Could not get PVD object list!");

    //wait out PVD zeroing 
    if (!fbe_test_util_is_simulation()) {
        for (pvd_i = 0; pvd_i < pvd_count; pvd_i++) {
            if (pvd_i > 4){ //skip vault drives
                while (zero_percent < 100) {
                    fbe_api_sleep(1000);
                    
                    status = fbe_api_provision_drive_get_zero_percentage(*(pvd_list+pvd_i), &zero_percent);
                    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Could not get PVD zero flag!");
                    mut_printf(MUT_LOG_TEST_STATUS, "PVD zero percent for PVD 0x%x: %d", *(pvd_list+pvd_i), zero_percent);
                }
                zero_percent = 0;
            }
        }
    }
    fbe_api_free_memory(pvd_list);

    //wrap up logical configuration elements
    logical_config->luns_bound = total_lun_count;
    logical_config->lun_capacity = physical_config->drive_capacity / EYE_OF_VECNA_MAX_LUNS_PER_RG_SIM;
    logical_config->drives_per_raidgroup = drives_per_raidgroup;
    return;
}

/*!**************************************************************
 * eye_of_vecna_verify_pdo_counters()
 ****************************************************************
 * @brief
 *  This function sends I/O to the PDOs via rdgen and verifies the
 *  results of some PDO counters.
 *
 * @param  physical_config: the configuration struct generated by
 *         eye_of_vecna_detect_configuration().
 * 
 * @return - None.
 *
 * @author
 *  06/22/2012:  Created. Darren Insko
 *
 ****************************************************************/
void eye_of_vecna_verify_pdo_counters(eye_of_vecna_physical_config_t *physical_config)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_u64_t                                   container_ptr = 0;
    fbe_perfstats_physical_package_container_t  *pp_container;
    fbe_u32_t                                   test_i = 0;
    fbe_u32_t                                   bus_i = 0;
    fbe_u32_t                                   occupied_bus_i = 0;
    fbe_u32_t                                   enclosure_i = 0;
    fbe_u32_t                                   slot_i = 0;
    fbe_object_id_t                             object_id;
    fbe_api_rdgen_context_t                     rdgen_context;
    fbe_rdgen_operation_t                       rdgen_op = FBE_RDGEN_OPERATION_READ_ONLY;
    fbe_lba_t                                   io_size = 1;
    fbe_u32_t                                   core = 0;
    fbe_u32_t                                   offset = 0; 
    fbe_u64_t                                   observed_io_count = 0;
    fbe_u64_t                                   observed_blk_count = 0;   
    /* BEGIN TEST MODIFICATION */
    /* NOTE: need these definitions to use fbe_api_rdgen_get_stats() below */
    fbe_api_rdgen_get_stats_t rdgen_stats;
    fbe_rdgen_filter_t        filter;
    /* END TEST MODIFICATION */

    //enable logging of physical package's performance statistics
    status = fbe_api_perfstats_enable_statistics_for_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Statistics enabling failed!");

    //get the perf stats container pointer
    if (fbe_test_util_is_simulation()) {
        status = fbe_api_perfstats_sim_get_statistics_container_for_package(&container_ptr,
                                                                            FBE_PACKAGE_ID_PHYSICAL);
    }
    else
    {
        status = fbe_api_perfstats_get_statistics_container_for_package(&container_ptr,
                                                                        FBE_PACKAGE_ID_PHYSICAL);
    }
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't get pointer to physical package's perfstats container!");
    pp_container = (fbe_perfstats_physical_package_container_t *) container_ptr;
    MUT_ASSERT_NOT_NULL_MSG(pp_container, "Statistics mapping failed!");

    //1000 IOs to each PDO, cycling through IO sizes, cores, and read/write
    test_i = 0;
    for (bus_i = 0; bus_i < physical_config->bus_count; bus_i++)
    {
        // Only buses that are occupied
        occupied_bus_i = physical_config->occupied_backend_buses[bus_i];
        for (enclosure_i = 0; enclosure_i < physical_config->enclosures_per_bus; enclosure_i++)
        {
            // Skip bus 0 enclosure 0
            if (occupied_bus_i == 0 && enclosure_i == 0)
                continue;

            for (slot_i = 0; slot_i < physical_config->drives_per_enclosure; slot_i++)
            {
                //get object ID for this target
                mut_printf(MUT_LOG_TEST_STATUS, "Looking up object ID for occupied bus: %d  enclosure: %d  slot: %d", occupied_bus_i, enclosure_i, slot_i);
                object_id = FBE_OBJECT_ID_INVALID;
                status = fbe_api_get_physical_drive_object_id_by_location(occupied_bus_i, enclosure_i, slot_i, &object_id);
                MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't get object ID for PDO!");

                if ((object_id == FBE_OBJECT_ID_INVALID) || (object_id > FBE_MAX_PHYSICAL_OBJECTS))
                {
                    mut_printf(MUT_LOG_TEST_STATUS, "Unable to setup rdgen test on PDO B_E_S: %d_%d_%d.  Unusable objectID: 0x%x",
                                                     occupied_bus_i, enclosure_i, slot_i, object_id);
                    continue;
                }
                else
                {
                    mut_printf(MUT_LOG_TEST_STATUS, "Setting up rdgen test on PDO B_E_S: %d_%d_%d  objectID: 0x%x",
                                                     occupied_bus_i, enclosure_i, slot_i, object_id);

                }
                
                status = fbe_api_rdgen_test_context_init(&rdgen_context,                  //context
                                                         object_id,                       //PDO
                                                         FBE_OBJECT_ID_INVALID,           //class ID not needed
                                                         FBE_PACKAGE_ID_PHYSICAL,         //testing PHYSICAL package
                                                         rdgen_op,                        //rdgen operation
                                                         FBE_RDGEN_PATTERN_LBA_PASS,      //rdgen pattern
                                                         0,                               //number of passes (not needed because of manual stop)
                                                         EYE_OF_VECNA_IO_COUNT,           //number of IOs 
                                                         0,                               //time before stop (not needed because of manual stop)
                                                         1,                               //thread count
                                                         FBE_RDGEN_LBA_SPEC_RANDOM,       //seek pattern
                                                         0x0,                             //starting LBA offset
                                                         0x0,                             //min LBA is 0
                                                         FBE_LBA_INVALID,                 //use capacity of object as the max LBA
                                                         FBE_RDGEN_BLOCK_SPEC_CONSTANT,   //constant IO size
                                                         io_size,                         //min IO size
                                                         io_size);                        //max IO size, same as min for constancy
                MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't initialize rdgen context for the PDO!");

                //set affinity
                core = (fbe_u32_t) test_i % physical_config->cores;

                mut_printf(MUT_LOG_TEST_STATUS, "Setting up affinity for rdgen context for PDO B_E_S: %d_%d_%d on core %d",
                                                occupied_bus_i, enclosure_i, slot_i, core);
 
                status = fbe_api_rdgen_io_specification_set_affinity(&rdgen_context.start_io.specification,
                                                                     FBE_RDGEN_AFFINITY_FIXED,
                                                                     core);
                MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't set rdgen context affinity!");

                //kick off IO
                status = fbe_api_rdgen_start_tests(&rdgen_context,
                                                   FBE_PACKAGE_ID_NEIT,
                                                   1);
                MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't start rdgen test!");

                /* BEGIN TEST MODIFICATION */
                /* NOTE: temporarily removing wait for IOs, because this call would time out
                //wait to finish
                status = fbe_api_rdgen_wait_for_ios(&rdgen_context,
                                                    FBE_PACKAGE_ID_NEIT,
                                                    1);
                MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't wait for IOs to finish!");
                */
                /* NOTE: Instead, I've borrowed the following logic from sally_struthers.c, which doesn't wait on IOs to complete */
                mut_printf(MUT_LOG_TEST_STATUS, "== %s starting I/O for 30 seconds ==", __FUNCTION__);
                fbe_api_sleep(30 * FBE_TIME_MILLISECONDS_PER_SECOND);

                // Make sure I/O is progressing OK
                status = fbe_api_rdgen_get_stats(&rdgen_stats, &filter);
                MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't obtain rdgen stats to determine IO progress!");

                // Stop I/O (if it hasn't already completed)
                status = fbe_api_rdgen_stop_tests(&rdgen_context, 1);
                MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't stop rdgen test!");
                /* END TEST MODIFICATION */

                //verify IO count (within some tolerance, although it really shouldn't be something other than 1000.
                status = fbe_api_perfstats_get_offset_for_object(object_id,
                                                                 FBE_PACKAGE_ID_PHYSICAL,
                                                                 &offset);
                MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't get object offset!");

                if (rdgen_op == FBE_RDGEN_OPERATION_WRITE_ONLY)
                {
                    observed_blk_count = pp_container->pdo_stats[offset].disk_blocks_written[core];
                    observed_io_count = pp_container->pdo_stats[offset].disk_writes[core];
                }  
                else
                {
                    observed_blk_count = pp_container->pdo_stats[offset].disk_blocks_read[core];
                    observed_io_count = pp_container->pdo_stats[offset].disk_reads[core];
                }
                mut_printf(MUT_LOG_TEST_STATUS, "Block count: %d IO count: %d", (int)observed_blk_count, (int)observed_io_count);
                MUT_ASSERT_TRUE_MSG((observed_blk_count >= EYE_OF_VECNA_IO_COUNT * io_size), "block count is incorrect");
                MUT_ASSERT_TRUE_MSG((observed_io_count >= EYE_OF_VECNA_IO_COUNT), "io count is incorrect");

                //set params for next iteration
                io_size = (io_size == 1024) ? 1 : io_size*2;
                rdgen_op = (rdgen_op == FBE_RDGEN_OPERATION_READ_ONLY) ? FBE_RDGEN_OPERATION_WRITE_ONLY : FBE_RDGEN_OPERATION_READ_ONLY;

                //clear the physical package's perfstats container
                status = fbe_api_perfstats_zero_statistics_for_package(FBE_PACKAGE_ID_PHYSICAL);
                MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't zero the physical package's perfstats container!");

                //destroy context
                status = fbe_api_rdgen_test_context_destroy(&rdgen_context);

                test_i++;
            }
        }
    }
}

void eye_of_vecna_verify_lun_counters(eye_of_vecna_physical_config_t *physical_config,
                                      eye_of_vecna_logical_config_t *logical_config){
    fbe_status_t                    status;
    fbe_u32_t                       test_i;
    fbe_u64_t                       stat_ptr;
    fbe_lba_t                       io_size = 1;

    fbe_api_rdgen_context_t         rdgen_context;
    fbe_rdgen_operation_t           rdgen_op = FBE_RDGEN_OPERATION_READ_ONLY;
    fbe_rdgen_lba_specification_t   lba_spec = FBE_RDGEN_LBA_SPEC_CATERPILLAR_INCREASING;
    
    fbe_perfstats_sep_container_t   *sep_stats;
    fbe_lun_performance_counters_t  lun_stats;
    fbe_object_id_t                 object_id; 
    
    fbe_u32_t                       core = 0;
    fbe_u32_t                       offset = 0; 
    fbe_u64_t                       observed_io_count = 0;
    fbe_u64_t                       observed_blk_count = 0;   
    fbe_u32_t                       bucket_i;
    fbe_u32_t                       disk_i = 0;
    fbe_bool_t                      is_enabled = FBE_FALSE;

    //enable stat logging
    status = fbe_api_perfstats_enable_statistics_for_package(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Statistics enabling failed for SEP!");

    //check to see if stat logging took hold
    status = fbe_api_perfstats_is_statistics_collection_enabled_for_package(FBE_PACKAGE_ID_SEP_0,
                                                                            &is_enabled);
    MUT_ASSERT_TRUE_MSG((is_enabled && status == FBE_STATUS_OK), "Package-wide statistics enabling failed for SEP!");

    //get stat pointer for SEP
    if (fbe_test_util_is_simulation()) {
        status = fbe_api_perfstats_sim_get_statistics_container_for_package(&stat_ptr,
                                                                            FBE_PACKAGE_ID_SEP_0);
    }
    else{
        //TODO: we'll need to copy this data over the transport.
        status = fbe_api_perfstats_get_statistics_container_for_package(&stat_ptr,
                                                                        FBE_PACKAGE_ID_SEP_0);
    }
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't get SEP stat pointer!");
    sep_stats = (fbe_perfstats_sep_container_t *) stat_ptr;
    MUT_ASSERT_NOT_NULL_MSG(sep_stats, "Statistics mapping failed! (SEP)");

    // IOs to each lun, cycling through IO sizes, cores, and read/write
    for(test_i = 0; test_i < logical_config->luns_bound; test_i++) {
        //get object_id for lun number
        mut_printf(MUT_LOG_TEST_STATUS, "Looking up object ID for LUN: %d", test_i);
        status = fbe_api_database_lookup_lun_by_number(test_i,&object_id);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't get object ID for LUN!");

        mut_printf(MUT_LOG_TEST_STATUS, "Initiating rdgen context for LUN: %d", test_i);
        status = fbe_api_rdgen_test_context_init(&rdgen_context,                 //context
                                                 object_id,                     //lun object
                                                 FBE_OBJECT_ID_INVALID,         //class ID not needed
                                                 FBE_PACKAGE_ID_SEP_0,          //testing SEP and down
                                                 rdgen_op,                      //rdgen operation
                                                 FBE_RDGEN_PATTERN_LBA_PASS,    //rdgen pattern
                                                 0,                             //number of passes (not needed because of manual stop)
                                                 EYE_OF_VECNA_IO_COUNT,         //number of IOs 
                                                 0,                             //time before stop (not needed because of manual stop)
                                                 EYE_OF_VECNA_THREAD_COUNT,     //thread count
                                                 lba_spec,                      //seek pattern
                                                 0x0,                           //starting LBA offset
                                                 0x0,                           //min LBA is 0
                                                 logical_config->lun_capacity,  //stroke the entirety of the bound LUN
                                                 FBE_RDGEN_BLOCK_SPEC_CONSTANT, //constant IO size
                                                 io_size,                       //min IO size
                                                 io_size);                      //max IO size, same as min for constancy
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't initialize rdgen context!");

        //set affinity
        mut_printf(MUT_LOG_TEST_STATUS, "Setting up affinity for rdgen context for LUN: %d", test_i);
 
        core = (fbe_u32_t) test_i % physical_config->cores;

        status = fbe_api_rdgen_io_specification_set_affinity(&rdgen_context.start_io.specification,
                                                             FBE_RDGEN_AFFINITY_FIXED,
                                                             core);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't set rdgen context affinity!");

        //set alignment
        /*status = fbe_api_rdgen_io_specification_set_alignment_size(&rdgen_context.start_io.specification,
                                                                   (fbe_u32_t) io_size);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't set rdgen context alignment!");*/

        //kick off IO
        status = fbe_api_rdgen_start_tests(&rdgen_context,
                                           FBE_PACKAGE_ID_NEIT,
                                           1);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't wait for IOs to finish!");

        //wait to finish
        status = fbe_api_rdgen_wait_for_ios(&rdgen_context,
                                            FBE_PACKAGE_ID_NEIT,
                                            1);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't wait for IOs to finish!");

        //destroy context
        status = fbe_api_rdgen_test_context_destroy(&rdgen_context);

        //verify IO count 
        status = fbe_api_perfstats_get_offset_for_object(object_id,
                                                         FBE_PACKAGE_ID_SEP_0,
                                                         &offset);
        status = fbe_api_lun_get_performance_stats(object_id,
                                                   &lun_stats);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't get object offset!");

        //make sure object ID wasn't zeroed
        MUT_ASSERT_INT_EQUAL_MSG(object_id, lun_stats.object_id, "Object ID mismatch!");

        if (rdgen_op == FBE_RDGEN_OPERATION_WRITE_ONLY) {
            observed_blk_count = lun_stats.lun_blocks_written[core];
            observed_io_count = 0;
            for (bucket_i = 0; bucket_i < FBE_PERFSTATS_HISTOGRAM_BUCKET_LAST; bucket_i++) {
                observed_io_count += lun_stats.lun_io_size_write_histogram[bucket_i][core];
            }
            MUT_ASSERT_TRUE_MSG((lun_stats.cumulative_write_response_time[core] > 0), "write response time is 0");
            MUT_ASSERT_TRUE_MSG((observed_io_count == lun_stats.lun_write_requests[core]), "histogram and total IOs don't align");
            if (io_size > EYE_OF_VECNA_STRIPE_SIZE)
            { 
                if (lba_spec == FBE_RDGEN_LBA_SPEC_RANDOM)
                {//should have hit stripe crossings
                    MUT_ASSERT_TRUE_MSG((lun_stats.stripe_crossings[core] > 0), "IO size is larger than stripe size, but no crossings.");
                }
                else
                {//stripe writes
                    MUT_ASSERT_TRUE_MSG((lun_stats.stripe_writes[core] > 0), "IO size is larger than stripe size, but no writes.");
                }
            }
        }
        else {
            observed_blk_count =lun_stats.lun_blocks_read[core];
            observed_io_count = 0;
            for (bucket_i = 0; bucket_i < FBE_PERFSTATS_HISTOGRAM_BUCKET_LAST; bucket_i++) {
                observed_io_count += lun_stats.lun_io_size_read_histogram[bucket_i][core];
            }
             MUT_ASSERT_TRUE_MSG((lun_stats.cumulative_read_response_time[core] > 0), "read response time is 0");
             MUT_ASSERT_TRUE_MSG((observed_io_count == lun_stats.lun_read_requests[core]), "histogram and total IOs don't align"); 
            if (io_size > EYE_OF_VECNA_STRIPE_SIZE && lba_spec == FBE_RDGEN_LBA_SPEC_RANDOM)
            { //should have hit stripe crossings
                MUT_ASSERT_TRUE_MSG((lun_stats.stripe_crossings[core] > 0), "IO size is larger than stripe size, but no crossings");
            }  
        }

        mut_printf(MUT_LOG_TEST_STATUS, "Block count: %d IO count: %d", (int)observed_blk_count, (int)observed_io_count);
        MUT_ASSERT_TRUE_MSG((lun_stats.sum_arrival_queue_length[core] > 0), "arrival queue length is 0");
        MUT_ASSERT_TRUE_MSG((lun_stats.non_zero_queue_arrivals[core] > 0), "nonzero queue arrivals are 0");  
        MUT_ASSERT_TRUE_MSG((observed_blk_count >= EYE_OF_VECNA_IO_COUNT * io_size * EYE_OF_VECNA_THREAD_COUNT), "block count is incorrect");
        MUT_ASSERT_TRUE_MSG((observed_io_count >= EYE_OF_VECNA_IO_COUNT * EYE_OF_VECNA_THREAD_COUNT), "io count is incorrect");

        //make sure SEP disk counters add up correctly as well
        observed_blk_count = 0;
        observed_io_count = 0;
        for (disk_i = 0; disk_i < logical_config->drives_per_raidgroup; disk_i++)
        {
            observed_blk_count += (lun_stats.disk_blocks_read[disk_i][core] + lun_stats.disk_blocks_written[disk_i][core]);
            observed_io_count += (lun_stats.disk_reads[disk_i][core] + lun_stats.disk_writes[disk_i][core]);
        }

        MUT_ASSERT_TRUE_MSG((observed_blk_count >= EYE_OF_VECNA_IO_COUNT * io_size), "block count is incorrect");
        MUT_ASSERT_TRUE_MSG((observed_io_count >= EYE_OF_VECNA_IO_COUNT), "io count is incorrect");

        //set params for next iteration
        io_size = (io_size == 1024) ? 1 : io_size*2;
        rdgen_op = (rdgen_op == FBE_RDGEN_OPERATION_READ_ONLY) ? FBE_RDGEN_OPERATION_WRITE_ONLY : FBE_RDGEN_OPERATION_READ_ONLY;
        lba_spec = (FBE_RDGEN_LBA_SPEC_CATERPILLAR_INCREASING || io_size < 256) ? FBE_RDGEN_LBA_SPEC_RANDOM : FBE_RDGEN_LBA_SPEC_CATERPILLAR_INCREASING ;

        //clear stats
        status = fbe_api_perfstats_zero_statistics_for_package(FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't zero stat container!");
    }

    //enable stat logging
    status = fbe_api_perfstats_disable_statistics_for_package(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Statistics disabling failed for SEP!");

    //check to see if stat logging took hold
    status = fbe_api_perfstats_is_statistics_collection_enabled_for_package(FBE_PACKAGE_ID_SEP_0,
                                                                            &is_enabled);
    MUT_ASSERT_TRUE_MSG((!is_enabled && status == FBE_STATUS_OK), "Package-wide statistics disabling failed for SEP!");

    //enable stat logging
    status = fbe_api_perfstats_disable_statistics_for_package(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Statistics disabling failed for SEP!");

    //check to see if stat logging took hold
    status = fbe_api_perfstats_is_statistics_collection_enabled_for_package(FBE_PACKAGE_ID_SEP_0,
                                                                            &is_enabled);
    MUT_ASSERT_TRUE_MSG((!is_enabled && status == FBE_STATUS_OK), "Package-wide statistics disabling failed for SEP!");

    mut_printf(MUT_LOG_TEST_STATUS, "All cycles completed with accurate recording of stats.\n");
    status = fbe_test_sep_util_destroy_all_user_luns();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_destroy_all_user_raid_groups();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);    
    return;
}


void eye_of_vecna_perf_stats(void)
{
   eye_of_vecna_physical_config_t      physical_config;
   eye_of_vecna_logical_config_t       logical_config;
   fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;

   fbe_bool_t                      is_enabled = FBE_FALSE;
   fbe_lun_performance_counters_t               lun_stats;
   fbe_u32_t                       test_i;
   fbe_object_id_t                 object_id;
   fbe_pdo_performance_counters_t                  pdo_stats;
   fbe_object_id_t                                 *pdo_list;
   fbe_u32_t                                       pdo_i;
   fbe_u32_t                                       pdo_count;
   fbe_class_id_t                       class_id;

   eye_of_vecna_detect_configuration(&physical_config);


   eye_of_vecna_configure_raid_groups(&physical_config,
                                       &logical_config,
                                       FBE_RAID_GROUP_TYPE_RAID5);

   //enable logging of SEP package's performance statistics
   status = fbe_api_perfstats_enable_statistics_for_package(FBE_PACKAGE_ID_SEP_0);
   MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Statistics enabling on SEP.");
   //check to see if stat logging took hold
   status = fbe_api_perfstats_is_statistics_collection_enabled_for_package(FBE_PACKAGE_ID_SEP_0,
                                                                           &is_enabled);
   MUT_ASSERT_TRUE_MSG((is_enabled && status == FBE_STATUS_OK), "Package-wide statistics enabling failed for SEP!");

   for (test_i = 0; test_i < logical_config.luns_bound; test_i++) {

      //get object_id for lun number
      mut_printf(MUT_LOG_TEST_STATUS, "Looking up object ID for LUN: %d", test_i);
      status = fbe_api_database_lookup_lun_by_number(test_i,&object_id);
      MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
      status = fbe_api_lun_get_performance_stats(object_id,&lun_stats);
      MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
   }

   //disable logging of SEP package's performance statistics
   status = fbe_api_perfstats_disable_statistics_for_package(FBE_PACKAGE_ID_SEP_0);
   MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Statistics disabling on SEP.");
   //check to see if stat logging took hold
   status = fbe_api_perfstats_is_statistics_collection_enabled_for_package(FBE_PACKAGE_ID_SEP_0,
                                                                           &is_enabled);
   MUT_ASSERT_TRUE_MSG((!is_enabled && status == FBE_STATUS_OK), "Package-wide statistics disabling failed for SEP!");

   for (test_i = 0; test_i < logical_config.luns_bound; test_i++) {

      //get object_id for lun number
      mut_printf(MUT_LOG_TEST_STATUS, "Looking up object ID for LUN: %d", test_i);
      status = fbe_api_database_lookup_lun_by_number(test_i,&object_id);
      MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
      status = fbe_api_lun_get_performance_stats(object_id,&lun_stats);
      MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);
   }

   status = fbe_api_get_physical_drive_object_id_by_location(0,0,0, &object_id);
   MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
   fbe_api_get_object_class_id(object_id, &class_id, FBE_PACKAGE_ID_PHYSICAL);
   status = fbe_api_enumerate_class(class_id,FBE_PACKAGE_ID_PHYSICAL,&pdo_list,&pdo_count);
   MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Could not get PDO object list!");
   mut_printf(MUT_LOG_TEST_STATUS, "PDO list head: 0x%X, pdo_count = %d", *pdo_list, pdo_count);

   //enable logging of physical package's performance statistics
   status = fbe_api_perfstats_enable_statistics_for_package(FBE_PACKAGE_ID_PHYSICAL);
   MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Statistics enabling on PP.");
   //check to see if stat logging took hold
   status = fbe_api_perfstats_is_statistics_collection_enabled_for_package(FBE_PACKAGE_ID_PHYSICAL,
                                                                           &is_enabled);
   MUT_ASSERT_TRUE_MSG((is_enabled && status == FBE_STATUS_OK), "Package-wide statistics enabling failed for PP!");

   for (pdo_i = 0; pdo_i < pdo_count; pdo_i++) {
      status = fbe_api_physical_drive_get_perf_stats(*(pdo_list+pdo_i),
                                                          &pdo_stats);

      MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
   }




   //disable logging of physical package's performance statistics
   status = fbe_api_perfstats_disable_statistics_for_package(FBE_PACKAGE_ID_PHYSICAL);
   MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Statistics disabling on PP.");
   //check to see if stat logging took hold
   status = fbe_api_perfstats_is_statistics_collection_enabled_for_package(FBE_PACKAGE_ID_PHYSICAL,
                                                                           &is_enabled);
   MUT_ASSERT_TRUE_MSG((!is_enabled && status == FBE_STATUS_OK), "Package-wide statistics disabling failed for PP!");

   for (pdo_i = 0; pdo_i < pdo_count; pdo_i++) {
      status = fbe_api_physical_drive_get_perf_stats(*(pdo_list+pdo_i),
                                                          &pdo_stats);
      MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);
   }


   return;
}


/*!**************************************************************
 * eye_of_vecna_test()
 ****************************************************************
 * @brief
 *  Test mapped memory functionality by getting handle from perfstats
 *  service memory.
 *  This is a single SP test.
 *
 * @param  None.            
 *
 * @return - None.
 *
 ****************************************************************/
void eye_of_vecna_test(void){
    eye_of_vecna_physical_config_t      physical_config;
    eye_of_vecna_logical_config_t       logical_config;

    eye_of_vecna_detect_configuration(&physical_config);

    //test RAID 5
    eye_of_vecna_configure_raid_groups(&physical_config,
                                       &logical_config,
                                       FBE_RAID_GROUP_TYPE_RAID5);

    eye_of_vecna_verify_lun_counters(&physical_config,
                                     &logical_config);

    //test RAID 6
    eye_of_vecna_configure_raid_groups(&physical_config,
                                       &logical_config,
                                       FBE_RAID_GROUP_TYPE_RAID6);

    eye_of_vecna_verify_lun_counters(&physical_config,
                                 &logical_config);

    //test RAID 0
    eye_of_vecna_configure_raid_groups(&physical_config,
                                       &logical_config,
                                       FBE_RAID_GROUP_TYPE_RAID0);

    eye_of_vecna_verify_lun_counters(&physical_config,
                                     &logical_config);
    eye_of_vecna_perf_stats();

    return;
}

/*!**************************************************************
 * eye_of_vecna_setup()
 ****************************************************************
 * @brief
 *  Setup for the Eye of Vecna. 
 *
 * @param None.               
 *
 * @return None.   
 *
 * @note    Only single-sp mode is supported
 *
 * @author
 *  04/05/2012 - Created. Ryan Gadsby
 *
 ****************************************************************/
void eye_of_vecna_setup(void){
    fbe_sep_package_load_params_t sep_params;
    
    fbe_u32_t cmd_line_flags = 0;

    fbe_zero_memory(&sep_params, sizeof(fbe_sep_package_load_params_t));
    //init and start packages if simulation
    if (fbe_test_util_is_simulation()) {        
        fbe_test_pp_util_create_physical_config_for_disk_counts(60, //8 enclosures + 0_0
                                                                0,//no 4160-bps drives
                                                                0x4B000000); //600GB drives
        if (fbe_test_sep_util_get_cmd_option_int("-raid_library_default_debug_flags", &cmd_line_flags))
        {
            sep_params.raid_library_debug_flags = cmd_line_flags;
            mut_printf(MUT_LOG_TEST_STATUS, "using raid library default debug flags of: 0x%x", sep_params.raid_library_debug_flags);
        }
        else
        {
            sep_params.raid_library_debug_flags = FBE_RAID_LIBRARY_DEBUG_FLAG_NONE;
        }
        sep_config_load_sep_and_neit_params(&sep_params, NULL);
    }

    fbe_test_sep_util_set_default_io_flags();
    fbe_test_common_util_test_setup_init();
    return;
}

/*!**************************************************************
 * eye_of_vecna_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the eye_of_vecna test.
 *
 * @param None.               
 *
 * @return None.
 *
 ****************************************************************/

void eye_of_vecna_cleanup(void){
    // Always clear dualsp mode
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }

    return;
}


