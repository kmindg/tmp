/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_cmi.c
 ***************************************************************************
 *
 * @brief This file is used to parse cmi related commands
 *
 * @ingroup fbe_cli
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
#include "fbe_cli_private.h"
#include "fbe_trace.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe_cli_cmi_service.h"
#include "fbe_cmi.h"


/*local functions*/
static void fbe_cli_print_cmi_info(void);
static void fbe_cli_print_cmi_io_statistics(void);
static void fbe_cli_clear_cmi_io_statistics(void);

/*!*******************************************************************
 * @var fbe_cli_cmi_commands
 *********************************************************************
 * @brief Function to implement cmi informationin FB CLI
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *********************************************************************/
void fbe_cli_cmi_commands(int argc, char** argv)
{
	if (argc < 1){
        /*If there are no arguments show usage*/
		fbe_cli_printf("%s", FBE_CLI_CMI_USAGE);
		return;
    }

    
	if ((strcmp(*argv, "-help") == 0) || (strcmp(*argv, "-h") == 0)){
		/*Print cmi usage*/
		fbe_cli_printf("%s", FBE_CLI_CMI_USAGE);
		return;
	}

	if ((strcmp(*argv, "-info") == 0)){
		
		fbe_cli_print_cmi_info();
		return;
	}

	if ((strcmp(*argv, "-get_io_stat") == 0)){
		
		fbe_cli_print_cmi_io_statistics();
		return;
	}

	if ((strcmp(*argv, "-clear_io_stat") == 0)){
		
		fbe_cli_clear_cmi_io_statistics();
		return;
	}

}

static void fbe_cli_print_cmi_info(void)
{
	fbe_cmi_service_get_info_t	cmi_info;
	fbe_status_t				status;

	status = fbe_api_cmi_service_get_info(&cmi_info);
	if (status != FBE_STATUS_OK) {
		fbe_cli_printf("Failed to get CMI info, error code:%d\n", status);
		return;
	}


	fbe_cli_printf("\nThis SP ID:%s\n", cmi_info.sp_id == FBE_CMI_SP_ID_A ? "SPA" : "SPB");
	fbe_cli_printf("Peer Alive:%s\n", FBE_IS_TRUE(cmi_info.peer_alive) ? "Yes" : "No");
	fbe_cli_printf("This SP State:");
	switch ( cmi_info.sp_state) {
	case FBE_CMI_STATE_PASSIVE:
		fbe_cli_printf("Passive\n");
		break;
	case FBE_CMI_STATE_ACTIVE:
		fbe_cli_printf("Active\n");
		break;
	case FBE_CMI_STATE_BUSY:
		fbe_cli_printf("Busy (This is not good !)\n");
		break;
	default:
		fbe_cli_error("Can't map CMI state\n");

	}
	fbe_cli_printf("\n");

	return;
}

/*!**************************************************************
 * fbe_cli_print_cmi_io_statistics()
 ****************************************************************
 * @brief
 *  This function print io statistics for each conduit. 
 *
 * @param - none
 *
 * @return - none.  
 *********************************************************************/
static void fbe_cli_print_cmi_io_statistics(void)
{
	fbe_cmi_service_get_io_statistics_t	io_stat;
	fbe_status_t status;
	fbe_u32_t i;

	for (i = FBE_CMI_CONDUIT_ID_SEP_IO_FIRST; i < (FBE_CMI_CONDUIT_ID_SEP_IO_FIRST + fbe_get_cpu_count()); i++)
	{
		io_stat.conduit_id = i;
		status = fbe_api_cmi_service_get_io_statistics(&io_stat);
		if (status != FBE_STATUS_OK) {
			fbe_cli_printf("Failed to get CMI IO statistics, error code:%d\n", status);
			return;
		}
	    fbe_cli_printf("FBE CMI conduit id: %d\n", i);
	    fbe_cli_printf("    Sent IOs: 0x%llx\n", (unsigned long long)io_stat.sent_ios);
	    fbe_cli_printf("    Sent bytes: 0x%llx\n", (unsigned long long)io_stat.sent_bytes);
	    fbe_cli_printf("    Received IOs: 0x%llx\n", (unsigned long long)io_stat.received_ios);
	    fbe_cli_printf("    Received bytes: 0x%llx\n", (unsigned long long)io_stat.received_bytes);
	}

	fbe_cli_printf("\n");
	return;
}

/*!**************************************************************
 * fbe_cli_clear_cmi_io_statistics()
 ****************************************************************
 * @brief
 *  This function clears io statistics for all conduits. 
 *
 * @param - none
 *
 * @return - none.  
 *********************************************************************/
static void fbe_cli_clear_cmi_io_statistics(void)
{
	fbe_status_t status;

	status = fbe_api_cmi_service_clear_io_statistics();
	if (status != FBE_STATUS_OK) {
		fbe_cli_printf("Failed to clear CMI IO statistics, error code:%d\n", status);
	}
	else {
		fbe_cli_printf("Successfully cleared CMI IO statistics\n");
	}

	fbe_cli_printf("\n");
	return;
}
