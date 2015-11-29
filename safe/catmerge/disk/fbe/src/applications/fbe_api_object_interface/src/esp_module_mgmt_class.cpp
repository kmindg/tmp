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
*      This file defines the methods of the ESP MODULE MANAGEMENT 
*      INTERFACE class.
*
*  Notes:
*      The ESP MODULE MGMT class (espModuleMgmt) is a derived class of 
*      the base class (bESP).
*
*  History:
*      1-August-2011 : initial version
*
*********************************************************************/

#ifndef T_ESP_MODULE_MGMT_CLASS_H
#include "esp_module_mgmt_class.h"
#endif

/*********************************************************************
* espModuleMgmt::espModuleMgmt() (Constructor)
*********************************************************************
*  Description:
*       Initializes class variables
*  Input: Parameters
*      none
*  Returns:
*      none
*********************************************************************/
espModuleMgmt::espModuleMgmt() : bESP()
{
    // Store Object number
    idnumber = (unsigned) ESP_MODULE_MGMT_INTERFACE;
    espModuleMgmtCount = ++gEspModuleMgmtCount;

    // Store Object name
    idname = new char [IDNAME_LENGTH];
    strcpy(idname, "ESP_MODULE_MGMT_INTERFACE");

    Esp_Module_Intializing_variable();
    
    if (Debug) {
        sprintf(temp, "espModuleMgmt class constructor (%d) %s\n", 
                idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API ESP MODULE Interface function calls>
    espModuleMgmtInterfaceFuncs = 
        "[function call]\n" \
        "[short name]       [FBE API ESP MODULE Interface]\n" \
        " ------------------------------------------------------------\n" \
        " getIOModuleInfo      fbe_api_esp_module_mgmt_getIOModuleInfo\n" \
        " getModuleStatus      fbe_api_esp_module_mgmt_get_module_status\n" \
        " getIOModulePortInfo  fbe_api_esp_module_mgmt_getIOModulePortInfo\n" \
        " getLimitsInfo        fbe_api_esp_module_mgmt_get_limits_info\n" \
        " getConfigPortInfo    fbe_api_esp_module_mgmt_get_config_mgmt_port_info\n" \
        " getCompInfo          fbe_api_esp_module_mgmt_get_mgmt_comp_info\n" \
        " ------------------------------------------------------------\n"\
        " setPortSpeed         fbe_api_esp_module_mgmt_set_mgmt_port_speed\n" \
        " ------------------------------------------------------------\n";
    
    // Define common usage for ESP MODULE commands
    usage_format = 
        " Usage: %s \n"
        " For example: %s ";
};

 /*********************************************************************
* espModuleMgmt Class :: MyIdNumIs() (Accessor method)
*********************************************************************
*  Description:
*       Returns id number of object
*  Input: Parameters
*      none
*  Returns:
*      unsigned int - id number
*********************************************************************/
unsigned espModuleMgmt::MyIdNumIs(void)
{
    return idnumber;
};

/*********************************************************************
* espModuleMgmt Class :: MyIdNameIs() (Accessor method)
*********************************************************************
*  Description:
*       Returns id name of object
*  Input: Parameters
*      none
*  Returns:
*      char* - id name
*********************************************************************/
char * espModuleMgmt::MyIdNameIs(void)
{
    return idname;
};

/*********************************************************************
* espModuleMgmt Class :: dumpme()
*********************************************************************
*  Description:
*       Dumps id number and esp module mgmt count
*  Input: Parameters
*      none
*  Returns:
*      none
*********************************************************************/
void espModuleMgmt::dumpme(void)
{ 
    strcpy (key, "espModuleMgmt::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n", 
            idnumber, espModuleMgmtCount);
    vWriteFile(key, temp);
}

/*********************************************************************
* espModuleMgmt Class :: select()
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
*       1-August-2011 : initial version
*
*********************************************************************/
fbe_status_t espModuleMgmt::select(int index, int argc, char *argv[]) 
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select ESP MODULE MGMT interface"); 

    // List ESP MODULE Interface calls if help option and 
    // less than 6 arguments were entered.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) espModuleMgmtInterfaceFuncs );

    // get IO Module info
    } else if (strcmp (argv[index], "GETIOMODULEINFO") == 0) {
        status = get_io_module_info(argc, &argv[index]);

    // get Module status
    } else if (strcmp (argv[index], "GETMODULESTATUS") == 0) {
        status = get_module_status(argc, &argv[index]);

    // get IO Module port info
    } else if (strcmp (argv[index], "GETIOMODULEPORTINFO") == 0) {
        status = get_io_module_port_info(argc, &argv[index]);

    // get limits info
    } else if (strcmp (argv[index], "GETLIMITSINFO") == 0) {
        status = get_limits_info(argc, &argv[index]);

    // get config port info
    } else if (strcmp (argv[index], "GETCONFIGPORTINFO") == 0) {
        status = get_config_port_info(argc, &argv[index]);

    // get component info
    } else if (strcmp (argv[index], "GETCOMPINFO") == 0) {
        status = get_comp_info(argc, &argv[index]);

    //set port speed
    } else if (strcmp (argv[index], "SETPORTSPEED") == 0) {
        status = set_port_speed(argc, &argv[index]);

    // can't find match on short name
    } else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n", 
                BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) espModuleMgmtInterfaceFuncs );
    } 

    return status;
}

/*********************************************************************
* espModuleMgmt Class :: get_io_module_info()
*********************************************************************
*
*  Description:
*      Gets the IO module information from the ESP Module Mgmt object.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t espModuleMgmt::get_io_module_info(int argc, char ** argv)
{
    // Define specific usage string  
    usage_format =
        " SP ID : (Local|Peer)\n"
        " Device Type: (IOMODULE|IOANNEX|MODULE|MEZZANINE|MCU)\n"
        " Usage: %s [sp id] [Device Type] [slot]\n"
        " For example: %s (LOCAL|PEER) IOMODULE 0";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getIOModuleInfo",
        "espModuleMgmt::get_io_module_info",
        "fbe_api_esp_module_mgmt_getIOModuleInfo",
        usage_format,
        argc, 9);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    argv++;
    //Get index for the SP. (0-local , 1-peer)
    io_module_info.header.sp = utilityClass::convert_sp_name_to_index(argv);
    if( io_module_info.header.sp >= INVALID_SP)
    {
        sprintf(temp,"Invalid SP Id!!   Valid values are LOCAL|PEER.");
        sprintf(params, "SP Id %d(%s)", io_module_info.header.sp,*argv);
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }

    argv++;
    io_module_info.header.type = utilityClass::convert_string_to_device_type(*argv);
    if( io_module_info.header.type == FBE_DEVICE_TYPE_INVALID)
    {
        sprintf(temp,"Invalid Device Type!!   Valid values are "
        "(IOMODULE|IOANNEX|MODULE|MEZZANINE|MCU).");
        sprintf(params, "SP Id: %d(%s)",
            io_module_info.header.sp,*argv-1);
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }

    argv++;
    io_module_info.header.slot = (fbe_u32_t)strtoul(*argv, 0, 0);

    sprintf(params, "SP Id: %d(%s) Device Type:0x%llx(%s)"
        "  Slot no: 0x%x(%s)",
        io_module_info.header.sp,*(argv-2),
        io_module_info.header.type,*(argv-1),
        io_module_info.header.slot,*argv);

    // Make api call: 
    status = fbe_api_esp_module_mgmt_getIOModuleInfo(
               &io_module_info);

    // Check status of call
    if (status != FBE_STATUS_OK)
    {
        sprintf(temp,"Unable to obtain IO Module Information for Sp Index: "
        "0x%x(%s),Device Type:0x%llx(%s),Slot no:%d(%s)\n",io_module_info.header.sp,*(argv-2),
        io_module_info.header.type,*(argv-1),io_module_info.header.slot,*argv);
    }
    else
    {
        edit_io_module_info(&io_module_info);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* espModuleMgmt Class :: get_module_status()
*********************************************************************
*
*  Description:
*      Gets the IO Module status from the ESP Module Mgmt object.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t espModuleMgmt::get_module_status(int argc, char ** argv)
{
    // Define specific usage string  
    usage_format =
        " SP ID : (Local|Peer)\n"
        " Device Type: (IOMODULE|IOANNEX|MODULE|MEZZANINE|MCU)\n"
        " Usage: %s [sp id] [Device Type] [slot]\n"
        " For example: %s (LOCAL|PEER) IOMODULE 0";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getModuleStatus",
        "espModuleMgmt::get_module_status",
        "fbe_api_esp_module_mgmt_get_module_status",
        usage_format,
        argc, 9);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    argv++;
    //Get index for the SP. (0-local , 1-peer)
    module_status_info.header.sp = utilityClass::convert_sp_name_to_index(argv);

    if( module_status_info.header.sp >= INVALID_SP)
    {
        sprintf(temp,"Invalid SP Id!!   Valid values are LOCAL|PEER.");
        sprintf(params, "SP Id %d(%s)", module_status_info.header.sp,*argv);
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }

    argv++;
    module_status_info.header.type = utilityClass::convert_string_to_device_type(*argv);
    if( module_status_info.header.type == FBE_DEVICE_TYPE_INVALID)
    {
        sprintf(temp,"Invalid Device Type!!   Valid values are "
        "(IOMODULE|IOANNEX|MODULE|MEZZANINE|MCU).");
        sprintf(params, "SP Id: %d(%s)",
            module_status_info.header.sp,*argv-1);
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }

    argv++;
    module_status_info.header.slot = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, "SP Id: %d(%s)  Slot no: 0x%x(%s)",
        module_status_info.header.sp,*(argv-1),
        module_status_info.header.slot,*argv);

    // Make api call: 
    status = fbe_api_esp_module_mgmt_get_module_status(
               &module_status_info);

    // Check status of call
    if (status != FBE_STATUS_OK)
    {
        sprintf(temp,"Unable to obtain module status for Sp Index: "
        "0x%x(%s),Device Type:0x%llx(%s),Slot no:%d(%s)\n",module_status_info.header.sp,*(argv-2),
        module_status_info.header.type,*(argv-1),module_status_info.header.slot,*argv);
    }
    else
    {
        edit_module_status(&module_status_info);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* espModuleMgmt Class :: get_io_module_port_info()
*********************************************************************
*
*  Description:
*      Gets the IO module port information from the ESP Module Mgmt object.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t espModuleMgmt::get_io_module_port_info(int argc, char ** argv)
{
    // Define specific usage string  
    usage_format =
        " SP ID : (Local|Peer)\n"
        " Device Type: (IOMODULE|IOANNEX|MODULE|MEZZANINE|MCU)\n"
        " Usage: %s [sp id] [Device Type] [port] [slot]\n"
        " For example: %s (LOCAL|PEER) IOMODULE 0";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getIOModulePortInfo",
        "espModuleMgmt::get_io_module_port_info",
        "fbe_api_esp_module_mgmt_getIOModulePortInfo",
        usage_format,
        argc, 10);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    argv++;
    //Get index for the SP. (0-local , 1-peer)
    io_port_info.header.sp = utilityClass::convert_sp_name_to_index(argv);

    if( io_port_info.header.sp >= INVALID_SP)
    {
        sprintf(temp,"Invalid SP Id!!   Valid values are LOCAL|PEER.");
        sprintf(params, "SP Id %d(%s)", io_port_info.header.sp,*argv);
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }
    
    argv++;
    io_port_info.header.type = utilityClass::convert_string_to_device_type(*argv);
    if( io_port_info.header.type == FBE_DEVICE_TYPE_INVALID)
    {
        sprintf(temp,"Invalid Device Type!!   Valid values are "
        "(IOMODULE|IOANNEX|MODULE|MEZZANINE|MCU).");
        sprintf(params, "SP Id: %d(%s)",
            io_port_info.header.sp,*(argv-1));
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }

    argv++;
    io_port_info.header.port = (fbe_u32_t)strtoul(*argv, 0, 0);

    argv++;
    io_port_info.header.slot = (fbe_u32_t)strtoul(*argv, 0, 0);

    sprintf(params, "SP Id: %d(%s) Device Type: 0x%llx(%s) Port no: 0x%x(%s)"
        "  Slot no: 0x%x(%s)",
        io_port_info.header.sp,*(argv-3),
        io_port_info.header.type,*(argv-2),
        io_port_info.header.port,*(argv-1),
        io_port_info.header.slot,*argv);

    // Make api call: 
    status = fbe_api_esp_module_mgmt_getIOModulePortInfo(
               &io_port_info);

        // Check status of call
        if (status != FBE_STATUS_OK)
        {
            sprintf(temp,"Unable to obtain io module port information for Sp Index: "
                "0x%x(%s),Device Type: 0x%llx(%s),Port no:%d Slot no:%d\n",io_port_info.header.sp,
                *(argv-3),io_port_info.header.type,*(argv-2),
                io_port_info.header.port,io_port_info.header.slot);
        }

    else
    {
        edit_io_module_port_info(&io_port_info);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* espModuleMgmt Class :: get_limits_info()
*********************************************************************
*
*  Description:
*      Gets the limits information from the ESP Module Mgmt object.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t espModuleMgmt::get_limits_info(int argc, char ** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getLimitsInfo",
        "espModuleMgmt::get_limits_info",
        "fbe_api_esp_module_mgmt_get_limits_info",
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    params[0] = '\0';

    // Make api call: 
    status = fbe_api_esp_module_mgmt_get_limits_info(
               &limits_info);

    // Check status of call
    if (status != FBE_STATUS_OK)
    {
        sprintf(temp,"Unable to obtain limits Information");
    }
    else
    {
        edit_limits_info(&limits_info);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* espModuleMgmt Class :: get_config_port_info()
*********************************************************************
*
*  Description:
*      Gets the port configuration from the ESP Module Mgmt object.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t espModuleMgmt::get_config_port_info(int argc, char ** argv)
{
    // Define specific usage string  
    usage_format =
        " Usage: %s [slot]\n"
        " For example: %s 0\n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getConfigPortinfo",
        "espModuleMgmt::get_config_port_info",
        "fbe_api_esp_module_mgmt_get_config_mgmt_port_info",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    argv++;
    mgmt_port_config.phys_location.slot = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, "Slot no: 0x%x(%s)",
        mgmt_port_config.phys_location.slot,*argv);

    // Make api call: 
    status = fbe_api_esp_module_mgmt_get_config_mgmt_port_info(
               &mgmt_port_config);

    // Check status of call
    if (status != FBE_STATUS_OK)
    {
        sprintf(temp,"Unable to obtain port configuration information for slot "
            "no:%d\n",mgmt_port_config.phys_location.slot);
    }
    else
    {
        edit_config_port_info(&mgmt_port_config);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* espModuleMgmt Class :: get_comp_info()
*********************************************************************
*
*  Description:
*      Gets the module component information from the ESP Module Mgmt object.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t espModuleMgmt::get_comp_info(int argc, char ** argv)
{
    // Define specific usage string  
    usage_format =
        " Usage: %s [sp id] [slot]\n"
        " For example: %s (LOCAL|PEER) 0\n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getCompInfo",
        "espModuleMgmt::get_comp_info",
        "fbe_api_esp_module_mgmt_get_mgmt_comp_info",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    argv++;
    //Get index for the SP. (0-local , 1-peer)
    mgmt_comp_info.phys_location.sp = utilityClass::convert_sp_name_to_index(argv);

    if( mgmt_comp_info.phys_location.sp >= INVALID_SP)
    {
        sprintf(temp,"Invalid SP Id!!   Valid values are LOCAL|PEER.");
        sprintf(params, "SP Id %d(%s)", mgmt_comp_info.phys_location.sp,*argv);
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }

    argv++;
    mgmt_comp_info.phys_location.slot = (fbe_u8_t)strtoul(*argv, 0, 0);
    sprintf(params, "SP Id: %d(%s)  Slot no: 0x%x(%s)",
        mgmt_comp_info.phys_location.sp,*(argv-1),
        mgmt_comp_info.phys_location.slot,*argv);

    // Make api call: 
    status = fbe_api_esp_module_mgmt_get_mgmt_comp_info(
               &mgmt_comp_info);

    // Check status of call
    if (status != FBE_STATUS_OK)
    {
        sprintf(temp,"Unable to obtain component info for Sp Index: "
        "0x%x(%s),Slot no:%d(%s)\n",mgmt_comp_info.phys_location.sp,*(argv-1),
        mgmt_comp_info.phys_location.slot,*argv);
    }
    else
    {
        edit_comp_info(&mgmt_comp_info);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* espModuleMgmt Class :: set_port_speed()
*********************************************************************
*
*  Description:
*      Sets the port speed from the ESP Module Mgmt object.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t espModuleMgmt::set_port_speed(int argc, char ** argv)
{
    // Define specific usage string  
    usage_format =
        " Usage: %s [sp id] [slot] [revert action] "\
        "[port auto neg action] [port speed] [port duplex mode]\n"
        "Sp Id: \n" 
        "   local\n"       
        "   peer\n"           
        "Revert Action\n"                
        "   revert\n"            
        "   norevert\n"            
        "port auto neg action\n"                 
        "   on\n"             
        "   off\n"
        "Port Speed\n"
        "   1000mbps\n"
        "   100mbps\n"
        "   10mbps\n"
        "port duplex mode\n"
        "   half\n"
        "   full\n"
        " For example: %s Local 0 revert on 10 full \n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "setPortSpeed",
        "espModuleMgmt::set_port_speed",
        "fbe_api_esp_module_mgmt_set_mgmt_port_speed",
        usage_format,
        argc, 12);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    argv++;
    //Get index for the SP. (0-local , 1-peer)
    set_mgmt_port_config.phys_location.sp = utilityClass::convert_sp_name_to_index(argv);

    if( set_mgmt_port_config.phys_location.sp >= INVALID_SP)
    {
        sprintf(temp,"Invalid SP Id!!   Valid values are LOCAL|PEER.");
        sprintf(params, "SP Id %d(%s)", set_mgmt_port_config.phys_location.sp,*argv);
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }

    argv++;
    set_mgmt_port_config.phys_location.slot = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, "SP Id: %d(%s)  Slot no: 0x%x(%s)",
        module_status_info.header.sp,*(argv-1),
        module_status_info.header.slot,*argv);

    argv++;
    status = get_module_revert_action(argv);
    if (status != FBE_STATUS_OK) {
        sprintf(params, "SP Id: %d(%s)  Slot no: 0x%x(%s)",
            mgmt_comp_info.phys_location.sp,*(argv-2),
            mgmt_comp_info.phys_location.slot,*(argv-1));
        sprintf(temp,"Invalid Revert action !!   Valid values are "
        "REVERT|NOREVERT.");
        call_post_processing(status, temp, key, params);
        return status;
    }

    argv++;
    status = get_port_auto_neg_action(argv);
    if (status != FBE_STATUS_OK) {
        sprintf(params, "SP Id: %d(%s)  Slot no: 0x%x(%s) Revert Action:%s",
            mgmt_comp_info.phys_location.sp,*(argv-3),
            mgmt_comp_info.phys_location.slot,*(argv-2),*(argv-1));
        sprintf(temp,"Invalid port auto neg action !!   Valid values are "
        "ON|OFF");
        call_post_processing(status, temp, key, params);
        return status;
    }

    argv++;
    status = get_port_speed(argv);
    if (status != FBE_STATUS_OK) {
        sprintf(params, "SP Id: %d(%s)  Slot no: 0x%x(%s)"
            "  Revert Action:%s  Port auto neg action:%s",
            mgmt_comp_info.phys_location.sp,*(argv-4),
            mgmt_comp_info.phys_location.slot,*(argv-3),
            *(argv-2),*(argv-1));
        sprintf(temp,"Invalid port speed !!   Valid values are "
        "1000MBPS|100MBPS|10MBPS");
        call_post_processing(status, temp, key, params);
        return status;
    }

    argv++;
    status = get_port_duplex_mode(argv);
    if (status != FBE_STATUS_OK) {
        sprintf(params, "SP Id: %d(%s)  Slot no: 0x%x(%s)"
            "  Revert Action:%s  Port auto neg action:%s  Port Speed:%s",
            mgmt_comp_info.phys_location.sp,*(argv-5),
            mgmt_comp_info.phys_location.slot,*(argv-4),
            *(argv-3),*(argv-2),*(argv-1));
        sprintf(temp,"Invalid port duplex mode !!   Valid values are "
            "HALF|FULL");
        call_post_processing(status, temp, key, params);
        return status;
    }

    sprintf(params, "SP Id: %d(%s)  Slot no: 0x%x(%s)"
    "  Revert Action:%s  Port auto neg action:%s  Port Speed:%s"
    "Port Duplex Mode:%s",
    mgmt_comp_info.phys_location.sp,*(argv-5),
    mgmt_comp_info.phys_location.slot,*(argv-4),
    *(argv-3),*(argv-2),*(argv-1),*argv);

    // Make api call: 
    status = fbe_api_esp_module_mgmt_set_mgmt_port_config(
               &set_mgmt_port_config);

    // Check status of call
    if (status != FBE_STATUS_OK)
    {
        sprintf(temp,"Unable to set port speed for Sp Id: "
        "0x%x(%s),Slot no:%d\n",set_mgmt_port_config.phys_location.sp,*(argv-5),
        set_mgmt_port_config.phys_location.slot);
    }
    else
    {
        sprintf(temp,"Successfully set port speed to %s for Sp Id: "
        "0x%x(%s),Slot no:%d\n",*(argv-1),set_mgmt_port_config.phys_location.sp,*(argv-5),
        set_mgmt_port_config.phys_location.slot);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}


/*********************************************************************
* espModuleMgmt Class :: edit_io_module_info() (private method)
*********************************************************************
*
*  Description:
*      Displays IO Module information.
*
*  Input: Parameters
*      io_module_info  - IO module information structure

*  Returns:
*      void
*********************************************************************/
void espModuleMgmt::edit_io_module_info(
                     fbe_esp_module_io_module_info_t *io_module_info)
{

    sprintf(temp,  "\n"
        "<SP>                       %s\n"
        "<Slot Number>              %d\n"
        "<Device Type>              %s\n"
        "<Env Interface Status>     %s\n"
        "<Local Fru>                %s\n"
        "<Inserted>                  %s\n"
        "<Power Status>             %s\n"
        "<Unique Id>                %s\n"
        "<IO Port Count>            %d\n"
        "<Fault LED Status>         %s\n"
        "<Location>                 %s\n",
        utilityClass::convert_sp_id_to_string(io_module_info->io_module_phy_info.associatedSp),
        io_module_info->io_module_phy_info.slotNumOnBlade,
        fbe_module_mgmt_device_type_to_string(
        io_module_info->io_module_phy_info.deviceType),
        fbe_module_mgmt_convert_env_interface_status_to_string(
        io_module_info->io_module_phy_info.envInterfaceStatus),
        (io_module_info->io_module_phy_info.isLocalFru) ? "Yes" : "No",
        (io_module_info->io_module_phy_info.inserted) ? "Yes" : "No",
        fbe_module_mgmt_power_status_to_string(
        io_module_info->io_module_phy_info.powerStatus),
        utilityClass::convert_hw_module_type_to_string(
        io_module_info->io_module_phy_info.uniqueID),
        io_module_info->io_module_phy_info.ioPortCount,
        utilityClass::convert_led_blink_rate_to_string(
        io_module_info->io_module_phy_info.faultLEDStatus),
        utilityClass::convert_io_module_location_to_string(
        io_module_info->io_module_phy_info.location));

    return;
}

/*********************************************************************
* espModuleMgmt Class :: edit_io_module_port_info() (private method)
*********************************************************************
*
*  Description:
*      Displays IO Module Port information.
*
*  Input: Parameters
*      io_port_info  - IO port information structure

*  Returns:
*      void
*********************************************************************/
void espModuleMgmt::edit_io_module_port_info(
                     fbe_esp_module_io_port_info_t *io_port_info)
{

    sprintf(temp,  "\n"
        "<Physical Information>\n"
        "<SP>                       %s\n"
        "<Slot Number>              %d\n"
        "<Port Num on Module>       %d\n"
        "<Local Fru>                %s\n"
        "<Present>                  %s\n"
        "<Power Status>             %s\n"
        "<Protocol>                 %s\n"
        "<SFP capable>              %s\n"
        "<Supported Speeds>         %s\n"
        "<PCI Data Bus>             0x%x\n"
        "<PCI Data Device>          0x%x\n"
        "<PCI Data Function>        0x%x\n"
        "<Device Type>              %s\n"
        "<IO Port LED>              %s\n"
        "<IO Port LED Color>        %s\n\n"
        "<Logical Information>\n"
        "<Port State>               %s\n"
        "<Port Substate>            %s\n"
        "<Logical Number>           %d\n"
        "<Port Role>                %s\n"
        "<Port Subrole>             %s\n",
        utilityClass::convert_sp_id_to_string(io_port_info->io_port_info.associatedSp),
        io_port_info->io_port_info.slotNumOnBlade,
        io_port_info->io_port_info.portNumOnModule,
        (io_port_info->io_port_info.isLocalFru) ? "Yes" : "No",
        fbe_module_mgmt_mgmt_status_to_string(
        io_port_info->io_port_info.present),
        fbe_module_mgmt_power_status_to_string(
        io_port_info->io_port_info.powerStatus),
        fbe_module_mgmt_protocol_to_string(
        io_port_info->io_port_info.protocol),
        fbe_module_mgmt_mgmt_status_to_string(
        io_port_info->io_port_info.SFPcapable),
        fbe_module_mgmt_supported_speed_to_string(
        io_port_info->io_port_info.supportedSpeeds),
        io_port_info->io_port_info.pciData.bus,
        io_port_info->io_port_info.pciData.device,
        io_port_info->io_port_info.pciData.function,
        fbe_module_mgmt_device_type_to_string(
        io_port_info->io_port_info.deviceType),
        utilityClass::convert_led_blink_rate_to_string(
        io_port_info->io_port_info.ioPortLED),
        utilityClass::convert_led_color_type_to_string(
        io_port_info->io_port_info.ioPortLEDColor),
        fbe_module_mgmt_port_state_to_string(
        io_port_info->io_port_logical_info.port_state),
        fbe_module_mgmt_port_substate_to_string(
        io_port_info->io_port_logical_info.port_substate),
        io_port_info->io_port_logical_info.logical_number,
        fbe_module_mgmt_port_role_to_string(
        io_port_info->io_port_logical_info.port_role),
        fbe_module_mgmt_port_subrole_to_string(
        io_port_info->io_port_logical_info.port_subrole));
    return;
}

/*********************************************************************
* espModuleMgmt Class :: edit_limits_info() (private method)
*********************************************************************
*
*  Description:
*      Displays Limits information.
*
*  Input: Parameters
*      limits_info  - limits information structure

*  Returns:
*      void
*********************************************************************/
void espModuleMgmt::edit_limits_info(fbe_esp_module_limits_info_t *limits_info)
{

    sprintf(temp,  "\n"
        "<Max slics>                0x%x\n"
        "<Max mezzanines>           0x%x\n\n"
        "<Num slic slots>           0x%x\n"
        "<Num mezzanine slots>      0x%x\n"
        "<Num ports>                0x%x\n"
        "<Num Back End Modules>     0x%x\n"
        "<Num mgmt modules>         0x%x\n\n"
        "<Max SAS BE>               0x%x\n"
        "<Max SAS FE>               0x%x\n"
        "<Max FC FE>                0x%x\n"
        "<Max iSCSI 1G FE>          0x%x\n"
        "<Max iSCSI 10G FE>         0x%x\n"
        "<Max FCOE FE>              0x%x\n",
        limits_info->platform_hw_limit.max_slics,
        limits_info->platform_hw_limit.max_mezzanines,
        limits_info->discovered_hw_limit.num_slic_slots,
        limits_info->discovered_hw_limit.num_mezzanine_slots,
        limits_info->discovered_hw_limit.num_ports,
        limits_info->discovered_hw_limit.num_bem,
        limits_info->discovered_hw_limit.num_mgmt_modules,
        limits_info->platform_port_limit.max_sas_be,
        limits_info->platform_port_limit.max_sas_fe,
        limits_info->platform_port_limit.max_fc_fe,
        limits_info->platform_port_limit.max_iscsi_1g_fe,
        limits_info->platform_port_limit.max_iscsi_10g_fe,
        limits_info->platform_port_limit.max_fcoe_fe);

    return;
}

/*********************************************************************
* espModuleMgmt Class :: edit_config_port_info() (private method)
*********************************************************************
*
*  Description:
*      Displays Port Configuration Information.
*
*  Input: Parameters
*      mgmt_port_config  - config port information structure

*  Returns:
*      void
*********************************************************************/
void espModuleMgmt::edit_config_port_info(
                     fbe_esp_module_mgmt_get_mgmt_port_config_t *mgmt_port_config)
{

    sprintf(temp,  "\n"
        "<Port Autonegation>        %s\n"
        "<Port Speed>               %s\n"
        "<Port Duplex Mode>         %s\n",
        fbe_module_mgmt_convert_mgmtPortAutoNeg_to_string(
        mgmt_port_config->mgmtPortConfig.mgmtPortAutoNeg),
        fbe_module_mgmt_convert_externalMgmtPortSpeed_to_string(
        mgmt_port_config->mgmtPortConfig.mgmtPortSpeed),
        fbe_module_mgmt_convert_externalMgmtPortDuplexMode_to_string(
        mgmt_port_config->mgmtPortConfig.mgmtPortDuplex));
    
    return;
}

/*********************************************************************
* espModuleMgmt Class :: edit_comp_info() (private method)
*********************************************************************
*
*  Description:
*      Displays component information.
*
*  Input: Parameters
*      mgmt_comp_info  - comp information structure

*  Returns:
*      void
*********************************************************************/
void espModuleMgmt::edit_comp_info(
                      fbe_esp_module_mgmt_get_mgmt_comp_info_t *mgmt_comp_info)
{

    sprintf(temp,  "\n"
        "<SP>                               %s\n"
        "<Slot Number>                      %d\n"
        "<Env Interface Status>             %s\n"
        "<Local Fru>                        %s\n"
        "<Unique Id>                        %s\n"
        "<Inserted>                         %s\n"
        "<General Fault>                    %s\n\n"
        "<Fault Info>                       0x%x\n"
        "<Fault LED Status>                 %s\n"
        "<VLan Config mode>                 %s\n"
        "<Management Port Link Status>      %s\n"
        "<Management Port Speed>            %s\n"
        "<Management Port Duplex>           %s\n"
        "<Management Port Auto Negotiate>   %s\n"
        "<Service Port Link Status>         %s\n"
        "<Service Port Speed>               %s\n"
        "<Service Port Duplex>              %s\n"
        "<Service Port Auto Negotiate>      %s\n"
        "<Service Port VLan Mapping>        %d\n"
        "<Firmware Rev Major>              %x\n"
        "<Firmware Rev Minor>              %x\n",
        utilityClass::convert_sp_id_to_string(
        	mgmt_comp_info->mgmt_module_comp_info.associatedSp),
        mgmt_comp_info->mgmt_module_comp_info.mgmtID,
        fbe_module_mgmt_convert_env_interface_status_to_string(
        	mgmt_comp_info->mgmt_module_comp_info.envInterfaceStatus),
        mgmt_comp_info->mgmt_module_comp_info.isLocalFru ? "Yes" : "No",
        utilityClass::convert_hw_module_type_to_string(
        	mgmt_comp_info->mgmt_module_comp_info.uniqueID),
        mgmt_comp_info->mgmt_module_comp_info.bInserted ? "Yes" : "No",
        fbe_module_mgmt_convert_general_fault_to_string(
        	mgmt_comp_info->mgmt_module_comp_info.generalFault),
        mgmt_comp_info->mgmt_module_comp_info.faultInfoStatus,
        utilityClass::convert_led_blink_rate_to_string(
        	mgmt_comp_info->mgmt_module_comp_info.faultLEDStatus),
        utilityClass::convert_lan_config_mode_to_string(
        	mgmt_comp_info->mgmt_module_comp_info.vLanConfigMode),
        mgmt_comp_info->mgmt_module_comp_info.managementPort.linkStatus ? "UP" 
        : "DOWN",
        utilityClass::convert_mgmt_port_speed_to_string(
        	mgmt_comp_info->mgmt_module_comp_info.managementPort.mgmtPortSpeed),
        mgmt_comp_info->mgmt_module_comp_info.managementPort.mgmtPortDuplex ? 
        "Half Duplex" : "Full Duplex",
        mgmt_comp_info->mgmt_module_comp_info.managementPort.mgmtPortAutoNeg ? 
        "ON" : "OFF",
        mgmt_comp_info->mgmt_module_comp_info.servicePort.linkStatus ? "Yes" 
        : "No",
        utilityClass::convert_lan_port_speed_to_string(
        mgmt_comp_info->mgmt_module_comp_info.servicePort.portSpeed),
        mgmt_comp_info->mgmt_module_comp_info.servicePort.portDuplex ? 
        "Half Duplex" : "Full Duplex",
        mgmt_comp_info->mgmt_module_comp_info.servicePort.autoNegotiate ? "Yes" 
        : "No",
        mgmt_comp_info->mgmt_module_comp_info.servicePort.vLanMapping,
        mgmt_comp_info->mgmt_module_comp_info.firmwareRevMajor,
        mgmt_comp_info->mgmt_module_comp_info.firmwareRevMinor);

    return;
}

/*********************************************************************
* espModuleMgmt Class :: edit_module_status() (private method)
*********************************************************************
*
*  Description:
*      Displays module status information.
*
*  Input: Parameters
*      module_status_info  - module status information structure

*  Returns:
*      void
*********************************************************************/
void espModuleMgmt::edit_module_status(
                     fbe_esp_module_mgmt_module_status_t  *module_status_info)
{

    sprintf(temp,  "\n"
        "<Module State>        %s\n"
        "<Module Substate>     %s\n"
        "<Module Slic Type>    %s\n",
        fbe_module_mgmt_module_state_to_string(
        module_status_info->state),
        fbe_module_mgmt_module_substate_to_string(
        module_status_info->substate),
        fbe_module_mgmt_slic_type_to_string(
        module_status_info->type));
    
    return;
}


/*********************************************************************
* espModuleMgmt Class:: get_module_revert_action()
*********************************************************************
*
*  Description:
*      Gets the module revert action.
*
*  Input: Parameters
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t espModuleMgmt::get_module_revert_action(char **argv) 
{
    if(strcmp(*argv, "REVERT") == 0) {
        set_mgmt_port_config.revert = FBE_TRUE;
    } else if(strcmp(*argv, "NOREVERT") == 0){
        set_mgmt_port_config.revert = FBE_FALSE;
    } else {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
* espModuleMgmt Class :: get_port_auto_neg_action()
*********************************************************************
*
*  Description:
*      Gets the port auto neg action.
*
*  Input: Parameters
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t espModuleMgmt::get_port_auto_neg_action(char **argv) 
{
    if(strcmp(*argv, "ON") == 0) {
        set_mgmt_port_config.mgmtPortRequest.mgmtPortAutoNeg= FBE_PORT_AUTO_NEG_ON;
    } else if(strcmp(*argv, "OFF") == 0){
        set_mgmt_port_config.mgmtPortRequest.mgmtPortAutoNeg = FBE_PORT_AUTO_NEG_OFF;
    } else {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
* espModuleMgmt Class :: get_port_speed()
*********************************************************************
*
*  Description:
*      Gets the port speed.
*
*  Input: Parameters
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t espModuleMgmt::get_port_speed(char **argv) 
{
    if(strcmp(*argv, "1000MBPS") == 0) {
        set_mgmt_port_config.mgmtPortRequest.mgmtPortSpeed= FBE_MGMT_PORT_SPEED_1000MBPS;
    } else if(strcmp(*argv, "100MBPS") == 0){
        set_mgmt_port_config.mgmtPortRequest.mgmtPortSpeed = FBE_MGMT_PORT_SPEED_100MBPS;
    } else if(strcmp(*argv, "10MBPS") == 0){
        set_mgmt_port_config.mgmtPortRequest.mgmtPortSpeed = FBE_MGMT_PORT_SPEED_10MBPS;
    } else {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
* espModuleMgmt Class :: get_port_duplex_mode()
*********************************************************************
*
*  Description:
*      Gets the port duplex mode.
*
*  Input: Parameters
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t espModuleMgmt::get_port_duplex_mode(char **argv) 
{
    if(strcmp(*argv, "HALF") == 0) {
        set_mgmt_port_config.mgmtPortRequest.mgmtPortDuplex = FBE_PORT_DUPLEX_MODE_HALF;
    } else if(strcmp(*argv, "FULL") == 0){
        set_mgmt_port_config.mgmtPortRequest.mgmtPortDuplex = FBE_PORT_DUPLEX_MODE_FULL;
    } else {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
* espModuleMgmt::Esp_Module_Intializing_variable (private method)
*********************************************************************/
void espModuleMgmt::Esp_Module_Intializing_variable()
{

    sp = 0;
    type = FBE_DEVICE_TYPE_INVALID;
    slot = 0;
    fbe_zero_memory(&io_module_info,sizeof(io_module_info));
    fbe_zero_memory(&module_status_info,sizeof(module_status_info)); 
    fbe_zero_memory(&io_port_info,sizeof(io_port_info));
    fbe_zero_memory(&limits_info,sizeof(limits_info)); 
    fbe_zero_memory(&mgmt_port_config,sizeof(mgmt_port_config));
    fbe_zero_memory(&mgmt_comp_info,sizeof(mgmt_comp_info)); 
    fbe_zero_memory(&set_mgmt_port_config,sizeof(set_mgmt_port_config));

    return;
}

/*********************************************************************
 * espModuleMgmt end of file
 *********************************************************************/
