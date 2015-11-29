/*********************************************************************
 * Copyright (C) EMC Corporation, 1989 - 2009
 * All Rights Reserved.
 * Licensed Material-Property of EMC Corporation.
 * This software is made available solely pursuant to the terms
 //* of a EMC license agreement which governs its use.
 *********************************************************************/

#ifndef T_INITFBEAPICLASS_H
#include "init_fbe_api_class.h"
#endif

#ifndef T_ARGCLASS_H
#include "arg_class.h"
#endif

#include "fbe/fbe_emcutil_shell_include.h"

//---------------------------------------------------------------------

//extern "C" void DbgBreakPoint(void);

static fbe_sim_transport_connection_target_t       sp_to_connect = FBE_SIM_SP_A;

static fbe_status_t fbe_cli_initialize_fbe_api (char driverType , char spId)
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
        //sp_to_connect = (fbe_sim_transport_connection_target_t)fbe_cli_get_target_sp(spId);
		sp_to_connect = FBE_SIM_SP_A;

		fbe_api_sim_transport_reset_client(FBE_CLIENT_MODE_DEV_PC);

		fbe_api_sim_transport_set_target_server(sp_to_connect);

        status = fbe_api_sim_transport_init_client(sp_to_connect, FBE_TRUE);/*connect w/o any notifications enabled*/
        if (status != FBE_STATUS_OK) {
            printf("\nCan't connect to FBE CLI Server, make sure FBE is running !!!\n");
            return status;
        }

    }
	/*else if ((driverType, 'k') || (driverType, 'K')) {
        //way more simple, just initilize the fbe api user 
        status  = fbe_api_common_init_user();
		if (status != FBE_STATUS_OK) {
			//fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s failed to init fbe api user\n", __FUNCTION__); 
			printf("\nfailed to init fbe api user\n");
		}
		//this part is taking care of getting notifications from jobs and letting FBE APIs know the job completed
		status  = fbe_api_common_init_job_notification();
		if (status != FBE_STATUS_OK) {
			//fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s failed to init notification interface\n", __FUNCTION__); 
			printf("\nfailed to init notification interface\n");
		}

    }
	*/
    return status;

}

//-----------------------------------------------------------------


int __cdecl main (int argc, char *argv[])
{

#include "fbe/fbe_emcutil_shell_maincode.h"

	/*
	if ( strncmp (argv[1],"-b", 2) == 0){ 
		DbgBreakPoint();
	}
	*/

	fbe_debug_break();

	//-----------------------------------------------------------------
	//fbe_status_t        status1;// = FBE_STATUS_GENERIC_FAILURE;
    //fbe_trace_init();
    //fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_WARNING);
    //signal(SIGINT, fbe_cli_destroy_fbe_api);/*route ctrl C to make sure we nicely clode the connections*/
/*
    //initialize the fbe api connections
    printf("\n\nFBE CLI - Initializing...");
    status1 = fbe_cli_initialize_fbe_api('s', FBE_SIM_SP_A);
    if (status1 != FBE_STATUS_OK) {
        printf("Failed to init FBE API");
        return -1;
    }
	printf("\n\nDone with Shay's code...");
	return 0;	
*/	
    //-----------------------------------------------------------------

    printf("\n\n***create initFbeApiCLASS object "
		"(pInit = new initFbeApiCLASS())\n");
    initFbeApiCLASS    * pInit = new initFbeApiCLASS();

    printf("\n\n***initialize simulator (pInit->Init_Simulator)\n");
    fbe_sim_transport_connection_target_t spId = FBE_SIM_SP_A;
    fbe_status_t status = pInit->Init_Simulator(spId);
    printf("status of pInit->Init_Simulator call [%x]\n", status);

    // assign default values
    fbe_u32_t object_id = 0x4;
    status = FBE_STATUS_GENERIC_FAILURE;
    char temp[1000]; 

    // structure to capture physical drive info from api
    fbe_physical_drive_information_t drive_information;
        
	printf("\n\n***call fbe_api_physical_drive_get_cached_drive_info: "
		 "object_id 0x4\n");
     status = fbe_api_physical_drive_get_cached_drive_info(
        object_id, 
        &drive_information, 
        FBE_PACKET_FLAG_NO_ATTRIB);
        
     // Status OK, write and display drive info. 
    sprintf(temp, "<Vendor ID> %d <Revision> %s <Serial Number> %s", 
            drive_information.drive_vendor_id,
            drive_information.product_rev,
            drive_information.product_serial_num);
	printf("%s\n", temp); 

    printf("status of fbe_api_physical_drive_get_cached_drive_info "
		"call [%x]\n", status);
    return 0;

    //-----------------------------------------------------------------
    
    printf("\n\n*** Call Arguments (create pArg obj) ...\n");
    Arguments * pArg = new Arguments(argc, argv);

    printf("\n\n***pArg->Do_Arguments(argc, argv) \n\n");
    status = pArg->Do_Arguments(argc, argv);
    printf("\nstatus of last call [%x]\n", status);
    return 0;

    printf("\n\n*** Get data from Arguments object...\n");
    printf("getDebug         %d\n", pArg->getDebug());
    printf("getSP            %d\n", pArg->getSP());
    printf("getPackage       %d\n", pArg->getPackage());
    printf("getPackageSubset %d\n", pArg->getPackageSubset());
    return 0;

    //-----------------------------------------------------------------

    printf("\n\n***create initFbeApiCLASS object (pApi)...\n");
    initFbeApiCLASS    * pApi = new initFbeApiCLASS();

    printf("\n\n***Call List_Arguments (pArg)(list program arguments)\n");
    pArg->List_Arguments(argc, argv);

    printf("\n\n***Call Usage (pArg) (help)\n");
    pArg->Usage("");

    printf("\n\n***Open files (pArg) \n");
    pArg->Open_Log_File();
    pArg->Open_Trans_File();
    
    printf("\n\n***create Object (base) object (pObj)...\n");
    Object    * pObj = new Object();
    
    printf("\n\n***create bPhysical object (pPhy)...\n");
    bPhysical * pPhy = new bPhysical();
    
    printf("\n\n***create phyDrive object (pdrv)...\n");
    PhyDrive    Drv;
    PhyDrive  * pDrv = &Drv;
    
    printf("\n\n***Call phy->HelpCmds...\n"); 
    pPhy->HelpCmds("");
    
    printf("\n\n***Call dumpme ...\n");
    pArg->dumpme();
    pObj->dumpme();
    pPhy->dumpme();
    pDrv->dumpme();
   
    printf("\n\n*** Call dumpme using cast to get base Object...\n");
    Object * p = (Object *) &Drv;
    p->dumpme();

    printf("\n\ndelete pArg pointer...\n");
    delete pArg;
    //delete pPhy;

    printf("\n\nexiting main...\n");
    printf("exiting main...\n");
    return 0;
}

/*
fbe_status_t __stdcall fbe_get_package_id(fbe_package_id_t * package_id)
{
	*package_id = FBE_PACKAGE_ID_PHYSICAL;
	return FBE_STATUS_OK;
}
*/
/*********************************************************************
 * fbe file routines
 *********************************************************************/
/*
static int openOutputFile(
	unsigned int outputMode, char *filename, HANDLE *outfileHandle)
{
    // Do nothing if we are using stdout only
    if ((outputMode == FBE_CLI_CONSOLE_OUTPUT_STDOUT) ||
        (outputMode == FBE_CLI_CONSOLE_OUTPUT_NONE)){
        *outfileHandle = 0;
        return 0;
    }

    // Filename must be provided.
    if (filename == NULL) {
        printf("No filename provided\n");
        return -1;
    }
    
    // Open file; will overwrite any existing file of the same name.
    printf("Opening: %s\n", filename);
    *outfileHandle = CreateFile(filename,
		GENERIC_WRITE,          // open for writing
		0,                      // do not share
		NULL,                   // default security
		CREATE_ALWAYS,          // overwrite existing
		FILE_ATTRIBUTE_NORMAL,  // normal file
		NULL);                  // no attr. template

    if (*outfileHandle == INVALID_HANDLE_VALUE) { 
        printf("Could not open file %s (error %d)\n", 
			filename, GetLastError());
		*outfileHandle = NULL;
        return -1;
    }

    return 0;
}

static int closeOutputFile(HANDLE outFileHandle)
{
    BOOL result = FALSE;

    // First check to see if output file exists
    if (outFileHandle) 
	{
        printf("Closing output file\n");
        result = CloseHandle(outFileHandle);

        if (result == FALSE){
            printf("Error when closing file (error %d)\n", 
				GetLastError());
            return -1;
        }
    }

    return 0;
}


void fbe_cli_printf(const fbe_u8_t * fmt, ...)
{
    
    char buffer[MAX_BUFFER_SIZEOF];
    va_list argptr;
    
    va_start(argptr, fmt);
    vsprintf(buffer, (char *) fmt, argptr);
    
	va_end(argptr);  
    fbe_cli_output(outFileHandle, buffer, 
		(ULONG)(strlen(buffer)), outputMode);
	
	return;
}

static void fbe_cli_trace_func(
	fbe_trace_context_t trace_context, const char * fmt, ...)
{
    va_list ap;
    char string_to_print[256];

    va_start(ap, fmt);
    _vsnprintf(string_to_print, (sizeof(string_to_print) - 1), fmt, ap);
    
	va_end(ap);
	fbe_cli_output(outFileHandle, string_to_print, 
		(ULONG)(strlen(string_to_print)), outputMode);
    
	return;
}

static void fbe_cli_error(const fbe_u8_t * fmt, ...)
{
	char buffer[MAX_BUFFER_SIZEOF];
	char buffer_fmt[MAX_BUFFER_SIZEOF];
    va_list argptr;
    
	va_start(argptr, fmt);
    vsprintf(buffer_fmt, (char *) fmt, argptr);
	strcpy(buffer,"FBE-CLI ERROR!  ");
	strcat(buffer, buffer_fmt);
    
	va_end(argptr);  
    fbe_cli_output(outFileHandle, buffer, 
		(ULONG)(strlen(buffer)), outputMode);
	
	return;
}

static void fbe_cli_output(
	HANDLE outputFile, char *buf_p, int len, int outputMode)
{
    // Verify we are in file output mode and file exists
    if ((outputMode & FBE_CLI_CONSOLE_OUTPUT_FILE) && (outputFile)){
        writeOutputFile(outputFile, buf_p, len);
    }
    
	// echo output to the console
    if (outputMode & FBE_CLI_CONSOLE_OUTPUT_STDOUT){
        printf("%s", buf_p);
    }
}

static int writeOutputFile(HANDLE outputFile, char *buf_p, int len)
{
	BOOL result      = FALSE;
	char *cur_p      = buf_p;
	int bytesWritten = 0;
    int bytesLeft    = len;
	int bytesToOutput;

    while (bytesLeft > 0){

		// cur_p[n] is pointer to current location in input buffer.
		switch (cur_p[0]){
            
			case '\n':  // LF
        
				// Insert a \r\n for any lone \n found. This was not 
				// preceded by a \r, or we would have skipped it.
				result = WriteFile(outputFile, 
					"\r\n", 2, (LPDWORD) &bytesWritten, NULL);
				cur_p++;
                bytesLeft--;
                break;

            case '\r':  // CR

				// Insert a \r\n for any lone \r found.
				result = WriteFile(outputFile, 
					"\r\n", 2, (LPDWORD) &bytesWritten, NULL);
                cur_p++;
                bytesLeft--;

                // If there is a following \n, skip it since 
				// we just output it.
                if (cur_p[0] == '\n'){
                    cur_p++;
                    bytesLeft--;
                }
                break;

            default:

				// Output any characters up to a linefeed or 
				// carriage return
                bytesToOutput = (int)(strcspn(cur_p, "\r\n"));

                if((bytesToOutput == 0) || (bytesToOutput > bytesLeft)){
                    bytesToOutput = bytesLeft;
                }

				result = WriteFile(outputFile, cur_p, 
					bytesToOutput, (LPDWORD) &bytesWritten, NULL);

                cur_p += bytesWritten;
                bytesLeft -= bytesWritten;
                break;
        }

        if (result == FALSE){
            printf("Error writing to file (error %d)\n", 
				GetLastError());
            return GetLastError();
        }
    }

    return 0;
}
*/
/*********************************************************************
 * test end of file
 *********************************************************************/
