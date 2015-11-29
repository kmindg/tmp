/***************************************************************************
* Copyright (C) EMC Corporation 2011 
* All rights reserved. 
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!*************************************************************************
*                 @file fbe_err_sniff_main.c
****************************************************************************
*
* @brief
*  This file contains the main function for fbe_err_sniff.
*
* @ingroup fbe_err_sniff
*
* @date
*  08/08/2011 - Created. Vera Wang
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_api_sim_transport.h"
#include "fbe/fbe_api_sim_transport_packet_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_trace.h"
#include "fbe/fbe_api_discovery_interface.h"
#include <signal.h>
#include "fbe_err_sniff_notification_handle.h"
#include "fbe_err_sniff_file_access.h"
#include "fbe/fbe_winddk.h"
#include "fbe_err_sniff_call_sp_collector.h"
#include "fbe/fbe_api_trace_interface.h"
#include "fbe/fbe_emcutil_shell_include.h"


/*!************************
*   LOCAL VARIABLES
**************************/
static FILE * fp;
static fbe_bool_t   simulation_mode = FBE_FALSE;
static fbe_sim_transport_connection_target_t sp_to_connect = FBE_SIM_SP_A;

/*!************************
*   LOCAL FUNCTIONS
**************************/
static fbe_status_t fbe_err_sniff_initialize_fbe_api (fbe_char_t driverType , fbe_char_t spId);
static fbe_status_t fbe_err_sniff_set_notify_level(fbe_char_t level);
static fbe_u32_t fbe_err_sniff_get_target_sp(fbe_char_t spstr);
void __cdecl fbe_err_sniff_destroy_fbe_api(int in);
static void CSX_GX_RT_DEFCC fbe_err_sniff_csx_cleanup_handler(csx_rt_proc_cleanup_context_t context);
static void dummyWait(void);
static fbe_char_t getSpId(fbe_char_t spId);
static fbe_char_t getDriver(fbe_char_t driverType);
static fbe_char_t getFMode(fbe_char_t fMode);
static fbe_char_t getLevel(fbe_char_t level);
static fbe_char_t getPanicFlag(fbe_char_t panicFlag);
static void printUsage(void);

int __cdecl main (int argc , char *argv[])
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_char_t spId = 0;
    fbe_char_t driverType = 0;
    fbe_char_t cmdLineChar;
    fbe_char_t fMode = 0;
    fbe_char_t level = 0;
    fbe_char_t panicFlag = 0;
    fbe_u32_t  currentArg;
    fbe_char_t input;

#include "fbe/fbe_emcutil_shell_maincode.h"

    for (currentArg = 1; currentArg < argc; currentArg++)
    {
        cmdLineChar = argv[currentArg][0];

        if ((cmdLineChar == 'k') || (cmdLineChar == 's'))
        {
            if (driverType)
            {
                /* Option is supplied twice ! */
                printUsage();
                dummyWait();
                exit(1);
            }
            else
                driverType = cmdLineChar;
        }
        else if ((cmdLineChar == 'a') || (cmdLineChar == 'b'))
        {
            if (spId)
            {
                // Option is supplied twice.
                printUsage();
                dummyWait();
                exit(2);
            }
            else
                spId = cmdLineChar ;
        }
        else if ((cmdLineChar == 'c') || (cmdLineChar == 'm'))
        { 
            if (fMode)
            {
                /* Option is supplied twice ! */
                printUsage();
                dummyWait();
                exit(3);
            }
            else
                fMode = cmdLineChar;
        }
        else if ((cmdLineChar == 'w') || (cmdLineChar == 'e'))
        {
            if (level)
            {
                /* Option is supplied twice ! */
                printUsage();
                dummyWait();
                exit(4);
            }
            else
                level = cmdLineChar;
        }
        else if ((cmdLineChar == 'p') || (cmdLineChar == 'n'))
        {
            if (panicFlag)
            {
                /* Option is supplied twice ! */
                printUsage();
                dummyWait();
                exit(5);
            }
            else
                panicFlag = cmdLineChar;
        }
        else
        {
            printUsage();
            dummyWait();
            exit(6);
        }
    }
    if ((driverType = getDriver(driverType)) == 0)
    {
        printf("Error in getting driver block\n");
        printUsage();
        dummyWait();
        exit(0);
    }
    if ((spId = getSpId(spId)) == 0)
    {
        printf("Error in getting SP ID\n");
        printUsage();
        dummyWait();
        exit(0);
    }

    if ((fMode = getFMode(fMode)) == 0)
    {
        printf("Error in getting file mode\n");
        printUsage();
        dummyWait();
        exit(0);
    }
    if ((level = getLevel(level)) == 0) 
    {
        printf("Error in getting trace level\n");
        printUsage();
        dummyWait();
        exit(0);
    }
    if ((panicFlag = getPanicFlag(panicFlag)) == 0) 
    {
        printf("Error in getting panic flag\n");
        printUsage();
        dummyWait();
        exit(0);
    }

    fbe_trace_init();
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_CRITICAL_ERROR);
#if 0 /* SAFEMESS - should let CSX handle this */
    signal(SIGINT, fbe_err_sniff_destroy_fbe_api);/*route ctrl C to make sure we nicely close the connections*/
#else
    csx_rt_proc_cleanup_handler_register(fbe_err_sniff_csx_cleanup_handler, fbe_err_sniff_csx_cleanup_handler, NULL);
#endif

    /*initialize the fbe api connections*/
    printf("\n\nfbe_err_sniff Version 2.1 - Initializing...\n");
    printf("\nUser choices: driver type %c, sp %c, file mode %c, trace level %c, panic flag %c \n", driverType, spId, fMode, level, panicFlag);
    status = fbe_err_sniff_initialize_fbe_api(driverType, spId);
    if (status != FBE_STATUS_OK) {
        printf("Failed to init FBE API");
        return -1;
    } 

    fbe_err_sniff_set_spcollect_run(FBE_FALSE); 
    fp = fbe_err_sniff_open_file("trace_info.txt", fMode);
    if (fp == NULL) 
    {
        printf("\n Can not open the file. Fail to write the log. \n");
        return -1;
    }    
    fprintf(fp,"fbe_err_sniff Version 2.1\n\n");
    fprintf(fp,"User choices: driver type %c, sp %c, file mode %c, trace level %c, panic flag %c\n\n", driverType, spId, fMode, level, panicFlag);

    status = fbe_err_sniff_set_notify_level(level);
    if (status != FBE_STATUS_OK) {
        printf("\nCan't set notify level!\n");
        return status;
    }

    status = fbe_err_sniff_notification_handle_init(fp,panicFlag);
    if (status != FBE_STATUS_OK) {
        printf("\nNotification_handle_init failed!\n");
        return status;
    }
 
    printf("\nPlease enter x or X if you want to exit this application. \n");
    input = getchar();

    while (input != 'x' && input != 'X') 
    {
        printf("\nWrong input. Please enter x or X if you want to exit this application. \n");
        input = getchar(); 
    }

    status = fbe_err_sniff_notification_handle_destroy();
    fbe_err_sniff_close_file(fp);

    csx_rt_proc_cleanup_handler_deregister(fbe_err_sniff_csx_cleanup_handler);

    fbe_err_sniff_destroy_fbe_api(0);

    return 0;
}

static fbe_status_t fbe_err_sniff_initialize_fbe_api (fbe_char_t driverType , fbe_char_t spId)
{
    fbe_status_t    							status = FBE_STATUS_GENERIC_FAILURE;
    
    /*we assume the function before checked we have the connection mode and target SP*/
    /*'s' means simulation*/
    if ((driverType == 's') || (driverType == 'S')) {

        simulation_mode = FBE_TRUE;

		 /*initialize the simulation side of fbe api*/
        fbe_api_common_init_sim();

        /*set function entries for commands that would go to the physical package*/
        fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_PHYSICAL,
                                                       fbe_api_sim_transport_send_io_packet,
                                                       fbe_api_sim_transport_send_client_control_packet);

        /*set function entries for commands that would go to the sep package*/
        fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_SEP_0,
                                                       fbe_api_sim_transport_send_io_packet,
                                                       fbe_api_sim_transport_send_client_control_packet);

		/*set function entries for commands that would go to the esp package*/
        fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_ESP,
                                                       fbe_api_sim_transport_send_io_packet,
                                                       fbe_api_sim_transport_send_client_control_packet);

		/*set function entries for commands that would go to the neit package*/
        fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_NEIT,
                                                       fbe_api_sim_transport_send_io_packet,
                                                       fbe_api_sim_transport_send_client_control_packet);

       /*need to initialize the client to connect to the server*/
        sp_to_connect = fbe_err_sniff_get_target_sp(spId);
		fbe_api_sim_transport_reset_client(FBE_CLIENT_MODE_DEV_PC);
		fbe_api_sim_transport_set_target_server(sp_to_connect);
        status = fbe_api_sim_transport_init_client(sp_to_connect, FBE_TRUE);/*connect w/o any notifications enabled*/
        if (status != FBE_STATUS_OK) {
            printf("\nCan't connect to FBE err_sniff Server, make sure FBE is running !!!\n");
            return status;
        }

    }else if ((driverType == 'k') || (driverType == 'K')) {
        /*way more simple, just initilize the fbe api user */
        status  = fbe_api_common_init_user(FBE_TRUE);
		if (status != FBE_STATUS_OK) {
			fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s failed to init fbe api user\n", __FUNCTION__); 
        }
    }

    return status;

}

static fbe_status_t fbe_err_sniff_set_notify_level(fbe_char_t level)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_trace_level_t notify_level;

    switch (level) 
    {
        case 'w':
            notify_level = FBE_TRACE_LEVEL_WARNING;
            break;
        default:
            notify_level = FBE_TRACE_LEVEL_ERROR;
            break;
    }

    status = fbe_api_trace_err_set_notify_level(notify_level,FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
            printf("\nCan't set notify level to SEP!!!\n");
            return status;
    }
    status = fbe_api_trace_err_set_notify_level(notify_level,FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
            printf("\nCan't set notify level to Physical!!!\n");
            return status;
    }
    status = fbe_api_trace_err_set_notify_level(notify_level,FBE_PACKAGE_ID_ESP);
    if (status != FBE_STATUS_OK) {
            printf("\nCan't set notify level to ESP!!!\n");
            return status;
    }
    return status;
}


static fbe_u32_t fbe_err_sniff_get_target_sp(fbe_char_t spstr)
{
    if (spstr == 'a' || spstr == 'A'){
        return FBE_SIM_SP_A;
    }else {
        return FBE_SIM_SP_B;
    }
}

void __cdecl fbe_err_sniff_destroy_fbe_api(int in)
{
    FBE_UNREFERENCED_PARAMETER(in);

    fbe_err_sniff_set_notify_level(FBE_TRACE_LEVEL_INVALID);
    if (fbe_err_sniff_spcollect_is_initialized()) {
        printf("\nYou cannot get out of the err sniff application until SPCOLLECT finishes");
        fbe_thread_wait(fbe_err_sniff_spcollect_get_handle());
    }
 
	printf("\nExiting fbe_err_sniff...");
   
    if (simulation_mode) {
        /* destroy job notification must be done before destroy err_sniffent,
         * since it uses the socket connection. */
        fbe_api_common_destroy_job_notification();
		fbe_api_sim_transport_destroy_client(sp_to_connect);
        fbe_api_common_destroy_sim();
    }else{
        fbe_api_common_destroy_user();
    }

	printf("\n");
	fflush(stdout);
    exit(0);

}

static void CSX_GX_RT_DEFCC fbe_err_sniff_csx_cleanup_handler(csx_rt_proc_cleanup_context_t context)
{
    CSX_UNREFERENCED_PARAMETER(context);
    if (CSX_IS_FALSE(csx_rt_assert_get_is_panic_in_progress())) {
        fbe_err_sniff_destroy_fbe_api(0);
    }
}


static fbe_char_t getDriver(fbe_char_t driverType)
{
    fbe_char_t response[80];

    response[0] = driverType;

    while(1)
    {
        if ((response[0] == 's') || (response[0] == 'k'))
        {
            driverType = response[0];
            break;
        }
        printf("Enter Driver Type [k:kernel intf, s:simulator intf] : ");
        fgets(response, sizeof(response), stdin);
    }
    return (driverType);
}

static fbe_char_t getSpId(fbe_char_t spId)
{
    fbe_char_t response[80];
    fbe_bool_t goodResponse = FALSE;

    response[0] = spId;

    while (! goodResponse)
    {
        switch (response[0])
        {

            case 'a':
            case 'A':
                goodResponse = TRUE;
                response[0] = 'a';
                break;

            case 'b':
            case 'B':
                goodResponse = TRUE;
                response[0] = 'b';
                break;
            default:
                printf ("Enter SP Type: a | b : ");
                fgets(response, sizeof(response), stdin);
                break;
        };
    }
    return (response[0]);
}

static fbe_char_t getFMode(fbe_char_t fMode)
{
    fbe_char_t response[80];
    fbe_bool_t goodResponse = FALSE;

    response[0] = fMode;

    while (! goodResponse)
    {
        switch (response[0])
        {

            case 'c':
            case 'C':
                goodResponse = TRUE;
                response[0] = 'c';
                break;

            case 'M':
            case 'm':
                goodResponse = TRUE;
                response[0] = 'm';
                break;
            default:
                printf ("Enter Write File Mode: c | m : ");
                fgets(response, sizeof(response), stdin);
                break;
        };
    }
    return (response[0]);
}


static fbe_char_t getLevel(fbe_char_t level)
{
    fbe_char_t response[80];
    fbe_bool_t goodResponse = FALSE;

    response[0] = level;

    while (! goodResponse)
    {
        switch (response[0])
        {
            case 'w':
            case 'W':
                goodResponse = TRUE;
                response[0] = 'w';
                break;
            case 'e':
            case 'E':
                goodResponse = TRUE;
                response[0] = 'e';
                break;
            default:
                printf ("Trace level: w | e: ");
                fgets(response, sizeof(response), stdin);
                break;
        };
    }
    return (response[0]);
}

static fbe_char_t getPanicFlag(fbe_char_t panicFlag)
{
    fbe_char_t response[80];
    fbe_bool_t goodResponse = FALSE;

    response[0] = panicFlag;

    while (! goodResponse)
    {
        switch (response[0])
        {

            case 'p':
            case 'P':
                goodResponse = TRUE;
                response[0] = 'p';
                break;
            case 'n':
            case 'N':
                goodResponse = TRUE;
                response[0] = 'n';
                break;
            default:
                printf ("panic flag: p | n: ");
                fgets(response, sizeof(response), stdin);
                break;
        };
    }
    return (response[0]);
}

static void dummyWait(void)
{
    printf("Press ^c to abort...\n");
    while(1)
    {
        printf(".");
        EmcutilSleep(500);
    };
}


static void printUsage(void)
{
    printf("Usage: fbe_err_sniff <driver_type><SP><fMode><level>\n");
    printf("       Where driver_type = k[kernel] | s[simulator]\n");
    printf("             SP = a | b\n");
    printf("             fMode = c(clean) | m(merge or append)\n");
    printf("             level = w(warning) | e(error)\n");
    printf("             panicFlag = p(panic) | n(no panic)\n");
    printf("Examples:\n");
    printf("  'fbe_err_sniff s a c e p' - Connect to fbe_err_sniff running on SP A with clean write file mode to trace error and panic SP if there is errors.\n\n");
    printf("  'fbe_err_sniff s a m w n' - Connect to fbe_err_sniff running on SP A with append write file mode to trace warning and don't panic SP.\n");
    return;
}   

fbe_status_t fbe_get_package_id(fbe_package_id_t * package_id)
{
    *package_id = FBE_PACKAGE_ID_PHYSICAL;
    return FBE_STATUS_OK;
}
