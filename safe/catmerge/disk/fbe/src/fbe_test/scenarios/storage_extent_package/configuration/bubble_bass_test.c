
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file bubble_bass_test.c
 ***************************************************************************
 *
 * @brief
 *   This file includes tests for unbinding new LUNs.
 *
 * @version
 *   1/2010 - Created.  rundbs
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
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "pp_utils.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_random.h"

/*-----------------------------------------------------------------------------
    TEST DESCRIPTION:
*/

char * bubble_bass_short_desc = "Basic LUN Unbind operation";
char * bubble_bass_long_desc =
    "\n"
    "\n"
    "The Bubble Bass scenario tests that the Lun object can be unbound successfully.\n"
    "\n"
    "Starting Config:\n"
    "\t [PP] 1-armada board\n"
    "\t [PP] 1-SAS PMC port\n"
    "\t [PP] 4-viper enclosure\n"
    "\t [PP] 60 SAS drives (PDO)\n"
    "\t [PP] 60 logical drives (LD)\n"
    "\n"
    "STEP 1: Bring up the initial topology.\n"
    "\t- Create and verify the initial physical package config.\n"
    "\t- Verify all the PDO, LDO and PVDs are created and in expected state.\n"
    "\n"
    "STEP 2: Create RG and LUNs\n"
    "\t- Create a raid object with I/O edges attached to all VDs.\n"
    "\t- Create a lun object with an I/O edge attached to the RAID\n"
    "\t  Lun object can be created with exact bind size\n"
    "\t  Lun object can be created with bind offset to test bind offset sizes\n"
    "\t- Verify that all configured objects are in the READY state.\n"
    "\n"
    "STEP 3: Start I/O to LUN.\n"
    "\n"
    "STEP 4: Destroy the LUN.\n"
    "\t- verify that the area used by the LUN object is availible in RAID object \n"
    "\t- Unbind the LUN\n"
    "\t- verify the LUN object is destroyed\n"
    "\t- verify that the database service does not have any information related to this LUN\n"
    "\t- verify the LUN is destroyed from the peer as well.\n"
    "\n"
    "*** Bubble Bass Post-Phase 1 ********************************************************* \n"
    "\n"
    "The Bubble Bass Scenario also needs to cover the following cases in follow-on phases:\n"
    "    - Verify that LUN Object is destroyed on both SPs in memory and persistently\n"
    "    - Verify that appropriate error handling is performed for the following cases:\n" 
    "      - invalid LUN ID\n"
    "      - Database Service cannot destroy LUN Object and update config database in memory\n" 
    "      - Database Service cannot destroy LUN Object and update config database persistently\n"   
    "      - Database Service cannot destroy LUN Object and update config database on peer\n" 
    "    - Ensure that if the maximum outstanding I/O count is changed, flushing and blocking\n"
    "      I/O will work properly when the LUN is destroyed\n"
    "\n"
    "Description last updated: 9/22/2011.\n";

/*-----------------------------------------------------------------------------
    DEFINITIONS:
*/

/*!*******************************************************************
 * @def BUBBLE_BASS_TEST_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Max number of RGs to create
 *
 *********************************************************************/
#define BUBBLE_BASS_TEST_RAID_GROUP_COUNT           1


/*!*******************************************************************
 * @def BUBBLE_BASS_TEST_MAX_RG_WIDTH
 *********************************************************************
 * @brief Max number of drives in a RG
 *
 *********************************************************************/
#define BUBBLE_BASS_TEST_MAX_RG_WIDTH               16


/*!*******************************************************************
 * @def BUBBLE_BASS_TEST_LUN_ELEMENT_SIZE
 *********************************************************************
 * @brief Element size of LUNs
 *
 *********************************************************************/
#define BUBBLE_BASS_TEST_LUN_ELEMENT_SIZE           128 


/*!*******************************************************************
 * @def BUBBLE_BASS_MAX_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will use for testing.
 *
 *********************************************************************/
#define BUBBLE_BASS_MAX_IO_SIZE_BLOCKS 1024


/*!*******************************************************************
 * @def BUBBLE_BASS_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects to create
 *        (1 board + 1 port + 4 encl + 60 pdo) 
 *
 *********************************************************************/
#define BUBBLE_BASS_TEST_NUMBER_OF_PHYSICAL_OBJECTS 66 


/*!*******************************************************************
 * @def BUBBLE_BASS_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive
 *
 *********************************************************************/
#define BUBBLE_BASS_TEST_DRIVE_CAPACITY             TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY


/*!*******************************************************************
 * @def BUBBLE_BASS_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Max number of LUNs in the RAID Group
 *
 *********************************************************************/
#define BUBBLE_BASS_TEST_LUNS_PER_RAID_GROUP        5


/*!*******************************************************************
 * @def BUBBLE_BASS_TEST_RAID_GROUP_ID
 *********************************************************************
 * @brief RAID Group id used by this test
 *
 *********************************************************************/
#define BUBBLE_BASS_TEST_RAID_GROUP_ID        0


/*!*******************************************************************
 * @def BUBBLE_BASS_TEST_NS_TIMEOUT
 *********************************************************************
 * @brief  Notification to timeout value in milliseconds 
 *
 *********************************************************************/
#define BUBBLE_BASS_TEST_NS_TIMEOUT        25000 /*wait maximum of 25 seconds*/


/*!*******************************************************************
 * @var bubble_bass_rg_configuration
 *********************************************************************
 * @brief This is rg_configuration
 *
 *********************************************************************/
static fbe_test_rg_configuration_t bubble_bass_rg_configuration[] = 
{
 /* width, capacity,        raid type,                        class,               block size, RAID-id,                        bandwidth.*/
   { 3,     FBE_LBA_INVALID, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY, 520,        BUBBLE_BASS_TEST_RAID_GROUP_ID, 0},
    
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};


/************************************************************************************
 * Components to support Table driven test methodology.
 * Following this approach, the parameters for the test will be stored
 * as rows in a table.
 * The test will then be executed for each row(i.e. set of parameters)
 * in the table.
 ***********************************************************************************/

/* The different test numbers.
 */
typedef enum bubble_bass_test_enclosure_num_e
{
    BUBBLE_BASS_TEST_ENCL1_WITH_SAS_DRIVES = 0,
    BUBBLE_BASS_TEST_ENCL2_WITH_SAS_DRIVES,
    BUBBLE_BASS_TEST_ENCL3_WITH_SAS_DRIVES,
    BUBBLE_BASS_TEST_ENCL4_WITH_SAS_DRIVES
} bubble_bass_test_enclosure_num_t;

/* The different drive types.
 */
typedef enum bubble_bass_test_drive_type_e
{
    SAS_DRIVE,
    SATA_DRIVE
} bubble_bass_test_drive_type_t;

/* This is a set of parameters for the Scoobert spare drive selection tests.
 */
typedef struct enclosure_drive_details_s
{
    bubble_bass_test_drive_type_t drive_type; /* Type of the drives to be inserted in this enclosure */
    fbe_u32_t port_number;                    /* The port on which current configuration exists */
    bubble_bass_test_enclosure_num_t  encl_number;      /* The unique enclosure number */
    fbe_block_size_t block_size;
} enclosure_drive_details_t;

/* Table containing actual enclosure and drive details to be inserted.
 */
static enclosure_drive_details_t encl_table[] = 
{
    {   SAS_DRIVE,
        0,
        BUBBLE_BASS_TEST_ENCL1_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        BUBBLE_BASS_TEST_ENCL2_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        BUBBLE_BASS_TEST_ENCL3_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        BUBBLE_BASS_TEST_ENCL4_WITH_SAS_DRIVES,
        520
    },
};

/* Count of rows in the table.
 */
#define BUBBLE_BASS_TEST_ENCL_MAX sizeof(encl_table)/sizeof(encl_table[0])

/* LUN info used to create a LUN */
typedef struct bubble_bass_test_create_info_s
{
    fbe_raid_group_type_t           raid_type; 
    fbe_raid_group_number_t              raid_group_id;
    fbe_block_transport_placement_t placement;
    fbe_lun_number_t                wwid;
    fbe_lba_t                       capacity;
    fbe_bool_t                      ndb_b;
    fbe_bool_t                      noinitialverify_b;
    fbe_lba_t                       ndb_addroffset;
} bubble_bass_test_create_info_t;

/* Contexts used with rdgen */
static fbe_api_rdgen_context_t bubble_bass_test_rdgen_contexts[BUBBLE_BASS_TEST_LUNS_PER_RAID_GROUP];


/*-----------------------------------------------------------------------------
    FORWARD DECLARATIONS:
*/

static void bubble_bass_test_load_physical_config(void);
static void bubble_bass_test_create_rg(fbe_test_rg_configuration_t     *rg_config_p);
static void bubble_bass_test_create_lun(bubble_bass_test_create_info_t in_test_lun_create_info);
static void bubble_bass_test_destroy_lun(fbe_lun_number_t);
static fbe_status_t bubble_bass_test_run_io_load(fbe_object_id_t lun_object_id);
static void bubble_bass_test_setup_rdgen_test_context(  fbe_api_rdgen_context_t *context_p,
                                                        fbe_object_id_t object_id,
                                                        fbe_class_id_t class_id,
                                                        fbe_rdgen_operation_t rdgen_operation,
                                                        fbe_lba_t capacity,
                                                        fbe_u32_t max_passes,
                                                        fbe_u32_t threads,
                                                        fbe_u32_t io_size_blocks);
void bubble_bass_test_write_background_pattern(fbe_element_size_t element_size, fbe_object_id_t lun_object_id);
void bubble_bass_test_setup_fill_rdgen_test_context(fbe_api_rdgen_context_t *context_p,
                                                    fbe_object_id_t object_id,
                                                    fbe_class_id_t class_id,
                                                    fbe_rdgen_operation_t rdgen_operation,
                                                    fbe_lba_t max_lba,
                                                    fbe_u32_t io_size_blocks);
static fbe_status_t bubble_bass_test_stop_io_load(void);
static void bubble_bass_run_test(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p);


/*-----------------------------------------------------------------------------
    FUNCTIONS:
*/


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

void bubble_bass_test(void)
{

    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    
    rg_config_p = &bubble_bass_rg_configuration[0];
    
    fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

    
    fbe_test_run_test_on_rg_config_no_create(rg_config_p, 0, bubble_bass_run_test);


    return;

}/* End plankton_test() */


/*!****************************************************************************
 *  bubble_bass_test
 ******************************************************************************
 *
 * @brief
 *  This is the main entry point for the Bubble Bass test.  
 *
 *
 * @param   None
 *
 * @return  None 
 *
 * @Modified
 * 15/07/2011 - Created. Vishnu Sharma
 *****************************************************************************/
void bubble_bass_run_test(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p)
{
    fbe_status_t                    status;
    bubble_bass_test_create_info_t  test_lun_create_info;
    fbe_object_id_t                 lun_object_id_on_spa;//, lun_object_id_on_spb;
    fbe_u32_t                       num_raid_groups = 0;
    fbe_u32_t                       index = 0;
    fbe_test_rg_configuration_t     *rg_config_p = NULL;
      

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_ptr);
    
    mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Bubble bass Test ===\n");

    for(index = 0; index < num_raid_groups; index++)
    {
        rg_config_p = rg_config_ptr + index;

        if(fbe_test_rg_config_is_enabled(rg_config_p))
        {
    
            /* Create RG */
            /* Note: rg create is separated from logical configuration create,
               so only one sp creates the rg */
            bubble_bass_test_create_rg(rg_config_p);

            mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Bubble Bass Test ===\n");


            /* Create a LUN */
            test_lun_create_info.raid_type = rg_config_p->raid_type;
            test_lun_create_info.raid_group_id = rg_config_p->raid_group_id;
            test_lun_create_info.wwid = 123;
            test_lun_create_info.capacity = 0x1000;
            test_lun_create_info.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
            test_lun_create_info.ndb_b = FBE_FALSE;
            test_lun_create_info.noinitialverify_b = FBE_FALSE;
            test_lun_create_info.ndb_addroffset = FBE_LBA_INVALID;

            bubble_bass_test_create_lun(test_lun_create_info);

            /* Get object ID for LUN just created */
            status = fbe_api_database_lookup_lun_by_number(test_lun_create_info.wwid, &lun_object_id_on_spa);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            mut_printf(MUT_LOG_TEST_STATUS, "LUN Created successfully on SPA with object_id %x\n", lun_object_id_on_spa);

            ///* Check on the peer SP. Get object ID for LUN just created */
            //mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPB ===\n");
            //fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
            //status = fbe_api_database_lookup_lun_by_number(test_lun_create_info.wwid, &lun_object_id_on_spb);
            //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            //mut_printf(MUT_LOG_TEST_STATUS, "LUN Created successfully on SPB with object_id %x\n", lun_object_id_on_spb);

            //mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");
            //fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

            ///* Set a delay for I/O completion in the terminator */
            //fbe_api_terminator_set_io_global_completion_delay(1000);

            ///* Run I/O to LUN */
            //status = bubble_bass_test_run_io_load(lun_object_id_on_spa);
            //MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

            ///* Let I/O run for a few seconds */
            //EmcutilSleep(5000);

            ///* Make sure I/O progressing okay */
            //status = fbe_api_rdgen_get_stats(&rdgen_stats, &filter);
            //MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            //MUT_ASSERT_INT_EQUAL(rdgen_stats.error_count, 0);

            ///* Set a delay for I/O completion in the terminator */
            //fbe_api_terminator_set_io_global_completion_delay(0);

            /* Test destroying LUN just created */
            bubble_bass_test_destroy_lun(test_lun_create_info.wwid);

            /* Get object ID for LUN should fail, because it is just destroyed */
            status = fbe_api_database_lookup_lun_by_number(test_lun_create_info.wwid, &lun_object_id_on_spa);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_GENERIC_FAILURE, status);
            mut_printf(MUT_LOG_TEST_STATUS, "LUN destroyed successfully on SPA\n");

            ///* Check on the peer SP. Get object ID for LUN should fail, because it is just destroyed */
            //mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPB ===\n");
            //fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
            //status = fbe_api_database_lookup_lun_by_number(test_lun_create_info.wwid, &lun_object_id_on_spb);
            //MUT_ASSERT_INT_EQUAL(FBE_STATUS_GENERIC_FAILURE, status);
            //mut_printf(MUT_LOG_TEST_STATUS, "LUN destroyed successfully on SPB\n");

            //mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");
            //fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

            ///* Stop I/O */
            //status = bubble_bass_test_stop_io_load();
           // MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


            fbe_test_sep_util_destroy_raid_group(rg_config_p->raid_group_id);
         }
    }

    return;

} /* End bubble_bass_test() */


/*!****************************************************************************
 *  bubble_bass_test_setup
 ******************************************************************************
 *
 * @brief
 *  This is the setup function for the Bubble Bass test. It is responsible
 *  for loading the physical and logical configuration for this test suite.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void bubble_bass_test_setup(void)
{

    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Bubble Bass test ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");

    if(fbe_test_util_is_simulation())
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        /* Load the physical configuration */
        bubble_bass_test_load_physical_config();

        /* Load sep and neit packages */
        sep_config_load_sep_and_neit();

        //mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPB ===\n");
        //fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        ///* Load the physical configuration */
        //bubble_bass_test_load_physical_config();
        ///* Load sep and neit packages */
        //sep_config_load_sep_and_neit();

        //mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");
        //fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        ///* Create RG */
        ///* Note: rg create is separated from logical configuration create,
        //   so only one sp creates the rg */
    }
    return;

} /* End bubble_bass_test_setup() */


/*!****************************************************************************
 *  bubble_bass_test_cleanup
 ******************************************************************************
 *
 * @brief
 *  This is the cleanup function for the Bubble Bass test.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void bubble_bass_test_cleanup(void)
{  
    /* Give sometime to let the object go away */
    EmcutilSleep(1000);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for Bubble Bass test ===\n");

    if(fbe_test_util_is_simulation())
    {
        /* Turn off delay for I/O completion in the terminator */
        fbe_api_terminator_set_io_global_completion_delay(0);

        mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        /* Destroy the test configuration */
        fbe_test_sep_util_destroy_neit_sep_physical();

        //mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPB ===\n");
        //fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

        ///* Destroy the test configuration */
        //fbe_test_sep_util_destroy_neit_sep_physical();
    }

    return;

} /* End bubble_bass_test_cleanup() */


/*
 *  The following functions are Bubble Bass test functions. 
 *  They test various features of the LUN Object focusing on LUN destroy (unbind).
 */


/*!****************************************************************************
 *  bubble_bass_test_create_lun
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
static void bubble_bass_test_create_lun(bubble_bass_test_create_info_t in_test_lun_create_info)
{
    fbe_status_t                       status;
    fbe_api_lun_create_t               fbe_lun_create_req;
    fbe_object_id_t                    object_id;
    fbe_job_service_error_type_t       job_error_type;
   
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

	status = fbe_api_create_lun(&fbe_lun_create_req, FBE_TRUE, BUBBLE_BASS_TEST_NS_TIMEOUT, &object_id, &job_error_type);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);   
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, object_id);

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: LUN 0x%X created successfully! ===\n", __FUNCTION__, object_id);

	/* check that given event message is logged correctly */
    status = fbe_test_sep_util_wait_for_LUN_create_event(object_id, 
										 fbe_lun_create_req.lun_number); 
                
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;

}  /* End bubble_bass_test_create_lun() */


/*!****************************************************************************
 *  bubble_bass_test_destroy_lun
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
static void bubble_bass_test_destroy_lun(fbe_lun_number_t   lun_number)
{
    fbe_status_t                    status;
    fbe_api_lun_destroy_t			fbe_lun_destroy_req;
	fbe_object_id_t                 object_id;
	fbe_bool_t						is_msg_present = FBE_FALSE;
    fbe_job_service_error_type_t    job_error_type;
    
    /* Destroy a LUN */
    fbe_lun_destroy_req.lun_number = lun_number;
	
    status = fbe_api_database_lookup_lun_by_number(fbe_lun_destroy_req.lun_number, &object_id);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Issue LUN destroy! ===\n");

    status = fbe_api_destroy_lun(&fbe_lun_destroy_req, FBE_TRUE,  BUBBLE_BASS_TEST_NS_TIMEOUT, &job_error_type);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN destroyed successfully! ===\n");

	/* check that given event message is logged correctly */
    status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_SEP_0, 
										&is_msg_present, 
										SEP_INFO_LUN_DESTROYED,
										fbe_lun_destroy_req.lun_number); 
                
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, is_msg_present, "Event log msg is not present!");

    return;

}  /* End bubble_bass_test_destroy_lun() */


/*
 * The following functions are utility functions used by this test suite
 */


/*!**************************************************************
 * bubble_bass_test_load_physical_config()
 ****************************************************************
 * @brief
 *  Configure the Bubble Bass test physical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/

static void bubble_bass_test_load_physical_config(void)
{

    fbe_status_t                            status;
    fbe_u32_t                               object_handle;
    fbe_u32_t                               port_idx;
    fbe_u32_t                               enclosure_idx;
    fbe_u32_t                               drive_idx;
    fbe_u32_t                               total_objects;
    fbe_class_id_t                          class_id;
    fbe_api_terminator_device_handle_t      port0_handle;
    fbe_api_terminator_device_handle_t      port0_encl_handle[BUBBLE_BASS_TEST_ENCL_MAX];
    fbe_api_terminator_device_handle_t      drive_handle;
    fbe_u32_t                               slot;
    fbe_u32_t                               enclosure;

    /* Initialize local variables */
    status          = FBE_STATUS_GENERIC_FAILURE;
    object_handle   = 0;
    port_idx        = 0;
    enclosure_idx   = 0;
    drive_idx       = 0;
    total_objects   = 0;

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
    for ( enclosure = 0; enclosure < BUBBLE_BASS_TEST_ENCL_MAX; enclosure++)
    { 
        fbe_test_pp_util_insert_viper_enclosure(0,
                                                enclosure,
                                                port0_handle,
                                                &port0_encl_handle[enclosure]);
    }

    /* Insert drives for enclosures.
     */
    for ( enclosure = 0; enclosure < BUBBLE_BASS_TEST_ENCL_MAX; enclosure++)
    {
        for ( slot = 0; slot < 15; slot++)
        {
            if(SAS_DRIVE == encl_table[enclosure].drive_type)
            {
                fbe_test_pp_util_insert_sas_drive(encl_table[enclosure].port_number,
                                                encl_table[enclosure].encl_number,
                                                slot,
                                                encl_table[enclosure].block_size,
                                                TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY,
                                                &drive_handle);
            }
            else if(SATA_DRIVE == encl_table[enclosure].drive_type)
            {
                fbe_test_pp_util_insert_sata_drive(encl_table[enclosure].port_number,
                                                encl_table[enclosure].encl_number,
                                                slot,
                                                encl_table[enclosure].block_size,
                                                TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY,
                                                &drive_handle);
            }
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
    status = fbe_api_wait_for_number_of_objects(BUBBLE_BASS_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
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
    MUT_ASSERT_TRUE(total_objects == BUBBLE_BASS_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");

    return;

} /* End bubble_bass_test_load_physical_config() */


/*!**************************************************************
 * bubble_bass_test_create_rg()
 ****************************************************************
 * @brief
 *  Configure the Bubble Bass test logical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/

static void bubble_bass_test_create_rg(fbe_test_rg_configuration_t     *rg_config_p)
{
    fbe_status_t                                status;
    fbe_object_id_t                             rg_object_id;
    fbe_job_service_error_type_t                job_error_code;
    fbe_status_t                                job_status;

    /* Initialize local variables */
    rg_object_id        = FBE_OBJECT_ID_INVALID;

    /* Create the SEP objects for the configuration */

    /* Create a RG */
    do
    {
        mut_printf(MUT_LOG_TEST_STATUS, "=== create a RAID Group ===\n");
        status = fbe_test_sep_util_create_raid_group(rg_config_p);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "RG creation failed");

        /* wait for notification from job service. */
        status = fbe_api_common_wait_for_job(rg_config_p->job_number,
                                             BUBBLE_BASS_TEST_NS_TIMEOUT,
                                             &job_error_code,
                                             &job_status,
                                             NULL);
        if ((status == FBE_STATUS_OK) && (job_error_code == FBE_JOB_SERVICE_ERROR_BAD_PVD_CONFIGURATION))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "PVD is not ready.  Let's retry...\n");
        }
    }
    while ((status == FBE_STATUS_OK) && (job_error_code == FBE_JOB_SERVICE_ERROR_BAD_PVD_CONFIGURATION));

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to created RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(
             rg_config_p->raid_group_id, &rg_object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* Verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group object %d\n", rg_object_id);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: END\n", __FUNCTION__);

    return;

} /* End bubble_bass_test_create_rg() */


/*!**************************************************************
 * bubble_bass_test_run_io_load()
 ****************************************************************
 * @brief
 *  Start rdgen I/O to the specified LUN object.
 *
 * @param lun_object_id - object ID of LUN to issue I/O to.              
 *
 * @return fbe_status_t
 *
 ****************************************************************/

static fbe_status_t bubble_bass_test_run_io_load(fbe_object_id_t lun_object_id)
{
    fbe_api_rdgen_context_t*    context_p = &bubble_bass_test_rdgen_contexts[0];
    fbe_status_t                status;


    /* Write a background pattern; necessary until LUN zeroing is fully supported */
    bubble_bass_test_write_background_pattern(BUBBLE_BASS_TEST_LUN_ELEMENT_SIZE, lun_object_id);

    /* Set up for issuing reads forever
     */
    bubble_bass_test_setup_rdgen_test_context(  context_p,
                                                lun_object_id,
                                                FBE_CLASS_ID_INVALID,
                                                FBE_RDGEN_OPERATION_READ_ONLY,
                                                FBE_LBA_INVALID,    /* use capacity */
                                                0,      /* run forever*/ 
                                                3,      /* threads per lun */
                                                BUBBLE_BASS_MAX_IO_SIZE_BLOCKS);

    /* Run our I/O test
     */
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    return FBE_STATUS_OK;

} /* End bubble_bass_test_run_io_load() */


/*!**************************************************************
 * bubble_bass_test_setup_rdgen_test_context()
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

static void bubble_bass_test_setup_rdgen_test_context(  fbe_api_rdgen_context_t *context_p,
                                                        fbe_object_id_t object_id,
                                                        fbe_class_id_t class_id,
                                                        fbe_rdgen_operation_t rdgen_operation,
                                                        fbe_lba_t capacity,
                                                        fbe_u32_t max_passes,
                                                        fbe_u32_t threads,
                                                        fbe_u32_t io_size_blocks)
{
    fbe_status_t status;

    status = fbe_api_rdgen_test_context_init(   context_p, 
                                                object_id, 
                                                class_id,
                                                FBE_PACKAGE_ID_SEP_0,
                                                rdgen_operation,
                                                FBE_RDGEN_PATTERN_LBA_PASS,
                                                max_passes,
                                                0, /* io count not used */
                                                0, /* time not used*/
                                                threads,
                                                FBE_RDGEN_LBA_SPEC_RANDOM,
                                                0,    /* Start lba*/
                                                0,    /* Min lba */
                                                capacity, /* max lba */
                                                FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                                1,    /* Min blocks */
                                                io_size_blocks  /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;

} /* End bubble_bass_test_setup_rdgen_test_context() */


/*!**************************************************************
 * bubble_bass_test_write_background_pattern()
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

void bubble_bass_test_write_background_pattern(fbe_element_size_t element_size, fbe_object_id_t lun_object_id)
{
    fbe_api_rdgen_context_t *context_p = &bubble_bass_test_rdgen_contexts[0];
    fbe_status_t status;

    /* Write a background pattern to the LUN
     */
    bubble_bass_test_setup_fill_rdgen_test_context( context_p,
                                                    lun_object_id,
                                                    FBE_CLASS_ID_INVALID,
                                                    FBE_RDGEN_OPERATION_WRITE_ONLY,
                                                    FBE_LBA_INVALID, /* use max capacity */
                                                    BUBBLE_BASS_TEST_LUN_ELEMENT_SIZE);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    
    /* Make sure there were no errors
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    /* Read back the pattern and check it was written OK
     */
    bubble_bass_test_setup_fill_rdgen_test_context( context_p,
                                                    lun_object_id,
                                                    FBE_CLASS_ID_INVALID,
                                                    FBE_RDGEN_OPERATION_READ_CHECK,
                                                    FBE_LBA_INVALID, /* use max capacity */
                                                    BUBBLE_BASS_TEST_LUN_ELEMENT_SIZE);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    return;

} /* end bubble_bass_test_write_background_pattern() */


/*!**************************************************************
 * bubble_bass_test_setup_fill_rdgen_test_context()
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

void bubble_bass_test_setup_fill_rdgen_test_context(fbe_api_rdgen_context_t *context_p,
                                                    fbe_object_id_t object_id,
                                                    fbe_class_id_t class_id,
                                                    fbe_rdgen_operation_t rdgen_operation,
                                                    fbe_lba_t max_lba,
                                                    fbe_u32_t io_size_blocks)
{
    fbe_status_t status;

    status = fbe_api_rdgen_test_context_init(   context_p, 
                                                object_id,
                                                class_id, 
                                                FBE_PACKAGE_ID_SEP_0,
                                                rdgen_operation,
                                                FBE_RDGEN_PATTERN_LBA_PASS,
                                                1,    /* We do one full sequential pass. */
                                                0,    /* num ios not used */
                                                0,    /* time not used*/
                                                1,    /* threads */
                                                FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                                0,    /* Start lba*/
                                                0,    /* Min lba */
                                                max_lba,    /* max lba */
                                                FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE,
                                                16,    /* Number of stripes to write. */
                                                io_size_blocks    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;

} /* end bubble_bass_test_setup_fill_rdgen_test_context() */


/*!**************************************************************
 * bubble_bass_test_stop_io_load()
 ****************************************************************
 * @brief
 *  Stop rdgen I/O tests.
 *
 * @param None.              
 *
 * @return fbe_status_t
 *
 ****************************************************************/

static fbe_status_t bubble_bass_test_stop_io_load(void)
{
    fbe_api_rdgen_context_t*    context_p = &bubble_bass_test_rdgen_contexts[0];
    fbe_status_t                status;


    /* Halt I/O; note that some I/O should have failed because the LUN
     * was being destroyed while I/O was in progress.
     */
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.statistics.error_count, 0);
    
    return FBE_STATUS_OK;

} /* End bubble_bass_test_stop_io_load() */

