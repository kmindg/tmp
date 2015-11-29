/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_rdgen_kernel_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains kernel related functionality for rdgen.
 *  This primarily has code for using the IRP interface.
 *  
 *
 * @version
 *   3/12/2012:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#ifdef C4_INTEGRATED
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#endif
#include "fbe/fbe_rdgen.h"
#include "fbe_rdgen_private.h"
#include "fbe/fbe_rdgen.h"
#include "fbe_database.h"
#include "fbe_rdgen_private_inlines.h"
#include "fbe_transport_memory.h"
#include "flare_export_ioctls.h"
#include "flare_ioctls.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static fbe_status_t fbe_rdgen_object_kernel_open(fbe_rdgen_object_t *object_p);
static fbe_status_t fbe_rdgen_object_kernel_get_capacity(fbe_rdgen_object_t *object_p);
static fbe_status_t fbe_rdgen_ts_send_to_bdev(fbe_rdgen_ts_t *ts_p,
                                  fbe_payload_block_operation_opcode_t opcode,
                                  fbe_lba_t lba,
                                  fbe_block_count_t blocks,
                                  fbe_bool_t b_is_read,
                                  fbe_payload_block_operation_flags_t payload_flags);

#define NUM_NANO_SECONDS           (1000L * 1000L * 1000L)

/*!*******************************************************************
 * @def FBE_RDGEN_SP_CACHE_NAME
 *********************************************************************
 * @brief The device name of sp cache, needed when allocating
 *        memory from SP cache.
 *
 *********************************************************************/
#define FBE_RDGEN_SP_CACHE_NAME "\\Device\\DRAMCache"

/*!*******************************************************************
 * @def  FBE_RDGEN_MEMORY_TAG
 *********************************************************************
 * @brief Tag we give to sp cache when we allocate memory.
 *
 *********************************************************************/
#define FBE_RDGEN_MEMORY_TAG 0x5244474E //"RDGN"

fbe_u32_t fbe_packet_priority_to_key(fbe_packet_priority_t priority)
{
    switch(priority)
    {
        case FBE_PACKET_PRIORITY_LOW: return KEY_PRIORITY_LOW;
        case FBE_PACKET_PRIORITY_NORMAL: return KEY_PRIORITY_NORMAL;
        case FBE_PACKET_PRIORITY_URGENT: return KEY_PRIORITY_URGENT;
    }
    return FBE_PACKET_PRIORITY_NORMAL;
}

/*!**************************************************************
 * fbe_rdgen_object_size_disk_object()
 ****************************************************************
 * @brief
 *  This opens the device and gets the capacity of the device.
 *
 * @param object_p - Rdgen's object struct.
 *
 * @return FBE_STATUS_OK
 *         Other status values indicate failure.
 *
 * @author
 *  3/17/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_object_size_disk_object(fbe_rdgen_object_t *object_p)
{
    fbe_status_t status;

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s opening %s\n", __FUNCTION__, object_p->device_name);

    status = fbe_rdgen_object_kernel_open(object_p);

    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }

    status = fbe_rdgen_object_kernel_get_capacity(object_p);

    if (status)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s device %s opened capacity: 0x%llx\n", __FUNCTION__,  
                                object_p->device_name, (unsigned long long)object_p->capacity);
        /* Fill out the rest of the object parameters.
         */
        object_p->block_size = FBE_BE_BYTES_PER_BLOCK;
        object_p->physical_block_size = FBE_BE_BYTES_PER_BLOCK;
        object_p->optimum_block_size = 1;
    }
    else
    {
        /* If the open fails, we are failed.
         */
        EmcpalExtDeviceClose(object_p->file_p);
        object_p->device_p = NULL;
    }

    return status;
}
/******************************************
 * end fbe_rdgen_object_size_disk_object()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_object_kernel_get_raid_info()
 ****************************************************************
 * @brief
 *  Issue the IOCTL_FLARE_GET_VOLUME_STATE to fetch our
 *  capacity information.
 *
 * @param object_p - The object that contains the device name.
 *
 * @return FBE_STATUS_OK
 *         FBE_STATUS_FAILED - Event cannot be created.
 *         FBE_STATUS_INSUFFICIENT_RESOURCES - no irp can be allocated
 *         FBE_STATUS_TIMEOUT - Took too long waiting for IRP to complete.
 *
 * @author
 *  4/18/2013 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_object_kernel_get_raid_info(fbe_rdgen_object_t *object_p)
{
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;
    PEMCPAL_IRP irp_p = NULL;
    EMCPAL_RENDEZVOUS_EVENT event;
    EMCPAL_IRP_STATUS_BLOCK ioStatus;
    EMCPAL_STATUS waitStatus      = EMCPAL_STATUS_UNSUCCESSFUL;
    EMCPAL_LARGE_INTEGER liTimeout = {0};
    LONG timeout_seconds = 30;
    PEMCPAL_DEVICE_OBJECT DevicePtr = object_p->device_p;
    FLARE_RAID_INFO raid_info;

    status = EmcpalRendezvousEventCreate(EmcpalDriverGetCurrentClientObject(), 
                                         &event, 
                                         "rdgen_get_size", 
                                         FALSE); 
    if (!EMCPAL_IS_SUCCESS(status))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s error creating EendezvousEvent 0x%x\n", __FUNCTION__, status);
        return FBE_STATUS_FAILED;
    }
    EmcpalRendezvousEventMustDestroy(&event);

    liTimeout.QuadPart = -((LONG)timeout_seconds * (NUM_NANO_SECONDS / 100L));

    irp_p = EmcpalExtIrpBuildIoctl(IOCTL_FLARE_GET_RAID_INFO,
                                    DevicePtr,
                                    NULL,     // No input buffer -- so length is zero too!
                                    0,
                                    &raid_info,
                                    sizeof(FLARE_RAID_INFO),
                                    FALSE,
                                    &event,
                                    &ioStatus);
    if (NULL == irp_p)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s irp ptr is null\n", __FUNCTION__);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    status = EmcpalExtIrpSendAsync(DevicePtr, irp_p);

    if (EMCPAL_STATUS_PENDING == status)
    {
        waitStatus = EmcpalRendezvousEventWait(&event, EMCPAL_NT_RELATIVE_TIME_TO_MSECS(&liTimeout));

        status = EmcpalExtIrpStatusBlockStatusFieldGet(&ioStatus);
    }

    if (EMCPAL_STATUS_TIMEOUT == waitStatus )
    {
        /* We took too long.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s %d sec timeout on get RAID_INFO\n", __FUNCTION__, timeout_seconds);
        return FBE_STATUS_TIMEOUT;
    }

    if (EMCPAL_IS_SUCCESS(status))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "raid_info: MaxQueueDepth: %u BytesPerSector: %u BytesPerSector: %u \n", 
                                 raid_info.MaxQueueDepth, raid_info.BytesPerSector, raid_info.BytesPerSector);
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "raid_info: Lun: %u GroupId: %u SectorsPerStripe: %u raid_info.StripeCount: %u\n", 
                                 raid_info.Lun, raid_info.GroupId, raid_info.SectorsPerStripe, raid_info.StripeCount);
        return FBE_STATUS_OK;
    }
    else
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s PAL status 0x%x on get capacity\n", __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
}
/******************************************
 * end fbe_rdgen_object_kernel_get_raid_info()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_object_kernel_get_capacity()
 ****************************************************************
 * @brief
 *  Issue the IOCTL_FLARE_GET_VOLUME_STATE to fetch our
 *  capacity information.
 *
 * @param object_p - The object that contains the device name.
 *
 * @return FBE_STATUS_OK
 *         FBE_STATUS_FAILED - Event cannot be created.
 *         FBE_STATUS_INSUFFICIENT_RESOURCES - no irp can be allocated
 *         FBE_STATUS_TIMEOUT - Took too long waiting for IRP to complete.
 *
 * @author
 *  3/17/2012 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_object_kernel_get_capacity(fbe_rdgen_object_t *object_p)
{
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;
    PEMCPAL_IRP irp_p = NULL;
    EMCPAL_RENDEZVOUS_EVENT event;
    EMCPAL_IRP_STATUS_BLOCK ioStatus;
    EMCPAL_STATUS waitStatus      = EMCPAL_STATUS_UNSUCCESSFUL;
    EMCPAL_LARGE_INTEGER liTimeout = {0};
    LONG timeout_seconds = 30;
    PEMCPAL_DEVICE_OBJECT DevicePtr = object_p->device_p;
    FLARE_VOLUME_STATE VolumeInfo;

    status = EmcpalRendezvousEventCreate(EmcpalDriverGetCurrentClientObject(), 
                                         &event, 
                                         "rdgen_get_size", 
                                         FALSE); 
    if (!EMCPAL_IS_SUCCESS(status))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s error creating EendezvousEvent 0x%x\n", __FUNCTION__, status);
        return FBE_STATUS_FAILED;
    }

    liTimeout.QuadPart = -((LONG)timeout_seconds * (NUM_NANO_SECONDS / 100L));

    irp_p = EmcpalExtIrpBuildIoctl(IOCTL_FLARE_GET_VOLUME_STATE,
                                    DevicePtr,
                                    NULL,     // No input buffer -- so length is zero too!
                                    0,
                                    &VolumeInfo,
                                    sizeof(FLARE_VOLUME_STATE),
                                    FALSE,
                                    &event,
                                    &ioStatus);
    if (NULL == irp_p)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s irp ptr is null\n", __FUNCTION__);
        EmcpalRendezvousEventDestroy(&event);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    status = EmcpalExtIrpSendAsync(DevicePtr, irp_p);

    if (EMCPAL_STATUS_PENDING == status)
    {
        waitStatus = EmcpalRendezvousEventWait(&event, EMCPAL_NT_RELATIVE_TIME_TO_MSECS(&liTimeout));

        status = EmcpalExtIrpStatusBlockStatusFieldGet(&ioStatus);
    }
    EmcpalRendezvousEventDestroy(&event);

    if (EMCPAL_STATUS_TIMEOUT == waitStatus )
    {
        /* We took too long.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s %d sec timeout on get capacity\n", __FUNCTION__, timeout_seconds);
        return FBE_STATUS_TIMEOUT;
    }

    if (EMCPAL_IS_SUCCESS(status))
    {
        object_p->capacity = VolumeInfo.capacity;
        /* Let's only trace this information if debug tracing is enabled.
         */
        if (fbe_rdgen_service_allow_trace(FBE_TRACE_LEVEL_DEBUG_LOW))
        {
            fbe_rdgen_object_kernel_get_raid_info(object_p);
        }
        return FBE_STATUS_OK;
    }
    else
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s PAL status 0x%x on get capacity\n", __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
}
/******************************************
 * end fbe_rdgen_object_kernel_get_capacity()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_object_kernel_open()
 ****************************************************************
 * @brief
 *  Opens the device object.
 * 
 *  Note that we do not do the open if it is already opened.
 *
 * @param object_p - The object that contains the device name.
 *
 * @return FBE_STATUS_OK
 *         FBE_STATUS_GENERIC_FAILURE
 *
 * @author
 *  3/17/2012 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_object_kernel_open(fbe_rdgen_object_t *object_p)
{
    EMCPAL_STATUS pal_status = EMCPAL_STATUS_SUCCESS;

    /* If the device is already open, don't open it again.
     */
    if (object_p->device_p != NULL)
    {
        return FBE_STATUS_OK;
    }

    /* Open the device stack.
     */
#ifdef ALAMOSA_WINDOWS_ENV
    pal_status = EmcpalExtDeviceOpen(object_p->device_name,
                                     FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                                     &object_p->file_p,
                                     &object_p->device_p);
#else
    pal_status = EmcpalExtDeviceOpen(object_p->device_name,
                                     FILE_ALL_ACCESS,
                                     &object_p->file_p,
                                     &object_p->device_p);
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - what should this be */

    if ( !EMCPAL_IS_SUCCESS(pal_status) )
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (object_p->device_p == NULL)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s device ptr is null status: 0x%x\n", __FUNCTION__, pal_status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* We use N+1 to be sure to include our level.
     */
    object_p->stack_size = EmcpalDeviceGetIrpStackSize(object_p->device_p)+ 1;

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_object_kernel_open()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_read_dca_callback()
 ****************************************************************
 * @brief
 *  This gets called when the client wants to push out the read data.
 *
 * @param irp_p - Current IRP doing callback.
 * @param dca_arg_p - The context from the server doing the callback.
 *  
 * @return None. 
 *
 * @author
 *  3/17/2012 - Created. Rob Foley
 *
 ****************************************************************/

VOID fbe_rdgen_ts_read_dca_callback(PEMCPAL_IRP irp_p, PFLARE_DCA_ARG dca_arg_p)
{
    dca_arg_p->DSCallback(EMCPAL_STATUS_SUCCESS, dca_arg_p->DSContext1);
    return;
}
/******************************************
 * end fbe_rdgen_ts_read_dca_callback()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_write_dca_callback()
 ****************************************************************
 * @brief
 *  This gets called when the client wants to pull in the write data.
 *
 * @param irp_p - Current IRP doing callback.
 * @param dca_arg_p - The context from the server doing the callback.
 *
 * @return None.   
 *
 * @author
 *  3/17/2012 - Created. Rob Foley
 *
 ****************************************************************/

VOID fbe_rdgen_ts_write_dca_callback(PEMCPAL_IRP irp_p, PFLARE_DCA_ARG dca_arg_p)
{
    dca_arg_p->DSCallback(EMCPAL_STATUS_SUCCESS, dca_arg_p->DSContext1);
    return;
}
/******************************************
 * end fbe_rdgen_ts_write_dca_callback()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_translate_irp_status()
 ****************************************************************
 * @brief
 *  Translate the IRP status into a packet status.
 *  We provide this mapping since the rest of rdgen is using
 *  the packet status value.
 *
 * @param ts_p - Tracking structure for this thread.
 * @param irp_p - Just completed IRP where we will pull the status.
 *
 * @return None
 *
 * @author
 *  3/17/2012 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_ts_translate_irp_status(fbe_rdgen_ts_t *ts_p, PEMCPAL_IRP irp_p)
{
    EMCPAL_STATUS status = EmcpalExtIrpStatusFieldGet(irp_p);
    ULONG_PTR information = EmcpalExtIrpInformationFieldGet(irp_p);
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s IRP status: 0x%x information: 0x%llx\n", __FUNCTION__, status, (unsigned long long)information);
    switch (status)
    {
        case EMCPAL_STATUS_SUCCESS:
            ts_p->last_packet_status.status = FBE_STATUS_OK;
            ts_p->last_packet_status.block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;
            ts_p->last_packet_status.block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
            break;
        case EMCPAL_STATUS_CANCELLED:
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s IRP cancelled status: 0x%x information: 0x%llx\n", __FUNCTION__, status, (unsigned long long)information);
            ts_p->last_packet_status.status = FBE_STATUS_OK;
            ts_p->last_packet_status.block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED;
            ts_p->last_packet_status.block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED;
            break;
        case EMCPAL_STATUS_DEVICE_NOT_READY:
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s IRP failed not ready status: 0x%x information: 0x%llx\n", __FUNCTION__, status, (unsigned long long)information);
            ts_p->last_packet_status.status = FBE_STATUS_BUSY;
            ts_p->last_packet_status.block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;
            ts_p->last_packet_status.block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
            break;
        case EMCPAL_STATUS_DEVICE_DOES_NOT_EXIST:
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s IRP failed no device status: 0x%x information: 0x%llx\n", __FUNCTION__, status, (unsigned long long)information);
            ts_p->last_packet_status.status = FBE_STATUS_FAILED;
            ts_p->last_packet_status.block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;
            ts_p->last_packet_status.block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
            break;
        case EMCPAL_STATUS_DISK_CORRUPT_ERROR:
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s IRP disk corrupt err status: 0x%x information: 0x%llx\n", __FUNCTION__, status, (unsigned long long)information);
            ts_p->last_packet_status.status = FBE_STATUS_OK;
            ts_p->last_packet_status.block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST;    
            ts_p->last_packet_status.block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR;
            break;
        case EMCPAL_STATUS_CRC_ERROR:
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s IRP failed crc status: 0x%x information: 0x%llx\n", __FUNCTION__, status, (unsigned long long)information);
            ts_p->last_packet_status.status = FBE_STATUS_OK;
            ts_p->last_packet_status.block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CRC_ERROR;
            ts_p->last_packet_status.block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
            break;
        case EMCPAL_STATUS_QUOTA_EXCEEDED:
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s IRP failed QUOTA_EXCEEDED status: 0x%x information: 0x%llx\n", __FUNCTION__, status, (unsigned long long)information);
            ts_p->last_packet_status.status = FBE_STATUS_OK;
            ts_p->last_packet_status.block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CONGESTED;
            ts_p->last_packet_status.block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
            break;
        case EMCPAL_STATUS_ALERTED:
            ts_p->last_packet_status.status = FBE_STATUS_OK;
            ts_p->last_packet_status.block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_STILL_CONGESTED;
            ts_p->last_packet_status.block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
            break;
        default:
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s unexpected IRP status: 0x%x information: 0x%x\n", __FUNCTION__, status, (unsigned int)information);
            ts_p->last_packet_status.status = FBE_STATUS_GENERIC_FAILURE;
            ts_p->last_packet_status.block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;
            ts_p->last_packet_status.block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST;
            break;
    };
}
/******************************************
 * end fbe_rdgen_ts_translate_irp_status()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_irp_completion()
 ****************************************************************
 * @brief
 *  This is our standard IRP completion function.
 *  Just translate the status from the IRP into our
 *  ts last_packet status.  Then wake up the ts.
 *
 * @param device_object_p - The device object we sent this IRP to.
 * @param irp_p - Current IRP that is completing.
 * @param context_p - Pointer to our tracking structure.
 *
 * @return EMCPAL_STATUS_MORE_PROCESSING_REQUIRED So the IRP will
 *         not get completed up to the next level.
 *
 * @author
 *  3/17/2012 - Created. Rob Foley
 *
 ****************************************************************/

EMCPAL_STATUS fbe_rdgen_ts_irp_completion(PEMCPAL_DEVICE_OBJECT device_object_p,
                                          PEMCPAL_IRP irp_p,
                                          PVOID context_p)
{
    fbe_rdgen_ts_t *ts_p = (fbe_rdgen_ts_t *)context_p;
    fbe_u32_t usec;

    /* We completed after many seconds.
     */
    usec = fbe_get_elapsed_microseconds(ts_p->last_send_time);
    if (usec >= FBE_RDGEN_SERVICE_TIME_MAX_MICROSEC)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, fbe_rdgen_object_get_object_id(ts_p->object_p),
                                "%s ts %p completed after %d usecs\n",
                                __FUNCTION__, ts_p, usec);
    }
    /* Update our response time statistics.
     */
    fbe_rdgen_ts_set_last_response_time(ts_p, usec);
    fbe_rdgen_ts_update_cum_resp_time(ts_p, usec);
    fbe_rdgen_ts_update_max_response_time(ts_p, usec);

    /* Save the status of the operation in the ts last packet status. 
     */ 
    fbe_rdgen_ts_translate_irp_status(ts_p, irp_p);

    if (ts_p->io_interface == FBE_RDGEN_INTERFACE_TYPE_IRP_MJ)
    {
        /* Free the MDL if we allocated one.
         */
        EmcpalIrpFinishIoBuffer(irp_p);
    }
    /* Simply put this on a thread.
     */
    fbe_rdgen_ts_enqueue_to_thread(ts_p);

    return EMCPAL_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_rdgen_ts_irp_completion()
 ******************************************/

/*!*******************************************************************
 * @var fbe_rdgen_read_dca_table
 *********************************************************************
 * @brief This is the global table we use for dca read callbacks.
 *
 *********************************************************************/
static DCA_TABLE fbe_rdgen_read_dca_table = 
{
    fbe_rdgen_ts_read_dca_callback,/* xfer callback */
    FBE_FALSE, /* FALSE to pass sg with 520s */
    0, /* resource pool only used when above == TRUE */
    FBE_FALSE, /* don't use virtual addrs in sgl, use physical instead. */
};

/*!*******************************************************************
 * @var fbe_rdgen_write_dca_table
 *********************************************************************
 * @brief This is the global table we use for dca write callbacks.
 *
 *********************************************************************/
static DCA_TABLE fbe_rdgen_write_dca_table = 
{
    fbe_rdgen_ts_write_dca_callback,/* xfer callback */
    FBE_FALSE, /* FALSE to pass sg with 520s */
    0, /* resource pool only used when above == TRUE */
    FBE_FALSE, /* don't use virtual addrs in sgl, use physical instead. */
};

/*!**************************************************************
 * fbe_rdgen_ts_send_dca()
 ****************************************************************
 * @brief
 *  Translate this TS into a DCA IRP.
 *
 * @param ts_p - Current tracking struct for thread.
 * @param opcode - The operation type to send.
 * @param lba - logical block address
 * @param blocks - Number of blocks to transfer.
 * @param b_is_read - TRUE if read FALSE if write.
 * @param payload_flags - Payload flags to get translated into irp and
 *                        sgl irp struct flags.            
 *
 * @return FBE_STATUS_OK
 *         FBE_STATUS_GENERIC_FAILURE
 *
 * @author
 *  3/17/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_ts_send_dca(fbe_rdgen_ts_t *ts_p,
                                   fbe_payload_block_operation_opcode_t opcode,
                                   fbe_lba_t lba,
                                   fbe_block_count_t blocks,
                                   fbe_bool_t b_is_read)
{
    fbe_packet_t *packet_p = NULL;
    PEMCPAL_IRP irp_p = NULL;
    EMCPAL_STATUS pal_status;
    fbe_u32_t irp_priority;

    /* Use the write packet for media modify operations.
     */
    if (fbe_payload_block_operation_opcode_is_media_modify(opcode))
    {
        packet_p = fbe_rdgen_ts_get_write_packet(ts_p);
    }
    else
    {
        packet_p = fbe_rdgen_ts_get_packet(ts_p);
    }
    fbe_rdgen_ts_set_packet_ptr(ts_p, packet_p);
    irp_p = (PEMCPAL_IRP)ts_p->packet_p;
    EmcpalIrpInitialize(irp_p, EmcpalIrpCalculateSize(ts_p->object_p->stack_size), ts_p->object_p->stack_size);

    if ((blocks * FBE_BYTES_PER_BLOCK) > FBE_U32_MAX)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s byte count for blocks is too big blocks: 0x%llx %s\n", 
                                __FUNCTION__, (unsigned long long)blocks, ts_p->object_p->device_name);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (b_is_read)
    {
        EmcpalIrpBuildDcaRequest(FBE_TRUE /* is read */,
                                 irp_p,
                                 (fbe_u32_t)(blocks * FBE_BYTES_PER_BLOCK),
                                 (lba * FBE_BYTES_PER_BLOCK),
                                 &fbe_rdgen_read_dca_table,
                                 ts_p);

        if (ts_p->request_p->specification.options & FBE_RDGEN_OPTIONS_USE_PRIORITY) {
            PEMCPAL_IO_STACK_LOCATION irp_stack_p = EmcpalIrpGetCurrentStackLocation(irp_p);
            irp_priority = fbe_packet_priority_to_key(ts_p->request_p->specification.priority);
            EmcpalIrpParamSetReadKey(irp_stack_p) = irp_priority;
        }
    }
    else
    {
        EmcpalIrpBuildDcaRequest(FBE_FALSE /* is not read */,
                                 irp_p,
                                 (fbe_u32_t)(blocks * FBE_BYTES_PER_BLOCK),
                                 (lba * FBE_BYTES_PER_BLOCK),
                                 &fbe_rdgen_write_dca_table,
                                 ts_p);

        if (ts_p->request_p->specification.options & FBE_RDGEN_OPTIONS_USE_PRIORITY) {
            PEMCPAL_IO_STACK_LOCATION irp_stack_p = EmcpalIrpGetCurrentStackLocation(irp_p);
            irp_priority = fbe_packet_priority_to_key(ts_p->request_p->specification.priority);
            EmcpalIrpParamSetWriteKey(irp_stack_p) = irp_priority;
        }
    }
    EmcpalExtIrpCompletionRoutineSet(irp_p, fbe_rdgen_ts_irp_completion, ts_p, CSX_TRUE, CSX_TRUE, CSX_TRUE);

    pal_status = EmcpalExtIrpSendAsync(ts_p->object_p->device_p, irp_p);

    if (pal_status != EMCPAL_STATUS_PENDING)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s status 0x%x not expected from EmcpalExtIrpSendAsync %s\n", 
                                __FUNCTION__, pal_status, ts_p->object_p->device_name);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_ts_send_dca()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_ts_send_sgl()
 ****************************************************************
 * @brief
 *  Build and send an sgl irp.
 *  We will translate the information from the TS in order to
 *  build this IRP.
 *
 * @param ts_p - Current tracking struct for thread.
 * @param opcode - The operation type to send.
 * @param lba - logical block address
 * @param blocks - Number of blocks to transfer.
 * @param b_is_read - TRUE if read FALSE if write.
 * @param payload_flags - Payload flags to get translated into irp and
 *                        sgl irp struct flags.
 *
 * @return FBE_STATUS_OK
 *         FBE_STATUS_GENERIC_FAILURE
 *
 * @author
 *  3/21/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_ts_send_sgl(fbe_rdgen_ts_t *ts_p,
                                   fbe_payload_block_operation_opcode_t opcode,
                                   fbe_lba_t lba,
                                   fbe_block_count_t blocks,
                                   fbe_bool_t b_is_read,
                                   fbe_payload_block_operation_flags_t payload_flags)
{
    PEMCPAL_IRP irp_p = NULL;
    fbe_packet_t *packet_p = NULL;
    EMCPAL_STATUS pal_status;
    PEMCPAL_IO_STACK_LOCATION irp_stack_p = NULL;
    FLARE_SGL_INFO *flare_sgl_info_p = NULL;
    fbe_u32_t irp_priority;

    /* Use the write packet for media modify operations.
     */
    if (fbe_payload_block_operation_opcode_is_media_modify(opcode))
    {
        irp_p = &ts_p->write_transport.disk_transport.irp;
        flare_sgl_info_p = &ts_p->write_transport.disk_transport.sgl_info;
        packet_p = fbe_rdgen_ts_get_write_packet(ts_p);
    }
    else
    {
        irp_p = &ts_p->read_transport.disk_transport.irp;
        flare_sgl_info_p = &ts_p->read_transport.disk_transport.sgl_info;
        packet_p = fbe_rdgen_ts_get_packet(ts_p);
    }
    fbe_rdgen_ts_set_packet_ptr(ts_p, packet_p);
    EmcpalIrpInitialize(irp_p, EmcpalIrpCalculateSize(ts_p->object_p->stack_size), ts_p->object_p->stack_size);

    if ((blocks * FBE_BYTES_PER_BLOCK) > FBE_U32_MAX)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s byte count for blocks is too big blocks: 0x%llx %s\n", 
                                __FUNCTION__, (unsigned long long)blocks, ts_p->object_p->device_name);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    flare_sgl_info_p->SglType = SGL_NORMAL;
    flare_sgl_info_p->FromPeer = 0;
    flare_sgl_info_p->SGList = (PSGL)ts_p->sg_ptr;
    flare_sgl_info_p->Flags = 0;

    flare_sgl_info_p->SglType |= SGL_VIRTUAL_ADDRESSES;
    if (ts_p->block_size != FBE_BYTES_PER_BLOCK)
    {
        flare_sgl_info_p->SglType |= SGL_SKIP_SECTOR_OVERHEAD_BYTES;
    }
    if (payload_flags & FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_ALLOW_FAIL_CONGESTION)
    {
        flare_sgl_info_p->Flags |= SGL_FLAG_ALLOW_FAIL_FOR_CONGESTION;
    }
    if (payload_flags & FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CHECK_CHECKSUM)
    {
        flare_sgl_info_p->Flags |= SGL_FLAG_VALIDATE_DATA;
    }
    if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED)
    {
        flare_sgl_info_p->Flags |= SGL_FLAG_NON_CACHED_WRITE;
    }
    if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE)
    {
        flare_sgl_info_p->Flags |= SGL_FLAG_VERIFY_BEFORE_WRITE;
    }

    irp_stack_p = EmcpalIrpGetNextStackLocation(irp_p);

    if (b_is_read)
    {
        EmcpalIrpStackSetFunction(irp_stack_p, EMCPAL_IRP_TYPE_CODE_INTERNAL_DEVICE_CONTROL, FLARE_SGL_READ);
        EmcpalIrpParamSetReadLength(irp_stack_p)     = (fbe_u32_t)(blocks * FBE_BYTES_PER_BLOCK);
        EmcpalIrpParamSetReadByteOffset(irp_stack_p) = (lba * FBE_BYTES_PER_BLOCK);

        if (ts_p->request_p->specification.options & FBE_RDGEN_OPTIONS_USE_PRIORITY) {
            irp_priority = fbe_packet_priority_to_key(ts_p->request_p->specification.priority);
            EmcpalIrpParamSetReadKey(irp_stack_p) = irp_priority;
        }
    }
    else
    {
        EmcpalIrpStackSetFunction(irp_stack_p, EMCPAL_IRP_TYPE_CODE_INTERNAL_DEVICE_CONTROL, FLARE_SGL_WRITE);
        EmcpalIrpParamSetWriteLength(irp_stack_p)     = (fbe_u32_t)(blocks * FBE_BYTES_PER_BLOCK);
        EmcpalIrpParamSetWriteByteOffset(irp_stack_p) = (lba * FBE_BYTES_PER_BLOCK);
        if (ts_p->request_p->specification.options & FBE_RDGEN_OPTIONS_USE_PRIORITY) {
            irp_priority = fbe_packet_priority_to_key(ts_p->request_p->specification.priority);
            EmcpalIrpParamSetWriteKey(irp_stack_p) = irp_priority;
        }
    }
    EmcpalExtIrpStackParamArg4Set(irp_stack_p, (PVOID)flare_sgl_info_p); 
    EmcpalExtIrpCompletionRoutineSet(irp_p, fbe_rdgen_ts_irp_completion, ts_p, CSX_TRUE, CSX_TRUE, CSX_TRUE);

    pal_status = EmcpalExtIrpSendAsync(ts_p->object_p->device_p, irp_p);

    if (pal_status != EMCPAL_STATUS_PENDING)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s status 0x%x not expected from EmcpalExtIrpSendAsync %s\n", 
                                __FUNCTION__, pal_status, ts_p->object_p->device_name);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_ts_send_sgl()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_ts_send_mj()
 ****************************************************************
 * @brief
 *  Build and send an irp mj read or write.
 *  We will translate the information from the TS in order to
 *  build this IRP.
 *
 * @param ts_p - Current tracking struct for thread.
 * @param opcode - The operation type to send.
 * @param lba - logical block address
 * @param blocks - Number of blocks to transfer.
 * @param b_is_read - TRUE if read FALSE if write.
 * @param payload_flags - Payload flags to get translated into irp and
 *                        sgl irp struct flags.
 *
 * @return FBE_STATUS_OK
 *         FBE_STATUS_GENERIC_FAILURE
 *
 * @author
 *  7/3/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_ts_send_mj(fbe_rdgen_ts_t *ts_p,
                                  fbe_payload_block_operation_opcode_t opcode,
                                  fbe_lba_t lba,
                                  fbe_block_count_t blocks,
                                  fbe_bool_t b_is_read,
                                  fbe_payload_block_operation_flags_t payload_flags)
{
    PEMCPAL_IRP irp_p = NULL;
    fbe_packet_t *packet_p = NULL;
    EMCPAL_STATUS pal_status;
    PEMCPAL_IO_STACK_LOCATION irp_stack_p = NULL;
    void *buffer_p = NULL;

    /* Use the write packet for media modify operations.
     */
    if (fbe_payload_block_operation_opcode_is_media_modify(opcode))
    {
        irp_p = &ts_p->write_transport.disk_transport.irp;
        packet_p = fbe_rdgen_ts_get_write_packet(ts_p);
    }
    else
    {
        irp_p = &ts_p->read_transport.disk_transport.irp;
        packet_p = fbe_rdgen_ts_get_packet(ts_p);
    }
    if (ts_p->memory_p != NULL)
    {
        buffer_p = ts_p->memory_p;
    }
    else
    {
        buffer_p = ts_p->sg_ptr->address;
    }

    fbe_rdgen_ts_set_packet_ptr(ts_p, packet_p);
    EmcpalIrpInitialize(irp_p, EmcpalIrpCalculateSize(ts_p->object_p->stack_size), ts_p->object_p->stack_size);

    if ((blocks * FBE_BYTES_PER_BLOCK) > FBE_U32_MAX)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s byte count for blocks is too big blocks: 0x%llx %s\n", 
                                __FUNCTION__, (unsigned long long)blocks, ts_p->object_p->device_name);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    irp_stack_p = EmcpalIrpGetNextStackLocation(irp_p);

    EmcpalIrpPrepareRWDirectIoBuffer(irp_p, buffer_p, (fbe_u32_t)(blocks * FBE_BYTES_PER_BLOCK));

    if (b_is_read)
    {
        EmcpalIrpBuildReadRequest(irp_p, (fbe_u32_t)(blocks * FBE_BYTES_PER_BLOCK), (lba * FBE_BYTES_PER_BLOCK), 0);
        EmcpalIrpStackSetFlags(irp_stack_p, 0);
        EmcpalIrpStackSetCurrentDeviceObject(irp_stack_p, ts_p->object_p->device_p);
    }
    else
    {
        EmcpalIrpBuildWriteRequest(irp_p, (fbe_u32_t)(blocks * FBE_BYTES_PER_BLOCK), (lba * FBE_BYTES_PER_BLOCK), 0);
        EmcpalIrpStackSetFlags(irp_stack_p, 0);
        EmcpalIrpStackSetCurrentDeviceObject(irp_stack_p, ts_p->object_p->device_p);
    }
        
    EmcpalExtIrpCompletionRoutineSet(irp_p, fbe_rdgen_ts_irp_completion, ts_p, CSX_TRUE, CSX_TRUE, CSX_TRUE);

    pal_status = EmcpalExtIrpSendAsync(ts_p->object_p->device_p, irp_p);

    if (pal_status != EMCPAL_STATUS_PENDING)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s status 0x%x not expected from EmcpalExtIrpSendAsync %s\n", 
                                __FUNCTION__, pal_status, ts_p->object_p->device_name);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_ts_send_mj()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_ts_send_zero()
 ****************************************************************
 * @brief
 *  Build and send an sgl irp.
 *  We will translate the information from the TS in order to
 *  build this IRP.
 *
 * @param ts_p - Current tracking struct for thread.
 * @param lba - logical block address
 * @param blocks - Number of blocks to transfer.
 *
 * @return FBE_STATUS_OK
 *         FBE_STATUS_GENERIC_FAILURE
 *
 * @author
 *  10/5/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_ts_send_zero(fbe_rdgen_ts_t *ts_p,
                                    fbe_lba_t lba,
                                    fbe_block_count_t blocks)
{
    PEMCPAL_IRP irp_p = NULL;
    fbe_packet_t *packet_p = NULL;
    EMCPAL_STATUS pal_status;
    PEMCPAL_IO_STACK_LOCATION irp_stack_p = NULL;
    FLARE_ZERO_FILL_INFO *zero_fill_info_p = NULL;

    /* Use the write packet for media modify operations.
     */
    irp_p = &ts_p->write_transport.disk_transport.irp;
    zero_fill_info_p = &ts_p->write_transport.disk_transport.zero_fill_info;
    packet_p = fbe_rdgen_ts_get_write_packet(ts_p);
    fbe_rdgen_ts_set_packet_ptr(ts_p, packet_p);

    zero_fill_info_p->Blocks = blocks;
    zero_fill_info_p->StartLba = lba;
    zero_fill_info_p->BlocksZeroed = 0;

    EmcpalIrpInitialize(irp_p, EmcpalIrpCalculateSize(ts_p->object_p->stack_size), ts_p->object_p->stack_size);
    //EmcpalExtIrpFlagsSet(irp_p, 0);
    if ((blocks * FBE_BYTES_PER_BLOCK) > FBE_U32_MAX)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s byte count for blocks is too big blocks: 0x%llx %s\n", 
                                __FUNCTION__, blocks, ts_p->object_p->device_name);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    irp_stack_p = EmcpalIrpGetNextStackLocation(irp_p);

    EmcpalIrpStackSetFunction(irp_stack_p, EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL, 0);
    EmcpalExtIrpStackParamIoctlCodeSet(irp_stack_p, IOCTL_FLARE_ZERO_FILL);
    EmcpalExtIrpStackParamIoctlOutputSizeSet(irp_stack_p, sizeof(FLARE_ZERO_FILL_INFO));
    EmcpalExtIrpStackParamIoctlInputSizeSet(irp_stack_p, sizeof(FLARE_ZERO_FILL_INFO));
            
    EmcpalExtIrpSystemBufferSet(irp_p, (PVOID)zero_fill_info_p); 
    EmcpalExtIrpCompletionRoutineSet(irp_p, fbe_rdgen_ts_irp_completion, ts_p, CSX_TRUE, CSX_TRUE, CSX_TRUE);

    pal_status = EmcpalExtIrpSendAsync(ts_p->object_p->device_p, irp_p);

    if (pal_status != EMCPAL_STATUS_PENDING)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s status 0x%x not expected from EmcpalExtIrpSendAsync %s\n", 
                                __FUNCTION__, pal_status, ts_p->object_p->device_name);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_ts_send_zero()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_send_disk_device()
 ****************************************************************
 * @brief
 *  This is our entry point for sending IRPs to other drivers.
 *
 * @param ts_p - Current tracking struct for thread.
 * @param opcode - The operation type to send.
 * @param lba - logical block address
 * @param blocks - Number of blocks to transfer.
 * @param b_is_read - TRUE if read FALSE if write.
 * @param payload_flags - Payload flags to get translated into irp and
 *                        sgl irp struct flags.            
 *
 * @return FBE_STATUS_OK
 *         FBE_STATUS_GENERIC_FAILURE   
 *
 * @author
 *  3/17/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_ts_send_disk_device(fbe_rdgen_ts_t *ts_p,
                                      fbe_payload_block_operation_opcode_t opcode,
                                      fbe_lba_t lba,
                                      fbe_block_count_t blocks,
                                      fbe_sg_element_t *sg_ptr,
                                      fbe_payload_block_operation_flags_t payload_flags)
{
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s send op: 0x%x lba: 0x%llx bl: 0x%llx dev:%s\n",
                            __FUNCTION__, opcode, (unsigned long long)lba, (unsigned long long)blocks, 
                            ts_p->object_p->device_name); 

    /* Update the last time we sent a packet for this ts.
     */
    fbe_rdgen_ts_update_last_send_time(ts_p);

    /* Set the opcode in the ts so that we know how to process errors.
     */
    fbe_rdgen_ts_set_block_opcode(ts_p, opcode);

    if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO)
    {
        return fbe_rdgen_ts_send_zero(ts_p, lba, blocks);
    }
    else if (ts_p->io_interface == FBE_RDGEN_INTERFACE_TYPE_IRP_DCA)
    {
        if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ)
        {
            return fbe_rdgen_ts_send_dca(ts_p, opcode, lba, blocks, FBE_TRUE /* is read */);
        }
        else if ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE) ||
                 (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED) ||
                 (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE))
        {
            return fbe_rdgen_ts_send_dca(ts_p, opcode, lba, blocks, FBE_FALSE /* is read */);
        }
    }
    else if (ts_p->io_interface == FBE_RDGEN_INTERFACE_TYPE_IRP_SGL)
    {
        if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ)
        {
            return fbe_rdgen_ts_send_sgl(ts_p, opcode, lba, blocks, FBE_TRUE, /* is read */ payload_flags);
        }
        else if ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE) ||
                 (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED) ||
                 (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE))

        {
            return fbe_rdgen_ts_send_sgl(ts_p, opcode, lba, blocks, FBE_FALSE, /* is read */ payload_flags);
        }
    }
    else if (ts_p->io_interface == FBE_RDGEN_INTERFACE_TYPE_IRP_MJ)
    {
        if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ)
        {
            return fbe_rdgen_ts_send_mj(ts_p, opcode, lba, blocks, FBE_TRUE, /* is read */ payload_flags);
        }
        else if ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE) ||
                 (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED) ||
                 (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE))

        {
            return fbe_rdgen_ts_send_mj(ts_p, opcode, lba, blocks, FBE_FALSE, /* is read */ payload_flags);
        }
    }
    else if (ts_p->io_interface == FBE_RDGEN_INTERFACE_TYPE_SYNC_IO)
    {
        if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ)
        {
            return fbe_rdgen_ts_send_to_bdev(ts_p, opcode, lba, blocks, FBE_TRUE, /* is read */ payload_flags);
        }
        else if ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE) ||
                 (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED) ||
                 (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE))

        {
            return fbe_rdgen_ts_send_to_bdev(ts_p, opcode, lba, blocks, FBE_FALSE, /* is read */ payload_flags);
        }
    }

    /* Anything else is unsupported.
     */
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s unknown interface %d op 0x%x lba: 0x%llx bl: 0x%llx dev:%s\n",
                            __FUNCTION__, ts_p->io_interface, 
                            opcode, (unsigned long long)lba, (unsigned long long)blocks, ts_p->object_p->device_name); 
    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************************
 * end fbe_rdgen_ts_send_disk_device()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_object_close()
 ****************************************************************
 * @brief
 *  Close the device object that we opened.
 *
 * @param object_p - The object that we previously opened to
 *                   another driver.               
 *
 * @return FBE_STATUS_OK
 *
 * @author
 *  3/17/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_object_close(fbe_rdgen_object_t *object_p)
{
    if (object_p->file_p != NULL)
    {
        EmcpalExtDeviceClose(object_p->file_p);
        object_p->device_p = NULL;
        object_p->file_p = NULL;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_rdgen_object_close()
 **************************************/

/*!**************************************************************
 * fbe_rdgen_sp_cache_open()
 ****************************************************************
 * @brief
 *  Opens the sp cache object.
 * 
 * @param file_p -
 * @param device_p - Output device ptr.
 *
 * @return FBE_STATUS_OK
 *         FBE_STATUS_GENERIC_FAILURE
 *
 * @author
 *  5/22/2012 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_sp_cache_open(PEMCPAL_DEVICE_OBJECT *device_p,
                                            PEMCPAL_FILE_OBJECT *file_p)
{
    EMCPAL_STATUS pal_status = EMCPAL_STATUS_SUCCESS;
    fbe_char_t device_name[FBE_RDGEN_DEVICE_NAME_CHARS];

    csx_p_snprintf(&device_name[0], FBE_RDGEN_DEVICE_NAME_CHARS, "%s", FBE_RDGEN_SP_CACHE_NAME);


    /* Open the device stack.
     */
#ifdef ALAMOSA_WINDOWS_ENV
    pal_status = EmcpalExtDeviceOpen(device_name,
                                     FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                                     file_p,
                                     device_p);
#else
    pal_status = EmcpalExtDeviceOpen(device_name,
                                     FILE_ALL_ACCESS,
                                     file_p,
                                     device_p);
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - what should this be */

    if ( !EMCPAL_IS_SUCCESS(pal_status) )
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (device_p == NULL)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s device ptr is null status: 0x%x\n", __FUNCTION__, pal_status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_sp_cache_open()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_alloc_cache_mem_irp_completion()
 ****************************************************************
 * @brief
 *  This is our standard IRP completion function.
 *  Just translate the status from the IRP into our
 *  ts last_packet status.  Then wake up the ts.
 *
 * @param device_object_p - The device object we sent this IRP to.
 * @param irp_p - Current IRP that is completing.
 * @param context_p - Pointer to our tracking structure.
 *
 * @return EMCPAL_STATUS_MORE_PROCESSING_REQUIRED So the IRP will
 *         not get completed up to the next level.
 *
 * @author
 *  7/16/2012 - Created. Rob Foley
 *
 ****************************************************************/

EMCPAL_STATUS fbe_rdgen_alloc_cache_mem_irp_completion(PEMCPAL_DEVICE_OBJECT device_object_p,
                                                       PEMCPAL_IRP irp_p,
                                                       PVOID context_p)
{
    EMCPAL_RENDEZVOUS_EVENT *event_p = (EMCPAL_RENDEZVOUS_EVENT*)context_p;

    EmcpalRendezvousEventSet(event_p);
    return EMCPAL_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_rdgen_alloc_cache_mem_irp_completion()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_alloc_cache_mem()
 ****************************************************************
 * @brief
 *  Issue the IOCTL_FLARE_GET_VOLUME_STATE to fetch our
 *  capacity information.
 *
 * @param bytes - The number of bytes to allocate.
 *
 * @return FBE_STATUS_OK
 *         FBE_STATUS_FAILED - Event cannot be created.
 *         FBE_STATUS_INSUFFICIENT_RESOURCES - no irp can be allocated
 *         FBE_STATUS_TIMEOUT - Took too long waiting for IRP to complete.
 *
 * @author
 *  5/22/2012 - Created. Rob Foley
 *
 ****************************************************************/

void * fbe_rdgen_alloc_cache_mem(fbe_u32_t bytes)
{
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;
    fbe_status_t fbe_status;
    PEMCPAL_IRP irp_p = NULL;
    EMCPAL_RENDEZVOUS_EVENT event;
    EMCPAL_STATUS wait_status = EMCPAL_STATUS_UNSUCCESSFUL;
    EMCPAL_LARGE_INTEGER li_timeout = {0};
    LONG timeout_seconds = 180;
    struct IoctlCacheAllocatePseudoControlMemory input_buffer;
    struct IoctlCacheAllocatePseudoControlMemoryOutput *output_buffer_p = 
        (struct IoctlCacheAllocatePseudoControlMemoryOutput *)&input_buffer;
    PEMCPAL_DEVICE_OBJECT device_p = NULL;
    PEMCPAL_FILE_OBJECT file_p = NULL;
    PEMCPAL_IO_STACK_LOCATION irp_stack_p = NULL;
    ULONG_PTR information;

    fbe_status = fbe_rdgen_sp_cache_open(&device_p, &file_p);
    if (fbe_status != FBE_STATUS_OK) 
    { 
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s error opening sp cache 0x%x\n", __FUNCTION__, fbe_status);
        return NULL; 
    }

    status = EmcpalRendezvousEventCreate(EmcpalDriverGetCurrentClientObject(), 
                                         &event, 
                                         "rdgen_get_size", 
                                         FALSE); 
    if (!EMCPAL_IS_SUCCESS(status))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s error creating EendezvousEvent 0x%x\n", __FUNCTION__, status);
        EmcpalRendezvousEventDestroy(&event);
        return NULL;
    }

    li_timeout.QuadPart = -((LONG)timeout_seconds * (NUM_NANO_SECONDS / 100L));

    irp_p = EmcpalIrpAllocate(10 /* stack size */);
    if (NULL == irp_p)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s irp ptr is null\n", __FUNCTION__);
        EmcpalRendezvousEventDestroy(&event);
        return NULL;
    }

    irp_stack_p = EmcpalIrpGetNextStackLocation(irp_p);
    EmcpalExtIrpStackMajorFunctionSet(irp_stack_p, EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL);
    EmcpalExtIrpStackMinorFunctionSet(irp_stack_p, 0);
    EmcpalExtIrpStackFlagsAnd(irp_stack_p, 0xFF);
    EmcpalExtIrpStackParamIoctlInputSizeSet(irp_stack_p, sizeof(struct IoctlCacheAllocatePseudoControlMemory));
    EmcpalExtIrpStackParamIoctlOutputSizeSet(irp_stack_p, sizeof(struct IoctlCacheAllocatePseudoControlMemoryOutput));
    EmcpalExtIrpStackParamIoctlCodeSet(irp_stack_p, IOCTL_CACHE_ALLOCATE_PSEUDO_CONTROL_MEMORY);
    EmcpalExtIrpSystemBufferSet(irp_p, &input_buffer);
    EmcpalExtIrpCompletionRoutineSet(irp_p, fbe_rdgen_alloc_cache_mem_irp_completion, &event, CSX_TRUE, CSX_TRUE, CSX_TRUE);

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s request %u bytes from SP Cache\n", __FUNCTION__, bytes);
    input_buffer.IncludeInCrashDump = FBE_FALSE;
    input_buffer.NumBytes = bytes;
    input_buffer.Tag = FBE_RDGEN_MEMORY_TAG;

    status = EmcpalExtIrpSendAsync(device_p, irp_p);

    if (EMCPAL_STATUS_PENDING == status)
    {
        wait_status = EmcpalRendezvousEventWait(&event, EMCPAL_NT_RELATIVE_TIME_TO_MSECS(&li_timeout));
    }

    status = EmcpalExtIrpStatusFieldGet(irp_p);
    information = EmcpalExtIrpInformationFieldGet(irp_p);

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s cache status: %u info: %llx\n", __FUNCTION__, status, (unsigned long long)information);
    if (file_p != NULL)
    {
        EmcpalExtDeviceClose(file_p);
    }

    EmcpalIrpFree(irp_p);
    EmcpalRendezvousEventDestroy(&event);

    if (EMCPAL_STATUS_TIMEOUT == wait_status )
    {
        /* We took too long.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s %d sec timeout on get memory\n", __FUNCTION__, timeout_seconds);
        return NULL;
    }

    if (EMCPAL_IS_SUCCESS(status))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s success alloc %u bytes ptr: 0x%llx status: 0x%x\n", 
                                __FUNCTION__, bytes, (unsigned long long)output_buffer_p->pseudoCtrlMemAddr, status);
        return (void*)output_buffer_p->pseudoCtrlMemAddr;
    }
    else
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s PAL status 0x%x on get memory of %d bytes\n", 
                                __FUNCTION__, status, bytes);
        return NULL;
    }
}
/******************************************
 * end fbe_rdgen_alloc_cache_mem()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_release_cache_mem()
 ****************************************************************
 * @brief
 *  Free memory that we allocated from the cache.
 *
 * @param  mem_ptr - Ptr of cache memory to free back to cache.               
 *
 * @return None.   
 *
 * @author
 *  5/24/2012 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_release_cache_mem(void * mem_ptr)
{
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;
    fbe_status_t fbe_status;
    PEMCPAL_IRP irp_p = NULL;
    EMCPAL_RENDEZVOUS_EVENT event;
    EMCPAL_IRP_STATUS_BLOCK io_status = {0};
    EMCPAL_STATUS wait_status      = EMCPAL_STATUS_UNSUCCESSFUL;
    EMCPAL_LARGE_INTEGER li_timeout = {0};
    LONG timeout_seconds = 180;
    struct IoctlCacheFreePseudoControlMemory input_buffer;
    PEMCPAL_DEVICE_OBJECT device_p = NULL;
    PEMCPAL_FILE_OBJECT file_p = NULL;
    PEMCPAL_IO_STACK_LOCATION irp_stack_p = NULL;
    
    fbe_status = fbe_rdgen_sp_cache_open(&device_p, &file_p);
    if (fbe_status != FBE_STATUS_OK) 
    { 
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s error opening sp cache 0x%x\n", __FUNCTION__, fbe_status);
        return; 
    }

    status = EmcpalRendezvousEventCreate(EmcpalDriverGetCurrentClientObject(), 
                                         &event, 
                                         "rdgen_get_size", 
                                         FALSE); 
    if (!EMCPAL_IS_SUCCESS(status))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s error creating EendezvousEvent 0x%x\n", __FUNCTION__, status);
        EmcpalRendezvousEventDestroy(&event);
        return;
    }

    li_timeout.QuadPart = -((LONG)timeout_seconds * (NUM_NANO_SECONDS / 100L));

    irp_p = EmcpalIrpAllocate(10 /* stack size */);
    if (NULL == irp_p)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s irp ptr is null\n", __FUNCTION__);
        EmcpalRendezvousEventDestroy(&event);
        return;
    }
    irp_stack_p = EmcpalIrpGetNextStackLocation(irp_p);
    EmcpalExtIrpStackMajorFunctionSet(irp_stack_p, EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL);
    EmcpalExtIrpStackMinorFunctionSet(irp_stack_p, 0);
    EmcpalExtIrpStackFlagsAnd(irp_stack_p, 0xFF);
    EmcpalExtIrpStackParamIoctlInputSizeSet(irp_stack_p, sizeof(struct IoctlCacheFreePseudoControlMemory));
    EmcpalExtIrpStackParamIoctlOutputSizeSet(irp_stack_p, 0);
    EmcpalExtIrpStackParamIoctlCodeSet(irp_stack_p, IOCTL_CACHE_FREE_PSEUDO_CONTROL_MEMORY);
    EmcpalExtIrpSystemBufferSet(irp_p, &input_buffer);
    EmcpalExtIrpCompletionRoutineSet(irp_p, fbe_rdgen_alloc_cache_mem_irp_completion, &event, CSX_TRUE, CSX_TRUE, CSX_TRUE);

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s free ptr %p from SP Cache\n", __FUNCTION__, mem_ptr);
    input_buffer.MemAddress = (ULONG64)mem_ptr;
    status = EmcpalExtIrpSendAsync(device_p, irp_p);

    if (EMCPAL_STATUS_PENDING == status)
    {
        wait_status = EmcpalRendezvousEventWait(&event, EMCPAL_NT_RELATIVE_TIME_TO_MSECS(&li_timeout));

        status = EmcpalExtIrpStatusBlockStatusFieldGet(&io_status);
    }

    status = EmcpalExtIrpStatusFieldGet(irp_p);

    if (file_p != NULL)
    {
        EmcpalExtDeviceClose(file_p);
    }
    EmcpalIrpFree(irp_p);
    EmcpalRendezvousEventDestroy(&event);

    if (EMCPAL_STATUS_TIMEOUT == wait_status )
    {
        /* We took too long.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s %d sec timeout on get memory\n", __FUNCTION__, (int)timeout_seconds);
        return;
    }

    if (EMCPAL_IS_SUCCESS(status))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s success free ptr: %p status: 0x%x\n", 
                                __FUNCTION__, mem_ptr, status);
        return;
    }
    else
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s failure PAL status 0x%x on free memory ptr: %p\n", 
                                __FUNCTION__, status, mem_ptr);
        return;
    }
    return;
}
/******************************************
 * end fbe_rdgen_release_cache_mem()
 ******************************************/

#ifdef C4_INTEGRATED
/*!**************************************************************
 * fbe_rdgen_object_init_bdev()
 ****************************************************************
 * @brief
 *  This opens the device and gets the capacity of the device.
 *
 * @param object_p - Rdgen's object struct.
 *
 * @return FBE_STATUS_OK
 *         Other status values indicate failure.
 *
 * @author
 *  11/12/2014 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_rdgen_object_init_bdev(fbe_rdgen_object_t *object_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u64_t size;
    int fd;

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s opening %s\n", __FUNCTION__, object_p->device_name);

    fd = open(object_p->device_name, O_RDONLY);
    if (fd == -1) 
    { 
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    
    if (ioctl(fd, BLKGETSIZE64, &size) == -1)
    {
        close(fd);
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    close(fd);
    
    object_p->device_p = (PEMCPAL_DEVICE_OBJECT)0xFFFFFFFF;
    object_p->file_p = (PEMCPAL_FILE_OBJECT)0;

    object_p->capacity = size / FBE_BYTES_PER_BLOCK;
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s %s capacity 0x%llx\n", __FUNCTION__, object_p->device_name, object_p->capacity);

    object_p->block_size = FBE_BE_BYTES_PER_BLOCK;
    object_p->physical_block_size = FBE_BE_BYTES_PER_BLOCK;
    object_p->optimum_block_size = 1;

    return status;
}
/******************************************
 * end fbe_rdgen_object_init_bdev()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_object_open_bdev()
 ****************************************************************
 * @brief
 *  This function opens the device.
 *
 * @param fp - Pointer to the device handler.
 *
 * @return FBE_STATUS_OK
 *         Other status values indicate failure.
 *
 * @author
 *  11/12/2014 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_rdgen_object_open_bdev(fbe_s32_t *fp, csx_cstring_t bdev)
{
    *fp = open(bdev, O_RDWR | O_DIRECT | O_SYNC);

    if (*fp == -1)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s failed: dev %s\n", __FUNCTION__, bdev);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_object_open_bdev()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_object_close_bdev()
 ****************************************************************
 * @brief
 *  This function closes the device.
 *
 * @param fp - Pointer to the device handler.
 *
 * @return FBE_STATUS_OK
 *         Other status values indicate failure.
 *
 * @author
 *  11/12/2014 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_rdgen_object_close_bdev(fbe_s32_t *fp)
{
    close(*fp);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_object_close_bdev()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_send_to_bdev()
 ****************************************************************
 * @brief
 *  This function sends the IO to block device.
 *
 * @param ts_p - Pointer to ts structure.
 *
 * @return FBE_STATUS_OK
 *         Other status values indicate failure.
 *
 * @author
 *  11/12/2014 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_rdgen_ts_send_to_bdev(fbe_rdgen_ts_t *ts_p,
                                  fbe_payload_block_operation_opcode_t opcode,
                                  fbe_lba_t lba,
                                  fbe_block_count_t blocks,
                                  fbe_bool_t b_is_read,
                                  fbe_payload_block_operation_flags_t payload_flags)
{
    fbe_packet_t *packet_p = NULL;
    void *buffer_p = NULL;
    fbe_u32_t usec;
    int nbytes = 0;

    /* Use the write packet for media modify operations.
     */
    if (fbe_payload_block_operation_opcode_is_media_modify(opcode))
    {
        packet_p = fbe_rdgen_ts_get_write_packet(ts_p);
    }
    else
    {
        packet_p = fbe_rdgen_ts_get_packet(ts_p);
    }
    if (ts_p->memory_p != NULL)
    {
        buffer_p = ts_p->memory_p;
    }
    else
    {
        buffer_p = ts_p->sg_ptr->address;
    }

    fbe_rdgen_ts_set_packet_ptr(ts_p, packet_p);

    if ((blocks * FBE_BYTES_PER_BLOCK) > FBE_U32_MAX)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s byte count for blocks is too big blocks: 0x%llx %s\n", 
                                __FUNCTION__, (unsigned long long)blocks, ts_p->object_p->device_name);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (lseek(ts_p->fp, (lba * FBE_BYTES_PER_BLOCK), SEEK_SET) < 0)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s lseek failed %s\n", 
                                __FUNCTION__, ts_p->object_p->device_name);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Alignment required for O_DIRECT flag */
    buffer_p = (void *)((((fbe_u64_t)buffer_p + FBE_BYTES_PER_BLOCK -1)/FBE_BYTES_PER_BLOCK) * FBE_BYTES_PER_BLOCK);
    if (b_is_read)
    {
        nbytes = read(ts_p->fp, buffer_p, (fbe_u32_t)(blocks * FBE_BYTES_PER_BLOCK));
        if (nbytes != (fbe_u32_t)(blocks * FBE_BYTES_PER_BLOCK))
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s read failed nbytes %d buf %p\n", 
                                    __FUNCTION__, nbytes, buffer_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else
    {
        nbytes = write(ts_p->fp, buffer_p, (fbe_u32_t)(blocks * FBE_BYTES_PER_BLOCK));
        if (nbytes != (fbe_u32_t)(blocks * FBE_BYTES_PER_BLOCK))
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s write failed nbytes %d\n", 
                                    __FUNCTION__, nbytes);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* */
    ts_p->last_packet_status.status = FBE_STATUS_OK;
    ts_p->last_packet_status.block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;
    ts_p->last_packet_status.block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;


    usec = fbe_get_elapsed_microseconds(ts_p->last_send_time);
    if (usec >= FBE_RDGEN_SERVICE_TIME_MAX_MICROSEC)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, fbe_rdgen_object_get_object_id(ts_p->object_p),
                                "%s ts %p completed after %d usecs\n",
                                __FUNCTION__, ts_p, usec);
    }
    /* Update our response time statistics.
     */
    fbe_rdgen_ts_set_last_response_time(ts_p, usec);
    fbe_rdgen_ts_update_cum_resp_time(ts_p, usec);
    fbe_rdgen_ts_update_max_response_time(ts_p, usec);

    fbe_rdgen_ts_enqueue_to_thread(ts_p);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_ts_send_to_bdev()
 ******************************************/
#else
/*!**************************************************************
 * fbe_rdgen_object_init_bdev()
 ****************************************************************
 * @brief
 *  This opens the device and gets the capacity of the device.
 *
 * @param object_p - Rdgen's object struct.
 *
 * @return FBE_STATUS_OK
 *         Other status values indicate failure.
 *
 * @author
 *  11/12/2014 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_rdgen_object_init_bdev(fbe_rdgen_object_t *object_p)
{
    object_p->device_p = (PEMCPAL_DEVICE_OBJECT)0xFFFFFFFF;
    object_p->file_p = (PEMCPAL_FILE_OBJECT)0;
    object_p->capacity = 0;
    object_p->block_size = FBE_BE_BYTES_PER_BLOCK;
    object_p->physical_block_size = FBE_BE_BYTES_PER_BLOCK;
    object_p->optimum_block_size = 1;

    return FBE_STATUS_OK; 
}
/******************************************
 * end fbe_rdgen_object_init_bdev()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_object_open_bdev()
 ****************************************************************
 * @brief
 *  This function opens the device.
 *
 * @param fp - Pointer to the device handler.
 *
 * @return FBE_STATUS_OK
 *         Other status values indicate failure.
 *
 * @author
 *  11/12/2014 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_rdgen_object_open_bdev(fbe_s32_t *fp, csx_cstring_t bdev)
{
    *fp = NULL;
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_object_open_bdev()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_object_close_bdev()
 ****************************************************************
 * @brief
 *  This function closes the device.
 *
 * @param fp - Pointer to the device handler.
 *
 * @return FBE_STATUS_OK
 *         Other status values indicate failure.
 *
 * @author
 *  11/12/2014 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_rdgen_object_close_bdev(fbe_s32_t *fp)
{
    *fp = NULL;
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_object_close_bdev()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_send_to_bdev()
 ****************************************************************
 * @brief
 *  This function sends the IO to block device.
 *
 * @param ts_p - Pointer to ts structure.
 *
 * @return FBE_STATUS_OK
 *         Other status values indicate failure.
 *
 * @author
 *  11/12/2014 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_rdgen_ts_send_to_bdev(fbe_rdgen_ts_t *ts_p,
                                  fbe_payload_block_operation_opcode_t opcode,
                                  fbe_lba_t lba,
                                  fbe_block_count_t blocks,
                                  fbe_bool_t b_is_read,
                                  fbe_payload_block_operation_flags_t payload_flags)
{
    return FBE_STATUS_OK;
}
#endif
/*************************
 * end file fbe_rdgen_kernel_main.c
 *************************/

