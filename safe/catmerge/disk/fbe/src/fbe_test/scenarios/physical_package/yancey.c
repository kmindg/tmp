#include "physical_package_tests.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_package.h"
#include "fbe_test_configurations.h"
#include "pp_utils.h"

char * yancey_short_desc = "1 board + 1 sas port + 1 sas encl + 12 sas drives + 1 fc FE port";
char * yancey_long_desc = "Load and verify Yancey  configuration";


/****************************************************************
 * yancey_verify_topology()
 ****************************************************************
 * Description:
 *  Function to get and verify the state of the enclosure 
 *  and its drives for yancey  configuration.
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  09/09/2008 - Created. Arunkumar Selvapillai
 *
 ****************************************************************/

fbe_status_t yancey_verify_enclosures_and_disks(fbe_u32_t port, fbe_u32_t encl, 
                            fbe_lifecycle_state_t state,
                            fbe_u32_t timeout)
{
    fbe_status_t    status;
    fbe_u32_t       no_of_encls;
    fbe_u32_t       object_handle = 0;
    fbe_u32_t       slot_num;
        
    /* Loop through all the enclosures and its drives to check its state */
    for ( no_of_encls = encl; no_of_encls < FBE_TEST_ENCLOSURES_PER_BUS; no_of_encls++ )
    {
        if ( no_of_encls < 1 )
        {
            /* Validate that the enclosure is in READY/ACTIVATE state */
            status = fbe_zrt_wait_enclosure_status(port, encl, state, timeout);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

            for( slot_num = 0; slot_num < 12; slot_num ++ )
            {
                /* Validate that the physical drives are in READY/ACTIVATE state */
                status = fbe_test_pp_util_verify_pdo_state(port, encl, slot_num, state, timeout);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            }
        }
        else
        {
            /* Get the handle of the port and validate enclosure exists or not */
            status = fbe_api_get_enclosure_object_id_by_location(port, no_of_encls, &object_handle);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(object_handle == FBE_OBJECT_ID_INVALID);
        }
    }
    return FBE_STATUS_OK;
}

fbe_status_t yancey_verify_topology(void)
{
    fbe_class_id_t      class_id;
    fbe_u32_t           object_handle = 0;
    fbe_u32_t           port_idx;
    fbe_port_number_t   port_number;
    fbe_u32_t			total_objects = 0;
    fbe_status_t status;
    /*we inserted a fleet board so we check for it*/
    /* board is always object id 0 ??? Maybe we need an interface to get the board object id*/
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type....Passed");

    /* We should have exactly two ports. */
    for (port_idx = 0; port_idx < FBE_TEST_ENCLOSURES_PER_BUS; port_idx++) 
    {
        if (port_idx < 3) 
        {
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

            if (port_idx < 1) {

                /* Verify the existence of the enclosure and its drives */
                status = yancey_verify_enclosures_and_disks(port_idx, 0, FBE_LIFECYCLE_STATE_READY, 
                                                    READY_STATE_WAIT_TIME);
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

    /* Let's see how many objects we have...
    1 board
    1 sas port
    1 enclosure
    12 physical drives
    12 logical drives

    1 fc port
    1 iscsi port
    29 objects

    */  
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == YANCEY_NUMBER_OF_OBJ);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");

    mut_printf(MUT_LOG_LOW, "%s: configuration verification SUCCEEDED!", __FUNCTION__);

    return status;
}
fbe_status_t yancey_load_and_verify(void)
{   
    fbe_status_t        status;


    /* Before initializing the physical package, initialize the terminator. */
    status = fbe_api_terminator_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "%s: terminator initialized.", __FUNCTION__);

    /* Load the test config */
    status = fbe_test_load_yancey_config();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/* Initialize the fbe_api on server. */ 
    mut_printf(MUT_LOG_LOW, "%s: starting fbe_api...", __FUNCTION__);
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize the physical package. */
    mut_printf(MUT_LOG_LOW, "%s: starting physical package...", __FUNCTION__);
    fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    
    fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(YANCEY_NUMBER_OF_OBJ, 20000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_LOW, "%s: starting configuration verification...", __FUNCTION__);

    yancey_verify_topology();
    return FBE_STATUS_OK;
}

void yancey(void)
{
    /* do nothing */
}