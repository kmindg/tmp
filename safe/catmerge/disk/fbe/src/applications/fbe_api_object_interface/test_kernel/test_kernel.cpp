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

static fbe_sim_transport_connection_target_t sp_to_connect = FBE_SIM_SP_A;

//-----------------------------------------------------------------

int __cdecl main (int argc, char *argv[])
{

#include "fbe/fbe_emcutil_shell_maincode.h"

    printf("\n\n***create initFbeApiCLASS object "
        "(pInit = new initFbeApiCLASS())\n");
    initFbeApiCLASS    * pInit = new initFbeApiCLASS();

    printf("\n\n***initialize kernel (pInit->Init_Simulator)\n");
    fbe_sim_transport_connection_target_t spId = FBE_SIM_SP_A;
    fbe_status_t status = pInit->Init_Kernel(spId);
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

/*********************************************************************
 * test end of file
 *********************************************************************/
