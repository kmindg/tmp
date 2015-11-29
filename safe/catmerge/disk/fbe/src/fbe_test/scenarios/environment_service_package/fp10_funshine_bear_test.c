/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fp10_funshine_bear.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the port link component info change notification test. 
 *
 * @version
 *
 *
 ***************************************************************************/
#include "esp_tests.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe/fbe_pe_types.h"
#include "fbe/fbe_api_esp_module_mgmt_interface.h"
#include "fbe/fbe_module_info.h"
#include "esp_tests.h"
#include "fbe_test_configurations.h"
#include "fbe_test_esp_utils.h"
#include "fbe/fbe_file.h"
#include "fbe/fbe_port.h"
#include "fbe_notification.h"
#include "fp_test.h"
#include "sep_tests.h"

char * fp10_funshine_bear_short_desc = "Test port link data change notification.";
char * fp10_funshine_bear_long_desc =
    "\n"
    "\n"
    "The funshine bear scenario tests the esp module managment object port link component data change notification.\n"
    "It includes: \n"
    "    - The state change notification for the link status change;\n"
    "    - The state change notification for the link speed change;\n"    
    "\n"
    "Starting Config:\n"
    "    [PP] armada board with processor enclosure data\n"
    "    [PP] SAS PMC port\n"
    "    [PP] viper enclosure\n"
    "    [PP] 15 SAS drives (PDO)\n"
    "    [PP] 15 logical drives (LDO)\n"
    "\n"
    "STEP 1: Bring up the initial topology.\n"
    "    - Create the initial physical package config.\n"
    "    - Verify that all configured objects are in the READY state.\n"
    "\n"  
    "STEP 2: Verify the IO Port link data change notification received.\n"
    "    - Get the IO Port link info.\n"
    "\n"
    "STEP 3: Verify the IO Port link data change notification received.\n"
    "    - Set the IO Port link info to degraded.\n"
    "    - Wait for data change notification for link information.\n"
    "    - Query and verify the link information.\n"
    "\n"
    "STEP 7: Shutdown the Terminator/Physical package.\n"
    ;


static fbe_semaphore_t          sem;
static fbe_notification_registration_id_t          reg_id;

static void fp10_funshine_bear_commmand_response_callback_function (update_object_msg_t * update_object_msg, void *context);
static fbe_status_t fp10_load_funshine_bear_config(void);

void fp10_funshine_bear_test(void)
{
    fbe_status_t                        fbeStatus = FBE_STATUS_GENERIC_FAILURE;    
    fbe_api_terminator_device_handle_t  port_handle;
    fbe_cpd_shim_port_lane_info_t       port_link_info;
    DWORD                               dwWaitResult;
    fbe_esp_module_io_port_info_t       port_info;
                                                            
    fbe_semaphore_init(&sem, 0, 1);    
    fbeStatus = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_ESP,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
                                                                  fp10_funshine_bear_commmand_response_callback_function,
                                                                  &sem,
                                                                  &reg_id);

    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    fbeStatus = fbe_api_terminator_get_port_handle(0, &port_handle);
    //status = fbe_api_get_port_object_id_by_location(0, &objectId);
    mut_printf(MUT_LOG_LOW, "Port handle 0x%llx, Status %d\n",
	       (unsigned long long)port_handle, fbeStatus);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, fbeStatus);

    /* Test Link Degraded Notification */
    fbe_zero_memory(&port_link_info,sizeof(port_link_info));
    port_link_info.portal_number = 0;
    port_link_info.link_speed = SPEED_SIX_GIGABIT;
    port_link_info.link_state = CPD_SHIM_PORT_LINK_STATE_DEGRADED;
    port_link_info.nr_phys = 2;
    port_link_info.phy_map = 0x03;
    port_link_info.nr_phys_enabled = 1;
    port_link_info.nr_phys_up = 1;
    mut_printf(MUT_LOG_LOW, "Setting port 0 to DEGRADED\n");
    fbeStatus = fbe_api_terminator_set_port_link_info(port_handle, &port_link_info);    
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    /* Wait 1 minute for misc data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    mut_printf(MUT_LOG_LOW, "fbe_semaphore_wait_ms 0x%llx, Status %d\n",
	       (unsigned long long)port_handle, dwWaitResult);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Check link information. */
    fbe_zero_memory(&port_info,sizeof(fbe_esp_module_io_port_info_t));
    port_info.header.sp = 0;
    port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
    port_info.header.slot = 0;
    port_info.header.port = 0;
    mut_printf(MUT_LOG_LOW, "Got notification from ESP, checking the link state from port info\n");

    fbeStatus = fbe_api_esp_module_mgmt_getIOModulePortInfo(&port_info);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "Port link notification link state 0x%x\n",port_info.io_port_info.link_info.link_state);
    MUT_ASSERT_TRUE(port_info.io_port_info.link_info.link_state == FBE_PORT_LINK_STATE_DEGRADED);
    


    /* Test Link Up Notification */
    port_link_info.link_state = CPD_SHIM_PORT_LINK_STATE_UP;
    port_link_info.portal_number = 0;
    port_link_info.link_speed = SPEED_SIX_GIGABIT;
    port_link_info.nr_phys = 2;
    port_link_info.phy_map = 0x03;
    port_link_info.nr_phys_enabled = 2;
    port_link_info.nr_phys_up = 2;
    mut_printf(MUT_LOG_LOW, "Setting port 0 to LINK UP\n");
    fbeStatus = fbe_api_terminator_set_port_link_info(port_handle, &port_link_info);
    mut_printf(MUT_LOG_LOW,
	       "fbe_api_terminator_set_port_link_info 0x%llx, Status %d\n",
	       (unsigned long long)port_handle, fbeStatus);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    /* Wait 1 minute for misc data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    mut_printf(MUT_LOG_LOW, "fbe_semaphore_wait_ms 0x%llx, Status %d\n",
	       (unsigned long long)port_handle, dwWaitResult);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    
    /* Check link information. */
    fbe_zero_memory(&port_info,sizeof(fbe_esp_module_io_port_info_t));
    port_info.header.sp = 0;
    port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
    port_info.header.slot = 0;
    port_info.header.port = 0;

    fbeStatus = fbe_api_esp_module_mgmt_getIOModulePortInfo(&port_info);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "Port link notification link state 0x%x\n",port_info.io_port_info.link_info.link_state);
    MUT_ASSERT_TRUE(port_info.io_port_info.link_info.link_state == FBE_PORT_LINK_STATE_UP);

    /* Unregistered the notification*/
    fbeStatus = fbe_api_notification_interface_unregister_notification(fp10_funshine_bear_commmand_response_callback_function,
                                                                    reg_id);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    fbe_semaphore_destroy(&sem);

    // delay 10 seconds because ESP sucks
    fbe_api_sleep(10000);

    return;
}

static void fp10_funshine_bear_commmand_response_callback_function (update_object_msg_t * update_object_msg, void *context)
{
    fbe_semaphore_t     *sem = (fbe_semaphore_t *)context;
    fbe_notification_data_changed_info_t * pDataChangeInfo = NULL;

    pDataChangeInfo = &update_object_msg->notification_info.notification_data.data_change_info;
   if((FBE_DEVICE_TYPE_IOMODULE == pDataChangeInfo->device_mask) &&
       (FBE_DEVICE_DATA_TYPE_PORT_INFO == pDataChangeInfo->data_type)){
           mut_printf(MUT_LOG_LOW, "Port link notification \n");           
           fbe_semaphore_release(sem, 0, 1 ,FALSE);
   }

    return;

}


/*!**************************************************************
 * fp10_funshine_bear_setup()
 ****************************************************************
 * @brief
 *  Setup for filling the interface structure
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   04/20/2010 - Created. Brion Philbin
 *
 ****************************************************************/
void fp10_funshine_bear_setup(void)
{

    fbe_status_t status;

    status = fbe_test_load_fp_config(SPID_PROMETHEUS_M1_HW_TYPE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== loaded required config ===\n");

    fp_test_start_physical_package();

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit_no_esp();

    status = fbe_test_sep_util_wait_for_database_service(20000);

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_wait_for_all_esp_objects_ready();

}
/**************************************
 * end fp10_funshine_bear_setup()
 **************************************/

fbe_status_t fp10_load_funshine_bear_config(void)
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_board_info_t 	board_info;
    fbe_terminator_sas_port_info_t	sas_port;
    fbe_terminator_sas_encl_info_t	sas_encl;
    fbe_terminator_sas_drive_info_t sas_drive;
    fbe_cpd_shim_sfp_media_interface_info_t configured_sfp_info;
    fbe_cpd_shim_port_lane_info_t   port_link_info;

    fbe_api_terminator_device_handle_t	port0_handle;
    fbe_api_terminator_device_handle_t	encl0_0_handle;
    fbe_api_terminator_device_handle_t	drive_handle;

    fbe_cpd_shim_hardware_info_t                hardware_info;

    fbe_u32_t                       slot = 0;

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_PROMETHEUS_M1_HW_TYPE;
    status  = fbe_api_terminator_insert_board (&board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a port*/
    sas_port.backend_number = 0;
    sas_port.io_port_number = 0;
    sas_port.portal_number = 0;
    sas_port.sas_address = 0x5000097A7800903F;
    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    status  = fbe_api_terminator_insert_sas_port (&sas_port, &port0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* configure port hardware info */
    fbe_zero_memory(&hardware_info,sizeof(fbe_cpd_shim_hardware_info_t));
    hardware_info.vendor = 0x11F8;
    hardware_info.device = 0x8001;
    hardware_info.bus = 1;
    hardware_info.slot = 0;
    hardware_info.function = 0;

    status = fbe_api_terminator_set_hardware_info(port0_handle,&hardware_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Set the link information to a good state*/

    fbe_zero_memory(&port_link_info,sizeof(port_link_info));
    port_link_info.portal_number = 0;
    port_link_info.link_speed = SPEED_SIX_GIGABIT;
    port_link_info.link_state = CPD_SHIM_PORT_LINK_STATE_UP;
    port_link_info.nr_phys = 2;
    port_link_info.phy_map = 0x03;
    port_link_info.nr_phys_enabled = 2;
    port_link_info.nr_phys_up = 2;
    status = fbe_api_terminator_set_port_link_info(port0_handle, &port_link_info);    
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*insert an enclosure to port 0*/
    sas_encl.backend_number = 0;
    sas_encl.encl_number = 0;
    sas_encl.uid[0] = 1; // some unique ID.
    sas_encl.sas_address = 0x5000097A78009071;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    status  = fbe_api_terminator_insert_sas_enclosure (port0_handle, &sas_encl, &encl0_0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    

    for (slot = 0; slot < SIMPLE_CONFIG_DRIVE_MAX; slot ++) {
		/*insert a sas_drive to port 0 encl 0 slot */
		sas_drive.backend_number = 0;
		sas_drive.encl_number = 0;
		sas_drive.capacity = 0x10B5C731;
		sas_drive.block_size = 520;
		sas_drive.sas_address = 0x5000097A78000000 + ((fbe_u64_t)(sas_drive.encl_number) << 16) + slot;
		sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
		status  = fbe_api_terminator_insert_sas_drive (encl0_0_handle, slot, &sas_drive, &drive_handle);
		MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }


    /*
     * STEP 2 - Insert SFP into IOM 0 Port 0 verify ESP
     * reports this as inserted.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Setting SFP status to INSERTED...");
    fbe_zero_memory(&configured_sfp_info,sizeof(fbe_cpd_shim_sfp_media_interface_info_t));
    configured_sfp_info.condition_type = FBE_CPD_SHIM_SFP_CONDITION_GOOD;
    configured_sfp_info.condition_additional_info = 0;
    status = fbe_api_terminator_set_sfp_media_interface_info(port0_handle,&configured_sfp_info);

    return status;
}

/*!**************************************************************
 * fp10_funshine_bear_destroy()
 ****************************************************************
 * @brief
 *  Cleanup the fp10_funshine_bear test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   04/20/2010 - Created. Brion Philbin
 *
 ****************************************************************/

void fp10_funshine_bear_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_common_destroy_all();
    return;
}
