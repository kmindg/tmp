/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_dest_api_kernel.c
 ***************************************************************************
 *
 *  Description
 *      Kernel implementation for DEST API interface
 *      
 *
 *  History:
 *      04/25/11 - Created.  Wayne Garrett
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
 *            fbe_dest_api_init ()
 *********************************************************************
 *
 *  Description: kernel version of init
 *
 *	Inputs: 
 *
 *  Return Value: 
 *
 *  History:
 *      04/25/11 - Created.  Wayne Garrett
 *    
 *********************************************************************/
fbe_status_t 
fbe_dest_api_init (void)
{
	EMCPAL_STATUS		status;
	PEMCPAL_FILE_OBJECT	file_object = NULL;

	/*get a pointer to the physical package*/
	status = EmcpalDeviceOpen(EmcpalDriverGetCurrentClientObject(),
				  FBE_NEIT_PACKAGE_DEVICE_NAME_CHAR,
				  &file_object);

	if (!EMCPAL_IS_SUCCESS(status)) {
		fbe_base_library_trace(FBE_LIBRARY_ID_FBE_DEST_API,
				       FBE_TRACE_LEVEL_ERROR,
				       FBE_TRACE_MESSAGE_ID_INFO,
				       "%s: DeviceOpen failed, status 0x%x\n",
				       __FUNCTION__, status);

	        return FBE_STATUS_GENERIC_FAILURE;
	}
	status = EmcpalDeviceIoctl(EmcpalDriverGetCurrentClientObject(),
				   file_object, FBE_DEST_PACKAGE_INIT_IOCTL,
				   NULL, 0, NULL, 0, NULL);

	(void) EmcpalDeviceClose(file_object);

	if (!EMCPAL_IS_SUCCESS(status)) {
		fbe_base_library_trace(FBE_LIBRARY_ID_FBE_DEST_API,
				       FBE_TRACE_LEVEL_ERROR,
				       FBE_TRACE_MESSAGE_ID_INFO,
				       "%s: DeviceIoctl failed, status 0x%x\n",
					__FUNCTION__, status);

		return FBE_STATUS_GENERIC_FAILURE;
	}
	return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_dest_api_destroy ()
 *********************************************************************
 *
 *  Description: kernel version of destroy
 *
 *	Inputs: 
 *
 *  Return Value: 
 *
 *  History:
 *      04/25/11 - Created.  Wayne Garrett
 *    
 *********************************************************************/
fbe_status_t 
fbe_dest_api_destroy (void)
{
	/* In kernel destroy is done by DEST driver */
	return FBE_STATUS_OK; 
}

/*********************************************************************
 *            fbe_dest_api_add_record ()
 *********************************************************************
 *
 *  Description:
 *    Send FBE_DEST_PACKAGE_ADD_RECORD_IOCTL to FBE DEST driver
 *
 *  Return Value:  success or failure
 *
 *  History:
 *      04/25/11 - Created.  Wayne Garrett
 *    
 *********************************************************************/
fbe_status_t 
fbe_dest_api_add_record(fbe_dest_error_record_t * dest_error_record,
		        fbe_dest_record_handle_t * dest_record_handle)
{
	fbe_dest_add_record_t	add_record;
	EMCPAL_STATUS 		status;
	PEMCPAL_FILE_OBJECT 	file_object = NULL;

	*dest_record_handle = 0;

	fbe_copy_memory(&add_record.dest_error_record, dest_error_record,
			sizeof (fbe_dest_error_record_t));

	/*get a pointer to the physical package*/
	status = EmcpalDeviceOpen(EmcpalDriverGetCurrentClientObject(),
				  FBE_NEIT_PACKAGE_DEVICE_NAME_CHAR,
				  &file_object);

	if (!EMCPAL_IS_SUCCESS(status)) {
		fbe_base_library_trace(FBE_LIBRARY_ID_FBE_DEST_API,
				       FBE_TRACE_LEVEL_ERROR,
				       FBE_TRACE_MESSAGE_ID_INFO,
				       "%s: DeviceOpen failed, status 0x%x\n",
				       __FUNCTION__, status);

	        return FBE_STATUS_GENERIC_FAILURE;
	}
	status = EmcpalDeviceIoctl(EmcpalDriverGetCurrentClientObject(),
				   file_object,
				   FBE_DEST_PACKAGE_ADD_RECORD_IOCTL,
				   &add_record, sizeof (fbe_dest_add_record_t),
				   &add_record, sizeof (fbe_dest_add_record_t),
				   NULL);

	(void) EmcpalDeviceClose(file_object);

	if (!EMCPAL_IS_SUCCESS(status)) {
		fbe_base_library_trace(FBE_LIBRARY_ID_FBE_DEST_API,
				       FBE_TRACE_LEVEL_ERROR,
				       FBE_TRACE_MESSAGE_ID_INFO,
				       "%s: DeviceIoctl failed, status 0x%x\n",
				       __FUNCTION__, status);

		return FBE_STATUS_GENERIC_FAILURE;
	}
	*dest_record_handle = add_record.dest_record_handle;

	return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_dest_api_remove_record ()
 *********************************************************************
 *
 *  Description:
 *    Send FBE_DEST_PACKAGE_REMOVE_RECORD_IOCTL to FBE DEST driver
 *
 *  Return Value:  success or failure
 *
 *  History:
 *      04/25/11 - Created.  Wayne Garrett
 *    
 *********************************************************************/
fbe_status_t 
fbe_dest_api_remove_record(fbe_dest_record_handle_t dest_record_handle)
{
	fbe_dest_remove_record_t	remove_record;
	EMCPAL_STATUS 			status;
	PEMCPAL_FILE_OBJECT 		file_object = NULL;

	remove_record.dest_record_handle = dest_record_handle;

	/*get a pointer to the physical package*/
	status = EmcpalDeviceOpen(EmcpalDriverGetCurrentClientObject(),
				  FBE_NEIT_PACKAGE_DEVICE_NAME_CHAR,
				  &file_object);

	if (!EMCPAL_IS_SUCCESS(status)) {
		fbe_base_library_trace(FBE_LIBRARY_ID_FBE_DEST_API,
				       FBE_TRACE_LEVEL_ERROR,
				       FBE_TRACE_MESSAGE_ID_INFO,
				       "%s: DeviceOpen failed, status 0x%x\n",
				       __FUNCTION__, status);

	        return FBE_STATUS_GENERIC_FAILURE;
	}

	status = EmcpalDeviceIoctl(EmcpalDriverGetCurrentClientObject(),
				   file_object,
    				   FBE_DEST_PACKAGE_REMOVE_RECORD_IOCTL,
				   &remove_record,
				   sizeof (fbe_dest_remove_record_t),
				   &remove_record,
				   sizeof (fbe_dest_remove_record_t),
				   NULL);

	(void) EmcpalDeviceClose(file_object);

	if (!EMCPAL_IS_SUCCESS(status)) {
		fbe_base_library_trace(FBE_LIBRARY_ID_FBE_DEST_API,
				       FBE_TRACE_LEVEL_ERROR,
				       FBE_TRACE_MESSAGE_ID_INFO,
				       "%s: DeviceIoctl failed, status 0x%x\n",
					__FUNCTION__, status);

		return FBE_STATUS_GENERIC_FAILURE;
	}
	return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_dest_api_remove_object ()
 *********************************************************************
 *
 *  Description:
 *    Send FBE_DEST_PACKAGE_REMOVE_OBJECT_IOCTL to FBE DEST driver
 *
 *  Return Value:  success or failure
 *
 *  History:
 *      04/25/11 - Created.  Wayne Garrett
 *    
 *********************************************************************/
fbe_status_t 
fbe_dest_api_remove_object(fbe_object_id_t object_id)
{
	fbe_dest_remove_object_t 	remove_object;
	EMCPAL_STATUS 			status;
	PEMCPAL_FILE_OBJECT 		file_object = NULL;

	remove_object.object_id = object_id;

	/*get a pointer to the physical package*/
	status = EmcpalDeviceOpen(EmcpalDriverGetCurrentClientObject(),
				  FBE_NEIT_PACKAGE_DEVICE_NAME_CHAR,
				  &file_object);

	if (!EMCPAL_IS_SUCCESS(status)) {
		fbe_base_library_trace(FBE_LIBRARY_ID_FBE_DEST_API,
				       FBE_TRACE_LEVEL_ERROR,
				       FBE_TRACE_MESSAGE_ID_INFO,
				       "%s: DeviceOpen failed, status 0x%x\n",
				       __FUNCTION__, status);

	        return FBE_STATUS_GENERIC_FAILURE;
	}

	status = EmcpalDeviceIoctl(EmcpalDriverGetCurrentClientObject(),
				   file_object,
				   FBE_DEST_PACKAGE_REMOVE_OBJECT_IOCTL,
				   &remove_object,
				   sizeof (fbe_dest_remove_object_t),
				   &remove_object,
				   sizeof (fbe_dest_remove_object_t),
				   NULL);

	(void) EmcpalDeviceClose(file_object);

	if (!EMCPAL_IS_SUCCESS(status)) {
		fbe_base_library_trace(FBE_LIBRARY_ID_FBE_DEST_API,
				       FBE_TRACE_LEVEL_ERROR,
				       FBE_TRACE_MESSAGE_ID_INFO,
				       "%s: DeviceIoctl failed, status 0x%x\n",
					__FUNCTION__, status);

		return FBE_STATUS_GENERIC_FAILURE;
	}
	return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_dest_api_start ()
 *********************************************************************
 *
 *  Description:
 *    Send FBE_DEST_PACKAGE_START_IOCTL to FBE DEST driver
 *
 *  Return Value:  success or failure
 *
 *  History:
 *      04/25/11 - Created.  Wayne Garrett
 *    
 *********************************************************************/
fbe_status_t 
fbe_dest_api_start(void)
{
	EMCPAL_STATUS 		status;
	PEMCPAL_FILE_OBJECT	file_object = NULL;

	/*get a pointer to the physical package*/
	status = EmcpalDeviceOpen(EmcpalDriverGetCurrentClientObject(),
				  FBE_NEIT_PACKAGE_DEVICE_NAME_CHAR,
				  &file_object);

	if (!EMCPAL_IS_SUCCESS(status)) {
		fbe_base_library_trace(FBE_LIBRARY_ID_FBE_DEST_API,
				       FBE_TRACE_LEVEL_ERROR,
				       FBE_TRACE_MESSAGE_ID_INFO,
				       "%s: DeviceOpen failed, status 0x%x\n",
				       __FUNCTION__, status);

	        return FBE_STATUS_GENERIC_FAILURE;
	}

	status = EmcpalDeviceIoctl(EmcpalDriverGetCurrentClientObject(),
				   file_object, FBE_DEST_PACKAGE_START_IOCTL,
				   NULL, 0, NULL, 0, NULL);

	(void) EmcpalDeviceClose(file_object);

	if (!EMCPAL_IS_SUCCESS(status)) {
		fbe_base_library_trace(FBE_LIBRARY_ID_FBE_DEST_API,
				       FBE_TRACE_LEVEL_ERROR,
				       FBE_TRACE_MESSAGE_ID_INFO,
				       "%s: DeviceIoctl failed, status 0x%x\n",
					__FUNCTION__, status);

		return FBE_STATUS_GENERIC_FAILURE;
	}
	return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_dest_api_stop ()
 *********************************************************************
 *
 *  Description:
 *    Send FBE_DEST_PACKAGE_STOP_IOCTL to FBE DEST driver
 *
 *  Return Value:  success or failure
 *
 *  History:
 *      04/25/11 - Created.  Wayne Garrett
 *    
 *********************************************************************/
fbe_status_t 
fbe_dest_api_stop(void)
{
	EMCPAL_STATUS 		status;
	PEMCPAL_FILE_OBJECT 	file_object = NULL;

	/*get a pointer to the physical package*/
	status = EmcpalDeviceOpen(EmcpalDriverGetCurrentClientObject(),
				  FBE_NEIT_PACKAGE_DEVICE_NAME_CHAR,
				  &file_object);

	if (!EMCPAL_IS_SUCCESS(status)) {
		fbe_base_library_trace(FBE_LIBRARY_ID_FBE_DEST_API,
				       FBE_TRACE_LEVEL_ERROR,
				       FBE_TRACE_MESSAGE_ID_INFO,
				       "%s: DeviceOpen failed, status 0x%x\n",
				       __FUNCTION__, status);

	        return FBE_STATUS_GENERIC_FAILURE;
	}

	status = EmcpalDeviceIoctl(EmcpalDriverGetCurrentClientObject(),
				   file_object, FBE_DEST_PACKAGE_STOP_IOCTL,
				   NULL, 0, NULL, 0, NULL);

	(void) EmcpalDeviceClose(file_object);

	if (!EMCPAL_IS_SUCCESS(status)) {
		fbe_base_library_trace(FBE_LIBRARY_ID_FBE_DEST_API,
				       FBE_TRACE_LEVEL_ERROR,
				       FBE_TRACE_MESSAGE_ID_INFO,
				       "%s: DeviceIoctl failed, status 0x%x\n",
					__FUNCTION__, status);

		return FBE_STATUS_GENERIC_FAILURE;
	}
	return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_dest_api_get_record ()
 *********************************************************************
 *
 *  Description:
 *    Send FBE_DEST_PACKAGE_GET_RECORD_IOCTL to FBE DEST driver
 *
 *  Return Value:  success or failure
 *
 *  History:
 *      04/25/11 - Created.  Wayne Garrett
 *    
 *********************************************************************/
fbe_status_t 
fbe_dest_api_get_record(fbe_dest_record_handle_t dest_record_handle,
			fbe_dest_error_record_t * dest_error_record)
{
	fbe_dest_get_record_t 	get_record;
	EMCPAL_STATUS 		status;
	PEMCPAL_FILE_OBJECT 	file_object = NULL;

	get_record.dest_record_handle = dest_record_handle;
	
	/*get a pointer to the physical package*/
	status = EmcpalDeviceOpen(EmcpalDriverGetCurrentClientObject(),
				  FBE_NEIT_PACKAGE_DEVICE_NAME_CHAR,
				  &file_object);

	if (!EMCPAL_IS_SUCCESS(status)) {
		fbe_base_library_trace(FBE_LIBRARY_ID_FBE_DEST_API,
				       FBE_TRACE_LEVEL_ERROR,
				       FBE_TRACE_MESSAGE_ID_INFO,
				       "%s: DeviceOpen failed, status 0x%x\n",
				       __FUNCTION__, status);

	        return FBE_STATUS_GENERIC_FAILURE;
	}

	status = EmcpalDeviceIoctl(EmcpalDriverGetCurrentClientObject(),
				   file_object,
				   FBE_DEST_PACKAGE_GET_RECORD_IOCTL,
				   &get_record, sizeof (fbe_dest_get_record_t),
				   &get_record, sizeof (fbe_dest_get_record_t),
				   NULL);

	(void) EmcpalDeviceClose(file_object);

	if (!EMCPAL_IS_SUCCESS(status)) {
		fbe_base_library_trace(FBE_LIBRARY_ID_FBE_DEST_API,
				       FBE_TRACE_LEVEL_ERROR,
				       FBE_TRACE_MESSAGE_ID_INFO,
				       "%s: DeviceIoctl failed, status 0x%x\n",
					__FUNCTION__, status);

		return FBE_STATUS_GENERIC_FAILURE;
	}
	fbe_copy_memory(dest_error_record, &get_record.dest_error_record,
			sizeof (fbe_dest_error_record_t));

	return FBE_STATUS_OK;
}
