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
*      This file defines the methods of the ESP COOLING MGMT class.
*
*  Notes:
*      The ESP COOLING MGMT class (espCoolingMgmt) is a derived class of 
*      the base class (bESP).
*
*  History:
*      05-Aug-2011 : initial version
*
*********************************************************************/
#ifndef T_ESP_COOLING_MGMT_CLASS_H
#include "esp_cooling_mgmt_class.h"
#endif


/*********************************************************************
* espCoolingMgmt class :: Constructor
*********************************************************************/
espCoolingMgmt::espCoolingMgmt() : bESP()
{
    // Store Object number
    idnumber = (unsigned) ESP_COOLING_MGMT_INTERFACE;
    espCoolingMgmtCount = ++gEspCoolingMgmtCount;

    // Store Object name
    idname = new char [IDNAME_LENGTH];
    strcpy(idname, "ESP_COOLING_MGMT_INTERFACE");

    fbe_zero_memory(&fan_info,sizeof(fan_info));
    fbe_zero_memory(&location,sizeof(location));
    env_status_str[0] = '\0';
    
    
    if (Debug) {
        sprintf(temp, "espCoolingMgmt class constructor (%d) %s\n", 
                idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API ESP COOLING MGMT Interface function calls>
    espCoolingMgmtInterfaceFuncs = 
        "[function call]\n" \
        "[short name]     [FBE API ESP COOLING MGMT Interface]\n" \
        " ----------------------------------------------------------\n" \
        " getFanInfo      fbe_api_esp_cooling_mgmt_get_fan_info\n" \
        " -----------------------------------------------------------\n";
    
    // Define common usage for ESP COOLING MGMT commands
    usage_format = 
        " Usage: %s [bus] [enclosure] [slot]\n"
        " For example: %s 0 0 1";
};

/*********************************************************************
* espCoolingMgmt class : Accessor methods
*********************************************************************/

unsigned espCoolingMgmt::MyIdNumIs(void)
{
    return idnumber;
};

char * espCoolingMgmt::MyIdNameIs(void)
{
    return idname;
};
void espCoolingMgmt::dumpme(void)
{ 
    strcpy (key, "espCoolingMgmt::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n", 
            idnumber, espCoolingMgmtCount);
    vWriteFile(key, temp);
};

/*********************************************************************
* espCoolingMgmt Class :: select(int index, int argc, char *argv[])
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
*      05-Aug-2011 : inital version
*
*********************************************************************/

fbe_status_t espCoolingMgmt::select(int index, int argc, char *argv[]) 
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select ESP interface"); 

    // List ESP COOLING MGMT Interface calls if help option and 
    // less than 6 arguments were entered.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) espCoolingMgmtInterfaceFuncs );

    // get cooling mgmt info
    } else if (strcmp (argv[index], "GETFANINFO") == 0) {
        status = get_esp_fan_info(argc, &argv[index]);

    // can't find match on short name
    } else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n", 
                BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) espCoolingMgmtInterfaceFuncs );
    } 

    return status;
}

/*********************************************************************
* espCoolingMgmt Class :: get_esp_fan_info(int argc, char ** argv)
*********************************************************************
*
*  Description:
*      Get the fan info from the ESP cooling mgmt object.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

 fbe_status_t espCoolingMgmt::get_esp_fan_info(int argc, char ** argv)
{
    fbe_status_t bus_status= FBE_STATUS_OK, encl_status = FBE_STATUS_OK, slot_status = FBE_STATUS_OK;
    char  tempStr[100]={0};

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getFanInfo",
        "espCoolingMgmt::get_esp_fan_info",
        "fbe_api_esp_cooling_mgmt_get_fan_info",
        usage_format,
        argc, 9);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    argv++;
    bus_status = get_and_verify_bus_number(argv,&location);
    argv++;
    encl_status = get_and_verify_enclosure_number(argv, &location);
    argv++;
    slot_status = get_and_verify_slot_number(argv, &location);

    sprintf(params, "Bus(%d)  enclosure(%d)  slot(%d)", 
        location.bus, 
        location.enclosure,
        location.slot);

    if((bus_status !=FBE_STATUS_OK) || (encl_status != FBE_STATUS_OK) 
        || (slot_status != FBE_STATUS_OK)){
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }

    // Make api call: 
    status = fbe_api_esp_cooling_mgmt_get_fan_info(&location, &fan_info);
    
    if(status != FBE_STATUS_OK) {
        sprintf(temp,"Failed to retrive fan information.\n");
    }
    else {
        if (fan_info.associatedSp == SP_A || fan_info.associatedSp == SP_B)
        {
            sprintf(tempStr, "<Slot On Sp Blade>            %d", fan_info.slotNumOnSpBlade);
        }
        else
        {
            sprintf(tempStr, "<Slot On Sp Blade>            N/A");
        }
        sprintf(temp, "\n"
            "<InProcessorEnclosure>        %s\n"
            "<Associated SP>               %s\n"
            "%s\n"
            "<Fan inserted>                %s\n"
            "<Fan Faulted>                 %s\n"
            "<Fan Degraded>                %s\n"
            "<Multi Fan Module Faulted>    %s\n"
            "<Env interface status>        %s\n",
            (fan_info.inProcessorEncl == FBE_TRUE ? "Yes" : "No"),
            (fan_info.associatedSp == SP_A)? "SP_A" : ((fan_info.associatedSp == SP_B)?"SP_B":"N/A"),
            tempStr,
            fbe_module_mgmt_mgmt_status_to_string(fan_info.inserted),
            fbe_module_mgmt_mgmt_status_to_string( fan_info.fanFaulted),
            fbe_module_mgmt_mgmt_status_to_string( fan_info.fanDegraded),
            fbe_module_mgmt_mgmt_status_to_string(fan_info.multiFanModuleFaulted),
            fbe_module_mgmt_convert_env_interface_status_to_string(
                fan_info.envInterfaceStatus));
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}


/*********************************************************************
* espCoolingMgmt Class :: get_and_verify_bus_number(
*     char ** argv,fbe_device_physical_location_t*  location_p )
*********************************************************************
*
*  Description:
*      get the bus number from command line and verify it.
*
*  Input: Parameters
*      *argv - pointer to arguments
*      *location_p - pointer to the physical location structure.
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t espCoolingMgmt::get_and_verify_bus_number(
    char ** argv,
    fbe_device_physical_location_t*  location_p )
{
    location_p->bus = (fbe_u8_t)atoi(*argv);
    
    if(location_p->bus >= FBE_PHYSICAL_BUS_COUNT && location_p->bus != FBE_XPE_PSEUDO_BUS_NUM){
        sprintf(temp,"Entered Bus number (%d) exceeds the total Bus"
            " Count (%d)\n",
            location_p->bus,
            FBE_PHYSICAL_BUS_COUNT);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
* espCoolingMgmt Class :: get_and_verify_enclosure_number(
*     char ** argv,fbe_device_physical_location_t*  location_p )
*********************************************************************
*
*  Description:
*      get the enclosure number from command line and verify it.
*
*  Input: Parameters
*      *argv - pointer to arguments
*      *location_p - pointer to the physical location structure.
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t espCoolingMgmt::get_and_verify_enclosure_number(
    char ** argv,
    fbe_device_physical_location_t*  location_p )
{
    fbe_u32_t enclCount;
    location_p->enclosure = (fbe_u8_t)atoi(*argv);

    //Get  the total enclosure count 
   status =fbe_api_esp_encl_mgmt_get_encl_count_on_bus(location_p,&enclCount);

    if(location_p->enclosure >= enclCount && location_p->enclosure != FBE_XPE_PSEUDO_ENCL_NUM ){
        sprintf(temp,"Enclosure number (%d) must be less than the "
            " enclosure count (%d) on the bus.\n",
            location_p->enclosure, 
            enclCount);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
* espCoolingMgmt Class :: get_and_verify_slot_number(
*     char ** argv,fbe_device_physical_location_t*  location_p )
*********************************************************************
*
*  Description:
*      get the slot number from command line and verify it.
*
*  Input: Parameters
*      *argv       - pointer to arguments
*      *location_p - pointer to the physical location structure.
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t espCoolingMgmt::get_and_verify_slot_number(
    char ** argv,
    fbe_device_physical_location_t*  location_p )
{
    fbe_esp_encl_mgmt_encl_info_t            enclInfo = {0};

    location_p->slot = (fbe_u8_t)atoi(*argv);

   status = fbe_api_esp_encl_mgmt_get_encl_info(location_p, &enclInfo);
    if(location_p->slot>= enclInfo.fanCount) {
        sprintf(temp,"Bus slot number %d must be less than the FAN device"
            " count (%d) on the array.\n",
            location_p->slot, 
            enclInfo.fanCount);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
