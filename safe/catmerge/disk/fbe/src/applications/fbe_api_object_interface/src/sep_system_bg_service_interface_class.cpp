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
*      This file defines the methods of the SEP SYSTEM BG SERVICE 
*      INTERFACE class.
*
*  Notes:
*      The SEP SYSTEM BG SERVICE INTERFACE class (sepSystemBgService) is a 
*      derived class of the base class (bSEP).
*
*  History:
*      09-April-2012 : initial version
*
*********************************************************************/

#ifndef T_SEP_SYSTEM_BG_SERVICE_INTERFACE_CLASS_H
#include "sep_system_bg_service_interface_class.h"
#endif

/*********************************************************************
* sepSystemBgService class :: Constructor
*********************************************************************/

sepSystemBgService::sepSystemBgService() : bSEP()
{
    // Store Object number
    idnumber = (unsigned) SEP_SYSTEM_BG_SERVICE_INTERFACE;
    systemBgServiceCount = ++gSepSystemBgServiceCount;

    // Store Object name
    idname = new char [64];
    strcpy(idname, "SEP_SYSTEM_BG_SERVICE_INTERFACE");

    if (Debug) {
        sprintf(temp, 
            "sepSystemBgService class constructor (%d) %s\n", 
            idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API SEP Provision Drive Interface function calls>
    sepSystemBgServiceInterfaceFuncs = 
        "[function call]\n" \
        "[short name]           [FBE API SEP System Bg Service Interface]\n" \
        " ---------------------------------------------------------------------\n" \
        " disableSniffVerify    fbe_api_disable_system_sniff_verify\n" \
        " isSniffVerifyEnabled  fbe_api_system_sniff_verify_is_enabled\n" \
        " enableSniffVerify     fbe_api_enable_system_sniff_verify\n" \
        " ---------------------------------------------------------------------\n";

    // Define common usage for provision drive commands
    usage_format = 
        " Usage: %s \n"
        " For example: %s ";
};

/*********************************************************************
* sepSystemBgService class : Accessor methods
*********************************************************************/

unsigned sepSystemBgService::MyIdNumIs(void)
{
    return idnumber;
};

char * sepSystemBgService::MyIdNameIs(void)
{
    return idname;
};

void sepSystemBgService::dumpme(void)
{ 
    strcpy (key, "sepSystemBgService::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n", 
            idnumber, systemBgServiceCount);
    vWriteFile(key, temp);
}

/*********************************************************************
* sepSystemBgService Class :: select()
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
*      09-April-2012 : initial version
*
*********************************************************************/

fbe_status_t sepSystemBgService::select(int index, int argc, char *argv[]) 
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select SEP interface"); 

    // List SEP Provision Drive calls if help option and less than 6
    // arguments were entered.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) sepSystemBgServiceInterfaceFuncs );

    // disable system sniff verify
    } else if (strcmp (argv[index], "DISABLESNIFFVERIFY") == 0) {
        status = set_system_disable_sniff_verify(argc, &argv[index]);

    // enable system sniff verify
    } else if (strcmp (argv[index], "ENABLESNIFFVERIFY") == 0) {
        status = set_system_enable_sniff_verify(argc, &argv[index]);

    // get if system sniff verify is enabled
    } else if (strcmp (argv[index], "ISSNIFFVERIFYENABLED") == 0) {
        status = get_is_system_sniff_verify_enabled(argc, &argv[index]);

        // can't find match on short name
    } else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n", 
                BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) sepSystemBgServiceInterfaceFuncs);
    } 

    return status;
}

/*********************************************************************
*  sepSystemBgService class :: set_system_disable_sniff_verify (
* int argc, char **argv)
*********************************************************************
*
*  Description:
*      Disable the system sniff verify
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t sepSystemBgService:: set_system_disable_sniff_verify (
    int argc, char **argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "disableSniffVerify",
        " sepSystemBgService:: set_system_disable_sniff_verify",
        "fbe_api_disable_system_sniff_verify",
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    //Initialize params
    params[0] ='\0';

    //make API call
    status = fbe_api_disable_system_sniff_verify();

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to disable system sniff verify");

    }else { 
        sprintf(temp, "disabled system sniff verify successfully");
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
*  sepSystemBgService class :: set_system_enable_sniff_verify
*   (int argc, char **argv)
*********************************************************************
*
*  Description:
*      Enable the system sniff verify
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t sepSystemBgService :: set_system_enable_sniff_verify (
   int argc, char **argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "enableSniffVerify",
        " sepSystemBgService:: set_system_enable_sniff_verify ",
        "fbe_api_enable_system_sniff_verify",
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    //Initialize params
    params[0] ='\0';

    //make API call
    status = fbe_api_enable_system_sniff_verify();

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to enable system sniff verify");

    }else { 
        sprintf(temp, "Enabled system sniff verify successfully");
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;

}

/*********************************************************************
*  sepSystemBgService class :: get_is_system_sniff_verify_enabled (
*   int argc, char **argv)
*********************************************************************
*
*  Description:
*      Check if system sniff verify is enabled
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t sepSystemBgService:: get_is_system_sniff_verify_enabled(
    int argc, char **argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "isSniffVerifyEnabled",
        " sepSystemBgService:: get_is_system_sniff_verify_enabled",
        "fbe_api_system_sniff_verify_is_enabled",
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    params[0] ='\0';

    //make API call
    verify_enabled = fbe_api_system_sniff_verify_is_enabled();

    // Check status of call
    if (verify_enabled == FBE_FALSE) {
        sprintf(temp, "System sniff verify is disabled");

    }else if (verify_enabled == FBE_TRUE) { 
        sprintf(temp, "System sniff verify is enabled");
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;

}
