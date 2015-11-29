/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_memory_persistence_kernel.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the persistent memory service.
 *
 * @version
 *   11/26/2013:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_memory.h"
#include "fbe_base_service.h"
#include "fbe_memory_private.h"
#include "flare_ioctls.h"
#include "flare_export_ioctls.h"

static PEMCPAL_DEVICE_OBJECT fbe_memory_persistence_device_p = NULL;
static PEMCPAL_FILE_OBJECT   fbe_memory_persistence_file_p   = NULL;

static fbe_status_t fbe_memory_persistence_sp_cache_open(PEMCPAL_DEVICE_OBJECT *device_p,
                                                         PEMCPAL_FILE_OBJECT *file_p);

static fbe_status_t fbe_memory_persistence_read_pin_start(fbe_packet_t *packet_p, 
                                                          PEMCPAL_IRP irp_p,
                                                          IoctlCacheReadAndPinReq *req_p);


#define FBE_MEMORY_PERSISTENCE_DEVICE_NAME_CHARS 80
/*!*******************************************************************
 * @def FBE_MEMORY_PERSISTENCE_SP_CACHE_NAME
 *********************************************************************
 * @brief The device name of sp cache, needed when allocating
 *        memory from SP cache.
 *
 *********************************************************************/
#define FBE_MEMORY_PERSISTENCE_SP_CACHE_NAME "\\Device\\DRAMCache"

/*!**************************************************************
 * fbe_memory_persistence_sp_cache_open()
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
 *  12/3/2013 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_memory_persistence_sp_cache_open(PEMCPAL_DEVICE_OBJECT *device_p,
                                                         PEMCPAL_FILE_OBJECT *file_p)
{
    EMCPAL_STATUS pal_status = EMCPAL_STATUS_SUCCESS;
    fbe_char_t device_name[FBE_MEMORY_PERSISTENCE_DEVICE_NAME_CHARS];

    memory_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);
    csx_p_snprintf(&device_name[0], FBE_MEMORY_PERSISTENCE_DEVICE_NAME_CHARS, "%s", FBE_MEMORY_PERSISTENCE_SP_CACHE_NAME);

    /* Open the device stack.
     */
    pal_status = EmcpalExtDeviceOpen(device_name,
                                     FILE_ALL_ACCESS,
                                     file_p,
                                     device_p);

    if ( !EMCPAL_IS_SUCCESS(pal_status) ) {
        memory_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                             "%s failed open status: 0x%x\n", __FUNCTION__, pal_status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (device_p == NULL) {
        memory_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s device ptr is null status: 0x%x\n", __FUNCTION__, pal_status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    memory_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                         "%s successful open status: 0x%x\n", __FUNCTION__, pal_status);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_memory_persistence_sp_cache_open()
 ******************************************/

fbe_status_t fbe_memory_persistence_setup(void)
{
    memory_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_memory_persistence_cleanup(void)
{
    memory_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);
    if (fbe_memory_persistence_file_p != NULL) {
        EmcpalExtDeviceClose(fbe_memory_persistence_file_p);
    }
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_memory_persistence_unpin_completion()
 ****************************************************************
 * @brief
 *  The unpin has completed.  Check status and return
 *  information from the irp.
 *
 * @param device_object_p - The device object we sent this IRP to.
 * @param irp_p - Current IRP that is completing.
 * @param context_p - Pointer to our tracking structure.
 *
 * @return EMCPAL_STATUS_MORE_PROCESSING_REQUIRED So the IRP will
 *         not get completed up to the next level.
 *
 * @author
 *  12/3/2013 - Created. Rob Foley
 *
 ****************************************************************/

static EMCPAL_STATUS fbe_memory_persistence_unpin_completion(PEMCPAL_DEVICE_OBJECT device_object_p,
                                                             PEMCPAL_IRP irp_p,
                                                             PVOID context_p)
{
    fbe_packet_t *packet_p = (fbe_packet_t *)context_p;
    fbe_payload_ex_t * sep_payload_p = NULL;
    fbe_payload_persistent_memory_operation_t * persistent_memory_operation_p = NULL;
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;
    ULONG_PTR information;
    status = EmcpalExtIrpStatusFieldGet(irp_p);
    information = EmcpalExtIrpInformationFieldGet(irp_p);
    
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    persistent_memory_operation_p =  fbe_payload_ex_get_persistent_memory_operation(sep_payload_p);

    if (status != EMCPAL_STATUS_SUCCESS) { 
        memory_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                         "unpin cache failed status: %u info: %llx\n", status, (unsigned long long)information);
        persistent_memory_operation_p->persistent_memory_status = FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_FAILURE;
    } else {
        memory_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                             "unpin cache success status: %u info: %llx\n", status, (unsigned long long)information);
        persistent_memory_operation_p->persistent_memory_status = FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_OK;
    }
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return EMCPAL_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_memory_persistence_unpin_completion()
 ******************************************/
/*!**************************************************************
 * fbe_memory_persistence_unpin_send_irp()
 ****************************************************************
 * @brief
 *  Handle completion of the read and pin.
 *
 * @param packet_p
 * @param irp_p
 * @param req_p
 *
 * @return fbe_status_t
 *
 * @author
 *  12/3/2013 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_memory_persistence_unpin_send_irp(fbe_packet_t *packet_p, 
                                                          PEMCPAL_IRP irp_p,
                                                          struct IoctlCacheUnpinInfo *req_p)
{
    EMCPAL_STATUS pal_status = EMCPAL_STATUS_SUCCESS;
    PEMCPAL_IO_STACK_LOCATION irp_stack_p = NULL;

    EmcpalIrpInitialize(irp_p, EmcpalIrpCalculateSize(10), 10);
    
    irp_stack_p = EmcpalIrpGetNextStackLocation(irp_p);

    EmcpalIrpStackSetFunction(irp_stack_p, EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL, 0);
    EmcpalExtIrpStackFlagsAnd(irp_stack_p, 0xFF);
    EmcpalExtIrpStackParamIoctlCodeSet(irp_stack_p, IOCTL_CACHE_UNPIN_DATA);
    EmcpalExtIrpStackParamIoctlInputSizeSet(irp_stack_p, sizeof(struct IoctlCacheUnpinInfo));
    EmcpalExtIrpStackParamIoctlOutputSizeSet(irp_stack_p, 0);
            
    EmcpalExtIrpSystemBufferSet(irp_p, (PVOID)req_p); 
    EmcpalExtIrpCompletionRoutineSet(irp_p, fbe_memory_persistence_unpin_completion, packet_p, CSX_TRUE, CSX_TRUE, CSX_TRUE);

    pal_status = EmcpalExtIrpSendAsync(fbe_memory_persistence_device_p, irp_p);

    if (pal_status != EMCPAL_STATUS_PENDING) {
        memory_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s status 0x%x not expected from EmcpalExtIrpSendAsync\n", 
                                __FUNCTION__, pal_status);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_memory_persistence_unpin_send_irp()
 ******************************************/

/*!**************************************************************
 * fbe_memory_persistence_unpin()
 ****************************************************************
 * @brief
 *  Setup and send the read and pin irp.
 *  
 * @param packet_p
 * 
 * @return fbe_status_t
 *
 * @author
 *  12/3/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_memory_persistence_unpin(fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload_p = NULL;
    fbe_payload_persistent_memory_operation_t * persistent_memory_operation_p = NULL;
    PEMCPAL_IRP irp_p = NULL;
    struct IoctlCacheUnpinInfo *req_p = NULL;
    fbe_u8_t *data_p = NULL;

    if (fbe_memory_persistence_device_p == NULL){
        status = fbe_memory_persistence_sp_cache_open(&fbe_memory_persistence_device_p,
                                                      &fbe_memory_persistence_file_p);
        if (status != FBE_STATUS_OK) {
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_OK;
        }
    }
    
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    persistent_memory_operation_p =  fbe_payload_ex_get_persistent_memory_operation(sep_payload_p);

    /* Allocate memory for the irp and control structures from the passed in information.
     */
    data_p = persistent_memory_operation_p->chunk_p;
    irp_p = (PEMCPAL_IRP)data_p;
    data_p += FBE_MEMORY_CHUNK_SIZE;

    req_p = (struct IoctlCacheUnpinInfo *)data_p;
    /* Check for truncation.
     */
    if (persistent_memory_operation_p->blocks > FBE_U32_MAX) {
        memory_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, 
                             "%s blocks %llx is too large\n", __FUNCTION__, persistent_memory_operation_p->blocks);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    switch (persistent_memory_operation_p->mode) {
        case FBE_PAYLOAD_PERSISTENT_MEMORY_UNPIN_MODE_DATA_WRITTEN:
            req_p->Action = UNPIN_HAS_BEEN_WRITTEN;
            break;
        case FBE_PAYLOAD_PERSISTENT_MEMORY_UNPIN_MODE_FLUSH_REQUIRED:
            req_p->Action = UNPIN_AND_FLUSH;
            break;
        case FBE_PAYLOAD_PERSISTENT_MEMORY_UNPIN_MODE_VERIFY_REQUIRED:
            req_p->Action = UNPIN_FLUSH_VERIFY;
            break;
        default:
             memory_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, 
                                  "%s unknown mode %d defaults to flush/verify\n", 
                                  __FUNCTION__, persistent_memory_operation_p->mode);
            req_p->Action = UNPIN_FLUSH_VERIFY;
            break;
    }
    req_p->Opaque0 = persistent_memory_operation_p->opaque;
    fbe_memory_persistence_unpin_send_irp(packet_p, irp_p, req_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_memory_persistence_unpin()
 ******************************************/
/*!**************************************************************
 * fbe_memory_persistence_read_pin_completion()
 ****************************************************************
 * @brief
 *  The read and pin has completed.  Check status and return
 *  information from the irp.
 *
 * @param device_object_p - The device object we sent this IRP to.
 * @param irp_p - Current IRP that is completing.
 * @param context_p - Pointer to our tracking structure.
 *
 * @return EMCPAL_STATUS_MORE_PROCESSING_REQUIRED So the IRP will
 *         not get completed up to the next level.
 *
 * @author
 *  12/3/2013 - Created. Rob Foley
 *
 ****************************************************************/

EMCPAL_STATUS fbe_memory_persistence_read_pin_completion(PEMCPAL_DEVICE_OBJECT device_object_p,
                                                                PEMCPAL_IRP irp_p,
                                                                PVOID context_p)
{
    fbe_packet_t *packet_p = (fbe_packet_t *)context_p;
    fbe_payload_ex_t * sep_payload_p = NULL;
    fbe_payload_persistent_memory_operation_t * persistent_memory_operation_p = NULL;
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;
    ULONG_PTR information;
    IoctlCacheReadAndPinResp *resp_p = NULL;
    fbe_u8_t *data_p = NULL;

    status = EmcpalExtIrpStatusFieldGet(irp_p);
    information = EmcpalExtIrpInformationFieldGet(irp_p);
    
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    persistent_memory_operation_p =  fbe_payload_ex_get_persistent_memory_operation(sep_payload_p);

    data_p = persistent_memory_operation_p->chunk_p;
    irp_p = (PEMCPAL_IRP)data_p;
    data_p += FBE_MEMORY_CHUNK_SIZE;

    resp_p = (IoctlCacheReadAndPinResp*)data_p;

    /* Save the information passed back about the operation.
     */
    persistent_memory_operation_p->opaque = resp_p->Opaque0;
    persistent_memory_operation_p->b_was_dirty = resp_p->wasDirty;

    if (status != EMCPAL_STATUS_SUCCESS) { 
        memory_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                             "read and pin cache failed status: 0x%x info: %llx\n", status, (unsigned long long)information);
        persistent_memory_operation_p->persistent_memory_status = FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_FAILURE;
    } else if (resp_p->Result != PIN_AND_MAKE_DIRTY) { 
        /* Cache not enabled.
         */ 
        memory_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                             "read and pin cache result was not dirty 0x%x status: 0x%x info: %llx\n", 
                             resp_p->Result, status, (unsigned long long)information);
        persistent_memory_operation_p->persistent_memory_status = FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_PINNED_NOT_PERSISTENT;
    }
    else {
        memory_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                             "read and pin cache success status: 0x%x info: %llx\n", status, (unsigned long long)information);
        persistent_memory_operation_p->persistent_memory_status = FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_OK;

        /* RPF Remove this once the cache interface is ready.
         */
        //persistent_memory_operation_p->sg_p = (fbe_sg_element_t*)resp_p->pSgl->SGList;
    }
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return EMCPAL_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_memory_persistence_read_pin_completion()
 ******************************************/
/*!**************************************************************
 * fbe_memory_persistence_read_pin_send_irp()
 ****************************************************************
 * @brief
 *  Handle completion of the read and pin.
 *
 * @param packet_p
 * @param irp_p
 * @param req_p
 *
 * @return fbe_status_t
 *
 * @author
 *  12/3/2013 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_memory_persistence_read_pin_send_irp(fbe_packet_t *packet_p, 
                                                          PEMCPAL_IRP irp_p,
                                                          IoctlCacheReadAndPinReq *req_p)
{
    EMCPAL_STATUS pal_status = EMCPAL_STATUS_SUCCESS;
    PEMCPAL_IO_STACK_LOCATION irp_stack_p = NULL;

    EmcpalIrpInitialize(irp_p, EmcpalIrpCalculateSize(10), 10);
    
    irp_stack_p = EmcpalIrpGetNextStackLocation(irp_p);

    EmcpalIrpStackSetFunction(irp_stack_p, EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL, 0);
    EmcpalExtIrpStackFlagsAnd(irp_stack_p, 0xFF);
    EmcpalExtIrpStackParamIoctlCodeSet(irp_stack_p, IOCTL_CACHE_READ_AND_PIN_DATA);
    EmcpalExtIrpStackParamIoctlInputSizeSet(irp_stack_p, sizeof(IoctlCacheReadAndPinReq));
    EmcpalExtIrpStackParamIoctlOutputSizeSet(irp_stack_p, sizeof(IoctlCacheReadAndPinResp));
            
    EmcpalExtIrpSystemBufferSet(irp_p, (PVOID)req_p); 
    EmcpalExtIrpCompletionRoutineSet(irp_p, fbe_memory_persistence_read_pin_completion, packet_p, CSX_TRUE, CSX_TRUE, CSX_TRUE);

    pal_status = EmcpalExtIrpSendAsync(fbe_memory_persistence_device_p, irp_p);

    if (pal_status != EMCPAL_STATUS_PENDING)
    {
        memory_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s status 0x%x not expected from EmcpalExtIrpSendAsync\n", 
                                __FUNCTION__, pal_status);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_memory_persistence_read_pin_send_irp()
 ******************************************/

/*!**************************************************************
 * fbe_memory_persistence_read_and_pin()
 ****************************************************************
 * @brief
 *  Setup and send the read and pin irp.
 *  
 * @param packet_p
 * 
 * @return fbe_status_t
 *
 * @author
 *  12/3/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_memory_persistence_read_and_pin(fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload_p = NULL;
    fbe_payload_persistent_memory_operation_t * persistent_memory_operation_p = NULL;
    PEMCPAL_IRP irp_p = NULL;
    IoctlCacheReadAndPinReq *req_p = NULL;
    fbe_u8_t *data_p = NULL;

    if (fbe_memory_persistence_device_p == NULL){
        status = fbe_memory_persistence_sp_cache_open(&fbe_memory_persistence_device_p,
                                                      &fbe_memory_persistence_file_p);
        if (status != FBE_STATUS_OK) {
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_OK;
        }
    }
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    persistent_memory_operation_p =  fbe_payload_ex_get_persistent_memory_operation(sep_payload_p);

    if ((persistent_memory_operation_p->chunk_p == NULL) ||
        (persistent_memory_operation_p->sg_p == NULL)) {
        memory_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, 
                             "%s no memory provided %p/%p\n", __FUNCTION__, 
                             persistent_memory_operation_p->sg_p, persistent_memory_operation_p->chunk_p);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }
    /* Allocate memory for the irp and control structures from the passed in information.
     */
    data_p = persistent_memory_operation_p->chunk_p;
    irp_p = (PEMCPAL_IRP)data_p;
    data_p += FBE_MEMORY_CHUNK_SIZE;

    req_p = (IoctlCacheReadAndPinReq *)data_p;
    
    /* Check for truncation.
     */
    if (persistent_memory_operation_p->blocks > FBE_U32_MAX) {
        memory_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, 
                             "%s blocks %llx is too large\n", __FUNCTION__, persistent_memory_operation_p->blocks);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }
    req_p->Action = PIN_AND_MAKE_DIRTY;
    req_p->StartLba = persistent_memory_operation_p->lba;
    req_p->BlockCnt = (fbe_u32_t)persistent_memory_operation_p->blocks;
    req_p->LunIndex = persistent_memory_operation_p->object_index;
    req_p->ExtendedAction = PIN_EXTENDED_NO_OPERATION;

    if (persistent_memory_operation_p->b_beyond_capacity) {
        req_p->ExtendedAction |= PIN_EXTENDED_DISABLE_CAPACITY_CHECK;
    }
    req_p->ClientProvidedSgl = (SGL*)persistent_memory_operation_p->sg_p;
    req_p->ClientProvidedSglMaxEntries = persistent_memory_operation_p->max_sg_entries;

    /* Cache expects a terminated sg list. */
    fbe_sg_element_terminate(&persistent_memory_operation_p->sg_p[0]);
    fbe_sg_element_terminate(&persistent_memory_operation_p->sg_p[persistent_memory_operation_p->max_sg_entries - 1]);

    fbe_memory_persistence_read_pin_send_irp(packet_p, irp_p, req_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_memory_persistence_read_and_pin()
 ******************************************/

/*!**************************************************************
 * fbe_memory_persistence_check_vault_completion()
 ****************************************************************
 * @brief
 *  The check vault completed.  Check status and return
 *  information from the irp.
 *
 * @param device_object_p - The device object we sent this IRP to.
 * @param irp_p - Current IRP that is completing.
 * @param context_p - Pointer to our tracking structure.
 *
 * @return EMCPAL_STATUS_MORE_PROCESSING_REQUIRED So the IRP will
 *         not get completed up to the next level.
 *
 * @author
 *  1/17/2014 - Created. Rob Foley
 *
 ****************************************************************/

EMCPAL_STATUS fbe_memory_persistence_check_vault_completion(PEMCPAL_DEVICE_OBJECT device_object_p,
                                                            PEMCPAL_IRP irp_p,
                                                            PVOID context_p)
{
    fbe_packet_t *packet_p = (fbe_packet_t *)context_p;
    fbe_payload_ex_t * sep_payload_p = NULL;
    fbe_payload_persistent_memory_operation_t * persistent_memory_operation_p = NULL;
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;
    ULONG_PTR information;
    struct IoctlCacheVaultImageOutput *output_p = NULL;

    status = EmcpalExtIrpStatusFieldGet(irp_p);
    information = EmcpalExtIrpInformationFieldGet(irp_p);
    
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    persistent_memory_operation_p =  fbe_payload_ex_get_persistent_memory_operation(sep_payload_p);
    output_p = (struct IoctlCacheVaultImageOutput *)(persistent_memory_operation_p->chunk_p + FBE_MEMORY_CHUNK_SIZE);

    if ((status != EMCPAL_STATUS_SUCCESS) || (output_p->status != CacheVaultImageNotInUse)) { 
        memory_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                             "check vault failed status: 0x%x info: %llx VaultImageStatus: 0x%x\n", 
                             status, (unsigned long long)information, output_p->status);
        persistent_memory_operation_p->persistent_memory_status = FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_BUSY;
    }
    else {
        memory_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                             "check vault success status: 0x%x info: %llx VaultImageStatus: %d\n", 
                             status, (unsigned long long)information, output_p->status);
        persistent_memory_operation_p->persistent_memory_status = FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_OK;
    }
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return EMCPAL_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_memory_persistence_check_vault_completion()
 ******************************************/
/*!**************************************************************
 * fbe_memory_persistence_check_vault_send_irp()
 ****************************************************************
 * @brief
 *  Send IRP to check if vault is busy.
 *
 * @param packet_p
 * @param irp_p
 * @param req_p
 *
 * @return fbe_status_t
 *
 * @author
 *  1/17/2014 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_memory_persistence_check_vault_send_irp(fbe_packet_t *packet_p, 
                                                                PEMCPAL_IRP irp_p,
                                                                fbe_u8_t *req_p)
{
    EMCPAL_STATUS pal_status = EMCPAL_STATUS_SUCCESS;
    PEMCPAL_IO_STACK_LOCATION irp_stack_p = NULL;

    EmcpalIrpInitialize(irp_p, EmcpalIrpCalculateSize(10), 10);
    
    irp_stack_p = EmcpalIrpGetNextStackLocation(irp_p);

    EmcpalIrpStackSetFunction(irp_stack_p, EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL, 0);
    EmcpalExtIrpStackFlagsAnd(irp_stack_p, 0xFF);

    EmcpalExtIrpStackParamIoctlCodeSet(irp_stack_p, IOCTL_CACHE_VAULT_IMAGE); 

    EmcpalExtIrpStackParamIoctlInputSizeSet(irp_stack_p, 0);

    EmcpalExtIrpStackParamIoctlOutputSizeSet(irp_stack_p, sizeof(struct IoctlCacheVaultImageOutput));

    EmcpalExtIrpSystemBufferSet(irp_p, (PVOID)req_p); 
    EmcpalExtIrpCompletionRoutineSet(irp_p, fbe_memory_persistence_check_vault_completion, packet_p, CSX_TRUE, CSX_TRUE, CSX_TRUE);

    pal_status = EmcpalExtIrpSendAsync(fbe_memory_persistence_device_p, irp_p);

    if (pal_status != EMCPAL_STATUS_PENDING){
        memory_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s status 0x%x not expected from EmcpalExtIrpSendAsync\n", 
                                __FUNCTION__, pal_status);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_memory_persistence_check_vault_send_irp()
 ******************************************/
/*!**************************************************************
 * fbe_memory_persistence_check_vault_busy()
 ****************************************************************
 * @brief
 *  Determine if the vault is busy with some operation.
 *  
 * @param packet_p
 * 
 * @return fbe_status_t
 *
 * @author
 *  1/17/2014 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_memory_persistence_check_vault_busy(fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload_p = NULL;
    fbe_payload_persistent_memory_operation_t * persistent_memory_operation_p = NULL;
    PEMCPAL_IRP irp_p = NULL;
    fbe_u8_t *data_p = NULL;

    if (fbe_memory_persistence_device_p == NULL){
        status = fbe_memory_persistence_sp_cache_open(&fbe_memory_persistence_device_p,
                                                      &fbe_memory_persistence_file_p);
        if (status != FBE_STATUS_OK) {
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_OK;
        }
    }
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    persistent_memory_operation_p =  fbe_payload_ex_get_persistent_memory_operation(sep_payload_p);

    if (persistent_memory_operation_p->chunk_p == NULL) {
        memory_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, 
                             "%s no memory provided %p/%p\n", __FUNCTION__, 
                             persistent_memory_operation_p->sg_p, persistent_memory_operation_p->chunk_p);
        persistent_memory_operation_p->persistent_memory_status = FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_INSUFFICIENT_RESOURCES;
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }
    /* Allocate memory for the irp and control structures from the passed in information.
     */
    data_p = persistent_memory_operation_p->chunk_p;
    irp_p = (PEMCPAL_IRP)data_p;
    data_p += FBE_MEMORY_CHUNK_SIZE;

    fbe_memory_persistence_check_vault_send_irp(packet_p, irp_p, data_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_memory_persistence_check_vault_busy()
 ******************************************/
/*************************
 * end file fbe_memory_persistence_kernel.c
 *************************/
