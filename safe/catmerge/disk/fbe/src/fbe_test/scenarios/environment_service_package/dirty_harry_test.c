/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file dirty_harry_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify PS event notification to the ESP PS Management Object.
 * 
 * @version
 *   04/27/2010 - Created. Joe Perry
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "esp_tests.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_esp_ps_mgmt_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_ps_info.h"
#include "esp_tests.h"
#include "fbe_test_configurations.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * dirty_harry_short_desc = "Verify PS event notification to the ESP PS Management Object";
char * dirty_harry_long_desc ="\
\n\
Dirty Harry (aka Harry Callahan)\n\
        -lead character in movies DIRTY HARRY, THE ENFORCER, MAGNUM FORCE\n\
        -famous quote 'Do I feel lucky?  Well do you punk?'\n\
                      'Go ahead, make my day!'\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] 3 viper enclosure\n\
        [PP] 3 SAS drives/enclosure\n\
        [PP] 3 logical drives/enclosure\n\
\n\
STEP 1: Bring up the initial topology.\n\
        - Create and verify the initial physical package config.\n\
        - Start the foll. service in user space:\n\
        - 1. Memory Service\n\
        - 2. Scheduler Service\n\
        - 3. Eventlog service\n\
        - 4. FBE API \n\
        - Verify that the foll. classes are loaded and initialized\n\
        - 1. ERP\n\
        - 2. Enclosure Firmware Download\n\
        - 3. PS Management\n\
        - 4. Flexports\n\
        - 5. EIR\n\
STEP 2: Validate event notifications to PS Mgmt\n\
        - generate PS state change in Board Object\n\
        - verify that PS Mgmt gets notification from Board Object\n\
        - generate PS state change in an Enclosure Object\n\
        - verify that PS Mgmt gets notification from the specified Enclosure Object\n\
";


static fbe_object_id_t	expected_object_id = FBE_OBJECT_ID_INVALID;
#if FALSE
static fbe_notification_registration_id_t  reg_id;
#endif

static void dirty_harry_commmand_response_callback_function (update_object_msg_t * update_object_msg, 
                                                               void *context)
{
	fbe_semaphore_t 	*sem = (fbe_semaphore_t *)context;

	mut_printf(MUT_LOG_LOW, "=== callback, objId 0x%x, exp 0x%x ===\n",
        update_object_msg->object_id, expected_object_id);

	if (update_object_msg->object_id == expected_object_id) 
	{
	    if (update_object_msg->notification_info.notification_data.data_change_info.device_mask == FBE_DEVICE_TYPE_PS) 
	    {
    	    fbe_semaphore_release(sem, 0, 1 ,FALSE);
        }
	}
}

/*!**************************************************************
 * dirty_harry_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test 
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   03/09/2010 - Created. Joe Perry
 *
 ****************************************************************/

void dirty_harry_test(void)
{
    fbe_status_t                        status;
    fbe_device_physical_location_t      location = {0};
    fbe_u32_t                           psCount;
    fbe_esp_ps_mgmt_ps_info_t           psInfo;
    fbe_u32_t                           ps;
//    fbe_base_board_mgmt_set_ps_info_t   boardPsInfo;
//    fbe_object_id_t                     objectId;
//    fbe_api_terminator_device_handle_t  encl_device_id;
	fbe_semaphore_t 					sem;
//    DWORD   							dwWaitResult;
//    ses_stat_elem_ps_struct             ps_struct;
	fbe_u32_t                           encl;
    fbe_object_id_t                     object_id_list[FBE_MAX_PHYSICAL_OBJECTS];
    fbe_u32_t                           enclosure_count = 0;

    fbe_semaphore_init(&sem, 0, 1);

    /*
     * Verify initial PS info
     */
    // verify the initial PE PS Info
    fbe_set_memory(&psInfo, 0, sizeof(fbe_esp_ps_mgmt_ps_count_t));
    
    location.bus = FBE_XPE_PSEUDO_BUS_NUM;
    location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;

    status = fbe_api_esp_ps_mgmt_getPsCount(&location, &psCount);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(psCount <= FBE_ESP_PS_MAX_COUNT_PER_ENCL);

    psInfo.location = location;

    for (ps = 0; ps < psCount; ps++)
    {
        psInfo.location.slot = ps;
        status = fbe_api_esp_ps_mgmt_getPsInfo(&psInfo);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        mut_printf(MUT_LOG_LOW, "ps %d, psInserted %d, fault %d, acFail %d\n", 
            ps,
            psInfo.psInfo.bInserted,
            psInfo.psInfo.generalFault,
            psInfo.psInfo.ACFail);
        MUT_ASSERT_TRUE(psInfo.psInfo.bInserted);
        MUT_ASSERT_FALSE(psInfo.psInfo.generalFault);
        MUT_ASSERT_FALSE(psInfo.psInfo.ACFail);
    }
    mut_printf(MUT_LOG_LOW, "***** Verified initial PE PS's *****\n");

    // verify the initial DAE PS Info
    status = fbe_api_get_all_enclosure_object_ids(&object_id_list[0], FBE_MAX_PHYSICAL_OBJECTS, &enclosure_count);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    for (encl = 0; encl < enclosure_count; encl++)
    {
        fbe_set_memory(&psInfo, 0, sizeof(fbe_esp_ps_mgmt_ps_info_t));
    	status = fbe_api_get_object_port_number (object_id_list[encl], 
                                                 (fbe_port_number_t *)&(location.bus));
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	    status = fbe_api_get_object_enclosure_number (object_id_list[encl], 
                                                      (fbe_enclosure_number_t *)&(location.enclosure));
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        status = fbe_api_esp_ps_mgmt_getPsCount(&location, &psCount);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(psCount <= FBE_ESP_PS_MAX_COUNT_PER_ENCL);

        psInfo.location = location;
        for (ps = 0; ps < psCount; ps++)
        {
            psInfo.location.slot = ps;
            status = fbe_api_esp_ps_mgmt_getPsInfo(&psInfo);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            mut_printf(MUT_LOG_LOW, "%d_%d: ps %d, psInserted %d, fault %d, acFail %d\n", 
                psInfo.location.bus,
                psInfo.location.enclosure,
                ps,
                psInfo.psInfo.bInserted,
                psInfo.psInfo.generalFault,
                psInfo.psInfo.ACFail);
            MUT_ASSERT_TRUE(psInfo.psInfo.bInserted);
            MUT_ASSERT_FALSE(psInfo.psInfo.generalFault);
            MUT_ASSERT_FALSE(psInfo.psInfo.ACFail);
        }
    }
    mut_printf(MUT_LOG_LOW, "***** Verified initial DAE PS's *****\n"); 

    
// JAP - disable for now
#if FALSE
    // Get Board Object ID
    status = fbe_api_get_board_object_id(&objectId);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(objectId != FBE_OBJECT_ID_INVALID);

    expected_object_id = objectId;

    // Initialize to get event notification
	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
																  FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL,
																  FBE_TOPOLOGY_OBJECT_TYPE_BOARD,
																  dirty_harry_commmand_response_callback_function,
																  &sem,
																  &reg_id);
    /*
     * Test processing of PE PS's (interactions with Board Object)
     */
    // change PS Info in Board Object
    psInfo.location.slot = 0;
    status = fbe_api_esp_ps_mgmt_getPsInfo(&psInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "ps %d, psInserted %d, fault %d, acFail %d\n", 
        0,
        psInfo.psInfo.bInserted,
        psInfo.psInfo.generalFault,
        psInfo.psInfo.ACFail);
    boardPsInfo.psIndex = 0;
    boardPsInfo.psInfo = psInfo.psInfo;
    boardPsInfo.psInfo.generalFault = FBE_MGMT_STATUS_TRUE;
    mut_printf(MUT_LOG_LOW, "Change PS Info, ps %d, psInserted %d, fault %d, acFail %d\n", 
               boardPsInfo.psIndex,
               boardPsInfo.psInfo.bInserted,
               boardPsInfo.psInfo.generalFault,
               boardPsInfo.psInfo.ACFail);
    status = fbe_api_board_set_ps_info(objectId, &boardPsInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    // Verify that ESP ps_mgmt detects PS Info change
	dwWaitResult = fbe_semaphore_wait_ms(&sem, 10000);
	MUT_ASSERT_TRUE(dwWaitResult != EMCPAL_STATUS_TIMEOUT);

    fbe_api_sleep (1000);           // delay so ps_mgmt can process the same event
    psInfo.location.slot = ps;
    status = fbe_api_esp_ps_mgmt_getPsInfo(&psInfo);
    mut_printf(MUT_LOG_LOW, "ESP PS Info, ps %d, psInserted %d, fault %d, acFail %d\n", 
               boardPsInfo.psIndex,
                psInfo.psInfo.bInserted,
                psInfo.psInfo.generalFault,
                psInfo.psInfo.ACFail);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(psInfo.psInfo.bInserted,
                         boardPsInfo.psInfo.bInserted);
    MUT_ASSERT_INT_EQUAL(psInfo.psInfo.generalFault,
                         boardPsInfo.psInfo.generalFault);
    MUT_ASSERT_INT_EQUAL(psInfo.psInfo.ACFail,
                         boardPsInfo.psInfo.ACFail);
    mut_printf(MUT_LOG_LOW, "***** Verified  PE PS Event Notification *****\n");

    /*
     * Test processing of DAE PS's (interactions with Enclosure Object)
     */
    // get current PS info enclosure 0_0
    status = fbe_api_terminator_get_enclosure_handle(0, 0, &encl_device_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_terminator_get_ps_eses_status(encl_device_id, PS_0, &ps_struct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // set AC_FAIL fault
    ps_struct.ac_fail = TRUE;
    status = fbe_api_terminator_set_ps_eses_status(encl_device_id, PS_0, ps_struct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    // Verify that ESP ps_mgmt detects PS Info change
	dwWaitResult = fbe_semaphore_wait_ms(&sem, 10000);
	MUT_ASSERT_TRUE(dwWaitResult != EMCPAL_STATUS_TIMEOUT);

    fbe_api_sleep (1000);           // delay so ps_mgmt can process the same event
    psInfo.location.bus = 0;
    psInfo.location.enclosure = 0;
    psInfo.location.slot = PS_0;
    status = fbe_api_esp_ps_mgmt_getPsInfo(&psInfo);
    mut_printf(MUT_LOG_LOW, "ESP PS Info, ps %d, psInserted %d, fault %d, acFail %d\n", 
               PS_0,
               psInfo.psInfo.bInserted,
               psInfo.psInfo.generalFault,
               psInfo.psInfo.ACFail);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(psInfo.psInfo.bInserted);
    MUT_ASSERT_FALSE(psInfo.psInfo.generalFault);
    MUT_ASSERT_TRUE(psInfo.psInfo.ACFail);

    // get current PS info enclosure 1_2
    status = fbe_api_terminator_get_enclosure_handle(1, 2, &encl_device_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_terminator_get_ps_eses_status(encl_device_id, PS_1, &ps_struct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // set AC_FAIL fault
    ps_struct.ac_fail = TRUE;
    status = fbe_api_terminator_set_ps_eses_status(encl_device_id, PS_1, ps_struct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    // Verify that ESP ps_mgmt detects PS Info change
	dwWaitResult = fbe_semaphore_wait_ms(&sem, 10000);
	MUT_ASSERT_TRUE(dwWaitResult != EMCPAL_STATUS_TIMEOUT);

    fbe_api_sleep (1000);           // delay so ps_mgmt can process the same event
    psInfo.location.bus = 1;
    psInfo.location.enclosure = 2;
    psInfo.location.slot = PS_1;
    status = fbe_api_esp_ps_mgmt_getPsInfo(&psInfo);
    mut_printf(MUT_LOG_LOW, "ESP PS Info, ps %d, psInserted %d, fault %d, acFail %d\n", 
               PS_1,
               psInfo.psInfo.bInserted,
               psInfo.psInfo.generalFault,
               psInfo.psInfo.ACFail);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(psInfo.psInfo.bInserted);
    MUT_ASSERT_FALSE(psInfo.psInfo.generalFault);
    MUT_ASSERT_TRUE(psInfo.psInfo.ACFail);

    mut_printf(MUT_LOG_LOW, "***** Verified  DAE PS Event Notification *****\n");

    // cleanup
    status = fbe_api_notification_interface_unregister_notification(dirty_harry_commmand_response_callback_function,
                                                                    reg_id);
#endif
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&sem);

    return;
}
/******************************************
 * end dirty_harry_test()
 ******************************************/
/*!**************************************************************
 * dirty_harry_setup()
 ****************************************************************
 * @brief
 *  Setup for filling the interface structure
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   03/09/2010 - Created. Joe Perry
 *
 ****************************************************************/
void dirty_harry_setup(void)
{
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_LAPA_RIOS_CONIG,
                                        SPID_UNKNOWN_HW_TYPE,
                                        NULL,
                                        NULL);
}
/**************************************
 * end dirty_harry_setup()
 **************************************/

/*!**************************************************************
 * dirty_harry_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the dirty_harry test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   03/09/2010 - Created. Joe Perry
 *
 ****************************************************************/

void dirty_harry_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
/******************************************
 * end dirty_harry_cleanup()
 ******************************************/
/*************************
 * end file dirty_harry_test.c
 *************************/


