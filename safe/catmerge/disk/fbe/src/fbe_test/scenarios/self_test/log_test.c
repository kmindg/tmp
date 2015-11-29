/***************************************************************************
 * Copyright (C) EMC Corporation 2010-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @log_test.c
 ***************************************************************************
 *
 * @brief
 *  fbe_test self test for SP log stuff.
 *
 *  01/27/2011 Created by Bo Gao
 *
 ***************************************************************************/

#define I_AM_NATIVE_CODE
#include <windows.h>
#include "physical_package_tests.h"
#include "fbe/fbe_physical_package.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"

#include "fbe_test_configurations.h"
#include "pp_utils.h"
#include "self_tests.h"

#define STATIC_NUMBER_OF_OBJ 5

char * fbe_test_self_test_log_test_short_desc = "This test loads and verifies simple configuration on both SPs to generate log and check existence of log files.";

static fbe_status_t fbe_test_load_simple_config_local(void);
static void simple_config_load_and_verify(fbe_sim_transport_connection_target_t target);
static void check_log_dir(void);

static void simple_config_load_and_verify(fbe_sim_transport_connection_target_t target)
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;

    fbe_u32_t                           object_handle = 0;
    fbe_u32_t                           port_idx = 0, drive_idx = 0;
    fbe_port_number_t                   port_number;
    fbe_u32_t                           total_objects = 0;
    fbe_class_id_t                      class_id;
    fbe_u32_t                           device_count = 0;

    fbe_api_sim_transport_set_target_server(target);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: using new creation API and Terminator Class Management\n", __FUNCTION__);
    mut_printf(MUT_LOG_LOW, "=== configuring terminator ===\n");
    /*before loading the physical package we initialize the terminator*/
    status = fbe_api_terminator_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_test_load_simple_config_local();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

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

    mut_printf(MUT_LOG_LOW, "=== starting system discovery ===\n");
    /* wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(STATIC_NUMBER_OF_OBJ, 20000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_LOW, "=== verifying configuration ===\n");

    /*we inserted a fleet board so we check for it*/
    /* board is always object id 0 ??? Maybe we need an interface to get the board object id*/
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type....Passed");

    /* We should have exactly one port. */
    for (port_idx = 0; port_idx < 8; port_idx++)
    {
        if (port_idx < 1)
        {
            /* Get the handle of the port and validate port exists*/
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

            /* Validate that the enclosure is in READY state */
            status = fbe_zrt_wait_enclosure_status(port_idx, 0,
                                FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

            for (drive_idx = 0; drive_idx < 2; drive_idx++)
            {
                /* Validate that the physical drives are in READY state */
                status = fbe_test_pp_util_verify_pdo_state(port_idx, 0, drive_idx,
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

    /*let's see how many objects we have, at this stage we should have 7 objects:
    1 board
    1 port
    1 enclosure
    2 physical drives
    */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == STATIC_NUMBER_OF_OBJ);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");

    status = fbe_api_terminator_get_devices_count_by_type_name("Board", &device_count);
    MUT_ASSERT_TRUE(device_count == 1);
    mut_printf(MUT_LOG_LOW, "Verifying board count...Passed");
    /*TODO, check here for the existanse of other objects, their SAS address and do on, using the FBE API*/

    mut_printf(MUT_LOG_LOW, "=== Configuration verification successful !!!!! ===\n");
    return;
}

static fbe_status_t fbe_test_load_simple_config_local(void)
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_board_info_t     board_info;
    fbe_terminator_sas_port_info_t  sas_port;
    fbe_terminator_sas_encl_info_t  sas_encl;
    fbe_terminator_sas_drive_info_t sas_drive;

    fbe_api_terminator_device_handle_t  port0_handle;
    fbe_api_terminator_device_handle_t  encl0_0_handle;
    fbe_api_terminator_device_handle_t  drive_handle;

    fbe_cpd_shim_hardware_info_t                hardware_info;

    fbe_u32_t                       slot = 0;

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_DEFIANT_M1_HW_TYPE;
    status  = fbe_api_terminator_insert_board (&board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a port*/
    sas_port.backend_number = 0;
    sas_port.io_port_number = 3;
    sas_port.portal_number = 5;
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
    return status;
}

/* check the self test log dir, if does not exist, create it */
static void check_log_dir(void)
{
    csx_p_file_find_info_t   FindFileData;
    csx_p_file_find_handle_t handle;
    csx_status_e             status;

    status = csx_p_file_find_first(&handle, FBE_TEST_SELF_TEST_LOG_DIR_NAME,
                                   CSX_P_FILE_FIND_FLAGS_NONE, &FindFileData);

    if (!CSX_SUCCESS(status))
    {
        status = csx_p_dir_create(FBE_TEST_SELF_TEST_LOG_DIR);
        MUT_ASSERT_TRUE_MSG(CSX_SUCCESS(status),
                            "error creating self test log dir!\n")
    }
    else
    {
       csx_p_file_find_close(handle);
    }
}

void fbe_test_self_test_log_test(void)
{
    char *mut_logdir = NULL, *mut_log_file_name = NULL, *slash, *dot;
    char buff[256], log_file_name[256];
    csx_p_file_find_info_t FindFileData;
    csx_p_file_find_handle_t handle;
    csx_status_e status;

    /* if log dir does not exist, create it */
    check_log_dir();

    /* load config on SPA to generate SPA log */
    mut_printf(MUT_LOG_LOW, "Starting load and verify on SPA");
    simple_config_load_and_verify(FBE_SIM_SP_A);
    /* load config on SPB to generate SPB log */
    mut_printf(MUT_LOG_LOW, "Starting load and verify on SPB");
    simple_config_load_and_verify(FBE_SIM_SP_B);

    /* check existence of log files */
    mut_logdir = mut_get_logdir();
    mut_log_file_name =  mut_get_log_file_name();
    /* generate log file names */
    slash = strrchr(mut_log_file_name, '/') > strrchr(mut_log_file_name, '\\') ? strrchr(mut_log_file_name, '/') : strrchr(mut_log_file_name, '\\');
    MUT_ASSERT_TRUE_MSG(slash != NULL, "mut log file name incorrect!\n");

    dot = strrchr(mut_log_file_name, '.');
    MUT_ASSERT_TRUE_MSG(dot != NULL, "mut log file name incorrect!\n");
    MUT_ASSERT_TRUE_MSG(dot - 1 > slash, "mut log file name incorrect!\n");

    strcpy(buff, mut_logdir);
    strncat(buff, slash + 1, dot - slash - 1);

    /* check SPA log file */
    mut_printf(MUT_LOG_LOW, "Starting check log file of SPA");
    strcpy(log_file_name, buff);
    strcat(log_file_name, "_spa.dbt");
    (void) csx_p_file_find_first(&handle, log_file_name,
                                 CSX_P_FILE_FIND_FLAGS_NONE, &FindFileData);
    MUT_ASSERT_TRUE_MSG(handle != NULL, "SP log file missing!\n")
    csx_p_file_find_close(handle);
    mut_printf(MUT_LOG_LOW, "Checking log file of SPA passes");

    /* check SPB log file */
    mut_printf(MUT_LOG_LOW, "Starting check log file of SPB");
    strcpy(log_file_name, buff);
    strcat(log_file_name, "_spb.dbt");
    status = csx_p_file_find_first(&handle, log_file_name,
                                 CSX_P_FILE_FIND_FLAGS_NONE, &FindFileData);
    MUT_ASSERT_TRUE_MSG(status == CSX_STATUS_SUCCESS, "SP log file missing!\n")
    csx_p_file_find_close(handle);
    mut_printf(MUT_LOG_LOW, "Checking log file of SPB passes");
}
