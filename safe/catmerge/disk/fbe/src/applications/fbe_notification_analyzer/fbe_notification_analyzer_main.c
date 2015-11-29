
/***************************************************************************
* Copyright (C) EMC Corporation 2012 
* All rights reserved. 
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!*************************************************************************
*                 @file fbe_notification_analyzer_main.c
****************************************************************************
*
* @brief
*  This file contains the main function for fbe_notification_analyzer.
*
* @ingroup fbe_notification_analyzer
*
* @date
*  05/30/2012 - Created. Vera Wang
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
#include <signal.h>
#include "fbe_notification_analyzer_file_access.h"
#include "fbe_notification_analyzer_notification_handle.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_api_trace_interface.h"
#include "fbe/fbe_emcutil_shell_include.h"
//#include <windows.h>

/*!************************
*   LOCAL VARIABLES
**************************/
static FILE * fp;
static fbe_bool_t   simulation_mode = FBE_FALSE;
static fbe_sim_transport_connection_target_t sp_to_connect = FBE_SIM_SP_A;

/*!************************
*   LOCAL FUNCTIONS
**************************/
static fbe_status_t fbe_notification_analyzer_initialize_fbe_api (fbe_char_t driverType , fbe_char_t spId);
static fbe_u32_t fbe_notification_analyzer_get_target_sp(fbe_char_t spstr);
void __cdecl fbe_notification_analyzer_destroy_fbe_api(int in);
static void CSX_GX_RT_DEFCC fbe_notification_analyzer_csx_cleanup_handler(csx_rt_proc_cleanup_context_t context);
static void dummyWait(void);
static fbe_char_t getSpId(fbe_char_t spId);
static fbe_char_t getDriver(fbe_char_t driverType);
static void printUsage(void);
static fbe_status_t fbe_notification_analyzer_get_notification_option(fbe_notification_type_t* type,
                                                              fbe_package_notification_id_mask_t* package, 
                                                              fbe_topology_object_type_t* object_type);
static fbe_notification_type_t fbe_notification_analyzer_get_notification_type(fbe_char_t* notification_type);
static fbe_package_notification_id_mask_t fbe_notification_analyzer_get_notification_package(fbe_char_t* notification_package);
static fbe_topology_object_type_t fbe_notification_analyzer_get_notification_object_type(fbe_char_t* notification_object_type);

int __cdecl main (int argc , char *argv[])
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_char_t spId = 0;
    fbe_char_t driverType = 0;
    fbe_char_t cmdLineChar;
    fbe_u32_t  currentArg;
    fbe_char_t input;
    fbe_notification_type_t type;
    fbe_package_notification_id_mask_t package;
    fbe_topology_object_type_t object_type; 
    fbe_char_t file_name[50] = "notification_analyzer_";

#include "fbe/fbe_emcutil_shell_maincode.h"

    for (currentArg = 1; currentArg < (fbe_u32_t)argc; currentArg++)
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
                spId = cmdLineChar;
        }
        else
        {
            printUsage();
            dummyWait();
            exit(3);
        }
    }
    if (driverType == 0)
    {
        /*if the user doesn't give an option, it will be set to kernal mode and SPA by default.*/
        printf("By default, driverType is K and spId is a. \n");
        driverType = 'k';
        spId = 'a';
    }
    if ((spId = getSpId(spId)) == 0)
    {
        printf("Error in getting SP ID\n");
        printUsage();
        dummyWait();
        exit(0);
    }

    fbe_trace_init();
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_CRITICAL_ERROR);
#if 0 /* SAFEMESS - should let CSX handle this */
    signal(SIGINT, fbe_notification_analyzer_destroy_fbe_api);/*route ctrl C to make sure we nicely close the connections*/
#else
    csx_rt_proc_cleanup_handler_register(fbe_notification_analyzer_csx_cleanup_handler, fbe_notification_analyzer_csx_cleanup_handler, NULL);
#endif

    /*initialize the fbe api connections*/
    printf("\n\nfbe_notification_analyzer Version 1.0 - Initializing...\n");
    //printf("\nUser choices: driver type %c, sp %c, file mode %c, trace level %c, panic flag %c \n", driverType, spId, fMode, level, panicFlag);
    status = fbe_notification_analyzer_initialize_fbe_api(driverType, spId);
    if (status != FBE_STATUS_OK) {
        printf("Failed to init FBE API");
        return -1;
    } 

    /* create the output file name with different spId. */
    file_name[strlen(file_name)] = spId;  
    strcat(file_name,".txt"); 
    fp = fbe_notification_analyzer_open_file(file_name, 'c');
    if (fp == NULL) 
    {
        printf("\n Can not open the file. \n");
        return -1;
    }
    /* Output to the file. */
    fprintf(fp,"fbe_notification_analyzer Version 1.0\n\n");
    fprintf(fp,"User choices: driver type %c, sp %c\n\n", 
            driverType, spId);

    status = fbe_notification_analyzer_get_notification_option(&type, &package, &object_type);
    if (status != FBE_STATUS_OK) {
        printf("Failed to get notification option from file. ");
        return -1;
    }  

    status = fbe_notification_analyzer_notification_handle_init(fp, type, package, object_type); 
    if (status != FBE_STATUS_OK) {
        printf("\nNotification_handle_init failed!\n");
        return -1;
    }
 
    printf("\nPlease enter x or X if you want to exit this application. \n");
    input = getchar();

    while (input != 'x' && input != 'X') 
    {
        printf("\nWrong input. Please enter x or X if you want to exit this application. \n");
        input = getchar(); 
    }

    status = fbe_notification_analyzer_notification_handle_destroy();
    fbe_notification_analyzer_close_file(fp);

    csx_rt_proc_cleanup_handler_deregister(fbe_notification_analyzer_csx_cleanup_handler);

    fbe_notification_analyzer_destroy_fbe_api(0);

    return 0;
}

static fbe_status_t fbe_notification_analyzer_initialize_fbe_api (fbe_char_t driverType , fbe_char_t spId)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    
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
        sp_to_connect = fbe_notification_analyzer_get_target_sp(spId);
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

static fbe_u32_t fbe_notification_analyzer_get_target_sp(fbe_char_t spstr)
{
    if (spstr == 'a' || spstr == 'A'){
        return FBE_SIM_SP_A;
    }else {
        return FBE_SIM_SP_B;
    }
}

void __cdecl fbe_notification_analyzer_destroy_fbe_api(int in)
{
    FBE_UNREFERENCED_PARAMETER(in);
 
	printf("\nExiting fbe_notification_analyzer...");
   
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

static void CSX_GX_RT_DEFCC fbe_notification_analyzer_csx_cleanup_handler(csx_rt_proc_cleanup_context_t context)
{
    CSX_UNREFERENCED_PARAMETER(context);
    if (CSX_IS_FALSE(csx_rt_assert_get_is_panic_in_progress())) {
        fbe_notification_analyzer_destroy_fbe_api(0);
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
    printf("Usage: fbe_notification_analyzer <driver_type><SP>\n");
    printf("       Where driver_type = k[kernel] | s[simulator]\n");
    printf("             SP = a | b\n");
    printf("Examples:\n");
    printf("  'fbe_notification_analyzer s a' - Connect to fbe_notification_analyzer running on SP A in simulation.\n\n");
    printf("  'fbe_notification_analyzer k b' - Connect to fbe_notification_analyzer running on SP B in kernal.\n");
    printf("  by default, if only run fbe_notification_analyzer.exe, it's set to option k a. \n");
    return;
}   

static fbe_status_t fbe_notification_analyzer_get_notification_option(fbe_notification_type_t* type,
                                                              fbe_package_notification_id_mask_t* package, 
                                                              fbe_topology_object_type_t* object_type)
{
    fbe_char_t notification_type[100] = {0};
    fbe_char_t notification_package[50] = {0};
    fbe_char_t notification_object_type[100] = {0}; 
#if 0
    FILE* fp_in;

    fp_in = fbe_notification_analyzer_open_file("notification.txt", 'r');
    if (fp_in == NULL) 
    {
        printf("\n Can not open the file. \n");
        return FBE_STATUS_GENERIC_FAILURE;
    }     

    while(feof(fp_in) == 0) 
    {
        fscanf(fp_in,"%s",notification_type);
        fscanf(fp_in,"%s",notification_package);
        fscanf(fp_in,"%s",notification_object_type);
    }

    fprintf(fp,"User choices: notification_type %s, notification_package %s, notification_object_type %s\n\n", 
            notification_type, notification_package, notification_object_type);
#endif

    *type = fbe_notification_analyzer_get_notification_type(notification_type);
    *package = fbe_notification_analyzer_get_notification_package(notification_package);
    *object_type = fbe_notification_analyzer_get_notification_object_type(notification_object_type);
    return FBE_STATUS_OK;
}


/*!**************************************************************
 * fbe_notification_analyzer_get_notification_type()
 ****************************************************************
 * @brief
 *  This function converts a text string to notification type.
 *
 * @param 
 *        fbe_database_state_t state
 * 
 * @return
 *        fbe_char_t *  A string for db state
 ***************************************************************/
static fbe_notification_type_t fbe_notification_analyzer_get_notification_type(fbe_char_t* notification_type)
{
    switch((fbe_u64_t)*notification_type)
    {
/*
        case "FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_SPECIALIZE":                
            return FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_SPECIALIZE;            
        case "FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_ACTIVATE":                  
            return FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_ACTIVATE;              
        case "FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY":                     
            return FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY;                 
            case "FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_HIBERNATE":             
            return FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_HIBERNATE;             
            case "FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_OFFLINE":               
            return FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_OFFLINE;               
            case "FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL":                  
            return FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL;                  
            case "FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_DESTROY":               
            return FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_DESTROY;               
            case "FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_READY":         
            return FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_READY;         
            case "FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_ACTIVATE":      
            return FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_ACTIVATE;      
            case "FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_HIBERNATE":     
            return FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_HIBERNATE;     
            case "FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_OFFLINE":       
            return FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_OFFLINE;       
            case "FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_FAIL":          
            return FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_FAIL;          
            case "FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_DESTROY":       
            return FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_DESTROY;       
            case "FBE_NOTIFICATION_TYPE_LIFECYCLE_ANY_STATE_CHANGE":            
            return FBE_NOTIFICATION_TYPE_LIFECYCLE_ANY_STATE_CHANGE;            
            case "FBE_NOTIFICATION_TYPE_LIFECYCLE_ANY_NON_PENDING_STATE_CHANGE":
            return FBE_NOTIFICATION_TYPE_LIFECYCLE_ANY_NON_PENDING_STATE_CHANGE;
            case "FBE_NOTIFICATION_TYPE_END_OF_LIFE":                           
            return FBE_NOTIFICATION_TYPE_END_OF_LIFE;                           
            case "FBE_NOTIFICATION_TYPE_RECOVERY":                              
            return FBE_NOTIFICATION_TYPE_RECOVERY;                              
            case "FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED":                   
            return FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED;                   
            case "FBE_NOTIFICATION_TYPE_CALLHOME":                              
            return FBE_NOTIFICATION_TYPE_CALLHOME;                              
            case "FBE_NOTIFICATION_TYPE_CHECK_QUEUED_IO":                       
            return FBE_NOTIFICATION_TYPE_CHECK_QUEUED_IO;                       
            case "FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED":              
            return FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;              
            case "FBE_NOTIFICATION_TYPE_SWAP_INFO":                             
            return FBE_NOTIFICATION_TYPE_SWAP_INFO;                             
            case "FBE_NOTIFICATION_TYPE_CONFIGURATION_CHANGED":                 
            return FBE_NOTIFICATION_TYPE_CONFIGURATION_CHANGED;                 
            case "FBE_NOTIFICATION_TYPE_OBJECT_DESTROYED":                      
            return FBE_NOTIFICATION_TYPE_OBJECT_DESTROYED;                      
            case "FBE_NOTIFICATION_TYPE_FBE_ERROR_TRACE":                       
            return FBE_NOTIFICATION_TYPE_FBE_ERROR_TRACE;                       
            case "FBE_NOTIFICATION_TYPE_CMS_TEST_DONE":                         
            return FBE_NOTIFICATION_TYPE_CMS_TEST_DONE;                         
            case "FBE_NOTIFICATION_TYPE_DATA_RECONSTRUCTION":                   
            return FBE_NOTIFICATION_TYPE_DATA_RECONSTRUCTION;                   
            case "FBE_NOTIFICATION_TYPE_ZEROING":                               
            return FBE_NOTIFICATION_TYPE_ZEROING;                               
            case "FBE_NOTIFICATION_TYPE_LU_DEGRADED_STATE_CHANGED":             
            return FBE_NOTIFICATION_TYPE_LU_DEGRADED_STATE_CHANGED;             
            case "FBE_NOTIFICATION_TYPE_ALL_WITHOUT_PENDING_STATES":            
            return FBE_NOTIFICATION_TYPE_ALL_WITHOUT_PENDING_STATES;            
            case "FBE_NOTIFICATION_TYPE_ALL":                                   
            return FBE_NOTIFICATION_TYPE_ALL;                                   
                                                                                
        default:                                                                
            return FBE_NOTIFICATION_TYPE_INVALID;                               
*/
    }
    return FBE_NOTIFICATION_TYPE_ALL;
}

static fbe_package_notification_id_mask_t fbe_notification_analyzer_get_notification_package(fbe_char_t* notification_package)
{
     switch(*notification_package)
     {
/*
        case "FBE_PACKAGE_NOTIFICATION_ID_INVALID":         
            return FBE_PACKAGE_NOTIFICATION_ID_INVALID;     
        case "FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL":        
            return FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL;    
        case "FBE_PACKAGE_NOTIFICATION_ID_NEIT":            
            return FBE_PACKAGE_NOTIFICATION_ID_NEIT;        
        case "FBE_PACKAGE_NOTIFICATION_ID_SEP_0":           
            return FBE_PACKAGE_NOTIFICATION_ID_SEP_0;       
        case "FBE_PACKAGE_NOTIFICATION_ID_ESP":             
            return FBE_PACKAGE_NOTIFICATION_ID_ESP;         
        case "FBE_PACKAGE_NOTIFICATION_ID_ALL_PACKAGES":    
            return FBE_PACKAGE_NOTIFICATION_ID_ALL_PACKAGES;
                                                            
        default:                                            
            return FBE_PACKAGE_NOTIFICATION_ID_LAST;        
*/
    }
     return FBE_PACKAGE_NOTIFICATION_ID_ALL_PACKAGES;
}
static fbe_topology_object_type_t fbe_notification_analyzer_get_notification_object_type(fbe_char_t* notification_object_type)
{
    switch(*notification_object_type)
     {
/*
        case "FBE_TOPOLOGY_OBJECT_TYPE_BOARD":                 
            return FBE_TOPOLOGY_OBJECT_TYPE_BOARD;             
        case "FBE_TOPOLOGY_OBJECT_TYPE_PORT":                  
            return FBE_TOPOLOGY_OBJECT_TYPE_PORT;              
        case "FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE":             
            return FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE;         
        case "FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE":        
            return FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE;    
        case "FBE_TOPOLOGY_OBJECT_TYPE_LOGICAL_DRIVE":         
            return FBE_TOPOLOGY_OBJECT_TYPE_LOGICAL_DRIVE;     
        case "FBE_TOPOLOGY_OBJECT_TYPE_RAID_GROUP":            
            return FBE_TOPOLOGY_OBJECT_TYPE_RAID_GROUP;        
        case "FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_RAID_GROUP":    
            return FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_RAID_GROUP;
        case "FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_DRIVE":         
            return FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_DRIVE;     
        case "FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE":     
            return FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE; 
        case "FBE_TOPOLOGY_OBJECT_TYPE_LUN":                   
            return FBE_TOPOLOGY_OBJECT_TYPE_LUN;               
        case "FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT":      
            return FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT;  
        case "FBE_TOPOLOGY_OBJECT_TYPE_LCC":                   
            return FBE_TOPOLOGY_OBJECT_TYPE_LCC;               
        case "FBE_TOPOLOGY_OBJECT_TYPE_ALL":                   
            return FBE_TOPOLOGY_OBJECT_TYPE_ALL;               
                                                               
        default:                                               
            return FBE_TOPOLOGY_OBJECT_TYPE_INVALID;           
*/
    }
    return FBE_TOPOLOGY_OBJECT_TYPE_ALL;  
}

fbe_status_t fbe_get_package_id(fbe_package_id_t * package_id)
{
    *package_id = FBE_PACKAGE_ID_SEP_0;
    return FBE_STATUS_OK;
}
