 /***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file elmo_physical_config.c
 ***************************************************************************
 *
 * @brief
 *  This file contains setup for the elmo tests.
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
 * @def ELMO_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects we will create.
 *        (1 board + 1 port + 4 encl + 60 pdo) 
 *
 *********************************************************************/
#define ELMO_TEST_NUMBER_OF_PHYSICAL_OBJECTS 66 

/*!*******************************************************************
 * @def ELMO_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive.
 *
 *********************************************************************/
#define ELMO_TEST_DRIVE_CAPACITY (TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY)

/*!*******************************************************************
 * @def ELMO_DRIVES_PER_ENCL
 *********************************************************************
 * @brief Allows us to change how many drives per enclosure we have.
 *
 *********************************************************************/
#define ELMO_DRIVES_PER_ENCL 15

/*!*******************************************************************
 * @def ELMO_NUM_SYTEM_DRIVES
 *********************************************************************
 * @brief Allows us to change how many system drives we have.
 *
 *********************************************************************/
#define ELMO_NUM_SYTEM_DRIVES 4


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!**************************************************************
 * elmo_create_physical_config()
 ****************************************************************
 * @brief
 *  Configure the elmo test's physical configuration. Note that
 *  only 4160 block SAS and 520 block SAS drives are supported.
 *
 * @param b_use_4160 - TRUE if we will use both 4160 and 520.              
 *
 * @return None.
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/

void elmo_create_physical_config(fbe_bool_t b_use_4160)
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
    fbe_u32_t secondary_block_size;

    if (b_use_4160)
    {
        secondary_block_size = 4160;
    }
    else
    {
        secondary_block_size = 520;
    }

    mut_printf(MUT_LOG_LOW, "=== %s configuring terminator ===", __FUNCTION__);
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
    for ( slot = 0; slot < ELMO_DRIVES_PER_ENCL; slot++)
    {
        if ( slot < ELMO_NUM_SYTEM_DRIVES )
        {
            fbe_test_pp_util_insert_sas_drive(0, 0, slot, 520, TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY, &drive_handle);
        }
        else{
            fbe_test_pp_util_insert_sas_drive(0, 0, slot, 520, ELMO_TEST_DRIVE_CAPACITY, &drive_handle);
        }
    }

    for ( slot = 0; slot < ELMO_DRIVES_PER_ENCL; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 1, slot, 520, ELMO_TEST_DRIVE_CAPACITY, &drive_handle);
    }

    /*! @note Native SATA drives are no longer supported.
     */
    for ( slot = 0; slot < ELMO_DRIVES_PER_ENCL; slot++)
    {
         fbe_test_pp_util_insert_sas_drive(0, 2, slot, secondary_block_size, ELMO_TEST_DRIVE_CAPACITY, &drive_handle);
    }

    /*! @note Native SATA drives are no longer supported.
     */
    for ( slot = 0; slot < ELMO_DRIVES_PER_ENCL; slot++)
    {
         fbe_test_pp_util_insert_sas_drive(0, 3, slot, secondary_block_size, ELMO_TEST_DRIVE_CAPACITY, &drive_handle);
    }

    mut_printf(MUT_LOG_LOW, "=== starting physical package===");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== set Physical Package entries ===");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	/* Init fbe api on server */
    mut_printf(MUT_LOG_LOW, "=== starting Server FBE_API ===");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(ELMO_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 60000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_LOW, "=== verifying configuration ===");

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
    MUT_ASSERT_TRUE(total_objects == ELMO_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");
    fbe_test_pp_util_enable_trace_limits();
    return;
}
/******************************************
 * end elmo_create_physical_config()
 ******************************************/

/*!**************************************************************
 * elmo_physical_config()
 ****************************************************************
 * @brief
 *  Configure the elmo test's physical configuration. Note that
 *  only 4160 block SAS and 520 block SAS drives are supported.
 *
 * @param None.              
 *
 * @return None.
 *
 * @author
 *  12/17/2010 - Created. Rob Foley
 *
 ****************************************************************/

void elmo_physical_config(void)
{
    elmo_create_physical_config(FBE_FALSE /* Only use drives with 520-bps */);
    return;
}
/******************************************
 * end elmo_physical_config()
 ******************************************/

/*!**************************************************************
 * elmo_create_physical_config_for_rg()
 ****************************************************************
 * @brief
 *  Configure the test's physical configuration based on the
 *  raid groups we intend to create.
 *
 * @param rg_config_p - We determine how many of each drive type
 *                      (520 or 4160) we need from the rg configs.
 * @param num_raid_groups - Number of raid groups in array
 *
 * @return None.
 *
 * @author
 *  4/30/2010 - Created. Rob Foley
 *
 ****************************************************************/

void elmo_create_physical_config_for_rg(fbe_test_rg_configuration_t *rg_config_p,
                                        fbe_u32_t num_raid_groups)
{
    fbe_test_create_physical_config_for_rg(rg_config_p,num_raid_groups);
    return;
}
/******************************************
 * end elmo_create_physical_config_for_rg()
 ******************************************/

/*************************
 * end file elmo_physical_config.c
 *************************/


