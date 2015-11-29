
/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file king_minos_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains a dynamic hardware performance test.
 *
 * @version
 *   10/05/2011:  Created. Ryan Gadsby
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
//TODO: check which ones king minos actually needs
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
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_esp_module_mgmt_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_dps_memory_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "pp_utils.h"
#include <time.h>
#include <math.h>
#include "fbe/fbe_random.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
//TODO: change from template stolen from elmo!
char * king_minos_short_desc = "Normal IO backend performance testing with logging.";
char * king_minos_long_desc = "\
\n\
\n\
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
Description last updated: 12/28/2011.\n";

 /*!*******************************************************************
 * @def KING_MINOS_MAXIMUM_CORES
 *********************************************************************
 * @brief Maximum number of cores on hardware, per SP.
 *
 *********************************************************************/
 #define KING_MINOS_MAXIMUM_CORES 20

 /*!*******************************************************************
 * @def KING_MINOS_MAXIMUM_PORTS
 *********************************************************************
 * @brief Maximum number of ports on hardware
 *
 *********************************************************************/
 #define KING_MINOS_MAXIMUM_PORTS 36

/*!*******************************************************************
 * @def KING_MINOS_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Number of LUNs per raid group.
 *
 *********************************************************************/
#define KING_MINOS_TEST_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def KING_MINOS_DATA_DRIVES_PER_RAID_GROUP
 *********************************************************************
 * @brief Number of non-parity drives per raid group
 *
 *********************************************************************/
#define KING_MINOS_DATA_DRIVES_PER_RAID_GROUP 4

/*!*******************************************************************
 * @def KING_MINOS_STROKE_FACTOR
 *********************************************************************
 * @brief Denominator of drive seek utilized as part of IO tests
 *
 *********************************************************************/
#define KING_MINOS_STROKE_FACTOR 2 //2 = half stroke (4 is quarter stroke, and so on)

/*!*******************************************************************
 * @def KING_MINOS_TEST_AREA_INCREASE_FACTOR
 *********************************************************************
 * @brief Number by which we multiply the number of test areas by on each
 * iteration of the rdgen loop. For example, if we start with 6 test areas,
 * an increase factor of 4 would have us testing 6, 24, 96, 384, ..., n with
 * n <= the luns bound on the array
 *
 *********************************************************************/
#define KING_MINOS_TEST_AREA_INCREASE_FACTOR 2 

/*!*******************************************************************
 * @def KING_MINOS_RANDOM_THREAD_COUNT
 *********************************************************************
 * @brief Thread count to each object in rdgen during performance testing
 * for random workloads
 *********************************************************************/
#define KING_MINOS_DEFAULT_RANDOM_THREAD_COUNT 64

/*!*******************************************************************
 * @def KING_MINOS_SEQUENTIAL_THREAD_COUNT
 *********************************************************************
 * @brief Thread count to each object in rdgen during performance testing
 * for sequential/caterpillar workloads
 *********************************************************************/
#define KING_MINOS_DEFAULT_SEQUENTIAL_THREAD_COUNT 1

/*!*******************************************************************
 * @def KING_MINOS_RDGEN_TASK_DURATION_MILLISECONDS_HARDWARE
 *********************************************************************
 * @brief IO profile count. Changing the number of IO profiles drastically
 * impacts the runtime of the 
 *
 *********************************************************************/
#define KING_MINOS_RDGEN_TASK_DEFAULT_DURATION_MILLISECONDS_HARDWARE 120000 //2 min

/*!*******************************************************************
 * @def KING_MINOS_RDGEN_TASK_DURATION_MILLISECONDS_SIMULATION
 *********************************************************************
 * @brief IO profile count. Changing the number of IO profiles drastically
 * impacts the runtime of the 
 *
 *********************************************************************/
#define KING_MINOS_RDGEN_TASK_DEFAULT_DURATION_MILLISECONDS_SIMULATION 5000 //5 sec

/*!*******************************************************************
 * @def KING_MINOS_SIMULATION_CORES
 *********************************************************************
 * @brief Number of processors to simulate when running in simulation mode.
 *
 *********************************************************************/
#define KING_MINOS_SIMULATION_CORES 6

/*!*******************************************************************
 * @def KING_MINOS_SIMULATION_DRIVES
 *********************************************************************
 * @brief Number of drives to simulate when running in simulation mode.
 *
 *********************************************************************/
#define KING_MINOS_SIMULATION_DRIVES 120 //0_0 is automatically added to this count, so it's actually 135 drives

/*!*******************************************************************
 * @def KING_MINOS_SIMULATION_DRIVES
 *********************************************************************
 * @brief Number of drives to simulate when running in simulation mode.
 *
 *********************************************************************/
 #define KING_MINOS_SIMULATION_CAPACITY 0x4B000000 //600GB drives, but this doesn't really matter.

/*!*******************************************************************
 * @def KING_MINOS_RESULT_DIRECTORY_WINDOWS
 *********************************************************************
 * @brief Where the result directory will be located on the machine
 * running this test scenario
 *
 *********************************************************************/
 #define KING_MINOS_RESULT_DIRECTORY_WINDOWS "C:\\FBE_Performance_Logs\\" 

/*!*******************************************************************
 * @def KING_MINOS_RESULT_DIRECTORY_LINUX
 *********************************************************************
 * @brief Where the result directory will be located on the machine
 * running this test scenario
 *
 *********************************************************************/
 #define KING_MINOS_RESULT_DIRECTORY_LINUX "/usr/FBE_Performance_Logs/"

/*!*******************************************************************
 * @def KING_MINOS_CONFIG_PATH_WINDOWS
 *********************************************************************
 * @brief Where the result directory will be located on the machine
 * running this test scenario
 *
 *********************************************************************/
 #define KING_MINOS_CONFIG_PATH_WINDOWS "C:\\kingminos.cfg"

 /*!*******************************************************************
 * @def KING_MINOS_PATH_BUFFER_MAX
 *********************************************************************
 * @brief How much space to allocate for the path buffer in the logging
 * function.
 *********************************************************************/
 #define KING_MINOS_PATH_BUFFER_MAX 1024

 /*!*******************************************************************
 * @def KING_MINOS_LINE_BUFFER_MAX
 *********************************************************************
 * @brief How much space to allocate for the line buffer in the logging
 * function
 *********************************************************************/
 #define KING_MINOS_LINE_BUFFER_MAX 1024

 /*!*******************************************************************
 * @def KING_MINOS_FILE_BUFFER_MAX
 *********************************************************************
 * @brief How much space to allocate for the file name buffer in the logging
 * function
 *********************************************************************/
 #define KING_MINOS_FILE_BUFFER_MAX 64

 /*!*******************************************************************
 * @def KING_MINOS_TOKEN_BUFFER_MAX
 *********************************************************************
 * @brief How much space to allocate for the token buffer in the logging
 * function
 *********************************************************************/
 #define KING_MINOS_TOKEN_BUFFER_MAX 64

 /*!*******************************************************************
 * @def KING_MINOS_RANDOM_TOKEN
 *********************************************************************
 * @brief String token denoting a random workloads
 *********************************************************************/
 #define KING_MINOS_RANDOM_TOKEN "Random.dat"

 /*!*******************************************************************
 * @def KING_MINOS_SEQUENTIAL_TOKEN
 *********************************************************************
 * @brief String token denoting a sequential workload
 *********************************************************************/
 #define KING_MINOS_SEQUENTIAL_TOKEN "Sequential.dat"

 /*!*******************************************************************
 * @def KING_MINOS_REGRESSION_TOLERANCE
 *********************************************************************
 * @brief Defines the minimum threshhold of performance a new result
 * must be in order not to fail the test. It's expressed as a percentage
 * of the baseline.
 *********************************************************************/
 #define KING_MINOS_REGRESSION_TOLERANCE .90 

 /*!*******************************************************************
 * @def KING_MINOS_GAIN_TOLERANCE
 *********************************************************************
 * @brief Defines the point past which a new result should probably be
 * closely examined. Often significant performance gains can be ill-gotten,
 * and a legitimate increase in performance means a new baseline should
 * be taken.
 *********************************************************************/
 #define KING_MINOS_GAIN_TOLERANCE 1.2 
 
/*!*******************************************************************
 * @def king_minos_physical_config_t
 *********************************************************************
 * @brief Holds various hardware details needed to configure the array correctly,
 * specify the correct workloads, and determine what directory to store
 * logs in.
 *********************************************************************/
typedef struct king_minos_physical_config_e {
    fbe_u32_t cores;
    fbe_u32_t bus_count;
    fbe_u32_t occupied_backend_buses[KING_MINOS_MAXIMUM_CORES];
    fbe_u32_t enclosures_per_bus;
    fbe_u32_t drives_per_enclosure;
    fbe_u32_t drive_speed;
    fbe_lba_t drive_capacity;
}king_minos_physical_config_t ;

/*!*******************************************************************
 * @def king_minos_logical_config_t
 *********************************************************************
 * @brief Populated after configuring raid groups and luns, this struct
 * holds any relevant data for the logical configuration at any given point
 * in the test. 
 *********************************************************************/
typedef struct king_minos_logical_config_e{
    fbe_raid_group_type_t raid_type;
    fbe_u32_t max_raidgroups;
    fbe_u32_t luns_bound;
    fbe_bool_t vertical_bus_striping;
    fbe_bool_t vertical_enclosure_striping;
    fbe_lba_t lun_capacity;
} king_minos_logical_config_t;

/*!*******************************************************************
 * @def king_minos_cmd_t
 *********************************************************************
 * @brief This struct holds all the data relevant to defining a given IO
 * profile, which we use to populate rdgen constructs. Read_threads and
 * write_threads denote the ratio to use for read_write mixes. (Currently,
 * rdgen doesn't support this, so these two variables are effectively booleans
 * that describe whether it's a read or write workload. Read has priority.
 *********************************************************************/
typedef struct king_minos_cmd_e {
    fbe_bool_t is_sequential;
    fbe_bool_t affine_per_core;

    fbe_u64_t io_size;
    fbe_u32_t target_count;
    fbe_u32_t read_threads;
    fbe_u32_t write_threads;

    fbe_u32_t duration_msec; 
}king_minos_cmd_t;

/*!*******************************************************************
 * @def king_minos_test_status_t
 *********************************************************************
 * @brief Holds the various states a given IO profile could have. 
 *********************************************************************/
typedef enum king_minos_test_status_e
{
    KING_MINOS_STATUS_FAILED,
    KING_MINOS_STATUS_PASSED,
    KING_MINOS_STATUS_PASSED_WITH_WARNING,
    KING_MINOS_STATUS_TOOK_NEW_BASELINE,
    KING_MINOS_STATUS_NOT_YET_EVALUATED,
    KING_MINOS_STATUS_OLD_BASELINE
}king_minos_test_status_t;

/*!*******************************************************************
 * @def king_minos_result_t
 *********************************************************************
 * @brief Holds the counters we get by querying rdgen threads and other
 * services. Anything that is a single variable is an aggregate figure across
 * cores, and anything in an array denotes a per-core variable.
 *********************************************************************/
typedef struct king_minos_result_e {
    king_minos_cmd_t *command_ran;
    king_minos_test_status_t test_status;
    double ios_std_dev;
    double cpu_std_dev;

    fbe_u32_t ios_per_seconds[KING_MINOS_MAXIMUM_CORES];
    fbe_u32_t cpu_utilization[KING_MINOS_MAXIMUM_CORES];  
    //fbe_u32_t latency_histogram[];
    //float avg_latency[];
    //float latency_std_dev;
}king_minos_result_t;

/*!*******************************************************************
 * @var king_minos_benchmark_task_list
 *********************************************************************
 * @brief This is a list of king_minos_cmd_t that specify the basic io
 * workloads that we're testing (4K random writes, 1M sequential reads,etc).
 * 
 * This list is designed to mimic the Performance group's list of tested workloads.
 * There are significant differences in the way these tests are run versus their
 * tests, perhaps the most important of which is that rdgen operates below read
 * and write cache. This way, any performance regressions we observe in King Minos
 * are almost certainly because of FBE code, and it's reasonable to assume that
 * any performance regression observed by an end-to-end IO workload will be similarly
 * observed in King Minos if it's directly because of FBE code. 
 * 
 * target_count will always be zero as this template gets filled in during
 * king_minos_generate_cmds.
 * 
 * IO size is represented in 512B sectors. 
 * 
 *********************************************************************/

static king_minos_cmd_t king_minos_default_task_list[] =
{
    //sequential,   affine per core,    io size,    target count,   read threads,                               write threads,                              duration                                                                                
    {FBE_TRUE,      FBE_TRUE,           0x200,      0,              KING_MINOS_DEFAULT_SEQUENTIAL_THREAD_COUNT,     0,                                          KING_MINOS_RDGEN_TASK_DEFAULT_DURATION_MILLISECONDS_HARDWARE}, //sequential 256K reads (Rich Media)
    {FBE_TRUE,      FBE_TRUE,           0x200,      0,              0,                                          KING_MINOS_DEFAULT_SEQUENTIAL_THREAD_COUNT,     KING_MINOS_RDGEN_TASK_DEFAULT_DURATION_MILLISECONDS_HARDWARE},  //sequential 256K writes (Backup)
    {FBE_FALSE,     FBE_TRUE,           0x10,       0,              KING_MINOS_DEFAULT_RANDOM_THREAD_COUNT,     0,                                          KING_MINOS_RDGEN_TASK_DEFAULT_DURATION_MILLISECONDS_HARDWARE}, //random 8K read
    {FBE_FALSE,     FBE_TRUE,           0x10,       0,              0,                                          KING_MINOS_DEFAULT_RANDOM_THREAD_COUNT,     KING_MINOS_RDGEN_TASK_DEFAULT_DURATION_MILLISECONDS_HARDWARE}, //random 8K write
};

static king_minos_cmd_t king_minos_file_task_list[16]; //max is 16 workloads from file for test time considerations.
static fbe_bool_t use_default = FBE_TRUE;
static fbe_u32_t king_minos_number_of_benchmark_tasks = sizeof(king_minos_default_task_list)/sizeof(king_minos_cmd_t);


/*!*******************************************************************
 * @def king_minos_log_single_fields[]
 *********************************************************************
 * @brief Holds the titles of the fields that hold a single variable (like
 * standard deviation) in the header of the log file. This is designed to make it
 * easier to define a new baseline and check format consistency.
 *********************************************************************/
char king_minos_single_fields[][32] =
{
    "Timestamp", "Test Status", "IOs/second(std_dev)", "CPU_Util%(std_dev)", //
};

/*!*******************************************************************
 * @def king_minos_log_per_core_fields[]
 *********************************************************************
 * @brief As with the single core fields, but this holds field titles for
 * counters that are recorded on a per-core basis.
 *********************************************************************/
char king_minos_per_core_fields[][32] =
{
    "IOs/second", "CPU_Util%", //"avg_latency",
};

/*!*******************************************************************
 * @def king_minos_status tokens[]
 *********************************************************************
 * @brief Holds string tokens to denote the possible command statuses
 * defined in king_minos_test_status_t.
 *********************************************************************/
char king_minos_status_tokens[][32] =
{
    "FAILED","PASSED","PASSED-WARNING","BASELINE","NOT_EVALUATED","OLDBLINE",
};



/*!**************************************************************
 * king_minos_detect_configuration()
 ****************************************************************
 * @brief
 *  Populates a struct containing the relevant information to create
 *  performance workloads
 * 
 *  This is a single SP test.
 *
 * @param  king_minos_config_t containing the number of cores,
 * the number of buses (occupied ports), the number of enclosures
 * per bus and number of drives per enclosure, as well as a code to
 * denote the pattern of port occupation. It also includes drive
 * speed and capacity.           
 *
 * @return - None
 *
 ***************************************************************/
void king_minos_detect_configuration(king_minos_physical_config_t *physical_config){
    fbe_status_t status;
    fbe_board_mgmt_platform_info_t platform_info;
    fbe_device_physical_location_t enc_location;
    fbe_object_id_t enclosure_object_id;
    fbe_physical_drive_information_t drive_info;

    fbe_object_id_t *pvd_list;
    fbe_lifecycle_state_t pvd_state;
    fbe_u32_t pvd_count;
    fbe_u32_t pvd_i;
    fbe_u32_t bus_enc_map[KING_MINOS_MAXIMUM_PORTS];
    fbe_u32_t bus_i, enc_i, drive_i;
    fbe_u32_t port_count;
    fbe_u32_t drive_slot_count;
    fbe_u32_t backend_bus_count = 0;
    fbe_u32_t min_enclosures_per_bus = 256; //is there a constant for the maximum number of enclosures on a given bus?

    fbe_object_id_t object_id;

    mut_printf(MUT_LOG_TEST_STATUS, "entering king_minos_detect_configuration.");

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

    for (bus_i = 0; bus_i < KING_MINOS_MAXIMUM_PORTS; bus_i++) {//initialize array
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
            //set zero flag on PVD
            //status = fbe_api_provision_drive_set_zero_checkpoint(*(pvd_list+pvd_i), FBE_LBA_INVALID);
            //MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Could not set PVD zero flag!");

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
        mut_printf(MUT_LOG_TEST_STATUS, "Adding bus %d to map at index %d", 0, backend_bus_count);
        physical_config->occupied_backend_buses[backend_bus_count++] = 0;
    }
    for (bus_i = 1; bus_i < KING_MINOS_MAXIMUM_PORTS; bus_i++) {
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
        physical_config->cores = KING_MINOS_SIMULATION_CORES;
    }
    else{
        physical_config->cores = (fbe_u32_t) platform_info.hw_platform_info.processorCount;
    }
    physical_config->bus_count = backend_bus_count;
    physical_config->enclosures_per_bus = min_enclosures_per_bus;
    physical_config->drives_per_enclosure = drive_slot_count;
    physical_config->drive_capacity = drive_info.gross_capacity;
    physical_config->drive_speed = drive_info.speed_capability;

    mut_printf(MUT_LOG_TEST_STATUS, "configuration information successfully gathered - cores: %d buses %d enclosures_per_bus: %d slot count: %d capacity: 0x%llx speed %d",
               physical_config->cores,
               physical_config->bus_count,
               physical_config->enclosures_per_bus,
               physical_config->drives_per_enclosure,
               (unsigned long long)physical_config->drive_capacity,
               physical_config->drive_speed);

    mut_printf(MUT_LOG_TEST_STATUS, "exiting king_minos_detect_configuration.");
    return;
}

/*!**************************************************************
 * king_minos_configure_raid_groups()
 ****************************************************************
 * @brief
 *  This function creates as many raidgroups as possible on the array,
 *  according to the specified raid type and physical location pattern
 *  specifications. It will not attempt to span raid groups across enclosures.
 * 
 *  The difference between using this function and the functions traditionally
 *  used to initialize raid group configurations is that this function is not
 *  agnostic when it comes to physical locations. There are hardware bottlenecks that port
 *  and enclosure location impact, so this function lets us get around them (or choose not to),
 *  as well as make sure we know exactly where our raidgroups are ahead of time.
 *
 * @param  king_minos_physical_config_t *config: the configuration struct generated
 * by king_minos_detect_configuration().
 * 
 * @param king_minos_logical_config_t *logical_config: the configuration struct modified
 * by king_minos_configure_raid_groups to include information about the logical configuration
 * elements of the array.
 * 
 * @param fbe_raid_group_type_t raid_type: The desired raid level. This test
 * scenario uses 4 data drives per raid group.
 * 
 * @param fbe_bool_t vertical_enclosure_striping: whether or not to stripe raid groups
 * across as many separate enclosures as possible. If this is true, we bind LUNs by
 * round-robin-ing across each enclosure on each port. Otherwise we get as many drives on
 * a single enclosure as possible before moving on to the next one. T
 * 
 * @param fbe_bool_t vertical_bus_striping: as with vertical_enclosure_striping, but with backend
 * buses.
 *
 * @return - None.
 *
 ****************************************************************/
void king_minos_configure_raid_groups(king_minos_physical_config_t *physical_config, king_minos_logical_config_t *logical_config, fbe_raid_group_type_t raid_type, fbe_bool_t vertical_enclosure_striping, 
                                                            fbe_bool_t vertical_port_striping){
    fbe_u32_t parity_drives;
    fbe_u32_t raidgroups_per_enclosure;
    fbe_u32_t maximum_raidgroup_count;
    fbe_u32_t drives_per_raidgroup;
    fbe_u32_t rg_id = 0;
    fbe_u32_t bus_i = 0;
    fbe_u32_t enclosure_i = 0;
    fbe_u32_t slot_i = 0;
    fbe_u32_t di;

    fbe_object_id_t *pvd_list;
    fbe_u32_t pvd_count;
    fbe_u32_t pvd_i;
    fbe_u16_t zero_percent = 0;

    fbe_status_t status;
    fbe_object_id_t object_id;

    fbe_api_raid_group_create_t rg_create_job;
    fbe_api_lun_create_t lun_create_job;
    fbe_job_service_error_type_t job_error_code;

    fbe_zero_memory(&rg_create_job, sizeof(fbe_api_raid_group_create_t));
    fbe_zero_memory(&lun_create_job, sizeof(fbe_api_lun_create_t));

    mut_printf(MUT_LOG_TEST_STATUS, "entering king_minos_configure_raid_groups.");

    //calculate maximum number of raid groups without spanning across enclosures
    mut_printf(MUT_LOG_TEST_STATUS, "RAID type: %d", raid_type);
    switch (raid_type) {
    case FBE_RAID_GROUP_TYPE_RAID0:
        parity_drives = 0;
        break;
    case FBE_RAID_GROUP_TYPE_RAID5:
        parity_drives = 1;
        break;
    case FBE_RAID_GROUP_TYPE_RAID6:
        parity_drives = 2;
        break;
    default:
        MUT_ASSERT_NOT_NULL_MSG(NULL, "Invalid raid type! Only RAID 0, 5, and 6 are supported by King Minos");
    }

    drives_per_raidgroup = KING_MINOS_DATA_DRIVES_PER_RAID_GROUP + parity_drives;
    raidgroups_per_enclosure = physical_config->drives_per_enclosure/drives_per_raidgroup;
 
    maximum_raidgroup_count = raidgroups_per_enclosure * physical_config->enclosures_per_bus * physical_config->bus_count;

    //populate logical configuration struct
    logical_config->raid_type = raid_type;
    logical_config->max_raidgroups = maximum_raidgroup_count;
    logical_config->vertical_bus_striping = vertical_port_striping;
    logical_config->vertical_enclosure_striping = vertical_enclosure_striping;

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
        rg_create_job.capacity = (fbe_block_count_t) physical_config->drive_capacity; //fully stroke on the RG
        
        //actually create the rg
        mut_printf(MUT_LOG_TEST_STATUS, "Creating RG: %d ...", rg_id);
        status = fbe_api_create_rg(&rg_create_job,
                                           FBE_TRUE, //wait for creation
                                           RG_READY_TIMEOUT, //60 seconds
                                           &object_id,
                                           &job_error_code); //not needed in this case
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "RAID Group creation failed!");

        //bind lun to that rg based on stroke percentage parameter
        mut_printf(MUT_LOG_TEST_STATUS, "Populating LUN create job struct for RG: %d ...", rg_id);
        lun_create_job.raid_type = raid_type;
        lun_create_job.raid_group_id = rg_id;
        lun_create_job.lun_number = (fbe_lun_number_t) rg_id;
        lun_create_job.capacity = physical_config->drive_capacity / KING_MINOS_STROKE_FACTOR;
        lun_create_job.addroffset = 0x0;
        lun_create_job.placement = FBE_BLOCK_TRANSPORT_FIRST_FIT;
        lun_create_job.noinitialverify_b = FBE_TRUE;
        lun_create_job.world_wide_name.bytes[0] = fbe_random() & 0xf;
        lun_create_job.world_wide_name.bytes[1] = fbe_random() & 0xf;
        
        
        mut_printf(MUT_LOG_TEST_STATUS, "Creating LUN for RG: %d ...", rg_id);
        status = fbe_api_create_lun(&lun_create_job, 
                                    FBE_TRUE,  //wait for creation
                                    10000, //10 seconds
                                    &object_id,
                                    &job_error_code); //not needed in this case
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "LUN creation failed!");
        
        rg_id++;
        
        //Pick next bus, enclosure, and slot indices based on striping booleans.
        
        //vertical in both directions
        if (FBE_IS_TRUE(vertical_port_striping) && FBE_IS_TRUE(vertical_enclosure_striping)) {
            bus_i = rg_id % physical_config->bus_count; //always move to the next one
            enclosure_i = (rg_id/physical_config->bus_count) % physical_config->enclosures_per_bus; //move to the next one once we've rotated through each bus
            slot_i = ((rg_id/physical_config->bus_count/physical_config->enclosures_per_bus) % raidgroups_per_enclosure) * drives_per_raidgroup; //increment only once we've visted each bus and enclosure once
        }
        //horizontal in both directions
        if (FBE_IS_FALSE(vertical_port_striping) && FBE_IS_FALSE(vertical_enclosure_striping)) {
            bus_i = rg_id/(physical_config->enclosures_per_bus*raidgroups_per_enclosure); //move on only when bus is filled 
            enclosure_i = (rg_id/raidgroups_per_enclosure) % physical_config->enclosures_per_bus; //move on every time we fill it
            slot_i = (rg_id % raidgroups_per_enclosure) * drives_per_raidgroup ; //always move on to the next slot
        }
        //vertical bus, horizontal enclosure
        if (FBE_IS_TRUE(vertical_port_striping) && FBE_IS_FALSE(vertical_enclosure_striping)) {
            bus_i = rg_id % physical_config->bus_count; //always move on
            enclosure_i = (rg_id/(physical_config->bus_count*raidgroups_per_enclosure)) % physical_config->enclosures_per_bus; //move on when the enclosure is filled
            slot_i = (rg_id/physical_config->bus_count % raidgroups_per_enclosure) * drives_per_raidgroup; //move on when bus rotates
        }
        //horizontal bus, vertical enclosure
        if (FBE_IS_FALSE(vertical_port_striping) && FBE_IS_TRUE(vertical_enclosure_striping)) {
            bus_i = rg_id/(physical_config->enclosures_per_bus*raidgroups_per_enclosure); //increment only when bus is filled 
            enclosure_i = rg_id % physical_config->enclosures_per_bus; //always increment
            slot_i = (rg_id/physical_config->enclosures_per_bus % raidgroups_per_enclosure) * drives_per_raidgroup;//increment when back to an already visited enclosure
        }
    }

    //wait for zero to complete on all PVD objects
    mut_printf(MUT_LOG_TEST_STATUS, "Getting PVD object list");
    status = fbe_api_enumerate_class(FBE_CLASS_ID_PROVISION_DRIVE,FBE_PACKAGE_ID_SEP_0,&pvd_list,&pvd_count);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Could not get PVD object list!");

    //wait out PVD zeroing 
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
    //wrap up logical configuration elements
    logical_config->luns_bound = rg_id;
    logical_config->lun_capacity = physical_config->drive_capacity / KING_MINOS_STROKE_FACTOR;
    mut_printf(MUT_LOG_TEST_STATUS, "exiting king_minos_configure_raid_groups.");
    return;
}

/*!**************************************************************
 * king_minos_generate_cmds()
 ****************************************************************
 * @brief
 *  This function creates a list of king_minos_cmd_t structs that each
 *  contain the information required to run a single IO workload. The different
 *  workloads are described in the variable havent_named_it_yet, and thoose workloads
 *  are expanded to scale out according to resources the test has available. 
 *
 * @param  king_minos_physical_config_t *physical_config: the configuration struct generated
 * by king_minos_detect_configuration().
 * 
 * @param king_minos_logical_config_t *logical_config: the configuration struct modified
 * by king_minos_configure_raid_groups to include information about the logical configuration
 * elements of the array. 
 * 
 * @return - pointer to list of king_minos_cmd_t pointers containing permutations
 * of the IO profiles designated by king_minos_benchmark_task_list.
 *
 ****************************************************************/ 
king_minos_cmd_t ** king_minos_generate_cmds(king_minos_physical_config_t *physical_config, 
                                             king_minos_logical_config_t *logical_config,
                                             king_minos_cmd_t **cmd_list){
    king_minos_cmd_t *new_cmd;
    fbe_u32_t cmd_count = 0;
    fbe_u32_t test_area_i = 1;
    fbe_u32_t cmd_i;

    mut_printf(MUT_LOG_TEST_STATUS, "entering king_minos_generate_cmds.");
    

    //find lcm of config->buses and config->cores so that we know we're balancing across cores. If striping horizontally
    //across ports, it doesn't matter.
    mut_printf(MUT_LOG_TEST_STATUS, "Determining starting test area count.");
    if (FBE_IS_TRUE(logical_config->vertical_bus_striping)) {
        mut_printf(MUT_LOG_TEST_STATUS, "Vertical bus striping: count is LCM(cores,buses).");
        while((test_area_i % physical_config->bus_count) || (test_area_i % physical_config->cores)) {
            test_area_i++;
        }
    }
    else{
        mut_printf(MUT_LOG_TEST_STATUS, "Vertical bus striping: count is cores.");
        test_area_i = physical_config->cores;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "Populating IO profile list.");
    while (test_area_i <= logical_config->max_raidgroups) { //one for each test area count, logarithmically increasing from lcm(cores, ports)
        //only add if largest possible config, or if we're in scaleout mode
        if(fbe_test_sep_util_get_cmd_option("-perf_suite_scaleout") || test_area_i * KING_MINOS_STROKE_FACTOR > logical_config->max_raidgroups) {
            mut_printf(MUT_LOG_TEST_STATUS, "Copying IO profiles for test area count: %d", test_area_i);
            //generate a command for each cmd_t in the benchmark list
            for (cmd_i = 0; cmd_i < king_minos_number_of_benchmark_tasks; cmd_i++) {
                //add command to cmd_list
                mut_printf(MUT_LOG_TEST_STATUS, "Adding profile: %d", cmd_i);
                new_cmd = (king_minos_cmd_t *) fbe_api_allocate_memory(sizeof(king_minos_cmd_t));
                MUT_ASSERT_NOT_NULL_MSG(new_cmd, "Memory allocation failed for temp_cmd!");

                new_cmd->affine_per_core = use_default ? king_minos_default_task_list[cmd_i].affine_per_core : king_minos_file_task_list[cmd_i].affine_per_core;
                new_cmd->is_sequential = use_default ? king_minos_default_task_list[cmd_i].is_sequential : king_minos_file_task_list[cmd_i].is_sequential;
                new_cmd->io_size = use_default ? king_minos_default_task_list[cmd_i].io_size : king_minos_file_task_list[cmd_i].io_size;
                new_cmd->read_threads = use_default ? king_minos_default_task_list[cmd_i].read_threads : king_minos_file_task_list[cmd_i].read_threads;
                new_cmd->write_threads = use_default ? king_minos_default_task_list[cmd_i].write_threads : king_minos_file_task_list[cmd_i].write_threads;
                new_cmd->duration_msec =  use_default ? king_minos_default_task_list[cmd_i].duration_msec : king_minos_file_task_list[cmd_i].duration_msec;
                new_cmd->target_count = test_area_i; 
                //print out temp_cmd contents
                mut_printf(MUT_LOG_TEST_STATUS,"Adding cmd to cmd_list...");
                mut_printf(MUT_LOG_TEST_STATUS, "cmd_i: %d affine_per_core: %d", cmd_i, new_cmd->affine_per_core);
                mut_printf(MUT_LOG_TEST_STATUS, "cmd_i: %d is_sequential: %d", cmd_i, new_cmd->is_sequential);
                mut_printf(MUT_LOG_TEST_STATUS, "cmd_i: %d io_size: 0x%x", cmd_i, (unsigned int)new_cmd->io_size);
                mut_printf(MUT_LOG_TEST_STATUS, "cmd_i: %d read_threads: %d", cmd_i, new_cmd->read_threads);
                mut_printf(MUT_LOG_TEST_STATUS, "cmd_i: %d write_threads: %d", cmd_i, new_cmd->write_threads);
                mut_printf(MUT_LOG_TEST_STATUS, "cmd_i: %d target_count: %d", cmd_i, new_cmd->target_count);

                *(cmd_list+cmd_count) = new_cmd;
                cmd_count++; 
            }
        }
        test_area_i *= KING_MINOS_TEST_AREA_INCREASE_FACTOR;
        mut_printf(MUT_LOG_TEST_STATUS, "New test_area count: %d", test_area_i);
    }
     mut_printf(MUT_LOG_TEST_STATUS, "exiting king_minos_generate_cmds.");
    return cmd_list;
}

/*!**************************************************************
 * king_minos_execute_cmd()
 ****************************************************************
 * @brief
 *  This function executes a single IO command specified by a king_minos_cmd_t. This kicks
 *  off a series of rdgen threads, waits an amount of time specified by KING_MINOS_RDGEN_TASK_DURATION_MILLISECONDS,
 *  populates the results struct with various information, then kills all rdgen threads before exiting.
 * 
 * @param king_minos_cmd_t *cmd: pointer to a cmd structure generated by king_minos_generate_cmds().
 * 
 * @param king_minos_result_t *result: pointer to a result struct to hold thread information for
 * the IO profile specified.
 * 
 * @param  king_minos_physical_config_t *physical_config: the configuration struct generated
 * by king_minos_detect_configuration().
 * 
 * @param king_minos_logical_config_t *logical_config: the configuration struct modified
 * by king_minos_configure_raid_groups to include information about the logical configuration
 * elements of the array. 
 * 
 * @return - None
 *
 ****************************************************************/
void king_minos_execute_cmd(king_minos_cmd_t *cmd, 
                                    king_minos_result_t *result, 
                                    king_minos_physical_config_t *physical_config, 
                                    king_minos_logical_config_t *logical_config){

    fbe_api_rdgen_context_t *rdgen_contexts;
    fbe_api_rdgen_get_thread_info_t * rdgen_thread_stats;
    fbe_object_id_t object_id;
    fbe_lun_number_t ta_i;
    fbe_u32_t thd_i;
    fbe_u32_t core_i;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_rdgen_operation_t rdgen_op = FBE_RDGEN_OPERATION_INVALID;
    fbe_rdgen_lba_specification_t lba_spec = FBE_RDGEN_LBA_SPEC_INVALID;
    fbe_rdgen_affinity_t affinity_spec = FBE_RDGEN_AFFINITY_INVALID;
    fbe_rdgen_options_t options = FBE_RDGEN_OPTIONS_PERFORMANCE | FBE_RDGEN_OPTIONS_DO_NOT_CHECK_CRC |
        FBE_RDGEN_OPTIONS_DO_NOT_PANIC_ON_DATA_MISCOMPARE;
    fbe_api_rdgen_get_stats_t stats;
    fbe_rdgen_filter_t filter;
    fbe_u32_t core = 0;
    fbe_u32_t ios_per_second[KING_MINOS_MAXIMUM_CORES];
    fbe_u32_t counter_mean = 0;
    fbe_u32_t variance_mean = 0;
    fbe_api_sim_server_get_cpu_util_t cpu_util;



    mut_printf(MUT_LOG_TEST_STATUS, "entering king_minos_execute_cmd.");

    result->command_ran = cmd;

    //initialize result fields
    for (core_i = 0; core_i < KING_MINOS_MAXIMUM_CORES; core_i++) {
        ios_per_second[core_i] = 0;
    }

    rdgen_contexts = (fbe_api_rdgen_context_t *) fbe_api_allocate_memory(sizeof(fbe_api_rdgen_context_t) * cmd->target_count);
    //determine operation and seek pattern from command. Will need to add support later for R/W mixes
    mut_printf(MUT_LOG_TEST_STATUS, "Determining if read or write threads");
    if (cmd->read_threads) {
        mut_printf(MUT_LOG_TEST_STATUS, "Cmd is a read workload.");
        rdgen_op = FBE_RDGEN_OPERATION_READ_ONLY;
    }
    else{
        mut_printf(MUT_LOG_TEST_STATUS, "Cmd is a write workload.");
        rdgen_op = FBE_RDGEN_OPERATION_WRITE_ONLY;
    }

    //seek
    mut_printf(MUT_LOG_TEST_STATUS, "Determining seek pattern.");
    lba_spec = cmd->is_sequential ? FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING : FBE_RDGEN_LBA_SPEC_RANDOM;
    mut_printf(MUT_LOG_TEST_STATUS, "Seek pattern: %d (5 is cat_inc, 1 is random)", lba_spec);

    mut_printf(MUT_LOG_TEST_STATUS, "Generating rdgen contexts for each test area.");
    //for each command, generate an rdgen context for each test area, and run them
    for (ta_i = 0; ta_i < cmd->target_count; ta_i++) {
        //get object_id for lun number
        mut_printf(MUT_LOG_TEST_STATUS, "Looking up object ID for LUN: %d", ta_i);
        status = fbe_api_database_lookup_lun_by_number(ta_i,&object_id);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't get object ID for LUN!");

        mut_printf(MUT_LOG_TEST_STATUS, "Initiating rdgen context for LUN: %d", ta_i);
        status = fbe_api_rdgen_test_context_init(rdgen_contexts + ta_i,          //context
                                                 object_id,                     //lun object
                                                 FBE_OBJECT_ID_INVALID,         //class ID not needed
                                                 FBE_PACKAGE_ID_SEP_0,          //testing SEP and down
                                                 rdgen_op,                      //rdgen operation
                                                 FBE_RDGEN_PATTERN_LBA_PASS,    //rdgen pattern
                                                 0x0,                           //number of passes (not needed because of manual stop)
                                                 0x0,                           //number of IOs (not needed because of manual stop)
                                                 0x0,                           //time before stop (not needed because of manual stop)
                                                 (cmd->read_threads + cmd->write_threads),       //thread count
                                                 lba_spec,                      //seek pattern
                                                 0x0,                           //starting LBA offset
                                                 0x0,                           //min LBA is 0
                                                 logical_config->lun_capacity,  //stroke the entirety of the bound LUN
                                                 FBE_RDGEN_BLOCK_SPEC_CONSTANT, //constant IO size
                                                 cmd->io_size,                  //min IO size
                                                 cmd->io_size);                 //max IO size, same as min for constancy
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't initialize rdgen context!");

        //set affinity
        mut_printf(MUT_LOG_TEST_STATUS, "Setting up affinity for rdgen context for LUN: %d", ta_i);
        //rotate around cores when striping vertically across backend buses
        if (logical_config->vertical_bus_striping){
             affinity_spec = FBE_RDGEN_AFFINITY_FIXED;
             core = (fbe_u32_t) ta_i % physical_config->cores;
             mut_printf(MUT_LOG_TEST_STATUS, "Affinity: Per Core (this context: core %d)", core);
        }

        //otherwise we should just pick randomly 
        else{
            affinity_spec = FBE_RDGEN_AFFINITY_NONE;
            mut_printf(MUT_LOG_TEST_STATUS, "Affinity: None");
        }
        status = fbe_api_rdgen_io_specification_set_affinity(&((rdgen_contexts+ta_i)->start_io.specification),
                                                             affinity_spec,
                                                             core);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't set rdgen context affinity!");

        //set options
        mut_printf(MUT_LOG_TEST_STATUS, "Setting up option flags for rdgen context for LUN: %d", ta_i);
        status = fbe_api_rdgen_io_specification_set_options(&((rdgen_contexts+ta_i)->start_io.specification), options);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't set rdgen option flags!");

        //set alignment
        mut_printf(MUT_LOG_TEST_STATUS, "Setting up IO alignment for rdgen context for LUN: %d", ta_i);
        status = fbe_api_rdgen_io_specification_set_alignment_size(&((rdgen_contexts+ta_i)->start_io.specification), (fbe_u32_t) cmd->io_size);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't set rdgen alignment spec!");
    }

    mut_printf(MUT_LOG_TEST_STATUS, "Starting all rdgen tests.");
    status = fbe_api_rdgen_start_tests(rdgen_contexts, FBE_PACKAGE_ID_NEIT, cmd->target_count);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't start rdgen tests!");

    //sleep for test duration
    if (fbe_test_util_is_simulation()) {
        mut_printf(MUT_LOG_TEST_STATUS, "Sleeping for test duration: %d milliseconds", KING_MINOS_RDGEN_TASK_DEFAULT_DURATION_MILLISECONDS_SIMULATION);
        fbe_api_sleep(KING_MINOS_RDGEN_TASK_DEFAULT_DURATION_MILLISECONDS_SIMULATION);    
    }
    else{
        mut_printf(MUT_LOG_TEST_STATUS, "Sleeping for test duration: %d milliseconds", cmd->duration_msec);
        fbe_api_sleep(cmd->duration_msec);    
    }
    mut_printf(MUT_LOG_TEST_STATUS, "Waking up.");
    
    //use get_thread_info to return counter information.
    fbe_api_rdgen_filter_init(&filter, 
                                  rdgen_contexts[0].start_io.filter.filter_type, 
                                  rdgen_contexts[0].start_io.filter.object_id, 
                                  rdgen_contexts[0].start_io.filter.class_id, 
                                  rdgen_contexts[0].start_io.filter.package_id,
                                  0    /* edge index */);
    status = fbe_api_rdgen_get_stats(&stats, &filter);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't get rdgen stat information!");

    //get thread information
    rdgen_thread_stats = fbe_api_allocate_memory(sizeof(fbe_api_rdgen_get_thread_info_t) * cmd->target_count *(cmd->read_threads + cmd->write_threads));
    mut_printf(MUT_LOG_TEST_STATUS, "Getting rdgen thread information for %d requests, %d threads", stats.requests, stats.threads);
    status = fbe_api_rdgen_get_thread_info(rdgen_thread_stats, cmd->target_count * (cmd->read_threads + cmd->write_threads));
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't get rdgen thread information!");

    mut_printf(MUT_LOG_TEST_STATUS, "Total_thread_count %d. Threads per object: %d ",
                                                            rdgen_thread_stats->num_threads,
                                                            rdgen_thread_stats->num_threads/cmd->target_count);

    //load stats into result struct
    mut_printf(MUT_LOG_TEST_STATUS, "Aggregating thread information for %d threads", rdgen_thread_stats->num_threads);
    for (ta_i = 0; ta_i < cmd->target_count; ta_i++) {
        for (thd_i = ta_i * (cmd->read_threads + cmd->write_threads); thd_i < (ta_i+1)* (cmd->read_threads + cmd->write_threads); thd_i++) {
            mut_printf(MUT_LOG_TEST_STATUS, "Thread number: %d, handle = 0x%x", thd_i, (unsigned int)rdgen_thread_stats->thread_info_array[thd_i].object_handle);
            if (rdgen_thread_stats->thread_info_array[thd_i].elapsed_milliseconds) { 
                ios_per_second[logical_config->vertical_bus_striping ?  ta_i % physical_config->cores : 0] += 
                                        1000 * rdgen_thread_stats->thread_info_array[thd_i].io_count / rdgen_thread_stats->thread_info_array[thd_i].elapsed_milliseconds;
            }
            else{
                //for whatever reason, a thread wasn't running. Throw up a warning message!
                mut_printf(MUT_LOG_TEST_STATUS, "thread %d did not run, performance not optimal for this environment!", thd_i);
            }
        }
    }
    //load results in, calculate variance
    for (core_i = 0; core_i < physical_config->cores; core_i++){
        mut_printf(MUT_LOG_TEST_STATUS, "IOs per second for core %d: %d", core_i, ios_per_second[core_i]);
        result->ios_per_seconds[core_i] = ios_per_second[core_i];
        counter_mean += ios_per_second[core_i];
    }
    counter_mean /= physical_config->cores;
    for (core_i = 0; core_i < physical_config->cores; core_i++){
        variance_mean += (fbe_u32_t) pow(ios_per_second[core_i] - counter_mean,2);
    }
    result->ios_std_dev = sqrt(variance_mean/physical_config->cores);

        //get CPU utilization
    mut_printf(MUT_LOG_TEST_STATUS, "Getting CPU utilization sample");
    status = fbe_api_sim_server_get_windows_cpu_utilization(&cpu_util);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,"Failed to get CPU utilization!");

    //load results while calculating std_dev
    counter_mean = 0;
    variance_mean = 0;
    for (core_i = 0; core_i < physical_config->cores; core_i++){
        result->cpu_utilization[core_i] = cpu_util.cpu_usage[core_i];
        counter_mean += cpu_util.cpu_usage[core_i];
    }
    counter_mean /= physical_config->cores;
    for (core_i = 0; core_i < physical_config->cores; core_i++){
        variance_mean += (fbe_u32_t) pow(cpu_util.cpu_usage[core_i] - counter_mean,2);
    }
    result->cpu_std_dev = sqrt(variance_mean/physical_config->cores);

    //stop all tests, destroy context structs
    mut_printf(MUT_LOG_TEST_STATUS, "Stopping all rdgen tests.");
    status = fbe_api_rdgen_stop_tests(rdgen_contexts, cmd->target_count);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't stop rdgen_tests!");

    fbe_api_sleep(500); //give a chance for all the threads to stop. 

    fbe_api_free_memory(rdgen_contexts);
    fbe_api_free_memory(rdgen_thread_stats);
    mut_printf(MUT_LOG_TEST_STATUS, "exiting king_minos_execute_cmd.");
    return;
}

/*!**************************************************************
 * king_minos_print_result_to_file()
 ****************************************************************
 * @brief
 *   This function takes an open file pointer, and writes a line containing all the information
 *   for the specified result struct to that log file.
 * 
 * @param FILE *log_p - open file handle to a log file generated by this test scenario.
 * 
 * @param  king_minos_physical_config_t *physical_config: the configuration struct generated
 * by king_minos_detect_configuration().
 * 
 * @param king_minos_result_t *result: pointer to a result struct.
 * 
 * @return - None
 *
 ****************************************************************/
void king_minos_print_result_to_file(FILE *log_p, king_minos_physical_config_t *physical_config,
                                       king_minos_result_t *result){

    time_t timestamp;
    struct tm *ltime;
    char timestamp_buffer[32];
    fbe_u32_t core_i;

    //handle timestamp
    timestamp = time(NULL);
    ltime = localtime(&timestamp);
    sprintf(timestamp_buffer, "%d/%d %02d:%02d\t",
                                ltime->tm_mon+1, //American format for dates :P
                                ltime->tm_mday,
                                ltime->tm_hour,
                                ltime->tm_min);
    fprintf(log_p, timestamp_buffer);

    //print non-timestamp singular fields
    //test status
    fprintf(log_p, "%s\t", king_minos_status_tokens[result->test_status]);

    //IOPS standard deviation
    fprintf(log_p, "%4f\t", result->ios_std_dev);

    //CPU standard deviation
    fprintf(log_p, "%4f\t", result->cpu_std_dev);

    //print per core fields
    //IOs per second
    for (core_i = 0; core_i < physical_config->cores; core_i++) {
        fprintf(log_p, "%d\t", result->ios_per_seconds[core_i]);
    }

    //cpu utilization
    for (core_i = 0; core_i < physical_config->cores; core_i++) {
        fprintf(log_p, "%d\t", result->cpu_utilization[core_i]);
    }
    //TODO: latency histogram
    fprintf(log_p, "\n");
    return;
}

/*!**************************************************************
 * king_minos_evaluate()
 ****************************************************************
 * @brief
 *   This function handles changing the value of a result_t's test
 *   status field when being compared to baseline value. This function
 *   will change that field's value to "failed" if the test doesn't meet
 *   the regression tolerance, "pass with warning" if the test scores
 *   above the gain tolerance, but WILL NOT change the status of the
 *   test otherwise, lest we prematurely past the test. The actual
 *   marking of the status as "passed" is handled in king_minos_log_and_compare().
 * 
 * @param king_minos_result_t *baseline_result: pointer to a results of a baseline read from a file.
 * 
 * @param  king_minos_result_t *new_result: pointer to the results of the IO test that was just run.  
 * @return - None.
 *
 ****************************************************************/
void king_minos_evaluate(king_minos_result_t *baseline_result, king_minos_result_t *new_result, king_minos_physical_config_t *physical_config){
    double performance_relative_to_baseline = 0.0;
    float baseline_iops_sum = 0.0;
    float new_iops_sum = 0.0;

    fbe_u32_t core_i = 0;

    //compare based on sum of IOPS
    for (core_i = 0; core_i < physical_config->cores; core_i++){
        mut_printf(MUT_LOG_TEST_STATUS, "Evaluating result - Baseline: %d New: %d", baseline_result->ios_per_seconds[core_i], new_result->ios_per_seconds[core_i]);
        baseline_iops_sum += baseline_result->ios_per_seconds[core_i];
        new_iops_sum += new_result->ios_per_seconds[core_i];
    }

    if(baseline_iops_sum) //should never be 0, but check anyway
        performance_relative_to_baseline = new_iops_sum/baseline_iops_sum; 
    else
        performance_relative_to_baseline = KING_MINOS_STATUS_PASSED_WITH_WARNING + 1;
    
    mut_printf(MUT_LOG_TEST_STATUS, "Performance_relative_to_baseline: %f", performance_relative_to_baseline);
    if (performance_relative_to_baseline < KING_MINOS_REGRESSION_TOLERANCE) 
        new_result->test_status = KING_MINOS_STATUS_FAILED;
    else if(performance_relative_to_baseline > KING_MINOS_GAIN_TOLERANCE) 
        new_result->test_status = KING_MINOS_STATUS_PASSED_WITH_WARNING;
    else new_result->test_status = KING_MINOS_STATUS_PASSED;
    return;
}



/*!**************************************************************
 * king_minos_compare_and_log_result()
 ****************************************************************
 * @brief
 *   This function is responsible for the following:
 *   -Checking to see if a baseline exists for the given configuration and workload
 *   -If a baseline doesn't exist, create a new one (printing out the headers, and this
 *   command's test.
 *   -If it does exist, check it for consistency, and read it into a result struct.
 *   -Compare the baseline to the current results to determine if we pass or fail.
 *   -Log the results to the file. 
 * 
 * @param king_minos_result_t *result: pointer to a result struct to hold thread information for
 * the IO profile specified.
 * 
 * @param  king_minos_physical_config_t *physical_config: the configuration struct generated
 * by king_minos_detect_configuration().
 * 
 * @param king_minos_logical_config_t *logical_config: the configuration struct modified
 * by king_minos_configure_raid_groups to include information about the logical configuration
 * elements of the array. 
 * 
 * @return - None
 *
 ****************************************************************/
void king_minos_compare_and_log_result(king_minos_result_t *result,
                                       king_minos_physical_config_t *physical_config,
                                       king_minos_logical_config_t *logical_config){
    char path_buffer[KING_MINOS_PATH_BUFFER_MAX];
    char file_buffer[KING_MINOS_PATH_BUFFER_MAX + KING_MINOS_FILE_BUFFER_MAX];
    char line_buffer[KING_MINOS_LINE_BUFFER_MAX];
    char mkdir_buffer[KING_MINOS_PATH_BUFFER_MAX+6]; //"mkdir " only requires six more characters
    char *token;
    fbe_u32_t field_i;
    fbe_u32_t core_i;
    FILE *log_p;

    king_minos_result_t baseline_result;
    baseline_result.test_status = KING_MINOS_STATUS_FAILED;

    mut_printf(MUT_LOG_TEST_STATUS, "entering king_minos_compare_and_log_result");
    result->test_status = KING_MINOS_STATUS_NOT_YET_EVALUATED;
    //make directory for result file. If it already exists, it doesn't matter.
    //platform, drive type, raid type, lun count
    mut_printf(MUT_LOG_TEST_STATUS, "Filling path buffer");
#ifdef ALAMOSA_WINDOWS_ENV
    sprintf(path_buffer, "%s%d_Cores_%d_Buses\\%d_speed_%d_GB\\RAID_%d\\%d_LUNs\\",
                            KING_MINOS_RESULT_DIRECTORY_WINDOWS,
                            physical_config->cores,
                            physical_config->bus_count,
                            physical_config->drive_speed,
                            physical_config->drive_capacity/2064888, //1024*1024*1024/520
                            logical_config->raid_type,
                            result->command_ran->target_count);
#else
    sprintf(path_buffer, "%s%d_Cores_%d_Buses/%d_speed_%d_GB/RAID_%d/%d_LUNs/",
                            KING_MINOS_RESULT_DIRECTORY_LINUX,
                            physical_config->cores,
                            physical_config->bus_count,
                            physical_config->drive_speed,
                            (int)(physical_config->drive_capacity/2064888), //1024*1024*1024/520
                            logical_config->raid_type,
                            result->command_ran->target_count);
#endif /* ALAMOSA_WINDOWS_ENV - PATHS */
    
    sprintf(mkdir_buffer, "mkdir %s", path_buffer);
    mut_printf(MUT_LOG_TEST_STATUS, "Trying to create directory: %s", path_buffer);
    system(mkdir_buffer);

    //attempt to open existing logfile
    //determine file name
    sprintf(file_buffer, "%lluKB_%dR_%dW_%s",
	    (unsigned long long)(result->command_ran->io_size/2),
            result->command_ran->read_threads,
            result->command_ran->write_threads,
            FBE_IS_TRUE(result->command_ran->is_sequential) ? KING_MINOS_SEQUENTIAL_TOKEN : KING_MINOS_RANDOM_TOKEN);
    strcat(path_buffer, file_buffer);
    
    mut_printf(MUT_LOG_TEST_STATUS, "Attempting to open log file: %s", path_buffer);
    log_p = fopen(path_buffer, "r+"); //open for read/update. If this returns NULL, the baseline doesn't exist and we create a new one.

    if(!log_p){
        //open new file for writing
        mut_printf(MUT_LOG_TEST_STATUS, "Baseline file not found, creating new baseline at path: %s", path_buffer);
        log_p = fopen(path_buffer,"w");
        MUT_ASSERT_NOT_NULL_MSG(log_p, "Problem creating new baseline file! Test can't proceed.");
        
        //print new header
        //singular fields
        for(field_i = 0; field_i < sizeof(king_minos_single_fields)/32; field_i++) {
            fprintf(log_p, "%s\t", king_minos_single_fields[field_i]);
        }
        //per-core fields
        for(field_i = 0; field_i < sizeof(king_minos_per_core_fields)/32; field_i++) {
            for(core_i = 0; core_i < physical_config->cores; core_i++) {
                fprintf(log_p, "%s(core_%d)\t", king_minos_per_core_fields[field_i], core_i);
            }    
        }
        fprintf(log_p, "\n"); //newline
        
        //pass test,status is "took new baseline"    
        result->test_status = KING_MINOS_STATUS_TOOK_NEW_BASELINE;
        
        //print result line
        king_minos_print_result_to_file(log_p, physical_config, result);
    }
    else{
        //get baseline data and read it into a struct
        mut_printf(MUT_LOG_TEST_STATUS, "Baseline file found!");
        fgets(line_buffer, KING_MINOS_LINE_BUFFER_MAX, log_p); //one extra to skip the header
                                                               
        while(fgets(line_buffer, KING_MINOS_LINE_BUFFER_MAX, log_p)){
            token = strtok(line_buffer,"\t"); //timestamp
            token = strtok(NULL,"\t"); //first test result string

            if(strcmp(token,"BASELINE") == 0) {
                mut_printf(MUT_LOG_TEST_STATUS, "Baseline entry found!");

                baseline_result.test_status = KING_MINOS_STATUS_OLD_BASELINE;
                //singular fields
                //IOPS standard deviation 
                token = strtok(NULL,"\t");
                baseline_result.ios_std_dev = atof(token);
                mut_printf(MUT_LOG_TEST_STATUS, "token: %s atof: %f ios_std_dev:%f", token, atof(token), baseline_result.ios_std_dev);

                //CPU standard deviation
                token = strtok(NULL,"\t");
                baseline_result.cpu_std_dev = atof(token);
                mut_printf(MUT_LOG_TEST_STATUS, "token: %s atof: %f cpu_std_dev:%f", token, atof(token), baseline_result.cpu_std_dev);
          
                //per-core fields
                //IOs per second
                for (core_i = 0; core_i < physical_config->cores; core_i++) {
                    token = strtok(NULL,"\t");
                    baseline_result.ios_per_seconds[core_i] = (fbe_u32_t) atoi(token);
                    mut_printf(MUT_LOG_TEST_STATUS, "token: %s atof: %d ios_per_second:%d", token, (int)atof(token), baseline_result.ios_per_seconds[core_i]);
                }

                //CPU utilization
                for (core_i = 0; core_i < physical_config->cores; core_i++) {
                    token = strtok(NULL,"\t");
                    baseline_result.cpu_utilization[core_i] = (fbe_u32_t) atoi(token);
                     mut_printf(MUT_LOG_TEST_STATUS, "token: %s atof: %d cpu_utilization:%d", token, (int)atof(token), baseline_result.cpu_utilization[core_i]);
                }
            }
        } //keep going until we've found the latest baseline

        //make sure we actually found a valid baseline
        MUT_ASSERT_INT_EQUAL_MSG(KING_MINOS_STATUS_OLD_BASELINE, baseline_result.test_status, "No valid baseline data in file!");
        //actually evaluate
        mut_printf(MUT_LOG_TEST_STATUS, "Evaluating result.");
        king_minos_evaluate(&baseline_result, result, physical_config);

        if (fbe_test_sep_util_get_cmd_option("-take_new_baseline") && result->test_status == KING_MINOS_STATUS_PASSED_WITH_WARNING) {
            result->test_status = KING_MINOS_STATUS_TOOK_NEW_BASELINE;
        }
        //re-open log for appending
        log_p = freopen(path_buffer,"a",log_p);
        MUT_ASSERT_NOT_NULL_MSG(log_p, "Problem reopening log file for appending test results!");  

        //write to log
        king_minos_print_result_to_file(log_p, physical_config, result);
    }

    //close file
    mut_printf(MUT_LOG_TEST_STATUS, "Closing log file: %s", path_buffer);
    MUT_ASSERT_INT_EQUAL_MSG(fclose(log_p), 0, "Problem closing log file!");
    mut_printf(MUT_LOG_TEST_STATUS, "exiting king_minos_compare_and_log_result");
    return;
}

/*!**************************************************************
 * king_minos_execute_cmd_list()
 ****************************************************************
 * @brief
 *  This function executes each command in a list of king_minos_cmd_t, and checks to see if it
 *  measures up to the baseline or not.
 * 
 * @param king_minos_cmd_t **cmd_list: List of of commands generated by king_minos_generate_cmds.
 * 
 * @param king_minos_result_t **result_list: List of result structs allocated in king_minos_test().
 *
 * @param  king_minos_physical_config_t *config: the configuration struct generated
 * by king_minos_detect_configuration().
 * 
 * @param king_minos_logical_config_t *logical_config: the configuration struct modified
 * by king_minos_configure_raid_groups to include information about the logical configuration
 * elements of the array.
 * 
 *
 * @return - None.
 *
 ****************************************************************/
void king_minos_execute_cmd_list(king_minos_cmd_t **cmd_list,
                                         king_minos_physical_config_t *physical_config,
                                         king_minos_logical_config_t *logical_config,
                                         fbe_u32_t *failure_count,
                                         fbe_u32_t *warning_count){
    fbe_u32_t cmd_i;
    king_minos_result_t *temp_result;
    fbe_u32_t cmd_count = king_minos_number_of_benchmark_tasks;

    mut_printf(MUT_LOG_TEST_STATUS, "entering king_minos_execute_cmd_list.");
    if (fbe_test_sep_util_get_cmd_option("-perf_suite_scaleout")) {
        cmd_count = cmd_count * (fbe_u32_t) floor(log(logical_config->luns_bound)/log(KING_MINOS_TEST_AREA_INCREASE_FACTOR));
    }
    //one command on the list is one file. 
    for (cmd_i = 0; cmd_i < cmd_count; cmd_i++) {
        //allocate memory for result struct
        mut_printf(MUT_LOG_TEST_STATUS, "Allocating memory for result struct: %d", cmd_i);
        temp_result = (king_minos_result_t *) fbe_api_allocate_memory(sizeof(king_minos_result_t));
        MUT_ASSERT_NOT_NULL_MSG(temp_result, "Memory allocation failed for result_t!");

        //execute command
        mut_printf(MUT_LOG_TEST_STATUS, "executing command: %d", cmd_i);
        king_minos_execute_cmd(*(cmd_list+cmd_i), temp_result, physical_config, logical_config);

        //Verify if it passed the benchmark.
        mut_printf(MUT_LOG_TEST_STATUS, "logging command: %d", cmd_i);
        king_minos_compare_and_log_result(temp_result, physical_config,logical_config);

        //update error and warning counts, free the result and its associated command
        switch(temp_result->test_status) {
        case KING_MINOS_STATUS_FAILED:
            mut_printf(MUT_LOG_TEST_STATUS,"Regression detected in cmd: %d (luns: %d io_size: %lluKB read: %d write %d sequential:%d)",
                       cmd_i,
                       temp_result->command_ran->target_count,
                       (unsigned long long)(temp_result->command_ran->io_size/2),
                       temp_result->command_ran->read_threads,
                       temp_result->command_ran->write_threads,
                       temp_result->command_ran->is_sequential);
            failure_count++;
            break;
        case KING_MINOS_STATUS_PASSED_WITH_WARNING:
            mut_printf(MUT_LOG_TEST_STATUS,"Abnormally high performance in cmd: %d (luns: %d io_size: %lluKB read: %d write %d sequential:%d)",
                       cmd_i,
                       temp_result->command_ran->target_count,
                       (unsigned long long)(temp_result->command_ran->io_size/2),
                       temp_result->command_ran->read_threads,
                       temp_result->command_ran->write_threads,
                       temp_result->command_ran->is_sequential);
            warning_count++;
            break;
        case KING_MINOS_STATUS_TOOK_NEW_BASELINE:
            mut_printf(MUT_LOG_TEST_STATUS, "New baseline recorded for cmd: %d", cmd_i);
            warning_count++;
            break;
        case KING_MINOS_STATUS_PASSED:
            mut_printf(MUT_LOG_TEST_STATUS, "Test passed - cmd: %d", cmd_i);
            break;
        case KING_MINOS_STATUS_NOT_YET_EVALUATED:
            mut_printf(MUT_LOG_TEST_STATUS, "Test not run - cmd: %d", cmd_i);
            failure_count++;
            break;
        default:
            mut_printf(MUT_LOG_TEST_STATUS, "Error determing test status - cmd: %d", cmd_i);
            failure_count++;
            break;
        }
        fbe_api_free_memory(temp_result->command_ran);
        fbe_api_free_memory(temp_result);
        
        //TODO: if failed, re-run the command, and run VTune. 
    }
    mut_printf(MUT_LOG_TEST_STATUS, "exiting king_minos_execute_cmd_list.");
    return;
}
/*!**************************************************************
 * king_minos_load_config_file()
 ****************************************************************
 * @brief
 *  This function checks for a user-specified IO profile list to use instead of the baseline.
 * 
 *  This file should be located in KING_MINOS_CONFIG_PATH_WINDOWS (C:\kingminos.cfg).
 * 
 *  The format is of this file should be simple CSV, each line is a different test.
 * 
 *  seek,affinity,size,read,write
 * 
 *  seek - 1 for caterpillar, 0 for random
 *  affinity - 1 for per-core, 0 for none
 *  size - in decimal blocks
 *  read/write ratio - 1,0 for pure reads; 0,1 for pure writes; 1,1 for 50/50 mix (if supported), etc
 *
 * @return - None.
 *
 ****************************************************************/

void king_minos_load_config_file(void){
    FILE *cfg_p;
    char line_buffer[KING_MINOS_LINE_BUFFER_MAX];
    char *token;

    cfg_p = fopen(KING_MINOS_CONFIG_PATH_WINDOWS, "r");
    if (cfg_p) {
        mut_printf(MUT_LOG_TEST_STATUS,"Config file exists!");
        fgets(line_buffer,KING_MINOS_LINE_BUFFER_MAX,cfg_p); //chomp header

        if (!cfg_p){
            mut_printf(MUT_LOG_TEST_STATUS, "Config file is empty, using default workload");
            return;
        }
        king_minos_number_of_benchmark_tasks = 0; 
        use_default = FBE_FALSE;
        //read in config file, reset static values that determine test list and size
        while (cfg_p){
            mut_printf(MUT_LOG_TEST_STATUS,"line loop");
            fgets(line_buffer,KING_MINOS_LINE_BUFFER_MAX,cfg_p);
            //seek (1 = sequential/caterpillar, 0 = random)
            token = strtok(line_buffer,",");
            mut_printf(MUT_LOG_TEST_STATUS,"%s",token);
            if (token == NULL) break;
            king_minos_file_task_list[king_minos_number_of_benchmark_tasks].is_sequential = strcmp(token,"seq") ? FBE_FALSE : FBE_TRUE;
            //affinity (1 = affine per core, 0 = no affinity)
            token = strtok(NULL,",");
            mut_printf(MUT_LOG_TEST_STATUS,"%s",token);
            if (token == NULL) break;
            king_minos_file_task_list[king_minos_number_of_benchmark_tasks].affine_per_core = strcmp(token,"affine") ? FBE_FALSE : FBE_TRUE;
            //size (blocks)
            token = strtok(NULL,",");
            mut_printf(MUT_LOG_TEST_STATUS,"%s",token);
            if (token == NULL) break;
            king_minos_file_task_list[king_minos_number_of_benchmark_tasks].io_size = (fbe_u64_t) atoi(token);
            //read ratio
            token = strtok(NULL,",");
            mut_printf(MUT_LOG_TEST_STATUS,"%s",token); 
            if (token == NULL) break;
            king_minos_file_task_list[king_minos_number_of_benchmark_tasks].read_threads = atoi(token);
            //write ratio
            token = strtok(NULL,",");
            mut_printf(MUT_LOG_TEST_STATUS,"%s",token);
            if (token == NULL) break;
            king_minos_file_task_list[king_minos_number_of_benchmark_tasks].write_threads = atoi(token);
            //duration
            token = strtok(NULL,",");
            mut_printf(MUT_LOG_TEST_STATUS,"%s",token);
            if (token == NULL) break;
            king_minos_file_task_list[king_minos_number_of_benchmark_tasks].duration_msec = atoi(token);

            king_minos_number_of_benchmark_tasks++;
        }
        fclose(cfg_p);
    }
    else{ //create config file with header to denote
        cfg_p = fopen(KING_MINOS_CONFIG_PATH_WINDOWS, "w");
        fprintf(cfg_p, "ran/seq, aff/noaff, blocks, read_thds, write_thds, duration\n");
        fclose(cfg_p);
    }
    return;
}

/*!**************************************************************
 * king_minos_test()
 ****************************************************************
 * @brief
 *  Bind as many RAID 5 LUNS as possible with the given hardware,
 *  generate list of IO workloads, perform them and append
 *  the results to a logfile on local storage.
 * 
 *  This is a single SP test.
 *
 * @param  None.            
 *
 * @return - None.
 *
 ****************************************************************/
void king_minos_test(void){
    king_minos_physical_config_t physical_config;
    king_minos_logical_config_t logical_config;
    king_minos_cmd_t **cmd_list;
    fbe_u32_t failure_count = 0;
    fbe_u32_t warning_count = 0;

    //check existence and read contents of config file
    mut_printf(MUT_LOG_TEST_STATUS, "checking for workload list");
    //king_minos_load_config_file(); 
 
    //get configuration
    mut_printf(MUT_LOG_TEST_STATUS, "Detecting hardware configuration.");
    king_minos_detect_configuration(&physical_config);

    //configure groups for RAID 5
    mut_printf(MUT_LOG_TEST_STATUS, "Configuring RAID groups and LUNs: RAID 5");
    king_minos_configure_raid_groups(&physical_config, 
                                     &logical_config, 
                                     FBE_RAID_GROUP_TYPE_RAID5,
                                     FBE_TRUE, 
                                     FBE_TRUE);
    //generate command list
    mut_printf(MUT_LOG_TEST_STATUS, "Generating IO Profile List");
    cmd_list = (king_minos_cmd_t **) fbe_api_allocate_memory(sizeof(king_minos_cmd_t *) * king_minos_number_of_benchmark_tasks * (fbe_u32_t)(log(logical_config.max_raidgroups)/log(KING_MINOS_TEST_AREA_INCREASE_FACTOR)));
    MUT_ASSERT_NOT_NULL_MSG(cmd_list, "Memory allocation failed for cmd_list!");
    king_minos_generate_cmds(&physical_config, &logical_config, cmd_list);

    //run commands
    mut_printf(MUT_LOG_TEST_STATUS, "Running IO workloads...");
    king_minos_execute_cmd_list(cmd_list, &physical_config, &logical_config, &failure_count, &warning_count);
    mut_printf(MUT_LOG_TEST_STATUS, "IO workloads complete. Failures: %d Warnings %d", failure_count, warning_count);

    //fail the test if any tests didn't meet the baseline (within tolerance). 
    MUT_ASSERT_INT_EQUAL_MSG(0, failure_count, "Results detected that were less than the baseline and outside the tolerance threshhold.");
    if(!fbe_test_sep_util_get_cmd_option("-take_new_baseline")) {
        MUT_ASSERT_INT_EQUAL_MSG(0, warning_count, "Results detected in excess of baseline by a significant margin. If these gains are legitimate, re-run with -km_take_new_baseline");
    }

    //free list
    fbe_api_free_memory(cmd_list);
    mut_printf(MUT_LOG_TEST_STATUS, "exiting king_minos_test.");
}

/*!**************************************************************
 * king_aeacus_test()
 ****************************************************************
 * @brief
 *  Same as King Minos, but RAID 0.
 *
 * @param  None.            
 *
 * @return - None.
 *
 ****************************************************************/
void king_aeacus_test(void){
    king_minos_physical_config_t physical_config;
    king_minos_logical_config_t logical_config;
    king_minos_cmd_t **cmd_list;
    fbe_u32_t failure_count = 0;
    fbe_u32_t warning_count = 0;

    //check existence and read contents of config file
    mut_printf(MUT_LOG_TEST_STATUS, "checking for workload list");
    king_minos_load_config_file(); 
 
    //get configuration
    mut_printf(MUT_LOG_TEST_STATUS, "Detecting hardware configuration.");
    king_minos_detect_configuration(&physical_config);


    //configure groups for RAID 0
    mut_printf(MUT_LOG_TEST_STATUS, "Configuring RAID groups and LUNs: RAID 0");
    king_minos_configure_raid_groups(&physical_config, 
                                     &logical_config, 
                                     FBE_RAID_GROUP_TYPE_RAID0,
                                     FBE_TRUE, 
                                     FBE_TRUE);
    //generate command list
    mut_printf(MUT_LOG_TEST_STATUS, "Generating IO Profile List");
    cmd_list = (king_minos_cmd_t **) fbe_api_allocate_memory(sizeof(king_minos_cmd_t *) * king_minos_number_of_benchmark_tasks * (fbe_u32_t)(log(logical_config.max_raidgroups)/log(KING_MINOS_TEST_AREA_INCREASE_FACTOR)));
    MUT_ASSERT_NOT_NULL_MSG(cmd_list, "Memory allocation failed for cmd_list!");
    king_minos_generate_cmds(&physical_config, &logical_config, cmd_list);

    //run commands
    mut_printf(MUT_LOG_TEST_STATUS, "Running IO workloads...");
    king_minos_execute_cmd_list(cmd_list, &physical_config, &logical_config, &failure_count, &warning_count);
    mut_printf(MUT_LOG_TEST_STATUS, "IO workloads complete. Failures: %d Warnings %d", failure_count, warning_count);

    //fail the test if any tests didn't meet the baseline (within tolerance). 
    MUT_ASSERT_INT_EQUAL_MSG(0, failure_count, "Results detected that were less than the baseline and outside the tolerance threshhold.");
    if(!fbe_test_sep_util_get_cmd_option("-take_new_baseline")) {
        MUT_ASSERT_INT_EQUAL_MSG(0, warning_count, "Results detected in excess of baseline by a significant margin. If these gains are legitimate, re-run with -km_take_new_baseline");
    }
    //free list
    fbe_api_free_memory(cmd_list);
    mut_printf(MUT_LOG_TEST_STATUS, "exiting king_aeacus_test.");
}

/*!**************************************************************
 * king_rhadamanthus_test()
 ****************************************************************
 * @brief
 *  Same as King Minos, but with RAID 6
 *
 * @param  None.            
 *
 * @return - None.
 *
 ****************************************************************/
void king_rhadamanthus_test(void){
    king_minos_physical_config_t physical_config;
    king_minos_logical_config_t logical_config;
    king_minos_cmd_t **cmd_list;
    fbe_u32_t failure_count = 0;
    fbe_u32_t warning_count = 0;

    //check existence and read contents of config file
    mut_printf(MUT_LOG_TEST_STATUS, "checking for workload list");
    king_minos_load_config_file(); 
 
    //get configuration
    mut_printf(MUT_LOG_TEST_STATUS, "Detecting hardware configuration.");
    king_minos_detect_configuration(&physical_config);

    //configure groups for RAID 5
    mut_printf(MUT_LOG_TEST_STATUS, "Configuring RAID groups and LUNs: RAID 6");
    king_minos_configure_raid_groups(&physical_config, 
                                     &logical_config, 
                                     FBE_RAID_GROUP_TYPE_RAID6,
                                     FBE_TRUE, 
                                     FBE_TRUE);
    //generate command list
    mut_printf(MUT_LOG_TEST_STATUS, "Generating IO Profile List");
    cmd_list = (king_minos_cmd_t **) fbe_api_allocate_memory(sizeof(king_minos_cmd_t *) * king_minos_number_of_benchmark_tasks * (fbe_u32_t)(log(logical_config.max_raidgroups)/log(KING_MINOS_TEST_AREA_INCREASE_FACTOR)));
    MUT_ASSERT_NOT_NULL_MSG(cmd_list, "Memory allocation failed for cmd_list!");
    king_minos_generate_cmds(&physical_config, &logical_config, cmd_list);

    //run commands
    mut_printf(MUT_LOG_TEST_STATUS, "Running IO workloads...");
    king_minos_execute_cmd_list(cmd_list, &physical_config, &logical_config, &failure_count, &warning_count);
    mut_printf(MUT_LOG_TEST_STATUS, "IO workloads complete. Failures: %d Warnings %d", failure_count, warning_count);

    //fail the test if any tests didn't meet the baseline (within tolerance). 
    MUT_ASSERT_INT_EQUAL_MSG(0, failure_count, "Results detected that were less than the baseline and outside the tolerance threshhold.");
    if(!fbe_test_sep_util_get_cmd_option("-take_new_baseline")) {
        MUT_ASSERT_INT_EQUAL_MSG(0, warning_count, "Results detected in excess of baseline by a significant margin. If these gains are legitimate, re-run with -km_take_new_baseline");
    }
    //free list
    fbe_api_free_memory(cmd_list);
    mut_printf(MUT_LOG_TEST_STATUS, "exiting king_rhadamanthus_test.");
}

/*!**************************************************************
 * king_minos_setup()
 ****************************************************************
 * @brief
 *  Setup for King Minos. Creates physical configuration ands loads
 *  physical package, sep, and neit if in simulation. 
 *
 * @param None.               
 *
 * @return None.   
 *
 * @note    Only single-sp mode is supported
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/
void king_minos_setup(void){
    fbe_sep_package_load_params_t sep_params; 
    fbe_status_t status;
    fbe_u32_t cmd_line_flags = 0;

    //init and start packages if simulation
    fbe_zero_memory(&sep_params, sizeof(fbe_sep_package_load_params_t));
    if (fbe_test_util_is_simulation()) {
        fbe_test_pp_util_create_physical_config_for_disk_counts(KING_MINOS_SIMULATION_DRIVES, //8 enclosures + 0_0
                                                                0,//no 4160-bps drives
                                                                KING_MINOS_SIMULATION_CAPACITY); //600GB drives
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
    else{
       
        status = fbe_api_dps_memory_add_more_memory(FBE_PACKAGE_ID_NEIT);
        //MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,"failed to give NEIT more memory");
        
    }
    fbe_test_sep_util_set_default_io_flags();
    fbe_test_common_util_test_setup_init();
    return;
}

/*!**************************************************************
 * king_minos_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the King_Minos test.
 *
 * @param None.               
 *
 * @return None.
 *
 ****************************************************************/

void king_minos_cleanup(void){
    fbe_status_t status; 

    //notify that logs have been written
    mut_printf(MUT_LOG_TEST_STATUS, "Performance logs written in directory %s",
#ifdef ALAMOSA_WINDOWS_ENV
               KING_MINOS_RESULT_DIRECTORY_WINDOWS
#else
               KING_MINOS_RESULT_DIRECTORY_LINUX
#endif /* ALAMOSA_WINDOWS_ENV - PATHS */
               );

    //destroy config and unload drivers
    // Always clear dualsp mode
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    else{//unbind all luns, destroy all raid groups
        status = fbe_test_sep_util_destroy_all_user_luns();
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_test_sep_util_destroy_all_user_raid_groups();
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    return;
}



