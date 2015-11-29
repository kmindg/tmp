#include "mut.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe_trace.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_sep.h"
#include "fbe_base_object.h"
#include "fbe/fbe_transport.h"
#include "fbe_transport_memory.h"
#include "fbe_base_package.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_raid_group.h"
#include "fbe/fbe_lun.h"
#include "fbe/fbe_base_config.h"
#include "fbe_test_io_api.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_emcutil_shell_include.h"

/* Peter Puhov fbe api needs this pointers */
fbe_service_control_entry_t physical_package_control_entry = NULL;
fbe_io_entry_function_t		physical_package_io_entry = NULL;

void sep_sim_setup(void)
{
    fbe_status_t status;
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Start ...\n", __FUNCTION__);
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_DEBUG_HIGH);

	/* Peter Puhov fbe api needs this pointers */
	physical_package_control_entry = fbe_service_manager_send_control_packet;
	physical_package_io_entry = fbe_topology_send_io_packet;

    status = sep_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Success! \n", __FUNCTION__);

    mut_printf(MUT_LOG_LOW, "=== starting fbe_api ===\n");
    fbe_api_common_init_sim();
	fbe_api_set_simulation_io_and_control_entries(FBE_PACKAGE_ID_PHYSICAL, physical_package_io_entry, physical_package_control_entry);

	return;
}
void sep_sim_teardown(void)
{
    fbe_status_t status;
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Start ...\n", __FUNCTION__);

    /* destroy the fbe_api first*/
    status = fbe_api_common_destroy_sim();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = sep_destroy();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Success! \n", __FUNCTION__);
    return;
}

static fbe_status_t 
sep_sem_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;

    fbe_semaphore_release(sem, 0, 1, FALSE);

    return FBE_STATUS_OK;
}

fbe_status_t 
sep_create_object(fbe_class_id_t class_id,
                  fbe_object_id_t *object_id_p)
{    
    fbe_packet_t * packet;
    fbe_base_object_mgmt_create_object_t base_object_mgmt_create_object;
    fbe_semaphore_t sem;
    fbe_status_t status;

    fbe_semaphore_init(&sem, 0, 1);

    /* Allocate packet */
    packet = fbe_transport_allocate_packet();
    if(packet == NULL) {
        fbe_base_package_trace( FBE_PACKAGE_ID_SEP_0,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fbe_transport_allocate_packet failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    fbe_transport_initialize_packet(packet);
    fbe_base_object_mgmt_create_object_init(&base_object_mgmt_create_object,
                                            class_id, 
                                            FBE_OBJECT_ID_INVALID);

    fbe_transport_build_control_packet(packet, 
                                       FBE_TOPOLOGY_CONTROL_CODE_CREATE_OBJECT,
                                       &base_object_mgmt_create_object,
                                       sizeof(fbe_base_object_mgmt_create_object_t),
                                       sizeof(fbe_base_object_mgmt_create_object_t),
                                       sep_sem_completion,
                                       &sem);

    /* Set packet address */
    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_SEP_0,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                FBE_OBJECT_ID_INVALID);

    fbe_service_manager_send_control_packet(packet);

    fbe_semaphore_wait(&sem, NULL);

    status = fbe_transport_get_status_code(packet);
    if (status != FBE_STATUS_OK) {
        fbe_base_package_trace( FBE_PACKAGE_ID_SEP_0,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: can't create mirror object, status: %X\n",
                                __FUNCTION__, status);
        return status;
    }
    *object_id_p = base_object_mgmt_create_object.object_id;
    fbe_semaphore_destroy(&sem);
    fbe_transport_release_packet(packet);
    return FBE_STATUS_OK;
}

fbe_status_t 
sep_set_edge(fbe_object_id_t object_id,
                      fbe_u32_t index,
                      fbe_object_id_t server_id)
{    
    fbe_packet_t * packet;
	fbe_block_transport_control_create_edge_t create_edge;
    fbe_semaphore_t sem;
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_operation_t *control_p = NULL;

    fbe_semaphore_init(&sem, 0, 1);

    /* Allocate packet */
    packet = fbe_transport_allocate_packet();
    if(packet == NULL) {
        fbe_base_package_trace( FBE_PACKAGE_ID_SEP_0,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fbe_transport_allocate_packet failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    fbe_transport_initialize_sep_packet(packet);
    sep_payload_p = fbe_transport_get_payload_ex(packet);

    control_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);

	create_edge.server_id = server_id;
	create_edge.client_index = index;
	create_edge.capacity = 0x500; 
	create_edge.offset = 0;

    fbe_payload_control_build_operation(control_p,
                                        FBE_BLOCK_TRANSPORT_CONTROL_CODE_CREATE_EDGE,
                                        &create_edge,
                                        sizeof(fbe_block_transport_control_create_edge_t));

    fbe_transport_set_completion_function(packet, sep_sem_completion, &sem);

    /* Set packet address */
    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_SEP_0,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                object_id);

    fbe_service_manager_send_control_packet(packet);

    fbe_semaphore_wait(&sem, NULL);

    fbe_semaphore_destroy(&sem);
    fbe_transport_release_packet(packet);
    return FBE_STATUS_OK;
}

fbe_status_t sep_set_raid_configuration(fbe_object_id_t object_id,
                                        fbe_u32_t width,
                                        fbe_lba_t capacity,
                                        fbe_raid_group_type_t raid_type)
{    
    fbe_packet_t * packet;
    fbe_raid_group_configuration_t set_configuration;
    fbe_semaphore_t sem;
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_operation_t *control_p = NULL;

    fbe_semaphore_init(&sem, 0, 1);

    /* Allocate packet */
    packet = fbe_transport_allocate_packet();
    if(packet == NULL) {
        fbe_base_package_trace( FBE_PACKAGE_ID_SEP_0,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fbe_transport_allocate_packet failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    fbe_transport_initialize_sep_packet(packet);

    set_configuration.width = width;
    set_configuration.capacity = capacity;
    set_configuration.chunk_size = (8 * FBE_RAID_SECTORS_PER_ELEMENT);
    set_configuration.raid_type = raid_type;
 set_configuration.debug_flags = 0;
 set_configuration.power_saving_enabled = FBE_FALSE;

    sep_payload_p = fbe_transport_get_payload_ex(packet);

    control_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);

    fbe_payload_control_build_operation(control_p,
                                        FBE_RAID_GROUP_CONTROL_CODE_SET_CONFIGURATION,
                                        &set_configuration,
                                        sizeof(fbe_raid_group_configuration_t));

    fbe_transport_set_completion_function(packet, sep_sem_completion,  &sem);

    /* Set packet address */
    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_SEP_0,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                object_id);

    fbe_service_manager_send_control_packet(packet);

    fbe_semaphore_wait(&sem, NULL);

    fbe_semaphore_destroy(&sem);
    fbe_transport_release_packet(packet);
    return FBE_STATUS_OK;
}

fbe_status_t sep_set_lun_configuration(fbe_object_id_t object_id,
                                       fbe_lba_t capacity)
{    
    fbe_packet_t * packet;
    fbe_lun_set_configuration_t set_configuration;
    fbe_semaphore_t sem;
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_operation_t *control_p = NULL;

    fbe_semaphore_init(&sem, 0, 1);

    /* Allocate packet */
    packet = fbe_transport_allocate_packet();
    if(packet == NULL) {
        fbe_base_package_trace( FBE_PACKAGE_ID_SEP_0,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fbe_transport_allocate_packet failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    fbe_transport_initialize_sep_packet(packet);

    set_configuration.capacity = capacity;
	set_configuration.power_saving_enabled = FBE_FALSE;
    sep_payload_p = fbe_transport_get_payload_ex(packet);

    control_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);

    fbe_payload_control_build_operation(control_p,
                                        FBE_LUN_CONTROL_CODE_SET_CONFIGURATION,
                                        &set_configuration,
                                        sizeof(fbe_lun_set_configuration_t));

    fbe_transport_set_completion_function(packet, sep_sem_completion,  &sem);

    /* Set packet address */
    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_SEP_0,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                object_id);

    fbe_service_manager_send_control_packet(packet);

    fbe_semaphore_wait(&sem, NULL);

    fbe_semaphore_destroy(&sem);
    fbe_transport_release_packet(packet);
    return FBE_STATUS_OK;
}
fbe_status_t sep_destroy_object(fbe_object_id_t object_id)
{    
    fbe_packet_t * packet;
    fbe_semaphore_t sem;
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_operation_t *control_p = NULL;

    fbe_semaphore_init(&sem, 0, 1);

    /* Allocate packet */
    packet = fbe_transport_allocate_packet();
    if(packet == NULL) {
        fbe_base_package_trace( FBE_PACKAGE_ID_SEP_0,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fbe_transport_allocate_packet failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    fbe_transport_initialize_sep_packet(packet);

    sep_payload_p = fbe_transport_get_payload_ex(packet);

    control_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);

    fbe_payload_control_build_operation(control_p,
                                        FBE_BASE_OBJECT_CONTROL_CODE_SET_DESTROY_COND,
                                        NULL,
                                        0);

    fbe_transport_set_completion_function(packet, sep_sem_completion,  &sem);

    /* Set packet address */
    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_LOGICAL_0,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                object_id);

    fbe_service_manager_send_control_packet(packet);

    fbe_semaphore_wait(&sem, NULL);

    fbe_semaphore_destroy(&sem);
    fbe_transport_release_packet(packet);
    return FBE_STATUS_OK;
}

fbe_status_t 
sep_create_virtual_drive(fbe_object_id_t *object_id_p,
                         fbe_u32_t width,
                         fbe_lba_t capacity)
{ 
    fbe_status_t status;
    fbe_object_id_t object_id = FBE_OBJECT_ID_INVALID;

    *object_id_p = FBE_OBJECT_ID_INVALID;

    /* Create the unconfigured object.
     */
    status = sep_create_object(FBE_CLASS_ID_VIRTUAL_DRIVE, &object_id);

    if (status == FBE_STATUS_OK)
    {
        *object_id_p = object_id;
        /* Setup the config of the virtual drive.
         */
        status = sep_set_raid_configuration(object_id,
                                            2,    /* width    */
                                            0x5000, /* capacity */
                                            FBE_RAID_GROUP_TYPE_SPARE /* Raid group type */);

    }
    if (status == FBE_STATUS_OK)
    {
        /* Wait for the object to come ready.
         */
        status = fbe_api_wait_for_object_lifecycle_state(object_id,
                                                         FBE_LIFECYCLE_STATE_READY,
                                                         20000    /* Milliseconds to wait. */,
														 FBE_PACKAGE_ID_SEP_0);
    }
    return status;
}

fbe_status_t 
sep_create_mirror_object(fbe_object_id_t *object_id_p,
                         fbe_object_id_t *virtual_drive_id_p,
                         fbe_u32_t width)
{    
    fbe_status_t status;
    fbe_object_id_t mirror_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t index;

    *object_id_p = FBE_OBJECT_ID_INVALID;

    /* Next create the mirror.
     */
    status = sep_create_object(FBE_CLASS_ID_MIRROR, &mirror_object_id);

    if (status == FBE_STATUS_OK)
    {
        *object_id_p = mirror_object_id;

        /* Setup the config of the mirror.
         */
        status = sep_set_raid_configuration(mirror_object_id,
                                            width,    /* width    */
                                            0x5000, /* capacity */
                                            FBE_RAID_GROUP_TYPE_RAID1 /* Raid group type */);
    }

    for (index = 0; (index < width && status == FBE_STATUS_OK); index++)
    {
        /* Attach the edge to the virtual drives.
         */
        status = sep_set_edge(mirror_object_id,
                               index,    /* index */
                               virtual_drive_id_p[index]);
    }
    if (status == FBE_STATUS_OK)
    {
        status = fbe_api_wait_for_object_lifecycle_state(mirror_object_id,
                                                         FBE_LIFECYCLE_STATE_READY,
                                                         20000    /* Milliseconds to wait. */,
														 FBE_PACKAGE_ID_SEP_0);
    }
    return status;
}

fbe_status_t 
sep_create_lun_object(fbe_object_id_t *object_id_p,
                      fbe_object_id_t raid_group_object_id)
{    
    fbe_status_t status;
    fbe_object_id_t lun_object_id;

    *object_id_p = FBE_OBJECT_ID_INVALID;

    /* Next create the LUN.
     */
    status = sep_create_object(FBE_CLASS_ID_LUN, &lun_object_id);

    if (status == FBE_STATUS_OK)
    {
        *object_id_p = lun_object_id;

        /* Setup the config of the mirror.
         */
        status = sep_set_lun_configuration(lun_object_id,
                                           0x5000    /* capacity */);
    }
    if (status == FBE_STATUS_OK)
    {
        /* Attach the edge to the virtual drives.
         */
        status = sep_set_edge(lun_object_id,
                                  0,    /* index */
                                  raid_group_object_id);
    }
    if (status == FBE_STATUS_OK)
    {
        status = fbe_api_wait_for_object_lifecycle_state(lun_object_id,
                                                         FBE_LIFECYCLE_STATE_READY,
                                                         20000    /* Milliseconds to wait. */,
														 FBE_PACKAGE_ID_SEP_0);
    }
    return status;
}

/* sep_sim_test_io_test_case_array
 *
 * This structure defines all the io test cases.
 */
fbe_test_io_case_t sep_sim_test_io_test_case_array[] =
{
    {520,       520,        0,    0x100,   1,       0x100,       1,       1,        1},

    /* Insert new records here.
     */

    /* This is the terminator record, it always should be zero.
     */
    {((fbe_block_size_t) -1), 0,         0,    0,    0,       0, 0},

};
/* end sep_sim_test_io_test_case_array global structure */
void sep_sim_test(void)
{
    fbe_status_t status;
    fbe_object_id_t lun_object_id[3];
    fbe_object_id_t mirror_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t virtual_drive_id[2];

    status = sep_create_virtual_drive(&virtual_drive_id[0],
                                      2,     /* width    */
                                      0x5000 /* capacity */);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = sep_create_virtual_drive(&virtual_drive_id[1],
                                      2,     /* width    */
                                      0x5000 /* capacity */);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = sep_create_mirror_object(&mirror_object_id,
                                      &virtual_drive_id[0],
                                      2);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = sep_create_lun_object(&lun_object_id[0], mirror_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = sep_create_lun_object(&lun_object_id[1], mirror_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = sep_create_lun_object(&lun_object_id[2], mirror_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_test_io_set_use_sep(FBE_TRUE);
    status = fbe_test_io_run_read_only_test(&sep_sim_test_io_test_case_array[0],
                                            lun_object_id[0],
                                            1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_io_run_read_only_test(&sep_sim_test_io_test_case_array[0],
                                            lun_object_id[1],
                                            1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_io_run_read_only_test(&sep_sim_test_io_test_case_array[0],
                                            lun_object_id[2],
                                            1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Destroying Objects ...\n", __FUNCTION__);

    status = sep_destroy_object(lun_object_id[0]);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = sep_destroy_object(lun_object_id[1]);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = sep_destroy_object(lun_object_id[2]);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = sep_destroy_object(mirror_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = sep_destroy_object(virtual_drive_id[0]);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = sep_destroy_object(virtual_drive_id[1]);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Destroy complete.\n", __FUNCTION__);

    return;
}
__cdecl main (int argc , char **argv)
{ 
    mut_testsuite_t *sep_sim_test_suite;                /* pointer to testsuite structure */

  #include "fbe/fbe_emcutil_shell_maincode.h"
 
    mut_init(argc, argv);                               /* before proceeding we need to initialize MUT infrastructure */

    sep_sim_test_suite = MUT_CREATE_TESTSUITE("sep_sim_test_suite");   /* testsuite is created */
    
    MUT_ADD_TEST(sep_sim_test_suite, sep_sim_test, sep_sim_setup, sep_sim_teardown);
    MUT_RUN_TESTSUITE(sep_sim_test_suite);
    return;
}


