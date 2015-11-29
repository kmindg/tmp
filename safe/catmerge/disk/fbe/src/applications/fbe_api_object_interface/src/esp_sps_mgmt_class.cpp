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
*      This file defines the methods of the ESP SPS MANAGEMENT 
*      INTERFACE class.
*
*  Notes:
*      The ESP SPS MGMT class (espSpsMgmt) is a derived class of 
*      the base class (bESP).
*
*  History:
*      15-July-2011 : initial version
*
*********************************************************************/

#ifndef T_ESP_SPS_MGMT_CLASS_H
#include "esp_sps_mgmt_class.h"
#endif

/*********************************************************************
* espSpsMgmt::espSpsMgmt() (Constructor)
*********************************************************************
*  Description:
*       Initializes class variables
*  Input: Parameters
*      none
*  Returns:
*      none
*********************************************************************/
espSpsMgmt::espSpsMgmt() : bESP()
{
    // Store Object number
    idnumber = (unsigned) ESP_SPS_MGMT_INTERFACE;
    espSpsMgmtCount = ++gEspSpsMgmtCount;

    // Store Object name
    idname = new char [IDNAME_LENGTH];
    strcpy(idname, "ESP_SPS_MGMT_INTERFACE");

    Esp_Sps_Intializing_variable();
    
    if (Debug) {
        sprintf(temp, "espSpsMgmt class constructor (%d) %s\n", 
                idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API ESP SPS Interface function calls>
    espSpsMgmtInterfaceFuncs = 
        "[function call]\n" \
        "[short name]       [FBE API ESP SPS Interface]\n" \
        " ------------------------------------------------------------\n" \
        " getInputPower     fbe_api_esp_sps_mgmt_getSpsInputPower\n" \
        " getSpsStatus      fbe_api_esp_sps_mgmt_getSpsStatus\n" \
        " getManufInfo      fbe_api_esp_sps_mgmt_getManufInfo\n" \
        " getSpsTestTime    fbe_api_esp_sps_mgmt_getSpsTestTime\n" \
        " ------------------------------------------------------------\n"\
        " spsPowerDown      fbe_api_esp_sps_mgmt_powerdown\n" \
        " setSpsTestTime    fbe_api_esp_sps_mgmt_setSpsTestTime\n" \
        " ------------------------------------------------------------\n";
    
    // Define common usage for ESP SPS commands
    usage_format = 
        " Usage: %s [Bus] [Enclosure] [SPS Index (LOCAL | PEER)]\n"
        " For example: %s 0 0 local";
};

 /*********************************************************************
* espSpsMgmt Class :: MyIdNumIs() (Accessor method)
*********************************************************************
*  Description:
*       Returns id number of object
*  Input: Parameters
*      none
*  Returns:
*      unsigned int - id number
*********************************************************************/
unsigned espSpsMgmt::MyIdNumIs(void)
{
    return idnumber;
};

/*********************************************************************
* espSpsMgmt Class :: MyIdNameIs() (Accessor method)
*********************************************************************
*  Description:
*       Returns id name of object
*  Input: Parameters
*      none
*  Returns:
*      char* - id name
*********************************************************************/
char * espSpsMgmt::MyIdNameIs(void)
{
    return idname;
};

/*********************************************************************
* espSpsMgmt Class :: dumpme()
*********************************************************************
*  Description:
*       Dumps id number and phy discovery count
*  Input: Parameters
*      none
*  Returns:
*      none
*********************************************************************/
void espSpsMgmt::dumpme(void)
{ 
    strcpy (key, "espSpsMgmt::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n", 
            idnumber, espSpsMgmtCount);
    vWriteFile(key, temp);
}

/*********************************************************************
* espSpsMgmt Class :: select()
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
*      15-July-2011 : initial version
*
*********************************************************************/
fbe_status_t espSpsMgmt::select(int index, int argc, char *argv[]) 
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select ESP SPS interface"); 

    // List ESP SPS Interface calls if help option and 
    // less than 6 arguments were entered.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) espSpsMgmtInterfaceFuncs );

    // get sps input power info
    } else if (strcmp (argv[index], "GETINPUTPOWER") == 0) {
        status = get_sps_input_power(argc, &argv[index]);

    // get sps status
    } else if (strcmp (argv[index], "GETSPSSTATUS") == 0) {
        status = get_sps_status(argc, &argv[index]);

    // get sps manufacturer info
    } else if (strcmp (argv[index], "GETMANUFINFO") == 0) {
        status = get_sps_manufacturer_information(argc, &argv[index]);

    // get sps test time
    } else if (strcmp (argv[index], "GETSPSTESTTIME") == 0) {
        status = get_sps_test_time(argc, &argv[index]);

    // power down the sps
    } else if (strcmp (argv[index], "SPSPOWERDOWN") == 0) {
        status = sps_power_down(argc, &argv[index]);

    // set sps test time
    } else if (strcmp (argv[index], "SETSPSTESTTIME") == 0) {
        status = set_sps_test_time(argc, &argv[index]);

    // can't find match on short name
    } else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n", 
                BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) espSpsMgmtInterfaceFuncs );
    } 

    return status;
}

/*********************************************************************
* espSpsMgmt Class :: get_sps_input_power()
*********************************************************************
*
*  Description:
*      Gets the SPS Input Power from the ESP SPS Mgmt object.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t espSpsMgmt::get_sps_input_power(int argc, char ** argv)
{
    // Define common usage for ESP SPS commands
    usage_format = 
        " Usage: %s [SPS Index (LOCAL | PEER)]\n"
        " For example: %s local";
    
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getInputPower",
        "espSpsMgmt::get_sps_input_power",
        "fbe_api_esp_sps_mgmt_getSpsInputPower",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    argv++;

    /*Get index for the SP. (0-local , 1-peer)*/
    spsInputPowerInfo.spsLocation.slot = utilityClass::convert_sp_name_to_index(argv);
    sprintf(params, "SPS Index %d", spsInputPowerInfo.spsLocation.slot);

    if(spsInputPowerInfo.spsLocation.slot >=  FBE_SPS_MAX_COUNT)
    {
        sprintf(temp,"Invalid SPS Index!!   Valid index are LOCAL|PEER.");
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }

    // Make api call: 
    status = fbe_api_esp_sps_mgmt_getSpsInputPower(
               &spsInputPowerInfo);

    // Check status of call
    if (status != FBE_STATUS_OK)
    {
        sprintf(temp,"Unable to obtain SPS Information.for Sps Index: 0x%x(%s)\n",
            spsInputPowerInfo.spsLocation.slot,*argv);
    }
    else
    {
        edit_sps_input_power(&spsInputPowerInfo);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* espSpsMgmt Class :: edit_sps_input_power() (private method)
*********************************************************************
*
*  Description:
*      Format the sps input power and status information to display
*
*  Input: Parameters
*      spsInputPowerInfo  - sps information structure

*  Returns:
*      void
*********************************************************************/
void espSpsMgmt::edit_sps_input_power(
                     fbe_esp_sps_mgmt_spsInputPower_t* spsInputPowerInfo)
{

    sprintf(temp,  "\n"
        "<SPS>                              %s\n\n"
        "<Current Input Power Status>       0x%x (%s)\n"
        "<Current Input Power>              %d Watts\n\n"
        "<Average Input Power Status>       0x%x (%s)\n"
        "<Average Input Power>              %d Watts\n\n"
        "<Maximum Input Power Status>       0x%x (%s)\n"
        "<MaximumInput Power>               %d Watts\n",
        
        fbe_sps_mgmt_getSpsString(&spsInputPowerInfo->spsLocation),

        spsInputPowerInfo->spsCurrentInputPower.status,
        utilityClass::convert_power_status_number_to_string( 
            spsInputPowerInfo->spsCurrentInputPower.status),
        spsInputPowerInfo->spsCurrentInputPower.inputPower,

        spsInputPowerInfo->spsAverageInputPower.status,
        utilityClass::convert_power_status_number_to_string( 
            spsInputPowerInfo->spsAverageInputPower.status),
        spsInputPowerInfo->spsAverageInputPower.inputPower,

        spsInputPowerInfo->spsMaxInputPower.status,
        utilityClass::convert_power_status_number_to_string( 
            spsInputPowerInfo->spsMaxInputPower.status),
        spsInputPowerInfo->spsMaxInputPower.inputPower);

    return;

}

/*********************************************************************
* espSpsMgmt Class :: get_sps_status()
*********************************************************************
*
*  Description:
*      Gets the SPS Status from the ESP SPS Mgmt object.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t espSpsMgmt::get_sps_status(int argc, char ** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getSpsStatus",
        "espSpsMgmt::get_sps_status",
        "fbe_api_esp_sps_mgmt_getSpsStatus",
        usage_format,
        argc, 9);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    argv++;
    // Get Bus 
        bus_status = get_and_verify_bus_number(argv,
        &spsStatusInfo.spsLocation);

    // Get enclosure
    argv++;
    encl_status = get_and_verify_enclosure_number(argv,
        &spsStatusInfo.spsLocation);

    /*Get index for the SP. (0-local , 1-peer)*/
    argv++;
    spsStatusInfo.spsLocation.slot = utilityClass::convert_sp_name_to_index(argv);

    sprintf(params, "Bus (%d), Enclosure (%d), SPS Index %d(%s)", 
        spsStatusInfo.spsLocation.bus, spsStatusInfo.spsLocation.enclosure, spsStatusInfo.spsLocation.slot,*argv);

    if((bus_status !=FBE_STATUS_OK) || (encl_status != FBE_STATUS_OK)){
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }

    if(spsStatusInfo.spsLocation.slot >=  FBE_SPS_MAX_COUNT)
    {
        sprintf(temp,"Invalid SPS Index!! Valid values are LOCAL | PEER.");
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }

    // Make api call: 
    status = fbe_api_esp_sps_mgmt_getSpsStatus(
               &spsStatusInfo);

    // Check status of call
    if (status != FBE_STATUS_OK)
    {
        sprintf(temp,"Unable to obtain Status for SPS Index: 0x%x(%s)\n",
            spsStatusInfo.spsLocation.slot,*argv);
    }
    else
    {
        edit_sps_status(&spsStatusInfo);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* espSpsMgmt Class :: get_sps_manufacturer_information()
*********************************************************************
*
*  Description:
*      Gets the SPS Manufacturer information from the ESP SPS Mgmt object.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t espSpsMgmt::get_sps_manufacturer_information(int argc, char ** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getManufInfo",
        "espSpsMgmt::get_sps_manufacturer_information",
        "fbe_api_esp_sps_mgmt_getManufInfo",
        usage_format,
        argc, 9);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    argv++;
    // Get Bus 
        bus_status = get_and_verify_bus_number(argv,
        &spsStatusInfo.spsLocation);

    // Get enclosure
    argv++;
    encl_status = get_and_verify_enclosure_number(argv,
        &spsStatusInfo.spsLocation);

    /*Get index for the SP. (0-local , 1-peer)*/
    spsStatusInfo.spsLocation.slot = utilityClass::convert_sp_name_to_index(argv);

    sprintf(params, "Bus (%d), Enclosure (%d), SPS Index %d(%s)", 
        spsStatusInfo.spsLocation.bus, spsStatusInfo.spsLocation.enclosure, spsStatusInfo.spsLocation.slot,*argv);

    if((bus_status !=FBE_STATUS_OK) || (encl_status != FBE_STATUS_OK)){
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }

    if(spsManufInfo.spsLocation.slot >=  FBE_SPS_MAX_COUNT)
    {
        sprintf(temp,"Invalid SPS Index!! Valid values are LOCAL | PEER.");
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }

    // Make api call: 
    status = fbe_api_esp_sps_mgmt_getManufInfo(
               &spsManufInfo);

    // Check status of call
    if (status != FBE_STATUS_OK)
    {
        sprintf(temp,"Unable to obtain manufacturer information"
            " for Sps Index: 0x%x(%s)\n",
            spsManufInfo.spsLocation.slot,*argv);
    }
    else
    {
        edit_sps_manufacturer_information(&spsManufInfo);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* espSpsMgmt Class :: get_sps_test_time()
*********************************************************************
*
*  Description:
*      Gets the SPS Test Time from the ESP SPS Mgmt object.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t espSpsMgmt::get_sps_test_time(int argc, char ** argv)
{
    // Define specific usage string  
    usage_format = 
    " Usage: %s \n"
    " For example: %s ";
    
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getSpsTestTime",
        "espSpsMgmt::get_sps_test_time",
        "fbe_api_esp_sps_mgmt_getSpsTestTime",
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    params[0] = '\0';

    // Make api call: 
    status = fbe_api_esp_sps_mgmt_getSpsTestTime(
               &spsTestTimeInfo);

    // Check status of call
    if (status != FBE_STATUS_OK)
    {
        sprintf(temp,"Unable to obtain SPS Test Time information.\n");
    }
    else
    {
        edit_sps_test_time(&spsTestTimeInfo);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* espSpsMgmt Class :: sps_power_down()
*********************************************************************
*
*  Description:
*      This function send request to ESP SPS Mgmt object to power down the sps.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t espSpsMgmt::sps_power_down(int argc, char ** argv)
{
    // Define specific usage string  
    usage_format = 
    " Usage: %s \n"
    " For example: %s ";
        
    // Check that all arguments have been entered
    status = call_preprocessing(
        "spsPowerDown",
        "espSpsMgmt::sps_power_down",
        "fbe_api_esp_sps_mgmt_powerdown",
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    params[0] = '\0';

    // Make api call: 
    status = fbe_api_esp_sps_mgmt_powerdown();

    // Check status of call
    if (status != FBE_STATUS_OK)
    {
        sprintf(temp,"SPS power down failed!!!\n");
    }
    else
    {
        sprintf(temp,"Successfully powered down SPS.\n");
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* espSpsMgmt Class :: set_sps_test_time()
*********************************************************************
*
*  Description:
*     Set SPS TestTime from the ESP SPS Mgmt object.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t espSpsMgmt::set_sps_test_time(int argc, char ** argv)
{
    fbe_system_time_t currentTime;
    fbe_s8_t timeString[8] = {'\0'};

    // Define specific usage string  
    usage_format = 
    " Usage: %s <Weekday> <Day>"\
    "<Hour> <Minute>"
    " For example: %s monday 12 11 30";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "setSpsTestTime",
        "espSpsMgmt::set_sps_test_time",
        "fbe_api_esp_sps_mgmt_setSpsTestTime",
        usage_format,
        argc, 10);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    params[0] = '\0';

    status = fbe_get_system_time(&currentTime);
    if (status != FBE_STATUS_OK)
    {
        sprintf(temp,"Unable to get the system time.\n");
        call_post_processing(status, temp, key, params);
        return status;
    }

    argv++;
    currentTime.weekday = utilityClass::convert_sp_weekday_to_index(argv);
    if (currentTime.weekday > 6)
    {
        sprintf(temp,"Invalid Weekday!!.\n");
        call_post_processing(status, temp, key, params);
        return status;
    }

    argv++;
    currentTime.day = (fbe_u32_t)strtoul(*argv, 0, 0);
    if (currentTime.day > 31)
    {
        sprintf(temp,"Invalid Day!!.\n");
        call_post_processing(status, temp, key, params);
        return status;
    }

    argv++;
    currentTime.hour= (fbe_u32_t)strtoul(*argv, 0, 0);
    if (currentTime.hour > 23)
    {
        sprintf(temp,"Invalid Hour!!.\n");
        call_post_processing(status, temp, key, params);
        return status;
    }

    argv++;
    currentTime.minute= (fbe_u32_t)strtoul(*argv, 0, 0);
    if (currentTime.minute > 59)
    {
        sprintf(temp,"Invalid Minute!!.\n");
        call_post_processing(status, temp, key, params);
        return status;
    }

    fbe_getTimeString(&currentTime,timeString);
    sprintf(params,"Weekly SPS Test Time     %s %s",
        fbe_getWeekDayString(currentTime.weekday),
        timeString);


    spsTestTimeInfo.spsTestTime = currentTime;
    
    // Make api call: 
    status = fbe_api_esp_sps_mgmt_setSpsTestTime(
               &spsTestTimeInfo);

    // Check status of call
    if (status != FBE_STATUS_OK)
    {
        sprintf(temp,"Unable to set SPS Test Time.\n");
    }
    else
    {
        sprintf(temp,"SPS Test time has been set to %s %s\n",
            fbe_getWeekDayString(currentTime.weekday),
            timeString);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* espSpsMgmt Class :: edit_sps_status() (private method)
*********************************************************************
*
*  Description:
*      Format the sps status information to display
*
*  Input: Parameters
*      spsStatusInfo  - sps status information structure

*  Returns:
*      void
*********************************************************************/
void espSpsMgmt::edit_sps_status(
                             fbe_esp_sps_mgmt_spsStatus_t *spsStatusInfo)
{

    sprintf(temp,  "\n"
        "<SPS>                   %s\n"
        "<SPS Inserted>          %s\n"
        "<SPS Status>            %s \n"
        "<SPS Faults>            %s\n"
        "<SPS Cabling Status>    %s\n",
        fbe_sps_mgmt_getSpsString(&spsStatusInfo->spsLocation),
        spsStatusInfo->spsModuleInserted ? "Yes" : "No",
        fbe_sps_mgmt_getSpsStatusString(spsStatusInfo->spsStatus),
        fbe_sps_mgmt_getSpsFaultString(&spsStatusInfo->spsFaultInfo),
        fbe_sps_mgmt_getSpsCablingStatusString(spsStatusInfo->spsCablingStatus));

    return;
}

/*********************************************************************
* espSpsMgmt Class :: edit_sps_manufacturer_information() (private method)
*********************************************************************
*
*  Description:
*      Format the sps manufacturer information to display
*
*  Input: Parameters
*      spsManufInfo  - sps manufacturer information structure

*  Returns:
*      void
*********************************************************************/
void espSpsMgmt::edit_sps_manufacturer_information(
                     fbe_esp_sps_mgmt_spsManufInfo_t *spsManufInfo)
{

    sprintf(temp,  "\n"
        "<SPS>                         %s\n"
        "<SPS Serial Number>           %s\n"
        "<SPS Part Number>             %s\n"
        "<SPS Part Number Revision>    %s\n"
        "<SPS Vendor>                  %s\n"
        "<SPS Vendor Model Number>     %s\n"
        "<SPS Firmware Revision>       %s\n"
        "<SPS Model String>            %s\n",
        fbe_sps_mgmt_getSpsString(&spsManufInfo->spsLocation),
        spsManufInfo->spsManufInfo.spsModuleManufInfo.spsSerialNumber,
        spsManufInfo->spsManufInfo.spsModuleManufInfo.spsPartNumber,
        spsManufInfo->spsManufInfo.spsModuleManufInfo.spsPartNumRevision,
        spsManufInfo->spsManufInfo.spsModuleManufInfo.spsVendor,
        spsManufInfo->spsManufInfo.spsModuleManufInfo.spsVendorModelNumber,
        spsManufInfo->spsManufInfo.spsModuleManufInfo.spsFirmwareRevision,
        spsManufInfo->spsManufInfo.spsModuleManufInfo.spsModelString);

    return;
}

/*********************************************************************
* espSpsMgmt Class :: edit_sps_test_time() (private method)
*********************************************************************
*
*  Description:
*      Format the sps test time to display
*
*  Input: Parameters
*      spsTestTimeInfo  - sps test time information structure

*  Returns:
*      void
*********************************************************************/
void espSpsMgmt::edit_sps_test_time(
                     fbe_esp_sps_mgmt_spsTestTime_t *spsTestTimeInfo)
{
    fbe_s8_t timeString[8] = {'\0'};
    fbe_getTimeString(&(spsTestTimeInfo->spsTestTime),timeString);
    sprintf(temp,"\n<Weekly SPS Test Time>     %s %s\n",
        fbe_getWeekDayString(spsTestTimeInfo->spsTestTime.weekday),
        timeString);

    return;
}

/*********************************************************************
* espSpsMgmt::Esp_Sps_Intializing_variable (private method)
*********************************************************************/
void espSpsMgmt::Esp_Sps_Intializing_variable()
{

    fbe_zero_memory(&spsInputPowerInfo,sizeof(spsInputPowerInfo));
    fbe_zero_memory(&spsStatusInfo,sizeof(spsStatusInfo));
    fbe_zero_memory(&spsManufInfo,sizeof(spsManufInfo));
    fbe_zero_memory(&spsTestTimeInfo,sizeof(spsTestTimeInfo));
    return;
}

/*********************************************************************
* espSpsMgmt Class :: get_and_verify_bus_number(
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
fbe_status_t espSpsMgmt::get_and_verify_bus_number(
    char ** argv,
    fbe_device_physical_location_t*  location_p )
{
    location_p->bus = (fbe_u8_t)strtoul(*argv, 0, 0);
    
    if((location_p->bus >= FBE_PHYSICAL_BUS_COUNT)&&(location_p->bus != 
    FBE_XPE_PSEUDO_BUS_NUM)){
        sprintf(temp,"Entered Bus number (%d) exceeds the total Bus Count (%d)\n",
            location_p->bus,
            FBE_PHYSICAL_BUS_COUNT);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
* espSpsMgmt Class :: get_and_verify_enclosure_number(
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
fbe_status_t espSpsMgmt::get_and_verify_enclosure_number(
    char ** argv,
    fbe_device_physical_location_t*  location_p )
{
    fbe_u32_t       enclCount;
    location_p->enclosure = (fbe_u8_t)strtoul(*argv, 0, 0);

    //Get  the total enclosure count 
   status =fbe_api_esp_encl_mgmt_get_encl_count_on_bus(location_p,&enclCount);

    if((location_p->enclosure >= enclCount)&&(location_p->enclosure != 
    FBE_XPE_PSEUDO_ENCL_NUM)){
        sprintf(temp,"Enclosure number (%d) must be less than the "
            " enclosure count (%d) on the bus.\n",
            location_p->enclosure, 
            enclCount);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
 * espSpsMgmt end of file
 *********************************************************************/
