/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_bvd_interface_kernel_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the kernel implementation for export luns in BVD.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "k10ntddk.h"
#include "EmcPAL_DriverShell.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_bvd_interface.h"
#include "fbe_bvd_interface_kernel.h"
#include "fbe/fbe_types.h"
#include "fbe_bvd_interface_private.h"
#include "fbe/fbe_sep.h"
#include "fbe/fbe_sep_kernel_interface.h"
#include "fbe/fbe_registry.h"
#include "VolumeAttributes.h"
#include "fbe_database.h"
#include "fbe_private_space_layout.h"
#include "fbe_cmi.h"

/*****************************************
 * LOCAL Variables DEFINITIONS
 *****************************************/
#include "ublockshim_ng_api.h"

typedef struct export_device_s {
    PEMCPAL_DEVICE_OBJECT               pDeviceObject;
    fbe_lba_t                           offset;    
    ublockshim_ng_block_device_handle_t blkshim_device;
}export_device_t;

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
extern EMCPAL_STATUS fbe_sep_shim_process_asynch_read_write_irp(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP	PIrp);
extern fbe_status_t fbe_database_get_export_lun_by_lun_number(fbe_u32_t lun_number, fbe_private_space_layout_lun_info_t * lun_info);
extern fbe_status_t fbe_database_get_platform_info(fbe_board_mgmt_platform_info_t *platform_info);
fbe_status_t fbe_bvd_interface_get_export_device_name(fbe_u32_t lun_number, fbe_u8_t *device_name, fbe_u32_t len);
fbe_status_t fbe_bvd_interface_get_export_device_offset(fbe_u32_t lun_number, fbe_lba_t *offset);

/*****************************************
 * LOCAL FUNCTIONS
 *****************************************/

/*!****************************************************************************
 * fbe_export_lun_read_write()
 ******************************************************************************
 * @brief
 *  This is the I/O function from BLOCK SHIM device.
 * 
 * @param  PDeviceObject     - Pointer to device object
 * @param  pIrp              - Pointer to an irp
 *
 * @return status            - status of the operation.
 *
 * @author
 *  09/05/2014 - Created. Lili Chen
 *
 ******************************************************************************/
EMCPAL_STATUS fbe_export_lun_read_write(IN PEMCPAL_DEVICE_OBJECT PDeviceObject, PEMCPAL_IRP pIrp)
{
    export_device_t * export_dev;
    PEMCPAL_DEVICE_OBJECT pDeviceObject = NULL;
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;
    PEMCPAL_IO_STACK_LOCATION pCurrentIrpStack = NULL;
    PEMCPAL_IO_STACK_LOCATION pIoStackLocation = NULL;

    //IRQL_APC();  // IRQ <= APC level 

    if ((PDeviceObject == NULL) || (pIrp == NULL))
    {
        EmcpalExtIrpStatusFieldSet(pIrp, EMCPAL_STATUS_INVALID_DEVICE_REQUEST);
        EmcpalExtIrpInformationFieldSet(pIrp, 0);
        EmcpalIrpSetNextStackLocation(pIrp);
        EmcpalIrpCompleteRequest(pIrp);
        return EMCPAL_STATUS_INVALID_DEVICE_REQUEST;
    }

#if 0
    if (EmcpalExtIrpMdlAddressGet(pIrp) != NULL)
    {
        EmcpalExtIrpStatusFieldSet(pIrp, EMCPAL_STATUS_INVALID_DEVICE_REQUEST);
        EmcpalExtIrpInformationFieldSet(pIrp, 0);
        EmcpalIrpSetNextStackLocation(pIrp);
        EmcpalIrpCompleteRequest(pIrp);
        return EMCPAL_STATUS_INVALID_DEVICE_REQUEST;
    }
#endif

    export_dev = (export_device_t *)PDeviceObject;
    pDeviceObject = export_dev->pDeviceObject;

    /* Get a pointer to the current stack location within this IRP.
     */
    pCurrentIrpStack = EmcpalIrpGetCurrentStackLocation(pIrp);

    if (EmcpalExtIrpStackMajorFunctionGet(pCurrentIrpStack) == EMCPAL_IRP_TYPE_CODE_READ ||
        EmcpalExtIrpStackMajorFunctionGet(pCurrentIrpStack) == EMCPAL_IRP_TYPE_CODE_WRITE)
    {
        fbe_u8_t                    operation = 0;
        fbe_u32_t                   transfer_size = 0;
        fbe_lba_t                   lba = 0;

        operation = EmcpalExtIrpStackMajorFunctionGet(pCurrentIrpStack);
        lba = EmcpalIrpParamGetWriteByteOffset(pCurrentIrpStack)/(LONGLONG)FBE_BYTES_PER_BLOCK;
        lba += export_dev->offset;
        transfer_size = EmcpalIrpParamGetWriteLength(pCurrentIrpStack);
        /*fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO,
                      "%s: BLKSHIM irp %p opcode %d, lba 0x%llx, size 0x%x\n",
                      __FUNCTION__, pIrp, operation, lba, transfer_size);*/

        pIoStackLocation = EmcpalIrpGetNextStackLocation(pIrp);
        EmcpalIrpStackSetFunction(pIoStackLocation, operation, 0);    
        EmcpalIrpGetReadParams(pIoStackLocation)->io_size_requested = EmcpalIrpGetReadParams(pCurrentIrpStack)->io_size_requested;
        EmcpalIrpGetReadParams(pIoStackLocation)->io_offset = lba * (LONGLONG)FBE_BYTES_PER_BLOCK;
        EmcpalIrpGetReadParams(pIoStackLocation)->io_buffer = EmcpalIrpGetReadParams(pCurrentIrpStack)->io_buffer;
        EmcpalIrpStackSetCurrentDeviceObject(pIoStackLocation, pDeviceObject);
        EmcpalIrpSetNextStackLocation(pIrp);

        status = fbe_sep_shim_process_asynch_read_write_irp(pDeviceObject, pIrp);
    }
    else /* IOCTL_DISK_VERIFY */
    {
        EmcpalExtIrpStatusFieldSet(pIrp, EMCPAL_STATUS_INVALID_DEVICE_REQUEST);
        EmcpalExtIrpInformationFieldSet(pIrp, 0);
        EmcpalIrpSetNextStackLocation(pIrp);
        EmcpalIrpCompleteRequest(pIrp);
        return EMCPAL_STATUS_INVALID_DEVICE_REQUEST;
    }

    // TODO Currently return success to make ublockshim happy
    if (status == EMCPAL_STATUS_PENDING) {
        status = EMCPAL_STATUS_SUCCESS;
    }

    return status;
}

CSX_LOCAL
csx_status_e
fbe_blockshim_io_callback (csx_p_dcall_body_t *dcall_object, csx_pvoid_t io_context)
{
    PEMCPAL_DEVICE_OBJECT   device_object   = (PEMCPAL_DEVICE_OBJECT) io_context;
    PEMCPAL_IRP             irp             = (PEMCPAL_IRP) dcall_object;

    /*
     * blockshim_ng does not call dcal_send.  Instead, it sets up the *next* stack
     * location in the dcall and directly calls the IO routine. So increment the
     * stack location to *next*.
     */
    csx_p_dcall_goto_next_level (dcall_object);
    return ((csx_status_e) (fbe_export_lun_read_write (device_object, irp)));
}

CSX_LOCAL csx_status_e fbe_bvd_interface_register_blockshim_device(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *os_device_info_p)
{
    csx_status_e                                status      = CSX_STATUS_SUCCESS;
    ublockshim_ng_block_device_handle_t         fbe_blockshim_handle;
    ublockshim_ng_add_block_device_config_t     uConfigBuf;
    kblockshim_ng_add_block_device_input_ss_t   kConfigBuf;
    fbe_block_edge_t                            *block_edge_p;
    fbe_lba_t                                   capacity;
    export_device_t                             *export_device_p;

    // Initialize
    block_edge_p = &os_device_info_p->block_edge;
    export_device_p = os_device_info_p->export_device;

     // Init the user config buffer
    csx_p_memzero (&uConfigBuf, sizeof (ublockshim_ng_add_block_device_config_t));
    ublockshim_ng_add_block_device_config_init (&uConfigBuf);
    uConfigBuf.do_io_callback = fbe_blockshim_io_callback;
    uConfigBuf.io_context = export_device_p;
    uConfigBuf.dcall_levels = 2;

     // Init the kernel config buffer
    csx_p_memzero (&kConfigBuf, sizeof (kblockshim_ng_add_block_device_input_ss_t));
    kblockshim_ng_add_block_device_input_init (&kConfigBuf);

    // Get capacity.
    fbe_block_transport_edge_get_capacity(block_edge_p, &capacity);
    kConfigBuf.size_in_sectors = capacity - export_device_p->offset - 1;

    // Get device name.
    fbe_bvd_interface_get_export_device_name(os_device_info_p->lun_number, 
                                             &kConfigBuf.device_name[0], 
                                             KBLOCKSHIM_NG_BLOCK_DEVICE_NAME_MAX_LEN);

    status = ublockshim_ng_register_device (CSX_MY_MODULE_CONTEXT (),
                                            &kConfigBuf,
                                            &uConfigBuf,
                                            &fbe_blockshim_handle);
    if (CSX_FAILURE (status))
    {
        fbe_base_object_trace((fbe_base_object_t *)bvd_object_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s register device %s failed status 0x%x.\n", 
                              __FUNCTION__, kConfigBuf.device_name, status);

    }

    else
    {
        // Store the blkshim device handle
        export_device_p->blkshim_device = fbe_blockshim_handle;
        fbe_base_object_trace((fbe_base_object_t *)bvd_object_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s register %s succeeded\n",
                              __FUNCTION__, kConfigBuf.device_name);
    }

    return status;
}

CSX_LOCAL csx_status_e fbe_bvd_interface_unregister_blockshim_device(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *os_device_info_p)
{
    csx_status_e status;
    
    status = ublockshim_ng_unregister_device(((export_device_t *)(os_device_info_p->export_device))->blkshim_device);
    if (status != CSX_STATUS_SUCCESS)
    {
        fbe_base_object_trace((fbe_base_object_t *)bvd_object_p,
                    FBE_TRACE_LEVEL_ERROR,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s, unexport device failed status 0x%x.\n", __FUNCTION__, status);
    }

    return status; 
}


/*!****************************************************************************
 * fbe_bvd_interface_update_export_device_info()
 ******************************************************************************
 * @brief
 *  This function updates device info from system layout.
 * 
 * @param  bvd_object_p      - Pointer to bvd object
 * @param  device_info_p     - Pointer to device info
 *
 * @return status            - status of the operation.
 *
 * @author
 *  09/05/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t fbe_bvd_interface_update_export_device_info(fbe_bvd_interface_t *bvd_object_p, fbe_export_device_info_t * device_info_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_private_space_layout_region_t region;

    status = fbe_private_space_layout_get_region_by_region_id(device_info_p->region_id, &region);
    if (status != FBE_STATUS_OK) {
		fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s, can't get region 0x%x\n", __FUNCTION__, device_info_p->region_id);
        return status;
    }

    device_info_p->start_address = region.starting_block_address;
    device_info_p->size_in_blocks = region.size_in_blocks;

    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_bvd_interface_get_export_device_name()
 ******************************************************************************
 * @brief
 *  This function exports devices for a lun.
 * 
 * @param  lun_number        - lun number
 * @param  device_name       - Pointer to device name buffer
 * @param  len               - max length of the device name
 *
 * @return status            - status of the operation.
 *
 * @author
 *  02/07/2015 - Created. Jibing Dong
 *
 ******************************************************************************/
fbe_status_t fbe_bvd_interface_get_export_device_name(fbe_u32_t lun_number, fbe_u8_t *device_name, fbe_u32_t len)
{
    fbe_status_t status;
    fbe_private_space_layout_lun_info_t lun_info;

    status = fbe_database_get_export_lun_by_lun_number(lun_number, &lun_info);
    if (status == FBE_STATUS_OK)
    {
        /* c4mirror lun */
        fbe_zero_memory(device_name, len);
        fbe_sprintf(device_name, len, "%s", lun_info.exported_device_name);
    }
    else
    {
        /* user export lun */
        fbe_zero_memory(device_name, len);
        fbe_sprintf(device_name, len, "fbelun%d", lun_number);
    }

    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_bvd_interface_get_export_device_offset()
 ******************************************************************************
 * @brief
 *  This function exports devices for a lun.
 * 
 * @param  lun_number        - lun number
 * @param  offset            - Pointer to offset
 *
 * @return status            - status of the operation.
 *
 * @author
 *  02/07/2015 - Created. Jibing Dong
 *
 ******************************************************************************/
fbe_status_t fbe_bvd_interface_get_export_device_offset(fbe_u32_t lun_number, fbe_lba_t *offset)
{
    fbe_status_t status;
    fbe_private_space_layout_lun_info_t lun_info;
    fbe_board_mgmt_platform_info_t platform_info;

    *offset = 0;

    status = fbe_database_get_export_lun_by_lun_number(lun_number, &lun_info);
    if (status == FBE_STATUS_OK) {
        if (lun_info.object_id == FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_UTILITY_BOOT_VOLUME_SPA ||
            lun_info.object_id == FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_UTILITY_BOOT_VOLUME_SPB )
        {
            /*
             * At the start of mirror area, c4mirror reserve 2 sectors for the MVS and MDB sectors.
             * For BEACHCOMBER, we need to keep the geometry of mirror device unchanged to support NDU (from KH+).
             * For MOONS, DDBS is replaced with FAT32, we don't need 2 sectors offset any more.
             */
            status = fbe_database_get_platform_info(&platform_info);
            if (status == FBE_STATUS_OK) {
                if (platform_info.hw_platform_info.cpuModule == BEACHCOMBER_CPU_MODULE) {
                    *offset = 0x2;
                }
                else {
                    *offset = 0x0;
                }
            }
        }
        else if (lun_info.object_id == FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_PRIMARY_BOOT_VOLUME_SPA ||
                 lun_info.object_id == FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_PRIMARY_BOOT_VOLUME_SPB )
        {
            /*
             * mirrora is 128-sector aligned (more or less)
             */
            *offset = 0x88;
        }
    }

    return FBE_STATUS_OK;
}
/*!****************************************************************************
 * fbe_bvd_interface_export_lun()
 ******************************************************************************
 * @brief
 *  This function exports devices for a lun.
 * 
 * @param  bvd_object_p      - Pointer to bvd object
 * @param  os_device_info_p  - Pointer to OS device info
 *
 * @return status            - status of the operation.
 *
 * @author
 *  09/05/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t fbe_bvd_interface_export_lun(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *os_device_info_p)
{
    csx_status_e csx_status = CSX_STATUS_SUCCESS;
    bvd_lun_device_exnention_t *bvd_lun_device_extention;
    export_device_t * export_dev = NULL;

    export_dev = (export_device_t *) fbe_memory_ex_allocate(sizeof(export_device_t));
    if (export_dev == NULL) {
        fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s, can't allocate memory for blkshim device\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_zero_memory(export_dev, sizeof(export_device_t));

    bvd_lun_device_extention = (bvd_lun_device_exnention_t *)os_device_info_p->opaque_dev_info; 
    export_dev->pDeviceObject = bvd_lun_device_extention->device_object;
    fbe_bvd_interface_get_export_device_offset(os_device_info_p->lun_number, &export_dev->offset);
    os_device_info_p->export_device = (void *)export_dev;

    csx_status = fbe_bvd_interface_register_blockshim_device(bvd_object_p, os_device_info_p);
    if (csx_status != CSX_STATUS_SUCCESS)
    {
        os_device_info_p->export_device = NULL;
        fbe_memory_ex_release(export_dev);

        fbe_base_object_trace((fbe_base_object_t *)bvd_object_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s register device failed status 0x%x.\n", 
                              __FUNCTION__, csx_status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)bvd_object_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s register lun %d succeeded.\n",
                              __FUNCTION__, os_device_info_p->lun_number);
    }

    return FBE_STATUS_OK;    
}

/*!****************************************************************************
 * fbe_bvd_interface_unexport_lun()
 ******************************************************************************
 * @brief
 *  This function unexport the blockshim devices on the lun.
 * 
 * @param  bvd_object_p      - Pointer to bvd object
 * @param  os_device_info_p  - Pointer to OS device info
 *
 * @return status            - status of the operation.
 *
 * @author
 *  11/05/2014 - Created. Jibing Dong
 *
 ******************************************************************************/
fbe_status_t fbe_bvd_interface_unexport_lun(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *os_device_info_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    csx_status_e csx_status;

    if (os_device_info_p->export_device)
    {
        csx_status = fbe_bvd_interface_unregister_blockshim_device(bvd_object_p, os_device_info_p);
        if (csx_status != CSX_STATUS_SUCCESS)
        {
            fbe_base_object_trace((fbe_base_object_t *)bvd_object_p,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, unexport device failed status 0x%x.\n", __FUNCTION__, csx_status);
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        else
        {
            fbe_memory_ex_release(os_device_info_p->export_device);
            os_device_info_p->export_device = NULL;
        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)bvd_object_p,
                    FBE_TRACE_LEVEL_ERROR,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s, lun %d is not exported.\n", __FUNCTION__, os_device_info_p->lun_number);
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

