/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_sep_shim_main_kernel.c
 ***************************************************************************
 *
 *  Description
 *      Kernel implementation for the SEP shim
 *      
 *
 *  History:
 *      06/24/09    sharel - Created
 *    
 ***************************************************************************/
#include <ntddk.h>
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_trace_interface.h"
#include "fbe/fbe_library_interface.h"
#include "fbe_service_manager.h"
#include "fbe_topology.h"
#include "fbe/fbe_queue.h"
#include "fbe_sep_shim_private_interface.h"
#include "fbe_sep_shim_private_ioctl.h"
#include "fbe/fbe_sep.h"
#include "flare_export_ioctls.h"
#include "fbe/fbe_memory.h"
#include "flare_ioctls.h"

/**************************************
				Local variables
**************************************/
#define FBE_SEP_SHIM_IOCTL_THEAD_MAX 2
/*******************************************
				Local functions
********************************************/
typedef enum fbe_sep_shim_ioctl_run_queue_thread_flag_e{
    FBE_SEP_SHIM_IOCTL_RUN_QUEUE_THREAD_RUN,
    FBE_SEP_SHIM_IOCTL_RUN_QUEUE_THREAD_STOP,
    FBE_SEP_SHIM_IOCTL_RUN_QUEUE_THREAD_DONE
}fbe_sep_shim_ioctl_run_queue_thread_flag_t;

static fbe_spinlock_t		fbe_sep_shim_ioctl_run_queue_lock; 
static fbe_queue_head_t	    fbe_sep_shim_ioctl_run_queue;

static fbe_thread_t			fbe_sep_shim_ioctl_run_queue_thread_handle[FBE_SEP_SHIM_IOCTL_THEAD_MAX];
static fbe_sep_shim_ioctl_run_queue_thread_flag_t  fbe_sep_shim_ioctl_run_queue_thread_flag;

static fbe_semaphore_t fbe_sep_shim_ioctl_run_queue_event;
static fbe_u64_t fbe_sep_shim_ioctl_run_queue_counter;

/* Forward declaration */ 
static void fbe_sep_shim_ioctl_run_queue_thread_func(void *context);
static fbe_status_t fbe_sep_shim_init_ioctl_handler(void);
static fbe_status_t fbe_sep_shim_destroy_ioctl_handler(void);


/*********************************************************************
 *            fbe_sep_shim_init ()
 *********************************************************************
 *
 *  Description: Initialize the sep shim
 *
 *	Inputs: 
 *
 *  Return Value: 
 *
 *    
 *********************************************************************/
fbe_status_t fbe_sep_shim_init (void)
{
	fbe_sep_shim_init_io_memory();

	fbe_sep_shim_init_ioctl_handler();

	/*in kernel, we want to connect to the background activity manager that will supply and
	external control of the bavkground services*/
//	fbe_sep_shim_init_background_activity_manager_interface();

	return FBE_STATUS_OK;
}

EMCPAL_STATUS fbe_sep_shim_kernel_process_ioctl_irp(PEMCPAL_DEVICE_OBJECT  PDeviceObject, PEMCPAL_IRP	PIrp)
{
	EMCPAL_STATUS 			status = EMCPAL_STATUS_SUCCESS;
    PEMCPAL_IO_STACK_LOCATION 	pIoStackLocation = NULL;
    ULONG 				IoControlCode = 0;

	fbe_sep_shim_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s, entry.\n", __FUNCTION__);

	pIoStackLocation   = EmcpalIrpGetCurrentStackLocation(PIrp);

    /* Figure out the IOCTL */
    IoControlCode = EmcpalExtIrpStackParamIoctlCodeGet(pIoStackLocation);

    if (EmcpalExtDeviceTypeGet(PDeviceObject) != FBE_SEP_DEVICE_TYPE) {
		fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s:invalid device, type: %X\n", __FUNCTION__, EmcpalExtDeviceTypeGet(PDeviceObject));
		EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_BAD_DEVICE_TYPE);
		EmcpalExtIrpInformationFieldSet(PIrp, 0);
		EmcpalIrpCompleteRequest(PIrp);
		return EMCPAL_STATUS_BAD_DEVICE_TYPE;
	}

	/*IOCTLs might come at DPC so all of them have to be processed asynchronoulsy*/
	status = fbe_sep_shim_process_asynch_io_irp(PDeviceObject, PIrp);

    return status;
}

/*********************************************************************
*            fbe_sep_shim_kernel_process_internal_ioctl_irp ()
*********************************************************************
*
*  Description: process IO types IRPs
*
*	Inputs: PDeviceObject  - pointer to the device object
*			PIRP - IRP from top driver
*
*  Return Value: 
*	success or failure
*    
*********************************************************************/
EMCPAL_STATUS fbe_sep_shim_kernel_process_internal_ioctl_irp(PEMCPAL_DEVICE_OBJECT PDeviceObject, PEMCPAL_IRP PIrp)
{
	EMCPAL_STATUS 			status = EMCPAL_STATUS_SUCCESS;
    PEMCPAL_IO_STACK_LOCATION 	pIoStackLocation = NULL;
    fbe_u8_t 			operation = 0;

    pIoStackLocation   = EmcpalIrpGetCurrentStackLocation(PIrp);

    /* Figure out the IOCTL */
    operation = EmcpalExtIrpStackMinorFunctionGet(pIoStackLocation);

    if (EmcpalExtDeviceTypeGet(PDeviceObject) != FBE_SEP_DEVICE_TYPE) {
		fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s:invalid device, type: %X\n", __FUNCTION__, EmcpalExtDeviceTypeGet(PDeviceObject));
		EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_BAD_DEVICE_TYPE);
		EmcpalExtIrpInformationFieldSet(PIrp, 0);
		EmcpalIrpCompleteRequest(PIrp);
		return EMCPAL_STATUS_BAD_DEVICE_TYPE;
	}

    switch ( operation )
    {
	case FLARE_SGL_READ:
		status = fbe_sep_shim_process_asynch_io_irp(PDeviceObject, PIrp);
		break;

	case FLARE_SGL_WRITE:
		status = fbe_sep_shim_process_asynch_io_irp(PDeviceObject, PIrp);
		break;	

	case FLARE_DCA_READ:
		status = fbe_sep_shim_process_asynch_io_irp(PDeviceObject, PIrp);
		break;

	case FLARE_DCA_WRITE:
		status = fbe_sep_shim_process_asynch_io_irp(PDeviceObject, PIrp);
		break;

	default: 
		fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s:invalid MinorFunction: %X\n",__FUNCTION__, operation);
		EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_INVALID_DEVICE_REQUEST);
		EmcpalExtIrpInformationFieldSet(PIrp, 0);
		status = EMCPAL_STATUS_INVALID_DEVICE_REQUEST;
		EmcpalIrpCompleteRequest(PIrp);
		break;
	
	}

    return status;
}

EMCPAL_STATUS fbe_sep_shim_kernel_process_read_irp(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP	PIrp)
{
	EMCPAL_STATUS 			status = EMCPAL_STATUS_SUCCESS;

	status = fbe_sep_shim_process_asynch_read_write_irp(PDeviceObject,PIrp);

	return status;
}

EMCPAL_STATUS fbe_sep_shim_kernel_process_write_irp(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP	PIrp)
{
	EMCPAL_STATUS 			status = EMCPAL_STATUS_SUCCESS;

	status = fbe_sep_shim_process_asynch_read_write_irp(PDeviceObject,PIrp);

	return status;
}

fbe_status_t fbe_sep_shim_destroy()
{
	fbe_sep_shim_destroy_io_memory();
	fbe_sep_shim_destroy_ioctl_handler();
//	fbe_sep_shim_destroy_background_activity_manager_interface();
	return FBE_STATUS_OK;
}

/*********************************************************************
*            fbe_sep_shim_ioctl_run_queue_push_packet ()
*********************************************************************
*
*  Description: This function pushes the packet to the ioctl handler queue
*  and sets the event for the handler thread to be woken up.
*
*  Inputs: packet - Pointer to the packet that has the ioctl information
*
*  Return Value: 
*   fbe status
*
*  History:
*   12/20/12 - Ashok Tamilarasan - Created     
*********************************************************************/
fbe_status_t fbe_sep_shim_ioctl_run_queue_push_packet(fbe_packet_t * packet)
{
	if(packet->packet_state == FBE_PACKET_STATE_QUEUED){
		fbe_sep_shim_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
				   "%s Invalid packet_state\n", __FUNCTION__);
	}

	fbe_spinlock_lock(&fbe_sep_shim_ioctl_run_queue_lock);
	fbe_queue_push(&fbe_sep_shim_ioctl_run_queue, &packet->queue_element);
	fbe_semaphore_release(&fbe_sep_shim_ioctl_run_queue_event,0, 1, FALSE);
	fbe_sep_shim_ioctl_run_queue_counter++;
	fbe_spinlock_unlock(&fbe_sep_shim_ioctl_run_queue_lock);

	return FBE_STATUS_OK;
}

/*********************************************************************
*            fbe_sep_shim_ioctl_run_queue_thread_func ()
*********************************************************************
*
*  Description: This function is the handler for processing the IOCTLs.
*
*  Inputs: context - Unreferenceed parameter
*
*  Return Value: 
*   None
*
*  History:
*   12/20/12 - Ashok Tamilarasan - Created     
*********************************************************************/
static void 
fbe_sep_shim_ioctl_run_queue_thread_func(void * context)
{
    fbe_status_t status;
    fbe_packet_t * packet = NULL;
    fbe_cpu_affinity_mask_t affinity = 0x1;
    fbe_queue_head_t tmp_queue;
    fbe_queue_element_t * queue_element = NULL;
    fbe_u32_t thread_id = (fbe_u32_t)context;
	fbe_cpu_id_t cpu_id = thread_id; 

    fbe_thread_set_affinity(&fbe_sep_shim_ioctl_run_queue_thread_handle[thread_id], 
                            affinity << cpu_id);

    fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "%s entry, thrad ID %d, core %d\n", __FUNCTION__, thread_id, cpu_id);

    fbe_queue_init(&tmp_queue);
    while (1)
    {
        fbe_semaphore_wait(&fbe_sep_shim_ioctl_run_queue_event, NULL);
        if (fbe_sep_shim_ioctl_run_queue_thread_flag == FBE_SEP_SHIM_IOCTL_RUN_QUEUE_THREAD_RUN)
        {
            fbe_spinlock_lock(&fbe_sep_shim_ioctl_run_queue_lock);
            
            while (queue_element = fbe_queue_pop(&fbe_sep_shim_ioctl_run_queue))
            {
	        fbe_queue_push(&tmp_queue, queue_element);
                if (fbe_sep_shim_ioctl_run_queue_counter == 0)
                {
                    fbe_sep_shim_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                       "%s queue corrupted\n", __FUNCTION__);
                }
                fbe_sep_shim_ioctl_run_queue_counter--;
            }
            fbe_spinlock_unlock(&fbe_sep_shim_ioctl_run_queue_lock);

            while (queue_element = fbe_queue_pop(&tmp_queue))
            {
                fbe_u64_t magic_number;

                magic_number = *(fbe_u64_t *)(((fbe_u8_t *)queue_element) + sizeof(fbe_queue_element_t));
                if (magic_number == FBE_MAGIC_NUMBER_BASE_PACKET)
                {
                    packet = fbe_transport_queue_element_to_packet(queue_element);
                    status = fbe_transport_get_status_code(packet);
                    /* We want the client to only see cancelled status. */
                    if (status == FBE_STATUS_CANCEL_PENDING)
                    {
                        fbe_transport_set_status(packet, FBE_STATUS_CANCELED, 0);
                    }
                    fbe_transport_complete_packet(packet);
                }
                else
                {
                    fbe_sep_shim_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                       "%s Invalid magic number %llX\n", __FUNCTION__, magic_number);
                }
            }
        }
        else
        {
            break;
        }
    }

    fbe_queue_destroy(&tmp_queue);

    fbe_sep_shim_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                       "%s done\n", __FUNCTION__);

    fbe_sep_shim_ioctl_run_queue_thread_flag = FBE_SEP_SHIM_IOCTL_RUN_QUEUE_THREAD_DONE;
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

/*********************************************************************
*            fbe_sep_shim_init_ioctl_handler ()
*********************************************************************
*
*  Description: This function inits the handler for processing the IOCTLs.
*
*  Inputs: context - None
*
*  Return Value: 
*   fbe status
*
*  History:
*   12/20/12 - Ashok Tamilarasan - Created     
*********************************************************************/
static fbe_status_t 
fbe_sep_shim_init_ioctl_handler(void)
{
	fbe_ptrhld_t thread_id = 0;

    fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO,
                       "%s\n", __FUNCTION__);

    fbe_queue_init(&fbe_sep_shim_ioctl_run_queue);
    fbe_semaphore_init(&fbe_sep_shim_ioctl_run_queue_event, 0, FBE_SEMAPHORE_MAX);
    fbe_spinlock_init(&fbe_sep_shim_ioctl_run_queue_lock);
    fbe_sep_shim_ioctl_run_queue_counter= 0;
    fbe_sep_shim_ioctl_run_queue_thread_flag = FBE_SEP_SHIM_IOCTL_RUN_QUEUE_THREAD_RUN;

	for (thread_id = 0; thread_id < FBE_SEP_SHIM_IOCTL_THEAD_MAX; thread_id++) {
		fbe_thread_init(&fbe_sep_shim_ioctl_run_queue_thread_handle[thread_id], "fbe_sep_shim_ioctl_runq", fbe_sep_shim_ioctl_run_queue_thread_func, (void *)(fbe_ptrhld_t)thread_id);
	}
    return FBE_STATUS_OK;
}

/*********************************************************************
*            fbe_sep_shim_destroy_ioctl_handler ()
*********************************************************************
*
*  Description: This function destroys the handler for processing the IOCTLs.
*
*  Inputs: context - None
*
*  Return Value: 
*   fbe status
*
*  History:
*   12/20/12 - Ashok Tamilarasan - Created     
*********************************************************************/
static fbe_status_t 
fbe_sep_shim_destroy_ioctl_handler(void)
{
	fbe_u32_t 	thread_id = 0;
    
    fbe_sep_shim_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                    "%s entry\n", __FUNCTION__);

	/* Stop thread's */
    fbe_sep_shim_ioctl_run_queue_thread_flag = FBE_SEP_SHIM_IOCTL_RUN_QUEUE_THREAD_STOP;

	/*twice so each thread will pick it up*/
    fbe_semaphore_release(&fbe_sep_shim_ioctl_run_queue_event,0, 1, FALSE);
	fbe_semaphore_release(&fbe_sep_shim_ioctl_run_queue_event,0, 1, FALSE);

    // notify all run queueu thread
	for (thread_id = 0; thread_id < FBE_SEP_SHIM_IOCTL_THEAD_MAX; thread_id++) {
		fbe_thread_wait(&fbe_sep_shim_ioctl_run_queue_thread_handle[thread_id]);
		fbe_thread_destroy(&fbe_sep_shim_ioctl_run_queue_thread_handle[thread_id]);
	}

    fbe_semaphore_destroy(&fbe_sep_shim_ioctl_run_queue_event);

    fbe_queue_destroy(&fbe_sep_shim_ioctl_run_queue);
    fbe_spinlock_destroy(&fbe_sep_shim_ioctl_run_queue_lock);

    return FBE_STATUS_OK;
}

