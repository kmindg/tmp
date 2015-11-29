#include "physical_package_tests.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe_test_configurations.h"
#include "pp_utils.h"

/*TODO:
 * 1) Create a configuration with 1 SAS BE (1 encl 15 drives attached), 1 FC FE and 1 SAS UNC.
 * 2) Set hwd info and SFP info.
 * 3) Query the SFP info.
 * 4) Modify SFP information for FC.
 * 5) Query to confirm that the Port object has updated information.
 * 6) Update the SFP information 3-4 times.
 * 7) Query to confirm that the Port object has updated information.
 */
#define ALMANCE_CONFIG_MAX_DRIVE_COUNT        12
char * almance_short_desc = "Creation of different protocol ports ith different roles.";
char * almance_long_desc =
    "\n"
    "\n"
    "The Almance scenario tests port scanning and creation of port objects for SAS,FC and iScsi.\n"
    "It includes:\n" 
    "     - creation of Frontend, Backend and Uncommitted ports\n"
    "     - SFP notification handling \n"
    "     - SFP and harware information query interface. \n"
    ;


static fbe_status_t almance_verify_topology(void);

static fbe_status_t almance_verify_topology(void)
{
    fbe_class_id_t       class_id;
    fbe_u32_t            object_handle = 0;
    fbe_u32_t            port_idx;
    fbe_u32_t            enclosure_idx;
    fbe_u32_t            drive_idx;
    fbe_port_number_t    port_number;
    fbe_u32_t            total_objects = 0;
    fbe_status_t         status;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: starting configuration verification...", __FUNCTION__);
	mut_printf(MUT_LOG_LOW, "%s: starting configuration verification...", __FUNCTION__);

    /*we inserted a fleet board so we check for it*/
    /* board is always object id 0 ??? Maybe we need an interface to get the board object id*/
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_TEST_STATUS, "Verifying board type....Passed");

    /* We should have exactly one port. */
    for (port_idx = 0; port_idx < FBE_TEST_PHYSICAL_BUS_COUNT; port_idx++) {
        if (port_idx < 1) {
            /*get the handle of the port and validate port exists*/
            status = fbe_api_get_port_object_id_by_location(port_idx, &object_handle);
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

        } else {
            /*get the handle of the port and validate port exists or not*/
            status = fbe_api_get_port_object_id_by_location(port_idx, &object_handle);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(object_handle == FBE_OBJECT_ID_INVALID);
        }
    }
    mut_printf(MUT_LOG_TEST_STATUS, "Verifying port existance, state and number ...Passed");

    /* We should have exactly 3 enclosure on the port. */
    for (port_idx = 0; port_idx < FBE_TEST_PHYSICAL_BUS_COUNT; port_idx++) {
        for (enclosure_idx = 0; enclosure_idx < FBE_TEST_ENCLOSURES_PER_BUS; enclosure_idx++) {
            if (port_idx == 0 && enclosure_idx < 1) {
                /*get the handle of the port and validate enclosure exists*/
                /* Validate that the enclosure is in READY state */
                status = fbe_zrt_wait_enclosure_status(port_idx, enclosure_idx,
                                    FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
          } else {
            /*get the handle of the port and validate enclosure exists or not*/
            status = fbe_api_get_enclosure_object_id_by_location(port_idx, enclosure_idx, &object_handle);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(object_handle == FBE_OBJECT_ID_INVALID);
          }
        }
    }
    mut_printf(MUT_LOG_TEST_STATUS, "Verifying enclosure existance and state....Passed");

    /* Check for the physical drives on the enclosure. */
    for (port_idx = 0; port_idx < FBE_TEST_PHYSICAL_BUS_COUNT; port_idx++) {
        for (enclosure_idx = 0; enclosure_idx < FBE_TEST_ENCLOSURES_PER_BUS; enclosure_idx++) {
            for (drive_idx = 0; drive_idx < FBE_TEST_ENCLOSURE_SLOTS; drive_idx++) {
                if ((port_idx < 1) && (enclosure_idx < 1) && (drive_idx < 12))
                {
                    /*get the handle of the port and validate drive exists*/
                    status = fbe_test_pp_util_verify_pdo_state(port_idx, enclosure_idx, drive_idx, 
                                                                FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                } else {
                    /*get the handle of the drive and validate drive exists*/
                    status = fbe_api_get_physical_drive_object_id_by_location(port_idx, enclosure_idx, drive_idx, &object_handle);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                    MUT_ASSERT_TRUE(object_handle == FBE_OBJECT_ID_INVALID);
                }
            }
        }
    }
    mut_printf(MUT_LOG_TEST_STATUS, "Verifying physical drive existance and state....Passed");

    /* Let's see how many objects we have...
    1 board
    1 port
    1 enclosures
    12 physical drives
    12 logical drives
    27 objects
    */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == 27);
    mut_printf(MUT_LOG_TEST_STATUS, "Verifying object count...Passed");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: configuration verification SUCCEEDED!", __FUNCTION__);

    return FBE_STATUS_OK;

}

static fbe_status_t almance_run(void)
{    
    fbe_status_t status;
    fbe_lifecycle_state_t api_lifecycle_state;
    fbe_u32_t object_handle_p;
    fbe_u32_t object1_handle_p;
    fbe_u32_t object2_handle_p;
    fbe_api_terminator_device_handle_t port0_handle;
    fbe_api_terminator_device_handle_t port1_handle;
    fbe_api_terminator_device_handle_t port2_handle;

    fbe_cpd_shim_sfp_media_interface_info_t     sfp_info;
   
    fbe_port_sfp_info_t                         sfp_port_info;

    fbe_port_hardware_info_t                    port_hardware_info;

    status = fbe_api_terminator_get_port_handle(0, &port0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_get_port_object_id_by_location(0, &object_handle_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


    status = fbe_api_terminator_get_port_handle(0, &port1_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_get_port_object_id_by_location(0, &object1_handle_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_terminator_get_port_handle(0, &port2_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_get_port_object_id_by_location(0, &object2_handle_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_get_object_lifecycle_state(object_handle_p, &api_lifecycle_state,FBE_PACKAGE_ID_PHYSICAL);    
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "ERROR can't get port lifecycle state, status");

    status = fbe_api_wait_for_object_lifecycle_state (object_handle_p, 
        FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


    /* 1. Set Cable Good status.*/
    mut_printf(MUT_LOG_TEST_STATUS, "Setting SFP status to INSERTED...");
    fbe_zero_memory(&sfp_info,sizeof(fbe_cpd_shim_sfp_media_interface_info_t));
    sfp_info.condition_type = FBE_CPD_SHIM_SFP_CONDITION_GOOD;
    sfp_info.condition_additional_info = 1; // speed len avail
    status = fbe_api_terminator_set_sfp_media_interface_info(port0_handle,&sfp_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Provide sufficient time for information to be updated. */
    EmcutilSleep(4500);

    /* Query HW information using port usurper.*/    
    status = fbe_api_get_port_hardware_information(object_handle_p,&port_hardware_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(port_hardware_info.vendor_id == 0x11F8);
    MUT_ASSERT_TRUE(port_hardware_info.device_id == 0x8001);
    MUT_ASSERT_TRUE(port_hardware_info.pci_bus == 0x34);


    /* Query SFP information using port usurper.*/        
    status = fbe_api_get_port_sfp_information(object_handle_p,&sfp_port_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "SFP Cond:%d AddlInfo:%d\n",sfp_port_info.condition_type, sfp_port_info.condition_additional_info);
    MUT_ASSERT_TRUE(sfp_port_info.condition_type == FBE_PORT_SFP_CONDITION_GOOD);
    MUT_ASSERT_TRUE(sfp_port_info.condition_additional_info == FBE_PORT_SFP_SUBCONDITION_CHECKSUM_PENDING);
    mut_printf(MUT_LOG_TEST_STATUS, "Verifying SFP status INSERTED...success");

    /* 2. Pull cable.*/
    mut_printf(MUT_LOG_TEST_STATUS, "Setting SFP status REMOVED...");
    fbe_zero_memory(&sfp_info,sizeof(fbe_cpd_shim_sfp_media_interface_info_t));
    sfp_info.condition_type = FBE_CPD_SHIM_SFP_CONDITION_REMOVED;
    sfp_info.condition_additional_info = 0; // none
    status = fbe_api_terminator_set_sfp_media_interface_info(port1_handle,&sfp_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Provide sufficient time for information to be updated. */
    EmcutilSleep(4500);
    /* Query SFP information using port usurper.*/       
    status = fbe_api_get_port_sfp_information(object1_handle_p,&sfp_port_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(sfp_port_info.condition_type == FBE_CPD_SHIM_SFP_CONDITION_REMOVED);
    MUT_ASSERT_TRUE(sfp_port_info.condition_additional_info == FBE_PORT_SFP_SUBCONDITION_NONE);   
    mut_printf(MUT_LOG_TEST_STATUS, "Verifying SFP status REMOVED...success");

    /* 3. Set cable inserted.*/
    mut_printf(MUT_LOG_TEST_STATUS, "Setting SFP status INSERTED...");
    fbe_zero_memory(&sfp_info,sizeof(fbe_cpd_shim_sfp_media_interface_info_t));
    sfp_info.condition_type = FBE_CPD_SHIM_SFP_CONDITION_INSERTED;
    sfp_info.condition_additional_info = 1; // speed len avail
    status = fbe_api_terminator_set_sfp_media_interface_info(port2_handle,&sfp_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Provide sufficient time for information to be updated. */
    EmcutilSleep(4500);
    /* Query SFP information using port usurper.*/       
    status = fbe_api_get_port_sfp_information(object2_handle_p,&sfp_port_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "SFP Cond:%d AddlInfo:%d\n",sfp_port_info.condition_type, sfp_port_info.condition_additional_info);
    MUT_ASSERT_TRUE(sfp_port_info.condition_type == FBE_CPD_SHIM_SFP_CONDITION_INSERTED);
    MUT_ASSERT_TRUE(sfp_port_info.condition_additional_info == FBE_PORT_SFP_SUBCONDITION_CHECKSUM_PENDING);   
    mut_printf(MUT_LOG_TEST_STATUS, "Verifying SFP status INSERTED...success");

    /* 3. Set SFP fault.*/
    mut_printf(MUT_LOG_TEST_STATUS, "Setting SFP status to FAULTED...");
    fbe_zero_memory(&sfp_info,sizeof(fbe_cpd_shim_sfp_media_interface_info_t));
    sfp_info.condition_type = FBE_CPD_SHIM_SFP_CONDITION_FAULT;
    sfp_info.condition_additional_info = 8196; // unqual part
    status = fbe_api_terminator_set_sfp_media_interface_info(port0_handle,&sfp_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Provide sufficient time for information to be updated. */
    EmcutilSleep(4500);
    /* Query SFP information using port usurper.*/       
    status = fbe_api_get_port_sfp_information(object_handle_p,&sfp_port_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "SFP Cond:%d AddlInfo:%d\n",sfp_port_info.condition_type, sfp_port_info.condition_additional_info);
    MUT_ASSERT_TRUE(sfp_port_info.condition_type == FBE_CPD_SHIM_SFP_CONDITION_FAULT);
    mut_printf(MUT_LOG_TEST_STATUS, "SFP condition_additional_info 0x%x\n", sfp_port_info.condition_additional_info);
    MUT_ASSERT_TRUE(sfp_port_info.condition_additional_info == FBE_PORT_SFP_SUBCONDITION_UNSUPPORTED);
    mut_printf(MUT_LOG_TEST_STATUS, "Verifying SFP status FAULTED...success");

    /* Sleep()+*/
    mut_printf(MUT_LOG_TEST_STATUS,
        "%s: ===== Verifying topology. ========.\n",
        __FUNCTION__);

    status = yancey_verify_topology();
    if (status !=  FBE_STATUS_OK) {
        mut_printf(MUT_LOG_TEST_STATUS,
            "%s: ERROR verification failed, status: 0x%X.",
            __FUNCTION__, status);

        return status;
    }
    return FBE_STATUS_OK;
}


void almance(void)
{

    fbe_status_t    status  = FBE_STATUS_GENERIC_FAILURE;
    status = almance_run();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

}

