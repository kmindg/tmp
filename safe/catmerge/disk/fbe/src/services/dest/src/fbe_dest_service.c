
/***************************************************************************
* Copyright (C) EMC Corporation 2009
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!*************************************************************************
* @file fbe_dest_service.c
***************************************************************************
*
* @brief
*  This file contains the main entry point for the drive error simulation 
*  tool service.
*
* History:
*   03/06/2012  Created. kothal
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/

#include "fbe_dest_service_private.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_dest_service.h"

/*!*******************************************************************
 * @def DEST_INJECTION_MAX_ERRORS
 *********************************************************************
 * @brief Number of maximum errors inserted using the single dest error 
 *           injection record. This number has been derived from the 
 *           FBE_MEDIA_ERROR_EDGE_TAP_MAX_RECORDS 
 *
 *********************************************************************/
#define DEST_INJECTION_MAX_ERRORS 256

typedef fbe_u32_t fbe_dest_magic_num_t;
enum {
   FBE_DEST_MAGIC_NUM = 0x44455354,  /* ASCII for DEST */
};

static fbe_dest_service_t  fbe_dest_service;

/* Declare our service methods */
fbe_status_t fbe_dest_service_control_entry(fbe_packet_t * packet); 
fbe_service_methods_t fbe_dest_service_methods = {FBE_SERVICE_ID_DEST, fbe_dest_service_control_entry};

/* This variable maintains the state of the DEST. It is initialized 
 * to DEST_NOT_INIT. Other states are init, start and stop.
 */
fbe_dest_state_monitor fbe_dest_state = {FBE_DEST_NOT_INIT};

/*************************
*   FUNCTION DEFINITIONS
*************************/
static fbe_status_t fbe_dest_service_add_record (fbe_packet_t * packet);
static fbe_status_t fbe_dest_service_get_record (fbe_packet_t * packet);
static fbe_status_t fbe_dest_service_get_record_next (fbe_packet_t * packet);
static fbe_status_t fbe_dest_service_get_record_handle(fbe_packet_t * packet_p);
static fbe_status_t fbe_dest_service_search_record(fbe_packet_t * packet_p);
static fbe_status_t fbe_dest_service_remove_record (fbe_packet_t * packet);
static fbe_status_t fbe_dest_service_remove_all_records (void);
static fbe_status_t fbe_dest_service_record_update_times_to_insert (fbe_packet_t * packet_p);
static fbe_status_t fbe_dest_service_start(void);
static fbe_status_t fbe_dest_service_stop(void);
static fbe_status_t fbe_dest_service_get_state(fbe_packet_t * packet_p);

/*!**************************************************************
* fbe_dest_service_init()
****************************************************************
* @brief
*  Initialize the DEST service.
*
* @param packet_p - The packet sent with the init request.
*
* @return fbe_status_t - Always FBE_STATUS_OK.
* 
* History:
*   03/06/2012  Created. kothal
*
****************************************************************/
static fbe_status_t fbe_dest_service_init(fbe_packet_t * packet_p)
{
   fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
   fbe_dest_service_trace(FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s: entry\n", __FUNCTION__);

   fbe_base_service_set_service_id((fbe_base_service_t *) &fbe_dest_service, FBE_SERVICE_ID_DEST);
   fbe_base_service_set_trace_level((fbe_base_service_t *) &fbe_dest_service, fbe_trace_get_default_trace_level());

   fbe_base_service_init((fbe_base_service_t *) &fbe_dest_service);

   status = fbe_dest_init();  
   if(status == FBE_STATUS_OK)
   {
	   /* set the DEST state to initialized. 
      */
       fbe_dest_state.current_state = FBE_DEST_INIT;
	   fbe_dest_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
							  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							  "%s: fbe_dest initialization\n", __FUNCTION__);
   }

   /* Always complete this request with good status.
    */
   fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
   fbe_transport_complete_packet(packet_p);

   return status; 
}
/******************************************
* end fbe_dest_service_init()
******************************************/

/*!**************************************************************
* fbe_dest_service_destroy()
****************************************************************
* @brief
*  Destroy the dest service.
*
* @param packet_p - Packet for the destroy operation.          
*
* @return FBE_STATUS_OK - Destroy successful.
*         FBE_STATUS_GENERIC_FAILURE - Destroy Failed.
* 
* History:
*   03/06/2012  Created. kothal
*
****************************************************************/

static fbe_status_t fbe_dest_service_destroy(fbe_packet_t * packet_p)
{
  fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

   fbe_dest_service_trace(FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s: entry\n", __FUNCTION__);

   status = fbe_dest_destroy();   
   fbe_base_service_destroy((fbe_base_service_t *) &fbe_dest_service);
   return status;
}
/******************************************
* end fbe_dest_service_destroy()
******************************************/

/*!**************************************************************
* fbe_dest_service_trace()
****************************************************************
* @brief
*  Trace function for use by the DEST service.
*  Takes into account the current global trace level and
*  the locally set trace level.
*
* @param trace_level - trace level of this message.
* @param message_id - generic identifier.
* @param fmt... - variable argument list starting with format.
*
* @return None.  
*
* History:
*   03/06/2012  Created. kothal
*
****************************************************************/
void fbe_dest_service_trace(fbe_trace_level_t trace_level,
                           fbe_trace_message_id_t message_id,
                           const char * fmt, ...)
{
   fbe_trace_level_t default_trace_level;
   fbe_trace_level_t service_trace_level;

   va_list args;

   /* Assume we are using the default trace level.
    */
   service_trace_level = default_trace_level = fbe_trace_get_default_trace_level();

   /* The global default trace level overrides the service trace level.
    */
   if (fbe_base_service_is_initialized(&fbe_dest_service.base_service))
   {
       service_trace_level = fbe_base_service_get_trace_level(&fbe_dest_service.base_service);
       if (default_trace_level > service_trace_level) 
       {
           /* Global trace level overrides the current service trace level.
            */
           service_trace_level = default_trace_level;
       }
   }

   /* If the service trace level passed in is greater than the
    * current chosen trace level then just return.
    */
   if (trace_level > service_trace_level) 
   {
       return;
   }

   va_start(args, fmt);
   fbe_trace_report(FBE_COMPONENT_TYPE_SERVICE,
                    FBE_SERVICE_ID_DEST,
                    trace_level,
                    message_id,
                    fmt, 
                    args);
   va_end(args);
   return;
}
/******************************************
* end fbe_dest_service_trace()
******************************************/
/*!**************************************************************
* fbe_dest_service_start()
****************************************************************
* @brief
*  Start the dest service.
*       
*
* @return FBE_STATUS_OK - Start successful.
* 
*
* History:
*   03/06/2012  Created. kothal
*
****************************************************************/

static fbe_status_t fbe_dest_service_start()
{
   fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
   fbe_dest_service_trace(FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s: entry\n", __FUNCTION__);
    if(fbe_dest_state.current_state != FBE_DEST_START)
    {
    /* Start Error Insertion.
    */        
        fbe_dest_state.current_state = FBE_DEST_START;
        status = fbe_dest_start();
    }
   return status;
}
/******************************************
* end fbe_dest_service_start()
******************************************/

/*!**************************************************************
* fbe_dest_service_stop()
****************************************************************
* @brief
*  Stop the dest service.
*       
*
* @return FBE_STATUS_OK - Start successful.
* 
*
* @version
*   03/06/2012:  Created. kothal
*
****************************************************************/

static fbe_status_t fbe_dest_service_stop()
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_dest_service_trace(FBE_TRACE_LEVEL_INFO, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s: entry\n", __FUNCTION__);
    /* Change of state to stop is only valid when the error insertion
    * is in progress. Check if the state is "START"
    */
    if(fbe_dest_state.current_state != FBE_DEST_START)
    {		
		fbe_dest_service_trace(FBE_TRACE_LEVEL_WARNING, 
							   FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							   "%s: DEST is not started yet.\n", __FUNCTION__);               		
    }
    else
    {
        /* Stop Error Insertion.
        */
        fbe_dest_state.current_state = FBE_DEST_STOP;       
        status = fbe_dest_stop();
	}
    return status;
}
/******************************************
* end fbe_dest_service_stop()
******************************************/

/*!**************************************************************
* fbe_dest_service_get_state()
****************************************************************
* @brief
*  Stop the dest service.
*       
*
* @return FBE_STATUS_OK - Start successful.
* 
*
* @version
*   03/06/2012:  Created. kothal
*
****************************************************************/

static fbe_status_t fbe_dest_service_get_state(fbe_packet_t * packet_p)
{     
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_dest_state_type *dest_state = NULL;    
    fbe_u32_t len = 0;
    
    fbe_dest_service_trace(FBE_TRACE_LEVEL_INFO, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s: entry\n", __FUNCTION__);
    
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);   
    
    fbe_payload_control_get_buffer(control_operation_p, &dest_state);
    if (dest_state == NULL)
    {
        fbe_dest_service_trace(FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s fbe_payload_control_get_buffer failed", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_dest_state_type))
    {
        fbe_dest_service_trace(FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Invalid len %d != %d ", __FUNCTION__, len, (int)sizeof(fbe_dest_state_type));
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    *dest_state = fbe_dest_state.current_state;     
    return FBE_STATUS_OK;
}
/******************************************
* end fbe_dest_service_get_state()
******************************************/
/*!**************************************************************
* fbe_dest_service_control_entry()
****************************************************************
* @brief
*  control entry for the DEST service.
*
* @param packet - control packet to process.
*
* @return None.  
*
*
* History:
*   03/06/2012  Created. kothal
*
****************************************************************/
fbe_status_t fbe_dest_service_control_entry(fbe_packet_t * packet)
{  
   fbe_payload_control_operation_opcode_t control_code;
   fbe_status_t status;   
   
   fbe_dest_service_trace(FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s: entry\n", __FUNCTION__);
   
   control_code = fbe_transport_get_control_code(packet);
   if (control_code == FBE_BASE_SERVICE_CONTROL_CODE_INIT) {
       fbe_dest_service_init(packet);
       return FBE_STATUS_OK;
   }
   
   if (fbe_base_service_is_initialized((fbe_base_service_t*)&fbe_dest_service) == FALSE) {
       fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
       fbe_transport_complete_packet(packet);
       return FBE_STATUS_NOT_INITIALIZED;
   }
   
   switch(control_code) {
       case FBE_BASE_SERVICE_CONTROL_CODE_DESTROY:
           status = fbe_dest_service_destroy(packet);
           break;
       case FBE_DEST_SERVICE_CONTROL_CODE_ADD_RECORD:
           status = fbe_dest_service_add_record(packet);
           break;      
       case FBE_DEST_SERVICE_CONTROL_CODE_REMOVE_RECORD:
           status = fbe_dest_service_remove_record(packet);
           break;
       case FBE_DEST_SERVICE_CONTROL_CODE_REMOVE_ALL_RECORDS:
           status = fbe_dest_service_remove_all_records();
           break;
       case FBE_DEST_SERVICE_CONTROL_CODE_START:
           status = fbe_dest_service_start();
           break;
       case FBE_DEST_SERVICE_CONTROL_CODE_STOP:
           status = fbe_dest_service_stop();
           break;                   
       case FBE_DEST_SERVICE_CONTROL_CODE_GET_RECORD:
           status = fbe_dest_service_get_record(packet);
           break;
       case FBE_DEST_SERVICE_CONTROL_CODE_GET_RECORD_HANDLE:
           status = fbe_dest_service_get_record_handle(packet);
           break;
       case FBE_DEST_SERVICE_CONTROL_CODE_SEARCH_RECORD:
           status = fbe_dest_service_search_record(packet);
           break;
       case FBE_DEST_SERVICE_CONTROL_CODE_GET_RECORD_NEXT:
           status = fbe_dest_service_get_record_next(packet);
           break;
       case FBE_DEST_SERVICE_CONTROL_CODE_RECORD_UPDATE_TIMES_TO_INSERT:
           status = fbe_dest_service_record_update_times_to_insert(packet);
           break;
       case FBE_DEST_SERVICE_CONTROL_CODE_GET_STATE:
           status = fbe_dest_service_get_state(packet);
           break; 
       default:
           return fbe_base_service_control_entry((fbe_base_service_t *) &fbe_dest_service, packet);
           break;
   }
   fbe_transport_set_status(packet, status, 0);
   fbe_transport_complete_packet(packet);
   return status;
}
/**************************************
* end fbe_dest_service_control_entry()
**************************************/

/*!**************************************************************
* fbe_dest_service_add_record()
****************************************************************
* @brief
*  Take a new control packet and attempt to add a record.
*
* @param - packet_p - Packet.
*
* @return fbe_status_t status.
* 
*
* History:
*   03/06/2012  Created. kothal
*
****************************************************************/
static fbe_status_t fbe_dest_service_add_record (fbe_packet_t * packet_p)
{
   fbe_payload_ex_t  *payload_p = NULL;
   fbe_payload_control_operation_t * control_operation_p = NULL;
   fbe_dest_control_add_record_t * add_record = NULL;
   fbe_u32_t len = 0;
   fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
   
   fbe_dest_service_trace(FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s: entry\n", __FUNCTION__);
   
   payload_p = fbe_transport_get_payload_ex(packet_p);
   control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
   
   fbe_payload_control_get_buffer(control_operation_p, &add_record);
   if (add_record == NULL)
   {
       fbe_dest_service_trace(FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s fbe_payload_control_get_buffer failed", __FUNCTION__);
       fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
       return FBE_STATUS_GENERIC_FAILURE;
   }
   
   fbe_payload_control_get_buffer_length(control_operation_p, &len);
   if (len != sizeof(fbe_dest_control_add_record_t))
   {
       fbe_dest_service_trace(FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Invalid len %d != %d ", __FUNCTION__, len, (int)sizeof(fbe_dest_control_add_record_t));
       fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
       return FBE_STATUS_GENERIC_FAILURE;
   }
     status = fbe_dest_add_record(&add_record->dest_error_record, &add_record->dest_record_handle);
   /* The caller is responsible for completing the packet. */
   return status;
}

/**************************************
* end fbe_dest_service_add_record()
**************************************/

/*!**************************************************************
* fbe_dest_service_remove_record()
****************************************************************
* @brief
*  Take a new control packet and attempt to remove a record.
*
* @param - packet_p - Packet.
*
* @return fbe_status_t status.
* 
*
* History:
*   03/06/2012  Created. kothal
*
****************************************************************/
static fbe_status_t fbe_dest_service_remove_record (fbe_packet_t * packet_p)
{
   fbe_payload_ex_t  *payload_p = NULL;
   fbe_payload_control_operation_t * control_operation_p = NULL;
   fbe_dest_control_remove_record_t * remove_record = NULL;
   fbe_u32_t len = 0;
   fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
   
   fbe_dest_service_trace(FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s: entry\n", __FUNCTION__);
   
   payload_p = fbe_transport_get_payload_ex(packet_p);
   control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
   
   fbe_payload_control_get_buffer(control_operation_p, &remove_record);
   if (remove_record == NULL)
   {
       fbe_dest_service_trace(FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s fbe_payload_control_get_buffer failed", __FUNCTION__);
       fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
       return FBE_STATUS_GENERIC_FAILURE;
   }
   
   fbe_payload_control_get_buffer_length(control_operation_p, &len);
   if (len != sizeof(fbe_dest_control_remove_record_t))
   {
       fbe_dest_service_trace(FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Invalid len %d != %d ", __FUNCTION__, len, (int)sizeof(fbe_dest_control_add_record_t));
       fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
       return FBE_STATUS_GENERIC_FAILURE;
   }
    status = fbe_dest_remove_record(remove_record->dest_record_handle);
   /* The caller is responsible for completing the packet. */
   return status;
}

/**************************************
* end fbe_dest_service_remove_record()
**************************************/
/*!**************************************************************
* fbe_dest_service_remove_all_records()
****************************************************************
* @brief
*  Take a new control packet and attempt to remove a record.
*
* @param - packet_p - Packet.
*
* @return fbe_status_t status.
* 
*
* History:
*   05/25/2012  Created. kothal
*
****************************************************************/
static fbe_status_t fbe_dest_service_remove_all_records ()
{   
   fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
   fbe_dest_service_trace(FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s: entry\n", __FUNCTION__);
   
   status =  fbe_dest_cleanup_queue();   
   return status;
}

/**************************************
* end fbe_dest_service_remove_all_records()
**************************************/

/*!**************************************************************
* fbe_dest_service_get_record()
****************************************************************
* @brief
*  Take a new control packet and attempt to get a record.
*
* @param - packet_p - Packet.
*
* @return fbe_status_t status.
* 
*
* History:
*   03/06/2012  Created. kothal
*
****************************************************************/
static fbe_status_t fbe_dest_service_get_record (fbe_packet_t * packet_p)
{
   fbe_payload_ex_t  *payload_p = NULL;
   fbe_payload_control_operation_t * control_operation_p = NULL;
   fbe_dest_control_add_record_t * get_record = NULL;
   fbe_u32_t len = 0;
   fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
   
   fbe_dest_service_trace(FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s: entry\n", __FUNCTION__);
   
   payload_p = fbe_transport_get_payload_ex(packet_p);
   control_operation_p = fbe_payload_ex_get_control_operation(payload_p);   
   
   fbe_payload_control_get_buffer(control_operation_p, &get_record);
   if (get_record == NULL)
   {
       fbe_dest_service_trace(FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s fbe_payload_control_get_buffer failed", __FUNCTION__);
       fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
       return FBE_STATUS_GENERIC_FAILURE;
   }
   
   fbe_payload_control_get_buffer_length(control_operation_p, &len);
   if (len != sizeof(fbe_dest_control_add_record_t))
   {
       fbe_dest_service_trace(FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Invalid len %d != %d ", __FUNCTION__, len, (int)sizeof(fbe_dest_control_add_record_t));
       fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
       return FBE_STATUS_GENERIC_FAILURE;
   }
    status = fbe_dest_get_record(get_record->dest_record_handle, &get_record->dest_error_record);
   /* The caller is responsible for completing the packet. */
   return status;
}

/**************************************
* end fbe_dest_service_get_record()
**************************************/

/*!**************************************************************
 * fbe_dest_service_get_record_handle()
 ****************************************************************
 * @brief
 *  Take a new control packet and attempt to get the record handle.
 *
 * @param - packet_p - Packet.
 *
 * @return fbe_status_t status.
 * 
 *
 ****************************************************************/
static fbe_status_t fbe_dest_service_get_record_handle(fbe_packet_t * packet_p)
{
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_dest_control_add_record_t * get_record = NULL;
    fbe_u32_t len = 0;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
	
    fbe_dest_service_trace(FBE_TRACE_LEVEL_INFO, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s: entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &get_record);
    if (get_record == NULL)
    {
        fbe_dest_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_dest_control_add_record_t))
    {
        fbe_dest_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %d ", __FUNCTION__, len, (int)sizeof(fbe_dest_control_add_record_t));
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_dest_get_record_handle( &get_record->dest_record_handle, &get_record->dest_error_record);
    /* The caller is responsible for completing the packet. */
    return status;

}
/**************************************
 * end fbe_dest_service_get_record_handle()
 **************************************/

/*!**************************************************************
 * fbe_dest_service_get_record_next()
 ****************************************************************
 * @brief
 *  Take a new control packet and attempt to add a record.
 *
 * @param - packet_p - Packet.
 *
 *
 *
 ****************************************************************/
static fbe_status_t fbe_dest_service_get_record_next (fbe_packet_t * packet_p)
{
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_dest_control_add_record_t * get_record = NULL;
    fbe_u32_t len = 0;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &get_record);
    if (get_record == NULL)
    {
        fbe_dest_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_dest_control_add_record_t))
    {
        fbe_dest_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %d ", __FUNCTION__, len, (int)sizeof(fbe_dest_control_add_record_t));
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status =  fbe_dest_get_record_next( &get_record->dest_record_handle, &get_record->dest_error_record);
    /* The caller is responsible for completing the packet. */
    return status;

}
/**************************************
 * end fbe_dest_service_get_record_next()
 **************************************/

/*!**************************************************************
 * fbe_dest_service_record_update_times_to_insert()
 ****************************************************************
 * @brief
 *  Take a new control packet and attempt to add a record.
 *
 * @param - packet_p - Packet.
 *
 *
 *
 ****************************************************************/
static fbe_status_t fbe_dest_service_record_update_times_to_insert (fbe_packet_t * packet_p)
{
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_dest_control_add_record_t * get_record = NULL;
    fbe_u32_t len = 0;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &get_record);
    if (get_record == NULL)
    {
        fbe_dest_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_dest_control_add_record_t))
    {
        fbe_dest_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %d ", __FUNCTION__, len, (int)sizeof(fbe_dest_control_add_record_t));
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status =  fbe_dest_update_times_to_insert( &get_record->dest_record_handle, &get_record->dest_error_record);
    /* The caller is responsible for completing the packet. */
    return status;

}
/**************************************
 * end fbe_dest_service_record_update_times_to_insert()
 **************************************/

 
/*!**************************************************************
 * fbe_dest_service_search_record()
 ****************************************************************
 * @brief
 *  Take a new control packet and attempt to get the record handle.
 *
 * @param - packet_p - Packet.
 *
 * @return fbe_status_t status.
 * 
 *
 ****************************************************************/
static fbe_status_t fbe_dest_service_search_record(fbe_packet_t * packet_p)
{
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_dest_control_add_record_t * get_record = NULL;
    fbe_u32_t len = 0;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
	
    fbe_dest_service_trace(FBE_TRACE_LEVEL_INFO, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s: entry\n", __FUNCTION__);
 
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
 
    fbe_payload_control_get_buffer(control_operation_p, &get_record);
    if (get_record == NULL)
    {
        fbe_dest_service_trace(FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s fbe_payload_control_get_buffer failed", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_dest_control_add_record_t))
    {
        fbe_dest_service_trace(FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Invalid len %d != %d ", __FUNCTION__, len, (int)sizeof(fbe_dest_control_add_record_t));
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_dest_search_record( &get_record->dest_record_handle, &get_record->dest_error_record);
    /* The caller is responsible for completing the packet. */
    return status;
 
}
/**************************************
 * end fbe_dest_service_search_record()
 **************************************/
