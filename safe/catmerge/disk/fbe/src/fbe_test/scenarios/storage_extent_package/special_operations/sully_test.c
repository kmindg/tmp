/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file sully_test.c
 ***************************************************************************
 *
 * @brief
 *   This file contains tests for wear leveling
 *
 * @version
 *   06/30/2015 - Created. Deanna Heng
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"
#include "fbe_test_package_config.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_trace_interface.h"
#include "fbe/fbe_random.h"
#include "fbe_test_common_utils.h"
#include "fbe_test_configurations.h"
#include "pp_utils.h"
#include "sep_utils.h"
#include "sep_rebuild_utils.h"
#include "sep_zeroing_utils.h"
#include "sep_tests.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_provision_drive.h"
#include "sep_hook.h"
#include "fbe/fbe_raid_group.h"
#include "fbe/fbe_api_terminator_interface.h"

//-----------------------------------------------------------------------------
//  TEST DESCRIPTION:

char* sully_short_desc = "Wear level reporting of ssd drives";
char* sully_long_desc =
    "\n"
    "\n"
    "This scenario validates wear leveling for ssds \n"
    "\n"
    "\n"
    "******* Sully *******************************************************\n"
    "\n"
    "\n"
    "\n"
    "STEP A: Setup - Bring up the initial topology for local SP\n"
    "    - Insert a new enclosure having non configured drives\n"
    "    - Create a PVD object\n"
    "    - Create a RAID GROUP with a LUN\n"
    "    - Create spare drives\n"
    "\n"
    "TEST 1: Wear leveling timer condition test\n"
    "\n"
    "    - Verify that the timer condition gets triggered appropriately\n"
    "    - Verify that the wear leveling reporting results in a notification\n"
    "      all the way up the stack for the lun\n"
    "    - Verify that the highest wear level is retrieved correctly from log page 31\n"
    "\n"
    "TEST 2: Wear level reporting during copy test\n"
    "\n"
    "    - Verify that the highest wear leveling is retrieved correcty during the copy\n"
    "    - Verify that the highest wear level is retrieved correcty after the swap\n"
    "\n"
    "TEST 3: Wear level reporting during degraded\n"
    "\n"
    "    - Verify that the wear leveling reports correctly while the raid group is degraded\n"
    "\n"
    "TEST 4: Wear level reporting on bring up\n"
    "\n"
    "    - Verify that the wear leveling reports correctly and notifications are sent\n"
    "      on both SPs\n"
    "\n";

/*!*******************************************************************
 * @def SULLY_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define SULLY_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def SULLY_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define SULLY_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def SULLY_CHUNK_SIZE
 *********************************************************************
 * @brief Size of one chunk
 *
 *********************************************************************/
#define SULLY_CHUNK_SIZE 0x800

/*!*******************************************************************
 * @def SULLY_TEST_WAIT_MSEC
 *********************************************************************
 * @brief Maximum wait time for database ready
 *
 *********************************************************************/
#define SULLY_TEST_WAIT_MSEC 30000

/*!*******************************************************************
 * @def SULLY_MAX_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define SULLY_MAX_IO_SIZE_BLOCKS (FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1) * FBE_RAID_MAX_BE_XFER_SIZE 

/*!*******************************************************************
 * @var sully_raid_group_config_random
 *********************************************************************
 * @brief This is the array of random configurations made from above configs
 *
 *********************************************************************/
fbe_test_rg_configuration_t sully_raid_group_config_random[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE];

/*!*******************************************************************
 * @var sep_rebuild_utils_number_physical_objects_g
 *********************************************************************
 * @brief Number of expected physical objects
 *
 *********************************************************************/
extern fbe_u32_t        sep_rebuild_utils_number_physical_objects_g;

/*!*******************************************************************
 * @var SULLY_MAX_ERASE_COUNT_PARAM_CODE
 *********************************************************************
 * @brief Parameter code in log page 31 for max erase count
 *
 *********************************************************************/
#define SULLY_MAX_ERASE_COUNT_PARAM_CODE 0x8000

/*!*******************************************************************
 * @var SULLY_EOL_PE_CYCLE_COUNT_PARAM_CODE
 *********************************************************************
 * @brief Parameter code in log page 31 for max erase count
 *
 *********************************************************************/
#define SULLY_EOL_PE_CYCLE_COUNT_PARAM_CODE 0x8FFF

/*!*******************************************************************
 * @typedef fbe_log_param_header_t
 *********************************************************************
 * @brief log page 31 parameter header format
 *
 *********************************************************************/
typedef struct fbe_log_param_header_s 
{
    fbe_u16_t   param_code;
    fbe_u8_t    flags;
    fbe_u8_t    length;
} fbe_log_param_header_t;

/*****************************************
 * EXTERNAL FUNCTIONS
 *****************************************/
extern void diabolicdiscdemon_get_source_destination_edge_index(fbe_object_id_t  vd_object_id,
                                                                fbe_edge_index_t * source_edge_index_p,
                                                                fbe_edge_index_t * dest_edge_index_p);
extern void diabolicdiscdemon_wait_for_proactive_spare_swap_in(fbe_object_id_t  vd_object_id,
                                                               fbe_edge_index_t spare_edge_index,
                                                               fbe_test_raid_group_disk_set_t * spare_location_p);
extern void diabolicdiscdemon_wait_for_proactive_copy_completion(fbe_object_id_t vd_object_id);
extern void diabolicdiscdemon_wait_for_source_drive_swap_out(fbe_object_id_t vd_object_id,
                                                             fbe_edge_index_t source_edge_index);

/*****************************************
 * LOCAL FUNCTIONS
 *****************************************/
void sully_run_tests(fbe_test_rg_configuration_t *rg_config_p, void * context_p);
void sully_degraded_wear_leveling_test(fbe_test_rg_configuration_t *rg_config_p, void * context_p);
void sully_verify_timer_wear_leveling_test(fbe_test_rg_configuration_t *rg_config_p, void * context_p); 
void sully_wear_leveling_notification_on_bringup_test(fbe_test_rg_configuration_t *rg_config_p, void * context_p);
void sully_get_wear_leveling_info_during_copy_test(fbe_test_rg_configuration_t *rg_config_p, void * context_p);

void sully_shutdown_sps(void);
void sully_set_log_page_31(fbe_u32_t port_number,
                           fbe_u32_t enclosure_number,
                           fbe_u32_t slot_number,
                           fbe_u8_t * log_page_31,
                           fbe_u32_t length);
void sully_get_log_page_31(fbe_u32_t port_number,
                           fbe_u32_t enclosure_number,
                           fbe_u32_t slot_number,
                           fbe_u8_t * log_page_31,
                           fbe_u32_t * length);
void sully_debug_print_log_page_31(fbe_u8_t * log_page_31, fbe_u32_t length);
void sully_set_log_page_31_param(fbe_u8_t * log_page_31, fbe_u32_t length, 
                                 fbe_u16_t in_param_code, fbe_u64_t param_value);
void sully_get_log_page_31_param(fbe_u8_t * log_page_31, fbe_u32_t length, 
                                 fbe_u16_t in_param_code, fbe_u64_t * param_value);
void sully_set_max_erase_count(fbe_u8_t * log_page_31, fbe_u32_t length, fbe_u64_t max_erase_count);
void sully_get_max_erase_count(fbe_u8_t * log_page_31, fbe_u32_t length, fbe_u64_t * max_erase_count);
void sully_set_eol_pe_cycle_count(fbe_u8_t * log_page_31, fbe_u32_t length, fbe_u64_t eol_pe_cycle_count);
void sully_get_eol_pe_cycle_count(fbe_u8_t * log_page_31, fbe_u32_t length, fbe_u64_t *eol_pe_cycle_count);
void sully_setup_for_wear_level_query(fbe_object_id_t rg_object_id);

/*!**************************************************************
 * sully_shutdown_sps()
 ****************************************************************
 * @brief
 *  Shutdown both sps.
 *
 * @return None.   
 *
 * @author
 *  07/22/2015 - Created. Deanna Heng
 ****************************************************************/
void sully_shutdown_sps(void)
{
    fbe_sim_transport_connection_target_t   original_target;

    original_target = fbe_api_sim_transport_get_target_server();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
    fbe_test_sep_util_destroy_neit_sep();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
    fbe_test_sep_util_destroy_neit_sep();

    fbe_api_sim_transport_set_target_server(original_target);
}
/***************************************************************
 * end sully_shutdown_sps()
 ***************************************************************/

/*!**************************************************************
 * sully_set_log_page_31()
 ****************************************************************
 * @brief
 *  Set the log page 31 on this drive in the terminator
 *
 * @param   port_number - drive bus
 *          enclosure_number - drive enclosure
 *          slot_number - drive slot
 *          log_page_31 - pointer to the log page 31 data
 *          length - length of the log page 31 data
 *
 * @return None.   
 *
 * @author
 *  07/22/2015 - Created. Deanna Heng
 ****************************************************************/
void sully_set_log_page_31(fbe_u32_t port_number,
                           fbe_u32_t enclosure_number,
                           fbe_u32_t slot_number,
                           fbe_u8_t * log_page_31,
                           fbe_u32_t length)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_terminator_device_handle_t drive_handle;

    status = fbe_api_terminator_get_drive_handle(port_number,
                                                 enclosure_number,
                                                 slot_number,
                                                 &drive_handle);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_terminator_drive_set_log_page_31_data(drive_handle,
                                                           log_page_31,
                                                           length);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

}
/***************************************************************
 * end sully_set_log_page_31()
 ***************************************************************/

/*!**************************************************************
 * sully_get_log_page_31()
 ****************************************************************
 * @brief
 *  Get the log page 31 on this drive in the terminator
 *
 * @param   port_number - drive bus
 *          enclosure_number - drive enclosure
 *          slot_number - drive slot
 *          log_page_31 - pointer to the log page 31 data
 *          length - length of the log page 31 data
 *
 * @return None.   
 *
 * @author
 *  07/22/2015 - Created. Deanna Heng
 ****************************************************************/
void sully_get_log_page_31(fbe_u32_t port_number,
                           fbe_u32_t enclosure_number,
                           fbe_u32_t slot_number,
                           fbe_u8_t * log_page_31,
                           fbe_u32_t * length)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_terminator_device_handle_t drive_handle;

    status = fbe_api_terminator_get_drive_handle(port_number,
                                                 enclosure_number,
                                                 slot_number,
                                                 &drive_handle);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_terminator_drive_get_log_page_31_data(drive_handle,
                                                           log_page_31,
                                                           length);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

}
/***************************************************************
 * end sully_get_log_page_31()
 ***************************************************************/

/*!**************************************************************
 * sully_debug_print_log_page_31()
 ****************************************************************
 * @brief
 *  Print the log page 31 on this drive
 *
 * @param   log_page_31 - pointer to the log page 31 data
 *          length - length of the log page 31 data
 *
 * @return None.   
 *
 * @author
 *  07/22/2015 - Created. Deanna Heng
 ****************************************************************/
void sully_debug_print_log_page_31(fbe_u8_t * log_page_31, fbe_u32_t length)
{
    fbe_u32_t i = 0;

    char * buf_str = (char*) malloc(4*length +1);
    char * buf_ptr = buf_str;

    for (i =0; i< length; i++)
    {
        buf_ptr += sprintf(buf_ptr, "%02x ", log_page_31[i]);

        if ((i+1)%8==0) 
        {
            buf_ptr += sprintf(buf_ptr, "\n");
        }
    }

    sprintf(buf_ptr,"\n");
    *(buf_ptr + 1) = '\0';
    mut_printf(MUT_LOG_LOW, buf_str);
}
/***************************************************************
 * end sully_debug_print_log_page_31()
 ***************************************************************/

/*!**************************************************************
 * sully_set_log_page_31_param()
 ****************************************************************
 * @brief
 *  Set the max erase count in the log page 31
 *
 * @param   log_page_31 - pointer to the log page 31 data
 *          length - length of the log page 31 data
 *          in_param_code - the parameter code to change
 *          param_value - the value of the parameter
 *
 * @return None.   
 *
 * @author
 *  07/22/2015 - Created. Deanna Heng
 ****************************************************************/
void sully_set_log_page_31_param(fbe_u8_t * log_page_31, fbe_u32_t length, 
                                 fbe_u16_t in_param_code, fbe_u64_t param_value)
{
    fbe_u32_t i = 4; 
    fbe_log_param_header_t *log_header_ptr;
    fbe_u8_t *byte_ptr = log_page_31;
    fbe_u16_t header_size = sizeof(fbe_log_param_header_t);
    fbe_u16_t param_code;
    fbe_u8_t param_length;

    byte_ptr += 4; // skip 4 bytes of header
    while ((i + header_size) < length) 
    {
        log_header_ptr = (fbe_log_param_header_t *)byte_ptr;
        param_code = csx_ntohs(log_header_ptr->param_code);
        param_length = log_header_ptr->length;
        if (param_code == in_param_code) 
        {
            if (param_length == 4) {
                *(fbe_u32_t *)(byte_ptr + header_size) = csx_ntohl((fbe_u32_t)param_value);
            } 
            else if (param_length == 8) 
            {
                *(fbe_u64_t *)(byte_ptr + header_size) = csx_ntohll(param_value);
            }

            return;
        }
        i += (header_size + param_length);
        byte_ptr += (header_size + param_length);
    }
}
/***************************************************************
 * end sully_set_log_page_31_param()
 ***************************************************************/

/*!**************************************************************
 * sully_get_log_page_31_param()
 ****************************************************************
 * @brief
 *  Get the max erase count in the log page 31
 *
 * @param   log_page_31 - pointer to the log page 31 data
 *          length - length of the log page 31 data
 *          in_param_code - the parameter code to get
 *          param_value - the value of the parameter
 *
 * @return None.   
 *
 * @author
 *  07/22/2015 - Created. Deanna Heng
 ****************************************************************/
void sully_get_log_page_31_param(fbe_u8_t * log_page_31, fbe_u32_t length, 
                                 fbe_u16_t in_param_code, fbe_u64_t * param_value)
{
    fbe_u32_t i = 4; 
    fbe_log_param_header_t *log_header_ptr;
    fbe_u8_t *byte_ptr = log_page_31;
    fbe_u16_t header_size = sizeof(fbe_log_param_header_t);
    fbe_u16_t param_code;
    fbe_u8_t param_length;

    *byte_ptr += 4; // skip 4 bytes of header
    while ((i + header_size) < length) 
    {
        log_header_ptr = (fbe_log_param_header_t *)byte_ptr;
        param_code = csx_ntohs(log_header_ptr->param_code);
        param_length = log_header_ptr->length;
        if (param_code == in_param_code) 
        {
            if (param_length == 4) {
                *param_value = csx_ntohl(*(fbe_u32_t *)(byte_ptr + header_size));
            } 
            else if (param_length == 8) 
            {
                *param_value = csx_ntohll(*(fbe_u64_t *)(byte_ptr + header_size));
            }
            return;
        }

        i += (header_size + param_length);
        byte_ptr += (header_size + param_length);
    }
}
/***************************************************************
 * end sully_get_log_page_31_param()
 ***************************************************************/

/*!**************************************************************
 * sully_set_max_erase_count()
 ****************************************************************
 * @brief
 *  Set the max erase count in the log page 31
 *
 * @param   log_page_31 - pointer to the log page 31 data
 *          length - length of the log page 31 data
 *          max_erase_count - current maximum erase count in the log page
 *
 * @return None.   
 *
 * @author
 *  07/22/2015 - Created. Deanna Heng
 ****************************************************************/
void sully_set_max_erase_count(fbe_u8_t * log_page_31, fbe_u32_t length, fbe_u64_t max_erase_count)
{
    sully_set_log_page_31_param(log_page_31, length, 
                                SULLY_MAX_ERASE_COUNT_PARAM_CODE, max_erase_count);
}
/***************************************************************
 * end sully_set_max_erase_count()
 ***************************************************************/

/*!**************************************************************
 * sully_get_max_erase_count()
 ****************************************************************
 * @brief
 *  Get the max erase count in the log page 31
 *
 * @param   log_page_31 - pointer to the log page 31 data
 *          length - length of the log page 31 data
 *          max_erase_count - current maximum erase count in the log page
 *
 * @return None.   
 *
 * @author
 *  07/22/2015 - Created. Deanna Heng
 ****************************************************************/
void sully_get_max_erase_count(fbe_u8_t * log_page_31, fbe_u32_t length, fbe_u64_t * max_erase_count)
{
    sully_get_log_page_31_param(log_page_31, length, 
                                SULLY_MAX_ERASE_COUNT_PARAM_CODE, max_erase_count);
}
/***************************************************************
 * end sully_get_max_erase_count()
 ***************************************************************/

/*!**************************************************************
 * sully_set_max_eol_pe_cycle_count()
 ****************************************************************
 * @brief
 *  Set the max erase count in the log page 31
 *
 * @param   log_page_31 - pointer to the log page 31 data
 *          length - length of the log page 31 data
 *          max_erase_count - current maximum erase count in the log page
 *
 * @return None.   
 *
 * @author
 *  07/22/2015 - Created. Deanna Heng
 ****************************************************************/
void sully_set_eol_pe_cycle_count(fbe_u8_t * log_page_31, fbe_u32_t length, fbe_u64_t eol_pe_cycle_count)
{
    sully_set_log_page_31_param(log_page_31, length, 
                                SULLY_EOL_PE_CYCLE_COUNT_PARAM_CODE, eol_pe_cycle_count);
}
/***************************************************************
 * end sully_set_eol_pe_cycle_count()
 ***************************************************************/

/*!**************************************************************
 * sully_get_eol_pe_cycle_count()
 ****************************************************************
 * @brief
 *  Get the max erase count in the log page 31
 *
 * @param   log_page_31 - pointer to the log page 31 data
 *          length - length of the log page 31 data
 *          max_erase_count - current maximum erase count in the log page
 *
 * @return None.   
 *
 * @author
 *  07/22/2015 - Created. Deanna Heng
 ****************************************************************/
void sully_get_eol_pe_cycle_count(fbe_u8_t * log_page_31, fbe_u32_t length, fbe_u64_t * eol_pe_cycle_count)
{
    sully_get_log_page_31_param(log_page_31, length, 
                                SULLY_EOL_PE_CYCLE_COUNT_PARAM_CODE, eol_pe_cycle_count);
}
/***************************************************************
 * end sully_get_eol_pe_cycle_count()
 ***************************************************************/

/*!****************************************************************************
 * sully_setup_for_wear_level_query()
 ******************************************************************************
 * @brief
 *  Set the timer and go through the wear leveling query
 *
 * @param   rg_object_id - the raid group to issue the query
 *
 * @return  None
 *
 * @author
 *  07/22/2015 - Created. Deanna Heng
 ******************************************************************************/
void sully_setup_for_wear_level_query(fbe_object_id_t rg_object_id)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_lifecycle_timer_info_t              timer_info = {0};

    status = fbe_api_scheduler_add_debug_hook(rg_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_WEAR_LEVELING_QUERY,
                                              FBE_RAID_GROUP_SUBSTATE_WEAR_LEVELING_START,
                                              0,
                                              NULL,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_LOG);

     MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* timer condition is 100ths of a second. this is 10 seconds
     */
    timer_info.interval = 100 * 10;
    timer_info.lifecycle_condition = FBE_RAID_GROUP_SET_LIFECYCLE_TIMER_COND_WEAR_LEVEL;
    mut_printf(MUT_LOG_LOW, "Setting lifecycle timer to %d seconds", timer_info.interval/100);
    fbe_api_raid_group_set_lifecycle_timer_interval(rg_object_id, &timer_info);

    /* verify that the raid group goes through the required condition
     */
    status = fbe_test_wait_for_debug_hook(rg_object_id, 
                                          SCHEDULER_MONITOR_STATE_RAID_GROUP_WEAR_LEVELING_QUERY,
                                          FBE_RAID_GROUP_SUBSTATE_WEAR_LEVELING_START,
                                          SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG, 0,0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


    status = fbe_api_scheduler_del_debug_hook(rg_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_WEAR_LEVELING_QUERY,
                                              FBE_RAID_GROUP_SUBSTATE_WEAR_LEVELING_START,
                                              0,
                                              NULL,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}
/***************************************************************
 * end sully_setup_for_wear_level_query()
 ***************************************************************/

/*!****************************************************************************
 * sully_get_wear_leveling_info_during_copy_test()
 ******************************************************************************
 * @brief
 *  Verify that wear leveling info is gathered correctly during a copy
 *
 * @param   rg_config_p - rg config to run the test against
 *          context_p - pointer to test case index
 *
 * @return  None
 *
 * @author
 *  06/30/2015 - Created. Deanna Heng
 ******************************************************************************/
void sully_get_wear_leveling_info_during_copy_test(fbe_test_rg_configuration_t *rg_config_p, void * context_p) 
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_u32_t                               raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_object_id_t                         rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                         vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                        source_edge_index;
    fbe_edge_index_t                        dest_edge_index;
    fbe_test_raid_group_disk_set_t          source_drive_location = {0};
    fbe_test_raid_group_disk_set_t          spare_drive_location = {0};
    fbe_object_id_t                         pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_provision_drive_copy_to_status_t    copy_status;
    fbe_u32_t                               drive_pos = 0;
    fbe_raid_group_get_wear_level_t         wear_level = {0};
    fbe_u32_t                               length = 0;
    fbe_u8_t                                log_page_31[TERMINATOR_SCSI_LOG_PAGE_31_BYTE_SIZE] = {0};
    fbe_u64_t                               max_erase_count = 0x30;
    fbe_u64_t                               eol_pe_cycle_count = 0x100;
    fbe_u32_t                               i = 0;


    mut_printf(MUT_LOG_LOW, "%s entry", __FUNCTION__);

    /*  Set up a spare drives 
     */
    sep_rebuild_utils_setup_hot_spare(rg_config_p->extra_disk_set[0].bus,
                                      rg_config_p->extra_disk_set[0].enclosure,
                                      rg_config_p->extra_disk_set[0].slot); 
    sep_rebuild_utils_setup_hot_spare(rg_config_p->extra_disk_set[1].bus,
                                      rg_config_p->extra_disk_set[1].enclosure,
                                      rg_config_p->extra_disk_set[1].slot); 

    source_drive_location = rg_config_p->rg_disk_set[drive_pos];
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[drive_pos].bus,
                                                            rg_config_p->rg_disk_set[drive_pos].enclosure,
                                                            rg_config_p->rg_disk_set[drive_pos].slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: start user copy operation on object 0x%x with bus:0x%x, encl:0x%x, slot:0x%x!!!.",
               __FUNCTION__, pvd_object_id, rg_config_p->rg_disk_set[drive_pos].bus, rg_config_p->rg_disk_set[drive_pos].enclosure, rg_config_p->rg_disk_set[drive_pos].slot);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* set up the log page 31 on all the drives in the raid group and get the wear level */
    sully_get_log_page_31(rg_config_p->rg_disk_set[drive_pos].bus, 
                          rg_config_p->rg_disk_set[drive_pos].enclosure, 
                          rg_config_p->rg_disk_set[drive_pos].slot, 
                          &log_page_31[0], 
                          &length);
    sully_set_max_erase_count(log_page_31, length, max_erase_count);
    sully_set_eol_pe_cycle_count(log_page_31, length, eol_pe_cycle_count);

    for (i=0; i< rg_config_p->width; i++)
    {
        sully_set_log_page_31(rg_config_p->rg_disk_set[i].bus, 
                              rg_config_p->rg_disk_set[i].enclosure, 
                              rg_config_p->rg_disk_set[i].slot, 
                              &log_page_31[0], 
                              length);
    }

    /* set the wear level timer condition so that the raid group updates its wear level info
     */
    sully_setup_for_wear_level_query(rg_object_id);

    status = fbe_api_raid_group_get_wear_level(rg_object_id, &wear_level);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "rg: 0x%x cur: 0x%llx max: 0x%llx poh: 0x%llx\n", 
               rg_object_id, wear_level.current_pe_cycle, wear_level.max_pe_cycle, wear_level.power_on_hours);
    MUT_ASSERT_UINT64_EQUAL(max_erase_count, wear_level.current_pe_cycle);
    MUT_ASSERT_UINT64_EQUAL(eol_pe_cycle_count, wear_level.max_pe_cycle);


    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, drive_pos, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

    /* get the edge index of the source and destination drive based on configuration mode. 
     */
    diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

    /* user initiate copy to. 
     */
    status = fbe_api_provision_drive_user_copy(pvd_object_id, &copy_status);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Set a debug hook after we switch to mirror mode but before we start to 
     * rebuild the paged metadata.
     */
    status = fbe_api_scheduler_add_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY,
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_START,
                                              0, 0,              
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the proactive spare to swap-in. */
    diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_edge_index, &spare_drive_location);

    /* prime the spare drive for log page 31 so that it would be the highest  
     * highest wear leveling is done by getting the highest percent. max pe cycle/eol pe cycle
     * let's change the eol pe cycle here
     */
    mut_printf(MUT_LOG_LOW, "rg: 0x%x set the eol pe cycle in log page 31 to 0x%llx\n", 
               rg_object_id, (eol_pe_cycle_count / 2));
    sully_set_eol_pe_cycle_count(log_page_31, length, (eol_pe_cycle_count / 2));
    sully_set_log_page_31(spare_drive_location.bus, 
                          spare_drive_location.enclosure, 
                          spare_drive_location.slot, 
                          log_page_31, 
                          length);

    /* Wait for rebuild logging to be clear for the destination. */
    status = sep_rebuild_utils_wait_for_rb_logging_clear_for_position(vd_object_id, dest_edge_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Validate that NR is set on the destination drive*/
    fbe_test_sep_rebuild_utils_check_bits_set_for_position(vd_object_id, dest_edge_index);

    /* Validate that no bits are set in the parent raid group*/
    sep_rebuild_utils_check_bits(rg_object_id);

    /* Remove the rebuild start hook. */
    status = fbe_api_scheduler_del_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY,
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_START,
                                              0, 0, 
                                              SCHEDULER_CHECK_STATES, 
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* stop the job service recovery queue to hold the job service swap command. */
    fbe_test_sep_util_disable_recovery_queue(vd_object_id);

    /* verify that the wear level information is still reporting from the correct drives
     * this should not include the spare
     */
    mut_printf(MUT_LOG_LOW, "Verify wear level during copy reports from original rg drives\n");
    sully_setup_for_wear_level_query(rg_object_id);
    status = fbe_api_raid_group_get_wear_level(rg_object_id, &wear_level);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "rg: 0x%x cur: 0x%llx max: 0x%llx poh: 0x%llx\n", 
               rg_object_id, wear_level.current_pe_cycle, wear_level.max_pe_cycle, wear_level.power_on_hours);
    MUT_ASSERT_UINT64_EQUAL(max_erase_count, wear_level.current_pe_cycle);
    MUT_ASSERT_UINT64_EQUAL(eol_pe_cycle_count, wear_level.max_pe_cycle);

    /* wait for the copy completion. */
    diabolicdiscdemon_wait_for_proactive_copy_completion(vd_object_id);

    /* start the job service recovery queue to process the swap-out command. */
    fbe_test_sep_util_enable_recovery_queue(vd_object_id);

    /* wait for the swap-out of the drive after copy completion. */
    diabolicdiscdemon_wait_for_source_drive_swap_out(vd_object_id, source_edge_index);

    /* verify that the wear level is reported from the spare
     */
    mut_printf(MUT_LOG_LOW, "Verify wear level during copy reports from spare drive with higher wear level\n");
    sully_setup_for_wear_level_query(rg_object_id);
    status = fbe_api_raid_group_get_wear_level(rg_object_id, &wear_level);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "rg: 0x%x cur: 0x%llx max: 0x%llx poh: 0x%llx\n", 
               rg_object_id, wear_level.current_pe_cycle, wear_level.max_pe_cycle, wear_level.power_on_hours);
    MUT_ASSERT_UINT64_EQUAL(max_erase_count, wear_level.current_pe_cycle);
    MUT_ASSERT_UINT64_EQUAL((eol_pe_cycle_count/2), wear_level.max_pe_cycle);

    /* Validate that the destination position is not marked NR */
    fbe_test_sep_rebuild_utils_check_bits_clear_for_position(vd_object_id, dest_edge_index);

    /* refresh drive info
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, raid_group_count);
    fbe_test_sep_util_raid_group_refresh_extra_disk_info(rg_config_p, raid_group_count);

}
/***************************************************************
 * end sully_get_wear_leveling_info_during_copy_test()
 ***************************************************************/

/*!****************************************************************************
 * sully_degraded_wear_leveling_test()
 ******************************************************************************
 * @brief
 *  Verify that wear leveling info is gathered correctly when raid group is degraded
 *
 * @param   rg_config_p - rg config to run the test against
 *          context_p - pointer to test case index
 *
 * @return  None
 *
 * @author
 *  06/30/2015 - Created. Deanna Heng
 ******************************************************************************/
void sully_degraded_wear_leveling_test(fbe_test_rg_configuration_t *rg_config_p, void * context_p) 
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_u32_t                           position = 0;
    fbe_api_terminator_device_handle_t  drive_info = {0};
    fbe_object_id_t                     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_raid_group_get_wear_level_t     wear_level = {0};
    fbe_u32_t                           raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    mut_printf(MUT_LOG_LOW, "%s entry", __FUNCTION__);

    /*  Set up a spare drives 
     */
    sep_rebuild_utils_setup_hot_spare(rg_config_p->extra_disk_set[0].bus,
                                      rg_config_p->extra_disk_set[0].enclosure,
                                      rg_config_p->extra_disk_set[0].slot); 
    sep_rebuild_utils_setup_hot_spare(rg_config_p->extra_disk_set[1].bus,
                                      rg_config_p->extra_disk_set[1].enclosure,
                                      rg_config_p->extra_disk_set[1].slot); 

    /* book keeping to keep track of how many objects to wait for later 
     */
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_test_sep_util_update_permanent_spare_trigger_timer(1000);
    status = fbe_api_base_config_disable_background_operation(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_REBUILD);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Remove a single drive in the RG.  Check the object states change correctly and that rb logging
     * is marked. 
     */
    sep_rebuild_utils_number_physical_objects_g -= 1;
    sep_rebuild_utils_remove_drive_and_verify(rg_config_p, position, sep_rebuild_utils_number_physical_objects_g, &drive_info);        

    status = fbe_api_scheduler_add_debug_hook(rg_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_WEAR_LEVELING_QUERY,
                                              FBE_RAID_GROUP_SUBSTATE_WEAR_LEVELING_START,
                                              0,
                                              NULL,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);

     MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


    /* tell the raid group to query for the wear level in this degraded state 
     */
    status = fbe_api_set_object_to_state(rg_object_id, FBE_LIFECYCLE_STATE_ACTIVATE, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* verify that the raid group goes through the required condition
     */
    status = fbe_test_wait_for_debug_hook(rg_object_id, 
                                          SCHEDULER_MONITOR_STATE_RAID_GROUP_WEAR_LEVELING_QUERY,
                                          FBE_RAID_GROUP_SUBSTATE_WEAR_LEVELING_START,
                                          SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE, 0,0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_scheduler_del_debug_hook(rg_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_WEAR_LEVELING_QUERY,
                                              FBE_RAID_GROUP_SUBSTATE_WEAR_LEVELING_START,
                                              0,
                                              NULL,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Verify the raid group get to ready state */
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id,
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* verify that the wear level is appropriate
     */
    status = fbe_api_raid_group_get_wear_level(rg_object_id, &wear_level);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "rg: 0x%x cur: 0x%llx max: 0x%llx poh: 0x%llx\n", 
               rg_object_id, wear_level.current_pe_cycle, wear_level.max_pe_cycle, wear_level.power_on_hours);

    fbe_test_sep_util_update_permanent_spare_trigger_timer(1);
    /* the spare pvd should've swapped in...
     */
    fbe_test_sep_drive_wait_permanent_spare_swap_in(rg_config_p, position);

    fbe_test_sep_util_wait_for_raid_group_to_start_rebuild(rg_object_id);
    status = fbe_api_base_config_enable_background_operation(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_REBUILD);

    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, position);

    sep_rebuild_utils_check_bits(rg_object_id);

    /* set the permanent spare trigger timer back to default. 
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME);

    sep_rebuild_utils_number_physical_objects_g += 1;
    /* Reinsert the drive and wait for the rebuild to start 
     */
    sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, position, sep_rebuild_utils_number_physical_objects_g, &drive_info);

    /* refresh drive info
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, raid_group_count);
    fbe_test_sep_util_raid_group_refresh_extra_disk_info(rg_config_p, raid_group_count);
}
/***************************************************************
 * end sully_degraded_wear_leveling_test()
 ***************************************************************/

/*!****************************************************************************
 * sully_verify_timer_wear_leveling_test()
 ******************************************************************************
 * @brief
 *  Verify that wear leveling info is gathered when timer condition is triggered
 *
 * @param   rg_config_p - rg config to run the test against
 *          context_p - pointer to test case index
 *
 * @return  None
 *
 * @author
 *  06/30/2015 - Created. Deanna Heng
 ******************************************************************************/
void sully_verify_timer_wear_leveling_test(fbe_test_rg_configuration_t *rg_config_p, void * context_p) 
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_object_id_t                         rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                         lun_object_id = FBE_OBJECT_ID_INVALID;
    fbe_raid_group_get_wear_level_t         wear_level = {0};
    fbe_lifecycle_timer_info_t              timer_info = {0};
    fbe_notification_info_t                 notification_info;
    fbe_u8_t                                log_page_31[TERMINATOR_SCSI_LOG_PAGE_31_BYTE_SIZE] = {0};
    fbe_u8_t                                position = 0;
    fbe_u32_t                               port_number = rg_config_p->rg_disk_set[position].bus;
    fbe_u32_t                               enclosure_number = rg_config_p->rg_disk_set[position].enclosure;
    fbe_u32_t                               slot_number = rg_config_p->rg_disk_set[position].slot;
    fbe_u32_t                               length = 0;
    fbe_u64_t                               max_erase_count = 0;

    mut_printf(MUT_LOG_LOW, "%s entry", __FUNCTION__);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list[0].lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sully_get_log_page_31(port_number, enclosure_number, slot_number, &log_page_31[0], &length);
    mut_printf(MUT_LOG_LOW, "position %d log page len:%d\n", position, length);
    sully_debug_print_log_page_31(log_page_31, length);

    /* verify that the wear level is appropriate
     */
    status = fbe_api_raid_group_get_wear_level(rg_object_id, &wear_level);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "rg: 0x%x cur: 0x%llx max: 0x%llx poh: 0x%llx\n", 
               rg_object_id, wear_level.current_pe_cycle, wear_level.max_pe_cycle, wear_level.power_on_hours);

    /* update that max erase count on one drive 
     */
    sully_set_max_erase_count(log_page_31, length, wear_level.current_pe_cycle +1);
    sully_set_log_page_31(port_number,enclosure_number,slot_number,log_page_31,length);

    sully_get_log_page_31(port_number, enclosure_number, slot_number, log_page_31, &length);
    mut_printf(MUT_LOG_LOW, "position %d log page len:%d\n", position, length);
    sully_debug_print_log_page_31(log_page_31, length);
    sully_get_max_erase_count(log_page_31, length, &max_erase_count);
    MUT_ASSERT_UINT64_EQUAL(max_erase_count, wear_level.current_pe_cycle +1);


    status = fbe_api_scheduler_add_debug_hook(rg_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_WEAR_LEVELING_QUERY,
                                              FBE_RAID_GROUP_SUBSTATE_WEAR_LEVELING_START,
                                              0,
                                              NULL,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);

     MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* timer condition is 100ths of a second. this is 10 seconds
     */
    timer_info.interval = 100 * 10;
    timer_info.lifecycle_condition = FBE_RAID_GROUP_SET_LIFECYCLE_TIMER_COND_WEAR_LEVEL;
    mut_printf(MUT_LOG_LOW, "Setting lifecycle timer to %d seconds", timer_info.interval/100);
    fbe_api_raid_group_set_lifecycle_timer_interval(rg_object_id, &timer_info);

    /* verify that the raid group goes through the required condition
     */
    status = fbe_test_wait_for_debug_hook(rg_object_id, 
                                          SCHEDULER_MONITOR_STATE_RAID_GROUP_WEAR_LEVELING_QUERY,
                                          FBE_RAID_GROUP_SUBSTATE_WEAR_LEVELING_START,
                                          SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE, 0,0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for notification that wear level reporting is successful*/
    status = fbe_test_common_set_notification_to_wait_for(lun_object_id,
                                                          FBE_TOPOLOGY_OBJECT_TYPE_LUN,
                                                          FBE_NOTIFICATION_TYPE_CONFIGURATION_CHANGED,
                                                          FBE_STATUS_OK,
                                                          FBE_JOB_SERVICE_ERROR_NO_ERROR);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_scheduler_del_debug_hook(rg_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_WEAR_LEVELING_QUERY,
                                              FBE_RAID_GROUP_SUBSTATE_WEAR_LEVELING_START,
                                              0,
                                              NULL,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the notification. */
    status = fbe_test_common_wait_for_notification(__FUNCTION__, __LINE__,
                                                   30000, 
                                                   &notification_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* verify that the wear level is updated
     */
    status = fbe_api_raid_group_get_wear_level(rg_object_id, &wear_level);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "rg: 0x%x cur: 0x%llx max: 0x%llx poh: 0x%llx\n", 
               rg_object_id, wear_level.current_pe_cycle, wear_level.max_pe_cycle, wear_level.power_on_hours);

    MUT_ASSERT_UINT64_EQUAL(max_erase_count, wear_level.current_pe_cycle);

}
/***************************************************************
 * end sully_verify_timer_wear_leveling_test()
 ***************************************************************/

/*!****************************************************************************
 * sully_wear_leveling_notification_on_bringup_test()
 ******************************************************************************
 * @brief
 *  Verify that notification is sent on both SPs on bring up
 *
 * @param   rg_config_p - rg config to run the test against
 *          context_p - pointer to test case index
 *
 * @return  None
 *
 * @author
 *  06/30/2015 - Created. Deanna Heng
 ******************************************************************************/
void sully_wear_leveling_notification_on_bringup_test(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_sim_transport_connection_target_t   original_target;
    fbe_sep_package_load_params_t           sep_params;
    fbe_neit_package_load_params_t          neit_params;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t lun_object_id = FBE_OBJECT_ID_INVALID;
    fbe_notification_info_t notification_info;

    mut_printf(MUT_LOG_TEST_STATUS, "Starting test case: %s\n", __FUNCTION__);

    original_target = fbe_api_sim_transport_get_target_server();

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list[0].lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for database ready */
    status = fbe_test_sep_util_wait_for_database_service(SULLY_TEST_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Shutdown */
    //fbe_test_base_suite_destroy_sp();
    sully_shutdown_sps();

    /* load sep and neit with hook*/
    fbe_zero_memory(&sep_params, sizeof(fbe_sep_package_load_params_t));
    fbe_zero_memory(&neit_params, sizeof(fbe_neit_package_load_params_t));
    sep_config_fill_load_params(&sep_params);
    neit_config_fill_load_params(&neit_params);
    mut_printf(MUT_LOG_TEST_STATUS, "Insert specialize hook for vault raid group");
    sep_params.scheduler_hooks[0].object_id = rg_object_id;
    sep_params.scheduler_hooks[0].monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_WEAR_LEVELING_QUERY;
    sep_params.scheduler_hooks[0].monitor_substate = FBE_RAID_GROUP_SUBSTATE_WEAR_LEVELING_START;
    sep_params.scheduler_hooks[0].check_type = SCHEDULER_CHECK_STATES;
    sep_params.scheduler_hooks[0].action = SCHEDULER_DEBUG_ACTION_PAUSE;
    sep_params.scheduler_hooks[0].val1 = NULL;
    sep_params.scheduler_hooks[0].val2 = NULL;
    /* and this will be our signal to stop */
    sep_params.scheduler_hooks[1].object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_sleep(2000); // wait a couple seconds before starting up again

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    sep_config_load_sep_and_neit_params(&sep_params, NULL);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    sep_config_load_sep_and_neit_params(&sep_params, NULL);
    fbe_api_sim_transport_set_target_server(original_target);


    status = fbe_test_sep_util_wait_for_database_service_ready(SULLY_TEST_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* verify that the raid group goes through the required condition
     */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_wait_for_debug_hook(rg_object_id, 
                                          SCHEDULER_MONITOR_STATE_RAID_GROUP_WEAR_LEVELING_QUERY,
                                          FBE_RAID_GROUP_SUBSTATE_WEAR_LEVELING_START,
                                          SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE, 0,0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_test_wait_for_debug_hook(rg_object_id, 
                                          SCHEDULER_MONITOR_STATE_RAID_GROUP_WEAR_LEVELING_QUERY,
                                          FBE_RAID_GROUP_SUBSTATE_WEAR_LEVELING_START,
                                          SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE, 0,0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for notification that wear level reporting is successful*/
    status = fbe_test_common_set_notification_to_wait_for(lun_object_id,
                                                          FBE_TOPOLOGY_OBJECT_TYPE_LUN,
                                                          FBE_NOTIFICATION_TYPE_CONFIGURATION_CHANGED,
                                                          FBE_STATUS_OK,
                                                          FBE_JOB_SERVICE_ERROR_NO_ERROR);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "Delete the debug hook on both SPs\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_scheduler_del_debug_hook(rg_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_WEAR_LEVELING_QUERY,
                                              FBE_RAID_GROUP_SUBSTATE_WEAR_LEVELING_START,
                                              NULL,
                                              NULL,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_scheduler_del_debug_hook(rg_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_WEAR_LEVELING_QUERY,
                                              FBE_RAID_GROUP_SUBSTATE_WEAR_LEVELING_START,
                                              NULL,
                                              NULL,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


    /* Wait for the notification. */
    status = fbe_test_common_wait_for_notification(__FUNCTION__, __LINE__,
                                                   30000, 
                                                   &notification_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


}
/***************************************************************
 * end sully_wear_leveling_notification_on_bringup_test()
 ***************************************************************/

/*!****************************************************************************
 * sully_run_tests()
 ******************************************************************************
 * @brief
 *  Run wear_leveling tests
 *
 * @param   rg_config_p - rg config to run the test against
 *          context_p - pointer to test case index
 *
 * @return  None
 *
 * @author
 *  06/30/2015 - Created. Deanna Heng
 ******************************************************************************/
void sully_run_tests(fbe_test_rg_configuration_t *rg_config_p, void * context_p) 
{
    if (fbe_test_sep_util_get_dualsp_test_mode()) 
    {
        sully_wear_leveling_notification_on_bringup_test(rg_config_p, context_p);
    }
    else
    {
        sully_verify_timer_wear_leveling_test(rg_config_p, context_p);
        sully_verify_timer_wear_leveling_test(rg_config_p, context_p); // verify notification can run again
        sully_get_wear_leveling_info_during_copy_test(rg_config_p, context_p);
        sully_degraded_wear_leveling_test(rg_config_p, context_p); 
    }

}
/***************************************************************
 * end sully_run_tests()
 ***************************************************************/

/*!****************************************************************************
 * sully_test()
 ******************************************************************************
 * @brief
 *  Run wear leveling tests
 *
 * @param   None
 *
 * @return  None
 *
 * @author
 *  06/30/2015 - Created. Deanna Heng
 ******************************************************************************/
void sully_test(void)
{

    /* Since these test do not generate corrupt crc etc, we don't expect
     * ANY errors.  So stop the system if any sector errors occur.
     */
    fbe_test_sep_util_set_sector_trace_stop_on_error(FBE_TRUE);
    fbe_test_run_test_on_rg_config_with_extra_disk_with_time_save(&sully_raid_group_config_random[0],
                                                              NULL,sully_run_tests,
                                                              SULLY_LUNS_PER_RAID_GROUP,
                                                              SULLY_CHUNKS_PER_LUN,
                                                              FBE_FALSE);

    fbe_test_sep_util_set_sector_trace_stop_on_error(FBE_FALSE);

    return;
}
/***************************************************************
 * end sully_test()
 ***************************************************************/

/*!****************************************************************************
 *  sully_setup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the sully test.
 *
 * @param   None
 *
 * @return  None
 *
 * @author
 *  06/30/2015 - Created. Deanna Heng
 *****************************************************************************/
void sully_setup(void)
{
    mut_printf(MUT_LOG_LOW, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_u32_t raid_group_count = 1;
        
        darkwing_duck_create_random_config(&sully_raid_group_config_random[0], raid_group_count);

        /* Initialize the raid group configuration 
         */
        fbe_test_sep_util_init_rg_configuration_array(&sully_raid_group_config_random[0]);

        /* initialize the number of extra drive required by each rg 
         */
        fbe_test_sep_util_populate_rg_num_extra_drives(&sully_raid_group_config_random[0]);
        fbe_test_pp_utils_set_default_520_sas_drive(FBE_SAS_DRIVE_SIM_520_FLASH_UNMAP);

        /* Setup the physical config for the raid groups
         */
        elmo_create_physical_config_for_rg(&sully_raid_group_config_random[0], 
                                           raid_group_count);
        elmo_load_sep_and_neit();

        /* Set the trace level to info. 
         */
        elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);        
    }

    /* update the permanent spare trigger timer so we don't need to wait long for the swap
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1/* 1 second */);

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    fbe_test_sep_drive_disable_sniff_verify_for_all_pvds();

    /* After sep is loaded setup notifications.
     */
    fbe_test_common_setup_notifications(FBE_FALSE /* This is a single SP test*/);

    return;
}
/***************************************************************
 * end sully_setup()
 ***************************************************************/

/*!****************************************************************************
 *  sully_cleanup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the sully test.
 *
 * @param   None
 *
 * @return  None
 *
 * @author
 *  06/30/2015 - Created. Deanna Heng
 *****************************************************************************/
void sully_cleanup(void)
{
    mut_printf(MUT_LOG_LOW, "%s entry", __FUNCTION__);
    fbe_test_common_cleanup_notifications(FBE_FALSE /* This is a single SP test*/);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/***************************************************************
 * end sully_cleanup()
 ***************************************************************/

/*!****************************************************************************
 *  sully_dualsp_setup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the sully test.
 *
 * @param   None
 *
 * @return  None
 *
 * @author
 *  06/30/2015 - Created. Deanna Heng
 *****************************************************************************/
void sully_dualsp_setup(void)
{
    fbe_u32_t raid_group_count = 1;
        
    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Sully test ===\n");

    if (FBE_FALSE == fbe_test_util_is_simulation()) return;

    darkwing_duck_create_random_config(&sully_raid_group_config_random[0], raid_group_count);
    fbe_test_sep_util_init_rg_configuration_array(&sully_raid_group_config_random[0]);
    fbe_test_sep_util_randomize_rg_configuration_array(&sully_raid_group_config_random[0]);

    fbe_test_sep_util_populate_rg_num_extra_drives(&sully_raid_group_config_random[0]);
    fbe_test_pp_utils_set_default_520_sas_drive(FBE_SAS_DRIVE_SIM_520_FLASH_UNMAP);


    mut_printf(MUT_LOG_TEST_STATUS, "=== Load Physical Package for SPB ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    /* Load the physical configuration */
    elmo_create_physical_config_for_rg(&sully_raid_group_config_random[0], raid_group_count);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Load Physical Package for SPA ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* Load the physical configuration */
    elmo_create_physical_config_for_rg(&sully_raid_group_config_random[0], raid_group_count);

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit_both_sps();

    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);


    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* update the permanent spare trigger timer so we don't need to wait long for the swap
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1/* 1 second */);

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    fbe_test_sep_drive_disable_sniff_verify_for_all_pvds();

    /* After sep is loaded setup notifications.
     */
    fbe_test_common_setup_notifications(FBE_TRUE /* This is a single SP test*/);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    return;
}
/***************************************************************
 * end sully_dualsp_setup()
 ***************************************************************/

/*!****************************************************************************
 *  sully_dualsp_cleanup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the sully test.
 *
 * @param   None
 *
 * @return  None
 *
 * @author
 *  06/30/2015 - Created. Deanna Heng
 *****************************************************************************/
void sully_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_LOW, "%s entry", __FUNCTION__);
    fbe_test_common_cleanup_notifications(FBE_TRUE /* This is a single SP test*/);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical_both_sps();
    }
    return;
}
/***************************************************************
 * end sully_dualsp_cleanup()
 ***************************************************************/
