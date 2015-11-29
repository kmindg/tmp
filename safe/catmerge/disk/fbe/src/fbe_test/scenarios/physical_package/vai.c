/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * vai.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Vai scenario.
 * 
 *  This scenario is for a 1 port, 1 enclosure configuration, aka the static
 *  configuration.
 * 
 *  The test run in this case is:
 *  - Set the Additional Status Page and EMC Specific Status page 'Unsupported'
 *  - Insert an enclosure and some drives to it.
 *  - Check the new enclosure and the drives are in in ACTIVATE state
 *  - Verify the Additional Status Page and EMC Specific Status page are 
 *      still marked 'Unsupported'
 *
 * HISTORY
 *   11/24/2008:  Created. ArunKumar Selvapillai
 *   11/20/2009:  Migrated from zeus_ready_test. Bo Gao
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "physical_package_tests.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe_test_configurations.h"
#include "fbe_test_package_config.h"
#include "fbe_test_io_api.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_sim_server.h" 
#include "pp_utils.h"

/*************************
 *   MACRO DEFINITIONS
 *************************/
#define VAI_NUMBER_OF_OBJ 7 /* 1(board)+1(port)+1(enclosure)+2(physical drives)+2(logical drives)*/
#define VAI_ADDRESS_LENGTH 12
#define VAI_SERIAL_NUM_LENGTH  10
#define VAI_SLOT_NUM_LENGTH 2


/*********************************************************************
 * Components to support Table driven test methodology.
 * Following this approach, the parameters for the test will be stored
 * as rows in a table.
 * The test will then be executed for each row(i.e. set of parameters)
 * in the table.
 *********************************************************************/

/*!*******************************************************************
 * @enum seychelles_tests_t
 *********************************************************************
 * @brief The list of unique configurations to be tested in 
 * seychelles_test.
 *********************************************************************/

typedef enum vai_e
{
    VAI_TEST_NAGA,
    VAI_TEST_VIPER,
    VAI_TEST_DERRINGER,
    VAI_TEST_VOYAGER,
    VAI_TEST_TABASCO,
} vai_tests_t;

typedef struct vai_test_params_s
{
    fbe_u8_t *title;
    fbe_board_type_t            board_type;
    fbe_port_type_t             port_type;
    fbe_u32_t                   io_port_number;
    fbe_u32_t                   portal_number;
    fbe_u32_t                   backend_number;
    fbe_sas_enclosure_type_t    encl_type;
    fbe_u32_t                   encl_number;
    fbe_sas_drive_type_t        drive_type;
    fbe_u32_t                   slot_num_max ;
        
} vai_test_params_t;


/*!*******************************************************************
 * @var test_table
 *********************************************************************
 * @brief This is a table of set of parameters for which the test has 
 * to be executed.
 *********************************************************************/

static vai_test_params_t test_table[] = 
{
     {
    "NAGA", 
    FBE_BOARD_TYPE_FLEET, 
    FBE_PORT_TYPE_SAS_PMC,
    0,
    1,
    0,
    FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP,
    0,
    FBE_SAS_DRIVE_CHEETA_15K,     
    0
    },
    {
    "VIPER",
    FBE_BOARD_TYPE_FLEET, 
    FBE_PORT_TYPE_SAS_PMC, 
    0,
    1,
    0,
    FBE_SAS_ENCLOSURE_TYPE_VIPER,
    0,
    FBE_SAS_DRIVE_CHEETA_15K,
    2
    },
    {
    "DERRINGER",
    FBE_BOARD_TYPE_FLEET, 
    FBE_PORT_TYPE_SAS_PMC, 
    0,
    1,
    0,
    FBE_SAS_ENCLOSURE_TYPE_DERRINGER,
    0,
    FBE_SAS_DRIVE_CHEETA_15K,
    2
    },
    {
    "VOYAGER", 
    FBE_BOARD_TYPE_FLEET, 
    FBE_PORT_TYPE_SAS_PMC,
    0,
    1,
    0,
    FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM,
    0,
    FBE_SAS_DRIVE_CHEETA_15K,     
    0
    },
    {
    "TABASCO",
    FBE_BOARD_TYPE_FLEET, 
    FBE_PORT_TYPE_SAS_PMC, 
    0,
    1,
    0,
    FBE_SAS_ENCLOSURE_TYPE_TABASCO,
    0,
    FBE_SAS_DRIVE_CHEETA_15K,
    2
    },
};

/*!*******************************************************************
 * @def VAI_TEST_MAX
 *********************************************************************
 * @brief Number of test scenarios to be executed.
 *********************************************************************/
#define VAI_TEST_MAX              sizeof(test_table)/sizeof(test_table[0])

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!***************************************************************
 * @fn vai_verify_status_page(fbe_u32_t port, fbe_u32_t encl)
 ****************************************************************
 * @brief:
 *  Function to verify the Status Pages (Additional and EMC specific)
 *  are still marked 'Unsupported' and the configuration is created in 
 *  the topology.
 *
 * @return 
 *  FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  11/24/2008 - Created. Arunkumar Selvapillai
 *
 ****************************************************************/

fbe_status_t vai_verify_status_page(fbe_u32_t port, fbe_u32_t encl)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       object_handle;

    status = fbe_api_get_enclosure_object_id_by_location (port, encl, &object_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

    status = fbe_api_wait_for_encl_attr(object_handle, 
                    FBE_ENCL_ADDITIONAL_STATUS_PAGE_UNSUPPORTED, TRUE, FBE_ENCL_ENCLOSURE, 0, 5000);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, 
                    "Additional Status Diagnostic Page is still marked UNSUPPORTED in Enclosure Blob");
    
    status = fbe_api_wait_for_encl_attr(object_handle, 
                    FBE_ENCL_EMC_SPECIFIC_STATUS_PAGE_UNSUPPORTED, TRUE, FBE_ENCL_ENCLOSURE, 0, 5000);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, 
                    "EMC Specific Status Page is still marked UNSUPPORTED in Enclosure Blob");

    return status;
}

/*!***************************************************************
 * @fn vai_run
 ****************************************************************
 * @brief:
 *  Function to 
 *  -   Create a static configuration
 *  -   Mark the additional status page and EMC specific Page as
 *      'Unsupported'
 *  -   Verify the configuration is added to the topology.
 *
 * @return 
 *  FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  11/24/2008 - Created. Arunkumar Selvapillai
 *
 ****************************************************************/

fbe_status_t vai_run(vai_test_params_t * test)
{
    fbe_status_t                        status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                           object_handle = 0;
    fbe_u32_t                           port_idx = 0;
    fbe_port_number_t                   port_number;
    fbe_class_id_t                      class_id;

    fbe_terminator_sas_drive_info_t     sas_drive;

    fbe_api_terminator_device_handle_t  port_handle = 0;
    fbe_api_terminator_device_handle_t  encl_handle = 0;
    fbe_api_terminator_device_handle_t  drive_handle0 = 0;
    fbe_u32_t                           drive_index = 0;
    fbe_u32_t                           slot_num;
    fbe_terminator_sas_encl_info_t    sas_encl;
    fbe_u32_t                           num_handles;
	
    mut_printf(MUT_LOG_LOW, "=== configuring terminator ===\n");
    /* Before loading the physical package we initialize the terminator */
    status = fbe_api_terminator_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    /* Insert a board */
    status = fbe_test_pp_util_insert_armada_board ();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Insert a port */
    status = fbe_test_pp_util_insert_sas_pmc_port( test->io_port_number,test->portal_number,test->backend_number,&port_handle);

    if(test->slot_num_max)
    {
        /* Set the ESES pages Unsupported and see if the configuration gets loaded into the topology */
        status = fbe_api_terminator_mark_eses_page_unsupported(FBE_SCSI_RECEIVE_DIAGNOSTIC,
                                                                SES_PG_CODE_ADDL_ELEM_STAT);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        mut_printf(MUT_LOG_LOW, "Additional Status Diagnostic Page is marked UNSUPPORTED");
        
        status = fbe_api_terminator_mark_eses_page_unsupported(FBE_SCSI_RECEIVE_DIAGNOSTIC, 
                                                                SES_PG_CODE_EMC_ENCL_STAT);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        mut_printf(MUT_LOG_LOW, "EMC Specific Status Page is marked UNSUPPORTED");
    }
    /* Insert an enclosure */
    sas_encl.backend_number = test->backend_number;
    sas_encl.encl_number = test->encl_number;
    sas_encl.uid[0] = test->backend_number; // some unique ID.
    sas_encl.sas_address = FBE_BUILD_PORT_SAS_ADDRESS(test->backend_number);
    sas_encl.encl_type = test->encl_type;
    status  = fbe_test_insert_sas_enclosure (port_handle, &sas_encl, &encl_handle, &num_handles);

    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	
    for(drive_index = 0;drive_index< test->slot_num_max; drive_index++)
    {
    /* Insert a drives to the enclosure in slots 0 & 1*/
        sas_drive.backend_number = test->backend_number;
        sas_drive.encl_number = test->encl_number;
        sas_drive.slot_number = drive_index;
        sas_drive.drive_type = test->drive_type;
        sas_drive.block_size = 520;
        sas_drive.capacity = 0x10BD0;
        sas_drive.sas_address = FBE_BUILD_DISK_SAS_ADDRESS(test->backend_number, test->encl_number, drive_index);
        status  = fbe_test_insert_sas_drive (encl_handle, drive_index, &sas_drive, &drive_handle0);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK)
    }

	/* Initialize the fbe_api on server. */ 
    mut_printf(MUT_LOG_LOW, "%s: starting fbe_api...", __FUNCTION__);
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize the physical package. */
    mut_printf(MUT_LOG_LOW, "%s: starting physical package...", __FUNCTION__);
    fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    
    fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the expected number of objects */
    status = fbe_test_wait_for_term_pp_sync(20000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_LOW, "=== verifying configuration ===\n");

    /* We inserted a fleet board so we check for it. */
    /* Board is always object id 0 ??? Maybe we need an interface to get the board object id*/
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type....Passed");

    /* We should have exactly one port. */
    for (port_idx = 0; port_idx < FBE_TEST_ENCLOSURES_PER_BUS; port_idx++) 
    {
        if (port_idx < 1) 
        {
            /* Get the handle of the port and validate port exists*/
            status = fbe_api_get_port_object_id_by_location (port_idx, &object_handle);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

            /* Use the handle to get lifecycle state*/
            status = fbe_api_wait_for_object_lifecycle_state (object_handle, 
                FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME, FBE_PACKAGE_ID_PHYSICAL);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

            /* Use the handle to get port number*/
            status = fbe_api_get_object_port_number (object_handle, &port_number);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(port_number == port_idx);

            /* Validate that the enclosure is in READY state */
            status = fbe_zrt_wait_enclosure_status(port_idx, 0, 
                                FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

            for(slot_num = 0; slot_num < test->slot_num_max; slot_num++ )
            {
                /* Validate that the physical drives are in READY state */
                status = fbe_test_pp_util_verify_pdo_state(port_idx, test->encl_number, slot_num, 
                                    FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
            }
        }
        else 
        {
            /* Get the handle of the port and validate port exists or not*/
            status = fbe_api_get_port_object_id_by_location (port_idx, &object_handle);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(object_handle == FBE_OBJECT_ID_INVALID);
        }
    }

    /* Let's see how many objects we have...
        1 board + 1 port + 1 enclosure + 2 physical drives + 2 logical drives = 7 objects
     */  
     status = fbe_test_verify_term_pp_sync();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");

    mut_printf(MUT_LOG_LOW, "%s: configuration verification SUCCEEDED!", __FUNCTION__);

    /* Verify the Status Pages are still set as UNSUPPORTED */
    if(test->slot_num_max)
    {
	        status = vai_verify_status_page(0, 0);
	        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    }
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn vai()
 ****************************************************************
 * @brief:
 *  Function to load the config and run the 
 *  Vai scenario.
 *
 * @return 
 *  FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  11/24/2008 - Created. 
 *
 ****************************************************************/

void vai(void)
{
    fbe_status_t run_status = FBE_STATUS_OK;
    fbe_u32_t          test_count = 0;

    for(test_count = 0; test_count < VAI_TEST_MAX; test_count++)
    {
         run_status = vai_run(&test_table[test_count]);
         fbe_test_physical_package_tests_config_unload();
    }
    MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);
}

