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
*      This file defines the methods of the ESP POWER SUPPLY MGMT 
*      INTERFACE class.
*
*  Notes:
*      The ESP POWER SUPPLY MGMT class (espPsMgmt) is a derived class of 
*      the base class (bESP).
*
*  History:
*      29-July-2011 : initial version
*
*********************************************************************/

#ifndef T_ESP_PS_MGMT_CLASS_H
#include "esp_ps_mgmt_class.h"
#endif

/*********************************************************************
* espPsMgmt class :: Constructor
*********************************************************************/

espPsMgmt::espPsMgmt() : bESP()
{
    // Store Object number
    idnumber = (unsigned) ESP_PS_MGMT_INTERFACE;
    espPsMgmtCount = ++gEspPsMgmtCount;

    // Store Object name
    idname = new char [IDNAME_LENGTH];
    strcpy(idname, "ESP_PS_MGMT_INTERFACE");
    
    // Initialize member variables
    initialize();

    if (Debug) {
        sprintf(temp, "espPsMgmt class constructor (%d) %s\n", 
                idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API ESP POWER SUPPLY MGMT Interface function calls>
    espPsMgmtInterfaceFuncs = 
        "[function call]\n" \
        "[short name]       [FBE API ESP POWER SUPPLY MGMT Interface]\n" \
        " ------------------------------------------------------------\n" \
        " getPsCount       fbe_api_esp_ps_mgmt_getPsCount\n" \
        " powerDownPS      fbe_api_esp_ps_mgmt_powerdown\n" \
        " getPsInfo        fbe_api_esp_ps_mgmt_getPsInfo\n" \
        " ------------------------------------------------------------\n";
    
    // Define common usage for ESP POWER SUPPLY MGMT commands
    usage_format = 
        " Usage: %s\n"
        " For example: %s";
};

/*********************************************************************
* espPsMgmt class : Accessor methods
*********************************************************************/

unsigned espPsMgmt::MyIdNumIs(void)
{
    return idnumber;
};

char * espPsMgmt::MyIdNameIs(void)
{
    return idname;
};

void espPsMgmt::dumpme(void)
{ 
    strcpy (key, "espPsMgmt::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n", 
            idnumber, espPsMgmtCount);
    vWriteFile(key, temp);
}

/*********************************************************************
* espPsMgmt Class :: select()
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
*      28-Jun-2011 : initial version
*
*********************************************************************/

fbe_status_t espPsMgmt::select(int index, int argc, char *argv[]) 
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select ESP interface"); 

    // List ESP POWER SUPPLY MGMT Interface calls if help option and 
    // less than 6 arguments were entered.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) espPsMgmtInterfaceFuncs );

        // get power supply count from POWER SUPPLY management object
    } else if (strcmp (argv[index], "GETPSCOUNT") == 0) {
        status = get_ps_count(argc, &argv[index]);
        
        // power down power supply
    } else if (strcmp (argv[index], "POWERDOWNPS") == 0) {
        status = power_down_ps(argc, &argv[index]);
        
        // power down power supply
    } else if (strcmp (argv[index], "GETPSINFO") == 0) {
        status = get_ps_info(argc, &argv[index]);

        // can't find match on short name
    } else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n", 
                BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) espPsMgmtInterfaceFuncs );
    } 

    return status;
}

/*********************************************************************
* espPsMgmt Class :: get_ps_count()
*********************************************************************
*
*  Description:
*      Gets power supply count from power supply management object
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t espPsMgmt::get_ps_count(int argc, char ** argv)
{
    // Define specific usage string  
    usage_format =
        " Usage: %s [bus] [enclosure]\n"
        " For example: %s 1 2";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getPsCount",
        "espPsMgmt::get_ps_count",
        "fbe_api_esp_ps_mgmt_getPsCount",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert bus, enclosure to hex.
    argv++;
    location.bus = (fbe_u32_t)strtoul(*argv, 0, 0);
    
    argv++;
    location.enclosure = (fbe_u32_t)strtoul(*argv, 0, 0);
    
    sprintf(params, " bus (%d) enclosure (%d)", 
        location.bus, location.enclosure);
    
    // Make api call: 
    status = fbe_api_esp_ps_mgmt_getPsCount(&location, &PsCount);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get the power supply count for "
                      "bus (%d) enclosure (%d)",
                      location.bus, 
                      location.enclosure);
    } else {
        edit_ps_count(PsCount); 
    }
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* espPsMgmt Class :: power_down_ps()
*********************************************************************
*
*  Description:
*      Power down power supply
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t espPsMgmt::power_down_ps(int argc, char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "powerDownPS",
        "espPsMgmt::power_down_ps",
        "fbe_api_esp_ps_mgmt_powerdown",
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Make api call: 
    status = fbe_api_esp_ps_mgmt_powerdown();

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't power down power supply");
    } else {
        sprintf(temp, "Power supply has been successfully powered down");
    }
    
    params[0] = '\0';

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* espPsMgmt Class :: get_ps_info()
*********************************************************************
*
*  Description:
*      Gets power supply info
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t espPsMgmt::get_ps_info(int argc, char ** argv)
{
    // Define specific usage string  
    usage_format =
        " Usage: %s [bus] [enclosure] [power slot]\n"
        " For example: %s 0 0 1";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getPsInfo",
        "espPsMgmt::get_ps_info",
        "fbe_api_esp_ps_mgmt_getPsInfo",
        usage_format,
        argc, 9);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    fbe_set_memory(&psInfo, 0, sizeof(fbe_esp_ps_mgmt_ps_info_t));
    
    // Convert bus, enclosure, power slot to hex.
    argv++;
    psInfo.location.bus = (fbe_u32_t)strtoul(*argv, 0, 0);
    
    argv++;
    psInfo.location.enclosure = (fbe_u32_t)strtoul(*argv, 0, 0);
    
    argv++;
    psInfo.location.slot = (fbe_u32_t)strtoul(*argv, 0, 0);

    sprintf(params, " bus (%d) enclosure (%d) power slot (%d)", 
        psInfo.location.bus, 
        psInfo.location.enclosure, 
        psInfo.location.slot);
    
    // Make api call: 
    status = fbe_api_esp_ps_mgmt_getPsInfo(&psInfo);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get power supply info for "
                      "bus (%d) enclosure (%d) power slot (%d)",
                      psInfo.location.bus, 
                      psInfo.location.enclosure,
                      psInfo.location.slot);
    } else {
        edit_ps_info(&psInfo); 
    }
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* espPsMgmt Class :: edit_ps_count() (private method)
*********************************************************************
*
*  Description:
*      Format the power supply count to display
*
*  Input: Parameters
*      PsCount  - power supply count

*  Returns:
*      void
*********************************************************************/

void espPsMgmt::edit_ps_count(fbe_u32_t PsCount)
{
    // power supply count
    sprintf(temp,         "\n "
        "<Ps Count>       0x%X\n ",
        PsCount);
}

/*********************************************************************
* espPsMgmt Class :: edit_ps_info() (private method)
*********************************************************************
*
*  Description:
*      Format the power supply info to display
*
*  Input: Parameters
*      psInfo  - power supply info structure

*  Returns:
*      void
*********************************************************************/

void espPsMgmt::edit_ps_info(fbe_esp_ps_mgmt_ps_info_t *psInfo)
{
    char temp_local[5000];

    // power supply info
    sprintf(temp,                           "\n "
        "<Location>                          \n"
        "<Is processor Enclosure>           %s\n "
        "<Bus>                              %d\n "
        "<Enclosure>                        %d\n "
        "<Slot>                             %d\n "
        "<SP>                               %d\n "
        "<Port>                             %d\n "
        "<SP ID>                            %d (%s)\n "
        "<Slot Number On Enclosure>         0x%X\n "
        "<Env Interface Status>             %d (%s)\n "
        "<In Processor Encl>                %s\n "
        "<Hw Module Type>                   %d (%s)\n "
        "<Is Local Fru>                     %s\n "
        "<Associated Sps>                   %d (%s)\n "
        "<bInserted>                        %s\n "
        "<General Fault>                    %d (%s)\n ",
       psInfo->location.processorEnclosure? "Yes" : "No",
        psInfo->location.bus,
        psInfo->location.enclosure,
        psInfo->location.slot,
        psInfo->location.sp,
        psInfo->location.port,
        
        psInfo->psInfo.associatedSp, 
        utilityClass::convert_sp_id_to_string(psInfo->psInfo.associatedSp),
        
        psInfo->psInfo.slotNumOnEncl,
        
        psInfo->psInfo.envInterfaceStatus, 
        fbe_module_mgmt_convert_env_interface_status_to_string(
        psInfo->psInfo.envInterfaceStatus),
        
        psInfo->psInfo.inProcessorEncl? "Yes" : "No",
        
        psInfo->psInfo.uniqueId,
        utilityClass::convert_hw_module_type_to_string(psInfo->psInfo.uniqueId),
        
        psInfo->psInfo.isLocalFru? "Yes" : "No",

        psInfo->psInfo.associatedSps,
        utilityClass::convert_sps_id_to_string(psInfo->psInfo.associatedSps),

        psInfo->psInfo.bInserted? "Yes" : "No",
        
        psInfo->psInfo.generalFault,
        fbe_module_mgmt_mgmt_status_to_string(psInfo->psInfo.generalFault) );

    
    sprintf(temp_local, 
        "<Internal Fan Fault>             %d (%s)\n ",
        psInfo->psInfo.internalFanFault,
        fbe_module_mgmt_convert_general_fault_to_string(psInfo->psInfo.internalFanFault) );

    strcat(temp, temp_local);
    
    sprintf(temp_local,          
        "<Ambient Overtemp Fault>           %d (%s)\n "
        "<DC Present>                       %d (%s)\n "
        "<AC Fail>                          %d (%s)\n "
        "<AC DC Input>                      %d (%s)\n "
        "<Firmware Rev Valid>               0x%X\n "
        "<bDownloadable>                    %s\n "
        "<Firmware Rev>                     %s\n "
        "<Sub Encl ProductId>               %s\n "
        "<Input Power Status>               0x%X\n "
        "<Input Power>                      0x%X\n "
        "<Ps Input Power>                   0x%X\n ",
        
        psInfo->psInfo.ambientOvertempFault,
        fbe_module_mgmt_convert_general_fault_to_string(psInfo->psInfo.ambientOvertempFault),

        psInfo->psInfo.DCPresent,
        fbe_module_mgmt_convert_general_fault_to_string(psInfo->psInfo.DCPresent),

        psInfo->psInfo.ACFail,
        fbe_module_mgmt_convert_general_fault_to_string(psInfo->psInfo.ACFail),

        psInfo->psInfo.ACDCInput,
        utilityClass::convert_ac_dc_input_to_string(psInfo->psInfo.ACDCInput),

        psInfo->psInfo.firmwareRevValid,
        psInfo->psInfo.bDownloadable? "Yes" : "No",
        psInfo->psInfo.firmwareRev,
        psInfo->psInfo.subenclProductId,
        psInfo->psInfo.inputPowerStatus,
        psInfo->psInfo.inputPower,
        psInfo->psInfo.psInputPower);
        
        strcat(temp, temp_local);
}

/*********************************************************************
* espPsMgmt Class :: initialize()
*********************************************************************
*
*  Description:
*      Initializes the required member variables
*
*  Input: Parameters - None
*
*  Returns:
*       void
*********************************************************************/
void espPsMgmt::initialize()
{
    // initialize the drive info structure
    
    location.processorEnclosure = 0;
    location.bus = 0;
    location.enclosure = 0;
    location.slot = 0;
    location.sp = 0;
    location.port = 0;

    psInfo.location.processorEnclosure = 0;
    psInfo.location.bus = 0;
    psInfo.location.enclosure = 0;
    psInfo.location.slot = 0;
    psInfo.location.sp = 0;
    psInfo.location.port = 0;


    PsCount = 0;

}

/*********************************************************************
* espPsMgmt end of file
*********************************************************************/
