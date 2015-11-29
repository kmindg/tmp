/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file esp_init.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the ESP startup and initialization code.
 * 
 *  
 * @ingroup esp_init_files
 * 
 * @version
 *   23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "csx_ext.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_memory.h"
#include "fbe_transport_memory.h"
#include "fbe_topology.h"
#include "fbe_notification.h"
#include "fbe_scheduler.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_esp.h"
#include "esp_class_table.h"
#include "esp_service_table.h"
#include "fbe/fbe_event_log_api.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_discovery_interface.h"

#define FBE_ESP_NUMBER_OF_CHUNKS 500
#define FBE_ESP_OBJECT_STATE_TRANSITION_TABLE_ENTRIES (FBE_CLASS_ID_ENVIRONMENT_MGMT_LAST - FBE_CLASS_ID_ENVIRONMENT_MGMT_FIRST - 2)

typedef struct fbe_object_state_transition_e
{
    fbe_object_id_t   objectId;
    fbe_bool_t        outOfSpecifiedLifecycleState;
}fbe_object_state_transition_t;

/*******************************
 *   Local Function Prototypes
 ******************************/
static fbe_status_t esp_send_create_object(fbe_class_id_t class_id);
static fbe_status_t esp_start_create_objects(void);
static fbe_status_t esp_sem_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t esp_destroy_all_objects(void);
static fbe_status_t esp_destroy_object(fbe_object_id_t object_id);
static fbe_status_t esp_get_total_objects(fbe_u32_t *total);
static fbe_status_t esp_enumerate_objects(fbe_object_id_t *obj_list, fbe_u32_t total, fbe_u32_t *actual);
static void esp_wait_for_objects_to_be_ready(void);
static void esp_wait_for_encl_mgmt_transition(void);
static fbe_status_t esp_wait_for_objects_transition(fbe_lifecycle_state_t lifecycle_state, 
                                            fbe_u32_t timeout_ms);

/*!***************************************************************
 * esp_log_error()
 **************************************************************** 
 * @brief
 *  ESP trace function to log error messages to the traces
 *
 * @param fmt - Pointer to the format characters
 *
 * @return None
 *
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
static void esp_log_error(const char * fmt, ...)
                          __attribute__((format(__printf_func__,1,2)));
static void esp_log_error(const char * fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_PACKAGE,
                     FBE_PACKAGE_ID_ESP,
                     FBE_TRACE_LEVEL_ERROR,
                     0, /* message_id */
                     fmt, 
                     args);
    va_end(args);
}
/******************************************
 * end esp_log_error()
 ******************************************/

/*!***************************************************************
 * esp_log_info()
 ****************************************************************
 * @brief
 *  ESP trace function to log informational messages to the
 *  traces
 *
 * @param fmt - Pointer to the format characters
 *
 * @return None
 *
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
static void esp_log_info(const char * fmt, ...)
                         __attribute__((format(__printf_func__,1,2)));
static void esp_log_info(const char * fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_PACKAGE,
                     FBE_PACKAGE_ID_ESP,
                     FBE_TRACE_LEVEL_INFO,
                     0, /* message_id */
                     fmt, 
                     args);
    va_end(args);
}
/******************************************
 * end esp_log_info()
 ******************************************/

/*!***************************************************************
 * @fn esp_log_warning()
 ****************************************************************
 * @brief
 *  ESP trace function to log warning messages to the
 *  traces
 *
 * @param fmt - Pointer to the format characters
 *
 * @return None
 *
 * @author
 *  19-Sep-2012: PHE - Created.
 *
 ****************************************************************/
static void esp_log_warning(const char * fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_PACKAGE,
                     FBE_PACKAGE_ID_ESP,
                     FBE_TRACE_LEVEL_WARNING,
                     0, /* message_id */
                     fmt,
                     args);
    va_end(args);
}
/******************************************
 * end esp_log_warning()
 ******************************************/

/*!***************************************************************
 * esp_init()
 ****************************************************************
 * @brief
 *  Main Entry point to init ESP services and objects
 *
 * @param None
 * 
 * @return fbe_status_t - Status of ESP init
 *
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t esp_init(void)
{
    fbe_status_t status;

    esp_log_info("%s starting...\n", __FUNCTION__);

    /* Init the various services needed by ESP */
    status = esp_init_services();

    if(status != FBE_STATUS_OK) {
        esp_log_error("%s: esp_init_services failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    /* Start the process of creating all the objects
     * needed by ESP
     */
    esp_start_create_objects();

    esp_wait_for_encl_mgmt_transition();

    esp_log_info("%s: Waiting for objects to become ready.\n", __FUNCTION__);

    esp_wait_for_objects_to_be_ready();/*SEP will need ESP to be stable when it comes up so we want to make sure all objects are in ready state*/

    esp_log_info("%s: All objects are ready.\n", __FUNCTION__);

    return FBE_STATUS_OK;
}
/******************
 * end esp_init()
 ******************/

/*!***************************************************************
 * esp_destroy()
 ****************************************************************
 * @brief
 *  Main Entry point to destroy ESP services and objects
 *
 * @param None
 * 
 * @return fbe_status_t - Status of ESP destroy operation
 *
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t esp_destroy(void)
{
    fbe_status_t status;

    esp_log_info("%s: starting...\n", __FUNCTION__);

    /* Destroy all the services that were started before during the
     * loading of ESP
     */

    status =  fbe_base_service_send_destroy_command(FBE_SERVICE_ID_EIR);
    if(status != FBE_STATUS_OK) {
        esp_log_error("%s: EIR Serivce destroy failed, status 0x%x.\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_ENVIRONMENT_LIMIT);
    if(status != FBE_STATUS_OK) {
        esp_log_error("%s: fbe_environment_limit_destroy failed, status 0x%x.\n",
                __FUNCTION__, status);
        return status;
    }

    esp_destroy_all_objects();

    status =  fbe_base_service_send_destroy_command(FBE_SERVICE_ID_TRACE);
    if(status != FBE_STATUS_OK) {
        esp_log_error("%s: fbe_trace_destroy failed, status 0x%x.\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_SCHEDULER);
    if(status != FBE_STATUS_OK) {
        esp_log_error("%s: fbe_scheduler_destroy failed, status 0x%x.\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_TOPOLOGY);
    if(status != FBE_STATUS_OK) {
        esp_log_error("%s: fbe_toplogy_destroy failed, status 0x%x.\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_NOTIFICATION);
    if(status != FBE_STATUS_OK) {
        esp_log_error("%s: fbe_notification_destroy failed, status 0x%x.\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_CMI);
    if(status != FBE_STATUS_OK) {
        esp_log_error("%s: CMI service destroy failed, status 0x%x.\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_EVENT_LOG);
    if(status != FBE_STATUS_OK) {
        esp_log_error("%s: fbe_event_log_destroy failed, status 0x%x.\n",
                __FUNCTION__, status);
        return status;
    }

    status =  fbe_base_service_send_destroy_command(FBE_SERVICE_ID_MEMORY);
    if(status != FBE_STATUS_OK) {
        esp_log_error("%s: fbe_memory_destroy failed, status 0x%x.\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_SERVICE_MANAGER);
    if(status != FBE_STATUS_OK) {
        esp_log_error("%s: fbe_service_manager_destroy failed, status 0x%x.\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_EVENT);
    if(status != FBE_STATUS_OK) {
        esp_log_error("%s: fbe_event_destroy failed, status 0x%x.\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_api_esp_common_destroy();
    if(status != FBE_STATUS_OK) {
        esp_log_error("%s: fbe esp common destroy failed, status 0x%x.\n",
                __FUNCTION__, status);
        return status;
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end esp_destroy()
 ******************************************/

/*!***************************************************************
 * esp_sem_completion()
 ****************************************************************
 * @brief
 *  Semaphore completion operation. This is a generic function
 *  that just releases the semaphore
 *
 * @param packet - Pointer to the fbe packet
 * @param context - Pointer to the context which in our case is
 *        is the sempahore to be released
 * 
 * @return fbe_status_t - FBE STATUS OK
 *
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t 
esp_sem_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;

    fbe_semaphore_release(sem, 0, 1, FALSE);

    return FBE_STATUS_OK;
}
/******************************************
 * end esp_sem_completion()
 ******************************************/

/*!***************************************************************
 * esp_init_services()
 ****************************************************************
 * @brief
 *  Main Entry point to init ESP services
 *
 * @param None
 * 
 * @return fbe_status_t - Status of ESP init
 *
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t esp_init_services(void)
{
    fbe_status_t status;
    fbe_object_id_t     objectId;

    esp_log_info("%s starting...\n", __FUNCTION__);

    status = fbe_api_esp_common_init();
    if(status != FBE_STATUS_OK) {
        esp_log_error("%s: API init failed, status: 0x%x\n",
                __FUNCTION__, status);
        return status;
    }

    // Update LED to indicate that ESP has started init
    status = fbe_api_get_board_object_id(&objectId);
    if(status != FBE_STATUS_OK) {
        esp_log_error("%s: get Board Obj failed, status: %X\n",
                __FUNCTION__, status);
    }
    else
    {
        status = fbe_api_board_set_spFaultLED(objectId, LED_BLINK_1HZ, BOOT_CONDITION);
        if(status != FBE_STATUS_OK) {
            esp_log_error("%s: set SP Fault LED 1Hz failed, status: %X\n",
                    __FUNCTION__, status);
        }
        else
        {
            esp_log_info("%s: SP Fault LED updated successfully\n",
                    __FUNCTION__);
        }
    }

    status = fbe_service_manager_init_service_table(esp_service_table);
    if(status != FBE_STATUS_OK) {
        esp_log_error("%s: fbe_service_manager_init_service_table failed, status: 0x%x\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_SERVICE_MANAGER);
    if(status != FBE_STATUS_OK) {
        esp_log_error("%s: esp_init_service failed, status: 0x%x\n",
                __FUNCTION__, status);
        return status;
    }

    /* Initialize the memory */
    status =  fbe_memory_init_number_of_chunks(FBE_ESP_NUMBER_OF_CHUNKS);
    if(status != FBE_STATUS_OK) {
        esp_log_error("%s: fbe_memory_init_number_of_chunks failed, status: 0x%x\n",
                __FUNCTION__, status);
        return status;
    }
    status =  fbe_base_service_send_init_command(FBE_SERVICE_ID_MEMORY);
    if(status != FBE_STATUS_OK) {
        esp_log_error("%s: fbe_memory_init failed, status: 0x%x\n",
                __FUNCTION__, status);
        return status;
    }

    status =  fbe_base_service_send_init_command(FBE_SERVICE_ID_EVENT_LOG);
    if(status != FBE_STATUS_OK) {
        esp_log_error("%s: Event Log Service Init failed, status: 0x%x\n",
                __FUNCTION__, status);
        return status;
    }

    status =  fbe_base_service_send_init_command(FBE_SERVICE_ID_EIR);
    if(status != FBE_STATUS_OK) {
        esp_log_error("%s: EIR Service Init failed, status: 0x%x\n",
                __FUNCTION__, status);
        return status;
    }
    
    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_CMI);
    if(status != FBE_STATUS_OK) {
        esp_log_error("%s: CMI init failed, status: 0x%x\n",
                __FUNCTION__, status);
        //There is currently no CMI driver in service mode, do not exit driverentry for this failure
    }

    /* Initialize topology */
    status = fbe_topology_init_class_table(esp_class_table);
    if(status != FBE_STATUS_OK) {
        esp_log_error("%s: fbe_topology_init_class_table failed, status: 0x%x\n",
                __FUNCTION__, status);
        return status;
    }

    /* Initialize notification service */
    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_NOTIFICATION);
    if(status != FBE_STATUS_OK) {
        esp_log_error("%s: fbe_notification_init failed, status: 0x%x\n",
                __FUNCTION__, status);
        return status;
    }

    /* Initialize scheduler */
    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_SCHEDULER);
    if(status != FBE_STATUS_OK) {
        esp_log_error("%s: fbe_scheduler_init failed, status: 0x%x\n",
                __FUNCTION__, status);
        return status;
    }
    /* Initialize event service */
    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_EVENT);
    if(status != FBE_STATUS_OK) {
        esp_log_error("%s: fbe_event_init failed, status: 0x%x\n",
                __FUNCTION__, status);
        return status;
    }

    /*initialize the drive trace service*/
    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_TRACE);
    if(status != FBE_STATUS_OK) {
        esp_log_error("%s: trace service init failed, status: 0x%x\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_TOPOLOGY);
    if(status != FBE_STATUS_OK) {
        esp_log_error("%s: Environment Init failed, status: 0x%x\n",
                __FUNCTION__, status);
        return status;
    }

    status =  fbe_base_service_send_init_command(FBE_SERVICE_ID_ENVIRONMENT_LIMIT);
    if(status != FBE_STATUS_OK) {
        esp_log_error("%s: Environment Limit  Service Init failed, status: 0x%x\n",
                __FUNCTION__, status);
        return status;
    }

    esp_log_info("%s all services started sucessfully...\n", __FUNCTION__);
    return FBE_STATUS_OK;
}
/******************************************
 * end esp_init_services()
 ******************************************/

/*!***************************************************************
 * esp_get_control_entry()
 ****************************************************************
 * @brief
 *  Returns the Main Entry point to ESP control operations. This
 *  will be used by clients outside of ESP to send control
 *  operation packets
 *
 * @param service_control_entry - Pointer to return the control
 * entry
 * 
 * @return fbe_status_t - FBE STATUS OK
 *
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t esp_get_control_entry(fbe_service_control_entry_t * service_control_entry)
{
    *service_control_entry = fbe_service_manager_send_control_packet;
    return FBE_STATUS_OK;
}
/******************************************
 * end esp_get_control_entry()
 ******************************************/

/*!***************************************************************
 * esp_set_object_destroy_condition()
 ****************************************************************
 * @brief
 *  This function destroys the objects in ESP in preparation for
 *  an unload
 *
 * @param object_id - ID of the object that needs to be
 * destroyed
 * 
 * @return fbe_status_t - Status of destroy operation
 *
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t esp_set_object_destroy_condition(fbe_object_id_t object_id)
{
    fbe_packet_t * packet;
    
    packet = fbe_transport_allocate_packet();
    if (packet == NULL) {
        esp_log_error("%s: fbe_transport_allocate_packet fail\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_packet(packet);
    fbe_transport_build_control_packet(packet, 
                                       FBE_BASE_OBJECT_CONTROL_CODE_SET_DESTROY_COND,
                                       NULL,
                                       0,
                                       0,
                                       NULL,
                                       NULL);
    fbe_transport_set_address(packet,
                              FBE_PACKAGE_ID_ESP,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id);


	fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    fbe_service_manager_send_control_packet(packet);

	fbe_transport_wait_completion(packet);
    
    fbe_transport_release_packet(packet);
    return FBE_STATUS_OK;
}
/******************************************
 * end esp_set_object_destroy_condition()
 ******************************************/

/*!***************************************************************
 * esp_get_io_entry()
 ****************************************************************
 * @brief
 *  Main Entry point to ESP IO Entry. 
 *
 * @param io_entry - Pointer to return the io entry function
 * 
 * @return fbe_status_t - FBE STATUS OK
 *
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t esp_get_io_entry(fbe_io_entry_function_t * io_entry)
{
    * io_entry = fbe_topology_send_io_packet;
    return FBE_STATUS_OK;
}
/******************************************
 * end esp_get_io_entry()
 ******************************************/

/*!***************************************************************
 * fbe_get_package_id()
 ****************************************************************
 * @brief
 *  Returns the package ID of this package which is ESP
 *
 * @param package_id - Buffer to return the package ID value
 * 
 * @return fbe_status_t - FBE STATUS OK
 *
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t fbe_get_package_id(fbe_package_id_t * package_id)
{
    *package_id = FBE_PACKAGE_ID_ESP;
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_get_package_id()
 ******************************************/

/*!***************************************************************
 * esp_start_create_objects()
 ****************************************************************
 * @brief
 *  Initiates the creation of objects in ESP by iterating over
 *  the class table. All objects that needs to be started
 *  automatically should be added to the class table
 *
 * @param None
 * 
 * @return fbe_status_t - FBE STAUS
 *
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t esp_start_create_objects(void)
{
    const fbe_class_methods_t *class_methods;
    fbe_u32_t i = 0;
    fbe_status_t status;

    /* Loop through the class table and start creating the objects
     * defined in the table
     */
    for(i = 0; esp_class_table[i] != NULL; i++) {
        class_methods = esp_class_table[i];
        status = esp_send_create_object(class_methods->class_id);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end esp_start_create_objects()
 ******************************************/

/*!***************************************************************
 * esp_send_create_object()
 ****************************************************************
 * @brief
 *  Sends a packet to the topology to create the object
 *  specified
 *
 * @param class_id : Create an object of the specified class
 * 
 * @return fbe_status_t - Status of Object creation
 *
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t esp_send_create_object(fbe_class_id_t class_id)
{    
    fbe_packet_t * packet;
    fbe_base_object_mgmt_create_object_t base_object_mgmt_create_object;
    fbe_semaphore_t sem;
    fbe_status_t status;

    fbe_semaphore_init(&sem, 0, 1);

    /* Allocate packet */
    packet = fbe_transport_allocate_packet();
    if(packet == NULL) {
        esp_log_error("%s: fbe_transport_allocate_packet failed\n", __FUNCTION__);
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
                                esp_sem_completion,
                                &sem);

    /* Set packet address */
    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_ESP,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                FBE_OBJECT_ID_INVALID);

    fbe_service_manager_send_control_packet(packet);

    fbe_semaphore_wait(&sem, NULL);

    status = fbe_transport_get_status_code(packet);
    if (status != FBE_STATUS_OK) {
        esp_log_error("%s: can't Object of class:%d, status: %X\n",
                __FUNCTION__, class_id, status);
    }
    
    fbe_semaphore_destroy(&sem);
    fbe_transport_release_packet(packet);
    return FBE_STATUS_OK;
}
/******************************************
 * end esp_send_create_object()
 ******************************************/

/*!***************************************************************
 * esp_destroy_all_objects()
 ****************************************************************
 * @brief
 *  Destroys all the objects in the ESP
 * 
 * @param None
 * 
 * @return fbe_status_t - Status of Object creation
 *
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t esp_destroy_all_objects(void)
{
    fbe_object_id_t *                       object_id_list = NULL;
    fbe_u32_t                               tenths_of_sec = 0;
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                               i = 0;
    fbe_u32_t                               total_objects = 0;
    fbe_u32_t                               actual_objects = 0;

    status = esp_get_total_objects(&total_objects);
    if (status != FBE_STATUS_OK) {
        esp_log_error("%s: can't get total objects!\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    object_id_list = (fbe_object_id_t *) fbe_allocate_contiguous_memory((sizeof(fbe_object_id_t) * total_objects));
    if (object_id_list == NULL) {
        esp_log_error("%s: can't allocate memory for object list!\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = esp_enumerate_objects(object_id_list, total_objects, &actual_objects);
    if (status != FBE_STATUS_OK) {
        esp_log_error("%s: can't enumerate objects!\n", __FUNCTION__);
        fbe_release_contiguous_memory(object_id_list);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for(i = 0; i < actual_objects; i++) {
        esp_set_object_destroy_condition(object_id_list[i]);
    }

    fbe_thread_delay(200);

    tenths_of_sec = 0;
    status = esp_enumerate_objects(object_id_list, total_objects, &actual_objects);

    while ((actual_objects != 0) &&
           (tenths_of_sec < 50)) { /* Wait five seconds (at most) */
        /* Wait 100 msec. */
        fbe_thread_delay(200);
        tenths_of_sec++;
        status = esp_enumerate_objects(object_id_list, total_objects, &actual_objects);
        if (status != FBE_STATUS_OK) {
            esp_log_error("%s: can't enumerate objects!\n", __FUNCTION__);
            fbe_release_contiguous_memory(object_id_list);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* print out object_id's */ 
    esp_log_info("%s: Found %X objects\n", __FUNCTION__, actual_objects);
    for(i = 0; i < actual_objects; i++) {
        /*
        sep_log_error("%s: It should never happen \n", __FUNCTION__);
        fbe_thread_delay(0xFFFFFFFF);
        */
        esp_log_error("%s: destroy object_id %X \n", __FUNCTION__, object_id_list[i]);
        esp_destroy_object(object_id_list[i]);
    }
    fbe_release_contiguous_memory(object_id_list);
    return FBE_STATUS_OK;
}
/******************************************
 * end esp_destroy_all_objects()
 ******************************************/

/*!***************************************************************
 * esp_destroy_object()
 ****************************************************************
 * @brief
 *  Sends a destroy command to the specified object
 *
 * @param object_id : Object ID that needs to be destroyed
 * 
 * @return fbe_status_t - Status of Object creation
 *
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t esp_destroy_object(fbe_object_id_t object_id)
{
    fbe_packet_t * packet = NULL;
    fbe_topology_mgmt_destroy_object_t topology_mgmt_destroy_object;

    topology_mgmt_destroy_object.object_id = object_id;
    
    /* Allocate packet */
    packet = fbe_transport_allocate_packet();
    if(packet == NULL) {
        esp_log_error("%s: fbe_transport_allocate_packet failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    fbe_transport_initialize_packet(packet);

    fbe_transport_build_control_packet(packet, 
                                FBE_TOPOLOGY_CONTROL_CODE_DESTROY_OBJECT,
                                &topology_mgmt_destroy_object,
                                sizeof(fbe_topology_mgmt_destroy_object_t),
                                0, /* no output data */
                                NULL,
                                NULL);

    /* Set packet address */
    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_ESP,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                FBE_OBJECT_ID_INVALID);




	fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    fbe_service_manager_send_control_packet(packet);

	fbe_transport_wait_completion(packet);
    
    fbe_transport_release_packet(packet);
    
    return FBE_STATUS_OK;
}
/******************************************
 * end esp_destroy_object()
 ******************************************/
/*!***************************************************************
 * esp_get_total_objects()
 ****************************************************************
 * @brief
 *  Finds out the total number of objects that was created in
 *  the ESP package
 *
 * @param total - Buffer to return the total number of objects
 * 
 * @return fbe_status_t - Status
 *
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t esp_get_total_objects(fbe_u32_t *total)
{
    fbe_packet_t *                              packet = NULL;
    fbe_topology_control_get_total_objects_t    get_total;
	
    /* Allocate packet */
    packet = fbe_transport_allocate_packet();
    if(packet == NULL) {
        esp_log_error("%s: fbe_transport_allocate_packet failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    fbe_transport_initialize_packet(packet);

    fbe_transport_build_control_packet(packet, 
                                       FBE_TOPOLOGY_CONTROL_CODE_GET_TOTAL_OBJECTS,
                                       &get_total,
                                       sizeof(fbe_topology_control_get_total_objects_t),
                                       sizeof(fbe_topology_control_get_total_objects_t),
                                       NULL,
                                       NULL);

    /* Set packet address */
    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_ESP,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                FBE_OBJECT_ID_INVALID);


	fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    fbe_service_manager_send_control_packet(packet);
   
	fbe_transport_wait_completion(packet);

    *total = get_total.total_objects;
    
    fbe_transport_release_packet(packet);

    return FBE_STATUS_OK;

}

/*!***************************************************************
 * esp_enumerate_objects()
 ****************************************************************
 * @brief
 *  Returns the list of objects in the ESP
 * 
 * @param obj_list - Buffer to store the object list
 *        total - Total number of objects that we think there is
 *        acutal - Buffer to store the actual number of objects
 * 
 * @return fbe_status_t - Status
 *
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t 
esp_enumerate_objects(fbe_object_id_t *obj_list, fbe_u32_t total, fbe_u32_t *actual)
{
    fbe_packet_t * packet = NULL;
    fbe_sg_element_t *  sg_list = NULL;
    fbe_sg_element_t *  temp_sg_list = NULL;
    fbe_payload_ex_t   *   payload = NULL;
    fbe_topology_mgmt_enumerate_objects_t   p_topology_mgmt_enumerate_objects;/*small enough to be on the stack*/

    sg_list = (fbe_sg_element_t *)fbe_allocate_contiguous_memory(sizeof(fbe_sg_element_t) * 2);/*one for the entry and one for the NULL termination*/
    if (sg_list == NULL) {
        esp_log_error("%s: fbe_allocate_contiguous_memory failed to allocate sg list\n", __FUNCTION__);
        *actual = 0;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    temp_sg_list = sg_list;
    temp_sg_list->address = (fbe_u8_t *)obj_list;
    temp_sg_list->count = total * sizeof(fbe_object_id_t);
    temp_sg_list ++;
    temp_sg_list->address = NULL;
    temp_sg_list->count = 0;

    p_topology_mgmt_enumerate_objects.number_of_objects = total;
    
    /* Allocate packet */
    packet = fbe_transport_allocate_packet();
    if(packet == NULL) {
        esp_log_error("%s: fbe_transport_allocate_packet failed\n", __FUNCTION__);
        fbe_release_contiguous_memory(sg_list);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    fbe_transport_initialize_packet(packet);

    fbe_transport_build_control_packet(packet, 
                                FBE_TOPOLOGY_CONTROL_CODE_ENUMERATE_OBJECTS,
                                &p_topology_mgmt_enumerate_objects,
                                0, /* no input data */
                                sizeof(fbe_topology_mgmt_enumerate_objects_t),
                                NULL,
                                NULL);

    /* Set packet address */
    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_ESP,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                FBE_OBJECT_ID_INVALID);


    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_set_sg_list(payload, sg_list, 2);

	fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    fbe_service_manager_send_control_packet(packet);
    fbe_transport_wait_completion(packet);

    *actual = p_topology_mgmt_enumerate_objects.number_of_objects_copied;

    fbe_transport_release_packet(packet);
    fbe_release_contiguous_memory(sg_list);
    return FBE_STATUS_OK;
}

static void esp_wait_for_objects_to_be_ready(void)
{
	fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_class_id_t									env_class_id;

	for (env_class_id = FBE_CLASS_ID_ENVIRONMENT_MGMT_FIRST +1; env_class_id < FBE_CLASS_ID_ENVIRONMENT_MGMT_LAST; env_class_id++) {
		if (env_class_id == FBE_CLASS_ID_BASE_ENVIRONMENT) {
			continue;/*base class*/
		}

		status = fbe_api_esp_getObjectId(env_class_id, &object_id);
		if (status != FBE_STATUS_OK){
			fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
				"%s:class:%d, err:%d\n", 
				__FUNCTION__, env_class_id, status);
			return ;
		}

		status = fbe_api_wait_for_object_lifecycle_state(object_id, FBE_LIFECYCLE_STATE_READY, 120000, FBE_PACKAGE_ID_ESP);
		if (status != FBE_STATUS_OK){
			fbe_api_trace (FBE_TRACE_LEVEL_WARNING, 
				"%s:failed to wait for ESP obj:0x%X READY\n", 
				__FUNCTION__, object_id);
			return ;
		}

	}

}

/*!*************************************************************************
* @fn esp_wait_for_encl_mgmt_transition(void)
***************************************************************************
* @brief
*       This function waits for the encl mgmt object getting out of the specialize
*       lifecycle state.
*
* @param None
* @return None
*
* @version
*  19-Sep-2012 PHE - Created.
***************************************************************************/
static void esp_wait_for_encl_mgmt_transition(void)
{
    fbe_object_id_t                                 object_id = FBE_OBJECT_ID_INVALID;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_u32_t                                       timeout_ms = 0;

    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &object_id);
    if (status != FBE_STATUS_OK){
        esp_log_error("%s, ENCL_MGMT, failed to get objectId, status 0x%X.\n",
                      __FUNCTION__, status);
        return;
    }

    /* wait for 40 seconds for encl_mgmt object to get out of the specialize state. */
    timeout_ms = 40000; /* milliseconds*/

    status = fbe_wait_for_object_to_transiton_from_lifecycle_state(object_id,
                                                               FBE_LIFECYCLE_STATE_SPECIALIZE,
                                                               timeout_ms,
                                                               FBE_PACKAGE_ID_ESP);

    if (status != FBE_STATUS_OK){
        esp_log_warning("%s, ENCL_MGMT, failed to get out of SPECIALIZE state in %d seconds.\n",
                     __FUNCTION__, timeout_ms/1000);
        return;
    }

    return;
}

/*!*************************************************************************
* @fn esp_wait_for_objects_to_transition_from_lifecycle_state(void)
***************************************************************************
* @brief
*       This function waits for all the ESP objects getting out of the specified lifecycle state.
*
* @param lifecycle_state
* @param timeout_ms
* @return fbe_status_t
*
* @version
*  09-Nov-2012 PHE - Created.
***************************************************************************/
static fbe_status_t esp_wait_for_objects_transition(fbe_lifecycle_state_t lifecycle_state, 
                                            fbe_u32_t timeout_ms)
{
    const fbe_class_methods_t    *class_methods;
    fbe_status_t                  status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t                     current_time = 0;
    fbe_lifecycle_state_t         current_state = FBE_LIFECYCLE_STATE_NOT_EXIST;
    fbe_object_state_transition_t objectStateTransitionTable[FBE_ESP_OBJECT_STATE_TRANSITION_TABLE_ENTRIES] = {0};
    fbe_u32_t                     i = 0;
    fbe_u32_t                     objectCount = 0;
    fbe_object_id_t               objectId = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                     objectsCompleted = 0;

    esp_log_info("%s starting...\n", __FUNCTION__);

    /* Loop through the class table and populate the initial value for objectStateTransitionTable.
     */
    for(i = 0; esp_class_table[i] != NULL; i++) 
    {
        class_methods = esp_class_table[i];

        status = fbe_api_esp_getObjectId(class_methods->class_id, &objectId);
        if (status != FBE_STATUS_OK)
        {
            esp_log_error("%s:class:%d, status:%d\n", 
                __FUNCTION__, class_methods->class_id, status);
            return status;
        }

        objectStateTransitionTable[i].objectId = objectId;

        if (class_methods->class_id == FBE_CLASS_ID_BASE_ENVIRONMENT)
        {
            objectStateTransitionTable[i].outOfSpecifiedLifecycleState = FBE_TRUE;
        }
        else
        {
            objectStateTransitionTable[i].outOfSpecifiedLifecycleState = FBE_FALSE;
        }
        objectCount++;
    }

    while (current_time < timeout_ms)
    {
        for(i = 0; i < objectCount; i++) 
        {
            if(!objectStateTransitionTable[i].outOfSpecifiedLifecycleState) 
            {
                objectId = objectStateTransitionTable[i].objectId;

                status = fbe_api_get_object_lifecycle_state(objectId, 
                                                            &current_state, 
                                                            FBE_PACKAGE_ID_ESP);
        
                if (status == FBE_STATUS_GENERIC_FAILURE)
                {
                    esp_log_error("%s: objectId %d Get Lifecycle state failed with status %d\n", 
                                  __FUNCTION__, objectId, status);          
                    continue;
                }
        
                if ((status == FBE_STATUS_NO_OBJECT) && (lifecycle_state != FBE_LIFECYCLE_STATE_NOT_EXIST)) 
                { 
                    objectStateTransitionTable[i].outOfSpecifiedLifecycleState = FBE_TRUE;
                    esp_log_error("%s: objectId %d status NO OBJECT, lifecycleState %d in %d ms!\n", 
                                  __FUNCTION__, objectId, lifecycle_state, current_time);   
                    continue;
                }
                    
                if (current_state != lifecycle_state)
                {
                    objectStateTransitionTable[i].outOfSpecifiedLifecycleState = FBE_TRUE;
                    esp_log_info("%s: objectId %d get out of state %d in %d ms!\n", 
                                  __FUNCTION__, objectId, lifecycle_state, current_time);
                    objectsCompleted++;  
                    continue;
                }
            }
        }
        if(objectsCompleted == objectCount)
        {
            // no need to wait the entire time, break out when all objects have transitioned
            break;
        }
        current_time = current_time + 200;  //200ms
        fbe_thread_delay(200);  //200ms
    }

    for(i = 0; i < objectCount; i++) 
    {
        if(!objectStateTransitionTable[i].outOfSpecifiedLifecycleState) 
        {
            objectId = objectStateTransitionTable[i].objectId;
            esp_log_error("%s: objectId %d didn't get out of state %d in %d ms!\n", 
                  __FUNCTION__, objectId, lifecycle_state, timeout_ms);
        }
    }
    return FBE_STATUS_OK;
}
