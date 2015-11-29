/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_neit_api_kernel.c
 ***************************************************************************
 *
 *  Description
 *      Kernel implementation for NEIT API interface
 *      
 *
 *  History:
 *      02/03/09    Peter Puhov - Created
 *    
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_trace_interface.h"
#include "fbe/fbe_library_interface.h"
#include "fbe_service_manager.h"
#include "fbe_topology.h"
#include "fbe/fbe_neit_flare_package.h"


/*********************************************************************
 *            fbe_neit_api_init ()
 *********************************************************************
 *
 *  Description: kernel version of init
 *
 *  Inputs: 
 *
 *  Return Value: 
 *
 *  History:
 *    02/03/09    Peter Puhov - Created
 *    
 *********************************************************************/
fbe_status_t 
fbe_neit_api_init (void)
{
    EMCPAL_STATUS       status;
    PEMCPAL_FILE_OBJECT file_object = NULL;

    /*get a pointer to the physical package*/
    status = EmcpalDeviceOpen(EmcpalDriverGetCurrentClientObject(),
                              FBE_NEIT_PACKAGE_DEVICE_NAME_CHAR,
                              &file_object);

    if (!EMCPAL_IS_SUCCESS(status)) {
        fbe_base_library_trace(FBE_LIBRARY_ID_FBE_NEIT_API,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: DeviceOpen failed, status 0x%x\n",
                               __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = EmcpalDeviceIoctl(EmcpalDriverGetCurrentClientObject(),
                               file_object, FBE_NEIT_PACKAGE_INIT_IOCTL,
                               NULL, 0, NULL, 0, NULL);

    (void) EmcpalDeviceClose(file_object);

    if (!EMCPAL_IS_SUCCESS(status)) {
        fbe_base_library_trace(FBE_LIBRARY_ID_FBE_NEIT_API,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: DeviceIoctl failed, status 0x%x\n",
                               __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_neit_api_destroy ()
 *********************************************************************
 *
 *  Description: kernel version of destroy
 *
 *  Inputs: 
 *
 *  Return Value: 
 *
 *  History:
 *    02/03/09    Peter Puhov - Created
 *    
 *********************************************************************/
fbe_status_t 
fbe_neit_api_destroy (void)
{
    /* In kernel destroy is done by NEIT driver */
    return FBE_STATUS_OK; 
}

/*********************************************************************
 *            fbe_neit_api_add_record ()
 *********************************************************************
 *
 *  Description:
 *    Send FBE_NEIT_PACKAGE_ADD_RECORD_IOCTL to FBE NEIT driver
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    02/03/09    Peter Puhov - Created
 *    
 *********************************************************************/
fbe_status_t 
fbe_neit_api_add_record(fbe_neit_error_record_t * neit_error_record,
                        fbe_neit_record_handle_t * neit_record_handle)
{
    fbe_neit_package_add_record_t add_record;
    EMCPAL_STATUS                 status;
    PEMCPAL_FILE_OBJECT           file_object = NULL;

    *neit_record_handle = 0;
    fbe_copy_memory(&add_record.neit_error_record, neit_error_record,
                    sizeof (fbe_neit_error_record_t));
    
    /*get a pointer to the physical package*/
    status = EmcpalDeviceOpen(EmcpalDriverGetCurrentClientObject(),
                              FBE_NEIT_PACKAGE_DEVICE_NAME_CHAR, &file_object);

    if (!EMCPAL_IS_SUCCESS(status)) {
        fbe_base_library_trace(FBE_LIBRARY_ID_FBE_NEIT_API,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: DeviceOpen failed, status 0x%x\n",
                               __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = EmcpalDeviceIoctl(EmcpalDriverGetCurrentClientObject(),
                               file_object,
                               FBE_NEIT_PACKAGE_ADD_RECORD_IOCTL,
                               &add_record,
                               sizeof (fbe_neit_package_add_record_t),
                               &add_record,
                               sizeof (fbe_neit_package_add_record_t),
                               NULL);

    (void) EmcpalDeviceClose(file_object);

    if (!EMCPAL_IS_SUCCESS(status)) {
        fbe_base_library_trace(FBE_LIBRARY_ID_FBE_NEIT_API,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: DeviceIoctl failed, status 0x%x\n",
                               __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    *neit_record_handle =  add_record.neit_record_handle;

    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_neit_api_remove_record ()
 *********************************************************************
 *
 *  Description:
 *    Send FBE_NEIT_PACKAGE_REMOVE_RECORD_IOCTL to FBE NEIT driver
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    02/03/09    Peter Puhov - Created
 *    
 *********************************************************************/
fbe_status_t 
fbe_neit_api_remove_record(fbe_neit_record_handle_t neit_record_handle)
{
    fbe_neit_package_remove_record_t remove_record;
    EMCPAL_STATUS                    status;
    PEMCPAL_FILE_OBJECT              file_object = NULL;

    remove_record.neit_record_handle = neit_record_handle;

    /*get a pointer to the physical package*/
    status = EmcpalDeviceOpen(EmcpalDriverGetCurrentClientObject(),
                              FBE_NEIT_PACKAGE_DEVICE_NAME_CHAR, &file_object);

    if (!EMCPAL_IS_SUCCESS(status)) {
        fbe_base_library_trace(FBE_LIBRARY_ID_FBE_NEIT_API,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: DeviceOpen failed, status 0x%x\n",
                               __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = EmcpalDeviceIoctl(EmcpalDriverGetCurrentClientObject(),
                               file_object,
                               FBE_NEIT_PACKAGE_REMOVE_RECORD_IOCTL,
                               &remove_record,
                               sizeof (fbe_neit_package_remove_record_t),
                               &remove_record,
                               sizeof (fbe_neit_package_remove_record_t),
                               NULL);

    (void) EmcpalDeviceClose(file_object);

    if (!EMCPAL_IS_SUCCESS(status)) {
        fbe_base_library_trace(FBE_LIBRARY_ID_FBE_NEIT_API,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: DeviceIoctl failed, status 0x%x\n",
                               __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_neit_api_remove_object ()
 *********************************************************************
 *
 *  Description:
 *    Send FBE_NEIT_PACKAGE_REMOVE_OBJECT_IOCTL to FBE NEIT driver
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    02/03/09    Peter Puhov - Created
 *    
 *********************************************************************/
fbe_status_t 
fbe_neit_api_remove_object(fbe_object_id_t object_id)
{
    fbe_neit_package_remove_object_t remove_object;
    EMCPAL_STATUS                    status;
    PEMCPAL_FILE_OBJECT              file_object = NULL;

    remove_object.object_id = object_id;

    /*get a pointer to the physical package*/
    status = EmcpalDeviceOpen(EmcpalDriverGetCurrentClientObject(),
                              FBE_NEIT_PACKAGE_DEVICE_NAME_CHAR, &file_object);

    if (!EMCPAL_IS_SUCCESS(status)) {
        fbe_base_library_trace(FBE_LIBRARY_ID_FBE_NEIT_API,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: DeviceOpen failed, status 0x%x\n",
                               __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = EmcpalDeviceIoctl(EmcpalDriverGetCurrentClientObject(),
                               file_object,
                               FBE_NEIT_PACKAGE_REMOVE_OBJECT_IOCTL,
                               &remove_object,
                               sizeof (fbe_neit_package_remove_object_t),
                               &remove_object,
                               sizeof (fbe_neit_package_remove_object_t),
                               NULL);

    (void) EmcpalDeviceClose(file_object);

    if (!EMCPAL_IS_SUCCESS(status)) {
        fbe_base_library_trace(FBE_LIBRARY_ID_FBE_NEIT_API,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: DeviceIoctl failed, status 0x%x\n",
                               __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_neit_api_start ()
 *********************************************************************
 *
 *  Description:
 *    Send FBE_NEIT_PACKAGE_START_IOCTL to FBE NEIT driver
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    02/03/09    Peter Puhov - Created
 *    
 *********************************************************************/
fbe_status_t 
fbe_neit_api_start(void)
{
    EMCPAL_STATUS       status;
    PEMCPAL_FILE_OBJECT file_object = NULL;

    /*get a pointer to the physical package*/
    status = EmcpalDeviceOpen(EmcpalDriverGetCurrentClientObject(),
                              FBE_NEIT_PACKAGE_DEVICE_NAME_CHAR, &file_object);

    if (!EMCPAL_IS_SUCCESS(status)) {
        fbe_base_library_trace(FBE_LIBRARY_ID_FBE_NEIT_API,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: DeviceOpen failed, status 0x%x\n",
                               __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = EmcpalDeviceIoctl(EmcpalDriverGetCurrentClientObject(),
                               file_object, FBE_NEIT_PACKAGE_START_IOCTL,
                               NULL, 0, NULL, 0, NULL);

    (void) EmcpalDeviceClose(file_object);

    if (!EMCPAL_IS_SUCCESS(status)) {
        fbe_base_library_trace(FBE_LIBRARY_ID_FBE_NEIT_API,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: DeviceIoctl failed, status 0x%x\n",
                               __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_neit_api_stop ()
 *********************************************************************
 *
 *  Description:
 *    Send FBE_NEIT_PACKAGE_STOP_IOCTL to FBE NEIT driver
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    02/03/09    Peter Puhov - Created
 *    
 *********************************************************************/
fbe_status_t 
fbe_neit_api_stop(void)
{
    EMCPAL_STATUS       status;
    PEMCPAL_FILE_OBJECT file_object = NULL;

    /*get a pointer to the physical package*/
    status = EmcpalDeviceOpen(EmcpalDriverGetCurrentClientObject(),
                              FBE_NEIT_PACKAGE_DEVICE_NAME_CHAR, &file_object);

    if (!EMCPAL_IS_SUCCESS(status)) {
        fbe_base_library_trace(FBE_LIBRARY_ID_FBE_NEIT_API,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: DeviceOpen failed, status 0x%x\n",
                               __FUNCTION__, status);

            return FBE_STATUS_GENERIC_FAILURE;
    }
    status = EmcpalDeviceIoctl(EmcpalDriverGetCurrentClientObject(),
                               file_object, FBE_NEIT_PACKAGE_STOP_IOCTL,
                               NULL, 0, NULL, 0, NULL);

    (void) EmcpalDeviceClose(file_object);

    if (!EMCPAL_IS_SUCCESS(status)) {
        fbe_base_library_trace(FBE_LIBRARY_ID_FBE_NEIT_API,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: DeviceIoctl failed, status 0x%x\n",
                               __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_neit_api_get_record ()
 *********************************************************************
 *
 *  Description:
 *    Send FBE_NEIT_PACKAGE_GET_RECORD_IOCTL to FBE NEIT driver
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    10/31/09    Christina Chiang - Created
 *    
 *********************************************************************/
fbe_status_t 
fbe_neit_api_get_record(fbe_neit_record_handle_t neit_record_handle,
                        fbe_neit_error_record_t * neit_error_record)
{
    fbe_neit_package_get_record_t get_record;
    EMCPAL_STATUS                 status;
    PEMCPAL_FILE_OBJECT           file_object = NULL;

    get_record.neit_record_handle = neit_record_handle;

    /*get a pointer to the physical package*/
    status = EmcpalDeviceOpen(EmcpalDriverGetCurrentClientObject(),
                              FBE_NEIT_PACKAGE_DEVICE_NAME_CHAR, &file_object);

    if (!EMCPAL_IS_SUCCESS(status)) {
        fbe_base_library_trace(FBE_LIBRARY_ID_FBE_NEIT_API,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: DeviceOpen failed, status 0x%x\n",
                               __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = EmcpalDeviceIoctl(EmcpalDriverGetCurrentClientObject(),
                               file_object,
                               FBE_NEIT_PACKAGE_GET_RECORD_IOCTL,
                               &get_record,
                               sizeof (fbe_neit_package_get_record_t),
                               &get_record,
                               sizeof (fbe_neit_package_get_record_t),
                               NULL);

    (void) EmcpalDeviceClose(file_object);

    if (!EMCPAL_IS_SUCCESS(status)) {
        fbe_base_library_trace(FBE_LIBRARY_ID_FBE_NEIT_API,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: DeviceIoctl failed, status 0x%x\n",
                               __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_copy_memory(neit_error_record, &get_record.neit_error_record,
                    sizeof (fbe_neit_error_record_t));

    return FBE_STATUS_OK;
}
