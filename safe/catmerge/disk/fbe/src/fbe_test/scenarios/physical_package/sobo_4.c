/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * sobo_4.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains the sobo_4 scenario.
 * 
 *  This scenario is for a 1 port, 3 enclosures configuration, aka the naxos.xml
 *  configuration.
 * 
 *  The test run in this case is:
 *  1) Create and validate the configuration. (1-Port 3-Enclosures)
 *  2) Remove the drive in slot-0 of the depth-0 enclosure, such that the port receives a LOGOUT, 
 *      and the slot-0 ESES insert-bit is cleared. 
 *  3) Validate that the corresponding physical and logical drive objects are destroyed.
 *  4) Insert a different drive in slot-0 of the depth-0 enclosure,
 *     the slot-0 ESES insert-bit is set, but the port does not discover the same SMP address of the physical drive.
 *  5) Validate that the physical drive is created and that it remains in the SPECIALIZED state.
 *  6) Cause the port to discover a different SMP address of the new drive.
 *  7) Validate that the physical drive is promoted to a SAS drive, it is in the READY state,
 *     and the logical drive is created and also in the READY state
 *  8) Remove the drive in slot-14 of the depth-0 enclosure, the port receives a LOGOUT, 
 *     but the ESES insert-bit is not cleared. 
 *  9) Validate that the physical drive is in the ACTIVATE state.
 * 10) Insert a drive in slot-14 of the depth-0 enclosure, the port discovers the original SMP address
 *     of the physical drive and the ESES insert-bit remains set. 
 *     The INQUIRY data returned by the drive has changed. 
 * 11) Validate that the original physical and logical drives are destroyed, 
 *     and that new physical and logical drives are created and in the READY state.
 * 12) Remove the drive in slot-14 of the depth-0 enclosure, such that the port receives a LOGOUT, 
 *     but the ESES insert-bit is not cleared. 
 * 13) Validate that the physical drive is in the ACTIVATE state.
 * 14) Insert a drive in slot-14 of the depth-0 enclosure, the port discovers the original SMP address 
 *     of the physical drive and the slot-0 ESES insert-bit remains set. 
 *     In addition, the INQUIRY data returned by the drive remains the same. 
 * 15) Validate that both the physical drive and logical drive are in the READY state.
 *
 * HISTORY
 *   08/01/2008:  Created. CLC
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "physical_package_tests.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "pp_utils.h"

/*! @def SOBO_4_LIFECYCLE_NOTIFICATION_TIMEOUT 
 *  
 *  @brief This is the number of seconds we will wait on a notification
 */
#define SOBO_4_LIFECYCLE_NOTIFICATION_TIMEOUT  15000 /*wait maximum of 15 seconds*/

/*!**************************************************************
 * @fn fbe_status_t sobo_4_run(fbe_drive_type_t drive_family)
 ****************************************************************
 * @brief
 *  Run the sobo_4 test scenario for either SATA or SAS drives.
 *
 * @param drive_family - If the value is FBE_DRIVE_TYPE_SAS the
 *                       test is run using SAS drives. If not then
 *                       the test is run using SATA drives.
 *
 * @return - FBE_STATUS_OK if the sobo_4 test runs error free.
 *
 *
 ****************************************************************/
 static fbe_status_t sobo_4_run(fbe_drive_type_t drive_family)
{
    fbe_status_t status;
    fbe_u32_t port_number;
    fbe_u32_t slot_number;
    fbe_u32_t enclosure_number;
    ses_stat_elem_array_dev_slot_struct drive_slot_stat;
    fbe_class_id_t class_id;
    fbe_u32_t object_handle_p;
    fbe_terminator_sas_drive_info_t  sas_drive;
    fbe_terminator_sata_drive_info_t sata_drive;

    fbe_api_terminator_device_handle_t  drive_handle = 0;
    fbe_api_terminator_device_handle_t  encl_handle = 0;

    /* register the notification */
    fbe_test_pp_util_lifecycle_state_ns_context_t ns_context; 
    ns_context.timeout_in_ms = SOBO_4_LIFECYCLE_NOTIFICATION_TIMEOUT;
    fbe_test_pp_util_register_lifecycle_state_notification(&ns_context);


    /*******************************************
     * Test steps 1-4: Slot-0 and Enclourse-0
     ******************************************/

    port_number = 0;
    slot_number = 0;
    enclosure_number = 0;
  
    /* find an enclosure handle for this configuration */
    status = fbe_api_terminator_get_enclosure_handle(port_number, enclosure_number, &encl_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK)

    /* Remove and logout the drive with changing the inset bit.
     */
    status = fbe_api_terminator_get_drive_handle(port_number,      
                                                 enclosure_number,    
                                                 slot_number,      
                                                 &drive_handle); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK)


    if (drive_family == FBE_DRIVE_TYPE_SAS)
    {
        status = fbe_api_terminator_remove_sas_drive(drive_handle);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK)
    }
    else if (drive_family == FBE_DRIVE_TYPE_SATA)
    {
        status = fbe_api_terminator_remove_sata_drive(drive_handle);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK)
    }
    else
    {
        mut_printf(MUT_LOG_LOW, "=== sobo_4_run drive family not recognized ===\n");
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    MUT_ASSERT_TRUE(status == FBE_STATUS_OK)

    /* wait for physical drive destroy notification */
    ns_context.object_type = FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE;
    ns_context.expected_lifecycle_state = FBE_LIFECYCLE_STATE_DESTROY;
    fbe_test_pp_util_wait_lifecycle_state_notification (&ns_context);
  
    /* Make sure that physical drives are destroyed. */
    status = fbe_test_pp_util_verify_pdo_state(port_number, 
                                               enclosure_number, 
                                               slot_number, 
                                               FBE_LIFECYCLE_STATE_DESTROY, 
                                               SOBO_4_LIFECYCLE_NOTIFICATION_TIMEOUT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status)


    /* the slot-0 ESES insert-bit is set.
     */
    drive_slot_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    drive_slot_stat.dev_bypassed_a = 0;
    drive_slot_stat.dev_off = 0;
    drive_slot_stat.app_client_bypassed_a = 0;
    status = fbe_api_terminator_set_sas_enclosure_drive_slot_eses_status(encl_handle, slot_number, drive_slot_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Enclosure see the drive is inserted and drive is not logged in yet.
     * Verify physical drive stays in specialize state. */
    status = fbe_test_pp_util_verify_pdo_state(port_number, 
                                                enclosure_number, 
                                                slot_number, 
                                                FBE_LIFECYCLE_STATE_SPECIALIZE,
                                                18000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "=== sobo_4_run checking object states FBE_LIFECYCLE_STATE_SPECIALIZE Successful  ===\n");

   /* Restore a different drive in slot-0 of the depth-0 enclosure, 
     * such that the slot-0 ESES insert-bit is set, 
     * but the port does not discover the same SMP address of the physical drive.
     */
    /* Login to the new drive (cause the port to discover a different SMP address of the new drive)
     */
    if (drive_family == FBE_DRIVE_TYPE_SAS)
    {
        /* create SAS drive device */
        sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
        sas_drive.sas_address = 0x987654321;
        sas_drive.backend_number = port_number;
        sas_drive.encl_number = enclosure_number;
        sas_drive.slot_number = slot_number;
        sas_drive.block_size = 520;
        sas_drive.capacity = TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY;
        strcpy(sas_drive.drive_serial_number, "DE00FB00C");

        status = fbe_api_terminator_insert_sas_drive (encl_handle, slot_number, &sas_drive, &drive_handle);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    }
    else if (drive_family == FBE_DRIVE_TYPE_SATA)
    {
        /* create SATA drive device */
        sata_drive.drive_type = FBE_SATA_DRIVE_HITACHI_HUA;
        sata_drive.sas_address = 0x987654321;
        sata_drive.backend_number = port_number;
        sata_drive.encl_number = enclosure_number;
        sata_drive.slot_number = slot_number;
        sata_drive.block_size = 512;
        sata_drive.capacity = TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY;
        strcpy(sata_drive.drive_serial_number, "DE00FB00C");
        status = fbe_api_terminator_insert_sata_drive (encl_handle, slot_number, &sata_drive, &drive_handle);
    }
    else
    {
        mut_printf(MUT_LOG_LOW, "=== sobo_4_run drive family not recognized ===\n");
        status = FBE_STATUS_GENERIC_FAILURE;
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    }

    /* wait for physical drive ready notification */
    ns_context.object_type = FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE;
    ns_context.expected_lifecycle_state = FBE_LIFECYCLE_STATE_READY;
    fbe_test_pp_util_wait_lifecycle_state_notification (&ns_context);
        
    /* Verify that the physical drive is promoted to a SAS/SATA drive.
     */
    status = fbe_api_terminator_get_drive_handle(port_number, enclosure_number, slot_number, &drive_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
   
    /* Check for the physical drives on the enclosure.
     */
    status = fbe_api_get_physical_drive_object_id_by_location(port_number, enclosure_number, slot_number, &object_handle_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(object_handle_p, FBE_OBJECT_ID_INVALID);

    status = fbe_api_get_object_class_id(object_handle_p, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    if (drive_family == FBE_DRIVE_TYPE_SAS)
    {
        MUT_ASSERT_INT_EQUAL(FBE_CLASS_ID_SAS_PHYSICAL_DRIVE, class_id);    
    }
    else if (drive_family == FBE_DRIVE_TYPE_SATA)
    {
        MUT_ASSERT_INT_EQUAL(FBE_CLASS_ID_SATA_PHYSICAL_DRIVE, class_id);    
    }
    else
    {
        mut_printf(MUT_LOG_LOW, "=== sobo_4_run drive family not recognized ===\n");
    }
  
    /* Verify the physical drive is in the READY state, and the logical drive is created 
     * and also in the READY state.
     */
    status = fbe_test_pp_util_verify_pdo_state(port_number, 
                                               enclosure_number, 
                                               slot_number, 
                                               FBE_LIFECYCLE_STATE_READY,
                                               5000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== SOBO_4_run checking object states FBE_LIFECYCLE_STATE_READY Successful  ===\n");


    /*******************************************
     * Test steps 5-8: Slot-14 and Enclosure-0
     ******************************************/

    /* Remove the drive in slot-14 of the depth-0 enclosure,
     * the port receives a LOGOUT, but the ESES insert-bit is not cleared. 
     */
    slot_number = 14;

    /* Get the drive information.
     */
    status = fbe_api_terminator_get_drive_handle(port_number, enclosure_number, slot_number, &drive_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_api_terminator_force_logout_device (drive_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== SOBO_4_run force logout drive %d_%d_%d ===\n", port_number, enclosure_number, slot_number);

    status = fbe_test_pp_util_verify_pdo_state(port_number, 
                                                enclosure_number, 
                                                slot_number, 
                                                FBE_LIFECYCLE_STATE_ACTIVATE,
                                                5000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "=== sobo_4_run checking object states FBE_LIFECYCLE_STATE_ACTIVATE Successful  ===\n");

    /* Insert a drive, such that the port discovers the original SMP address of the physical drive
     * and the slot-0 ESES insert-bit remains set. However, the INQUIRY data returned by the drive has changed.
     * the original physical and logical drives will be destroyed and new objects will be created.
     */
    status = fbe_api_terminator_get_drive_handle(port_number, enclosure_number, slot_number, &drive_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK)
   status = fbe_api_terminator_set_device_attribute(drive_handle, "SN", "Dtest123450000000000");
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK)    
    status = fbe_api_terminator_force_login_device (drive_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "=== SOBO_4_run force login drive %d_%d_%d with different SN ===\n", port_number, enclosure_number, slot_number);

    mut_printf(MUT_LOG_LOW, "=== SOBO_4_run wait for logical drive %d_%d_%d to be destroyed ===\n", port_number, enclosure_number, slot_number);
    /* wait for logical drive destroy notification */
    ns_context.object_type = FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE;
    ns_context.expected_lifecycle_state = FBE_LIFECYCLE_STATE_DESTROY;
    fbe_test_pp_util_wait_lifecycle_state_notification (&ns_context);

    mut_printf(MUT_LOG_LOW, "=== SOBO_4_run wait for new physical and logical drive %d_%d_%d to be ready ===\n", port_number, enclosure_number, slot_number);
    /* Verify the new physical and logical drives are created and in the READY state. */
    status = fbe_test_pp_util_verify_pdo_state(port_number, 
                                               enclosure_number, 
                                               slot_number, 
                                               FBE_LIFECYCLE_STATE_READY,
                                               10000);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "=== sobo_4_run FBE_LIFECYCLE_STATE_READY Successful  ===\n");
    
    /* Save the SAS drive information.*/
    /* Get the drive handle */
    status = fbe_api_terminator_get_drive_handle(port_number, enclosure_number, slot_number, &drive_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK)

    if (drive_family == FBE_DRIVE_TYPE_SAS)
    {
        /* Get the SAS drive attributes */
        status = fbe_api_terminator_get_sas_drive_info(drive_handle, &sas_drive);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK)
    }
    else if (drive_family == FBE_DRIVE_TYPE_SATA)
    {
        /* Get the SAS drive attributes */
        status = fbe_api_terminator_get_sata_drive_info(drive_handle, &sata_drive);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK)
    }    
    else
    {
        mut_printf(MUT_LOG_LOW, "=== sobo_4_run drive family not recognized ===\n");
        status = FBE_STATUS_GENERIC_FAILURE;
    }    

    /* Remove the drive in slot-14 of the depth-0 enclosure,
     * the port receives a LOGOUT, but the ESES insert-bit is not cleared. 
     */
    mut_printf(MUT_LOG_LOW, "=== sobo_4_run Force logout  ===\n");
    status = fbe_api_terminator_force_logout_device (drive_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
     
    /* Verify that the corresponding physical 
     * and logical drive objects are in the ACTIVATE state.
     */
    status = fbe_test_pp_util_verify_pdo_state(port_number, 
                                               enclosure_number, 
                                               slot_number, 
                                               FBE_LIFECYCLE_STATE_ACTIVATE, 
                                               5000);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "=== sobo_4_run After force destroy in FBE_LIFECYCLE_STATE_ACTIVATE Successful  ===\n");

    /* Reinsert a drive, such that the port discovers the original SMP address of the physical drive
     * and the slot-0 ESES insert-bit remains set. The INQUIRY data returned by the drive remains the same.
     */
    status = fbe_api_terminator_activate_device (drive_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* wait for logical drive ready notification */
    ns_context.object_type = FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE;
    ns_context.expected_lifecycle_state = FBE_LIFECYCLE_STATE_READY;
    fbe_test_pp_util_wait_lifecycle_state_notification (&ns_context);

    /* Verify that the original physical and logical drives come back to READY state.
     */
    status = fbe_test_pp_util_verify_pdo_state(port_number, 
                                               enclosure_number, 
                                               slot_number, 
                                               FBE_LIFECYCLE_STATE_READY,
                                               10000);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== sobo_4_run FBE_LIFECYCLE_STATE_READY Successful  ===\n");

    fbe_test_pp_util_unregister_lifecycle_state_notification(&ns_context);
  
   return FBE_STATUS_OK;
}

void sobo_4(void)
{
        fbe_status_t run_status;

        /* The sobo_4 test is run with sas drives so we specify the drive
         * family as FBE_DRIVE_TYPE_SAS.
         */
        run_status = sobo_4_run(FBE_DRIVE_TYPE_SAS);
        
        MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);
}

void sobo_4_sata(void)
{
        fbe_status_t run_status;

        /* The sobo_4_sata test is run with sata drives so we specify the drive
         * family as FBE_DRIVE_TYPE_SATA.
         */
        run_status = sobo_4_run(FBE_DRIVE_TYPE_SATA);
        
        MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);
}
