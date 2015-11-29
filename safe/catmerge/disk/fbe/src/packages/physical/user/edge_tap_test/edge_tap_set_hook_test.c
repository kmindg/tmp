#include "edge_tap_main.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_object_map_interface.h"
#include "fbe_topology.h"
#include "fbe_test_io_api.h"

static fbe_test_io_case_t edge_tap_520_io_case_array[] =
{
    /* 
     * exported imported start  end     start    end     increment exp opt   imp opt
     *  bl size block    lba    lba     blocks   blocks  blocks    bl sizes  bl size
     */
    {520,       520,    0x0,    0x20,    1,       1024,       1,       1,        1},
    /* This is the terminator record, it always should be zero.
     */
    {FBE_TEST_IO_INVALID_FIELD, 0,         0,    0,    0,       0, 0},

};

static fbe_u32_t edge_hook_received_packet_count = 0;

static fbe_status_t set_hook_test_hook_function(void * packet);

void edge_tap_set_hook_on_physical_drive_test(void)
{
    fbe_status_t status;
    fbe_object_id_t obj_id;

    fbe_api_set_edge_hook_t 	hook_info;

    /* get the physical drive object id */
    status = fbe_api_object_map_interface_get_physical_drive_obj_id(0, 0, 0, &obj_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* set the hook on to physical drive ssp edge*/
    hook_info.hook = set_hook_test_hook_function;
    status = fbe_api_set_ssp_edge_hook (obj_id, &hook_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: successfully set edge hook to object %d. ===\n", __FUNCTION__, obj_id);

    /* get the logical drive object id */
    status = fbe_api_object_map_interface_get_logical_drive_obj_id(0, 0, 0, &obj_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Set the global counter to 0*/
    edge_hook_received_packet_count = 0;
    /* Kick off write/read/check I/O to the 520 bytes per block drive.*/
    status = fbe_test_io_run_write_read_check_test(edge_tap_520_io_case_array,
                                                   obj_id,
                                                   /* No max case to run.
                                                    * Just run all the test cases.  
                                                    */
                                                   FBE_TEST_IO_INVALID_CASE_COUNT);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s:  write/read/check test to 520 byte per block drive passed. ===", __FUNCTION__);

    /* Hook was connected, so hook received packet count should be the number of packets generated base on edge_tap_520_io_case_array. */
    /* TODO: 2388 is a hack, should be replaced with the packet counts calculated from the edge_tap_520_io_case_array*/
    MUT_ASSERT_INT_EQUAL(2388, edge_hook_received_packet_count);

    /* get the physical drive object id */
    status = fbe_api_object_map_interface_get_physical_drive_obj_id(0, 0, 0, &obj_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* remove the hook on physical drive ssp edge*/
    hook_info.hook = NULL;
    status = fbe_api_set_ssp_edge_hook (obj_id, &hook_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: successfully removed edge hook to object %d. ===\n", __FUNCTION__, obj_id);

    /* get the logical drive object id */
    status = fbe_api_object_map_interface_get_logical_drive_obj_id(0, 0, 0, &obj_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Set the global counter to 0*/
    edge_hook_received_packet_count = 0;

    /* Kick off write/read/check I/O to the 520 bytes per block drive.*/
    status = fbe_test_io_run_write_read_check_test(edge_tap_520_io_case_array,
                                                   obj_id,
                                                   /* No max case to run.
                                                    * Just run all the test cases.  
                                                    */
                                                   FBE_TEST_IO_INVALID_CASE_COUNT);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s:  write/read/check test to 520 byte per block drive passed. ===", __FUNCTION__);

    /* Hook was disconnected, so hook received packet count should not change. */
    MUT_ASSERT_INT_EQUAL(0, edge_hook_received_packet_count);
}

/* Packets will redirect to this function, where packets are filtered and processed here
 * things we can do here are:
 * - change the content of a packet and forward it on.
 * - change the content of a packet and complete it.
 * - change the completion function, so the packet can be intercepted on the way back.
 * - add packet to a queue so we can add delay or change order of execution.
 */
static fbe_status_t set_hook_test_hook_function(void * packet)
{
    fbe_status_t status;

    /* Count the packets */
    edge_hook_received_packet_count++;
    mut_printf(MUT_LOG_TEST_STATUS, "*** %s: receives %d packets. ***", __FUNCTION__, edge_hook_received_packet_count);

    /* send the io packet */
    status = fbe_topology_send_io_packet(packet);
    return status;
}

void edge_tap_set_hook_on_sas_enclosure_test(void)
{
    fbe_status_t status;
    fbe_object_id_t obj_id;

    fbe_api_set_edge_hook_t 	hook_info;

    /* get the enclosure object id */
    status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 0, &obj_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* set the hook on to enclosure ssp edge*/
    hook_info.hook = set_hook_test_hook_function;
    status = fbe_api_set_ssp_edge_hook (obj_id, &hook_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: successfully set edge hook to object %d. ===\n", __FUNCTION__, obj_id);

    /* Set the global counter to 0*/
    edge_hook_received_packet_count = 0;

    /* enclosure does polls once a while should we should be able to capture some io in 15 secs*/
    EmcutilSleep(15000);
    /* Hook was connected, so hook received packet count should be the number of packets generated base on edge_tap_520_io_case_array. */
    MUT_ASSERT_INT_NOT_EQUAL(0, edge_hook_received_packet_count);

    /* get the enclosure object id */
    status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 0, &obj_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* remove the hook on enclosure ssp edge*/
    hook_info.hook = NULL;
    status = fbe_api_set_ssp_edge_hook (obj_id, &hook_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: successfully removed edge hook to object %d. ===\n", __FUNCTION__, obj_id);

    /* Set the global counter to 0*/
    edge_hook_received_packet_count = 0;

    /* enclosure does polls once a while should we should be able to capture some io in 15 secs*/
    EmcutilSleep(15000);
    /* Hook was disconnected, so hook received packet count should not change. */
    MUT_ASSERT_INT_EQUAL(0, edge_hook_received_packet_count);
}

