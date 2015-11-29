/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_lun_event_log.c
 ***************************************************************************
 *
 * @brief
 *   This file contains the code to log event log messages for the lun object.
 * 
 * @ingroup lun_class_files
 * 
 * @version
 *   11/18/2010:  Created. Amit Dhaduk
 *
 ***************************************************************************/



/*****************************************
 * LOCAL FUNCTION PROTOTYPES
 *****************************************/
#include "fbe_lun_private.h"
#include "fbe/fbe_event_log_api.h"                      /*  for fbe_event_log_write */
#include "fbe/fbe_event_log_utils.h"                    /*  for message codes */
#include "fbe_database.h"
#include "fbe/fbe_service_manager_interface.h"
#include "fbe_transport_memory.h"


static fbe_status_t fbe_lun_event_log_send_control_packet_and_get_response(
                                        fbe_packet_t*                           packet_p, 
                                        fbe_service_id_t                        destination_service,
                                        fbe_object_id_t                         destination_object_id,
                                        fbe_payload_control_operation_opcode_t  control_code,
                                        fbe_payload_control_buffer_t            io_buffer_p,
                                        fbe_u32_t                               buffer_size);
fbe_status_t fbe_lun_event_log_lun_bv(fbe_u32_t  lun_number,fbe_object_id_t lun_object_id,
                                         fbe_event_event_log_request_t*  event_log_data_p,
                                         fbe_u32_t message_code);
fbe_status_t fbe_lun_event_log_lun_bv_started(fbe_u32_t  lun_number,fbe_object_id_t in_lun_object_id,
                                         fbe_event_event_log_request_t*  event_log_data_p);
fbe_status_t fbe_lun_event_log_lun_bv_completed(fbe_u32_t  lun_number,fbe_object_id_t in_lun_object_id,
                                         fbe_event_event_log_request_t*  event_log_data_p);


/***************
 * FUNCTIONS
 ***************/
 
/*!****************************************************************************
 * fbe_lun_event_log_lun_zero_start_or_complete 
 ******************************************************************************
 * @brief
 *   This function will log a message that a LUN zeroing has started or 
 *   completed. 
 * 
 * @param lun_p                 - pointer to the LUN
 * @param is_lun_start_b        - TRUE indicates LUN zeroing start; FALSE indicates
 *                                LUN zeroing finished
 *  
 *
 * @return  fbe_status_t           
 *
 * @author
 *   11/18/2010 - Created. Amit Dhaduk 
 *   09/15/2011 - Modified. Vishnu Sharma 
 *
 ******************************************************************************/
fbe_status_t fbe_lun_event_log_lun_zero_start_or_complete(fbe_lun_t *     lun_p,
                                                          fbe_bool_t      is_lun_zero_start
                                                          )
{


    fbe_u32_t                           lun_number;                 /* Navi LUN number/ALU */
    fbe_object_id_t                     object_id;                  /* object id of the LUN object */
    fbe_u32_t                           message_code;               /* message code to log */

    /*  Get the LUN object id 
     */
    fbe_base_object_get_object_id((fbe_base_object_t*) lun_p, &object_id);


    fbe_database_get_lun_number(object_id, &lun_number);

    /*  Determine whether the message to log is a LUN zeroing started or LUN zeroing 
     * completed
     */
    if (is_lun_zero_start == FBE_TRUE)
    {
        message_code = SEP_INFO_LUN_OBJECT_LUN_ZEROING_STARTED;
    }
    else
    {
        message_code = SEP_INFO_LUN_OBJECT_LUN_ZEROING_COMPLETED;
    }    

    /*  Write the message to the event log  */
    fbe_event_log_write(message_code, NULL, 0, "%d %x",lun_number, object_id);

    /*  Return success  */
    return FBE_STATUS_OK;

} 
/******************************************************************************
 * end fbe_lun_event_log_lun_zero_start_or_complete()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_event_log_event 
 ******************************************************************************
 * @brief
 *   This function will log a message that a LUN copy/rebuild,verify has started or 
 *   completed. 
 * 
 * @param lun_p                            - pointer to the LUN
 * @param event_log_data_p                 - pointer to event log structure.
 * 
 *  
 *
 * @return  fbe_status_t           
 *
 * @author
 *   09/30/2011 - Modified. Vishnu Sharma 
 *
 ******************************************************************************/

 fbe_status_t fbe_lun_event_log_event(fbe_lun_t *     lun_p,
                                      fbe_event_event_log_request_t*  event_log_data_p)
{
    fbe_u32_t                           lun_number;                 /* Navi LUN number/ALU */
    fbe_object_id_t                     object_id;                  /* object id of the LUN object */
    fbe_status_t                        status = FBE_STATUS_OK;

    /*  Get the LUN object id 
     */
    fbe_base_object_get_object_id((fbe_base_object_t*) lun_p, &object_id);


    fbe_database_get_lun_number(object_id, &lun_number);


    
    switch(event_log_data_p->event_log_type)
    {
        case FBE_EVENT_LOG_EVENT_TYPE_LUN_BV_STARTED:
            status = fbe_lun_event_log_lun_bv_started(lun_number,object_id,event_log_data_p);
            break;
        case FBE_EVENT_LOG_EVENT_TYPE_LUN_BV_COMPLETED:
            status = fbe_lun_event_log_lun_bv_completed(lun_number,object_id,event_log_data_p);
            break;    
    }

    /*  Return success  */
    return FBE_STATUS_OK;

}

/******************************************************************************
 * end fbe_lun_event_log_event()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_event_log_lun_bv_started 
 ******************************************************************************
 * @brief
 *   This function will log a message that a LUN BV has started.  
 *    
 *   @param lun_number                       - lun number of the lun.
 *   @param lun_object_id                    - object id of the lun.
 *   @param event_log_data_p                 - pointer to event log structure.
 *
 * 
 *  
 *
 * @return  fbe_status_t           
 *
 * @author
 *   09/30/2011 - Modified. Vishnu Sharma 
 *
 ******************************************************************************/

 fbe_status_t fbe_lun_event_log_lun_bv_started(fbe_u32_t  lun_number,fbe_object_id_t in_lun_object_id,
                                         fbe_event_event_log_request_t*  event_log_data_p)
{


    
    fbe_u32_t message_code;
    
    if(event_log_data_p->verify_flags == FBE_RAID_BITMAP_VERIFY_USER_READ_WRITE)
    {
        message_code = SEP_INFO_RAID_OBJECT_USER_RW_BCKGRND_VERIFY_STARTED;
    }
    else if(event_log_data_p->verify_flags == FBE_RAID_BITMAP_VERIFY_USER_READ_ONLY)
    {
        message_code = SEP_INFO_RAID_OBJECT_USER_RO_BCKGRND_VERIFY_STARTED;
    }
    else if(event_log_data_p->verify_flags == FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE)
    {
        message_code = SEP_INFO_RAID_OBJECT_LU_AUTOMATIC_VERIFY_STARTED;
    }
    else if(event_log_data_p->verify_flags == FBE_RAID_BITMAP_VERIFY_SYSTEM)
    {
        message_code = SEP_INFO_RAID_OBJECT_LU_AUTOMATIC_VERIFY_STARTED;
    }
    else if(event_log_data_p->verify_flags == FBE_RAID_BITMAP_VERIFY_ERROR)
    {
        /* We will not log system initiated error verifies */
        return FBE_STATUS_OK;
    }
    else
    {
        /* Not yet supported case */
        return FBE_STATUS_OK;
    }

    //  Write the message to the event log 
     fbe_event_log_write(message_code, NULL, 0, "%d %x", lun_number, in_lun_object_id);
    /*  Return success  */
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_lun_event_log_lun_bv_completed 
 ******************************************************************************
 * @brief
 *   This function will log a message that a LUN BV has  
 *   completed. 
 * 
 *   @param lun_number                       - lun number of the lun.
 *   @param lun_object_id                    - object id of the lun.
 *   @param event_log_data_p                 - pointer to event log structure.
 *   @param message_code                     - code of the message.  
 * 
 *  
 *
 * @return  fbe_status_t           
 *
 * @author
 *   09/30/2011 - Modified. Vishnu Sharma 
 *
 ******************************************************************************/

 fbe_status_t fbe_lun_event_log_lun_bv_completed(fbe_u32_t  lun_number,fbe_object_id_t in_lun_object_id,
                                         fbe_event_event_log_request_t*  event_log_data_p)
{

    
    if(event_log_data_p->verify_flags == FBE_RAID_BITMAP_VERIFY_USER_READ_WRITE)
    {
        fbe_event_log_write(SEP_INFO_USER_BCKGRND_VERIFY_COMPLETED, NULL, 0, "%d %x",
                            lun_number, in_lun_object_id);
    }
    else if(event_log_data_p->verify_flags == FBE_RAID_BITMAP_VERIFY_USER_READ_ONLY)
    {
        fbe_event_log_write(SEP_INFO_USER_RO_BCKGRND_VERIFY_COMPLETED, NULL, 0, "%d %x",
                            lun_number, in_lun_object_id);
    }
    else if(event_log_data_p->verify_flags == FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE)
    {
        fbe_event_log_write(SEP_INFO_RAID_OBJECT_LU_AUTOMATIC_VERIFY_COMPLETE, NULL, 0, "%d %x",
                            lun_number, in_lun_object_id);
    }
    else if(event_log_data_p->verify_flags == FBE_RAID_BITMAP_VERIFY_SYSTEM)
    {
        fbe_event_log_write(SEP_INFO_RAID_OBJECT_LU_AUTOMATIC_VERIFY_COMPLETE, NULL, 0, "%d %x",
                            lun_number, in_lun_object_id);
    }    
    /*  Return success  */
    return FBE_STATUS_OK;

}


/******************************************
 * end fbe_lun_event_log.c
 *****************************************/


