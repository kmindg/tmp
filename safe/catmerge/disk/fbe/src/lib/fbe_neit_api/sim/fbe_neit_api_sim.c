/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_neit_api_sim.c
 ***************************************************************************
 *
 *  Description
 *      Simulation implementation for NEIT API interface
 *      
 *
 *  History:
 *      02/04/09    Peter Puhov - Created
 *    
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_neit_flare_package.h"

/*********************************************************************
 *            fbe_neit_api_init ()
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
fbe_neit_api_init (void)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
   status = neit_flare_package_init();
	return status; 
}

/*********************************************************************
 *            fbe_neit_api_destroy ()
 *********************************************************************
 *
 *  Description: simulation version of destroy
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
fbe_neit_api_destroy (void)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
   status = neit_flare_package_destroy();
	return status; 
}

/*********************************************************************
 *            fbe_neit_api_add_record ()
 *********************************************************************
 *
 *  Description:
 *    Calls neit_package_add_record function
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    02/04/09    Peter Puhov - Created
 *    
 *********************************************************************/
fbe_status_t 
fbe_neit_api_add_record(fbe_neit_error_record_t * neit_error_record, fbe_neit_record_handle_t * neit_record_handle)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

  status = neit_package_add_record(neit_error_record, neit_record_handle);
	return status;
}

/*********************************************************************
 *            fbe_neit_api_remove_record ()
 *********************************************************************
 *
 *  Description:
 *    Calls neit_package_remove_record function
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    02/04/09    Peter Puhov - Created
 *    
 *********************************************************************/
fbe_status_t 
fbe_neit_api_remove_record(fbe_neit_record_handle_t neit_record_handle)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

	status = neit_package_remove_record(neit_record_handle);
	return status;
}

/*********************************************************************
 *            fbe_neit_api_remove_object ()
 *********************************************************************
 *
 *  Description:
 *    Calls neit_package_remove_object function
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    02/06/09    Peter Puhov - Created
 *    
 *********************************************************************/
fbe_status_t 
fbe_neit_api_remove_object(fbe_object_id_t object_id)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

  status = neit_package_remove_object(object_id);
	return status;
}

/*********************************************************************
 *            fbe_neit_api_start ()
 *********************************************************************
 *
 *  Description:
 *    Calls neit_package_start function
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    02/04/09    Peter Puhov - Created
 *    
 *********************************************************************/
fbe_status_t 
fbe_neit_api_start(void)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

  status = neit_package_start();
	return status;
}

/*********************************************************************
 *            fbe_neit_api_stop ()
 *********************************************************************
 *
 *  Description:
 *    Calls neit_package_stop function
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    02/04/09    Peter Puhov - Created
 *    
 *********************************************************************/
fbe_status_t 
fbe_neit_api_stop(void)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

  status = neit_package_stop();
	return status;
}

/*********************************************************************
 *            fbe_neit_api_get_record ()
 *********************************************************************
 *
 *  Description:
 *    Calls neit_package_get_record function
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    10/31/09    Christina Chiang Puhov - Created
 *    
 *********************************************************************/
fbe_status_t 
fbe_neit_api_get_record(fbe_neit_record_handle_t neit_record_handle, fbe_neit_error_record_t * neit_error_record)
{
	fbe_status_t status;

	status = neit_package_get_record(neit_record_handle, neit_error_record);
	return status;
}

