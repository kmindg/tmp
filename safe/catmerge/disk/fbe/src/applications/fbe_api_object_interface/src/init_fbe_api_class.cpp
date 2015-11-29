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
 *
 *      This file defines methods of the initFbeApiClass class. These
 *      methods handle the processing to initialize the k[kernel] mode  
 *      or s[simulator] mode of operation. 
 *
 *      This class also contains methods that do not fit into the 
 *      other Fbe Api classes. Mainly, utility methods of a general 
 *      nature that log and make FBE API function calls.
 *
 *  History:
 *      12-May-2010 : initial version
 *
 *********************************************************************/

#ifndef T_INITFBEAPICLASS_H
#include "init_fbe_api_class.h"
#endif

/*********************************************************************
 * initFbeApiClass Class :: Accessor methods
 *********************************************************************
 *
 *  Description:
 *      This section contains routines that operate on class data.
 *
 *  Input: Parameters
 *      None
 *
 *  Returns:
 *      None
 *
 *  Output: Console
 *      dumpme() - general info about the object 
 *
 *  History
 *      12-May-2010 : initial version
 *
 *********************************************************************/

 void initFbeApiCLASS::dumpme(void) 
{   
    strcpy (key, "initFbeApiCLASS::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n", idnumber, apiCount);
    vWriteFile(key, temp);
}

 /*********************************************************************
 * initFbeApiClass::Init_Simulator ()                             
 *********************************************************************
 *
 *  Description:  
 *      Initialize the simulation side of fbe api. 
 *
 *  Input: Parameters
 *      sp_to_connect - FBE_SIM_SP_A or FBE_SIM_SP_B
 *
 *  Output: Console.
 *      - Error Messages.
 *      - Argument list if debug option is set.
 *
 *  Returns:
 *      fbe_status_t status - 
 *          FBE_STATUS_OK (1) or 
 *          FBE_STATUS_GENERIC_FAILURE 
 *
 *  History:          
 *       12-May-2010 : initial version.
 *
 *********************************************************************/

fbe_status_t initFbeApiCLASS::Init_Simulator (
    fbe_sim_transport_connection_target_t sp_to_connect)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    simulation_mode = FBE_TRUE;
    strcpy (key, "Init_Simulator");

    fbe_trace_init();
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_CRITICAL_ERROR);

    // Initialize the simulation side of fbe api 
    fbe_api_common_init_sim();

    // Set function entries for commands that would go to the physical package
    fbe_api_set_simulation_io_and_control_entries (
        FBE_PACKAGE_ID_PHYSICAL,
        fbe_api_sim_transport_send_io_packet,
        fbe_api_sim_transport_send_client_control_packet);
   
    // Set function entries for commands that would go to the sep package    
    fbe_api_set_simulation_io_and_control_entries (
        FBE_PACKAGE_ID_SEP_0,
        fbe_api_sim_transport_send_io_packet,
        fbe_api_sim_transport_send_client_control_packet);

    // set function entries for commands that would go to the esp package
    fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_ESP,
       fbe_api_sim_transport_send_io_packet,
       fbe_api_sim_transport_send_client_control_packet);

    // Set function entries for commands that would go to the sep package
    fbe_api_set_simulation_io_and_control_entries (
        FBE_PACKAGE_ID_NEIT,
        fbe_api_sim_transport_send_io_packet,
        fbe_api_sim_transport_send_client_control_packet);

    // Initialize the client to connect to the server
    fbe_api_sim_transport_reset_client(FBE_CLIENT_MODE_DEV_PC);
    fbe_api_sim_transport_set_target_server(sp_to_connect);
   
    // Connect w/o any notifications enabled
    status = fbe_api_sim_transport_init_client(sp_to_connect, FBE_TRUE);

    // Check status from fbe_api_sim_transport_init_client
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "<ERROR!> %s\n %s(%d)\n", 
            "Can't connect to FBE Simulator: SP(a:1) or SP(b:2)!!!",
            "Make sure fbesim is running for SP",
            sp_to_connect);
        vWriteFile(key, temp); 
    
    // Check Debug
    } else if (Debug) {
        sprintf(temp, "%s\n sp_to_connect: %d\n status: %d\n",
            "initFbeApiClass simulator initialization complete.",
            sp_to_connect, status);
        vWriteFile(dkey, temp);
    }

    return (status);
}

/*********************************************************************
 * initFbeApiClass::Init_Kernel()                             
 *********************************************************************
 *
 *  Description:  
 *      Initilize the fbe api user for kernel mode processing.
 *
 *  Input: Parameters
 *       sp_to_connect - FBE_SIM_SP_A or FBE_SIM_SP_B
 *          Note: not used - running on SP.
 *
 *  Output: Console.
 *      - Error Messages.
 *      - Argument list if debug option is set.
 *
 *  Returns:
 *      fbe_status_t status - 
 *          FBE_STATUS_OK (1) or
 *          FBE_STATUS_GENERIC_FAILURE 
 *
 *  History:          
 *       12-May-2010 : initial version.
 *
 *********************************************************************/

fbe_status_t initFbeApiCLASS::Init_Kernel (
    fbe_sim_transport_connection_target_t sp_to_connect)
{
     fbe_status_t status;

    simulation_mode = FBE_FALSE;
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_CRITICAL_ERROR);
    status = fbe_api_common_init_user(FBE_TRUE);

    // Check status 
    if (status != FBE_STATUS_OK) {
        sprintf(temp, 
                "%s Failed init user on SP: %d - status: %d \n",
                __FUNCTION__, sp_to_connect, status);
        vWriteFile(key, temp);
        return (status);
        
    // Check Debug
    } else if (Debug) {
        sprintf(temp, "%s\n sp_to_connect: %d\n status: %d\n",
            "initFbeApiClass kernel initialization complete.",
            sp_to_connect, status);
        vWriteFile(dkey, temp);
    }

    // This part takes care of getting notifications from jobs 
    // and letting FBE APIs know the job completed
    status  = fbe_api_common_init_job_notification();
    if (status != FBE_STATUS_OK) {
        sprintf (temp, "%s failed to init notification interface\n",
                __FUNCTION__);
        vWriteFile(key, temp);
        return (status);
    }

    return (status);
}

/*********************************************************************
 * initFbeApiCLASS :: fbe_cli_destroy_fbe_api
 *********************************************************************/

void  initFbeApiCLASS::fbe_cli_destroy_fbe_api(
    fbe_sim_transport_connection_target_t sp_to_connect)
{
    //FBE_UNREFERENCED_PARAMETER(0);


    if (simulation_mode) {
        //destroy job notification must be done before destroy client,
        //since it uses the socket connection. 
        fbe_api_common_destroy_job_notification();
        fbe_api_sim_transport_destroy_client(sp_to_connect);
        fbe_api_common_destroy_sim();
    
    }else{
        fbe_api_common_destroy_user();
    }
    //closeOutputFile(outFileHandle);
    printf("\r");
    fflush(stdout);

    return;
}

/*********************************************************************
 * initFbeApiCLASS :: getObjects 
 *********************************************************************
 *
 *  Description:
 *      getObjects - generates a list of objects
 *
 *  Input: Parameters
 *      getObjects() - none.
 *
 *  Returns:
 *      getObjects() - FBE_STATUS_OK or <error code>
 *
 *  Output: Console
 *      getObjects() - List of object ids with associated class codes
 *      and names.
 *
 *  History
 *      07-March-2011 : initial version
 *
 *********************************************************************/

fbe_status_t initFbeApiCLASS::getObjects(void)
{
    fbe_class_id_t          class_id;
    fbe_const_class_info_t* p_class_info;
    fbe_object_id_t *       object_list = NULL;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_u32_t               ii = 0;
    fbe_u32_t               object_count = 0;
    fbe_u32_t               total_objects =0;
    const fbe_u8_t *        p_class_name;

    //fbe_lifecycle_state_t   lifecycle_state;
    //const fbe_u8_t *        p_lifecycle_state_name;
    
    // Get number of objects
    status = fbe_api_get_total_objects(
        &object_count, FBE_PACKAGE_ID_PHYSICAL);
    
    if (status != FBE_STATUS_OK) 
    {
        strcpy (key, "getObjects [fbe_api_get_total_objects]");
        sprintf(temp, "<ERROR!> call status [%x] %s\n",
            (unsigned) status, "Can't get object count");
        vWriteFile(key, temp);
        return status;
    }

    // Allocate memory
    object_list = (fbe_object_id_t *) fbe_api_allocate_memory(
        sizeof(fbe_object_id_t) * object_count);

    // Store pointer to structure in object_list
    status = fbe_api_enumerate_objects (
        object_list, 
        object_count, 
        &total_objects, 
        FBE_PACKAGE_ID_PHYSICAL);

    // Failure ... release memory and return
    if (status != FBE_STATUS_OK) 
    {
        fbe_api_free_memory(object_list);

        strcpy (key, "getObjects [fbe_api_allocate_memory]");
        sprintf(temp, "<ERROR!> %s\n call status [%x] %s\n", 
            "fileUtilClass::getObjects",
            (unsigned) status, 
            "Can't allocate memory");
        vWriteFile(key, temp);
        return status;
    }

    // Return object Id, class Id, and class name 
    strcpy (key, "Objects Discovered");
    
    for (ii = 0; ii < total_objects; ii++) 
    {
        // set default values
        p_class_name = (fbe_u8_t *) "<error>";
        class_id = (fbe_class_id_t) 0;

        // Get class Id
        status = fbe_api_get_object_class_id(
            object_list[ii], 
            &class_id, 
            FBE_PACKAGE_ID_PHYSICAL);

        // If class Id call OK, get class info 
        if (status == FBE_STATUS_OK) 
        {
            status = fbe_get_class_by_id(class_id, &p_class_info);

            if (status == FBE_STATUS_OK) {
                p_class_name = (fbe_u8_t *) p_class_info->class_name;
            }
        }

        // Output line
        sprintf(temp, "<%s> 0x%02X <%s> 0x%02X <%s> %-20s\n", 
            "Object Id",  object_list[ii], 
            "Class Type", class_id, 
            "Class name", p_class_name);
        vWriteFile(key, temp);

        // Set key to blank to list remaining data under 1 key.
        strcpy (key, "");
    }
    
    // release memory - OK
    fbe_api_free_memory(object_list);
    return status;

}

/*********************************************************************
 * initFbeApiCLASS end of file
 *********************************************************************/
