/***************************************************************************
 * Copyright (C) EMC Corporation  2010
 * All rights reserved. Licensed material -- property of EMC
 * Corporation Unpublished
 * - all rights reserved under the copyright laws
 * of the United States
 ***********************************************************************************/

/************************************************************************************
 *                          eir_main.c
 ************************************************************************************
 *
 * DESCRIPTION:
 *   Comments for Dummies:  Since I have complained about the lack of comments in other code in the services directory,
 *   I will here provide some high level comments describing how this eir service functions.  Some of these comments are
 *   fairly basic, but could be much appreciated by new members of the team.
 *
 *   This file contains functions related to Environmental Information Reporting.
 *
 ************************************************************************************
 *
 *    The EIR service is built as a library, fbe_eir.lib, that is included in the ESP package.
 *
 *    The library target type is specified in SOURCES.mak located in the fbe\src\services\eir\src directory:
 *
 *    TARGETNAME = fbe_eir
 *    TARGETTYPE = LIBRARY
 *    DLLTYPE = REGULAR
 *    TARGETMODES = kernel user user32
 *    UMTYPE = console
 *    SOURCES = \
 *        eir_main.c \
 *        eir_state.c
 *
 **----------------------------------------------------------------------------------------------
 *    EIR is listed as one of several other services that ESP uses:
 *
 *    esp_service_table.h:
 *
 *    extern fbe_service_methods_t fbe_memory_service_methods;
 *    extern fbe_service_methods_t fbe_notification_service_methods;
 *    extern fbe_service_methods_t fbe_service_manager_service_methods;
 *    extern fbe_service_methods_t fbe_scheduler_service_methods;
 *    extern fbe_service_methods_t fbe_event_log_service_methods;
 *    extern fbe_service_methods_t fbe_topology_service_methods;
 *    extern fbe_service_methods_t fbe_event_service_methods;
 *    extern fbe_service_methods_t fbe_trace_service_methods;
 *    extern fbe_service_methods_t fbe_cmi_service_methods;
 *    extern fbe_service_methods_t fbe_eir_service_methods;
 *
 *    // Note we will compile different tables for different packages
 *    static const fbe_service_methods_t * esp_service_table[] = &fbe_memory_service_methods,
 *                                                                &fbe_notification_service_methods,
 *                                                                &fbe_service_manager_service_methods,
 *                                                                &fbe_scheduler_service_methods,
 *                                                                &fbe_event_log_service_methods,
 *                                                                &fbe_topology_service_methods,
 *                                                                &fbe_event_service_methods,
 *                                                                &fbe_trace_service_methods,
 *                                                                &fbe_cmi_service_methods,
 *                                                                &fbe_eir_service_methods,
 *    NULL };
 *----------------------------------------------------------------------------------------------
 *
 *    ESP starts up each of these services in function fbe_status_t esp_init_services(void), by sending the service ID to
 *    the base service.  Only two services are shown here for clarity:
 *
 *    esp_init.c
 *
 *        status =  fbe_base_service_send_init_command(FBE_SERVICE_ID_EVENT_LOG);
 *        if(status != FBE_STATUS_OK)
 *        {
 *            esp_log_error("%s: Event Log Serivce Init failed, status: %X\n",
 *                    __FUNCTION__, status);
 *            return status;
 *        }
 *
 *        status =  fbe_base_service_send_init_command(FBE_SERVICE_ID_EIR);
 *        if(status != FBE_STATUS_OK)
 *        {
 *            esp_log_error("%s: EIR Serivce Init failed, status: %X\n",
 *                    __FUNCTION__, status);
 *            return status;
 *        }
 *
 *    The eir service ID is tied into the eir entry function with this line at the top of eir_main.c:
 *
 *    fbe_service_methods_t fbe_eir_service_methods = {FBE_SERVICE_ID_EIR, fbe_eir_entry};
 *
 **----------------------------------------------------------------------------------------------
 *    Here's the entry function in eir_main.c:

 *    fbe_status_t fbe_eir_entry(fbe_packet_t * packet)
 *
 *    Once in fbe_eir_entry, fbe_eir_init(packet) is called.  Inside fbe_eir_init, various things are initialized and most
 *       importantly, the eir thread is started.  The eir_thread_handle is used to reference the thread when shutting it down.
 *       The eir_thread_func is a function pointer that's called when the thread starts.
 *
 *        // Kick off the eir thread here.....
 *        eir_thread_flag = EIR_THREAD_RUN;
 *        fbe_thread_init(&eir_thread_handle, eir_thread_func, NULL);
 *
 *    Function eir_thread_func is primarily a loop that contains a semaphore that runs one loop iteration every 3 seconds.
 *    The loop is terminated if the eir_thread_flag is changed to something other than EIR_THREAD_RUN.  This flag is set to
 *    EIR_THREAD_STOP by eir_destroy() which is called when a FBE_BASE_SERVICE_CONTROL_CODE_DESTROY control code message
 *    is sent to the eir service, as handled by fbe_eir_entry().
 *
 *  HISTORY:
 *      20-Apr-2010    Dan McFarland  Created.
 *
 ***********************************************************************************/

#include "eir_main.h"
#include "fbe/fbe_api_esp_eir_interface.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_esp_encl_mgmt.h"
#include "fbe/fbe_eir_info.h"

/* Declare our service methods */
fbe_status_t fbe_eir_entry(fbe_packet_t * packet);
fbe_service_methods_t fbe_eir_service_methods = {FBE_SERVICE_ID_EIR, fbe_eir_entry};

// This needs to move down inside the service:
typedef enum eir_thread_flag_e
{
    EIR_THREAD_RUN = 1,
    EIR_THREAD_STOP,
    EIR_THREAD_DONE
}eir_thread_flag_t;

typedef struct eir_service_s
{   
    eir_thread_flag_t   eir_thread_flag;
    fbe_base_service_t  base_service;
    fbe_spinlock_t      eir_data_lock;
}eir_service_t;

static fbe_eir_service_data_t   eir_data;
static eir_service_t            eir_service;

static fbe_semaphore_t                     eir_semaphore;
static fbe_thread_t                        eir_thread_handle;

/************************************************************************************
 *                          fbe_eir_trace
 ************************************************************************************
 *
 * DESCRIPTION:
 *   Local trace facility so eir can have its own trace level.
 *
 *  PARAMETERS:
 *      trace_level - Trace level value that message needs to be
 *      logged at
 *      message_id - Message Id
 *      fmt - Insertion Strings
 *
 *  RETURN VALUES/ERRORS:
 *       None
 *
 *  NOTES:
 *
 * HISTORY:
 *      20-Apr-2010    Dan McFarland  Created.
 *
 ***********************************************************************************/
void fbe_eir_trace(fbe_trace_level_t trace_level,
                         fbe_trace_message_id_t message_id,
                         const char * fmt, ...)
{
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t service_trace_level;

    // va_list is a ddk type that enables variable-length function argument lists.
    va_list args;

    service_trace_level = default_trace_level = fbe_trace_get_default_trace_level();

    if (fbe_base_service_is_initialized((fbe_base_service_t *) &eir_service))
    {
        service_trace_level = fbe_base_service_get_trace_level((fbe_base_service_t *) &eir_service);
        if (default_trace_level > service_trace_level)
        {
            service_trace_level = default_trace_level;
        }
    }

    if (trace_level > service_trace_level)
    {
      return;
    }

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_SERVICE,
                     FBE_SERVICE_ID_EIR,
                     trace_level,
                     message_id,
                     fmt,
                     args);  // args is a variable length list.
    va_end(args);
}

/************************************************************************************
 *                          fbe_eir_init
 ************************************************************************************
 *
 * DESCRIPTION:
 *   This function initializes the eir service and calls
 *   any platform specific initialization routines
 *
 *  PARAMETERS:
 *      packet - Pointer to the packet received by the event log
 *      service
 *
 *  RETURN VALUES/ERRORS:
 *       fbe_status_t
 *
 *  NOTES:
 *
 * HISTORY:
 *      20-Apr-2010    Dan McFarland  Created.
 *
 ***********************************************************************************/
static fbe_status_t fbe_eir_init(fbe_packet_t * packet)
{
    fbe_base_service_set_service_id((fbe_base_service_t *) &eir_service, FBE_SERVICE_ID_EIR);

    fbe_base_service_init((fbe_base_service_t *) &eir_service);
    fbe_spinlock_init(&eir_service.eir_data_lock);

    // The semaphore is used for waking up the eir thread on 3 second intervals
    fbe_semaphore_init(&eir_semaphore, 0, 0x0fffffff); // It may be many requests

    // Initialize the data structure that is used to pass data up to the host:
    fbe_eir_clear_data(&eir_data);

    // Kick off the eir thread here.....
    fbe_thread_init(&eir_thread_handle, "fbe_eir", eir_thread_func, NULL);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

/************************************************************************************
 *                          eir_destroy
 ************************************************************************************
 *
 * DESCRIPTION:
 *   This function performs the necessary clean up in
 *   preparation to stop the eir service
 *
 *  PARAMETERS:
 *      packet - Pointer to the packet received by the event log
 *      service
 *
 *  RETURN VALUES/ERRORS:
 *       fbe_status_t
 *
 *  NOTES:
 *
 * HISTORY:
 *      20-Apr-2010    Dan McFarland  Created.
 *
 ***********************************************************************************/
static fbe_status_t eir_destroy(fbe_packet_t * packet)
{
    eir_service.eir_thread_flag = EIR_THREAD_STOP;
    fbe_semaphore_release(&eir_semaphore, 0, 1, FALSE);

    // The KeWaitForSingleObject routine puts the current thread into a wait state
    //   until the given dispatcher object is set to a signaled state or (optionally) until the wait times out.
    fbe_thread_wait(&eir_thread_handle);
    fbe_thread_destroy(&eir_thread_handle);

    fbe_eir_trace(FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s: <<<<<<<<<<<< EIR EXIT >>>>>>>>>>>>\n", __FUNCTION__);

    fbe_semaphore_destroy(&eir_semaphore);
    fbe_spinlock_destroy(&eir_service.eir_data_lock);
    fbe_base_service_destroy((fbe_base_service_t *)  &eir_service);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

/************************************************************************************
 *                          eir_entry
 ************************************************************************************
 *
 * DESCRIPTION:
 *   This function starts the eir thread and services incoming control packets.
 *
 *  PARAMETERS:
 *      packet - Pointer to the packet received by the event log
 *      service
 *
 *  RETURN VALUES/ERRORS:
 *       fbe_status_t
 *
 *  NOTES:
 *
 * HISTORY:
 *      20-Apr-2010    Dan McFarland  Created.
 *
 ***********************************************************************************/
fbe_status_t fbe_eir_entry(fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_control_operation_opcode_t control_code = FBE_PAYLOAD_CONTROL_OPERATION_OPCODE_INVALID;

    control_code = fbe_transport_get_control_code(packet);
    if(control_code == FBE_BASE_SERVICE_CONTROL_CODE_INIT)
    {
        status = fbe_eir_init(packet);
        return status;
    }

    // No operation is allowed until the eir service is
    //  initialized. Return the status immediately
    if(!fbe_base_service_is_initialized((fbe_base_service_t *) &eir_service))
    {
        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_NOT_INITIALIZED;
    }
                          
    switch(control_code)
    {
        case FBE_BASE_SERVICE_CONTROL_CODE_DESTROY:
            status = eir_destroy(packet);
            break;
        case FBE_EVENT_EIR_CONTROL_CODE_GET_STATUS:
            //status = fbe_eir_getStatus(packet);
            break;
        case FBE_EVENT_EIR_CONTROL_CODE_GET_DATA:
            status = fbe_esp_eir_getData(packet);
            break;
        case FBE_EVENT_EIR_CONTROL_CODE_GET_SAMPLE:
            status = fbe_esp_eir_getSample(packet);
            break;
        default:
            fbe_eir_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s: EIR Unexpected control code:%d\n", __FUNCTION__, control_code);
    
            status = fbe_base_service_control_entry((fbe_base_service_t*)&eir_service, packet);
            break;
    }
      
    return status;
}

/************************************************************************************
 *                          eir_thread_func
 ************************************************************************************
 *
 * DESCRIPTION: This function sends usurper commands to the enclosure and sps managemtent
 *              objects to consolidate summarized environmental data.
 *
 *  PARAMETERS:
 *
 *  RETURN VALUES/ERRORS:
 *
 *  NOTES:
 *
 * HISTORY:
 *      20-Apr-2010    Dan McFarland  Created.
 *
 ***********************************************************************************/
static void eir_thread_func(void * context)
{
    fbe_esp_sps_mgmt_spsInputPowerTotal_t   *spsEirDataPtr;
    fbe_esp_encl_mgmt_get_eir_info_t        *enclEirDataPtr;
    fbe_eir_input_power_sample_t            *inputPowerSamplePtr;
    fbe_eir_input_power_sample_t            inputPowerSum;
    fbe_u32_t                               inputPowerCnt;
    fbe_u32_t                               sampleIndex;

    FBE_UNREFERENCED_PARAMETER(context);

    eir_service.eir_thread_flag = EIR_THREAD_RUN;

    // allocate necessary buffers for EIR data
    spsEirDataPtr = fbe_memory_ex_allocate(sizeof(fbe_esp_sps_mgmt_spsInputPowerTotal_t));
    enclEirDataPtr = fbe_memory_ex_allocate(sizeof(fbe_esp_encl_mgmt_get_eir_info_t));

    while(1)
    {                                                               
        fbe_eir_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s, EIR Thread: Wait %d seconds.....\n",
                      __FUNCTION__,
                      (FBE_STATS_POLL_INTERVAL / FBE_TIME_MILLISECONDS_PER_SECOND));
        // Let the CPU work on something else while we kill some time:
        fbe_semaphore_wait_ms(&eir_semaphore, FBE_STATS_POLL_INTERVAL);

        if (eir_service.eir_thread_flag == EIR_THREAD_RUN)
        {
 
            /*
             * Request InputPower from Enclosures
             */
            fbe_zero_memory(enclEirDataPtr, sizeof(fbe_esp_encl_mgmt_get_eir_info_t));
            enclEirDataPtr->eirInfoType = FBE_ENCL_MGMT_EIR_IP_TOTAL;
            fbe_esp_send_usurper_cmd(FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_EIR_INFO, 
                                     FBE_CLASS_ID_ENCL_MGMT, 
                                     (fbe_payload_control_buffer_t *)(enclEirDataPtr),
                                     sizeof(fbe_esp_encl_mgmt_get_eir_info_t));
            eir_data.totalEnclInputPower = enclEirDataPtr->eirEnclInfo.eirEnclInputPowerTotal.inputPowerAll;
            fbe_eir_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, TotalEnclInputPower %d, status %d\n", 
                          __FUNCTION__, 
                          eir_data.totalEnclInputPower.inputPower,
                          eir_data.totalEnclInputPower.status);

            /*
             * Request InputPower from SPS's
             */
            fbe_zero_memory(spsEirDataPtr, sizeof(fbe_esp_sps_mgmt_spsInputPowerTotal_t));
            fbe_esp_send_usurper_cmd(FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SPS_INPUT_POWER_TOTAL, 
                                     FBE_CLASS_ID_SPS_MGMT, 
                                     (fbe_payload_control_buffer_t *)(spsEirDataPtr),
                                     sizeof(fbe_esp_sps_mgmt_spsInputPowerTotal_t));
            eir_data.totalSpsInputPower = spsEirDataPtr->inputPowerAll;
            fbe_eir_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, TotalSpsInputPower %d, status %d\n", 
                          __FUNCTION__, 
                          eir_data.totalSpsInputPower.inputPower,
                          eir_data.totalSpsInputPower.status);

            /*
             * Perform EIR calculations (Average & Max)
             */
            // update count
            if (eir_data.sampleCount < FBE_SAMPLE_DATA_SIZE)
            {
                eir_data.sampleCount++;
            }

            //calculate Current InputPower & save sample
            eir_data.arrayCurrentInputPower.inputPower = 
                eir_data.totalEnclInputPower.inputPower + eir_data.totalSpsInputPower.inputPower;
            eir_data.arrayCurrentInputPower.status = 
                eir_data.totalEnclInputPower.status | eir_data.totalSpsInputPower.status;
            eir_data.inputPowerSamples[eir_data.sampleIndex] = eir_data.arrayCurrentInputPower;

            //set Average & Max InputPower
            inputPowerSamplePtr = &(eir_data.inputPowerSamples[0]);
            inputPowerSum.inputPower = 0;
            inputPowerSum.status = FBE_ENERGY_STAT_UNINITIALIZED;
            inputPowerCnt = 0;
            for (sampleIndex = 0; sampleIndex < eir_data.sampleCount; sampleIndex++)
            {
                if (inputPowerSamplePtr->status == FBE_ENERGY_STAT_VALID)
                {
                    inputPowerSum.inputPower += inputPowerSamplePtr->inputPower;
                    inputPowerSum.status = inputPowerSamplePtr->status;
                    inputPowerCnt++;
                    if (inputPowerSamplePtr->inputPower > eir_data.arrayMaxInputPower.inputPower)
                    {
                        eir_data.arrayMaxInputPower.inputPower = inputPowerSamplePtr->inputPower;
                        eir_data.arrayMaxInputPower.status = FBE_ENERGY_STAT_VALID;
                    }
                }
                inputPowerSamplePtr++;
            }
            eir_data.arrayAverageInputPower.status = inputPowerSum.status;
            if (inputPowerCnt > 0)
            {
                eir_data.arrayAverageInputPower.inputPower = inputPowerSum.inputPower / inputPowerCnt;
                // update Status if sample size too small
                if (inputPowerCnt < FBE_SAMPLE_DATA_SIZE)
                {
                    eir_data.arrayAverageInputPower.status |= FBE_ENERGY_STAT_SAMPLE_TOO_SMALL;
                }
                fbe_eir_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, sum %d, cnt %d, status 0x%x\n",
                              __FUNCTION__,
                              inputPowerSum.inputPower, inputPowerCnt, inputPowerSum.status);
            }

            fbe_eir_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, Array Current InputPower %d, status 0x%x\n",
                          __FUNCTION__,
                          eir_data.arrayCurrentInputPower.inputPower, eir_data.arrayCurrentInputPower.status);
            fbe_eir_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, Array Average InputPower %d, status 0x%x\n",
                          __FUNCTION__,
                          eir_data.arrayAverageInputPower.inputPower, eir_data.arrayAverageInputPower.status);
            fbe_eir_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, Array Max InputPower %d, status 0x%x\n",
                          __FUNCTION__,
                          eir_data.arrayMaxInputPower.inputPower, eir_data.arrayMaxInputPower.status);

            // update index
            if (++eir_data.sampleIndex >= FBE_SAMPLE_DATA_SIZE)
            {
                eir_data.sampleIndex = 0;
            }

            fbe_eir_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, EIR Thread: Wakeup, sampleCount %d, sampleIndex %d\n",
                          __FUNCTION__, eir_data.sampleCount, eir_data.sampleIndex);

        }
        else
        {
            break;
        }
    }

    fbe_eir_trace(FBE_TRACE_LEVEL_INFO,
                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                  "%s eir thread exiting \n", __FUNCTION__);

    // release buffers
    fbe_memory_ex_release(spsEirDataPtr);
    fbe_memory_ex_release(enclEirDataPtr);

    eir_service.eir_thread_flag = EIR_THREAD_DONE;
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

/************************************************************************************
 *                          fbe_eir_clear_data
 ************************************************************************************
 *
 * DESCRIPTION:
 *   This function clears the data structures that store array-specific environmental
 *      info
 *
 *  PARAMETERS:
 *      event_log_stat _ pointer to the buffer whose
 *      information needs to be cleared
 *
 *  RETURN VALUES/ERRORS:
 *       fbe_status_t
 *
 *  NOTES:
 *      The lock needs to be held by the caller, if needed
 *
 * HISTORY:
 *      21-Apr-2010    Dan McFarland  Created.
 *
 ***********************************************************************************/
static fbe_status_t fbe_eir_clear_data(fbe_eir_service_data_t *eir_dataPtr)
{
    // Initialize all the eir data here.
    fbe_eir_trace(FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s  Initialize EIR data structure\n", __FUNCTION__);

    fbe_set_memory(&eir_data, 0, sizeof(fbe_eir_service_data_t));

    return(FBE_STATUS_OK);
}

/************************************************************************************
 *                          fbe_api_esp_eir_getStatus
 ************************************************************************************
 *
 * DESCRIPTION:
 *   Return status from eir.  This is used to determine whether the EIR thread is running,
 *   primarily for simulation testing.
 *
 *  PARAMETERS:
 *      eir_dataPtr _ pointer to the buffer whose
 *      information needs to be returned
 *
 *  RETURN VALUES/ERRORS:
 *       fbe_status_t
 *
 *  NOTES:
 *
 * HISTORY:
 *      21-Apr-2010    Dan McFarland  Created.
 *
 ***********************************************************************************/

fbe_status_t fbe_eir_getStatus(fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_length_t buffer_length;
    fbe_eir_data_t  *eir_dataPtr = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    // Get the pointer to the return buffer in the payload, and get a local pointer to it, eir_dataPtr.
    fbe_payload_control_get_buffer(control_operation, &eir_dataPtr);

    if(eir_dataPtr == NULL)
    {
        fbe_eir_trace(FBE_TRACE_LEVEL_ERROR, 
                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                      "%s NULL POINTER\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // Get the size of the buffer from the packet:
    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);

    // If the expected buffer is not the same length as our data structure, then there's a nasty bug here.  
    if(buffer_length != sizeof(fbe_eir_data_t))
    {
        fbe_eir_trace(FBE_TRACE_LEVEL_ERROR, 
                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                      "%s Invalid buffer_length:%d\n", __FUNCTION__, buffer_length);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Need to get the lock first then copy the data to the buffer
     * passed in
     */
    fbe_spinlock_lock(&eir_service.eir_data_lock);
    fbe_copy_memory(eir_dataPtr, &eir_data, buffer_length);   // dst,src,length
    fbe_spinlock_unlock(&eir_service.eir_data_lock);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/************************************************************************************
 *                          fbe_api_esp_eir_getData
 ************************************************************************************
 *
 * DESCRIPTION:
 *   Return eir data to an outside requestor.
 *
 *  PARAMETERS:
 *      eir_dataPtr _ pointer to the buffer whose
 *      information needs to be returned
 *
 *  RETURN VALUES/ERRORS:
 *       fbe_status_t
 *
 *  NOTES:
 *
 * HISTORY:
 *      21-Apr-2010    Dan McFarland  Created.
 *
 ***********************************************************************************/

fbe_status_t fbe_esp_eir_getData(fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_length_t buffer_length;
    fbe_eir_data_t  *eir_dataPtr = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    // Get the pointer to the return buffer in the payload, and get a local pointer to it, eir_dataPtr.
    fbe_payload_control_get_buffer(control_operation, &eir_dataPtr);

    // getStatus was copied here just to satisfy build needs for now....

    if(eir_dataPtr == NULL)
    {
        fbe_eir_trace(FBE_TRACE_LEVEL_ERROR, 
                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                      "%s NULL POINTER\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // Get the size of the buffer from the packet:
    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);

    // If the expected buffer is not the same length as our data structure, then there's a nasty bug here.  
    if(buffer_length != sizeof(fbe_eir_data_t))
    {
        fbe_eir_trace(FBE_TRACE_LEVEL_ERROR, 
                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                      "%s Invalid buffer_length:%d\n", __FUNCTION__, buffer_length);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Need to get the lock first then copy the data to the buffer
     * passed in
     */
    fbe_spinlock_lock(&eir_service.eir_data_lock);
//    fbe_copy_memory(eir_dataPtr, &eir_data, buffer_length);   // dst,src,length
    eir_dataPtr->arrayInputPower.inputPower = 
        eir_data.totalEnclInputPower.inputPower + eir_data.totalSpsInputPower.inputPower;
    eir_dataPtr->arrayInputPower.status = 
        eir_data.totalEnclInputPower.status | eir_data.totalSpsInputPower.status;

    eir_dataPtr->arrayAvgInputPower = eir_data.arrayAverageInputPower;
    eir_dataPtr->arrayMaxInputPower = eir_data.arrayMaxInputPower;

    fbe_spinlock_unlock(&eir_service.eir_data_lock);
    fbe_eir_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                  FBE_TRACE_MESSAGE_ID_INFO,
                  "%s, Array inputPower %d, status 0x%x\n", 
                  __FUNCTION__, 
                  eir_dataPtr->arrayInputPower.inputPower,
                  eir_dataPtr->arrayInputPower.status);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/************************************************************************************
 *                          fbe_esp_eir_getSample
 ************************************************************************************
 *
 * DESCRIPTION:
 *   Return eir input power sample data to an outside requestor.
 *
 *  PARAMETERS:
 *      sample_data_ptr _ pointer to the buffer whose
 *      information needs to be returned
 *
 *  RETURN VALUES/ERRORS:
 *       fbe_status_t
 *
 *  NOTES:
 *
 * HISTORY:
 *      09-May-2012    Dongz  Created.
 *
 ***********************************************************************************/

fbe_status_t fbe_esp_eir_getSample(fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_length_t buffer_length;
    fbe_eir_input_power_sample_t  *sample_data_ptr;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    // Get the pointer to the return buffer in the payload, and get a local pointer to it, eir_dataPtr.
    fbe_payload_control_get_buffer(control_operation, &sample_data_ptr);

    // getStatus was copied here just to satisfy build needs for now....

    if(sample_data_ptr == NULL)
    {
        fbe_eir_trace(FBE_TRACE_LEVEL_ERROR,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s NULL POINTER\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // Get the size of the buffer from the packet:
    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);

    // If the expected buffer is not the same length as our data structure, then there's a nasty bug here.
    if(buffer_length != sizeof(fbe_eir_input_power_sample_t)* FBE_SAMPLE_DATA_SIZE)
    {
        fbe_eir_trace(FBE_TRACE_LEVEL_ERROR,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s Invalid buffer_length:%d\n", __FUNCTION__, buffer_length);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Need to get the lock first then copy the data to the buffer
     * passed in
     */
    fbe_spinlock_lock(&eir_service.eir_data_lock);

    fbe_copy_memory(
            sample_data_ptr,
            eir_data.inputPowerSamples,
            sizeof(fbe_eir_input_power_sample_t) * eir_data.sampleCount);

    fbe_spinlock_unlock(&eir_service.eir_data_lock);

    fbe_eir_trace(FBE_TRACE_LEVEL_INFO,
                  FBE_TRACE_MESSAGE_ID_INFO,
                  "%s, Copyed %d samples\n",
                  __FUNCTION__,
                  eir_data.sampleCount);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

