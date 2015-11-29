/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * chautauqua.c
 ***************************************************************************
 *
 * HISTORY
 *   11/19/2009:  Migrated from zeus_ready_test. Bo Gao
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "physical_package_tests.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe_test_configurations.h"

#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_package.h"
#include "pp_utils.h"

/*********************************************************************
* Components to support Table driven test methodology.
* Following this approach, the parameters for the test will be stored
* as rows in a table.
* The test will then be executed for each row(i.e. set of parameters)
* in the table.
*********************************************************************/

/*!*******************************************************************
* @enum chautauqua_test_number_t
*********************************************************************
* @brief The list of unique configurations to be tested in 
* chautauqua_test.
*********************************************************************/
typedef enum chautauqua_test_number_e
{
    CHAUTAUQUA_TEST_VIPER,
    CHAUTAUQUA_TEST_DERRINGER,
    CHAUTAUQUA_TEST_BUNKER,
    CHAUTAUQUA_TEST_CITADEL,
    CHAUTAUQUA_TEST_FALLBACK,
    CHAUTAUQUA_TEST_BOXWOOD,
    CHAUTAUQUA_TEST_KNOT,
    CHAUTAUQUA_TEST_ANCHO,
    CHAUTAUQUA_TEST_CALYPSO,
    CHAUTAUQUA_TEST_RHEA,
    CHAUTAUQUA_TEST_MIRANDA,
    CHAUTAUQUA_TEST_TABASCO,
} chautauqua_test_number_t;

/*!*******************************************************************
* @var test_table
*********************************************************************
* @brief This is a table of set of parameters for which the test has 
* to be executed.
*********************************************************************/
fbe_test_params_t chautauqua_table[] = 
{   
    {CHAUTAUQUA_TEST_VIPER,
    "VIPER",
    FBE_BOARD_TYPE_FLEET,
    FBE_PORT_TYPE_SAS_PMC,
    3,
    5,
    0,
    FBE_SAS_ENCLOSURE_TYPE_VIPER,
    0,
    FBE_SAS_DRIVE_CHEETA_15K,
    INVALID_DRIVE_NUMBER,
    MAX_DRIVE_COUNT_VIPER,
    CHAUTAUQUA_MAX_PORTS,
    CHAUTAUQUA_MAX_ENCLS,
    VIPER_LOW_DRVCNT_MASK,
    },
    {CHAUTAUQUA_TEST_DERRINGER,
    "DERRINGER",
    FBE_BOARD_TYPE_FLEET,
    FBE_PORT_TYPE_SAS_PMC,
    3,
    5,
    0,
    FBE_SAS_ENCLOSURE_TYPE_DERRINGER,
    0,
    FBE_SAS_DRIVE_CHEETA_15K,
    INVALID_DRIVE_NUMBER,
    MAX_DRIVE_COUNT_DERRINGER,
    CHAUTAUQUA_MAX_PORTS,
    CHAUTAUQUA_MAX_ENCLS,
    DERRINGER_LOW_DRVCNT_MASK,
    },
    {CHAUTAUQUA_TEST_BUNKER,
    "BUNKER",
    FBE_BOARD_TYPE_FLEET,
    FBE_PORT_TYPE_SAS_PMC,
    3,
    5,
    0,
    FBE_SAS_ENCLOSURE_TYPE_BUNKER,
    0,
    FBE_SAS_DRIVE_CHEETA_15K,
    INVALID_DRIVE_NUMBER,
    MAX_DRIVE_COUNT_BUNKER,
    CHAUTAUQUA_MAX_PORTS,
    CHAUTAUQUA_MAX_ENCLS,
    BUNKER_LOW_DRVCNT_MASK,
    },
    {CHAUTAUQUA_TEST_CITADEL,
    "CITADEL",
    FBE_BOARD_TYPE_FLEET,
    FBE_PORT_TYPE_SAS_PMC,
    3,
    5,
    0,
    FBE_SAS_ENCLOSURE_TYPE_CITADEL,
    0,
    FBE_SAS_DRIVE_CHEETA_15K,
    INVALID_DRIVE_NUMBER,
    MAX_DRIVE_COUNT_CITADEL,
    CHAUTAUQUA_MAX_PORTS,
    CHAUTAUQUA_MAX_ENCLS,
    CITADEL_LOW_DRVCNT_MASK,
    },
    {CHAUTAUQUA_TEST_FALLBACK,
    "FALLBACK",
    FBE_BOARD_TYPE_FLEET,
    FBE_PORT_TYPE_SAS_PMC,
    3,
    5,
    0,
    FBE_SAS_ENCLOSURE_TYPE_FALLBACK,
    0,
    FBE_SAS_DRIVE_CHEETA_15K,
    INVALID_DRIVE_NUMBER,
    MAX_DRIVE_COUNT_FALLBACK,
    CHAUTAUQUA_MAX_PORTS,
    CHAUTAUQUA_MAX_ENCLS,
    FALLBACK_LOW_DRVCNT_MASK,
    },
    {CHAUTAUQUA_TEST_BOXWOOD,
    "BOXWOOD",
    FBE_BOARD_TYPE_FLEET,
    FBE_PORT_TYPE_SAS_PMC,
    3,
    5,
    0,
    FBE_SAS_ENCLOSURE_TYPE_BOXWOOD,
    0,
    FBE_SAS_DRIVE_CHEETA_15K,
    INVALID_DRIVE_NUMBER,
    MAX_DRIVE_COUNT_BOXWOOD,
    CHAUTAUQUA_MAX_PORTS,
    CHAUTAUQUA_MAX_ENCLS,
    BOXWOOD_LOW_DRVCNT_MASK,
    },
    {CHAUTAUQUA_TEST_KNOT,
    "KNOT",
    FBE_BOARD_TYPE_FLEET,
    FBE_PORT_TYPE_SAS_PMC,
    3,
    5,
    0,
    FBE_SAS_ENCLOSURE_TYPE_KNOT,
    0,
    FBE_SAS_DRIVE_CHEETA_15K,
    INVALID_DRIVE_NUMBER,
    MAX_DRIVE_COUNT_KNOT,
    CHAUTAUQUA_MAX_PORTS,
    CHAUTAUQUA_MAX_ENCLS,
    KNOT_LOW_DRVCNT_MASK,
    },
    {CHAUTAUQUA_TEST_ANCHO,
    "ANCHO",
    FBE_BOARD_TYPE_FLEET,            /* board_type */
    FBE_PORT_TYPE_SAS_PMC,           /* port_type */
    3,                               /* io_port_number */
    5,                               /* portal_number */
    0,                               /* backend_number */
    FBE_SAS_ENCLOSURE_TYPE_ANCHO,    /* encl_type */
    0,                               /* encl_number */
    FBE_SAS_DRIVE_CHEETA_15K,        /* drive_type */
    INVALID_DRIVE_NUMBER,            /* drive_number */
    MAX_DRIVE_COUNT_ANCHO,           /* #define MAX_DRIVE_COUNT_ANCHO 15 */
    CHAUTAUQUA_MAX_PORTS,            /* max_ports */
    CHAUTAUQUA_MAX_ENCLS,            /* max_enclosures */
    ANCHO_LOW_DRVCNT_MASK,           /* drive_mask */
    },
// Moons
    {CHAUTAUQUA_TEST_CALYPSO,
    "CALYPSO",
    FBE_BOARD_TYPE_FLEET,
    FBE_PORT_TYPE_SAS_PMC,
    3,
    5,
    0,
    FBE_SAS_ENCLOSURE_TYPE_CALYPSO,
    0,
    FBE_SAS_DRIVE_CHEETA_15K,
    INVALID_DRIVE_NUMBER,
    MAX_DRIVE_COUNT_CALYPSO,
    CHAUTAUQUA_MAX_PORTS,
    CHAUTAUQUA_MAX_ENCLS,
    CALYPSO_LOW_DRVCNT_MASK,
    },
    {CHAUTAUQUA_TEST_RHEA,
    "RHEA",
    FBE_BOARD_TYPE_FLEET,
    FBE_PORT_TYPE_SAS_PMC,
    3,
    5,
    0,
    FBE_SAS_ENCLOSURE_TYPE_RHEA,
    0,
    FBE_SAS_DRIVE_CHEETA_15K,
    INVALID_DRIVE_NUMBER,
    MAX_DRIVE_COUNT_RHEA,
    CHAUTAUQUA_MAX_PORTS,
    CHAUTAUQUA_MAX_ENCLS,
    RHEA_LOW_DRVCNT_MASK,
    },
    {CHAUTAUQUA_TEST_MIRANDA,
    "MIRANDA",
    FBE_BOARD_TYPE_FLEET,
    FBE_PORT_TYPE_SAS_PMC,
    3,
    5,
    0,
    FBE_SAS_ENCLOSURE_TYPE_MIRANDA,
    0,
    FBE_SAS_DRIVE_CHEETA_15K,
    INVALID_DRIVE_NUMBER,
    MAX_DRIVE_COUNT_MIRANDA,
    CHAUTAUQUA_MAX_PORTS,
    CHAUTAUQUA_MAX_ENCLS,
    MIRANDA_LOW_DRVCNT_MASK,
    },
    {CHAUTAUQUA_TEST_TABASCO,
    "TABASCO",
    FBE_BOARD_TYPE_FLEET,
    FBE_PORT_TYPE_SAS_PMC,
    3,
    5,
    0,
    FBE_SAS_ENCLOSURE_TYPE_TABASCO,
    0,
    FBE_SAS_DRIVE_CHEETA_15K,
    INVALID_DRIVE_NUMBER,
    MAX_DRIVE_COUNT_TABASCO,
    CHAUTAUQUA_MAX_PORTS,
    CHAUTAUQUA_MAX_ENCLS,
    TABASCO_LOW_DRVCNT_MASK,
    },
} ;

/*!*******************************************************************
* @def CHAUTAUQUA_TEST_MAX
*********************************************************************
* @brief Number of test scenarios to be executed.
*********************************************************************/
#define CHAUTAUQUA_TEST_MAX     sizeof(chautauqua_table)/sizeof(chautauqua_table[0])


fbe_status_t chautauqua_load_and_verify(fbe_test_params_t *test)
{
    fbe_class_id_t      class_id;
    fbe_u32_t           object_handle = 0;
    fbe_u32_t           port_idx;
    fbe_u32_t           enclosure_idx;
    fbe_u32_t           drive_idx;
    fbe_port_number_t   port_number;
    fbe_status_t        status;

    /* Before initializing the physical package, initialize the terminator. */
    status = fbe_api_terminator_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "%s: terminator initialized for %s.", __FUNCTION__, test->title);

    /* Load the test config*/
    status = fbe_test_load_chautauqua_config(test, SPID_UNKNOWN_HW_TYPE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "%s: chautauqua config loaded for %s.", __FUNCTION__, test->title);

	/* Initialize the fbe_api on server. */ 
    mut_printf(MUT_LOG_LOW, "%s: starting fbe_api for %s....", __FUNCTION__, test->title);
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize the physical package. */
    mut_printf(MUT_LOG_LOW, "%s: starting physical package for %s....", __FUNCTION__, test->title);
    fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    
    fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the expected number of objects */
    status = fbe_test_wait_for_term_pp_sync(80000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_LOW, "%s: starting configuration verification for %s.", __FUNCTION__, test->title);

    /*we inserted a fleet board so we check for it*/
    /* board is always object id 0 ??? Maybe we need an interface to get the board object id*/
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type for %s....Passed", test->title);

    /* We should have exactly TWO ports. */
    for (port_idx = 0; port_idx < FBE_TEST_PHYSICAL_BUS_COUNT; port_idx++) {
        if (port_idx < test->max_ports) {
            /*get the handle of the port and validate port exists*/
            status = fbe_api_get_port_object_id_by_location (port_idx, &object_handle);
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
            status = fbe_api_get_port_object_id_by_location (port_idx, &object_handle);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(object_handle == FBE_OBJECT_ID_INVALID);
        }
    }
    mut_printf(MUT_LOG_LOW, "Verifying port existance, state and number for %s ...Passed", test->title);

    /* We should have exactly 8 enclosure on each port. */
    for (port_idx = 0; port_idx < FBE_TEST_PHYSICAL_BUS_COUNT; port_idx++) {
        for (enclosure_idx = 0; enclosure_idx < FBE_TEST_ENCLOSURES_PER_BUS; enclosure_idx++) {
            if (port_idx < test->max_ports) {
                /*get the handle of the port and validate enclosure exists*/
                status = fbe_api_get_enclosure_object_id_by_location (port_idx, enclosure_idx, &object_handle);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

                /* Use the handle to get lifecycle state*/
                status = fbe_zrt_wait_enclosure_status (port_idx, enclosure_idx, FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

                /* Use the handle to get port number*/
                status = fbe_api_get_object_port_number (object_handle, &port_number);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                MUT_ASSERT_TRUE(port_number == port_idx);
            }
            else
            {
                /*get the handle of the drive and validate drive exists*/
                status = fbe_api_get_enclosure_object_id_by_location(port_idx, enclosure_idx, &object_handle);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                MUT_ASSERT_TRUE(object_handle == FBE_OBJECT_ID_INVALID);
            }
        }
    }
    mut_printf(MUT_LOG_LOW, "Verifying enclosure existance and state for %s....Passed", test->title);

    /* Check for the physical drives on the enclosure. */
    for (port_idx = 0; port_idx < FBE_TEST_PHYSICAL_BUS_COUNT; port_idx++) {
        for (enclosure_idx = 0; enclosure_idx < FBE_TEST_ENCLOSURES_PER_BUS; enclosure_idx++) {
            for (drive_idx = 0; drive_idx < test->max_drives; drive_idx++) {
                if (port_idx < test->max_ports) {
                    /* Use the handle to get lifecycle state*/
                    status = fbe_test_pp_util_verify_pdo_state(port_idx,
                                                               enclosure_idx,
                                                               drive_idx,
                                                               FBE_LIFECYCLE_STATE_READY,
                                                               READY_STATE_WAIT_TIME);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                    /* Get the handle of the drive*/
                    status = fbe_api_get_physical_drive_object_id_by_location(port_idx, enclosure_idx, drive_idx, &object_handle);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                    MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

                    /* Use the handle to get port number of the drive*/
                    status = fbe_api_get_object_port_number (object_handle, &port_number);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                    MUT_ASSERT_TRUE(port_number == port_idx);

                }
                else
                {
                    /*get the handle of the drive and validate drive exists*/
                    status = fbe_api_get_physical_drive_object_id_by_location(port_idx, enclosure_idx, drive_idx, &object_handle);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                    MUT_ASSERT_TRUE(object_handle == FBE_OBJECT_ID_INVALID);
                }
            }
        }
    }
    mut_printf(MUT_LOG_LOW, "Verifying physical drive existance and state for %s....Passed", test->title);

    /* Let's see how many objects we have...
    1 board
    2 port
    2* 8 enclosure
    2* 8*15 physical drives
    2* 8*15 logical drives
    499 objects
    */  
    status = fbe_test_verify_term_pp_sync();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "Verifying object count for %s ...Passed", test->title);

    mut_printf(MUT_LOG_LOW, "%s: configuration verification for %s SUCCEEDED!", __FUNCTION__, test->title);

    return FBE_STATUS_OK;
}

fbe_status_t chautauqua_load_and_verify_with_platform(fbe_test_params_t *test, SPID_HW_TYPE platform_type)
{
    fbe_class_id_t      class_id;
    fbe_u32_t           object_handle = 0;
    fbe_u32_t           port_idx;
    fbe_u32_t           enclosure_idx;
    fbe_u32_t           drive_idx;
    fbe_port_number_t   port_number;
    fbe_status_t        status;

    /* Before initializing the physical package, initialize the terminator. */
    status = fbe_api_terminator_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "%s: terminator initialized for %s.", __FUNCTION__, test->title);

    /* Load the test config*/
    status = fbe_test_load_chautauqua_config(test, platform_type);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "%s: chautauqua config loaded for %s.", __FUNCTION__, test->title);

	/* Initialize the fbe_api on server. */ 
    mut_printf(MUT_LOG_LOW, "%s: starting fbe_api for %s....", __FUNCTION__, test->title);
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize the physical package. */
    mut_printf(MUT_LOG_LOW, "%s: starting physical package for %s....", __FUNCTION__, test->title);
    fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    
    fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the expected number of objects */
    status = fbe_test_wait_for_term_pp_sync(80000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_LOW, "%s: starting configuration verification for %s.", __FUNCTION__, test->title);

    /*we inserted a fleet board so we check for it*/
    /* board is always object id 0 ??? Maybe we need an interface to get the board object id*/
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type for %s....Passed", test->title);

    /* We should have exactly TWO ports. */
    for (port_idx = 0; port_idx < FBE_TEST_PHYSICAL_BUS_COUNT; port_idx++) {
        if (port_idx < test->max_ports) {
            /*get the handle of the port and validate port exists*/
            status = fbe_api_get_port_object_id_by_location (port_idx, &object_handle);
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
            status = fbe_api_get_port_object_id_by_location (port_idx, &object_handle);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(object_handle == FBE_OBJECT_ID_INVALID);
        }
    }
    mut_printf(MUT_LOG_LOW, "Verifying port existance, state and number for %s ...Passed", test->title);

    /* We should have exactly 8 enclosure on each port. */
    for (port_idx = 0; port_idx < FBE_TEST_PHYSICAL_BUS_COUNT; port_idx++) {
        for (enclosure_idx = 0; enclosure_idx < FBE_TEST_ENCLOSURES_PER_BUS; enclosure_idx++) {
            if (port_idx < test->max_ports) {
                /*get the handle of the port and validate enclosure exists*/
                status = fbe_api_get_enclosure_object_id_by_location (port_idx, enclosure_idx, &object_handle);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

                /* Use the handle to get lifecycle state*/
                status = fbe_zrt_wait_enclosure_status (port_idx, enclosure_idx, FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

                /* Use the handle to get port number*/
                status = fbe_api_get_object_port_number (object_handle, &port_number);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                MUT_ASSERT_TRUE(port_number == port_idx);
            }
            else
            {
                /*get the handle of the drive and validate drive exists*/
                status = fbe_api_get_enclosure_object_id_by_location(port_idx, enclosure_idx, &object_handle);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                MUT_ASSERT_TRUE(object_handle == FBE_OBJECT_ID_INVALID);
            }
        }
    }
    mut_printf(MUT_LOG_LOW, "Verifying enclosure existance and state for %s....Passed", test->title);

    /* Check for the physical drives on the enclosure. */
    for (port_idx = 0; port_idx < FBE_TEST_PHYSICAL_BUS_COUNT; port_idx++) {
        for (enclosure_idx = 0; enclosure_idx < FBE_TEST_ENCLOSURES_PER_BUS; enclosure_idx++) {
            for (drive_idx = 0; drive_idx < test->max_drives; drive_idx++) {
                if (port_idx < test->max_ports) {
                    /* Use the handle to get lifecycle state*/
                    status = fbe_test_pp_util_verify_pdo_state(port_idx,
                                                               enclosure_idx,
                                                               drive_idx,
                                                               FBE_LIFECYCLE_STATE_READY,
                                                               READY_STATE_WAIT_TIME);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                    /* Get the handle of the drive*/
                    status = fbe_api_get_physical_drive_object_id_by_location(port_idx, enclosure_idx, drive_idx, &object_handle);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                    MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

                    /* Use the handle to get port number of the drive*/
                    status = fbe_api_get_object_port_number (object_handle, &port_number);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                    MUT_ASSERT_TRUE(port_number == port_idx);

                }
                else
                {
                    /*get the handle of the drive and validate drive exists*/
                    status = fbe_api_get_physical_drive_object_id_by_location(port_idx, enclosure_idx, drive_idx, &object_handle);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                    MUT_ASSERT_TRUE(object_handle == FBE_OBJECT_ID_INVALID);
                }
            }
        }
    }
    mut_printf(MUT_LOG_LOW, "Verifying physical drive existance and state for %s....Passed", test->title);

    /* Let's see how many objects we have...
    1 board
    2 port
    2* 8 enclosure
    2* 8*15 physical drives
    2* 8*15 logical drives
    499 objects
    */  
    status = fbe_test_verify_term_pp_sync();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "Verifying object count for %s ...Passed", test->title);

    mut_printf(MUT_LOG_LOW, "%s: configuration verification for %s SUCCEEDED!", __FUNCTION__, test->title);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_test_get_chautauqua_test_table()
 ****************************************************************
 * @brief
 * This function takes index num and returns a pointer to the table entry.
 *
 * @return pointer to fbe_test_params_t structure .
 * 
 * @author
 *  9/9/2011 - Created.
 ****************************************************************/

fbe_test_params_t  *fbe_test_get_chautauqua_test_table(fbe_u32_t index_num)
{
     return (&chautauqua_table[index_num]);
}

/*!***************************************************************
 * fbe_test_get_chautauqua_num_of_tests()
 ****************************************************************
 * @brief
 * This function returns number of table entries.
 *
 * @ returns number of table entries.
 * 
 * @author
 *   9/9/2011 - Created. 
 ****************************************************************/

fbe_u32_t fbe_test_get_chautauqua_num_of_tests(void)
{
     return  (sizeof(chautauqua_table))/(sizeof(chautauqua_table[0]));
}

void chautauqua(void)
{
    fbe_status_t run_status;
    fbe_u32_t test_num;

    for(test_num = 0; test_num < CHAUTAUQUA_TEST_MAX; test_num++)
    {
        /* Load the configuration for test
        */
        run_status = chautauqua_load_and_verify(&chautauqua_table[test_num]);
        MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);

        /* Unload the configuration
        */
        fbe_test_physical_package_tests_config_unload();
    }
}

