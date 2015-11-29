/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * zeus_ready_test_common_utils.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains common functions that were called from different 
 *  zeus ready test scenarios. (home, republica_dominicana, wallis_pond, etc.)
 * 
 * HISTORY
 *   09/09/2008:  Created. ArunKumar Selvapillai
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "physical_package_tests.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe_discovery_transport.h"
#include "fbe_ssp_transport.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe_test_common_utils.h"
#include "sep_tests.h"
#include "pp_utils.h"


/*************************
 *   MACRO DEFINITIONS
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/****************************************************************
 * fbe_zrt_wait_enclosure_status()
 ****************************************************************
 * Description:
 *  Function to get the existence and state of the enclosure.
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  08/01/2008 - Created. Arunkumar Selvapillai
 *  09/09/2008 - Moved to a common file to remove redudancy. AS
 *
 ****************************************************************/

fbe_status_t fbe_zrt_wait_enclosure_status (fbe_u32_t port, fbe_u32_t encl, 
                                          fbe_lifecycle_state_t lifecycle_state,
                                          fbe_u32_t timeout)
{
    fbe_status_t        status;
    fbe_u32_t           object_handle;
    fbe_port_number_t   port_number;

    /* Verify if the enclosure exists */
	object_handle = FBE_OBJECT_ID_INVALID;
	while(object_handle == FBE_OBJECT_ID_INVALID) {
		status = fbe_api_get_enclosure_object_id_by_location(port, encl, &object_handle);
		MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
		if(object_handle == FBE_OBJECT_ID_INVALID){
			mut_printf(MUT_LOG_LOW, "No ENCL %d_%d\n", port, encl);
			fbe_api_sleep(1000);
		}
	}

    MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

    /* Use the handle to get lifecycle state of the enclosure */
    status = fbe_api_wait_for_object_lifecycle_state (object_handle, lifecycle_state, timeout,FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Use the handle to get port number, as an additional check*/
    status = fbe_api_get_object_port_number (object_handle, &port_number);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(port_number == port);

    mut_printf(MUT_LOG_LOW, "Verifying enclosure %d existance and state....Passed", encl);
    return FBE_STATUS_OK;
}


/****************************************************************
 * zrt_verify_edge_info_of_removed_enclosure()
 ****************************************************************
 * Description:
 *  Function to get the state of the discovery edge and the 
 *  protocol edge of the enclosure we removed.
 *
 *  - Discovery edge should be marked NOT_PRESENT and 
 *  - Protocol edge should be marked CLOSED
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  08/20/2008 - Created. Arunkumar Selvapillai
 *  09/09/2008 - Moved to a common file to remove redudancy. AS
 *
 ****************************************************************/

fbe_status_t fbe_zrt_verify_edge_info_of_logged_out_enclosure(fbe_u32_t port, fbe_u32_t encl)
{
    fbe_status_t        edge_status;
    fbe_u32_t           edge_handle;
    fbe_port_number_t   port_number;
    
    /* Get the handle for discovery and Protocol edge */
    edge_status = fbe_api_get_enclosure_object_id_by_location(port, encl, &edge_handle);
    MUT_ASSERT_TRUE(edge_status == FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(edge_handle, FBE_OBJECT_ID_INVALID);

    /* Verify if the discovery edge contains the NOT_PRESENT attribute set */        
    edge_status = fbe_api_wait_for_discovery_edge_attr(edge_handle, FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT, 3000);
    MUT_ASSERT_TRUE(edge_status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Discovery edge is set to NOT_PRESENT for encl %d", encl);

    /* Verify if the Protocol edge contains the CLOSED attribute set */        
    edge_status = fbe_api_wait_for_protocol_edge_attr(edge_handle, FBE_SSP_PATH_ATTR_CLOSED, 3000);
    MUT_ASSERT_TRUE(edge_status == FBE_STATUS_OK);
 
    mut_printf(MUT_LOG_LOW, "Protocol edge is set to CLOSED for encl %d", encl);

    /* Use the handle to get port number, as an additional check*/
    edge_status = fbe_api_get_object_port_number(edge_handle, &port_number);
    MUT_ASSERT_TRUE(edge_status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(port_number == port);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_test_physical_package_tests_config_unload()
 ****************************************************************
 * @brief
 *  Common unload for Physical Package test suite.  
 *
 * @param none.
 *
 * @return fbe_status_t - FBE_STATUS_OK/FBE_STATUS_GENERIC_FAILURE.
 *
 * HISTORY:
 *  8/31/2009 - Created. VG
 *
 ****************************************************************/
fbe_status_t fbe_test_physical_package_tests_config_unload(void)
{
    fbe_status_t status;
    fbe_test_package_list_t package_list;
	
    fbe_zero_memory(&package_list, sizeof(package_list));

    mut_printf(MUT_LOG_LOW, "=== %s starts ===\n", __FUNCTION__);

    package_list.number_of_packages = 1;
    package_list.package_list[0] = FBE_PACKAGE_ID_PHYSICAL;

    fbe_test_common_util_package_destroy(&package_list);
    status = fbe_api_terminator_destroy();
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "destroy terminator api failed");

    fbe_test_common_util_verify_package_destroy_status(&package_list);
    fbe_test_common_util_verify_package_trace_error(&package_list);

    mut_printf(MUT_LOG_LOW, "=== %s completes ===\n", __FUNCTION__);

    return status;
}

/*!**************************************************************
 *          physical_package_test_load_physical_package_and_neit()
 ****************************************************************
 * @brief
 *  Common load for Physical Package test suite.  
 *
 * @param   None
 *
 * @return  None
 *
 ****************************************************************/
void physical_package_test_load_physical_package_and_neit(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_neit_package_load_params_t neit_params;

    mut_printf(MUT_LOG_LOW, "=== Initializing Terminator ===\n");
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_terminator_set_io_mode(FBE_TERMINATOR_IO_MODE_ENABLED);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_test_load_simple_config();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== starting FBE API ===\n");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== starting Physical Package ===\n");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== set Physical Package entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== starting NEIT package ===\n");
    neit_config_fill_load_params(&neit_params);
    neit_params.load_raw_mirror = FBE_FALSE;
    status = fbe_api_sim_server_load_package_params(FBE_PACKAGE_ID_NEIT, &neit_params, sizeof(neit_params));
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== set NEIT package entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_test_pp_util_set_terminator_drive_debug_flags(0 /* no default */);

    return;
}
/************************************************************
 * end physical_package_test_load_physical_package_and_neit()
 ************************************************************/

/*!**************************************************************
 *          physical_package_test_load_physical_package_and_neit_both_sps()
 ****************************************************************
 * @brief
 *  Common load for Physical Package test suite.  
 *
 * @param   None
 *
 * @return  None
 *
 ****************************************************************/
void physical_package_test_load_physical_package_and_neit_both_sps(void)
{
    mut_printf(MUT_LOG_LOW, "=== Loading physical package and neit on SPA ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    physical_package_test_load_physical_package_and_neit();

    mut_printf(MUT_LOG_LOW, "=== Loading physical package and neit on SPB ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    physical_package_test_load_physical_package_and_neit();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    return;
}
/**********************************************************************
 * end physical_package_test_load_physical_package_and_neit_both_sps()
 *********************************************************************/

/*!**************************************************************
 *          physical_package_test_unload_physical_package_and_neit()
 ****************************************************************
 * @brief
 *  Common unload for Physical Package test suite.  
 *
 * @param none.
 *
 * @return none
 *
 * HISTORY:
 *  8/31/2009 - Created. VG
 *
 ****************************************************************/
void physical_package_test_unload_physical_package_and_neit(void)
{
    fbe_status_t            status;
    fbe_test_package_list_t package_to_unload;

    mut_printf(MUT_LOG_LOW, "=== %s starts ===\n", __FUNCTION__);

    package_to_unload.number_of_packages = 2;
    package_to_unload.package_list[0] = FBE_PACKAGE_ID_NEIT;
    package_to_unload.package_list[1] = FBE_PACKAGE_ID_PHYSICAL;

    fbe_test_common_util_package_destroy(&package_to_unload);
    status = fbe_api_terminator_destroy();
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "destroy terminator api failed");

    fbe_test_common_util_verify_package_destroy_status(&package_to_unload);
    fbe_test_common_util_verify_package_trace_error(&package_to_unload);

    mut_printf(MUT_LOG_LOW, "=== %s completes ===\n", __FUNCTION__);

    return;
}
/************************************************************
 * end physical_package_test_unload_physical_package_and_neit()
 ************************************************************/

/*!**************************************************************
 *          physical_package_test_unload_physical_package_and_neit_both_sps()
 ****************************************************************
 * @brief
 *  Common unload for Physical Package test suite.  
 *
 * @param   None
 *
 * @return  None
 *
 ****************************************************************/
void physical_package_test_unload_physical_package_and_neit_both_sps(void)
{
    mut_printf(MUT_LOG_LOW, "=== Unloading physical package and neit on SPA ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    physical_package_test_unload_physical_package_and_neit();

    mut_printf(MUT_LOG_LOW, "=== Unloading physical package and neit on SPB ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    physical_package_test_unload_physical_package_and_neit();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    return;
}
/**********************************************************************
 * end physical_package_test_unload_physical_package_and_neit_both_sps()
 *********************************************************************/


/************************************************
 * end file physical_package_test_common_utils.c
 ************************************************/
