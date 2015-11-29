#ifndef FBE_DRIVE_MGMT_DIEH_XML_H
#define FBE_DRIVE_MGMT_DIEH_XML_H


/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/********************************************************************
 * fbe_drive_mgmt_dieh_xml.h
 ********************************************************************
 *
 * Description:
 *  File contains datastructures and functions needed for parsing FBE
 *  Drive Error Thresholding XML File. 
 *
 * Functions:
 *
 *
 * History:
 *       03/20/12 : Wayne Garrett -- Created. 
 *
 *********************************************************************/

/*************************************************************************
 ** Include Files
 *************************************************************************/

#include "fbe/fbe_api_drive_configuration_service_interface.h" 
#include "fbe/fbe_stat.h"
#include "fbe_drive_mgmt_private.h"
#include "fbe_drive_mgmt_xml_api.h"



#define FBE_DMO_THRSH_XML_VERSION_MAJOR "3"   /*latest version (major) this parser supports. */

#define FBE_DMO_THRSH_FILENAME_LEN   128
#define FBE_DMO_THRSH_PATHNAME_LEN   256
#define DIEH_XML_BUFFER_BYTES 1024

#define FBE_DMO_THRSH_CONFIG_FILE "DriveConfiguration.xml"


typedef enum fbe_dmo_thrsh_state_type_e{    
    FBE_DMO_THRSH_DOC_NOT_INIT = 0, 
    FBE_DMO_THRSH_DOC_INIT,
    FBE_DMO_THRSH_DOC_ERROR,
    FBE_DMO_THRSH_DOC_PARSE_IN_PRC,
    FBE_DMO_THRSH_DOC_ELEM_PRC,
    FBE_DMO_THRSH_DOC_ELEM_EXISTS,
    FBE_DMO_THRSH_DOC_FNSH, 
    FBE_DMO_THRSH_ELEM_MEM_NULL,
    FBE_DMO_THRSH_DOC_REINIT,          
} fbe_dmo_thrsh_state_type_t;


/* Defines for XML Element, attributes and attribut value string defines*/
#define FBE_DMO_XML_ELEM_DMO_THRSH_CFG_TAG_STR          "fbeThresholdConfiguration"
#define FBE_DMO_XML_ELEM_PORT_CFGS_TAG_STR              "portConfigurations"
#define FBE_DMO_XML_ELEM_PORT_CFG_TAG_STR               "portConfiguration"
#define FBE_DMO_XML_ELEM_DRV_STATS_TAG_STR              "driveStats"
#define FBE_DMO_XML_ELEM_ENCL_STATS_TAG_STR             "enclosureStats"
#define FBE_DMO_XML_ELEM_DRV_CFG_TAG_STR                "driveConfigurations"
#define FBE_DMO_XML_ELEM_SAS_DRV_CFG_TAG_STR            "sasDriveConfiguration"
#define FBE_DMO_XML_ELEM_DRV_STATCS_TAG_STR             "driveStatistics"
#define FBE_DMO_XML_ELEM_CMLT_STATS_TAG_STR             "cumulativeStats"
#define FBE_DMO_XML_ELEM_RCVR_ERR_STATS_TAG_STR         "recoveredStats"
#define FBE_DMO_XML_ELEM_MEDIA_ERR_STATS_TAG_STR        "mediaStats"
#define FBE_DMO_XML_ELEM_HEALTHCHECK_ERR_STATS_TAG_STR  "healthCheckStats"
#define FBE_DMO_XML_ELEM_HW_ERR_STATS_TAG_STR           "hardwareStats"
#define FBE_DMO_XML_ELEM_LINK_ERR_STATS_TAG_STR         "linkStats"
#define FBE_DMO_XML_ELEM_DRV_RST_STATS_TAG_STR          "resetStats"
#define FBE_DMO_XML_ELEM_DRV_PWR_CYC_TAG_STR            "powerCycleStats"
#define FBE_DMO_XML_ELEM_DATA_ERR_STATS_TAG_STR         "dataStats"
#define FBE_DMO_XML_ELEM_RCVR_TAG_RDC_TAG_STR           "recoveryTagReduce"
#define FBE_DMO_XML_ELEM_ERR_BRST_WT_RDC_TAG_STR        "errorBurstWeightReduce"
#define FBE_DMO_XML_ELEM_ERR_BRST_DLT_TAG_STR           "errorBurstDelta"
#define FBE_DMO_XML_ELEM_DRV_INFO_TAG_STR               "driveInformation"
#define FBE_DMO_XML_ELEM_DRV_VND_TAG_STR                "vendor"
#define FBE_DMO_XML_ELEM_DRV_PART_NO_TAG_STR            "partNumber"
#define FBE_DMO_XML_ELEM_DRV_FRMW_REV_TAG_STR           "firmwareRev"
#define FBE_DMO_XML_ELEM_DRV_SER_NO_TAG_STR             "serialNumber"
#define FBE_DMO_XML_ELEM_DRV_Q_DEPTH_TAG_STR            "DriveQDepth"
#define FBE_DMO_XML_ELEM_DRV_GLOBAL_PARAMETERS_TAG_STR    "GlobalParameters"
//#define FBE_DMO_XML_ELEM_DRV_TYP_TAG_STR              "driveType"
#define FBE_DMO_XML_ELEM_SAS_EXCP_CODES_TAG_STR         "sasDriveExceptionCodes"
#define FBE_DMO_XML_ELEM_SCSI_EXCP_CODE_TAG_STR         "scsiExcpetionCode"
#define FBE_DMO_XML_ELEM_SCSI_CODE_TAG_STR              "scsiCode"
#define FBE_DMO_XML_ELEM_SATA_EXCP_STS_TAG_STR          "sataDriveExceptionStatuses"
#define FBE_DMO_XML_ELEM_IO_STATUS_TAG_STR              "ioStatus"
#define FBE_DMO_XML_ELEM_ERR_CLSFY_TAG_STR              "errorClassify"
#define FBE_DMO_XML_ELEM_CONTROL_DFTL_TAG_STR           "controlDefault"
#define FBE_DMO_XML_ELEM_WEIGHT_CHANGE_TAG_STR          "weightChange"

#define FBE_DMO_XML_ATTR_PORT_TYPE_TAG_STR              "portType"
#define FBE_DMO_XML_ATTR_DRV_TYPE_TAG_STR               "driveType"
#define FBE_DMO_XML_ATTR_VND_NAME_TAG_STR               "name"
#define FBE_DMO_XML_ATTR_DRV_PART_NO_TAG_STR            "partNumber"
#define FBE_DMO_XML_ATTR_DRV_FRMW_REV_TAG_STR           "firmwareRev"
#define FBE_DMO_XML_ATTR_DRV_SER_NO_START_TAG_STR       "sNoStart"
#define FBE_DMO_XML_ATTR_DRV_SER_NO_END_TAG_STR         "sNoEnd"
#define FBE_DMO_XML_ATTR_DRV_DEFAULT_QDEPTH_TAG_STR     "DefaultQDepth"
#define FBE_DMO_XML_ATTR_DRV_SYSTEM_NORMAL_QDEPTH_TAG_STR      "SystemNormalQDepth"
#define FBE_DMO_XML_ATTR_DRV_SYSTEM_REDUCED_QDEPTH_TAG_STR     "SystemReducedQDepth"
#define FBE_DMO_XML_ATTR_DRV_SERVICE_TIMEOUT_STR        "ServiceTimeout"
#define FBE_DMO_XML_ATTR_DRV_REMAP_SERVICE_TIMEOUT_STR  "RemapServiceTimeout"
#define FBE_DMO_XML_ATTR_DRV_EMEH_SERVICE_TIMEOUT_STR   "EmehServiceTimeout"
#define FBE_DMO_XML_ATTR_ERR_THRSH_TAG_STR              "threshold"
#define FBE_DMO_XML_ATTR_IO_INTV_TAG_STR                "io_interval"
#define FBE_DMO_XML_ATTR_ERR_WGHT_TAG_STR               "weight"
#define FBE_DMO_XML_ATTR_VAL_TAG_STR                    "value"
#define FBE_DMO_XML_ATTR_RCVR_TAG_RDC_TAG_STR           FBE_DMO_XML_ATTR_VAL_TAG_STR
#define FBE_DMO_XML_ATTR_ERR_BRST_WT_RDC_TAG_STR        FBE_DMO_XML_ATTR_VAL_TAG_STR
#define FBE_DMO_XML_ATTR_ERR_BRST_DLT_TAG_STR           FBE_DMO_XML_ATTR_VAL_TAG_STR
#define FBE_DMO_XML_ATTR_SENSE_KEY_TAG_STR              "senseKey"
#define FBE_DMO_XML_ATTR_SENSE_CODE_START_TAG_STR       "senseCodeStart"
#define FBE_DMO_XML_ATTR_SENSE_CODE_END_TAG_STR         "senseCodeEnd"
#define FBE_DMO_XML_ATTR_SENSE_QUAL_ST_TAG_STR          "senseQualStart"
#define FBE_DMO_XML_ATTR_SENSE_QUAL_END_TAG_STR         "senseQualEnd"
#define FBE_DMO_XML_ATTR_IO_STATUS_TAG_STR              "status"
#define FBE_DMO_XML_ATTR_TYPE_N_ACTION_TAG_STR          "typeAction"
#define FBE_DMO_XML_ATTR_CONTROL_FLAG_TAG_STR           "control_flag"
#define FBE_DMO_XML_ATTR_VERSION_STR                    "version"
#define FBE_DMO_XML_ATTR_DESCRIPTION_STR                "description"

#define FBE_DMO_XML_ATTR_TYPE_NO_ERROR_VAL_TAG_STR      "noError"
#define FBE_DMO_XML_ATTR_TYPE_RCV_ERROR_VAL_TAG_STR     "recoveredError"
#define FBE_DMO_XML_ATTR_TYPE_MEDIA_ERROR_VAL_TAG_STR   "mediaError"
#define FBE_DMO_XML_ATTR_TYPE_HW_ERROR_VAL_TAG_STR      "hardwareError"
#define FBE_DMO_XML_ATTR_TYPE_LINK_ERROR_VAL_TAG_STR    "linkError"
#define FBE_DMO_XML_ATTR_TYPE_REMAP_ERROR_VAL_TAG_STR   "remapped"
#define FBE_DMO_XML_ATTR_ACTION_PACO_VAL_TAG_STR        "proactiveCopy"
#define FBE_DMO_XML_ATTR_TYPE_FATAL_ERROR_VAL_TAG_STR   "fatal"
#define FBE_DMO_XML_ATTR_TNA_UNKNOWN_VAL_TAG_STR        "unknown"

#define FBE_DMO_XML_ELEM_QUEUING_CFG_TAG_STR            "queuingConfigurations"
#define FBE_DMO_XML_ELEM_SAS_QUEUING_CFG_TAG_STR        "sasQueuingConfiguration"
#define FBE_DMO_XML_ELEM_EQ_TIMER_TAG_STR               "eqTimer"
#define FBE_DMO_XML_ATTR_LPQ_TIMER_TAG_STR              "lpqTimer"
#define FBE_DMO_XML_ATTR_HPQ_TIMER_TAG_STR              "hpqTimer"

#define FBE_DMO_XML_ELEM_RELIABLY_CHALLENGED_TAG_STR    "rc"
#define FBE_DMO_XML_ATTR_RELIABLY_CHALLENGED_TAG_STR    FBE_DMO_XML_ATTR_VAL_TAG_STR

/* Structure to store port type values and correspoding port type values*/
typedef struct fbe_dmo_thrsh_port_type_attr_val_s{
    char                    *port_val;
    fbe_port_type_t         port_type;
}
fbe_dmo_thrsh_port_type_attr_val_t;


typedef struct fbe_dmo_thrsh_drv_vendors_name_s{
    char                    *vendor_name;
    fbe_drive_vendor_id_t   drive_vendor_id;
}fbe_dmo_thrsh_drv_vendors_name_t;


typedef struct fbe_dmo_thrsh_cdb_fis_status_s{
    char                            *name;
    fbe_payload_cdb_fis_io_status_t status;
}fbe_dmo_thrsh_cdb_fis_status_t;

typedef struct fbe_dmo_thrsh_cdb_fis_error_flags_s{
    char                                *name;
    fbe_payload_cdb_fis_error_flags_t   type_action;
}fbe_dmo_thrsh_cdb_fis_error_flags_t;

typedef enum fbe_dmo_xml_element_tag_type_e{
    FBE_DMO_XML_ELEM_DMO_UNDEFINED_TAG = 0,
    FBE_DMO_XML_ELEM_DMO_THRSH_CFG_TAG,
    FBE_DMO_XML_ELEM_PORT_CFGS_TAG,
    FBE_DMO_XML_ELEM_PORT_CFG_TAG,
    FBE_DMO_XML_ELEM_DRV_STATS_TAG,
    FBE_DMO_XML_ELEM_ENCL_STATS_TAG,
    FBE_DMO_XML_ELEM_DRV_CFG_TAG,
    FBE_DMO_XML_ELEM_SAS_DRV_CFG_TAG,
    FBE_DMO_XML_ELEM_SATA_DRV_CFG_TAG,
    FBE_DMO_XML_ELEM_DRV_STATCS_TAG,
    FBE_DMO_XML_ELEM_CMLT_STATS_TAG,
    FBE_DMO_XML_ELEM_RCVR_ERR_STATS_TAG,
    FBE_DMO_XML_ELEM_MEDIA_ERR_STATS_TAG,
    FBE_DMO_XML_ELEM_HW_ERR_STATS_TAG,
    FBE_DMO_XML_ELEM_HEALTHCHECK_ERR_STATS_TAG,
    FBE_DMO_XML_ELEM_LINK_ERR_STATS_TAG,
    FBE_DMO_XML_ELEM_DRV_RST_STATS_TAG,
    FBE_DMO_XML_ELEM_DRV_PWR_CYC_TAG,
    FBE_DMO_XML_ELEM_DATA_ERR_STATS_TAG,
    FBE_DMO_XML_ELEM_RCVR_TAG_RDC_TAG,
    FBE_DMO_XML_ELEM_ERR_BRST_WT_RDC_TAG,
    FBE_DMO_XML_ELEM_ERR_BRST_DLT_TAG,
    FBE_DMO_XML_ELEM_DRV_INFO_TAG,
    FBE_DMO_XML_ELEM_DRV_VND_TAG,
    FBE_DMO_XML_ELEM_DRV_PART_NO_TAG,
    FBE_DMO_XML_ELEM_DRV_FRMW_REV_TAG,
    FBE_DMO_XML_ELEM_DRV_SER_NO_TAG,
//  FBE_DMO_XML_ELEM_DRV_TYP_TAG,
    FBE_DMO_XML_ELEM_SAS_EXCP_CODES_TAG,
    FBE_DMO_XML_ELEM_SCSI_EXCP_CODE_TAG,
    FBE_DMO_XML_ELEM_SCSI_CODE_TAG, 
    FBE_DMO_XML_ELEM_IO_STATUS_TAG,
    FBE_DMO_XML_ELEM_ERR_CLSFY_TAG,
    FBE_DMO_XML_ELEM_CONTROL_DFTL_TAG,
    FBE_DMO_XML_ELEM_WEIGHT_CHANGE_TAG,    
    FBE_DMO_XML_ELEM_DRV_Q_DEPTH_TAG,
    FBE_DMO_XML_ELEM_DRV_GLOBAL_PARAMETERS_TAG,
    FBE_DMO_XML_ELEM_QUEUING_CFG_TAG,
    FBE_DMO_XML_ELEM_SAS_QUEUING_CFG_TAG,
    FBE_DMO_XML_ELEM_EQ_TIMER_TAG,
    FBE_DMO_XML_ELEM_RELIABLY_CHALLENGED_TAG,
}fbe_dmo_xml_element_tag_type_t;


/*Structure used for storing tag type and correspoding XML Tag Names*/
typedef struct fbe_dmo_thrsh_xml_element_tags_s{
    char                            *tag_name;
    fbe_dmo_xml_element_tag_type_t  tag_type;
}
fbe_dmo_thrsh_xml_element_tags_t;

/**************************************************************************
 * Flags to keep track which drive information elements have been processed
 **************************************************************************/ 
typedef enum fbe_drv_info_proc_flags_e{     
    NO_DRV_INFO_PROC        = 0x00000000,
    DRV_INFO_DRV_TYPE_PROC  = 0x00000001,
    DRV_INFO_VND_PROC       = 0x00000002,
    DRV_INFO_PART_NO_PROC   = 0x00000004,
    DRV_INFO_FW_REV_PROC    = 0x00000008,
    DRV_INFO_SNO_PROC       = 0x00000010,   
    DRV_INFO_VND_ATTR_VN_ID_PROC    = 0x00000020,
    DRV_INFO_PART_NO_ATTR_PNO_PROC  = 0x00000040,
    DRV_INFO_FW_ATTR_FW_REV_PROC    = 0x00000080,
    DRV_INFO_SNO_ATTR_SNO_ST_PROC   = 0x00000100,
    DRV_INFO_SNO_ATTR_SNO_END_PROC  = 0x00000200,
    DRV_INFO_QDEPTH_PROC                              = 0x00001000,
    DRV_INFO_QDEPTH_ATTR_DEFAULT_QDEPTH_PROC          = 0x00002000,
    DRV_INFO_QDEPTH_ATTR_SYSTEM_NORMAL_QDEPTH_PROC    = 0x00004000,
    DRV_INFO_QDEPTH_ATTR_SYSTEM_REDUCED_QDEPTH_PROC   = 0x00008000,
} fbe_drv_info_proc_flags_t;


/********************************************************************** 
 * Structure to track drive information element
 **********************************************************************/
typedef struct fbe_drv_info_element_parser_s{
    fbe_drv_info_proc_flags_t   drv_info_proc;    
    
    // Drive Information record to store the drive header information from XML
    fbe_drive_configuration_drive_info_t    drive_info;    
    fbe_u32_t                               default_q_depth;
    fbe_u32_t                               system_normal_q_depth;
    fbe_u32_t                               system_reduced_q_depth;    
}fbe_drv_info_element_parser_t;


/**********************************************************************************
 * Flags to keep track what all elements have been processed, Values are coded to 
 * be set bitwise, so that multiple of the flags can be detected by the same flag
 **********************************************************************************/
typedef enum fbe_drv_excp_code_flags_e{     
    NO_DRV_EXCP_CODE_PROC               = 0x00000000,
    DRV_SCSI_CODE_ATTR_SK_PROC          = 0x00000001,
    DRV_SCSI_CODE_ATTR_ASC_START_PROC   = 0x00000002,
    DRV_SCSI_CODE_ATTR_ASC_END_PROC     = 0x00000004,
    DRV_SCSI_CODE_ATTR_ASCQ_ST_PROC     = 0x00000008,
    DRV_SCSI_CODE_ATTR_ASCQ_END_PROC    = 0x00000010,
    DRV_IO_STATUS_ATTR_CDB_FIS_STS_PROC = 0x00000020,
    DRV_ERR_CLSFY_ATTR_ACT_TYPE_PROC    = 0x00000040,
    DRV_EXCP_CODE_SCSI_CODE_PROC        = 0x00000080,

    DRV_EXCP_CODE_IO_STATUS_PROC            = 0x00000400,
    DRV_EXCP_CODE_ERR_CLSFY_ACT_TYPE_PROC   = 0x00000800,   
    DRV_EXCP_CODE_ANY_ERR_CLSFY_PROC_ERROR  = 0x00001000,   
    DRV_EXCP_CODE_SCSI_EXCP_CODE_PROC       = 0x00002000,
    DRV_SAS_EXCP_CODE_PROC                  = 0x00004000,
} fbe_drv_excp_code_flags_t;

/**********************************************************************************
 * Flags to keep track while loading any record from configuration, if there 
 * were any error countered. If so the whole record would have to marked dirty
 **********************************************************************************/
typedef enum fbe_drv_excp_any_err_detected_e{       
    DRV_EXCP_NO_ERROR_RCD                   = 0x00000000,
    DRV_EXCP_SCSI_CODE_PROC_ERR_DETECTED    = 0x00000001,   

    DRV_EXCP_IO_STATUS_PROC_ERR_DETECTED    = 0x00000004,
    DRV_EXCP_ERR_CLSFY_PROC_ERR_DETECTED    = 0x00000008,
    DRV_EXCP_CODES_EXCEED_MAX_COUNT         = 0x00000010,
} fbe_drv_excp_any_err_detected_t;

/********************************************************************** 
 * Structure to track SCSI and FIS Exception code elements
 **********************************************************************/
typedef struct fbe_drv_excp_code_element_parser_s
{    
    // Flag to keep track of which element is currently being 
    fbe_drv_excp_code_flags_t           excp_list_flag;    
    fbe_drv_excp_any_err_detected_t     excp_list_err_any;

    // Flag to keep count of how many scsi/fis exception list have been processed
    fbe_u32_t excp_code_cnt;
    
    // Drive Information record to store the drive header information from XML
    fbe_drive_config_scsi_fis_exception_code_t  excp_code[MAX_EXCEPTION_CODES];    
}fbe_drv_excp_code_element_parser_t;

/********************************************************************** 
 * Structure to track drive error statistics
 **********************************************************************/
/* Flags to keep track what all elements have been processed, Values are coded to be set bitwise, 
 * so that multiple of the flags can be detected by the same flag
 */
typedef enum fbe_drv_stats_flags_e{     
    NO_DRV_STATS_PROC                   = 0x00000000,
    DRV_CML_STATS_PROC                  = 0x00000001,
    DRV_RCVR_STATS_PROC                 = 0x00000002,
    DRV_MEDIA_STATS_PROC                = 0x00000004,
    DRV_HW_STATS_THRSH_PROC             = 0x00000008,
    DRV_LINK_STATS_PROC                 = 0x00000010,
    DRV_RST_STATS_PROC                  = 0x00000020,
    DRV_PW_CYC_STATS_PROC               = 0x00000040,
    DRV_DATA_ERR_STATS_PROC             = 0x00000080,
    DRV_RCVR_TAG_RDC_PROC               = 0x00000100,
    DRV_ERR_BRST_WT_RDC_PROC            = 0x00000200,
    DRV_HEATHCHECK_STATS_PROC           = 0x00000400,
    DRV_ERR_BRST_DLT_PROC               = 0x00004000,
    DRV_RCVR_TAG_RDC_ATTR_VAL_PROC      = 0x00008000,
    DRV_ERR_BRST_WT_RDC_ATTR_VAL_PROC   = 0x00010000,
    DRV_ERR_BRST_DLT_ATTR_VAL_PROC      = 0x00020000,  
} fbe_drv_stats_flags_t;

typedef struct fbe_drv_stats_element_parser_s{       
    // Flag to keep track of which element is currently being 
    fbe_drv_stats_flags_t   drv_stats_flag;     
    
    // Drive Information record to store the drive header information from XML
    fbe_drive_stat_t    drv_stats;    
}fbe_drv_stats_element_parser_t;


//Flag to indicate the threshold value has been parsed
typedef enum fbe_drv_config_flags_e{        
    NO_DRV_CONFIG_PROC      = 0x0,
    DRV_CONFIG_SAS_PROC     = 0x1,
    DRV_CONFIG_SATA_PROC    = 0x2,
    DRV_CONFIG_STATS_PROC   = 0x4,
    DRV_CONFIG_EXCP_PROC    = 0x8,
    DRV_CONFIG_DRV_INFO_PROC= 0x10,
    DRV_CONFIG_SAS_DETECTED = 0x20,
    DRV_CONFIG_SATA_DETECTED= 0x40,
} fbe_drv_config_flags_t;

typedef enum fbe_drv_config_global_param_attr_e {
    FBE_DMO_GLOBAL_PARAM_ATTR_NONE                       = 0x00000000,
    FBE_DMO_GLOBAL_PARAM_ATTR_SERVICE_TIME               = 0x00000001,
    FBE_DMO_GLOBAL_PARAM_ATTR_REMAP_SERVICE_TIME         = 0x00000002,
    FBE_DMO_GLOBAL_PARAM_ATTR_EMEH_SERVICE_TIME          = 0x00000004,
}fbe_dmo_global_param_attr_t;

typedef struct fbe_dmo_global_parameters_s{
    fbe_dcs_tunable_params_t    params;
    fbe_dmo_global_param_attr_t set_flags;
    fbe_bool_t global_parameters_is_changeable;
}fbe_dmo_global_parameters_t;

typedef struct fbe_dmo_drv_config_parser_s{     
    fbe_drive_configuration_record_t drv_config_recd;
    fbe_stats_control_flag_t         default_control_flag;

    fbe_drv_config_flags_t          drv_config_flags;
    //Track number of SAS drive record processed
    fbe_u32_t                           num_sas_drv_rcd_prcd;
    fbe_drv_stats_element_parser_t      drv_stats_elem_parser;
    fbe_drv_excp_code_element_parser_t  drv_excp_code_parser;
    fbe_drv_info_element_parser_t       drv_info_parser;
   
    fbe_dmo_global_parameters_t        global_parameters;    
}fbe_dmo_drv_config_parser_t;


/********************************************************************** 
 * Flag to indicate the threshold value has been parsed
 **********************************************************************/
typedef enum fbe_port_config_flags_e{       
    NO_PORT_CONFIG_PROC         = 0x0,
    PORT_CONFIG_PORT_TYPE_PROC  = 0x1,
    PORT_CONFIG_DRV_STATS_PROC  = 0x2,
    PORT_CONFIG_ENCL_STATS_PROC = 0x4,  
} fbe_port_config_flags_t;


/********************************************************************** 
 * Structure to track port statistics
 **********************************************************************/
typedef struct fbe_port_config_parser_s{    
    // Flag to keep track of which element is currently being 
    fbe_port_config_flags_t port_config_flag;       
    
    // Drive Information record to store the drive header information from XML
    fbe_drive_configuration_port_record_t port_rcd;
}fbe_port_config_parser_t;


//Flag to indicate the threshold value has been parsed
typedef enum fbe_port_configs_flags_e{      
    NO_PORT_CONFIGS_PROC        = 0x0,
    PORT_CONFIGS_RCD_PROC       = 0x1,  
} fbe_port_configs_flags_t;


/*! @enum fbe_stats_errcat_attr_t  
 *  
 *  @brief These are the attributes that are
 *      set for a given error category.
 */
typedef enum fbe_stats_errcat_attr_e {
    FBE_NO_STATS_PROC           =        0x0,
    FBE_STATS_THRSH_PROC        =        0x1,
    FBE_STATS_INTV_PROC         =        0x2,
    FBE_STATS_WT_PROC           =        0x4,
    FBE_STATS_CNTRL_FLG_PROC    =        0x8,
    FBE_STATS_RESET_PROC        =       0x10,
    FBE_STATS_EOL_PROC          =       0x20,
    FBE_STATS_EOL_CH_PROC       =       0x40,
    FBE_STATS_KILL_PROC         =       0x80,
    FBE_STATS_KILL_CH_PROC      =      0x100,
    FBE_STATS_EVENT_PROC        =      0x200,  /* generic event which clients can register callbacks for*/
    FBE_STATS_INVALID_PROC      = 0x80000000
}fbe_stats_errcat_attr_t;


/*! @enum fbe_stats_errcat_attr_element_t  
 *  
 *  @brief Structure use for storing error category attribute value
 *      and corresponding string name.
 */
typedef struct fbe_stats_errcat_attr_element_s{
    fbe_u8_t                       *name;
    fbe_stats_errcat_attr_t         type;
}
fbe_stats_errcat_attr_element_t;

typedef struct fbe_stats_attr_s
{
    fbe_stats_errcat_attr_t     stats_flag;
    fbe_stat_t                  fbe_stat;
}fbe_stats_attr_parser_t;


/*! @enum fbe_stats_wtx_attr_t  
 *  
 *  @brief These are the attributes that are
 *      set for a given weight change category.
 */
typedef enum fbe_stats_wtx_attr_e {
    FBE_STATS_WTX_NONE_PROC =           0x0,
    FBE_STATS_WTX_OPCODE_PROC =         0x1,
    FBE_STATS_WTX_CC_ERROR_PROC =       0x2,
    FBE_STATS_WTX_PORT_ERROR_PROC =     0x4,
    FBE_STATS_WTX_CHANGE_PROC  =        0x8,
    FBE_STATS_WTX_INVALID_PROC = 0x80000000
}fbe_stats_wtx_attr_t;

/*! @enum fbe_stats_wtx_attr_element_t  
 *  
 *  @brief Structure use for storing weight change attribute value
 *      and corresponding string name.
 */
typedef struct fbe_stats_wtx_attr_element_s{
    fbe_u8_t                       *name;
    fbe_stats_wtx_attr_t         type;
}
fbe_stats_wtx_attr_element_t;


/********************************************************************** 
 * Structure to track port statistics
 **********************************************************************/
typedef struct fbe_port_configs_parser_struct{  
    // Flag to keep track of which element is currently being 
    fbe_port_configs_flags_t    port_configs_flag;      
    
    // Drive Information record to store the drive header information from XML
    fbe_port_config_parser_t port_config;

    fbe_u32_t num_port_config_rcd_prcd;
}fbe_port_configs_parser_t, *fbe_port_configs_parser_ptr;


typedef struct fbe_dmo_queuing_parser_s{     
    fbe_drive_configuration_queuing_record_t queuing_record;
}fbe_dmo_queuing_parser_t;


typedef struct fbe_dmo_thrsh_parser_s{
    fbe_esp_dmo_driveconfig_xml_info_t          xml_info;
    fbe_dmo_drv_config_parser_t drv_config;
    fbe_stats_attr_parser_t     stats;
    fbe_dmo_queuing_parser_t    queuing_config;    

    //To keep track of the XML Parsing
    fbe_dmo_thrsh_state_type_t  doc_state;
    fbe_dieh_load_status_t      status;
    
    //Tracks which is the current element being processed 
    fbe_dmo_xml_element_tag_type_t          element_tag_type;
}fbe_dmo_thrsh_parser_t;



/**************************************
 * XML Parsing Error definitions
 *************************************/
typedef enum dieh_xml_error_info_e
{
    DIEH_XML_SUCCESS = 0,
    
    /* Generic failure error. 
     */
    DIEH_XML_FAILURE,

    /* This error is used to denote error in parsing XML file.
     */
    DIEH_XML_PARSE_ERROR,

    /* This is error return for failure on reading a file.
     */
    DIEH_XML_FILE_READ_ERROR, 

    /* Returns the following when there is a error in writing to file.
     */
    DIEH_XML_FILE_WRITE_ERROR,  

    /* When there is a memory allocation error, the following error
     * is returned. 
     */
    DIEH_XML_MEMORY_ALLOCATION_ERROR, 
    
}
dieh_xml_error_info_t;


#endif   /* FBE_DRIVE_MGMT_DIEH_XML_H */
