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
*      This file defines the methods of the ESP Enclosure Management class.
*
*  Notes:
*      The ESP Encl Mgmt class (espEnclMgmt) is a derived class of 
*      the base class (bESP).
*
*  History:
*      20-July-2011 : initial version
*
*********************************************************************/

#ifndef T_ESP_ENCL_MGMT_CLASS_H
#include "esp_encl_mgmt_class.h"
#endif

/*********************************************************************
* espEnclMgmt class :: Constructor
*********************************************************************/
espEnclMgmt::espEnclMgmt() : bESP()
{
    // Store Object number
    idnumber = (unsigned) ESP_ENCL_MGMT_INTERFACE;
    espEnclMgmtCount = ++gEspEnclMgmtCount;

    // Store Object name
    idname = new char [IDNAME_LENGTH];
    strcpy(idname, "ESP_ENCL_MGMT_INTERFACE");
    Esp_Encl_Intializing_variable();
    
    if (Debug) {
        sprintf(temp, "espEnclMgmt class constructor (%d) %s\n", 
                idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API ESP ENCL MGMT Interface function calls>
    espEnclMgmtInterfaceFuncs = 
        "[function call]\n" \
        "[short name]           [FBE API ESP ENCL MGMT Interface]\n" \
        " ---------------------------------------------------------------\n" \
        " getTotalEnclCount      fbe_api_esp_encl_mgmt_get_total_encl_count\n" \
        " getEnclLocation        fbe_api_esp_encl_mgmt_get_encl_location\n"\
        " getEnclCountOnBus      fbe_api_esp_encl_mgmt_get_encl_count_on_bus\n"\
        " getEirInfo             fbe_api_esp_encl_mgmt_get_eir_info\n"\
        " ---------------------------------------------------------------\n"\
        " getLccCount            fbe_api_esp_encl_mgmt_get_lcc_count\n"\
        " getLccInfo             fbe_api_esp_encl_mgmt_get_lcc_info\n"\
        " getPsCount             fbe_api_esp_encl_mgmt_get_ps_count\n"\
        " getFanCount            fbe_api_esp_encl_mgmt_get_fan_count\n"\
        " getDriveSlotCount      fbe_api_esp_encl_mgmt_get_drive_slot_count\n"\
        " ---------------------------------------------------------------\n"\
        " getEnclInfo            fbe_api_esp_encl_mgmt_get_encl_info\n"\
        " ---------------------------------------------------------------\n";

    // Define common usage for ESP ENCL MGMT commands
    usage_format = 
        " Usage: %s\n"
        " For example: %s";
};

/*********************************************************************
* espEnclMgmt class : Accessor methods
*********************************************************************/

unsigned espEnclMgmt::MyIdNumIs(void)
{
    return idnumber;
};

char * espEnclMgmt::MyIdNameIs(void)
{
    return idname;
};
void espEnclMgmt::dumpme(void)
{ 
    strcpy (key, "espEnclMgmt::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n", 
            idnumber, espEnclMgmtCount);
    vWriteFile(key, temp);
};

/*********************************************************************
* espEnclMgmt Class :: select(int index, int argc, char *argv[])
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
*      20-July-2011 : initial version
*
*********************************************************************/

fbe_status_t espEnclMgmt::select(int index, int argc, char *argv[]) 
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select ESP interface"); 

    // List ESP Encl Mgmt Interface calls if help option and 
    // less than 6 arguments were entered.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) espEnclMgmtInterfaceFuncs );

        // get Encl Mgmt info
    } else if (strcmp (argv[index], "GETTOTALENCLCOUNT") == 0) {
        status = get_total_encl_count(argc, &argv[index]);

    } else if (strcmp (argv[index], "GETENCLLOCATION") == 0) {
        status = get_encl_location(argc, &argv[index]);

    } else if (strcmp (argv[index], "GETENCLCOUNTONBUS") == 0) {
        status = get_encl_count_on_bus(argc, &argv[index]);

    } else if (strcmp (argv[index], "GETEIRINFO") == 0) {
        status = get_eir_info(argc, &argv[index]);

   } else if (strcmp (argv[index], "GETLCCCOUNT") == 0) {
        status = get_lcc_count(argc, &argv[index]);

    } else if (strcmp (argv[index], "GETLCCINFO") == 0) {
        status = get_lcc_info(argc, &argv[index]);

    } else if (strcmp (argv[index], "GETPSCOUNT") == 0) {
        status = get_ps_count(argc, &argv[index]);

    } else if (strcmp (argv[index], "GETFANCOUNT") == 0) {
        status = get_fan_count(argc, &argv[index]);

    } else if (strcmp (argv[index], "GETDRIVESLOTCOUNT") == 0) {
        status = get_drive_slot_count(argc, &argv[index]);

    } else if (strcmp (argv[index], "GETENCLINFO") == 0) {
        status = get_encl_info(argc, &argv[index]);

    // can't find match on short name
    } else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n", 
                BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) espEnclMgmtInterfaceFuncs );
    } 

    return status;
}

/*********************************************************************
* espEnclMgmt Class :: get_total_encl_count(int argc, char ** argv)
*********************************************************************
*
*  Description:
*      Get the total enclosure count.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t espEnclMgmt::get_total_encl_count(int argc, char ** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getTotalEnclCount",
        "espEnclMgmt::get_total_encl_count",
        "fbe_api_esp_encl_mgmt_get_total_encl_count",
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    // Make api call: 
    status = fbe_api_esp_encl_mgmt_get_total_encl_count(&enclCount);
    
    if(status != FBE_STATUS_OK) {
        sprintf(temp,"Failed to retrive data.\n");
    }
    else {
        sprintf(temp,"\n<Total Enclosure count>    %d\n",enclCount);
    }

    params[0] = '\0';
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* espEnclMgmt Class :: get_encl_location(int argc, char ** argv)
*********************************************************************
*
*  Description:
*      Get the enclosure location.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t espEnclMgmt::get_encl_location(int argc, char ** argv)
{
    usage_format = 
        " Usage: %s [Enclosure Index]\n"
        " For example: %s 0";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getEnclLocation",
        "espEnclMgmt::get_encl_location",
        "fbe_api_esp_encl_mgmt_get_encl_location",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    argv++;
    enclLocationInfo.enclIndex = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, "Enclosure (%d)", enclLocationInfo.enclIndex);

    //Get  the total enclosure count 
    status = fbe_api_esp_encl_mgmt_get_total_encl_count(&enclCount);

    if(enclLocationInfo.enclIndex >= enclCount){
        sprintf(temp,"Enclosure Index (%d) must be less than the total"
            " enclosure count (%d) on the array.\n",
            enclLocationInfo.enclIndex, 
            enclCount);
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }

    // Make api call: 
    status = fbe_api_esp_encl_mgmt_get_encl_location(&enclLocationInfo);

    if(status != FBE_STATUS_OK) {
       sprintf(temp,"Failed to retrive data.\n");
    }
    else {
        edit_encl_info( &enclLocationInfo);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* espEnclMgmt Class :: get_encl_count_on_bus(int argc, char ** argv)
*********************************************************************
*
*  Description:
*      Get the total enclosure count on bus.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t espEnclMgmt::get_encl_count_on_bus(int argc, char ** argv)
{
    fbe_status_t bus_status;
    usage_format = 
        " Usage: %s [Bus]\n"
        " For example: %s 0";
    
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getEnclCountOnBus ",
        "espEnclMgmt::get_encl_count_on_bus",
        "fbe_api_esp_encl_mgmt_get_encl_count_on_bus",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    argv++;
    bus_status = get_and_verify_bus_number(argv,&location);

    sprintf(params, "Bus (%d)", location.bus);

    if((bus_status !=FBE_STATUS_OK)){
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }
    
    // Make api call: 
    status = fbe_api_esp_encl_mgmt_get_encl_count_on_bus(&location, &enclCount);

    if(status != FBE_STATUS_OK) {
        sprintf(temp,"Failed to retrive data.\n");
    }
    else {
        sprintf(temp,"\n<Enclosure count on Bus(%d)>     %d\n",
            location.bus, enclCount);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* espEnclMgmt Class :: get_eir_info(int argc, char ** argv)
*********************************************************************
*
*  Description:
*      Get the eir information..
*
*  Input: Parameters
*      argc - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t espEnclMgmt::get_eir_info(int argc, char ** argv)
{
    fbe_status_t bus_status, encl_status;
    usage_format = 
        " Usage: %s [Bus] [Enclosure]\n"
        " For example: %s 0 1";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getEirInfo ",
        "espEnclMgmt::get_eir_info",
        "fbe_api_esp_encl_mgmt_get_eir_info",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    argv++;
    bus_status = get_and_verify_bus_number(argv,
        &eir_info.eirEnclInfo.eirEnclIndividual.location);
    argv++;
    encl_status = get_and_verify_enclosure_number(argv,
        &eir_info.eirEnclInfo.eirEnclIndividual.location);

    sprintf(params, "Bus(%d)  enclosure(%d)", 
        eir_info.eirEnclInfo.eirEnclIndividual.location.bus,
        eir_info.eirEnclInfo.eirEnclIndividual.location.enclosure);

    if((bus_status !=FBE_STATUS_OK) || (encl_status != FBE_STATUS_OK)){
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }

   // Make api call: 
    status = fbe_api_esp_encl_mgmt_get_eir_info(&eir_info);
    if(status != FBE_STATUS_OK) {
        sprintf(temp,"Failed to retrive data.\n");
    }
    else {
        edit_eir_info(&eir_info);
    }
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* espEnclMgmt Class :: get_lcc_count(int argc, char ** argv)
*********************************************************************
*
*  Description:
*      Get the LCC count on a Enclosure.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t espEnclMgmt::get_lcc_count(int argc, char ** argv)
{
    fbe_status_t bus_status, encl_status;
    usage_format = 
        " Usage: %s [Bus] [Enclosure]\n"
        " For example: %s 0 1";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getLccCount",
        "espEnclMgmt::get_lcc_count",
        "fbe_api_esp_encl_mgmt_get_lcc_count",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    argv++;
    bus_status = get_and_verify_bus_number(argv,&location);
    argv++;
    encl_status = get_and_verify_enclosure_number(argv, &location);

    sprintf(params, "Bus(%d)  enclosure(%d)", 
        location.bus, 
        location.enclosure);

    if((bus_status !=FBE_STATUS_OK) || (encl_status != FBE_STATUS_OK)){
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }

    // Make api call: 
    status = fbe_api_esp_encl_mgmt_get_lcc_count(&location, &deviceCount);
    if(status != FBE_STATUS_OK) {
        sprintf(temp,"Failed to retrive LCC count.\n");
    }
    else {
        sprintf(temp, "\n<LCC count>       %d", deviceCount);
    }
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* espEnclMgmt Class :: get_lcc_info(int argc, char ** argv)
*********************************************************************
*
*  Description:
*      Get the LCC information.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t espEnclMgmt::get_lcc_info(int argc, char ** argv)
{
    fbe_status_t bus_status, encl_status, slot_status;
    usage_format = 
        " Usage: %s [Bus] [Enclosure] [slot]\n"
        " For example: %s 0 1 0";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getLccInfo",
        "espEnclMgmt::get_lcc_info",
        "fbe_api_esp_encl_mgmt_get_lcc_info",
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

    //Make API call
    status = fbe_api_esp_encl_mgmt_get_lcc_info(&location, &lccInfo);
    if(status != FBE_STATUS_OK) {
        sprintf(temp,"Failed to retrive LCC information.\n");
    }
    else {
        edit_lcc_info(&lccInfo);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}


/*********************************************************************
* espEnclMgmt Class :: get_ps_count(int argc, char ** argv)
*********************************************************************
*
*  Description:
*      Get the PS count on enclosure.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t espEnclMgmt::get_ps_count(int argc, char ** argv)
{
    fbe_status_t bus_status, encl_status;
    usage_format = 
        " Usage: %s [Bus] [Enclosure]\n"
        " For example: %s 0 1";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getPsCount",
        "espEnclMgmt::get_ps_count",
        "fbe_api_esp_encl_mgmt_get_ps_count",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    argv++;
    bus_status = get_and_verify_bus_number(argv,&location);
    argv++;
    encl_status = get_and_verify_enclosure_number(argv, &location);

    sprintf(params, "Bus(%d)  enclosure(%d)", 
        location.bus, 
        location.enclosure);

    if((bus_status !=FBE_STATUS_OK) || (encl_status != FBE_STATUS_OK)){
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }
    
    // Make api call: 
    status = fbe_api_esp_encl_mgmt_get_ps_count(&location, &deviceCount);
    if(status != FBE_STATUS_OK) {
        sprintf(temp,"Failed to retrive PS count.\n");
    }
    else {
        sprintf(temp,"\n<PS Count>       %d\n",deviceCount);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* espEnclMgmt Class :: get_fan_count(int argc, char ** argv)
*********************************************************************
*
*  Description:
*      Get the fan count on enclosure.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t espEnclMgmt::get_fan_count(int argc, char ** argv)
{
    fbe_status_t bus_status, encl_status;
    usage_format = 
        " Usage: %s [Bus] [Enclosure]\n"
        " For example: %s 0 1";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getFanCount",
        "espEnclMgmt::get_fan_count",
        "fbe_api_esp_encl_mgmt_get_fan_count",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    argv++;
    bus_status = get_and_verify_bus_number(argv,&location);
    argv++;
    encl_status = get_and_verify_enclosure_number(argv, &location);

    sprintf(params, "Bus(%d)  enclosure(%d)", 
        location.bus, 
        location.enclosure);

    if((bus_status !=FBE_STATUS_OK) || (encl_status != FBE_STATUS_OK)){
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }

    //Make API call
    status = fbe_api_esp_encl_mgmt_get_fan_count(&location, &deviceCount);
    if(status != FBE_STATUS_OK) {
        sprintf(temp,"Failed to retrive fan count.\n");
    }
    else {
        sprintf(temp,"\n<Fan Count>       %d\n",deviceCount);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* espEnclMgmt Class :: get_drive_slot_count(int argc, char ** argv)
*********************************************************************
*
*  Description:
*      Get the drive slot count on specified enclosure.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t espEnclMgmt::get_drive_slot_count(int argc, char ** argv)
{
    fbe_status_t bus_status, encl_status;
    usage_format = 
        " Usage: %s [Bus] [Enclosure]\n"
        " For example: %s 0 1";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getDriveSlotCount",
        "espEnclMgmt::get_drive_slot_count",
        "fbe_api_esp_encl_mgmt_get_drive_slot_count",
        usage_format,
        argc,8 );

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    argv++;
    bus_status = get_and_verify_bus_number(argv,&location);
    argv++;
    encl_status = get_and_verify_enclosure_number(argv, &location);

    sprintf(params, "Bus(%d)  enclosure(%d)", 
        location.bus, 
        location.enclosure);

    if((bus_status !=FBE_STATUS_OK) || (encl_status != FBE_STATUS_OK)){
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }

    //Make API call
    status = fbe_api_esp_encl_mgmt_get_drive_slot_count(
        &location,
        &deviceCount);

    if(status != FBE_STATUS_OK) {
        sprintf(temp,"Failed to retrive drive slot count.\n");
    }
    else {
        sprintf(temp,"\n<Drive Slot Count>       %d\n",deviceCount);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}


/*********************************************************************
* espEnclMgmt Class :: edit_eir_info() (private method)
*********************************************************************
*
*  Description:
*      Edit the eir input power and status information to display
*
*  Input: Parameters
*      eirInfo  - EIR information structure

*  Returns:
*      void
*********************************************************************/

void espEnclMgmt::edit_eir_info(fbe_esp_encl_mgmt_get_eir_info_t* eirInfo)
{
    sprintf(temp,  "\n"
        "<Current Input Power Status>      0x%x (%s)\n"
        "<Current Input Power>             %d Watts\n\n"
        "<Average Input Power Status>      0x%x (%s)\n"
        "<Average Input Power>             %d Watts\n\n"
        "<Maximum Input Power Status>      0x%x (%s)\n"
        "<Maximum Input Power>             %d Watts\n",
        eirInfo->eirEnclInfo.eirEnclIndividual.eirInfo.enclCurrentInputPower.status,
        utilityClass::convert_power_status_number_to_string( 
        eirInfo->eirEnclInfo.eirEnclIndividual.eirInfo.enclCurrentInputPower.status),
        eirInfo->eirEnclInfo.eirEnclIndividual.eirInfo.enclCurrentInputPower.inputPower,

        eirInfo->eirEnclInfo.eirEnclIndividual.eirInfo.enclAverageInputPower.status,
        utilityClass::convert_power_status_number_to_string( 
            eirInfo->eirEnclInfo.eirEnclIndividual.eirInfo.enclAverageInputPower.status),
        eirInfo->eirEnclInfo.eirEnclIndividual.eirInfo.enclAverageInputPower.inputPower,

        eirInfo->eirEnclInfo.eirEnclIndividual.eirInfo.enclMaxInputPower.status,
        utilityClass::convert_power_status_number_to_string( 
            eirInfo->eirEnclInfo.eirEnclIndividual.eirInfo.enclMaxInputPower.status),
        eirInfo->eirEnclInfo.eirEnclIndividual.eirInfo.enclMaxInputPower.inputPower);

}

/*********************************************************************
* espEnclMgmt Class :: edit_encl_info( 
*    fbe_esp_encl_mgmt_get_encl_loc_t* encl_info) (private method)
*********************************************************************
*
*  Description:
*      Edit the enclosure information to display
*
*  Input: Parameters
*      encl_info  - enclosure information

*  Returns:
*      void
*********************************************************************/
void espEnclMgmt::edit_encl_info( fbe_esp_encl_mgmt_get_encl_loc_t* encl_info)
{

    sprintf(temp,"\n"
        "<Bus>                    %d\n"
        "<Enclosure>              %d\n",
        encl_info->location.bus,
        encl_info->location.enclosure);
}

/*********************************************************************
* espEnclMgmt Class :: edit_lcc_info(fbe_esp_encl_mgmt_lcc_info_t* lccInfo_p)
*********************************************************************
*
*  Description:
*      Edit the LCC information to display.
*
*  Input: Parameters
*      *lccInfo_p - pointer to LCC info structure
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
void espEnclMgmt::edit_lcc_info(fbe_esp_encl_mgmt_lcc_info_t* lccInfo_p)
{
    char inserted[6];
    char faulted[6];

    if(lccInfo_p->inserted == FBE_TRUE){
        sprintf(inserted, "Yes");
    }
    else {
        sprintf(inserted, "No");
    }

    if(lccInfo_p->faulted == FBE_TRUE){
        sprintf(faulted, "Yes");
    }
    else {
        sprintf(faulted, "No");
    }
    
    sprintf(temp,"\n"
        "<LCC Inserted>            %s\n"
        "<LCC Faulted>             %s\n"
        "<LCC FirmwareRev>         %s\n"
        "<LCC Shunted>             %d\n"
        "<LCC ExpansionPortOpen>   %d\n"
        "<LCC LccType>             %d\n"
        "<LCC CurrentSpeed>        %d\n"
        "<LCC MaxSpeed>            %d\n"
        "<LCC IsLocal>             %d\n",
        inserted,
        faulted,
        lccInfo_p->firmwareRev,
        lccInfo_p->shunted,
        lccInfo_p->expansionPortOpen,
        lccInfo_p->lccType,
        lccInfo_p->currentSpeed,
        lccInfo_p->maxSpeed,
        lccInfo_p->isLocal
        );
}

/*********************************************************************
* espEnclMgmt Class :: get_and_verify_bus_number(
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
fbe_status_t espEnclMgmt::get_and_verify_bus_number(
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
* espEnclMgmt Class :: get_and_verify_enclosure_number(
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
fbe_status_t espEnclMgmt::get_and_verify_enclosure_number(
    char ** argv,
    fbe_device_physical_location_t*  location_p )
{
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
* espEnclMgmt Class :: get_and_verify_slot_number(
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
fbe_status_t espEnclMgmt::get_and_verify_slot_number(
    char ** argv,
    fbe_device_physical_location_t*  location_p )
{
    location_p->slot = (fbe_u8_t)strtoul(*argv, 0, 0);

   status = fbe_api_esp_encl_mgmt_get_lcc_count(location_p, &deviceCount);
    if(location_p->slot>= deviceCount) {
        sprintf(temp,"Bus slot number %d must be less than the LCC device "
            "count (%d) on the array.\n",
            location_p->slot, 
            deviceCount);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
* espEnclMgmt Class :: get_encl_info(int argc, char ** argv)
*********************************************************************
*
*  Description:
*      Get the enclosure information.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t espEnclMgmt::get_encl_info(int argc, char ** argv)
{
    fbe_status_t bus_status, encl_status;
    usage_format = 
        " Usage: %s [Bus] [Enclosure]\n"
        " For example: %s 0 1";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getEnclInfo",
        "espEnclMgmt::get_encl_info",
        "fbe_api_esp_encl_mgmt_get_encl_info",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    argv++;
    bus_status = get_and_verify_bus_number(argv,&location);
    argv++;
    encl_status = get_and_verify_enclosure_number(argv, &location);

    sprintf(params, "Bus(%d)  enclosure(%d)", 
        location.bus, 
        location.enclosure);

    if((bus_status !=FBE_STATUS_OK) || (encl_status != FBE_STATUS_OK)){
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }
    
    // Make api call: 
    status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &EnclInfo);
    if(status != FBE_STATUS_OK) {
        sprintf(temp,"Failed to retrive Enclosure information.\n");
    }
    else {
        edit_encl_mgmt_info(&EnclInfo);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* espEnclMgmt Class :: edit_encl_mgmt_info( 
*    fbe_esp_encl_mgmt_get_encl_loc_t* encl_info) (private method)
*********************************************************************
*
*  Description:
*      Edit the enclosure information to display
*
*  Input: Parameters
*      encl_info  - enclosure information

*  Returns:
*      void
*********************************************************************/
void espEnclMgmt::edit_encl_mgmt_info( fbe_esp_encl_mgmt_encl_info_t* encl_info)
{
    sprintf(temp,"\n"
        " <enclPresent>             %s\n"
        " <Enclosure Type>          %s\n"
        " <LCC Count>               0x%x\n"
        " <PS Count>                0x%x\n"
        " <Fan Count>               0x%x\n"
        " <DriveSlot Count>         0x%x\n"
        " <Processor Enclosure>     %s\n"
        " <Shutdown Reason>         %s\n"
        " <enclstate>               %s\n"
        " <encl Fault Symptom>      %s\n"
        " <driveSlotCountPerBank>   0x%x\n"
        " <spsCount>                0x%x\n"
        " <spCount>                 0x%x\n"
        " <maxEnclSpeed>            0x%x\n"
        " <currentEnclSpeed>        0x%x\n"
        " <enclFaultLedStatus>      %s\n"
        " <enclFaultLedReason>      %s\n"
        " <crossCable>              %s\n",
        encl_info->enclPresent ? "YES" : "NO",
        get_encl_type_string(encl_info->encl_type),
        encl_info->lccCount,
        encl_info->psCount,
        encl_info->fanCount,
        encl_info->driveSlotCount,
        encl_info->processorEncl ? "YES" : "NO",
        get_enclShutdownReasonString(encl_info->shutdownReason),
        decode_encl_state(encl_info->enclState),
        decode_encl_fault_symptom(encl_info->enclFaultSymptom),
        encl_info->driveSlotCountPerBank,
        encl_info->spsCount,
        encl_info->spCount,
        encl_info->maxEnclSpeed,
        encl_info->currentEnclSpeed,
        decode_led_status(encl_info->enclFaultLedStatus),
        decode_encl_fault_led_reason(encl_info->enclFaultLedReason),
        encl_info->crossCable ? "YES" : "NO");

}

/*********************************************************************
* espEnclMgmt::Esp_Encl_Intializing_variable (private method)
*********************************************************************/
void espEnclMgmt::Esp_Encl_Intializing_variable()
{
    enclCount = 0;
    deviceCount = 0;
    fbe_zero_memory(&enclLocationInfo,sizeof(enclLocationInfo));
    fbe_zero_memory(&location,sizeof(location));
    fbe_zero_memory(&eir_info,sizeof(eir_info));
    fbe_zero_memory(&lccInfo,sizeof(lccInfo));
    fbe_zero_memory(&EnclInfo,sizeof(EnclInfo));
    return;
}


/*!**************************************************************
 * espEnclMgmt::decode_encl_fault_led_reason(fbe_u32_t enclFaultLedReason)
 ****************************************************************
 * Description:
 *  This function decodes the enclFaultLedReason.
 *
 * Input: Parameters
 *       enclFaultLedReason
 * 
 * Returns:
 *    string for enclFaultLedReason
 * 
 ****************************************************************/
char * espEnclMgmt::decode_encl_fault_led_reason(fbe_encl_fault_led_reason_t enclFaultLedReason)
{
    char        *string = enclFaultReasonString;
    fbe_u32_t    bit = 0;

    string[0] = 0;
    if (enclFaultLedReason == FBE_ENCL_FAULT_LED_NO_FLT)
    {
        strcat(string, "No Fault");
    }
    else
    {
        for(bit = 0; bit < 64; bit++)
        {

            if((1ULL << bit) >= FBE_ENCL_FAULT_LED_BITMASK_MAX)
            {
                break;
            }

            switch (enclFaultLedReason & (1ULL << bit))
            {
            case FBE_ENCL_FAULT_LED_NO_FLT:
                break;
            case FBE_ENCL_FAULT_LED_PS_FLT:
                strcat(string, " PS Fault");
                break;
            case FBE_ENCL_FAULT_LED_FAN_FLT:
                strcat(string, " Fan Fault");
                break;
            case FBE_ENCL_FAULT_LED_DRIVE_FLT:
                strcat(string, " Drive Fault");
                break;
            case FBE_ENCL_FAULT_LED_SPS_FLT:
                strcat(string, " SPS Fault");
                break;
            case FBE_ENCL_FAULT_LED_OVERTEMP_FLT:
                strcat(string, " Overtemp Fault");
                break;
            case FBE_ENCL_FAULT_LED_LCC_FLT:
                strcat(string, " LCC Fault");
                break;
            case FBE_ENCL_FAULT_LED_LCC_RESUME_PROM_FLT:
                strcat(string, " LCC RP Fault");
                break;
            case FBE_ENCL_FAULT_LED_CONNECTOR_FLT:
                strcat(string, " Connecter Fault");
                break;
            case FBE_ENCL_FAULT_LED_ENCL_LIFECYCLE_FAIL:
                strcat(string, " Encl Lifecycle Fail");
                break;
            case FBE_ENCL_FAULT_LED_SUBENCL_LIFECYCLE_FAIL:
                strcat(string, " Subencl Lifecycle Fail");
                break;
            case FBE_ENCL_FAULT_LED_BATTERY_FLT:
                strcat(string, " BBU Fault");
                break;
            case FBE_ENCL_FAULT_LED_BATTERY_RESUME_PROM_FLT:
                strcat(string, " BBU RP Fault");
                break;
            case FBE_ENCL_FAULT_LED_MIDPLANE_RESUME_PROM_FLT:
                strcat(string, " Midplane RP Fault");
                break;
            case FBE_ENCL_FAULT_LED_DRIVE_MIDPLANE_RESUME_PROM_FLT:
                strcat(string, " Drive Midplane RP Fault");
                break;
            case FBE_ENCL_FAULT_LED_LCC_CABLING_FLT:
                strcat(string, " LCC Cabling Fault");
                break;
            case FBE_ENCL_FAULT_LED_EXCEEDED_MAX:
                strcat(string, " Exceed Max Encl Limit");
                break;
            case FBE_ENCL_FAULT_LED_SP_FLT:
                strcat(string, " SP Fault");
                break;
            case FBE_ENCL_FAULT_LED_SP_FAULT_REG_FLT:
                strcat(string, " FaultReg Fault");
                break;
            case FBE_ENCL_FAULT_LED_SYSTEM_SERIAL_NUMBER_FLT:
                strcat(string, " System Serial Number Invalid");
                break;
            case FBE_ENCL_FAULT_LED_MGMT_MODULE_FLT:
                strcat(string, " Management Module Fault");
                break;
            case FBE_ENCL_FAULT_LED_MGMT_MODULE_RESUME_PROM_FLT:
                strcat(string, " Management Module RP Fault");
                break;
            case FBE_ENCL_FAULT_LED_PEER_SP_FLT:
                strcat(string, " Peer SP Fault");
                break;
            case FBE_ENCL_FAULT_LED_CACHE_CARD_FLT:
                strcat(string, " Cache Card Fault");
                break;
            case FBE_ENCL_FAULT_LED_DIMM_FLT:
                strcat(string, " DIMM Fault");
                break;
            case FBE_ENCL_FAULT_LED_SSC_FLT:
                strcat(string, " SSC Fault");
                break;
            case FBE_ENCL_FAULT_LED_INTERNAL_CABLE_FLT:
                strcat(string, " SP Internal Cable Fault");
                break;
            default:
                strcat(string, " Unknown Fault");
                break;
            }   // end of switch
        }   // for bit
    }

    strcat(string, "\0");       // NULL terminate string
    return(string);
}   

/*!**************************************************************
 * @espEnclMgmt::decode_led_status(fbe_led_status_t ledStatus)
 ****************************************************************
 * Description:
 *  This function decodes the LED status.
 *
 * Input: Parameters
 *     ledStatus
 *
 * Returns:
 *    The LED status string.
 *
 *
 ****************************************************************/
char * espEnclMgmt::decode_led_status(fbe_led_status_t ledStatus)
{
    char * statusStr;

    switch(ledStatus)
    {
        case FBE_LED_STATUS_OFF:
            statusStr = "OFF";
            break;

        case FBE_LED_STATUS_ON:
            statusStr = "ON";
            break;

        case FBE_LED_STATUS_MARKED:
            statusStr = "MARKED";
            break;

        case FBE_LED_STATUS_INVALID:
            statusStr = "INVALID";
            break;

        default:
            statusStr = "UNKNOWN";
            break;
    }

    return statusStr;
}

/*!**************************************************************
 * espEnclMgmt Class ::get_encl_type_string()
 ****************************************************************
 * Description:
 *  This function converts the enclosure type to a text string.
 *
 * Input: Parameters
 *        fbe_esp_encl_type_t enclosure_type
 * 
 * Returns:
 *        fbe_char_t *string for enclosure string
 *
 ****************************************************************/

fbe_char_t * espEnclMgmt ::get_encl_type_string(fbe_esp_encl_type_t enclosure_type)
{
    switch(enclosure_type)
    {
        case  FBE_ESP_ENCL_TYPE_INVALID:
            return "Invalid";
        case  FBE_ESP_ENCL_TYPE_BUNKER:
            return "Bunker";
        case  FBE_ESP_ENCL_TYPE_CITADEL:
            return "Citadel";
        case  FBE_ESP_ENCL_TYPE_VIPER:
            return "Viper";
        case FBE_ESP_ENCL_TYPE_DERRINGER:
            return "Derringer";
        case FBE_ESP_ENCL_TYPE_VOYAGER:
            return "Voyager";
        case FBE_ESP_ENCL_TYPE_FOGBOW:
            return "Fogbow";
        case FBE_ESP_ENCL_TYPE_BROADSIDE:
            return "Broadside";
        case FBE_ESP_ENCL_TYPE_RATCHET:
            return "Ratchet";
        case FBE_ESP_ENCL_TYPE_SIDESWIPE:
            return "Sideswipe";
        case FBE_ESP_ENCL_TYPE_FALLBACK:
            return "Stratosphere Fallback";
        case FBE_ESP_ENCL_TYPE_BOXWOOD:
            return "Boxwood";
        case FBE_ESP_ENCL_TYPE_KNOT:
            return "Knot";
        case FBE_ESP_ENCL_TYPE_PINECONE:
            return "Pinecone";
        case  FBE_ESP_ENCL_TYPE_STEELJAW:
            return "Steeljaw";
        case FBE_ESP_ENCL_TYPE_RAMHORN:
            return "Ramhorn";
        case FBE_ESP_ENCL_TYPE_ANCHO:
            return "Ancho";
        case FBE_ESP_ENCL_TYPE_TABASCO:
            return "Tabasco";
        case FBE_ESP_ENCL_TYPE_CAYENNE:
            return "Cayenne";
        case FBE_ESP_ENCL_TYPE_NAGA:
            return "Naga";
        case FBE_ESP_ENCL_TYPE_PROTEUS:
            return "Proteus";
        case FBE_ESP_ENCL_TYPE_TELESTO:
            return "Telesto";
        case FBE_ESP_ENCL_TYPE_CALYPSO:
            return "Calypso";
        case FBE_ESP_ENCL_TYPE_MIRANDA:
            return "Miranda";
        case FBE_ESP_ENCL_TYPE_RHEA:
            return "Rhea";
        case FBE_ESP_ENCL_TYPE_UNKNOWN:
            return "Unknown";
        default:
            return "Invalid Enclosure Type";
    }
}

/*!**************************************************************
 * espEnclMgmt ::get_enclShutdownReasonString()
 ****************************************************************
 * Description
 *  This function converts the enclosure ShutdownReason to
 *  a text string.
 *
 * Input parameters
 *        fbe_u32_t shutdownReason
 * 
 * Returns
 *        fbe_char_t *string for Shutdown Reason string
 *
 ****************************************************************/

fbe_char_t * espEnclMgmt ::get_enclShutdownReasonString(fbe_u32_t shutdownReason)
{
    switch(shutdownReason)
    {
    case FBE_ESES_SHUTDOWN_REASON_NOT_SCHEDULED:
        return "ShutdownNotScheduled";
    case FBE_ESES_SHUTDOWN_REASON_CLIENT_REQUESTED_POWER_CYCLE:
        return "ClientReqPowerCycle";
    case FBE_ESES_SHUTDOWN_REASON_CRITICAL_TEMP_FAULT:
        return "CriticalTempFault";
    case FBE_ESES_SHUTDOWN_REASON_CRITICAL_COOLIG_FAULT:
        return "CriticalCoolingFault";
    case FBE_ESES_SHUTDOWN_REASON_PS_NOT_INSTALLED:
        return "PowerSupplyNotInstalled";
    case FBE_ESES_SHUTDOWN_REASON_UNSPECIFIED_HW_NOT_INSTALLED:
        return "UnspecifiedHwNotInstalled";
    case FBE_ESES_SHUTDOWN_REASON_UNSPECIFIED_REASON:
        return "UnspecifiedReason";
    default:
        return "Invalid ShutdownReason";
    }
}

/*!**************************************************************
 *  espEnclMgmt ::decode_encl_state(fbe_esp_encl_state_t enclState)
 ****************************************************************
 * Description
 *  This function decodes the ESP encl state.
 *
 * Input parameters
 *     enclState - 
 *
 * Returns
 *     the ESP encl state string.
 *
 ****************************************************************/
char *  espEnclMgmt ::decode_encl_state(fbe_esp_encl_state_t enclState)
{
    char *enclStateStr;

    switch(enclState)
    {
        case FBE_ESP_ENCL_STATE_MISSING:
            enclStateStr = "MISSING";
            break;
    
        case FBE_ESP_ENCL_STATE_OK:
            enclStateStr = "OK";
            break;

        case FBE_ESP_ENCL_STATE_FAILED:
            enclStateStr = "FAILED";
            break;

        default:
            enclStateStr = "UNDEFINED";
            break;
    }

    return enclStateStr;
} 

/*!**************************************************************
 * espEnclMgmt ::decode_encl_fault_symptom(fbe_esp_encl_fault_sym_t enclFaultSymptom)
 ****************************************************************
 * Description
 *  This function decodes the ESP encl fault symptom.
 *
 * Input parameters
 *    enclState - 
 *
 * Returns
 *    the ESP encl state string.
 *
 ****************************************************************/
char * espEnclMgmt ::decode_encl_fault_symptom(fbe_esp_encl_fault_sym_t enclFaultSymptom)
{
    char *enclFaultSymStr;

    switch(enclFaultSymptom)
    {
        case FBE_ESP_ENCL_FAULT_SYM_NO_FAULT:
            enclFaultSymStr = "NONE";
            break;
    
        case FBE_ESP_ENCL_FAULT_SYM_LIFECYCLE_STATE_FAIL:
            enclFaultSymStr = "LIFECYCLE STATE FAIL";
            break;

        default:
            enclFaultSymStr = "UNDEFINED";
            break;
    }

    return enclFaultSymStr;
}

