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
*      This file defines the methods of the ESP EIR class.
*
*  Notes:
*      The ESP EIR class (espEir) is a derived class of 
*      the base class (bESP).
*
*  History:
*      18-July-2011 : initial version
*
*********************************************************************/

#ifndef T_ESP_EIR_CLASS_H
#include "esp_eir_class.h"
#endif

/*********************************************************************
* espEir class :: Constructor
*********************************************************************/
espEir::espEir() : bESP()
{
    // Store Object number
    idnumber = (unsigned) ESP_EIR_INTERFACE;
    espEirCount = ++gEspEirCount;

    // Store Object name
    idname = new char [IDNAME_LENGTH];
    strcpy(idname, "ESP_EIR_INTERFACE");


    //initializing variables
    esp_eir_initializing_variable();


    if (Debug) {
        sprintf(temp, "espEir class constructor (%d) %s\n", 
                idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API ESP EIR Interface function calls>
    espEirInterfaceFuncs = 
        "[function call]\n" \
        "[short name]     [FBE API ESP EIR Interface]\n" \
        " ----------------------------------------------------------\n" \
        " getData         fbe_api_esp_eir_getData\n" \
        " -----------------------------------------------------------\n";
    
    // Define common usage for ESP EIR commands
    usage_format = 
        " Usage: %s\n"
        " For example: %s";
};

/*********************************************************************
* espEir class : Accessor methods
*********************************************************************/

unsigned espEir::MyIdNumIs(void)
{
    return idnumber;
};

char * espEir::MyIdNameIs(void)
{
    return idname;
};
void espEir::dumpme(void)
{ 
    strcpy (key, "espEir::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n", 
            idnumber, espEirCount);
    vWriteFile(key, temp);
};

/*********************************************************************
* espEir Class :: select(int index, int argc, char *argv[])
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
*      18-July-2011 : initial version
*
*********************************************************************/

fbe_status_t espEir::select(int index, int argc, char *argv[]) 
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select ESP interface"); 

    // List ESP EIR Interface calls if help option and 
    // less than 6 arguments were entered.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) espEirInterfaceFuncs );

        // get eir info
    } else if (strcmp (argv[index], "GETDATA") == 0) {
        status = get_esp_eir_data(argc, &argv[index]);

        // can't find match on short name
    } else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n", 
                BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) espEirInterfaceFuncs );
    } 

    return status;
}

/*********************************************************************
* espEir Class :: get_esp_eir_data(int argc, char ** argv)
*********************************************************************
*
*  Description:
*      Gets the data from the ESP eir object.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

 fbe_status_t espEir::get_esp_eir_data(int argc, char ** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getData",
        "espEir::get_esp_eir_data",
        "fbe_api_esp_eir_getData",
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    // Make api call: 
    status = fbe_api_esp_eir_getData(&fbe_eir_data);
    
    if(status != FBE_STATUS_OK) {
        sprintf(temp,"Failed to retrive data.\n");
    }
    else {
 
        sprintf(temp,"\n"
            "<Status>        0x%x (%s)\n"
            "<Input Power>   %d Watts\n",
            fbe_eir_data.arrayInputPower.status, 
            utilityClass::convert_power_status_number_to_string(
                fbe_eir_data.arrayInputPower.status), 
            fbe_eir_data.arrayInputPower.inputPower);

        #if 0
        // The Other fields can also be displayed.
        sprintf(data, "\n"
            "<EIR loop count>       %d\n"
            "<Array EIR data>       0x%x\n",
            fbe_eir_data.eir_loop_count,
            fbe_eir_data.eir_loop_count, fbe_eir_data.some_data );

            strcat(temp,data);

            edit_encl_data(&fbe_eir_data);
            edit_eir_sps_data(&fbe_eir_data);
        #endif
    }

    params[0] = '\0';
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* espEir Class :: edit_encl_data(fbe_eir_data_t* eir_data) (private method)
*********************************************************************
*
*  Description:
*      Format the EIR enclosure information to display
*
*  Input: Parameters
*      eir_data
*
*  Returns:
*      void
*********************************************************************/

void espEir::edit_encl_data(fbe_eir_data_t* eir_data)
{
    int i=0;
    strcat(temp, " <Enclosure data>\n");

    for(i=0; i<FBE_EIR_MAX_ENCL_COUNT; i++){
        sprintf(encl_data,"\n"
            "<Enclosure>  %d\n\n"
            "<Current Input Power status>      0x%x\n"
            "<Current Input Power>                     %d\n\n"
            "<Average Input Power status>      0x%x\n"
            "<Average Input power>                     %d\n\n"
            "<Maximum Input Power status>      0x%x\n"
            "<Maximum Input power>                     %d\n\n"
            "<Current Air Inlet temp status>   0x%x\n"
             "<Current Air Inlet temp>                 %d\n\n"
            "<Average Air Inlet temp status>   0x%x\n"
            "<Average Air Inlet temp>                  %d\n\n"
            "<Maximum Air Inlet temp status>   0x%x\n"
            "<Maximum Air Inlet temp>                  %d\n\n",
            (i+1),
            eir_data->encl_data[i].enclCurrentInputPower.status,
            eir_data->encl_data[i].enclCurrentInputPower.inputPower,
            eir_data->encl_data[i].enclAverageInputPower.status,
            eir_data->encl_data[i].enclAverageInputPower.inputPower,
            eir_data->encl_data[i].enclMaxInputPower.status,
            eir_data->encl_data[i].enclMaxInputPower.inputPower,
            eir_data->encl_data[i].currentAirInletTemp.status,
            eir_data->encl_data[i].currentAirInletTemp.airInletTemp,
            eir_data->encl_data[i].averageAirInletTemp.status,
            eir_data->encl_data[i].averageAirInletTemp.airInletTemp,
            eir_data->encl_data[i].maxAirInletTemp.status,
            eir_data->encl_data[i].maxAirInletTemp.airInletTemp );
        
            strcat(temp, encl_data);
    }

}

/*********************************************************************
* espEir Class :: edit_eir_sps_data(fbe_eir_data_t* eir_data) (private method)
*********************************************************************
*
*  Description:
*      Format the EIR sps information to display
*
*  Input: Parameters
*      eir_data
*
*  Returns:
*      void
*********************************************************************/

void espEir::edit_eir_sps_data(fbe_eir_data_t * eir_data)
{
    int i =0;
    char sp [4];

    strcat(temp, "\n <SPS EIR Data>\n");
    
    for (i=0; i<FBE_SPS_MAX_COUNT; i++) {
        if( i == 0){
            sprintf(sp, "SPA");
        }else if(i ==1){
            sprintf(sp, "SPB");
        }
            
        sprintf(sps_data,"\n"
            "<SP>  %s\n"
            "<Current Input Power status>     0x%x\n"
            " <Current Input power>                   %d\n\n"
            "<Average Input Power status>     0x%x\n"
            "<Average Input power>                    %d\n\n"
            "<Maximum Input Power status>     0x%x\n"
            "<Maximum Input power>                    %d\n\n",
            sp,
            eir_data->sps_data[i].spsCurrentInputPower.status,
            eir_data->sps_data[i].spsCurrentInputPower.inputPower,
            eir_data->sps_data[i].spsAverageInputPower.status,
            eir_data->sps_data[i].spsAverageInputPower.inputPower,
            eir_data->sps_data[i].spsMaxInputPower.status,
            eir_data->sps_data[i].spsMaxInputPower.inputPower );

            strcat(temp, sps_data);
    }
}

/*********************************************************************
* espEir Class :: esp_eir_initializing_variable (private method)
*********************************************************************/
void espEir :: esp_eir_initializing_variable ()
{
    fbe_zero_memory( &fbe_eir_data, sizeof( fbe_eir_data));
}


