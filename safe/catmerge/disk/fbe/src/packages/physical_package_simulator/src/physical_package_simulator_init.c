
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe_transport_memory.h"
#include "fbe_notification.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_physical_package_simulator.h"
#include "physical_package_simulator_service_table.h"
#include "fbe_simulator.h"

/***************************************************************************
 *  physical_package_simulator_init.c
 ***************************************************************************
 *
 *  Description
 *      main entry point to the simulator
 *      
 *
 *  History:
 *      08/09/07	sharel	Created
 *    
 ***************************************************************************/


/********************************/
/*               Locals         */
/*******************************/


/************************/
/* Forward declaration */
/***********************/

/*********************************************************************
 *            physical_package_siumlator_init ()
 *********************************************************************
 *
 *  Description: initialize all we need in the simulation package
 *
 *
 *  Return Value: success or failure
 *
 *  History:
 *    8/09/07: sharel	created
 *    
 *********************************************************************/
fbe_status_t physical_package_simulator_init(void)
{
	fbe_status_t status;
	KvTrace("physical_package_siumlator_init: %s entry\n", __FUNCTION__);

	/* Initialize service manager */
	status = fbe_service_manager_init_service_table(physical_package_simulator_service_table, FBE_PACKAGE_ID_PHYSICAL);
	if(status != FBE_STATUS_OK) {
		KvTrace("%s fbe_service_manager_init_service_table fail status %X\n", __FUNCTION__, status);
		return status;
	}

	status = fbe_base_service_send_init_command(FBE_SERVICE_ID_SERVICE_MANAGER);
	if(status != FBE_STATUS_OK) {
		KvTrace("%s physical_package_init_service_manager fail status %X\n", __FUNCTION__, status);
		return status;
	}
	
	/* Initialize notification service */
	status = fbe_base_service_send_init_command(FBE_SERVICE_ID_NOTIFICATION);
	if(status != FBE_STATUS_OK) {
		KvTrace("physical_package_siumlator_init: %s fbe_notification_init fail status %X\n", __FUNCTION__, status);
		return status;
	}

	status  = fbe_simulator_init ();
	if(status != FBE_STATUS_OK) {
		KvTrace("physical_package_siumlator_init: %s fbe_simulator_init fail status %X\n", __FUNCTION__, status);
		return status;
	}

	KvTrace("physical_package_siumlator_init: %s success\n", __FUNCTION__);
	return FBE_STATUS_OK;
}

/*********************************************************************
 *            physical_package_simulator_destroy ()
 *********************************************************************
 *
 *  Description: destroy all used resources
 *
 *
 *  Return Value: success or failure
 *
 *  History:
 *    8/09/07: sharel	created
 *    
 *********************************************************************/
fbe_status_t physical_package_simulator_destroy(void)
{
	fbe_status_t status;
    
	KvTrace("%s: entry\n", __FUNCTION__);
    
	status =  fbe_simulator_destroy();
	if(status != FBE_STATUS_OK) {
		KvTrace("%s: fbe_simulator_destroy fail status %X\n", __FUNCTION__, status);
		return status;
	}

	status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_NOTIFICATION);
	if(status != FBE_STATUS_OK) {
		KvTrace("%s: fbe_notification_destroy fail status %X\n", __FUNCTION__, status);
		return status;
	}
    
	status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_SERVICE_MANAGER);
	if(status != FBE_STATUS_OK) {
		KvTrace("%s physical_package_init_service_manager fail status %X\n", __FUNCTION__, status);
		return status;
	}

	KvTrace("%s: success\n", __FUNCTION__);
	return FBE_STATUS_OK;
}

/*********************************************************************
 *            physical_package_simulator_get_control_entry ()
 *********************************************************************
 *
 *  Description: get the pointer to the simulator service entry point
 *
 *
 *  Return Value: function to the pointer
 *
 *  History:
 *    8/09/07: sharel	created
 *    
 *********************************************************************/
fbe_service_control_entry_t physical_package_simulator_get_control_entry(void)
{
	
	return &fbe_service_manager_send_control_packet;

}
