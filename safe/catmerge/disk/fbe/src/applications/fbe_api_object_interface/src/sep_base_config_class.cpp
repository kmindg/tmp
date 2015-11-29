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
       SEP BASE CONFIG INTERFACE class.
*
*  Notes:
*      The SEP Base Config class (sepBaseConfig) is a derived class of 
*      the base class (bSEP).
* 
*  Created By : Sonali Kudtarkar 
*  History:
*      18-Oct-2011 : initial version
*
*********************************************************************/

#ifndef T_SEP_BASE_CONFIG_CLASS_H
#include "sep_base_config_class.h"
#endif

/*********************************************************************
* sepBaseConfig class :: Constructor
*********************************************************************/

sepBaseConfig::sepBaseConfig() : bSEP()
{
    // Store Object number
    idnumber = (unsigned) SEP_BASE_CONFIG_INTERFACE;
    sepBaseConfigCount = ++gSepBaseConfigCount;

    // Store Object name
    idname = new char [IDNAME_LENGTH];
    strcpy(idname, "SEP_BASE_CONFIG_INTERFACE");

    Sep_Base_Intializing_variable();

    if (Debug) {
        sprintf(temp, 
            "sepBaseConfig class constructor (%d) %s\n", 
            idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API SEP BaseConfig Interface function calls>
    sepBaseConfigInterfaceFuncs = 
        "[function call]\n" \
        "[short name] [FBE API SEP BaseConfig Interface]\n" \
        " --------------------------------------------------------\n" \
        " enableBgOp            fbe_api_base_config_enable_background_operation\n" \
        " disableBgOp           fbe_api_base_config_disable_background_operation\n" \
        " getBgOpStatus         fbe_api_base_config_is_background_operation_enabled\n"\
        " getUpstreamObj        fbe_api_base_config_get_upstream_object_list\n"\
        " getDownstreamObj      fbe_api_base_config_get_downstream_object_list\n"\
        " --------------------------------------------------------\n" \
        " getMetadataInfo       fbe_api_base_config_metadata_get_info\n"\
        " clearMetadataCache    fbe_api_base_config_metadata_paged_clear_cache\n"\
        " --------------------------------------------------------\n" \
        " postPassiveObjRequest fbe_api_base_config_passive_request\n" \
        " --------------------------------------------------------\n";
    // Define common usage for base config commands
    usage_format = 
        " Usage: %s [object id] %s [Background Operation]\n"
        " For example: %s 4 2\n";
};

/*********************************************************************
* sepDatabase class : Accessor methods
*********************************************************************/

unsigned sepBaseConfig::MyIdNumIs(void)
{
    return idnumber;
};

char * sepBaseConfig::MyIdNameIs(void)
{
    return idname;
};

void sepBaseConfig::dumpme(void)
{ 
    strcpy (key, "sepBaseConfig::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n", 
            idnumber, sepBaseConfigCount);
    vWriteFile(key, temp);
}

/*********************************************************************
* sepBaseConfig Class :: select()
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
*      18-Oct-2011 : initial version
*
*********************************************************************/

fbe_status_t sepBaseConfig::select(int index, int argc, char *argv[]) 
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select SEP interface"); 

    // List sep Base Config calls if help option and less than 6
    // arguments were entered.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) sepBaseConfigInterfaceFuncs );

         // enable BG Operation
    } else if (strcmp (argv[index], "ENABLEBGOP") == 0) {
        status = enable_BgOp(argc, &argv[index]);

        // disable BG Operation
    }else if (strcmp (argv[index], "DISABLEBGOP") == 0) {
        status = disable_BgOp(argc, &argv[index]);

        // Is BG Operation enabled?
    }else if (strcmp (argv[index], "GETBGOPSTATUS") == 0) {
        status = get_BgOp_Status(argc, &argv[index]);

        // Gets upstream object list
    }else if (strcmp (argv[index], "GETUPSTREAMOBJ") == 0) {
        status = get_upstream_object_list(argc, &argv[index]);

        // Gets downstream object list
    }else if (strcmp (argv[index], "GETDOWNSTREAMOBJ") == 0) {
        status = get_downstream_object_list(argc, &argv[index]);

        // Get metada information
    }else if (strcmp (argv[index], "GETMETADATAINFO") == 0) {
        status =  get_metadata_info(argc, &argv[index]);

        // Post passive object request
    }else if (strcmp (argv[index], "CLEARMETADATACACHE") == 0) {
        status = clear_metadata_cache(argc, &argv[index]);
        
    }else if (strcmp (argv[index], "POSTPASSIVEOBJREQUEST") == 0) {
        status = post_passive_obj_request(argc, &argv[index]);

    } else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n", 
                BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) sepBaseConfigInterfaceFuncs );
    } 

    return status;
}

/*********************************************************************
* sepBaseConfig class :: enable_BgOp ()
*********************************************************************/

fbe_status_t sepBaseConfig::enable_BgOp(int argc , char ** argv)
{
    // Assign default values
    object_id = FBE_STATUS_GENERIC_FAILURE;

    // Define specific usage string  
    usage_format =
        " Usage: %s [Object ID] [Background Operation]\n"
        " For example: %s 1 2 \n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "enableBgOp",
        "sepBaseConfig::enable_BgOp",
        "fbe_api_base_config_enable_background_operation",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object ID to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);

    argv++;
    background_op = (fbe_u16_t)strtoul(*argv, 0, 0);

    sprintf(params, " Object ID 0x%x (%u) fbe_u16_t (%d)",
            object_id, object_id, background_op);

    // Make api call: 
    status = fbe_api_base_config_enable_background_operation(
        object_id, background_op);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Could not enable BG Operation on Obj ID %d", object_id);
    } else {
        sprintf(temp, "Background Operation Enabled successfully!");
    }
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepBaseConfig class :: disable_BgOp ()
*********************************************************************/

fbe_status_t sepBaseConfig::disable_BgOp(int argc , char ** argv)
{
    // Assign default values
    object_id = FBE_STATUS_GENERIC_FAILURE;

    // Define specific usage string  
    usage_format =
        " Usage: %s [Object ID] [Background Operation]\n"
        " For example: %s 1 2 \n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "disableBgOp",
        "sepBaseConfig::disable_BgOp",
        "fbe_api_base_config_disable_background_operation",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object ID to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);

    argv++;
    background_op = (fbe_u16_t)strtoul(*argv, 0, 0);

    sprintf(params, " Object ID 0x%x (%u) fbe_u16_t (%d)",
            object_id, object_id, background_op);

    // Make api call: 
    status = fbe_api_base_config_disable_background_operation(
        object_id, background_op);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Could not disable BG Operation on Obj ID %d", object_id);
    } else {
        sprintf(temp, "Background Operation Disabled successfully!");
    }
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepBaseConfig class :: get_BgOp_Status () 
*********************************************************************/

fbe_status_t sepBaseConfig::get_BgOp_Status(int argc , char ** argv)
{
    // Assign default values
    object_id = FBE_STATUS_GENERIC_FAILURE;

    // Define specific usage string  
    usage_format =
        " Usage: %s [Object ID] [Background Operation]\n"
        " For example: %s 1 2 \n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getBgOpStatus",
        "sepBaseConfig::get_BgOp_Status",
        "fbe_api_base_config_is_background_operation_enabled",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object ID to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
   
    argv++;
    background_op = (fbe_u16_t)strtoul(*argv, 0, 0);
    
    sprintf(params, " Object ID 0x%x (%u) fbe_u16_t (%d)",
            object_id, object_id, background_op);

    // Make api call: 
    status = fbe_api_base_config_is_background_operation_enabled(
        object_id, background_op, &is_enabled);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Could not get BG Operation status on Obj ID %d", object_id);
    } else {
        if (is_enabled) {
            sprintf(temp, "Background Operation is Enabled!");
        }else {
            sprintf(temp, "Background Operation is Disabled!");
        }
    }
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepBaseConfig class :: get_upstream_object_list ()
*********************************************************************/
fbe_status_t sepBaseConfig::get_upstream_object_list(int argc , char ** argv)
{
    // Assign default values
    object_id = FBE_STATUS_GENERIC_FAILURE;

    // Define specific usage string
    usage_format =
        " Usage: %s [Object ID] \n"
        " For example: %s 1 \n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getUpstreamObj",
        "sepBaseConfig::get_upstream_object_list",
        "fbe_api_base_config_get_upstream_object_list",
        usage_format,
        argc, 7);

    char *data = temp;

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object ID to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);

    sprintf(params, " Object ID 0x%x (%u) ",
            object_id, object_id);

    // Make api call:
    status = fbe_api_base_config_get_upstream_object_list(object_id,
                                                          &upstream_object_list_p);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(data, "Could not get upstream object id on Obj ID %d", object_id);
    } else {
        data = edit_upstream_object_list(&upstream_object_list_p);
    }
    // Output results from call or description of error
    call_post_processing(status, data, key, params);
    return status;
}

/*********************************************************************
* sepBaseConfig class :: get_downstream_object_list () 
*********************************************************************/
fbe_status_t sepBaseConfig::get_downstream_object_list(int argc , char ** argv)
{
    // Assign default values
    object_id = FBE_STATUS_GENERIC_FAILURE;

    // Define specific usage string  
    usage_format =
        " Usage: %s [Object ID] \n"
        " For example: %s 1 \n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getDownstreamObj",
        "sepBaseConfig::get_downstream_object_list",
        "fbe_api_base_config_get_downstream_object_list",
        usage_format,
        argc, 7);

    char *data = temp;

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object ID to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    
    sprintf(params, " Object ID 0x%x (%u)",
            object_id, object_id);

    // Make api call: 
    status = fbe_api_base_config_get_downstream_object_list(object_id,
                                                          &downstream_object_list_p);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(data, "Could not get get_downstream object id on Obj ID %d", object_id);
    } else {
        data = edit_downstream_object_list(&downstream_object_list_p);
    }
    // Output results from call or description of error
    call_post_processing(status, data, key, params);
    return status;
}


/*********************************************************************
* sepBaseConfig class :: edit_upstream_object_list ()
*********************************************************************/
char * sepBaseConfig::edit_upstream_object_list(
    fbe_api_base_config_upstream_object_list_t * upstream_object_list_p)
{
    char *mytemp = NULL,*data = NULL;

    sprintf(temp, "\n <Number of upstream objects>  %d\n",
        upstream_object_list_p->number_of_upstream_objects);
    data = utilityClass::append_to (data, temp);

    sprintf (temp, " <Upstream object list>         ");
    data = utilityClass::append_to (data, temp);

    for(index=0;index < upstream_object_list_p->number_of_upstream_objects; index++)
    {
        mytemp = new char[50];
        sprintf(mytemp, "0x%x  "
            ,upstream_object_list_p->upstream_object_list[index]);
        data = utilityClass::append_to(data, mytemp);
        delete [] mytemp;
    }
    return (data);
}

/*********************************************************************
* sepBaseConfig class :: edit_downstream_object_list ()
*********************************************************************/
char * sepBaseConfig::edit_downstream_object_list(
    fbe_api_base_config_downstream_object_list_t * downstream_object_list_p)
{
    char *mytemp = NULL,*data = NULL;

    sprintf(temp, "\n <Number of Downstream objects>  %d\n"
        " <Downstream object list>  ",
        downstream_object_list_p->number_of_downstream_objects);
    data = utilityClass::append_to (data, temp);

    for(index=0;index < downstream_object_list_p->number_of_downstream_objects; index++)
    {
        mytemp = new char[50];
        sprintf(mytemp, "0x%x  "
            ,downstream_object_list_p->downstream_object_list[index]);
        data = utilityClass::append_to(data, mytemp);
        delete [] mytemp;
    }
    return (data);
}

/*********************************************************************
*  sepBaseConfig::Sep_Base_Intializing_variable (private method)
*********************************************************************/
void sepBaseConfig::Sep_Base_Intializing_variable()
{
    object_id = FBE_OBJECT_ID_INVALID;
    is_enabled = FBE_FALSE;
    index = 0;
    background_op = (fbe_base_config_background_operation_t)0;

    fbe_zero_memory(&upstream_object_list_p,sizeof(upstream_object_list_p));
    fbe_zero_memory(&downstream_object_list_p,sizeof(downstream_object_list_p));

    return;
}

/*********************************************************************
* sepBaseConfig class :: get_metadata_info(int argc, char **argv)
*********************************************************************
*
*  Description:
*      Get metadata info
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t sepBaseConfig:: get_metadata_info(int argc , char ** argv)
{
    // Define specific usage string  
    usage_format =
        " Usage: %s [Object ID] \n"
        " For example: %s 0x10f \n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getMetadataInfo",
        "sepBaseConfig::get_metadata_info",
        "fbe_api_base_config_metadata_get_info",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object ID to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
       
    sprintf(params, " Object ID 0x%x (%u) ", object_id, object_id);

    // Make api call: 
    status = fbe_api_base_config_metadata_get_info( object_id, &metadata_info);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to get metadata information for object 0x%x", object_id);
    } else {
        sprintf(temp, "\n <Metadata status>    %s", edit_metadata_info(
            metadata_info.metadata_element_state));
    }
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;

}

/*********************************************************************
* sepBaseConfig class :: edit_metadata_info(fbe_metadata_element_state_t   m_info)
*********************************************************************
*
*  Description:
*      edit the metadata info to display.
*
*  Input: Parameters
*      m_info - metadata element state
*
*  Returns:
*              char*
*********************************************************************/

char* sepBaseConfig:: edit_metadata_info( fbe_metadata_element_state_t   m_info)
{
    switch(m_info){
        case FBE_METADATA_ELEMENT_STATE_ACTIVE:
            return "FBE_METADATA_ELEMENT_STATE_ACTIVE";
        case FBE_METADATA_ELEMENT_STATE_PASSIVE:
            return "FBE_METADATA_ELEMENT_STATE_PASSIVE";
        default:
            return "FBE_METADATA_ELEMENT_STATE_INVALID";
    }

}

/*********************************************************************
* sepBaseConfig class :: clear_metadata_cache(int argc, char **argv)
*********************************************************************
*
*  Description:
*      Clear metadata cache
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t sepBaseConfig:: clear_metadata_cache(int argc , char ** argv)
{
    // Define specific usage string  
    usage_format =
        " Usage: %s [Object ID] \n"
        " For example: %s 0x10f \n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "clearMetadataCache",
        "sepBaseConfig::clear_metadata_cache",
        "fbe_api_base_config_metadata_paged_clear_cache",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object ID to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
       
    sprintf(params, " Object ID 0x%x (%u) ", object_id, object_id);

    // Make api call: 
    status = fbe_api_base_config_metadata_paged_clear_cache( object_id);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to clear metadata cache for object 0x%x", object_id);
    } else {
        sprintf(temp, "\nMetadata cache cleared for ObjectId 0x%x\n",object_id);
    }
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;

}

/*********************************************************************
* sepBaseConfig class :: post_passive_obj_request () 
*
*  Description:
*      Verify object's availability by its ID.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK, if the object is available, or any other status code
*      from fbe_types.h.
*********************************************************************/
fbe_status_t sepBaseConfig::post_passive_obj_request(int argc , char ** argv)
{
    // Assign default values
    object_id = FBE_STATUS_GENERIC_FAILURE;

    // Define specific usage string  
    usage_format =
        " Usage: %s [Object ID] \n"
        " For example: %s 1 \n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "postPassiveObjRequest",
        "sepBaseConfig::post_passive_obj_request",
        "fbe_api_base_config_passive_request",
        usage_format,
        argc, 7);

    char *data = temp;

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object ID to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    
    sprintf(params, " Object ID 0x%x (%u) ",
            object_id, object_id);

    // Make api call: 
    status = fbe_api_base_config_passive_request(object_id);

    // Check status of call
    if (status == FBE_STATUS_OK) {
        sprintf(data, "Successfully sent passive request to object 0x%x",
                object_id);
    } else {
        sprintf(data, "Failed to send passive request to object 0x%x",
                object_id);
    }
    // Output results from call or description of error
    call_post_processing(status, data, key, params);
    return status;
}
