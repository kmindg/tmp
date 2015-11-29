/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_rdgen_user_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains simulation only related functionality for rdgen.
 *
 * @version
 *   3/12/2012:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_rdgen.h"
#include "fbe_rdgen_private_inlines.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static fbe_bool_t fbe_rdgen_fail_cache_memory_alloc = FBE_FALSE;
void fbe_rdgen_release_cache_mem(void * ptr)
{
    fbe_memory_native_release(ptr);
}
void * fbe_rdgen_alloc_cache_mem(fbe_u32_t bytes)
{
    if (fbe_rdgen_fail_cache_memory_alloc)
    {
        return NULL;
    }
    return fbe_memory_native_allocate(bytes);
}
/*!**************************************************************
 * fbe_rdgen_object_size_disk_object()
 ****************************************************************
 * @brief
 *  This is our simulation entry point for opening a device.
 *  Simulation is not able to connect to these devices, but
 *  we will fake out the successful open and fetch of capacity anyway.
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
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s sim mode does not support disk device interface: %s faking out capacity\n",
                            __FUNCTION__, object_p->device_name);  
    object_p->capacity = 0x500000;
    object_p->block_size = FBE_BE_BYTES_PER_BLOCK;
    object_p->physical_block_size = FBE_BE_BYTES_PER_BLOCK;
    object_p->optimum_block_size = 1;
    object_p->device_p = (PEMCPAL_DEVICE_OBJECT)0xFFFFFFFF;
    object_p->file_p = (PEMCPAL_FILE_OBJECT)0xFFFFFFFF;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_rdgen_object_size_disk_object
 **************************************/

/*!**************************************************************
 * fbe_rdgen_ts_send_disk_device()
 ****************************************************************
 * @brief
 *  This is our simulation entry point for sending disk device irps.
 *  We currently don't send disk device IRPs in sim, so we just complete
 *  the packet with good status.
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

    fbe_u32_t usec;

    /* Set the opcode in the ts so that we know how to process errors.
     */
    fbe_rdgen_ts_set_block_opcode(ts_p, opcode);

    /* Update the last time we sent a packet for this ts.
     */
    fbe_rdgen_ts_update_last_send_time(ts_p);

    usec = fbe_get_elapsed_microseconds(ts_p->last_send_time);
    /* Update our response time statistics.
     */
    fbe_rdgen_ts_set_last_response_time(ts_p, usec);
    fbe_rdgen_ts_update_cum_resp_time(ts_p, usec);
    fbe_rdgen_ts_update_max_response_time(ts_p, usec);
    /* We just fake out status so that we can test this in simulation.
     */
    ts_p->last_packet_status.status = FBE_STATUS_OK;
    ts_p->last_packet_status.block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;
    ts_p->last_packet_status.block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;

    fbe_rdgen_ts_enqueue_to_thread(ts_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_rdgen_ts_send_disk_device()
 **************************************/

/*!**************************************************************
 * fbe_rdgen_object_close()
 ****************************************************************
 * @brief
 *  Close the device object that we opened.
 *  Since this is simulation, we just fake this out and
 *  NULL out the device and file ptrs.
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
    object_p->device_p = NULL;
    object_p->file_p = NULL;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_rdgen_object_close
 **************************************/

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
    return fbe_rdgen_object_size_disk_object(object_p);
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
fbe_status_t fbe_rdgen_object_open_bdev(int *fp, csx_cstring_t bdev)
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
fbe_status_t fbe_rdgen_object_close_bdev(int *fp)
{
    *fp = NULL;
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_object_close_bdev()
 ******************************************/

/*************************
 * end file fbe_rdgen_user_main.c
 *************************/


