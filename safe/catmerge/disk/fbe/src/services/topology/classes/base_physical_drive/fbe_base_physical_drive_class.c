/**********************************************************************
 *  Copyright (C)  EMC Corporation 2008
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_transport.h"
//#include "fbe/fbe_ctype.h"

#include "fbe_transport_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"

#include "swap_exports.h"

#include "base_physical_drive_private.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe_service_manager.h"


static fbe_status_t fbe_physical_drive_prefstats_set_enabled(fbe_packet_t * packet);
static fbe_status_t fbe_physical_drive_prefstats_get_total_objects(fbe_package_id_t package_id, fbe_u32_t *total_objects_p);
static fbe_status_t fbe_physical_drive_prefstats_send_packet_to_service(fbe_payload_control_operation_opcode_t control_code,
                                                 fbe_payload_control_buffer_t buffer,
                                                 fbe_payload_control_buffer_length_t buffer_length,
                                                 fbe_service_id_t service_id,
                                                 fbe_sg_element_t *sg_list,
                                                 fbe_u32_t sg_list_count,
                                                 fbe_packet_attr_t attr,
                                                 fbe_package_id_t package_id);

static fbe_status_t fbe_physical_drive_prefstats_forward_packet_to_object(fbe_packet_t *packet,
                                                fbe_payload_control_operation_opcode_t control_code,
                                                fbe_payload_control_buffer_t buffer,
                                                fbe_payload_control_buffer_length_t buffer_length,
                                                fbe_object_id_t object_id,
                                                fbe_sg_element_t *sg_list,
                                                fbe_u32_t sg_list_count,
                                                fbe_packet_attr_t attr,
                                                fbe_package_id_t package_id);

static fbe_status_t fbe_physical_drive_allocate_memory(fbe_u32_t object_count, fbe_object_id_t** obj_list);
static fbe_status_t fbe_physical_drive_enumerate_object(fbe_object_id_t * obj_array, 
                                                    fbe_u32_t total_objects, 
                                                    fbe_u32_t *actual_objects, 
                                                    fbe_package_id_t package_id);
static fbe_status_t fbe_physical_drive_prefstats_enable_peformance_stats(fbe_packet_t * packet, fbe_object_id_t object_id, fbe_package_id_t package_id, fbe_class_id_t filter_class_id, fbe_bool_t enable);
static fbe_status_t fbe_physical_drive_get_object_class_id(fbe_packet_t * packet, fbe_object_id_t object_id, fbe_class_id_t *class_id, fbe_package_id_t package_id);
static fbe_status_t fbe_physical_drive_perfstats_forward_packet_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_physical_drive_prefstats_enable_disable_peformance_stats(fbe_object_id_t object_id, fbe_packet_t *packet, fbe_bool_t enable);

/* Disable perf stats*/

static fbe_status_t fbe_physical_drive_prefstats_set_disabled(fbe_packet_t * packet);

fbe_status_t fbe_base_physical_drive_class_control_entry(fbe_packet_t * packet)
{
    fbe_status_t                            status;
    fbe_payload_ex_t *                     pp_payload = NULL;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_payload_control_operation_opcode_t  opcode;

    pp_payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(pp_payload);
    fbe_payload_control_get_opcode(control_operation, &opcode);

    switch (opcode) {
        case  FBE_PHYSICAL_DRIVE_CONTROL_CODE_CLASS_PREFSTATS_SET_ENABLED:
            status = fbe_physical_drive_prefstats_set_enabled(packet);
            break;
        case  FBE_PHYSICAL_DRIVE_CONTROL_CODE_CLASS_PREFSTATS_SET_DISABLED:
            status = fbe_physical_drive_prefstats_set_disabled(packet);
            break;
        default:
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            status = fbe_transport_complete_packet(packet);
            break;
    }
   return status;
}

static fbe_status_t fbe_physical_drive_prefstats_set_enabled(fbe_packet_t * packet) 
{
   fbe_status_t status = FBE_STATUS_OK;
   fbe_u32_t                                   obj_i = 0;
   fbe_object_id_t                             *obj_list = NULL;
   fbe_u32_t                                   obj_count = 0;
   fbe_u32_t                                   actual_obj_count = 0;
   fbe_payload_ex_t *                                      pp_payload = NULL;
   fbe_payload_control_operation_t *                       control_operation = NULL; 
   fbe_class_id_t*                               filter_class_id;

     /* get the payload from packet */
   pp_payload = fbe_transport_get_payload_ex(packet);
   control_operation = fbe_payload_ex_get_control_operation(pp_payload);  

   fbe_payload_control_get_buffer(control_operation, &filter_class_id); 

   /* Get total objects*/
    status = fbe_physical_drive_prefstats_get_total_objects(FBE_PACKAGE_ID_PHYSICAL, &obj_count);
    if ( obj_count == 0 ){
        obj_list  = NULL;
        return status;
    }

    status = fbe_physical_drive_allocate_memory(sizeof(fbe_object_id_t) * obj_count, &obj_list);
    if (status != FBE_STATUS_OK) {
       fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: Unable to allocate memory for object list\n", __FUNCTION__);
    }

    status = fbe_physical_drive_enumerate_object(obj_list, obj_count, &actual_obj_count, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
       fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: Unable to obtain object list for package ID: 0x%x\n", __FUNCTION__, FBE_PACKAGE_ID_PHYSICAL);

	   fbe_release_nonpaged_pool_with_tag(obj_list, 'ljbo');

	   /* set the packet status with completion and return it back to source */
	   fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
	   fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
	   fbe_transport_complete_packet(packet);
	   return FBE_STATUS_OK;   

    }

    for(obj_i = 0; obj_i < obj_count; obj_i++) {
       fbe_object_id_t object_id = obj_list[obj_i];
       status = fbe_physical_drive_prefstats_enable_peformance_stats(packet, object_id, FBE_PACKAGE_ID_PHYSICAL, (*filter_class_id), FBE_TRUE);
        if (status != FBE_STATUS_OK) {
           fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: Unable to enable performance stats for package ID: 0x%x\n", __FUNCTION__, FBE_PACKAGE_ID_PHYSICAL);
        }
    }

    /* releasing obj_list. It was allocated in fbe_physical_drive_allocate_memory and is being used in this function. */
    fbe_release_nonpaged_pool_with_tag(obj_list, 'ljbo');
    
    /* set the packet status with completion and return it back to source */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;   

}

static fbe_status_t fbe_physical_drive_prefstats_enable_peformance_stats(fbe_packet_t *packet, fbe_object_id_t object_id, fbe_package_id_t package_id, fbe_class_id_t filter_class_id, fbe_bool_t enable)
{
   fbe_status_t status;
   fbe_u32_t                                   count = 0;
   fbe_class_id_t  class_id = FBE_CLASS_ID_INVALID;

   /*now, let's wait if the object is in specialize state*/
   do {
      if (count != 0) {
         EmcutilSleep(70);/*wait for object to get out of specialized state*/
      }
      if (count == 1000) {
         status = FBE_STATUS_GENERIC_FAILURE;
         fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: Waited for object id: 0x%x for more than 70 seconds\n", __FUNCTION__, object_id);
         break;
      }
      count++;
      status = fbe_physical_drive_get_object_class_id(packet, object_id, &class_id, package_id);
   } while (status == FBE_STATUS_BUSY);

   /* Match the class id to the filter class id. */
   if ((status == FBE_STATUS_OK) && (class_id == filter_class_id))
   {
      status = fbe_physical_drive_prefstats_enable_disable_peformance_stats(object_id, packet, enable);
      if (status != FBE_STATUS_OK) {
         fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: Unable to enable performance stats for package ID: 0x%x\n", __FUNCTION__, FBE_PACKAGE_ID_PHYSICAL);
         return status;
      }
   } /* End of class id fetched OK and it is a match */
   else if (status != FBE_STATUS_OK)
   {
      fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INFO, "%s: Can't get object class_id for id [%x], status: %x\n",
                    __FUNCTION__, object_id, status);
   }

   /* We return OK because we dont want to stop enabling perfstats due to one failed drive*/
   return FBE_STATUS_OK;

}


static fbe_status_t fbe_physical_drive_get_object_class_id(fbe_packet_t * packet, fbe_object_id_t object_id, fbe_class_id_t *class_id, fbe_package_id_t package_id)
{
   fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
   fbe_base_object_mgmt_get_class_id_t             base_object_mgmt_get_class_id;  

   status = fbe_physical_drive_prefstats_forward_packet_to_object(packet, FBE_BASE_OBJECT_CONTROL_CODE_GET_CLASS_ID,
                                                         &base_object_mgmt_get_class_id,
                                                         sizeof(fbe_base_object_mgmt_get_class_id_t),
                                                         object_id,
                                                         NULL,
                                                         0,
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         FBE_PACKAGE_ID_PHYSICAL);
   if (status != FBE_STATUS_OK) {
      fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: cant get object class id for package ID: 0x%x\n", __FUNCTION__, FBE_PACKAGE_ID_PHYSICAL);
      return status;
   }

   *class_id = base_object_mgmt_get_class_id.class_id;

   return status;
}


static fbe_status_t fbe_physical_drive_prefstats_get_total_objects(fbe_package_id_t package_id, fbe_u32_t *total_objects_p)
{
   fbe_status_t status;
   fbe_topology_control_get_total_objects_t get_total;

   get_total.total_objects = 0;
   
   status = fbe_physical_drive_prefstats_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_GET_TOTAL_OBJECTS,
                                                     &get_total,
                                                     sizeof (fbe_topology_control_get_total_objects_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     NULL,  /* no sg list*/
                                                     0,  /* no sg list*/
                                                     FBE_PACKET_FLAG_NO_ATTRIB,  /* no sg list*/
                                                     package_id
                                                     );

   *total_objects_p = get_total.total_objects;

   return status;
}


static fbe_status_t fbe_physical_drive_allocate_memory(fbe_u32_t object_count, fbe_object_id_t** obj_list)
{
   fbe_status_t status = FBE_STATUS_OK;
   *obj_list = fbe_allocate_nonpaged_pool_with_tag(sizeof(fbe_object_id_t) * object_count, 'ljbo' );
    if (*obj_list == NULL) {
        fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s out of memory\n",
                                 __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   return status;
}

static fbe_status_t fbe_physical_drive_enumerate_object(fbe_object_id_t * obj_array, 
                                                    fbe_u32_t total_objects, 
                                                    fbe_u32_t *actual_objects, 
                                                    fbe_package_id_t package_id)
{
   fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
   fbe_topology_mgmt_enumerate_objects_t   enumerate_objects;/*structure is small enough to be on the stack*/

   /* one for the entry and one for the NULL termination.
     */
    fbe_sg_element_t *                      sg_list = NULL;
    fbe_sg_element_t *                      temp_sg_list = NULL;

    sg_list = fbe_allocate_nonpaged_pool_with_tag(sizeof(fbe_sg_element_t) * FBE_TOPLOGY_CONTROL_ENUMERATE_CLASS_SG_COUNT, 'tats' );
    if (sg_list == NULL) {
        fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s out of memory\n",
                                 __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    temp_sg_list = sg_list;

    /*set up the sg list to point to the list of objects the user wants to get*/
    temp_sg_list->address = (fbe_u8_t *)obj_array;
    temp_sg_list->count = total_objects * sizeof(fbe_object_id_t);;
    temp_sg_list ++;
    temp_sg_list->address = NULL;
    temp_sg_list->count = 0;

    enumerate_objects.number_of_objects = total_objects;

    status = fbe_physical_drive_prefstats_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_ENUMERATE_OBJECTS,
                                                                        &enumerate_objects,
                                                                        sizeof (fbe_topology_mgmt_enumerate_objects_t),
                                                                        FBE_SERVICE_ID_TOPOLOGY,
                                                                        sg_list,
                                                                        FBE_TOPLOGY_CONTROL_ENUMERATE_CLASS_SG_COUNT, /* sg entries*/
                                                                        FBE_PACKET_FLAG_NO_CONTIGUOS_ALLOCATION_NEEDED,
                                                                        package_id);
    if (status != FBE_STATUS_OK) {
        fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s:packet error:%d\n",
                                 __FUNCTION__,status);
        fbe_release_nonpaged_pool_with_tag(sg_list, 'tats');
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (actual_objects != NULL) {

       *actual_objects = enumerate_objects.number_of_objects_copied;
    }
    
    fbe_release_nonpaged_pool_with_tag(sg_list, 'tats');

    return FBE_STATUS_OK;
}


static fbe_status_t fbe_physical_drive_prefstats_enable_disable_peformance_stats(fbe_object_id_t object_id, fbe_packet_t *packet, fbe_bool_t enable)
{
   fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;

   if (object_id >= FBE_OBJECT_ID_INVALID) {
      return FBE_STATUS_GENERIC_FAILURE;
   }

   status = fbe_physical_drive_prefstats_forward_packet_to_object (packet, 
                                                            FBE_PHYSICAL_DRIVE_CONTROL_CODE_ENABLE_DISABLE_PERF_STATS,
                                                            &enable,
                                                            sizeof(fbe_bool_t),
                                                            object_id,
                                                            NULL,
                                                            0,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            FBE_PACKAGE_ID_PHYSICAL);

   if (status != FBE_STATUS_OK) {
      fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s:packet error:%d\n",
                                    __FUNCTION__,status);
      return FBE_STATUS_GENERIC_FAILURE;
   }
   
   return status;
}

static fbe_status_t fbe_physical_drive_prefstats_send_packet_to_service(fbe_payload_control_operation_opcode_t control_code,
                                                 fbe_payload_control_buffer_t buffer,
                                                 fbe_payload_control_buffer_length_t buffer_length,
                                                 fbe_service_id_t service_id,
                                                 fbe_sg_element_t *sg_list,
                                                 fbe_u32_t sg_list_count,
                                                 fbe_packet_attr_t attr,
                                                 fbe_package_id_t package_id)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet = NULL;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_payload_control_status_qualifier_t status_qualifier;
	fbe_cpu_id_t cpu_id;

    /* Allocate packet.*/
    packet = fbe_transport_allocate_packet();
    if (packet == NULL) {
        fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                  FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s: unable to allocate packet\n", 
                        __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    fbe_transport_initialize_packet(packet);
    payload = fbe_transport_get_payload_ex (packet);
	fbe_payload_ex_allocate_memory_operation(payload);

    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    if (sg_list != NULL) {    
		fbe_payload_ex_set_sg_list (payload, sg_list, sg_list_count);
    }

   	cpu_id = fbe_get_cpu_id();
	fbe_transport_set_cpu_id(packet, cpu_id);

    fbe_payload_control_build_operation (control_operation,
                                         control_code,
                                         buffer,
                                         buffer_length);
    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              package_id,
                              service_id,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID); 
    
    /* Mark packet with the attribute, either traverse or not */
    fbe_transport_set_packet_attr(packet, attr);

    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    status = fbe_service_manager_send_control_packet(packet);
    if (status != FBE_STATUS_OK) {
        fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                  FBE_TRACE_LEVEL_WARNING, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s: error in sending 0x%x to srv 0x%x, status:0x%x\n",
                                 __FUNCTION__, control_code, service_id, status);  
    }

    fbe_transport_wait_completion(packet);

    /* Save the status for returning.*/
    status = fbe_transport_get_status_code(packet);
    if (status != FBE_STATUS_OK ) {
        fbe_topology_class_trace (FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                  FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: 0x%x to srv 0x%x, return status:0x%x\n",
                       __FUNCTION__, control_code, service_id, status);  
    }
    else
    {
        fbe_payload_control_get_status(control_operation, &control_status);
        if (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK ) {
            fbe_topology_class_trace (FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                      FBE_TRACE_LEVEL_WARNING,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: 0x%x to srv 0x%x, return payload status:0x%x\n",
                           __FUNCTION__, control_code, service_id, control_status);
    
            switch (control_status){
            case FBE_PAYLOAD_CONTROL_STATUS_BUSY:
                status = FBE_STATUS_BUSY;
                break;
            case FBE_PAYLOAD_CONTROL_STATUS_INSUFFICIENT_RESOURCES:
                status = FBE_STATUS_INSUFFICIENT_RESOURCES;
                break;
            case FBE_PAYLOAD_CONTROL_STATUS_FAILURE:
                fbe_payload_control_get_status_qualifier(control_operation, &status_qualifier);
                status = (status_qualifier == FBE_OBJECT_ID_INVALID) ? FBE_STATUS_NO_OBJECT: FBE_STATUS_GENERIC_FAILURE;
                break;
            default:
                status = FBE_STATUS_GENERIC_FAILURE;
                break;
            }
            fbe_topology_class_trace (FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                      FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: Set return status to 0x%x\n",
                           __FUNCTION__, status);  
        }
    }

    /* free the memory*/
    fbe_payload_ex_release_control_operation(payload, control_operation);
	fbe_payload_ex_release_memory_operation(payload, payload->payload_memory_operation);
    fbe_transport_release_packet(packet);

    return status;
}  
 
static fbe_status_t fbe_physical_drive_prefstats_forward_packet_to_object(fbe_packet_t *packet,
                                                fbe_payload_control_operation_opcode_t control_code,
                                                fbe_payload_control_buffer_t buffer,
                                                fbe_payload_control_buffer_length_t buffer_length,
                                                fbe_object_id_t object_id,
                                                fbe_sg_element_t *sg_list,
                                                fbe_u32_t sg_list_count,
                                                fbe_packet_attr_t attr,
                                                fbe_package_id_t package_id)
{
   fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE, send_status;
   fbe_payload_ex_t *                  payload = NULL;
   fbe_payload_control_operation_t *   control_operation = NULL;
   fbe_payload_control_status_t        control_status;
   fbe_cpu_id_t  						cpu_id;         
   fbe_semaphore_t						sem;/* we use our own and our own completion function in case the packet is already using it's own semaphore*/
   fbe_packet_attr_t					original_attr;/*preserve the sender attribute*/

   fbe_semaphore_init(&sem, 0, 1);

   cpu_id = fbe_get_cpu_id();
   fbe_transport_set_cpu_id(packet, cpu_id);


   payload = fbe_transport_get_payload_ex(packet);
   control_operation = fbe_payload_ex_allocate_control_operation(payload); 

   fbe_payload_control_build_operation (control_operation,
                                         control_code,
                                         buffer,
                                         buffer_length);
    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              package_id,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id); 
    
    /* Mark packet with the attribute, either traverse or not */
	fbe_transport_get_packet_attr(packet, &original_attr);
    fbe_transport_clear_packet_attr(packet, original_attr);/*remove the old one, we'll restore it later*/
	fbe_transport_set_packet_attr(packet, attr);

	fbe_transport_set_completion_function(packet, fbe_physical_drive_perfstats_forward_packet_completion, (fbe_packet_completion_context_t) &sem);

    send_status = fbe_service_manager_send_control_packet(packet);
    if ((send_status != FBE_STATUS_OK)&&(send_status != FBE_STATUS_NO_OBJECT) && (send_status != FBE_STATUS_PENDING) && (send_status != FBE_STATUS_MORE_PROCESSING_REQUIRED)) {
        fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                 FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: error in sending 0x%x to obj 0x%x, status:0x%x\n",
                       __FUNCTION__, control_code, object_id, send_status);  
    }

    fbe_semaphore_wait(&sem, NULL);

    /* Save the status for returning.*/
    status = fbe_transport_get_status_code(packet);
    fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                             FBE_TRACE_LEVEL_DEBUG_HIGH,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: 0x%x to obj 0x%x, packet status:0x%x\n",
                   __FUNCTION__, control_code, object_id, status);  

    fbe_payload_control_get_status(control_operation, &control_status);
    if (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK ) {
        fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                 FBE_TRACE_LEVEL_DEBUG_HIGH,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: 0x%x to obj 0x%x, return payload status:0x%x\n",
                       __FUNCTION__, control_code, object_id, control_status);

        switch (control_status){
        case FBE_PAYLOAD_CONTROL_STATUS_BUSY:
            status = FBE_STATUS_BUSY;
            break;
        case FBE_PAYLOAD_CONTROL_STATUS_INSUFFICIENT_RESOURCES:
            status = FBE_STATUS_INSUFFICIENT_RESOURCES;
            break;
        case FBE_PAYLOAD_CONTROL_STATUS_FAILURE:
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
        }
        fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                 FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Set return status to 0x%x\n",
                       __FUNCTION__, status);  
    }

    fbe_transport_set_packet_attr(packet, original_attr);
    fbe_semaphore_destroy(&sem);
    fbe_payload_ex_release_control_operation(payload, control_operation);

    return status;
}

static fbe_status_t fbe_physical_drive_perfstats_forward_packet_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_semaphore_t * sem = (fbe_semaphore_t *)context;
	fbe_semaphore_release(sem, 0, 1, FBE_FALSE);
	return FBE_STATUS_MORE_PROCESSING_REQUIRED;/*important, otherwise we'll complete the original one*/
}

static fbe_status_t fbe_physical_drive_prefstats_set_disabled(fbe_packet_t * packet) 
{
   fbe_status_t status = FBE_STATUS_OK;
   fbe_u32_t                                   obj_i = 0;
   fbe_object_id_t                             *obj_list = NULL;
   fbe_u32_t                                   obj_count = 0;
   fbe_u32_t                                   actual_obj_count = 0;
   fbe_payload_ex_t *                                      pp_payload = NULL;
   fbe_payload_control_operation_t *                       control_operation = NULL; 
   fbe_class_id_t                               *filter_class_id;

     /* get the payload from packet */
   pp_payload = fbe_transport_get_payload_ex(packet);
   control_operation = fbe_payload_ex_get_control_operation(pp_payload);  

   fbe_payload_control_get_buffer(control_operation, &filter_class_id); 

   /* Get total objects*/
    status = fbe_physical_drive_prefstats_get_total_objects(FBE_PACKAGE_ID_PHYSICAL, &obj_count);
    if ( obj_count == 0 ){
        obj_list  = NULL;
        return status;
    }

    status = fbe_physical_drive_allocate_memory(sizeof(fbe_object_id_t) * obj_count, &obj_list);
    if (status != FBE_STATUS_OK) {
       fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: Unable to allocate memory for object list\n", __FUNCTION__);
    }

    status = fbe_physical_drive_enumerate_object(obj_list, obj_count, &actual_obj_count, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
       fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: Unable to obtain object list for package ID: 0x%x\n", __FUNCTION__, FBE_PACKAGE_ID_PHYSICAL);
       fbe_release_nonpaged_pool_with_tag(obj_list, 'ljbo');
	   fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
	   fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
	   fbe_transport_complete_packet(packet);
	   return FBE_STATUS_OK;
    }

    for(obj_i = 0; obj_i < obj_count; obj_i++) {
       fbe_object_id_t object_id = obj_list[obj_i];
       status = fbe_physical_drive_prefstats_enable_peformance_stats(packet, object_id, FBE_PACKAGE_ID_PHYSICAL, (*filter_class_id), FBE_FALSE);
        if (status != FBE_STATUS_OK) {
           fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: Unable to enable performance stats for package ID: 0x%x\n", __FUNCTION__, FBE_PACKAGE_ID_PHYSICAL);
        }
    }

    /* releasing obj_list. It was allocated in fbe_physical_drive_allocate_memory and is being used in this function. */
    fbe_release_nonpaged_pool_with_tag(obj_list, 'ljbo');
    
    /* set the packet status with completion and return it back to source */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;   

}
