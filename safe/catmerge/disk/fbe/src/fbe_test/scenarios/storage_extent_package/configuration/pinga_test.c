/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file pinga_test.c
 ***************************************************************************
 *
 * @brief
 *   This file validates the physical drive
 *
 * @version
 *   06/05/2014 - Created.  Wayne Garrett
 *
 ***************************************************************************/


/*-----------------------------------------------------------------------------
    INCLUDES OF REQUIRED HEADER FILES:
*/
#include "fbe/fbe_physical_drive.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_terminator_common.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_random.h"
#include "fbe_test_common_utils.h"
#include "fbe_test_configurations.h"
#include "pp_utils.h"
#include "sep_tests.h"
#include "sep_utils.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_panic_sp_interface.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_drive_configuration_interface.h"
#include "fbe/fbe_api_drive_configuration_service_interface.h"
#include "fbe/fbe_esp_drive_mgmt.h"
#include "fbe/fbe_api_esp_drive_mgmt_interface.h"
#include "fbe/fbe_api_dest_injection_interface.h"



/*-----------------------------------------------------------------------------
    TEST DESCRIPTION:
*/

char * pinga_drive_validation_short_desc = "Physical Drive validation";
char * pinga_drive_validation_long_desc = 
    "Tests Port Lockup handling in PDO \n"
    "Validates - 4K formated drive not allowed in system slot \n"
    "Validates - Reports correct speed capacity on different drives\n"
    "Validates - Datbase works on very slow drives\n"
    "Validates - Unmap on SSDs\n"
    ;


/*-----------------------------------------------------------------------------
    DEFINITIONS:
*/

typedef struct pinga_validate_info_s {
    fbe_sas_drive_type_t        drive_type;
    fbe_u32_t                   block_size;
    fbe_physical_drive_speed_t  speed_capability;
} pinga_validate_info_t;

static pinga_validate_info_t validate_drives[] = {
    {
        FBE_SAS_DRIVE_CHEETA_15K,
        520,
        FBE_DRIVE_SPEED_6GB,
    },
    {
        FBE_SAS_DRIVE_SIM_520,
        520,
        FBE_DRIVE_SPEED_6GB,
    },
    {
        FBE_SAS_DRIVE_SIM_520_FLASH_HE,
        520,
        FBE_DRIVE_SPEED_6GB,
    },
    {
        FBE_SAS_DRIVE_SIM_520_12G,
        520,
        FBE_DRIVE_SPEED_12GB,
    }
};

/*****************************************************************************/

#define PINGA_IGNORE_WPD 0xFF

typedef struct pinga_validate_drive_type_s {
    fbe_drive_type_t            pdo_drive_type;
    fbe_sas_drive_type_t        terminator_drive_type;
    fbe_u8_t                    writes_per_day;  /* page C0, byte 20:  high bit means valid, remaining are wpd */
    fbe_bool_t                  has_sata_paddlecard;
} pinga_validate_drive_type_t;


static pinga_validate_drive_type_t validate_drive_types[] = {
    {
        FBE_DRIVE_TYPE_SAS,
        FBE_SAS_DRIVE_SIM_520,
        PINGA_IGNORE_WPD,
        FBE_FALSE
    },
    {
        FBE_DRIVE_TYPE_SAS_NL,
        FBE_SAS_NL_DRIVE_SIM_520,
        PINGA_IGNORE_WPD,
        FBE_FALSE
    },
    {
        FBE_DRIVE_TYPE_SAS_FLASH_HE,
        FBE_SAS_DRIVE_SIM_520_FLASH_HE,
        0x00,
        FBE_FALSE
    },
    {
        FBE_DRIVE_TYPE_SAS_FLASH_HE,
        FBE_SAS_DRIVE_SIM_520_FLASH_HE,
        PINGA_IGNORE_WPD,
        FBE_FALSE
    },
    {
        FBE_DRIVE_TYPE_SATA_FLASH_HE,
        FBE_SAS_DRIVE_SIM_520_FLASH_HE,
        0x00,
        FBE_TRUE
    },
    {
        FBE_DRIVE_TYPE_SATA_FLASH_HE,
        FBE_SAS_DRIVE_SIM_520_FLASH_HE,
        PINGA_IGNORE_WPD,
        FBE_TRUE
    },
    {
        FBE_DRIVE_TYPE_SAS_FLASH_ME,
        FBE_SAS_DRIVE_SIM_520_FLASH_ME,
        PINGA_IGNORE_WPD,
        FBE_FALSE
    },
    {
        FBE_DRIVE_TYPE_SAS_FLASH_LE,
        FBE_SAS_DRIVE_SIM_520_FLASH_LE,
        PINGA_IGNORE_WPD,
        FBE_FALSE
    },
    {
        FBE_DRIVE_TYPE_SAS_FLASH_RI,
        FBE_SAS_DRIVE_SIM_520_FLASH_RI,
        PINGA_IGNORE_WPD,
        FBE_FALSE
    },

};


/*****************************************************************************/
#define PINGA_EMC_STRING_SIZE 8

typedef struct pinga_validate_product_id_s {
    /* create drive with these parameters*/
    fbe_u8_t                    emc_string[PINGA_EMC_STRING_SIZE+1];   
    fbe_u8_t                    tla[FBE_SCSI_INQUIRY_TLA_SIZE+1]; 
    fbe_sas_drive_type_t        terminator_drive_type;  
    /* validate this*/
    fbe_bool_t                  is_drive_supported;
    fbe_drive_type_t            pdo_drive_type;  
    fbe_bool_t                  is_spindown_capable;
} pinga_validate_product_id_t;


static pinga_validate_product_id_t validate_product_ids[] = {
    {
        "CLAR3000",
        "005051111PWR",
        FBE_SAS_DRIVE_SIM_520,
        FBE_TRUE,               /* supported */
        FBE_DRIVE_TYPE_SAS,
        FBE_TRUE,               /* spindown capable */
    },
    {
        "CLAR3000",
        "005051111   ",
        FBE_SAS_DRIVE_SIM_520,
        FBE_TRUE,               /* supported */
        FBE_DRIVE_TYPE_SAS,
        FBE_FALSE,               /* spindown capable */
    },
    {
        " NEO3000",
        "005051111PWR",
        FBE_SAS_DRIVE_SIM_520,
        FBE_TRUE,               /* supported */
        FBE_DRIVE_TYPE_SAS,
        FBE_TRUE,               /* spindown capable */
    },
    {
        " EMC3000",
        "005051111   ",
        FBE_SAS_DRIVE_SIM_520,
        FBE_TRUE,               /* supported */
        FBE_DRIVE_TYPE_SAS,
        FBE_TRUE,               /* spindown capable */
    },
    {
        "CLAR3200",
        "005051111M  ",
        FBE_SAS_DRIVE_SIM_520_FLASH_LE,
        FBE_TRUE,               /* supported */
        FBE_DRIVE_TYPE_SAS_FLASH_LE,
        FBE_FALSE,              /* spindown capable */
    },
    {
        " NEO3200",
        "005051111M  ",
        FBE_SAS_DRIVE_SIM_520_FLASH_LE,
        FBE_TRUE,               /* supported */
        FBE_DRIVE_TYPE_SAS_FLASH_LE,
        FBE_FALSE,              /* spindown capable */
    },
    {
        " EMC3200",
        "005051111   ",
        FBE_SAS_DRIVE_SIM_520_FLASH_LE,
        FBE_TRUE,               /* supported */
        FBE_DRIVE_TYPE_SAS_FLASH_LE,
        FBE_FALSE,              /* spindown capable */
    },
    {
        "_EMC3000",             /* invalid ID */
        "005051111   ",
        FBE_SAS_DRIVE_SIM_520,
        FBE_FALSE,              /* supported */
        FBE_DRIVE_TYPE_SAS,
        FBE_TRUE,               /* spindown capable */
    },
    {
        " EMC3000",             
        "005051111PWR",         /* invalid ID */
        FBE_SAS_DRIVE_SIM_520,
        FBE_FALSE,              /* supported */
        FBE_DRIVE_TYPE_SAS,
        FBE_TRUE,               /* spindown capable */
    },
};


/*-----------------------------------------------------------------------------
     GLOBALS 
*/

/*-----------------------------------------------------------------------------
    EXTERN:
*/
extern void         biff_insert_new_drive(fbe_sas_drive_type_t drive_type, fbe_terminator_sas_drive_type_default_info_t *new_page_info_p, fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);
extern void         biff_remove_drive(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_bool_t wait_for_destroy);
extern void         biff_verify_drive_is_logically_online(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);
extern void         sleeping_beauty_verify_event_log_drive_offline(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_physical_drive_information_t *pdo_drive_info_p, fbe_u32_t death_reason);
extern void         sleeping_beauty_verify_event_log_drive_online(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);
extern fbe_status_t fbe_test_get_sas_drive_info_extend(fbe_terminator_sas_drive_info_t *sas_drive_p,
                                                       fbe_u32_t backend_number,
                                                       fbe_u32_t encl_number,
                                                       fbe_u32_t slot_number,
                                                       fbe_block_size_t block_size,
                                                       fbe_lba_t required_capacity,
                                                       fbe_sas_address_t sas_address,
                                                       fbe_sas_drive_type_t drive_type);
extern fbe_status_t sleeping_beauty_wait_for_state(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_lifecycle_state_t state, fbe_u32_t timeout_ms);
extern fbe_status_t fbe_test_dest_utils_init_error_injection_record(fbe_dest_error_record_t * error_injection_record);

/*-----------------------------------------------------------------------------
    FORWARD DECLARATIONS:
*/
void pinga_validate_4k_in_system_slots(void);
static void pinga_insert_drive(pinga_validate_info_t *info,
                               fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_lba_t capacity,
                               fbe_api_terminator_device_handle_t *drive_handle_p);
static void ping_insert_new_drive(fbe_u32_t port_number, fbe_u32_t enclosure_number,
                             fbe_u32_t slot_number);
static void pinga_validate_drive_speed_capacity(void);
static void pinga_validate_drive_tla(void);
static void pinga_validate_database_on_slow_drive(void);
static void pinga_test_port_lockup(void);
static void pinga_test_port_lockup_one_first_io(void);
static void pinga_validate_product_ids(void);
static void pinga_validate_all_drive_types(void);
static void pinga_validate_unamp_support(void);
static void pinga_validate_unamp_cmd(void);
static void pinga_build_write_same_10_unmap(fbe_physical_drive_send_pass_thru_t *write_same_10_unmap_p, fbe_u8_t *data_out_buff, fbe_u32_t bufflen, fbe_lba_t lba, fbe_block_count_t block_count);
static void pinga_build_write_same_16_unmap(fbe_physical_drive_send_pass_thru_t *write_same_16_unmap_p, fbe_u8_t *data_out_buff, fbe_u32_t bufflen, fbe_lba_t lba, fbe_block_count_t block_count);
static void pinga_build_10_byte_cdb(fbe_physical_drive_mgmt_send_pass_thru_cdb_t* cmd, fbe_lba_t lba, fbe_block_count_t block_count);
static void pinga_build_16_byte_cdb(fbe_physical_drive_mgmt_send_pass_thru_cdb_t* cmd, fbe_lba_t lba, fbe_block_count_t block_count);
void pinga_disable_background_services(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);
void pinga_enable_background_services(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);

/*-----------------------------------------------------------------------------
    FUNCTIONS:
*/

/*!****************************************************************************
 * pinga_drive_validation_test()
 ******************************************************************************
 * @brief
 *  This is the main entry point for the drive validation test   
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  06/05/2014 - Created. Wayne Garrett
 ******************************************************************************/

void pinga_drive_validation_test(void)
{    

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s START ===", __FUNCTION__); 

    pinga_validate_product_ids();
    pinga_test_port_lockup();
    pinga_validate_all_drive_types();
    pinga_validate_unamp_support();
    pinga_validate_unamp_cmd();
    pinga_validate_4k_in_system_slots();
    pinga_validate_drive_speed_capacity();
    pinga_validate_database_on_slow_drive();
#ifdef FBE_HANDLE_UNKNOWN_TLA_SUFFIX
    pinga_validate_drive_tla();
#endif
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s END ===", __FUNCTION__); 
}

/*!****************************************************************************
 *  pinga_drive_validation_setup
 ******************************************************************************
 *
 * @brief
 *  This is the setup function for the drive validation test. It is responsible
 *  for loading the physical and logical configuration for this test suite.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 * 03/10/2014 - Created. Wayne Garrett
 *****************************************************************************/
void pinga_drive_validation_setup(void)
{

    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for pinga_slot_limit test ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");

    if (fbe_test_util_is_simulation())
    {
        fbe_test_pp_util_create_physical_config_for_disk_counts(0 /*520*/,
                                                                0 /*4k*/,
                                                                TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY);

        elmo_load_esp_sep_and_neit();
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
}


/*!****************************************************************************
 *  pinga_drive_validation_cleanup
 ******************************************************************************
 *
 * @brief
 *  This is the cleanup function 
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 * 06/05/2014 - Created. Wayne Garrett
 *****************************************************************************/
void pinga_drive_validation_cleanup(void)
{  
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s ===\n", __FUNCTION__);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_esp_neit_sep_physical();
    }
    return;
} 


/*!****************************************************************************
 *  pinga_validate_4k_in_system_slots
 ******************************************************************************
 *
 * @brief
 *  Test 4k bps formatted drive is allowed in system slots.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 * 06/05/2014 - Created. Wayne Garrett
 * 01/17/2015 - 4k system drive supoort. Jibing Dong
 *****************************************************************************/
void pinga_validate_4k_in_system_slots(void)
{
    fbe_api_terminator_device_handle_t  drive_handle_520, drive_handle_4k;
    fbe_status_t status;
    fbe_u32_t system_slot = fbe_random() % 4;

    /* Test case:
            Remove system slot 0_0_0 and verify it's removed.
            Insert a 4K drive and verify it is ready.
            Move 4K drive to a differnt slot and verify it comes ready.
            Replace slot 0_0_0 with a valid system drive and verify it comes ready.
    */

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s START ===", __FUNCTION__);


    mut_printf(MUT_LOG_TEST_STATUS, "%s removing 0_0_%d", __FUNCTION__, system_slot);
    biff_remove_drive(0,0,system_slot, FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "%s inserting 4k in slot 0_0_%d.  expect ready.", __FUNCTION__, system_slot);

    status = fbe_test_pp_util_insert_unique_sas_drive(0,0,system_slot, FBE_BLOCK_SIZES_4160, TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY, &drive_handle_4k);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    /* verify 4k failed */
    status = fbe_test_pp_util_verify_pdo_state(0,0,system_slot, FBE_LIFECYCLE_STATE_READY, 10000 /*timeout*/);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    sleeping_beauty_verify_event_log_drive_online(0,0,system_slot);     

    mut_printf(MUT_LOG_TEST_STATUS, "%s relocating 4k to user slot 0_0_4.  expect ready", __FUNCTION__);

    fbe_api_sleep(1000);
    status = fbe_test_sep_drive_verify_pvd_state(0, 0, system_slot, FBE_LIFECYCLE_STATE_READY, 60000);

    status = fbe_test_pp_util_pull_drive(0,0,system_slot, &drive_handle_4k);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_pp_util_reinsert_drive(0,0,4, drive_handle_4k);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Wait until this POD is destroyed, or ESP may fail to detect the state change.
     * It can't happen in hardware as we can't replace a drive in milliseconds. */
    status = sleeping_beauty_wait_for_state(0, 0, system_slot, FBE_LIFECYCLE_STATE_DESTROY, 10000 /*ms*/);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s insert new qualified system drive to slot 0_0_0.  expect ready", __FUNCTION__);

    status = fbe_test_pp_util_insert_unique_sas_drive(0,0,system_slot, FBE_BLOCK_SIZES_520, TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY, &drive_handle_520);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_pp_util_verify_pdo_state(0,0,system_slot, FBE_LIFECYCLE_STATE_READY, 10000 /*timeout*/);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_api_sleep(1000);
    status = fbe_test_sep_drive_verify_pvd_state(0, 0, system_slot, FBE_LIFECYCLE_STATE_READY, 60000);

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s END ===\n", __FUNCTION__);
}

/*!****************************************************************************
 *  pinga_insert_drive
 ******************************************************************************
 *
 * @brief
 *  Insert a drive.
 *
 *
 * @return  None
 *
 * @author
 * 06/10/2014 - Created. Jamin Kang
 *****************************************************************************/
static void pinga_insert_drive(pinga_validate_info_t *info,
                               fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_lba_t capacity,
                               fbe_api_terminator_device_handle_t *drive_handle_p)
{
    fbe_status_t status;
    fbe_terminator_sas_drive_info_t sas_drive;
    fbe_api_terminator_device_handle_t encl_handle;
    fbe_sas_address_t new_address = fbe_test_pp_util_get_unique_sas_address();

    status = fbe_api_terminator_get_enclosure_handle(bus, encl, &encl_handle);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_test_get_sas_drive_info_extend(&sas_drive, bus, encl, slot, info->block_size, capacity, new_address, info->drive_type);
    mut_printf(MUT_LOG_LOW, "=== %s %d_%d_%d serial: %s inserted. ===",
               __FUNCTION__, bus, encl, slot, sas_drive.drive_serial_number);
    status  = fbe_api_terminator_insert_sas_drive(encl_handle, slot, &sas_drive, drive_handle_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

/*!****************************************************************************
 *  pinga_validate_drive_speed_capacity
 ******************************************************************************
 *
 * @brief
 *  Test PDO can recongize different sppeed capacity.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 * 2014/06/11 - Created. Jamin Kang
 *****************************************************************************/
static void pinga_validate_drive_speed_capacity(void)
{
    fbe_u32_t slot = 5;
    fbe_u32_t i;
    fbe_status_t status;

    for (i = 0; i < sizeof(validate_drives) / sizeof(validate_drives[0]); i += 1, slot += 1) {
        pinga_validate_info_t *drive = &validate_drives[i];
        fbe_u32_t object_handle = 0;
        fbe_physical_drive_information_t get_drive_information;
        fbe_api_terminator_device_handle_t  drive_handle;

        pinga_insert_drive(drive, 0, 0, slot, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY, &drive_handle);
        status = fbe_test_pp_util_verify_pdo_state(0, 0, slot, FBE_LIFECYCLE_STATE_READY, 10000 /*timeout*/);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_get_physical_drive_object_id_by_location(0, 0, slot, &object_handle);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);
        status = fbe_api_physical_drive_get_drive_information(object_handle, &get_drive_information, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        mut_printf(MUT_LOG_TEST_STATUS, "%u_%u_%u: speed cap: %u, block_size: %u",
                   0, 0, slot, get_drive_information.speed_capability,
                   get_drive_information.block_size);
        MUT_ASSERT_TRUE(get_drive_information.speed_capability == drive->speed_capability);

        fbe_api_sleep(1000);
        status = fbe_test_sep_drive_verify_pvd_state(0, 0, slot, FBE_LIFECYCLE_STATE_READY, 60000);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_test_pp_util_pull_drive(0, 0, slot, &drive_handle);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        status = fbe_api_wait_till_object_is_destroyed(object_handle, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
}

static void ping_insert_new_drive(fbe_u32_t port_number, fbe_u32_t enclosure_number,
                             fbe_u32_t slot_number)
{
    fbe_status_t    status;
    fbe_sas_address_t   blank_new_sas_address;
    fbe_api_terminator_device_handle_t handle;

    blank_new_sas_address = fbe_test_pp_util_get_unique_sas_address();
    status = fbe_test_pp_util_insert_sas_drive_extend(port_number, enclosure_number, slot_number,
                                                      520, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY,
                                                      blank_new_sas_address, FBE_SAS_DRIVE_SIM_520,
                                                      &handle);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

static void wait_for_database_transaction(void)
{
    fbe_status_t                     status;
    database_transaction_t           trans_info;
    fbe_u32_t                        timeout = 180000;
    fbe_u32_t                        count = 0;

    do {
        status = fbe_api_database_get_transaction_info(&trans_info);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        if (trans_info.state == DATABASE_TRANSACTION_STATE_INACTIVE) {
            mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait transaction done, count: %u\n",
                       __FUNCTION__, count);
            return;
        }
        count += 1000;
        fbe_api_sleep(1000);
    } while (count < timeout);

    MUT_FAIL_MSG("Timeout on waiting for transaction");
}


/*!****************************************************************************
 *  pinga_validate_database_on_slow_drive
 ******************************************************************************
 *
 * @brief
 *  Test Database work as expected on very slow system drive (Bad drives?
 *
 * @param   None
 *
 * @return  None
 *
 * @author
 * 2015/03/31 - Created. Jamin Kang
 *****************************************************************************/
static void pinga_validate_database_on_slow_drive(void)
{
    fbe_status_t status;
    fbe_object_id_t rg_obj = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR;
    fbe_object_id_t lun_obj = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MCR_DATABASE;
    fbe_api_lun_get_lun_info_t lun_info;
    fbe_object_id_t pvd_obj;
    fbe_raid_group_map_info_t rg_map;
    fbe_lba_t pba;
    fbe_test_raid_group_disk_set_t disk_set = { 0, 0, 7 };
    fbe_api_logical_error_injection_record_t error_record = {
        0x1,  /* pos_bitmap */
        0x3, /* width */
        0x0,  /* lba */
        0x0,  /* blocks */
        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN,
        FBE_API_LOGICAL_ERROR_INJECTION_MODE_COUNT, /* error mode */
        0x0,  /* error count */
        12000,  /* 16 seconds */
        0x0,  /* skip count */
        0x0 , /* skip limit */
        0x1,  /* error adjcency */
        0x0,  /* start bit */
        0x0,  /* number of bits */
        0x0,  /* erroneous bits */
        0x0,  /* crc detectable */
    };

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s START ===", __FUNCTION__);

    status = fbe_api_base_config_disable_all_bg_services();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_wait_for_database_service(60000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_lun_get_lun_info(lun_obj, &lun_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s Inject delay error ===", __FUNCTION__);
    /* Disable LEI and clear all records */
    status = fbe_api_logical_error_injection_disable_object(rg_obj, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_disable_records(0, 255);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    memset(&rg_map, 0, sizeof(rg_map));
    rg_map.lba = lun_info.offset;
    status = fbe_api_raid_group_map_lba(rg_obj, &rg_map);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    pba = rg_map.pba - rg_map.offset;
    error_record.lba = pba;
    error_record.blocks = lun_info.capacity;

    status = fbe_api_logical_error_injection_create_record(&error_record);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_enable_object(rg_obj, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    mut_printf(MUT_LOG_TEST_STATUS, "=== %s Inject new drive %u_%u_%u ===",
               __FUNCTION__, disk_set.bus, disk_set.enclosure, disk_set.slot);
    ping_insert_new_drive(disk_set.bus, disk_set.enclosure, disk_set.slot);
    status = fbe_test_pp_util_verify_pdo_state(disk_set.bus, disk_set.enclosure, disk_set.slot,
                                               FBE_LIFECYCLE_STATE_READY, 10000 /*timeout*/);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_test_sep_util_wait_for_pvd_creation(&disk_set, 1, 120000);

    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(disk_set.bus, disk_set.enclosure, disk_set.slot, &pvd_obj);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Wait for persist service to finish */
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s wait for persist service to finish ===", __FUNCTION__);
    wait_for_database_transaction();

    /* Disable LEI and clear all records */
    status = fbe_api_logical_error_injection_disable_object(rg_obj, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_disable_records(0, 255);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /**
     * Sleep several seconds, so that the panic can happen before we leave this function.
     * That will make the debug easier
     */
    fbe_api_sleep(2000);

    status = fbe_api_wait_for_object_lifecycle_state(pvd_obj, FBE_LIFECYCLE_STATE_READY, 60000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s finished ===", __FUNCTION__);
}

/*!****************************************************************************
 *  pinga_validate_drive_tla
 ******************************************************************************
 *
 * @brief
 *  Test PDO is in fail state when there is an invalid TLA
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 * 2015/06/15 - Created. Saranyadevi Ganesan
 *****************************************************************************/
static void pinga_validate_drive_tla(void)
{
    fbe_terminator_sas_drive_type_default_info_t drive_page_info = {0};
    fbe_dcs_tunable_params_t params = {0};
    fbe_physical_drive_attributes_t attributes = {0};
    fbe_u8_t invalid_tla[FBE_SCSI_INQUIRY_TLA_SIZE] = "005049197BLA";
    const fbe_u32_t bus = 0;
    const fbe_u32_t encl = 0;
    const fbe_u32_t slot = 5;
    fbe_bool_t prev_ignore_invalid_identity_policy_state = FBE_FALSE;
    fbe_bool_t prev_eq_policy_state = FBE_FALSE;
    fbe_bool_t curr_ignore_invalid_identity_policy_state = FBE_FALSE;
    fbe_bool_t curr_eq_policy_state = FBE_FALSE;
    fbe_object_id_t pdo_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_death_reason_t death_reason;
    const fbe_u8_t * death_reason_str = NULL;
    fbe_status_t status;
    fbe_u32_t i;

    mut_printf(MUT_LOG_LOW, "=== %s: Test Begin ===\n", __FUNCTION__);

    /* Verify the default policy to handle drive inserts with invalid TLA */
    status = fbe_api_drive_configuration_interface_get_params(&params);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL((params.control_flags & FBE_DCS_IGNORE_INVALID_IDENTITY), FBE_DCS_IGNORE_INVALID_IDENTITY);

    /* Get the previous DCS policy state */
    fbe_api_drive_configuration_interface_get_params(&params);
    prev_ignore_invalid_identity_policy_state = (params.control_flags & FBE_DCS_IGNORE_INVALID_IDENTITY)?FBE_TRUE:FBE_FALSE;
    prev_eq_policy_state = (params.control_flags & FBE_DCS_ENFORCE_ENHANCED_QUEUING_DRIVES)?FBE_TRUE:FBE_FALSE;
    mut_printf(MUT_LOG_LOW, "=== %s: Previous policy states: ignore invalid id: %d, eq: %d ===\n",
                   __FUNCTION__, prev_ignore_invalid_identity_policy_state, prev_eq_policy_state);

    /* Loop through all types of drives */
    for (i = 0; i < sizeof(validate_drives) / sizeof(validate_drives[0]); i += 1) 
    {
        fbe_sas_drive_type_t drive_type = validate_drives[i].drive_type;

        /* Insert test drive with invalid TLA and check if PDO object is FAIL */
        mut_printf(MUT_LOG_LOW, "=== %s: Insert drive type: %d in %d_%d_%d with wrong TLA and see it FAIL ===\n",
                   __FUNCTION__, drive_type, bus, encl, slot);
        fbe_api_terminator_sas_drive_get_default_page_info(drive_type, &drive_page_info);
        fbe_copy_memory(&drive_page_info.inquiry[FBE_SCSI_INQUIRY_TLA_OFFSET], invalid_tla, sizeof(invalid_tla));
        biff_insert_new_drive(drive_type, &drive_page_info, bus, encl, slot);
        status = fbe_test_pp_util_verify_pdo_state(bus, encl, slot, FBE_LIFECYCLE_STATE_FAIL, 10000 /* ms */);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Verify the death reason */
        status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &pdo_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        status = fbe_api_get_object_death_reason(pdo_object_id,
                                             &death_reason,
                                             &death_reason_str,
                                             FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(FBE_SATA_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_SET_DEV_INFO, death_reason);
        
        /* Cleanup: Remove drive */
        biff_remove_drive(bus, encl, slot, FBE_TRUE);
    }

    /* Enable ignore invalid identity flag and disable Enhanced queuing as it takes priority over the invalid identity */
    fbe_api_drive_configuration_set_control_flag(FBE_DCS_IGNORE_INVALID_IDENTITY, FBE_TRUE /*enable*/);
    fbe_api_drive_configuration_set_control_flag(FBE_DCS_ENFORCE_ENHANCED_QUEUING_DRIVES, FBE_FALSE /*disable*/);
    fbe_api_drive_configuration_interface_get_params(&params);
    curr_ignore_invalid_identity_policy_state = (params.control_flags & FBE_DCS_IGNORE_INVALID_IDENTITY)?FBE_TRUE:FBE_FALSE;
    curr_eq_policy_state = (params.control_flags & FBE_DCS_ENFORCE_ENHANCED_QUEUING_DRIVES)?FBE_TRUE:FBE_FALSE;
    mut_printf(MUT_LOG_LOW, "=== %s: Current policy states set: ignore invalid id: %d, eq: %d ===\n",
                   __FUNCTION__, curr_ignore_invalid_identity_policy_state, curr_eq_policy_state);

    /* Loop through all types of drives */
    for (i = 0; i < sizeof(validate_drives) / sizeof(validate_drives[0]); i += 1) 
    {
        fbe_sas_drive_type_t drive_type = validate_drives[i].drive_type;

        /* Insert test drive with invalid TLA and check if PDO object is READY and logically offline */
        mut_printf(MUT_LOG_LOW, "=== %s: Insert drive type: %d in %d_%d_%d with wrong TLA and see it OFFLINE ===\n",
                   __FUNCTION__, drive_type, bus, encl, slot);
        fbe_api_terminator_sas_drive_get_default_page_info(drive_type, &drive_page_info);
        fbe_copy_memory(&drive_page_info.inquiry[FBE_SCSI_INQUIRY_TLA_OFFSET], invalid_tla, sizeof(invalid_tla));
        biff_insert_new_drive(drive_type, &drive_page_info, bus, encl, slot);
        status = fbe_test_pp_util_verify_pdo_state(bus, encl, slot, FBE_LIFECYCLE_STATE_READY, 10000 /* ms */);                                                
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Get PDO object ID */
        status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &pdo_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Verify PDO not in maintenance mode since PDO policy is disabled */
        status = fbe_api_physical_drive_get_attributes(pdo_object_id, &attributes);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL_MSG(attributes.maintenance_mode_bitmap, FBE_PDO_MAINTENANCE_FLAG_INVALID_IDENTITY, "PDO not in maintenance mode");

        /* Cleanup: Remove drive */
        biff_remove_drive(bus, encl, slot, FBE_TRUE);
    }

    /* Restore previous policy state */
    fbe_api_drive_configuration_set_control_flag(FBE_DCS_IGNORE_INVALID_IDENTITY, prev_ignore_invalid_identity_policy_state);
    fbe_api_drive_configuration_set_control_flag(FBE_DCS_ENFORCE_ENHANCED_QUEUING_DRIVES, prev_eq_policy_state);
    fbe_api_drive_configuration_interface_get_params(&params);
    curr_ignore_invalid_identity_policy_state = (params.control_flags & FBE_DCS_IGNORE_INVALID_IDENTITY)?FBE_TRUE:FBE_FALSE;
    curr_eq_policy_state = (params.control_flags & FBE_DCS_ENFORCE_ENHANCED_QUEUING_DRIVES)?FBE_TRUE:FBE_FALSE;
    MUT_ASSERT_TRUE_MSG(curr_ignore_invalid_identity_policy_state == prev_ignore_invalid_identity_policy_state, 
                        "=== Ignore invalid identity policy restoration failed ===");
    MUT_ASSERT_TRUE_MSG(curr_eq_policy_state == prev_eq_policy_state, 
                        "=== EQ policy restoration failed ===");
    mut_printf(MUT_LOG_LOW, "=== %s: Restored policy states: ignore invalid id: %d, eq: %d ===\n", 
                   __FUNCTION__, curr_ignore_invalid_identity_policy_state, curr_eq_policy_state);

    mut_printf(MUT_LOG_LOW, "=== %s: Test End ===\n", __FUNCTION__);
}
/*****************************************
 * end pinga_validate_drive_tla()
 *****************************************/



/*!****************************************************************************
 *  pinga_test_port_lockup
 ******************************************************************************
 *
 * @brief
 *  Test handling port lockup when ever IO is a timeout.  Also verifies that
 *  perodic timeouts do not trigger port lockup handling.
 *
 * @param   None
 *
 * @return  None
 *
 * @author
 * 2015/09/08  Wayne Garrett
 *****************************************************************************/
static void pinga_test_port_lockup(void)
{
    fbe_u32_t bus=0, encl=0, slot=4;
    fbe_u32_t service_time_limit_ms = 400;
    fbe_object_id_t pdo_id, pvd_id;
    fbe_api_rdgen_context_t rdgen_context = {0};  
    fbe_physical_drive_dieh_stats_t dieh_stats = {0};  
    fbe_u32_t num_hc_to_kill = 0;
    fbe_u32_t wait_time_ms = 0;
    fbe_u32_t wait_interval_ms = 110;  /* slightly greater burst_reduction */
    fbe_status_t status;
    fbe_dest_error_record_t dest_record = {0};
    fbe_dest_error_record_t dest_record_out = {0};
    fbe_dest_record_handle_t dest_record_handle = NULL;
    fbe_test_raid_group_disk_set_t disk_set;
    fbe_u64_t hc_tag = 0;


    /* Test case:  
           disable bgs
           insert new drive
           inject timeout errors until HC occurs                 
           inject periodic timeouts
           verify HC does not occur.
           enable bgs
           remove drive
       */

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s START ===", __FUNCTION__);

    status = fbe_api_base_config_disable_all_bg_services();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);    
    
    biff_insert_new_drive(FBE_SAS_DRIVE_SIM_520, NULL, bus, encl, slot);
    status = fbe_test_pp_util_verify_pdo_state(bus, encl, slot, FBE_LIFECYCLE_STATE_READY, 10000 /* ms */);                                                
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &pdo_id); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(pdo_id != FBE_OBJECT_ID_INVALID);

    /* reduce service time to speed up test */
    fbe_api_physical_drive_set_service_time(pdo_id, service_time_limit_ms);   

    /* Setup error injection while PVD is connecting.  This will cause HC to be initiated. */
    fbe_test_dest_utils_init_error_injection_record(&dest_record);
    dest_record.object_id = pdo_id;
    dest_record.dest_error_type = FBE_DEST_ERROR_TYPE_PORT;
    dest_record.dest_error.dest_scsi_error.port_status = FBE_PORT_REQUEST_STATUS_ABORT_TIMEOUT;
    dest_record.lba_start = 0;
    dest_record.lba_end = FBE_U64_MAX; 
    dest_record.dest_error.dest_scsi_error.scsi_command[0] = FBE_SCSI_WRITE_10;    
    dest_record.dest_error.dest_scsi_error.scsi_command[1] = FBE_SCSI_WRITE_16;
    dest_record.dest_error.dest_scsi_error.scsi_command[2] = FBE_SCSI_READ_10;    
    dest_record.dest_error.dest_scsi_error.scsi_command[3] = FBE_SCSI_READ_16;
    dest_record.dest_error.dest_scsi_error.scsi_command_count = 4;    
    dest_record.num_of_times_to_insert = 100;
    dest_record.delay_io_msec = 0;

    status = fbe_api_dest_injection_add_record(&dest_record, &dest_record_handle);    
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_dest_injection_start();    
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);    

    /* Wait for PVD to come Ready. */
    disk_set.bus = bus;
    disk_set.enclosure = encl;
    disk_set.slot = slot;
    fbe_test_sep_util_wait_for_pvd_creation(&disk_set, 1, 10000);

    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(disk_set.bus, disk_set.enclosure, disk_set.slot, &pvd_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    /* Calc num HCs to Kill drive */
    status = fbe_api_physical_drive_get_dieh_info(pdo_id, &dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);  
    num_hc_to_kill = (fbe_u32_t)(dieh_stats.drive_stat.healthCheck_stat.interval / dieh_stats.drive_stat.healthCheck_stat.weight) + 1;

    /* Send IO to ensure HC is triggered if it hasn't already happened due to PVD metadata IO triggering it. */
    for (wait_time_ms = 0;  wait_time_ms < (service_time_limit_ms*2); wait_time_ms+=wait_interval_ms)
    {  
        status = fbe_api_rdgen_send_one_io(&rdgen_context,
                                           pvd_id,                              // object id
                                           FBE_CLASS_ID_INVALID,             // class id
                                           FBE_PACKAGE_ID_SEP_0,          // package id
                                           FBE_RDGEN_OPERATION_WRITE_ONLY,    // rdgen opcode
                                           FBE_RDGEN_PATTERN_LBA_PASS,       // data pattern to generate
                                           0x10000,                          // start lba
                                           0x1,                              // min blocks
                                           FBE_RDGEN_OPTIONS_INVALID,        // rdgen options
                                           0,0,
                                           FBE_API_RDGEN_PEER_OPTIONS_INVALID);     
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE); 
    
        status = fbe_api_physical_drive_get_dieh_info(pdo_id, &dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);                
    
        if (dieh_stats.error_counts.drive_error.healthCheck_error_tag > 0)
        {
            break;
        }

        fbe_api_sleep(wait_interval_ms);
    }

    MUT_ASSERT_INT_NOT_EQUAL_MSG((fbe_u32_t)dieh_stats.error_counts.drive_error.healthCheck_error_tag, 0, "Health Check was never hit");

    fbe_api_dest_injection_get_record(&dest_record_out, dest_record_handle);
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s Health Check hit.  error_inj:%d tag:0x%llx ===", 
               __FUNCTION__, dest_record_out.times_inserted, dieh_stats.error_counts.drive_error.healthCheck_error_tag);


    fbe_api_dest_injection_stop(); 
    fbe_api_dest_injection_remove_all_records();    

    status = fbe_api_wait_for_object_lifecycle_state(pvd_id, FBE_LIFECYCLE_STATE_READY, 10000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "PVD not Ready");    

    /* Verify periodic IO TIMEOUTs will not cause a Health Check.   This also verify the Timeout Drain/Resume path. */

    /* Setup error injection while PVD is connecting.  This will cause HC to be initiated. */
    fbe_test_dest_utils_init_error_injection_record(&dest_record);
    dest_record.object_id = pdo_id;
    dest_record.dest_error_type = FBE_DEST_ERROR_TYPE_PORT;
    dest_record.dest_error.dest_scsi_error.port_status = FBE_PORT_REQUEST_STATUS_ABORT_TIMEOUT;
    dest_record.lba_start = 0;
    dest_record.lba_end = FBE_U64_MAX; 
    dest_record.dest_error.dest_scsi_error.scsi_command[0] = FBE_SCSI_WRITE_10;    
    dest_record.dest_error.dest_scsi_error.scsi_command[1] = FBE_SCSI_WRITE_16;
    dest_record.dest_error.dest_scsi_error.scsi_command_count = 2;    
    dest_record.num_of_times_to_insert = 100;
    dest_record.frequency = 2;  /* inject 1 out N IOs */
    dest_record.delay_io_msec = 0;

    status = fbe_api_dest_injection_add_record(&dest_record, &dest_record_handle);    
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_dest_injection_start();    
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);    

    hc_tag = dieh_stats.error_counts.drive_error.healthCheck_error_tag;
    wait_interval_ms = 50;
    for (wait_time_ms = 0;  wait_time_ms < (service_time_limit_ms*3); wait_time_ms+=wait_interval_ms)
    {     
        status = fbe_api_rdgen_send_one_io(&rdgen_context,
                                           pvd_id,                          // object id
                                           FBE_CLASS_ID_INVALID,            // class id
                                           FBE_PACKAGE_ID_SEP_0,            // package id
                                           FBE_RDGEN_OPERATION_WRITE_ONLY,  // rdgen opcode
                                           FBE_RDGEN_PATTERN_LBA_PASS,      // data pattern to generate
                                           0x10000,                         // start lba
                                           0x1,                             // min blocks
                                           FBE_RDGEN_OPTIONS_INVALID,       // rdgen options
                                           0,0,
                                           FBE_API_RDGEN_PEER_OPTIONS_INVALID);    
        fbe_api_sleep(wait_interval_ms);

        status = fbe_api_physical_drive_get_dieh_info(pdo_id, &dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 
        MUT_ASSERT_UINT64_EQUAL_MSG(hc_tag, dieh_stats.error_counts.drive_error.healthCheck_error_tag, "Health Check occurred");
    }

    fbe_api_dest_injection_stop(); 
    fbe_api_dest_injection_remove_all_records();

    /* cleanup */
    biff_remove_drive(bus, encl, slot, FBE_TRUE);
    status = fbe_api_base_config_enable_all_bg_services();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s END ===", __FUNCTION__);
}


/*!****************************************************************************
 *  pinga_validate_product_ids
 ******************************************************************************
 *
 * @brief
 *  Test that each product ID is recognized
 *
 * @param   None
 *
 * @return  None
 *
 * @author
 * 2015/09/25  Wayne Garrett
 *****************************************************************************/
static void pinga_validate_product_ids(void)
{

    fbe_u32_t i;
    fbe_terminator_sas_drive_type_default_info_t new_page_info;    
    fbe_status_t status;
    fbe_object_id_t pdo;
    fbe_physical_drive_information_t drive_info = {0};
    fbe_u32_t bus=0, encl=0, slot=4;


    mut_printf(MUT_LOG_TEST_STATUS, "=== %s START ===", __FUNCTION__);

    for (i = 0; i < sizeof(validate_product_ids) / sizeof(validate_product_ids[0]); i += 1) {        
        mut_printf(MUT_LOG_TEST_STATUS, "=== %s validating id:\"%s\" tla:\"%s\" drive_type:%d, rec:%d ===", 
                   __FUNCTION__, validate_product_ids[i].emc_string, validate_product_ids[i].tla, validate_product_ids[i].pdo_drive_type, i);        

        fbe_api_terminator_sas_drive_get_default_page_info(validate_product_ids[i].terminator_drive_type, &new_page_info); 
        fbe_copy_memory(new_page_info.inquiry+FBE_SCSI_INQUIRY_PRODUCT_ID_OFFSET+8, 
                        validate_product_ids[i].emc_string, 
                        PINGA_EMC_STRING_SIZE);

        fbe_copy_memory(new_page_info.inquiry+FBE_SCSI_INQUIRY_TLA_OFFSET, 
                        validate_product_ids[i].tla, 
                        FBE_SCSI_INQUIRY_TLA_SIZE);

        biff_insert_new_drive(validate_product_ids[i].terminator_drive_type, &new_page_info, bus, encl, slot);

        if (validate_product_ids[i].is_drive_supported)
        {        
            status = fbe_test_pp_util_verify_pdo_state(bus, encl, slot, FBE_LIFECYCLE_STATE_READY, 10000 /* ms */);                                                
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);    
    
            status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &pdo);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
            status = fbe_api_physical_drive_get_drive_information(pdo, &drive_info, FBE_PACKET_FLAG_NO_ATTRIB);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
            MUT_ASSERT_BUFFER_EQUAL_MSG(drive_info.product_id+8, validate_product_ids[i].emc_string, 
                                        PINGA_EMC_STRING_SIZE, "product ID mismatch");
            MUT_ASSERT_BUFFER_EQUAL_MSG(drive_info.tla_part_number, validate_product_ids[i].tla, 
                                        FBE_SCSI_INQUIRY_TLA_SIZE, "TLA mismatch");
            MUT_ASSERT_INT_EQUAL_MSG(drive_info.drive_type, validate_product_ids[i].pdo_drive_type , "invalid drive type");
            MUT_ASSERT_INT_EQUAL_MSG(drive_info.spin_down_qualified, validate_product_ids[i].is_spindown_capable , "spindown not supported");
        }
        else
        {
            fbe_object_death_reason_t death_reason;
            const fbe_u8_t * death_reason_str = NULL;

            status = fbe_test_pp_util_verify_pdo_state(bus, encl, slot, FBE_LIFECYCLE_STATE_FAIL, 10000 /* ms */);                                                
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);  
           
            status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &pdo);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);                       
                       
            status  = fbe_api_get_object_death_reason(pdo, &death_reason, &death_reason_str, FBE_PACKAGE_ID_PHYSICAL);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);    

            MUT_ASSERT_INT_EQUAL_MSG(death_reason, FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_SET_DEV_INFO, "wrong death reason");
        }

        biff_remove_drive(bus, encl, slot, FBE_TRUE);

    }

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s END ===", __FUNCTION__);
}


static void pinga_verify_drive_type(pinga_validate_drive_type_t *drive, fbe_object_id_t pdo)
{
    fbe_status_t status;
    fbe_database_additional_drive_types_supported_t types;
    fbe_block_transport_logical_state_t logical_state;

    status = fbe_api_database_get_additional_supported_drive_types(&types);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_physical_drive_get_logical_drive_state(pdo, &logical_state);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    if ((types & FBE_DATABASE_DRIVE_TYPE_SUPPORTED_LE) == 0)
    {
        if (drive->pdo_drive_type == FBE_DRIVE_TYPE_SAS_FLASH_LE)
        {
            MUT_ASSERT_INT_EQUAL(logical_state, FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_SSD_LE);
            return;
        }
    }
    
    if ((types & FBE_DATABASE_DRIVE_TYPE_SUPPORTED_RI) == 0)
    {
        if (drive->pdo_drive_type == FBE_DRIVE_TYPE_SAS_FLASH_RI)
        {
            MUT_ASSERT_INT_EQUAL(logical_state, FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_SSD_RI);
            return;
        }
    }

    if ((types & FBE_DATABASE_DRIVE_TYPE_SUPPORTED_520_HDD) == 0)
    {
        if ((drive->pdo_drive_type == FBE_DRIVE_TYPE_SAS || drive->pdo_drive_type == FBE_DRIVE_TYPE_SAS_NL) &&
            (drive->terminator_drive_type == FBE_SAS_DRIVE_SIM_520 || drive->terminator_drive_type == FBE_SAS_NL_DRIVE_SIM_520))
        {
            MUT_ASSERT_INT_EQUAL(logical_state, FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_HDD_520);
            return;
        }
    }
           
    MUT_ASSERT_TRUE(logical_state == FBE_BLOCK_TRANSPORT_LOGICAL_STATE_ONLINE || 
                    logical_state == FBE_BLOCK_TRANSPORT_LOGICAL_STATE_UNINITALIZED);
}

/*!****************************************************************************
 *  pinga_validate_drive_types
 ******************************************************************************
 *
 * @brief
 *  Test that each drive type is recognized
 *
 * @param   None
 *
 * @return  None
 *
 * @author
 * 2015/08/25  Wayne Garrett
 *****************************************************************************/
static void pinga_validate_drive_types(void)
{

    fbe_u32_t i;
    fbe_terminator_sas_drive_type_default_info_t new_page_info;    
    fbe_status_t status;
    fbe_object_id_t pdo;
    fbe_physical_drive_information_t drive_info = {0};
    fbe_u32_t bus=0, encl=0, slot=4;

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s START ===", __FUNCTION__);
    
    for (i = 0; i < sizeof(validate_drive_types) / sizeof(validate_drive_types[0]); i += 1) {
        mut_printf(MUT_LOG_TEST_STATUS, "=== %s validating pdo drive_type %d, rec %d ===", 
                   __FUNCTION__, validate_drive_types[i].pdo_drive_type, i);        

        fbe_api_terminator_sas_drive_get_default_page_info(validate_drive_types[i].terminator_drive_type, &new_page_info); 

        if (validate_drive_types[i].writes_per_day != PINGA_IGNORE_WPD)
        {
            fbe_u8_t old_wpd = new_page_info.vpd_inquiry_c0[20];            
            new_page_info.vpd_inquiry_c0[20] = validate_drive_types[i].writes_per_day;
            mut_printf(MUT_LOG_TEST_STATUS, "=== %s changing wpd from 0x%x to 0x%x ===", __FUNCTION__, old_wpd, new_page_info.vpd_inquiry_c0[20]);
        }  
        if (validate_drive_types[i].has_sata_paddlecard)
        { 
            fbe_u8_t old_vendor_id[FBE_SCSI_INQUIRY_VENDOR_ID_SIZE+1] = {0};
            fbe_copy_memory(old_vendor_id, &new_page_info.inquiry[FBE_SCSI_INQUIRY_VENDOR_ID_OFFSET], FBE_SCSI_INQUIRY_VENDOR_ID_SIZE);
            fbe_copy_memory(&new_page_info.inquiry[FBE_SCSI_INQUIRY_VENDOR_ID_OFFSET], "SATA-MIC", FBE_SCSI_INQUIRY_VENDOR_ID_SIZE);
            mut_printf(MUT_LOG_TEST_STATUS, "=== %s changing vendor from %s to SATA-MIC ===", __FUNCTION__, old_vendor_id);
        }

        biff_insert_new_drive(validate_drive_types[i].terminator_drive_type, &new_page_info, bus, encl, slot);
        status = fbe_test_pp_util_verify_pdo_state(bus, encl, slot, FBE_LIFECYCLE_STATE_READY, 10000 /* ms */);                                                
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);    

        status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &pdo);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_physical_drive_get_drive_information(pdo, &drive_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        MUT_ASSERT_INT_EQUAL_MSG(drive_info.drive_type, validate_drive_types[i].pdo_drive_type , "invalid drive type");

        pinga_verify_drive_type(&validate_drive_types[i], pdo);

        biff_remove_drive(bus, encl, slot, FBE_TRUE);

    }

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s END ===", __FUNCTION__);
}

/*!****************************************************************************
 *  pinga_validate_all_drive_types
 ******************************************************************************
 *
 * @brief
 *  Test that each drive type is recognized
 *
 * @param   None
 *
 * @return  None
 *
 * @author
 * 2015/11/22 Jibing Dong 
 *****************************************************************************/
static void pinga_validate_all_drive_types(void)
{
    fbe_u32_t type;
    fbe_u32_t status;
    fbe_database_control_set_supported_drive_type_t set_supported_drive_type_param;
    fbe_database_additional_drive_types_supported_t supported_drive_types;

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s START ===", __FUNCTION__);

    status = fbe_api_database_get_additional_supported_drive_types(&supported_drive_types);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* 1. verify all drive types with default set. */
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s Testing supported drive types %d  ===", __FUNCTION__, supported_drive_types);
 
    pinga_validate_drive_types();

    /* 2. flip each type setting */
    type = FBE_DATABASE_DRIVE_TYPE_SUPPORTED_LE;
    for (type = FBE_DATABASE_DRIVE_TYPE_SUPPORTED_LE; type <= FBE_DATABASE_DRIVE_TYPE_SUPPORTED_520_HDD; type *= 2)
    {
        set_supported_drive_type_param.do_enable = (supported_drive_types & type)?FBE_FALSE:FBE_TRUE;
        set_supported_drive_type_param.type = type;

        status = fbe_api_database_set_additional_supported_drive_types(&set_supported_drive_type_param);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /* 3.  verify all drive types when each type flip. */
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s Testing supported drive types %d  ===", __FUNCTION__, ~supported_drive_types);

    pinga_validate_drive_types();

    /* 4. restore each type setting */
    type = FBE_DATABASE_DRIVE_TYPE_SUPPORTED_LE;
    for (type = FBE_DATABASE_DRIVE_TYPE_SUPPORTED_LE; type <= FBE_DATABASE_DRIVE_TYPE_SUPPORTED_520_HDD; type *= 2)
    {
        set_supported_drive_type_param.do_enable = (supported_drive_types & type)?FBE_TRUE:FBE_FALSE;
        set_supported_drive_type_param.type = type;

        status = fbe_api_database_set_additional_supported_drive_types(&set_supported_drive_type_param);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s END ===", __FUNCTION__);
}

/*!****************************************************************************
 *  pinga_validate_unamp_support
 ******************************************************************************
 *
 * @brief
 *  Test that the unmap is only supported for non High Endurance SSD drives
 *
 * @param   None
 *
 * @return  None
 *
 * @author
 * 2015/07/30  Wayne Garrett
 *****************************************************************************/
static void pinga_validate_unamp_support(void)
{
    fbe_u32_t bus=0, encl=0, slot=4;
    fbe_status_t status;
    fbe_object_id_t pdo;
    fbe_physical_drive_information_t drive_info = {0};
    fbe_terminator_sas_drive_type_default_info_t new_page_info, default_info;

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s START ===", __FUNCTION__);


    /* 1.  verify unmap supported on ME drive. */
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s Testing ME ===", __FUNCTION__); 

    biff_insert_new_drive(FBE_SAS_DRIVE_SIM_520_FLASH_ME, NULL, bus, encl, slot);
    biff_verify_drive_is_logically_online(bus, encl, slot);

    status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &pdo);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_physical_drive_get_drive_information(pdo, &drive_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    MUT_ASSERT_INT_EQUAL_MSG(drive_info.drive_type, FBE_DRIVE_TYPE_SAS_FLASH_ME, "invalid drive type");
    MUT_ASSERT_INT_EQUAL_MSG((drive_info.drive_parameter_flags & FBE_PDO_FLAG_SUPPORTS_WS_UNMAP), FBE_PDO_FLAG_SUPPORTS_WS_UNMAP, "WS UNMAP not supported");

    biff_remove_drive(bus, encl, slot, FBE_TRUE);


    /* 2. verify unmap NOT supported on HE drive */
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s Testing HE ===", __FUNCTION__); 

    /*first enable unmap support in vpd inquiry, then verify that PDO does not allow
      support for it*/
    fbe_api_terminator_sas_drive_get_default_page_info(FBE_SAS_DRIVE_SIM_520_FLASH_HE, &new_page_info);
    new_page_info.vpd_inquiry_b2[5] = 0xE0;   /* byte 5: unmap, ws16, ws with unmap supported*/
    biff_insert_new_drive(FBE_SAS_DRIVE_SIM_520_FLASH_HE, &new_page_info, bus, encl, slot);
    biff_verify_drive_is_logically_online(bus, encl, slot);

    status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &pdo);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_physical_drive_get_drive_information(pdo, &drive_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    MUT_ASSERT_INT_EQUAL_MSG(drive_info.drive_type, FBE_DRIVE_TYPE_SAS_FLASH_HE, "invalid drive type");
    MUT_ASSERT_INT_EQUAL_MSG((drive_info.drive_parameter_flags & (FBE_PDO_FLAG_SUPPORTS_WS_UNMAP|FBE_PDO_FLAG_SUPPORTS_UNMAP)), 0, "WS_UNMAP or UNMAP supported");

    biff_remove_drive(bus, encl, slot, FBE_TRUE);

    /* 3. verify unmap NOT support on LE drives. 
       By default LE simulated drives have unmap enabled.  Verify that PDO does allow unmap */
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s Testing LE ===", __FUNCTION__); 


    /* verify that terminator has it enabled. */
    fbe_api_terminator_sas_drive_get_default_page_info(FBE_SAS_DRIVE_SIM_520_FLASH_LE, &default_info);
    MUT_ASSERT_INT_EQUAL_MSG(default_info.vpd_inquiry_b2[5], 0xE0, "LE ws_unmap and unmap bits not set in terminator defaults");

    biff_insert_new_drive(FBE_SAS_DRIVE_SIM_520_FLASH_LE, NULL, bus, encl, slot);
    status = fbe_test_pp_util_verify_pdo_state(bus, encl, slot, FBE_LIFECYCLE_STATE_READY, 10000 /* ms */);                                                
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &pdo);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_physical_drive_get_drive_information(pdo, &drive_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    MUT_ASSERT_INT_EQUAL_MSG(drive_info.drive_type, FBE_DRIVE_TYPE_SAS_FLASH_LE, "invalid drive type");
    MUT_ASSERT_INT_EQUAL_MSG((drive_info.drive_parameter_flags & (FBE_PDO_FLAG_SUPPORTS_WS_UNMAP|FBE_PDO_FLAG_SUPPORTS_UNMAP)), 0, "WS_UNMAP or UNMAP supported");

    biff_remove_drive(bus, encl, slot, FBE_TRUE);


    /* 4. verify unmap NOT support on RI drives. 
       By default RI simulated drives have unmap enabled.  Verify that PDO does allow unmap */
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s Testing RI ===", __FUNCTION__); 

    /* verify that terminator has it enabled. */
    fbe_api_terminator_sas_drive_get_default_page_info(FBE_SAS_DRIVE_SIM_520_FLASH_RI, &default_info);
    MUT_ASSERT_INT_EQUAL_MSG(default_info.vpd_inquiry_b2[5], 0xE0, "RI ws_unmap and unmap bits not set in terminator defaults");

    biff_insert_new_drive(FBE_SAS_DRIVE_SIM_520_FLASH_RI, NULL, bus, encl, slot);
    status = fbe_test_pp_util_verify_pdo_state(bus, encl, slot, FBE_LIFECYCLE_STATE_READY, 10000 /* ms */);                                                
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);    

    status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &pdo);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_physical_drive_get_drive_information(pdo, &drive_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    MUT_ASSERT_INT_EQUAL_MSG(drive_info.drive_type, FBE_DRIVE_TYPE_SAS_FLASH_RI, "invalid drive type");
    MUT_ASSERT_INT_EQUAL_MSG((drive_info.drive_parameter_flags & (FBE_PDO_FLAG_SUPPORTS_WS_UNMAP|FBE_PDO_FLAG_SUPPORTS_UNMAP)), 0, "WS_UNMAP or UNMAP supported");

    biff_remove_drive(bus, encl, slot, FBE_TRUE);



}


/*!****************************************************************************
 *  pinga_validate_unamp_cmd
 ******************************************************************************
 *
 * @brief
 *  Test that the unmap command works correctly.
 *  1.  Unmap an unmapped block.
 *  2.  Read around edges of unmapped area to make sure data is not unmapped.
 *  3.  Write in middle of unmapped area and verify data can be read.
 *
 * @param   None
 *
 * @return  None
 *
 * @author
 * 2015/05/11  Wayne Garrett
 *****************************************************************************/
static void pinga_validate_unamp_cmd(void)
{
    fbe_u32_t bus=0, encl=0, slot=4;
    fbe_status_t status;
    fbe_object_id_t pdo;
    fbe_physical_drive_send_pass_thru_t write_same_10_unmap = {0};
    fbe_physical_drive_send_pass_thru_t write_same_16_unmap = {0};
    fbe_physical_drive_send_pass_thru_t *passthru_p;
    fbe_u8_t write_same_buff[520] = {0};
    fbe_api_rdgen_context_t rdgen_context = {0};    

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s START ===", __FUNCTION__);


    biff_insert_new_drive(FBE_SAS_DRIVE_SIM_520_FLASH_HE, NULL, bus, encl, slot);
    biff_verify_drive_is_logically_online(bus, encl, slot);


    pinga_disable_background_services(bus, encl, slot);

    status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &pdo);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    pinga_build_write_same_10_unmap(&write_same_10_unmap, write_same_buff, 520 /*bufflen*/, 0x10000 /*lba*/, 0x800 /*block_count*/);
    pinga_build_write_same_16_unmap(&write_same_16_unmap, write_same_buff, 520 /*bufflen*/, 0x10400 /*lba*/, 0x800 /*block_count*/);


    mut_printf(MUT_LOG_TEST_STATUS, "=== %s sending unmap ===", __FUNCTION__);

    /* send WS 10 */
    passthru_p = &write_same_10_unmap;
    status = fbe_api_physical_drive_send_pass_thru(pdo, passthru_p);
    if (status != FBE_STATUS_OK || 
        passthru_p->cmd.port_request_status != FBE_PORT_REQUEST_STATUS_SUCCESS || 
        passthru_p->cmd.payload_cdb_scsi_status != FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD) 
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s pass_thru cmd failed. return stat:%d, port stat:%d, scsi stat:0x%x\n", 
                   __FUNCTION__, status, passthru_p->cmd.port_request_status, passthru_p->cmd.payload_cdb_scsi_status);
        MUT_FAIL_MSG("pass_thru failed");
    }

    /* send WS 16 */
    passthru_p = &write_same_16_unmap;
    status = fbe_api_physical_drive_send_pass_thru(pdo, passthru_p);
    if (status != FBE_STATUS_OK || 
        passthru_p->cmd.port_request_status != FBE_PORT_REQUEST_STATUS_SUCCESS || 
        passthru_p->cmd.payload_cdb_scsi_status != FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD) 
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s pass_thru cmd failed. return stat:%d, port stat:%d, scsi stat:0x%x\n", 
                   __FUNCTION__, status, passthru_p->cmd.port_request_status, passthru_p->cmd.payload_cdb_scsi_status);
        MUT_FAIL_MSG("pass_thru failed");
    }


    mut_printf(MUT_LOG_TEST_STATUS, "=== %s verifying unmap ===", __FUNCTION__);


    /* If you attempt to read an unmapped area, terminator will error and SP stops.  Verify we can read
       just outside the edges */

    status = fbe_api_rdgen_send_one_io(&rdgen_context,
                                       pdo,                              // object id
                                       FBE_CLASS_ID_INVALID,             // class id
                                       FBE_PACKAGE_ID_PHYSICAL,          // package id
                                       FBE_RDGEN_OPERATION_READ_ONLY,    // rdgen opcode
                                       FBE_RDGEN_PATTERN_LBA_PASS,       // data pattern to generate
                                       0xFFFF,                           // start lba
                                       0x1,                              // min blocks
                                       FBE_RDGEN_OPTIONS_INVALID,        // rdgen options
                                       0,0,
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID);     
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 

    status = fbe_api_rdgen_send_one_io(&rdgen_context,
                                       pdo,                              // object id
                                       FBE_CLASS_ID_INVALID,             // class id
                                       FBE_PACKAGE_ID_PHYSICAL,          // package id
                                       FBE_RDGEN_OPERATION_READ_ONLY,    // rdgen opcode
                                       FBE_RDGEN_PATTERN_LBA_PASS,       // data pattern to generate
                                       0x10C00,                          // start lba
                                       0x1,                              // min blocks
                                       FBE_RDGEN_OPTIONS_INVALID,        // rdgen options
                                       0,0,
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID);     
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 
        

    /* write a block in middle of unmapped region and verify we can read this. */
    status = fbe_api_rdgen_send_one_io(&rdgen_context,
                                       pdo,                              // object id
                                       FBE_CLASS_ID_INVALID,             // class id
                                       FBE_PACKAGE_ID_PHYSICAL,          // package id
                                       FBE_RDGEN_OPERATION_WRITE_ONLY,    // rdgen opcode
                                       FBE_RDGEN_PATTERN_LBA_PASS,       // data pattern to generate
                                       0x10700,                          // start lba
                                       0x1,                              // min blocks
                                       FBE_RDGEN_OPTIONS_INVALID,        // rdgen options
                                       0,0,
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID);     
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 

    status = fbe_api_rdgen_send_one_io(&rdgen_context,
                                       pdo,                              // object id
                                       FBE_CLASS_ID_INVALID,             // class id
                                       FBE_PACKAGE_ID_PHYSICAL,          // package id
                                       FBE_RDGEN_OPERATION_READ_ONLY,    // rdgen opcode
                                       FBE_RDGEN_PATTERN_LBA_PASS,       // data pattern to generate
                                       0x10700,                          // start lba
                                       0x1,                              // min blocks
                                       FBE_RDGEN_OPTIONS_INVALID,        // rdgen options
                                       0,0,
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID);     
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    /* cleanup */
    pinga_enable_background_services(bus, encl, slot);
    biff_remove_drive(bus, encl, slot, FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s finished ===", __FUNCTION__);
}

static void pinga_build_write_same_10_unmap(fbe_physical_drive_send_pass_thru_t *write_same_10_unmap_p, fbe_u8_t *data_out_buff, fbe_u32_t bufflen, fbe_lba_t lba, fbe_block_count_t block_count)
{
    pinga_build_10_byte_cdb(&write_same_10_unmap_p->cmd, lba, block_count);
    write_same_10_unmap_p->cmd.cdb[0] = FBE_SCSI_WRITE_SAME_10;  
    write_same_10_unmap_p->cmd.cdb[1] = 0x08;  /* unmap bit */
    write_same_10_unmap_p->cmd.payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_OUT;
    write_same_10_unmap_p->cmd.transfer_count = bufflen;
    write_same_10_unmap_p->response_buf = data_out_buff;    
    write_same_10_unmap_p->status = FBE_STATUS_PENDING;
}

static void pinga_build_write_same_16_unmap(fbe_physical_drive_send_pass_thru_t *write_same_16_unmap_p, fbe_u8_t *data_out_buff, fbe_u32_t bufflen, fbe_lba_t lba, fbe_block_count_t block_count)
{
    pinga_build_16_byte_cdb(&write_same_16_unmap_p->cmd, lba, block_count);
    write_same_16_unmap_p->cmd.cdb[0] = FBE_SCSI_WRITE_SAME_16;  
    write_same_16_unmap_p->cmd.cdb[1] = 0x08;  /* unmap bit */
    write_same_16_unmap_p->cmd.payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_OUT;
    write_same_16_unmap_p->cmd.transfer_count = bufflen;
    write_same_16_unmap_p->response_buf = data_out_buff;    
    write_same_16_unmap_p->status = FBE_STATUS_PENDING;
}


static void pinga_build_10_byte_cdb(fbe_physical_drive_mgmt_send_pass_thru_cdb_t* cmd,
                                    fbe_lba_t lba,
                                    fbe_block_count_t block_count)
{
    /* Set common parameters */    
    cmd->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cmd->cdb_length = 10;  
    
    /* Set the lba and blockcount */   
    cmd->cdb[2] = (fbe_u8_t)((lba >> 24) & 0xFF); /* MSB */
    cmd->cdb[3] = (fbe_u8_t)((lba >> 16) & 0xFF); 
    cmd->cdb[4] = (fbe_u8_t)((lba >> 8) & 0xFF);
    cmd->cdb[5] = (fbe_u8_t)(lba & 0xFF); /* LSB */
    cmd->cdb[6] = 0;
    cmd->cdb[7] = (fbe_u8_t)((block_count >> 8) & 0xFF); /* MSB */
    cmd->cdb[8] = (fbe_u8_t)(block_count & 0xFF); /* LSB */
    cmd->cdb[9] = 0;    
}


static void pinga_build_16_byte_cdb(fbe_physical_drive_mgmt_send_pass_thru_cdb_t* cmd,
                                    fbe_lba_t lba,
                                    fbe_block_count_t block_count)
{
    /* Set common parameters */    
    cmd->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cmd->cdb_length = 16;      
    
    /* Set the lba and blockcount */       
    cmd->cdb[2] = (fbe_u8_t)((lba >> 56) & 0xFF); /* MSB */
    cmd->cdb[3] = (fbe_u8_t)((lba >> 48) & 0xFF); 
    cmd->cdb[4] = (fbe_u8_t)((lba >> 40) & 0xFF);
    cmd->cdb[5] = (fbe_u8_t)((lba >> 32) & 0xFF); 
    cmd->cdb[6] = (fbe_u8_t)((lba >> 24) & 0xFF); 
    cmd->cdb[7] = (fbe_u8_t)((lba >> 16) & 0xFF); 
    cmd->cdb[8] = (fbe_u8_t)((lba >> 8) & 0xFF); 
    cmd->cdb[9] = (fbe_u8_t)(lba & 0xFF); /* LSB */
    cmd->cdb[10] = (fbe_u8_t)((block_count >> 24) & 0xFF); /* MSB */
    cmd->cdb[11] = (fbe_u8_t)((block_count >> 16) & 0xFF); 
    cmd->cdb[12] = (fbe_u8_t)((block_count >> 8) & 0xFF);
    cmd->cdb[13] = (fbe_u8_t)(block_count & 0xFF); /* LSB */       
    cmd->cdb[14] = 0;
    cmd->cdb[15] = 0;
}

void pinga_disable_background_services(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot)
{
    fbe_status_t status;
    fbe_object_id_t pvd;

    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(bus, encl, slot, &pvd);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_provision_drive_disable_zeroing(pvd);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_provision_drive_disable_verify (pvd);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

void pinga_enable_background_services(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot)
{
    fbe_status_t status;
    fbe_object_id_t pvd;

    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(bus, encl, slot, &pvd);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_provision_drive_enable_zeroing(pvd);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_provision_drive_enable_verify(pvd);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
