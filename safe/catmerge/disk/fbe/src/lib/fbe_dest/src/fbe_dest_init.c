/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_dest_init.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main entry points for the Drive Error Simulator
 *  Tool (DEST). This includes the initialization, add record, start
 *  injection, and stop injection.
 * 
 *  The design uses a hook callback function which is inserted on an SSP edge
 *  for a specific drive.  When IO is sent down the edge the hook function
 *  will be called.  This function will look through a queue of error records
 *  to see if this IO qualifies for error injection.  If so it will add a new
 *  completion function to the packets stack.  (This completion function will do
 *  the actual error injection)  The IO is then sent to the drive and on the way
 *  up the error will be injected.
 * 
 * @version
 *   01-June-2011:  Created. Wayne Garrett
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "swap_exports.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_queue.h"
#include "fbe_transport_memory.h"
#include "fbe_topology.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_object_map_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_dest.h"
#include "fbe/fbe_time.h"
#include "fbe/fbe_random.h"
#include "fbe_dest_private.h"
#include "fbe_ssp_transport.h"
#include "fbe_stp_transport.h"

typedef fbe_u32_t fbe_dest_magic_number_t;
enum {
    FBE_DEST_MAGIC_NUMBER = 0x44455354,  /* ASCII for DEST */
};

typedef struct fbe_dest_error_element_s {
    fbe_queue_element_t queue_element;
    fbe_dest_magic_number_t magic_number;
    fbe_dest_error_record_t dest_error_record; 	
}fbe_dest_error_element_t;


static fbe_queue_head_t dest_error_queue;
static fbe_spinlock_t   dest_error_queue_lock;
static fbe_bool_t       dest_started = FBE_FALSE;
static fbe_trace_level_t dest_trace_level = FBE_TRACE_LEVEL_INFO; /* Set INFO by default for debugging purposes */



/*************************
 *   FUNCTION DEFINITIONS
 *************************/

static fbe_status_t fbe_dest_edge_hook_function(fbe_packet_t * packet);
static fbe_status_t fbe_dest_edge_hook_function_completion(struct fbe_packet_s * packet, fbe_packet_completion_context_t context);
static void fbe_dest_drive_lifecycle_notification_callback(update_object_msg_t * update_object_msg, void * context);
static fbe_status_t fbe_dest_handle_fis_error_type (fbe_payload_fis_operation_t * fis_operation, fbe_dest_error_record_t * dest_error_record);
static fbe_status_t dest_handle_port_error(fbe_packet_t * packet, fbe_dest_error_record_t * error_record);
static fbe_status_t fbe_dest_get_start_lba_and_blocks(fbe_payload_cdb_operation_t * cdb_operation,
		fbe_lba_t *lba_start,      /*out*/
		fbe_block_count_t *blocks); /*out*/
static fbe_dest_error_element_t * fbe_dest_find_error_element_by_object_id(fbe_object_id_t object_id);
static fbe_status_t fbe_dest_delay_io_completion(struct fbe_packet_s * packet, fbe_packet_completion_context_t context);
static fbe_bool_t   fbe_dest_record_qualify_for_injection(fbe_packet_t * packet, fbe_payload_cdb_operation_t * cdb_operation, fbe_dest_error_element_t * dest_error_element);
static fbe_status_t fbe_dest_check_cdb_error_record(fbe_packet_t * packet, fbe_payload_cdb_operation_t * cdb_operation, fbe_dest_error_element_t * dest_error_element);
static fbe_status_t fbe_dest_set_hook_function(fbe_dest_error_record_t * rec);
static void         fbe_dest_mark_drive_destroyed(fbe_object_id_t object_id);
static fbe_status_t fbe_dest_mark_drive_inserted(fbe_object_id_t object_id);
static void         fbe_dest_insert_error(fbe_packet_t * packet, fbe_payload_cdb_operation_t * cdb_operation, fbe_dest_error_record_t * error_record);
static fbe_status_t fbe_dest_insert_error_glitch( fbe_dest_error_record_t *error_record);  
static fbe_status_t fbe_dest_insert_error_fail(fbe_dest_error_record_t *error_record);  


/************************************************************
 *  dest_log_error()
 ************************************************************
 *
 * @brief   Log ktrace error
 *
 * @param   fmt - format
 * @return  void
 * @author
 *  06/01/2011  Wayne Garrett  - Created
 *
 ************************************************************/
void
dest_log_error(const char * fmt, ...)
{
    va_list args;
    if(FBE_TRACE_LEVEL_ERROR <= dest_trace_level) {
        va_start(args, fmt);
        fbe_trace_report(FBE_COMPONENT_TYPE_PACKAGE,
                        FBE_PACKAGE_ID_NEIT,
                        FBE_TRACE_LEVEL_ERROR,
                        0, /* message_id */
                        fmt, 
                        args);
        va_end(args);
    }
}
/*****************************************
 * dest_log_error()
 *****************************************/


/************************************************************
 *  dest_log_info()
 ************************************************************
 *
 * @brief   Log ktrace info
 *
 * @param   fmt - format
 * @return  void
 * @author
 *  06/01/2011  Wayne Garrett  - Created
 *
 ************************************************************/
void
dest_log_info(const char * fmt, ...)
{
    va_list args;

    if(FBE_TRACE_LEVEL_INFO <= dest_trace_level)  {
        va_start(args, fmt);
        fbe_trace_report(FBE_COMPONENT_TYPE_PACKAGE,
                        FBE_PACKAGE_ID_NEIT,
                        FBE_TRACE_LEVEL_INFO,
                        0, /* message_id */
                        fmt, 
                        args);
        va_end(args);
    }
}
/*****************************************
 * dest_log_info()
 *****************************************/

/************************************************************
 *  dest_log_debug()
 ************************************************************
 *
 * @brief   Log ktrace debug
 *
 * @param   fmt - format
 * @return  void
 * @author
 *  06/01/2011  Wayne Garrett  - Created
 *
 ************************************************************/
void
dest_log_debug(const char * fmt, ...)
{
    va_list args;

    if(FBE_TRACE_LEVEL_DEBUG_HIGH <= dest_trace_level)  {
        va_start(args, fmt);
        fbe_trace_report(FBE_COMPONENT_TYPE_PACKAGE,
                        FBE_PACKAGE_ID_NEIT,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        0, /* message_id */
                        fmt, 
                        args);
        va_end(args);
    }
}
/*****************************************
 * dest_log_debug()
 *****************************************/

/************************************************************
 *  fbe_dest_init()
 ************************************************************
 *
 * @brief   DEST initialization
 *
 * @param   void
 * @return  status
 * @author
 *  06/01/2011  Wayne Garrett  - Created
 *
 ************************************************************/
fbe_status_t 
fbe_dest_init(void)
{
   // fbe_notification_type_t state_mask;

    dest_log_info("%s entry\n", __FUNCTION__);
    fbe_queue_init(&dest_error_queue);
    fbe_spinlock_init(&dest_error_queue_lock);
	
	/*Todo - Commented out temporary until we can determine why we need to register for state changes.
	*/
    //fbe_dest_initialize_physical_package_control_entry(&dest_physical_package_control_entry);
    //fbe_api_common_init();
    
   //state_mask = FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_DESTROY | FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_DESTROY;

   //fbe_api_object_map_interface_register_notification(state_mask, fbe_dest_destroy_notification_callback);
    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_dest_init()
 *****************************************/


/************************************************************
 *  fbe_dest_destroy()
 ************************************************************
 *
 * @brief   DEST destruction
 *
 * @param   void
 * @return  status
 * @author
 *  06/01/2011  Wayne Garrett  - Created
 *
 ************************************************************/
fbe_status_t 
fbe_dest_destroy(void)
{   
    dest_log_info("%s entry\n", __FUNCTION__);
    dest_started = FBE_FALSE;    
	
    fbe_dest_cleanup_queue();
    
  //  fbe_api_object_map_interface_unregister_notification(fbe_dest_destroy_notification_callback);
    
    fbe_queue_destroy(&dest_error_queue);
    fbe_spinlock_destroy(&dest_error_queue_lock);
    
    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_dest_destroy()
 *****************************************/

/************************************************************
 *  fbe_dest_add_record()
 ************************************************************
 *
 * @brief   Adds an error record for a given drive and sets
 *          the callback hook function, which handles the
 *          error injection.
 * 
 * @param   dest_error_record - ptr to error record to be added
 *          dest_record_handle - returns handle to error record
 *              queue element.
 * @return  status
 * @author
 *  06/01/2011  Wayne Garrett  - Created
 *
 ************************************************************/
fbe_status_t 
fbe_dest_add_record(fbe_dest_error_record_t * dest_error_record, fbe_dest_record_handle_t * dest_record_handle)
{
    fbe_status_t status;
    fbe_dest_error_element_t * dest_error_element = NULL;       
    fbe_physical_drive_information_t drive_information;
    fbe_dest_error_record_t *rec_p = NULL;
    *dest_record_handle = NULL;

    dest_log_info("%s entry\n", __FUNCTION__);

    dest_error_element = fbe_memory_ex_allocate(sizeof(fbe_dest_error_element_t));

    if(dest_error_element == NULL){
        dest_log_error("%s Can't allocate memory\n", __FUNCTION__);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    dest_error_element->magic_number = FBE_DEST_MAGIC_NUMBER;
    fbe_copy_memory(&dest_error_element->dest_error_record, dest_error_record, sizeof(fbe_dest_error_record_t));
        

    /* calculate times to reactivate if user selected random */
    if (dest_error_element->dest_error_record.is_random_react_interations)
    {
        /* When user selects random, num_of_times is the max range for calcuating random value.  Reset num_of_times to
           be the random value */
        dest_error_element->dest_error_record.num_of_times_to_reactivate =
            fbe_random() % (dest_error_element->dest_error_record.num_of_times_to_reactivate+1);       
        dest_log_info("DEST: %s random times to reactivate = %d \n", __FUNCTION__, dest_error_element->dest_error_record.num_of_times_to_reactivate);
    }

	/* Determine if we need to use descriptor format for the sense code
	*   information - Get the drive information
	*/
    status = fbe_api_physical_drive_get_drive_information(dest_error_element->dest_error_record.object_id, &drive_information, FBE_PACKET_FLAG_NO_ATTRIB);		
    if (status != FBE_STATUS_OK) {
        dest_log_error("%s Get Drive Info Error %d\n", __FUNCTION__, status);
        fbe_memory_ex_release(dest_error_element);
        return FBE_STATUS_GENERIC_FAILURE;
    }
	
    /* Set the flag in the error_record.  It will be referecned later
	*   when the error in this record is actually injected.
	*/
    dest_error_element->dest_error_record.requires_descriptor_format_sense_data = (drive_information.drive_parameter_flags & FBE_PDO_FLAG_USES_DESCRIPTOR_FORMAT)?FBE_TRUE:FBE_FALSE;           

    status = fbe_dest_set_hook_function(&dest_error_element->dest_error_record);

    if (status != FBE_STATUS_OK) {
        dest_log_error("%s Set Edge Hook Error %d\n", __FUNCTION__, status);
        fbe_memory_ex_release(dest_error_element);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_spinlock_lock(&dest_error_queue_lock);
    fbe_queue_push(&dest_error_queue, &dest_error_element->queue_element);
    fbe_spinlock_unlock(&dest_error_queue_lock);

    * dest_record_handle = dest_error_element;	
    rec_p = &dest_error_element->dest_error_record;

    dest_log_info("DEST: %s dest_record_handle 0x%llX \n",__FUNCTION__, (unsigned long long)*dest_record_handle); 
    dest_log_info("DEST: %d_%d_%d pdo:0x%x op_str:\"%s\" op0:0x%x lba:[0x%llx..0x%llx] num_insert:%d num_reactivate:%d frequency:%d\n", 
                  rec_p->bus, rec_p->enclosure, rec_p->slot, rec_p->object_id, rec_p->opcode, rec_p->dest_error.dest_scsi_error.scsi_command[0], rec_p->lba_start, rec_p->lba_end, 
                  rec_p->num_of_times_to_insert, rec_p->num_of_times_to_reactivate, rec_p->frequency);
    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_dest_add_record()
 *****************************************/
/************************************************************
 *  fbe_dest_set_hook_function()
 ************************************************************
 *
 * @brief   Sets a callback hook function on the appropriate
 *          edge.
 * 
 * @param   rec - ptr to error record 
 * @return  status
 * @author
 *  06/01/2011  Wayne Garrett  - Created
 *
 ************************************************************/
static fbe_status_t 
fbe_dest_set_hook_function(fbe_dest_error_record_t * rec)
{
    fbe_api_set_edge_hook_t hook_info;
    fbe_class_id_t class_id;
    fbe_status_t status = FBE_STATUS_OK;

	hook_info.hook = fbe_dest_edge_hook_function;

    /* Let's figure out if it is SAS or SATA drive or SAS enclosure */
    fbe_api_get_object_class_id (rec->object_id, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    if((class_id > FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_FIRST) && (class_id < FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_LAST)/* SAS drive */
       ||(class_id > FBE_CLASS_ID_SAS_ENCLOSURE_FIRST) && (class_id < FBE_CLASS_ID_SAS_ENCLOSURE_LAST)) /*SAS Enclosures */
    { 
        status = fbe_api_set_ssp_edge_hook ( rec->object_id, &hook_info);
    }else if((class_id > FBE_CLASS_ID_SATA_PHYSICAL_DRIVE_FIRST) && (class_id < FBE_CLASS_ID_SATA_PHYSICAL_DRIVE_LAST)){ /* SATA drive */
        status = fbe_api_set_stp_edge_hook ( rec->object_id, &hook_info);
    } else {
        dest_log_error("%s Invalid class_id %X\n", __FUNCTION__, class_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/*****************************************
 * fbe_dest_set_hook_function()
 *****************************************/

/************************************************************
 *  fbe_dest_mark_drive_destroyed()
 ************************************************************
 *
 * @brief   Mark drive as going away.  The record will be
 *          maintained in case drive comes back online,
 *          in which case the hook will be re-established.
 * 
 * @param   object_id - Object ID to drive that was destroyed
 * @return  status
 * @author
 *  06/01/2011  Wayne Garrett  - Created
 *
 ************************************************************/

static void
fbe_dest_mark_drive_destroyed(fbe_object_id_t object_id)
{
    fbe_dest_error_element_t * dest_error_element = NULL;    

    /* mark all records for this drive as being destroyed by setting it's object ID to invalid.*/
    fbe_spinlock_lock(&dest_error_queue_lock);
    dest_error_element = (fbe_dest_error_element_t *)fbe_queue_front(&dest_error_queue);
    while(dest_error_element != NULL){
        if(dest_error_element->dest_error_record.object_id == object_id){ /* We found our record */
            dest_log_info("%s Record disabled %d_%d_%d oid=0x%x rec=0x%x\n", __FUNCTION__, 
                          dest_error_element->dest_error_record.bus, dest_error_element->dest_error_record.enclosure, dest_error_element->dest_error_record.slot,
                          object_id, (unsigned int)&dest_error_element->dest_error_record);
            dest_error_element->dest_error_record.object_id = FBE_OBJECT_ID_INVALID;
            dest_error_element = (fbe_dest_error_element_t *)fbe_queue_next(&dest_error_queue, (fbe_queue_element_t *)dest_error_element);
        } else {
            dest_error_element = (fbe_dest_error_element_t *)fbe_queue_next(&dest_error_queue, (fbe_queue_element_t *)dest_error_element);
        }
    }
    fbe_spinlock_unlock(&dest_error_queue_lock);

    /* note, no need to clean up edge hook since object is being destroyed.*/
}
/*****************************************
 * fbe_dest_mark_drive_destroyed()
 *****************************************/


/************************************************************
 *  fbe_dest_mark_drive_inserted()
 ************************************************************
 *
 * @brief   Mark drive as being inserted after it went away.
 *          This will re-enable all the records for the
 *          given drive.
 * 
 * @param   object_id - Object ID to drive that may have been
 *                      inserted.
 * @return  status
 * @author
 *  06/01/2011  Wayne Garrett  - Created
 *
 ************************************************************/
/*  */
static fbe_status_t
fbe_dest_mark_drive_inserted(fbe_object_id_t object_id)
{
    /* 1. get b-e-s for object id
       2. loop through error records for matching drive and set new object id.
       3. call add_record to have hook inserted on protocol edge
    */

    fbe_port_number_t port;
    fbe_enclosure_number_t encl;
    fbe_enclosure_slot_number_t slot;
    fbe_dest_error_element_t * dest_error_element = NULL;
    fbe_status_t status = FBE_STATUS_OK;


    status = fbe_api_get_object_port_number (object_id, &port);
    if (status != FBE_STATUS_OK) {
        dest_log_error("%s port not found\n", __FUNCTION__);
        return status;
    }
    status = fbe_api_get_object_enclosure_number(object_id, &encl);

    if (status != FBE_STATUS_OK) {
        dest_log_error("%s port enclosure found\n", __FUNCTION__);
        return status;
    }
    status = fbe_api_get_object_drive_number (object_id, &slot);
    if (status != FBE_STATUS_OK) {
        dest_log_error("%s slot not found\n", __FUNCTION__);
        return status;
    }


    fbe_spinlock_lock(&dest_error_queue_lock);
    dest_error_element = (fbe_dest_error_element_t *)fbe_queue_front(&dest_error_queue);
    while(dest_error_element != NULL){
        if(dest_error_element->dest_error_record.bus       == (fbe_u32_t)port &&
           dest_error_element->dest_error_record.enclosure == (fbe_u32_t)encl &&
           dest_error_element->dest_error_record.slot      == (fbe_u32_t)slot )
        {
            /* only update rec if object was removed. If object_id is valid then we never went through destroy
               so this must be a glitch case, not drive intialization*/
            if (dest_error_element->dest_error_record.object_id == FBE_OBJECT_ID_INVALID) 
            {            
                dest_log_info("%s Reinserting error record for %d_%d_%d oid=0x%x rec=0x%x\n", __FUNCTION__, port, encl, slot, object_id, (unsigned int)&dest_error_element->dest_error_record);
    
                dest_error_element->dest_error_record.object_id = object_id; 
    
                /* Reset hook on new object */
                status = fbe_dest_set_hook_function(&dest_error_element->dest_error_record);
            
                if (status != FBE_STATUS_OK) {
                    dest_log_error("%s Set Edge Hook Error %d\n", __FUNCTION__, status);
                    status = FBE_STATUS_GENERIC_FAILURE;
                    break;
                }
            }

            dest_error_element = (fbe_dest_error_element_t *)fbe_queue_next(&dest_error_queue, (fbe_queue_element_t *)dest_error_element);
        } 
        else 
        {
            dest_error_element = (fbe_dest_error_element_t *)fbe_queue_next(&dest_error_queue, (fbe_queue_element_t *)dest_error_element);
        }
    }

    fbe_spinlock_unlock(&dest_error_queue_lock);    
    return status;
}
/*****************************************
 * fbe_dest_mark_drive_inserted()
 *****************************************/
/************************************************************
 *  fbe_dest_remove_object()
 ************************************************************
 *
 * @brief    Removes all records for particular object
 * 
 * @param   object_id - Object ID to drive being removed
 * @return  status
 * @author
 *  06/01/2011  Wayne Garrett  - Created
 *
 ************************************************************/
/* Removes all records for particular object */
fbe_status_t 
fbe_dest_remove_object(fbe_object_id_t object_id)
{
    fbe_dest_error_element_t * dest_error_element = NULL;
    fbe_api_set_edge_hook_t hook_info;
    fbe_status_t status;
    fbe_class_id_t class_id;

    fbe_spinlock_lock(&dest_error_queue_lock);
    dest_error_element = (fbe_dest_error_element_t *)fbe_queue_front(&dest_error_queue);
    while(dest_error_element != NULL){
        if(dest_error_element->dest_error_record.object_id == object_id){ /* We found our record */
            /* todo: only remove if no outstanding error injection, such as one that has been delayed or injected on completion path
               otherwise return failure to caller indicating why*/
                
            if(dest_error_element->magic_number != FBE_DEST_MAGIC_NUMBER) {
                dest_log_error("%s Invalid handle\n", __FUNCTION__);
                return FBE_STATUS_GENERIC_FAILURE;
            }	
           
            fbe_queue_remove(&dest_error_element->queue_element);	    
            fbe_memory_ex_release(dest_error_element);    
    
            dest_error_element = (fbe_dest_error_element_t *)fbe_queue_front(&dest_error_queue); /* Start from the beginning */
        } else {
            dest_error_element = (fbe_dest_error_element_t *)fbe_queue_next(&dest_error_queue, (fbe_queue_element_t *)dest_error_element);
        }
    }/* while(dest_error_element != NULL) */
    fbe_spinlock_unlock(&dest_error_queue_lock);

    hook_info.hook = NULL;
    /* Let's figure out if it is SAS or SATA drive */
    fbe_api_get_object_class_id (object_id, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    if((class_id > FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_FIRST) && (class_id < FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_LAST)/* SAS drive */
       ||(class_id > FBE_CLASS_ID_SAS_ENCLOSURE_FIRST) && (class_id < FBE_CLASS_ID_SAS_ENCLOSURE_LAST)) /*SAS Enclosures */
    { 
        status = fbe_api_set_ssp_edge_hook ( object_id, &hook_info);
    }else if((class_id > FBE_CLASS_ID_SATA_PHYSICAL_DRIVE_FIRST) && (class_id < FBE_CLASS_ID_SATA_PHYSICAL_DRIVE_LAST)){ /* SATA drive */
        status = fbe_api_set_stp_edge_hook ( object_id, &hook_info);
    }

    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_dest_remove_object()
 *****************************************/

/************************************************************
 *  fbe_dest_remove_record()
 ************************************************************
 *
 * @brief    Removes an error record
 * 
 * @param   dest_record_handle - ptr to error record queue
 *              element.
 * @return  status
 * @author
 *  06/01/2011  Wayne Garrett  - Created
 *
 ************************************************************/
fbe_status_t 
fbe_dest_remove_record(fbe_dest_record_handle_t dest_record_handle)
{
    fbe_status_t status;
    fbe_dest_error_element_t * dest_error_element = NULL;
    fbe_api_set_edge_hook_t hook_info;

    dest_error_element = (fbe_dest_error_element_t *)dest_record_handle;

    if(dest_error_element->magic_number != FBE_DEST_MAGIC_NUMBER) {
        dest_log_error("%s Invalid handle\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock(&dest_error_queue_lock);
    fbe_queue_remove(&dest_error_element->queue_element);
    fbe_spinlock_unlock(&dest_error_queue_lock);

    /* If we have not records for this object id we should disable the hook */
    if(NULL == fbe_dest_find_error_element_by_object_id(dest_error_element->dest_error_record.object_id)){
        hook_info.hook = NULL;
        status = fbe_api_set_ssp_edge_hook ( dest_error_element->dest_error_record.object_id, &hook_info);
    }

    fbe_memory_ex_release(dest_error_element);
    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_dest_remove_record()
 *****************************************/

/************************************************************
 *  fbe_dest_get_record()
 ************************************************************
 *
 * @brief    Return a copy of an error record
 * 
 * @param   dest_record_handle - ptr to error record queue
 *                      element.
 *          dest_error_record - record to copy to
 * @return  status
 * @author
 *  06/01/2011  Wayne Garrett  - Created
 *
 ************************************************************/
fbe_status_t 
fbe_dest_get_record(fbe_dest_record_handle_t dest_record_handle, fbe_dest_error_record_t * dest_error_record)
{
    fbe_dest_error_element_t * dest_error_element = NULL;
	
    dest_log_info("%s entry\n", __FUNCTION__);
	 
    dest_error_element = (fbe_dest_error_element_t *)dest_record_handle;   
	
    if((dest_error_element == NULL)) {
        dest_log_error("%s Invalid handle\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(dest_error_record, &dest_error_element->dest_error_record, sizeof(fbe_dest_error_record_t));

    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_dest_get_record()
 *****************************************/

/************************************************************
 *  fbe_dest_get_record_next()
 ************************************************************
 *
 * @brief    Return a copy of an error record
 * 
 * @param   dest_record_handle - ptr to error record queue
 *                      element.
 *          dest_error_record - record to copy to
 * @return  status 
 *
  * History:
 *  05/01/2012  Created. kothal
 ************************************************************/
fbe_status_t 
fbe_dest_get_record_next(fbe_dest_record_handle_t * dest_record_handle, fbe_dest_error_record_t * dest_error_record)
{
    fbe_dest_error_element_t * dest_error_element = NULL;
	  
    if(dest_error_record == NULL){
		dest_log_error("%s Received record is NULL\n", __FUNCTION__);      
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
	
    fbe_spinlock_lock(&dest_error_queue_lock);
	
    if(*dest_record_handle == NULL)
    {
        dest_error_element = (fbe_dest_error_element_t *)fbe_queue_front(&dest_error_queue);
    }
    else
    {		
         dest_error_element = (fbe_dest_error_element_t *)fbe_queue_next(&dest_error_queue,(fbe_queue_element_t *)*dest_record_handle);
    }
	
    if(dest_error_element != NULL)
    {
        *dest_error_record = dest_error_element->dest_error_record;
    }
	
	*dest_record_handle = (fbe_dest_record_handle_t *)dest_error_element;
	
    fbe_spinlock_unlock(&dest_error_queue_lock);
	
    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_dest_get_record_next()
 *****************************************/

/************************************************************
 *  fbe_dest_get_record_handle()
 ************************************************************
 *
 * @brief    Return a copy of handle to error record 
 * 
 * @param   dest_record_handle - ptr to error record queue
 *                      element.
 *          dest_error_record - record to copy to
 * @return  status 
 *
 * History:
 *  05/01/2012  Created. kothal
 ************************************************************/
fbe_status_t 
fbe_dest_get_record_handle(fbe_dest_record_handle_t * dest_record_handle, fbe_dest_error_record_t * dest_error_record)
{  
 
    fbe_dest_error_element_t * dest_error_element = NULL;
	
    dest_log_info("%s entry\n", __FUNCTION__);

    fbe_spinlock_lock(&dest_error_queue_lock);
    
    dest_error_element = (fbe_dest_error_element_t *)fbe_queue_front(&dest_error_queue);

    while(dest_error_element != NULL)
    {        
        if(dest_error_element->dest_error_record.record_id == dest_error_record->record_id)
        {
            *dest_record_handle = (fbe_dest_record_handle_t *)dest_error_element;   
            dest_log_info("DEST: %s dest_record_handle 0x%llX \n",__FUNCTION__, (unsigned long long)*dest_record_handle); 
            break;  
        }
        else
        {
            *dest_record_handle = NULL;
        }
        
        dest_error_element = (fbe_dest_error_element_t *)fbe_queue_next(&dest_error_queue, (fbe_queue_element_t *)dest_error_element);
    }   	
    fbe_spinlock_unlock(&dest_error_queue_lock);  
    
    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_dest_get_record_handle()
 *****************************************/
 
/************************************************************
 *  fbe_dest_search_record()
 ************************************************************
 *
 * @brief    Return a copy of handle to error record that matches
 *            the search criteria. 
 * 
 * @param   dest_record_handle - ptr to error record queue
 *                      element.
 *          dest_error_record - ptr to error record 
 * @return  status 
 *
 * History:
 *  09/24/2012  Created. kothal
 ************************************************************/
fbe_status_t 
fbe_dest_search_record(fbe_dest_record_handle_t * dest_record_handle, fbe_dest_error_record_t * dest_error_record)
{  
    
    fbe_dest_error_element_t * dest_error_element = NULL;
	
    dest_log_info("%s entry\n", __FUNCTION__);
 
    fbe_spinlock_lock(&dest_error_queue_lock);    
    
    do
    {
        /* if record handle is null, start the search from the start of the queue - when fbe_dest_search_record() is called the first time*/
        if(*dest_record_handle == NULL)
        {
            dest_error_element = (fbe_dest_error_element_t *)fbe_queue_front(&dest_error_queue);
            
        }
        /* if record handle is valid, search from the next element of queue*/
        else
        {		
            dest_error_element = (fbe_dest_error_element_t *)fbe_queue_next(&dest_error_queue,(fbe_queue_element_t *)*dest_record_handle);              
        }
        
        if(dest_error_element != NULL)
        {
            if((dest_error_element->dest_error_record.object_id == dest_error_record->object_id))
            {
                /* Match LBA range.
                */
                if((dest_error_element->dest_error_record.lba_start == dest_error_record->lba_start) &&
                        (dest_error_element->dest_error_record.lba_end == dest_error_record->lba_end))
                {
                    *dest_record_handle = (fbe_dest_record_handle_t *)dest_error_element;  
                    *dest_error_record = dest_error_element->dest_error_record;
                }  
                else
                {
                    *dest_record_handle = NULL;
                }
                break;  
            }
            *dest_record_handle = (fbe_dest_record_handle_t *)dest_error_element;
        }   
        else
        {
             *dest_record_handle = NULL;
              break;  
        }
        
	}while((dest_error_element != NULL));  
    
    fbe_spinlock_unlock(&dest_error_queue_lock);
    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_dest_search_record()
 *****************************************/

/*!**************************************************************
 * fbe_dest_update_times_to_insert()
 ****************************************************************
 * @brief
 *  This function updates a record.
 *
 * @param   dest_record_handle - ptr to error record queue
 *                      element.
 *          dest_error_record - record to copy to
 *
 *
 * History:
 *  05/01/2012  Created. kothal
 ****************************************************************/
fbe_status_t 
fbe_dest_update_times_to_insert(fbe_dest_record_handle_t * dest_record_handle, fbe_dest_error_record_t * dest_error_record)
{
    fbe_dest_error_element_t * dest_error_element = NULL; 
    
    dest_log_info("DEST: %s Entry. times_to_insert=%d\n", __FUNCTION__, dest_error_record->num_of_times_to_insert);

    dest_error_element = (fbe_dest_error_element_t *)dest_record_handle;   
	
    if((dest_error_element == NULL)) {
        dest_log_error("%s Invalid handle\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }  

    fbe_spinlock_lock(&dest_error_queue_lock);
	
    dest_error_element = (fbe_dest_error_element_t *)fbe_queue_next(&dest_error_queue, (fbe_queue_element_t *)dest_error_element);

    if(dest_error_element != NULL)
    {
       dest_error_element->dest_error_record.num_of_times_to_insert = dest_error_record->num_of_times_to_insert;
    }

    fbe_spinlock_unlock(&dest_error_queue_lock);

    return FBE_STATUS_OK; 
}
/**************************************
 * end fbe_dest_update_times_to_insert()
 **************************************/

/************************************************************
 *  fbe_dest_start()
 ************************************************************
 *
 * @brief   Start error injection.   This allow callback hook
 *          function to start processing error injections for
 *          the records that have already been added.
 * 
 * @param   void
 * @return  status
 * @author
 *  06/01/2011  Wayne Garrett  - Created
 *
 ************************************************************/

fbe_status_t 
fbe_dest_start(void)
{
    fbe_status_t status;
    dest_log_info("DEST: %s Entry\n", __FUNCTION__);
    status = fbe_dest_timer_init();      // starts a thread to manage delay timer
    if(status != FBE_STATUS_OK)
    {
        dest_log_info("DEST: %s Failed to start delay timer thread.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }	

    dest_started = FBE_TRUE;    // okay to process edge hook
    return FBE_STATUS_OK;

}
/*****************************************
 * fbe_dest_start()
 *****************************************/

/************************************************************
 *  fbe_dest_stop()
 ************************************************************
 *
 * @brief   Stop error injection.   This prevents callback hook
 *          function to start processing error injections for
 *          the records that have already been added.
 * 
 * @param   void
 * @return  status
 * @author
 *  06/01/2011  Wayne Garrett  - Created
 *
 ************************************************************/
fbe_status_t 
fbe_dest_stop(void)
{
   dest_log_info("DEST: %s Entry\n", __FUNCTION__);
    dest_started = FBE_FALSE;
    fbe_dest_timer_destroy();
    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_dest_stop()
 *****************************************/


/************************************************************
 *  fbe_dest_edge_hook_function()
 ************************************************************
 *
 * @brief   Edge hook callback function which handles the
 *          error injection.
 * 
 * @param   packet -> IO Packet which is used to determine if
 *          an IO qualifies for injection.
 * 
 * @return  status
 * @author
 *  06/01/2011  Wayne Garrett  - Created
 *
 ************************************************************/
static fbe_status_t 
fbe_dest_edge_hook_function(fbe_packet_t * packet)
{
    /* Find record by object_id */
    fbe_dest_error_element_t * dest_error_element = NULL;
    fbe_base_edge_t * edge;
    fbe_object_id_t client_id;
    fbe_transport_id_t transport_id;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * cdb_operation = NULL;
//    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_status_t status;
    
    /* Check if this is the `remove hook' control code.
    */
	payload = fbe_transport_get_payload_ex(packet);
    if (((fbe_payload_operation_header_t *)payload->current_operation)->payload_opcode == FBE_PAYLOAD_OPCODE_CONTROL_OPERATION)
    {
        fbe_payload_control_operation_t                *control_operation = NULL; 
        fbe_payload_control_operation_opcode_t          opcode;
        fbe_transport_control_remove_edge_tap_hook_t   *remove_hook_p = NULL;
        
        control_operation = fbe_payload_ex_get_control_operation(payload);
        fbe_payload_control_get_opcode(control_operation, &opcode);
        if ((opcode == FBE_SSP_TRANSPORT_CONTROL_CODE_REMOVE_EDGE_TAP_HOOK) ||
                (opcode == FBE_STP_TRANSPORT_CONTROL_CODE_REMOVE_EDGE_TAP_HOOK))
        {
            fbe_payload_control_get_buffer(control_operation, &remove_hook_p);
            dest_log_info("%s request to remove hook for obj: 0x%x\n", 
                          __FUNCTION__, remove_hook_p->object_id);
            
            /* Now remove all records associated with this object and then
             * remove the object.
             */
            status = fbe_dest_remove_object(remove_hook_p->object_id);
            return status;
        }
    }

    if(!dest_started){
        return FBE_STATUS_CONTINUE;
    }

    dest_log_debug("%s Entry \n",__FUNCTION__);

    edge = fbe_transport_get_edge(packet);
    payload = fbe_transport_get_payload_ex(packet);
    fbe_base_transport_get_client_id(edge, &client_id);
    fbe_base_transport_get_transport_id(edge, &transport_id);


    /* currently error injection only supported for SSP edge*/

    if (transport_id != FBE_TRANSPORT_ID_SSP)
    {
        dest_log_error("%s Error Injection currently not supported for transport ID %X\n", __FUNCTION__, transport_id);
        return FBE_STATUS_CONTINUE;
    }

    //if(transport_id == FBE_TRANSPORT_ID_SSP){
        cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    //}else if(transport_id == FBE_TRANSPORT_ID_STP){
    //    fis_operation = fbe_payload_ex_get_fis_operation(payload);
    //} else {
    //    dest_log_error("%s Invalid transport_id %X\n", __FUNCTION__, transport_id);
    //    return FBE_STATUS_CONTINUE; /* We do not want to be too disruptive */
    //}

    fbe_spinlock_lock(&dest_error_queue_lock);
    dest_error_element = (fbe_dest_error_element_t *)fbe_queue_front(&dest_error_queue);
    while(dest_error_element != NULL){
        if(dest_error_element->dest_error_record.object_id == client_id) /* We found our record */
        {                        
            if (fbe_dest_record_qualify_for_injection(packet, cdb_operation, dest_error_element))
            {              
                        
                /* Errors injected during spinup/down are injected on send path */
                if (dest_error_element->dest_error_record.injection_flag & FBE_DEST_INJECTION_ON_SEND_PATH ||
                    cdb_operation->cdb[0] == FBE_SCSI_REASSIGN_BLOCKS) // reassign will always return immediately to avoid remapping healthy blocks.
                {

                    dest_log_info("DEST: Injecting error on send. opcode=0x%x oid=0x%x rec=0x%x pkt:0x%llx\n", 
                                  cdb_operation->cdb[0], client_id, (unsigned int)&dest_error_element->dest_error_record, (unsigned long long)packet);

                    /* call hook completion function directly since error will be injected now (on send path) and returned immediately*/
                    status = fbe_dest_edge_hook_function_completion(packet, dest_error_element);                          
                    fbe_spinlock_unlock(&dest_error_queue_lock);    

                    if (status != FBE_STATUS_MORE_PROCESSING_REQUIRED)
                    {
                        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
                        fbe_transport_complete_packet(packet);  
                    }                                           
                    return FBE_STATUS_OK;
                }
                else  // inject on completion path
                {
                    /* add error injection completion routine to stack, which will be called when IO returns.*/
                    dest_log_info("DEST: Injecting error on completion. opcode=0x%x oid=0x%x rec=0x%x pkt:0x%llx\n", 
                                  cdb_operation->cdb[0], client_id, (unsigned int)&dest_error_element->dest_error_record, (unsigned long long)packet);
                    fbe_transport_set_completion_function(packet, fbe_dest_edge_hook_function_completion, dest_error_element);
                    fbe_spinlock_unlock(&dest_error_queue_lock); 
                    return FBE_STATUS_CONTINUE;  /* causes IO to continue down path to drive*/
                }
            }            
             
        } /* It is not our record */

        dest_error_element = (fbe_dest_error_element_t *)fbe_queue_next(&dest_error_queue, (fbe_queue_element_t *)dest_error_element);
    }/* while(dest_error_element != NULL) */

    fbe_spinlock_unlock(&dest_error_queue_lock);


    /* SPECIAL CASE: If a Reassign was caused by error injection then we don't send it to drive, we'll complete it immediately.
       This avoids remapping healthy blocks */
    if ((cdb_operation->cdb[0] == FBE_SCSI_REASSIGN_BLOCKS) && 
        (payload->physical_drive_transaction.last_error_code == 0xFFFFFFFF)) {  //already injected

        dest_log_info("DEST: Simulating reassign on send path, due to previous error injection\n");
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);                        
        return FBE_STATUS_OK;
    }  

    return FBE_STATUS_CONTINUE;
}
/*****************************************
 * fbe_dest_edge_hook_function()
 *****************************************/

/************************************************************
 *  fbe_dest_edge_hook_function_completion()
 ************************************************************
 *
 * @brief   This completion function is used to injected an IO
 *          on completion path.  IO must be sent to drive before
 *          being injected, otherwise there is a potentional to
 *          cause a Data Miss Compare (DMC).
 * 
 * @param   packet - IO Packet which will be injected.
 *          context - Error record to inject
 * 
 * @return  status
 * @author
 *  06/01/2011  Wayne Garrett  - Created
 *
 ************************************************************/
/* this completion function is used when IO is injected on completion path*/
static fbe_status_t fbe_dest_edge_hook_function_completion(struct fbe_packet_s * packet, fbe_packet_completion_context_t context)
{
    fbe_dest_error_element_t *error_element = (fbe_dest_error_element_t *)context;
    fbe_payload_cdb_operation_t * cdb_operation = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_status_t status;

    if (error_element->magic_number != FBE_DEST_MAGIC_NUMBER)
    {       
        dest_log_error("DEST: %s Magic Number 0x%x does not match 0x%x.  Most likely code bug by setting wrong context\n", 
                       __FUNCTION__, error_element->magic_number, FBE_DEST_MAGIC_NUMBER);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload = fbe_transport_get_payload_ex(packet);

      
    /* currently only supports ssp scsi injection*/   
    cdb_operation = fbe_payload_ex_get_cdb_operation(payload);  
    if (cdb_operation == NULL)
    {
        dest_log_error("DEST: %s CDB operation is NULL.  Currently only supporting CDB operations\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* check for delay insertion.  If so, set timer and completion function */
    if (error_element->dest_error_record.delay_io_msec > 0)
    {
        fbe_lba_t start_lba;
        fbe_block_count_t blocks;

        status = fbe_dest_set_timer(packet, 
                           error_element->dest_error_record.delay_io_msec, 
                           fbe_dest_delay_io_completion, 
                           &error_element->dest_error_record); /*context*/

        if (status != FBE_STATUS_OK)
        {
            dest_log_info("DEST: %s Failed to set timer.  Injecting error now\n", __FUNCTION__);
            fbe_dest_insert_error(packet, cdb_operation, &error_element->dest_error_record);
            return FBE_STATUS_OK;
        }

        fbe_dest_get_start_lba_and_blocks(cdb_operation, &start_lba, &blocks);

        dest_log_info("DEST: Inserting delay %dms. oid:0x%x opcode:0x%x, start_lba:0x%llx, blocks:0x%llx, pkt:0x%p\n", 
                      error_element->dest_error_record.delay_io_msec,
                      error_element->dest_error_record.object_id,
                      (cdb_operation->cdb)[0],
                      (unsigned long long)start_lba,
                      (unsigned long long)blocks,
                      packet);  

        return FBE_STATUS_MORE_PROCESSING_REQUIRED;  // pkt will be completed when timer expires
    }
    else
    {
        fbe_dest_insert_error(packet, cdb_operation, &error_element->dest_error_record);
    } 

    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_dest_edge_hook_function_completion()
 *****************************************/


/************************************************************
 *  fbe_dest_find_error_element_by_object_id()
 ************************************************************
 *
 * @brief   Find error element by object ID.
 * 
 * @param   object_id - PDO's Object ID.
 * 
 * @return  error record queue element
 * @author
 *  06/01/2011  Wayne Garrett  - Created
 *
 ************************************************************/
static fbe_dest_error_element_t * 
fbe_dest_find_error_element_by_object_id(fbe_object_id_t object_id)
{
    fbe_dest_error_element_t * dest_error_element = NULL;

    fbe_spinlock_lock(&dest_error_queue_lock);
    dest_error_element = (fbe_dest_error_element_t *)fbe_queue_front(&dest_error_queue);
    while(dest_error_element != NULL){
        if(dest_error_element->dest_error_record.object_id == object_id){ /* We found our record */
            fbe_spinlock_unlock(&dest_error_queue_lock);
            return dest_error_element;
        } else {
            dest_error_element = (fbe_dest_error_element_t *)fbe_queue_next(&dest_error_queue, (fbe_queue_element_t *)dest_error_element);
        }
    }/* while(dest_error_element != NULL) */

    fbe_spinlock_unlock(&dest_error_queue_lock);
    return NULL;
}
/*****************************************
 * fbe_dest_find_error_element_by_object_id()
 *****************************************/

/************************************************************
 *  fbe_dest_drive_lifecycle_notification_callback()
 ************************************************************
 *
 * @brief   Register callback for PDO state changes.  This handles
 *          detection of drive going through inialization or
 *          being destroyed.   
 * 
 * @param   update_object_msg - state change information
 *          context - NULL.  Not used.
 * 
 * @return  void
 * @author
 *  06/01/2011  Wayne Garrett  - Created
 *
 ************************************************************/
static void 
fbe_dest_destroy_notification_callback(void * context)
{
    update_object_msg_t * update_object_msg = NULL;
    
    update_object_msg = (update_object_msg_t *)context;

    fbe_dest_remove_object(update_object_msg->object_id);
}
/*****************************************
 * fbe_dest_drive_lifecycle_notification_callback()
 *****************************************/


/************************************************************
 *  fbe_dest_handle_fis_error_type()
 ************************************************************
 *
 * @brief   Handle FIS error type.   
 * 
 * @param   fis_operation - fis operation
 *          dest_error_record - error record
 * 
 * @return  status
 *
 * @author  06/01/2011  Wayne Garrett  - Created
 *
 ************************************************************/
static fbe_status_t 
fbe_dest_handle_fis_error_type (fbe_payload_fis_operation_t * fis_operation, fbe_dest_error_record_t * dest_error_record)
{
    if ((dest_error_record->dest_error.dest_sata_error.sata_command == 0xFF) || 
        (dest_error_record->dest_error.dest_sata_error.sata_command == fis_operation->fis[FBE_SATA_COMMAND_FIS_COMMAND_OFFSET]))
    {
        if(dest_error_record->dest_error.dest_sata_error.port_status == FBE_PORT_REQUEST_STATUS_SATA_NCQ_ERROR)
        {
            fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_LOW_OFFSET] = fis_operation->fis[FBE_SATA_RESPONSE_FIS_LBA_LOW_OFFSET];
            fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_MID_OFFSET] = fis_operation->fis[FBE_SATA_RESPONSE_FIS_LBA_MID_OFFSET];
            fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_HIGH_OFFSET] = fis_operation->fis[FBE_SATA_RESPONSE_FIS_LBA_HIGH_OFFSET];
            fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_LOW_EXT_OFFSET] = fis_operation->fis[FBE_SATA_RESPONSE_FIS_LBA_LOW_EXT_OFFSET];
            fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_MID_EXT_OFFSET] = fis_operation->fis[FBE_SATA_RESPONSE_FIS_LBA_MID_EXT_OFFSET];
            fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_HIGH_EXT_OFFSET] = fis_operation->fis[FBE_SATA_RESPONSE_FIS_LBA_HIGH_EXT_OFFSET];
        }
        else
        {
            fbe_payload_fis_set_request_status(fis_operation, dest_error_record->dest_error.dest_sata_error.port_status);
            fbe_copy_memory(fis_operation->response_buffer, dest_error_record->dest_error.dest_sata_error.response_buffer, FBE_PAYLOAD_FIS_RESPONSE_BUFFER_SIZE);                       
        }
        
        dest_log_info("DEST: %s,Object ID:0x%x,sata_command:0x%x,port_status:0x%x\n",
                      __FUNCTION__, dest_error_record->object_id, 
                      dest_error_record->dest_error.dest_sata_error.sata_command,
                      dest_error_record->dest_error.dest_sata_error.port_status);

        fbe_atomic_32_increment(&dest_error_record->times_inserted);
        return FBE_STATUS_OK;
    }
    return FBE_STATUS_CONTINUE;
}
/************************************************************
 *  fbe_dest_handle_fis_error_type()
 ************************************************************/

/************************************************************
 *  fbe_dest_record_qualify_for_injection()
 ************************************************************
 *
 * @brief   Check if record qualifies for error injection
 * 
 * @param   packet - IO packet
 *          cdb_operation - CDB from packet payload
 *          dest_error_element - returned matching error record
 * 
 * @return  bool - true if matches
 *
 * @author  06/01/2011  Wayne Garrett  - Created
 *
 ************************************************************/

static fbe_bool_t 
fbe_dest_record_qualify_for_injection(fbe_packet_t * packet,
                                      fbe_payload_cdb_operation_t * cdb_operation,
                                      fbe_dest_error_element_t * dest_error_element)
{
    fbe_dest_error_record_t *error_record = &(dest_error_element->dest_error_record);
    fbe_status_t status;

    /* Check error rules */
    switch(error_record->dest_error_type)
    {
        case FBE_DEST_ERROR_TYPE_SCSI:    
        case FBE_DEST_ERROR_TYPE_PORT:  
        case FBE_DEST_ERROR_TYPE_NONE:  
        case FBE_DEST_ERROR_TYPE_GLITCH:
        case FBE_DEST_ERROR_TYPE_FAIL:
            status = fbe_dest_check_cdb_error_record(packet, cdb_operation, dest_error_element);
            if(status != FBE_STATUS_OK){
                dest_log_debug("DEST: %s Record doesn't qualify for error injection\n", __FUNCTION__);
                return FBE_FALSE;
            }
            dest_log_debug("DEST: %s Record qualifies for error injection\n", __FUNCTION__);
            break;

        case FBE_DEST_ERROR_TYPE_FIS:            
        default:
            dest_log_error("DEST: %s error injection for type %d not supported\n", __FUNCTION__, error_record->dest_error_type);
            return FBE_FALSE;
            break;
    }


    return FBE_TRUE;
}
/************************************************************
 *  fbe_dest_record_qualify_for_injection()
 ************************************************************/


/************************************************************
 *  fbe_dest_delay_io_completion()
 ************************************************************
 *
 * @brief   Completion function if delay was added.
 * 
 * @param   packet - IO packet
 *          context - error record
 * 
 * @return  bool - true if matches
 *
 * @author  06/01/2011  Wayne Garrett  - Created
 *
 ************************************************************/

static fbe_status_t 
fbe_dest_delay_io_completion(struct fbe_packet_s * packet, fbe_packet_completion_context_t context)
{
    fbe_dest_error_record_t *error_record = (fbe_dest_error_record_t*)context;
    fbe_payload_ex_t * payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_cdb_operation_t * cdb_operation = NULL;

    dest_log_info("DEST: %s Entry. oid:0x%x, rec:0x%p, pkt:0x%p\n", __FUNCTION__, error_record->object_id, error_record, packet);

    cdb_operation = fbe_payload_ex_get_cdb_operation(payload);

    if (cdb_operation == NULL)
    {
        dest_log_error("DEST: %s CDB operation is NULL.  Currently only supporting CDB operations\n", __FUNCTION__);
    }
    else
    {
        fbe_dest_insert_error(packet, cdb_operation, error_record);                   
    }

    return FBE_STATUS_OK;
}
/************************************************************
 *  fbe_dest_delay_io_completion()
 ************************************************************/
 
/************************************************************
 *  fbe_dest_insert_error()
 ************************************************************
 *
 * @brief   Insert the error for the given IO.
 * 
 * @param   packet - IO packet
 *          cdb_operation - CDB from packet payload
 *          error_record - error record to inject
 * 
 * @return  void
 *
 * @author  06/01/2011  Wayne Garrett  - Created
 *
 ************************************************************/
static void
fbe_dest_insert_error(fbe_packet_t * packet,
                           fbe_payload_cdb_operation_t * cdb_operation,
                           fbe_dest_error_record_t *error_record )
{
    fbe_u8_t * sense_buffer = NULL;
    fbe_lba_t lba = 0, bad_lba;
    fbe_block_count_t blocks;
    fbe_dest_scsi_error_t *scsi_error_record = &(error_record->dest_error.dest_scsi_error);    
    fbe_payload_ex_t * payload = NULL;
    fbe_u8_t valid_lba_field, error_code_field;
    fbe_status_t status;  
    fbe_atomic_32_t previous_val, times_inserted;

    dest_log_info("DEST: %s Entry. oid=0x%x rec=0x%x\n", __FUNCTION__, error_record->object_id, (unsigned int)error_record);  
        
    times_inserted = fbe_atomic_32_increment(&error_record->times_inserted);


    if ( times_inserted == error_record->num_of_times_to_insert )
    {
        /* After this injection the record will expire.  Initialize values used
           for reactivation*/

        /* Set the timestamp so that we know the exact time
         * when we went over the threshold.
         */
        error_record->max_times_hit_timestamp = fbe_get_time();

        /* Update the error tracker */
        previous_val = fbe_atomic_32_exchange(&error_record->is_active_record, FALSE);
        if (previous_val == FALSE)
        {
            dest_log_error("DEST: Deactivating a deactivated record. skipping insertion. oid=0x%x rec=0x%x\n", error_record->object_id, (unsigned int)error_record);
            fbe_atomic_32_decrement(&error_record->times_inserted);
            return;
        }

        dest_log_info("DEST: Record will be deactivated after insertion. oid=0x%x rec=0x%x\n", error_record->object_id, (unsigned int)error_record);
                      
        /* If we are doing random gap between record reactivation, then
         * initialize are random counters.
         */
        if(error_record->is_random_gap)
        {
            if(error_record->react_gap_type == FBE_DEST_REACT_GAP_TIME) 
            {
                error_record->react_gap_msecs = fbe_random() % (error_record->max_rand_msecs_to_reactivate+1);   
                dest_log_info("DEST: %s setting random secs to reactivate %d. oid=0x%x rec=0x%p\n", __FUNCTION__, error_record->react_gap_msecs, error_record->object_id, error_record);
            }
            else // FBE_DEST_REACT_GAP_IO_COUNT
            {
                error_record->react_gap_io_count = fbe_random() % (error_record->max_rand_react_gap_io_count+1);   
                dest_log_info("DEST: %s setting random io count reactivate %d. oid=0x%x rec=0x%p\n", __FUNCTION__, error_record->react_gap_io_count, error_record->object_id, error_record);
            }
        }
    }
    else if ( times_inserted > error_record->num_of_times_to_insert )
    {
        dest_log_error("DEST: too many insertions. skipping. inserting:%d inserted:%d num2ins:%d oid=0x%x rec=0x%x\n", 
                       error_record->times_inserting, times_inserted, error_record->num_of_times_to_insert, error_record->object_id, (unsigned int)error_record);        
        fbe_atomic_32_decrement(&error_record->times_inserted);
        return;
    }

		
    		
    switch(error_record->dest_error_type)
    {
    case FBE_DEST_ERROR_TYPE_GLITCH:
        status = fbe_dest_insert_error_glitch(error_record);  
        if(status != FBE_STATUS_OK){
            return;
        }
        break;
    case FBE_DEST_ERROR_TYPE_FAIL:
        status = fbe_dest_insert_error_fail(error_record);
        if(status != FBE_STATUS_OK){
            return;
        }
        break;
    case FBE_DEST_ERROR_TYPE_NONE:
         return;
    case FBE_DEST_ERROR_TYPE_SCSI:
    case FBE_DEST_ERROR_TYPE_PORT:
        dest_log_info("DEST: %s Error type %d\n", __FUNCTION__, error_record->dest_error_type);
        break;
    default:
        dest_log_error("DEST: %s Unhandled error type %d\n", __FUNCTION__, error_record->dest_error_type);
    	break;
    }

    /* Mark the payload to be inserted error */
    payload = fbe_transport_get_payload_ex(packet);
    if (payload) {
        payload->physical_drive_transaction.last_error_code = 0xFFFFFFFF;
    }
    

    if (FBE_DEST_ERROR_TYPE_PORT == error_record->dest_error_type)
    {
        /* If port error is inserted, then handle the port error first. */
        /* TODO: note that this is checking scsi_error_record, instead of port_error_record.  It's a
           bug which DEST is using wrong error structure.  This needs to be fixed.  -wayne */
        if(scsi_error_record->port_status != FBE_PORT_REQUEST_STATUS_SUCCESS) {
            fbe_payload_cdb_set_scsi_status(cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD);
            fbe_payload_cdb_set_request_status(cdb_operation, scsi_error_record->port_status);
            dest_log_info("DEST: Inserting Port error. oid=0x%x, rec=0x%x, prtSt=%d, Lba[0x%llx..0x%llx]\n",
                          error_record->object_id, (unsigned int)error_record, scsi_error_record->port_status,
                          (unsigned long long)error_record->lba_start, (unsigned long long)error_record->lba_end);
            return;
        }
    }              
    else if (FBE_DEST_ERROR_TYPE_SCSI == error_record->dest_error_type)
    {      
        /* Insert scsi status and sense data. */
        fbe_payload_cdb_set_scsi_status(cdb_operation, scsi_error_record->scsi_status);
        fbe_payload_cdb_set_request_status(cdb_operation, scsi_error_record->port_status);
        
        if (FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION != scsi_error_record->scsi_status)
        {
            /* if this is not a CC, then no more stuff to fill in.*/
            dest_log_info("DEST: %s,Object ID:0x%x, opcode:0x%x, prtSt:%d, scsiSt:0x%x\n",
                          __FUNCTION__, error_record->object_id, cdb_operation->cdb[0], scsi_error_record->port_status, scsi_error_record->scsi_status);
            return;
        }

        /* remaining stuff is only relevant to a check condition */

        if (error_record->valid_lba)
        {    
            fbe_dest_get_start_lba_and_blocks(cdb_operation, &lba, &blocks);  
        
            /* Get bad lba. */
            if ( lba <= error_record->lba_start )
            {
                  bad_lba = error_record->lba_start;
            }
            else 
            {
                  bad_lba = lba;
            }     
            valid_lba_field = 0x80;   /* bit 7*/
        }
        else
        {
            bad_lba = -1;
            valid_lba_field = 0x00;   /* 0 = invalid*/
        }
    
    
        if (error_record->deferred)
        {
            error_code_field = 0x01;  /* bit 0 */
        }
        else
        {
            error_code_field = 0x00;  /* 0 = current */
        }
    
        /* Get a pointer to the sense buffer */
        fbe_payload_cdb_operation_get_sense_buffer(cdb_operation, &sense_buffer);	
        
        if (error_record->requires_descriptor_format_sense_data == FBE_FALSE)
        {		
            /* Use fixed format for sense data */
            sense_buffer[FBE_SCSI_SENSE_DATA_RESPONSE_CODE_OFFSET] = 0x70 | valid_lba_field | error_code_field;        
            sense_buffer[FBE_SCSI_SENSE_DATA_SENSE_KEY_OFFSET] = scsi_error_record->scsi_sense_key;    
            sense_buffer[FBE_SCSI_SENSE_DATA_ASC_OFFSET] = scsi_error_record->scsi_additional_sense_code;    
            sense_buffer[FBE_SCSI_SENSE_DATA_ASCQ_OFFSET] = scsi_error_record->scsi_additional_sense_code_qualifier;	
    
            /* Fill the bad LBA value. This will be the bad LBA for media errors. */
            sense_buffer[6] = (fbe_u8_t)(0xFF & bad_lba);    
            sense_buffer[5] = (fbe_u8_t)(0xFF & (bad_lba >> 8));    
            sense_buffer[4] = (fbe_u8_t)(0xFF & (bad_lba >> 16));    
            sense_buffer[3] = (fbe_u8_t)(0xFF & (bad_lba >> 24));
        }
        else
        {
            /* Use descriptor format for sense data 
             *  fill in the header information
             */
            sense_buffer[FBE_SCSI_SENSE_DATA_RESPONSE_CODE_OFFSET] = 0x72 | error_code_field; 
            sense_buffer[FBE_SCSI_SENSE_DATA_SENSE_KEY_OFFSET_DESCRIPTOR_FORMAT] = scsi_error_record->scsi_sense_key;    
            sense_buffer[FBE_SCSI_SENSE_DATA_ASC_OFFSET_DESCRIPTOR_FORMAT] = scsi_error_record->scsi_additional_sense_code;    
            sense_buffer[FBE_SCSI_SENSE_DATA_ASCQ_OFFSET_DESCRIPTOR_FORMAT] = scsi_error_record->scsi_additional_sense_code_qualifier;	
            
            /* For the bad lba information fill in the descriptor information for
             *   descriptor 0x00 the Information Sense Data Descriptor 
             */
            sense_buffer[FBE_SCSI_SENSE_DATA_ADDITIONAL_SENSE_LENGTH_OFFSET_DESCRIPTOR_FORMAT] = 0x0B; 
            sense_buffer[FBE_SCSI_SENSE_DATA_DESCRIPTOR_TYPE_OFFSET_DESCRIPTOR_FORMAT] = 0x00;        
            sense_buffer[FBE_SCSI_SENSE_DATA_VALID_BIT_OFFSET_DESCRIPTOR_FORMAT] = 0x00 | valid_lba_field;
                               
            /* Fill in the 64 bit bad LBA value. This will be the bad LBA for media errors. */
            sense_buffer[FBE_SCSI_SENSE_DATA_BAD_LBA_MSB_OFFSET_DESCRIPTOR_FORMAT] = (fbe_u8_t)(0xFF & (bad_lba >> 56));
            sense_buffer[FBE_SCSI_SENSE_DATA_BAD_LBA_MSB_OFFSET_DESCRIPTOR_FORMAT + 1] = (fbe_u8_t)(0xFF & (bad_lba >> 48));		
            sense_buffer[FBE_SCSI_SENSE_DATA_BAD_LBA_MSB_OFFSET_DESCRIPTOR_FORMAT + 2] = (fbe_u8_t)(0xFF & (bad_lba >> 40));		
            sense_buffer[FBE_SCSI_SENSE_DATA_BAD_LBA_MSB_OFFSET_DESCRIPTOR_FORMAT + 3] = (fbe_u8_t)(0xFF & (bad_lba >> 32));		
            sense_buffer[FBE_SCSI_SENSE_DATA_BAD_LBA_MSB_OFFSET_DESCRIPTOR_FORMAT + 4] = (fbe_u8_t)(0xFF & (bad_lba >> 24));		
            sense_buffer[FBE_SCSI_SENSE_DATA_BAD_LBA_MSB_OFFSET_DESCRIPTOR_FORMAT + 5] = (fbe_u8_t)(0xFF & (bad_lba >> 16));		
            sense_buffer[FBE_SCSI_SENSE_DATA_BAD_LBA_MSB_OFFSET_DESCRIPTOR_FORMAT + 6] = (fbe_u8_t)(0xFF & (bad_lba >> 8));		
            sense_buffer[FBE_SCSI_SENSE_DATA_BAD_LBA_MSB_OFFSET_DESCRIPTOR_FORMAT + 7] = (fbe_u8_t)(0xFF & (bad_lba));		
        }
    
        dest_log_info("DEST: %s,Object ID:0x%x, 0x%02x|%02x|%02x, opcode:0x%x\n",
                      __FUNCTION__, error_record->object_id, scsi_error_record->scsi_sense_key,
                      scsi_error_record->scsi_additional_sense_code,
                      scsi_error_record->scsi_additional_sense_code_qualifier,
                      cdb_operation->cdb[0]);
    }

}
/************************************************************
 *  fbe_dest_insert_error()
 ************************************************************/

/************************************************************
 *  fbe_dest_get_start_lba_and_blocks()
 ************************************************************
 *
 * @brief   Get start LBA and num blocks for the IO request.
 * 
 * @param   cdb_operation - CDB from packet payload
 *          start_lba - returns start LBA
 *          blocks - returns num blocks
 * 
 * @return  status
 *
 * @author  06/01/2011  Wayne Garrett  - Created
 *
 ************************************************************/

/* Returns the starting LBA and block count from the CDB
 */
static fbe_status_t fbe_dest_get_start_lba_and_blocks(fbe_payload_cdb_operation_t * cdb_operation,
                                                      fbe_lba_t *lba_start,      /*out*/
                                                      fbe_block_count_t *blocks) /*out*/
{
    if (cdb_operation == NULL)
    {
        dest_log_error("DEST: %s cdb_operation is NULL\n", __FUNCTION__);
        *lba_start = (fbe_lba_t)-1;
        *blocks = 0;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get start lba_start and blocks in cdb payload. */
    if (cdb_operation->cdb_length == 6)
    {
        *lba_start = fbe_get_six_byte_cdb_lba(&cdb_operation->cdb[0]);
        *blocks = fbe_get_six_byte_cdb_blocks(&cdb_operation->cdb[0]);
    }
    else if (cdb_operation->cdb_length == 10)
    {
        *lba_start = fbe_get_ten_byte_cdb_lba(&cdb_operation->cdb[0]);
        *blocks = fbe_get_ten_byte_cdb_blocks(&cdb_operation->cdb[0]);
    }
    else if (cdb_operation->cdb_length == 16)
    {
        *lba_start = fbe_get_sixteen_byte_cdb_lba(&cdb_operation->cdb[0]);
        *blocks = fbe_get_sixteen_byte_cdb_blocks(&cdb_operation->cdb[0]);
    }

    return FBE_STATUS_OK;
}
/************************************************************
 *  fbe_dest_get_start_lba_and_blocks()
 ************************************************************/


/************************************************************
 *  fbe_dest_check_cdb_error_record()
 ************************************************************
 *
 * @brief   Verify if CDB matches the error record
 * 
 * @param   packet - IO packet
 *          cdb_operation - CDB from packet payload
 *          dest_error_element - error record
 * 
 * @return  status
 *
 * @author  06/01/2011  Wayne Garrett  - Created
 *
 ************************************************************/
static fbe_status_t 
fbe_dest_check_cdb_error_record(fbe_packet_t * packet, 
                                fbe_payload_cdb_operation_t * cdb_operation,
                                fbe_dest_error_element_t * dest_error_element)
{
    fbe_lba_t lba_start = 0;
    fbe_block_count_t blocks = 0;
    fbe_dest_error_record_t *error_record = &(dest_error_element->dest_error_record);
    fbe_u32_t elapsed_msecs = 0;
    fbe_u8_t *cmd, i = 0;
    fbe_u32_t *count;
    fbe_payload_ex_t * payload = NULL;
    fbe_atomic_32_t times_inserting, times_inserted;


    if (cdb_operation == NULL)
    {
        dest_log_error("DEST: %s cdb_operation is NULL. object id 0x%x\n", __FUNCTION__, error_record->object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get start lba_start and blocks in cdb payload. */
    fbe_dest_get_start_lba_and_blocks(cdb_operation, &lba_start, &blocks);

    dest_log_debug("DEST: %s cdb=0x%x lba=0x%llx blocks=0x%llx \n",
		   __FUNCTION__, cdb_operation->cdb[0],
		   (unsigned long long)lba_start, (unsigned long long)blocks);
  
    /* LOGIC:  Must pass the following in order to be inserted
       1. Does command match     
       2. Does LBA match
       3. a) Is record Active.
          b) If record is not Active, then check to see if it should be reactivated.
       4. Does frequency match
    */


    /* 1.   CMD Match check 
     * Now look for specific opcode. If there is no match, check if there is
     * any thing with any range.  
     */
    cmd = error_record->dest_error.dest_scsi_error.scsi_command;
    count = &error_record->dest_error.dest_scsi_error.scsi_command_count;

    if (*count == 0)  
    {
        dest_log_error("DEST: %s opcode list is empty. object id 0x%x\n",__FUNCTION__, error_record->object_id);
        return FBE_STATUS_CONTINUE;
    }
    if (cmd[0] != 0xFF)   /* If first entry is 0xFF, indicates ANY opcode. Skip checking for match */
    {
        i = 0;    
        while (i < *count) 
        {
            if (cmd[i] == cdb_operation->cdb[0])
            {
                /* We got an exact match here. */
                break;
            }
            i++;
        }
        if (i == *count)  //checked all elements and no match found
        {
            dest_log_debug("DEST: %s cmd does not match. object id 0x%x\n",__FUNCTION__, error_record->object_id);
            return FBE_STATUS_CONTINUE;
        }
    }
 

    /* 2.   LBA Match check
     * There is a match if the records overlap in some way
     * AND the record is active
     */

    /* If reassign cmd then get LBA from defect list */
    if (cdb_operation->cdb[0] == FBE_SCSI_REASSIGN_BLOCKS)  
    {
        fbe_u8_t * defect_list_p = NULL;
        fbe_sg_element_t * sg_list = NULL;

        payload = fbe_transport_get_payload_ex(packet);
        fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
        defect_list_p = sg_list[0].address;
        if (cdb_operation->cdb[1] & 0x02)  // long lba  (byte1/bit1)
        {
            lba_start = swap64(*(fbe_u64_t *)(defect_list_p+4));
            blocks = 1;
        }
        else
        {
            lba_start = (fbe_lba_t)swap32(*(fbe_u32_t *)(defect_list_p+4));
            blocks = 1;
        }
    }

    /* LBA Match check isn't needed on special commands. If so then skip this check.*/    
    if ( cdb_operation->cdb[0] != FBE_SCSI_INQUIRY &&
         cdb_operation->cdb[0] != FBE_SCSI_LOG_SENSE &&
         cdb_operation->cdb[0] != FBE_SCSI_MODE_SELECT_6 &&
         cdb_operation->cdb[0] != FBE_SCSI_MODE_SELECT_10 &&
         cdb_operation->cdb[0] != FBE_SCSI_MODE_SENSE_6 &&
         cdb_operation->cdb[0] != FBE_SCSI_MODE_SENSE_10 &&
         cdb_operation->cdb[0] != FBE_SCSI_READ_BUFFER &&
         cdb_operation->cdb[0] != FBE_SCSI_RCV_DIAGNOSTIC &&
         cdb_operation->cdb[0] != FBE_SCSI_RELEASE &&
         cdb_operation->cdb[0] != FBE_SCSI_RELEASE_10 &&
         cdb_operation->cdb[0] != FBE_SCSI_RESERVE &&
         cdb_operation->cdb[0] != FBE_SCSI_RESERVE_10 &&
         cdb_operation->cdb[0] != FBE_SCSI_SEND_DIAGNOSTIC &&
         cdb_operation->cdb[0] != FBE_SCSI_RECEIVE_DIAGNOSTIC &&
         cdb_operation->cdb[0] != FBE_SCSI_START_STOP_UNIT &&
         cdb_operation->cdb[0] != FBE_SCSI_TEST_UNIT_READY &&
         cdb_operation->cdb[0] != FBE_SCSI_WRITE_BUFFER &&
         cdb_operation->cdb[0] != FBE_SCSI_SMART_DUMP )
    {
        if (lba_start > error_record->lba_end ||
            error_record->lba_start >= lba_start + blocks)
        {
            dest_log_debug("DEST: %s LBA does not match. object id 0x%x, lba=0x%llx, blocks=0x%llx\n",
                           __FUNCTION__, error_record->object_id, (unsigned long long)lba_start, (unsigned long long)blocks);
            return FBE_STATUS_CONTINUE;
        }
    }

    /* 3a.   Active Record check*/

    times_inserting = error_record->times_inserting;
    times_inserted = error_record->times_inserted;

    if (times_inserting >= error_record->num_of_times_to_insert)  // is record inactive
    {
        if (error_record->is_active_record == FALSE)
        {
            if (times_inserting != times_inserted)
            {
                dest_log_error("DEST: %s inserting!=inserted %d!=%d\n", __FUNCTION__, times_inserting, times_inserted);
            }

            if (error_record->times_reset < error_record->num_of_times_to_reactivate)    // can record be reactivated
            {
                /* 3b. Does record need to be reactivated */
            
                switch(error_record->react_gap_type) 
                {
                    case FBE_DEST_REACT_GAP_TIME:
                        elapsed_msecs = (fbe_u32_t) (fbe_get_time() - error_record->max_times_hit_timestamp);        
                        if (error_record->react_gap_msecs != FBE_DEST_INVALID &&
                            elapsed_msecs >= error_record->react_gap_msecs)
                        {
                            /* This record should be reactivated now by resetting
                            * times_inserted.
                            */
                            error_record->times_inserting = 0;
                            error_record->times_inserted = 0;
                            /* Increment the number of times we have reset
                            * in the times_reset field.
                            */
                            error_record->times_reset++;
            
                            dest_log_debug("DEST: %s Record reactivated. oid=0x%x rec=0x%x reset=%d\n",
                                           __FUNCTION__, error_record->object_id, (unsigned int)error_record, error_record->times_reset);
                        }
                        else
                        {
                            dest_log_debug("DEST: %s Record inactive. oid=0x%x rec=0x%x\n",__FUNCTION__, error_record->object_id, (unsigned int)error_record);
                            return FBE_STATUS_CONTINUE;
                        }         
                        break;
            
                    case FBE_DEST_REACT_GAP_IO_COUNT:
                        if (error_record->react_gap_io_skipped >= error_record->react_gap_io_count) // have enough IO been skipped
                        {            
                            /* This record should be reactivated now by resetting
                            * times_inserted.
                            */
                            error_record->times_inserting = 0;
                            error_record->times_inserted = 0;
                            /* Increment the number of times we have reset
                            * in the times_reset field.
                            */
                            error_record->times_reset++;
            
                            /* clear gap skip counter */
                            error_record->react_gap_io_skipped = 0;
                        }
                        else
                        {
                            error_record->react_gap_io_skipped++;
                            dest_log_debug("DEST: %s record not eligable for reactivation, base on io count. object id 0x%x\n",__FUNCTION__, error_record->object_id);
                            return FBE_STATUS_CONTINUE;
                        }            
                        break;
            
                    default:
                        dest_log_error("DEST: %s Invalid gap type %d. object id 0x%x\n", __FUNCTION__, error_record->react_gap_type, error_record->object_id);
                        return FBE_STATUS_CONTINUE;
                }
            
                dest_log_info("DEST: %s Record 0x%X has been reactivated. object id 0x%x\n", __FUNCTION__, (unsigned int)error_record, error_record->object_id);
            
            }
            else // record cannot be reactivated
            {
                dest_log_debug("DEST: %s record cannot be reactivated. object id 0x%x\n",__FUNCTION__, error_record->object_id);
                return FBE_STATUS_CONTINUE;
            }
        }
        else // record cannot be reactivated as insertion is in progress
        {
            dest_log_debug("DEST: %s record cannot be reactivated as error insertion is in progress. object id 0x%x\n",__FUNCTION__, error_record->object_id);
            return FBE_STATUS_CONTINUE;
        }
    }


    /* 4.  Frequency check */

    error_record->qualified_io_counter++;   // this IO qualifies for insertion

    if(error_record->frequency != 0){   // was frequency enabled
        if(error_record->is_random_frequency) 
        {
            fbe_u32_t rand = fbe_random();
            if ( rand % error_record->frequency != 0)
            {
                dest_log_debug("DEST: %s random frequency not hit. rand=0x%x, freq=0x%x. oid=0x%x\n",__FUNCTION__, rand, error_record->frequency, error_record->object_id);
                return FBE_STATUS_CONTINUE;
            }
        }
        else   // fixed frequency
        {
            if((error_record->qualified_io_counter % error_record->frequency) != 0){
                dest_log_debug("DEST: %s fixed frequency not hit. object id 0x%x\n",__FUNCTION__,error_record->object_id);
                return FBE_STATUS_CONTINUE;
            }
        }
    }

    /* Update activate state */
    error_record->is_active_record = TRUE;
      
    /* Update inserting count */
    times_inserting = fbe_atomic_32_increment(&error_record->times_inserting);    

    if (times_inserting > error_record->num_of_times_to_insert)   /* covers case where multiple threads inserting at same time */
    {   
        dest_log_info("DEST: %s attempted qualify too many IOs. oid=0x%x\n",__FUNCTION__,error_record->object_id);
        fbe_atomic_32_decrement(&error_record->times_inserting);  
        return FBE_STATUS_CONTINUE;
    }

    /* ok to insert */
    dest_log_debug("DEST: %s Ok to insert. object id 0x%x , lba=0x%llx \n",__FUNCTION__,error_record->object_id, (unsigned long long)lba_start);
    return FBE_STATUS_OK;
}
/************************************************************
 *  fbe_dest_check_cdb_error_record()
 ************************************************************/

static fbe_status_t 
dest_handle_port_error(fbe_packet_t * packet, fbe_dest_error_record_t * error_record)
{
    fbe_base_edge_t * edge = NULL;
    fbe_transport_id_t transport_id;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * cdb_operation = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_atomic_32_t times_inserted;

    edge = fbe_transport_get_edge(packet);
    payload = fbe_transport_get_payload_ex(packet);
    fbe_base_transport_get_transport_id(edge, &transport_id);

    if(transport_id == FBE_TRANSPORT_ID_SSP){
        cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    }else if(transport_id == FBE_TRANSPORT_ID_STP){
        fis_operation = fbe_payload_ex_get_fis_operation(payload);
    } else {
        dest_log_error("%s Invalid transport_id %X\n", __FUNCTION__, transport_id);
        return FBE_STATUS_CONTINUE; /* We do not want to be too disruptive */
    }

    /* Check num_of_times_to_insert */
    if(error_record->num_of_times_to_insert == 0){
        return FBE_STATUS_CONTINUE;
    }

    times_inserted = fbe_atomic_32_increment(&error_record->times_inserted);

    if( error_record->num_of_times_to_insert < times_inserted){
        return FBE_STATUS_CONTINUE;
    }

    if(error_record->frequency != 0){
        if((times_inserted % error_record->frequency) != 0){
            return FBE_STATUS_CONTINUE;
        }
    }

    if(transport_id == FBE_TRANSPORT_ID_SSP){                   
        fbe_payload_cdb_set_request_status(cdb_operation, error_record->dest_error.dest_port_error.port_status);
        return FBE_STATUS_OK;
    }else {
        /* transport_id = FBE_TRANSPORT_ID_STP */
        fbe_payload_fis_set_request_status(fis_operation, error_record->dest_error.dest_port_error.port_status);
        return FBE_STATUS_OK;
    } 
}
/************************************************************
 *  fbe_dest_cleanup_queue()
 ************************************************************
 *
 * @brief   Cleans up error record queue
 * 
 * @param   void
 * 
 * @return  status
 *
 * @author  06/01/2011  Wayne Garrett  - Created
 *
 ************************************************************/
fbe_status_t 
fbe_dest_cleanup_queue(void)
{
    fbe_dest_error_element_t * dest_error_element = NULL;

    dest_log_info("DEST: %s entry\n", __FUNCTION__);     
 
    /* If error record is existed, free memory. */
    dest_error_element = (fbe_dest_error_element_t *)fbe_queue_front(&dest_error_queue);
    while(dest_error_element != NULL){
        fbe_dest_remove_record(dest_error_element);
        dest_error_element = (fbe_dest_error_element_t *)fbe_queue_front(&dest_error_queue);
    }

    /* Init the dest error queue. */
    fbe_spinlock_lock(&dest_error_queue_lock);
    fbe_queue_init(&dest_error_queue);
    fbe_spinlock_unlock(&dest_error_queue_lock);

    return FBE_STATUS_OK;
}
/************************************************************
 *  fbe_dest_cleanup_queue()
 ************************************************************/
/************************************************************
 *  fbe_dest_insert_error_glitch()
 ************************************************************
 *
 * @brief   Glitches a drive
 * 
 * @param  fbe_dest_error_record_t - error record
 * 
 * @return  status
 *
 * @author  kothal  - Created
 *
 ************************************************************/
fbe_status_t 
fbe_dest_insert_error_glitch(fbe_dest_error_record_t * error_record)
{   
    fbe_status_t status;
    dest_log_info("DEST: Inserting glitch. oid=0x%x rec=0x%p\n", error_record->object_id, error_record);
    status = fbe_api_physical_drive_enter_glitch_drive(error_record->object_id, error_record->glitch_time);
    if (status != FBE_STATUS_OK) {
         dest_log_error("DEST: %s Glitch fail Error %d\n", __FUNCTION__, status);
         return status;
    }
	return FBE_STATUS_OK;
}
/************************************************************
 *  fbe_dest_insert_error_glitch()
 ************************************************************/
/************************************************************
 *  fbe_dest_insert_error_fail()
 ************************************************************
 *
 * @brief   Fails a drive
 * 
 * @param  fbe_dest_error_record_t - error record
 * 
 * @return  status
 *
 * @author  kothal  - Created
 *
 ************************************************************/
fbe_status_t 
fbe_dest_insert_error_fail(fbe_dest_error_record_t * error_record)
{
    fbe_status_t status;
    dest_log_info("DEST: Failing drive. oid=0x%x rec=0x%p\n", error_record->object_id, error_record);
    status = fbe_api_physical_drive_fail_drive(error_record->object_id, FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_SIMULATED);
    if (status != FBE_STATUS_OK) {
        dest_log_error("DEST: %s Fail drive fail Error %d\n", __FUNCTION__, status);
        return status;
    }
    return  FBE_STATUS_OK;
}
/************************************************************
 *  fbe_dest_insert_error_fail()
 ************************************************************/
