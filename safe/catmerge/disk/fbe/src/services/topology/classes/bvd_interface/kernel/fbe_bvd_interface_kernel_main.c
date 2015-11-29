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
 *  This file contains the kernel implementation main entry points for the basic volume driver object.
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

#ifdef EMCPAL_USE_CSX_DCALL
#include "ublockshim_ng_api.h"
#endif
/*****************************************
 * LOCAL Variables DEFINITIONS
 *****************************************/
#define FBE_BVD_DEVICE_NAME_MAX_LENGTH 40


PEMCPAL_NATIVE_DRIVER_OBJECT 			driver_ptr = NULL;

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/

static void bvd_interface_get_device_string(fbe_u8_t *device_naming, fbe_u32_t length);

/************************************************************************************************************************************/													

fbe_status_t 
fbe_bvd_interface_load(void)
{
    FBE_ASSERT_AT_COMPILE_TIME(FBE_OS_OPAQUE_DEV_INFO_SIZE > sizeof(bvd_lun_device_exnention_t));
    fbe_bvd_interface_monitor_load_verify();
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_bvd_interface_unload(void)
{
    /* Destroy any global data.*/
    return FBE_STATUS_OK;
}


fbe_status_t fbe_bvd_interface_init (fbe_bvd_interface_t *bvd_object_p)
{
    fbe_status_t		status = FBE_STATUS_GENERIC_FAILURE;
    

    status = sep_get_driver_object_ptr(&driver_ptr);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s failed to get driver pointer, status %d\n", __FUNCTION__, status);

    }

    return FBE_STATUS_OK;

}


static fbe_status_t fbe_bvd_interface_export_os_device_internal(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *os_device_info)
{
    EMCPAL_STATUS               status = EMCPAL_STATUS_SUCCESS;
    bvd_lun_device_exnention_t *bvd_lun_device_extention = (bvd_lun_device_exnention_t *)os_device_info->opaque_dev_info; 

    status = EmcpalExtDeviceCreate(driver_ptr,
                            0,/*when passing 0 it means we have the memory*/
                            bvd_lun_device_extention->cDevName,
                            FBE_BVD_LUN_DEVICE_TYPE,
                            0,	// DeviceCharacteristics,
                            FBE_FALSE,
                            &bvd_lun_device_extention->device_object);

    if ( !EMCPAL_IS_SUCCESS(status)){
        fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s failed to create Windows device, status:0x%X\n\n", __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    EmcpalExtDeviceIoDirectSet(bvd_lun_device_extention->device_object);

    /* Init device_extension */
    EmcpalExtDeviceExtensionSet(bvd_lun_device_extention->device_object, (PVOID)os_device_info);

    /* Create a link from our device name to a name in the Win32 namespace. */
    status = EmcpalExtDeviceAliasCreateA( bvd_lun_device_extention->cDevLinkName, bvd_lun_device_extention->cDevName );

    if ( !EMCPAL_IS_SUCCESS(status) ) {
        fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s IoCreateSymbolicLink failed with status:0x%X\n", __FUNCTION__, status);
        EmcpalExtDeviceDestroy(bvd_lun_device_extention->device_object);
        return FBE_STATUS_GENERIC_FAILURE;
    }	


    EmcpalExtDeviceEnable(bvd_lun_device_extention->device_object);

	fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s Exporting: %s \n", __FUNCTION__, bvd_lun_device_extention->cDevLinkName);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_bvd_interface_export_os_device(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *os_device_info)
{
    fbe_u32_t                   lun_num = (fbe_u32_t)os_device_info->lun_number;
    EMCPAL_STATUS               status = EMCPAL_STATUS_SUCCESS;
    bvd_lun_device_exnention_t *bvd_lun_device_extention = (bvd_lun_device_exnention_t *)os_device_info->opaque_dev_info; 
    fbe_u8_t         			bvd_device_naming[FBE_BVD_DEVICE_NAME_MAX_LENGTH];

     /*set some default bits on the volume we are about to export, we don't set the previous here so we can
       guarantee notification to the Cache layer above us the firt time it registers*/
     os_device_info->current_attributes |= (VOL_ATTR_FORCE_ON_BIT1 | VOL_ATTR_FORCE_ON_BIT2);

     /* set default value for ready state */
     os_device_info->ready_state = FBE_TRI_STATE_INVALID;

     if (os_device_info->export_lun) {
         fbe_zero_memory(bvd_device_naming, FBE_BVD_DEVICE_NAME_MAX_LENGTH);
         fbe_copy_memory(bvd_device_naming, "ExportLun", 9);
     } else {
         bvd_interface_get_device_string(bvd_device_naming, FBE_BVD_DEVICE_NAME_MAX_LENGTH);
     }

     fbe_zero_memory(bvd_lun_device_extention->cDevName, 64);
     fbe_sprintf(bvd_lun_device_extention->cDevName, 64, "\\Device\\%s%d", bvd_device_naming, lun_num);

     fbe_zero_memory(bvd_lun_device_extention->cDevLinkName, 64);
     fbe_sprintf(bvd_lun_device_extention->cDevLinkName, 64, "\\DosDevices\\%sLink%d", bvd_device_naming, lun_num);

     status = fbe_bvd_interface_export_os_device_internal(bvd_object_p, os_device_info);

     // Return the status
     return status;
}

fbe_status_t fbe_bvd_interface_unexport_os_device(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *os_device_info)
{
    EMCPAL_STATUS               status = EMCPAL_STATUS_SUCCESS;
    bvd_lun_device_exnention_t *bvd_lun_device_extention = (bvd_lun_device_exnention_t *)os_device_info->opaque_dev_info; 

    /* This name was created before in fbe_bvd_export_os_device */
    status = EmcpalExtDeviceAliasDestroyA(bvd_lun_device_extention->cDevLinkName);

    if ( !EMCPAL_IS_SUCCESS(status) ) {
        fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s IoDeleteSymbolicLink failed with status:0x%X\n", __FUNCTION__, status);
  
        return FBE_STATUS_GENERIC_FAILURE;
    }

    EmcpalExtDeviceDestroy(bvd_lun_device_extention->device_object);

    return FBE_STATUS_OK;
}


fbe_status_t fbe_bvd_interface_export_os_special_device(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *os_device_info,
                                                        csx_puchar_t dev_name, csx_puchar_t link_name)
{
    EMCPAL_STATUS               status = EMCPAL_STATUS_SUCCESS;
    bvd_lun_device_exnention_t *bvd_lun_device_extention = (bvd_lun_device_exnention_t *)os_device_info->opaque_dev_info; 
    
    csx_p_sprintf(bvd_lun_device_extention->cDevName, dev_name);
    csx_p_sprintf(bvd_lun_device_extention->cDevLinkName, link_name);
    
    status = fbe_bvd_interface_export_os_device_internal(bvd_object_p, os_device_info);
    
    // return the status
    return status;
}

fbe_status_t fbe_bvd_interface_destroy (fbe_bvd_interface_t *bvd_object_p)
{
    return FBE_STATUS_OK;
}

static void bvd_interface_get_device_string(fbe_u8_t *device_naming, fbe_u32_t str_legth)
{
    fbe_status_t status;

    fbe_zero_memory(device_naming, FBE_BVD_DEVICE_NAME_MAX_LENGTH);

    status = fbe_registry_read(NULL,
                               SEP_REG_PATH,
                               "SepDeviceNamingConvention",
                               device_naming,
                               FBE_BVD_DEVICE_NAME_MAX_LENGTH,
                               FBE_REGISTRY_VALUE_SZ,
                               "CLARiiONdisk",/*"FlareDisk",*/
                               FBE_BVD_DEVICE_NAME_MAX_LENGTH,
                               FALSE);

    if (status != FBE_STATUS_OK) {
        fbe_copy_memory(device_naming, "CLARiiONdisk", 12);
    }

    return;
}
