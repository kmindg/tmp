/*********************************************************************
* Copyright (C) EMC Corporation, 1989 - 2011                    
* All Rights Reserved.                                          
* Licensed Material-Property of EMC Corporation.                
* This software is made available solely pursuant to the terms  
* of a EMC license agreement which governs its use.             
*********************************************************************/

/*********************************************************************
*
*  Description: 
*      This file defines the methods of the ESP BOARD MGMT 
*      INTERFACE class.
*
*  Notes:
*      The ESP BOARD MGMT class (espBoardMgmt) is a derived class of 
*      the base class (bESP).
*
*  History:
*      05-Aug-2011 : inital version
*
*********************************************************************/

#ifndef T_ESP_BOARD_MGMT_CLASS_H
#include "esp_board_mgmt_class.h"
#endif

/*********************************************************************
* espBoardMgmt class :: Constructor
*********************************************************************/

espBoardMgmt::espBoardMgmt() : bESP()
{
    // Store Object number
    idnumber = (unsigned) ESP_BOARD_MGMT_INTERFACE;
    espBoardMgmtCount = ++gEspBoardMgmtCount;

    // Store Object name
    idname = new char [IDNAME_LENGTH];
    strcpy(idname, "ESP_BOARD_MGMT_INTERFACE");

    fbe_zero_memory(&post_reset,sizeof(post_reset));
    sp = SP_INVALID;
    
    if (Debug) {
        sprintf(temp, "espBoardMgmt class constructor (%d) %s\n", 
                idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API ESP BOARD MGMT Interface function calls>
    espBoardMgmtInterfaceFuncs = 
        "[function call]\n" \
        "[short name]       [FBE API ESP BOARD MGMT Interface]\n" \
        " ------------------------------------------------------------\n" \
        " rebootSp          fbe_api_esp_board_mgmt_reboot_sp\n" \
        " ------------------------------------------------------------\n";
};

/*********************************************************************
* espBoardMgmt class : Accessor methods
*********************************************************************/

unsigned espBoardMgmt::MyIdNumIs(void)
{
    return idnumber;
};

char * espBoardMgmt::MyIdNameIs(void)
{
    return idname;
};

void espBoardMgmt::dumpme(void)
{ 
    strcpy (key, "espBoardMgmt::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n", 
            idnumber, espBoardMgmtCount);
    vWriteFile(key, temp);
}

/*********************************************************************
* espBoardMgmt Class :: select()
*********************************************************************
*
*  Description:
*      This function looks up the function short name to validate
*      it and then calls the function with the index passed back 
*      from the compare.
*            
*  Input: Parameters
*      index - index into arguments
*      argc  - argument count 
*      *argv - pointer to arguments 
*    
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h 
*
*  History
*      28-Jun-2011 : inital version
*
*********************************************************************/

fbe_status_t espBoardMgmt::select(int index, int argc, char *argv[]) 
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select ESP interface"); 

    // List ESP BOARD MGMT Interface calls if help option and 
    // less than 6 arguments were entered.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) espBoardMgmtInterfaceFuncs );
    }
    else if (strcmp (argv[index], "REBOOTSP") == 0) {
        // reboot sp
        status = reboot_sp(argc, &argv[index]);
    }    
    else {
        // can't find match on short name
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n", 
                BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) espBoardMgmtInterfaceFuncs );
    } 

    return status;
}

/*********************************************************************
* espBoardMgmt Class :: reboot_sp()
*********************************************************************
*
*  Description:
*      Reboots sp
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t espBoardMgmt::reboot_sp(int argc, char ** argv)
{
    // Define specific usage string  
    usage_format =
        " Usage: %s [Sp Id] [Hold In Post] [Hold In Reset] [Reboot Blade]\n"
        "   Sp Id         - spa or spb \n"
        "   Hold In Post  - 1 or 0 \n"
        "   Hold In Reset - 1 or 0 \n"
        "   Reboot Blade  - 1 or 0 \n"
        " For example: %s spa 1 0 1 \n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "rebootSp",
        "espBoardMgmt::reboot_sp",
        "fbe_api_esp_board_mgmt_reboot_sp",
        usage_format,
        argc, 10);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // extract parameters
    argv++;
    if (strcmp (*argv, "SPA") == 0){
        post_reset.sp_id = SP_A;
    }
    else if (strcmp (*argv, "SPB") == 0){
        post_reset.sp_id = SP_B;
    } 
    else {
        status = FBE_STATUS_GENERIC_FAILURE;
        sprintf(temp, "Invalid Sp Id (%s)", *argv);
        params[0] = '\0';
        call_post_processing(status, temp, key, params);
        return status;
    }

    sprintf(params, " sp id (%s) ", *argv);
    char spid[10];
    strcpy(spid, *argv);

    argv++;
    post_reset.holdInPost = (fbe_bool_t)strtoul(*argv, 0, 0);
    
    argv++;
    post_reset.holdInReset= (fbe_bool_t)strtoul(*argv, 0, 0);

    argv++;
    post_reset.rebootBlade = (fbe_bool_t)strtoul(*argv, 0, 0);

    sprintf(params_temp, " Hold In Post (%d) Hold In Reset (%d) "
                         " Reboot Blade (%d)",
                         post_reset.holdInPost,
                         post_reset.holdInPost,
                         post_reset.rebootBlade);

     if(strlen(params_temp) > MAX_PARAMS_SIZE)
    {
        sprintf(temp, "<ERROR!> Params length is larger that buffer value");
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }
    strcat(params, params_temp);
    
    // Make api call: 
    status = fbe_api_esp_board_mgmt_reboot_sp(&post_reset);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to reboot %s", spid);
    } else {
        sprintf(temp, "Successfully rebooted %s", spid);
    }
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* espBoardMgmt end of file
*********************************************************************/
