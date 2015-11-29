/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
/*!**************************************************************************
 * @file fbe_event_log_test_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains event log service test entry function.
 *
 * @version
 *   6-July-2010-  Created
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"
#include "fbe_event_log_test.h"
#include "fbe/fbe_event_log_api.h"
#include "fbe/fbe_service.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_physical_package.h"
#include "PhysicalPackageMessages.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_event_log_utils.h"
#include "fbe/fbe_emcutil_shell_include.h"

/*************************
 *  Forward declaration 
 *************************/
extern fbe_service_methods_t fbe_service_manager_service_methods;
extern fbe_service_methods_t fbe_event_log_service_methods;
extern fbe_service_methods_t fbe_memory_service_methods;
extern fbe_status_t fbe_event_log_control_entry(fbe_packet_t * packet);
static fbe_status_t 
fbe_event_log_test_check_msg(fbe_u32_t msg_id,...);
static fbe_status_t 
fbe_event_log_test_send_packet(fbe_payload_control_operation_opcode_t control_code,
                               fbe_payload_control_buffer_t buffer,
                               fbe_payload_control_buffer_length_t buffer_length);

/* Note we will compile different tables for different packages */
static const fbe_service_methods_t * physical_package_service_table[] = {&fbe_service_manager_service_methods,
                                                                          /* &fbe_transport_service_methods, */
                                                                         &fbe_event_log_service_methods,
                                                                         &fbe_memory_service_methods,
                                                                         NULL };

/*!***************************************************************
 * @fn main (int argc , char ** argv)
 ****************************************************************
 * @brief
 *  This is main entry function for event log test
 *
 * @author
 *   6-July-2010-  Created
 *
 ****************************************************************/
int __cdecl main (int argc , char ** argv)
{
    mut_testsuite_t *suite;
 
    #include "fbe/fbe_emcutil_shell_maincode.h"
    mut_init(argc, argv);

    suite = MUT_CREATE_TESTSUITE("EventLogSuite");

    MUT_ADD_TEST(suite, fbe_event_log_test_insert_msg, fbe_event_log_test_setup, fbe_event_log_test_teardown);
    MUT_ADD_TEST(suite, fbe_event_log_test_count_msg, fbe_event_log_test_setup, fbe_event_log_test_teardown);
    MUT_ADD_TEST(suite, fbe_event_log_test_statistics, fbe_event_log_test_setup, fbe_event_log_test_teardown);    
    MUT_ADD_TEST(suite, fbe_event_log_test_uninit, NULL, NULL);

    MUT_RUN_TESTSUITE(suite)
    
    exit(0);
    return 0;
}
/***********************
*   end of main()
************************/
/*!***************************************************************
 * @fn fbe_event_log_test_setup()
 ****************************************************************
 * @brief
 *  This function setup the required service before start test.
 *
 * @param 
 * 
 * @return
 *
 * @author
 *  6-July-2010: Created.
 *
 ****************************************************************/
void fbe_event_log_test_setup()
{
    fbe_status_t status;
    /* Initialize service manager */
    status = fbe_service_manager_init_service_table(physical_package_service_table);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_SERVICE_MANAGER);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Initialize the memory */
    status =  fbe_memory_init_number_of_chunks(FBE_PHYSICAL_PACKAGE_NUMBER_OF_CHUNKS);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status =  fbe_base_service_send_init_command(FBE_SERVICE_ID_MEMORY);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status =  fbe_base_service_send_init_command(FBE_SERVICE_ID_EVENT_LOG);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*
    status =  fbe_base_service_send_init_command(FBE_SERVICE_ID_TRANSPORT);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    */
    
}
/***********************
*   end of main()
************************/
/*!***************************************************************
 * @fn fbe_event_log_test_setup()
 ****************************************************************
 * @brief
 *  This function does the clean up work after test complete.
 *
 * @param 
 * 
 * @return
 *
 * @author
 *  6-July-2010: Created.
 *
 ****************************************************************/
void fbe_event_log_test_teardown()
{
    fbe_status_t status;
    /*
    status =  fbe_base_service_send_destroy_command(FBE_SERVICE_ID_TRANSPORT);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    */
    /* Clear the event_log*/
    fbe_event_log_test_send_packet(FBE_EVENT_LOG_CONTROL_CODE_CLEAR_LOG,
                                   NULL, 
                                   0);

    status =  fbe_base_service_send_destroy_command(FBE_SERVICE_ID_MEMORY);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status =  fbe_base_service_send_destroy_command(FBE_SERVICE_ID_EVENT_LOG);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_SERVICE_MANAGER);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
/****************************************
*   end of fbe_event_log_test_teardown()
*****************************************/
/*!***************************************************************
 * @fn fbe_event_log_test_setup()
 ****************************************************************
 * @brief
 *  This case test the basic check to make sure messages are
 *  getting inserted correctly or not
 *
 * @param 
 * 
 * @return
 *
 * @author
 *  6-July-2010: Created.
 *
 ****************************************************************/
#pragma warning(disable: 4068) /* Suppress warnings about unknown pragmas */
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#pragma warning(default: 4068)
void fbe_event_log_test_insert_msg()
{
    fbe_status_t status;
    fbe_u8_t *date = __DATE__;
    fbe_u8_t *time = __TIME__;
    fbe_u32_t invalid_msg_id = 0x01765555;
    fbe_u32_t error_counter = 0;
										 
    /* 1) Log the valid message*/
    status = fbe_event_log_write(PHYSICAL_PACKAGE_INFO_LOAD_VERSION,
                                 NULL, 
                                 0, 
                                 "%s %s",
                                 date,
                                 time);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*Check for message*/
    status = fbe_event_log_test_check_msg(PHYSICAL_PACKAGE_INFO_LOAD_VERSION,
                                          date,
                                          time);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* 2) Check for invalid message which does not exist in event log 
     *    it should return error 
     */
    status = fbe_event_log_test_check_msg(invalid_msg_id,
                                          date,
                                          time);
   MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);

    /* 3) Check for valid message but invalid argument 
     *    it should return error 
     */
    status = fbe_event_log_test_check_msg(PHYSICAL_PACKAGE_INFO_LOAD_VERSION,
                                         "invalid_date",
                                         "invalid_time");
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);

    /* 4) Log message more than 256 char to check 
     *    buffer overflow
     */
    status = fbe_event_log_write(SAMPLE_TEST_MESSAGE,
                                 NULL, 
                                 0, 
                                 "",
                                 0,
                                 0);

    error_counter = fbe_trace_get_error_counter();
    MUT_ASSERT_INT_EQUAL(1, error_counter);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);

    /* 5) Log invalid message and check for error count
     */
    status = fbe_event_log_write(invalid_msg_id,
                                 NULL, 
                                 0, 
                                 "",
                                 0,
                                 0);

    error_counter = fbe_trace_get_error_counter();
    MUT_ASSERT_INT_EQUAL(2, error_counter);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);

    /* 6) Log valid message id but string is not there
    */
    status = fbe_event_log_write(invalid_msg_id,
                                 NULL, 
                                 0, 
                                 "",
                                 0,
                                 0);

    error_counter = fbe_trace_get_error_counter();
    MUT_ASSERT_INT_EQUAL(3, error_counter);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);

}
#pragma warning(disable: 4068) /* Suppress warnings about unknown pragmas */
#pragma GCC diagnostic warning "-Wformat-zero-length"
#pragma warning(default: 4068)
/****************************************
*   end of fbe_event_log_test_insert_msg()
*****************************************/
/*!***************************************************************
 * @fn fbe_event_log_test_check_msg(fbe_u32_t msg_id,...)
 ****************************************************************
 * @brief
 *  This function check for particular message present in event log
 *  or not.
 *
 * @param msg_id  Message ID
 * @param .
 * 
 * @return
 *
 * @author
 *  6-July-2010- Vaibhav Gaonkar    Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_event_log_test_check_msg(fbe_u32_t msg_id,...)
{
    va_list arg;
    fbe_event_log_check_msg_t msg;
    msg.msg_id = msg_id;
    msg.is_msg_present =  FALSE;
    fbe_zero_memory(msg.msg, 256);

    va_start(arg, msg_id);
    fbe_event_log_get_full_msg(msg.msg, 256,msg_id,
                                                 arg);
    va_end(arg);
    fbe_event_log_test_send_packet(FBE_EVENT_LOG_CONTROL_CODE_CHECK_MSG,
                                   &msg,
                                   sizeof(msg));
    if(msg.is_msg_present == TRUE)
    {
        return FBE_STATUS_OK;
    }
    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************************
*   end of fbe_event_log_test_check_msg()
******************************************/
/*!***************************************************************
 * @fn fbe_event_log_test_count_msg()
 ****************************************************************
 * @brief
 * This function inserts multiple messages and checks if all
*  those messages are saved correctly
 *
 * @param .
 * 
 * @return
 *
 * @author
 *  6-July-2010-    Created.
 *
 ****************************************************************/
void fbe_event_log_test_count_msg()
{
    fbe_u32_t i = 0;
    fbe_status_t status;
    fbe_event_log_msg_count_t msg;

    for(i = 0; i < 10; i++) 
    {
        status = fbe_event_log_write(PHYSICAL_PACKAGE_INFO_LOAD_VERSION,
                                     NULL, 0, "%s %s", __DATE__, __TIME__);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

     /* Initialized the fbe_event_log_msg_count_t structure*/
     msg.msg_id = PHYSICAL_PACKAGE_INFO_LOAD_VERSION;
     msg.msg_count = 0;
     status = fbe_event_log_test_send_packet(FBE_EVENT_LOG_CONTROL_CODE_GET_MSG_COUNT,
                                             &msg,
                                             sizeof(msg));

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(msg.msg_count, 10);
}
/*****************************************
*   end of fbe_event_log_test_count_msg()
*****************************************/
/*!***************************************************************
 * @fn fbe_event_log_test_count_msg()
 ****************************************************************
 * @brief
 * The test case validates the statistics information
 *  maintained by the event log framework
 *
 * @param .
 * 
 * @return
 *
 * @author
 *  6-July-2010-    Created.
 *
 ****************************************************************/
void fbe_event_log_test_statistics()
{
    fbe_u32_t i = 0;
    fbe_status_t status;
    fbe_event_log_statistics_t stats;

    /* Log the multiple messages*/
    for(i = 0; i < 10; i++) 
    {
        status = fbe_event_log_write(PHYSICAL_PACKAGE_INFO_LOAD_VERSION,
                                     NULL, 0, "%s %s", __DATE__, __TIME__);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    
    status = fbe_event_log_test_send_packet(FBE_EVENT_LOG_CONTROL_CODE_GET_STATISTICS,
                                            &stats,
                                            sizeof(stats));
    MUT_ASSERT_INT_EQUAL(stats.total_msgs_logged, 10);
}
/*****************************************
*   end of fbe_event_log_test_statistics()
*****************************************/
/*!***************************************************************
 * @fn fbe_event_log_test_uninit()
 ****************************************************************
 * @brief
 *  Validate that we dont log any message until the event log
 *  service is initialized
 *
 * @param .
 * 
 * @return
 *
 * @author
 *  6-July-2010-    Created.
 *
 ****************************************************************/
void fbe_event_log_test_uninit()
{
    fbe_status_t status;
    status = fbe_event_log_write(PHYSICAL_PACKAGE_INFO_LOAD_VERSION,
                                 NULL, 0, "%s %s", __DATE__, __TIME__);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_NOT_INITIALIZED);

}
/**************************************
*   end of fbe_event_log_test_uninit()
**************************************/
/*!***************************************************************
 * @fn fbe_event_log_test_send_packet(fbe_package_id_t package_id,
                                  fbe_payload_control_operation_opcode_t control_code,
                                  fbe_payload_control_buffer_t buffer,
                                  fbe_payload_control_buffer_length_t buffer_length)
 ****************************************************************
 * @brief
 *  This set the packet and directly call the control entry function for event log.
 *
 * @param package_id    Package ID
 * @param control_code  Operational control code
 * @param buffer            Bfffer to store struct data
 * @param buffer_length Buffer length
 * 
 * @return fbe_status_t 
 *
 * @author
 *  6-July - 2010: Vaibhav Gaonkar Created.
 *
 ****************************************************************/
 static fbe_status_t 
 fbe_event_log_test_send_packet(fbe_payload_control_operation_opcode_t control_code,
                                fbe_payload_control_buffer_t buffer,
                                fbe_payload_control_buffer_length_t buffer_length)
{
    fbe_packet_t *                      packet;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_status_t status;

    /* Allocate packet.*/
    packet = (fbe_packet_t *) fbe_api_allocate_memory (sizeof (fbe_packet_t));

    if (packet == NULL) 
    {  
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_packet(packet);
    payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    fbe_payload_control_build_operation (control_operation,
                                         control_code,
                                         buffer,
                                         buffer_length);

     /* Set packet address.*/
    fbe_transport_set_address(packet,
                              FBE_PACKAGE_ID_PHYSICAL,
                              FBE_SERVICE_ID_EVENT_LOG,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID); 


    /* Mark packet with the attribute, either traverse or not */
    fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_NO_ATTRIB);

    /*  Since here directly calling the control entry for event log service
     *  set the current operation before it call.
     */
    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_increment_control_operation_level( payload);

    /* Calling the control entry for event log service*/
    status = fbe_event_log_control_entry(packet);

    fbe_transport_destroy_packet(packet);
    fbe_api_free_memory(packet);

    return status;
}
/***************************************
*   end of fbe_event_log_test_send_packet()
***************************************/
/*!***************************************************************
 * @fn fbe_get_package_id(fbe_package_id_t * package_id)
 ****************************************************************
 * @brief
 *  Defined locally just to statify the build for added fbe_transport.lib
 *
 * @param package_id    Package ID
 * 
 * @return fbe_status_t 
 *
 * @author
 *  6-July - 2010: Vaibhav Gaonkar Created.
 *
 ****************************************************************/
fbe_status_t fbe_get_package_id(fbe_package_id_t * package_id)
{
    *package_id = FBE_PACKAGE_ID_INVALID;
    return FBE_STATUS_OK;
}
