/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_api_sim_server_interface.c
 ***************************************************************************
 *
 *  Description
 *      Define the interfaces for server controls
 ****************************************************************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_transport.h"
#include "fbe/fbe_api_sim_transport.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_board_interface.h"

/**************************************
	    Local variables
**************************************/

/*******************************************
	    Local functions
********************************************/

/*********************************************************************
 *            fbe_api_sim_server_load_package ()
 *********************************************************************
 *
 *  Description: load specified package
 *
 *	Inputs: package_id - package to load
 *
 *  Return Value: success or failure
 *
 *  History:
 *    10/28/09: guov	created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_sim_server_load_package(fbe_package_id_t package_id)
{
    fbe_status_t	status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t	status_info;
    status = fbe_api_common_send_control_packet_to_service (FBE_SIM_SERVER_CONTROL_CODE_LOAD_PACKAGE,
							    NULL,
							    0,
							    FBE_SERVICE_ID_SIM_SERVER,
							    FBE_PACKET_FLAG_NO_ATTRIB,
							    &status_info,
							    package_id);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
	fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
		       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
	return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_api_sim_server_load_package_params()
 ****************************************************************
 * @brief
 *  Load a package using a set of parameters.
 *
 * @param package_id - the package to load.
 * @param buffer_p - The control buffer to pass to the load. 
 *                   This contains the parameters we wish to pass
 *                   to the package load routine.
 * @param buffer_size - the size of this control buffer.
 *
 * @return fbe_status_t   
 *
 * @author
 *  9/2/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t FBE_API_CALL fbe_api_sim_server_load_package_params(fbe_package_id_t package_id,
                                                                 void *buffer_p,
                                                                 fbe_u32_t buffer_size)
{
    fbe_status_t	status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t	status_info;
    status = fbe_api_common_send_control_packet_to_service (FBE_SIM_SERVER_CONTROL_CODE_LOAD_PACKAGE,
							    buffer_p,
							    buffer_size,
							    FBE_SERVICE_ID_SIM_SERVER,
							    FBE_PACKET_FLAG_NO_ATTRIB,
							    &status_info,
							    package_id);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
	fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
		       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
	return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_api_sim_server_load_package_params()
 ******************************************/

/*********************************************************************
 *            fbe_api_sim_server_init_fbe_api ()
 *********************************************************************
 *
 *  Description: init fbe api on the server side
 *
 *	Inputs: none
 *
 *  Return Value: success or failue
 *
 *  History:
 *    10/28/09: guov	created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_sim_server_init_fbe_api(void)
{
    fbe_status_t	status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t	status_info;
    status = fbe_api_common_send_control_packet_to_service (FBE_SIM_SERVER_CONTROL_CODE_INIT_FBE_API,
							    NULL,
							    0,
							    FBE_SERVICE_ID_SIM_SERVER,
							    FBE_PACKET_FLAG_NO_ATTRIB,
							    &status_info,
							    FBE_PACKAGE_ID_PHYSICAL); /* package_id is not checked by the other end, just to pass thru the transport */

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
	fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
		       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
	return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_sim_server_destroy_fbe_api ()
 *********************************************************************
 *
 *  Description: destroy fbe api on the server side
 *
 *	Inputs: none
 *
 *  Return Value: success or failue
 *
 *  History:
 *    12/04/09: guov	created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_sim_server_destroy_fbe_api(void)
{
    fbe_status_t	status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t	status_info;
    status = fbe_api_common_send_control_packet_to_service (FBE_SIM_SERVER_CONTROL_CODE_DESTROY_FBE_API,
							    NULL,
							    0,
							    FBE_SERVICE_ID_SIM_SERVER,
							    FBE_PACKET_FLAG_NO_ATTRIB,
							    &status_info,
							    FBE_PACKAGE_ID_PHYSICAL); /* package_id is not checked by the other end, just to pass thru the transport */

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
	fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
		       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
		return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

fbe_status_t FBE_API_CALL fbe_api_sim_server_wait_for_database_service(void)
{
	fbe_u32_t                 timeout = 600; /* 1 min */
    fbe_database_state_t state = FBE_DATABASE_STATE_READY;
    fbe_status_t              status;
    
	do
    {
		status = fbe_api_database_get_state(&state);

        if ( status != FBE_STATUS_OK )
        {
            return status;
        }

        if (state == FBE_DATABASE_STATE_READY  || 
			state == FBE_DATABASE_STATE_SERVICE_MODE  || 
			state == FBE_DATABASE_STATE_CORRUPT ||
			state == FBE_DATABASE_STATE_BOOTFLASH_SERVICE_MODE)
        {
            return FBE_STATUS_OK;
        }

        if (timeout == 0 )
        {
            break;
        }
        
		fbe_api_sleep(100);
		timeout --;
    }
    while(1);
    
    fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s database service is not READY\n", __FUNCTION__);
    return FBE_STATUS_GENERIC_FAILURE;
}

/*********************************************************************
 *            fbe_api_sim_server_set_package_entries ()
 *********************************************************************
 *
 *  Description: set the entry points for the packages
 *
 *	Inputs: none
 *
 *  Return Value: success or failue
 *
 *  History:
 *    10/28/09: guov	created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_sim_server_set_package_entries(fbe_package_id_t package_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_api_sim_server_set_package_entries_no_wait(package_id);

    if ( status != FBE_STATUS_OK )
    {
        return status;
    }
    
    if(package_id == FBE_PACKAGE_ID_SEP_0){
		/* Wait for database service to become ready */
		status = fbe_api_sim_server_wait_for_database_service();
	}
    return status;
}

fbe_status_t FBE_API_CALL fbe_api_sim_server_set_package_entries_no_wait(fbe_package_id_t package_id)
{
    fbe_status_t	status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t	status_info;

    status = fbe_api_common_send_control_packet_to_service (FBE_SIM_SERVER_CONTROL_CODE_SET_PACKAGE_ENTRIES,
							    NULL,
							    0,
							    FBE_SERVICE_ID_SIM_SERVER,
							    FBE_PACKET_FLAG_NO_ATTRIB,
							    &status_info,
							    package_id); /* package_id is not checked by the other end, just to pass thru the transport */

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
	fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
		       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
		return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*********************************************************************
 *            fbe_api_sim_server_unload_package ()
 *********************************************************************
 *
 *  Description: unload package
 *
 *	Inputs: package_id - package to unload
 *
 *  Return Value: success or failue
 *
 *  History:
 *    10/28/09: guov	created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_sim_server_unload_package(fbe_package_id_t package_id)
{
    fbe_status_t	status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t	status_info;
    status = fbe_api_common_send_control_packet_to_service (FBE_SIM_SERVER_CONTROL_CODE_UNLOAD_PACKAGE,
							    NULL,
							    0,
							    FBE_SERVICE_ID_SIM_SERVER,
							    FBE_PACKET_FLAG_NO_ATTRIB,
							    &status_info,
							    package_id);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
	fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
		       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
	return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/*!**************************************************************
 * fbe_api_sim_server_disable_package()
 ****************************************************************
 * @brief
 *  Disable this package prior to unloading it.
 *
 * @param  package_id - id of package to destroy.               
 *
 * @return fbe_status_t    
 *
 * @author
 *  5/28/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t FBE_API_CALL fbe_api_sim_server_disable_package(fbe_package_id_t package_id)
{
    fbe_status_t	status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t	status_info;
    status = fbe_api_common_send_control_packet_to_service (FBE_SIM_SERVER_CONTROL_CODE_DISABLE_PACKAGE,
							    NULL,
							    0,
							    FBE_SERVICE_ID_SIM_SERVER,
							    FBE_PACKET_FLAG_NO_ATTRIB,
							    &status_info,
							    package_id);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
	fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
		       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
	return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_api_sim_server_disable_package()
 ******************************************/
/*********************************************************************
 *            fbe_api_sim_server_get_pid ()
 *********************************************************************
 *
 *  Description: get the process ID of the server
 *
 *	Inputs: None
 *  Output: Process ID
 *
 *  Return Value: success or failue
 *
 *  History:
 *  7/29/2010 Created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_sim_server_get_pid(fbe_api_sim_server_pid *pid)
{
    fbe_status_t	status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t	status_info;
    fbe_api_sim_server_get_pid_t get_pid;
    status = fbe_api_common_send_control_packet_to_service (FBE_SIM_SERVER_CONTROL_CODE_GET_PID,
							    &get_pid,
							    sizeof (fbe_api_sim_server_get_pid_t),
							    FBE_SERVICE_ID_SIM_SERVER,
							    FBE_PACKET_FLAG_NO_ATTRIB,
							    &status_info,
							    FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
    	fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
    	return FBE_STATUS_GENERIC_FAILURE;
    }
    *pid = get_pid.pid;
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_sim_server_suite_start_notification ()
 *********************************************************************
 *
 *  Description: send suite start notification to sim server
 *
 *  Inputs: log_path, log_file_name
 *  Output: None
 *
 *  Return Value: success or failue
 *
 *  History:
 *  1/7/2011 Bo Gao Created
 *
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_sim_server_suite_start_notification(const char *log_path, const char *log_file_name)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_api_sim_server_suite_start_notification_t suite_start_notification;

    strncpy(suite_start_notification.log_path, log_path, sizeof(suite_start_notification.log_path) - 1);
    strncpy(suite_start_notification.log_file_name, log_file_name, sizeof(suite_start_notification.log_file_name) - 1);

    status = fbe_api_common_send_control_packet_to_service (FBE_SIM_SERVER_CONTROL_CODE_SUITE_START_NOTIFICATION,
                                &suite_start_notification,
                                sizeof (fbe_api_sim_server_suite_start_notification_t),
                                FBE_SERVICE_ID_SIM_SERVER,
                                FBE_PACKET_FLAG_NO_ATTRIB,
                                &status_info,
                                FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/*********************************************************************
 *            fbe_api_sim_server_get_windows_cpu_utilization ()
 *********************************************************************
 *
 *  Description: get a snapshot of the CPU utilization on the SP
 *
 *	Inputs: None
 *  Output: per-core CPU utilization expressed as an array of integers (scale 0-100)
 *
 *  Return Value: success or failure
 *
 *  History:
 *  2/16/2012 Created - Ryan Gadsby
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_sim_server_get_windows_cpu_utilization(fbe_api_sim_server_get_cpu_util_t *cpu_util)
{
    fbe_status_t	status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t	status_info;

    fbe_object_id_t object_id;
    fbe_board_mgmt_platform_info_t platform_info;
    fbe_api_sim_server_get_windows_cpu_utilization_t get_cpu;

    char *token;

    status = fbe_api_common_send_control_packet_to_service (FBE_SIM_SERVER_CONTROL_CODE_GET_WINDOWS_CPU_UTILIZATION,
							    &get_cpu,
							    sizeof (fbe_api_sim_server_get_windows_cpu_utilization_t),
							    FBE_SERVICE_ID_SIM_SERVER,
							    FBE_PACKET_FLAG_NO_ATTRIB,
							    &status_info,
							    FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
    	fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
    	return FBE_STATUS_GENERIC_FAILURE;
    }

    //get core count
    status = fbe_api_get_board_object_id(&object_id);
     if (status != FBE_STATUS_OK) {
    	fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:couldn't get board object ID\n", __FUNCTION__);
    	return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_api_board_get_platform_info(object_id, &platform_info);
    if (status != FBE_STATUS_OK) {
    	fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:couldn't get board platform info\n", __FUNCTION__);
    	return FBE_STATUS_GENERIC_FAILURE;
    }

    cpu_util->core_count = 0;

    //parse typeperf return string
    token = strtok(get_cpu.typeperf_str, ",\"");  //ignore timestamp
    token = strtok(NULL,",\"");
    while (token && cpu_util->core_count < platform_info.hw_platform_info.processorCount){
        cpu_util->cpu_usage[cpu_util->core_count] = (fbe_u32_t) (atof(token)+0.5); //round up
        token = strtok(NULL,",\"");
        cpu_util->core_count++;
    }
    if (cpu_util->core_count != platform_info.hw_platform_info.processorCount) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:error parsing typeperf string, core_count:%d processorCount:%d\n", __FUNCTION__,
                       cpu_util->core_count, platform_info.hw_platform_info.processorCount);
        return FBE_STATUS_GENERIC_FAILURE; //something messed up when processing the typeperf string
    }
    
    return FBE_STATUS_OK;
}

fbe_status_t FBE_API_CALL fbe_api_sim_server_get_ica_status(fbe_bool_t *is_ica_complete)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t	status_info;
    fbe_api_sim_server_get_ica_util_t get_ica;
    fbe_zero_memory(&get_ica, sizeof(fbe_api_sim_server_get_ica_util_t));

    status = fbe_api_common_send_control_packet_to_service (FBE_SIM_SERVER_CONTROL_CODE_GET_ICA_STATUS,
							    &get_ica,
							    sizeof (fbe_api_sim_server_get_ica_util_t),
							    FBE_SERVICE_ID_SIM_SERVER,
							    FBE_PACKET_FLAG_NO_ATTRIB,
							    &status_info,
							    FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
    	fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
    	return FBE_STATUS_GENERIC_FAILURE;
    }

    *is_ica_complete = get_ica.is_ica_done;
    return FBE_STATUS_OK;
}
