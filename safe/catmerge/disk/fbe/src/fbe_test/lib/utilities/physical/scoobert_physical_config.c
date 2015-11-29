/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file scoobert_physical_config.c
 ***************************************************************************
 *
 * @brief
 *  This file contains setup for the scoobert tests.
 *
 * @version
 *   12/02/2009 - Created. Dhaval Patel
 *   8/30/2011 - Modified Vishnu Sharma.
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
#include "sep_dll.h"
#include "physical_package_dll.h"

#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "pp_utils.h"


/*!*******************************************************************
 * @def SCOOBERT_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects we will create.
 *        (1 board + 3 port + 5 encl + 75 pdo) 
 * Note: Whenever we change make sure to change in scoobert_test.c file.
 *********************************************************************/
#define SCOOBERT_TEST_NUMBER_OF_PHYSICAL_OBJECTS 84

/*!*******************************************************************
 * @def SCOOBERT_TEST_DEFAULT_DRIVE_CAPACITY_ADJUSTMENT
 *********************************************************************
 * @brief Default number of blocks to adjust capacity to.
 *********************************************************************/
#define SCOOBERT_TEST_DEFAULT_DRIVE_CAPACITY_ADJUSTMENT 0x10000

/*!*******************************************************************
 * @def SCOOBERT_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive with different drive
 * and enclosure slot.
 *********************************************************************/
static fbe_lba_t scoobert_encl0_0_capacity[15] = { 0 };

static fbe_lba_t scoobert_encl0_1_capacity[15] = { 0 };

static fbe_lba_t scoobert_encl0_2_capacity[15] = { 0 };

static fbe_lba_t scoobert_encl1_0_capacity[15] = { 0 };

static fbe_lba_t scoobert_encl2_0_capacity[15] = { 0 };


/*************************    
 *   FUNCTION DEFINITIONS     
 *************************/   

/*!**************************************************************
 * scoobert_physical_config_populate_drive_capacity()
 ****************************************************************
 * @brief
 *  Populate the array of drive capacitys for scoobert test().
 *
 * @param None.              
 *
 * @return None.
 *
 * @author
 *  06/22/2012  Ron Proulx  - Created.
 ****************************************************************/
static void scoobert_physical_config_populate_drive_capacity(void)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_lba_t min_sd_cap  = TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY;
    fbe_lba_t min_nsd_cap = TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY;
    fbe_lba_t std_nsd_cap = (TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY + SCOOBERT_TEST_DEFAULT_DRIVE_CAPACITY_ADJUSTMENT);
    fbe_lba_t sml_nsd_cap = std_nsd_cap - (1 * SCOOBERT_TEST_DEFAULT_DRIVE_CAPACITY_ADJUSTMENT);
    fbe_lba_t med_nsd_cap = std_nsd_cap + (1 * SCOOBERT_TEST_DEFAULT_DRIVE_CAPACITY_ADJUSTMENT);
    fbe_lba_t lrg_nsd_cap = std_nsd_cap + (2 * SCOOBERT_TEST_DEFAULT_DRIVE_CAPACITY_ADJUSTMENT);
    fbe_lba_t big_nsd_cap = std_nsd_cap + (3 * SCOOBERT_TEST_DEFAULT_DRIVE_CAPACITY_ADJUSTMENT);

    /* Validate minimum system drive and non-system drive capacities. */
    if ((min_sd_cap == FBE_LBA_INVALID) ||
        (min_sd_cap <= min_nsd_cap    )    )
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: unexpected system cap: 0x%llx", 
                   __FUNCTION__, (unsigned long long)min_sd_cap);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Validate minimum system drive and non-system drive capacities. */
    if ((min_nsd_cap == FBE_LBA_INVALID) ||
        (min_nsd_cap <= 0x10000    )        )
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: unexpected non-sytem cap: 0x%llx", 
                   __FUNCTION__, (unsigned long long)min_nsd_cap);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Simply a visual way to see the capacities that we populate.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s: system cap: 0x%llx standard cap: 0x%llx small cap: 0x%llx", 
               __FUNCTION__, (unsigned long long)min_sd_cap, (unsigned long long)std_nsd_cap, (unsigned long long)sml_nsd_cap);
    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s: medium cap: 0x%llx large cap: 0x%llx big cap: 0x%llx", 
               __FUNCTION__, (unsigned long long)med_nsd_cap, (unsigned long long)lrg_nsd_cap, (unsigned long long)big_nsd_cap);
 
    /* 0_0_slot*/
    scoobert_encl0_0_capacity[ 0] = min_sd_cap + std_nsd_cap;   /* RG[0]: 0_0_0: */
    scoobert_encl0_0_capacity[ 1] = min_sd_cap + std_nsd_cap;   /* RG[0]: 0_0_1: */
    scoobert_encl0_0_capacity[ 2] = min_sd_cap + std_nsd_cap;   /* 0_0_2: */
    scoobert_encl0_0_capacity[ 3] = min_sd_cap + std_nsd_cap;   /* RG[0]: 0_0_3: */
    scoobert_encl0_0_capacity[ 4] = min_sd_cap + std_nsd_cap;   /* 0_0_4: */
    scoobert_encl0_0_capacity[ 5] = min_sd_cap + std_nsd_cap;   /* 0_0_5: */
    scoobert_encl0_0_capacity[ 6] = min_sd_cap + std_nsd_cap;   /* 0_0_6: */
    scoobert_encl0_0_capacity[ 7] = sml_nsd_cap;                /* Drive 7 */
    scoobert_encl0_0_capacity[ 8] = std_nsd_cap;                /* Drive 8 */
    scoobert_encl0_0_capacity[ 9] = std_nsd_cap;                /* Drive 9 */
    scoobert_encl0_0_capacity[10] = std_nsd_cap;                /* Drive 10 SATA*/
    scoobert_encl0_0_capacity[11] = std_nsd_cap;                /* Drive 11 SATA */
    scoobert_encl0_0_capacity[12] = std_nsd_cap;                /* Drive 12 SATA */
    scoobert_encl0_0_capacity[13] = std_nsd_cap;                /* Drive 13 */
    scoobert_encl0_0_capacity[14] = std_nsd_cap;                /* Drive 14 HS3*/

    /* 0_1_slot */
    scoobert_encl0_1_capacity[ 0] = std_nsd_cap;                /* Drive 0 */
    scoobert_encl0_1_capacity[ 1] = std_nsd_cap;                /* Drive 1 */
    scoobert_encl0_1_capacity[ 2] = std_nsd_cap;                /* Drive 2 */
    scoobert_encl0_1_capacity[ 3] = std_nsd_cap;                /* Drive 3 */
    scoobert_encl0_1_capacity[ 4] = sml_nsd_cap;                /* Drive 4 */
    scoobert_encl0_1_capacity[ 5] = big_nsd_cap;                /* Drive 5 */
    scoobert_encl0_1_capacity[ 6] = big_nsd_cap;                /* Drive 6 */
    scoobert_encl0_1_capacity[ 7] = big_nsd_cap;                /* Drive 7 */
    scoobert_encl0_1_capacity[ 8] = big_nsd_cap;                /* Drive 8 */
    scoobert_encl0_1_capacity[ 9] = std_nsd_cap;                /* Drive 9 */
    scoobert_encl0_1_capacity[10] = big_nsd_cap;                /* Drive 10 */
    scoobert_encl0_1_capacity[11] = std_nsd_cap;                /* Drive 11 */
    scoobert_encl0_1_capacity[12] = std_nsd_cap;                /* Drive 12 */
    scoobert_encl0_1_capacity[13] = std_nsd_cap;                /* Drive 13 */
    scoobert_encl0_1_capacity[14] = std_nsd_cap;                /* Drive 14 */

    /* 0_2_slot */
    scoobert_encl0_2_capacity[ 0] = std_nsd_cap;                /* Drive 0 */
    scoobert_encl0_2_capacity[ 1] = std_nsd_cap;                /* Drive 1 */
    scoobert_encl0_2_capacity[ 2] = med_nsd_cap;                /* Drive 2 */
    scoobert_encl0_2_capacity[ 3] = std_nsd_cap;                /* Drive 3 */
    scoobert_encl0_2_capacity[ 4] = sml_nsd_cap;                /* Drive 4 */
    scoobert_encl0_2_capacity[ 5] = big_nsd_cap;                /* Drive 5 */
    scoobert_encl0_2_capacity[ 6] = big_nsd_cap;                /* Drive 6 */
    scoobert_encl0_2_capacity[ 7] = big_nsd_cap;                /* Drive 7 */
    scoobert_encl0_2_capacity[ 8] = big_nsd_cap;                /* Drive 8 */
    scoobert_encl0_2_capacity[ 9] = std_nsd_cap;                /* Drive 9 */
    scoobert_encl0_2_capacity[10] = std_nsd_cap;                /* RG[1]: 0_2_10 virtual drive metadata should be at lowest. (std_nsd_cap) */
    scoobert_encl0_2_capacity[11] = big_nsd_cap;                /* RG[1]: 0_2_11 virtual drive metadata should be at lowest. (std_nsd_cap) */
    scoobert_encl0_2_capacity[12] = med_nsd_cap;                /* RG[1]: 0_2_12 virtual drive metadata should be at lowest. (std_nsd_cap) */
    scoobert_encl0_2_capacity[13] = std_nsd_cap;                /* RG[1]: 0_2_13 virtual drive metadata should be at lowest. (std_nsd_cap) */
    scoobert_encl0_2_capacity[14] = std_nsd_cap;                /* RG[1]: 0_2_14 virtual drive metadata should be at lowest. (std_nsd_cap) */


    /* 1_0_slot */
    scoobert_encl1_0_capacity[ 0] = min_sd_cap + std_nsd_cap;   /* 1_0_0:  */
    scoobert_encl1_0_capacity[ 1] = min_sd_cap + std_nsd_cap;   /* 1_0_1:  */
    scoobert_encl1_0_capacity[ 2] = min_sd_cap + std_nsd_cap;   /* 1_0_2:  */
    scoobert_encl1_0_capacity[ 3] = min_sd_cap + std_nsd_cap;   /* 1_0_3: */
    scoobert_encl1_0_capacity[ 4] = std_nsd_cap;                /* Drive 4 */
    scoobert_encl1_0_capacity[ 5] = std_nsd_cap;                /* Drive 5 */
    scoobert_encl1_0_capacity[ 6] = std_nsd_cap;                /* Drive 6 HS2*/
    scoobert_encl1_0_capacity[ 7] = std_nsd_cap;                /* Drive 7 */
    scoobert_encl1_0_capacity[ 8] = std_nsd_cap;                /* Drive 8 */
    scoobert_encl1_0_capacity[ 9] = std_nsd_cap;                /* Drive 9 */
    scoobert_encl1_0_capacity[10] = std_nsd_cap;                /* Drive 10 */
    scoobert_encl1_0_capacity[11] = std_nsd_cap;                /* Drive 11 */
    scoobert_encl1_0_capacity[12] = std_nsd_cap;                /* Drive 12 */
    scoobert_encl1_0_capacity[13] = std_nsd_cap;                /* Drive 13 */
    scoobert_encl1_0_capacity[14] = std_nsd_cap;                /* Drive 14 sas hs2*/

    /* 2_0_slot */
    scoobert_encl2_0_capacity[ 0] = min_sd_cap + std_nsd_cap;   /* 2_0_0:  */
    scoobert_encl2_0_capacity[ 1] = min_sd_cap + std_nsd_cap;   /* 2_0_1:  */
    scoobert_encl2_0_capacity[ 2] = min_sd_cap + std_nsd_cap;   /* 2_0_2:  */
    scoobert_encl2_0_capacity[ 3] = min_sd_cap + std_nsd_cap;   /* 2_0_3:  */
    scoobert_encl2_0_capacity[ 4] = sml_nsd_cap;                /* Drive 4 */
    scoobert_encl2_0_capacity[ 5] = big_nsd_cap;                /* Drive 5 */
    scoobert_encl2_0_capacity[ 6] = big_nsd_cap;                /* Drive 6 */
    scoobert_encl2_0_capacity[ 7] = big_nsd_cap;                /* Drive 7 */
    scoobert_encl2_0_capacity[ 8] = big_nsd_cap;                /* Drive 8 */
    scoobert_encl2_0_capacity[ 9] = std_nsd_cap;                /* Drive 9 */
    scoobert_encl2_0_capacity[10] = big_nsd_cap;                /* Drive 10 */
    scoobert_encl2_0_capacity[11] = std_nsd_cap;                /* Drive 11 */
    scoobert_encl2_0_capacity[12] = med_nsd_cap;                /* Drive 12 */
    scoobert_encl2_0_capacity[13] = std_nsd_cap;                /* Drive 13 */
    scoobert_encl2_0_capacity[14] = std_nsd_cap;                /* Drive 14 */

    return;
}

/*!**************************************************************
 * scoobert_physical_config()
 ****************************************************************
 * @brief
 *  Configure the scoobert test's physical configuration.
 *
 * @param None.              
 *
 * @return None.
 * 
 * @note    Number of objects we will create.
 *        (1 board + 3 port + 5 encl + 75 pdo) = SCOOBERT_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 * 
 * @author
 *  9/1/2009 - Created. Rob Foley
 *  10/08/2012  Ron Proulx  - Updated.
 ****************************************************************/
void scoobert_physical_config(fbe_block_size_t block_size)
{
    fbe_status_t                        status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                           total_boards = 0;
    fbe_u32_t                           total_ports = 0;
    fbe_u32_t                           total_enclosures = 0;
    fbe_u32_t                           total_pdos = 0;
    fbe_u32_t                           total_expected_objects = 0; 
    fbe_u32_t                           actual_total_objects = 0;
    fbe_class_id_t                      class_id;
    fbe_api_terminator_device_handle_t  port0_handle;
    fbe_api_terminator_device_handle_t  port1_handle;
    fbe_api_terminator_device_handle_t  port2_handle;
    fbe_api_terminator_device_handle_t  encl0_0_handle;
    fbe_api_terminator_device_handle_t  encl0_1_handle;
    fbe_api_terminator_device_handle_t  encl0_2_handle;
    fbe_api_terminator_device_handle_t  encl1_0_handle;
    fbe_api_terminator_device_handle_t  encl2_0_handle;
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_u32_t slot;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: using new creation API and Terminator Class Management\n", __FUNCTION__);
    mut_printf(MUT_LOG_LOW, "=== configuring terminator ===\n");
    /*before loading the physical package we initialize the terminator*/
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* insert a board
     */
    status = fbe_test_pp_util_insert_board_of_type(SPID_DEFIANT_M4_HW_TYPE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    total_boards++;

    /* insert a port
     */
    fbe_test_pp_util_insert_sas_pmc_port(1, /* io port */
                                         2, /* portal */
                                         0, /* backend number */ 
                                         &port0_handle);
    fbe_test_pp_util_insert_sas_pmc_port(2, /* io port */
                                         2, /* portal */
                                         1, /* backend number */ 
                                         &port1_handle);
    fbe_test_pp_util_insert_sas_pmc_port(3, /* io port */
                                         2, /* portal */
                                         2, /* backend number */ 
                                         &port2_handle);
    total_ports += 3;


    /* insert an enclosures to port 0
     */
    fbe_test_pp_util_insert_viper_enclosure(0, 0, port0_handle, &encl0_0_handle);
    fbe_test_pp_util_insert_viper_enclosure(0, 1, port0_handle, &encl0_1_handle);
    fbe_test_pp_util_insert_viper_enclosure(0, 2, port0_handle, &encl0_2_handle);
    total_enclosures += 3;

    /* insert an enclosures to port 1
     */
    fbe_test_pp_util_insert_viper_enclosure(1, 0, port1_handle, &encl1_0_handle);
    total_enclosures++;

    /* insert an enclosures to port 2
     */
    fbe_test_pp_util_insert_viper_enclosure(2, 0, port2_handle, &encl2_0_handle);
    total_enclosures++;
    

    /* Initialize the drive capacities. */
    scoobert_physical_config_populate_drive_capacity();

    /* Insert drives for enclosures.
     */
    for ( slot = 0; slot < 15; slot++)
    {
        if (( slot == 12 ) ||( slot == 11 ) ||( slot == 10 ))
        {
            fbe_test_pp_util_insert_sas_flash_drive(0, 0, slot, 520, scoobert_encl0_0_capacity[slot], &drive_handle);
        }
        else    
        {
            if((slot == 0) || (slot == 1) || (slot == 2) || (slot == 3) || (slot == 4))
            {
                fbe_test_pp_util_insert_sas_drive(0, 0, slot, 520, scoobert_encl0_0_capacity[slot], &drive_handle);
            } 
            else
            {
                fbe_test_pp_util_insert_sas_drive(0, 0, slot, block_size, scoobert_encl0_0_capacity[slot], &drive_handle);
            }
        }
    }
    total_pdos += 15;

    for ( slot = 0; slot < 15; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 1, slot, block_size, scoobert_encl0_1_capacity[slot], &drive_handle);
    }
    total_pdos += 15;

    for ( slot = 0; slot < 15; slot++)
    {
        if (( slot == 0 ) || ( slot == 5 ))
        {
            fbe_test_pp_util_insert_sas_drive(0, 2, slot, 520, scoobert_encl0_2_capacity[slot], &drive_handle);
        }
        else if ((slot == 1) || ( slot == 6 ))
        {
            fbe_test_pp_util_insert_sas_drive(0, 2, slot, 4160, scoobert_encl0_2_capacity[slot], &drive_handle);
        }
        else
        {
            fbe_test_pp_util_insert_sas_drive(0, 2, slot, block_size, scoobert_encl0_2_capacity[slot], &drive_handle);
        }
    }
    total_pdos += 15;

    for ( slot = 0; slot < 15; slot++)
    {
        if ( slot == 12 )
        {    
            fbe_test_pp_util_insert_sas_flash_drive(1, 0, slot, 520, scoobert_encl1_0_capacity[slot], &drive_handle);
        }
        else    
        {
            fbe_test_pp_util_insert_sas_drive(1, 0, slot, block_size, scoobert_encl1_0_capacity[slot], &drive_handle);
        }
    }
    total_pdos += 15;

    for ( slot = 0; slot < 15; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(2, 0, slot, block_size, scoobert_encl2_0_capacity[slot], &drive_handle);
    }
    total_pdos += 15;

    /* validate the total_expected objects against the previously calculated constant */
    total_expected_objects = total_boards + total_ports + total_enclosures + total_pdos;
    if (total_expected_objects != SCOOBERT_TEST_NUMBER_OF_PHYSICAL_OBJECTS)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s Actual total: %d doesnt match expected: %d change SCOOBERT_TEST_NUMBER_OF_PHYSICAL_OBJECTS",
                   __FUNCTION__, total_expected_objects, SCOOBERT_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
        mut_printf(MUT_LOG_TEST_STATUS, "=== Actual boards: %d ports: %d enclosures: %d pdos: %d",
                   total_boards, total_ports, total_enclosures, total_pdos);
    }

    /* Init fbe api on server */
    mut_printf(MUT_LOG_LOW, "=== starting Server FBE_API ===\n");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== starting physical package===\n");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== set Physical Package entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the expected number of objects */
    mut_printf(MUT_LOG_LOW, "=== %s waiting for %d physical objects start ===",__FUNCTION__, total_expected_objects);
    status = fbe_api_wait_for_number_of_objects(total_expected_objects, 20000, FBE_PACKAGE_ID_PHYSICAL);
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
    status = fbe_api_get_total_objects(&actual_total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(actual_total_objects == total_expected_objects);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");

    return;
}
/******************************************
 * end scoobert_physical_config()
 ******************************************/

/*************************
 * end file scoobert_physical_config.c
 *************************/



