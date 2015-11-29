/***************************************************************************
 * Copyright (C) EMC Corporation 2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file jigglypuff_new_drive_test.c
 ***************************************************************************
 *
 * @brief
 *   This file validates new physical drive and clone from pinga test.
 *
 * @version
 *
 ***************************************************************************/


/*-----------------------------------------------------------------------------
    INCLUDES OF REQUIRED HEADER FILES:
*/
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
#include "fbe/fbe_api_board_interface.h"

typedef struct jigglypuff_validate_info_s {
    fbe_sas_drive_type_t        drive_type;
    fbe_block_size_t            block_size;
    fbe_physical_drive_speed_t  speed_capability;
    fbe_drive_type_t            phys_drive_type;
    fbe_u32_t                   drive_writes_per_day;
} jigglypuff_validate_info_t;

/*-----------------------------------------------------------------------------
    TEST DESCRIPTION:
*/

char * jigglypuff_drive_validation_short_desc = "Physical Endurance Drives Validation";
char * jigglypuff_drive_validation_long_desc = 
    "Validates - Reports correct drive type on different drives\n"
    "Validates - Reports correct drive writes per day on different drives\n"
    "Validates - Datbase works on very slow drives\n"
    ;


/*-----------------------------------------------------------------------------
    DEFINITIONS:
*/


#define GET_SAS_DRIVE_SAS_ADDRESS(BN, EN, SN) \
    (0x5000097B80000000 + ((fbe_u64_t)(BN) << 16) + ((fbe_u64_t)(EN) << 8) + (SN))
#define GET_SATA_DRIVE_SAS_ADDRESS(BN, EN, SN) \
    (0x5000097A80000000 + ((fbe_u64_t)(BN) << 16) + ((fbe_u64_t)(EN) << 8) + (SN))
#define GET_SAS_ENCLOSURE_SAS_ADDRESS(BN, EN) \
    (0x5000097A79000000 + ((fbe_u64_t)(EN) << 16) + BN)
#define GET_SAS_PMC_PORT_SAS_ADDRESS(BN) \
    (0x5000097A7800903F + (BN))

static jigglypuff_validate_info_t validate_drives[] = {
    {
        FBE_SAS_DRIVE_SIM_520,          // Terminator SAS Drive Type
        FBE_BLOCK_SIZES_520,            // Block size
        FBE_DRIVE_SPEED_6GB,            // Speed Capacity
        FBE_DRIVE_TYPE_SAS,             // Physical Drive Type
        0                               // Drive WPD
    },
    {
        FBE_SAS_DRIVE_CHEETA_15K,       // Terminator SAS Drive Type
        FBE_BLOCK_SIZES_520,            // Block size
        FBE_DRIVE_SPEED_6GB,            // Speed Capacity
        FBE_DRIVE_TYPE_SAS,             // Physical Drive Type
        0                               // Drive WPD
    },
    {
        FBE_SAS_DRIVE_SIM_520_FLASH_HE, // Terminator SAS Drive Type
        FBE_BLOCK_SIZES_520,            // Block size
        FBE_DRIVE_SPEED_6GB,            // Speed Capacity
        FBE_DRIVE_TYPE_SAS_FLASH_HE,    // Physical Drive Type
        25                              // Drive WPD
    },
    {
        FBE_SAS_DRIVE_SIM_520_12G,      // Terminator SAS Drive Type
        FBE_BLOCK_SIZES_520,            // Block size
        FBE_DRIVE_SPEED_12GB,           // Speed Capacity
        FBE_DRIVE_TYPE_SAS,             // Physical Drive Type
        0                               // Drive WPD
    },
    {
        FBE_SAS_DRIVE_SIM_520_12G,      // Terminator SAS Drive Type
        FBE_BLOCK_SIZES_4160,           // Block size
        FBE_DRIVE_SPEED_12GB,           // Speed Capacity
        FBE_DRIVE_TYPE_SAS,             // Physical Drive Type
        0                               // Drive WPD
    },
    {
        FBE_SAS_DRIVE_SIM_520_FLASH_ME, // Terminator SAS Drive Type
        FBE_BLOCK_SIZES_520,            // Block size
        FBE_DRIVE_SPEED_6GB,            // Speed Capacity
        FBE_DRIVE_TYPE_SAS_FLASH_ME,    // Physical Drive Type
        10                              // Drive WPD
    },
    {
        FBE_SAS_DRIVE_SIM_520_FLASH_LE, // Terminator SAS Drive Type
        FBE_BLOCK_SIZES_520,            // Block size
        FBE_DRIVE_SPEED_6GB,            // Speed Capacity
        FBE_DRIVE_TYPE_SAS_FLASH_LE,    // Physical Drive Type
        3                               // Drive WPD
    },
    {
        FBE_SAS_DRIVE_SIM_520_FLASH_RI, // Terminator SAS Drive Type
        FBE_BLOCK_SIZES_520,            // Block size
        FBE_DRIVE_SPEED_6GB,            // Speed Capacity
        FBE_DRIVE_TYPE_SAS_FLASH_RI,    // Physical Drive Type
        1                               // Drive WPD
    },
};


/*-----------------------------------------------------------------------------
     GLOBALS 
*/

/*-----------------------------------------------------------------------------
    EXTERN:
*/
extern fbe_status_t fbe_test_get_sas_drive_info_extend(fbe_terminator_sas_drive_info_t *sas_drive_p,
                                                       fbe_u32_t backend_number,
                                                       fbe_u32_t encl_number,
                                                       fbe_u32_t slot_number,
                                                       fbe_block_size_t block_size,
                                                       fbe_lba_t required_capacity,
                                                       fbe_sas_address_t sas_address,
                                                       fbe_sas_drive_type_t drive_type);

/*-----------------------------------------------------------------------------
    FORWARD DECLARATIONS:
*/
static char * jigglypuff_convert_drive_type_enum_to_string(fbe_drive_type_t drive_type);
static void jigglypuff_insert_new_drive(fbe_u32_t port_number, fbe_u32_t enclosure_number,
                             fbe_u32_t slot_number, fbe_block_size_t block_size, 
                             fbe_lba_t capacity, fbe_sas_drive_type_t sas_drive_type);
static void jigglypuff_insert_drive(jigglypuff_validate_info_t *info,
                               fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_lba_t capacity,
                               fbe_api_terminator_device_handle_t *drive_handle_p);
static void jigglypuff_validate_drive_type(void);
static void jigglypuff_validate_drive_writes_per_day(void);
static void jigglypuff_validate_database_on_slow_drive(void);

/*-----------------------------------------------------------------------------
    FUNCTIONS:
*/

/*!****************************************************************************
 * jigglypuff_drive_validation_test()
 ******************************************************************************
 * @brief
 *  This is the main entry point for the drive validation test   
 *
 * @param None.
 *
 * @return None.
 *
 ******************************************************************************/
void jigglypuff_drive_validation_test(void)
{    

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s START ===", __FUNCTION__); 

    /* Validate drive type */
    jigglypuff_validate_drive_type();

    /* Validate write per day based on drive type */
    jigglypuff_validate_drive_writes_per_day();

    /* Test Slow drive */
    jigglypuff_validate_database_on_slow_drive();

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s END ===", __FUNCTION__); 

    return; 

} /* end of jigglypuff_drive_validation_test() */


/*!****************************************************************************
 * jigglypuff_drive_create_physical_config_for_disk_counts()
 ******************************************************************************
 * @brief
 *  Create configuration based on the number of disks for 520 and/or 4160 types.
 *  It will create the DB disks as Low Endurance Drives.
 *
 * @param None.
 *
 * @return None.
 *
 ******************************************************************************/
void jigglypuff_drive_create_physical_config_for_disk_counts(fbe_u32_t num_520_drives,
                                                             fbe_u32_t num_4160_drives,
                                                             fbe_block_count_t drive_capacity)
{
    fbe_status_t                        status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                           total_objects = 0;
    fbe_class_id_t                      class_id;
    fbe_api_terminator_device_handle_t  port_handle;
    fbe_api_terminator_device_handle_t  encl_handle;
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_u32_t slot;
    fbe_u32_t num_objects = 1; /* start with board */
    fbe_u32_t encl_number = 0;
    fbe_u32_t encl_index;
    fbe_u32_t num_520_enclosures;
    fbe_u32_t num_4160_enclosures;
    fbe_u32_t num_first_enclosure_drives;

    num_520_enclosures = (num_520_drives / FBE_TEST_DRIVES_PER_ENCL) + ((num_520_drives % FBE_TEST_DRIVES_PER_ENCL) ? 1 : 0);
    num_4160_enclosures = (num_4160_drives / FBE_TEST_DRIVES_PER_ENCL) + ((num_4160_drives % FBE_TEST_DRIVES_PER_ENCL) ? 1 : 0);

    /* Before loading the physical package we initialize the terminator.
     */
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* insert a board
     */
    status = fbe_test_pp_util_insert_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    num_first_enclosure_drives = fbe_private_space_layout_get_number_of_system_drives();

    /* insert a port
     */
    fbe_test_pp_util_insert_sas_pmc_port(1, /* io port */
                                 2, /* portal */
                                 0, /* backend number */ 
                                 &port_handle);

    /* We start off with:
     *  1 board
     *  1 port
     *  1 enclosure
     *  plus one pdo for each of the first 10 drives.
     */
    num_objects = 3 +  num_first_enclosure_drives; 

    /* Next, add on all the enclosures and drives we will add.
     */
    num_objects += num_520_enclosures + num_520_drives;
    num_objects += num_4160_enclosures + num_4160_drives;

    /* First enclosure has 4 drives for system rgs.
     */
    fbe_test_pp_util_insert_viper_enclosure(0, encl_number, port_handle, &encl_handle);
    for (slot = 0; slot < num_first_enclosure_drives; slot++)
    {
        /* Insert LE drives as System Drives */
        status = fbe_test_pp_util_insert_sas_drive_extend(0, encl_number, slot,
                                                          520, TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY + drive_capacity,
                                                          GET_SAS_DRIVE_SAS_ADDRESS(0, encl_number, slot), FBE_DRIVE_TYPE_SAS_FLASH_LE,
                                                          &drive_handle);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    }

    /* We inserted one enclosure above, so we are starting with encl #1.
     */
    encl_number = 1;

    /* Insert enclosures and drives for 520.  Start at encl number one.
     */
    for ( encl_index = 0; encl_index < num_520_enclosures; encl_index++)
    {
        fbe_test_pp_util_insert_viper_enclosure(0, encl_number, port_handle, &encl_handle);

        slot = 0;
        while ((slot < FBE_TEST_DRIVES_PER_ENCL) && num_520_drives)
        {
            fbe_test_pp_util_insert_sas_drive(0, encl_number, slot, 520, TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY + drive_capacity, &drive_handle);
            num_520_drives--;
            slot++;
        }
        encl_number++;
    }
    /* Insert enclosures and drives for 512.
     * We pick up the enclosure number from the prior loop. 
     */
    for ( encl_index = 0; encl_index < num_4160_enclosures; encl_index++)
    {
        fbe_test_pp_util_insert_viper_enclosure(0, encl_number, port_handle, &encl_handle);
        slot = 0;
        while ((slot < FBE_TEST_DRIVES_PER_ENCL) && num_4160_drives)
        {
            fbe_test_pp_util_insert_sas_drive(0, encl_number, slot, 4160, drive_capacity, &drive_handle);

            num_4160_drives--;
            slot++;
        }
        encl_number++;
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
    status = fbe_api_wait_for_number_of_objects(num_objects, 60000, FBE_PACKAGE_ID_PHYSICAL);
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
    MUT_ASSERT_TRUE(total_objects == num_objects);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");

    if (fbe_test_should_use_sim_psl()) {
        mut_printf(MUT_LOG_TEST_STATUS, "Use FBE Simulator PSL.");
        fbe_api_board_sim_set_psl(0);
    }
    return;
}

/*!****************************************************************************
 *  jigglypuff_drive_validation_setup
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
 *****************************************************************************/
void jigglypuff_drive_validation_setup(void)
{

    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for jigglypuff_slot_limit test ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");

    if (fbe_test_util_is_simulation())
    {
        jigglypuff_drive_create_physical_config_for_disk_counts(0, 0, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY);

        elmo_load_sep_and_neit();
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();

    return;

} /* end of jigglypuff_drive_validation_setup() */


/*!****************************************************************************
 *  jigglypuff_drive_validation_cleanup
 ******************************************************************************
 *
 * @brief
 *  This is the cleanup function 
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void jigglypuff_drive_validation_cleanup(void)
{  
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s ===\n", __FUNCTION__);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
} /* end of jigglypuff_drive_validation_cleanup() */

/*!***************************************************************************
 * jigglypuff_convert_drive_type_enum_to_string()
 ******************************************************************************
 * @brief
 *  Return a string for drive type.
 *
 * @param drive_type - Drive Type for the object.
 *
 * @return - char * string that represents drive type.
 *
 ****************************************************************/
static char * jigglypuff_convert_drive_type_enum_to_string(fbe_drive_type_t drive_type)
{
    /* Drive Class is an enumerated type - print raw and interpreted value */
    switch(drive_type)
    {
        case FBE_DRIVE_TYPE_FIBRE:
            return("FIBRE");

         case FBE_DRIVE_TYPE_SAS:
            return("SAS");

         case FBE_DRIVE_TYPE_SATA:
            return("SATA");

         case FBE_DRIVE_TYPE_SAS_FLASH_HE:
            return ("SAS FLASH HE");

         case FBE_DRIVE_TYPE_SATA_FLASH_HE:
            return ("SATA FLASH HE");

        case FBE_DRIVE_TYPE_SAS_FLASH_ME:
            return ("SAS FLASH ME");

        case FBE_DRIVE_TYPE_SAS_FLASH_LE:
            return ("SAS FLASH LE");

        case FBE_DRIVE_TYPE_SAS_FLASH_RI:
            return ("SAS FLASH RI");

        case FBE_DRIVE_TYPE_SAS_NL:
            return ("SAS NL");

        default:
            break;
    }

    /* For default case */
    return("UNKNOWN");

} /* end of jigglypuff_convert_drive_type_enum_to_string() */

/*!****************************************************************************
 *  jigglypuff_insert_drive
 ******************************************************************************
 *
 * @brief
 *  Insert a drive.
 *
 *
 * @return  None
 *
 *****************************************************************************/
static void jigglypuff_insert_drive(jigglypuff_validate_info_t *info,
                               fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_lba_t capacity,
                               fbe_api_terminator_device_handle_t *drive_handle_p)
{
    fbe_status_t status;
    fbe_terminator_sas_drive_info_t sas_drive;
    fbe_api_terminator_device_handle_t encl_handle;
    fbe_sas_address_t new_address = GET_SAS_DRIVE_SAS_ADDRESS(bus, encl, slot);

    status = fbe_api_terminator_get_enclosure_handle(bus, encl, &encl_handle);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    if((encl == 0) && (slot < 4) && (capacity < TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY))
    {
        capacity = TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY + TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY;
        mut_printf(MUT_LOG_LOW, "%s: Increased drive capacity! ===\n", __FUNCTION__);
    }

    status = fbe_test_get_sas_drive_info_extend(&sas_drive, bus, encl, slot, info->block_size, capacity, new_address, info->drive_type);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "%s: %d_%d_%d serial: %s, sas_drive_type: %d, blksize: %d inserted.",
               __FUNCTION__, bus, encl, slot, sas_drive.drive_serial_number, sas_drive.drive_type, sas_drive.block_size);

    status  = fbe_api_terminator_insert_sas_drive(encl_handle, slot, &sas_drive, drive_handle_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;

} /* end of jigglypuff_insert_drive() */

/*!****************************************************************************
 *  jigglypuff_validate_drive_type
 ******************************************************************************
 *
 * @brief
 *  Test PDO can recongize different drive type.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
static void jigglypuff_validate_drive_type(void)
{
    fbe_u32_t slot = 5;
    fbe_u32_t i;
    fbe_status_t status;


    mut_printf(MUT_LOG_TEST_STATUS, "=== %s START ===", __FUNCTION__);

    for (i = 0; i < sizeof(validate_drives) / sizeof(validate_drives[0]); i += 1) {
        jigglypuff_validate_info_t *drive = &validate_drives[i];
        fbe_u32_t object_handle = 0;
        fbe_physical_drive_information_t get_drive_information;
        fbe_api_terminator_device_handle_t  drive_handle;

        jigglypuff_insert_drive(drive, 0, 0, slot, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY, &drive_handle);
        status = fbe_test_pp_util_verify_pdo_state(0, 0, slot, FBE_LIFECYCLE_STATE_READY, 10000 /*timeout*/);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_get_physical_drive_object_id_by_location(0, 0, slot, &object_handle);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);
        status = fbe_api_physical_drive_get_drive_information(object_handle, &get_drive_information, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        mut_printf(MUT_LOG_TEST_STATUS, "...%d_%d_%d: phys drive type: %s(%d)...\n",
                   0, 0, slot, jigglypuff_convert_drive_type_enum_to_string(get_drive_information.drive_type),
                   get_drive_information.drive_type);
        MUT_ASSERT_TRUE(get_drive_information.drive_type == drive->phys_drive_type);

        status = fbe_test_pp_util_pull_drive(0, 0, slot, &drive_handle);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_wait_till_object_is_destroyed(object_handle, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s END: PASSED ===\n", __FUNCTION__);

    return;
} /* end of jigglypuff_validate_drive_type() */

/*!****************************************************************************
 *  jigglypuff_validate_drive_writes_per_day
 ******************************************************************************
 *
 * @brief
 *  Test PDO can recongize different writes per day.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
static void jigglypuff_validate_drive_writes_per_day(void)
{
    fbe_u32_t slot = 5;
    fbe_u32_t i;
    fbe_status_t status;


    mut_printf(MUT_LOG_TEST_STATUS, "=== %s START ===", __FUNCTION__);

    for (i = 0; i < sizeof(validate_drives) / sizeof(validate_drives[0]); i += 1) {
        jigglypuff_validate_info_t *drive = &validate_drives[i];
        fbe_u32_t object_handle = 0;
        fbe_physical_drive_information_t get_drive_information;
        fbe_api_terminator_device_handle_t  drive_handle;
        fbe_u32_t drive_writes_per_day = 0;

        jigglypuff_insert_drive(drive, 0, 0, slot, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY, &drive_handle);
        status = fbe_test_pp_util_verify_pdo_state(0, 0, slot, FBE_LIFECYCLE_STATE_READY, 10000 /*timeout*/);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_get_physical_drive_object_id_by_location(0, 0, slot, &object_handle);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);
        status = fbe_api_physical_drive_get_drive_information(object_handle, &get_drive_information, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        drive_writes_per_day = (fbe_u32_t)(get_drive_information.drive_parameter_flags & FBE_PDO_FLAG_FLASH_DRIVE_WRITES_PER_DAY);                   

        mut_printf(MUT_LOG_TEST_STATUS, "...%d_%d_%d: phys drive_type: %s(%d), drive_param: 0x%x, wpd: 0x%x",
                   0, 0, slot, jigglypuff_convert_drive_type_enum_to_string(get_drive_information.drive_type),
                   get_drive_information.drive_type, get_drive_information.drive_parameter_flags,
                   drive_writes_per_day);
        MUT_ASSERT_TRUE(drive_writes_per_day == drive->drive_writes_per_day);

        /* remove drive when test is done */
        status = fbe_test_pp_util_pull_drive(0, 0, slot, &drive_handle);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        status = fbe_api_wait_till_object_is_destroyed(object_handle, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s END: PASSED ===\n", __FUNCTION__);

    return;

} /* end of jigglypuff_validate_drive_writes_per_day() */

/*!****************************************************************************
 * jigglypuff_insert_new_drive()
 ******************************************************************************
 * @brief
 *  Insert new drive  
 *
 * @param port_number - Port number
 *        enclosure_number - enclosure number
 *        slot_number - slot number
 *        block_size - Block size
 *        sas_drive_type - Drive type 
 *
 * @return None.
 *
 ******************************************************************************/
static void jigglypuff_insert_new_drive(fbe_u32_t port_number, fbe_u32_t enclosure_number,
                             fbe_u32_t slot_number, fbe_block_size_t block_size,
                             fbe_lba_t capacity, fbe_sas_drive_type_t sas_drive_type)
{
    fbe_status_t    status;
    fbe_sas_address_t   blank_new_sas_address;
    fbe_api_terminator_device_handle_t handle;

    blank_new_sas_address = fbe_test_pp_util_get_unique_sas_address();
    status = fbe_test_pp_util_insert_sas_drive_extend(port_number, enclosure_number, slot_number,
                                                      block_size, capacity,
                                                      blank_new_sas_address, sas_drive_type,
                                                      &handle);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;

} /* end of jigglypuff_insert_new_drive() */

/*!****************************************************************************
 * wait_for_database_transaction()
 ******************************************************************************
 * @brief
 *  Wait for DB transaction 
 *
 * @param none
 *
 * @return None.
 *
 ******************************************************************************/
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

    return;

} /* end of wait_for_database_transaction() */

/*!****************************************************************************
 *  jigglypuff_validate_database_on_slow_drive
 ******************************************************************************
 *
 * @brief
 *  Test Database work as expected on very slow system drive (Bad drives?
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
static void jigglypuff_validate_database_on_slow_drive(void)
{
    fbe_status_t status;
    fbe_database_control_set_supported_drive_type_t drive_type;
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

    /* enable LE support */
    drive_type.do_enable = FBE_TRUE;
    drive_type.type = FBE_DATABASE_DRIVE_TYPE_SUPPORTED_LE;
    status = fbe_api_database_set_additional_supported_drive_types(&drive_type);

    status = fbe_api_lun_get_lun_info(lun_obj, &lun_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Inject delay error ===", __FUNCTION__);
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


    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Insert new drive %u_%u_%u ===",
               __FUNCTION__, disk_set.bus, disk_set.enclosure, disk_set.slot);

    jigglypuff_insert_new_drive(disk_set.bus, disk_set.enclosure, disk_set.slot, FBE_BLOCK_SIZES_520, 
                                TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY,FBE_SAS_DRIVE_SIM_520_FLASH_LE);

    status = fbe_test_pp_util_verify_pdo_state(disk_set.bus, disk_set.enclosure, disk_set.slot,
                                               FBE_LIFECYCLE_STATE_READY, 10000 /*timeout*/);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_test_sep_util_wait_for_pvd_creation(&disk_set, 1, 120000);

    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(disk_set.bus, disk_set.enclosure, disk_set.slot, &pvd_obj);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Wait for persist service to finish */
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Start waiting for persist service to finish ===", __FUNCTION__);
    wait_for_database_transaction();
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: End waiting for persist service to finish ===", __FUNCTION__);

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

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s END: PASSED ===", __FUNCTION__);

    return;

} /* end of jigglypuff_validate_database_on_slow_drive() */

