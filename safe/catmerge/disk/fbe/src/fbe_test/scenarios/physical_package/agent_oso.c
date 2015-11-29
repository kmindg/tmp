/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file agent_oso.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the port link component info change notification test. 
 *
 * @version
 *
 *
 ***************************************************************************/
#include "physical_package_tests.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe/fbe_port.h"
#include "fbe_notification.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_port_interface.h"
#include "speeds.h"

char * agent_oso_short_desc = "Test port link data change notification.";
char * agent_oso_long_desc =
    "\n"
    "\n"
    "The special agent OSO scenario tests the physical package port object link component data change notification.\n"
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

static void agent_oso_commmand_response_callback_function (update_object_msg_t * update_object_msg, void *context);

void agent_oso(void)
{
    fbe_status_t        fbeStatus = FBE_STATUS_GENERIC_FAILURE;    
    fbe_api_terminator_device_handle_t port_handle;
    fbe_cpd_shim_port_lane_info_t port_link_info;
    DWORD                   dwWaitResult;
    fbe_object_id_t         port_object_id;
    fbe_port_link_information_t port_object_link_info;

                                                            
    fbe_semaphore_init(&sem, 0, 1);    
    fbeStatus = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_PORT,
                                                                  agent_oso_commmand_response_callback_function,
                                                                  &sem,
                                                                  &reg_id);

    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    fbeStatus = fbe_api_terminator_get_port_handle(0, &port_handle);
    //status = fbe_api_get_port_object_id_by_location(0, &objectId);
    mut_printf(MUT_LOG_LOW, "Port handle 0x%llx, Status %d\n",
	       (unsigned long long)port_handle,fbeStatus);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, fbeStatus);

    /* Setting up the initial values for test.*/
    /* On hardware, these values are retrieved from miniport during initialization of the port. */
    fbe_zero_memory(&port_link_info,sizeof(port_link_info));
    port_link_info.portal_number = 5;
    port_link_info.link_speed = SPEED_SIX_GIGABIT;
    port_link_info.link_state = CPD_SHIM_PORT_LINK_STATE_UP;
    port_link_info.nr_phys = 2;
    port_link_info.phy_map = 0x03;
    port_link_info.nr_phys_enabled = 2;
    port_link_info.nr_phys_up = 2;
    fbeStatus = fbe_api_terminator_set_port_link_info(port_handle, &port_link_info);    
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    /* Wait 1 minute for misc data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    mut_printf(MUT_LOG_LOW, "fbe_semaphore_wait_ms 0x%llx, Status %d\n",
	       (unsigned long long)port_handle,dwWaitResult);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Check link information in the terminator. */
    fbeStatus = fbe_api_terminator_get_port_link_info(port_handle, &port_link_info);
    mut_printf(MUT_LOG_LOW, "fbe_api_terminator_get_port_link_info 0x%llx, Status %d\n",
	       (unsigned long long)port_handle,fbeStatus);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "Terminator Port link notification link speed 0x%x, link state 0x%x\n",port_link_info.link_speed,port_link_info.link_state);
    MUT_ASSERT_TRUE(port_link_info.link_state == CPD_SHIM_PORT_LINK_STATE_UP);

    /* Check link information in the Physical Package*/

    fbe_api_get_port_object_id_by_location(0, &port_object_id);
    fbeStatus = fbe_api_port_get_link_information(port_object_id, &port_object_link_info);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "Port object, link notification link speed 0x%x, link state 0x%x\n",port_object_link_info.link_speed,port_object_link_info.link_state);
    MUT_ASSERT_TRUE(port_object_link_info.link_state == FBE_PORT_LINK_STATE_UP);


    port_link_info.link_state = CPD_SHIM_PORT_LINK_STATE_DEGRADED;
    port_link_info.portal_number = 5;
    port_link_info.link_speed = SPEED_SIX_GIGABIT;
    port_link_info.nr_phys = 2;
    port_link_info.phy_map = 0x03;
    port_link_info.nr_phys_enabled = 1;
    port_link_info.nr_phys_up = 1;
    fbeStatus = fbe_api_terminator_set_port_link_info(port_handle, &port_link_info);
    mut_printf(MUT_LOG_LOW, "fbe_api_terminator_set_port_link_info 0x%llx, Status %d\n",
	       (unsigned long long)port_handle,fbeStatus);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    /* Wait 1 minute for misc data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    mut_printf(MUT_LOG_LOW, "fbe_semaphore_wait_ms 0x%llx, Status %d\n",
	       (unsigned long long)port_handle,dwWaitResult);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Check whether the link has been marked as DEGRADED. */
    fbe_zero_memory(&port_link_info,sizeof(port_link_info));
    fbeStatus = fbe_api_terminator_get_port_link_info(port_handle, &port_link_info);
    mut_printf(MUT_LOG_LOW, "Port link notification link speed 0x%x, link state 0x%x\n",port_link_info.link_speed,port_link_info.link_state);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(port_link_info.link_state == CPD_SHIM_PORT_LINK_STATE_DEGRADED);

     /* Check link information in the Physical Package*/

    fbe_api_get_port_object_id_by_location(0, &port_object_id);
    fbeStatus = fbe_api_port_get_link_information(port_object_id, &port_object_link_info);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "Port object, link notification link speed 0x%x, link state 0x%x\n",port_object_link_info.link_speed,port_object_link_info.link_state);
    MUT_ASSERT_TRUE(port_object_link_info.link_state == FBE_PORT_LINK_STATE_DEGRADED);

    fbeStatus = fbe_api_notification_interface_unregister_notification(agent_oso_commmand_response_callback_function, reg_id);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    fbe_semaphore_destroy(&sem);

    return;
}

static void agent_oso_commmand_response_callback_function (update_object_msg_t * update_object_msg, void *context)
{
    fbe_semaphore_t     *sem = (fbe_semaphore_t *)context;
    fbe_notification_data_changed_info_t * pDataChangeInfo = NULL;

    pDataChangeInfo = &update_object_msg->notification_info.notification_data.data_change_info;
   if((FBE_DEVICE_TYPE_PORT_LINK == pDataChangeInfo->device_mask) &&
       (FBE_DEVICE_DATA_TYPE_PORT_INFO == pDataChangeInfo->data_type)){
           mut_printf(MUT_LOG_LOW, "Port link notification \n");           
           fbe_semaphore_release(sem, 0, 1 ,FALSE);
   }

    return;

}

