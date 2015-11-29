/**************************************************************************
 * Copyright (C) EMC Corporation 2008 - 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
***************************************************************************/

#include "terminator_test.h"
#include "terminator_port.h"
#include "terminator_enclosure.h"
#include "terminator_drive.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe_terminator_device_registry.h"
#include "fbe_terminator_miniport_interface.h"
#include "fbe_terminator.h"
#include "fbe_terminator_file_api.h"
#include "fbe/fbe_winddk.h"

/**********************************/
/*        local variables         */
/**********************************/
static fbe_u32_t fbe_cpd_shim_callback_count;
static fbe_u32_t fbe_cpd_shim_logout_count;
static fbe_u32_t fbe_cpd_shim_login_count;


/**********************************/
/*        local functions         */
/**********************************/
static fbe_status_t reset_mock(fbe_terminator_device_ptr_t device_handle);
static fbe_status_t fbe_cpd_shim_callback_function_mock(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context)
{
    fbe_cpd_shim_callback_count++;
        switch(callback_info->callback_type)
        {
        case FBE_CPD_SHIM_CALLBACK_TYPE_LOGOUT:
            mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGOUT: device_type: %d", callback_info->info.logout.device_type);
        mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGOUT: device_id: 0x%llX", (unsigned long long)callback_info->info.logout.device_id);
        mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGOUT: device_addr: 0x%llX", (unsigned long long)callback_info->info.logout.device_address);
        mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGOUT: parent_dev_id: 0x%llX", (unsigned long long)callback_info->info.logout.parent_device_id);
                fbe_cpd_shim_logout_count++;
                break;
        case FBE_CPD_SHIM_CALLBACK_TYPE_LOGIN:
        mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGIN: device_type: %d", callback_info->info.login.device_type);
        mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGIN: device_id: 0x%llX", (unsigned long long)callback_info->info.login.device_id);
        mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGIN: device_addr: 0x%llX", (unsigned long long)callback_info->info.login.device_address);
        mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGIN: parent_dev_id: 0x%llX", (unsigned long long)callback_info->info.login.parent_device_id);
                fbe_cpd_shim_login_count++;
                break;
        default:
                break;
        }
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_cpd_shim_device_state_test_callback(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context)
{
    return FBE_STATUS_OK;
}

/* this test makes sure device_id get reset for each termiantor_init */
void terminator_api_init_destroy_test(void)
{
    fbe_status_t                                        status;
    fbe_terminator_board_info_t         board_info, temp_board_info;
    fbe_terminator_sas_port_info_t      sas_port;
    fbe_terminator_sas_encl_info_t      sas_encl;
    fbe_terminator_sas_drive_info_t sas_drive;
    
    fbe_terminator_api_device_handle_t  port1_handle, port2_handle;
    fbe_terminator_api_device_handle_t  encl1_0_handle, encl1_1_handle, encl1_2_handle;
    fbe_terminator_api_device_handle_t  encl1_1_handle_new, encl1_2_handle_new, encl2_0_handle_new;
    fbe_terminator_api_device_handle_t  drive_handle;
    
    
    fbe_terminator_api_device_handle_t          device_list[MAX_DEVICES];
    terminator_cpd_port_t           cpd_port;
    fbe_u32_t                       port_index;
    fbe_cpd_shim_enumerate_backend_ports_t cpd_shim_enumerate_backend_ports;
    fbe_u32_t   loop_count;
    fbe_u32_t   device_count;
    
    cpd_port.state = PMC_PORT_STATE_INITIALIZED;
    cpd_port.payload_completion_function = NULL;
    cpd_port.payload_completion_context = NULL;
    cpd_port.callback_function = fbe_cpd_shim_callback_function_mock;
    cpd_port.callback_context = NULL;

    mut_printf(MUT_LOG_HIGH, "%s %s ",__FILE__, __FUNCTION__);

    for (loop_count = 0; loop_count < 3; loop_count ++)
    {
        mut_printf(MUT_LOG_HIGH, "%s loop %d", __FUNCTION__, loop_count);

        /* reset the callback counter */
        fbe_cpd_shim_callback_count = 0;

        /*initialize the terminator*/
        status = fbe_terminator_api_init();
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        mut_printf(MUT_LOG_LOW, "%s: terminator initialized.", __FUNCTION__);

        /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
        board_info.platform_type = SPID_DREADNOUGHT_HW_TYPE;
        status  = fbe_terminator_api_insert_board (&board_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /*verify board type*/
        status = fbe_terminator_api_get_board_info(&temp_board_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_EQUAL(board_info.platform_type, temp_board_info.platform_type);

        /*insert a port*/
        sas_port.sas_address = (fbe_u64_t)0x123456789;
        sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
        sas_port.io_port_number = 1;
        sas_port.portal_number = 2;
        sas_port.backend_number = 1;
        status  = fbe_terminator_api_insert_sas_port (&sas_port, &port1_handle);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_terminator_miniport_api_port_init(1, 2, &port_index);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Register a callback to port 0, so we can destroy the object properly */
        status = fbe_terminator_miniport_api_register_callback(port_index, (fbe_terminator_miniport_api_callback_function_t)fbe_cpd_shim_callback_function_mock, NULL);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        
        /*insert a port*/
        sas_port.sas_address = (fbe_u64_t)0x22222222;
        sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
        sas_port.io_port_number = 2;
        sas_port.portal_number = 3;
        sas_port.backend_number = 2;
        status  = fbe_terminator_api_insert_sas_port (&sas_port, &port2_handle);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_terminator_miniport_api_port_init(2, 3, &port_index);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Register a callback to port 1, so we can destroy the object properly */
        status = fbe_terminator_miniport_api_register_callback(port_index, (fbe_terminator_miniport_api_callback_function_t)fbe_cpd_shim_callback_function_mock, NULL);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* enumerate the ports */
        status  = fbe_terminator_miniport_api_enumerate_cpd_ports (&cpd_shim_enumerate_backend_ports);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /*insert an enclosure to port 0*/
        sas_encl.backend_number = 1;
        sas_encl.encl_number = 0;
        sas_encl.uid[0] = 1; // some unique ID.
        sas_encl.sas_address = (fbe_u64_t)0x123456780;
        sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_BULLET;
        status  = fbe_terminator_api_insert_sas_enclosure (port1_handle, &sas_encl, &encl1_0_handle);
        sas_encl.connector_id = 0;
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
        /*insert an enclosure to port 0*/
        sas_encl.backend_number = 1;
        sas_encl.encl_number = 1;
        sas_encl.uid[0] = 2; // some unique ID.
        sas_encl.sas_address = (fbe_u64_t)0x123456782;
        sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_BULLET;
        sas_encl.connector_id = 0;
        status  = fbe_terminator_api_insert_sas_enclosure (port1_handle, &sas_encl, &encl1_1_handle);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
        /*insert an enclosure to port 0*/
        sas_encl.backend_number = 1;
        sas_encl.encl_number = 2;
        sas_encl.uid[0] = 3; // some unique ID.
        sas_encl.sas_address = (fbe_u64_t)0x123456783;
        sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
        sas_encl.connector_id = 0;
        status  = fbe_terminator_api_insert_sas_enclosure (port1_handle, &sas_encl, &encl1_2_handle);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
        /* enumerate the devices on ports */
        status = fbe_terminator_api_enumerate_devices(device_list, MAX_DEVICES);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        /* check the number of devices in terminator */
        MUT_ASSERT_INT_EQUAL(6, fbe_terminator_api_get_devices_count());
        /* check the number of Board in terminator */
        status = fbe_terminator_api_get_devices_count_by_type_name("Board",&device_count);
        MUT_ASSERT_INT_EQUAL(1,device_count);
        /* check the number of Port in terminator */
        status = fbe_terminator_api_get_devices_count_by_type_name("Port",&device_count);
        MUT_ASSERT_INT_EQUAL(2,device_count);
        /* check the number of Enclosure in terminator */
        status = fbe_terminator_api_get_devices_count_by_type_name("Enclosure",&device_count);
        MUT_ASSERT_INT_EQUAL(3,device_count);
        EmcutilSleep(1000);
        /* check the callback count, 2 port linkup and 3 enclosure and 3 virtual phy login */
        MUT_ASSERT_INT_EQUAL (8, fbe_cpd_shim_callback_count);
    
        fbe_cpd_shim_callback_count = 0;
        /* remove sas enclosure 0-1 */
        status = fbe_terminator_api_remove_device(encl1_1_handle);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        /* enumerate the devices on ports */
        status = fbe_terminator_api_enumerate_devices(device_list, MAX_DEVICES);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        /* check the number of devices in terminator, 1 board, 2 ports and 1 enclosure */
        MUT_ASSERT_INT_EQUAL(4, fbe_terminator_api_get_devices_count());
        EmcutilSleep(1000);
        /* check the callback count, 2 enclosure and 2 virtual phy logoit */
        MUT_ASSERT_INT_EQUAL (4, fbe_cpd_shim_callback_count);

        /*verify enclosure 0-1 and 0-2 are not in the device registry anymore*/
        status = fbe_terminator_api_get_enclosure_handle(1, 1, &encl1_1_handle_new);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_GENERIC_FAILURE, status);
        MUT_ASSERT_INTEGER_NOT_EQUAL(encl1_1_handle, encl1_1_handle_new);
    
        status = fbe_terminator_api_get_enclosure_handle(1, 2, &encl1_2_handle_new);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_GENERIC_FAILURE, status);
        MUT_ASSERT_INTEGER_NOT_EQUAL(encl1_2_handle, encl1_2_handle_new);
    
        /*insert an enclosure*/
        sas_encl.backend_number = 1;
        sas_encl.encl_number = 1;
        sas_encl.uid[0] = 1; // some unique ID.
        sas_encl.sas_address = (fbe_u64_t)0x123456781;
        sas_encl.connector_id = 0;
        status  = fbe_terminator_api_insert_sas_enclosure (port1_handle, &sas_encl, &encl1_1_handle_new);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INTEGER_NOT_EQUAL(encl1_1_handle, encl1_1_handle_new);

        /*insert an enclosure*/
        sas_encl.backend_number = 1;
        sas_encl.encl_number = 2;
        sas_encl.uid[0] = 2; // some unique ID.
        sas_encl.sas_address = (fbe_u64_t)0x123456782;
        sas_encl.connector_id = 0;
        status  = fbe_terminator_api_insert_sas_enclosure (port1_handle, &sas_encl, &encl1_2_handle_new);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INTEGER_NOT_EQUAL(encl1_2_handle, encl1_2_handle_new);

        /*insert an enclosure*/
        sas_encl.backend_number = 2;
        sas_encl.encl_number = 0;
        sas_encl.uid[0] = 3; // some unique ID.
        sas_encl.sas_address = (fbe_u64_t)0x123456783;
        sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
        sas_encl.connector_id = 0;
        status  = fbe_terminator_api_insert_sas_enclosure (port2_handle, &sas_encl, &encl2_0_handle_new);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
        /* check the number of devices in terminator, 1 board, 2 ports and 4 enclosure */
        MUT_ASSERT_INT_EQUAL(7, fbe_terminator_api_get_devices_count());

        /*insert a sas_drive to port 0 encl 2 */
        sas_drive.backend_number = 1;
        sas_drive.encl_number = 2;
        sas_drive.sas_address = (fbe_u64_t)0x987654321;
        sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
        sas_drive.capacity = 0x10BD0;
        sas_drive.block_size = 520;
        status  = fbe_terminator_api_insert_sas_drive (encl1_2_handle_new, 3, &sas_drive, &drive_handle);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* check the number of devices in terminator, 1 board, 2 ports and 4 enclosure 1 drive*/
        MUT_ASSERT_INT_EQUAL(8, fbe_terminator_api_get_devices_count());
    
        /*insert a sas_drive to the last enclosure*/
        sas_drive.backend_number = 2;
        sas_drive.encl_number = 0;
        sas_drive.sas_address = (fbe_u64_t)0x987654322;
        sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
        sas_drive.capacity = 0x10BD0;
        sas_drive.block_size = 520;
        status  = fbe_terminator_api_insert_sas_drive (encl2_0_handle_new, 0, &sas_drive, &drive_handle);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        /* check the number of devices in terminator, 1 board, 2 ports and 4 enclosure 2 drive*/
        MUT_ASSERT_INT_EQUAL(9, fbe_terminator_api_get_devices_count());
    
        status = fbe_terminator_api_destroy();
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        mut_printf(MUT_LOG_LOW, "%s: terminator destroyed.", __FUNCTION__);
    
        /* wait a reasonable time */
        EmcutilSleep(2000);
        /*check the callback counter */
        MUT_ASSERT_TRUE(fbe_cpd_shim_callback_count != 0);
    }
}

/* this setup creates a board, port 0, enclosure 0 and enclosure 1, drive 0-1-3*/
void terminator_api_test_setup(void)
{
    fbe_status_t                        status;
    fbe_terminator_board_info_t             board_info, temp_board_info;
    fbe_terminator_sas_port_info_t      sas_port;
    fbe_terminator_sas_encl_info_t      sas_encl;
    fbe_terminator_sas_drive_info_t     sas_drive;
    
    fbe_terminator_api_device_handle_t  port1_handle;
    fbe_terminator_api_device_handle_t  encl1_0_handle, encl1_1_handle;
    fbe_terminator_api_device_handle_t  drive_handle;
    fbe_u32_t                       port_index;

    mut_printf(MUT_LOG_HIGH, "%s %s ",__FILE__, __FUNCTION__);

    /*initialize the terminator*/
    status = fbe_terminator_api_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "***** %s: terminator initialized. *****", __FUNCTION__);

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_DREADNOUGHT_HW_TYPE;
    status  = fbe_terminator_api_insert_board (&board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*verify board type*/
    status = fbe_terminator_api_get_board_info(&temp_board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(board_info.board_type, temp_board_info.board_type);

    /*insert a port*/
    sas_port.sas_address = (fbe_u64_t)0x123456789;
    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    sas_port.io_port_number = 1;
    sas_port.portal_number = 2;
    sas_port.backend_number = 1;
    status  = fbe_terminator_api_insert_sas_port (&sas_port, &port1_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_terminator_miniport_api_port_init(1, 2, &port_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Register a callback to port 0, so we can destroy the object properly */
    status = fbe_terminator_miniport_api_register_callback(port_index, (fbe_terminator_miniport_api_callback_function_t)fbe_cpd_shim_callback_function_mock, NULL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert an enclosure to port 1*/
    sas_encl.backend_number = 1;
    sas_encl.encl_number = 0;
    sas_encl.uid[0] = 1; // some unique ID.
    sas_encl.sas_address = (fbe_u64_t)0x123456780;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_BULLET;
    sas_encl.connector_id = 0;
    status  = fbe_terminator_api_insert_sas_enclosure (port1_handle, &sas_encl, &encl1_0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert an enclosure to port 1*/
    sas_encl.backend_number = 1;
    sas_encl.encl_number = 1;
    sas_encl.uid[0] = 2; // some unique ID.
    sas_encl.sas_address = (fbe_u64_t)0x123456782;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_BULLET;
    sas_encl.connector_id = 0;
    status  = fbe_terminator_api_insert_sas_enclosure (port1_handle, &sas_encl, &encl1_1_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a sas_drive to port 1 encl 1 */
    sas_drive.backend_number = 1;
    sas_drive.encl_number = 1;
    sas_drive.sas_address = (fbe_u64_t)0x987654321;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.capacity = 0x10BD0;
    sas_drive.block_size = 520;
    status  = fbe_terminator_api_insert_sas_drive (encl1_1_handle, 3, &sas_drive, &drive_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* check the number of devices in terminator, 1 board, 2 ports and 4 enclosure 1 drive*/
    MUT_ASSERT_INT_EQUAL(5, fbe_terminator_api_get_devices_count());

    // give some time for the logins to complete
    EmcutilSleep(2000);

}

void terminator_api_test_teardown(void)
{
    fbe_status_t status = fbe_terminator_api_destroy();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "%s: terminator destroyed.", __FUNCTION__);
}

/*
 * D60 section
 */

/* some device sas address */
#define D60_PORT_ADDR (fbe_u64_t)0x11111111
#define D60_ICM_ADDR_0 (fbe_u64_t)0x22222222
#define D60_EE_ADDR_0 (fbe_u64_t)0x33333333
#define D60_EE_ADDR_1 (fbe_u64_t)0x44444444
#define D60_EE_ADDR_2 (fbe_u64_t)0x55555555
#define D60_VIRTUAL_PHY_ADDR_0 ((D60_ICM_ADDR_0 & VIPER_VIRTUAL_PHY_SAS_ADDR_MASK) + VIPER_VIRTUAL_PHY_SAS_ADDR_LAST_6_BITS)
#define D60_VIRTUAL_PHY_ADDR_1 ((D60_EE_ADDR_0 & VIPER_VIRTUAL_PHY_SAS_ADDR_MASK) + VIPER_VIRTUAL_PHY_SAS_ADDR_LAST_6_BITS)
#define D60_VIRTUAL_PHY_ADDR_2 ((D60_EE_ADDR_1 & VIPER_VIRTUAL_PHY_SAS_ADDR_MASK) + VIPER_VIRTUAL_PHY_SAS_ADDR_LAST_6_BITS)
#define D60_VIRTUAL_PHY_ADDR_3 ((D60_EE_ADDR_2 & VIPER_VIRTUAL_PHY_SAS_ADDR_MASK) + VIPER_VIRTUAL_PHY_SAS_ADDR_LAST_6_BITS)

typedef struct device_address_map_s
{
        fbe_u64_t device_address;
        fbe_u64_t parent_address;
        fbe_pmc_shim_discovery_device_type_t device_type;
} device_address_map_t;

/* a map from child device address to parent device address and device type.
 *                                                              ----------------------------------------------------------------------------------------
 *                                                                  device_address                  parent_address      device_type
 *                                                              ----------------------------------------------------------------------------------------*/
device_address_map_t device_map[] = {{                 D60_ICM_ADDR_0,         D60_PORT_ADDR,   FBE_PMC_SHIM_DEVICE_TYPE_ENCLOSURE},
                                                                                                          {D60_EE_ADDR_0,          D60_ICM_ADDR_0,  FBE_PMC_SHIM_DEVICE_TYPE_ENCLOSURE},
                                                                                                          {D60_EE_ADDR_1,          D60_ICM_ADDR_0,  FBE_PMC_SHIM_DEVICE_TYPE_ENCLOSURE},
                                                                                                          {D60_VIRTUAL_PHY_ADDR_0, D60_ICM_ADDR_0,  FBE_PMC_SHIM_DEVICE_TYPE_VIRTUAL},
                                                                                                          {D60_VIRTUAL_PHY_ADDR_1, D60_EE_ADDR_0,   FBE_PMC_SHIM_DEVICE_TYPE_VIRTUAL},
                                                                                                          {D60_VIRTUAL_PHY_ADDR_2, D60_EE_ADDR_1,   FBE_PMC_SHIM_DEVICE_TYPE_VIRTUAL}
                                                                                                         };

fbe_u32_t fbe_pmc_shim_callback_count;
fbe_status_t fbe_cpd_shim_callback_function_mock_D60(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context)
{
        int i, count = (int)(sizeof(device_map) / sizeof(device_address_map_t));
        mut_printf(MUT_LOG_HIGH, "Callback info: Callback_type %d", callback_info->callback_type);
    switch (callback_info->callback_type)
    {
    case FBE_CPD_SHIM_CALLBACK_TYPE_LOGIN:
        mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGIN: device_type: %d", callback_info->info.login.device_type);
        mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGIN: device_id: 0x%llX", (unsigned long long)callback_info->info.login.device_id);
        mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGIN: device_addr: 0x%llX", (unsigned long long)callback_info->info.login.device_address);
        mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGIN: parent_dev_id: 0x%llX", (unsigned long long)callback_info->info.login.parent_device_id);
        mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGIN: parent_device_address: 0x%llX", (unsigned long long)callback_info->info.login.parent_device_address);
        break;
    case FBE_CPD_SHIM_CALLBACK_TYPE_LOGOUT:
        mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGOUT: device_type: %d", callback_info->info.logout.device_type);
        mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGOUT: device_id: 0x%llX", (unsigned long long)callback_info->info.logout.device_id);
        mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGOUT: device_addr: 0x%llX", (unsigned long long)callback_info->info.logout.device_address);
        mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGOUT: parent_dev_id: 0x%llX", (unsigned long long)callback_info->info.logout.parent_device_id);
        mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGOUT: parent_device_address: 0x%llX", (unsigned long long)callback_info->info.login.parent_device_address);
        break;
    default:
        break;
    }
    fbe_pmc_shim_callback_count++;

    /* check the topology */
        for(i = 0; i < count; ++i)
        {
                if(device_map[i].device_address == callback_info->info.login.device_address)
                {
                        MUT_ASSERT_TRUE(device_map[i].parent_address == callback_info->info.login.parent_device_address);
                        MUT_ASSERT_TRUE(device_map[i].device_type == callback_info->info.login.device_type);
                }
        }

    return FBE_STATUS_OK;
}

/* this tests D60 enclosures can be created and login generated */
void terminator_api_d60_test(void)
{  
    fbe_status_t                                        status = FBE_STATUS_GENERIC_FAILURE;
    tdr_status_t tdr_status = TDR_STATUS_GENERIC_FAILURE;
    fbe_terminator_board_info_t         board_info, temp_board_info;
    fbe_terminator_sas_port_info_t      sas_port;
    fbe_terminator_sas_encl_info_t      sas_encl;
    
    fbe_terminator_api_device_handle_t  port_handle;
    fbe_terminator_api_device_handle_t  encl_icm0_0_handle, encl_ee_0_0_0_handle, encl_ee_0_0_1_handle, encl_ee_0_0_2_handle;

    //fbe_terminator_api_device_handle_t        drive_handle;
    //fbe_terminator_sas_drive_info_t sas_drive;
    fbe_terminator_api_device_handle_t          device_list[MAX_DEVICES];
    fbe_u32_t                                           connector_id;
    terminator_cpd_port_t           cpd_port;
    fbe_u32_t                       port_index;
    fbe_cpd_shim_enumerate_backend_ports_t cpd_shim_enumerate_backend_ports;
    
    fbe_terminator_device_ptr_t port_ptr, icm_ptr, ee_ptr0, ee_ptr1;
    terminator_enclosure_t *enclosure_ptr;
    terminator_sas_enclosure_info_t *attributes = NULL;

    cpd_port.state = PMC_PORT_STATE_INITIALIZED;
    cpd_port.payload_completion_function = NULL;
    cpd_port.payload_completion_context = NULL;
    cpd_port.callback_function = fbe_cpd_shim_callback_function_mock_D60;
    cpd_port.callback_context = NULL;

    mut_printf(MUT_LOG_HIGH, "%s %s ",__FILE__, __FUNCTION__);

    /* reset the callback counter */
    fbe_pmc_shim_callback_count = 0;

    /*initialize the terminator*/
    status = fbe_terminator_api_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "%s: terminator initialized.", __FUNCTION__);

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_DREADNOUGHT_HW_TYPE;
    status  = fbe_terminator_api_insert_board (&board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*verify board type*/
    status = fbe_terminator_api_get_board_info(&temp_board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(board_info.board_type, temp_board_info.board_type);

        /*insert a port*/
        sas_port.sas_address = D60_PORT_ADDR;
        sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    sas_port.io_port_number = 1;
    sas_port.portal_number = 2;
    sas_port.backend_number = 0;
        status  = fbe_terminator_api_insert_sas_port (&sas_port, &port_handle);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_terminator_miniport_api_port_init(1, 2, &port_index);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Register a callback to port 0, so we can destroy the object properly */
    status = fbe_terminator_miniport_api_register_callback(port_index, (fbe_terminator_miniport_api_callback_function_t)fbe_cpd_shim_callback_function_mock_D60, NULL);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* enumerate the ports */
        status  = fbe_terminator_miniport_api_enumerate_cpd_ports (&cpd_shim_enumerate_backend_ports);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /*insert an enclosure to port 0*/
    sas_encl.backend_number = 0;
    sas_encl.encl_number = 0;
    sas_encl.uid[0] = 1; // some unique ID.
    sas_encl.sas_address = D60_ICM_ADDR_0;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM;
    sas_encl.connector_id = 0;
        status  = fbe_terminator_api_insert_sas_enclosure (port_handle, &sas_encl, &encl_icm0_0_handle);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /*insert an EE enclosure to ICM at connector #2*/
    sas_encl.backend_number = 0;
    sas_encl.encl_number = 0;
    sas_encl.uid[0] = 2; // some unique ID.
    sas_encl.sas_address = D60_EE_ADDR_0;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE;
    sas_encl.connector_id = 4;
    status  = fbe_terminator_api_insert_sas_enclosure (encl_icm0_0_handle, &sas_encl, &encl_ee_0_0_0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
        /*insert an EE enclosure to ICM at connector #3*/
    sas_encl.backend_number = 0;
    sas_encl.encl_number = 0;
    sas_encl.uid[0] = 3; // some unique ID.
    sas_encl.sas_address = D60_EE_ADDR_1;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE;
    sas_encl.connector_id = 5;
    status  = fbe_terminator_api_insert_sas_enclosure (encl_icm0_0_handle, &sas_encl, &encl_ee_0_0_1_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /*insert an EE enclosure to ICM at connector #2, it should fail*/
    sas_encl.backend_number = 0;
    sas_encl.encl_number = 0;
    sas_encl.uid[0] = 4; // some unique ID.
    sas_encl.sas_address = D60_EE_ADDR_2;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE;
    sas_encl.connector_id = 4;
    status  = fbe_terminator_api_insert_sas_enclosure (encl_icm0_0_handle, &sas_encl, &encl_ee_0_0_2_handle);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
    
    /* enumerate the devices on ports */
    status = fbe_terminator_api_enumerate_devices(device_list, MAX_DEVICES);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* check the number of devices in terminator:
     1 board, 1 port, 1 ICM, 2 EE and 1 invalid EE enclosure*/
    MUT_ASSERT_INT_EQUAL(6, fbe_terminator_api_get_devices_count());
    EmcutilSleep(1000);

    /* check the callback count, 1 port linkup and 3 enclosure and 3 virtual phy login */
    MUT_ASSERT_INT_EQUAL (7, fbe_pmc_shim_callback_count);
    
    /* check the main enclosure chain */
    tdr_status = fbe_terminator_device_registry_get_device_ptr(port_handle, &port_ptr);
    MUT_ASSERT_INT_EQUAL(TDR_STATUS_OK, tdr_status);
    enclosure_ptr = port_get_last_enclosure(port_ptr);
    attributes = base_component_get_attributes(&enclosure_ptr->base);
    MUT_ASSERT_TRUE(attributes->login_data.device_address == D60_ICM_ADDR_0);

    /* check the connector id */
    tdr_status = fbe_terminator_device_registry_get_device_ptr(encl_icm0_0_handle, &icm_ptr);
    MUT_ASSERT_INT_EQUAL(TDR_STATUS_OK, tdr_status);
    tdr_status = fbe_terminator_device_registry_get_device_ptr(encl_ee_0_0_0_handle, &ee_ptr0);
    MUT_ASSERT_INT_EQUAL(TDR_STATUS_OK, tdr_status);
    tdr_status = fbe_terminator_device_registry_get_device_ptr(encl_ee_0_0_1_handle, &ee_ptr1);
    MUT_ASSERT_INT_EQUAL(TDR_STATUS_OK, tdr_status);
    sas_enclosure_get_connector_id_by_enclosure(ee_ptr0, icm_ptr, &connector_id);
    MUT_ASSERT_INT_EQUAL(4, connector_id);
    sas_enclosure_get_connector_id_by_enclosure(ee_ptr1, icm_ptr, &connector_id);
    MUT_ASSERT_INT_EQUAL(5, connector_id); // Based on the Edge expander connection ids.
    sas_enclosure_get_connector_id_by_enclosure(icm_ptr, ee_ptr1, &connector_id);
    MUT_ASSERT_INT_EQUAL(CONN_IS_UPSTREAM, connector_id);

    fbe_pmc_shim_callback_count = 0;
    /* remove ICM */
    status = fbe_terminator_api_remove_device(encl_icm0_0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* enumerate the devices on ports */
    status = fbe_terminator_api_enumerate_devices(device_list, MAX_DEVICES);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* check the number of devices in terminator, 1 board, 1 port and 1 invalid EE enclosure*/
    MUT_ASSERT_INT_EQUAL(3, fbe_terminator_api_get_devices_count());
    EmcutilSleep(1000);

    /* check the callback count, 3 enclosure and 3 virtual phy logout */
    MUT_ASSERT_INT_EQUAL (6, fbe_pmc_shim_callback_count);

    ///*verify enclosure 0-1 and 0-2 are not in the device registry anymore*/
    //status = fbe_terminator_api_get_enclosure_handle(1, 1, &encl1_1_handle_new);
    //MUT_ASSERT_INT_EQUAL(FBE_STATUS_GENERIC_FAILURE, status);
    //MUT_ASSERT_INTEGER_NOT_EQUAL(encl1_1_handle, encl1_1_handle_new);
    //
    //    status = fbe_terminator_api_get_enclosure_handle(1, 2, &encl1_2_handle_new);
    //  MUT_ASSERT_INT_EQUAL(FBE_STATUS_GENERIC_FAILURE, status);
    //    MUT_ASSERT_INTEGER_NOT_EQUAL(encl1_2_handle, encl1_2_handle_new);
    //
    //    /*insert an enclosure*/
    //    sas_encl.backend_number = 1;
    //    sas_encl.encl_number = 1;
    //    sas_encl.uid[0] = 1; // some unique ID.
    //    sas_encl.sas_address = (fbe_u64_t)0x123456781;
    //  status  = fbe_terminator_api_insert_sas_enclosure (port1_handle, &sas_encl, &encl1_1_handle_new);
    //  MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    //    MUT_ASSERT_INTEGER_NOT_EQUAL(encl1_1_handle, encl1_1_handle_new);

    //    /*insert an enclosure*/
    //    sas_encl.backend_number = 1;
    //    sas_encl.encl_number = 2;
    //    sas_encl.uid[0] = 2; // some unique ID.
    //    sas_encl.sas_address = (fbe_u64_t)0x123456782;
    //  status  = fbe_terminator_api_insert_sas_enclosure (port1_handle, &sas_encl, &encl1_2_handle_new);
    //  MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    //    MUT_ASSERT_INTEGER_NOT_EQUAL(encl1_2_handle, encl1_2_handle_new);

    //    /*insert an enclosure*/
    //    sas_encl.backend_number = 2;
    //    sas_encl.encl_number = 0;
    //    sas_encl.uid[0] = 3; // some unique ID.
    //    sas_encl.sas_address = (fbe_u64_t)0x123456783;
    //    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    //  status  = fbe_terminator_api_insert_sas_enclosure (port2_handle, &sas_encl, &encl2_0_handle_new);
    //  MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    //
    //    /* check the number of devices in terminator, 1 board, 2 ports and 4 enclosure */
        //MUT_ASSERT_INT_EQUAL(7, fbe_terminator_api_get_devices_count());

    //    /*insert a sas_drive to port 0 encl 2 */
    //    sas_drive.backend_number = 1;
    //    sas_drive.encl_number = 2;
    //    sas_drive.sas_address = (fbe_u64_t)0x987654321;
    //    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    //    status  = fbe_terminator_api_insert_sas_drive (encl1_2_handle_new, 3, &sas_drive, &drive_handle);
    //    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    //    /* check the number of devices in terminator, 1 board, 2 ports and 4 enclosure 1 drive*/
    //          MUT_ASSERT_INT_EQUAL(8, fbe_terminator_api_get_devices_count());
    //
    //    /*insert a sas_drive to the last enclosure*/
    //    sas_drive.backend_number = 2;
    //    sas_drive.encl_number = 0;
    //    sas_drive.sas_address = (fbe_u64_t)0x987654322;
    //    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    //    status  = fbe_terminator_api_insert_sas_drive (encl2_0_handle_new, 0, &sas_drive, &drive_handle);
    //    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    //    /* check the number of devices in terminator, 1 board, 2 ports and 4 enclosure 2 drive*/
    //          MUT_ASSERT_INT_EQUAL(9, fbe_terminator_api_get_devices_count());
    //
    //    status = fbe_terminator_api_destroy();
    //    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    //    mut_printf(MUT_LOG_LOW, "%s: terminator destroyed.", __FUNCTION__);
    //
    //    /* wait a reasonable time */
    //    EmcutilSleep(2000);
    //    /*check the callback counter */
    //    MUT_ASSERT_TRUE(fbe_cpd_shim_callback_count != 0);
    //}

}


/*
 * D120 section
 */

/* some device sas address */
#define D120_PORT_ADDR (fbe_u64_t)0x11111111
#define D120_IOSXP_ADDR_0 (fbe_u64_t)0x22222222
#define D120_DRVSXP_ADDR_0 (fbe_u64_t)0x33333333
#define D120_DRVSXP_ADDR_1 (fbe_u64_t)0x44444444
#define D120_DRVSXP_ADDR_2 (fbe_u64_t)0x55555555
#define D120_DRVSXP_ADDR_3 (fbe_u64_t)0x66666666
#define D120_VIRTUAL_PHY_ADDR_0 ((D120_IOSXP_ADDR_0 & VIPER_VIRTUAL_PHY_SAS_ADDR_MASK) + VIPER_VIRTUAL_PHY_SAS_ADDR_LAST_6_BITS)
#define D120_VIRTUAL_PHY_ADDR_1 ((D120_DRVSXP_ADDR_0 & VIPER_VIRTUAL_PHY_SAS_ADDR_MASK) + VIPER_VIRTUAL_PHY_SAS_ADDR_LAST_6_BITS)
#define D120_VIRTUAL_PHY_ADDR_2 ((D120_DRVSXP_ADDR_1 & VIPER_VIRTUAL_PHY_SAS_ADDR_MASK) + VIPER_VIRTUAL_PHY_SAS_ADDR_LAST_6_BITS)
#define D120_VIRTUAL_PHY_ADDR_3 ((D120_DRVSXP_ADDR_2 & VIPER_VIRTUAL_PHY_SAS_ADDR_MASK) + VIPER_VIRTUAL_PHY_SAS_ADDR_LAST_6_BITS)
#define D120_VIRTUAL_PHY_ADDR_4 ((D120_DRVSXP_ADDR_3 & VIPER_VIRTUAL_PHY_SAS_ADDR_MASK) + VIPER_VIRTUAL_PHY_SAS_ADDR_LAST_6_BITS)

/* a map from child device address to parent device address and device type.
 *                                               ----------------------------------------------------------------------------------------
 *                                                    device_address                  parent_address      device_type
 *                                               ----------------------------------------------------------------------------------------*/
device_address_map_t d120_device_map[] = {            {D120_IOSXP_ADDR_0,       D120_PORT_ADDR,     FBE_PMC_SHIM_DEVICE_TYPE_ENCLOSURE},
                                                      {D120_DRVSXP_ADDR_0,      D120_IOSXP_ADDR_0,  FBE_PMC_SHIM_DEVICE_TYPE_ENCLOSURE},
                                                      {D120_DRVSXP_ADDR_1,      D120_IOSXP_ADDR_0,  FBE_PMC_SHIM_DEVICE_TYPE_ENCLOSURE},
                                                      {D120_DRVSXP_ADDR_2,      D120_IOSXP_ADDR_0,  FBE_PMC_SHIM_DEVICE_TYPE_ENCLOSURE},
                                                      {D120_DRVSXP_ADDR_3,      D120_IOSXP_ADDR_0,  FBE_PMC_SHIM_DEVICE_TYPE_ENCLOSURE},
                                                      {D120_VIRTUAL_PHY_ADDR_0, D120_IOSXP_ADDR_0,  FBE_PMC_SHIM_DEVICE_TYPE_VIRTUAL},
                                                      {D120_VIRTUAL_PHY_ADDR_1, D120_DRVSXP_ADDR_0, FBE_PMC_SHIM_DEVICE_TYPE_VIRTUAL},
                                                      {D120_VIRTUAL_PHY_ADDR_2, D120_DRVSXP_ADDR_1, FBE_PMC_SHIM_DEVICE_TYPE_VIRTUAL},
                                                      {D120_VIRTUAL_PHY_ADDR_3, D120_DRVSXP_ADDR_2, FBE_PMC_SHIM_DEVICE_TYPE_VIRTUAL},
                                                      {D120_VIRTUAL_PHY_ADDR_4, D120_DRVSXP_ADDR_3, FBE_PMC_SHIM_DEVICE_TYPE_VIRTUAL}
                                    };

fbe_u32_t fbe_pmc_shim_callback_count;
fbe_status_t fbe_cpd_shim_callback_function_mock_D120(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context)
{
    int i, count = (int)(sizeof(d120_device_map) / sizeof(device_address_map_t));
    mut_printf(MUT_LOG_HIGH, "Callback info: Callback_type %d", callback_info->callback_type);
    switch (callback_info->callback_type)
    {
    case FBE_CPD_SHIM_CALLBACK_TYPE_LOGIN:
        mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGIN: device_type: %d", callback_info->info.login.device_type);
        mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGIN: device_id: 0x%llX", (unsigned long long)callback_info->info.login.device_id);
        mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGIN: device_addr: 0x%llX", (unsigned long long)callback_info->info.login.device_address);
        mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGIN: parent_dev_id: 0x%llX", (unsigned long long)callback_info->info.login.parent_device_id);
        mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGIN: parent_device_address: 0x%llX", (unsigned long long)callback_info->info.login.parent_device_address);
        break;
    case FBE_CPD_SHIM_CALLBACK_TYPE_LOGOUT:
        mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGOUT: device_type: %d", callback_info->info.logout.device_type);
        mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGOUT: device_id: 0x%llX", (unsigned long long)callback_info->info.logout.device_id);
        mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGOUT: device_addr: 0x%llX", (unsigned long long)callback_info->info.logout.device_address);
        mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGOUT: parent_dev_id: 0x%llX", (unsigned long long)callback_info->info.logout.parent_device_id);
        mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGOUT: parent_device_address: 0x%llX", (unsigned long long)callback_info->info.login.parent_device_address);
        break;
    default:
        break;
    }
    fbe_pmc_shim_callback_count++;

    /* check the topology */
    for(i = 0; i < count; ++i)
    {
        if(d120_device_map[i].device_address == callback_info->info.login.device_address)
        {
        mut_printf(MUT_LOG_HIGH, "Callback info: Index %d, Count %d", i,count);
            MUT_ASSERT_TRUE(d120_device_map[i].parent_address == callback_info->info.login.parent_device_address);
            MUT_ASSERT_TRUE(d120_device_map[i].device_type == callback_info->info.login.device_type);
        }
    }

    return FBE_STATUS_OK;
}


/* this tests D120 enclosures can be created and login generated */
void terminator_api_d120_test(void)
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    tdr_status_t                    tdr_status = TDR_STATUS_GENERIC_FAILURE;
    fbe_terminator_board_info_t     board_info, temp_board_info;
    fbe_terminator_sas_port_info_t  sas_port;
    fbe_terminator_sas_encl_info_t  sas_encl;
    
    fbe_terminator_api_device_handle_t  port_handle;
    fbe_terminator_api_device_handle_t  encl_iosxp_0_0_handle, encl_drvsxp_0_0_0_handle, encl_drvsxp_0_0_1_handle, encl_drvsxp_0_0_2_handle, encl_drvsxp_0_0_3_handle;

    fbe_terminator_api_device_handle_t  device_list[MAX_DEVICES];
    fbe_u32_t                           connector_id;
    terminator_cpd_port_t               cpd_port;
    fbe_u32_t                           port_index;
    fbe_cpd_shim_enumerate_backend_ports_t cpd_shim_enumerate_backend_ports;
    
    fbe_terminator_device_ptr_t          port_ptr, iosxp_ptr, drvsxp_ptr0, drvsxp_ptr1, drvsxp_ptr2, drvsxp_ptr3;
    terminator_enclosure_t               *enclosure_ptr;
    terminator_sas_enclosure_info_t      *attributes = NULL;

    cpd_port.state = PMC_PORT_STATE_INITIALIZED;
    cpd_port.payload_completion_function = NULL;
    cpd_port.payload_completion_context = NULL;
    cpd_port.callback_function = fbe_cpd_shim_callback_function_mock_D60;
    cpd_port.callback_context = NULL;

    mut_printf(MUT_LOG_HIGH, "%s %s ",__FILE__, __FUNCTION__);

    /* reset the callback counter */
    fbe_pmc_shim_callback_count = 0;

    /*initialize the terminator*/
    status = fbe_terminator_api_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "%s: terminator initialized.", __FUNCTION__);

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_DREADNOUGHT_HW_TYPE;
    status  = fbe_terminator_api_insert_board (&board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

  mut_printf(MUT_LOG_LOW, " InsertBoard");
    /*verify board type*/
    status = fbe_terminator_api_get_board_info(&temp_board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(board_info.board_type, temp_board_info.board_type);

  mut_printf(MUT_LOG_LOW, " InsertPort");
    /*insert a port*/
    sas_port.sas_address = D120_PORT_ADDR;
    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    sas_port.io_port_number = 1;
    sas_port.portal_number = 2;
    sas_port.backend_number = 0;
    status  = fbe_terminator_api_insert_sas_port (&sas_port, &port_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

  mut_printf(MUT_LOG_LOW, " MiniportInit");
    status = fbe_terminator_miniport_api_port_init(1, 2, &port_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

  mut_printf(MUT_LOG_LOW, " MiniportRegCallback");
    /* Register a callback to port 0, so we can destroy the object properly */
    status = fbe_terminator_miniport_api_register_callback(port_index, 
                                                           (fbe_terminator_miniport_api_callback_function_t)fbe_cpd_shim_callback_function_mock_D120, 
                                                           NULL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

  mut_printf(MUT_LOG_LOW, " MiniportEnumeratePorts");
    /* enumerate the ports */
    status  = fbe_terminator_miniport_api_enumerate_cpd_ports (&cpd_shim_enumerate_backend_ports);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    // Viking: 1 IOSXP, 4 DRVSXP

  mut_printf(MUT_LOG_LOW, " InsertIOSXP");
    /*insert IOSXP enclosure to port 0*/
    sas_encl.backend_number = 0;
    sas_encl.encl_number = 0;
    sas_encl.uid[0] = 1; // some unique ID.
    sas_encl.sas_address = D120_IOSXP_ADDR_0;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP;
    sas_encl.connector_id = 0;
    status  = fbe_terminator_api_insert_sas_enclosure (port_handle, &sas_encl, &encl_iosxp_0_0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

  mut_printf(MUT_LOG_LOW, " InsertDRVSXP0");
    /*insert a DRVSXP enclosure to IOSXP at connector #2*/
    sas_encl.backend_number = 0;
    sas_encl.encl_number = 0;
    sas_encl.uid[0] = 2; // some unique ID.
    sas_encl.sas_address = D120_DRVSXP_ADDR_0;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP;
    sas_encl.connector_id = 2;
    status  = fbe_terminator_api_insert_sas_enclosure (encl_iosxp_0_0_handle, &sas_encl, &encl_drvsxp_0_0_0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
  mut_printf(MUT_LOG_LOW, " InsertDRVSXP1");
    /*insert a DRVSXP enclosure to IOSXP at connector #3*/
    sas_encl.backend_number = 0;
    sas_encl.encl_number = 0;
    sas_encl.uid[0] = 3; // some unique ID.
    sas_encl.sas_address = D120_DRVSXP_ADDR_1;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP;
    sas_encl.connector_id = 3;
    status  = fbe_terminator_api_insert_sas_enclosure (encl_iosxp_0_0_handle, &sas_encl, &encl_drvsxp_0_0_1_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

  mut_printf(MUT_LOG_LOW, " InsertDRVSXP2");
    /*insert a DRVSXP enclosure to IOSXP at connector #4 */
    sas_encl.backend_number = 0;
    sas_encl.encl_number = 0;
    sas_encl.uid[0] = 4; // some unique ID.
    sas_encl.sas_address = D120_DRVSXP_ADDR_2;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP;
    sas_encl.connector_id = 4;
    status  = fbe_terminator_api_insert_sas_enclosure (encl_iosxp_0_0_handle, &sas_encl, &encl_drvsxp_0_0_2_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
  mut_printf(MUT_LOG_LOW, " InsertDRVSXP3");
    /*insert a DRVSXP enclosure to IOSXP at connector #5 */
    sas_encl.backend_number = 0;
    sas_encl.encl_number = 0;
    sas_encl.uid[0] = 5; // some unique ID.
    sas_encl.sas_address = D120_DRVSXP_ADDR_3;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP;
    sas_encl.connector_id = 5;
    status  = fbe_terminator_api_insert_sas_enclosure (encl_iosxp_0_0_handle, &sas_encl, &encl_drvsxp_0_0_3_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
  mut_printf(MUT_LOG_LOW, " EnumerateDevices");
    /* enumerate the devices on ports */
    status = fbe_terminator_api_enumerate_devices(device_list, MAX_DEVICES);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* check the number of devices in terminator:
     1 board, 1 port, 1 IOSXP, 4 DRVSXP enclosure*/
    MUT_ASSERT_INT_EQUAL(7, fbe_terminator_api_get_devices_count());
    EmcutilSleep(1000);

    /* check the callback count, 1 port linkup and 5 enclosure and 5 virtual phy login */
    MUT_ASSERT_INT_EQUAL (11, fbe_pmc_shim_callback_count);
    
  mut_printf(MUT_LOG_LOW, " DevRegGetptr port");
    /* check the main enclosure chain */
    tdr_status = fbe_terminator_device_registry_get_device_ptr(port_handle, &port_ptr);
    MUT_ASSERT_INT_EQUAL(TDR_STATUS_OK, tdr_status);
    enclosure_ptr = port_get_last_enclosure(port_ptr);
    attributes = base_component_get_attributes(&enclosure_ptr->base);
    MUT_ASSERT_TRUE(attributes->login_data.device_address == D120_IOSXP_ADDR_0);

  mut_printf(MUT_LOG_LOW, " DevRegGetptr iosxp");
    /* check the connector id */
    tdr_status = fbe_terminator_device_registry_get_device_ptr(encl_iosxp_0_0_handle, &iosxp_ptr);
    MUT_ASSERT_INT_EQUAL(TDR_STATUS_OK, tdr_status);
    tdr_status = fbe_terminator_device_registry_get_device_ptr(encl_drvsxp_0_0_0_handle, &drvsxp_ptr0);
    MUT_ASSERT_INT_EQUAL(TDR_STATUS_OK, tdr_status);
    tdr_status = fbe_terminator_device_registry_get_device_ptr(encl_drvsxp_0_0_1_handle, &drvsxp_ptr1);
    MUT_ASSERT_INT_EQUAL(TDR_STATUS_OK, tdr_status);
    tdr_status = fbe_terminator_device_registry_get_device_ptr(encl_drvsxp_0_0_2_handle, &drvsxp_ptr2);
    MUT_ASSERT_INT_EQUAL(TDR_STATUS_OK, tdr_status);
    tdr_status = fbe_terminator_device_registry_get_device_ptr(encl_drvsxp_0_0_3_handle, &drvsxp_ptr3);
    MUT_ASSERT_INT_EQUAL(TDR_STATUS_OK, tdr_status);
    sas_enclosure_get_connector_id_by_enclosure(drvsxp_ptr0, iosxp_ptr, &connector_id);
    MUT_ASSERT_INT_EQUAL(2, connector_id);
    sas_enclosure_get_connector_id_by_enclosure(drvsxp_ptr1, iosxp_ptr, &connector_id);
    MUT_ASSERT_INT_EQUAL(3, connector_id); // Based on the Edge expander connection ids.
    sas_enclosure_get_connector_id_by_enclosure(drvsxp_ptr2, iosxp_ptr, &connector_id);
    MUT_ASSERT_INT_EQUAL(4, connector_id);
    sas_enclosure_get_connector_id_by_enclosure(drvsxp_ptr3, iosxp_ptr, &connector_id);
    MUT_ASSERT_INT_EQUAL(5, connector_id); // Based on the Edge expander connection ids.
    sas_enclosure_get_connector_id_by_enclosure(iosxp_ptr, drvsxp_ptr1, &connector_id);
    MUT_ASSERT_INT_EQUAL(CONN_IS_UPSTREAM, connector_id);

    fbe_pmc_shim_callback_count = 0;
    /* remove ICM */
    status = fbe_terminator_api_remove_device(encl_iosxp_0_0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* enumerate the devices on ports */
    status = fbe_terminator_api_enumerate_devices(device_list, MAX_DEVICES);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* check the number of devices in terminator, 1 board, 1 port*/
    MUT_ASSERT_INT_EQUAL(2, fbe_terminator_api_get_devices_count());
    EmcutilSleep(1000);

    /* check the callback count, 5 enclosure and 5 virtual phy logout */
    MUT_ASSERT_INT_EQUAL (10, fbe_pmc_shim_callback_count);
    
    

    ///*verify enclosure 0-1 and 0-2 are not in the device registry anymore*/
    //status = fbe_terminator_api_get_enclosure_handle(1, 1, &encl1_1_handle_new);
    //MUT_ASSERT_INT_EQUAL(FBE_STATUS_GENERIC_FAILURE, status);
    //MUT_ASSERT_INTEGER_NOT_EQUAL(encl1_1_handle, encl1_1_handle_new);
    //
    //    status = fbe_terminator_api_get_enclosure_handle(1, 2, &encl1_2_handle_new);
    //  MUT_ASSERT_INT_EQUAL(FBE_STATUS_GENERIC_FAILURE, status);
    //    MUT_ASSERT_INTEGER_NOT_EQUAL(encl1_2_handle, encl1_2_handle_new);
    //
    //    /*insert an enclosure*/
    //    sas_encl.backend_number = 1;
    //    sas_encl.encl_number = 1;
    //    sas_encl.uid[0] = 1; // some unique ID.
    //    sas_encl.sas_address = (fbe_u64_t)0x123456781;
    //  status  = fbe_terminator_api_insert_sas_enclosure (port1_handle, &sas_encl, &encl1_1_handle_new);
    //  MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    //    MUT_ASSERT_INTEGER_NOT_EQUAL(encl1_1_handle, encl1_1_handle_new);

    //    /*insert an enclosure*/
    //    sas_encl.backend_number = 1;
    //    sas_encl.encl_number = 2;
    //    sas_encl.uid[0] = 2; // some unique ID.
    //    sas_encl.sas_address = (fbe_u64_t)0x123456782;
    //  status  = fbe_terminator_api_insert_sas_enclosure (port1_handle, &sas_encl, &encl1_2_handle_new);
    //  MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    //    MUT_ASSERT_INTEGER_NOT_EQUAL(encl1_2_handle, encl1_2_handle_new);

    //    /*insert an enclosure*/
    //    sas_encl.backend_number = 2;
    //    sas_encl.encl_number = 0;
    //    sas_encl.uid[0] = 3; // some unique ID.
    //    sas_encl.sas_address = (fbe_u64_t)0x123456783;
    //    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    //  status  = fbe_terminator_api_insert_sas_enclosure (port2_handle, &sas_encl, &encl2_0_handle_new);
    //  MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    //
    //    /* check the number of devices in terminator, 1 board, 2 ports and 4 enclosure */
        //MUT_ASSERT_INT_EQUAL(7, fbe_terminator_api_get_devices_count());

    //    /*insert a sas_drive to port 0 encl 2 */
    //    sas_drive.backend_number = 1;
    //    sas_drive.encl_number = 2;
    //    sas_drive.sas_address = (fbe_u64_t)0x987654321;
    //    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    //    status  = fbe_terminator_api_insert_sas_drive (encl1_2_handle_new, 3, &sas_drive, &drive_handle);
    //    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    //    /* check the number of devices in terminator, 1 board, 2 ports and 4 enclosure 1 drive*/
    //          MUT_ASSERT_INT_EQUAL(8, fbe_terminator_api_get_devices_count());
    //
    //    /*insert a sas_drive to the last enclosure*/
    //    sas_drive.backend_number = 2;
    //    sas_drive.encl_number = 0;
    //    sas_drive.sas_address = (fbe_u64_t)0x987654322;
    //    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    //    status  = fbe_terminator_api_insert_sas_drive (encl2_0_handle_new, 0, &sas_drive, &drive_handle);
    //    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    //    /* check the number of devices in terminator, 1 board, 2 ports and 4 enclosure 2 drive*/
    //          MUT_ASSERT_INT_EQUAL(9, fbe_terminator_api_get_devices_count());
    //
    //    status = fbe_terminator_api_destroy();
    //    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    //    mut_printf(MUT_LOG_LOW, "%s: terminator destroyed.", __FUNCTION__);
    //
    //    /* wait a reasonable time */
    //    EmcutilSleep(2000);
    //    /*check the callback counter */
    //    MUT_ASSERT_TRUE(fbe_cpd_shim_callback_count != 0);
    //}

}
