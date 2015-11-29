
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file transformers_test.c
 ***************************************************************************
 *
 * @brief
 *   This file includes tests for testing the clearing of DDMI region on disk 
 *
 * @version
 *   7/2012 - Created.  gaoh1
 *
 ***************************************************************************/


/*-----------------------------------------------------------------------------
    INCLUDES OF REQUIRED HEADER FILES:
*/

#include "sep_tests.h"
#include "sep_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "pp_utils.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_power_saving_interface.h"

#include "fbe_test_configurations.h"

/*-----------------------------------------------------------------------------
    TEST DESCRIPTION:
*/

char * transformers_short_desc = "tests whether the DDMI region of disk is zeroed";
char * transformers_long_desc =
    "\n"
    "\n"
    "tests whether the DDMI region of disk is zeroed.\n"
    "\n"
    "Read data from the DDMI region, and check it. If the data is zero, the it is ok\n "
    "\n";

/*-----------------------------------------------------------------------------
    DEFINITIONS:
*/

/*!*******************************************************************
 * @def TRANSFORMERS_TEST_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Max number of RGs to create
 *
 *********************************************************************/
#define TRANSFORMERS_TEST_RAID_GROUP_COUNT           1

/*!*******************************************************************
 * @def TRANSFORMERS_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects to create
 *        (1 board + 1 port + 4 encl + 60 pdo) 
 *
 *********************************************************************/
#define TRANSFORMERS_TEST_NUMBER_OF_PHYSICAL_OBJECTS 66 


/*!*******************************************************************
 * @def TRANSFORMERS_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive
 *
 *********************************************************************/
#define TRANSFORMERS_TEST_DRIVE_CAPACITY             TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY

/*!*******************************************************************
 * @def TRANSFORMERS_TEST_RAID_GROUP_ID
 *********************************************************************
 * @brief RAID Group id used by this test
 *
 *********************************************************************/
#define TRANSFORMERS_TEST_RAID_GROUP_ID        0


/*!*******************************************************************
 * @def TRANSFORMERS_TEST_NS_TIMEOUT
 *********************************************************************
 * @brief  Notification to timeout value in milliseconds 
 *
 *********************************************************************/
#define TRANSFORMERS_TEST_NS_TIMEOUT        25000 /*wait maximum of 25 seconds*/


/*!*******************************************************************
 * @var TRANSFORMERS_abort_rg_configuration
 *********************************************************************
 * @brief This is rg_configuration
 *
 *********************************************************************/
#if 0
static fbe_test_rg_configuration_t transformers_rg_configuration[TRANSFORMERS_TEST_RAID_GROUP_COUNT] = 
{
 /* width, capacity,        raid type,                        class,               block size, RAID-id,                        bandwidth.*/
    3,     FBE_LBA_INVALID, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY, 512,        TRANSFORMERS_TEST_RAID_GROUP_ID, 0,
    /* rg_disk_set */
    {{0,0,4}, {0,0,5}, {0,0,6}}
};
#endif


/************************************************************************************
 * Components to support Table driven test methodology.
 * Following this approach, the parameters for the test will be stored
 * as rows in a table.
 * The test will then be executed for each row(i.e. set of parameters)
 * in the table.
 ***********************************************************************************/

/* The different test numbers.
 */
typedef enum transformers_test_enclosure_num_e
{
    TRANSFORMERS_TEST_ENCL1_WITH_SAS_DRIVES = 0,
    TRANSFORMERS_TEST_ENCL2_WITH_SAS_DRIVES,
    TRANSFORMERS_TEST_ENCL3_WITH_SAS_DRIVES,
    TRANSFORMERS_TEST_ENCL4_WITH_SAS_DRIVES
} transformers_test_enclosure_num_t;

/* The different drive types.
 */
typedef enum transformers_test_drive_type_e
{
    SAS_DRIVE,
    SATA_DRIVE
} transformers_test_drive_type_t;

/* This is a set of parameters for the Scoobert spare drive selection tests.
 */
typedef struct enclosure_drive_details_s
{
    transformers_test_drive_type_t drive_type; /* Type of the drives to be inserted in this enclosure */
    fbe_u32_t port_number;                    /* The port on which current configuration exists */
    transformers_test_enclosure_num_t  encl_number;      /* The unique enclosure number */
    fbe_block_size_t block_size;
} enclosure_drive_details_t;

/* Table containing actual enclosure and drive details to be inserted.
 */
static enclosure_drive_details_t encl_table[] = 
{
    {   SAS_DRIVE,
        0,
        TRANSFORMERS_TEST_ENCL1_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        TRANSFORMERS_TEST_ENCL2_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        TRANSFORMERS_TEST_ENCL3_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        TRANSFORMERS_TEST_ENCL4_WITH_SAS_DRIVES,
        520
    },
};

/* Count of rows in the table.
 */
#define TRANSFORMERS_TEST_ENCL_MAX sizeof(encl_table)/sizeof(encl_table[0])

/*-----------------------------------------------------------------------------
    FORWARD DECLARATIONS:
*/

static void transformers_test_load_physical_config(void);
static fbe_s32_t transformers_compare_memory(char *b1, char *b2, fbe_u32_t len);

fbe_status_t transformers_check_ddmi_region (void);



/*-----------------------------------------------------------------------------
    FUNCTIONS:
*/
static fbe_s32_t transformers_compare_memory(char *b1, char *b2, fbe_u32_t len)
{
    if (len) do {
      if (*b1++ != *b2++)
        return (*((char *)b1-1) - *((char *)b2-1));
    } while (--len);

    return(0);
}

fbe_status_t transformers_check_ddmi_region (void)
{
    fbe_u32_t bus, enclosure, index;
    fbe_u32_t total_bytes, returned_bytes;
    fbe_u8_t *tmp_buffer;
    fbe_u8_t *p_ddmi_data;
    fbe_status_t status;

    
    mut_printf(MUT_LOG_TEST_STATUS, "~~~Checking the DDMI region.\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    tmp_buffer = (fbe_u8_t *)fbe_api_allocate_memory(FBE_BYTES_PER_BLOCK * 3);
    if(NULL == tmp_buffer){
        mut_printf(MUT_LOG_TEST_STATUS, "~~allocate memory failed.~~\n");
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }
    
    fbe_zero_memory(tmp_buffer, FBE_BYTES_PER_BLOCK * 3);
    p_ddmi_data = tmp_buffer + FBE_BYTES_PER_BLOCK;
    total_bytes = FBE_BYTES_PER_BLOCK;

    bus = 0;
    enclosure = 0;
 
    for (index = 0; index < 15; index++) 
    {
        status = fbe_api_database_get_disk_ddmi_data(p_ddmi_data, bus, enclosure, index,
                                            total_bytes, &returned_bytes);
        
        if (status != FBE_STATUS_OK)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "~~fbe_api_database_get_disk_ddmi_data return fail.~~\n");
            fbe_api_free_memory(tmp_buffer);
            return status;
        }
        if (total_bytes != returned_bytes)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "~~The data bytes are mismatching.~~\n");
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_api_free_memory(tmp_buffer);
            return status;
        }
        if (transformers_compare_memory(tmp_buffer, p_ddmi_data, FBE_BYTES_PER_BLOCK))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "~~The data region is NOT zero.~~\n");
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_api_free_memory(tmp_buffer);
            return status;
        }
        fbe_zero_memory(p_ddmi_data, FBE_BYTES_PER_BLOCK * 2);      
            
    }
    fbe_api_free_memory(tmp_buffer);
    return FBE_STATUS_OK;

}

/*!****************************************************************************
 *  transformers_test
 ******************************************************************************
 *
 * @brief
 *  This is the main entry point for the TRANSFORMERS test.  
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void transformers_test(void)
{
    fbe_status_t status;
    
    status = transformers_check_ddmi_region();
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "~~~Checking DDMI region failed~~\n");

    return;
} /* End transformers_test() */

/*!****************************************************************************
 *  transformers_test_setup
 ******************************************************************************
 *
 * @brief
 *  This is the setup function for the transformers test. It is responsible
 *  for loading the physical and logical configuration for this test suite.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void transformers_test_setup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Transformers test ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* Load the physical configuration */
    //transformers_test_load_physical_config();
    /* It is a little strange , when we use elmo_physical_config to set up the physical topology,
    * the PVD won't use too much memory when initializing. 
    * But if I use the transformers_test_load_physical_config(), the PVD initialization will use too much memory.
    */
    elmo_physical_config();
    
    /* Load sep and neit packages */
    sep_config_load_sep_and_neit();

    /* If we don't wait for the PVDs to be ready then we will fail during reads.
     */
    fbe_test_wait_for_all_pvds_ready();
    return;

} /* End transformers_test_setup() */


/*!****************************************************************************
 *  transformers_test_cleanup
 ******************************************************************************
 *
 * @brief
 *  This is the cleanup function for the Shrek test.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void transformers_test_cleanup(void)
{  
    /* Give sometime to let the object go away */
    EmcutilSleep(1000);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for Transformers test ===\n");
   
    mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* Destroy the test configuration */
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;

} /* End transformers_test_cleanup() */


static void transformers_test_load_physical_config(void)
{

    fbe_status_t                            status;
    fbe_u32_t                               object_handle;
    fbe_u32_t                               port_idx;
    fbe_u32_t                               enclosure_idx;
    fbe_u32_t                               drive_idx;
    fbe_u32_t                               total_objects;
    fbe_class_id_t                          class_id;
    fbe_api_terminator_device_handle_t      port0_handle;
    fbe_api_terminator_device_handle_t      port0_encl_handle[TRANSFORMERS_TEST_ENCL_MAX];
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
    for ( enclosure = 0; enclosure < TRANSFORMERS_TEST_ENCL_MAX; enclosure++)
    { 
        fbe_test_pp_util_insert_viper_enclosure(0,
                                                enclosure,
                                                port0_handle,
                                                &port0_encl_handle[enclosure]);
    }

    /* Insert drives for enclosures.
     */
    for ( enclosure = 0; enclosure < TRANSFORMERS_TEST_ENCL_MAX; enclosure++)
    {
        for ( slot = 0; slot < 15; slot++)
        {
            if(SAS_DRIVE == encl_table[enclosure].drive_type)
            {
                if (slot < 4 && enclosure == 0) {
                    fbe_test_pp_util_insert_sas_drive(encl_table[enclosure].port_number,
                                                encl_table[enclosure].encl_number,
                                                slot,
                                                encl_table[enclosure].block_size,
                                                TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY,
                                                &drive_handle);
                }
                else {
                    fbe_test_pp_util_insert_sas_drive(encl_table[enclosure].port_number,
                                                encl_table[enclosure].encl_number,
                                                slot,
                                                encl_table[enclosure].block_size,
                                                TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY,
                                                &drive_handle);
                }
            }
            else if(SATA_DRIVE == encl_table[enclosure].drive_type)
            {
                fbe_test_pp_util_insert_sata_drive(encl_table[enclosure].port_number,
                                                encl_table[enclosure].encl_number,
                                                slot,
                                                encl_table[enclosure].block_size,
                                                TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY,
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
    status = fbe_api_wait_for_number_of_objects(TRANSFORMERS_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 60000, FBE_PACKAGE_ID_PHYSICAL);
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
    MUT_ASSERT_TRUE(total_objects == TRANSFORMERS_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");

    return;

} /* End transformers_test_load_physical_config() */

