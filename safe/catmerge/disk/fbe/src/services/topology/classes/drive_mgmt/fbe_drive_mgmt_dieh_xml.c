/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!***************************************************************************
 * @file fbe_drive_mgmt_dieh_xml.c
 * ***************************************************************************
 *
 * @brief
 *  Handles parsing the DIEH xml file and adding records to the
 *  drive_configuration_service.  After records are added the
 *  drive_configuration_service will notify all PDO objects to
 *  register for new handles.  PDO uses these handles for determining
 *  the correct DIEH behavior when it detects a drive error.
 * 
 *  This parsing code depends on the Expat opensource parser.
 * 
 * @ingroup drive_mgmt_class_files
 *  
 * @version
 *   03/20/2012 :  Wayne Garrett - Ported from DH Flare.
 *
 ***************************************************************************/

/*************************************************************************
 ** Include Files
 *************************************************************************/
#include "fbe/fbe_drive_configuration_interface.h"
#include "fbe/fbe_api_drive_configuration_service_interface.h"
#include "fbe_topology.h"
#include "fbe_drive_mgmt_private.h"
#include "fbe_drive_mgmt_dieh_xml.h"

#define FBE_DMO_XML_DEFLT_ATTR_DRV_INFO_TAG_STR         "default"
/*************************************************************************
 **                             Globals
 *************************************************************************/
/*Array for storing valud tag and their correspoding XML Element Tag Names in XML Document*/
fbe_dmo_thrsh_xml_element_tags_t dmo_thrsh_xml_tags[] =
{
    {FBE_DMO_XML_ELEM_DMO_THRSH_CFG_TAG_STR,           FBE_DMO_XML_ELEM_DMO_THRSH_CFG_TAG}, 
    {FBE_DMO_XML_ELEM_PORT_CFGS_TAG_STR,               FBE_DMO_XML_ELEM_PORT_CFGS_TAG}, 
    {FBE_DMO_XML_ELEM_PORT_CFG_TAG_STR,                FBE_DMO_XML_ELEM_PORT_CFG_TAG}, 
    {FBE_DMO_XML_ELEM_DRV_STATS_TAG_STR,               FBE_DMO_XML_ELEM_DRV_STATS_TAG}, 
    {FBE_DMO_XML_ELEM_ENCL_STATS_TAG_STR,              FBE_DMO_XML_ELEM_ENCL_STATS_TAG}, 
    {FBE_DMO_XML_ELEM_DRV_CFG_TAG_STR,                 FBE_DMO_XML_ELEM_DRV_CFG_TAG}, 
    {FBE_DMO_XML_ELEM_SAS_DRV_CFG_TAG_STR,             FBE_DMO_XML_ELEM_SAS_DRV_CFG_TAG}, 
    {FBE_DMO_XML_ELEM_DRV_STATCS_TAG_STR,              FBE_DMO_XML_ELEM_DRV_STATCS_TAG}, 
    {FBE_DMO_XML_ELEM_CMLT_STATS_TAG_STR,              FBE_DMO_XML_ELEM_CMLT_STATS_TAG},
    {FBE_DMO_XML_ELEM_RCVR_ERR_STATS_TAG_STR,          FBE_DMO_XML_ELEM_RCVR_ERR_STATS_TAG}, 
    {FBE_DMO_XML_ELEM_MEDIA_ERR_STATS_TAG_STR,         FBE_DMO_XML_ELEM_MEDIA_ERR_STATS_TAG}, 
    {FBE_DMO_XML_ELEM_HEALTHCHECK_ERR_STATS_TAG_STR,   FBE_DMO_XML_ELEM_HEALTHCHECK_ERR_STATS_TAG}, 
    {FBE_DMO_XML_ELEM_HW_ERR_STATS_TAG_STR,            FBE_DMO_XML_ELEM_HW_ERR_STATS_TAG}, 
    {FBE_DMO_XML_ELEM_LINK_ERR_STATS_TAG_STR,          FBE_DMO_XML_ELEM_LINK_ERR_STATS_TAG}, 
    {FBE_DMO_XML_ELEM_DRV_RST_STATS_TAG_STR,           FBE_DMO_XML_ELEM_DRV_RST_STATS_TAG}, 
    {FBE_DMO_XML_ELEM_DRV_PWR_CYC_TAG_STR,             FBE_DMO_XML_ELEM_DRV_PWR_CYC_TAG}, 
    {FBE_DMO_XML_ELEM_DATA_ERR_STATS_TAG_STR,          FBE_DMO_XML_ELEM_DATA_ERR_STATS_TAG}, 
    {FBE_DMO_XML_ELEM_RCVR_TAG_RDC_TAG_STR,            FBE_DMO_XML_ELEM_RCVR_TAG_RDC_TAG}, 
    {FBE_DMO_XML_ELEM_ERR_BRST_WT_RDC_TAG_STR,         FBE_DMO_XML_ELEM_ERR_BRST_WT_RDC_TAG}, 
    {FBE_DMO_XML_ELEM_ERR_BRST_DLT_TAG_STR,            FBE_DMO_XML_ELEM_ERR_BRST_DLT_TAG},     
    {FBE_DMO_XML_ELEM_DRV_INFO_TAG_STR,                FBE_DMO_XML_ELEM_DRV_INFO_TAG}, 
    {FBE_DMO_XML_ELEM_DRV_VND_TAG_STR,                 FBE_DMO_XML_ELEM_DRV_VND_TAG}, 
    {FBE_DMO_XML_ELEM_DRV_PART_NO_TAG_STR,             FBE_DMO_XML_ELEM_DRV_PART_NO_TAG}, 
    {FBE_DMO_XML_ELEM_DRV_FRMW_REV_TAG_STR,            FBE_DMO_XML_ELEM_DRV_FRMW_REV_TAG}, 
    {FBE_DMO_XML_ELEM_DRV_SER_NO_TAG_STR,              FBE_DMO_XML_ELEM_DRV_SER_NO_TAG}, 
    {FBE_DMO_XML_ELEM_DRV_Q_DEPTH_TAG_STR,             FBE_DMO_XML_ELEM_DRV_Q_DEPTH_TAG},
    {FBE_DMO_XML_ELEM_DRV_GLOBAL_PARAMETERS_TAG_STR,   FBE_DMO_XML_ELEM_DRV_GLOBAL_PARAMETERS_TAG},
    {FBE_DMO_XML_ELEM_SAS_EXCP_CODES_TAG_STR,          FBE_DMO_XML_ELEM_SAS_EXCP_CODES_TAG}, 
    {FBE_DMO_XML_ELEM_SCSI_EXCP_CODE_TAG_STR,          FBE_DMO_XML_ELEM_SCSI_EXCP_CODE_TAG}, 
    {FBE_DMO_XML_ELEM_SCSI_CODE_TAG_STR,               FBE_DMO_XML_ELEM_SCSI_CODE_TAG}, 
    {FBE_DMO_XML_ELEM_IO_STATUS_TAG_STR,               FBE_DMO_XML_ELEM_IO_STATUS_TAG}, 
    {FBE_DMO_XML_ELEM_ERR_CLSFY_TAG_STR,               FBE_DMO_XML_ELEM_ERR_CLSFY_TAG}, 
    {FBE_DMO_XML_ELEM_CONTROL_DFTL_TAG_STR,            FBE_DMO_XML_ELEM_CONTROL_DFTL_TAG},
    {FBE_DMO_XML_ELEM_WEIGHT_CHANGE_TAG_STR,           FBE_DMO_XML_ELEM_WEIGHT_CHANGE_TAG},
    {FBE_DMO_XML_ELEM_QUEUING_CFG_TAG_STR,             FBE_DMO_XML_ELEM_QUEUING_CFG_TAG}, 
    {FBE_DMO_XML_ELEM_SAS_QUEUING_CFG_TAG_STR,         FBE_DMO_XML_ELEM_SAS_QUEUING_CFG_TAG}, 
    {FBE_DMO_XML_ELEM_EQ_TIMER_TAG_STR,                FBE_DMO_XML_ELEM_EQ_TIMER_TAG}, 
    {FBE_DMO_XML_ELEM_RELIABLY_CHALLENGED_TAG_STR,     FBE_DMO_XML_ELEM_RELIABLY_CHALLENGED_TAG},
};

/* Lookup table used to converting error category attribute string to/from its value */
const static fbe_stats_errcat_attr_element_t dmo_thrsh_xml_errcat_attrib[] =
{
    {"threshold",       FBE_STATS_THRSH_PROC},
    {"io_interval",     FBE_STATS_INTV_PROC},
    {"weight",          FBE_STATS_WT_PROC},
    {"control_flag",    FBE_STATS_CNTRL_FLG_PROC},
    {"reset",           FBE_STATS_RESET_PROC},    
    {"eol",             FBE_STATS_EOL_PROC},
    {"eol_call_home",   FBE_STATS_EOL_CH_PROC},
    {"kill",            FBE_STATS_KILL_PROC},
    {"kill_call_home",  FBE_STATS_KILL_CH_PROC},
    {"event",           FBE_STATS_EVENT_PROC},
    {NULL,              FBE_STATS_INVALID_PROC}
};

#define FBE_DMO_ATTR_STR_MAX 64
#define FBE_DMO_MAX_ATTR_FIELDS 10

typedef fbe_u8_t* fbe_dmo_attribute_field_array_t[FBE_DMO_MAX_ATTR_FIELDS];


/* Lookup table used to converting error category attribute string to/from its value */
const static fbe_stats_wtx_attr_element_t dmo_thrsh_xml_wtx_attrib[] =
{
    {"opcode",      FBE_STATS_WTX_OPCODE_PROC},
    {"cc",          FBE_STATS_WTX_CC_ERROR_PROC},
    {"port",        FBE_STATS_WTX_PORT_ERROR_PROC},
    {"change",      FBE_STATS_WTX_CHANGE_PROC},
    {NULL,          FBE_STATS_WTX_INVALID_PROC}
};

const static fbe_dmo_thrsh_port_type_attr_val_t fbe_dmo_xml_attr_port_type_values[] =   
{
    {"Fiber",       FBE_PORT_TYPE_FIBRE}, \
    {"SAS LSI",     FBE_PORT_TYPE_SAS_LSI}, \
    {"SAS PMC",     FBE_PORT_TYPE_SAS_PMC}, \
    {NULL,          FBE_PORT_TYPE_INVALID}, \
};


// Listing of all possible drive vendors. this table has to be in synch with fbe_drive_vendor_id_t listing in disk\interface\fbe\fbe_physcial_drive.h
const static fbe_dmo_thrsh_drv_vendors_name_t fbe_dmo_drive_vendor_table[] =
{
    {"simulation",  FBE_DRIVE_VENDOR_SIMULATION}, \
    {"hitachi",     FBE_DRIVE_VENDOR_HITACHI}, \
    {"seagate",     FBE_DRIVE_VENDOR_SEAGATE}, \
    {"ibm",         FBE_DRIVE_VENDOR_IBM}, \
    {"fujitsu",     FBE_DRIVE_VENDOR_FUJITSU}, \
    {"maxtor",      FBE_DRIVE_VENDOR_MAXTOR}, \
    {"western digital", FBE_DRIVE_VENDOR_WESTERN_DIGITAL}, \
    {"stec",        FBE_DRIVE_VENDOR_STEC}, \
    {"samsung",     FBE_DRIVE_VENDOR_SAMSUNG}, \
    {"samsung",     FBE_DRIVE_VENDOR_SAMSUNG_2}, \
    {"emulex",      FBE_DRIVE_VENDOR_EMULEX}, \
    {"toshiba",     FBE_DRIVE_VENDOR_TOSHIBA}, \
    {"anobit",      FBE_DRIVE_VENDOR_ANOBIT}, \
    {"micron",      FBE_DRIVE_VENDOR_MICRON}, \
    {"sandisk",     FBE_DRIVE_VENDOR_SANDISK}, \
    {NULL,          FBE_DRIVE_VENDOR_ILLEGAL},
}; 

// Listing of all possible IO status. this table has to be in synch with fbe_payload_cdb_fis_io_status_t listing in disk\interface\fbe\fbe_payload_cdb_fis.h
const static fbe_dmo_thrsh_cdb_fis_status_t fbe_dmo_cdb_fis_status_table[] = 
{
    {"ok",          FBE_PAYLOAD_CDB_FIS_IO_STATUS_OK}, \
    {"failRetry",   FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY}, \
    {"failNoRetry", FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_NO_RETRY}, \
    {NULL,          FBE_PAYLOAD_CDB_FIS_IO_STATUS_OK},
}; // to do: Once there is entry to mark the entry illegal replace entry for row with null value with that illegal entry

const static fbe_dmo_thrsh_cdb_fis_error_flags_t fbe_dmo_type_action[] =
{
    {"noError",                 FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR},
    {"recoveredError",          FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_RECOVERED},
    {"mediaError",              FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_MEDIA},
    {"hardwareError",           FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE},
    {"linkError",               FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_LINK},
    {"dataError",               FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_DATA},
    {"remapped",                FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_REMAPPED},
    {"notSpinning",             FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NOT_SPINNING},
    {"proactiveCopy",           FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_END_OF_LIFE},
    {"proactiveCopyCallHome",   FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_END_OF_LIFE_CALLHOME},
    {"fatal",                   FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_FATAL},
    {"fatalCallHome",           FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_FATAL_CALLHOME},
    {"unknown",                 FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_UKNOWN},
    {NULL,                      FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_INVALID},

};


typedef struct fbe_dmo_attribute_record_s{
    fbe_u8_t                       *name;
    fbe_u32_t                      type_flag;
    void *                         default_value;
}
fbe_dmo_attribute_record_t;


/* Defines Attribute table for Global Parameters */
const static fbe_dmo_attribute_record_t dmo_global_parameter_table[] =
{
    {FBE_DMO_XML_ATTR_DRV_SERVICE_TIMEOUT_STR,           FBE_DMO_GLOBAL_PARAM_ATTR_SERVICE_TIME,          (void*)FBE_DCS_PARAM_USE_DEFAULT},
    {FBE_DMO_XML_ATTR_DRV_REMAP_SERVICE_TIMEOUT_STR,     FBE_DMO_GLOBAL_PARAM_ATTR_REMAP_SERVICE_TIME,    (void*)FBE_DCS_PARAM_USE_DEFAULT},
    {FBE_DMO_XML_ATTR_DRV_EMEH_SERVICE_TIMEOUT_STR,      FBE_DMO_GLOBAL_PARAM_ATTR_EMEH_SERVICE_TIME,     (void*)FBE_DCS_PARAM_USE_DEFAULT},
    {NULL, 0, 0}
};


/*************************************************************************
 **                             Imports/ Externs
 *************************************************************************/


/*************************************************************************
 **                             Functions
 *************************************************************************/


/*************************************************************************
 ** Proto-types. Defined as prototypes more as they are private to this file
 *************************************************************************/
static void fbe_dmo_drv_info_initialize(fbe_drv_info_element_parser_t *element_parser_ptr);
static void fbe_dmo_drv_excp_code_initialize(fbe_drv_excp_code_element_parser_t *element_parser_ptr);
static void fbe_dmo_drv_stats_initialize(fbe_drv_stats_element_parser_t *element_parser_ptr);
static void fbe_dmo_drv_global_parameters_initialize(fbe_dmo_global_parameters_t *element_parser_ptr);
static void fbe_stats_parser_initialize(fbe_stats_attr_parser_t * parser_ptr, fbe_stats_control_flag_t default_control_flag);

static fbe_port_type_t fbe_dmo_thrsh_process_port_type(const char* value);

static void fbe_dmo_thrsh_process_element(fbe_dmo_thrsh_parser_t *dmo_xml_parser, const char *element, const char **attributes);
static void fbe_dmo_thrsh_process_end_element(fbe_dmo_thrsh_parser_t *dmo_xml_parser, const char *element);

static fbe_stats_errcat_attr_t  get_error_category_attribute(const char * attr_name);
static fbe_stats_wtx_attr_t     get_weight_change_attribute(const char * attr_name);
static fbe_bool_t               is_error_category_action_attr(fbe_stats_errcat_attr_t attr);
static fbe_bool_t               add_error_category_action(fbe_stats_attr_parser_t *stats, fbe_stats_errcat_attr_t attr, const char* stats_attr_value);
static fbe_bool_t               dmo_dieh_add_weight_exception(fbe_stats_attr_parser_t *stats_p, fbe_stat_weight_exception_code_t *wtx);
static fbe_stat_action_flags_t  attr_to_action_flag(fbe_stats_errcat_attr_t attr);
static void                     dmo_dieh_initialize_weight_exception(fbe_stat_weight_exception_code_t *wtx);
static void                     dmo_dieh_set_sense_code(fbe_stat_scsi_sense_code_t *sense_code, fbe_u32_t temp_value);
static fbe_u32_t                dmo_dieh_get_num_attributes(const char** attributes);
static const char*              dmo_dieh_get_element_str(const fbe_dmo_xml_element_tag_type_t element);
static void                     fbe_dmo_thrsh_process_error_classify(fbe_dmo_thrsh_parser_t *dmo_xml_parser, fbe_dmo_xml_element_tag_type_t element_type, const char **attributes, fbe_u32_t num_attributes);
fbe_status_t                    dmo_dieh_parse_string(const fbe_u8_t * delimiter, fbe_u32_t min_fields, fbe_u32_t max_fields, fbe_u8_t * in_string, fbe_dmo_attribute_field_array_t out_fields_array_p, fbe_u32_t * out_num_fields_p);
static const fbe_dmo_attribute_record_t * fbe_dmo_get_attribute(const fbe_dmo_attribute_record_t *table, const char * attr_name);


/*!*********************************************************************
 * @file fbe_drive_mgmt_dieh_xml_parse() 
 **********************************************************************
 *
 * @brief
 *  This function parses the provided XML file. File handlers are
 *  invoked and EXPAT parser is initialized. Then the configuration
 *  XML file is parsed. 
 *
 * @param  drive_mgmt  - [I] drive mgmt object
 * @param  filename,   - [I] File to parse.
 * @param  start_handler, - [I] Function for Expat to use as callback.
 * @param  end_handler, - [I] Function for Expat to use as callback.
 * @param  data_handler, - [I] Function for Expat to use as callback.
 *  
 * @return
 *   DIEH_XML_FILE_READ_ERROR - not able to find the file or read it
 *   DIEH_XML_PARSE_ERROR - error parsing the file
 *   DIEH_XML_SUCCESS - no errors
 * 
 * @author
 *  03/20/12   Wayne Garrett -  Ported from Flare.
 *
 **********************************************************************/

dieh_xml_error_info_t  fbe_drive_mgmt_dieh_xml_parse( fbe_drive_mgmt_t * drive_mgmt,
                                                      char * filename,
                                                      XML_StartElementHandler start_handler,
                                                      XML_EndElementHandler end_handler,
                                                      XML_CharacterDataHandler data_handler )
{
    fbe_u32_t   bytes_read;
    fbe_file_handle_t inputFile;    /* Handle for accessing file. */
    void *xml_parser;               /* Parser used by expat. */ 
    fbe_bool_t done = FBE_FALSE;    /* Flag to check on the parsing progress. */
    
    
    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: file=%s\n", __FUNCTION__, filename);    
        
        
    /* open file */
    if( (inputFile = fbe_file_open(filename, FBE_FILE_RDONLY, 0, NULL)) == FBE_FILE_INVALID_HANDLE )
    {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: can't find file %s\n", __FUNCTION__, filename);

        return DIEH_XML_FILE_READ_ERROR;
    }    

     
    /* Initialize the EXPAT parser and set the user implemented callback 
     * functions to point to the EXPAT Callback.
     */

    xml_parser = (XML_Parser) fbe_parser_create(NULL);

    fbe_set_user_data(xml_parser, drive_mgmt);  /*user_data is passed to callback functions*/

    fbe_set_element_handler(xml_parser, start_handler, end_handler);
    
    fbe_set_character_data_handler(xml_parser, data_handler);

    /* Now that the EXPAT is initialized and file is all set to be read,
     * start parsing the file.
     */
    while(!done)
    {
        char buff[DIEH_XML_BUFFER_BYTES];
        bytes_read = fbe_file_read(inputFile, buff, sizeof(buff), NULL);
        
        if (bytes_read < sizeof(buff))
        {
            /* We reached the end of the file, set done so we exit loop.
             */
           done = TRUE;
        }
        
        /* Parse the data we just read in from file.
         */
        if( !fbe_expat_parse((XML_Parser)xml_parser, 
                             (char*)buff, 
                             (int)bytes_read, /*SAFEBUG*//* Bad casting to 32-bit in SAFE environment.*/
                             done) )
        {
            /* An error was encountered while parsing.
             */
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "DIEH parsing failed, file: %s, error %s, at line %d\n", filename,
                                  fbe_error_string(fbe_get_error_code((XML_Parser)xml_parser)),
                                  fbe_get_current_line_number((XML_Parser)xml_parser) );

            /* Free the parser, memory and close file handle,
             * since we will be returning here.
             */
            fbe_parser_free( (XML_Parser)xml_parser ); 
            fbe_file_close(inputFile);
            return DIEH_XML_PARSE_ERROR;
            
        }/* End if parsing error. */

    } /* End while not done. */

    /* Free the parser, memory and close file handle,
     * since we are done.
     */
    fbe_parser_free( (XML_Parser)xml_parser );
    fbe_file_close(inputFile);
    return DIEH_XML_SUCCESS;
}





/**********************************************************************
 * fbe_dmo_drv_stats_initialize()
 **********************************************************************
 *  Precondition: Valid non-null pointer to Drive Statistics Element Struct
 *********************************************************************/      
static void fbe_dmo_drv_info_initialize(fbe_drv_info_element_parser_t *element_parser_ptr){   
     
    if(element_parser_ptr == NULL){
        return;
    }       
    element_parser_ptr->drv_info_proc = NO_DRV_INFO_PROC;       
    element_parser_ptr->drive_info.drive_vendor = FBE_DRIVE_VENDOR_ILLEGAL;
    
    strcpy(element_parser_ptr->drive_info.part_number, "");
    strcpy(element_parser_ptr->drive_info.fw_revision, "");
    
    strcpy((char *)element_parser_ptr->drive_info.serial_number_start, "");
    strcpy((char *)element_parser_ptr->drive_info.serial_number_end, "");

    return;
}

/*!*********************************************************************
 * @file fbe_dmo_global_param_set_default() 
 **********************************************************************
 *
 * @brief
 *  Set the default value based on what is provided in the attribute record.
 * 
 * @author
 *  06/20/15   Wayne Garrett -  Created.
 *
 **********************************************************************/
static void fbe_dmo_global_param_set_default(const char * attr_name, void ** data_member_p)
{
    fbe_dmo_attribute_record_t const *attr;

    *data_member_p = 0;

    attr = fbe_dmo_get_attribute(dmo_global_parameter_table, attr_name);
    if (attr != NULL)
    {
        *data_member_p = attr->default_value;
    }
    else
    {
        fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: Failed to find attr %s in table\n", __FUNCTION__, attr_name);                
    }
}

/*!*********************************************************************
 * @file fbe_dmo_drv_global_parameters_initialize() 
 **********************************************************************
 *
 * @brief
 *  Initialized the global_parameter structure which is used when
 *  parsing the XML file.
 * 
 * @author
 *  06/20/15   Wayne Garrett -  Created.
 *
 **********************************************************************/
static void fbe_dmo_drv_global_parameters_initialize(fbe_dmo_global_parameters_t *global_params_p)
{

    fbe_zero_memory(global_params_p, sizeof(*global_params_p));  
    global_params_p->set_flags = FBE_DMO_GLOBAL_PARAM_ATTR_NONE;

    fbe_dmo_global_param_set_default(FBE_DMO_XML_ATTR_DRV_SERVICE_TIMEOUT_STR,        (void**)&global_params_p->params.service_time_limit_ms);
    fbe_dmo_global_param_set_default(FBE_DMO_XML_ATTR_DRV_REMAP_SERVICE_TIMEOUT_STR,  (void**)&global_params_p->params.remap_service_time_limit_ms);
    fbe_dmo_global_param_set_default(FBE_DMO_XML_ATTR_DRV_EMEH_SERVICE_TIMEOUT_STR,   (void**)&global_params_p->params.emeh_service_time_limit_ms);
}

/**********************************************************************
 * fbe_dmo_drv_excp_code_initialize()
 **********************************************************************
 *  Precondition: Valid non-null pointer to Drive Statistics Element Struct
 *********************************************************************/      
static void fbe_dmo_drv_excp_code_initialize(fbe_drv_excp_code_element_parser_t *element_parser_ptr){
    // Precondition for this function is the we should have valid element_parser_ptr
    // Check to make sure the ptr is not NULL.
     
    if(element_parser_ptr == NULL){
        return;
    }     

    //Initialize 
    element_parser_ptr->excp_list_flag = NO_DRV_EXCP_CODE_PROC;
    element_parser_ptr->excp_list_err_any = DRV_EXCP_NO_ERROR_RCD;

    element_parser_ptr->excp_code_cnt = 0;
    memset(&element_parser_ptr->excp_code[0], 0, sizeof(fbe_drive_config_scsi_fis_exception_code_t)*MAX_EXCEPTION_CODES);         
    return;
}


/**********************************************************************
 * fbe_dmo_drv_stats_initialize()
 **********************************************************************
 *  Precondition: Valid non-null pointer to Drive Statistics Element Struct
 *********************************************************************/      
static void fbe_dmo_drv_stats_initialize(fbe_drv_stats_element_parser_t *element_parser_ptr){
    
    if(element_parser_ptr == NULL){
        return;
    }
    /*Set the the threshold to false and set thresholds to be too large to indicate the value has to be processed*/
    element_parser_ptr->drv_stats_flag = NO_DRV_STATS_PROC;
    memset(&element_parser_ptr->drv_stats, 0, sizeof(fbe_drive_stat_t));
    //element_parser_ptr->drv_stats = false;        
    return;
}



void dmo_drv_xml_info_initialize(fbe_esp_dmo_driveconfig_xml_info_t *xml_info_p)
{
    if (xml_info_p == NULL){
        return;
    }
    fbe_zero_memory(xml_info_p->version, sizeof(xml_info_p->version));
    fbe_zero_memory(xml_info_p->description, sizeof(xml_info_p->description));
    fbe_zero_memory(xml_info_p->filepath, sizeof(xml_info_p->filepath));
}

/**********************************************************************
 * fbe_drv_config_parser_initialize()
 **********************************************************************
 *  
 *********************************************************************/      
void dmo_drv_config_parser_initialize(fbe_dmo_drv_config_parser_t * parser_ptr){
    
    if(parser_ptr == NULL){
        return;
    }    
    parser_ptr->drv_config_flags = NO_DRV_CONFIG_PROC;
    parser_ptr->default_control_flag =  FBE_STATS_CNTRL_FLAG_DEFAULT;
    parser_ptr->num_sas_drv_rcd_prcd = 0;
    fbe_dmo_drv_stats_initialize(&parser_ptr->drv_stats_elem_parser);
    fbe_dmo_drv_excp_code_initialize(&parser_ptr->drv_excp_code_parser);
    fbe_dmo_drv_info_initialize(&parser_ptr->drv_info_parser);
    fbe_dmo_drv_global_parameters_initialize(&parser_ptr->global_parameters);
    

    return;
}


/**********************************************************************
 * fbe_port_config_parser_initialize()
 **********************************************************************
 *  
 *********************************************************************/      
void dmo_port_config_parser_initialize(fbe_port_configs_parser_ptr parser_ptr){
    
    if(parser_ptr == NULL){
        return;
    }
    
    parser_ptr->port_configs_flag = NO_PORT_CONFIGS_PROC;
    parser_ptr->num_port_config_rcd_prcd = 0;   

    parser_ptr->port_config.port_config_flag = NO_PORT_CONFIG_PROC;

    return;
}


/**********************************************************************
 * fbe_stats_parser_initialize()
 **********************************************************************
 *  
 *********************************************************************/      
static void fbe_stats_parser_initialize(fbe_stats_attr_parser_t * parser_ptr, fbe_stats_control_flag_t default_control_flag){
    fbe_u32_t i;

    if(parser_ptr == NULL){
        return;
    }   
    parser_ptr->stats_flag = FBE_NO_STATS_PROC;
    memset(&parser_ptr->fbe_stat, 0, sizeof(fbe_stat_t));   

    parser_ptr->fbe_stat.control_flag = default_control_flag;

    for (i=0; i<FBE_STAT_MAX_WEIGHT_EXCEPTION_CODES; i++)
    {
        dmo_dieh_initialize_weight_exception(&parser_ptr->fbe_stat.weight_exceptions[i]);
    }
}


/**********************************************************************
 * fbe_queuing_parser_initialize()
 **********************************************************************
 *  
 *********************************************************************/      
void fbe_queuing_parser_initialize(fbe_dmo_queuing_parser_t * parser_ptr){
    
    if(parser_ptr == NULL){
        return;
    }    
    parser_ptr->queuing_record.lpq_timer = ENHANCED_QUEUING_TIMER_MS_INVALID;
    parser_ptr->queuing_record.hpq_timer = ENHANCED_QUEUING_TIMER_MS_INVALID;

    parser_ptr->queuing_record.drive_info.drive_vendor = FBE_DRIVE_VENDOR_INVALID;
    strcpy(parser_ptr->queuing_record.drive_info.part_number, "");
    strcpy(parser_ptr->queuing_record.drive_info.fw_revision, "");
    strcpy((char *)parser_ptr->queuing_record.drive_info.serial_number_start, "");
    strcpy((char *)parser_ptr->queuing_record.drive_info.serial_number_end, "");

    return;
}

/*!*********************************************************************
 * @file fbe_dmo_update_dcs_global_parameter() 
 **********************************************************************
 *
 * @brief
 *  Send the global_parameter changes to the Drive Configuration Service (DCS).
 * 
 * @author
 *  06/20/15   Wayne Garrett -  Created.
 *
 **********************************************************************/
void fbe_dmo_update_dcs_global_parameter(fbe_dmo_global_parameters_t  * global_params_p)
{
    fbe_status_t status;
    fbe_dcs_tunable_params_t dcs_params = {0};    

    status = fbe_api_drive_configuration_interface_get_params(&dcs_params);
    if (status != FBE_STATUS_OK)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: get dcs params failed \n", __FUNCTION__);        
        return;
    }
    
    /* Update dcs_params based on what was supplied in Config file or what it was initialized
       to if attr was not supplied. */

    dcs_params.service_time_limit_ms =          global_params_p->params.service_time_limit_ms;
    dcs_params.remap_service_time_limit_ms =    global_params_p->params.remap_service_time_limit_ms;
    dcs_params.emeh_service_time_limit_ms = global_params_p->params.emeh_service_time_limit_ms;

    status = fbe_api_drive_configuration_interface_set_params(&dcs_params);
    if (status != FBE_STATUS_OK)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: Set Global Parameters failed \n", __FUNCTION__);        
        return;
    }
}


/**********************************************************************
 * fbe_dmo_thrsh_parser_initialize()
 **********************************************************************
 *  
 *********************************************************************/      
void fbe_dmo_thrsh_parser_initialize(fbe_dmo_thrsh_parser_t *parser_ptr)
{
    /* Precondition for this function is the we should have valid xml_element_ptr
     * Check to make sure the ptr is not NULL.
     */
    if(parser_ptr == NULL){
        return;
    }    

    parser_ptr->doc_state = FBE_DMO_THRSH_DOC_INIT;
    parser_ptr->status = FBE_DMO_THRSH_NO_ERROR;

    dmo_drv_xml_info_initialize(&parser_ptr->xml_info);
    dmo_drv_config_parser_initialize(&parser_ptr->drv_config);
    
    fbe_stats_parser_initialize(&parser_ptr->stats, parser_ptr->drv_config.default_control_flag);
    fbe_queuing_parser_initialize(&parser_ptr->queuing_config);    

    return;
}


/**********************************************************************
 * fbe_dmo_thrsh_are_fbe_stats_valid() 
 **********************************************************************
 *  Function takes in drive stats  structure and verifies if all the 
 *  drive statistic element attributes have been processedvalue 
 *  Parameters:
 *      FBE_STATS_ATTR_PARSER stats:    Passed in the stats structure 
 *  Returns
 *      bool:       True if all the attributes have been processed; 
 *                  False if any of the attribute has not been processed
 **********************************************************************/
static __forceinline bool fbe_dmo_thrsh_are_fbe_stats_valid(fbe_stats_attr_parser_t stats){

    bool stats_valid = false;
    // Check if required attributes were processed
    if( stats.stats_flag & FBE_STATS_INTV_PROC && 
        stats.stats_flag & FBE_STATS_WT_PROC   &&
        !(stats.stats_flag & FBE_STATS_INVALID_PROC) ){
        stats_valid = true;
    }else{
        stats_valid = false;
        //KTRACE("FBE DMO: XML Parser Statistics - Invalid threshold value detected!", 0, 0, 0, 0);             
    }   
    return stats_valid;
}

/*TODO: is there an fbe min macro?*/
#define MIN(a,b) ((a) < (b) ? (a) : (b))

/**********************************************************************
 * fbe_dmo_thrsh_process_dmo_thrsh_cfg_tag() 
 **********************************************************************
 *  Parses DMO Thresh Tag for version info.  If an older version is
 *  detected then function will return false, indicating XML file can't
 *  be parsed.  XML version information will be in the format
 *  "(major).(minor)".  Only the major part is checked.   The major
 *  part represents changes made to the parser, while the minor part
 *  represents XML setting changes.
 * 
 * 
 *  Parameters:
 *      parser     :  Ptr to XML Parser
 *      attributes :  Ptr to the attributes of the default control tag
 *  Returns
 *      true if version is supported.
 **********************************************************************/
fbe_bool_t fbe_dmo_thrsh_process_dmo_thrsh_cfg_tag(fbe_dmo_thrsh_parser_t *dmo_xml_parser_p, const char **attributes)
{
    fbe_u32_t i;
    csx_string_t saveptr;
    fbe_u32_t len;
    fbe_bool_t is_supported = FALSE;

    i=0;
    while (attributes[i] != NULL)
    {
        if(!strcmp(attributes[i], FBE_DMO_XML_ATTR_VERSION_STR))
        {       
            char version_str[FBE_DMO_DRIVECONFIG_XML_MAX_VERSION_LEN+1];
            char * major_p;

            fbe_zero_memory(version_str, sizeof(version_str));  

            strncpy(version_str, attributes[i+1], MIN(strlen(attributes[i+1]),FBE_DMO_DRIVECONFIG_XML_MAX_VERSION_LEN));
            
            major_p = csx_p_strtok_r(version_str, ".", &saveptr);

            if (major_p == NULL){
                    fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                             "%s: DIEH XML Parser - Major Version Not Found\n", __FUNCTION__);
                return FBE_FALSE;
            }

            if(strcmp(major_p, FBE_DMO_THRSH_XML_VERSION_MAJOR) != 0){
                /* not supported.*/
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s: DIEH XML Parser - Major Version [%s.xxx] Not Supported\n", __FUNCTION__, major_p);      
                return FBE_FALSE;   
            }

            is_supported = FBE_TRUE;

            len = MIN((fbe_u32_t)strlen(attributes[i+1]),FBE_DMO_DRIVECONFIG_XML_MAX_VERSION_LEN);
            strncpy(dmo_xml_parser_p->xml_info.version, attributes[i+1], len);
            dmo_xml_parser_p->xml_info.version[len] = '\0';
        }
        else if(!strcmp(attributes[i], FBE_DMO_XML_ATTR_DESCRIPTION_STR))
        {
            len = MIN((fbe_u32_t)strlen(attributes[i+1]),FBE_DMO_DRIVECONFIG_XML_MAX_DESCRIPTION_LEN);
            strncpy(dmo_xml_parser_p->xml_info.description, attributes[i+1], len);
            dmo_xml_parser_p->xml_info.description[len] = '\0';
        }

        i+=2;
    }

    return is_supported;  
}

/**********************************************************************
 * fbe_dmo_thrsh_process_port_type() 
 **********************************************************************
 *  Function parses for port type attribute of port configuration and 
 *  extracts port type 
 *  Parameters:
 *      char * value:   Passed in the String representation of port type
 *  Returns
 **********************************************************************/

static fbe_port_type_t fbe_dmo_thrsh_process_port_type(const char* value){
    int index = 0;
    fbe_port_type_t port_type = FBE_PORT_TYPE_INVALID;  
    
    while( (fbe_dmo_xml_attr_port_type_values[index].port_val != NULL) && 
           (strcmp(value, fbe_dmo_xml_attr_port_type_values[index].port_val )) )
    {
        index++;
    }
    
    port_type = fbe_dmo_xml_attr_port_type_values[index].port_type;

    return port_type;
}

/**********************************************************************
 * fbe_dmo_thrsh_process_default_control_flag() 
 **********************************************************************
 *  Parses the default control tag.  The attribute for this tag controls
 *  the behavior of the DIEH error handling algorithm.
 * 
 *  Parameters:
 *      drv_config :  Ptr to the parsers drive configuration elememnt
 *      attributes :  Ptr to the attributes of the default control tag
 *  Returns
 **********************************************************************/
void fbe_dmo_thrsh_process_default_control_flag(fbe_dmo_drv_config_parser_t *drv_config, const char **attributes){
    fbe_u32_t temp_value = DMO_MAX_UINT_32;

    if(!strcmp(attributes[0], FBE_DMO_XML_ATTR_CONTROL_FLAG_TAG_STR)){
        temp_value = dmo_str2int(attributes[1]);
        if(temp_value != DMO_MAX_UINT_32){
            drv_config->default_control_flag = temp_value;
        }else{
            fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s: DIEH XML Parser Statistics - Invalid Default control_flag value detected!\n", __FUNCTION__);               
        }
    }            
}

/*!**************************************************************
 * @fn fbe_dmo_thrsh_process_fbe_stats
 ****************************************************************
 * @brief
 *  Processes an error categories attributes.   If any error is
 *  detected then the FBE_STATS_INVALID_PROC attrib flag will 
 *  be set.  
 * 
 * @param stats - pointer to parser tracking structure
 * @param stats_attr -  pointer to attributes.
 * 
 * @return none 
 *
 * @version
 *   07/02/2012:  Wayne Garrett - Created.
 *
 ****************************************************************/
void fbe_dmo_thrsh_process_fbe_stats(fbe_stats_attr_parser_t *stats, const char **stats_attr){

    fbe_u32_t temp_value = DMO_MAX_UINT_32;
    fbe_stats_errcat_attr_t   attr;
    fbe_bool_t is_valid_value = FBE_TRUE;
    fbe_bool_t is_add_action_success = FBE_TRUE;
    fbe_u32_t name_i = 0;
    fbe_u32_t value_i = name_i + 1;

    while (NULL != stats_attr[name_i]  && 
           NULL != stats_attr[value_i])
    {        
        attr = get_error_category_attribute(stats_attr[name_i]);

        value_i = name_i+1;

        switch(attr)
        {
            case FBE_STATS_THRSH_PROC:
                temp_value = dmo_str2int(stats_attr[value_i]);
                if(DMO_MAX_UINT_32 == temp_value) {
                    is_valid_value = FBE_FALSE;
                    break;
                }
                stats->fbe_stat.threshold = temp_value;               
                stats->stats_flag |= attr;                   
                break;     
                           
            case FBE_STATS_INTV_PROC:
                temp_value = dmo_str2int(stats_attr[value_i]);
                if(DMO_MAX_UINT_32 == temp_value) {
                    is_valid_value = FBE_FALSE;
                    break;
                }
                stats->fbe_stat.interval = temp_value;
                stats->stats_flag |= attr;                   
                break;

            case FBE_STATS_WT_PROC:
                temp_value = dmo_str2int(stats_attr[value_i]);
                if(DMO_MAX_UINT_32 == temp_value) {
                    is_valid_value = FBE_FALSE;
                    break;
                }
                stats->fbe_stat.weight = temp_value;
                stats->stats_flag |= attr;                   
                break; 
                               
            case FBE_STATS_CNTRL_FLG_PROC:
                temp_value = dmo_str2int(stats_attr[value_i]);
                if(DMO_MAX_UINT_32 == temp_value) {
                    is_valid_value = FBE_FALSE;
                    break;
                }
                stats->fbe_stat.control_flag = temp_value;
                stats->stats_flag |= attr;                   
                break;

            /* Action attributes */
            case FBE_STATS_RESET_PROC:                      
            case FBE_STATS_EOL_PROC:
            case FBE_STATS_EOL_CH_PROC:
            case FBE_STATS_KILL_PROC:
            case FBE_STATS_KILL_CH_PROC:
            case FBE_STATS_EVENT_PROC:
                is_add_action_success = add_error_category_action(stats, attr, stats_attr[value_i]);
                break;

            case FBE_STATS_INVALID_PROC:                   
            default:
                stats->stats_flag |= FBE_STATS_INVALID_PROC;
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s: DIEH XML - Invalid attr name %s %d\n", __FUNCTION__, stats_attr[name_i], attr);
                break;
        }                    

        if (!is_valid_value ||
            !is_add_action_success){
            break;  /* problem.  exiting parsing.*/
        }

        name_i += 2;  /* all attrs have a name/value pair.  increment to next attr name*/
    }

    /* do some error checking 
    */
    if ( NULL != stats_attr[name_i] && 
         NULL == stats_attr[value_i] )
    {
        stats->stats_flag |= FBE_STATS_INVALID_PROC;
        fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s: DIEH XML - Provided attr name but no value. name=%s\n", __FUNCTION__, stats_attr[name_i]);        
        return;
    }

    if (!is_valid_value)
    {
        stats->stats_flag |= FBE_STATS_INVALID_PROC;
        fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: DIEH XML - Invalid value %s=%s\n", __FUNCTION__, stats_attr[name_i], stats_attr[value_i]);
        return;
    }

    if (!is_add_action_success)
    {
        stats->stats_flag |= FBE_STATS_INVALID_PROC;
        fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s: DIEH XML - add action failed. %s=%s\n", __FUNCTION__, stats_attr[name_i], stats_attr[value_i]); 
        return;
                                                                                     
    }
}

/*!**************************************************************
 * @fn fbe_dmo_thrsh_process_weight_change()
 ****************************************************************
 * @brief Processes the weight change exception for a given
 *        error category.
 * 
 * @param stats - pointer to parser tracking structure
 * @param stats_attr -  pointer to attributes.
 * 
 * @return none 
 *
 * @version
 *   11/30/2012:  Wayne Garrett - Created.
 *
 ****************************************************************/
void fbe_dmo_thrsh_process_weight_change(fbe_stats_attr_parser_t *stats_p, const char **stats_attr)
{
    fbe_bool_t is_valid_value = FBE_TRUE;
    fbe_u32_t temp_value = DMO_MAX_UINT_32;
    fbe_u32_t name_i = 0;   
    fbe_u32_t value_i = name_i + 1;
    fbe_stat_weight_exception_code_t wtx;
    fbe_stats_wtx_attr_t attr;

    dmo_dieh_initialize_weight_exception(&wtx);


    /* expected formats are:
         opcode="0xHH" change="D+" 
         cc="0xHHHHHHHH" change="D+"  cc format is byte3=SK, byte2=ASC, btye1=ASCQ_start, byte0=ASCQ_end
         port="D+" change="D+"
    */
     
        
    while (NULL != stats_attr[name_i]  && 
           NULL != stats_attr[value_i])
    {   
        attr = get_weight_change_attribute(stats_attr[name_i]);
        
        switch(attr)
        {
            case FBE_STATS_WTX_OPCODE_PROC:
                /* validate opcode.  format = 0xHH */
                if(strncmp(stats_attr[value_i], "0x", 2) != 0 ||
                   strlen(stats_attr[value_i]) != 4)
                {
                    is_valid_value = FBE_FALSE;
                    break;
                }

                temp_value = dmo_str2int(stats_attr[value_i]);
                if(DMO_MAX_UINT_32 == temp_value) {
                    is_valid_value = FBE_FALSE;
                    break;
                }
                          
                if (wtx.type != FBE_STAT_WEIGHT_EXCEPTION_INVALID)  /* test this wasn't already set*/
                {
                    is_valid_value = FBE_FALSE;
                    break;
                }

                wtx.type = FBE_STAT_WEIGHT_EXCEPTION_OPCODE;
                wtx.u.opcode = temp_value;
                break;     
    
            case FBE_STATS_WTX_CC_ERROR_PROC:
                /* validate scsi cc error.  format = 0xHHHHHHHHHH */
                if(strncmp(stats_attr[value_i], "0x", 2) != 0 ||
                   strlen(stats_attr[value_i]) != 10)
                {
                    is_valid_value = FBE_FALSE;
                    break;
                }

                temp_value = dmo_str2int(stats_attr[value_i]);
                if(DMO_MAX_UINT_32 == temp_value) {
                    is_valid_value = FBE_FALSE;
                    break;
                }

                if (wtx.type != FBE_STAT_WEIGHT_EXCEPTION_INVALID)  /* test this wasn't already set*/
                {
                    is_valid_value = FBE_FALSE;
                    break;
                }

                wtx.type = FBE_STAT_WEIGHT_EXCEPTION_CC_ERROR;
                dmo_dieh_set_sense_code(&wtx.u.sense_code, temp_value);
                break;

            case FBE_STATS_WTX_PORT_ERROR_PROC:
                /* format = decimal */
                temp_value = dmo_str2int(stats_attr[value_i]);
                if(DMO_MAX_UINT_32 == temp_value) {
                    is_valid_value = FBE_FALSE;
                    break;
                }

                if (wtx.type != FBE_STAT_WEIGHT_EXCEPTION_INVALID)  /* test this wasn't already set*/
                {
                    is_valid_value = FBE_FALSE;
                    break;
                }

                wtx.type = FBE_STAT_WEIGHT_EXCEPTION_PORT_ERROR;
                wtx.u.port_error = temp_value;                             
                break;

            case FBE_STATS_WTX_CHANGE_PROC:
                /* format = decimal */
                temp_value = dmo_str2int(stats_attr[value_i]);
                if(DMO_MAX_UINT_32 == temp_value) {
                    is_valid_value = FBE_FALSE;
                    break;
                }

                if (wtx.change != FBE_STAT_WTX_CHANGE_INVALID)  /* test this wasn't already set*/
                {
                    is_valid_value = FBE_FALSE;
                    break;
                }
                wtx.change = temp_value;
                break;

            default:
                stats_p->stats_flag |= FBE_STATS_INVALID_PROC;
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s: DIEH XML - Invalid attr name %s %d\n", __FUNCTION__, stats_attr[name_i], attr);
        }

        if (!is_valid_value){
            break;  /* problem.  exiting parsing.*/
        }

        name_i += 2;  /* all attrs have a name/value pair.  increment to next attr name*/
        value_i = name_i + 1;
    }


    /* do some error checking
    */
    if (!is_valid_value)
    {
        stats_p->stats_flag |= FBE_STATS_INVALID_PROC;
        fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: DIEH XML - Invalid value %s=%s\n", __FUNCTION__, stats_attr[name_i], stats_attr[value_i]);
        return;
    }

    if (FBE_STAT_WEIGHT_EXCEPTION_INVALID == wtx.type ||
        FBE_STAT_WTX_CHANGE_INVALID == wtx.change)
    {
        stats_p->stats_flag |= FBE_STATS_INVALID_PROC;
        fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: DIEH XML - Not all fields set: type=%d, change=0x%x\n", __FUNCTION__, wtx.type, wtx.change);
        return;

    }

    /* add it */
    if (! dmo_dieh_add_weight_exception(stats_p,&wtx))
    {
        stats_p->stats_flag |= FBE_STATS_INVALID_PROC;
        fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: DIEH XML - Failed to add. max exceptions=%d\n", __FUNCTION__, FBE_STAT_MAX_WEIGHT_EXCEPTION_CODES);
        return;
    }
}



/*!**************************************************************
 * @fn fbe_dmo_get_attribute
 ****************************************************************
 * @brief
 *  Generic function to return an attribute record.
 * 
 * @param attr_name - string name for attribute.
 * 
 * @return attribute record
 *
 * @version
 *   6/18/2015:  Wayne Garrett - Created.
 *
 ****************************************************************/
static const fbe_dmo_attribute_record_t *     
fbe_dmo_get_attribute(const fbe_dmo_attribute_record_t *table, const char * attr_name)
{
    fbe_u32_t i = 0;

    if (attr_name == NULL)
    {
        return NULL;
    }
    
    while (table[i].name != NULL)
    {
        if (strncmp(attr_name, table[i].name, FBE_DMO_ATTR_STR_MAX) == 0)
        {
            return &table[i];
        }
        i++;
    }

    return NULL;
}


/*!**************************************************************
 * @fn fbe_dmo_thrsh_process_global_parameters()
 ****************************************************************
 * @brief Processes global drive related parameters.  The global
 *    parameters have already been initialized with current defaults.
 *    Only an attribute that is provided and valid will be modified.
 * 
 * @param stats - pointer to parser tracking structure
 * @param stats_attr -  pointer to attributes.
 * 
 * @return none 
 *
 * @version
 *   06/17/2015:  Wayne Garrett - Created.
 *
 ****************************************************************/
void fbe_dmo_thrsh_process_global_parameters(fbe_dmo_global_parameters_t *global_parameters_p, const char **stats_attr)
{    
    fbe_u32_t temp_value = DMO_MAX_UINT_32;
    fbe_u32_t name_i = 0;   
    fbe_u32_t value_i = name_i + 1;
    fbe_dmo_attribute_record_t const * attr = NULL;

    global_parameters_p->set_flags = 0;

    attr = fbe_dmo_get_attribute(dmo_global_parameter_table, stats_attr[name_i]);

    while (attr != NULL)
    {           
        switch (attr->type_flag)
        {
        case FBE_DMO_GLOBAL_PARAM_ATTR_SERVICE_TIME:
            temp_value = dmo_str2int(stats_attr[value_i]);
            if(temp_value != DMO_MAX_UINT_32){
                global_parameters_p->params.service_time_limit_ms = (fbe_time_t)temp_value;
                global_parameters_p->set_flags |= FBE_DMO_GLOBAL_PARAM_ATTR_SERVICE_TIME;
            }else{
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s: DIEH XML Parser - failed parsing %s=%s\n", __FUNCTION__, stats_attr[name_i], stats_attr[value_i]);
            }
            break;

        case FBE_DMO_GLOBAL_PARAM_ATTR_REMAP_SERVICE_TIME:
            temp_value = dmo_str2int(stats_attr[value_i]);
            if(temp_value != DMO_MAX_UINT_32){                
                global_parameters_p->params.remap_service_time_limit_ms = (fbe_time_t)temp_value;  
                global_parameters_p->set_flags |= FBE_DMO_GLOBAL_PARAM_ATTR_REMAP_SERVICE_TIME;
            }else{
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s: DIEH XML Parser - failed parsing %s=%s\n", __FUNCTION__, stats_attr[name_i], stats_attr[value_i]);
            }
            break;

        case FBE_DMO_GLOBAL_PARAM_ATTR_EMEH_SERVICE_TIME:
            temp_value = dmo_str2int(stats_attr[value_i]);
            if(temp_value != DMO_MAX_UINT_32){
                global_parameters_p->params.emeh_service_time_limit_ms = (fbe_time_t)temp_value;
                global_parameters_p->set_flags |= FBE_DMO_GLOBAL_PARAM_ATTR_EMEH_SERVICE_TIME;
            }else{
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s: DIEH XML Parser - failed parsing %s=%s\n", __FUNCTION__, stats_attr[name_i], stats_attr[value_i]);
            }
            break;

        default:
            fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s: DIEH XML Parser - unknown attr %s [%d]\n", __FUNCTION__, attr->name, attr->type_flag);
        break;
        }

        name_i += 2;  /* all attrs have a name/value pair.  increment to next attr name*/
        value_i = name_i + 1;
        attr = fbe_dmo_get_attribute(dmo_global_parameter_table, stats_attr[name_i]);
    }
}


/*!**************************************************************
 * @fn fbe_dmo_thrsh_xml_start_element()
 ****************************************************************
 * @brief Call back function which is to start the XML processing.
 *
 ****************************************************************/
void fbe_dmo_thrsh_xml_start_element( void *user_data, const char *element, const char **attributes){
    fbe_drive_mgmt_t *drive_mgmt = (fbe_drive_mgmt_t*) user_data;
    fbe_u32_t index;
    fbe_bool_t valid_tag_found = FBE_FALSE;


    /* If a previous error was detected then stop parsing.  This parser does not
       provide a way to abort. By returning immediately it will cause parser
       to skip the remaining elements.
     */
    if (drive_mgmt->xml_parser->doc_state ==  FBE_DMO_THRSH_DOC_ERROR){
        return;
    }
        
    if( (strcmp(element, FBE_DMO_XML_ELEM_DMO_THRSH_CFG_TAG_STR) == 0)) {
        if(drive_mgmt->xml_parser->doc_state == FBE_DMO_THRSH_DOC_NOT_INIT){            
            fbe_dmo_thrsh_parser_initialize(drive_mgmt->xml_parser );
        }           
        drive_mgmt->xml_parser->doc_state = FBE_DMO_THRSH_DOC_PARSE_IN_PRC;
    }
    
    index = 0;
    while(dmo_thrsh_xml_tags[index].tag_name != NULL){
        if(!strcmp(element, dmo_thrsh_xml_tags[index].tag_name)){            
            drive_mgmt->xml_parser->element_tag_type = dmo_thrsh_xml_tags[index].tag_type;
            valid_tag_found = FBE_TRUE;            
            break;
        }        
        index++;
    }

    if(valid_tag_found == FBE_FALSE){
        drive_mgmt->xml_parser->element_tag_type = FBE_DMO_XML_ELEM_DMO_UNDEFINED_TAG;
    }

    fbe_dmo_thrsh_process_element(drive_mgmt->xml_parser, element, attributes);
   
    return;                
}


/*!**************************************************************
 * @fn fbe_dmo_thrsh_process_reliably_challenged()
 ****************************************************************
 * @brief Processes the reliably challenged element.  If there is 
 *        an error during process then the xml parser status will
 *        be set, causing the XML processing to be aborted.
 * 
 * @param stats - pointer to parser tracking structure
 * @param stats_attr -  pointer to attributes.
 * 
 * @return none 
 *
 * @version
 *   06/08/2015:  Wayne Garrett - Created.
 *
 ****************************************************************/
void fbe_dmo_thrsh_process_reliably_challenged(fbe_dmo_thrsh_parser_t* dmo_xml_parser, fbe_dmo_xml_element_tag_type_t element_type, const char **stats_attr)
{    
    fbe_u32_t name_i = 0;   
    fbe_u32_t value_i = name_i + 1;

    if(!strcmp(stats_attr[name_i], FBE_DMO_XML_ATTR_RELIABLY_CHALLENGED_TAG_STR))
    {
        if (!strcmp(stats_attr[value_i],"true"))
        {
            dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats.record_flags |= FBE_STATS_FLAG_RELIABLY_CHALLENGED;            
        }                
        else
        {
            if (strcmp(stats_attr[value_i],"false") != 0)
            {
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                         "DIEH XML Parser Failed - <%s> id:%d - Invalid RC value %s\n", 
                                         dmo_dieh_get_element_str(element_type), element_type, stats_attr[value_i]);         
                dmo_xml_parser->doc_state = FBE_DMO_THRSH_DOC_ERROR;
                dmo_xml_parser->status = FBE_DMO_THRSH_XML_ELEMENT_PARSE_ERROR;
            }
        }
    }
    else
    {
        fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                 "DIEH XML Parser Failed - <%s> id:%d - Invalid RC name %s\n", 
                                 dmo_dieh_get_element_str(element_type), element_type, stats_attr[name_i]);
        dmo_xml_parser->doc_state = FBE_DMO_THRSH_DOC_ERROR;
        dmo_xml_parser->status = FBE_DMO_THRSH_XML_ELEMENT_PARSE_ERROR;
    }
}


/**********************************************************************
 * fbe_dmo_thrsh_xml_data_element() 
 **********************************************************************
 * Handler routine to process data section of xml tags. as there is no 
 * data to be processed for fbe threshold xml file, this function is more 
 * of stub
 ***********************************************************************/

void fbe_dmo_thrsh_xml_data_element(void* data, const char* el, int len){
    return;
}

void fbe_dmo_thrsh_xml_end_element(void *user_data, const char* element){
    fbe_drive_mgmt_t *drive_mgmt = (fbe_drive_mgmt_t*) user_data;
    fbe_u32_t index;    
    
    /* If a previous error was detected then stop parsing.  This parser does not
       provide a way to abort. By returning immediately it will cause parser
       to skip the remaining elements.
     */
    if (drive_mgmt->xml_parser->doc_state == FBE_DMO_THRSH_DOC_ERROR){
        return;
    }

    index = 0;
    while(dmo_thrsh_xml_tags[index].tag_name != NULL){
        if(!strcmp(element, dmo_thrsh_xml_tags[index].tag_name)){            
            drive_mgmt->xml_parser->element_tag_type = dmo_thrsh_xml_tags[index].tag_type;          
            break;        
        }        
        index++;
    }

    fbe_dmo_thrsh_process_end_element(drive_mgmt->xml_parser, element);
}

/**********************************************************************
 * fbe_dmo_thrsh_process_element() 
 **********************************************************************
 *  Function parses the XML element tags, extracts data from element and 
 *  it appropriate data structures.
 *  Parameters:
 **********************************************************************/
static void fbe_dmo_thrsh_process_element(fbe_dmo_thrsh_parser_t *dmo_xml_parser, const char *element, const char **attributes)
{
    fbe_dmo_xml_element_tag_type_t element_type = FBE_DMO_XML_ELEM_DMO_UNDEFINED_TAG;
    fbe_u32_t temp_value;
    fbe_u32_t str_len = 0, index = 0;
    static fbe_u32_t excp_code_cnt = 0;
    fbe_status_t status;
    const fbe_u32_t num_attributes = dmo_dieh_get_num_attributes(attributes);
    
    // Read the XML tag which is being processed    
    element_type = dmo_xml_parser->element_tag_type;
    
    switch(element_type)
    {
        case FBE_DMO_XML_ELEM_DMO_UNDEFINED_TAG:
            // Invalid tag detected in the configuration mark the configuration 
            break;

        case FBE_DMO_XML_ELEM_DMO_THRSH_CFG_TAG:                // Start of the configuration document. No attributes or states to be processed
            if (!fbe_dmo_thrsh_process_dmo_thrsh_cfg_tag(dmo_xml_parser, attributes)){
                /* Failed to parse.  This XML file is not supported. Parsing will be
                   aborted and default values will be used.  Defaults are automatically
                   initialized by the DriveConfiguration service, so no explicit defaults
                   will be added here*/
                dmo_xml_parser->doc_state = FBE_DMO_THRSH_DOC_ERROR;  
                dmo_xml_parser->status = FBE_DMO_THRSH_XML_ELEMENT_PARSE_ERROR;
            }
            break;

        /* OBSOLETE */
        case FBE_DMO_XML_ELEM_DRV_STATS_TAG:    // Element(s) contain drive statistics, process the attributes and extract values of them
        case FBE_DMO_XML_ELEM_ENCL_STATS_TAG:   // Element(s) contain drive statistics, process the attributes and extract values of them
            fbe_stats_parser_initialize(&dmo_xml_parser->stats, dmo_xml_parser->drv_config.default_control_flag);
            fbe_dmo_thrsh_process_fbe_stats(&dmo_xml_parser->stats, attributes);            
            break;

        case FBE_DMO_XML_ELEM_DRV_CFG_TAG:
            {
                fbe_dcs_dieh_status_t dcs_status;

                if (dmo_xml_parser->drv_config.drv_config_flags != NO_DRV_CONFIG_PROC){             
                    fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                             "%s: DIEH XML Parser - More than one Drive Configurations Tags detected!, flag=0x%x\n", 
                                             __FUNCTION__, dmo_xml_parser->drv_config.drv_config_flags);            
                }                

                dcs_status = fbe_api_drive_configuration_interface_start_update();   
                if (dcs_status != FBE_DCS_DIEH_STATUS_OK)
                {           
                     fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: DIEH XML start_update failed. status=%d\n",  __FUNCTION__, dcs_status);                    
                     dmo_xml_parser->doc_state = FBE_DMO_THRSH_DOC_ERROR;
    
                     switch (dcs_status)
                     {
                         case FBE_DCS_DIEH_STATUS_FAILED_UPDATE_IN_PROGRESS:
                             dmo_xml_parser->status = FBE_DMO_THRSH_UPDATE_ALREADY_IN_PROGRESS; 
                             break;
            
                         case FBE_DCS_DIEH_STATUS_FAILED:
                         default:
                             dmo_xml_parser->status = FBE_DMO_THRSH_ERROR; 
                             break;
                     }                 
                }                
            }
            break;
        case FBE_DMO_XML_ELEM_SAS_DRV_CFG_TAG:
            // To begin with mark the drive type as SAS and exception list processed (drive configuration file with no exception 
            // blocks is valid)Drive Information and Statistics blocks needs to be present, so mark them as those blocks have not been processed yet.
            dmo_xml_parser->drv_config.drv_config_flags = NO_DRV_CONFIG_PROC;
            dmo_xml_parser->drv_config.drv_config_flags |= (DRV_CONFIG_SAS_DETECTED | DRV_CONFIG_EXCP_PROC);            
            // Should the number of SAS / SATA drive table processed initialized here ???
            dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag = NO_DRV_STATS_PROC;
            // We must zero SCSI exception codes in the parser and temp drive config record because
            // these do not have to be specified for a SAS drive configuration and we want to avoid
            // adding stale data to the drive table from the last drive config that was parsed.
            fbe_dmo_drv_excp_code_initialize(&dmo_xml_parser->drv_config.drv_excp_code_parser);
            fbe_zero_memory(&dmo_xml_parser->drv_config.drv_config_recd.category_exception_codes[0], sizeof(fbe_drive_config_scsi_fis_exception_code_t)*MAX_EXCEPTION_CODES);            
            break;

        case FBE_DMO_XML_ELEM_CONTROL_DFTL_TAG:
           // Only one Default Control tag is allow once per Drive Configuration tag.  If more than one is supplied then last will
           // take affect.   This tag must come before the Stats tag, otherwise these overriden defaults will not be applied.
           dmo_xml_parser->drv_config.default_control_flag = FBE_STATS_CNTRL_FLAG_DEFAULT;   // initialize
           fbe_dmo_thrsh_process_default_control_flag(&dmo_xml_parser->drv_config, attributes);
           break;

        case FBE_DMO_XML_ELEM_DRV_STATCS_TAG:
            // For each drive configuration there can be only one drive statistics block. Check there was 
            // no other statistics block being processed earlier.
            if(dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag == NO_DRV_STATS_PROC){                
                fbe_dmo_drv_stats_initialize(&dmo_xml_parser->drv_config.drv_stats_elem_parser);
            }else{
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s: DIEH XML Parser Drive Statistics - another record being processed, flag=0x%x\n", 
                                         __FUNCTION__, dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag);            
            }
            break;
        // Process the error category statistics for the drive
        case FBE_DMO_XML_ELEM_CMLT_STATS_TAG:                       //Cumulative Error
        case FBE_DMO_XML_ELEM_RCVR_ERR_STATS_TAG:                   // Recovered Error
        case FBE_DMO_XML_ELEM_MEDIA_ERR_STATS_TAG:                  // Media Error
        case FBE_DMO_XML_ELEM_HEALTHCHECK_ERR_STATS_TAG:            // HealthCheck Error
        case FBE_DMO_XML_ELEM_HW_ERR_STATS_TAG:                     // Hardware Error
        case FBE_DMO_XML_ELEM_LINK_ERR_STATS_TAG:                   // Link Error
        case FBE_DMO_XML_ELEM_DRV_RST_STATS_TAG:                    // Drive Reset threholding 
        case FBE_DMO_XML_ELEM_DRV_PWR_CYC_TAG:                      // Drive Power Cycle thresholding
        case FBE_DMO_XML_ELEM_DATA_ERR_STATS_TAG:                   // Data Error (Errors not detected at drive, but by caller - RAID: currently one case are checksum errors detected by RAID)
            fbe_stats_parser_initialize(&dmo_xml_parser->stats, dmo_xml_parser->drv_config.default_control_flag);   
            fbe_dmo_thrsh_process_fbe_stats(&dmo_xml_parser->stats, attributes);            
            break;

        // Process weight change tags.  These are nested with the Error Category tags above.
        case FBE_DMO_XML_ELEM_WEIGHT_CHANGE_TAG:
            fbe_dmo_thrsh_process_weight_change(&dmo_xml_parser->stats, attributes);
            break;
        case FBE_DMO_XML_ELEM_DRV_Q_DEPTH_TAG:            
            if(!strcmp(attributes[0], FBE_DMO_XML_ATTR_DRV_DEFAULT_QDEPTH_TAG_STR)){
                temp_value = dmo_str2int(attributes[1]);
                if(temp_value != DMO_MAX_UINT_32){
                    dmo_xml_parser->drv_config.drv_info_parser.default_q_depth = temp_value;
                    dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc |= DRV_INFO_QDEPTH_ATTR_DEFAULT_QDEPTH_PROC;
                }else{
                    dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc &= ~DRV_INFO_QDEPTH_ATTR_DEFAULT_QDEPTH_PROC;
                }

            }else{
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s: DIEH XML Parser Drive Info - Default Queue Depth attribute not detected\n", __FUNCTION__);
                dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc &= ~DRV_INFO_QDEPTH_ATTR_DEFAULT_QDEPTH_PROC;
            }

            if(!strcmp(attributes[2], FBE_DMO_XML_ATTR_DRV_SYSTEM_NORMAL_QDEPTH_TAG_STR)){
                temp_value = dmo_str2int(attributes[3]);
                if(temp_value != DMO_MAX_UINT_32){
                    dmo_xml_parser->drv_config.drv_info_parser.system_normal_q_depth = temp_value;
                    dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc |= DRV_INFO_QDEPTH_ATTR_SYSTEM_NORMAL_QDEPTH_PROC;
                }else{
                    dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc &= ~DRV_INFO_QDEPTH_ATTR_SYSTEM_NORMAL_QDEPTH_PROC;
                }
            }
            else{
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s: DIEH XML Parser Drive Info - System Normal Queue Depth attribute not detected\n", __FUNCTION__);
                dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc &= ~DRV_INFO_QDEPTH_ATTR_SYSTEM_NORMAL_QDEPTH_PROC;
            }

            if(!strcmp(attributes[4], FBE_DMO_XML_ATTR_DRV_SYSTEM_REDUCED_QDEPTH_TAG_STR)){
                temp_value = dmo_str2int(attributes[5]);
                if(temp_value != DMO_MAX_UINT_32){
                    dmo_xml_parser->drv_config.drv_info_parser.system_reduced_q_depth = temp_value;
                    dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc |= DRV_INFO_QDEPTH_ATTR_SYSTEM_REDUCED_QDEPTH_PROC;
                }else{
                    dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc &= ~DRV_INFO_QDEPTH_ATTR_SYSTEM_REDUCED_QDEPTH_PROC;
                }
            }
            else{
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s: DIEH XML Parser Drive Info - System Reduced Queue Depth attribute not detected\n", __FUNCTION__);
                dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc &= ~DRV_INFO_QDEPTH_ATTR_SYSTEM_REDUCED_QDEPTH_PROC;
            }

            break;

        case FBE_DMO_XML_ELEM_DRV_GLOBAL_PARAMETERS_TAG:       
            fbe_dmo_thrsh_process_global_parameters(&dmo_xml_parser->drv_config.global_parameters, attributes);
        break;


        case FBE_DMO_XML_ELEM_RCVR_TAG_RDC_TAG:                     // Recovery Tag Reduce
            if(!strcmp(attributes[0], FBE_DMO_XML_ATTR_RCVR_TAG_RDC_TAG_STR)){
                temp_value = dmo_str2int(attributes[1]);
                if(temp_value != DMO_MAX_UINT_32){
                    dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats.recovery_tag_reduce = temp_value;
                    dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag |= DRV_RCVR_TAG_RDC_ATTR_VAL_PROC;
                }else{
                    fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                             "%s: DIEH XML Parser Statistics - Invalid Recovery Tag Reduce detected\n", __FUNCTION__);           
                }
            }
            break;
        case FBE_DMO_XML_ELEM_ERR_BRST_WT_RDC_TAG:                  // Error Burst Weight Reduce
            if(!strcmp(attributes[0], FBE_DMO_XML_ATTR_ERR_BRST_WT_RDC_TAG_STR)){
                temp_value = dmo_str2int(attributes[1]);
                if(temp_value != DMO_MAX_UINT_32){
                    dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats.error_burst_weight_reduce = temp_value;
                    dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag |= DRV_ERR_BRST_WT_RDC_ATTR_VAL_PROC;
                }else{
                    fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                             "%s: DIEH XML Parser Statistics - Invalid Error Burst Weight Reduce detected\n", __FUNCTION__);         
                }
            }
            break;
        case FBE_DMO_XML_ELEM_ERR_BRST_DLT_TAG:                     // Burst Delta
            if(!strcmp(attributes[0], FBE_DMO_XML_ATTR_ERR_BRST_DLT_TAG_STR)){
                temp_value = dmo_str2int(attributes[1]);
                if(temp_value != DMO_MAX_UINT_32){
                    dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats.error_burst_delta = (fbe_time_t)temp_value;
                    dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag |= DRV_ERR_BRST_DLT_ATTR_VAL_PROC;
                }else{
                    dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag &= ~DRV_ERR_BRST_DLT_ATTR_VAL_PROC;
                    fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                             "%s: DIEH XML Parser Statistics - Invalid Error burst delta detected\n", __FUNCTION__);         
                }
            }
            break;
        // Process drive information
        case FBE_DMO_XML_ELEM_DRV_INFO_TAG:                         // Drive Information
            
            fbe_dmo_drv_info_initialize(&dmo_xml_parser->drv_config.drv_info_parser);

            // Based on the SAS block being processed, set drive type to be appropriate field
            if (dmo_xml_parser->drv_config.drv_config_flags & DRV_CONFIG_SAS_DETECTED){
                dmo_xml_parser->drv_config.drv_info_parser.drive_info.drive_type = FBE_DRIVE_TYPE_SAS;
            }else{
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s: DIEH XML Parser Drive Info - Invalid drive type detected\n", __FUNCTION__);            
                dmo_xml_parser->drv_config.drv_info_parser.drive_info.drive_type = FBE_DRIVE_TYPE_INVALID;              
            }
            break;
        case FBE_DMO_XML_ELEM_DRV_VND_TAG:
            if(!strcmp(attributes[0], FBE_DMO_XML_ATTR_VND_NAME_TAG_STR)){
                str_len = (fbe_u32_t)strlen(attributes[1]);
                if(str_len > 0 ){
                    // to do - convert to lower here 
                    index = 0;
                    while( (fbe_dmo_drive_vendor_table[index].vendor_name != NULL) && 
                        (strcmp(attributes[1], fbe_dmo_drive_vendor_table[index].vendor_name)) ){
                            index++;                        
                        }
                    dmo_xml_parser->drv_config.drv_info_parser.drive_info.drive_vendor = fbe_dmo_drive_vendor_table[index].drive_vendor_id;
                    if(dmo_xml_parser->drv_config.drv_info_parser.drive_info.drive_vendor == FBE_DRIVE_VENDOR_ILLEGAL){
                        dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc &= ~DRV_INFO_VND_ATTR_VN_ID_PROC;
                    }else{
                        dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc |= DRV_INFO_VND_ATTR_VN_ID_PROC;
                    }               
                }else if (str_len == 0){
                    dmo_xml_parser->drv_config.drv_info_parser.drive_info.drive_vendor = FBE_DRIVE_VENDOR_INVALID;
                    dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc |= DRV_INFO_VND_ATTR_VN_ID_PROC;
                }else{
                    fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                             "%s: DIEH XML Parser Drive Info - Invalid/unsupported Vendor detected\n", __FUNCTION__);            
                    dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc &= ~DRV_INFO_VND_ATTR_VN_ID_PROC;
                }
            }else{
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s: DIEH XML Parser Drive Info - vendor name tag not detected\n", __FUNCTION__);           
                dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc &= ~DRV_INFO_VND_ATTR_VN_ID_PROC;
            }
            break;
        case FBE_DMO_XML_ELEM_DRV_PART_NO_TAG:
            if(!strcmp(attributes[0], FBE_DMO_XML_ATTR_DRV_PART_NO_TAG_STR)){
                str_len = (fbe_u32_t)strlen(attributes[1]);
                if((str_len > 0)&(str_len < (FBE_SCSI_INQUIRY_PART_NUMBER_SIZE + 1))){
                    strcpy(dmo_xml_parser->drv_config.drv_info_parser.drive_info.part_number, attributes[1]);
                    dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc |= DRV_INFO_PART_NO_ATTR_PNO_PROC;
                }else if (str_len == 0){
                    strcpy(dmo_xml_parser->drv_config.drv_info_parser.drive_info.part_number, "");
                    dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc |= DRV_INFO_PART_NO_ATTR_PNO_PROC;
                }
                else if(str_len >= (FBE_SCSI_INQUIRY_PART_NUMBER_SIZE + 1)){
                    fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                             "%s: DIEH XML Parser Drive Info - drive part number too long detected\n", __FUNCTION__);            
                    dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc &= ~DRV_INFO_PART_NO_ATTR_PNO_PROC;
                }else{
                    fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                             "%s: DIEH XML Parser Drive Info - drive part number string length < 0\n", __FUNCTION__);
                    dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc &= ~DRV_INFO_PART_NO_ATTR_PNO_PROC;
                }
            }else{
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s: DIEH XML Parser Drive Info - drive part number attribute not detected\n", __FUNCTION__);

                dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc &= ~DRV_INFO_PART_NO_ATTR_PNO_PROC;
            }
            break;
        case FBE_DMO_XML_ELEM_DRV_FRMW_REV_TAG:
            if(!strcmp(attributes[0], FBE_DMO_XML_ATTR_DRV_FRMW_REV_TAG_STR)){
                str_len = (fbe_u32_t)strlen(attributes[1]);
                if((str_len > 0)&(str_len < (FBE_SCSI_INQUIRY_REVISION_SIZE + 1))){
                    strcpy(dmo_xml_parser->drv_config.drv_info_parser.drive_info.fw_revision, attributes[1]);
                    dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc |= DRV_INFO_FW_ATTR_FW_REV_PROC;
                }else if (str_len == 0){
                    strcpy(dmo_xml_parser->drv_config.drv_info_parser.drive_info.fw_revision, "");
                    dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc |= DRV_INFO_FW_ATTR_FW_REV_PROC;
                }
                else if(str_len >= (FBE_SCSI_INQUIRY_REVISION_SIZE + 1)){
                    fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                             "%s: DIEH XML Parser Drive Info - firmware revision too long\n", __FUNCTION__);
                    dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc &= ~DRV_INFO_FW_ATTR_FW_REV_PROC;
                }else{
                    fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                             "%s: DIEH XML Parser Drive Info - firmware revision string length < 0 \n", __FUNCTION__);
                    dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc &= ~DRV_INFO_FW_ATTR_FW_REV_PROC;
                }
            }else{
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s: DIEH XML Parser Drive Info - firmware revision attribute not detected\n", __FUNCTION__);
                dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc &= ~DRV_INFO_FW_ATTR_FW_REV_PROC;
            }
            break;
        case FBE_DMO_XML_ELEM_DRV_SER_NO_TAG:
            if(!strcmp(attributes[0], FBE_DMO_XML_ATTR_DRV_SER_NO_START_TAG_STR)){
                str_len = (fbe_u32_t)strlen(attributes[1]);
                if((str_len > 0)&(str_len < (FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE + 1))){
                    strcpy((char *)dmo_xml_parser->drv_config.drv_info_parser.drive_info.serial_number_start, attributes[1]);
                    dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc |= DRV_INFO_SNO_ATTR_SNO_ST_PROC;
                }else if (str_len == 0){
                    strcpy((char *)dmo_xml_parser->drv_config.drv_info_parser.drive_info.serial_number_start, "");
                    dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc |= DRV_INFO_SNO_ATTR_SNO_ST_PROC;
                }
                else if(str_len >= (FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE + 1)){
                    fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                             "%s: DIEH XML Parser Drive Info - Serial Number Start too long\n", __FUNCTION__);
                    dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc &= ~DRV_INFO_SNO_ATTR_SNO_ST_PROC;
                }
            }else{
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s: DIEH XML Parser Drive Info - Serial Number Start attribute not detected\n", __FUNCTION__);
                dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc &= ~DRV_INFO_SNO_ATTR_SNO_ST_PROC;
            }

            if(!strcmp(attributes[2], FBE_DMO_XML_ATTR_DRV_SER_NO_END_TAG_STR)){
                str_len = (fbe_u32_t)strlen(attributes[3]);
                if((str_len > 0)&(str_len < (FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE + 1))){
                    strcpy((char *)dmo_xml_parser->drv_config.drv_info_parser.drive_info.serial_number_end, attributes[3]);
                    dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc |= DRV_INFO_SNO_ATTR_SNO_END_PROC;
                }else if (str_len == 0){
                    strcpy((char *)dmo_xml_parser->drv_config.drv_info_parser.drive_info.serial_number_end, "");
                    dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc |= DRV_INFO_SNO_ATTR_SNO_END_PROC;
                }
                else if(str_len >= (FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE + 1)){
                    fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                             "%s: DIEH XML Parser Drive Info - Serial Number End too long\n", __FUNCTION__);
                    dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc &= ~DRV_INFO_SNO_ATTR_SNO_END_PROC;
                }
            }else{
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s: DIEH XML Parser Drive Info - Serial Number End attribute not detected\n", __FUNCTION__);
                dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc &= ~DRV_INFO_SNO_ATTR_SNO_END_PROC;
            }
            break;
        case FBE_DMO_XML_ELEM_SAS_EXCP_CODES_TAG:
            if(!(dmo_xml_parser->drv_config.drv_config_flags & DRV_CONFIG_SAS_DETECTED)){
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s: DIEH XML Parser Exception List - Invalid Exception type for SAS drive detected\n", __FUNCTION__);
                // TO do: flag this as invalid configuration and invalidate the complete record
            }
            break;
        case FBE_DMO_XML_ELEM_SCSI_EXCP_CODE_TAG:
            excp_code_cnt = dmo_xml_parser->drv_config.drv_excp_code_parser.excp_code_cnt;
            if(excp_code_cnt >= MAX_EXCEPTION_CODES){
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s: DIEH XML Parser Exception List - Number of exception codes exceeded the max limit (%d)\n", __FUNCTION__, MAX_EXCEPTION_CODES);
                dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_err_any  |= DRV_EXCP_CODES_EXCEED_MAX_COUNT;
            }
            break;
        case FBE_DMO_XML_ELEM_SCSI_CODE_TAG:    
            if(num_attributes != 10)
            {
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                         "DIEH XML process_element <%s> id:%d Invalid number of attributes=%d\n",
                                         dmo_dieh_get_element_str(element_type), element_type, num_attributes);
                dmo_xml_parser->doc_state = FBE_DMO_THRSH_DOC_ERROR;
                dmo_xml_parser->status = FBE_DMO_THRSH_XML_ELEMENT_PARSE_ERROR;
                break;
            }
            if(!(dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_err_any & DRV_EXCP_CODES_EXCEED_MAX_COUNT)){
                if(!strcmp(attributes[0], FBE_DMO_XML_ATTR_SENSE_KEY_TAG_STR)){             
                    temp_value = dmo_str2int(attributes[1]);
                    if(temp_value != DMO_MAX_UINT_32){
                        dmo_xml_parser->drv_config.drv_excp_code_parser.excp_code[excp_code_cnt].scsi_fis_union.scsi_code.sense_key = 
                            (fbe_u8_t)temp_value;
                        dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag |= DRV_SCSI_CODE_ATTR_SK_PROC;
                    }else{
                        dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag &= ~DRV_SCSI_CODE_ATTR_SK_PROC;
                        dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_err_any |= DRV_EXCP_SCSI_CODE_PROC_ERR_DETECTED;
                        fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                                 "%s: DIEH XML Parser SCSI Exception Code - Invalid Sense Key detected. excp:%d\n", __FUNCTION__, excp_code_cnt);
                    }
                }else{
                    dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag &= ~DRV_SCSI_CODE_ATTR_SK_PROC;
                    dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_err_any |= DRV_EXCP_SCSI_CODE_PROC_ERR_DETECTED;
                    fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                             "%s: DIEH XML Parser SCSI Exception Code - SCSI Sense Key attribute not detected. excp:%d\n", __FUNCTION__, excp_code_cnt);
                }

                if(!strcmp(attributes[2], FBE_DMO_XML_ATTR_SENSE_CODE_START_TAG_STR)){            
                    temp_value = dmo_str2int(attributes[3]);
                    if(temp_value != DMO_MAX_UINT_32){
                        dmo_xml_parser->drv_config.drv_excp_code_parser.excp_code[excp_code_cnt].scsi_fis_union.scsi_code.asc_range_start = 
                            (fbe_u8_t)temp_value;
                        dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag |= DRV_SCSI_CODE_ATTR_ASC_START_PROC;
                    }else{
                        dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag &= ~DRV_SCSI_CODE_ATTR_ASC_START_PROC;
                        dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_err_any |= DRV_EXCP_SCSI_CODE_PROC_ERR_DETECTED;
                        fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                                 "%s: DIEH XML Parser SCSI Exception Code - Invalid Sense Code Start detected. excp:%d\n", __FUNCTION__, excp_code_cnt);
                    }
                }else{
                    dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag &= ~DRV_SCSI_CODE_ATTR_ASC_START_PROC;
                    dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_err_any |= DRV_EXCP_SCSI_CODE_PROC_ERR_DETECTED;
                    fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                             "%s: DIEH XML Parser SCSI Exception Code - SCSI Sense Code Start attribute not detected. excp:%d\n", __FUNCTION__, excp_code_cnt);
                }


                if(!strcmp(attributes[4], FBE_DMO_XML_ATTR_SENSE_CODE_END_TAG_STR)){            
                    temp_value = dmo_str2int(attributes[5]);
                    if(temp_value != DMO_MAX_UINT_32){
                        dmo_xml_parser->drv_config.drv_excp_code_parser.excp_code[excp_code_cnt].scsi_fis_union.scsi_code.asc_range_end = 
                            (fbe_u8_t)temp_value;
                        dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag |= DRV_SCSI_CODE_ATTR_ASC_END_PROC;
                    }else{
                        dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag &= ~DRV_SCSI_CODE_ATTR_ASC_END_PROC;
                        dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_err_any |= DRV_EXCP_SCSI_CODE_PROC_ERR_DETECTED;
                        fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                                 "%s: DIEH XML Parser SCSI Exception Code - Invalid Sense Code End detected. excp:%d\n", __FUNCTION__, excp_code_cnt);
                    }
                }else{
                    dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag &= ~DRV_SCSI_CODE_ATTR_ASC_END_PROC;
                    dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_err_any |= DRV_EXCP_SCSI_CODE_PROC_ERR_DETECTED;
                    fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                             "%s: DIEH XML Parser SCSI Exception Code - SCSI Sense Code End attribute not detected. excp:%d\n", __FUNCTION__, excp_code_cnt);
                }

                if(!strcmp(attributes[6], FBE_DMO_XML_ATTR_SENSE_QUAL_ST_TAG_STR)){         
                    temp_value = dmo_str2int(attributes[7]);
                    if(temp_value != DMO_MAX_UINT_32){
                        dmo_xml_parser->drv_config.drv_excp_code_parser.excp_code[excp_code_cnt].scsi_fis_union.scsi_code.ascq_range_start = 
                            (fbe_u8_t)temp_value;
                        dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag |= DRV_SCSI_CODE_ATTR_ASCQ_ST_PROC;
                    }else{
                        dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag &= ~DRV_SCSI_CODE_ATTR_ASCQ_ST_PROC;
                        dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_err_any |= DRV_EXCP_SCSI_CODE_PROC_ERR_DETECTED;
                        fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                                 "%s: DIEH XML Parser SCSI Exception Code - Invalid Sense Qual Start detected. excp:%d\n", __FUNCTION__, excp_code_cnt);
                    }
                }else{
                    dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag &= ~DRV_SCSI_CODE_ATTR_ASCQ_ST_PROC;
                    dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_err_any |= DRV_EXCP_SCSI_CODE_PROC_ERR_DETECTED;
                    fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                             "%s: DIEH XML Parser SCSI Exception Code - SCSI Sense Qual Start attribute not detected. excp:%d\n", __FUNCTION__, excp_code_cnt);
                }
                
                if(!strcmp(attributes[8], FBE_DMO_XML_ATTR_SENSE_QUAL_END_TAG_STR)){            
                    temp_value = dmo_str2int(attributes[9]);
                    if(temp_value != DMO_MAX_UINT_32){
                        dmo_xml_parser->drv_config.drv_excp_code_parser.excp_code[excp_code_cnt].scsi_fis_union.scsi_code.ascq_range_end = 
                            (fbe_u8_t)temp_value;
                        dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag |= DRV_SCSI_CODE_ATTR_ASCQ_END_PROC;
                    }else{
                        dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag &= ~DRV_SCSI_CODE_ATTR_ASCQ_END_PROC;
                        dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_err_any |= DRV_EXCP_SCSI_CODE_PROC_ERR_DETECTED;
                        fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                                 "%s: DIEH XML Parser SCSI Exception Code - Invalid Sense Qual End detected. excp:%d\n", __FUNCTION__, excp_code_cnt);
                    }
                }else{
                    dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag &= ~DRV_SCSI_CODE_ATTR_ASCQ_END_PROC;
                    dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_err_any |= DRV_EXCP_SCSI_CODE_PROC_ERR_DETECTED;
                    fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                             "%s: DIEH XML Parser SCSI Exception Code - SCSI Sense Qual End attribute not detected. excp:%d\n", __FUNCTION__, excp_code_cnt);
                }
            }else{
            }
            break;

        case FBE_DMO_XML_ELEM_IO_STATUS_TAG:
            if(!(dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_err_any & DRV_EXCP_CODES_EXCEED_MAX_COUNT)){
                if(!strcmp(attributes[0], FBE_DMO_XML_ATTR_IO_STATUS_TAG_STR)){
                    str_len = (fbe_u32_t)strlen(attributes[1]);
                    if(str_len > 0 ){
                        // to do - convert to lower here 
                        index = 0;
                        while( (fbe_dmo_cdb_fis_status_table[index].name != NULL) && 
                            (strcmp(attributes[1], fbe_dmo_cdb_fis_status_table[index].name)) ){
                                index++;                        
                            }
                        dmo_xml_parser->drv_config.drv_excp_code_parser.excp_code[excp_code_cnt].status = fbe_dmo_cdb_fis_status_table[index].status;

                        if (dmo_xml_parser->drv_config.drv_excp_code_parser.excp_code[excp_code_cnt].status == FBE_PAYLOAD_CDB_FIS_IO_STATUS_INVALID){
                            dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag &= ~DRV_IO_STATUS_ATTR_CDB_FIS_STS_PROC;
                            dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_err_any |= DRV_EXCP_IO_STATUS_PROC_ERR_DETECTED;
                            fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                                     "%s: DIEH XML Parser Exception List - Invalid IO status detected. excp:%d\n", 
                                                     __FUNCTION__, excp_code_cnt);          
                        }else{
                            dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag |= DRV_IO_STATUS_ATTR_CDB_FIS_STS_PROC;
                        }
                    }else{
                        fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                                 "%s:2 DIEH XML Parser Exception List - Invalid IO status detected. excp:%d\n", 
                                                 __FUNCTION__, excp_code_cnt);
                        dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag &= ~DRV_IO_STATUS_ATTR_CDB_FIS_STS_PROC;
                    }
                }else{
                    fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                             "%s: DIEH XML Parser Exception List - IO Status not detected. excp:%d\n", 
                                             __FUNCTION__, excp_code_cnt);
                    dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag &= ~DRV_IO_STATUS_ATTR_CDB_FIS_STS_PROC;
                    dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_err_any |= DRV_EXCP_IO_STATUS_PROC_ERR_DETECTED;
                }
            }else{
            }
            break;

        case FBE_DMO_XML_ELEM_ERR_CLSFY_TAG:
            fbe_dmo_thrsh_process_error_classify(dmo_xml_parser, element_type, attributes, num_attributes);
            break;

        case FBE_DMO_XML_ELEM_QUEUING_CFG_TAG:
            status = fbe_api_drive_configuration_interface_reset_queuing_table();   
            if (status != FBE_STATUS_OK)
            {           
                 fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: DIEH XML reset enhanced queuing table failed. status=%d\n",  __FUNCTION__, status);                    
                 dmo_xml_parser->doc_state = FBE_DMO_THRSH_DOC_ERROR;
                 dmo_xml_parser->status = FBE_DMO_THRSH_ERROR;                 
            }
            break;
        case FBE_DMO_XML_ELEM_SAS_QUEUING_CFG_TAG:
            fbe_queuing_parser_initialize(&dmo_xml_parser->queuing_config);
            dmo_xml_parser->queuing_config.queuing_record.drive_info.drive_type = FBE_DRIVE_TYPE_SAS;
            dmo_xml_parser->drv_config.drv_config_flags = NO_DRV_CONFIG_PROC;
            dmo_xml_parser->drv_config.drv_config_flags |= DRV_CONFIG_SAS_DETECTED;
            break;
        case FBE_DMO_XML_ELEM_EQ_TIMER_TAG:
            if(!strcmp(attributes[0], FBE_DMO_XML_ATTR_LPQ_TIMER_TAG_STR)){
                dmo_xml_parser->queuing_config.queuing_record.lpq_timer = dmo_str2int(attributes[1]);
            }else{
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s: DIEH XML Parser lpq timer attribute not detected\n", __FUNCTION__);
            }

            if(!strcmp(attributes[2], FBE_DMO_XML_ATTR_HPQ_TIMER_TAG_STR)){
                dmo_xml_parser->queuing_config.queuing_record.hpq_timer = dmo_str2int(attributes[3]);
            }else{
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s: DIEH XML Parser Drive Info - Serial Number End attribute not detected\n", __FUNCTION__);
            }
            break;

        case FBE_DMO_XML_ELEM_RELIABLY_CHALLENGED_TAG:
            fbe_dmo_thrsh_process_reliably_challenged(dmo_xml_parser, element_type, attributes);
            break;

        default:
            break;
    }

}


static void fbe_dmo_thrsh_process_end_element(fbe_dmo_thrsh_parser_t *dmo_xml_parser, const char *element){
    
    fbe_dmo_xml_element_tag_type_t element_type = FBE_DMO_XML_ELEM_DMO_UNDEFINED_TAG;
    
    fbe_drive_configuration_handle_t fbe_rcd_handle;

    // Read the XML tag which is being processed    
    element_type = dmo_xml_parser->element_tag_type;

    switch(element_type){
        // Process the drive information table 
        case FBE_DMO_XML_ELEM_DRV_VND_TAG:
            // Vendor information element has vendor name attribut which contains name of drive vendor. 
            // Verify that the name was extracted and was a valid one.
            if(dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc & DRV_INFO_VND_ATTR_VN_ID_PROC){
                // Mark the element has being processed with valid data
                dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc |= DRV_INFO_VND_PROC;              
                //dmo_xml_parser->drv_config.drv_config_recd.drive_info.drive_vendor = dmo_xml_parser->drv_config.drv_info_parser.drive_info.drive_vendor;

            }else{
                // Mark the element as being not processed
                dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc &= ~DRV_INFO_VND_PROC;
            }
            break;
        case FBE_DMO_XML_ELEM_DRV_PART_NO_TAG:
            if(dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc & DRV_INFO_PART_NO_ATTR_PNO_PROC){
                dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc |= DRV_INFO_PART_NO_PROC;
                //strcpy(dmo_xml_parser->drv_config.drv_config_recd.drive_info.part_number, dmo_xml_parser->drv_config.drv_info_parser.drive_info.part_number);
            }else{
                dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc &= ~DRV_INFO_PART_NO_PROC;
            }
            break;
        case FBE_DMO_XML_ELEM_DRV_FRMW_REV_TAG:
            if(dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc & DRV_INFO_FW_ATTR_FW_REV_PROC){
                dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc |= DRV_INFO_FW_REV_PROC;
                //strcpy(dmo_xml_parser->drv_config.drv_config_recd.drive_info.fw_revision, dmo_xml_parser->drv_config.drv_info_parser.drive_info.fw_revision);
            }else{
                dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc &= ~DRV_INFO_FW_REV_PROC;
            }
            break;
        case FBE_DMO_XML_ELEM_DRV_SER_NO_TAG:
            if((dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc & DRV_INFO_SNO_ATTR_SNO_ST_PROC) &&
                (dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc & DRV_INFO_SNO_ATTR_SNO_END_PROC)){
                dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc |= DRV_INFO_SNO_PROC;
                //strcpy(dmo_xml_parser->drv_config.drv_config_recd.drive_info.serial_number_start, dmo_xml_parser->drv_config.drv_info_parser.drive_info.serial_number_start);
                //strcpy(dmo_xml_parser->drv_config.drv_config_recd.drive_info.serial_number_end, dmo_xml_parser->drv_config.drv_info_parser.drive_info.serial_number_end);
            }else{
                dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc &= ~DRV_INFO_SNO_PROC;
            }
            break;
        case FBE_DMO_XML_ELEM_DRV_Q_DEPTH_TAG:
            if((dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc & DRV_INFO_QDEPTH_ATTR_DEFAULT_QDEPTH_PROC) &&
                (dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc & DRV_INFO_QDEPTH_ATTR_SYSTEM_NORMAL_QDEPTH_PROC) &&
                (dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc & DRV_INFO_QDEPTH_ATTR_SYSTEM_REDUCED_QDEPTH_PROC)){
                dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc |= DRV_INFO_QDEPTH_PROC;
            }else{
                dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc &= ~DRV_INFO_QDEPTH_PROC;
            }
            break;
        case FBE_DMO_XML_ELEM_DRV_INFO_TAG:
            // Check if the drive type detected was valid one, if not mark drive informaton block as invalid.
            if(dmo_xml_parser->drv_config.drv_info_parser.drive_info.drive_type == FBE_DRIVE_TYPE_INVALID){
                dmo_xml_parser->drv_config.drv_config_flags &= ~DRV_CONFIG_DRV_INFO_PROC;
            }else{
                // Check if the constituents element tags of drive information - vendor, part number, firmware revision
                // and serial number are marked as processed, else mark drive information block as invalid
                if( (dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc & DRV_INFO_VND_PROC) &&
                    (dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc & DRV_INFO_PART_NO_PROC)&&
                    (dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc & DRV_INFO_FW_REV_PROC) &&
                    (dmo_xml_parser->drv_config.drv_info_parser.drv_info_proc & DRV_INFO_SNO_PROC)){
                        // all the drive elements were processed, mark drive information as processed
                        dmo_xml_parser->drv_config.drv_config_flags |= DRV_CONFIG_DRV_INFO_PROC;
                        //dmo_xml_parser->drv_config.drv_config_recd.drive_info.drive_type = dmo_xml_parser->drv_config.drv_info_parser.drive_info.drive_type;
                        memcpy(&dmo_xml_parser->drv_config.drv_config_recd.drive_info, &dmo_xml_parser->drv_config.drv_info_parser.drive_info,
                                    sizeof(fbe_drive_configuration_drive_info_t));
                }else{
                    dmo_xml_parser->drv_config.drv_config_flags &= ~DRV_CONFIG_DRV_INFO_PROC;
                }
            }
            break;
        // Process  Drive Statistics
        case FBE_DMO_XML_ELEM_CMLT_STATS_TAG:               // Cumulative or overall IO statistics
            if(fbe_dmo_thrsh_are_fbe_stats_valid(dmo_xml_parser->stats) == true){
                memcpy(&dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats.io_stat, 
                                &dmo_xml_parser->stats.fbe_stat, sizeof(fbe_stat_t));
                dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag |= DRV_CML_STATS_PROC;
            }else{
                dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag &= ~DRV_CML_STATS_PROC;
            }
            break;
        case FBE_DMO_XML_ELEM_RCVR_ERR_STATS_TAG:           // Recovered Error Statistics 
            if(fbe_dmo_thrsh_are_fbe_stats_valid(dmo_xml_parser->stats) == true){
                memcpy(&dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats.recovered_stat, 
                                &dmo_xml_parser->stats.fbe_stat, sizeof(fbe_stat_t));
                dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag |= DRV_RCVR_STATS_PROC;
            }else{
                dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag &= ~DRV_RCVR_STATS_PROC;
            }
            break;
        case FBE_DMO_XML_ELEM_MEDIA_ERR_STATS_TAG:          // Media Error Statistics
            if(fbe_dmo_thrsh_are_fbe_stats_valid(dmo_xml_parser->stats) == true){
                memcpy(&dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats.media_stat, 
                                &dmo_xml_parser->stats.fbe_stat, sizeof(fbe_stat_t));
                dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag |= DRV_MEDIA_STATS_PROC;
            }else{
                dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag &= ~DRV_MEDIA_STATS_PROC;
            }
            break;
        case FBE_DMO_XML_ELEM_HW_ERR_STATS_TAG:             // Hardware Error Statistics
            if(fbe_dmo_thrsh_are_fbe_stats_valid(dmo_xml_parser->stats) == true){
                memcpy(&dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats.hardware_stat, 
                                &dmo_xml_parser->stats.fbe_stat, sizeof(fbe_stat_t));
                dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag |= DRV_HW_STATS_THRSH_PROC;
            }else{
                dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag &= ~DRV_HW_STATS_THRSH_PROC;
            }
            break;

        case FBE_DMO_XML_ELEM_HEALTHCHECK_ERR_STATS_TAG:             // HealthCheck Error Statistics
            if(fbe_dmo_thrsh_are_fbe_stats_valid(dmo_xml_parser->stats) == true){
                memcpy(&dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats.healthCheck_stat, 
                                &dmo_xml_parser->stats.fbe_stat, sizeof(fbe_stat_t));
                dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag |= DRV_HEATHCHECK_STATS_PROC;
            }else{
                dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag &= ~DRV_HEATHCHECK_STATS_PROC;
            }
            break;

        case FBE_DMO_XML_ELEM_LINK_ERR_STATS_TAG:           // Link Error Statistics
            if(fbe_dmo_thrsh_are_fbe_stats_valid(dmo_xml_parser->stats) == true){
                memcpy(&dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats.link_stat, 
                                &dmo_xml_parser->stats.fbe_stat, sizeof(fbe_stat_t));
                dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag |= DRV_LINK_STATS_PROC;
            }else{
                dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag &= ~DRV_LINK_STATS_PROC;
            }
            break;
        case FBE_DMO_XML_ELEM_DRV_RST_STATS_TAG:            // Drive Reset Statistics
            if(fbe_dmo_thrsh_are_fbe_stats_valid(dmo_xml_parser->stats) == true){
                memcpy(&dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats.reset_stat, 
                                &dmo_xml_parser->stats.fbe_stat, sizeof(fbe_stat_t));
                dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag |= DRV_RST_STATS_PROC;
            }else{
                dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag &= ~DRV_RST_STATS_PROC;
            }
            break;
        case FBE_DMO_XML_ELEM_DRV_PWR_CYC_TAG:              // Drive Power Cycle Statistics
            if(fbe_dmo_thrsh_are_fbe_stats_valid(dmo_xml_parser->stats) == true){
                memcpy(&dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats.power_cycle_stat, 
                                &dmo_xml_parser->stats.fbe_stat, sizeof(fbe_stat_t));
                dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag |= DRV_PW_CYC_STATS_PROC;
            }else{
                dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag &= ~DRV_PW_CYC_STATS_PROC;
            }
            break;
        case FBE_DMO_XML_ELEM_DATA_ERR_STATS_TAG:           // Data Error Statistics 
            if(fbe_dmo_thrsh_are_fbe_stats_valid(dmo_xml_parser->stats) == true){
                memcpy(&dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats.data_stat, 
                                &dmo_xml_parser->stats.fbe_stat, sizeof(fbe_stat_t));
                dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag |= DRV_DATA_ERR_STATS_PROC;
            }else{
                dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag &= ~DRV_DATA_ERR_STATS_PROC;
            }
            break;
        case FBE_DMO_XML_ELEM_WEIGHT_CHANGE_TAG:            // Weight Change exceptions
            /* nothing to do.  this is an optional tag*/
            break;
        case FBE_DMO_XML_ELEM_RCVR_TAG_RDC_TAG:             // Recovery Tag Statistics
            if(dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag & DRV_RCVR_TAG_RDC_ATTR_VAL_PROC){
                dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag |= DRV_RCVR_TAG_RDC_PROC;
            }else{
                dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag &= ~DRV_RCVR_TAG_RDC_PROC;
            }
            break;
        case FBE_DMO_XML_ELEM_ERR_BRST_WT_RDC_TAG:          // Error Burst weight reduce statistics
            if(dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag & DRV_ERR_BRST_WT_RDC_ATTR_VAL_PROC){
                dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag |= DRV_ERR_BRST_WT_RDC_PROC;
            }else{
                dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag &= ~DRV_ERR_BRST_WT_RDC_PROC;
            }
            break;
        case FBE_DMO_XML_ELEM_ERR_BRST_DLT_TAG:             // Error Burst Delta
            if(dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag & DRV_ERR_BRST_DLT_ATTR_VAL_PROC){
                dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag |= DRV_ERR_BRST_DLT_PROC;
            }else{
                dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag &= ~DRV_ERR_BRST_DLT_PROC;
            }
            break;
        case FBE_DMO_XML_ELEM_DRV_STATCS_TAG:
            {
                const fbe_drv_stats_flags_t expected = ( DRV_CML_STATS_PROC |
                                                         DRV_RCVR_STATS_PROC |
                                                         DRV_MEDIA_STATS_PROC |
                                                         DRV_HW_STATS_THRSH_PROC |
                                                         DRV_LINK_STATS_PROC |
                                                         DRV_DATA_ERR_STATS_PROC |                                                         
                                                         DRV_ERR_BRST_WT_RDC_PROC |
                                                         DRV_HEATHCHECK_STATS_PROC |
                                                         DRV_ERR_BRST_DLT_PROC );
                if ( expected == (expected & dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag)) 
                {
                    dmo_xml_parser->drv_config.drv_config_flags |= DRV_CONFIG_STATS_PROC;
                    memcpy(&dmo_xml_parser->drv_config.drv_config_recd.threshold_info,
                        &dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats, sizeof(fbe_drive_stat_t));
                }else{                
                    dmo_xml_parser->drv_config.drv_config_flags &= ~DRV_CONFIG_STATS_PROC;
    
                    fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                             "DIEH XML - parsing drive stats failed. expected flag:0x%x, actual:0x%x\n",  
                                             expected, expected & dmo_xml_parser->drv_config.drv_stats_elem_parser.drv_stats_flag);
                }
            }
            break;
        // Process SAS Drive Exception List
        case FBE_DMO_XML_ELEM_SCSI_CODE_TAG:
            {
                const fbe_drv_excp_code_flags_t expected = ( DRV_SCSI_CODE_ATTR_SK_PROC |
                                                             DRV_SCSI_CODE_ATTR_ASC_START_PROC |
                                                             DRV_SCSI_CODE_ATTR_ASC_END_PROC |
                                                             DRV_SCSI_CODE_ATTR_ASCQ_ST_PROC |
                                                             DRV_SCSI_CODE_ATTR_ASCQ_END_PROC );

                if(expected == (expected & dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag)){
                        dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag |= DRV_EXCP_CODE_SCSI_CODE_PROC;
                }else{
                    dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag &= ~DRV_EXCP_CODE_SCSI_CODE_PROC;
    
                    fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                             "DIEH XML - parsing scsiCode failed. expected flag:0x%x, actual:0x%x\n",  
                                             expected, expected & dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag);
                }
            }
            break;

        case FBE_DMO_XML_ELEM_IO_STATUS_TAG:
            {
                const fbe_drv_excp_code_flags_t expected = (DRV_IO_STATUS_ATTR_CDB_FIS_STS_PROC);

                if(expected == (expected & dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag)){
                    dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag |= DRV_EXCP_CODE_IO_STATUS_PROC;
                }else{
                    dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag &= ~DRV_EXCP_CODE_IO_STATUS_PROC;

                    fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                             "DIEH XML - parsing ioStatus failed. expected flag:0x%x, actual:0x%x\n",  
                                             expected, expected & dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag);
                }
            }
            break;

        case FBE_DMO_XML_ELEM_ERR_CLSFY_TAG:
            {
                const fbe_drv_excp_code_flags_t expected = (DRV_ERR_CLSFY_ATTR_ACT_TYPE_PROC);
                if(expected == (expected & dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag)){
                    dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag |= DRV_EXCP_CODE_ERR_CLSFY_ACT_TYPE_PROC;
                }else{
                    dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag &= ~DRV_EXCP_CODE_ERR_CLSFY_ACT_TYPE_PROC;
                    // If any of the attribute encountered an error set error bit
                    dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag |= DRV_EXCP_CODE_ANY_ERR_CLSFY_PROC_ERROR;

                    fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                             "DIEH XML - parsing errorClassify failed. expected flag:0x%x, actual:0x%x\n",  
                                             expected, expected & dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag);
                }
            }
            break;
        case FBE_DMO_XML_ELEM_SCSI_EXCP_CODE_TAG:
            if(dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_err_any){
                dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag &= ~DRV_EXCP_CODE_SCSI_EXCP_CODE_PROC;
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                         "DIEH XML - parsing scsiExcpetionCode failed. \n");                                         
            }else{
                dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag |= DRV_EXCP_CODE_SCSI_EXCP_CODE_PROC;
                dmo_xml_parser->drv_config.drv_excp_code_parser.excp_code_cnt++;        //Increment the number of SCSI Exception processed              
            }
            break;
        case FBE_DMO_XML_ELEM_SAS_EXCP_CODES_TAG:
            if(dmo_xml_parser->drv_config.drv_config_flags & DRV_CONFIG_SAS_DETECTED){
                if(dmo_xml_parser->drv_config.drv_excp_code_parser.excp_code_cnt >= 1){
                    /*TODO: how do drv_config_rcd keep track of num of except recs? Hopefully it's zero'd somewhere. */
                    memcpy(&dmo_xml_parser->drv_config.drv_config_recd.category_exception_codes[0],
                        &dmo_xml_parser->drv_config.drv_excp_code_parser.excp_code[0], 
                        dmo_xml_parser->drv_config.drv_excp_code_parser.excp_code_cnt*sizeof(fbe_drive_config_scsi_fis_exception_code_t));
                    dmo_xml_parser->drv_config.drv_config_flags |= DRV_CONFIG_EXCP_PROC;
                }else{
                    dmo_xml_parser->drv_config.drv_config_flags &= ~DRV_CONFIG_EXCP_PROC;
                    fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                             "DIEH XML - parsing sasDriveExceptionCodes failed. count invalid\n");   
                }
            }else{
                dmo_xml_parser->drv_config.drv_config_flags &= ~DRV_CONFIG_EXCP_PROC;
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                         "DIEH XML - parsing sasDriveExceptionCodes failed. Not a SAS drive\n");   
            }
            break;
        // Verify SAS Drive record for condition checks and if good push record down to physical package service.
        case FBE_DMO_XML_ELEM_SAS_DRV_CFG_TAG:
            if((dmo_xml_parser->drv_config.drv_config_flags & DRV_CONFIG_STATS_PROC) &&
                (dmo_xml_parser->drv_config.drv_config_flags & DRV_CONFIG_DRV_INFO_PROC)&&
                (dmo_xml_parser->drv_config.drv_config_flags & DRV_CONFIG_EXCP_PROC))
                {
                    dmo_xml_parser->drv_config.drv_config_flags |= DRV_CONFIG_SAS_PROC;
                    dmo_xml_parser->drv_config.num_sas_drv_rcd_prcd++;                  
                    // Add the currently processed drive table
                    if(fbe_api_drive_configuration_interface_add_drive_table_entry(&dmo_xml_parser->drv_config.drv_config_recd, &fbe_rcd_handle) == FBE_STATUS_GENERIC_FAILURE)
                    {
                        fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                                 "%s: DIEH XML - SAS Drive Configuration record insertion failed at service level!\n", __FUNCTION__);
                    }
                    else
                    {
                        fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                                 "%s: DIEH XML - SAS Drive Configuration %d records inserted. handle 0x%x\n", __FUNCTION__, dmo_xml_parser->drv_config.num_sas_drv_rcd_prcd, fbe_rcd_handle);    
                    }

            }else{
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                                 "%s: DIEH XML - Error parsing sasDriveConfiguration tag (%d).  Aborting.\n", __FUNCTION__, element_type); 

                dmo_xml_parser->drv_config.drv_config_flags &= ~DRV_CONFIG_SAS_PROC;
                dmo_xml_parser->doc_state = FBE_DMO_THRSH_DOC_ERROR;
                dmo_xml_parser->status = FBE_DMO_THRSH_XML_ELEMENT_PARSE_ERROR;
            }
            break;

        case FBE_DMO_XML_ELEM_DRV_CFG_TAG:
            fbe_dmo_update_dcs_global_parameter(&dmo_xml_parser->drv_config.global_parameters); 
            fbe_api_drive_configuration_interface_end_update();
            break;
        case FBE_DMO_XML_ELEM_PORT_CFGS_TAG:
            fbe_api_drive_configuration_interface_end_update();
            break;
        case FBE_DMO_XML_ELEM_QUEUING_CFG_TAG:
            fbe_api_drive_configuration_interface_activate_queuing_table();
            break;
        case FBE_DMO_XML_ELEM_SAS_QUEUING_CFG_TAG:
            if (dmo_xml_parser->drv_config.drv_config_flags & DRV_CONFIG_DRV_INFO_PROC)
            {
                fbe_copy_memory(&dmo_xml_parser->queuing_config.queuing_record.drive_info, &dmo_xml_parser->drv_config.drv_info_parser.drive_info,
                                sizeof(fbe_drive_configuration_drive_info_t));
                dmo_xml_parser->drv_config.drv_config_flags = NO_DRV_CONFIG_PROC;
            }
            // Add the currently processed entry
            if(fbe_api_drive_configuration_interface_add_queuing_entry(&dmo_xml_parser->queuing_config.queuing_record) != FBE_STATUS_OK)
            {
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s: DIEH XML - SAS Drive enhanced queuing entry insertion failed!\n", __FUNCTION__);
            }
            else
            {
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s: DIEH XML - SAS Drive enhanced queuing entry inserted.\n", __FUNCTION__);    
            }
            break;
                        
        case FBE_DMO_XML_ELEM_RELIABLY_CHALLENGED_TAG:
            /* nothing to do.  this is an optional tag*/           
            break;

        default:
            break;

    }
}

/*!**************************************************************
 * @fn get_error_category_attribute
 ****************************************************************
 * @brief
 *  Returns a flag that represents the string error category
 *  attribute.  If attribute name is unknown then an invalid
 *  flag will be returned.
 * 
 * @param attr_name - string name for attribute.
 * 
 * @return fbe_stats_errcat_attr_t - flag that represents attribute
 *
 * @version
 *   07/02/2012:  Wayne Garrett - Created.
 *
 ****************************************************************/
static fbe_stats_errcat_attr_t 
get_error_category_attribute(const char * attr_name)
{
    fbe_u32_t i = 0;

    while (dmo_thrsh_xml_errcat_attrib[i].name != NULL)
    {
        if (strncmp(attr_name, dmo_thrsh_xml_errcat_attrib[i].name, FBE_DMO_ATTR_STR_MAX) == 0)
        {
            return dmo_thrsh_xml_errcat_attrib[i].type;
        }
        i++;
    }

    return FBE_STATS_INVALID_PROC;
}

/*!**************************************************************
 * @fn get_error_category_attribute
 ****************************************************************
 * @brief
 *  Returns a flag that represents the string weight change
 *  attribute.  If attribute name is unknown then an invalid
 *  flag will be returned.
 * 
 * @param attr_name - string name for attribute.
 * 
 * @return fbe_stats_wtx_attr_t - flag that represents attribute
 *
 * @version
 *   11/30/2012:  Wayne Garrett - Created.
 *
 ****************************************************************/
static fbe_stats_wtx_attr_t     
get_weight_change_attribute(const char * attr_name)
{
    fbe_u32_t i = 0;

    while (dmo_thrsh_xml_wtx_attrib[i].name != NULL)
    {
        if (strncmp(attr_name, dmo_thrsh_xml_wtx_attrib[i].name, FBE_DMO_ATTR_STR_MAX) == 0)
        {
            return dmo_thrsh_xml_wtx_attrib[i].type;
        }
        i++;
    }

    return FBE_STATS_WTX_INVALID_PROC;
}

/*!**************************************************************
 * @fn is_error_category_action_attr
 ****************************************************************
 * @brief
 *  Returns true if the attribute is an error category action
 *  threshold.
 * 
 * @param attr - error category attribute.
 * 
 * @return fbe_bool_t - true if attr is an action
 *
 * @version
 *   07/02/2012:  Wayne Garrett - Created.
 *
 ****************************************************************/
static fbe_bool_t 
is_error_category_action_attr(fbe_stats_errcat_attr_t attr)
{
    switch (attr)
    {
        case FBE_STATS_RESET_PROC:       
        case FBE_STATS_EOL_PROC:       
        case FBE_STATS_EOL_CH_PROC:       
        case FBE_STATS_KILL_PROC:       
        case FBE_STATS_KILL_CH_PROC:       
        case FBE_STATS_EVENT_PROC:       
            return FBE_TRUE;
    }

    return FBE_FALSE;
}

/*!**************************************************************
 * @fn add_error_category_action
 ****************************************************************
 * @brief
 *  Adds error category actions, which consists of an action
 *  attribute and a comma-separated list of values.  For each
 *  value a new action entry will be added.
 * 
 *  FORMAT = ratio_tuple [, ratio_tuple [, ...]];
 *           ratio_tuple = ratio[/reactivate_ratio]
 * 
 * @param stats - ptr to parser tracking structure.
 * @param attr - error category attribute
 * @param stats_attr_value - comma-separated list of attribute values.
 * 
 * @return fbe_bool_t - true if no errors adding action entries..
 *
 * @version
 *   07/02/2012:  Wayne Garrett - Created.
 *
 ****************************************************************/
static fbe_bool_t
add_error_category_action(fbe_stats_attr_parser_t *stats, fbe_stats_errcat_attr_t attr, const char* stats_attr_value)
{
    fbe_u8_t stats_attr_value_tmp[FBE_DMO_ATTR_STR_MAX+1] = {0};
    fbe_u8_t action_str[FBE_DMO_ATTR_STR_MAX+1] = {0};
    fbe_dmo_attribute_field_array_t fields_array;
    //fbe_u8_t *fields_array[FBE_DMO_MAX_ATTR_FIELDS];
    fbe_u32_t num_fields;
    fbe_u8_t *ratio_tuple_str;    
    fbe_u32_t ratio;
    fbe_u32_t reactivate_ratio;
    fbe_stat_action_flags_t action_flag = attr_to_action_flag(attr);
    fbe_status_t status;
    csx_string_t saveptr;

    if (FBE_STAT_ACTION_FLAG_FLAG_NO_ACTION == action_flag)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: DIEH XML - attr is not an action 0x%x\n", __FUNCTION__, attr); 
        return FBE_FALSE;
    }

    fbe_copy_memory(stats_attr_value_tmp, stats_attr_value, FBE_DMO_ATTR_STR_MAX);

    ratio_tuple_str = csx_p_strtok_r(stats_attr_value_tmp, ",", &saveptr);
    if (NULL == ratio_tuple_str) /* we should have atleast one valid value.*/
    {  
        fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s: DIEH XML - str2int failed for value %s\n", __FUNCTION__, stats_attr_value);
        return FBE_FALSE;
    }
    while (NULL != ratio_tuple_str)
    {           
        if (stats->fbe_stat.actions.num >= FBE_STAT_ACTIONS_MAX)
        {
            fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s: DIEH XML - Exceeded action limit of %d\n", __FUNCTION__, FBE_STAT_ACTIONS_MAX); 
            return FBE_FALSE;
        }

        status = dmo_dieh_parse_string("/", 1 /*min_fields*/, 2 /*max_fields*/, ratio_tuple_str, fields_array, &num_fields);
        if (FBE_STATUS_OK != status)
        {
            fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s: DIEH XML - parsing failed for action:%s attr:%s\n", __FUNCTION__, action_str, stats_attr_value);
            return FBE_FALSE;
        }
       
        ratio = dmo_str2int(fields_array[0]);

        if (DMO_MAX_UINT_32 == ratio) 
        {
            fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s: DIEH XML - str2int failed2 for ratio:%s attr:%s\n", __FUNCTION__, fields_array[0], stats_attr_value); 
            return FBE_FALSE;
        }

        reactivate_ratio = FBE_U32_MAX;

        if (num_fields >= 2)
        {
            reactivate_ratio = dmo_str2int(fields_array[1]);
            if (DMO_MAX_UINT_32 == reactivate_ratio) 
            {
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s: DIEH XML - str2int failed2 for reactivate_ratio:%s attr:%s\n", __FUNCTION__, fields_array[1], stats_attr_value); 
                return FBE_FALSE;
            }
        }

        stats->fbe_stat.actions.entry[stats->fbe_stat.actions.num].flag = action_flag;                    
        stats->fbe_stat.actions.entry[stats->fbe_stat.actions.num].ratio = ratio;
        stats->fbe_stat.actions.entry[stats->fbe_stat.actions.num].reactivate_ratio = reactivate_ratio;
        stats->fbe_stat.actions.num++;  
        stats->stats_flag |= attr; 

        ratio_tuple_str = csx_p_strtok_r(CSX_NULL, ",", &saveptr);
    }   

    return FBE_TRUE;
}

/*!**************************************************************
 * @fn dmo_dieh_add_weight_exception
 ****************************************************************
 * @brief
 *  Adds error category weight exception to the current error
 *  category being processed.
 * 
 * @param stats - ptr to parser tracking structure.
 * @param wtx - weight change info
 * 
 * @return fbe_bool_t - true if add was successful
 *
 * @version
 *   11/21/2012:  Wayne Garrett - Created.
 *
 ****************************************************************/
static fbe_bool_t                     
dmo_dieh_add_weight_exception(fbe_stats_attr_parser_t *stats_p, fbe_stat_weight_exception_code_t *wtx)
{
    fbe_u32_t i=0;

    /* loop through weigh exception array searching for first empty spot.*/

    for (i=0; i<FBE_STAT_MAX_WEIGHT_EXCEPTION_CODES; i++)
    {
        /* is this spot empty*/
        if (FBE_STAT_WEIGHT_EXCEPTION_INVALID == stats_p->fbe_stat.weight_exceptions[i].type)
        {
            stats_p->fbe_stat.weight_exceptions[i] = *wtx;
            return FBE_TRUE;
        }
    }
    
    return FBE_FALSE;
}

/*!**************************************************************
 * @fn attr_to_action_flag
 ****************************************************************
 * @brief
 *  Returns the stats action flag that corresponds to the
 *  error category attribute.   If the attribute is not an action
 *  type, the NO_ACTION is returned.   
 * 
 * @param attr - error category attribute.
 * 
 * @return fbe_stat_action_flags_t - action flag.
 *
 * @version
 *   07/02/2012:  Wayne Garrett - Created.
 *
 ****************************************************************/
static fbe_stat_action_flags_t attr_to_action_flag(fbe_stats_errcat_attr_t attr)
{
    switch (attr)
    {
        case FBE_STATS_RESET_PROC: 
            return FBE_STAT_ACTION_FLAG_FLAG_RESET;     
             
        case FBE_STATS_EOL_PROC:       
            return FBE_STAT_ACTION_FLAG_FLAG_END_OF_LIFE;    
                
        case FBE_STATS_EOL_CH_PROC:       
            return FBE_STAT_ACTION_FLAG_FLAG_END_OF_LIFE_CALL_HOME;

        case FBE_STATS_KILL_PROC:       
            return FBE_STAT_ACTION_FLAG_FLAG_FAIL;

        case FBE_STATS_KILL_CH_PROC:       
            return FBE_STAT_ACTION_FLAG_FLAG_FAIL_CALL_HOME;

        case FBE_STATS_EVENT_PROC: 
            return FBE_STAT_ACTION_FLAG_FLAG_EVENT;
    }

    /* attr is not an action*/
    return FBE_STAT_ACTION_FLAG_FLAG_NO_ACTION;
}

/*!**************************************************************
 * @fn dmo_dieh_initialize_weight_exception
 ****************************************************************
 * @brief
 *  Initialize the weight exception structure.  
 * 
 * @param wtx - struct to init.
 * 
 * @return none.
 *
 * @version
 *   11/30/2012:  Wayne Garrett - Created.
 *
 ****************************************************************/
static void dmo_dieh_initialize_weight_exception(fbe_stat_weight_exception_code_t *wtx)
{
    wtx->type   = FBE_STAT_WEIGHT_EXCEPTION_INVALID;
    wtx->change = FBE_STAT_WTX_CHANGE_INVALID;
}

/*!**************************************************************
 * @fn dmo_dieh_set_sense_code
 ****************************************************************
 * @brief
 *  Initialize the weight exception structure.  
 * 
 * @param wtx - struct to init.
 * 
 * @return none.
 *
 * @version
 *   11/30/2012:  Wayne Garrett - Created.
 *
 ****************************************************************/
static void dmo_dieh_set_sense_code(fbe_stat_scsi_sense_code_t *sense_code, fbe_u32_t temp_value)
{
    sense_code->sense_key   = (fbe_u8_t)((temp_value & 0xFF000000) >> 24);
    sense_code->asc         = (fbe_u8_t)((temp_value &   0xFF0000) >> 16);
    sense_code->ascq_start  = (fbe_u8_t)((temp_value &     0xFF00) >> 8);
    sense_code->ascq_end    = (fbe_u8_t)( temp_value &       0xFF);   
}

/*!**************************************************************
 * @fn dmo_dieh_get_num_attributes
 ****************************************************************
 * @brief
 *  Get number of attributes 
 * 
 * @param attributes - ptr to array of attributes. Null terminated.
 * 
 * @return none.
 *
 * @version
 *   11/13/2013:  Wayne Garrett - Created.
 *
 ****************************************************************/
static fbe_u32_t dmo_dieh_get_num_attributes(const char** attributes)
{
    fbe_u32_t count = 0;
    while(attributes[count] != NULL)
    {
        count++;
    }
    return count;
}

/*!**************************************************************
 * @fn dmo_dieh_get_element_str
 ****************************************************************
 * @brief
 *  Return XML element string 
 * 
 * @param element - element tag type
 * 
 * @return none.
 *
 * @version
 *   11/13/2013:  Wayne Garrett - Created.
 */
static const char* dmo_dieh_get_element_str(const fbe_dmo_xml_element_tag_type_t element)
{
    fbe_u32_t index = 0;
    while(dmo_thrsh_xml_tags[index].tag_name != NULL){
        if(element == dmo_thrsh_xml_tags[index].tag_type){
            return dmo_thrsh_xml_tags[index].tag_name;
        }
        index++;
    }
    return "UNKNOWN_ELEMENT";
}

/*!**************************************************************
 * @fn fbe_dmo_thrsh_process_error_classify
 ****************************************************************
 * @brief
 *  Processes the attributes for the Error Classify element.   If
 *  any error is detected then the FBE_STATS_INVALID_PROC attrib
 *  flag will be set.  
 * 
 * @param dmo_xml_parser - parser structure that's being modified.
 * @param element_type -   element being processed
 * @param attributes   -   attributes for this element.
 * 
 * @return none 
 *
 * @version
 *   11/18/2013:  Wayne Garrett - Created.
 *
 ****************************************************************/
static void fbe_dmo_thrsh_process_error_classify(fbe_dmo_thrsh_parser_t *dmo_xml_parser, 
                                                 fbe_dmo_xml_element_tag_type_t element_type, 
                                                 const char **attributes, 
                                                 fbe_u32_t num_attributes)
{
    fbe_u32_t index, str_len;
    fbe_u32_t excp_code_cnt = dmo_xml_parser->drv_config.drv_excp_code_parser.excp_code_cnt;

    if(num_attributes != 2)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                 "DIEH XML process_error_classify element <%s> id:%d Invalid number of attributes=%d\n",
                                 dmo_dieh_get_element_str(element_type), element_type, num_attributes);
        dmo_xml_parser->doc_state = FBE_DMO_THRSH_DOC_ERROR;
        dmo_xml_parser->status = FBE_DMO_THRSH_XML_ELEMENT_PARSE_ERROR;
        return;
    }         
       
    if(!(dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_err_any & DRV_EXCP_CODES_EXCEED_MAX_COUNT)){
        if(!strcmp(attributes[0], FBE_DMO_XML_ATTR_TYPE_N_ACTION_TAG_STR)){
            str_len = (fbe_u32_t)strlen(attributes[1]);
            if(str_len > 0 ){
                index = 0;
                while( (fbe_dmo_type_action[index].type_action != FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_INVALID) && 
                    (strcmp(attributes[1], fbe_dmo_type_action[index].name)) ){
                        index++;                        
                    }
                dmo_xml_parser->drv_config.drv_excp_code_parser.excp_code[excp_code_cnt].type_and_action |= fbe_dmo_type_action[index].type_action;

                if(dmo_xml_parser->drv_config.drv_excp_code_parser.excp_code[excp_code_cnt].type_and_action == FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_INVALID){
                    dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag &= ~DRV_ERR_CLSFY_ATTR_ACT_TYPE_PROC;
                    dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_err_any |= DRV_EXCP_ERR_CLSFY_PROC_ERR_DETECTED;
                    fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                             "DIEH XML process_error_classify - Invalid error classification detected. excp:%d\n", excp_code_cnt);
                }else{
                    dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag |= DRV_ERR_CLSFY_ATTR_ACT_TYPE_PROC;
                }
            }else{
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                         "DIEH XML process_error_classify 2 - Invalid error classification detected. excp:%d\n", excp_code_cnt);
                dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag &= ~DRV_ERR_CLSFY_ATTR_ACT_TYPE_PROC;
                dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_err_any |= DRV_EXCP_ERR_CLSFY_PROC_ERR_DETECTED;
            }
        }else{
            fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                     "DIEH XML process_error_classify - Error classification not detected. excp:%d\n", excp_code_cnt);
            dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_flag &= ~DRV_ERR_CLSFY_ATTR_ACT_TYPE_PROC;
            dmo_xml_parser->drv_config.drv_excp_code_parser.excp_list_err_any |= DRV_EXCP_ERR_CLSFY_PROC_ERR_DETECTED;
        }
    }
}

/*!**************************************************************
 * @fn dmo_dieh_parse_string
 ****************************************************************
 * @brief
 *  Parses a string into fields.    NOTE, the string passed in
 *  will be modified.  Caller should expect this.  Also the
 *  fields are pointers into in_string.   Once in_string is deallocated
 *  by caller, the field values will not be valid.  
 * 
 * @param delimiter - field separator
 * @param min_fields -   min fields expected
 * @param max_fields - max fields expected
 * @param in_string  - string to be parsed.  
 * @param out_fields_array - returned array of fields
 * @param out_num_fields - num fields found.
 * 
 * @return fbe_status_t
 *
 * @version
 *   01/24/2014:  Wayne Garrett - Created.
 *
 ****************************************************************/
fbe_status_t dmo_dieh_parse_string(const fbe_u8_t * delimiter, fbe_u32_t min_fields, fbe_u32_t max_fields, fbe_u8_t * in_string, fbe_dmo_attribute_field_array_t out_fields_array_p, fbe_u32_t * out_num_fields_p)
{
    
    csx_string_t saveptr; 

    *out_num_fields_p = 0;
   
    out_fields_array_p[*out_num_fields_p] = csx_p_strtok_r(in_string, delimiter, &saveptr);

    while (NULL != out_fields_array_p[*out_num_fields_p] && 
           (*out_num_fields_p) < FBE_DMO_MAX_ATTR_FIELDS)
    {
        (*out_num_fields_p)++;
        out_fields_array_p[*out_num_fields_p] = csx_p_strtok_r(CSX_NULL, delimiter, &saveptr);
    }

    if ((*out_num_fields_p) >= FBE_DMO_MAX_ATTR_FIELDS)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s: DIEH XML - max fields [%d] hit str:%s\n", __FUNCTION__, (*out_num_fields_p), in_string);
    }

    if ((*out_num_fields_p) < min_fields)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s: DIEH XML - num fields:%d < %d. str:%s\n", __FUNCTION__, (*out_num_fields_p), min_fields, in_string);
        return FBE_STATUS_GENERIC_FAILURE;  
    }

    if ((*out_num_fields_p) > max_fields)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s: DIEH XML - num fields:%d > %d. str:%s\n", __FUNCTION__, (*out_num_fields_p), max_fields, in_string);
        return FBE_STATUS_GENERIC_FAILURE;  
    }

    return FBE_STATUS_OK;
}
