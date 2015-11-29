 /***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file bluemeanie_physical_config.c
 ***************************************************************************
 *
 * @brief
 *  This file contains setup for the bluemeanie tests.
 *
 * @version
 *   9/1/2009 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "sep_tests.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_database.h"
#include "sep_dll.h"
#include "physical_package_dll.h"
#include "fbe_terminator_miniport_interface.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "pp_utils.h"


/*!*******************************************************************
 * @def BLUEMEANIE_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects we will create.
 *        (1 board + 1 port + 4 encl + 60 pdo) 
 *
 *********************************************************************/
#define BLUEMEANIE_TEST_NUMBER_OF_PHYSICAL_OBJECTS 66 

/*!*******************************************************************
 * @def BLUEMEANIE_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive.
 *
 *********************************************************************/
#define BLUEMEANIE_TEST_DRIVE_CAPACITY ((TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY) * 4)

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!**************************************************************
 * bluemeanie_physical_config()
 ****************************************************************
 * @brief
 *  Configure the bluemeanie test's physical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/

void bluemeanie_physical_config(void)
{
    fbe_status_t                        status  = FBE_STATUS_GENERIC_FAILURE;

    fbe_u32_t                           total_objects = 0;
    fbe_class_id_t                      class_id;

    fbe_api_terminator_device_handle_t  port0_handle;
    fbe_api_terminator_device_handle_t  encl0_0_handle;
    fbe_api_terminator_device_handle_t  encl0_1_handle;
    fbe_api_terminator_device_handle_t  encl0_2_handle;
    fbe_api_terminator_device_handle_t  encl0_3_handle;
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_u32_t slot;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: using new creation API and Terminator Class Management\n", __FUNCTION__);
    mut_printf(MUT_LOG_LOW, "=== configuring terminator ===\n");
    /*before loading the physical package we initialize the terminator*/
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* insert a board
     */
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* insert a port
     */
    fbe_test_pp_util_insert_sas_pmc_port(1, /* io port */
                                 2, /* portal */
                                 0, /* backend number */ 
                                 &port0_handle);

    /* insert an enclosures to port 0
     */
    fbe_test_pp_util_insert_viper_enclosure(0, 0, port0_handle, &encl0_0_handle);
    fbe_test_pp_util_insert_viper_enclosure(0, 1, port0_handle, &encl0_1_handle);
    fbe_test_pp_util_insert_viper_enclosure(0, 2, port0_handle, &encl0_2_handle);
    fbe_test_pp_util_insert_viper_enclosure(0, 3, port0_handle, &encl0_3_handle);

    /* Insert drives for enclosures.
     */
    for ( slot = 0; slot < 15; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 0, slot, 520, BLUEMEANIE_TEST_DRIVE_CAPACITY, &drive_handle);
    }

    for ( slot = 0; slot < 15; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 1, slot, 520, BLUEMEANIE_TEST_DRIVE_CAPACITY, &drive_handle);
    }

    for ( slot = 0; slot < 15; slot++)
    {
         fbe_test_pp_util_insert_sas_drive(0, 2, slot, 4160, BLUEMEANIE_TEST_DRIVE_CAPACITY, &drive_handle);
    }

    for ( slot = 0; slot < 15; slot++)
    {
         fbe_test_pp_util_insert_sas_drive(0, 3, slot, 4160, BLUEMEANIE_TEST_DRIVE_CAPACITY, &drive_handle);
    }

    mut_printf(MUT_LOG_LOW, "=== starting physical package===\n");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== set Physical Package entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	/* Init fbe api on server */
    mut_printf(MUT_LOG_LOW, "=== starting Server FBE_API ===\n");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(BLUEMEANIE_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_LOW, "=== verifying configuration ===\n");

    /* We inserted a armada board so we check for it.
     * board is always assumed to be object id 0.
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type....Passed");

    /* Make sure we have the expected number of objects.
     */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == BLUEMEANIE_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");
    return;
}
/******************************************
 * end bluemeanie_physical_config()
 ******************************************/

/*************************
 * end file bluemeanie_physical_config.c
 *************************/


