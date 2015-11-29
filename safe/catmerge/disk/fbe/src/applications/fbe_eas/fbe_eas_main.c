#include <windows.h>
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_api_sim_transport.h"
#include "fbe/fbe_api_sim_transport_packet_interface.h"
#include "fbe_trace.h"
#include "fbe/fbe_api_discovery_interface.h"
#include <signal.h>
#include "fbe/fbe_cli.h"/*this is the old fbe cli header file, we will take it out later*/
#include "fbe_eas_private.h"
#include "fbe/fbe_emcutil_shell_include.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_board.h"
#include "fbe/fbe_raid_group.h"

 
static fbe_status_t fbe_eas_initialize_fbe_api (char driverType , char spId, fbe_u16_t sp_port_base);
static fbe_u32_t fbe_eas_get_target_sp(char spstr);
void __cdecl fbe_eas_destroy_fbe_api(int in);
static void CSX_GX_RT_DEFCC fbe_eas_csx_cleanup_handler(csx_rt_proc_cleanup_context_t context);
static void dummyWait(void);
static char getSpId(char spId);
static char getDriver(char driverType);
static fbe_bool_t extractFilename(char *argPtr, char *filename);
static fbe_bool_t extractSPPortBase(char *argPtr, fbe_u16_t *sp_port_base);
static unsigned int parseOutputMode(char outputModeChar);
static void getFilename(char *filename);
static int openOutputFile(unsigned int outputMode, char *filename, csx_p_file_t *outfileHandle);
static int closeOutputFile(csx_p_file_t *outFileHandle);
static int writeOutputFile(csx_p_file_t outputFile, char *buf_p, int len);
static void printUsage(void);
static void fbe_cli_output(csx_p_file_t outputFile, char *buf_p, int len, int outputMode);
static int handleScript(char *scriptFileName_p) ;
static fbe_sim_transport_connection_target_t       sp_to_connect = FBE_SIM_SP_A;
static fbe_bool_t sim_mode = FBE_FALSE;
static fbe_bool_t is_destroy_needed = FBE_TRUE;

csx_p_file_t outFileHandle = NULL;
unsigned int outputMode;
char filename[FBE_CLI_CONSOLE_MAX_FILENAME];
static fbe_bool_t   simulation_mode = FBE_FALSE;
unsigned int scriptFile;
char scriptFileName[FBE_CLI_CONSOLE_MAX_FILENAME];
char direct_command[FBE_CLI_CONSOLE_MAX_FILENAME];


const char *fbe_eas_pwr_up_p =
"\nCopyright (c) EMC CORPORATION   1991 - 2012   ALL RIGHTS RESERVED\n\n"
"LICENSED MATERIAL - PROPERTY OF EMC CORPORATION, HOPKINTON MA 01748\n\n"
"Unpublished - all rights reserved under the copyright laws of the United States\n\n"
"\t\t\t\tCORE SOFTWARE\n\n"
"This software is considered to be \"Core Software\".  Core Software may\n"
"be used only by users properly licensed to use Core Software, and only\n"
"according to the terms of that license.\n\n";


int __cdecl main (int argc , char *argv[])
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    char spId = 0;
    char driverType = 0;
    char cmdLineChar;
    int currentArg;
    fbe_u16_t sp_port_base = 0;
	char *command_to_run = NULL;
    fbe_base_board_get_base_board_info_t base_board_info;
    fbe_object_id_t board_object_id;

#include "fbe/fbe_emcutil_shell_maincode.h"
    scriptFile = 0;
    scriptFileName[0] = 0;
	eas_operating_mode = USER_MODE;

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
        else if ((cmdLineChar == 'o') || (cmdLineChar == 'x'))
        {
            if (outputMode)
            {
                // Option is supplied twice.
                printUsage();
                dummyWait();
                exit(2);
            }

            outputMode = parseOutputMode(cmdLineChar);
            
            // If there is no filename following the 'o' or 'x', query the user.
            if (extractFilename(argv[currentArg], filename) == FALSE)
            {
                getFilename(filename);
            }
            
            if (openOutputFile(outputMode, filename, &outFileHandle) < 0)
            {
                printf("Error opening file %s\n", filename);
                printUsage();
                dummyWait();
                exit(1);
            }
        }
        else if ((cmdLineChar == 'z'))
        {
            if(scriptFile)
            {
                printUsage();
                dummyWait();
                exit(2);
            } 
            else 
            {
                scriptFile = 0x1;
                
                // If there is no filename following the 'z', exit
                if (extractFilename(argv[currentArg], scriptFileName) == FALSE)
                {
                    printUsage();
                    printf("A script file path is required.");
                    exit(1);
                }
              
                printf("Script File: %s\n",scriptFileName);
            }
        }
        else if ((cmdLineChar == 'p'))
        {
            if (sp_port_base)
            {
                // Option is supplied twice.
                printUsage();
                dummyWait();
                exit(2);
            }
            // If there is no filename following the 'z', exit
            if (extractSPPortBase(argv[currentArg], &sp_port_base) == FALSE)
            {
                printUsage();
                printf("sp port base is required.");
                dummyWait();
                exit(1);
            }
        }
		else if ((cmdLineChar == 'c'))
        {
            if (command_to_run)
            {
                /* Option is supplied twice ! */
                printUsage();
                dummyWait();
                exit(1);
            }
            else
			{
				fbe_zero_memory(direct_command, FBE_CLI_CONSOLE_MAX_FILENAME);
				fbe_copy_memory(direct_command, &argv[currentArg][1], (fbe_u32_t)strlen(argv[currentArg]) - 1);
				command_to_run = direct_command;
				
			}
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
        printf("\nBy default, driverType is K and spId is a. If you would like to run fbeeas.exe on simulation, please use fbeeas.exe s a or b\n");
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
    if (outputMode == 0)
    {
        outputMode = FBE_CLI_CONSOLE_OUTPUT_STDOUT;
        outFileHandle = NULL;
    }

    fbe_trace_init();
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_CRITICAL_ERROR);
#if 0 /* SAFEMESS - should let CSX handle this */
    signal(SIGINT, fbe_eas_destroy_fbe_api);/*route ctrl C to make sure we nicely clode the connections*/
#else
    csx_rt_proc_cleanup_handler_register(fbe_eas_csx_cleanup_handler, fbe_eas_csx_cleanup_handler, NULL);
#endif


    /*initialize the fbe api connections*/
    printf("\n\nFBE EAS - Initializing...");
    if (driverType == 's') {
        sim_mode = FBE_TRUE;
    }
    status = fbe_eas_initialize_fbe_api(driverType, spId, sp_port_base);
    if (status != FBE_STATUS_OK) {
        printf("Failed to init FBE API\n");
        return -1;
    }

    /* Initialize the PSL */
    status = fbe_api_get_board_object_id(&board_object_id);
    if (status != FBE_STATUS_OK) {
        printf("Failed to init get base board object id");
        return -1;
    }

    status = fbe_api_board_get_base_board_info(board_object_id, &base_board_info);
    if (status != FBE_STATUS_OK) {
        printf("Failed to get base board info");
        return -1;
    }
    status = fbe_private_space_layout_initialize_library(base_board_info.platformInfo.platformType);
    if (status != FBE_STATUS_OK) {
        printf("Failed to init Private Space Layout");
        return -1;
    }

    /* Output the power-up message */
    printf(fbe_eas_pwr_up_p);
    printf("Ready\n\n");
    // Handle a script file if provided
    if(*scriptFileName != 0)
    {
        if(handleScript(scriptFileName) == -1) 
        {
            exit(1);
        }
       csx_rt_proc_cleanup_handler_deregister(fbe_eas_csx_cleanup_handler);
       fbe_eas_destroy_fbe_api(0);
       is_destroy_needed = FBE_TRUE;
	   return 0;
    }

	if (command_to_run) {
		fbe_eas_run_direct_command(direct_command);
	}else{
		fbe_eas_prompt_mode();
	}

    csx_rt_proc_cleanup_handler_deregister(fbe_eas_csx_cleanup_handler);
    fbe_eas_destroy_fbe_api(0);
    is_destroy_needed = FBE_TRUE;
    return 0;
}

static fbe_status_t fbe_eas_initialize_fbe_api (char driverType , char spId, fbe_u16_t sp_port_base)
{
    fbe_status_t    							status = FBE_STATUS_GENERIC_FAILURE;
    

    /*we assume the function before checked we have the connection mode and target SP*/
    /*'s' means simulation*/
    if ((driverType == 's') || (driverType == 'S')) {

        simulation_mode = FBE_TRUE;

        /* set sp port base */
        if(sp_port_base > 0)
        {
            fbe_api_sim_transport_set_server_mode_port(sp_port_base);
        }

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
        sp_to_connect = fbe_eas_get_target_sp(spId);
		fbe_api_sim_transport_reset_client(FBE_CLIENT_MODE_DEV_PC);
		fbe_api_sim_transport_set_target_server(sp_to_connect);
        status = fbe_api_sim_transport_init_client(sp_to_connect, FBE_TRUE);/*connect w/o any notifications enabled*/
        if (status != FBE_STATUS_OK) {
            printf("\nCan't connect to FBE EAS Server, make sure FBE is running !!!\n");
            return status;
        }

    }
    else if ((driverType == 'k') || (driverType == 'K')) 
    {
        /*way more simple, just initilize the fbe api user */
        status  = fbe_api_common_init_user(FBE_TRUE);
		if (status != FBE_STATUS_OK) {
			fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s failed to init fbe api user\n", __FUNCTION__); 
            return FBE_STATUS_GENERIC_FAILURE;
		}
		/*this part is taking care of getting notifications from jobs and letting FBE APIs know the job completed*/
		status  = fbe_api_common_init_job_notification();
		if (status != FBE_STATUS_OK) {
			fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s failed to init notification interface\n", __FUNCTION__); 
		}

    }

    return status;

}

static fbe_u32_t fbe_eas_get_target_sp(char spstr)
{
    if (spstr == 'a' || spstr == 'A'){
        return FBE_SIM_SP_A;
    }else {
        return FBE_SIM_SP_B;
    }
}

void __cdecl fbe_eas_destroy_fbe_api(int in)
{
    FBE_UNREFERENCED_PARAMETER(in);

	printf("\nExiting FBE EAS...");

    if(is_destroy_needed)
    {
        /* Set this to FALSE when we call this function */
        is_destroy_needed = FBE_FALSE;
        if (simulation_mode) {
            /* destroy job notification must be done before destroy client,
             * since it uses the socket connection. */
            fbe_api_common_destroy_job_notification();
            fbe_api_sim_transport_destroy_client(sp_to_connect);
            fbe_api_common_destroy_sim();
        }else{
            fbe_api_common_destroy_user();
        }
        if (NULL != outFileHandle) {
            closeOutputFile(&outFileHandle);
        }
        printf("\n");
        fflush(stdout);
    }

    return;
}

static void CSX_GX_RT_DEFCC
fbe_eas_csx_cleanup_handler(
    csx_rt_proc_cleanup_context_t context)
{
    FBE_UNREFERENCED_PARAMETER(context);
    fbe_eas_destroy_fbe_api(0);
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
        fgets(response, sizeof(response), stdin);
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

static fbe_bool_t extractFilename(char *argPtr, char *filename)
{
    // If there is nothing following the 'o' or 'x', return failure.
    if (strlen(argPtr) == 1)
    {
        return FALSE;
    }

    argPtr++;
    strncpy(filename, argPtr, FBE_CLI_CONSOLE_MAX_FILENAME);

    // Force filename to be null-terminated if it is max sized.
    filename[FBE_CLI_CONSOLE_MAX_FILENAME-1] = '\0';
    
    return TRUE;
}

static fbe_bool_t extractSPPortBase(char *argPtr, fbe_u16_t *sp_port_base)
{
    // If there is nothing following the 'p', return failure.
    if (strlen(argPtr) == 1)
    {
        return FALSE;
    }

    argPtr++;

    *sp_port_base = atoi(argPtr);

    return TRUE;
}

static unsigned int parseOutputMode(char outputModeChar)
{
    unsigned int outputMode = 0;

    switch(outputModeChar)
    {
        case 'x':
            outputMode = FBE_CLI_CONSOLE_OUTPUT_FILE;
            break;

        case 'o':
            outputMode = FBE_CLI_CONSOLE_OUTPUT_FILE | FBE_CLI_CONSOLE_OUTPUT_STDOUT;
            break;

        default:
            outputMode = FBE_CLI_CONSOLE_OUTPUT_STDOUT;
            break;
    }
    return outputMode;
}

static void getFilename(char *filename)
{
    printf("Enter output filename (default <%s>: ", FBE_CLI_CONSOLE_OUTPUT_FILENAME);

    // fgets is safer since we can specify a max size.  However, it places the newline
    //  in the output buffer, so we need to remove this.
    fgets(filename, FBE_CLI_CONSOLE_MAX_FILENAME, stdin);
    if (filename[strlen(filename) - 1] == '\n')
    {
        filename[strlen(filename) - 1] = '\0';
    }
    
    // Check for empty filename, use default
    if (filename[0] == '\0')
    {
        strncpy(filename, FBE_CLI_CONSOLE_OUTPUT_FILENAME, FBE_CLI_CONSOLE_MAX_FILENAME);
    }

    return;
}

static int openOutputFile(unsigned int outputMode, char *filename, csx_p_file_t *outfileHandle)
{
    csx_status_e status;

    // Do nothing if we are using stdout only
    if ((outputMode == FBE_CLI_CONSOLE_OUTPUT_STDOUT) ||
        (outputMode == FBE_CLI_CONSOLE_OUTPUT_NONE))
    {
        *outfileHandle = 0;
        return 0;
    }

    // Filename must be provided.
    if (filename == NULL)
    {
        printf("No filename provided\n");
        return -1;
    }
    
    // Echo filename
    printf("Opening: %s\n", filename);

    status = csx_p_file_open(CSX_MY_MODULE_CONTEXT(), outfileHandle, filename,
                             "wx");

    if (!CSX_SUCCESS(status)) 
    { 
        printf("Could not open file %s (error 0x%x)\n", filename, (int)status);
        *outfileHandle = NULL;
        return -1;
    }

    return 0;
}

static int closeOutputFile(csx_p_file_t *outFileHandle)
{
    csx_status_e status;

    // First check to see if output file exists
    if (outFileHandle)
    {
        printf("Closing output file\n");

        status = csx_p_file_close(outFileHandle);

        if (!CSX_SUCCESS(status))
        {
            printf("Error when closing file (error 0x%x)\n", (int)status);
            return -1;
        }
        outFileHandle = NULL;
    }

    return 0;
}

void fbe_cli_printf(const fbe_char_t * fmt, ...)
{
    
    char buffer[MAX_BUFFER_SIZEOF];
    va_list argptr;
    
    va_start(argptr, fmt);
    vsprintf(buffer, fmt, argptr);
    va_end(argptr);  
    fbe_cli_output(outFileHandle, buffer, (ULONG)(strlen(buffer)), outputMode);
	return;
}

void fbe_cli_trace_func(fbe_trace_context_t trace_context, const char * fmt, ...)
{
    va_list ap;
    char string_to_print[256];

    va_start(ap, fmt);
    _vsnprintf(string_to_print, (sizeof(string_to_print) - 1), fmt, ap);
    va_end(ap);
	fbe_cli_output(outFileHandle, string_to_print, (ULONG)(strlen(string_to_print)), outputMode);
    return;
}

void fbe_cli_error(const fbe_char_t * fmt, ...)
{
    char buffer[MAX_BUFFER_SIZEOF];
    char buffer_fmt[MAX_BUFFER_SIZEOF];
    va_list argptr;

    va_start(argptr, fmt);
    vsprintf(buffer_fmt, fmt, argptr);
    fbe_sprintf(buffer, sizeof(buffer), "FBE-EAS ERROR!  %s", buffer_fmt);
    va_end(argptr);  

    fbe_cli_output(outFileHandle, buffer, (ULONG)(strlen(buffer)), outputMode);
    return;
}
static void fbe_cli_output(csx_p_file_t outputFile, char *buf_p, int len, int outputMode)
{
    // Verify we are in file output mode and file exists
    if ((outputMode & FBE_CLI_CONSOLE_OUTPUT_FILE) && (outputFile))
    {
        writeOutputFile(outputFile, buf_p, len);
    }

    if (outputMode & FBE_CLI_CONSOLE_OUTPUT_STDOUT)
    {
        // echo output to the console
        printf("%s", buf_p);
    }
}

static int writeOutputFile(csx_p_file_t outputFile, char *buf_p, int len)
{
    csx_status_e status;
    csx_size_t   bytesWritten = 0;
    char         *cur_p;   // Pointer to current location in input buffer
    csx_size_t   bytesLeft;
    csx_size_t   bytesToOutput;

    bytesLeft = len;
    cur_p = buf_p;

    while(bytesLeft > 0)
    {
        // Output any characters up to a linefeed or carriage return
        bytesToOutput = (int)(strcspn(cur_p, "\n"));

        if((bytesToOutput == 0) || (bytesToOutput > bytesLeft))
        {
            bytesToOutput = bytesLeft;
        }

        status = csx_p_file_write(&outputFile, cur_p, bytesToOutput,
                &bytesWritten);
        cur_p += bytesWritten;
        bytesLeft -= bytesWritten;

        if (!CSX_SUCCESS(status))
        {
            printf("Error writing to file (error 0x%x)\n", (int)status);
            return (int)status;
        }
    }

    return 0;
}

static void printUsage(void)
{
    printf("Usage: fbeeas <driver_type><SP> <output_type><filename>\n");
	printf("         <script_file><filename> [sp_port_base][port_base_number]\n");
    printf("       Where driver_type = k[kernel] | s[simulator]\n");
    printf("             SP = a | b\n");
    printf("             output_type = o[utputToFile] | x[clusiveOutputToFile]\n");
	printf("             script = z[scriptFile]\n");
    printf("             filename = Path to output file surrounded by quotes.  No space\n");
    printf("                        should be used between the output_type and filename.\n");
    printf("             sp_port_base = p[portBase]\n");
    printf("             port_base_number = SP port base for fbeeas connects to.  No space\n");
    printf("                        should be used between the sp_port_base and port_base_number.\n");
    printf("Examples:\n");
    printf("  'fbeeas s a' - Connect to fbeeas running on SP A.\n\n");
    printf("  'fbeeas s a o\"C:\\fbe_eas_output.txt\"' - Connect to fbeeas running on SP A\n");
    printf("   and log the output to the file \"C:\\fbe_eas_output.txt\".\n");
    printf("  by default, if only run fbeeas.exe, it's set to option k a. \n");
    return;
}

static int handleScript(char *scriptFileName_p) 
{
   
    fbe_cli_cmd_t *cmd = NULL;
    char *argv[CMD_MAX_ARGC] = {0};
    fbe_u32_t argc = 1;

    argv[0] = "run_script";
    argv[1] = scriptFileName_p;
 
    fbe_eas_cmd_lookup(argv[0], &cmd);
    (*cmd->func)(argc, &argv[1]);
    return 0;
}

fbe_bool_t fbe_cli_is_simulation(){
    return sim_mode;
}

