#include "mut.h"
#include "fbe_test_package_config.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"

#include "sep_tests.h"
#include "fbe_test_configurations.h"
#include "pp_utils.h"

void kilgore_trout_config_init(void)
{
    /* 1 board, 1 port, 1 enclosure, 8 PDOs       */
    /* 3 PDO/LDOs are for the test 3 are for the system drive */
#define NUMBER_OF_OBJECTS 11

    fbe_api_terminator_device_handle_t  port_handle;
    fbe_api_terminator_device_handle_t  encl_handle;
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_u32_t i;
    fbe_u32_t num_drives = 8;
    fbe_u32_t total_objects = 0;
    fbe_class_id_t class_id;

    fbe_status_t status;

    mut_printf(MUT_LOG_LOW, "=== init kilgore_trout configuration ===\n");

    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "init terminator api failed");

    /* Insert a board. */
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "init fleet board failed");

    /* Insert port (0). */
    fbe_test_pp_util_insert_sas_pmc_port(1, /* io port */
            2, /* portal */
            0, /* backend number */ 
            &port_handle);

    /* Insert an enclosure (0,0) . */
    fbe_test_pp_util_insert_viper_enclosure(0, 0, port_handle, &encl_handle);

    for (i = 0; i < num_drives; i++)
    {
        /* Insert a SAS drive (0,0,i). */
        fbe_test_pp_util_insert_sas_drive(0, 0, i, 520, 0x10B5C733, &drive_handle);
    }

    mut_printf(MUT_LOG_LOW, "=== starting physical package===\n");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


    /* Init fbe api on server */
    mut_printf(MUT_LOG_LOW, "=== starting Server FBE_API ===\n");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== set Physical Package entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    /* Wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(NUMBER_OF_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
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
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == NUMBER_OF_OBJECTS);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");

	/* Wait for LDO to become ready */
	fbe_api_sleep(2000);

    /* Load Storage Extent Package */
	sep_config_load_sep();

    return;

#undef NUMBER_OF_OBJECTS
}

void kilgore_trout_config_destroy(void)
{
    mut_printf(MUT_LOG_LOW, "=== destroy configuration ===\n");
    fbe_test_sep_util_destroy_sep_physical();
}


