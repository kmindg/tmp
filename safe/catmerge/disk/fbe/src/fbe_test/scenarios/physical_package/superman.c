#include "physical_package_tests.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe_test_configurations.h"
#include "pp_utils.h"


#define SUPERMAN_CONFIG_MAX_DRIVE_COUNT        12
char * superman_short_desc = "Creation of different protocol ports ith different roles.";
char * superman_long_desc =
    "\n"
    "\n"
    "The Superman scenario tests port scanning and creation of port objects for SAS,FC and iScsi.\n"
    "It includes:\n" 
    "     - creation of Frontend, Backend and Uncommitted ports;\n"
    ;


static fbe_status_t superman_verify_topology(void);

static fbe_status_t superman_verify_topology(void)
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

static fbe_status_t superman_run(void)
{    
    fbe_status_t status;
    fbe_lifecycle_state_t api_lifecycle_state;
    fbe_u32_t object_handle_p;
    fbe_api_terminator_device_handle_t port_handle, enclosure_handle;

    /*
     * 1. Send reset begin and all related logouts and aborts.
     * 2. Verify topoloty to confirm that 
     *       a) There are no valid entries in the device table.
     *       b) All proto edges to Port have CLOSED attrib set.
     *       c) Discovery edge to Port have NOT PRESENT attrib set.
     *       d) Port object has transition to ACTIVATE state.
     * 3. Send reset complete and all related logins
     * 4. Wait for 15s - 30s to allow for all logins and state transition of all objects
     * 5. Verify the topology to see whether all objects are in READY state and all edges are READY.
     */
    mut_printf(MUT_LOG_TEST_STATUS,
        "%s: ===== Starting Port reset for port 0 ========.\n",
        __FUNCTION__);
    status = fbe_api_terminator_get_port_handle(0, &port_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_terminator_start_port_reset(port_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* Verify() 
     * 1. Whether port object transitioned out of READY state
     * 2. Whether the CLOSED attribute is set on each edge.
     * 3. Whether the edge state has transitioned from READY.
     */
    EmcutilSleep(100);
    mut_printf(MUT_LOG_TEST_STATUS,
        "%s: ===== Verifying port state. ========.\n",
        __FUNCTION__);

    status = fbe_api_get_port_object_id_by_location(0, &object_handle_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_get_object_lifecycle_state(object_handle_p, &api_lifecycle_state,FBE_PACKAGE_ID_PHYSICAL);    
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "ERROR can't get port lifecycle state, status");

    status = fbe_api_wait_for_object_lifecycle_state (object_handle_p, 
        FBE_LIFECYCLE_STATE_ACTIVATE, READY_STATE_WAIT_TIME, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS,
        "%s: ===== Starting LOGOUT of all devices for port 0 ========.\n",
        __FUNCTION__);

    status = fbe_api_terminator_get_enclosure_handle(0,0, &enclosure_handle);

    status = fbe_api_terminator_force_logout_device(enclosure_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    EmcutilSleep(100);
    /*Verify() that all drives have CLOSED attribute set.*/

    mut_printf(MUT_LOG_TEST_STATUS,
        "%s: ===== Completing Port reset for port 0 ========.\n",
        __FUNCTION__);

    status = fbe_api_terminator_complete_port_reset(port_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*Verify()*/
    EmcutilSleep(1500);
    mut_printf(MUT_LOG_TEST_STATUS,
        "%s: ===== Verifying port state. ========.\n",
        __FUNCTION__);

    status = fbe_api_get_port_object_id_by_location(0, &object_handle_p);
    if (status !=  FBE_STATUS_OK) {
        mut_printf(MUT_LOG_TEST_STATUS,
            "%s: ERROR can't get port handle, status: 0x%X.",
            __FUNCTION__, status);
        return status;
    }

    status = fbe_api_get_object_lifecycle_state(object_handle_p, &api_lifecycle_state,FBE_PACKAGE_ID_PHYSICAL);    
    if (status !=  FBE_STATUS_OK) {
        mut_printf(MUT_LOG_TEST_STATUS,
            "%s: ERROR can't get port [%u] lifecycle state, status: 0x%X.",
            __FUNCTION__, 0, status);
        return status;
    }

    /*MUT_ASSERT_TRUE((api_lifecycle_state == FBE_LIFECYCLE_STATE_READY));*/
    status = fbe_api_wait_for_object_lifecycle_state (object_handle_p, 
        FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS,
        "%s: ===== Starting LOGIN of all devices for port 0 ========.\n",
        __FUNCTION__);

    /* login enclosure will get all children device logged in */
    status = fbe_api_terminator_force_login_device(enclosure_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    EmcutilSleep(1000);
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


void superman(void)
{

    fbe_status_t    status  = FBE_STATUS_GENERIC_FAILURE;
    status = superman_run();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

}

