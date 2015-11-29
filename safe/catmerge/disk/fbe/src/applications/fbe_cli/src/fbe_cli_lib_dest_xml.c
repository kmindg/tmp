
/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_cli_lib_dest_xml.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains XML parsing and related functionality. 
 *  
 * TABLE OF CONTENTS
 *  fbe_dest_usr_intf_load
 *  fbe_cli_dest_read_and_parse_xml_file
 *  fbe_cli_dest_parser_var_initialize
 *  fbe_cli_dest_parse_xml_file
 *  fbe_cli_dest_parse_xml_start_element
 *  fbe_cli_dest_parse_xml_end_element
 *  fbe_cli_dest_parse_xml_data_element 
 *  fbe_dest_usr_intf_save
 *  dest_save_records
 *
 * NOTES
 * 
  * History:
 *  05/10/2012  Created. kothal
 ***************************************************************************/

/*************************
 ** INCLUDE FILES
 *************************/
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "generics.h"
#include "fbe_cli_dest.h"
#include "fbe_api_physical_drive_interface.h"
#include "fbe_file.h"
#include "fbe_cli_lurg.h"
#include "fbe/fbe_dest.h"
#include "fbe_cli_lib_expat.h"
#include "fbe_dest_private.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_api_dest_injection_interface.h"
#include "fbe_cli_private.h"


 /* XML Data Strings correspond to the XML_DATA_TYPE defined in the
  * fbe_cli_dest_xml.h file. This is an array of strings that are tag types 
  * in the XML configuration file. A string array and a corresponding
  * enum of the tags will make life easy during the parsing for easily
  * locating the tag that is being parsed at the time. 
  */

fbe_u8_t *FBE_DEST_XML_TAG_STRINGS[] = {
    "fru",\
    "lba_start",\
    "lba_end",\
    "opcode",\
    "error_type",\
    "error",\
    "valid_lba",\
    "deferred",\
    "num_of_times",\
    "frequency",\
    "is_random_frequency",\
    "reactivation_gap_type",\
    "reactivation_gap_msecs",\
    "reactivation_gap_io_count",\
    "is_random_gap",\
    "max_rand_msecs_to_reactivate",\
    "max_rand_react_gap_io_count",\
    "is_random_reactivation",\
    "num_reactivations",\
    "delay_io_msec",\
    NULL
};

/* This global structure will be used to identify the XML flags, 
 * and contains the global structures required across the Expat Callback
 * functions.
 */
FBE_DEST_XML_PARSING fbe_dest_parser_tools;
fbe_dest_record_handle_t dest_record_handle = NULL;

/**********************************************************************
 *fbe_dest_usr_intf_load() 
 **********************************************************************
 *
 * DESCRIPTION:
 *  This is the first function to be called for DEST initialization.
 *  This function initializes the structure of the error
 *  lookup table. Error Lookup table is usually initialized with the
 *  values available from a configuration XML file placed in 
 *  dest_config directory in "C" drive. By default the filename is config.xml. 
 *  User can name it differently and pass that file name. 
 *
 * ASSUMPTIONS:
 *  None.
 *  
 * PARAMETERS:
 *  filename, [I] - Optional input of configuration XML file name. 
 *
 * RETURNS:
 *  DEST_SUCCESS on success.
 *  FBE_XML_FILE_READ_ERROR - not able to find the file or read it.
 *  FBE_XML_PARSE_ERROR - error parsing the file
 *  
 *   
 **********************************************************************/
FBE_DEST_ERROR_INFO fbe_dest_usr_intf_load(fbe_char_t *filename)
{	
    fbe_char_t pwfilename[FBE_DEST_MAX_STR_LEN+1];
    FBE_XML_ERROR_INFO status;    
    
    if(filename != NULL)
    {
        strncpy(pwfilename, filename, FBE_DEST_MAX_STR_LEN);
    }
    else
    {
        /* Use the default file name. 
         */
        strncpy(pwfilename, FBE_DEST_CONFIG_FILE, FBE_DEST_MAX_STR_LEN);
    }   
    
    /* Invoke the parse XML file commands on the given filename to
     * parse the  error rule configuration.
     */
    status = fbe_cli_dest_read_and_parse_xml_file(pwfilename);
    if(status != DEST_SUCCESS)
    {       
        fbe_cli_printf("\nDEST:Error %X parsing XML Config File: %s \n", status, pwfilename);
        return status;      
    }
    
    return DEST_SUCCESS;
} 
/**********************************************
 * end of fbe_dest_init_xml
 *********************************************/
/**********************************************************************
 * fbe_cli_dest_read_and_parse_xml_file() 
 **********************************************************************
 *
 * DESCRIPTION:
 *  This function parses the provided XML file. File handlers are
 *  invoked and EXPAT parser is initialized. Then the configuration
 *  XML file is parsed. 
 *
 * PARAMETERS:
 *  filename,   [I] - file to parse
 *
 * ASSUMPTIONS:
 *  None
 *  
 * RETURNS:   
 *   FBE_DEST_FILE_READ_ERROR   if not able to find the file or read the file
 *   FBE_DEST_XML_PARSE_ERROR   if there is a problem with parsing the file
 *   FBE_DEST_SUCCESS           if parsing is successful
 *  
 **********************************************************************/
FBE_DEST_ERROR_INFO fbe_cli_dest_read_and_parse_xml_file(fbe_char_t *filename_p)
{   
    fbe_char_t full_path[FBE_DEST_MAX_PATHNAME_LEN];    /* Full Path, Directory + Filename. */
    FBE_XML_ERROR_INFO status;  /* Error status from reading xml. */   
    EMCUTIL_STATUS path_status;

    /* Set the file name with full path. 
     */     
    path_status = EmcutilFilePathMake(full_path, FBE_DEST_MAX_PATHNAME_LEN,
                                      FBE_EMC_ROOT, FBE_DEST_DIRECTORY,
                                      filename_p, NULL);
    if (EMCUTIL_STATUS_OK != path_status) {
        return DEST_FAILURE;
    }
	
    fbe_cli_printf("DEST: Parsing XML File: %s\n", full_path);
    /* Call our library function to parse the file.
     */
    /* Path of file to parse. */
    /* The next three params are the three */
    /* callbacks used by the expat parser.*/
    status = fbe_xml_parse_file( full_path, 
                                 fbe_cli_dest_parse_xml_start_element,
                                 fbe_cli_dest_parse_xml_end_element,
                                 fbe_cli_dest_parse_xml_data_element );
    
    if ( status == FBE_XML_FILE_READ_ERROR )
    {
        /* Found a read error, display it.
         */
        fbe_cli_printf("DEST: Could not Open file <%s> to read!\n", full_path);
        return DEST_FILE_READ_ERROR;
    }    
    else if ( status == FBE_XML_PARSE_ERROR )
    {
        /* Found a read error, display it.
         */
        fbe_cli_printf("DEST: Error while parsing file <%s>!\n", full_path);
        return DEST_XML_PARSE_ERROR;
    }
    else
    {
        fbe_cli_printf("DEST: Success parsing file <%s>!\n", full_path);
    }
    
    return DEST_SUCCESS;
}
/**********************************************
 * end of dest_parse_xml_file
 *********************************************/

/**********************************************************************
 * fbe_cli_dest_parse_xml_start_element() 
 **********************************************************************
 *
 * DESCRIPTION:
 *  This is the registered callback handler with EXPAT parser. Expat
 *  triggers this function every time it encounters the start of an 
 *  element tag. 
 *
 * PARAMETERS:
 *   data,  [I] - XML data buffer
 *   element,    [I] - element name
 *   attr,  [I] - any attributes
 *
 * ASSUMPTIONS:
 *  It is assumed that the XML file contains the data element tags
 *  as defined in the fbe_cli_dest.h file. 
 * 
 * RETURNS:
 *   Nothing. 
 **********************************************************************/
void fbe_cli_dest_parse_xml_start_element(  void *data, 
                                    const fbe_char_t *element, 
                                    const fbe_char_t **attr)
{	
    /* If the element type we encountered is <ErrorRule>, we allot 
     * memory for the new error rule and start processing the rest of 
     * the data. Beginning of error rule marks the beginning of the 
     * record and end of the error rule tag is the end of the record.
     */
    if( (strcmp(element, FBE_DEST_XML_ERROR_RULE_TAG) == 0))
    {
       // fbe_cli_printf("%s:\tDEST: element %s\n" , __FUNCTION__, element);
		 
        /* We have a new error rule. Allocate resources (parsing 
         * tools structure) and DEST error record.
         */
        fbe_cli_dest_parser_var_initialize(&fbe_dest_parser_tools);
        
        /* Check out if the initialization is done right. Parser 
         * structure and the error record pointers in that structure
         * should be initialized by now.
         */
        if(fbe_dest_parser_tools.err_record_ptr == NULL)
        {
            /* There is memory allocation failure with fbe_cli_dest_parser_var_initialize function.
             * Fail here.
             */
             fbe_cli_printf("%s:\tDEST: fbe_cli_dest_parser_var_initialize failure" , __FUNCTION__);
            return;       
        }
    }

    /* All the elements might not be the data values for DEST. So 
     * every time we encounter the beginning of a tag, we set
     * the data type to be false and data tag enumeration to be 0.
     * Only after the check is done on the tag type, we set the 
     * data flag to be true.
     */ 
    fbe_dest_parser_tools.data_flag = FALSE;
    fbe_dest_parser_tools.data_tag = 0;
    
    /* Check for the element tag string from the FBE_DEST_XML_TAG_STRINGS 
     * lookup table. If there is a match, then set the data_flag to be
     * true.
     */
    while(FBE_DEST_XML_TAG_STRINGS[fbe_dest_parser_tools.data_tag] != NULL)
    {
        if(!strcmp(element, FBE_DEST_XML_TAG_STRINGS[fbe_dest_parser_tools.data_tag]))
        {
            /* If there is a match, set the data_type and
             * data is available flag. 
             */
            fbe_dest_parser_tools.data_type = fbe_dest_parser_tools.data_tag;
            fbe_dest_parser_tools.data_flag = TRUE;
            memset(fbe_dest_parser_tools.data, '\0', FBE_DEST_MAX_STR_LEN);
           //fbe_cli_printf("%s:\tDEST: match here: element %s\n" , __FUNCTION__, element);
            break;
        }        
        fbe_dest_parser_tools.data_tag++;
    }
    
    return;                
}
/**********************************************
 * end of fbe_cli_dest_parse_xml_start_element
 *********************************************/
/**********************************************************************
 * fbe_cli_dest_parse_xml_end_element() 
 **********************************************************************
 *
 * DESCRIPTION:
 *  This is the callback handler registered with EXPAT Parser. 
 *  This function will be called whenever expat parser identifies a 
 *  end tag. This function compares the element type with the 
 *  suggested definitions and removes the data flag set by the start
 *  element handler. When the final error rule end tag is matched,
 *  DEST error record is inserted.
 *
 * PARAMETERS:
 *  data,   [I] - XML data buffer
 *  element,     [I] - element name
 *
 * ASSUMPTIONS:
 *  None
 *  
 * RETURNS:
 *  Nothing. 
 **********************************************************************/
void fbe_cli_dest_parse_xml_end_element(void *data, const fbe_char_t* element)
{
    fbe_status_t status;
    fbe_job_service_bes_t bes;
    fbe_dest_record_handle_t record_handle = NULL;
	
    /* Set the flag to 0 so that data handler wont be triggered.
     */
    fbe_dest_parser_tools.data_flag = FALSE;
    
    /* Compare to see if the element name is Error Rule. If so, then
     * complete the record and add the record to the error table     
     */
    if(!strcmp(element, FBE_DEST_XML_ERROR_RULE_TAG))
    {	
        /* Get the FRU number from the user entered bus, encl, slot 
         * numbers.
         */		
        status = fbe_cli_convert_diskname_to_bed(fbe_dest_parser_tools.fru, &bes);
		
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s:\tDEST: Specified disk number %s is invalid. Sts:%d\n", __FUNCTION__, fbe_dest_parser_tools.fru, status);            
        }        
        else
        {
            status = fbe_api_get_physical_drive_object_id_by_location(bes.bus,
                     bes.enclosure, bes.slot, &(fbe_dest_parser_tools.err_record_ptr->object_id));
            
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s:\tDEST: Error occured while finding the Object ID for drive %d_%d_%d. Sts:%d\n",
                               __FUNCTION__, bes.bus, bes.enclosure, bes.slot, status);
            }
            else
            {
                status = fbe_cli_dest_validate_assign_lba_range(fbe_dest_parser_tools.err_record_ptr, fbe_dest_parser_tools.err_record_ptr->object_id);
        
                if (status != DEST_SUCCESS)
                {
                    fbe_cli_printf("%s:\tDEST: Start and Stop LBA range (0x%x - 0x%x) invalid. Sts:%d\n",
                                   __FUNCTION__, (unsigned int)fbe_dest_parser_tools.err_record_ptr->lba_start, (unsigned int)fbe_dest_parser_tools.err_record_ptr->lba_end, status);
                }                          
                else
                {
                    /* Todo - Check if the record exists already*/
					
                    /* Validate and Insert the error rule.
                     */
                    if (fbe_dest_validate_new_record(fbe_dest_parser_tools.err_record_ptr) != DEST_SUCCESS)
                    {
                        fbe_cli_printf("DEST: ADD failed.  There is a problem validating the record.\n");                    
                    }
                    
                    status = fbe_api_dest_injection_add_record(fbe_dest_parser_tools.err_record_ptr, &dest_record_handle);
	
                    if (status != FBE_STATUS_OK)
                    {
                        fbe_cli_printf("%s:\tDEST: Failed to create a record. Sts:%d\n", __FUNCTION__, status);
                    }
                    fbe_cli_printf("%s:\tDEST: Created a record succesfully.\n", __FUNCTION__);
                    status = fbe_dest_display_error_records(&record_handle);	
                    if (status != FBE_STATUS_OK)
                    {
                        fbe_cli_printf("%s:\tDEST: Failed to list the record. %X\n", __FUNCTION__, status);		
                    }
                }
            }
        }        
    }
    return;
}

/**********************************************
 * end of fbe_cli_dest_parse_xml_end_element
 *********************************************/
 
/**********************************************************************
 * fbe_cli_dest_parse_xml_data_element() 
 **********************************************************************
 *
 * DESCRIPTION:
 *  This is the callback handler registered with EXPAT Parser. 
 *  This function will be called whenever expat parser identifies the 
 *  data. This function checks if a data_flag is set by start element.
 *  Then checks the validity of the data and then saves the data in the
 *  error record as the algorithm is defined.
 *
 * PARAMETERS:
 *  data,   [I] - XML data buffer.
 *  element,[I] - element name.
 *  len,    [I] - length of valid data in the given buffer.
 *   
 * ASSUMPTIONS:
 *  None
 * 
 * RETURNS
 *  Nothing.
 *    
 **********************************************************************/
void fbe_cli_dest_parse_xml_data_element(void* data, const fbe_char_t* element, fbe_s32_t len)
{    
    /* The following variable is not necessary, but to make the code 
    * look good it is used to cutdown a long deference.
    */
    fbe_dest_error_record_t * error_rec_ptr;
    fbe_u32_t str_length = 0;
    fbe_u32_t error_val;
    fbe_char_t *temp_str_ptr;      
 
    /* If the data flag is set, and the data is not equal to newline 
    * fbe_char_tacter or return fbe_char_tacter or tab fbe_char_tacter, process the data.
    */
    if(fbe_dest_parser_tools.data_flag && 
            (element[0] != '\n') && 
            (element[0] != '\r') && 
            (element[0] != '\t') )
    {
 
        /* The following check makes sure that len is with in the range
        * of the alloted string length. This will prevent buffer
        * overun.
        */
        if( len > (FBE_DEST_MAX_STR_LEN - strlen(fbe_dest_parser_tools.data)) )
        {
            return;
        }
        
        /* Copy the valid data into our buffer. 
         */
        strncat(fbe_dest_parser_tools.data, element, len);
		
        /* Check the data_type and proceed.
        */
        if(strlen(fbe_dest_parser_tools.data))
        {
            error_rec_ptr = fbe_dest_parser_tools.err_record_ptr;
            str_length = (fbe_s32_t)strlen(fbe_dest_parser_tools.data) + len + 1;            
 
            switch(fbe_dest_parser_tools.data_type)
            {				
                fbe_cli_printf("%s:\tDEST:fbe_dest_parser_tools.data_type %u" , __FUNCTION__, fbe_dest_parser_tools.data_type);
                case fru:
                    strncpy(fbe_dest_parser_tools.fru, fbe_dest_parser_tools.data, FBE_FRU_STR_LEN-1);
                    break;			
					
                case lba_start:
                    error_rec_ptr->lba_start = fbe_dest_strtoint64(fbe_dest_parser_tools.data);
                    break;
					
                case lba_end:
                    error_rec_ptr->lba_end = fbe_dest_strtoint64(fbe_dest_parser_tools.data);
                    break;
					
                case opcode:                   
                    fbe_dest_str_to_upper(error_rec_ptr->opcode, fbe_dest_parser_tools.data, len+1);  
                    break;
					
                case error_type:
                    error_rec_ptr->dest_error_type = fbe_dest_strtoint(fbe_dest_parser_tools.data);
                    /*TODO: currently XML only supports a scsi status of check condition.   This should be fixed to support 
                      any kind of scsi status. */
                    if (FBE_DEST_ERROR_TYPE_SCSI == error_rec_ptr->dest_error_type)
                    {
                       error_rec_ptr->dest_error.dest_scsi_error.scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION;
                    }
                    break;	
					
                case error:                                     
                 
                    temp_str_ptr = strstr(fbe_dest_parser_tools.data, "0x");       /* Returns a pointer to the beginning of the 0x, or NULL if not found */
                    if (temp_str_ptr != NULL)
                    {     
                        /* This may be a valid hex value. */	
                        temp_str_ptr += 2;    /* We have a leading 0x that we need to skip over. */		                      
                        if (fbe_atoh(temp_str_ptr) == 0xFFFFFFFFU )  
                        {
                            fbe_cli_printf("DEST: Invalid hex value. Please try again.\n"); 
                            break;
                        }
                        else 
                        {
                            error_val = fbe_atoh(temp_str_ptr); 
                        }  
                    }
                    else
                    {		                
                        error_val = fbe_atoh(fbe_dest_parser_tools.data); 
                    }
			                    
                    dest_scsi_opcode_lookup_fbe(error_rec_ptr->opcode, 
			                                                error_rec_ptr->dest_error.dest_scsi_error.scsi_command,
			                                                &error_rec_ptr->dest_error.dest_scsi_error.scsi_command_count); 	
			                    
					
                    error_rec_ptr->dest_error.dest_scsi_error.scsi_sense_key = (error_val >> 16) & 0xFF;
                    error_rec_ptr->dest_error.dest_scsi_error.scsi_additional_sense_code = (error_val >> 8) & 0xFF;
                    error_rec_ptr->dest_error.dest_scsi_error.scsi_additional_sense_code_qualifier = error_val & 0xFF;
                    error_rec_ptr->dest_error.dest_scsi_error.port_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
                    break;					
					
                case valid_lba:
                    error_rec_ptr->valid_lba = fbe_dest_strtoint(fbe_dest_parser_tools.data);
                    break;
					
                case deferred:
                    error_rec_ptr->deferred = fbe_dest_strtoint(fbe_dest_parser_tools.data);
                    break;
					
                case num_of_times:
                    error_rec_ptr->num_of_times_to_insert = fbe_dest_strtoint(fbe_dest_parser_tools.data);
                    break;
					
                case frequency:
                    error_rec_ptr->frequency = fbe_dest_strtoint(fbe_dest_parser_tools.data);
                    break;
					
                case is_random_frequency:
                    error_rec_ptr->is_random_frequency = fbe_dest_strtoint(fbe_dest_parser_tools.data);
                    break;
					
                case reactivation_gap_type:
                    error_rec_ptr->react_gap_type = fbe_dest_strtoint(fbe_dest_parser_tools.data);
                    break;
					
                case reactivation_gap_msecs:
                    error_rec_ptr->react_gap_msecs = fbe_dest_strtoint(fbe_dest_parser_tools.data);
                    break;
					
                case reactivation_gap_io_count:
                    error_rec_ptr->react_gap_io_count = fbe_dest_strtoint(fbe_dest_parser_tools.data);
                    break;
					
                case is_random_gap:
                    error_rec_ptr->is_random_gap = (fbe_bool_t)fbe_dest_strtoint(fbe_dest_parser_tools.data);
                    break;
					
                case max_rand_msecs_to_reactivate:
                    error_rec_ptr->max_rand_msecs_to_reactivate = fbe_dest_strtoint(fbe_dest_parser_tools.data);
                    break;
					
                case max_rand_react_gap_io_count:
                    error_rec_ptr->max_rand_react_gap_io_count = fbe_dest_strtoint(fbe_dest_parser_tools.data);
                    break;
					
                case is_random_reactivation:
                    error_rec_ptr->is_random_react_interations = (fbe_bool_t)fbe_dest_strtoint(fbe_dest_parser_tools.data);
                    break;
					
                case num_reactivations:
                    error_rec_ptr->num_of_times_to_reactivate = fbe_dest_strtoint(fbe_dest_parser_tools.data);
                    break;
					
                case delay_io_msec:
                    error_rec_ptr->delay_io_msec = fbe_dest_strtoint(fbe_dest_parser_tools.data);
                    break;
					
                default:
                    fbe_cli_printf("DEST: Default valus is : %d", fbe_dest_parser_tools.data_type);
                    break;
            }
        }
    }
    
    return;
}
/**********************************************
 * end of fbe_cli_dest_parse_xml_data_element
 *********************************************/

/**********************************************************************
 * fbe_cli_dest_parser_var_initialize
 **********************************************************************
 * 
 * DESCRIPTION:
 *  Initialize the parser miscellaneous structure used globally 
 *  in the context of this file. For every new error record, values of
 *  bus number, enclosure number, slot number, dest error record,
 *  xml data type and xml data flag are reset. 
 *
 * PARAMETERS:
 *  parsing_ptr,    [O] - structure is initialized. 
 *                      
 * ASSUMPTIONS:
 *  None                  
 *      
 * RETURNS:
 *  None
 * 
 *********************************************************************/      
VOID fbe_cli_dest_parser_var_initialize(FBE_DEST_XML_PARSING* parsing_ptr)
{
    /* We know that parsing_ptr could not be a NULL value since it
     * is declared for sure. But the check will ensure the same. 
     */
    if(parsing_ptr == NULL)
    {
        return;
    }

    /* Initialize the entire XML parsing structure with 0. Some
     * of the fields have the default values of 0 in the structure. 
     */
    memset(parsing_ptr, 0, sizeof(FBE_DEST_XML_PARSING));  
    
    /* Allocate Memory for the error record Pointer with in the 
     * parsing_ptr.
     */
   parsing_ptr->err_record_ptr = (fbe_dest_error_record_t *)fbe_api_allocate_memory(sizeof(fbe_dest_error_record_t));
        
    /* Make sure the memory is allocated. 
     */
    if(parsing_ptr->err_record_ptr == NULL)
    {
        return;
    }

    /*Initialize the Error record*/
    fbe_cli_dest_err_record_initialize(parsing_ptr->err_record_ptr);

    return;
}
/******************************************
 * end of fbe_cli_dest_parser_var_initialize
 *****************************************/
 
 
/**********************************************************************
 * dest_save_records()
 **********************************************************************
 *
 * Description:
 *  Save all the error sim records to a file.
 *
 * PARAMETERS:
 *  handle,   [I] - Handle to write to.
 *  buffer_p, [I] - Buffer space we can use for writing.
 *
 * ASSUMPTIONS:
 *  None
 *
 * RETURNS:
 *   FBE_XML_FILE_WRITE_ERROR - If an error encountered writing the file.
 *   FBE_XML_SUCCESS - If writing is successful.
 *
 *
 **********************************************************************/
FBE_XML_ERROR_INFO dest_save_records( HANDLE file_handle,
									  UCHAR *buffer_p )
{ 
    FBE_XML_ERROR_INFO wr_status = FBE_XML_SUCCESS;
    fbe_dest_record_handle_t handle_p = NULL;	
    fbe_dest_error_record_t record_p;
    fbe_status_t status;	
    fbe_job_service_bes_t bes;
    fbe_package_id_t package_id = FBE_PACKAGE_ID_PHYSICAL;
    fbe_class_id_t class_id = FBE_CLASS_ID_SAS_PHYSICAL_DRIVE ;	
    fbe_u32_t number_of_records = 0;
    fbe_char_t error_val[FBE_DEST_MAX_PATHNAME_LEN];      
	
    /*Initialize the Error record*/
    fbe_cli_dest_err_record_initialize(&record_p); 
	
    do{	   
 
    /* For getting first record, we send handle_p as NULL. */
    status = fbe_api_dest_injection_get_record_next(&record_p, &handle_p);
    
    if(handle_p == NULL || status != FBE_STATUS_OK)
    {
        //fbe_cli_printf("%s:\tDEST: Failed to get records. %X\n", __FUNCTION__, status);
        if(number_of_records){
		    //do nothing
            return wr_status;
        } 
        else{
            return FBE_XML_FAILURE;
        }
    }	
    number_of_records ++; 
	
    status = fbe_cli_get_bus_enclosure_slot_info(record_p.object_id,
             class_id, &bes.bus, &bes.enclosure, &bes.slot, package_id);
	
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s:\tDEST: Failed to get bes info. %X\n", __FUNCTION__, status);
        return FBE_XML_FAILURE;
    }			     
         
    _snprintf(error_val, 10, "0%x%x0%x\n", record_p.dest_error.dest_scsi_error.scsi_sense_key, record_p.dest_error.dest_scsi_error.scsi_additional_sense_code,  record_p.dest_error.dest_scsi_error.scsi_additional_sense_code_qualifier);
	
    fbe_zero_memory(buffer_p, FBE_DEST_MAX_BUFFER_LEN);
    _snprintf(buffer_p, FBE_DEST_MAX_BUFFER_LEN, 
              "\n\t<ErrorRule>\n\
              \t\t<fru>%u_%u_%u</fru>\n\
              \t\t<lba_start>0x%llx</lba_start>\n\
              \t\t<lba_end>0x%llx</lba_end>\n\
              \t\t<opcode>%s</opcode>\n\
              \t\t<error_type>0x%x</error_type>\n\
              \t\t<error>%s</error>\n\
              \t\t<valid_lba>0x%x</valid_lba>\n\
              \t\t<deferred>0x%x</deferred>\n\
              \t\t<num_of_times>0x%x</num_of_times>\n\
              \t\t<frequency>0x%x</frequency>\n\
              \t\t<is_random_frequency>0x%x</is_random_frequency>\n\
              \t\t<reactivation_gap_type>0x%x</reactivation_gap_type>\n\
              \t\t<reactivation_gap_msecs>0x%x</reactivation_gap_msecs>\n\
              \t\t<reactivation_gap_io_count>0x%x</reactivation_gap_io_count>\n\
              \t\t<is_random_gap>0x%x</is_random_gap>\n\
              \t\t<max_rand_msecs_to_reactivate>0x%x</max_rand_msecs_to_reactivate>\n\
              \t\t<max_rand_react_gap_io_count>0x%x</max_rand_react_gap_io_count>\n\
              \t\t<is_random_reactivation>0x%x</is_random_reactivation>\n\
              \t\t<num_reactivations>0x%x</num_reactivations>\n\
              \t\t<delay_io_msec>0x%x</delay_io_msec>\n\
              \t</ErrorRule>\n", 
              bes.bus, bes.enclosure, bes.slot,
              record_p.lba_start,
              record_p.lba_end,
              record_p.opcode, 
              record_p.dest_error_type, 
              error_val,
              record_p.valid_lba,
              record_p.deferred, 
              record_p.num_of_times_to_insert, 
              record_p.frequency, 
              record_p.is_random_frequency, 
              record_p.react_gap_type, 
              record_p.react_gap_msecs, 
              record_p.react_gap_io_count, 
              record_p.is_random_gap, 
              record_p.max_rand_msecs_to_reactivate, 
              record_p.max_rand_react_gap_io_count,
              record_p.is_random_react_interations, 
              record_p.num_of_times_to_reactivate,
              record_p.delay_io_msec);
	
    /* Write the error rule to the file.
    */
    if ( (wr_status = fbe_xml_write_buffer(file_handle, buffer_p))!= FBE_XML_SUCCESS )
    {
        return wr_status;
    }         
    
    }while(handle_p != NULL);
    return wr_status;
}
/**************************************************
 * dest_save_records()
 **************************************************/

/**********************************************************************
 *fbe_dest_usr_intf_save() 
 **********************************************************************
 *
 * Description:
 *  save the runtime configuration to the config file. 
 *
 * PARAMETERS:
 *  filename_p, [I] - Filename to write the configuration to.
 *
 * ASSUMPTIONS:
 *  None
 *
 * RETURNS:
 *   DEST_FILE_READ_ERROR   if not able to find the file or read the
 *                          file
 *   DEST_XML_PARSE_ERROR   if there is a problem with parsing the file
 *   DEST_SUCCESS           if parsing is successful
 * 
 * 
 **********************************************************************/
FBE_DEST_ERROR_INFO fbe_dest_usr_intf_save(TEXT* filename_p)
{   
	/* Full Path, Directory + Filename. 
	*/    
    fbe_char_t full_path[FBE_DEST_MAX_PATHNAME_LEN]; 
    FBE_XML_ERROR_INFO status;  /* Error status from writing xml. */
    EMCUTIL_STATUS path_status;
    csx_status_e dir_status;  
	
    //Emc directory exists on drive, but not on sim. Create one if doesn't exist.
    path_status = EmcutilFilePathMake(full_path, FBE_DEST_MAX_PATHNAME_LEN,
                                      FBE_EMC_ROOT, NULL);
    if (EMCUTIL_STATUS_OK != path_status) {
        return DEST_FAILURE;
    }
    
    dir_status = csx_p_dir_create(full_path); //needs to be changed to fbe_*
    if(CSX_SUCCESS(dir_status) || (dir_status == CSX_STATUS_OBJECT_ALREADY_EXISTS))
    {
        path_status = EmcutilFilePathExtend(full_path,
                                            FBE_DEST_MAX_PATHNAME_LEN,
                                            FBE_DEST_DIRECTORY,
                                            NULL);
        if (EMCUTIL_STATUS_OK != path_status) {
            return DEST_FAILURE;
        }

        dir_status = csx_p_dir_create(full_path); //needs to be changed to fbe_*
        
        if(CSX_SUCCESS(dir_status) || (dir_status == CSX_STATUS_OBJECT_ALREADY_EXISTS))
        {
            fbe_cli_printf("DEST: Check Root Directory: success.\n");
        }
        else
        {
            fbe_cli_printf("DEST: Check Root Directory: failed.\n");
            return DEST_FILE_READ_ERROR;
        }
    }
    else
    {
        fbe_cli_printf("DEST: Check Root Directory: failed.\n");
        return DEST_FILE_READ_ERROR;
    }   
    	
    /* Check if the file name passed to is NULL. In that case
     * use the default configuration file name. 
     */	
	
    if (filename_p == NULL || !strlen(filename_p))
    {
        path_status = EmcutilFilePathExtend(full_path,
                                            FBE_DEST_MAX_PATHNAME_LEN,
                                            FBE_DEST_CONFIG_FILE, NULL);
    }
    else
    {
        path_status = EmcutilFilePathExtend(full_path,
                                            FBE_DEST_MAX_PATHNAME_LEN,
                                            filename_p, NULL);
    }
    if (EMCUTIL_STATUS_OK != path_status) {
        return DEST_FAILURE;
    }
		
    /* Just call the library function to save our file.
     * It will automatically write the headers and footers for us.
     * It will call our rg_err_save_records() so that
     * we will do the work of writing all our records.
     */
    status = fbe_xml_save_file( full_path, /* full path. */
                               FBE_DEST_XML_HEADER_STRING, /* Header. */
                               FBE_DEST_XML_FOOTER_STRING, /* Footer. */
                               /* Fn to write our records. */
                               dest_save_records );
    
    if ( status == FBE_XML_SUCCESS )
    {
        /* Success writing.
         */
        fbe_cli_printf("DEST: Saved the configuration to %s\n", full_path);
        return DEST_SUCCESS;
    }
    else
    {
        /* Failure writing, display error.
         */
        fbe_cli_printf("DEST: Could not write to the file. %d %s\n", status, filename_p );
        return DEST_FAILURE;
    }  
  
}
/**********************************************************************
 *fbe_dest_usr_intf_save() 
 **********************************************************************/

/*****************************************
 * end of fbe_cli_lib_dest_xml.c file
 *****************************************/
