/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_drive_mgmt_dieh.c
 ***************************************************************************
 * @brief
 *  This file contains the implemention for DIEH (Drive Improved Error Handling).
 *
 * @ingroup drive_mgmt_class_files
 *
 * @version
 *   03/15/2012:  Wayne Garrett -- created.
 ***************************************************************************/

 /*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_event_log_api.h"
#include "EspMessages.h"
#include "fbe_drive_mgmt_xml_api.h"
#include "fbe_drive_mgmt_private.h"
#include "fbe_drive_mgmt_dieh_xml.h"


 /***************************************
 * Definitions
 ***************************************/

 /***************************************
 * Function Prototypes
 ***************************************/

static fbe_dieh_load_status_t   dmo_dieh_parse_xml_file(fbe_drive_mgmt_t * drive_mgmt, char* pFilename);
static fbe_status_t             fbe_drive_mgmt_dieh_replace_filename(fbe_drive_mgmt_t *drive_mgmt, fbe_u8_t * pathfilename, fbe_u32_t buff_len, fbe_u8_t * newfilename);

 /***************************************
 * Functions
 ***************************************/


/*!**************************************************************
 * @fn fbe_drive_mgmt_dieh_init
 ****************************************************************
 * @brief
 *  This function initializes the DIEH (Drive Improved Error Handling)
 *  feature
 * 
 * @param drive_mgmt - pointer to drive mgmt object.
 * @param packet - pointer to packet.
 * 
 * @return fbe_dieh_load_status_t - status of load
 *
 * @version
 *   03/15/2012:  Wayne Garrett - Created.
 *
 ****************************************************************/
fbe_dieh_load_status_t 
fbe_drive_mgmt_dieh_init(fbe_drive_mgmt_t *drive_mgmt, fbe_u8_t * file)
{   
    fbe_dieh_load_status_t parsing_status;
    fbe_u8_t pathfilename[FBE_DIEH_PATH_LEN] = {0};
    fbe_status_t status;

    /* Initialize DIEH XML data structures. */
    fbe_zero_memory(drive_mgmt->xml_parser, sizeof(fbe_dmo_thrsh_parser_t));
   
    fbe_drive_mgmt_dieh_get_path(pathfilename, FBE_DIEH_PATH_LEN);

    /* Replace filename if provided */
    if( NULL != file && strlen(file) > 0)  
    {   
        status = fbe_drive_mgmt_dieh_replace_filename(drive_mgmt, pathfilename, FBE_DIEH_PATH_LEN, file);
        if (status != FBE_STATUS_OK)
        {
            parsing_status = FBE_DMO_THRSH_ERROR;
            fbe_event_log_write(ESP_ERROR_DIEH_LOAD_FAILED, NULL, 0, "%d", parsing_status);
            return parsing_status;
        }
    }

    parsing_status = dmo_dieh_parse_xml_file(drive_mgmt, pathfilename);

    if (FBE_DMO_THRSH_NO_ERROR == parsing_status)
    {
        fbe_event_log_write(ESP_INFO_DIEH_LOAD_SUCCESS, NULL, 0, NULL);

        /* Update XML info after loaded successfully */
        fbe_copy_memory(drive_mgmt->drive_config_xml_info.version, drive_mgmt->xml_parser->xml_info.version, sizeof(drive_mgmt->drive_config_xml_info.version));
        fbe_copy_memory(drive_mgmt->drive_config_xml_info.description, drive_mgmt->xml_parser->xml_info.description, sizeof(drive_mgmt->drive_config_xml_info.description));
        fbe_copy_memory(drive_mgmt->drive_config_xml_info.filepath, pathfilename, sizeof(drive_mgmt->drive_config_xml_info.filepath));
    }
    else
    {    
        /* TODO: should set packet control_status (in calling function) to indicate why we failed, so error can be
       displayed to user*/

        fbe_event_log_write(ESP_ERROR_DIEH_LOAD_FAILED, NULL, 0, "%d", parsing_status);
    }

    return parsing_status;
}


/*!**************************************************************
 * @fn fbe_drive_mgmt_dieh_replace_filename
 ****************************************************************
 * @brief
 *  This function replaces the filename in the path-file-name. 
 *
 * @version
 *   05/20/2015:  Wayne Garrett - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_drive_mgmt_dieh_replace_filename(fbe_drive_mgmt_t *drive_mgmt, fbe_u8_t * pathfilename, fbe_u32_t buff_len, fbe_u8_t * newfilename)
{
    fbe_u8_t * foundstr = NULL;

#ifdef ALAMOSA_WINDOWS_ENV
    fbe_u8_t slash = '\\';
#else
    fbe_u8_t slash = '/';
#endif


    foundstr = strrchr(pathfilename, slash);
    if (foundstr == NULL)  /* entire string is filename */
    {
        foundstr = pathfilename;
    }
    else
    {
        foundstr++;  /* increment passed slash */
    }

    /* Make sure there is no overrun */
    if ((foundstr-pathfilename)+strlen(newfilename) > buff_len-1)
    {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: Overrun pathname buffer.\n", __FUNCTION__);  
        return FBE_STATUS_GENERIC_FAILURE;
    }

    strncpy(foundstr, newfilename, strlen(newfilename)+1);

    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                      "%s: loading %s\n", __FUNCTION__, pathfilename);  
    return FBE_STATUS_OK;
}


/*!**************************************************************
 * @fn dmo_dieh_parse_xml_file
 ****************************************************************
 * @brief
 * Passes the xml file name passed in.  If file name is NULL then
 * default file is used.  This codes used the Expat opensource
 * parser.  Function will registers start, data and end elements
 * handler to the parser. It also checks if errors occurred
 * during parsing.
 * 
 * @param drive_mgmt - pointer to drive mgmt object.
 * @param pFilename -  XML file to parse.
 * 
 * @return status of parsing 
 *
 * @version
 *   03/15/2012:  Wayne Garrett - Ported from DH Flare
 *
 ****************************************************************/
static fbe_dieh_load_status_t dmo_dieh_parse_xml_file(fbe_drive_mgmt_t * drive_mgmt, char* pFilename)
{    
    dieh_xml_error_info_t status;						/* Error status from reading xml. */
    
    status = fbe_drive_mgmt_dieh_xml_parse( drive_mgmt,
                                            pFilename, 
                                            fbe_dmo_thrsh_xml_start_element,
                                            fbe_dmo_thrsh_xml_end_element,
                                            fbe_dmo_thrsh_xml_data_element );
    
    if ( status == DIEH_XML_FILE_READ_ERROR ){
        // Error opening the file
		drive_mgmt->xml_parser->doc_state = FBE_DMO_THRSH_DOC_ERROR;
		drive_mgmt->xml_parser->status = FBE_DMO_THRSH_FILE_READ_ERROR;
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: XML Parser  - Could not Open the file to read.\n", __FUNCTION__);        
        return FBE_DMO_THRSH_FILE_READ_ERROR;

    }else if ( status == DIEH_XML_PARSE_ERROR ){
        // Found a read error.  
		drive_mgmt->xml_parser->doc_state = FBE_DMO_THRSH_DOC_ERROR;
		drive_mgmt->xml_parser->status = FBE_DMO_THRSH_XML_ELEMENT_PARSE_ERROR;
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: XML Parser - File read error.\n", __FUNCTION__);   
        /* DCS DIEH state machine has started an update.  We need to clear it since it failed part way through
         */
        fbe_api_drive_configuration_interface_dieh_force_clear_update();

		return FBE_DMO_THRSH_XML_ELEMENT_PARSE_ERROR;        
	
    }else if (drive_mgmt->xml_parser->doc_state == FBE_DMO_THRSH_DOC_ERROR){  
        /* Set by dmo parsing code if doc_state already set to error.  In this case the 
           parsing code will also set the status. */
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: XML Parser - Parsing failed.\n", __FUNCTION__);      
        
        if (drive_mgmt->xml_parser->status == FBE_DMO_THRSH_XML_ELEMENT_PARSE_ERROR)
        {       
            /* DCS DIEH state machine has started an update.  We need to clear it since it failed part way through.
               Only checking PARSE_ERROR status since it possible we failed becauses a start update is already
               in progress.  We wouldn't want to clear it in that case.
             */
            fbe_api_drive_configuration_interface_dieh_force_clear_update();
        }
            
        return drive_mgmt->xml_parser->status;  

    }else if (status == DIEH_XML_SUCCESS){
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: XML Parser - Parsing Succesfull.\n", __FUNCTION__);       

    } else{
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: XML Parser - Unhandled Error %d.\n", __FUNCTION__, status);    
    }
    return FBE_DMO_THRSH_NO_ERROR;
}

