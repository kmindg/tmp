
/***************************************************************************
 * Copyright (C) EMC Corporation 2010-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file spongebob_test.c
 ***************************************************************************
 *
 * @brief
 *   This file includes test to verify that when object gets destroy, it will wait to drain all 
 *   associate events from event service queue before destroy take place.
 *
 * !@note This test is written to test following logic.
 *  1. Object destroy needs to pend to drain pending events from event service queue.
 *  2. While destroy the object, set the flag to block transport server so that no any
 *      more events enqueue to event service queue.
 *    
 *    To test above functionality, this test sends events in loop asynchronously and destroy the 
 *    object while continue sending events. But while running the test, it may possible that when 
 *    object receive  destroy command, there will be none associate events present in event
 *    service queue and because of that object will destroy immediately as no need to wait
 *    to drain pending events. There is no mechanism to hold events in queue.
 * 
 *  
 *
 * @version
 *  7/11/2011  Amit Dhaduk  - Created.
 *
 ***************************************************************************/


/*-----------------------------------------------------------------------------
    INCLUDES OF REQUIRED HEADER FILES:
*/
 

/*************************
 *   INCLUDE FILES
 *************************/
#include "sep_tests.h"
#include "sep_utils.h"
#include "pp_utils.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_database_interface.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * spongebob_short_desc = "Validate handling of pending events from Event Service queue";
char * spongebob_long_desc  = "\
The Spongebob scenario tests handling of pending events from Event Service queue when destroying RAID Group Object.\n\
This test runs in both Simulation and Hardware environment.\n"\
"\n"\
"-raid_test_level 0 \n\
\n\
STEP 1: Configure all raid groups and LUNs.\n\
        - Tests cover 520 block sizes.\n\
        - Tests cover one Raid-5 RG.\n\
\n\
STEP 2: Fill out disk set at run time from available disk pool \n\
\n\
STEP 3: Testing to handle pending events while destroy object(raid group) \n\
        - Create RG \n\
        - Wait for RG to become ready \n\
        - Send multiple zero permit event request to provision drive object asynchronously.\n\
        - Provision drive object will send zero permit events to upstream objects (VD and Raid) \n\
          through event service.\n\
        - Destroy the raid group at some point when enough events queued in event service queue \n\
          which have destination to that raid group. \n\
        - Verify that\n\
          - Raid group destroy process wait for events to complete from event service queue.\n\
          - Raid group destroy completes when no more events pending\n\
          - Continue sending events to provision drive after raid group destroy\n\
          - Check that all events sent from test get complete successfully\n\
\n\
Description last updated: 10/06/2011.\n";

/*!*******************************************************************
 * @var event_complete_counter
 *********************************************************************
 * @brief This counter track the number of complete events..
 *
 *********************************************************************/
fbe_atomic_t event_complete_counter =0;


/*!*******************************************************************
 * @def SPONGEBOB_TEST_NS_TIMEOUT
 *********************************************************************
 * @brief  Notification to timeout value in milliseconds 
 *
 *********************************************************************/
#define SPONGEBOB_TEST_NS_TIMEOUT        25000 /*wait maximum of 25 seconds*/


/*!*******************************************************************
 * @def SPONGEBOB_POLLING_INTERVAL
 *********************************************************************
 * @brief polling interval time to check for disk zeroing complete
 *
 *********************************************************************/
#define SPONGEBOB_POLLING_INTERVAL 100 /*ms*/


/*!*******************************************************************
 * @def SPONGEBOB_TEST_RAID_GROUP_ID
 *********************************************************************
 * @brief RAID Group id used by this test
 *
 *********************************************************************/
#define SPONGEBOB_TEST_RAID_GROUP_ID        0

/*!*******************************************************************
 * @def SPONGEBOB_TEST_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Max number of raid groups we will test with.
 *
 *********************************************************************/
#define SPONGEBOB_TEST_RAID_GROUP_COUNT 1

#define SPONGEBOB_TEST_RAID_GROUP_WIDTH 3

#define SPONGEBOB_TEST_EVENT_COUNT 200

#define SPONGEBOB_TEST_LUNS_PER_RAID_GROUP      2

#define SPONGEBOB_TEST_CHUNKS_PER_LUN       3

/*!*******************************************************************
 * @var spongebob_rg_configuration
 *********************************************************************
 * @brief This is rg_configuration
 *
 *********************************************************************/
static fbe_test_rg_configuration_t spongebob_rg_configuration[] = 
{
     /* width,                        capacity          raid type,                  class,                block size  RAID-id.  bandwidth.*/
     {SPONGEBOB_TEST_RAID_GROUP_WIDTH, 0xE000,  FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,  520,        0,        0},

        
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

static void spongebob_test_create_rg(fbe_test_rg_configuration_t *rg_config_p);
static void spongebob_test_destroy_rg(fbe_test_rg_configuration_t *rg_config_p);
static fbe_status_t spongebob_send_asynch_events(fbe_test_rg_configuration_t *rg_config_p);
static fbe_status_t spongebob_asynch_event_complete(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);


/*!****************************************************************************
 *  spongebob_send_asynch_events
 ******************************************************************************
 *
 * @brief
 *   This function enqueue zero permit events in event service queue through provision drive 
 *   object in loop asynchronously.
 *
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *  7/11/2011 - Created. Amit Dhaduk
 *
 *****************************************************************************/

fbe_status_t spongebob_send_asynch_events(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_packet_t * packet_p = NULL;
    
    fbe_object_id_t           rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t          pvd_id = FBE_OBJECT_ID_INVALID;
    fbe_semaphore_t sem[200];    /* The array width needs to be in sync with SPONGEBOB_TEST_EVENT_COUNT*/
    fbe_u32_t index;

    fbe_status_t status;

    spongebob_test_create_rg(rg_config_p);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
    
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY, SPONGEBOB_TEST_NS_TIMEOUT, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[0].bus,
                                                    rg_config_p->rg_disk_set[0].enclosure,
                                                    rg_config_p->rg_disk_set[0].slot,
                                                    &pvd_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, pvd_id);
    
    /* fill out the zero permit request event data 
     * all events will use the same data. The basic purpose is that flood event service
     * queue with events while destoy the raid group
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s sending events start",  __FUNCTION__);

    /* send zero permit request event in loop asynchronously */
    for(index =0 ; index <SPONGEBOB_TEST_EVENT_COUNT; index ++)
    {
        /* select any random lba and block count which will be same for all sending events 
         */
        fbe_base_config_control_send_event_t config_data; 
        config_data.lba = 0x10000;
        config_data.block_count = 0x800;
        config_data.event_data.permit_request.permit_event_type = FBE_PERMIT_EVENT_TYPE_ZERO_REQUEST;
        config_data.event_type = FBE_EVENT_TYPE_PERMIT_REQUEST;
        config_data.medic_action_priority = 10;
        
        /* Allocate packet.*/
        packet_p = fbe_api_get_contiguous_packet();
        MUT_ASSERT_NOT_NULL(packet_p);

        
        fbe_semaphore_init(&sem[index], 0, 1);

        status = fbe_api_base_config_send_event(packet_p, pvd_id, &config_data, spongebob_asynch_event_complete, &sem[index]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
        /* destroy the raid group in the middle of sending events with expectation that object destory
         *  pends till all events drain out from event service queue for this raid group.
         * 
         */
        if(index == (SPONGEBOB_TEST_EVENT_COUNT/2))
        {
            spongebob_test_destroy_rg(rg_config_p);
            mut_printf(MUT_LOG_TEST_STATUS, "%s destroy raid group complete\n",  __FUNCTION__);
        }
    }
    
    /* wait for the event to complete */
    for(index =0 ; index < SPONGEBOB_TEST_EVENT_COUNT; index ++)
    {
        fbe_semaphore_wait(&sem[index], NULL);
    }

    for(index =0 ; index < SPONGEBOB_TEST_EVENT_COUNT; index ++)
    {
        fbe_semaphore_destroy(&sem[index]); /* SAFEBUG - needs destroy */
    }

    mut_printf(MUT_LOG_TEST_STATUS, "%s sending events complete",  __FUNCTION__);

   return FBE_STATUS_OK;

}

/*!****************************************************************************
 *  spongebob_asynch_event_complete
 ******************************************************************************
 *
 * @brief
 *  This function handles asynch event completion.
 *
 * @param   packet_p - event completion packet
 * @param context - semaphore to release.
 *
 * @return  None 
 *
 *****************************************************************************/

static fbe_status_t spongebob_asynch_event_complete(fbe_packet_t * packet_p,
                                                    fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t *) context;
    fbe_payload_ex_t * sep_payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_status_t            status;

    status = fbe_transport_get_status_code(packet_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    /* increment the event completion counter */
    fbe_atomic_increment(&event_complete_counter);

    /* Release the payload and control operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    status = fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    fbe_api_return_contiguous_packet(packet_p);

    /* Release the semaphore. */
    fbe_semaphore_release(sem, 0, 1, FALSE);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * spongebob_test_create_rg()
 ****************************************************************
 * @brief
 *  This is the test function for creating a Raid Group.
 *
 * @param rg_config_p - pointer to rg config table
 *
 * @return None.
 *
 ****************************************************************/

static void spongebob_test_create_rg(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                                status;
    fbe_object_id_t                             rg_object_id;
    fbe_job_service_error_type_t                job_error_code;
    fbe_status_t                                job_status;

    /* Initialize local variables */
    rg_object_id        = FBE_OBJECT_ID_INVALID;

    /* Create the SEP objects for the configuration */

    /* Create a RG */
    mut_printf(MUT_LOG_TEST_STATUS, "=== create a RAID Group ===\n");
    status = fbe_test_sep_util_create_raid_group(rg_config_p);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "RG creation failed");

    /* wait for notification from job service. */
    status = fbe_api_common_wait_for_job(rg_config_p->job_number,
                                         SPONGEBOB_TEST_NS_TIMEOUT,
                                         &job_error_code,
                                         &job_status,
                                         NULL);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to created RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(
             rg_config_p->raid_group_id, &rg_object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* Verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group object %d\n", rg_object_id);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: END\n", __FUNCTION__);

    return;

} 
/**************************************************
 * end spongebob_test_create_rg()
 **************************************************/

/*!****************************************************************************
 * spongebob_test_destroy_rg()
 ******************************************************************************
 * @brief
 *  This function destroy the raid group.
 *
 * @param rg_config_p - pointer to raid group configuration table..
 *
 * @return None.
 *
 ******************************************************************************/
static void spongebob_test_destroy_rg(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t    status;
    status = fbe_test_sep_util_destroy_raid_group(rg_config_p->raid_group_id);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Destroy RAID group failed.");
}


/*!****************************************************************************
 * spongebob_test()
 ******************************************************************************
 * @brief
 *  This is main entry function for spongebob test.
 *
 * @param None.
 *
 * @return None.
 *
 ******************************************************************************/
void spongebob_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t local_drive_counts[FBE_TEST_BLOCK_SIZE_LAST][FBE_DRIVE_TYPE_LAST];    
    fbe_bool_t  disk_found = FBE_FALSE;

    fbe_test_raid_group_disk_set_t * disk_set_p = NULL;
    fbe_test_discovered_drive_locations_t drive_locations;    
    fbe_test_block_size_t   block_size;
    fbe_drive_type_t        drive_type;
    fbe_u32_t               index;
    fbe_status_t status;
    rg_config_p = &spongebob_rg_configuration[0];


    /* fill out disk set at run time from available disk pool
     */
    fbe_test_sep_util_discover_all_drive_information(&drive_locations);
    fbe_copy_memory(&local_drive_counts[0][0], &drive_locations.drive_counts[0][0], (sizeof(fbe_u32_t) * FBE_TEST_BLOCK_SIZE_LAST * FBE_DRIVE_TYPE_LAST));

    status = fbe_test_sep_util_get_block_size_enum(rg_config_p->block_size, &block_size);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    for (drive_type = 0; drive_type < FBE_DRIVE_TYPE_LAST; drive_type++)
    {
        if (rg_config_p->width <= local_drive_counts[block_size][drive_type])
        {
            for (index = 0; index < rg_config_p->width; index++)
            {
                disk_set_p = NULL;
                local_drive_counts[block_size][drive_type]--;

                /* Fill in the next drive.
                 */
                status = fbe_test_sep_util_get_next_disk_in_set(local_drive_counts[block_size][drive_type], 
                                                                drive_locations.disk_set[block_size][drive_type],
                                                                &disk_set_p);
                if (status != FBE_STATUS_OK)
                {
                    mut_printf(MUT_LOG_TEST_STATUS, "spongebob - can't get next disk set so terminate the test\n");
                    return;
                }
                rg_config_p->rg_disk_set[index]= *disk_set_p;
            }
            disk_found = FBE_TRUE;
            break;
        }
    }

    if(disk_found == FBE_FALSE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "spongebob - do not have available disk to run the test\n");
        return;
    }

    /* disable all background service so event service will receive only events send from test code */
    //    fbe_api_base_config_disable_all_bg_services();

    /* set the event completion counter to zero. This counter will use to track and validate
     * event completion at end of test
     */
    event_complete_counter = 0;

    /* send events to event service */
    status = spongebob_send_asynch_events(rg_config_p);

    /* validate that all the events are completed */
    MUT_ASSERT_UINT64_EQUAL_MSG(SPONGEBOB_TEST_EVENT_COUNT, event_complete_counter, "spongebob : Did not complete all events.");

    return;
} // end of spongebob_test()


/*!****************************************************************************
 *  spongebob_setup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the spongebob test.  
 *
 * @param   None
 *
 * @return  None 
 *
  *****************************************************************************/

void spongebob_setup(void)
{

    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;

        rg_config_p = &spongebob_rg_configuration[0];
        
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

        elmo_load_config(rg_config_p, SPONGEBOB_TEST_LUNS_PER_RAID_GROUP, SPONGEBOB_TEST_CHUNKS_PER_LUN);

        /* set the trace level */
        elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);    
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    
    return; 
}
/******************************************
 * end spongebob_setup()
 ******************************************/
 
/*!****************************************************************************
 * spongebob_cleanup()
 ******************************************************************************
 * @brief
 *  Cleanup function.
 *
 * @param None.
 *
 * @return None.
 *
 ******************************************************************************/

void spongebob_cleanup(void)
{
    mut_printf(MUT_LOG_LOW, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/*************************
 * end file spongebob_test.c
 *************************/


