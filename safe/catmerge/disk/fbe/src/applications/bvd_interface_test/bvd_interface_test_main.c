#include "bvd_interface_test.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_api_sim_transport.h"
#include "fbe/fbe_api_sim_transport_packet_interface.h"
#include "fbe_trace.h"
#include <signal.h>
#include "fbe/fbe_emcutil_shell_include.h"

static fbe_status_t bvd_interface_test_initialize_fbe_api (char driverType , char spId);
static fbe_u32_t bvd_interface_test_get_target_sp(char spstr);
static void __cdecl bvd_interface_test_destroy_fbe_api(int in);
static void dummyWait();
static char getSpId(char spId);
static char getDriver(char driverType);
static void printUsage();
static fbe_sim_transport_connection_target_t       sp_to_connect = FBE_SIM_SP_A;
static fbe_bool_t   simulation_mode = FBE_FALSE;

static void CSX_GX_RT_DEFCC bvd_interface_test_csx_cleanup_handler(csx_rt_proc_cleanup_context_t context);

#define CMD_MAX_ARGC 10 /* Max number of command line arguments. */


char *mainMenu[] = {"Exit",
                    "IOCTL_FLARE_DESCRIBE_EXTENTS (not used anymore)",
                    "IOCTL_DISK_GET_DRIVE_GEOMETRY",
                    "IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS",
                    "IOCTL_FLARE_TRESPASS_EXECUTE",
                    "IOCTL_FLARE_MARK_BLOCK_BAD",
                    "IOCTL_FLARE_MODIFY_CAPACITY",
                    "IOCTL_FLARE_REGISTER_FOR_CHANGE_NOTIFICATION",
                    "IOCTL_FLARE_SET_POWER_SAVING_POLICY",
                    "IOCTL_FLARE_CAPABILITIES_QUERY",
                    "IOCTL_FLARE_GET_VOLUME_STATE",
                    "IOCTL_FLARE_GET_RAID_INFO",
                    "IOCTL_FLARE_READ_BUFFER",
                    "IOCTL_FLARE_WRITE_BUFFER",
                    "IOCTL_STORAGE_EJECT_MEDIA",
                    "IOCTL_STORAGE_LOAD_MEDIA",
                    "IOCTL_FLARE_ZERO_FILL"};


void 
PRINT_MENU(char* menu[], int iSize, char* title)
{
    int i=0;

    printf("\n\n\n");
    for (i=0; i<iSize; i++)
        printf("  %i: %s\n", i, menu[i]);

    printf("\nSelection[0]: "); 
}

void PRINT_LINE(void)
{
    printf("\n\tPress any key to go back ..."); 
    _getch(); 
    printf("\n\n");

}

/* Temporary hack - should be removed */
fbe_status_t 
fbe_get_package_id(fbe_package_id_t * package_id)
{
    *package_id = FBE_PACKAGE_ID_NEIT;
    return FBE_STATUS_OK;
}

int __cdecl main (int argc , char *argv[])
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    char spId = 0;
    char driverType = 0;
    char cmdLineChar;
    fbe_s32_t currentArg;

#include "fbe/fbe_emcutil_shell_include.h"

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
            spId = cmdLineChar ;
        }
        else
        {
            printUsage();
            dummyWait();
            exit(3);
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
    fbe_trace_init();
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_WARNING);
#if 0 /* SAFEMESS - should let CSX handle this */
    signal(SIGINT, bvd_interface_test_destroy_fbe_api);/*route ctrl C to make sure we nicely clode the connections*/
#else
    csx_rt_proc_cleanup_handler_register(bvd_interface_test_csx_cleanup_handler, bvd_interface_test_csx_cleanup_handler, NULL);
#endif

    /*initialize the fbe api connections*/
    printf("\n\nBVD INTERFACE TEST - Initializing...");
    status = bvd_interface_test_initialize_fbe_api(driverType, spId);
    if (status != FBE_STATUS_OK) {
        printf("BVD INTERFACE TEST INTI FAILED");
        return -1;
    }
    bvd_interface_test_prompt_mode();
    
    csx_rt_proc_cleanup_handler_deregister(bvd_interface_test_csx_cleanup_handler);

    bvd_interface_test_destroy_fbe_api(0);

    return 0;
}

static void printUsage()
{
    printf("Usage: bvd_interface_test <driver_type>\n");
    printf("       Where driver_type = k[kernel] | s[simulator]\n");
    printf("             SP = a | b\n");
    printf("Examples:\n");
    printf("  'bvd_interface_test s b' - Connect to bvd_interface_test running on SP B.\n\n");
    return;
}

static void dummyWait()
{
    printf("Press ^c to abort...\n");
    while(1)
    {
        printf(".");
        EmcutilSleep(500);
    };
}

static fbe_status_t bvd_interface_test_initialize_fbe_api (char driverType , char spId)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    

    /*we assume the function before checked we have the connection mode and target SP*/
    /*'s' means simulation*/
    if ((driverType == 's') || (driverType == 'S')) {

        simulation_mode = FBE_TRUE;

         /*initialize the simulation side of fbe api*/
        fbe_api_common_init_sim();

        /*make sure that when a packet will be sent to NEIT package it will be routed accordingally*/
        fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_NEIT,
                                                       fbe_api_sim_transport_send_io_packet,
                                                       fbe_api_sim_transport_send_client_control_packet);

       /*need to initialize the client to connect to the server*/
        sp_to_connect = bvd_interface_test_get_target_sp(spId);
        fbe_api_sim_transport_reset_client(FBE_CLIENT_MODE_DEV_PC);
        fbe_api_sim_transport_set_target_server(sp_to_connect);
        status = fbe_api_sim_transport_init_client(sp_to_connect, FBE_FALSE);/*connect w/o any notifications enabled*/
        if (status != FBE_STATUS_OK) {
            printf("\nCan't connect to FBE SIMULATOR, make sure FBE is running !!!\n");
            return status;
        }

    }else if ((driverType, 'k') || (driverType, 'K')) {
        /*way more simple, just initilize the fbe api user */
        status  = fbe_api_common_init_user();

    }

    return status;

}

static fbe_u32_t bvd_interface_test_get_target_sp(char spstr)
{
    if (spstr == 'a' || spstr == 'A'){
        return FBE_SIM_SP_A;
    }else {
        return FBE_SIM_SP_B;
    }
}

static void __cdecl bvd_interface_test_destroy_fbe_api(int in)
{
    FBE_UNREFERENCED_PARAMETER(in);

    if (simulation_mode) {
        fbe_api_sim_transport_destroy_client(sp_to_connect);
        fbe_api_common_destroy_sim();
    }else{
        fbe_api_common_destroy_user();
    }
    printf("\r");
    fflush(stdout);
    exit(0);

}

static void CSX_GX_RT_DEFCC
fbe_sp_sim_csx_cleanup_handler(
    csx_rt_proc_cleanup_context_t context)
{
    CSX_UNREFERENCED_PARAMETER(context);
    if (CSX_IS_FALSE(csx_rt_assert_get_is_panic_in_progress())) {
        bvd_interface_test_destroy_fbe_api(0);
    }
}

void bvd_interface_test_prompt_mode()
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u8_t line[CMD_MAX_SIZE+1];
    fbe_s32_t lineNum = 0;
    
    do {
        PRINT_MENU(mainMenu, sizeof(mainMenu) / sizeof(char *), "Main Menu");
        gets(line);
        if(strlen(line) > 1){
            PRINT_LINE();
            continue;
        }
        
        lineNum = atoi(line);

        switch(lineNum) {
        // Exit
        case 0:
            printf("\n");
            printf("Exiting menu cleanly\n");
            bvd_interface_test_destroy_fbe_api(0);
        break;

        // Describe Extent
        case 1:
        break;
            
        case 2:
            printf("IOCTL_DISK_GET_DRIVE_GEOMETRY\n");
        break;

        case 3:
           printf("IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS\n");
        break;

        case 4:
           printf("IOCTL_FLARE_TRESPASS_EXECUTE\n");
        break;

        case 5:
           printf("IOCTL_FLARE_MARK_BLOCK_BAD\n");
        break;

        case 6:
           printf("IOCTL_FLARE_MODIFY_CAPACITY\n");
        break;

        case 7:
           printf("IOCTL_FLARE_REGISTER_FOR_CHANGE_NOTIFICATION\n");
        break;

        case 8:
           printf("IOCTL_FLARE_SET_POWER_SAVING_POLICY\n");
        break;

        case 9:
           printf("IOCTL_FLARE_CAPABILITIES_QUERY\n");
        break;

        case 0xA:
           printf("IOCTL_FLARE_GET_VOLUME_STATE\n");
        break;

        case 0xB:
           printf("IOCTL_FLARE_GET_RAID_INFO\n");
        break;

        case 0xC:
           printf("IOCTL_FLARE_READ_BUFFER\n");
        break;

        case 0xD:
           printf("IOCTL_STORAGE_EJECT_MEDIA\n");
        break;

        case 0xE:
           printf("IOCTL_STORAGE_LOAD_MEDIA\n");
        break;

        case 0xF:
           printf("IOCTL_FLARE_ZERO_FILL\n");
        break;

        default:
           bvd_interface_test_destroy_fbe_api(0);
        break;
        }
        if (status != FBE_STATUS_OK) {
        printf("SEND IOCTL Failed\n");
        }
        PRINT_LINE();
    }
    while(1);
}

void bvd_interface_test_printf(const fbe_char_t * fmt, ...)
{
    
    char buffer[MAX_BUFFER_SIZEOF];
    va_list argptr;
    
    va_start(argptr, fmt);
    vsprintf(buffer, fmt, argptr);
    va_end(argptr);  
}

static char getDriver(char driverType)
{
    char response[80];

    response[0] = driverType;

    while(1)
    {
        if ((response[0] == 's') || (response[0] == 'k'))
        {
            driverType = response[0];
            break;
        }
        printf("Enter Driver Type [k:kernel intf, s:simulator intf] : ");
        gets(response);
    }
    return (driverType);
}

static char getSpId(char spId)
{
    char response[80];
    BOOL goodResponse = FALSE;

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
                gets(response);
                break;
        };
    }
    return (response[0]);
}

