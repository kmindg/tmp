#include "fbe_api_user_interface_test.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_object_map_interface.h"
#include "fbe/fbe_transport.h"

void fbe_api_get_object_type_test(void)
{
	fbe_u32_t object_handle_p;
	fbe_status_t status;
	fbe_topology_object_type_t type;

	status = fbe_api_object_map_interface_get_port_obj_id(0, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(object_handle_p == 1);

	fbe_api_get_object_type(object_handle_p, &type);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(type == FBE_TOPOLOGY_OBJECT_TYPE_PORT);

	status = fbe_api_object_map_interface_get_port_obj_id(1, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_INT_EQUAL(object_handle_p, FBE_OBJECT_ID_INVALID);

	status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 0, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_INT_NOT_EQUAL(object_handle_p, FBE_OBJECT_ID_INVALID);
    
    fbe_api_get_object_type(object_handle_p, &type);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(type == FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE);

	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Verify fbe_api_get_object_type successfully!!! ===\n");
	return;
}

void fbe_api_get_object_class_id_test(void)
{
	fbe_u32_t object_handle_p;
	fbe_status_t status;
	fbe_class_id_t class_id;

	status = fbe_api_object_map_interface_get_port_obj_id(0, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(object_handle_p == 1);

	status = fbe_api_get_object_class_id(object_handle_p, &class_id);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_SAS_PMC_PORT);

	status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 0, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_INT_NOT_EQUAL(object_handle_p, FBE_OBJECT_ID_INVALID);

	status = fbe_api_get_object_class_id(object_handle_p, &class_id);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_SAS_VIPER_ENCLOSURE);

	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Verify fbe_api_get_object_class_id successfully!!! ===\n");
	return;
}

void fbe_api_get_object_port_number_test(void)
{
	fbe_u32_t object_handle_p;
	fbe_status_t status;
	fbe_port_number_t port_number;

	status = fbe_api_object_map_interface_get_port_obj_id(0, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(object_handle_p == 1);

	status = fbe_api_get_object_port_number(object_handle_p, &port_number);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(port_number == 0);

	status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 0, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_INT_NOT_EQUAL(object_handle_p, FBE_OBJECT_ID_INVALID);

	status = fbe_api_get_object_port_number(object_handle_p, &port_number);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(port_number == 0);

	status = fbe_api_object_map_interface_get_physical_drive_obj_id(0, 0, 0, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_INT_NOT_EQUAL(object_handle_p, FBE_OBJECT_ID_INVALID);

	status = fbe_api_get_object_port_number(object_handle_p, &port_number);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(port_number == 0);

	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Verify fbe_api_get_object_class_id successfully!!! ===\n");
	return;
}

void fbe_api_get_object_lifecycle_state_test(void)
{
	fbe_u32_t object_handle_p;
	fbe_status_t status;
	fbe_lifecycle_state_t lifecycle_state;

	status = fbe_api_object_map_interface_get_port_obj_id(0, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(object_handle_p == 1);

	status = fbe_api_get_object_lifecycle_state(object_handle_p, &lifecycle_state, FBE_PACKAGE_ID_PHYSICAL);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(lifecycle_state != FBE_LIFECYCLE_STATE_INVALID);

	status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 0, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_INT_NOT_EQUAL(object_handle_p, FBE_OBJECT_ID_INVALID);

	status = fbe_api_get_object_lifecycle_state(object_handle_p, &lifecycle_state, FBE_PACKAGE_ID_PHYSICAL);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(lifecycle_state != FBE_LIFECYCLE_STATE_INVALID);

	status = fbe_api_object_map_interface_get_physical_drive_obj_id(0, 0, 0, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_INT_NOT_EQUAL(object_handle_p, FBE_OBJECT_ID_INVALID);

	status = fbe_api_get_object_lifecycle_state(object_handle_p, &lifecycle_state, FBE_PACKAGE_ID_PHYSICAL);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(lifecycle_state != FBE_LIFECYCLE_STATE_INVALID);

	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Verify fbe_api_get_object_class_id successfully!!! ===\n");
	return;
}

void fbe_api_topology_enumerate_objects_test(void)
{
	fbe_status_t status;
	fbe_object_id_t	topology_mgmt_enumerate_objects[FBE_MAX_PHYSICAL_OBJECTS];
	fbe_u32_t total =0;

	status = fbe_api_enumerate_objects(topology_mgmt_enumerate_objects, &total);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	/* 1 board + 1 port + 3 encl + 5 pdos + 5 ldos= 15*/
	MUT_ASSERT_TRUE(total == 15);

	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Verify topology_enumerate_objects successfully!!! ===\n");
	return;
}

void fbe_api_get_object_discovery_edge_info_test(void)
{
	fbe_u32_t object_handle_p;
	fbe_status_t status;
	fbe_api_get_discovery_edge_info_t edge_info;

	status = fbe_api_object_map_interface_get_port_obj_id(0, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(object_handle_p == 1);

    status = fbe_api_get_discovery_edge_info(object_handle_p, &edge_info);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 0, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_INT_NOT_EQUAL(object_handle_p, FBE_OBJECT_ID_INVALID);

    status = fbe_api_get_discovery_edge_info(object_handle_p, &edge_info);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	status = fbe_api_object_map_interface_get_physical_drive_obj_id(0, 0, 0, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_INT_NOT_EQUAL(object_handle_p, FBE_OBJECT_ID_INVALID);

    status = fbe_api_get_discovery_edge_info(object_handle_p, &edge_info);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Verify fbe_api_get_object_discovery_edge_info successfully!!! ===\n");
	return;
}

void fbe_api_get_object_ssp_transport_edge_info_test(void)
{
	fbe_u32_t 				object_handle_p;
	fbe_status_t 			status;
	fbe_api_get_ssp_edge_info_t ssp_transport_edge_info;

	/* Port does not have ssp edge, only enclosure and physical drive objects has it */
	status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 0, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_INT_NOT_EQUAL(object_handle_p, FBE_OBJECT_ID_INVALID);

    status = fbe_api_get_ssp_edge_info(object_handle_p, &ssp_transport_edge_info);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_INT_EQUAL(ssp_transport_edge_info.path_state, 1);
	MUT_ASSERT_INT_EQUAL(ssp_transport_edge_info.transport_id, FBE_TRANSPORT_ID_SSP);
	MUT_ASSERT_INT_EQUAL(ssp_transport_edge_info.client_id, object_handle_p);
	MUT_ASSERT_INT_EQUAL(ssp_transport_edge_info.path_attr, 0);

	status = fbe_api_object_map_interface_get_physical_drive_obj_id(0, 0, 0, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_INT_NOT_EQUAL(object_handle_p, FBE_OBJECT_ID_INVALID);

    status = fbe_api_get_ssp_edge_info(object_handle_p, &ssp_transport_edge_info);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_INT_EQUAL(ssp_transport_edge_info.path_state, 1);
	MUT_ASSERT_INT_EQUAL(ssp_transport_edge_info.transport_id, FBE_TRANSPORT_ID_SSP);
	MUT_ASSERT_INT_EQUAL(ssp_transport_edge_info.client_id, object_handle_p);
	MUT_ASSERT_INT_EQUAL(ssp_transport_edge_info.path_attr, 0);

	/*try some bad stuff*/
	status = fbe_api_get_ssp_edge_info(FBE_OBJECT_ID_INVALID, &ssp_transport_edge_info);
	MUT_ASSERT_TRUE(status != FBE_STATUS_OK);

	status = fbe_api_get_ssp_edge_info(object_handle_p, NULL);
	MUT_ASSERT_TRUE(status != FBE_STATUS_OK);


	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Verify fbe_api_get_object_ssp_transport_edge_info_test successfully!!! ===\n");
	return;
}

void fbe_api_get_object_block_edge_info_test(void)
{
	fbe_u32_t object_handle_p;
	fbe_status_t status;
	fbe_api_get_block_edge_info_t block_edge_info;

	/* only logical drive object supports block edge info */
	status = fbe_api_object_map_interface_get_logical_drive_obj_id(0, 0, 0, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_INT_NOT_EQUAL(object_handle_p, FBE_OBJECT_ID_INVALID);

    status = fbe_api_get_block_edge_info(object_handle_p, 0, &block_edge_info, FBE_PACKAGE_ID_PHYSICAL);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Verify fbe_api_get_object_block_edge_info_test successfully!!! ===\n");
	return;
}

void fbe_api_get_all_objects_in_state_test(void)
{
	fbe_status_t 	status;
	fbe_object_id_t	topology_mgmt_enumerate_objects[FBE_MAX_PHYSICAL_OBJECTS];
	fbe_u32_t 		total =0;

	status = fbe_api_get_all_objects_in_state(FBE_LIFECYCLE_STATE_READY, topology_mgmt_enumerate_objects, FBE_MAX_PHYSICAL_OBJECTS, &total);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/*we want to make sure that all of them are ready*/
	MUT_ASSERT_TRUE(total == 15);

	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Verify fbe_api_get_all_objects_in_state successfully!!! ===\n");
}

void fbe_api_get_physical_state_object_from_topology_test(void)
{
	fbe_lifecycle_state_t 	state;
	fbe_status_t 			status = FBE_LIFECYCLE_STATE_INVALID;
    
    status = fbe_api_get_physical_drive_object_state(0, 0, 0, &state);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_INT_NOT_EQUAL(state, FBE_LIFECYCLE_STATE_INVALID);
    
	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Verify fbe_api_get_physical_state_object_from_topology_test successfully!!! ===\n");
	return;
}

void fbe_api_get_logical_drive_state_from_topology_test(void)
{
	fbe_lifecycle_state_t 	state;
	fbe_status_t 			status = FBE_LIFECYCLE_STATE_INVALID;
    
    status = fbe_api_get_logical_drive_object_state(0, 0, 0, &state);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_INT_NOT_EQUAL(state, FBE_LIFECYCLE_STATE_INVALID);

	/*do some negative testing, slot 3 should fail since it has no drive*/
	status = fbe_api_get_logical_drive_object_state(0, 0, 3, &state);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);
	MUT_ASSERT_INT_EQUAL(state, FBE_LIFECYCLE_STATE_INVALID);
    
	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Verify fbe_api_get_logical_drive_state_from_topology_test successfully!!! ===\n");
	return;
}

void fbe_api_get_enclosure_state_from_topology_test(void)
{
	fbe_lifecycle_state_t 	state;
	fbe_status_t 			status = FBE_LIFECYCLE_STATE_INVALID;
    
    status = fbe_api_get_enclosure_object_state(0, 0, &state);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_INT_NOT_EQUAL(state, FBE_LIFECYCLE_STATE_INVALID);

	/*do some negative testing, slot 3 should fail since it does not exist*/
	status = fbe_api_get_enclosure_object_state(0, 3, &state);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);
	MUT_ASSERT_INT_EQUAL(state, FBE_LIFECYCLE_STATE_INVALID);
    
	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Verify fbe_api_get_enclosure_state_from_topology_test successfully!!! ===\n");
	return;
}
void fbe_api_get_port_state_from_topology_test(void)
{
	fbe_lifecycle_state_t 	state;
	fbe_status_t 			status = FBE_LIFECYCLE_STATE_INVALID;
    
    status = fbe_api_get_port_object_state(0,&state);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_INT_NOT_EQUAL(state, FBE_LIFECYCLE_STATE_INVALID);

	/*do some negative testing*/
	status = fbe_api_get_port_object_state(12, &state);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);
	MUT_ASSERT_INT_EQUAL(state, FBE_LIFECYCLE_STATE_INVALID);
    
	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Verify fbe_api_get_port_state_from_topology_test successfully!!! ===\n");
	return;
}

void fbe_api_get_object_stp_transport_edge_info_test(void)
{
	fbe_object_id_t 		object_handle_p;
	fbe_status_t 			status;
	fbe_api_get_stp_edge_info_t stp_transport_edge_info;

	/* Port and enclosure does not have stp edge only drive*/
    status = fbe_api_object_map_interface_get_physical_drive_obj_id(0, 2, 0, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_INT_NOT_EQUAL(object_handle_p, FBE_OBJECT_ID_INVALID);

	status = fbe_api_get_stp_edge_info(object_handle_p, &stp_transport_edge_info);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_INT_EQUAL(stp_transport_edge_info.path_state, 1);
	MUT_ASSERT_INT_EQUAL(stp_transport_edge_info.transport_id, FBE_TRANSPORT_ID_STP);
	MUT_ASSERT_INT_EQUAL(stp_transport_edge_info.client_id, object_handle_p);
	MUT_ASSERT_INT_EQUAL(stp_transport_edge_info.path_attr, 0);

	/*check some bad state*/
	status = fbe_api_get_stp_edge_info(FBE_OBJECT_ID_INVALID, &stp_transport_edge_info);
	MUT_ASSERT_TRUE(status != FBE_STATUS_OK);

	status = fbe_api_get_stp_edge_info(object_handle_p, NULL);
	MUT_ASSERT_TRUE(status != FBE_STATUS_OK);


	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Verify %s successfully!!! ===\n", __FUNCTION__);
	return;
}

void fbe_api_get_object_smp_transport_edge_info_test(void)
{
	fbe_object_id_t 		object_handle_p;
	fbe_status_t 			status;
	fbe_api_get_smp_edge_info_t smp_transport_edge_info;

	/* Port and drive does not have smp edge only enclosure*/
    status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 0, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_INT_NOT_EQUAL(object_handle_p, FBE_OBJECT_ID_INVALID);

	status = fbe_api_get_smp_edge_info(object_handle_p, &smp_transport_edge_info);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_INT_EQUAL(smp_transport_edge_info.path_state, 1);
	MUT_ASSERT_INT_EQUAL(smp_transport_edge_info.transport_id, FBE_TRANSPORT_ID_SMP);
	MUT_ASSERT_INT_EQUAL(smp_transport_edge_info.client_id, object_handle_p);
	MUT_ASSERT_INT_EQUAL(smp_transport_edge_info.path_attr, 0);

	/*check some bad state*/
	status = fbe_api_get_smp_edge_info(FBE_OBJECT_ID_INVALID, &smp_transport_edge_info);
	MUT_ASSERT_TRUE(status != FBE_STATUS_OK);

	status = fbe_api_get_smp_edge_info(object_handle_p, NULL);
	MUT_ASSERT_TRUE(status != FBE_STATUS_OK);


	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Verify %s successfully!!! ===\n", __FUNCTION__);
	return;
}
