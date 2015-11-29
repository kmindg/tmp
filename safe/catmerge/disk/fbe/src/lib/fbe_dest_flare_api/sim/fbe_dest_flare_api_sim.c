/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_dest_api_sim.c
 ***************************************************************************
 *
 *  Description
 *      Simulation implementation for DEST API interface
 *      
 *
 *  History:
 *      04/25/11    Created.  Wayne Garrett
 *    
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_dest.h"

/*********************************************************************
 *            fbe_dest_api_init ()
 *********************************************************************
 *
 *  Description: simulation version of init
 *
 *	Inputs: 
 *
 *  Return Value: 
 *
 *  History:
 *    02/04/09    Peter Puhov - Created
 *    
 *********************************************************************/
fbe_status_t 
fbe_dest_api_init (void)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
   status = fbe_dest_init();
	return status; 
}

/*********************************************************************
 *            fbe_dest_api_destroy ()
 *********************************************************************
 *
 *  Description: simulation version of destroy
 *
 *	Inputs: 
 *
 *  Return Value: 
 *
 *  History:
 *    04/25/11    Created.  Wayne Garrett
 *          
 *********************************************************************/
fbe_status_t 
fbe_dest_api_destroy (void)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
   status = fbe_dest_destroy();
	return status; 
}

/*********************************************************************
 *            fbe_dest_api_add_record ()
 *********************************************************************
 *
 *  Description:
 *    Calls dest_add_record function
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    04/25/11    Created.  Wayne Garrett
 *    
 *********************************************************************/
fbe_status_t 
fbe_dest_api_add_record(fbe_dest_error_record_t * dest_error_record, fbe_dest_record_handle_t * dest_record_handle)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

  status = fbe_dest_add_record(dest_error_record, dest_record_handle);
	return status;
}

/*********************************************************************
 *            fbe_dest_api_remove_record ()
 *********************************************************************
 *
 *  Description:
 *    Calls dest_remove_record function
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    04/25/11    Created.  Wayne Garrett
 *    
 *********************************************************************/
fbe_status_t 
fbe_dest_api_remove_record(fbe_dest_record_handle_t dest_record_handle)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

	status = fbe_dest_remove_record(dest_record_handle);
	return status;
}

/*********************************************************************
 *            fbe_dest_api_remove_object ()
 *********************************************************************
 *
 *  Description:
 *    Calls dest_remove_object function
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    04/25/11    Created.  Wayne Garrett
 *    
 *********************************************************************/
fbe_status_t 
fbe_dest_api_remove_object(fbe_object_id_t object_id)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_dest_remove_object(object_id);
	return status;
}

/*********************************************************************
 *            fbe_dest_api_start ()
 *********************************************************************
 *
 *  Description:
 *    Calls dest_start function
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    04/25/11    Created.  Wayne Garrett
 *    
 *********************************************************************/
fbe_status_t 
fbe_dest_api_start(void)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_dest_start();
	return status;
}

/*********************************************************************
 *            fbe_dest_api_stop ()
 *********************************************************************
 *
 *  Description:
 *    Calls dest_stop function
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    04/25/11    Created.  Wayne Garrett
 *    
 *********************************************************************/
fbe_status_t 
fbe_dest_api_stop(void)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_dest_stop();
	return status;
}

/*********************************************************************
 *            fbe_dest_api_get_record ()
 *********************************************************************
 *
 *  Description:
 *    Calls dest_get_record function
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    04/25/11    Created.  Wayne Garrett
 *    
 *********************************************************************/
fbe_status_t 
fbe_dest_api_get_record(fbe_dest_record_handle_t dest_record_handle, fbe_dest_error_record_t * dest_error_record)
{
	fbe_status_t status;

	status = fbe_dest_get_record(dest_record_handle, dest_error_record);
	return status;
}
