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
*      This file defines the methods of the 
       SEP METADATA INTERFACE class.
*
*  Notes:
*      The SEP Metadata class (sepMetadata) is a derived class of 
*      the base class (bSEP).
*
*  History:
*      27-July-2011 : initial version
*
*********************************************************************/

#ifndef T_SEP_METADATA_CLASS_H
#include "sep_metadata_class.h"
#endif

/*********************************************************************
* sepMetadata class :: Constructor
*********************************************************************/

sepMetadata::sepMetadata() : bSEP()
{
    // Store Object number
    idnumber = (unsigned) SEP_METADATA_INTERFACE;
    sepMetadataCount = ++gSepMetadataCount;

    // Store Object name
    idname = new char [IDNAME_LENGTH];
    strcpy(idname, "SEP_METADATA_INTERFACE");

    if (Debug) {
        sprintf(temp, 
            "sepMetadata class constructor (%d) %s\n", 
            idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API SEP Metadata Interface function calls>
    sepMetadataInterfaceFuncs = 
        "[function call]\n" \
        "[short name] [FBE API SEP Metadata Interface]\n" \
        " --------------------------------------------------------\n" \
        " setAllObjState    fbe_api_metadata_set_all_objects_state\n" \
        " setSingleObjState fbe_api_metadata_set_single_object_state\n" \
        " --------------------------------------------------------\n";

    // Define common usage for metadata commands
    usage_format = 
        " Usage: %s [object id]\n"
        " For example: %s 4";
};

/*********************************************************************
* sepMetadata class : Accessor methods
*********************************************************************/

unsigned sepMetadata::MyIdNumIs(void)
{
    return idnumber;
};

char * sepMetadata::MyIdNameIs(void)
{
    return idname;
};

void sepMetadata::dumpme(void)
{ 
    strcpy (key, "sepMetadata::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n", 
            idnumber, sepMetadataCount);
    vWriteFile(key, temp);
}

/*********************************************************************
* sepMetadata Class :: select()
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
*      29-Mar-2011 : initial version
*
*********************************************************************/

fbe_status_t sepMetadata::select(int index, int argc, char *argv[]) 
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select SEP interface"); 

    // List SEP Metadata calls if help option and less than 6
    // arguments were entered.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) sepMetadataInterfaceFuncs );

        // set all objects state
    } else if (strcmp (argv[index], "SETALLOBJSTATE") == 0) {
        status = set_all_objects_state(argc, &argv[index]);

        // set single object state
    } else if (strcmp (argv[index], "SETSINGLEOBJSTATE") == 0) {
        status = set_single_object_state(argc, &argv[index]);
       
        // can't find match on short name
    } else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n", 
                BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) sepMetadataInterfaceFuncs );
    } 

    return status;
}

/*********************************************************************
* sepMetadata Class :: set_all_objects_state()
*********************************************************************
*
*  Description:
*      Set the state of all objects
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepMetadata::set_all_objects_state(
    int argc, 
    char **argv) 
{
    // Assign default values
    obj_state = FBE_METADATA_ELEMENT_STATE_INVALID;
    
    // Define specific usage string  
    usage_format =
        " Usage: %s [state]\n"
        " state: active | passive\n"
        " For example: %s active\n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "setAllObjState",
        "sepMetadata::set_all_objects_state",
        "fbe_api_metadata_set_all_objects_state",  
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Extract the state
    argv++;
    obj_state = utilityClass::convert_string_to_object_state(*argv);
    if (FBE_METADATA_ELEMENT_STATE_INVALID == obj_state){
        sprintf(temp, "The entered object state (%s) is invalid",
                      *argv);
        status = FBE_STATUS_GENERIC_FAILURE;
        params[0] = '\n';
        call_post_processing(status, temp, key, params);
        return status;
    }
    sprintf(params, " objects state 0x%x (%s)", obj_state, *argv);
    
    // Make api call: 
    status = fbe_api_metadata_set_all_objects_state(obj_state);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to set the state for "
                      "all objects to 0x%X (%s)",
                      obj_state, 
                      utilityClass::convert_object_state_to_string(obj_state));

    }else if (status == FBE_STATUS_OK){ 
        sprintf(temp, "Objects have been set to state 0x%X (%s)",
                      obj_state, 
                      utilityClass::convert_object_state_to_string(obj_state));
    }     

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepMetadata Class :: set_single_object_state()
*********************************************************************
*
*  Description:
*      Set the state of single object
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepMetadata::set_single_object_state(
    int argc, 
    char **argv) 
{
    // Assign default values
    object_id = FBE_OBJECT_ID_INVALID;
    obj_state = FBE_METADATA_ELEMENT_STATE_INVALID;
    
    // Define specific usage string  
    usage_format =
        " Usage: %s [object id] [state]\n"
        " state: active | passive\n"
        " For example: %s 4 active\n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "setSingleObjState",
        "sepMetadata::set_single_object_state",
        "fbe_api_metadata_set_single_object_state",  
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " object id 0x%x (%s)", object_id, *argv);

    // Extract the state
    argv++;
    obj_state = utilityClass::convert_string_to_object_state(*argv);
    if (FBE_METADATA_ELEMENT_STATE_INVALID == obj_state){
        sprintf(temp, "The entered object state (%s) is invalid",
                      *argv);
        status = FBE_STATUS_GENERIC_FAILURE;
        params[0] = '\n';
        call_post_processing(status, temp, key, params);
        return status;
    }
    sprintf(params_temp, " object state 0x%x (%s)", obj_state, *argv);
    if(strlen(params_temp) > MAX_PARAMS_SIZE)
    {
        sprintf(temp, "<ERROR!> Params length is larger that buffer value");
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }
    strcat(params, params_temp);

    // Make api call: 
    status = fbe_api_metadata_set_single_object_state(
        object_id, 
        obj_state);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to set the state to 0x%X (%s) for "
                      "object id 0x%X", 
                      obj_state,  
                      utilityClass::convert_object_state_to_string(obj_state),
                      object_id);

    }else if (status == FBE_STATUS_OK){ 
        sprintf(temp, "Set the state to 0x%X (%s) for object id 0x%X", 
                      obj_state, 
                      utilityClass::convert_object_state_to_string(obj_state),
                      object_id);
    }     

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepMetadata::Sep_Metadata_Intializing_variable (private method)
*********************************************************************/
void sepMetadata::Sep_Metadata_Intializing_variable()
{
    params_temp[0] = '\0';
    object_id = FBE_OBJECT_ID_INVALID;
    obj_state = FBE_METADATA_ELEMENT_STATE_INVALID;

    return;
}

/*********************************************************************
* sepMetadata end of file
*********************************************************************/