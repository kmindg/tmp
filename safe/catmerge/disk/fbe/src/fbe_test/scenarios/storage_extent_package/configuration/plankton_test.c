
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file plankton_test.c
 ***************************************************************************
 *
 * @brief
 *   This file includes tests for binding new LUNs with the Non-Destructive
 *   Bind (NDB) option.
 *
 * @version
 *   4/2010 - Created.  rundbs
 *
 ***************************************************************************/


/*-----------------------------------------------------------------------------
    INCLUDES OF REQUIRED HEADER FILES:
*/

#include "sep_tests.h"
#include "sep_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "pp_utils.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe_test_esp_utils.h"
#include "fbe_test.h"
#include "fbe_test_configurations.h"
#include "sep_rebuild_utils.h"
#include "fbe/fbe_random.h"

/*-----------------------------------------------------------------------------
    TEST DESCRIPTION:
*/

char * plankton_short_desc = "Non-Destructive Bind";
char * plankton_long_desc =
    "\n"
    "\n"
    " The Plankton Scenario tests the ability to bind a LUN that was previously bound,\n"
    "retaining the previous user data.  In this case LUN zeroing is disabled.\n"
    "\n"
    "Dependencies: \n"
    "\t- The ndb-zero interaction is in place.\n"
    "\n"
    "Starting Config: \n"
    "\t[PP] armada board\n"
    "\t[PP] SAS PMC port\n"
    "\t[PP] viper enclosure\n"
    "\t[PP] fifteen SAS drives \n"
    "\t[PP] fifteen logical drives \n"
    "\t[SEP] fifteen provisioned drives \n"
    "\t[SEP] fifteen virtual drives\n"
    "\t[SEP] two RAID Objects (Raid-5 and Raid-1)\n"
    "\t[SEP] four LUN Objects (two for each Raid Object)\n"
    "\n"
    "STEP 1: Create the initial topology.\n"
    "\t- Create and verify the initial physical package config.\n"
    "\t- Create provisioned drives and attach edges to logical drives.\n"
    "\t- Create virtual drives and attach edges to provisioned drives.\n"
    "\t- Create a five drive raid-5 raid group object and attach edges from raid-5 to the virtual drives.\n"
    "\t- Create and attach two LUNs to the raid-5 object.\n"
    "\t- Create a two drive raid-1 raid group object and attach edges from raid-1 to the virtual drives.\n"
    "\t- Create and attach two LUNs to the raid-1 object.\n"
    "\t- Create and attach a BVD object to the LUNs and export the LUNs.\n"
    "STEP 2: Run IO to the LUNs.\n"
    "\t- Create a known pattern on the LUN by writing to it.\n"
    "\t- Pattern should be written to start middle and end of LBA range.\n"
    "STEP 3: Unbind the LUN.\n"
    "\t- Ask the configuration service to unbind the LUN we wrote to.\n"
    "STEP 4: Re-Bind the LUN.\n"
    "\t- Wait for a while after unbind and ask the configuration service to re-bind the same LUN to the same RAID group.\n"
    "STEP 5: Verify new LUN did not zero previous pattern.\n"
    "\t- Wait for the bind process to end and wait for a while for a potential unintended zero.\n"
    "\t- Read the patterns we wrote on step 2 and verify it’s the same and was not zeroed.\n"
    "\n"
    "*** Plankton Post-Phase 1 ********************************************************* \n"
    "\n"
    "The Plankton Scenario also needs to cover the following cases in follow-on phases:\n"
    "    - Verify a RG object can be recreated along with its underlying objects\n"
    "    - Verify that appropriate error handling is performed for the following cases:\n" 
    "      - SP goes down before LUN signature updated for NDB\n" 
    "\n"
    "Description last updated: 9/27/2011.\n";


/*-----------------------------------------------------------------------------
    DEFINITIONS:
*/

/*!*******************************************************************
 * @def PLANKTON_TEST_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Max number of RGs to create
 *
 *********************************************************************/
#define PLANKTON_TEST_RAID_GROUP_COUNT           1


/*!*******************************************************************
 * @def PLANKTON_TEST_MAX_RG_WIDTH
 *********************************************************************
 * @brief Max number of drives in a RG
 *
 *********************************************************************/
#define PLANKTON_TEST_MAX_RG_WIDTH               16


/*!*******************************************************************
 * @def PLANKTON_TEST_LUN_ELEMENT_SIZE
 *********************************************************************
 * @brief Element size of LUNs
 *
 *********************************************************************/
#define PLANKTON_TEST_LUN_ELEMENT_SIZE           128 

/*!*******************************************************************
 * @def PLANKTON_TEST_LUN_CHUNK_SIZE
 *********************************************************************
 * @brief LUN chunk size
 *
 *********************************************************************/
#define PLANKTON_TEST_LUN_CHUNK_SIZE             0x800

/*!*******************************************************************
 * @def PLANKTON_MAX_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will use for testing.
 *
 *********************************************************************/
#define PLANKTON_MAX_IO_SIZE_BLOCKS             1024


/*!*******************************************************************
 * @def PLANKTON_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects to create
 *        (1 board + 1 port + 4 encl + 60 pdo) 
 *
 *********************************************************************/
#define PLANKTON_TEST_NUMBER_OF_PHYSICAL_OBJECTS 66 


/*!*******************************************************************
 * @def PLANKTON_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive
 *
 *********************************************************************/
#define PLANKTON_TEST_DRIVE_CAPACITY             TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY


/*!*******************************************************************
 * @def PLANKTON_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Max number of LUNs in the RAID Group
 *
 *********************************************************************/
#define PLANKTON_TEST_LUNS_PER_RAID_GROUP        3


/*!*******************************************************************
 * @def PLANKTON_TEST_RAID_GROUP_ID
 *********************************************************************
 * @brief RAID Group id used by this test
 *
 *********************************************************************/
#define PLANKTON_TEST_RAID_GROUP_ID        0


/*!*******************************************************************
 * @def PLANKTON_TEST_NS_TIMEOUT
 *********************************************************************
 * @brief  Notification to timeout value in milliseconds 
 *
 *********************************************************************/
#define PLANKTON_TEST_NS_TIMEOUT        100000 /*wait maximum of 10 seconds*/

/*!*******************************************************************
 * @def  PLANKTON_MAX_RG_SIZE
 *********************************************************************
 * @brief max size of a raid group.
 *
 *********************************************************************/
#define PLANKTON_MAX_RG_SIZE 200 * PLANKTON_TEST_LUN_CHUNK_SIZE

/*!*******************************************************************
 * @def  PLANKTON_TEST_LUN_ID_OFFSET
 *********************************************************************
 * @brief Offset for LUN ID
 *
 *********************************************************************/
#define PLANKTON_TEST_LUN_ID_OFFSET 0xF

/************************************************************************************
 * Components to support Table driven test methodology.
 * Following this approach, the parameters for the test will be stored
 * as rows in a table.
 * The test will then be executed for each row(i.e. set of parameters)
 * in the table.
 ***********************************************************************************/

/* The different test numbers.
 */
typedef enum plankton_test_enclosure_num_e
{
    PLANKTON_TEST_ENCL1_WITH_SAS_DRIVES = 0,
    PLANKTON_TEST_ENCL2_WITH_SAS_DRIVES,
    PLANKTON_TEST_ENCL3_WITH_SAS_DRIVES,
    PLANKTON_TEST_ENCL4_WITH_SAS_DRIVES
} plankton_test_enclosure_num_t;

/* The different drive types.
 */
typedef enum plankton_test_drive_type_e
{
    SAS_DRIVE,
    SATA_DRIVE
} plankton_test_drive_type_t;

/* This is a set of parameters for the Scoobert spare drive selection tests.
 */
typedef struct enclosure_drive_details_s
{
    plankton_test_drive_type_t drive_type; /* Type of the drives to be inserted in this enclosure */
    fbe_u32_t port_number;                    /* The port on which current configuration exists */
    plankton_test_enclosure_num_t  encl_number;      /* The unique enclosure number */
    fbe_block_size_t block_size;
} enclosure_drive_details_t;

/* Table containing actual enclosure and drive details to be inserted.
 */
static enclosure_drive_details_t encl_table[] = 
{
    {   SAS_DRIVE,
        0,
        PLANKTON_TEST_ENCL1_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        PLANKTON_TEST_ENCL2_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        PLANKTON_TEST_ENCL3_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        PLANKTON_TEST_ENCL4_WITH_SAS_DRIVES,
        520
    },
};

/* The different test cases.
 */
typedef enum plankton_test_case_e
{
    PLANKTON_TEST_GREATER_ADDRESS_OFFSET =0,
    PLANKTON_TEST_SMALLER_ADDRESS_OFFSET,
    PLANKTON_TEST_GREATER_CAPACITY,
    PLANKTON_TEST_CORRECT_ADDRESS_OFFSET_AND_CAPACITY,
    PLANKTON_TEST_INVALID
} plankton_test_case_t;

/* Count of rows in the table.
 */
#define PLANKTON_TEST_ENCL_MAX sizeof(encl_table)/sizeof(encl_table[0])

/* LUN info used to create a LUN */
typedef struct plankton_test_create_info_s
{
    fbe_raid_group_type_t           raid_type; 
    fbe_raid_group_number_t         raid_group_id;
    fbe_block_transport_placement_t placement;
    fbe_lun_number_t                wwid;
    fbe_lba_t                       capacity;
    fbe_bool_t                      ndb_b;
    fbe_bool_t                      noinitialverify_b;
    fbe_lba_t                       ndb_addroffset;
    fbe_bool_t                      error_b;
    fbe_job_service_error_type_t    error_code;
} plankton_test_create_info_t;



/*!*******************************************************************
 * @var plankton_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t plankton_raid_group_config[] = 
{
    /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    {3,       PLANKTON_MAX_RG_SIZE,     FBE_RAID_GROUP_TYPE_RAID5,    FBE_CLASS_ID_PARITY,    520,            0,          0},
    {4,       PLANKTON_MAX_RG_SIZE,     FBE_RAID_GROUP_TYPE_RAID6,    FBE_CLASS_ID_PARITY,    520,            0,          0},
    {4,       PLANKTON_MAX_RG_SIZE,     FBE_RAID_GROUP_TYPE_RAID10,   FBE_CLASS_ID_STRIPER,   520,            0,          0},
    {2,       PLANKTON_MAX_RG_SIZE,     FBE_RAID_GROUP_TYPE_RAID1,    FBE_CLASS_ID_MIRROR,    520,            0,          0},
    {3,       PLANKTON_MAX_RG_SIZE,     FBE_RAID_GROUP_TYPE_RAID0,    FBE_CLASS_ID_STRIPER,   520,            0,          0},

     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};



/* Contexts used with rdgen */
static fbe_api_rdgen_context_t plankton_test_rdgen_contexts[PLANKTON_TEST_LUNS_PER_RAID_GROUP];
static fbe_lba_t                   lun_addroffset[PLANKTON_TEST_LUNS_PER_RAID_GROUP];

/*-----------------------------------------------------------------------------
    FORWARD DECLARATIONS:
*/

static void plankton_test_load_physical_config(void);
static void plankton_test_load_logical_config(void);
static fbe_status_t plankton_test_create_lun(plankton_test_create_info_t in_test_lun_create_info);
static void plankton_test_destroy_lun(fbe_lun_number_t);
static void plankton_test_setup_rdgen_test_context( fbe_api_rdgen_context_t* in_context_p,
                                                    fbe_object_id_t in_object_id,
                                                    fbe_class_id_t in_class_id,
                                                    fbe_rdgen_operation_t in_rdgen_operation,
                                                    fbe_lba_t in_capacity,
                                                    fbe_u32_t in_max_passes,
                                                    fbe_u32_t in_threads,
                                                    fbe_u32_t in_io_size_blocks);
void plankton_test_write_background_pattern(fbe_api_rdgen_context_t* in_context_p,
                                            fbe_block_count_t in_element_size, 
                                            fbe_object_id_t in_lun_object_id);
void plankton_test_setup_fill_rdgen_test_context(fbe_api_rdgen_context_t* in_context_p,
                                                 fbe_object_id_t in_object_id,
                                                 fbe_class_id_t in_class_id,
                                                 fbe_rdgen_operation_t in_rdgen_operation,
                                                 fbe_lba_t in_max_lba,
                                                 fbe_u64_t in_io_size_blocks);
void plankton_test_read_background_pattern(fbe_api_rdgen_context_t* in_context_p, 
                                           fbe_block_count_t in_element_size, 
                                           fbe_object_id_t in_lun_object_id);
static fbe_status_t plankton_test_stop_io_load(fbe_api_rdgen_context_t* in_context_p);
void plankton_run_test(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p);
fbe_status_t plankton_test_create_raid_group(fbe_test_rg_configuration_t *rg_config_ptr);
static fbe_status_t plankton_test_get_lun_capacity(fbe_test_rg_configuration_t *rg_config_ptr,
                                                   fbe_lba_t *lun_capacity);
static fbe_status_t plankton_test_run_basic_ndb_test(fbe_test_rg_configuration_t *rg_config_ptr);
static fbe_status_t plankton_test_run_degraded_ndb_test(fbe_test_rg_configuration_t *rg_config_p);
static fbe_status_t plankton_test_run_reboot_ndb_test(fbe_test_rg_configuration_t *rg_config_p);
static void plankton_test_reboot_sp(void); 
static void plankton_test_get_rg_object_id(fbe_test_rg_configuration_t *rg_config_p, fbe_object_id_t *rg_object_id_array);
static fbe_status_t plankton_test_validate_mark_nr(fbe_object_id_t rg_object_id, fbe_u32_t rebuild_index);
static void plankton_test_create_all_luns(fbe_test_rg_configuration_t *rg_config_p, fbe_bool_t do_ndb);
static fbe_status_t plankton_test_destroy_config(fbe_test_rg_configuration_t *rg_config_p);
static fbe_status_t plankton_test_validate_lun_data(void);

/*-----------------------------------------------------------------------------
    FUNCTIONS:
*/


/*!****************************************************************************
 *  plankton_run_test
 ******************************************************************************
 *
 * @brief
 *  This is the main executing function of the Plankton test.  
 *
 *
 * @param   
 *
 * rg_config_ptr    - pointer to raid group 
 * context_pp       - pointer to context
 *
 * @return  None 
 * 
 * @Modified
 * 15/07/2011 - Created. Vishnu Sharma 
 *****************************************************************************/
void plankton_run_test(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_pp)
{
    fbe_u32_t                   num_raid_groups = 0;
    fbe_u32_t                   index = 0;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    num_raid_groups = fbe_test_get_rg_array_length(rg_config_ptr);
    
    mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Plankton Test ===\n");

    for(index = 0; index < num_raid_groups; index++)
    {
        rg_config_p = rg_config_ptr + index;

        if(fbe_test_rg_config_is_enabled(rg_config_p))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Plankton Basic NDB Test ===\n");
            plankton_test_run_basic_ndb_test(rg_config_p);

            mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Plankton Degraded NDB Test ===\n");
            plankton_test_run_degraded_ndb_test(rg_config_p);

            mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Plankton reboot NDB Test ===\n");
            plankton_test_run_reboot_ndb_test(rg_config_p);
        }
            
    }
    return;

} /* End plankton_run_test() */


/*!****************************************************************************
 * plankton_test()
 ******************************************************************************
 * @brief
 *  This is the main entry point for the Plankton test.  
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  15/07/2011 - Created. Vishnu Sharma
 ******************************************************************************/

void plankton_test(void)
{

    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    
    rg_config_p = &plankton_raid_group_config[0];
    
    fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

    
    fbe_test_run_test_on_rg_config_no_create(rg_config_p, 0, plankton_run_test);


    return;

}/* End plankton_test() */

/*!****************************************************************************
 *  plankton_test_setup
 ******************************************************************************
 *
 * @brief
 *  This is the setup function for the Plankton test. It is responsible
 *  for loading the physical and logical configuration for this test suite.
 *
 * @param   None
 *
 * @return  None 
 * @Modified
 * 15/07/2011 - Created. Vishnu Sharma 
 *
 *****************************************************************************/
void plankton_test_setup(void)
{

    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Plankton test ===\n");

    if(fbe_test_util_is_simulation())
    {
        /* Load the physical configuration */
        plankton_test_load_physical_config();

        /* Load the logical configuration */
        plankton_test_load_logical_config();
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();

    fbe_test_wait_for_all_pvds_ready();

    return;

} /* End plankton_test_setup() */


/*!****************************************************************************
 *  plankton_test_cleanup
 ******************************************************************************
 *
 * @brief
 *  This is the cleanup function for the Plankton test.
 *
 * @param   None
 *
 * @return  None
 * @Modified
 * 15/07/2011 - Created. Vishnu Sharma 
 *
 *****************************************************************************/
void plankton_test_cleanup(void)
{  

    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for Plankton test ===\n");

    if(fbe_test_util_is_simulation())
    {
        /* Destroy the test configuration */
        fbe_test_sep_util_destroy_neit_sep_physical();
    }

    return;

} /* End plankton_test_cleanup() */


/*
 *  The following functions are Plankton test functions. 
 *  They test various features of the LUN Object focusing on LUN destroy (unbind).
 */

/*!****************************************************************************
 *  plankton_test_create_lun
 ******************************************************************************
 *
 * @brief
 *   This is the test function for creating a LUN.
 *
 * @param   wwn - name of LUN to create
 *
 * @return  None 
 *
 *****************************************************************************/
static fbe_status_t plankton_test_create_lun(plankton_test_create_info_t in_test_lun_create_info)
{
    fbe_status_t                        status;
    fbe_api_lun_create_t                fbe_lun_create_req;
    fbe_object_id_t                     object_id = FBE_OBJECT_ID_INVALID;
    fbe_job_service_error_type_t        job_error_type;
   
    /* Create a LUN */ 
    fbe_zero_memory(&fbe_lun_create_req, sizeof(fbe_api_lun_create_t));
    fbe_lun_create_req.raid_type        = in_test_lun_create_info.raid_type;
    fbe_lun_create_req.raid_group_id    = in_test_lun_create_info.raid_group_id;
    fbe_lun_create_req.lun_number       = in_test_lun_create_info.wwid;
    fbe_lun_create_req.capacity         = in_test_lun_create_info.capacity;
    fbe_lun_create_req.placement        = in_test_lun_create_info.placement;
    fbe_lun_create_req.ndb_b            = in_test_lun_create_info.ndb_b;
    fbe_lun_create_req.noinitialverify_b = in_test_lun_create_info.noinitialverify_b;
    fbe_lun_create_req.addroffset       = in_test_lun_create_info.ndb_addroffset;
    fbe_lun_create_req.world_wide_name.bytes[0] = fbe_random() & 0xf;
    fbe_lun_create_req.world_wide_name.bytes[1] = fbe_random() & 0xf;

    status = fbe_api_create_lun(&fbe_lun_create_req, FBE_TRUE, (PLANKTON_TEST_NS_TIMEOUT +  20000), &object_id, &job_error_type);
    
    if (in_test_lun_create_info.error_b == FALSE)
    {
        /* Make sure the LUN Object created successfully */
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, object_id);
 
        mut_printf(MUT_LOG_TEST_STATUS, "=== %s: LUN 0x%X created successfully! ===\n", __FUNCTION__,object_id);
    }
    else
    {
        /* Make sure the LUN Object not created */
        MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_EQUAL(FBE_OBJECT_ID_INVALID, object_id);
    }

    return status;
 
}  /* End plankton_test_create_lun() */


/*!****************************************************************************
 *  plankton_test_destroy_lun
 ******************************************************************************
 *
 * @brief
 *   This is the test function for destroying a LUN.  
 *
 * @param   wwn - name of LUN to destroy
 *
 * @return  None 
 *
 *****************************************************************************/
static void plankton_test_destroy_lun(fbe_lun_number_t   in_lun_number)
{
    fbe_status_t                                status;
    fbe_api_lun_destroy_t                       fbe_lun_destroy_req;
    fbe_job_service_error_type_t                job_error_type;
   
   /* Destroy a LUN */
    fbe_lun_destroy_req.lun_number = in_lun_number;

     status = fbe_api_destroy_lun(&fbe_lun_destroy_req, FBE_TRUE, 100000, &job_error_type);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
   
    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN destroyed successfully! ===\n");
    
    return;

}  /* End plankton_test_destroy_lun() */


/*
 * The following functions are utility functions used by this test suite
 */


/*!**************************************************************
 * plankton_test_load_physical_config()
 ****************************************************************
 * @brief
 *  Configure the Plankton test physical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/

static void plankton_test_load_physical_config(void)
{

    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                               total_objects = 0;
    fbe_class_id_t                          class_id;
    fbe_api_terminator_device_handle_t      port0_handle;
    fbe_api_terminator_device_handle_t      port0_encl_handle[PLANKTON_TEST_ENCL_MAX];
    fbe_api_terminator_device_handle_t      drive_handle;
    fbe_u32_t                               slot;
    fbe_u32_t                               enclosure;


    mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== configuring terminator ===\n");
    /* before loading the physical package we initialize the terminator */
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Insert a board
     */
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Insert a port
     */
    fbe_test_pp_util_insert_sas_pmc_port(   1, /* io port */
                                            2, /* portal */
                                            0, /* backend number */ 
                                            &port0_handle);

    /* Insert enclosures to port 0
     */
    for ( enclosure = 0; enclosure < PLANKTON_TEST_ENCL_MAX; enclosure++)
    { 
        fbe_test_pp_util_insert_viper_enclosure(0,
                                                enclosure,
                                                port0_handle,
                                                &port0_encl_handle[enclosure]);
    }

    /* Insert drives for enclosures.
     */
    for ( enclosure = 0; enclosure < PLANKTON_TEST_ENCL_MAX; enclosure++)
    {
        for ( slot = 0; slot < 15; slot++)
        {
            fbe_test_pp_util_insert_sas_drive(encl_table[enclosure].port_number,
                                              encl_table[enclosure].encl_number,
                                              slot,
                                              encl_table[enclosure].block_size,
                                              TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY,
                                              &drive_handle);
        }
    }

    mut_printf(MUT_LOG_TEST_STATUS, "=== starting physical package===\n");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "=== set physical package entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Init fbe api on server */
    mut_printf(MUT_LOG_TEST_STATUS, "=== starting Server FBE_API ===\n");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(PLANKTON_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 60000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== verifying configuration ===\n");

    /* Inserted a armada board so we check for it;
     * board is always assumed to be object id 0
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying board type....Passed");

    /* Make sure we have the expected number of objects.
     */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == PLANKTON_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");

    return;

} /* End plankton_test_load_physical_config() */


/*!**************************************************************
 * plankton_test_load_logical_config()
 ****************************************************************
 * @brief
 *  Configure the Plankton test logical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/

static void plankton_test_load_logical_config(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Logical Configuration ===\n");

    sep_config_load_sep_and_neit();

    mut_printf(MUT_LOG_TEST_STATUS, "%s: END\n", __FUNCTION__);

    return;

} /* End plankton_test_load_logical_config() */


/*!**************************************************************
 * plankton_test_create_raid_group()
 ****************************************************************
 * @brief
 *  To create a raid group.
 *
 * @param 
 * 
 * rg_config_ptr       - Pointer to raid group configuration
 * @return status.
 *
 * @author
 *  07/15/2011 - Created. Vishnu Sharma
 *
 ****************************************************************/


fbe_status_t plankton_test_create_raid_group(fbe_test_rg_configuration_t *rg_config_ptr)
{
    fbe_status_t                                        status;
    fbe_object_id_t                                     rg_object_id;
    fbe_u32_t                                           iter;
    fbe_api_job_service_raid_group_create_t             fbe_raid_group_create_req;
    fbe_job_service_error_type_t                        job_error_code;
    fbe_status_t                                        job_status;

    /* Initialize local variables */
    rg_object_id        = FBE_OBJECT_ID_INVALID;

    
    /* Create the SEP objects for the configuration */

    /* Create a RG */
    mut_printf(MUT_LOG_TEST_STATUS, "=== create a RAID Group ===\n");

    fbe_zero_memory(&fbe_raid_group_create_req, sizeof(fbe_api_job_service_raid_group_create_t));
    fbe_raid_group_create_req.b_bandwidth = 0;
    fbe_raid_group_create_req.capacity = rg_config_ptr->capacity;
    //fbe_raid_group_create_req.explicit_removal = 0;
    fbe_raid_group_create_req.power_saving_idle_time_in_seconds = FBE_LUN_DEFAULT_POWER_SAVE_IDLE_TIME;
    fbe_raid_group_create_req.is_private = FBE_TRI_FALSE;
    fbe_raid_group_create_req.max_raid_latency_time_is_sec = FBE_LUN_MAX_LATENCY_TIME_DEFAULT;
    fbe_raid_group_create_req.raid_group_id = rg_config_ptr->raid_group_id;
    fbe_raid_group_create_req.raid_type = rg_config_ptr->raid_type;
    fbe_raid_group_create_req.drive_count = rg_config_ptr->width;
    fbe_raid_group_create_req.wait_ready  = FBE_FALSE;
    fbe_raid_group_create_req.ready_timeout_msec = PLANKTON_TEST_NS_TIMEOUT;

    for (iter = 0; iter < rg_config_ptr->width; ++iter)    {
        fbe_raid_group_create_req.disk_array[iter].bus = rg_config_ptr->rg_disk_set[iter].bus;
        fbe_raid_group_create_req.disk_array[iter].enclosure = rg_config_ptr->rg_disk_set[iter].enclosure;
        fbe_raid_group_create_req.disk_array[iter].slot = rg_config_ptr->rg_disk_set[iter].slot;
    }
    do 
    {
        status = fbe_api_job_service_raid_group_create(&fbe_raid_group_create_req);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "=== Job_service_raid group_create request sent successfully! ===\n");
        mut_printf(MUT_LOG_TEST_STATUS, "Job number 0x%llX\n", fbe_raid_group_create_req.job_number);

        /* wait for notification */
        status = fbe_api_common_wait_for_job(fbe_raid_group_create_req.job_number,
                                             PLANKTON_TEST_NS_TIMEOUT,
                                             &job_error_code,
                                             &job_status,
                                             NULL);
        if ((status == FBE_STATUS_OK) && (job_error_code == FBE_JOB_SERVICE_ERROR_BAD_PVD_CONFIGURATION))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "PVD is not ready.  Let's retry...\n");
        }
    }
    while ((status == FBE_STATUS_OK) && (job_error_code == FBE_JOB_SERVICE_ERROR_BAD_PVD_CONFIGURATION));

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(
            fbe_raid_group_create_req.raid_group_id, &rg_object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* Verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);
    
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group object %d\n", rg_object_id);
    
    mut_printf(MUT_LOG_TEST_STATUS, "%s: END\n", __FUNCTION__);

    return FBE_STATUS_OK;

}


/*!**************************************************************
 * plankton_test_setup_rdgen_test_context()
 ****************************************************************
 * @brief
 *  Setup the context for a rdgen operation.  This operation
 *  will sweep over an area with a given capacity using
 *  i/os of the same size.
 *
 * @param context_p - Context structure to setup.  
 * @param object_id - object id to run io against.
 * @param class_id - class id to test.
 * @param rdgen_operation - operation to start.
 * @param max_lba - largest lba to test with in blocks.
 * @param io_size_blocks - io size to fill with in blocks.             
 *
 * @return None.
 *
 ****************************************************************/

static void plankton_test_setup_rdgen_test_context( fbe_api_rdgen_context_t* in_context_p,
                                                    fbe_object_id_t in_object_id,
                                                    fbe_class_id_t in_class_id,
                                                    fbe_rdgen_operation_t in_rdgen_operation,
                                                    fbe_lba_t in_capacity,
                                                    fbe_u32_t in_max_passes,
                                                    fbe_u32_t in_threads,
                                                    fbe_u32_t in_io_size_blocks)
{
    fbe_status_t status;

    status = fbe_api_rdgen_test_context_init(   in_context_p, 
                                                in_object_id, 
                                                in_class_id,
                                                FBE_PACKAGE_ID_SEP_0,
                                                in_rdgen_operation,
                                                FBE_RDGEN_PATTERN_LBA_PASS,
                                                in_max_passes,
                                                0, /* io count not used */
                                                0, /* time not used*/
                                                in_threads,
                                                FBE_RDGEN_LBA_SPEC_RANDOM,
                                                0,    /* Start lba*/
                                                0,    /* Min lba */
                                                in_capacity, /* max lba */
                                                FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                                1,    /* Min blocks */
                                                in_io_size_blocks  /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;

} /* End plankton_test_setup_rdgen_test_context() */


/*!**************************************************************
 * plankton_test_write_background_pattern()
 ****************************************************************
 * @brief
 *  Seed the LUN with a pattern using rdgen.
 *
 * @param element_size - element size.              
 * @param lun_object_id - object ID of LUN to issue I/O to.              
 *
 * @return None.
 *
 ****************************************************************/

void plankton_test_write_background_pattern(fbe_api_rdgen_context_t* in_context_p, 
                                            fbe_block_count_t in_element_size, 
                                            fbe_object_id_t in_lun_object_id)
{
    fbe_status_t status;

    /* Write a background pattern to the LUN
     */
    plankton_test_setup_fill_rdgen_test_context(in_context_p,
                                                in_lun_object_id,
                                                FBE_CLASS_ID_INVALID,
                                                FBE_RDGEN_OPERATION_WRITE_ONLY,
                                                FBE_LBA_INVALID, /* use max capacity */
                                                in_element_size);
    status = fbe_api_rdgen_test_context_run_all_tests(in_context_p, FBE_PACKAGE_ID_NEIT, 1);
    
    /* Make sure there were no errors
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(in_context_p->start_io.statistics.error_count, 0);

    return;

} /* end plankton_test_write_background_pattern() */


/*!**************************************************************
 * plankton_test_read_background_pattern()
 ****************************************************************
 * @brief
 *  Read background pattern from LUN using rdgen.
 *
 * @param element_size - element size.              
 * @param lun_object_id - object ID of LUN to issue I/O to.              
 *
 * @return None.
 *
 ****************************************************************/

void plankton_test_read_background_pattern(fbe_api_rdgen_context_t* in_context_p, 
                                           fbe_block_count_t in_element_size, 
                                           fbe_object_id_t in_lun_object_id)
{
    fbe_status_t status;


    /* Read back the pattern and check it was written OK
     */
    plankton_test_setup_fill_rdgen_test_context(in_context_p,
                                                in_lun_object_id,
                                                FBE_CLASS_ID_INVALID,
                                                FBE_RDGEN_OPERATION_READ_CHECK,
                                                FBE_LBA_INVALID, /* use max capacity */
                                                in_element_size);
    status = fbe_api_rdgen_test_context_run_all_tests(in_context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(in_context_p->start_io.statistics.error_count, 0);

    return;

} /* end plankton_test_read_background_pattern() */


/*!**************************************************************
 * plankton_test_setup_fill_rdgen_test_context()
 ****************************************************************
 * @brief
 *  Setup the context for a rdgen fill operation. This operation
 *  will sweep over an area with a given capacity using
 *  i/os of the same size.
 *
 * @param context_p - Context structure to setup.  
 * @param object_id - object id to run io against.
 * @param class_id - class id to test.
 * @param rdgen_operation - operation to start.
 * @param max_lba - largest lba to test with in blocks.
 * @param io_size_blocks - io size to fill with in blocks.             
 *
 * @return None.
 *
 ****************************************************************/

void plankton_test_setup_fill_rdgen_test_context(   fbe_api_rdgen_context_t* in_context_p,
                                                    fbe_object_id_t in_object_id,
                                                    fbe_class_id_t in_class_id,
                                                    fbe_rdgen_operation_t in_rdgen_operation,
                                                    fbe_lba_t in_max_lba,
                                                    fbe_u64_t in_io_size_blocks)
{
    fbe_status_t status;

    status = fbe_api_rdgen_test_context_init(   in_context_p, 
                                                in_object_id,
                                                in_class_id, 
                                                FBE_PACKAGE_ID_SEP_0,
                                                in_rdgen_operation,
                                                FBE_RDGEN_PATTERN_LBA_PASS,
                                                1,    /* We do one full sequential pass. */
                                                0,    /* num ios not used */
                                                0,    /* time not used*/
                                                1,    /* threads */
                                                FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                                0,    /* Start lba*/
                                                0,    /* Min lba */
                                                in_max_lba,    /* max lba */
                                                FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE,
                                                16,    /* Number of stripes to write. */
                                                in_io_size_blocks    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;

} /* end plankton_test_setup_fill_rdgen_test_context() */


/*!**************************************************************
 * plankton_test_stop_io_load()
 ****************************************************************
 * @brief
 *  Stop rdgen I/O tests.
 *
 * @param in_context_p - Context structure for I/O to stop.              
 *
 * @return fbe_status_t
 *
 ****************************************************************/

static fbe_status_t plankton_test_stop_io_load(fbe_api_rdgen_context_t* in_context_p)
{
    fbe_status_t                status;


    /* Halt I/O; note that some I/O should have failed because the LUN
     * was being destroyed while I/O was in progress.
     */
    status = fbe_api_rdgen_stop_tests(in_context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(in_context_p->start_io.statistics.error_count, 0);
    
    return FBE_STATUS_OK;

} /* End plankton_test_stop_io_load() */

/*!**************************************************************
 * plankton_test_get_lun_capacity()
 ****************************************************************
 * @brief
 *  This function returns the per LUN capacity given the RG
 *
 * @param rg_config_ptr - Raid Group Config
 * @param lun_capacity - Buffer to store the lun capacity              
 *
 * @return fbe_status_t
 *
 ****************************************************************/
static fbe_status_t plankton_test_get_lun_capacity(fbe_test_rg_configuration_t *rg_config_p,
                                                   fbe_lba_t *lun_capacity)
{
    fbe_status_t status;
    fbe_object_id_t rg_object_id;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_lba_t rg_capacity;
    fbe_api_lun_calculate_capacity_info_t capacity_info;

    /* Get the rg object ID */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* Now get the RG capacity */
    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    rg_capacity = rg_info.capacity;

    rg_capacity = FBE_MIN(rg_capacity, PLANKTON_MAX_RG_SIZE);
    if (rg_capacity != 0)
    {
        // Calculate the LUN size:
        capacity_info.imported_capacity = rg_capacity/PLANKTON_TEST_LUNS_PER_RAID_GROUP;
        capacity_info.lun_align_size = rg_info.lun_align_size;
        status = fbe_api_lun_calculate_exported_capacity(&capacity_info);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        *lun_capacity = capacity_info.exported_capacity;
    }
    else
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

static fbe_status_t plankton_test_run_basic_ndb_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                status;
    plankton_test_create_info_t test_lun_create_info[PLANKTON_TEST_LUNS_PER_RAID_GROUP];
    fbe_object_id_t             lun_object_id[PLANKTON_TEST_LUNS_PER_RAID_GROUP];
    fbe_database_lun_info_t lun_info[PLANKTON_TEST_LUNS_PER_RAID_GROUP];
    fbe_u32_t                   lun_index = 0;
    fbe_lba_t                   lun_capacity;
    fbe_api_rdgen_get_stats_t   rdgen_stats;
    fbe_rdgen_filter_t          filter;
    fbe_api_rdgen_context_t*    context_p = &plankton_test_rdgen_contexts[0];
    fbe_u32_t                   ith_test = 0;
    
    /*Create the RAID group*/
    status = plankton_test_create_raid_group(rg_config_p);         
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = plankton_test_get_lun_capacity(rg_config_p, &lun_capacity);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Create LUNs */
    for (lun_index = 0; lun_index < PLANKTON_TEST_LUNS_PER_RAID_GROUP; lun_index++)
    {
        test_lun_create_info[lun_index].raid_type = rg_config_p->raid_type;
        test_lun_create_info[lun_index].raid_group_id = rg_config_p->raid_group_id;
        test_lun_create_info[lun_index].wwid = 0xF + lun_index;
        test_lun_create_info[lun_index].capacity = lun_capacity;
        test_lun_create_info[lun_index].placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
        test_lun_create_info[lun_index].ndb_b = FBE_FALSE;
        test_lun_create_info[lun_index].noinitialverify_b = FBE_FALSE;
        test_lun_create_info[lun_index].ndb_addroffset = FBE_LBA_INVALID; 
        test_lun_create_info[lun_index].error_b = FALSE;
        test_lun_create_info[lun_index].error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

        plankton_test_create_lun(test_lun_create_info[lun_index]);

        /* Get object ID for LUN just created */
        status = fbe_api_database_lookup_lun_by_number(test_lun_create_info[lun_index].wwid, &lun_object_id[lun_index]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Get LUN info */
        lun_info[lun_index].lun_object_id = lun_object_id[lun_index];
        status = fbe_api_database_get_lun_info(&lun_info[lun_index]);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        /* Get address offset of LUN just created; will need it later */
        lun_addroffset[lun_index] = lun_info[lun_index].offset;
        

        /* Confirm capacity is correct */
        MUT_ASSERT_INTEGER_EQUAL(lun_info[lun_index].capacity, lun_capacity);

        /* verify the lun connect to BVD in reasonable time. */
        status = fbe_test_sep_util_wait_for_LUN_edge_on_all_sps(FBE_FALSE,
                                                                lun_info[lun_index].lun_object_id, 
                                                                10000);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    }


    /* Perform test on each LUN */
    for (lun_index=0; lun_index<PLANKTON_TEST_LUNS_PER_RAID_GROUP; lun_index++)
    {
        context_p = &(plankton_test_rdgen_contexts[lun_index]);

        mut_printf(MUT_LOG_TEST_STATUS, "Running IO tests on RG Type:0x%x RG ID:%d LUN Obj:0x%x LUN ID:%d!\n", 
                   rg_config_p->raid_type, rg_config_p->raid_group_id, lun_object_id[lun_index], 
                   test_lun_create_info[lun_index].wwid);

        /* Start I/O to LUN */
        /* First, write a background pattern */
        plankton_test_write_background_pattern(context_p, PLANKTON_TEST_LUN_ELEMENT_SIZE, lun_object_id[lun_index]);

        /* Read back the background pattern to ensure no errors */
        plankton_test_read_background_pattern(context_p, PLANKTON_TEST_LUN_ELEMENT_SIZE, lun_object_id[lun_index]);

        /* Set context for issuing reads forever */
        plankton_test_setup_rdgen_test_context( context_p,
                                                lun_object_id[lun_index],
                                                FBE_CLASS_ID_INVALID,
                                                FBE_RDGEN_OPERATION_READ_ONLY,
                                                FBE_LBA_INVALID,    /* use capacity */
                                                0,                  /* run forever*/ 
                                                3,                  /* threads per lun */
                                                PLANKTON_MAX_IO_SIZE_BLOCKS);

        /* Run I/O test */
        status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Let I/O run for a few seconds */
        EmcutilSleep(5000);

        /* Make sure I/O progressing okay */
        fbe_api_rdgen_filter_init(&filter, FBE_RDGEN_FILTER_TYPE_INVALID, 
                                  FBE_OBJECT_ID_INVALID, 
                                  FBE_CLASS_ID_INVALID, 
                                  FBE_PACKAGE_ID_INVALID, 0 /* edge index */);
        status = fbe_api_rdgen_get_stats(&rdgen_stats, &filter); 
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(rdgen_stats.error_count, 0);

        /* Destroy one of the LUNs we just created.
         */
        plankton_test_destroy_lun(test_lun_create_info[lun_index].wwid);

        /* Stop I/O  */
        status = fbe_api_rdgen_stop_tests(context_p, 1);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        // Geng: if you are lucky, you might not meet error IOs
        // MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.statistics.error_count,0);

        for (ith_test=0; ith_test<PLANKTON_TEST_INVALID; ith_test++)
        {
            switch (ith_test)
            {
                /* Try to recreate LUN with incorrect address offset i.e. adress offset is greater 
                 * than correct address offset.
                 */
                case PLANKTON_TEST_GREATER_ADDRESS_OFFSET:
                    {
                        test_lun_create_info[lun_index].ndb_addroffset = lun_addroffset[lun_index] + 0x4000;
                        test_lun_create_info[lun_index].capacity = lun_info[lun_index].capacity;

                        test_lun_create_info[lun_index].error_b = TRUE;
                        test_lun_create_info[lun_index].error_code = FBE_JOB_SERVICE_ERROR_INVALID_ADDRESS_OFFSET;
                    }
                    break;

                    /* Try to recreate LUN with incorrect address offset i.e. adress offset is smaller 
                     * than correct address offset.
                     */
                case PLANKTON_TEST_SMALLER_ADDRESS_OFFSET:
                    {
                        test_lun_create_info[lun_index].ndb_addroffset = lun_addroffset[lun_index] - 0x4000;
                        test_lun_create_info[lun_index].capacity = lun_info[lun_index].capacity;

                        test_lun_create_info[lun_index].error_b = TRUE;
                        test_lun_create_info[lun_index].error_code = FBE_JOB_SERVICE_ERROR_INVALID_ADDRESS_OFFSET;
                    }
                    break;

                    /* Try to recreate LUN with incorrect LUN capacity i.e. capacity is greater 
                     * than original capacity.
                     */
                case PLANKTON_TEST_GREATER_CAPACITY:
                    {
                        test_lun_create_info[lun_index].capacity = lun_info[lun_index].capacity + 0x4000;
                        test_lun_create_info[lun_index].ndb_addroffset = lun_addroffset[lun_index];

                        test_lun_create_info[lun_index].error_b = TRUE;
                        test_lun_create_info[lun_index].error_code = FBE_JOB_SERVICE_ERROR_INVALID_CAPACITY;
                    }
                    break;

                    /* Try to recreate LUN with correct address offset & LUN capacity.
                     */
                case PLANKTON_TEST_CORRECT_ADDRESS_OFFSET_AND_CAPACITY:
                    {
                        test_lun_create_info[lun_index].ndb_addroffset = lun_addroffset[lun_index];
                        test_lun_create_info[lun_index].capacity = lun_info[lun_index].capacity;

                        test_lun_create_info[lun_index].error_b = FALSE;
                        test_lun_create_info[lun_index].error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
                    }
                    break;

                default:
                    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Invalid Case! %d \n", __FUNCTION__, ith_test);
                    continue;
            }

            /* Recreate LUN using NDB option and specified address offset and capacity */
            test_lun_create_info[lun_index].ndb_b = TRUE;
            status = plankton_test_create_lun(test_lun_create_info[lun_index]);

            if (status == FBE_STATUS_OK)
            {
                /* Get object ID for LUN just created */
                status = fbe_api_database_lookup_lun_by_number(test_lun_create_info[lun_index].wwid, &lun_object_id[lun_index]);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                /* Check the lifecycle state before issuing a read since the lun might not be connected to BVD object yet
                   before you issue a read and sometimes the test might fail.
                */
                status = fbe_api_wait_for_object_lifecycle_state(lun_object_id[lun_index], FBE_LIFECYCLE_STATE_READY,
                                                                 20000, FBE_PACKAGE_ID_SEP_0);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                /* Read test pattern from LUN; should be the same */
                plankton_test_read_background_pattern(context_p, PLANKTON_TEST_LUN_ELEMENT_SIZE, lun_object_id[lun_index]);
            }
        }
    }

    mut_printf(MUT_LOG_TEST_STATUS, "Running LUN Placement tests on RG Type:0x%x RG ID:%d LUN Obj:0x%x LUN ID:%d!\n", 
                           rg_config_p->raid_type, rg_config_p->raid_group_id, lun_object_id[1], 
                           test_lun_create_info[1].wwid);

    /* Destroy one of the LUNs we just created. Let's also destroy the LUN right before it,
     * to verify that our requested placement is respected.
     */
    plankton_test_destroy_lun(test_lun_create_info[0].wwid);
    plankton_test_destroy_lun(test_lun_create_info[1].wwid);

    /* Recreate LUN using NDB option and specify the address offset */
    test_lun_create_info[1].ndb_b = TRUE;
    test_lun_create_info[1].ndb_addroffset = lun_addroffset[1];
    test_lun_create_info[1].error_b = FALSE;
    test_lun_create_info[1].error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    plankton_test_create_lun(test_lun_create_info[1]);

    /* Get object ID for LUN just created */
    status = fbe_api_database_lookup_lun_by_number(test_lun_create_info[1].wwid, &lun_object_id[1]);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Make sure that our requested offset was respected */
    lun_info[1].lun_object_id = lun_object_id[1];
    status = fbe_api_database_get_lun_info(&lun_info[1]);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL((int)lun_info[1].offset, (int)test_lun_create_info[1].ndb_addroffset);

    /* Check the lifecycle state before issuing a read since the lun might not be connected to BVD object yet
       before you issue a read and sometimes the test might fail.
     */
    status = fbe_api_wait_for_object_lifecycle_state(lun_object_id[1], FBE_LIFECYCLE_STATE_READY,
                                                     20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Destroy All LUNs */
    for (lun_index=1; lun_index<PLANKTON_TEST_LUNS_PER_RAID_GROUP; lun_index++)
    {
        plankton_test_destroy_lun(test_lun_create_info[lun_index].wwid);
    }

    /* DEstroy a RAID group with user provided configuration. */
    status = fbe_test_sep_util_destroy_raid_group(rg_config_p->raid_group_id);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Destroy RAID group failed.");
    return status;
}

/*!****************************************************************************
 * plankton_test_run_degraded_ndb_test()
 ******************************************************************************
 * @brief
 *  This test case validates that we can mark drive(s) for NR and then do an NDB
 * and the data on the LUNs is still correct
 *
 * @param rg_config_p - RG configuration Information
 * 
 *
 * @return fbe_status_t.
 *
 * @author
 *  06/22/2012 - Created. Ashok Tamilarasan
 ******************************************************************************/
static fbe_status_t plankton_test_run_degraded_ndb_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                status;
    fbe_object_id_t             rg_object_id[2];
    
    if(rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)
    {
        /* Cannot do degraded on raid 0*/
        return FBE_STATUS_OK;
    }
    /*Create the RAID group*/
    status = plankton_test_create_raid_group(rg_config_p);         
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    plankton_test_get_rg_object_id(rg_config_p, &rg_object_id[0]);
    fbe_api_base_config_disable_background_operation(rg_object_id[0], FBE_RAID_GROUP_BACKGROUND_OP_ALL);
    fbe_api_sleep(5000);
    status = fbe_api_raid_group_force_mark_nr(rg_object_id[0], 
                                              rg_config_p->rg_disk_set[0].bus,
                                              rg_config_p->rg_disk_set[0].enclosure,
                                              rg_config_p->rg_disk_set[0].slot);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    plankton_test_validate_mark_nr(rg_object_id[0], 0);

    /* For R10 we can test marking one drive from both the Mirrors */
    if(rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        fbe_api_base_config_disable_background_operation(rg_object_id[1], FBE_RAID_GROUP_BACKGROUND_OP_ALL);
        status = fbe_api_raid_group_force_mark_nr(rg_object_id[1], 
                                                  rg_config_p->rg_disk_set[2].bus,
                                                  rg_config_p->rg_disk_set[2].enclosure,
                                                  rg_config_p->rg_disk_set[2].slot);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        plankton_test_validate_mark_nr(rg_object_id[1], 0);
    }
    else if(rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6)
    {
        /* RAID 6 can have two drives marked for NR. So mark the second one here */
        status = fbe_api_raid_group_force_mark_nr(rg_object_id[0], 
                                                  rg_config_p->rg_disk_set[1].bus,
                                                  rg_config_p->rg_disk_set[1].enclosure,
                                                  rg_config_p->rg_disk_set[1].slot);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        plankton_test_validate_mark_nr(rg_object_id[0], 1);
    }

    plankton_test_create_all_luns(rg_config_p, FBE_TRUE);

    plankton_test_validate_lun_data();

    plankton_test_destroy_config(rg_config_p);
    
    return status;
}

/*!****************************************************************************
 * plankton_test_run_reboot_ndb_test()
 ******************************************************************************
 * @brief
 *  This test case validates that we can reboot the SP after the destroying the 
 * configuration and the recreate with NDB option the data is still preserved
 *
 * @param rg_config_p - RG configuration Information
 * 
 *
 * @return fbe_status_t.
 *
 * @author
 *  06/22/2012 - Created. Ashok Tamilarasan
 ******************************************************************************/
static fbe_status_t plankton_test_run_reboot_ndb_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                status;
    
    plankton_test_reboot_sp();

    /*Create the RAID group*/
    status = plankton_test_create_raid_group(rg_config_p);         
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    plankton_test_create_all_luns(rg_config_p, FBE_TRUE);

    plankton_test_validate_lun_data();

    plankton_test_destroy_config(rg_config_p);
    return status;
}

static void plankton_test_reboot_sp() 
{
    fbe_status_t status;

   // Reboot SEP.
    mut_printf(MUT_LOG_TEST_STATUS, "%s Rebooting SEP", __FUNCTION__);    
    fbe_test_sep_util_destroy_neit_sep();
    sep_config_load_sep_and_neit();
    status = fbe_test_sep_util_wait_for_database_service(30000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

/*!****************************************************************************
 * plankton_test_get_rg_object_id()
 ******************************************************************************
 * @brief
 *  Get the object ID for the specified RG Type. For R10 it returns the object 
 *  IDs of the Mirror below
 *
 * @param rg_config_p - RG configuration Information
 * @param rg_object_id_array - Buffer to store the list of object IDs
 *
 * @return fbe_status_t.
 *
 * @author
 *  06/22/2012 - Created. Ashok Tamilarasan
 ******************************************************************************/
static void plankton_test_get_rg_object_id(fbe_test_rg_configuration_t *rg_config_p, 
                                           fbe_object_id_t *rg_object_id_array)
{
    fbe_object_id_t rg_object_id;

    /* Get the rg object ID */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        fbe_u32_t index;
        fbe_api_base_config_downstream_object_list_t downstream_object_list;

        fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

        for (index = 0; 
             index < downstream_object_list.number_of_downstream_objects; 
             index++)
        {
            *rg_object_id_array = downstream_object_list.downstream_object_list[index];
            rg_object_id_array++;
        }
    }
    else
    {
        *rg_object_id_array = rg_object_id;
    }
}

/*!****************************************************************************
 * plankton_test_validate_mark_nr()
 ******************************************************************************
 * @brief
 *  Validate if NR is correctly marked for the RG in the specified index
 *
 * @param rg_object_id - Object ID for RG to check for NR
 * @param rebuild_index - Rebuild Index
 *
 * @return fbe_status_t.
 *
 * @author
 *  06/22/2012 - Created. Ashok Tamilarasan
 ******************************************************************************/
static fbe_status_t plankton_test_validate_mark_nr(fbe_object_id_t rg_object_id, 
                                                   fbe_u32_t rebuild_index)
{
    fbe_status_t status;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_api_raid_group_get_paged_info_t paged_info;
    fbe_chunk_count_t chunk_count;
    fbe_lba_t checkpoint;

    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    status = fbe_api_raid_group_get_paged_bits(rg_object_id, &paged_info, FBE_TRUE /* Get data from disk*/);

    chunk_count = (fbe_chunk_count_t)(rg_info.capacity / rg_info.num_data_disk) / FBE_RAID_DEFAULT_CHUNK_SIZE;

    MUT_ASSERT_UINT64_EQUAL_MSG(paged_info.num_nr_chunks, chunk_count, "Not enought chunks marked for NR\n");
    checkpoint = rg_info.rebuild_checkpoint[rebuild_index];
    MUT_ASSERT_UINT64_EQUAL_MSG(checkpoint, 0, "Checkpoint should be Zero\n");
    return status;
}

/*!****************************************************************************
 * plankton_test_create_all_luns()
 ******************************************************************************
 * @brief
 *  Creates the LUN configuration using the NDB options passed in
 *
 * @param rg_config_p - RG configuration Information
 * @param do_ndb - Boolean to do NDB or not
 *
 * @return fbe_status_t.
 *
 * @author
 *  06/22/2012 - Created. Ashok Tamilarasan
 ******************************************************************************/
static void plankton_test_create_all_luns(fbe_test_rg_configuration_t *rg_config_p, fbe_bool_t do_ndb)
{
    plankton_test_create_info_t test_lun_create_info[PLANKTON_TEST_LUNS_PER_RAID_GROUP];
    fbe_u32_t lun_index;
    fbe_status_t status;
    fbe_lba_t                   lun_capacity;
    fbe_database_lun_info_t lun_info[PLANKTON_TEST_LUNS_PER_RAID_GROUP];
    fbe_object_id_t             lun_object_id[PLANKTON_TEST_LUNS_PER_RAID_GROUP];

    status = plankton_test_get_lun_capacity(rg_config_p, &lun_capacity);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Create LUNs with NDB option to true */
    for (lun_index = 0; lun_index < PLANKTON_TEST_LUNS_PER_RAID_GROUP; lun_index++)
    {
        test_lun_create_info[lun_index].raid_type = rg_config_p->raid_type;
        test_lun_create_info[lun_index].raid_group_id = rg_config_p->raid_group_id;
        test_lun_create_info[lun_index].wwid = 0xF + lun_index;
        test_lun_create_info[lun_index].capacity = lun_capacity;
        test_lun_create_info[lun_index].placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
        test_lun_create_info[lun_index].ndb_b = do_ndb;
        test_lun_create_info[lun_index].noinitialverify_b = FBE_FALSE;
        test_lun_create_info[lun_index].ndb_addroffset = (do_ndb ? lun_addroffset[lun_index]: FBE_LBA_INVALID); 
        test_lun_create_info[lun_index].error_b = FALSE;
        test_lun_create_info[lun_index].error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

        plankton_test_create_lun(test_lun_create_info[lun_index]);

        /* Get object ID for LUN just created */
        status = fbe_api_database_lookup_lun_by_number(test_lun_create_info[lun_index].wwid, &lun_object_id[lun_index]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Get LUN info */
        lun_info[lun_index].lun_object_id = lun_object_id[lun_index];
        status = fbe_api_database_get_lun_info(&lun_info[lun_index]);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        /* Confirm capacity is correct */
        MUT_ASSERT_INTEGER_EQUAL(lun_info[lun_index].capacity, lun_capacity);

        /* verify the lun connect to BVD in reasonable time. */
        status = fbe_test_sep_util_wait_for_LUN_edge_on_all_sps(FBE_FALSE,
                                                                lun_info[lun_index].lun_object_id, 
                                                                10000);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

         /* Get object ID for LUN just created */
        status = fbe_api_database_lookup_lun_by_number(test_lun_create_info[lun_index].wwid, &lun_object_id[lun_index]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Check the lifecycle state before issuing a read since the lun might not be connected to BVD object yet
           before you issue a read and sometimes the test might fail.
        */
        status = fbe_api_wait_for_object_lifecycle_state(lun_object_id[lun_index], FBE_LIFECYCLE_STATE_READY,
                                                         20000, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    return;
}

/*!****************************************************************************
 * plankton_test_destroy_config()
 ******************************************************************************
 * @brief
 *  Destroy the RG and LUN config
 *
 * @param rg_object_id - Object ID for RG to check for NR
 *
 * @return fbe_status_t.
 *
 * @author
 *  06/22/2012 - Created. Ashok Tamilarasan
 ******************************************************************************/
static fbe_status_t plankton_test_destroy_config(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_u32_t lun_index;

    for (lun_index=0; lun_index<PLANKTON_TEST_LUNS_PER_RAID_GROUP; lun_index++)
    {
         /* Destroy one of the LUNs we just created.
          */
         plankton_test_destroy_lun(PLANKTON_TEST_LUN_ID_OFFSET + lun_index);
    }

    /* DEstroy a RAID group with user provided configuration. */
    status = fbe_test_sep_util_destroy_raid_group(rg_config_p->raid_group_id);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Destroy RAID group failed.");

    return status;
}

/*!****************************************************************************
 * plankton_test_validate_lun_data()
 ******************************************************************************
 * @brief
 *  Validate that the data on the LUNs are correct
 *
 * @return fbe_status_t.
 *
 * @author
 *  06/22/2012 - Created. Ashok Tamilarasan
 ******************************************************************************/
static fbe_status_t plankton_test_validate_lun_data(void)
{
    fbe_u32_t lun_index;
    fbe_object_id_t             lun_object_id[PLANKTON_TEST_LUNS_PER_RAID_GROUP];
    fbe_api_rdgen_context_t*    context_p = &plankton_test_rdgen_contexts[0];
    fbe_status_t status;

     /* Validate that the data is as expected */
    for (lun_index = 0; lun_index < PLANKTON_TEST_LUNS_PER_RAID_GROUP; lun_index++)
    {
        context_p = &(plankton_test_rdgen_contexts[lun_index]);

        /* Get object ID for LUN just created */
        status = fbe_api_database_lookup_lun_by_number(PLANKTON_TEST_LUN_ID_OFFSET + lun_index, &lun_object_id[lun_index]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Read test pattern from LUN; should be the same */
        plankton_test_read_background_pattern(context_p, PLANKTON_TEST_LUN_ELEMENT_SIZE, lun_object_id[lun_index]);
    }
    return status;
}
