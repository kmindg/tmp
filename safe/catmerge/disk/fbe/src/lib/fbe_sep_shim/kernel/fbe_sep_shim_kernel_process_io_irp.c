/***************************************************************************
* Copyright (C) EMC Corporation 2001-2009, 2014
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************
*  fbe_sep_shim_kernel_process_io_irp.c
***************************************************************************
*
*  Description
*      Kernel implementation for the SEP shim to process IO IRPs
*      
*
*  History:
*      06/24/09    sharel - Created
*    
***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_trace_interface.h"
#include "fbe/fbe_library_interface.h"
#include "fbe_service_manager.h"
#include "fbe_topology.h"
#include "fbe/fbe_queue.h"
#include "fbe_sep_shim_private_interface.h"
#include "fbe/fbe_sep.h"
#include "flare_export_ioctls.h"
#include "fbe/fbe_bvd_interface.h"
#include "fbe/fbe_memory.h"
#include "flare_ioctls.h"
#include "fbe/fbe_xor_api.h"
#include "fbe_sep_shim_private_ioctl.h"
#include "fbe_private_space_layout.h"
#include "ListsOfIRPs.h"

#define FBE_SEP_SHIM_RETRY_MSEC 500

/*!********************************************************************* 
 * @enum fbe_waiting_irps_thread_flag_t 
 *  
 * @brief 
 *  This contains the data info for waiting irps thread flag.
 *
 *********************************************************************/
typedef enum fbe_waiting_irps_thread_flag_t{
    FBE_WAITING_IRPS_THREAD_RUN,   /*!< Run Thread Flag */
    FBE_WAITING_IRPS_THREAD_STOP,  /*!< Stop Thread Flag */
    FBE_WAITING_IRPS_THREAD_DONE   /*!< Done Thread Flag */
}fbe_waiting_irps_thread_flag_t;

/**************************************
            Local variables
**************************************/
static fbe_bool_t sep_shim_async_io = FBE_TRUE;
static fbe_bool_t sep_shim_async_io_compl = FBE_FALSE;
static fbe_transport_rq_method_t sep_shim_rq_method = FBE_TRANSPORT_RQ_METHOD_SAME_CORE;
static fbe_u32_t sep_shim_alert_time = 0; /* If set the EMCPAL_STATUS_SUCCESS will be changed to EMCPAL_STATUS_ALERTED */
static fbe_atomic_t sep_shim_alert_count = 0; /* Overall number of allerted I/O's */


typedef struct fbe_waiting_irps_info_s{
    ListOfIRPs fbe_waiting_irps_queue;
    fbe_spinlock_t   fbe_waiting_irps_lock;
    fbe_thread_t     fbe_waiting_irps_thread_handle;
    fbe_semaphore_t  fbe_waiting_irps_thread_semaphore;
    fbe_semaphore_t  fbe_waiting_irps_event;
    fbe_waiting_irps_thread_flag_t fbe_waiting_irps_thread_flag;
    fbe_u32_t cpu_id;
}fbe_waiting_irps_info_t;

static fbe_waiting_irps_info_t fbe_waiting_irps_info[FBE_CPU_ID_MAX];

/*******************************************
            Local functions
********************************************/
static fbe_status_t fbe_sep_shim_process_sgl_read_write_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_sep_shim_process_dca_read_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_sep_shim_process_dca_write_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_sep_shim_process_dca_arg_read_completion(EMCPAL_STATUS nt_status, fbe_sep_shim_io_struct_t * sep_shim_io_struct);
fbe_status_t fbe_sep_shim_translate_status(fbe_packet_t *packet, fbe_payload_block_operation_t * block_operation, PEMCPAL_IRP PIrp, fbe_status_t packet_status, fbe_bool_t *io_failed);

static fbe_status_t fbe_sep_shim_process_zero_fill_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t
fbe_sep_shim_process_asynch_io_irp_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context);

static fbe_status_t
fbe_sep_shim_process_asynch_read_write_irp_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context);

EMCPAL_STATUS fbe_sep_shim_process_mj_write(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP	PIrp, fbe_sep_shim_io_struct_t *sep_shim_io_struct);
static fbe_status_t fbe_sep_shim_process_mj_write_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
EMCPAL_STATUS fbe_sep_shim_process_mj_read(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP	PIrp, fbe_sep_shim_io_struct_t *sep_shim_io_struct);
static fbe_status_t fbe_sep_shim_process_mj_read_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_sep_shim_process_mj_read_async_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_sep_shim_process_mj_write_async_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_sep_shim_send_io (fbe_packet_t *packet);
static fbe_status_t fbe_sep_shim_retry_io(void * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_sep_shim_continue_completion (fbe_packet_t * packet_p, fbe_sep_shim_io_struct_t * io_struct, fbe_bool_t io_failed);
static fbe_status_t fbe_sep_shim_send_finish_completion(void * packet_p, fbe_packet_completion_context_t context);

static fbe_status_t fbe_sep_shim_convert_irp_flags(fbe_payload_block_operation_t * block_operation_p, fbe_irp_stack_flags_t irp_Flags);
static fbe_status_t fbe_sep_shim_convert_sgl_info_flags(fbe_payload_block_operation_t * block_operation_p, UINT_16 sgl_info_flags);

static fbe_status_t fbe_sep_shim_convert_irp_flags(fbe_payload_block_operation_t * block_operation_p, fbe_irp_stack_flags_t irp_Flags);
static fbe_status_t fbe_sep_shim_continue_io_asynch(void * packet_p, fbe_packet_completion_context_t context);

static fbe_status_t fbe_sep_shim_process_sgl_read_async_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_sep_shim_process_sgl_write_async_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_sep_shim_complete_io_async(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
fbe_packet_priority_t KeyToPacketPriority(fbe_u64_t Key);
static fbe_status_t fbe_sep_shim_process_dca_op_async_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_sep_shim_process_dca_arg_write_completion(EMCPAL_STATUS nt_status, fbe_sep_shim_io_struct_t * sep_shim_io_struct);
static fbe_status_t fbe_sep_shim_setup_dca_buffer(fbe_sep_shim_io_struct_t * sep_shim_io_struct, fbe_memory_request_t * memory_request);
static void fbe_waiting_irps_thread_func(void * context);
static void  fbe_waiting_irps_dispatch_queue(fbe_u32_t cpu_id);
static EMCPAL_STATUS fbe_sep_shim_start_asynch_io_irp( fbe_sep_shim_io_struct_t *  sep_shim_io_struct, 
                                                               PEMCPAL_DEVICE_OBJECT  PDeviceObject,
                                                               PEMCPAL_IRP	PIrp);
static EMCPAL_STATUS fbe_sep_shim_start_asynch_read_write_irp( fbe_sep_shim_io_struct_t *  sep_shim_io_struct, 
                                                               PEMCPAL_DEVICE_OBJECT  PDeviceObject,
                                                               PEMCPAL_IRP	PIrp);
static fbe_status_t
fbe_sep_shim_process_asynch_dca_op_irp_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context);
static fbe_status_t fbe_sep_shim_process_asynch_dca_read(fbe_sep_shim_io_struct_t *	sep_shim_io_struct);
static fbe_status_t fbe_sep_shim_process_asynch_dca_write(fbe_sep_shim_io_struct_t *	sep_shim_io_struct);



/***************************************************************************************************/

/*********************************************************************
*            fbe_sep_shim_process_sgl_read ()
*********************************************************************
*
*  Description: This is the READ IO from cache
*
*  Inputs: PDeviceObject  - pointer to the device object
*          PIRP - IRP from top driver
*
*  Return Value: 
*   success or failure
*    
*********************************************************************/
EMCPAL_STATUS fbe_sep_shim_process_sgl_read(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP	PIrp, fbe_sep_shim_io_struct_t *sep_shim_io_struct)
{
    EMCPAL_STATUS                       status = EMCPAL_STATUS_SUCCESS;
    PEMCPAL_IO_STACK_LOCATION           pIoStackLocation = NULL;
    fbe_status_t                        fbe_status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet_p = NULL;
    fbe_payload_ex_t *                  sep_payload_p = NULL;
    fbe_payload_block_operation_t *     block_operation_p = NULL;
    fbe_sg_element_t *                  sg_p = NULL;
    fbe_lba_t                           lba = 0;
    fbe_block_count_t                   blocks = 0;
    fbe_packet_priority_t               priority;
    os_device_info_t *                  dev_info_p = (os_device_info_t *)EmcpalDeviceGetExtension(PDeviceObject);
    fbe_block_size_t                    exp_block_size = 0, imp_block_size = 0;
    PEMCPAL_IRP_CANCEL_FUNCTION         old_cancel_routine = NULL;
    FLARE_SGL_INFO *					cache_sgl_info;

    pIoStackLocation = EmcpalIrpGetCurrentStackLocation(PIrp);

    lba = EmcpalIrpParamGetReadByteOffset(pIoStackLocation)/(LONGLONG)FBE_BYTES_PER_BLOCK ;
    blocks = EmcpalIrpParamGetReadLength(pIoStackLocation)/ FBE_BYTES_PER_BLOCK ;
    priority = KeyToPacketPriority((fbe_u64_t)EmcpalIrpParamGetReadKey(pIoStackLocation));

    packet_p = sep_shim_io_struct->packet;
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_allocate_block_operation(sep_payload_p);
    if (block_operation_p == NULL) {
        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }

    fbe_block_transport_get_exported_block_size(dev_info_p->block_edge.block_edge_geometry, &exp_block_size);
    fbe_block_transport_get_physical_block_size(dev_info_p->block_edge.block_edge_geometry, &imp_block_size);

    fbe_status = fbe_payload_block_build_operation(block_operation_p,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                                   lba,
                                                   blocks,
                                                   exp_block_size,
                                                   imp_block_size,
                                                   NULL/*HACK for now, we don't need it*/);

    /* Based on flags in the IRP, set the corresponding flags in the block operation payload.
     */
    fbe_sep_shim_convert_irp_flags(block_operation_p, EmcpalIrpStackGetFlags(pIoStackLocation));

    /* Set the sg ptr into the packet.*/
    cache_sgl_info = (FLARE_SGL_INFO *)EmcpalExtIrpStackParamArg4Get(pIoStackLocation);/*based on spec, this is where sg list is*/

    /* Based on sgl info flags, set the corresponding flags in the block operation payload.
     */
    fbe_sep_shim_convert_sgl_info_flags(block_operation_p, cache_sgl_info->Flags);
    
    sg_p = (fbe_sg_element_t *)cache_sgl_info->SGList;
    status = fbe_payload_ex_set_sg_list(sep_payload_p, sg_p, 0);


    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              dev_info_p->bvd_object_id); 

    fbe_transport_set_packet_priority(packet_p, priority);

    /*make sure the packet has our edge*/
    packet_p->base_edge = &dev_info_p->block_edge.base_edge;

    //Store context for cancellation
    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = (PVOID)packet_p;    
    
    old_cancel_routine = EmcpalIrpCancelRoutineSet(PIrp, sep_handle_cancel_irp_function);
    if (EmcpalIrpIsCancelInProgress(PIrp)){
      old_cancel_routine = EmcpalIrpCancelRoutineSet(PIrp, NULL);
      EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_CANCELLED); 
      //clean up context for cancellation
      EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
      EmcpalExtIrpInformationFieldSet(PIrp, 0);
      EmcpalIrpCompleteRequest(PIrp);
      fbe_sep_shim_return_io_structure(sep_shim_io_struct);
      return EMCPAL_STATUS_CANCELLED;

    }

    fbe_transport_set_completion_function(packet_p, 
                                          fbe_sep_shim_process_sgl_read_write_completion, 
                                          sep_shim_io_struct);

    fbe_sep_shim_send_io(packet_p);

    return EMCPAL_STATUS_PENDING;
}

/*********************************************************************
*            fbe_sep_shim_process_sgl_write ()
*********************************************************************
*
*  Description: This is the WRITE IO from cache
*
*  Inputs: PDeviceObject  - pointer to the device object
*          PIRP - IRP from top driver
*
*  Return Value: 
*    success or failure
*    
*********************************************************************/
EMCPAL_STATUS fbe_sep_shim_process_sgl_write(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP	PIrp, fbe_sep_shim_io_struct_t *sep_shim_io_struct)
{

    EMCPAL_STATUS                       status = EMCPAL_STATUS_SUCCESS;
    PEMCPAL_IO_STACK_LOCATION           pIoStackLocation = NULL;
    fbe_status_t                        fbe_status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet_p = NULL;
    fbe_payload_ex_t *                  sep_payload_p = NULL;
    fbe_payload_block_operation_t *     block_operation_p = NULL;
    fbe_sg_element_t *                  sg_p = NULL;
    fbe_lba_t                           lba = 0;
    fbe_block_count_t                   blocks = 0;
    fbe_packet_priority_t               priority;
    os_device_info_t *                  dev_info_p = (os_device_info_t *)EmcpalDeviceGetExtension(PDeviceObject);
    fbe_block_size_t                    exp_block_size = 0, imp_block_size = 0;
    PEMCPAL_IRP_CANCEL_FUNCTION         old_cancel_routine = NULL;
    FLARE_SGL_INFO *					cache_sgl_info;

    pIoStackLocation = EmcpalIrpGetCurrentStackLocation(PIrp);

    lba = EmcpalIrpParamGetReadByteOffset(pIoStackLocation)/(LONGLONG)FBE_BYTES_PER_BLOCK;
    blocks = EmcpalIrpParamGetReadLength(pIoStackLocation)/ FBE_BYTES_PER_BLOCK;
    priority = KeyToPacketPriority((fbe_u64_t)EmcpalIrpParamGetReadKey(pIoStackLocation));

    packet_p = sep_shim_io_struct->packet;
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_allocate_block_operation(sep_payload_p);
    if (block_operation_p == NULL) {
        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }

    fbe_block_transport_get_exported_block_size(dev_info_p->block_edge.block_edge_geometry, &exp_block_size);
    fbe_block_transport_get_physical_block_size(dev_info_p->block_edge.block_edge_geometry, &imp_block_size);

    fbe_status = fbe_payload_block_build_operation(block_operation_p,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                                   lba,
                                                   blocks,
                                                   exp_block_size,
                                                   imp_block_size,
                                                   NULL/*HACK for now, we don't need it*/);

    /* Based on flags in the IRP, set the corresponding flags in the block operation payload.
     */
    fbe_sep_shim_convert_irp_flags(block_operation_p, EmcpalIrpStackGetFlags(pIoStackLocation));

    /* Set the sg ptr into the packet.*/
    cache_sgl_info = (FLARE_SGL_INFO *)EmcpalExtIrpStackParamArg4Get(pIoStackLocation);/*based on spec, this is where sg list is*/

    /* Based on sgl info flags, set the corresponding flags in the block operation payload.
     */    
    fbe_sep_shim_convert_sgl_info_flags(block_operation_p, cache_sgl_info->Flags);
        
    sg_p = (fbe_sg_element_t *)cache_sgl_info->SGList;
    status = fbe_payload_ex_set_sg_list(sep_payload_p, sg_p, 0);

    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              dev_info_p->bvd_object_id); 

    fbe_transport_set_packet_priority(packet_p, priority);

    /*make sure the packet has our edge*/
    packet_p->base_edge = &dev_info_p->block_edge.base_edge;

    //Store context for cancellation
    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = (PVOID)packet_p;    
    
    old_cancel_routine = EmcpalIrpCancelRoutineSet(PIrp, sep_handle_cancel_irp_function);
    if (EmcpalIrpIsCancelInProgress(PIrp)){
      old_cancel_routine = EmcpalIrpCancelRoutineSet(PIrp, NULL);
      EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_CANCELLED); 
      //clean up context for cancellation
      EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
      EmcpalExtIrpInformationFieldSet(PIrp, 0);
      EmcpalIrpCompleteRequest(PIrp);
      fbe_sep_shim_return_io_structure(sep_shim_io_struct);
      return EMCPAL_STATUS_CANCELLED;

    }

    fbe_transport_set_completion_function(packet_p, 
                                          fbe_sep_shim_process_sgl_read_write_completion, 
                                          sep_shim_io_struct);

    fbe_sep_shim_send_io(packet_p);

    return EMCPAL_STATUS_PENDING;
}

static fbe_status_t fbe_sep_shim_process_sgl_read_write_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sep_shim_io_struct_t *				sep_shim_io_struct = (fbe_sep_shim_io_struct_t *)context;
    PEMCPAL_IRP                             PIrp = sep_shim_io_struct->associated_io;
    fbe_payload_ex_t *                     	payload = NULL;
    fbe_payload_block_operation_t *         block_operation = NULL;
    fbe_status_t							fbe_status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bool_t								io_failed;

    payload = fbe_transport_get_payload_ex (packet);
    block_operation = fbe_payload_ex_get_block_operation(payload);
    fbe_status = fbe_transport_get_status_code(packet);
    /* Translate the fbe status to an IRP status.
     */
    fbe_sep_shim_translate_status(packet, block_operation, PIrp, fbe_status, &io_failed);

    fbe_payload_ex_release_block_operation(payload, block_operation);/*don't need it anymore*/

    /*we might be completing here at DPC level so we have to break the context before returning the IRP,
    some layered apps might not support running at this level*/
    return fbe_sep_shim_continue_completion(packet, sep_shim_io_struct, io_failed);
}

/*********************************************************************
*            fbe_sep_shim_process_zero_fill ()
*********************************************************************
*
*  Description: This handles the Zero fill IOCTL from upper
*  drives
*
*  Inputs: PDeviceObject  - pointer to the device object
*          PIRP - IRP from top driver
*
*  Return Value: 
*    success or failure
*    
*********************************************************************/
EMCPAL_STATUS fbe_sep_shim_process_zero_fill(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP	PIrp, fbe_sep_shim_io_struct_t *sep_shim_io_struct)
{

    fbe_status_t                        fbe_status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet_p = NULL;
    fbe_payload_ex_t *                 sep_payload_p = NULL;
    fbe_payload_block_operation_t *     block_operation_p = NULL;
    fbe_lba_t                           lba = 0;
    fbe_block_count_t                   blocks = 0;
    os_device_info_t *                  dev_info_p = (os_device_info_t *)EmcpalDeviceGetExtension(PDeviceObject);
    fbe_block_size_t                    exp_block_size = 0, imp_block_size = 0;
    PFLARE_ZERO_FILL_INFO               zero_fill_ptr;
    PEMCPAL_IRP_CANCEL_FUNCTION         old_cancel_routine = NULL;
    
    if(PIrp == NULL || EmcpalExtIrpSystemBufferGet(PIrp) == NULL) {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: Null IRP or System Buffer\n",
                        __FUNCTION__);

        if (PIrp != NULL) {
            //clean up context for cancellation
            EmcpalIrpCancelRoutineSet (PIrp, NULL);
            EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
            EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
            EmcpalExtIrpInformationFieldSet(PIrp, 0);
            EmcpalIrpCompleteRequest(PIrp);
        }

        fbe_sep_shim_return_io_structure(sep_shim_io_struct);

        return EMCPAL_STATUS_UNSUCCESSFUL;
    }
    

    zero_fill_ptr = (PFLARE_ZERO_FILL_INFO) EmcpalExtIrpSystemBufferGet(PIrp);
    lba = zero_fill_ptr->StartLba;
    blocks = (zero_fill_ptr->Blocks == (ULONG)zero_fill_ptr->Blocks)?(ULONG)zero_fill_ptr->Blocks:0xfffffff0;
    
    packet_p = sep_shim_io_struct->packet;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_allocate_block_operation(sep_payload_p);
    if (block_operation_p == NULL) {
        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }

    fbe_block_transport_get_exported_block_size(dev_info_p->block_edge.block_edge_geometry, &exp_block_size);
    fbe_block_transport_get_physical_block_size(dev_info_p->block_edge.block_edge_geometry, &imp_block_size);

    fbe_status = fbe_payload_block_build_operation(block_operation_p,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO,
                                                   lba,
                                                   blocks,
                                                   exp_block_size,
                                                   imp_block_size,
                                                   NULL/*HACK for now, we don't need it*/);

    fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "Shim Zero fill: lba 0x%llx, blocks %llu, exp block size %d, imp block size %d\n",
                       (unsigned long long)lba, (unsigned long long)blocks,
               exp_block_size, imp_block_size);

    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              dev_info_p->bvd_object_id); 

    /*make sure the packet has our edge*/
    packet_p->base_edge = &dev_info_p->block_edge.base_edge;

    //Store context for cancellation
    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = (PVOID)packet_p;    
    
    old_cancel_routine = EmcpalIrpCancelRoutineSet(PIrp, sep_handle_cancel_irp_function);
    if (EmcpalIrpIsCancelInProgress(PIrp)){
      old_cancel_routine = EmcpalIrpCancelRoutineSet(PIrp, NULL);
      EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_CANCELLED); 
      //clean up context for cancellation
      EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
      EmcpalExtIrpInformationFieldSet(PIrp, 0);
      EmcpalIrpCompleteRequest(PIrp);
      fbe_sep_shim_return_io_structure(sep_shim_io_struct);
      return EMCPAL_STATUS_CANCELLED;
    }

    fbe_transport_set_completion_function(packet_p, 
                                          fbe_sep_shim_process_zero_fill_completion, 
                                          sep_shim_io_struct);

    //fbe_topology_send_io_packet(packet_p);
    fbe_sep_shim_send_io(packet_p);

    return EMCPAL_STATUS_PENDING;
}

static fbe_status_t fbe_sep_shim_process_zero_fill_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sep_shim_io_struct_t *				sep_shim_io_struct = (fbe_sep_shim_io_struct_t *)context;
    PEMCPAL_IRP                             PIrp = sep_shim_io_struct->associated_io;
    fbe_payload_ex_t *                      payload = NULL;
    fbe_payload_block_operation_t *         block_operation = NULL;
    fbe_status_t                            packet_status = fbe_transport_get_status_code(packet);
    PFLARE_ZERO_FILL_INFO                   zero_fill_ptr;
    fbe_bool_t								io_failed;

    payload = fbe_transport_get_payload_ex (packet);
    block_operation = fbe_payload_ex_get_block_operation(payload);
    
    zero_fill_ptr = (PFLARE_ZERO_FILL_INFO) EmcpalExtIrpSystemBufferGet(PIrp);

    /**** 
     * PFLARE_ZERO_FILL_INFO structure has the output that indicates
     * number of blocks that has been zeroed. Where do I get that
     * information from. Is it block_operation->block_count?
     * 
     */
     if (packet_status == FBE_STATUS_OK)
     {
         zero_fill_ptr->BlocksZeroed = block_operation->block_count;
     }
     else
     {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, 
                           "%s:failed IO status, PIrp:0x%llX status:0x%X, lba:0x%llX\n",
                           __FUNCTION__, (unsigned long long)PIrp, packet_status, (unsigned long long)block_operation->lba);
        zero_fill_ptr->BlocksZeroed = 0;
     }


    /* Translate the fbe status to an IRP status.
     */
    fbe_sep_shim_translate_status(packet, block_operation, PIrp, packet_status, &io_failed);

    /*we have to overside the regular way of sending information because Lower redirector asserts for a a certain size*/
    EmcpalExtIrpInformationFieldSet(PIrp, (ULONG_PTR)(sizeof(FLARE_ZERO_FILL_INFO)));
    
    fbe_payload_ex_release_block_operation(payload, block_operation);/*don't need it anymore*/

    /*we might be completing here at DPC level so we have to break the context before returning the IRP,
    some layered apps might not support running at this level*/
    return fbe_sep_shim_continue_completion(packet, sep_shim_io_struct, io_failed);
}
fbe_status_t fbe_sep_shim_display_irp(PEMCPAL_IRP PIrp, fbe_trace_level_t trace_level)
{
    PEMCPAL_IO_STACK_LOCATION     pIoStackLocation = NULL;
    UCHAR                         minorFunction = 0;
    UCHAR                         majorFunction = 0;
    ULONG                         IoControlCode = 0;
    fbe_u64_t byte_count;
    fbe_block_count_t blocks;
    fbe_u64_t byte_offset;
    fbe_lba_t lba;
    PFLARE_ZERO_FILL_INFO zero_fill_ptr = NULL;
    fbe_irp_stack_flags_t irp_flags;
    FLARE_SGL_INFO *sgl_info = NULL;
    fbe_u64_t total_bytes;
    fbe_u64_t total_sgs;
    void* dca_arg = NULL;

    pIoStackLocation = EmcpalIrpGetCurrentStackLocation(PIrp);
    irp_flags = EmcpalIrpStackGetFlags(pIoStackLocation);
    majorFunction = EmcpalExtIrpStackMajorFunctionGet(pIoStackLocation);
    minorFunction = EmcpalExtIrpStackMinorFunctionGet(pIoStackLocation);

    switch (majorFunction)
    {
        case EMCPAL_IRP_TYPE_CODE_READ:
        case EMCPAL_IRP_TYPE_CODE_WRITE:
            byte_offset = EmcpalIrpParamGetWriteByteOffset(pIoStackLocation);
            lba = byte_offset / FBE_BYTES_PER_BLOCK;
            byte_count = EmcpalIrpParamGetWriteLength(pIoStackLocation);
            blocks = byte_count / FBE_BYTES_PER_BLOCK;
            fbe_sep_shim_trace(trace_level, "irp: %p MJ %s lba: 0x%llx blocks: 0x%llx\n", 
                               PIrp, (majorFunction == EMCPAL_IRP_TYPE_CODE_READ) ? "read" : "write", lba, blocks);
            fbe_sep_shim_trace(trace_level, "irp: %p byte offset: 0x%llx byte count: 0x%llx\n", 
                               PIrp, byte_offset, byte_count);
            fbe_sep_shim_trace(trace_level, "irp: %p irp_flags: 0x%x\n", 
                               PIrp, irp_flags);
            break;

        case EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL:

            IoControlCode = EmcpalExtIrpStackParamIoctlCodeGet(pIoStackLocation);
            switch (IoControlCode) {
                case IOCTL_FLARE_ZERO_FILL:
                    zero_fill_ptr = (PFLARE_ZERO_FILL_INFO) EmcpalExtIrpSystemBufferGet(PIrp);
                    lba = zero_fill_ptr->StartLba;
                    blocks = zero_fill_ptr->Blocks;
                    fbe_sep_shim_trace(trace_level, "irp: %p zero fill lba: 0x%llx blocks: 0x%llx\n", 
                                       PIrp, lba, blocks);
                    break;
                default:
                    fbe_sep_shim_trace(FBE_TRACE_LEVEL_WARNING, "%s:unknown IoControlCode: 0x%X\n",__FUNCTION__, IoControlCode);
                    break;
            };
            break;

        case EMCPAL_IRP_TYPE_CODE_INTERNAL_DEVICE_CONTROL:
            byte_offset = EmcpalIrpParamGetWriteByteOffset(pIoStackLocation);
            lba = byte_offset / FBE_BYTES_PER_BLOCK;
            byte_count = EmcpalIrpParamGetWriteLength(pIoStackLocation);
            blocks = byte_count / FBE_BYTES_PER_BLOCK;
            switch (minorFunction) 
            {
                case FLARE_SGL_READ:
                case FLARE_SGL_WRITE:
                    sgl_info = (FLARE_SGL_INFO *)EmcpalExtIrpStackParamArg4Get(pIoStackLocation);
                    total_sgs = fbe_sg_element_count_entries_and_bytes((fbe_sg_element_t*)sgl_info->SGList,
                                                                       byte_count,
                                                                       &total_bytes);

                    fbe_sep_shim_trace(trace_level, "irp: %p sgl %s lba: 0x%llx blocks: 0x%llx\n", 
                                       PIrp, (minorFunction == FLARE_SGL_READ) ? "read" : "write", lba, blocks);
                    fbe_sep_shim_trace(trace_level, "irp: %p byte offset: 0x%llx byte count: 0x%llx\n", 
                                       PIrp, byte_offset, byte_count);
                    fbe_sep_shim_trace(trace_level, "irp: %p irp_flags: 0x%x\n", 
                                       PIrp, irp_flags);
                    fbe_sep_shim_trace(trace_level, "irp: %p sgl: %p flags: 0x%x SglType: %d FromPeer: %d\n", 
                                       PIrp, sgl_info->SGList, sgl_info->Flags, sgl_info->SglType, sgl_info->FromPeer);
                    fbe_sep_shim_trace(trace_level, "irp: %p sg_bytes: 0x%llx sg_elements: %lld\n", 
                                       PIrp, total_bytes, total_sgs);
                    break;
                    
                case FLARE_DCA_READ:
                case FLARE_DCA_WRITE:
                    dca_arg = EmcpalExtIrpStackParamArg4Get(pIoStackLocation);
                    fbe_sep_shim_trace(trace_level, "irp: %p DCA %s lba: 0x%llx blocks: 0x%llx\n", 
                                       PIrp, (minorFunction == FLARE_DCA_READ) ? "read" : "write", lba, blocks);
                    fbe_sep_shim_trace(trace_level, "irp: %p byte offset: 0x%llx byte count: 0x%llx\n", 
                                       PIrp, byte_offset, byte_count);
                    fbe_sep_shim_trace(trace_level, "irp: %p irp_flags: 0x%x dca_arg: %p\n", 
                                       PIrp, irp_flags, dca_arg);
                    break;
                default: 
                    fbe_sep_shim_trace(FBE_TRACE_LEVEL_WARNING, "%s: unknown minorFunction: 0x%X\n",__FUNCTION__, minorFunction);
                    break;
            };
            break;
        default: 
            fbe_sep_shim_trace(FBE_TRACE_LEVEL_WARNING, "%s: unknown majorFunction: 0x%X\n",__FUNCTION__, majorFunction);
            break;
    };
    return FBE_STATUS_OK;
}
fbe_status_t fbe_sep_shim_translate_status(fbe_packet_t *packet,
                                           fbe_payload_block_operation_t * block_operation,
                                           PEMCPAL_IRP PIrp, fbe_status_t packet_status,
                                           fbe_bool_t *io_failed)
{
    fbe_payload_block_operation_status_t    block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    fbe_payload_block_operation_qualifier_t block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;
    FLARE_SGL_INFO                          *cache_sgl_info = NULL;
    PEMCPAL_IO_STACK_LOCATION               pIoStackLocation = NULL;
    UCHAR                                   minorFunction = 0;
    UCHAR                                   majorFunction = 0;
    LONGLONG                                blockSize = 0;
    fbe_payload_ex_t                       *payload = NULL;
    fbe_sg_element_t                       *sg_list = NULL;

    pIoStackLocation = EmcpalIrpGetCurrentStackLocation(PIrp);
    minorFunction = EmcpalExtIrpStackMinorFunctionGet(pIoStackLocation);
    majorFunction = EmcpalExtIrpStackMajorFunctionGet(pIoStackLocation);
    *io_failed = FBE_FALSE;/*we are very optimistic*/

    /*let's look at the packet status first*/
    switch (packet_status) {
    case FBE_STATUS_OK:
        break;/*nothing to do, go on*/
    case FBE_STATUS_CANCELED:
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "%s:PIrp:0x%llX FBE_STATUS_CANCELED\n",__FUNCTION__, (unsigned long long)PIrp);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_CANCELLED);
        return FBE_STATUS_OK;
    case FBE_STATUS_BUSY:
    case FBE_STATUS_EDGE_NOT_ENABLED:
    case FBE_STATUS_TIMEOUT:
        /*we should get that when RAID is draining and going to activate. This can happen in a corner case when metadata was lost
        and RG is re-writing it and going to activae together with the LUN, in this case we will let cache retry it*/
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "%s:PIrp:0x%llX,lba:0x%llx,status:%d\n",
                           __FUNCTION__, (unsigned long long)PIrp, (unsigned long long)block_operation->lba, packet_status);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_DEVICE_NOT_READY);/*cache will retry*/
        *io_failed = FBE_TRUE;
        return FBE_STATUS_OK;
    case FBE_STATUS_FAILED:
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s:PIrp:0x%llX, lba:0x%llx status:FBE_STATUS_FAILED\n",__FUNCTION__, (unsigned long long)PIrp, (unsigned long long)block_operation->lba);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_DEVICE_DOES_NOT_EXIST);
        *io_failed = FBE_TRUE;
        return FBE_STATUS_OK; 
    default:
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s:unhandled IO status, PIrp:0x%llX status:0x%llx, lba:0x%llx\n",__FUNCTION__, (unsigned long long)PIrp, (unsigned long long)packet_status, (unsigned long long)block_operation->lba);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_DEVICE_DOES_NOT_EXIST);
        *io_failed = FBE_TRUE;
        return FBE_STATUS_OK;
    }
    /* Fetch the block status and qualifier of the block operation.
     */
    fbe_payload_block_get_status(block_operation, &block_status);
    fbe_payload_block_get_qualifier(block_operation, &block_qualifier);

    /* For read write IRPs use block size 520 for SGL 512.
     */
    if((EMCPAL_IRP_TYPE_CODE_READ == majorFunction) ||
       (EMCPAL_IRP_TYPE_CODE_WRITE == majorFunction))
    {
        blockSize = block_operation->block_size;
    }
    else
    {
        blockSize = FBE_BYTES_PER_BLOCK;
    }

    /* Handle each FBE status appropriately, setting the corresponding Status 
     * in the IRP.
     */
    switch (block_status)
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS:
            if (block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_STILL_CONGESTED)
            {
                /* Set the Number of bytes transferred.
                 */
                EmcpalExtIrpInformationFieldSet(PIrp, (ULONG_PTR)(block_operation->block_count * blockSize));
                EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_ALERTED);
            }
            else
            {
                /* Set the Number of bytes transferred.
                 */
                EmcpalExtIrpInformationFieldSet(PIrp, (ULONG_PTR)(block_operation->block_count * blockSize));
                EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
            }
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR:
            switch(minorFunction)
            {
                case FLARE_SGL_READ:
                    /* Special case where some clients (Fct) need to know that there are one or more
                     * invalidated blocks in the request. If the SGL_FLAG_REPORT_VALIDATION_ERROR is set 
                     * then we must fail the request. 
                     */
                    cache_sgl_info = (FLARE_SGL_INFO *)EmcpalExtIrpStackParamArg4Get(pIoStackLocation);/*based on spec, this is where sg list is*/
                    if ((cache_sgl_info->Flags & SGL_FLAG_REPORT_VALIDATION_ERROR) == 0)
                    {
                        /* We got a media error but we have to send a good status on a READ
                         * to upper layers even though we have uncorrectables containing 
                         * invalidated blocks...
                         */
                        EmcpalExtIrpInformationFieldSet(PIrp, (ULONG_PTR)(block_operation->block_count * blockSize));
                        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
                        break;
                    }
                    // Fall-thru
        
                default:
                    /* Set the Number of bytes transferred even though there was an error. This is because
                     * media errors still transfer as much data as possible. Locations that were uncorrectable
                     * contain invalidated blocks.
                     */
                    fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "%s:PIrp:0x%llX, lba:0x%llx status:MEDIA_ERROR\n",__FUNCTION__, (unsigned long long)PIrp, (unsigned long long)block_operation->lba);
                    EmcpalExtIrpInformationFieldSet(PIrp, (ULONG_PTR)(block_operation->block_count * blockSize));
                    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_DISK_CORRUPT_ERROR);
                    *io_failed = FBE_TRUE;
                    break;
            }
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED:

            switch(block_qualifier)
            {
                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CRC_ERROR:
                    /* The data provided contained a CRC error. No data was transfered.
                     */
                    fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "%s:PIrp:0x%llX, lba:0x%llx status:CRC_ERROR\n",__FUNCTION__, (unsigned long long)PIrp, (unsigned long long)block_operation->lba);                     
                    EmcpalExtIrpInformationFieldSet(PIrp, 0);
                    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_CRC_ERROR);
                    *io_failed = FBE_TRUE;
                    break;

                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE:
                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE:
                    fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "%s:PIrp:0x%llX, lba:0x%llx status:NOT_READY Retry: %s\n",
                                       __FUNCTION__, (unsigned long long)PIrp, (unsigned long long)block_operation->lba,
                                       (FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE == block_qualifier)? "Yes" : "NO");
                    EmcpalExtIrpInformationFieldSet(PIrp, 0);
                    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_DEVICE_NOT_READY);/*cache will retry or we will if it's a PSL LUN*/
                    *io_failed = FBE_TRUE;
                    break;

                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CONGESTED:
                    EmcpalExtIrpInformationFieldSet(PIrp, 0);
                    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_QUOTA_EXCEEDED);
                    /* We do not consider this failed.  We will allow it to bubble back to upper levels even for
                       private LUNs. */
                    break;

                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NOT_PREFERRED:
                    EmcpalExtIrpInformationFieldSet(PIrp, 0);
                    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_LOCAL_DISCONNECT);
                    break;

                default:
                    /* Set the Number of bytes transferred to zero on error.
                    */
                    fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s:unhandled qualifier, PIrp:0x%llX, qualifier:0x%x\n",__FUNCTION__, (unsigned long long)PIrp, (unsigned int)block_qualifier);
                    EmcpalExtIrpInformationFieldSet(PIrp, 0);
                    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_DEVICE_NOT_READY);/*cache will retry*/
                    *io_failed = FBE_TRUE;
                    break;
            }
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED:
            /* Set the Number of bytes transferred.
             */
            EmcpalExtIrpInformationFieldSet(PIrp, 0);
            EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_CANCELLED);
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID:
            fbe_sep_shim_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                               "%s:Unexpected block status, PIrp:0x%llX, status:0x%x\n",
                               __FUNCTION__, (unsigned long long)PIrp, (unsigned int)block_status);
            *io_failed = FBE_TRUE;
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST:
            /* We may get invalid_request status on these occasions: 
             * - When an IO request block count exceeded the capacity.
             */
            payload = fbe_transport_get_payload_ex(packet);
            fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
            fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, 
                               "%s:invalid req block status, irp: %p\n",
                               __FUNCTION__, PIrp);
            fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, 
                               "irp: %p packet: %p sg_list: %p\n",
                               PIrp, packet, sg_list);
            fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, 
                               "irp: %p Block count: 0x%llx size:0x%x lba:0x%llx, status:%d qual: %d\n",
                               PIrp, block_operation->block_count, block_operation->block_size,
                               block_operation->lba, block_status, block_qualifier);
            fbe_sep_shim_display_irp(PIrp, FBE_TRACE_LEVEL_ERROR);   
            EmcpalExtIrpInformationFieldSet(PIrp, 0);
            EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_REQUEST_NOT_ACCEPTED); /* Let the sender handle it. */
            *io_failed = FBE_TRUE;
            break;

        default:
            /* Set the Number of bytes transferred to zero on error.
             */
            fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, 
                               "%s:Unhandled block status, PIrp:0x%llX, packet: %p status: %d\n",
                               __FUNCTION__, (unsigned long long)PIrp, packet, block_status);
            EmcpalExtIrpInformationFieldSet(PIrp, 0);
            EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_DEVICE_NOT_READY);/*cache will retry*/
            *io_failed = FBE_TRUE;
            break;
    }
    return FBE_STATUS_OK;
}


EMCPAL_STATUS fbe_sep_shim_process_asynch_io_irp(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP	PIrp)
{
    /*for all the asynch IOs, we first allocate memory and then we continue processing*/

    fbe_sep_shim_io_struct_t *	sep_shim_io_struct = NULL;
    EMCPAL_STATUS				status;
    fbe_status_t                fbe_status;
    
    fbe_status = fbe_sep_shim_get_io_structure(PIrp, &sep_shim_io_struct);
    if (fbe_status == FBE_STATUS_CANCELED)
    {
        return EMCPAL_STATUS_CANCELLED;
    }

    if (sep_shim_io_struct == NULL) {
        EmcpalIrpMarkPending(PIrp);
        return EMCPAL_STATUS_PENDING;
    }

    /* Now that we got memory, start the IO */
    status = fbe_sep_shim_start_asynch_io_irp(sep_shim_io_struct, PDeviceObject, PIrp);
    return status;

}
static EMCPAL_STATUS fbe_sep_shim_start_asynch_io_irp(fbe_sep_shim_io_struct_t *sep_shim_io_struct, 
                                                      PEMCPAL_DEVICE_OBJECT  PDeviceObject, PEMCPAL_IRP	PIrp)
{
    fbe_status_t status;
    
    /*! make sure we know which IO caused us to generate this structure 
     *
     *  @todo  Need to generate the memory io_stamp from the IRP
     */
    sep_shim_io_struct->associated_io = (void *)PIrp;
    sep_shim_io_struct->associated_device = (void *)PDeviceObject;
    
    //clean up context for cancellation
    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    

    EmcpalIrpMarkPending(PIrp);

    /*and allocate a packet*/
    fbe_memory_request_init(&sep_shim_io_struct->memory_request);
    fbe_memory_build_chunk_request(&sep_shim_io_struct->memory_request, 
                                    FBE_MEMORY_CHUNK_SIZE_FOR_PACKET,
                                    1, /* we need one packet */
                                   0, /* Lowest resource priority */
                                   FBE_MEMORY_REQUEST_IO_STAMP_INVALID,
                                    (fbe_memory_completion_function_t)fbe_sep_shim_process_asynch_io_irp_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                    sep_shim_io_struct);

    status = fbe_memory_request_entry(&sep_shim_io_struct->memory_request);
    
    if ((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING)) {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR,
                           "%s: Memory allocation failed. status: %d \n",
                           __FUNCTION__, status);
        sep_shim_io_struct->packet = NULL;
        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }
    return EMCPAL_STATUS_PENDING;

}

static fbe_status_t
fbe_sep_shim_process_asynch_io_irp_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context)
{
    fbe_memory_header_t *         master_memory_header = NULL;
    fbe_sep_shim_io_struct_t *    sep_shim_io_struct = (fbe_sep_shim_io_struct_t *)context;
    PEMCPAL_IO_STACK_LOCATION     pIoStackLocation = NULL;
    ULONG                         IoControlCode = 0;
    PEMCPAL_DEVICE_OBJECT         PDeviceObject;
    PEMCPAL_IRP                   PIrp;
    fbe_status_t                  fbe_status = FBE_STATUS_GENERIC_FAILURE;
    UCHAR                         minorFunction = 0;
    UCHAR                         majorFunction = 0;
    fbe_cpu_id_t                  cpu_id;
    
    PDeviceObject = (PEMCPAL_DEVICE_OBJECT)sep_shim_io_struct->associated_device;
    PIrp = (PEMCPAL_IRP)sep_shim_io_struct->associated_io;

    /* Handle case where allocation fails */
    if (fbe_memory_request_is_allocation_complete(memory_request) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR,
                           "%s: Memory allocation failed. request state: %d \n",
                           __FUNCTION__, memory_request->request_state);
        sep_shim_io_struct->packet = NULL;
        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return FBE_STATUS_OK;
    }

    /*now we should get a memory chunk we can use for the packet*/
    master_memory_header = fbe_memory_get_first_memory_header(memory_request);
    sep_shim_io_struct->packet = (fbe_packet_t *)fbe_memory_header_to_data(master_memory_header);


    /*init here for evryone to use*/
    fbe_status = fbe_transport_initialize_sep_packet(sep_shim_io_struct->packet);
    if (fbe_status != FBE_STATUS_OK) {
        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return FBE_STATUS_OK;
    }

    /* Get cpu_id*/
    //fbe_transport_get_cpu_id(sep_shim_io_struct->packet, &cpu_id);
    //if(cpu_id == FBE_CPU_ID_INVALID) {
        cpu_id = fbe_get_cpu_id();
    fbe_transport_set_cpu_id(sep_shim_io_struct->packet, cpu_id);
    //}

    fbe_payload_ex_allocate_memory_operation(&sep_shim_io_struct->packet->payload_union.payload_ex);

    /*for stuff in the control path we have to be carefull, but regular IO path should be able to process IO at DPC level*/
    pIoStackLocation = EmcpalIrpGetCurrentStackLocation(PIrp);
    majorFunction = EmcpalExtIrpStackMajorFunctionGet(pIoStackLocation);
    minorFunction = EmcpalExtIrpStackMinorFunctionGet(pIoStackLocation);

    switch (majorFunction)
    {
        case EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL:
            IoControlCode = EmcpalExtIrpStackParamIoctlCodeGet(pIoStackLocation);
            switch (IoControlCode) {
                case IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS:
                    fbe_sep_shim_trespass_ownership_loss_ioctl(PDeviceObject, PIrp, sep_shim_io_struct);
                    break;
                case IOCTL_FLARE_CAPABILITIES_QUERY:
                    fbe_sep_shim_capabilities_query_ioctl(PDeviceObject, PIrp, sep_shim_io_struct);
                    break;

                default:
                    /*we break the context for all IOCTL requests at this point*/
                    fbe_transport_set_completion_function(sep_shim_io_struct->packet, (fbe_packet_completion_function_t)fbe_sep_shim_continue_io_asynch, sep_shim_io_struct);
                    fbe_sep_shim_ioctl_run_queue_push_packet(sep_shim_io_struct->packet);
                    break;
            }
            break;
        case EMCPAL_IRP_TYPE_CODE_INTERNAL_DEVICE_CONTROL:
            /*regular IO path, just continue to execute*/
            switch (minorFunction) 
            {
            case FLARE_SGL_READ:
                if(!sep_shim_async_io){
                    fbe_sep_shim_process_sgl_read(PDeviceObject, PIrp, sep_shim_io_struct);
                } else {
                    fbe_transport_set_completion_function(sep_shim_io_struct->packet, 
                        fbe_sep_shim_process_sgl_read_async_completion, 
                        sep_shim_io_struct);
                    fbe_transport_set_packet_attr(sep_shim_io_struct->packet, FBE_PACKET_FLAG_BVD);
                    fbe_transport_run_queue_push_packet(sep_shim_io_struct->packet, sep_shim_rq_method);
                }
                break;
            case FLARE_SGL_WRITE:
                if(!sep_shim_async_io){
                    fbe_sep_shim_process_sgl_write(PDeviceObject, PIrp, sep_shim_io_struct);
                } else {
                    fbe_transport_set_completion_function(sep_shim_io_struct->packet, 
                        fbe_sep_shim_process_sgl_write_async_completion, 
                        sep_shim_io_struct);
                    fbe_transport_set_packet_attr(sep_shim_io_struct->packet, FBE_PACKET_FLAG_BVD);
                    fbe_transport_run_queue_push_packet(sep_shim_io_struct->packet, sep_shim_rq_method);
                }
                break;

            case FLARE_DCA_READ:
            case FLARE_DCA_WRITE:
                if(!sep_shim_async_io){
                    fbe_sep_shim_process_dca_op(PDeviceObject, PIrp, sep_shim_io_struct);
                } else {
                    fbe_transport_set_completion_function(sep_shim_io_struct->packet, 
                        fbe_sep_shim_process_dca_op_async_completion, 
                        sep_shim_io_struct);
                    fbe_transport_set_packet_attr(sep_shim_io_struct->packet, FBE_PACKET_FLAG_BVD);
                    fbe_transport_run_queue_push_packet(sep_shim_io_struct->packet, sep_shim_rq_method);
                }
                break;
            default: 
                fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s:invalid minorFunction: 0x%X\n",__FUNCTION__, minorFunction);
                //clean up context for cancellation
                EmcpalIrpCancelRoutineSet (PIrp, NULL);
                EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
                EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_INVALID_DEVICE_REQUEST);
                EmcpalExtIrpInformationFieldSet(PIrp, 0);
                EmcpalIrpCompleteRequest(PIrp);
                fbe_sep_shim_return_io_structure(sep_shim_io_struct);
                break;
            }
            break;
        default:
            fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s:invalid majorFunction: 0x%X\n",__FUNCTION__, majorFunction);
            //clean up context for cancellation
            EmcpalIrpCancelRoutineSet (PIrp, NULL);
            EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
            EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_INVALID_DEVICE_REQUEST);
            EmcpalExtIrpInformationFieldSet(PIrp, 0);
            EmcpalIrpCompleteRequest(PIrp);
            fbe_sep_shim_return_io_structure(sep_shim_io_struct);
            break;
    }

    return FBE_STATUS_OK;

}
/* IRP MJ Interface */
EMCPAL_STATUS fbe_sep_shim_process_asynch_read_write_irp(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP	PIrp)
{
    fbe_sep_shim_io_struct_t *  sep_shim_io_struct = NULL;
    EMCPAL_STATUS status;
    fbe_status_t                fbe_status;
    
    fbe_status = fbe_sep_shim_get_io_structure(PIrp, &sep_shim_io_struct);
    if (fbe_status == FBE_STATUS_CANCELED)
    {
        return EMCPAL_STATUS_CANCELLED;
    }

    if (sep_shim_io_struct == NULL) {
        EmcpalIrpMarkPending(PIrp);
        return EMCPAL_STATUS_PENDING;
    }

    status = fbe_sep_shim_start_asynch_read_write_irp(sep_shim_io_struct, PDeviceObject, PIrp);
    return status;
}

/* IRP MJ Interface */
static EMCPAL_STATUS fbe_sep_shim_start_asynch_read_write_irp( fbe_sep_shim_io_struct_t *  sep_shim_io_struct, 
                                                               PEMCPAL_DEVICE_OBJECT  PDeviceObject,
                                                               PEMCPAL_IRP	PIrp)
{
    /*for all the asynch IOs, we first allocate memory and then we continue processing*/
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    PEMCPAL_IO_STACK_LOCATION   pIoStackLocation = NULL;
    fbe_u32_t                   number_of_chunks = 0;
    fbe_memory_chunk_size_t     chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;
    fbe_u32_t                   transfer_size = 0;
    fbe_lba_t                   lba = 0;
    fbe_block_count_t           blocks = 0;
   
    pIoStackLocation = EmcpalIrpGetCurrentStackLocation(PIrp);

    /* we assume 512 alignment here */
    lba = EmcpalIrpParamGetWriteByteOffset(pIoStackLocation)/(LONGLONG)FBE_BYTES_PER_BLOCK;
    transfer_size = EmcpalIrpParamGetWriteLength(pIoStackLocation);
    blocks = transfer_size/ FBE_BYTES_PER_BLOCK;
    if ((transfer_size%FBE_BYTES_PER_BLOCK)!=0)
    {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s protocol error, wrong request size %d\n", 
                        __FUNCTION__, transfer_size);
        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }

    /* this will include the memory for a packet */
    status = fbe_sep_shim_calculate_allocation_chunks(blocks, &number_of_chunks, &chunk_size);

    EmcpalIrpMarkPending(PIrp);
    
    /*! make sure we know which IO caused us to generate this structure 
     *
     *  @todo  Need to generate the memory io_stamp from the IRP
     */
    sep_shim_io_struct->associated_io = (void *)PIrp;
    sep_shim_io_struct->associated_device = (void *)PDeviceObject;

    fbe_memory_request_init(&sep_shim_io_struct->memory_request);
    fbe_memory_build_chunk_request(&sep_shim_io_struct->memory_request, 
                                    chunk_size,
                                    number_of_chunks, 
                                   0, /* Lowest resource priority */
                                   FBE_MEMORY_REQUEST_IO_STAMP_INVALID,
                                    (fbe_memory_completion_function_t) fbe_sep_shim_process_asynch_read_write_irp_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */ 
                                    sep_shim_io_struct);

    
    status = fbe_memory_request_entry(&sep_shim_io_struct->memory_request);

    if ((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING)) { 
        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);        
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }
    return EMCPAL_STATUS_PENDING;

}
static fbe_status_t
fbe_sep_shim_process_asynch_read_write_irp_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context)
{
    fbe_packet_t * 				packet = NULL;
    fbe_memory_header_t *	 	master_memory_header = NULL;
    fbe_sep_shim_io_struct_t *	sep_shim_io_struct = (fbe_sep_shim_io_struct_t *)context;
    PEMCPAL_IO_STACK_LOCATION 	pIoStackLocation = NULL;
    fbe_u8_t 					operation = 0;
    PEMCPAL_DEVICE_OBJECT  		PDeviceObject;
    PEMCPAL_IRP 				PIrp = NULL;
    fbe_sg_element_t *          sg_list_p;
    fbe_memory_header_t *       current_memory_header = NULL;
    fbe_u32_t					transfer_size = 0;
    fbe_u8_t *                  buffer_p = NULL;
    fbe_u32_t                   sg_list_size = 0;
    fbe_u32_t                   current_size = 0;
    fbe_u32_t                   chunks_used = 0;
    fbe_status_t				fbe_status;
    fbe_u32_t                   xor_move_size = 0;
    fbe_cpu_id_t                cpu_id;

    if ((sep_shim_io_struct == NULL) || (sep_shim_io_struct->associated_io == NULL)){
        /* This should never happen.*/        
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s:Invalid Parameter sep_shim_io_struct 0x%p\n",
                            __FUNCTION__, sep_shim_io_struct);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the IRP pointer of the original IRP. We need it for error conditions.*/
    PIrp = (PEMCPAL_IRP)sep_shim_io_struct->associated_io;

    /* Handle case where allocation fails */
    if (fbe_memory_request_is_allocation_complete(memory_request) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR,
                           "%s: Memory allocation failed. request state: %d \n",
                           __FUNCTION__, memory_request->request_state);
        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_NO_MEMORY);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }

    /*now we should get a memory chunk we can use for the packet*/
    master_memory_header = fbe_memory_get_first_memory_header(memory_request);
    current_memory_header = master_memory_header;

    /* number_of_chunks includes the one for the packet, but sg_list need one entry for termination */
    sg_list_size = (master_memory_header->number_of_chunks+1) * sizeof(fbe_sg_element_t);
    /* round the sg list size to align with 64-bit boundary. */
    sg_list_size += (sg_list_size % sizeof(fbe_u64_t));

    /* prepare memory to do xor move */
    xor_move_size = sizeof(fbe_xor_mem_move_t);
    xor_move_size += (xor_move_size % sizeof(fbe_u64_t));

    /* if packet and sg_list and xor_mov memory can fit in the last chunk */
    if (master_memory_header->memory_chunk_size > (sizeof(fbe_packet_t)+sg_list_size+xor_move_size))
    {        
        /* find the last chunk */
        for (chunks_used = 0; chunks_used < master_memory_header->number_of_chunks -1; chunks_used ++)
        {
            current_memory_header = current_memory_header->u.hptr.next_header;
        }

        /* carve packet from the end of the last chunk */
        buffer_p = current_memory_header->data + master_memory_header->memory_chunk_size - sizeof(fbe_packet_t);
        sep_shim_io_struct->packet = (fbe_packet_t *)buffer_p;

        /* carve out sg_list memory above that */
        buffer_p -= sg_list_size;
        sg_list_p = (fbe_sg_element_t *) buffer_p;
        sep_shim_io_struct->context = (void *)sg_list_p;

        /* carve out xor_mov memory above that */
        buffer_p -= xor_move_size;
        sep_shim_io_struct->xor_mem_move_ptr = (void *)buffer_p;
    }
    else
    {
        /*
         * If xor_move_size + packet size is greater than chunck size, the chunck size
         * must be just for the packet.  In this case, we have allocated 2 chuncks to fit
         * everything.  There will be no problem fitting sg_list with 
         * the xor_mov in the same chunk.
         */
        if (master_memory_header->number_of_chunks < 2)
        {
            fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s:insufficient memory chunk %d\n",
                                __FUNCTION__, master_memory_header->number_of_chunks);
            //clean up context for cancellation
            EmcpalIrpCancelRoutineSet (PIrp, NULL);
            EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
            EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_NO_MEMORY);
            EmcpalExtIrpInformationFieldSet(PIrp, 0);
            EmcpalIrpCompleteRequest(PIrp);

            fbe_sep_shim_return_io_structure(sep_shim_io_struct);

            return FBE_STATUS_OK;
        }
        /* find the second to last chunk */
        for (chunks_used = 0; chunks_used < master_memory_header->number_of_chunks -2; chunks_used ++)
        {
            current_memory_header = current_memory_header->u.hptr.next_header;
        }

        /* put sg_list and xor_mov memory in this chunk */
        buffer_p = current_memory_header->data;
        sg_list_p = (fbe_sg_element_t *) buffer_p;
        sep_shim_io_struct->context = (void *)sg_list_p;
        buffer_p += sg_list_size;
        sep_shim_io_struct->xor_mem_move_ptr = (void *)buffer_p;

        /* put packet to the last chunk */
        current_memory_header = current_memory_header->u.hptr.next_header;
        buffer_p = current_memory_header->data;
        sep_shim_io_struct->packet = (fbe_packet_t *)buffer_p;

        buffer_p -= sg_list_size;
    }

    /* Debug trace:
        Saw atleast one instance where the allocated memory pointed to a packet ememory where 
        packet->magic_number was FBE_MAGIC_NUMBER_BASE_PACKET
        */
    if (sep_shim_io_struct->packet->magic_number == FBE_MAGIC_NUMBER_BASE_PACKET){
        /* TODO: Debug this scenario and identify which code path leaked the packet.*/
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: Packet 0x%p. Allocated packet was not previously destroyed??\n",
                           __FUNCTION__, packet);
        /* We are ignoring this for now since this was supposed to be newly allocated memory.*/
        /* TODO: Debug this later.*/
    }
    
    /*init packet here and use later*/
    fbe_status = fbe_transport_initialize_sep_packet(sep_shim_io_struct->packet);
    if (fbe_status != FBE_STATUS_OK) {
        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return FBE_STATUS_OK;
    }

    cpu_id = fbe_get_cpu_id();
    fbe_transport_set_cpu_id(sep_shim_io_struct->packet, cpu_id);

    /* we need this to make sure memory_io_master is properly set up so that
     * we won't have problem when siots needs deferred allocation.
     */
    fbe_payload_ex_allocate_memory_operation(&sep_shim_io_struct->packet->payload_union.payload_ex);

    PDeviceObject = (PEMCPAL_DEVICE_OBJECT)sep_shim_io_struct->associated_device;
    /* Figure out the IOCTL */
    pIoStackLocation = EmcpalIrpGetCurrentStackLocation(PIrp);
    operation = EmcpalExtIrpStackMajorFunctionGet(pIoStackLocation);

    switch ( operation ){
    case EMCPAL_IRP_TYPE_CODE_READ:
    case EMCPAL_IRP_TYPE_CODE_WRITE:
        transfer_size = EmcpalIrpParamGetWriteLength(pIoStackLocation)/ FBE_BYTES_PER_BLOCK * FBE_BE_BYTES_PER_BLOCK;

        /* get the next memory header. */
        current_memory_header = master_memory_header;
        while ((transfer_size > 0)&& (current_memory_header!=NULL))
        {
            current_size = master_memory_header->memory_chunk_size ;
            if (current_size>transfer_size)
            {
                current_size = transfer_size;
            }
            buffer_p = current_memory_header->data;
            fbe_sg_element_init(sg_list_p, current_size, buffer_p);
            fbe_sg_element_increment(&sg_list_p);

            transfer_size -= current_size;
            /* next chunk */
            current_memory_header = current_memory_header->u.hptr.next_header;
        }
        fbe_sg_element_terminate(sg_list_p);

        if (transfer_size!=0)
        {
            fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s:more memory needed %d\n",
                                __FUNCTION__, transfer_size);
            //clean up context for cancellation
            EmcpalIrpCancelRoutineSet (PIrp, NULL);
            EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
            EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_NO_MEMORY);
            EmcpalExtIrpInformationFieldSet(PIrp, 0);
            EmcpalIrpCompleteRequest(PIrp);

            fbe_sep_shim_return_io_structure(sep_shim_io_struct);

            return FBE_STATUS_OK;
        }

        if (operation == EMCPAL_IRP_TYPE_CODE_READ)
        {
            if(!sep_shim_async_io){
                fbe_sep_shim_process_mj_read(PDeviceObject, PIrp, sep_shim_io_struct);
            } else {
                fbe_transport_set_completion_function(sep_shim_io_struct->packet, 
                    fbe_sep_shim_process_mj_read_async_completion, 
                    sep_shim_io_struct);
                fbe_transport_set_packet_attr(sep_shim_io_struct->packet, FBE_PACKET_FLAG_BVD);
                fbe_transport_run_queue_push_packet(sep_shim_io_struct->packet, sep_shim_rq_method);
            }
        }
        else
        {
            if(!sep_shim_async_io){
                fbe_sep_shim_process_mj_write(PDeviceObject, PIrp, sep_shim_io_struct);
            } else {
                fbe_transport_set_completion_function(sep_shim_io_struct->packet, 
                    fbe_sep_shim_process_mj_write_async_completion, 
                    sep_shim_io_struct);
                fbe_transport_set_packet_attr(sep_shim_io_struct->packet, FBE_PACKET_FLAG_BVD);
                fbe_transport_run_queue_push_packet(sep_shim_io_struct->packet, sep_shim_rq_method);
            }
        }        

        break;
    default: 
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s:invalid MajorFunction: %X\n",__FUNCTION__, operation);
        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_INVALID_DEVICE_REQUEST);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);

        break;
    
    }

    return FBE_STATUS_OK;
}

void sep_handle_cancel_irp_function(PEMCPAL_DEVICE_OBJECT DeviceObject, PEMCPAL_IRP Irp)
{
    EMCPAL_KIRQL             CancelIrql;
    fbe_packet_t *    packet_p = NULL;
    os_device_info_t * 	dev_info_p = (os_device_info_t *)EmcpalDeviceGetExtension(DeviceObject);

    CancelIrql = EmcpalExtIrpCancelIrqlGet(Irp);

    packet_p = (fbe_packet_t *)(EmcpalExtIrpCurrentStackDriverContextGet(Irp)[0]);

    EmcpalIrpCancelLockRelease (CancelIrql);

    /* TODO: Verify packet_p. */
    /* Check whether the fbe_ call is allowed at DISPATCH level.*/
    if (packet_p != NULL){
        fbe_transport_cancel_packet(packet_p);
    }
    else {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "%s:#:%d already completed, IRP:0x%llX \n",__FUNCTION__,dev_info_p->lun_number, (unsigned long long)Irp);
    }

    return ;
}

/******************************************************************************
*            fbe_sep_shim_get_io_structure_cancel_irp ()
*******************************************************************************
*
*  Description: This is the cancellation function that is called when the 
* request is getting cancelled by the higher level drivers and we are waiting 
* for the IO Structure
*
* 
*  Inputs: DeviceObject  - pointer to the device Object
*          PIrp - Pointer to the IRP that needs to be cancelled
*
*  Return Value: 
*   success or failure
*
*  History:
*   06/29/12 - Ashok Tamilarasan - Created     
******************************************************************************/
void fbe_sep_shim_get_io_structure_cancel_irp(PEMCPAL_DEVICE_OBJECT DeviceObject, PEMCPAL_IRP PIrp)
{
    fbe_cpu_id_t cpu_id, new_cpu_id;
    BOOLEAN success_removal;
    
    /* Need to release the lock first which is held by windows when they call this 
     * function. This lock prevents anybody from resetting it before the function is 
     * called
     */
    cpu_id = (fbe_cpu_id_t)(((fbe_u64_t )(EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0])) & 0xFFFFFFFF);
    EmcpalIrpCancelLockRelease( EmcpalExtIrpCancelIrqlGet(PIrp));

    /* We dont' expect any cancel from cache, we do an error trace to get a stack trace
     * to find out who's issuing cancel.
     */
    fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: irp %p core %d\n",
                        __FUNCTION__, PIrp, cpu_id);

    /* Remove the request from the queue and cancel it */
    fbe_spinlock_lock(&fbe_waiting_irps_info[cpu_id].fbe_waiting_irps_lock);
    success_removal = ListOfIRPsRemove(&fbe_waiting_irps_info[cpu_id].fbe_waiting_irps_queue, PIrp);
    if (!success_removal)
    {
        new_cpu_id = (fbe_cpu_id_t)(((fbe_u64_t )(EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0])) & 0xFFFFFFFF);
        /* If cpu id is not changing, meaning it's already picked up by the wait thread, 
         * let's not to complete the IRP twice.
         */
        if (cpu_id == new_cpu_id)
        {
            fbe_sep_shim_trace(FBE_TRACE_LEVEL_WARNING, "%s: irp %p already picked up by wait thread.\n",
                                __FUNCTION__, PIrp);
            fbe_spinlock_unlock(&fbe_waiting_irps_info[cpu_id].fbe_waiting_irps_lock);
            return;
        }
        fbe_spinlock_unlock(&fbe_waiting_irps_info[cpu_id].fbe_waiting_irps_lock);
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "%s: irp %p not on core %d , new cpu id %d\n",
                            __FUNCTION__, PIrp, cpu_id, new_cpu_id);
        cpu_id = new_cpu_id; 
        fbe_spinlock_lock(&fbe_waiting_irps_info[cpu_id].fbe_waiting_irps_lock);
        success_removal = ListOfIRPsRemove(&fbe_waiting_irps_info[cpu_id].fbe_waiting_irps_queue, PIrp);
        if (!success_removal)
        {
            fbe_sep_shim_trace(FBE_TRACE_LEVEL_WARNING, "%s: irp %p picked up by wait thread on core %d\n",
                                __FUNCTION__, PIrp, new_cpu_id);
            fbe_spinlock_unlock(&fbe_waiting_irps_info[cpu_id].fbe_waiting_irps_lock);
            return;
        }
    }
    fbe_sep_shim_decrement_wait_stats(cpu_id);
    /* There might be some resources waiting for requests wake them up */
    fbe_sep_shim_wake_waiting_request(cpu_id);
    fbe_spinlock_unlock(&fbe_waiting_irps_info[cpu_id].fbe_waiting_irps_lock);

    //clean up context for cancellation
    EmcpalIrpCancelRoutineSet (PIrp, NULL);
    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_CANCELLED); 
    EmcpalExtIrpInformationFieldSet(PIrp, 0);
    EmcpalIrpCompleteRequest(PIrp);
}

EMCPAL_STATUS fbe_sep_shim_process_mj_write(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP	PIrp, fbe_sep_shim_io_struct_t *sep_shim_io_struct)
{

    EMCPAL_STATUS                       status = EMCPAL_STATUS_SUCCESS;
    PEMCPAL_IO_STACK_LOCATION           pIoStackLocation = NULL;
    fbe_status_t                        fbe_status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet_p = NULL;
    fbe_payload_ex_t *                  sep_payload_p = NULL;
    fbe_payload_block_operation_t *     block_operation_p = NULL;
    fbe_sg_element_t *                  sg_p = NULL;
    fbe_u32_t							transfer_size = 0;
    fbe_lba_t                           lba = 0;
    fbe_block_count_t                   blocks = 0;
    os_device_info_t *                  dev_info_p = (os_device_info_t *)EmcpalDeviceGetExtension(PDeviceObject);
    fbe_block_size_t                    exp_block_size = 0, imp_block_size = 0;
    void *								userbuffer = NULL;
    fbe_sg_element_t					sg_source[2];	
    fbe_xor_mem_move_t *                xor_mem_move = (fbe_xor_mem_move_t *)sep_shim_io_struct->xor_mem_move_ptr;
    PEMCPAL_IRP_CANCEL_FUNCTION         old_cancel_routine = NULL;
    fbe_payload_block_operation_opcode_t block_operation_opcode;
    fbe_packet_priority_t               priority;
    
    pIoStackLocation = EmcpalIrpGetCurrentStackLocation(PIrp);

    lba = EmcpalIrpParamGetWriteByteOffset(pIoStackLocation)/(LONGLONG)FBE_BYTES_PER_BLOCK;
    transfer_size = EmcpalIrpParamGetWriteLength(pIoStackLocation);
    blocks = transfer_size/ FBE_BYTES_PER_BLOCK;
    priority = KeyToPacketPriority((fbe_u64_t)EmcpalIrpParamGetReadKey(pIoStackLocation));
    
    packet_p = sep_shim_io_struct->packet;
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_allocate_block_operation(sep_payload_p);
    if (block_operation_p == NULL) {
        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }

    fbe_block_transport_get_exported_block_size(dev_info_p->block_edge.block_edge_geometry, &exp_block_size);
    fbe_block_transport_get_physical_block_size(dev_info_p->block_edge.block_edge_geometry, &imp_block_size);

    /*need to set up the corect opcode */
    /*
    if (!(EmcpalIrpStackGetFlags(pIoStackLocation) & SL_WRITE_THROUGH)){
        block_operation_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE;
    }else{
        block_operation_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED;
    }
    */

    /* We want to treat all MJ writes as noncached. */
    block_operation_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED;

    fbe_status = fbe_payload_block_build_operation(block_operation_p,
                                                   block_operation_opcode,
                                                   lba,
                                                   blocks,
                                                   exp_block_size,
                                                   imp_block_size,
                                                   NULL/*HACK for now, we don't need it*/);

    /* Based on flags in the IRP, set the corresponding flags in the block operation payload.
     */
    fbe_sep_shim_convert_irp_flags(block_operation_p, EmcpalIrpStackGetFlags(pIoStackLocation));

    //1. Setup SGLs - source and dest	
    sg_p = (fbe_sg_element_t *)sep_shim_io_struct->context;
    if (EmcpalExtIrpMdlAddressGet(PIrp) != NULL) {
        userbuffer = EmcpalExtMdlGetSystemAddressSafe(EmcpalExtIrpMdlAddressGet(PIrp), NormalPagePriority);
    } else {
#ifdef EMCPAL_USE_CSX_DCALL
        userbuffer = (void *)EmcpalIrpGetWriteParams(pIoStackLocation)->io_buffer;
#endif        
    }
    fbe_sg_element_init(&sg_source[0],transfer_size,userbuffer);
    fbe_sg_element_terminate(&sg_source[1]);

    //2. Do XOR copy from 512 byte buffer to 520 byte buffer.
    fbe_zero_memory(xor_mem_move, sizeof(fbe_xor_mem_move_t));
    xor_mem_move->status = FBE_XOR_STATUS_NO_ERROR;
    // Don't check the checksum (since we are generating it)
    // Don't set FBE_XOR_OPTION_CHK_CRC
    xor_mem_move->option = 0;
    xor_mem_move->disks = 1;
    xor_mem_move->fru[0].source_sg_p = sg_source;
    xor_mem_move->fru[0].dest_sg_p = sg_p;
    xor_mem_move->fru[0].count     = blocks;
    
    fbe_status = fbe_xor_lib_mem_move(xor_mem_move,FBE_XOR_MOVE_COMMAND_MEM512_TO_MEM520,
                                      0,1,0,0,0);

    if ((fbe_status != FBE_STATUS_OK)||
         (xor_mem_move->status != FBE_XOR_STATUS_NO_ERROR)){
         
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: fbe_xor_mem_move failed Status: %d , %d\n",
                           __FUNCTION__, fbe_status,xor_mem_move->status);
        
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }

    status = fbe_payload_ex_set_sg_list(sep_payload_p, sg_p, 0);

    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              dev_info_p->bvd_object_id); 

    fbe_transport_set_packet_priority(packet_p, priority);
    
    /*make sure the packet has our edge*/
    packet_p->base_edge = &dev_info_p->block_edge.base_edge;

    //Store context for cancellation
    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = (PVOID)packet_p;

    old_cancel_routine = EmcpalIrpCancelRoutineSet(PIrp, sep_handle_cancel_irp_function);
    if (EmcpalIrpIsCancelInProgress(PIrp)){
      old_cancel_routine = EmcpalIrpCancelRoutineSet(PIrp, NULL);
      EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_CANCELLED); 
      //clean up context for cancellation
      EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
      EmcpalExtIrpInformationFieldSet(PIrp, 0);
      EmcpalIrpCompleteRequest(PIrp);
      fbe_sep_shim_return_io_structure(sep_shim_io_struct);
      return EMCPAL_STATUS_CANCELLED;
    }

    fbe_transport_set_completion_function(packet_p, 
                                          fbe_sep_shim_process_mj_write_completion,
                                          sep_shim_io_struct);

    fbe_sep_shim_send_io(packet_p);

    return EMCPAL_STATUS_PENDING;
}

static fbe_status_t fbe_sep_shim_process_mj_write_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sep_shim_io_struct_t *				sep_shim_io_struct = (fbe_sep_shim_io_struct_t *)context;
    PEMCPAL_IRP                             PIrp = sep_shim_io_struct->associated_io;
    fbe_payload_ex_t *                     	payload = NULL;
    fbe_payload_block_operation_t *         block_operation = NULL;
    fbe_u32_t								transfer_size = 0;
    fbe_block_count_t						blocks = 0;
    fbe_status_t							fbe_status = fbe_transport_get_status_code(packet);
    fbe_bool_t								io_failed;

    payload = fbe_transport_get_payload_ex (packet);
    block_operation = fbe_payload_ex_get_block_operation(payload);
   
    fbe_sep_shim_translate_status(packet, block_operation, PIrp, fbe_status, &io_failed);
    
    if (EmcpalExtIrpStatusFieldGet(PIrp) == EMCPAL_STATUS_SUCCESS){

        /* Calculate the size.
        */
        transfer_size = (fbe_u32_t)EmcpalExtIrpInformationFieldGet(PIrp);
        blocks = transfer_size/FBE_BE_BYTES_PER_BLOCK ;

        if((blocks*FBE_BYTES_PER_BLOCK) > FBE_U32_MAX)
        {
            /* We do not expect the block to exceed 32bit limit
            */
            fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, 
                               "%s: block count crossing 32bit limit 0x%llx\n",
                               __FUNCTION__, (unsigned long long)blocks*FBE_BYTES_PER_BLOCK);
            EmcpalExtIrpInformationFieldSet(PIrp, 0);
            EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        }
        else
        {
            EmcpalExtIrpInformationFieldSet(PIrp, (ULONG_PTR)(blocks*FBE_BYTES_PER_BLOCK)); /* Map to user block size*/
        }
    }

    /* Handle I/O's with big responce time */
    if((sep_shim_alert_time != 0) && (EmcpalExtIrpStatusFieldGet(PIrp) == EMCPAL_STATUS_SUCCESS)){
        fbe_u32_t current_coarse_time;
        fbe_atomic_t old_count;
        os_device_info_t *  dev_info_p = (os_device_info_t *)EmcpalDeviceGetExtension((PEMCPAL_DEVICE_OBJECT)sep_shim_io_struct->associated_device);

        current_coarse_time = fbe_transport_get_coarse_time();
        if((current_coarse_time - sep_shim_io_struct->coarse_time) > sep_shim_alert_time){ /* It took too long to process the I/O */
            EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_ALERTED);
            old_count = fbe_atomic_increment(&sep_shim_alert_count);
            if(old_count % 100){
                fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "MCR ALERT: LUN:%d, time: %d, IRP:0x%p \n",
                                                                        dev_info_p->lun_number,
                                                                        current_coarse_time - sep_shim_io_struct->coarse_time,
                                                                        sep_shim_io_struct->associated_io);

            }
        }
    }

    fbe_payload_ex_release_block_operation(payload, block_operation);/*don't need it anymore*/
    /*we might be completing here at DPC level so we have to break the context before returning the IRP,
    some layered apps might not support running at this level*/
    return fbe_sep_shim_continue_completion(packet, sep_shim_io_struct, io_failed);

}



EMCPAL_STATUS fbe_sep_shim_process_mj_read(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP	PIrp, fbe_sep_shim_io_struct_t *sep_shim_io_struct)
{
    EMCPAL_STATUS                       status = EMCPAL_STATUS_SUCCESS;
    PEMCPAL_IO_STACK_LOCATION           pIoStackLocation = NULL;
    fbe_status_t                        fbe_status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet_p = NULL;
    fbe_payload_ex_t *                  sep_payload_p = NULL;
    fbe_payload_block_operation_t *     block_operation_p = NULL;
    fbe_sg_element_t *                  sg_p = NULL;
    fbe_u32_t							transfer_size = 0;
    fbe_lba_t                           lba = 0;
    fbe_block_count_t                   blocks = 0;
    os_device_info_t *                  dev_info_p = (os_device_info_t *)EmcpalDeviceGetExtension(PDeviceObject);
    fbe_block_size_t                    exp_block_size = 0, imp_block_size = 0;
    PEMCPAL_IRP_CANCEL_FUNCTION         old_cancel_routine = NULL;
    fbe_packet_priority_t               priority;
    
    pIoStackLocation = EmcpalIrpGetCurrentStackLocation(PIrp);

    lba = EmcpalIrpParamGetReadByteOffset(pIoStackLocation)/(LONGLONG)FBE_BYTES_PER_BLOCK;
    transfer_size = EmcpalIrpParamGetReadLength(pIoStackLocation);
    blocks = transfer_size / FBE_BYTES_PER_BLOCK;
    priority = KeyToPacketPriority((fbe_u64_t)EmcpalIrpParamGetReadKey(pIoStackLocation));
    
    packet_p = sep_shim_io_struct->packet;
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_allocate_block_operation(sep_payload_p);
    if (block_operation_p == NULL) {
        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }

    fbe_block_transport_get_exported_block_size(dev_info_p->block_edge.block_edge_geometry, &exp_block_size);
    fbe_block_transport_get_physical_block_size(dev_info_p->block_edge.block_edge_geometry, &imp_block_size);

    fbe_status = fbe_payload_block_build_operation(block_operation_p,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                                   lba,
                                                   blocks,
                                                   exp_block_size,
                                                   imp_block_size,
                                                   NULL/*HACK for now, we don't need it*/);

    /* Based on flags in the IRP, set the corresponding flags in the block operation payload.
     */
    fbe_sep_shim_convert_irp_flags(block_operation_p, EmcpalIrpStackGetFlags(pIoStackLocation));
    fbe_payload_block_set_flag(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CHECK_CHECKSUM);

    
#if 0
    /* Set the sg ptr into the packet.*/
    sg_p = (fbe_sg_element_t *)EmcpalExtIrpStackParamArg4Get(pIoStackLocation);/*based on spec, this is where sg list is*/
    status = fbe_payload_ex_set_sg_list(sep_payload_p, sg_p, 0);
#else
    sg_p = (fbe_sg_element_t *)sep_shim_io_struct->context;

    status = fbe_payload_ex_set_sg_list(sep_payload_p, sg_p, 0);
#endif

    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              dev_info_p->bvd_object_id); 

    fbe_transport_set_packet_priority(packet_p, priority);
    
    /*make sure the packet has our edge*/
    packet_p->base_edge = &dev_info_p->block_edge.base_edge;

    //Store context for cancellation
    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = (PVOID)packet_p;    
    
    old_cancel_routine = EmcpalIrpCancelRoutineSet(PIrp, sep_handle_cancel_irp_function);
    if (EmcpalIrpIsCancelInProgress(PIrp)){
      old_cancel_routine = EmcpalIrpCancelRoutineSet(PIrp, NULL);
      EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_CANCELLED); 
      EmcpalExtIrpInformationFieldSet(PIrp, 0);
      //clean up context for cancellation
      EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
      EmcpalIrpCompleteRequest(PIrp);
      fbe_sep_shim_return_io_structure(sep_shim_io_struct);
      return EMCPAL_STATUS_CANCELLED;
    }

    fbe_transport_set_completion_function(packet_p, 
                                          fbe_sep_shim_process_mj_read_completion, 
                                          sep_shim_io_struct);

    fbe_sep_shim_send_io(packet_p);

    return EMCPAL_STATUS_PENDING;
}


static fbe_status_t fbe_sep_shim_process_mj_read_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sep_shim_io_struct_t *				sep_shim_io_struct = (fbe_sep_shim_io_struct_t *)context;
    PEMCPAL_IRP                             PIrp = sep_shim_io_struct->associated_io;
    fbe_payload_ex_t *                     	payload = NULL;
    fbe_payload_block_operation_t *         block_operation = NULL;
    void *									userbuffer = NULL;
    fbe_sg_element_t *						sg_p = NULL;
    fbe_u32_t 								sg_list_count = 0;
    fbe_u32_t								transfer_size = 0;
    fbe_block_count_t						blocks = 0;
    fbe_xor_mem_move_t *                    xor_mem_move = (fbe_xor_mem_move_t *)sep_shim_io_struct->xor_mem_move_ptr;
    fbe_sg_element_t						sg_user[2];
    fbe_status_t							fbe_status = fbe_transport_get_status_code(packet);
    fbe_bool_t								io_failed;
    PEMCPAL_IO_STACK_LOCATION               pIoStackLocation = NULL;

    payload = fbe_transport_get_payload_ex (packet);
    block_operation = fbe_payload_ex_get_block_operation(payload);
    
    fbe_sep_shim_translate_status(packet, block_operation, PIrp, fbe_status, &io_failed);
    
    if (EmcpalExtIrpStatusFieldGet(PIrp) == EMCPAL_STATUS_SUCCESS){

        fbe_payload_ex_get_sg_list(payload,&sg_p,&sg_list_count);

        /* Copy READ data to user buffer.
        */
        transfer_size = (fbe_u32_t)EmcpalExtIrpInformationFieldGet(PIrp);
        blocks = transfer_size/FBE_BE_BYTES_PER_BLOCK ;

        //1. Setup SGLs - source and dest	
        if (EmcpalExtIrpMdlAddressGet(PIrp) != NULL) {
            userbuffer = EmcpalExtMdlGetSystemAddressSafe(EmcpalExtIrpMdlAddressGet(PIrp), NormalPagePriority);
        } else {
#ifdef EMCPAL_USE_CSX_DCALL
            pIoStackLocation = EmcpalIrpGetCurrentStackLocation(PIrp);
            userbuffer = (void *)EmcpalIrpGetReadParams(pIoStackLocation)->io_buffer;
#endif        
        }

        if((blocks*FBE_BYTES_PER_BLOCK) > FBE_U32_MAX)
        {
            /* We do not expect the sg element to exceed 32bit limit
            */
            fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, 
            "%s: sg element count crossing 32bit limit 0x%llx\n",
            __FUNCTION__, (unsigned long long)blocks*FBE_BYTES_PER_BLOCK);
            EmcpalExtIrpInformationFieldSet(PIrp, 0);
            EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        }
        else
        {
            fbe_sg_element_init(&sg_user[0],(fbe_u32_t)(blocks*FBE_BYTES_PER_BLOCK),userbuffer);
            fbe_sg_element_terminate(&sg_user[1]);
            EmcpalExtIrpInformationFieldSet(PIrp, (ULONG_PTR)(blocks*FBE_BYTES_PER_BLOCK)); /* Map to user block size*/
        
            //FBE_ASSERT(sg_list_count == 1)

            //3. Do XOR copy from 512 byte buffer to 520 byte buffer.
            fbe_zero_memory(xor_mem_move, sizeof(fbe_xor_mem_move_t));
            xor_mem_move->status = FBE_XOR_STATUS_NO_ERROR;
            // We have completed a non-cached read.  Check the checksums
            xor_mem_move->option = FBE_XOR_OPTION_CHK_CRC;
            xor_mem_move->disks = 1;
            xor_mem_move->fru[0].source_sg_p = sg_p;
            xor_mem_move->fru[0].dest_sg_p = sg_user; 
            xor_mem_move->fru[0].count     = blocks;
            
            fbe_status = fbe_xor_lib_mem_move(xor_mem_move,FBE_XOR_MOVE_COMMAND_MEM520_TO_MEM512,
                                          0,1,0,0,0);
            if ((fbe_status != FBE_STATUS_OK)||
                (xor_mem_move->status != FBE_XOR_STATUS_NO_ERROR))
            {
                fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: fbe_xor_mem_move failed Status: %d , %d\n",
                                    __FUNCTION__, fbe_status,xor_mem_move->status);
    
                /* We do not want to pass bogus data to user.
                 * Fail the IO.
                 */
            
                EmcpalExtIrpInformationFieldSet(PIrp, 0);
                EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
            }
        }
    }

    /* Handle I/O's with big responce time */
    if((sep_shim_alert_time != 0) && (EmcpalExtIrpStatusFieldGet(PIrp) == EMCPAL_STATUS_SUCCESS)){
        fbe_u32_t current_coarse_time;
        fbe_atomic_t old_count;
        os_device_info_t *  dev_info_p = (os_device_info_t *)EmcpalDeviceGetExtension((PEMCPAL_DEVICE_OBJECT)sep_shim_io_struct->associated_device);

        current_coarse_time = fbe_transport_get_coarse_time();
        if((current_coarse_time - sep_shim_io_struct->coarse_time) > sep_shim_alert_time){ /* It took too long to process the I/O */
            EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_ALERTED);
            old_count = fbe_atomic_increment(&sep_shim_alert_count);
            if(old_count % 100){
                fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "MCR ALERT: LUN:%d, time: %d, IRP:0x%p \n",
                                                                        dev_info_p->lun_number,
                                                                        current_coarse_time - sep_shim_io_struct->coarse_time,
                                                                        sep_shim_io_struct->associated_io);

            }
        }
    }

    fbe_payload_ex_release_block_operation(payload, block_operation);

    /*we might be completing here at DPC level so we have to break the context before returning the IRP,
    some layered apps might not support running at this level*/
    return fbe_sep_shim_continue_completion(packet, sep_shim_io_struct, io_failed);
}

static fbe_status_t 
fbe_sep_shim_process_mj_read_async_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sep_shim_io_struct_t *    sep_shim_io_struct = (fbe_sep_shim_io_struct_t *)context;
    PEMCPAL_DEVICE_OBJECT         PDeviceObject;
    PEMCPAL_IRP                   PIrp;

    PDeviceObject = (PEMCPAL_DEVICE_OBJECT)sep_shim_io_struct->associated_device;
    PIrp = (PEMCPAL_IRP)sep_shim_io_struct->associated_io;
    
    fbe_sep_shim_process_mj_read(PDeviceObject, PIrp, sep_shim_io_struct);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

static fbe_status_t 
fbe_sep_shim_process_mj_write_async_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sep_shim_io_struct_t *    sep_shim_io_struct = (fbe_sep_shim_io_struct_t *)context;
    PEMCPAL_DEVICE_OBJECT         PDeviceObject;
    PEMCPAL_IRP                   PIrp;

    PDeviceObject = (PEMCPAL_DEVICE_OBJECT)sep_shim_io_struct->associated_device;
    PIrp = (PEMCPAL_IRP)sep_shim_io_struct->associated_io;

    fbe_sep_shim_process_mj_write(PDeviceObject, PIrp, sep_shim_io_struct);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}


/*this function might switch the context in the future if needed*/
static fbe_status_t fbe_sep_shim_send_io (fbe_packet_t *packet_p)
{
    fbe_cpu_id_t cpu_id = fbe_get_cpu_id();
    fbe_transport_set_cpu_id(packet_p, cpu_id);
    return fbe_topology_send_io_packet(packet_p);
}

/*this function is called on the context of the transport thread pool, and we use it to retry IOs from the sep shim*/
static fbe_status_t fbe_sep_shim_retry_io(void * packet_p, fbe_packet_completion_context_t context)
{
    fbe_sep_shim_io_struct_t *	io_struct = (fbe_sep_shim_io_struct_t *)context;
    PEMCPAL_DEVICE_OBJECT		PDeviceObject = (PEMCPAL_DEVICE_OBJECT)io_struct->associated_device;
    PEMCPAL_IRP					PIrp = (PEMCPAL_IRP)io_struct->associated_io;
    PEMCPAL_IO_STACK_LOCATION   pIoStackLocation = NULL;
    UCHAR                       majorFunction = 0;

    
    /*we'll restart the packet from scrath since we are retrying*/
    fbe_sep_shim_return_io_structure(io_struct);/*don't need this one anymore and we'll clean up propperly*/

    /*where do we go from here ?*/
    pIoStackLocation = EmcpalIrpGetCurrentStackLocation(PIrp);
    majorFunction = EmcpalExtIrpStackMajorFunctionGet(pIoStackLocation);

    if ((majorFunction == EMCPAL_IRP_TYPE_CODE_READ) || (majorFunction == EMCPAL_IRP_TYPE_CODE_WRITE)) {
        fbe_sep_shim_process_asynch_read_write_irp(PDeviceObject, PIrp);
    }else{
        fbe_sep_shim_process_asynch_io_irp(PDeviceObject, PIrp);
    }

    return FBE_STATUS_OK;

}

static fbe_status_t fbe_sep_shim_continue_completion (fbe_packet_t * packet_p, fbe_sep_shim_io_struct_t * io_struct, fbe_bool_t io_failed)
{

    os_device_info_t *  dev_info_p = (os_device_info_t *)EmcpalDeviceGetExtension((PEMCPAL_DEVICE_OBJECT)io_struct->associated_device);
    fbe_object_id_t		lu_id;
    
    /*the status might be BUSY when the object is draining as it transition between states
    when it's in ACTIVATE, we will get a state of BUSY,. When it's failed we get FAIL and we 
    take care of it and DESTORY is impossible in the way the Xlu stack is built

    What we check here is if we have an object in ACTIVATE but it's one of these LUNs that
    are expected to always be available like PSM or the cache CDR. In this case, we don't want to scare them
    since they don't even do a retry so we will do the retry on our own*/
    fbe_block_transport_get_server_id(&dev_info_p->block_edge, &lu_id);
    if (io_failed) {

        fbe_bool_t is_triple_mirror = FBE_FALSE;

        fbe_private_space_layout_is_lun_on_tripple_mirror(lu_id, &is_triple_mirror);
        /* Immediately return CRC Error back for client to address bad data sent and avoid timeout 
         */
        if (FBE_IS_TRUE(is_triple_mirror) && 
            EmcpalExtIrpStatusFieldGet((PEMCPAL_IRP)io_struct->associated_io) != EMCPAL_STATUS_CRC_ERROR) 
        {

            fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "%s:MCR retry:IRP:0x%llX ,LUN:%d\n",
                                        __FUNCTION__, (unsigned long long)io_struct->associated_io, dev_info_p->lun_number);
        
            fbe_transport_set_timer(packet_p,
                                    FBE_SEP_SHIM_RETRY_MSEC,
                                    (fbe_packet_completion_function_t)fbe_sep_shim_retry_io,
                                    io_struct);
        
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
    }

    if(sep_shim_async_io_compl){
        fbe_queue_head_t tmp_queue;	
    
        fbe_queue_init(&tmp_queue);
        fbe_queue_push(&tmp_queue, &packet_p->queue_element);            
        fbe_transport_run_queue_push(&tmp_queue, fbe_sep_shim_complete_io_async, io_struct);		
        fbe_queue_destroy(&tmp_queue);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

#if 0
	if (dev_info_p->lun_number == 0 || dev_info_p->lun_number == 1) {
            fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "BLKSHIM:IRP:0x%llX completed\n",
                               (unsigned long long)io_struct->associated_io);
	}
#endif

    /*when we get here, we are at thread context so we can continue sending the packet*/
    /*using the associated_io filed is a bit of a hack because we are not supposed to know about it, but we save heve the neet for multiple contexts....*/

    //clean up context for cancellation
    EmcpalExtIrpCurrentStackDriverContextGet((PEMCPAL_IRP)io_struct->associated_io)[0] = NULL;    

    EmcpalIrpCancelRoutineSet((PEMCPAL_IRP)io_struct->associated_io, NULL);
    EmcpalIrpCompleteRequest((PEMCPAL_IRP)io_struct->associated_io);

    fbe_sep_shim_return_io_structure(io_struct);

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_sep_shim_send_finish_completion(void * packet_p, fbe_packet_completion_context_t context)
{

    fbe_sep_shim_io_struct_t * io_struct = (fbe_sep_shim_io_struct_t *)context;

    /*when we get here, we are at thread context so we can continue sending the packet*/
    /*using the associated_io filed is a bit of a hack because we are not supposed to know about it, but we save heve the neet for multiple contexts....*/
    //clean up context for cancellation
    EmcpalExtIrpCurrentStackDriverContextGet((PEMCPAL_IRP)io_struct->associated_io)[0] = NULL;    
    EmcpalIrpCancelRoutineSet((PEMCPAL_IRP)io_struct->associated_io, NULL);
    EmcpalIrpCompleteRequest((PEMCPAL_IRP)io_struct->associated_io);

    fbe_sep_shim_return_io_structure(io_struct);

    return FBE_STATUS_OK;

}


static fbe_status_t fbe_sep_shim_convert_irp_flags(fbe_payload_block_operation_t * block_operation_p, fbe_irp_stack_flags_t irp_Flags)
{
    CSX_ASSERT_AT_COMPILE_TIME(sizeof(fbe_irp_stack_flags_t) == sizeof(EMCPAL_IRP_STACK_FLAGS));
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_sep_shim_convert_sgl_info_flags(fbe_payload_block_operation_t * block_operation_p, UINT_16 sgl_info_flags)
{
    fbe_payload_block_operation_opcode_t	block_opcode;
    /* Based on the sgl info flags, set the appropriate flags in the block operation payload.
     */
    if (!(sgl_info_flags & SGL_FLAG_VALIDATE_DATA))
    {
        /* Block operations by default have the check checksum flag set. If the caller does NOT have SL_VALIDATE_DATA
         * set, clear the check checksum flag.
         */
        fbe_payload_block_clear_flag(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CHECK_CHECKSUM);
    }

    if (sgl_info_flags & SGL_FLAG_ALLOW_FAIL_FOR_CONGESTION)
    {
        /* If the client wants us to fail back when we get congested, set the corresponding bit in the block op.
         */
        fbe_payload_block_set_flag(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_ALLOW_FAIL_CONGESTION);
    }

    if (sgl_info_flags & SGL_FLAG_ALLOW_FAIL_FOR_REDIRECTION)
    {
        /* If the client wants us to fail back when local path is broken, set the corresponding bit in the block op.
         */
        fbe_payload_block_set_flag(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_ALLOW_FAIL_NOT_PREFERRED);
    }

    fbe_payload_block_get_opcode(block_operation_p, &block_opcode);

    if ((sgl_info_flags & SGL_FLAG_NON_CACHED_WRITE) && (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE))
    {
        /* This flag indicates that we should create a non cached write opcode.
         */
        block_operation_p->block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED;
    }

    if (sgl_info_flags & SGL_FLAG_VERIFY_BEFORE_WRITE && (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE))
    {
        /* This flag indicates that we should create a write verify opcode.
         */
        block_operation_p->block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE;
    }
    
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_sep_shim_continue_io_asynch(void * packet_p, fbe_packet_completion_context_t context)
{
    fbe_sep_shim_io_struct_t *    sep_shim_io_struct = (fbe_sep_shim_io_struct_t *)context;
    PEMCPAL_IO_STACK_LOCATION     pIoStackLocation = NULL;
    ULONG                         IoControlCode = 0;
    PEMCPAL_DEVICE_OBJECT         PDeviceObject;
    PEMCPAL_IRP                   PIrp;
    UCHAR                         minorFunction = 0;
    UCHAR                         majorFunction = 0;

    PDeviceObject = (PEMCPAL_DEVICE_OBJECT)sep_shim_io_struct->associated_device;
    PIrp = (PEMCPAL_IRP)sep_shim_io_struct->associated_io;

    /* Figure out the IOCTL */
    pIoStackLocation = EmcpalIrpGetCurrentStackLocation(PIrp);
    majorFunction = EmcpalExtIrpStackMajorFunctionGet(pIoStackLocation);
    minorFunction = EmcpalExtIrpStackMinorFunctionGet(pIoStackLocation);

    switch (majorFunction){
        case EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL:
            IoControlCode = EmcpalExtIrpStackParamIoctlCodeGet(pIoStackLocation);
            switch (IoControlCode) {
                case IOCTL_FLARE_ZERO_FILL:
                    fbe_sep_shim_process_zero_fill(PDeviceObject, PIrp, sep_shim_io_struct);
                    break;
                case IOCTL_FLARE_WRITE_BUFFER:
                    fbe_sep_shim_write_buffer_ioctl(PDeviceObject, PIrp, sep_shim_io_struct);
                    break;
                case IOCTL_FLARE_TRESPASS_EXECUTE:
                    fbe_sep_shim_trespass_execute_ioctl(PDeviceObject, PIrp, sep_shim_io_struct);
                    break;
                case IOCTL_FLARE_REGISTER_FOR_CHANGE_NOTIFICATION:
                    fbe_sep_shim_register_for_change_notification_ioctl(PDeviceObject, PIrp, sep_shim_io_struct);
                    break;
                case IOCTL_FLARE_SET_POWER_SAVING_POLICY:
                    fbe_sep_shim_set_power_saving_policy_ioctl(PDeviceObject, PIrp, sep_shim_io_struct);
                    break;
                case IOCTL_FLARE_GET_RAID_INFO:
                    fbe_sep_shim_get_raid_info_ioctl(PDeviceObject, PIrp, sep_shim_io_struct);
                    break;
                case IOCTL_FLARE_READ_BUFFER:
                    fbe_sep_shim_read_buffer_ioctl(PDeviceObject, PIrp, sep_shim_io_struct);
                    break;
                case IOCTL_FLARE_DESCRIBE_EXTENTS:
                    fbe_sep_shim_describe_extents_ioctl(PDeviceObject, PIrp, sep_shim_io_struct);
                    break;
                case IOCTL_STORAGE_EJECT_MEDIA:
                    fbe_sep_shim_eject_media_ioctl(PDeviceObject, PIrp, sep_shim_io_struct);
                    break;
                case IOCTL_STORAGE_LOAD_MEDIA:
                    fbe_sep_shim_load_media_ioctl(PDeviceObject, PIrp, sep_shim_io_struct);
                    break;
                case IOCTL_FLARE_MODIFY_CAPACITY:
                    fbe_sep_shim_process_modify_capacity(PDeviceObject, PIrp, sep_shim_io_struct);
                    break;
                case IOCTL_FLARE_GET_VOLUME_CACHE_CONFIG:
                    fbe_sep_shim_process_get_volume_cache_config(PDeviceObject, PIrp, sep_shim_io_struct);
                    break;
                case IOCTL_DISK_IS_WRITABLE:
                    fbe_sep_shim_process_disk_is_writable(PDeviceObject, PIrp, sep_shim_io_struct);
                    break;
                case IOCTL_FLARE_VOLUME_MAY_HAVE_FAILED:
                    fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "SEP: IOCTL_FLARE_VOLUME_MAY_HAVE_FAILED received\n");
                    //clean up context for cancellation
                    EmcpalIrpCancelRoutineSet (PIrp, NULL);
                    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
                    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
                    EmcpalExtIrpInformationFieldSet(PIrp, 0);
                    EmcpalIrpCompleteRequest(PIrp);
                    fbe_sep_shim_return_io_structure(sep_shim_io_struct);
                    break;
                case IOCTL_CACHE_SEND_READ_AND_PIN_INDEX:
                    fbe_sep_shim_send_read_pin_index(PDeviceObject, PIrp, sep_shim_io_struct);
                    break;
                case IOCTL_FLARE_GET_VOLUME_STATE:
                    fbe_sep_shim_get_volume_state_ioctl(PDeviceObject, PIrp, sep_shim_io_struct);
                    break;
                default:
                    fbe_sep_shim_trace(FBE_TRACE_LEVEL_WARNING, "%s:invalid IoControlCode: 0x%X\n",__FUNCTION__, IoControlCode);
                    EmcpalIrpCancelRoutineSet (PIrp, NULL);
                    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
                    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_INVALID_DEVICE_REQUEST);
                    EmcpalExtIrpInformationFieldSet(PIrp, 0);
                    EmcpalIrpCompleteRequest(PIrp);
                    fbe_sep_shim_return_io_structure(sep_shim_io_struct);
                    break;
            }
            break;
            
        default:
            fbe_sep_shim_trace(FBE_TRACE_LEVEL_WARNING, "%s:invalid majorFunction: 0x%X\n",__FUNCTION__, majorFunction);
            //clean up context for cancellation
            EmcpalIrpCancelRoutineSet (PIrp, NULL);
            EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
            EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_INVALID_DEVICE_REQUEST);
            EmcpalExtIrpInformationFieldSet(PIrp, 0);
            EmcpalIrpCompleteRequest(PIrp);
            fbe_sep_shim_return_io_structure(sep_shim_io_struct);
            break;
    }

    return FBE_STATUS_OK;
}


fbe_status_t 
fbe_sep_shim_set_async_io(fbe_bool_t async_io)
{
    sep_shim_async_io = async_io;
    fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "sep_shim_async_io: 0x%X\n", sep_shim_async_io);
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sep_shim_set_async_io_compl(fbe_bool_t async_io_compl)
{
    sep_shim_async_io_compl = async_io_compl;
    fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "sep_shim_async_io_compl: 0x%X\n", sep_shim_async_io_compl);
    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_sep_shim_process_sgl_read_async_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sep_shim_io_struct_t *    sep_shim_io_struct = (fbe_sep_shim_io_struct_t *)context;
    PEMCPAL_DEVICE_OBJECT         PDeviceObject;
    PEMCPAL_IRP                   PIrp;

    PDeviceObject = (PEMCPAL_DEVICE_OBJECT)sep_shim_io_struct->associated_device;
    PIrp = (PEMCPAL_IRP)sep_shim_io_struct->associated_io;

    fbe_sep_shim_process_sgl_read(PDeviceObject, PIrp, sep_shim_io_struct);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

static fbe_status_t 
fbe_sep_shim_process_sgl_write_async_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sep_shim_io_struct_t *    sep_shim_io_struct = (fbe_sep_shim_io_struct_t *)context;
    PEMCPAL_DEVICE_OBJECT         PDeviceObject;
    PEMCPAL_IRP                   PIrp;

    PDeviceObject = (PEMCPAL_DEVICE_OBJECT)sep_shim_io_struct->associated_device;
    PIrp = (PEMCPAL_IRP)sep_shim_io_struct->associated_io;

    fbe_sep_shim_process_sgl_write(PDeviceObject, PIrp, sep_shim_io_struct);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

static fbe_status_t 
fbe_sep_shim_process_dca_op_async_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sep_shim_io_struct_t *    sep_shim_io_struct = (fbe_sep_shim_io_struct_t *)context;
    PEMCPAL_DEVICE_OBJECT         PDeviceObject;
    PEMCPAL_IRP                   PIrp;

    PDeviceObject = (PEMCPAL_DEVICE_OBJECT)sep_shim_io_struct->associated_device;
    PIrp = (PEMCPAL_IRP)sep_shim_io_struct->associated_io;

    fbe_sep_shim_process_dca_op(PDeviceObject, PIrp, sep_shim_io_struct);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}


static fbe_status_t 
fbe_sep_shim_complete_io_async(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_sep_shim_io_struct_t * io_struct = (fbe_sep_shim_io_struct_t *)context;

    //clean up context for cancellation
    EmcpalExtIrpCurrentStackDriverContextGet((PEMCPAL_IRP)io_struct->associated_io)[0] = NULL;    
    EmcpalIrpCancelRoutineSet((PEMCPAL_IRP)io_struct->associated_io, NULL);
    EmcpalIrpCompleteRequest((PEMCPAL_IRP)io_struct->associated_io);
    
    fbe_sep_shim_return_io_structure(io_struct);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sep_shim_set_rq_method(fbe_transport_rq_method_t rq_method)
{
    sep_shim_rq_method = rq_method;
    fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "sep_shim_rq_method: 0x%X\n", sep_shim_rq_method);
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sep_shim_set_alert_time(fbe_u32_t alert_time)
{
    sep_shim_alert_time = alert_time;
    fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "sep_shim_alert_time: %d\n", sep_shim_alert_time);
    return FBE_STATUS_OK;
}

/*
 * Map Irp priority to Packet priority.
 * Prioriry encoded in (Key & 0x7).
 */
fbe_packet_priority_t KeyToPacketPriority(fbe_u64_t Key)
{
    switch(Key & 0x7)
    {
        case KEY_PRIORITY_DEFAULT:  return FBE_PACKET_PRIORITY_NORMAL;
        case KEY_PRIORITY_OPTIONAL: return FBE_PACKET_PRIORITY_LOW;
        case KEY_PRIORITY_LOW:      return FBE_PACKET_PRIORITY_LOW;
        case KEY_PRIORITY_LOW1:     return FBE_PACKET_PRIORITY_LOW;
        case KEY_PRIORITY_NORMAL:   return FBE_PACKET_PRIORITY_NORMAL;
        case KEY_PRIORITY_NORMAL1:  return FBE_PACKET_PRIORITY_NORMAL;
        case KEY_PRIORITY_HIGH:     return FBE_PACKET_PRIORITY_NORMAL;
        case KEY_PRIORITY_URGENT:   return FBE_PACKET_PRIORITY_URGENT;
    }
    return FBE_PACKET_PRIORITY_NORMAL;
}
/*********************************************************************
*            fbe_sep_shim_process_dca_op ()
*********************************************************************
*
*  Description: This is the DCA operation from upper drivers
*
*  Inputs: PDeviceObject  - pointer to the device object
*          PIRP - IRP from top driver
*          sep_shim_io_struct - Pointer to the io struct
*
*  Return Value: 
*   success or failure
*
*  History:
*   06/29/12 - Ashok Tamilarasan - Created     
*********************************************************************/
EMCPAL_STATUS fbe_sep_shim_process_dca_op(PEMCPAL_DEVICE_OBJECT  PDeviceObject,
                                            PEMCPAL_IRP	PIrp, 
                                            fbe_sep_shim_io_struct_t *sep_shim_io_struct)
{
    PEMCPAL_IO_STACK_LOCATION           pIoStackLocation = NULL;
    fbe_status_t                        fbe_status;
    fbe_block_count_t                   blocks;
    fbe_u32_t                   transfer_size;
    fbe_u32_t                   number_of_chunks;
    fbe_memory_chunk_size_t     chunk_size;
    DCA_TABLE                   *dca_table;

    pIoStackLocation = EmcpalIrpGetCurrentStackLocation(PIrp);

    transfer_size = EmcpalIrpParamGetWriteLength(pIoStackLocation);
    blocks = transfer_size/ FBE_BYTES_PER_BLOCK;

    if ((transfer_size%FBE_BYTES_PER_BLOCK)!=0)
    {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s protocol error, wrong request size %d\n", 
                        __FUNCTION__, transfer_size);
        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }

    dca_table = (DCA_TABLE *)EmcpalExtIrpStackParamArg4Get(pIoStackLocation);/*based on spec, this is where sg list is*/

    /* MCR does not support Physical Address in SGL and so return Error */
    if(!dca_table->ReportVirtualAddressesInSGL) {
         fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s MCR does not support Physical Address in SGL\n", 
                        __FUNCTION__);
        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_NOT_SUPPORTED);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return EMCPAL_STATUS_NOT_SUPPORTED;
    }
    /* Request Memory for this IO Request this will include the memory for a dca arg structure */
    fbe_status = fbe_sep_shim_calculate_dca_allocation_chunks(blocks, &number_of_chunks, &chunk_size);
    
    fbe_memory_request_init(&sep_shim_io_struct->buffer);
    fbe_memory_build_chunk_request(&sep_shim_io_struct->buffer, 
                                   chunk_size,
                                   number_of_chunks, 
                                   0, /* Lowest resource priority */
                                   FBE_MEMORY_REQUEST_IO_STAMP_INVALID,
                                   (fbe_memory_completion_function_t) fbe_sep_shim_process_asynch_dca_op_irp_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */ 
                                   sep_shim_io_struct);

    fbe_status = fbe_memory_request_entry(&sep_shim_io_struct->buffer);

    if ((fbe_status != FBE_STATUS_OK) && (fbe_status != FBE_STATUS_PENDING)) { 
        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);        
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }
    return EMCPAL_STATUS_PENDING;
}

/******************************************************************************
*            fbe_sep_shim_process_asynch_dca_op_irp_allocation_completion ()
*******************************************************************************
*
*  Description: This completion function that gets called after the memory for
*  this IO Request is allocated
*
*  Inputs: memory_request  - pointer to the memory request
*          context - Context
*
*  Return Value: 
*   success or failure
*
*  History:
*   06/29/12 - Ashok Tamilarasan - Created     
*********************************************************************/
static fbe_status_t
fbe_sep_shim_process_asynch_dca_op_irp_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context)
{
    fbe_sep_shim_io_struct_t *	sep_shim_io_struct = (fbe_sep_shim_io_struct_t *)context;
    PEMCPAL_IO_STACK_LOCATION 	pIoStackLocation = NULL;
    PEMCPAL_IRP 				PIrp;
    UCHAR                       minorFunction = 0;
    fbe_status_t				fbe_status;
    PEMCPAL_DEVICE_OBJECT         PDeviceObject;

    PDeviceObject = (PEMCPAL_DEVICE_OBJECT)sep_shim_io_struct->associated_device;
    PIrp = (PEMCPAL_IRP)sep_shim_io_struct->associated_io;

    /* Handle case where allocation fails */
    if (fbe_memory_request_is_allocation_complete(memory_request) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR,
                           "%s: Memory allocation failed. request state: %d \n",
                           __FUNCTION__, memory_request->request_state);
        sep_shim_io_struct->packet = NULL;
        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return FBE_STATUS_OK;
    }

    /* Setup the DCA buffer to start the actual operation */
    fbe_status = fbe_sep_shim_setup_dca_buffer(sep_shim_io_struct, memory_request);

    if(fbe_status != FBE_STATUS_OK) 
    {
        /* Not much to do. The IRP would have been completed with error status */
        return fbe_status;
    }

     /*for stuff in the control path we have to be carefull, but regular IO path should be able to process IO at DPC level*/
    pIoStackLocation = EmcpalIrpGetCurrentStackLocation(PIrp);
    minorFunction = EmcpalExtIrpStackMinorFunctionGet(pIoStackLocation);

    if(minorFunction == FLARE_DCA_READ) 
    {
        fbe_status = fbe_sep_shim_process_asynch_dca_read(sep_shim_io_struct);
    }
    else
    {
        fbe_status = fbe_sep_shim_process_asynch_dca_write(sep_shim_io_struct);
    }

    return fbe_status;
}
/******************************************************************************
*            fbe_sep_shim_process_asynch_dca_read ()
*******************************************************************************
*
*  Description: This process the DCA read request from the upper drivers
*
*  Inputs: memory_request  - pointer to the memory request
*          context - Context
*
*  Return Value: 
*   success or failure
*
*  History:
*   06/29/12 - Ashok Tamilarasan - Created     
*********************************************************************/
static fbe_status_t
fbe_sep_shim_process_asynch_dca_read(fbe_sep_shim_io_struct_t *	sep_shim_io_struct)
{
    fbe_packet_t *     			packet_p;
    PEMCPAL_IO_STACK_LOCATION 	pIoStackLocation = NULL;
    PEMCPAL_IRP 				PIrp = NULL;
    fbe_status_t				fbe_status;
    fbe_payload_ex_t *                  sep_payload_p = NULL;
    fbe_payload_block_operation_t *     block_operation_p = NULL;
    fbe_sg_element_t *                  sg_p = NULL;
    fbe_lba_t                           lba = 0;
    fbe_block_count_t                   blocks = 0;
    fbe_packet_priority_t               priority;
    os_device_info_t *                  dev_info_p;
    fbe_block_size_t                    exp_block_size = 0, imp_block_size = 0;
    PEMCPAL_IRP_CANCEL_FUNCTION         old_cancel_routine = NULL;
    FLARE_DCA_ARG *                     flare_dca_arg;
    fbe_u32_t                   transfer_size;
    PEMCPAL_DEVICE_OBJECT   PDeviceObject;


    PDeviceObject = (PEMCPAL_DEVICE_OBJECT)sep_shim_io_struct->associated_device;
    dev_info_p = (os_device_info_t *)EmcpalDeviceGetExtension(PDeviceObject);
    PIrp = (PEMCPAL_IRP)sep_shim_io_struct->associated_io;
    pIoStackLocation = EmcpalIrpGetCurrentStackLocation(PIrp);

    lba = EmcpalIrpParamGetReadByteOffset(pIoStackLocation)/(LONGLONG)FBE_BYTES_PER_BLOCK ;
    transfer_size = EmcpalIrpParamGetWriteLength(pIoStackLocation);
    blocks = transfer_size/ FBE_BYTES_PER_BLOCK;

    
    packet_p = sep_shim_io_struct->packet;
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_allocate_block_operation(sep_payload_p);
    if (block_operation_p == NULL) {
        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }

    fbe_block_transport_get_exported_block_size(dev_info_p->block_edge.block_edge_geometry, &exp_block_size);
    fbe_block_transport_get_physical_block_size(dev_info_p->block_edge.block_edge_geometry, &imp_block_size);

    fbe_status = fbe_payload_block_build_operation(block_operation_p,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                                   lba,
                                                   blocks,
                                                   exp_block_size,
                                                   imp_block_size,
                                                   NULL/*HACK for now, we don't need it*/);

    /* Set the sg ptr into the packet.*/
    sep_shim_io_struct->dca_table = (DCA_TABLE *)EmcpalExtIrpStackParamArg4Get(pIoStackLocation);/*based on spec, this is where sg list is*/
    sg_p = (fbe_sg_element_t *)sep_shim_io_struct->context;
    fbe_status = fbe_payload_ex_set_sg_list(sep_payload_p, sg_p, 0);

    /* Setup the DCA ARG structure for this request */
    flare_dca_arg = (FLARE_DCA_ARG *)sep_shim_io_struct->flare_dca_arg;
    flare_dca_arg->Offset = 0;
    flare_dca_arg->TransferBytes = transfer_size = EmcpalIrpParamGetWriteLength(pIoStackLocation);
    flare_dca_arg->SGList = (PSGL)sg_p;
    flare_dca_arg->DSCallback = (PDSCB)fbe_sep_shim_process_dca_arg_read_completion;
    flare_dca_arg->DSContext1 = sep_shim_io_struct;
    flare_dca_arg->SglType = SGL_SKIP_SECTOR_OVERHEAD_BYTES | SGL_VIRTUAL_ADDRESSES;

    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              dev_info_p->bvd_object_id); 

    priority = KeyToPacketPriority((fbe_u64_t)EmcpalIrpParamGetReadKey(pIoStackLocation));
    fbe_transport_set_packet_priority(packet_p, priority);

    /*make sure the packet has our edge*/
    packet_p->base_edge = &dev_info_p->block_edge.base_edge;

    //Store context for cancellation
    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = (PVOID)packet_p;    
    old_cancel_routine = EmcpalIrpCancelRoutineSet(PIrp, sep_handle_cancel_irp_function);
    if (EmcpalIrpIsCancelInProgress(PIrp)){
      old_cancel_routine = EmcpalIrpCancelRoutineSet(PIrp, NULL);
      EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_CANCELLED); 
      EmcpalExtIrpInformationFieldSet(PIrp, 0);
      //clean up context for cancellation
      EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
      EmcpalIrpCompleteRequest(PIrp);
      fbe_sep_shim_return_io_structure(sep_shim_io_struct);
      return EMCPAL_STATUS_CANCELLED;
    }

    fbe_transport_set_completion_function(packet_p, 
                                          fbe_sep_shim_process_dca_read_completion, 
                                          sep_shim_io_struct);

    fbe_sep_shim_send_io(packet_p);

    return EMCPAL_STATUS_PENDING;
}

/******************************************************************************
*            fbe_sep_shim_process_dca_read_completion ()
*******************************************************************************
*
*  Description: This callback function that gets called after the read is completed
*
*  Inputs: packet  - pointer to the packet
*          context - Context
*
*  Return Value: 
*   success or failure
*
*  History:
*   06/29/12 - Ashok Tamilarasan - Created     
*********************************************************************/
static fbe_status_t fbe_sep_shim_process_dca_read_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sep_shim_io_struct_t *				sep_shim_io_struct = (fbe_sep_shim_io_struct_t *)context;
    PEMCPAL_IRP                             PIrp = sep_shim_io_struct->associated_io;
    fbe_payload_ex_t *                     	payload = NULL;
    fbe_payload_block_operation_t *         block_operation = NULL;
    fbe_status_t							fbe_status = FBE_STATUS_GENERIC_FAILURE;
    DCA_TABLE                               *dca_table;
    fbe_bool_t                              io_failed;

    payload = fbe_transport_get_payload_ex (packet);
    block_operation = fbe_payload_ex_get_block_operation(payload);
    fbe_status = fbe_transport_get_status_code(packet);
    /* Translate the fbe status to an IRP status.
     */
    fbe_sep_shim_translate_status(packet, block_operation, PIrp, fbe_status, &io_failed);

    fbe_payload_ex_release_block_operation(payload, block_operation);/*don't need it anymore*/

    if (!EMCPAL_IS_SUCCESS(EmcpalExtIrpStatusFieldGet(PIrp)))
    {
        //clean up context for cancellation
        EmcpalExtIrpCurrentStackDriverContextGet((PEMCPAL_IRP)sep_shim_io_struct->associated_io)[0] = NULL;    
        EmcpalIrpCancelRoutineSet((PEMCPAL_IRP)sep_shim_io_struct->associated_io, NULL);
        EmcpalIrpCompleteRequest((PEMCPAL_IRP)sep_shim_io_struct->associated_io);
        /* Free the memory now that the client is done with this buffer*/
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return FBE_STATUS_OK;
    }

    dca_table = (DCA_TABLE *) sep_shim_io_struct->dca_table;
    /* Call the client specified called back to hand control over to them to process the 
     * data that was read
     */
    dca_table->XferFunction(PIrp, (FLARE_DCA_ARG*)(sep_shim_io_struct->flare_dca_arg));

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/******************************************************************************
*            fbe_sep_shim_process_dca_arg_read_completion ()
*******************************************************************************
*
*  Description: This callback function that gets called after the client has
*  transferred the data from our buffer
*
*  Inputs: packet  - pointer to the packet
*          context - Context
*
*  Return Value: 
*   success or failure
*
*  History:
*   06/29/12 - Ashok Tamilarasan - Created     
*********************************************************************/
static fbe_status_t fbe_sep_shim_process_dca_arg_read_completion(EMCPAL_STATUS nt_status, fbe_sep_shim_io_struct_t * sep_shim_io_struct)
{

    /*when we get here, we are at thread context so we can continue sending the packet*/
    /*using the associated_io filed is a bit of a hack because we are not supposed to know about it, but we save heve the neet for multiple contexts....*/

    //clean up context for cancellation
    EmcpalExtIrpCurrentStackDriverContextGet((PEMCPAL_IRP)sep_shim_io_struct->associated_io)[0] = NULL;    
    EmcpalIrpCancelRoutineSet((PEMCPAL_IRP)sep_shim_io_struct->associated_io, NULL);
    EmcpalIrpCompleteRequest((PEMCPAL_IRP)sep_shim_io_struct->associated_io);

    
    /* Free the memory now that the client is done with this buffer*/
    fbe_sep_shim_return_io_structure(sep_shim_io_struct);

    return FBE_STATUS_OK;
}

/******************************************************************************
*            fbe_sep_shim_process_asynch_dca_write ()
*******************************************************************************
*
*  Description: This completion function that gets called after the memory for
*  this IO Request is allocated
*
*  Inputs: memory_request  - pointer to the memory request
*          context - Context
*
*  Return Value: 
*   success or failure
*
*  History:
*   06/29/12 - Ashok Tamilarasan - Created     
******************************************************************************/
static fbe_status_t
fbe_sep_shim_process_asynch_dca_write(fbe_sep_shim_io_struct_t *	sep_shim_io_struct)
{
    PEMCPAL_IO_STACK_LOCATION 	pIoStackLocation = NULL;
    PEMCPAL_IRP 				PIrp = NULL;
    FLARE_DCA_ARG *             flare_dca_arg;
    fbe_u32_t                   transfer_size;
    fbe_sg_element_t *          sg_p = NULL;
    DCA_TABLE *                 dca_table;

    PIrp = (PEMCPAL_IRP)sep_shim_io_struct->associated_io;
    pIoStackLocation = EmcpalIrpGetCurrentStackLocation(PIrp);

    /* Set the sg ptr into the packet.*/
    sep_shim_io_struct->dca_table = (DCA_TABLE *)EmcpalExtIrpStackParamArg4Get(pIoStackLocation);/*based on spec, this is where sg list is*/
    sg_p = (fbe_sg_element_t *)sep_shim_io_struct->context;

    flare_dca_arg = (FLARE_DCA_ARG *)sep_shim_io_struct->flare_dca_arg;
    flare_dca_arg->Offset = 0;
    flare_dca_arg->TransferBytes = transfer_size = EmcpalIrpParamGetWriteLength(pIoStackLocation);
    flare_dca_arg->SGList = (PSGL)sg_p;
    flare_dca_arg->DSCallback = (PDSCB)fbe_sep_shim_process_dca_arg_write_completion;
    flare_dca_arg->DSContext1 = sep_shim_io_struct;
    flare_dca_arg->SglType = SGL_SKIP_SECTOR_OVERHEAD_BYTES | SGL_VIRTUAL_ADDRESSES;

    dca_table = (DCA_TABLE *) sep_shim_io_struct->dca_table;

    /* Call the client specified callback function, to get the write data into the buffer
     * we have allocated
     */
    dca_table->XferFunction(PIrp, (FLARE_DCA_ARG*)(sep_shim_io_struct->flare_dca_arg));

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/******************************************************************************
*            fbe_sep_shim_process_dca_arg_write_completion ()
*******************************************************************************
*
*  Description: This is called by the client when it has transferred the data 
* into the buffer we have allocated to start the write operation
*
*  Inputs: memory_request  - pointer to the memory request
*          context - Context
*
*  Return Value: 
*   success or failure
*
*  History:
*   06/29/12 - Ashok Tamilarasan - Created     
******************************************************************************/
static fbe_status_t fbe_sep_shim_process_dca_arg_write_completion(EMCPAL_STATUS nt_status, fbe_sep_shim_io_struct_t * sep_shim_io_struct)
{
    fbe_packet_t *         		packet_p = NULL;
    PEMCPAL_IRP 				PIrp = NULL;
    fbe_status_t				fbe_status;
    fbe_payload_ex_t *                  sep_payload_p = NULL;
    fbe_payload_block_operation_t *     block_operation_p = NULL;
    fbe_block_size_t                    exp_block_size = 0, imp_block_size = 0;
    os_device_info_t *                  dev_info_p;
    PEMCPAL_DEVICE_OBJECT  		PDeviceObject;
    PEMCPAL_IRP_CANCEL_FUNCTION         old_cancel_routine = NULL;
    fbe_lba_t                           lba = 0;
    fbe_block_count_t                   blocks = 0;
    fbe_packet_priority_t               priority;
    PEMCPAL_IO_STACK_LOCATION 	pIoStackLocation = NULL;
    fbe_sg_element_t *          sg_p = NULL;

    PIrp = (PEMCPAL_IRP)sep_shim_io_struct->associated_io;

    packet_p = sep_shim_io_struct->packet;
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_allocate_block_operation(sep_payload_p);
    if (block_operation_p == NULL) {
        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }

    PDeviceObject = (PEMCPAL_DEVICE_OBJECT)sep_shim_io_struct->associated_device;
    dev_info_p = (os_device_info_t *)EmcpalDeviceGetExtension(PDeviceObject);
    pIoStackLocation = EmcpalIrpGetCurrentStackLocation(PIrp);

    lba = EmcpalIrpParamGetReadByteOffset(pIoStackLocation)/(LONGLONG)FBE_BYTES_PER_BLOCK ;
    blocks = EmcpalIrpParamGetWriteLength(pIoStackLocation) / FBE_BYTES_PER_BLOCK;

    fbe_block_transport_get_exported_block_size(dev_info_p->block_edge.block_edge_geometry, &exp_block_size);
    fbe_block_transport_get_physical_block_size(dev_info_p->block_edge.block_edge_geometry, &imp_block_size);

    fbe_status = fbe_payload_block_build_operation(block_operation_p,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED,
                                                   lba,
                                                   blocks,
                                                   exp_block_size,
                                                   imp_block_size,
                                                   NULL/*HACK for now, we don't need it*/);

    sg_p = (fbe_sg_element_t *)sep_shim_io_struct->context;
    fbe_status = fbe_payload_ex_set_sg_list(sep_payload_p, sg_p, 0);

    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              dev_info_p->bvd_object_id); 

    /* Figure out the IOCTL */
    priority = KeyToPacketPriority((fbe_u64_t)EmcpalIrpParamGetReadKey(pIoStackLocation));
    fbe_transport_set_packet_priority(packet_p, priority);

    /*make sure the packet has our edge*/
    packet_p->base_edge = &dev_info_p->block_edge.base_edge;

     //Store context for cancellation
    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = (PVOID)packet_p;  

    old_cancel_routine = EmcpalIrpCancelRoutineSet(PIrp, sep_handle_cancel_irp_function);
    if (EmcpalIrpIsCancelInProgress(PIrp)){
      old_cancel_routine = EmcpalIrpCancelRoutineSet(PIrp, NULL);
      EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_CANCELLED); 
      EmcpalExtIrpInformationFieldSet(PIrp, 0);
      //clean up context for cancellation
      EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
      EmcpalIrpCompleteRequest(PIrp);
      fbe_sep_shim_return_io_structure(sep_shim_io_struct);
      return EMCPAL_STATUS_CANCELLED;
    }

    fbe_transport_set_completion_function(packet_p, 
                                          fbe_sep_shim_process_dca_write_completion, 
                                          sep_shim_io_struct);

    fbe_sep_shim_send_io(packet_p);

    return EMCPAL_STATUS_PENDING;
}

/******************************************************************************
*            fbe_sep_shim_process_dca_write_completion ()
*******************************************************************************
*
*  Description: This is callback when the write gets completed and we complete
*  the IRP back to the client
* 
*  Inputs: packet  - pointer to the original packet
*          context - Context
*
*  Return Value: 
*   success or failure
*
*  History:
*   06/29/12 - Ashok Tamilarasan - Created     
******************************************************************************/
static fbe_status_t fbe_sep_shim_process_dca_write_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sep_shim_io_struct_t *				sep_shim_io_struct = (fbe_sep_shim_io_struct_t *)context;
    PEMCPAL_IRP                             PIrp = sep_shim_io_struct->associated_io;
    fbe_payload_ex_t *                      payload = NULL;
    fbe_payload_block_operation_t *         block_operation = NULL;
    fbe_status_t							fbe_status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bool_t                              io_failed;

    payload = fbe_transport_get_payload_ex (packet);
    block_operation = fbe_payload_ex_get_block_operation(payload);
    fbe_status = fbe_transport_get_status_code(packet);

    /* Translate the fbe status to an IRP status.
     */
    fbe_sep_shim_translate_status(packet, block_operation, PIrp, fbe_status, &io_failed);

    fbe_payload_ex_release_block_operation(payload, block_operation);/*don't need it anymore*/

    /*we might be completing here at DPC level so we have to break the context before returning the IRP,
    some layered apps might not support running at this level*/
    return fbe_sep_shim_continue_completion(packet, sep_shim_io_struct, io_failed);
}

/******************************************************************************
*            fbe_sep_shim_setup_dca_buffer ()
*******************************************************************************
*
*  Description: This function sets up the DCA buffer from the memory that we got
*
* 
*  Inputs: sep_shim_io_struct  - pointer to the original IO struct
*          memory_request - Pointer to the memory request that has the buffer we need
*
*  Return Value: 
*   success or failure
*
*  History:
*   06/29/12 - Ashok Tamilarasan - Created     
******************************************************************************/
static fbe_status_t fbe_sep_shim_setup_dca_buffer(fbe_sep_shim_io_struct_t * sep_shim_io_struct,
                                                  fbe_memory_request_t * memory_request)
{
    fbe_memory_header_t *	 	master_memory_header = NULL;
    PEMCPAL_IRP 				PIrp = NULL;
    fbe_sg_element_t *          sg_list_p;
    fbe_memory_header_t *       current_memory_header = NULL;
    fbe_u32_t					transfer_size = 0;
    fbe_u8_t *                  buffer_p = NULL;
    fbe_u32_t                   sg_list_size = 0;
    fbe_u32_t                   current_size = 0;
    fbe_u32_t                   chunks_used = 0;
    PEMCPAL_IO_STACK_LOCATION 	pIoStackLocation = NULL;

    if ((sep_shim_io_struct == NULL) || (sep_shim_io_struct->associated_io == NULL)){
        /* This should never happen.*/        
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s:Invalid Parameter sep_shim_io_struct 0x%x\n",
                            __FUNCTION__, (unsigned int)sep_shim_io_struct);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    PIrp = (PEMCPAL_IRP)sep_shim_io_struct->associated_io;

    /* Handle case where allocation fails */
    if (fbe_memory_request_is_allocation_complete(memory_request) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR,
                           "%s: Memory allocation failed. request state: %d \n",
                           __FUNCTION__, memory_request->request_state);
        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_NO_MEMORY);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    master_memory_header = fbe_memory_get_first_memory_header(memory_request);
    current_memory_header = master_memory_header;

    /* number_of_chunks includes the one for the dca arg, but sg_list need one entry for termination */
    sg_list_size = (master_memory_header->number_of_chunks+1) * sizeof(fbe_sg_element_t);
    /* round the sg list size to align with 64-bit boundary. */
    sg_list_size += (sg_list_size % sizeof(fbe_u64_t));

    /* find the last chunk */
    for (chunks_used = 0; chunks_used < master_memory_header->number_of_chunks - 1; chunks_used ++)
    {
        current_memory_header = current_memory_header->u.hptr.next_header;
    }

    /* carve dca arg from the end of the last chunk */
    buffer_p = current_memory_header->data + master_memory_header->memory_chunk_size - sizeof(FLARE_DCA_ARG);
    sep_shim_io_struct->flare_dca_arg = (FLARE_DCA_ARG *)buffer_p;

    /* carve out sg_list memory above that */
    buffer_p -= sg_list_size;
    sg_list_p = (fbe_sg_element_t *) buffer_p;
    sep_shim_io_struct->context = (void *)sg_list_p;

    /* Figure out the IOCTL */
    pIoStackLocation = EmcpalIrpGetCurrentStackLocation(PIrp);
    
    /* 512 to 520 */
    transfer_size = EmcpalIrpParamGetWriteLength(pIoStackLocation)/ FBE_BYTES_PER_BLOCK * FBE_BE_BYTES_PER_BLOCK;

    /* get the next memory header. */
    current_memory_header = master_memory_header;
    while ((transfer_size > 0)&& (current_memory_header!=NULL))
    {
        current_size = master_memory_header->memory_chunk_size;
        if (current_size > transfer_size)
        {
            current_size = transfer_size;
        }
        buffer_p = current_memory_header->data;
        fbe_sg_element_init(sg_list_p, current_size, buffer_p);
        fbe_sg_element_increment(&sg_list_p);

        transfer_size -= current_size;
        /* next chunk */
        current_memory_header = current_memory_header->u.hptr.next_header;
    }
    fbe_sg_element_terminate(sg_list_p);

    if (transfer_size!=0)
    {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s:more memory needed %d\n",
                           __FUNCTION__, transfer_size);
        //clean up context for cancellation
        EmcpalIrpCancelRoutineSet (PIrp, NULL);
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_NO_MEMORY);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);

        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return FBE_STATUS_GENERIC_FAILURE;

    }
    return FBE_STATUS_OK;
}

/******************************************************************************
*            fbe_sep_shim_calculate_dca_allocation_chunks ()
*******************************************************************************
*
*  Description: This function calculates the total chunks and the chunk size 
* required for the DCA operation including the memory for the data transfer and
* other associated tracking structure(SG and DCA structure)
*
* 
*  Inputs: blocks  - Total data blocks to be read or written
*          chunks - Number of chunks required to satisfy the request
*          chunk_size - Chunk size
*
*  Return Value: 
*   success or failure
*
*  History:
*   06/29/12 - Ashok Tamilarasan - Created     
******************************************************************************/
fbe_u32_t fbe_sep_shim_calculate_dca_allocation_chunks(fbe_block_count_t blocks, 
                                                       fbe_u32_t *chunks, 
                                                       fbe_memory_chunk_size_t *chunk_size)
{
    fbe_u32_t                           number_of_chunks = 0;
    fbe_u32_t                           number_of_data_chunks = 0;
    fbe_memory_chunk_size_t             size;
    fbe_memory_chunk_size_block_count_t chunk_blocks;
    fbe_u32_t                           sg_list_size = 0;
    fbe_u64_t                           total_transfer_size_with_ts = 0;


    if(blocks <= FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_1)
    {
        size = FBE_MEMORY_CHUNK_SIZE_FOR_1_BLOCK_IO;
        chunk_blocks = FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_1;
    }
    else if (blocks > FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_PACKET)
    {
        size = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO;
        chunk_blocks = FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64;
    }
    else
    {
        size = FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;
        chunk_blocks = FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_PACKET;
    }

    // 1. number_of_chunks need to enough to hold all blocks
    if((blocks/chunk_blocks) > FBE_U32_MAX)
    {
        /* we do not expect the number of chunks to cross 32 bit limit
          */
        return FBE_STATUS_GENERIC_FAILURE;
    }

    number_of_data_chunks = (fbe_u32_t)(blocks / chunk_blocks);

    if (blocks % chunk_blocks)
    {
        number_of_data_chunks ++; //one more chunk for the leftover
    }
    // 2. size the sg_list, enough to hold one extra chunk
    sg_list_size = (sizeof(fbe_sg_element_t) * (number_of_data_chunks+2));
    // round up to align with 64-bit boundary.
    sg_list_size += (sg_list_size % sizeof(fbe_u64_t));

    /* we need to add tracking structure and sg element to the total size for memory needed*/
    total_transfer_size_with_ts = (blocks * FBE_BE_BYTES_PER_BLOCK) + sg_list_size + sizeof(FLARE_DCA_ARG);

    number_of_chunks = (fbe_u32_t) total_transfer_size_with_ts / size;

    if (total_transfer_size_with_ts % size)
    {
        number_of_chunks ++; //one more chunk for the leftover
    }

    *chunks = number_of_chunks;
    *chunk_size = size; 

    return (FBE_STATUS_OK);
}

/******************************************************************************
*            fbe_sep_shim_init_waiting_request_data ()
*******************************************************************************
*
*  Description: Initializes the data structure to track the IO Requests waiting
* for memory to get started
*
*  Return Value: 
*   None
*
*  History:
*   08/22/12 - Ashok Tamilarasan - Created     
******************************************************************************/
void fbe_sep_shim_init_waiting_request_data(void)
{
    fbe_u32_t cpu_count = fbe_get_cpu_count();
    fbe_u32_t i;
    EMCPAL_STATUS           nt_status;

    for (i = 0; i < cpu_count; i++)
    {
        ListOfIRPsInitialize(&fbe_waiting_irps_info[i].fbe_waiting_irps_queue);
        fbe_spinlock_init(&fbe_waiting_irps_info[i].fbe_waiting_irps_lock);
        /*initialize the semaphore that will control the thread*/
        fbe_semaphore_init(&fbe_waiting_irps_info[i].fbe_waiting_irps_event, 0, 0x0FFFFFFF);
        fbe_semaphore_init(&fbe_waiting_irps_info[i].fbe_waiting_irps_thread_semaphore, 0, 1);

        fbe_waiting_irps_info[i].fbe_waiting_irps_thread_flag = FBE_WAITING_IRPS_THREAD_RUN;
        fbe_waiting_irps_info[i].cpu_id = i;

        /*start the thread that will execute updates from the queue*/
        nt_status = fbe_thread_init(&fbe_waiting_irps_info[i].fbe_waiting_irps_thread_handle, 
                                    "fbe_waiting_irps", 
                                    fbe_waiting_irps_thread_func, &fbe_waiting_irps_info[i]);
    }
}

/******************************************************************************
*            fbe_sep_shim_is_waiting_request_queue_empty ()
*******************************************************************************
*
*  Description: Checks if there are any requests already waiting in the queue
* associated with the core for memory
*
*  Inputs: cpu_id - Core ID to check if there are already waiting requests
*
*  Return Value: 
*   TRUE - If there are no waiting requests. FALSE - Otherwise
*
*  History:
*   08/22/12 - Ashok Tamilarasan - Created     
******************************************************************************/
fbe_bool_t fbe_sep_shim_is_waiting_request_queue_empty(fbe_u32_t cpu_id)
{
    fbe_bool_t queue_empty;

    fbe_spinlock_lock(&fbe_waiting_irps_info[cpu_id].fbe_waiting_irps_lock);
    queue_empty = ListOfIRPsIsEmpty(&fbe_waiting_irps_info[cpu_id].fbe_waiting_irps_queue);
    fbe_spinlock_unlock(&fbe_waiting_irps_info[cpu_id].fbe_waiting_irps_lock);
    
    return queue_empty;
}

/******************************************************************************
*            fbe_sep_shim_enqueue_request ()
*******************************************************************************
*
*  Description: Enqueues the request to the queue associated with the core. This
*  will be dequeued when the memory is available or request is cancelled
* 
*  Inputs: 
*   io_request - IO Request to be enqueued
*   cpu_id - Core # which this request come on. 

*  Return Value: 
*   None
*
*  History:
*   08/22/12 - Ashok Tamilarasan - Created     
******************************************************************************/
fbe_status_t fbe_sep_shim_enqueue_request(void *io_request, fbe_u32_t cpu_id)
{
    PEMCPAL_IRP PIrp = (PEMCPAL_IRP) io_request;
    PEMCPAL_IRP_CANCEL_FUNCTION         cancel_not_called = NULL;

    fbe_spinlock_lock(&fbe_waiting_irps_info[cpu_id].fbe_waiting_irps_lock);

    /* First set the cancel routine before queueing to handle cancellation of IRPs
     */
    EmcpalIrpCancelRoutineSet(PIrp, fbe_sep_shim_get_io_structure_cancel_irp);

    /* Check if the IRP is in the process of getting cancelled...*/
    if (EmcpalIrpIsCancelInProgress(PIrp))
    {
        /* ....The IRP is getting cancelled */

        /* Clear the cancel routine*/
        cancel_not_called = EmcpalIrpCancelRoutineSet(PIrp, NULL);

        /* Cancel routine is not called yet and so cancel it here */
        fbe_spinlock_unlock(&fbe_waiting_irps_info[cpu_id].fbe_waiting_irps_lock);
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_CANCELLED); 
        //clean up context for cancellation
        EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        return FBE_STATUS_CANCELED;
    }
    ListOfIRPsEnqueue(&fbe_waiting_irps_info[cpu_id].fbe_waiting_irps_queue, PIrp);
    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = 
        (PVOID)((((fbe_u64_t )(fbe_transport_get_coarse_time()))<<32) | cpu_id);
    EmcpalIrpMarkPending(PIrp);
    fbe_sep_shim_increment_wait_stats(cpu_id);
    fbe_spinlock_unlock(&fbe_waiting_irps_info[cpu_id].fbe_waiting_irps_lock);
    return FBE_STATUS_OK;
}

/******************************************************************************
*            fbe_sep_shim_wake_waiting_request ()
*******************************************************************************
*
*  Description: Resources are available and so kick of the thread can process
* the request on the specified core
* 
*  Inputs: 
*   cpu_id - Core # which has free resources. 

*  Return Value: 
*   None
*
*  History:
*   08/22/12 - Ashok Tamilarasan - Created     
******************************************************************************/
void fbe_sep_shim_wake_waiting_request(fbe_u32_t cpu_id)
{
    fbe_semaphore_release(&fbe_waiting_irps_info[cpu_id].fbe_waiting_irps_event, 0, 1, FBE_FALSE);
}

/******************************************************************************
*            fbe_waiting_irps_thread_func ()
*******************************************************************************
*
*  Description: This is the thread that monitors the queue of waiting requests
* 
*  Inputs: 
*   context - Pointer to the caller's context for this thread

*  Return Value: 
*   None
*
*  History:
*   08/22/12 - Ashok Tamilarasan - Created     
******************************************************************************/
static void fbe_waiting_irps_thread_func(void * context)
{
    EMCPAL_STATUS           nt_status;
    fbe_cpu_affinity_mask_t affinity = 0x1;
    fbe_waiting_irps_info_t *fbe_waiting_irps_info_context;
    fbe_u32_t cpu_id;

    fbe_waiting_irps_info_context = (fbe_waiting_irps_info_t *) context;

    cpu_id = fbe_waiting_irps_info_context->cpu_id;

    /* Affine the thread */
    fbe_thread_set_affinity(&fbe_waiting_irps_info_context->fbe_waiting_irps_thread_handle, 
                            affinity << cpu_id);

    FBE_UNREFERENCED_PARAMETER(context);
    while(1)    
    {
        /*block until someone takes down the semaphore*/
        nt_status = fbe_semaphore_wait(&fbe_waiting_irps_info_context->fbe_waiting_irps_event, NULL);
        if(fbe_waiting_irps_info_context->fbe_waiting_irps_thread_flag == FBE_WAITING_IRPS_THREAD_RUN) {
            /*check if there are any notifictions waiting on the queue*/
            fbe_waiting_irps_dispatch_queue(cpu_id);
        } else {
            /* The thread is being terminated and so break out of the loop */
            break;
        }
    }

    fbe_waiting_irps_info_context->fbe_waiting_irps_thread_flag = FBE_WAITING_IRPS_THREAD_DONE;
    fbe_semaphore_release(&fbe_waiting_irps_info_context->fbe_waiting_irps_thread_semaphore, 0, 1, FBE_FALSE);

    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

/******************************************************************************
*            fbe_waiting_irps_dispatch_queue ()
*******************************************************************************
*
*  Description: Dispatcher function that gets called when resources are available
*  This then dequeues the request that was waiting for memory, gets the memory
*  and starts processing
* 
*  Inputs: 
*   cpu_id - core which has resources available

*  Return Value: 
*   None
*
*  History:
*   08/22/12 - Ashok Tamilarasan - Created     
******************************************************************************/
static void  fbe_waiting_irps_dispatch_queue(fbe_u32_t cpu_id)
{
    PEMCPAL_DEVICE_OBJECT		PDeviceObject;
    PEMCPAL_IO_STACK_LOCATION   pIoStackLocation = NULL;
    fbe_sep_shim_io_struct_t *	io_struct;
    PEMCPAL_IRP PIrp;
    UCHAR                       majorFunction = 0;
    PEMCPAL_IRP_CANCEL_FUNCTION         old_cancel_routine = NULL;

    fbe_spinlock_lock(&fbe_waiting_irps_info[cpu_id].fbe_waiting_irps_lock);
    while(!ListOfIRPsIsEmpty(&fbe_waiting_irps_info[cpu_id].fbe_waiting_irps_queue))
    {
        /* Now that we have got the request structure remove the IRP from 
         * the waiting queue*/
        PIrp = ListOfIRPsDequeue(&fbe_waiting_irps_info[cpu_id].fbe_waiting_irps_queue);

        /* Clear the cancel routine that we established*/
        old_cancel_routine = EmcpalIrpCancelRoutineSet(PIrp, NULL);
        if(EmcpalIrpIsCancelInProgress(PIrp)) {
            /* This irp is cancelled after we enqueue it, we need to complete it here */
            fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "%s: irp %p has been cancelled from core %d\n",
                               __FUNCTION__, PIrp, cpu_id);
            fbe_sep_shim_decrement_wait_stats(cpu_id);
            EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
            EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_CANCELLED); 
            EmcpalExtIrpInformationFieldSet(PIrp, 0);
            EmcpalIrpCompleteRequest(PIrp);
            continue;  /* let's move on to the next */
        }
        
        /* There are some waiting requests. First get the memory */
        io_struct = fbe_sep_shim_process_io_structure_request(cpu_id, PIrp);

        /* If we did not get the memory, then break out. We will come back
         * here when we get some memory
         */
        if(io_struct == NULL) 
        {
            EmcpalIrpCancelRoutineSet(PIrp, fbe_sep_shim_get_io_structure_cancel_irp);
            /* can't process the IRP yet, put it back to the queue, back to waiting */
            ListOfIRPsRequeue(&fbe_waiting_irps_info[cpu_id].fbe_waiting_irps_queue, PIrp);
            fbe_spinlock_unlock(&fbe_waiting_irps_info[cpu_id].fbe_waiting_irps_lock);
            return;
        }

        fbe_sep_shim_decrement_wait_stats(cpu_id);

        fbe_spinlock_unlock(&fbe_waiting_irps_info[cpu_id].fbe_waiting_irps_lock);
    
        io_struct->coarse_wait_time = (fbe_u32_t)(((fbe_u64_t)(EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0]) >> 32) & 0xFFFFFFFF);


        /* Kick start the request */
        pIoStackLocation = EmcpalIrpGetCurrentStackLocation(PIrp);
        majorFunction = EmcpalExtIrpStackMajorFunctionGet(pIoStackLocation);
        PDeviceObject = EmcpalIrpStackGetCurrentDeviceObject(pIoStackLocation);
        if ((majorFunction == EMCPAL_IRP_TYPE_CODE_READ) || (majorFunction == EMCPAL_IRP_TYPE_CODE_WRITE)){
            fbe_sep_shim_start_asynch_read_write_irp(io_struct, PDeviceObject, PIrp);
        }else{
            fbe_sep_shim_start_asynch_io_irp(io_struct, PDeviceObject, PIrp);
        }
        fbe_spinlock_lock(&fbe_waiting_irps_info[cpu_id].fbe_waiting_irps_lock);
    }
    fbe_spinlock_unlock(&fbe_waiting_irps_info[cpu_id].fbe_waiting_irps_lock);
}

/******************************************************************************
*            fbe_sep_shim_destroy_waiting_requests ()
*******************************************************************************
*
*  Description: Tear down all the data structure that is used to manage the
*  requests that was waiting for memory
* 
*  Inputs: 
*   None 
*  Return Value: 
*   None
*
*  History:
*   08/22/12 - Ashok Tamilarasan - Created     
******************************************************************************/
void fbe_sep_shim_destroy_waiting_requests(void)
{
    fbe_u32_t cpu_count = fbe_get_cpu_count();
    fbe_u32_t i;
    PEMCPAL_IRP PIrp;

    for (i = 0; i < cpu_count; i++)
    {
        fbe_waiting_irps_info_t *info_p = &fbe_waiting_irps_info[i];

        fbe_spinlock_lock(&info_p->fbe_waiting_irps_lock);
        while (!ListOfIRPsIsEmpty(&info_p->fbe_waiting_irps_queue))
        {
            /* Return all the IRPs back to the caller
             */
            PIrp = ListOfIRPsDequeue(&info_p->fbe_waiting_irps_queue);
            fbe_sep_shim_decrement_wait_stats(i);
            //clean up context for cancellation
            EmcpalIrpCancelRoutineSet (PIrp, NULL);
            EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = NULL;    
            EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
            EmcpalExtIrpInformationFieldSet(PIrp, 0);
            EmcpalIrpCompleteRequest(PIrp);
        }
        fbe_spinlock_unlock(&info_p->fbe_waiting_irps_lock);
        info_p->fbe_waiting_irps_thread_flag = FBE_WAITING_IRPS_THREAD_STOP;
        fbe_semaphore_release(&info_p->fbe_waiting_irps_event, 0, 1, FBE_FALSE);

        /*wait for it to end*/
        fbe_semaphore_wait_ms(&info_p->fbe_waiting_irps_thread_semaphore, 2000);

        if (FBE_WAITING_IRPS_THREAD_DONE == info_p->fbe_waiting_irps_thread_flag) {
            fbe_thread_wait(&info_p->fbe_waiting_irps_thread_handle);
        } else {
            fbe_sep_shim_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                          "%s: waiting irps thread (%p) on cpu %d did not exit!\n",
                          __FUNCTION__,
                          info_p->fbe_waiting_irps_thread_handle.thread, i);
        }
        fbe_semaphore_destroy(&info_p->fbe_waiting_irps_thread_semaphore);
        fbe_thread_destroy(&info_p->fbe_waiting_irps_thread_handle);

        fbe_spinlock_destroy(&info_p->fbe_waiting_irps_lock);
        fbe_semaphore_destroy(&info_p->fbe_waiting_irps_event);
    }
}   
