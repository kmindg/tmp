#ifndef FBE_ESP_DRIVE_MGMT_H
#define FBE_ESP_DRIVE_MGMT_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_esp_drive_mgmt.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions for the ESP Drive Mgmt
 *  object.
 * 
 * @ingroup esp_drive_mgmt_class_files
 * 
 * @revision
 *   03/24/2010:  Created. Ashok Tamilarasan
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_object.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_physical_drive.h"
#include "spid_enum_types.h"
#include "fbe/fbe_devices.h"

/* The size of the header info used for Download Drive Microcode
 * is fixed at 8192 bytes.  That is, we prepend a vendor's ucode
 * image with an 8KB header.  It really has nothing to do with how
 * large a sector is.
 */
#define FBE_FDF_INSTALL_HEADER_SIZE   8192


/***************************************************************************/
/* This is a special marker used to identify a firmware image header block */
/***************************************************************************/
#define FBE_FDF_HEADER_MARKER         0x76543210


/* Maximum concurrent downloads allowed.  This is used by online 
 * drive firmware download.  This is an artifical limit.
 * It is supposed to be set to the max rebuild logs allowed, but the
 * design has changed such that there is no longer a limit.  Keeping
 * this as carry over from the legacy s/w. 
 */
#define FBE_MAX_CONFIG_DL_DRIVES 20  


#define FBE_FDF_CFLAGS_RESERVED            0x01
#define FBE_FDF_CFLAGS_SELECT_DRIVE        0x02  /* check drive bitmap for targeted drives              */
#define FBE_FDF_CFLAGS_CHECK_REVISION      0x04  /* Compare revision on Drive vs File revision          */
#define FBE_FDF_CFLAGS_NO_REBOOT           0x08  /* Don't reboot SP after download (NorthStar)          */
#define FBE_FDF_CFLAGS_TRAILER_PRESENT     0x10  /* Indicates if TLA trailer is present in header       */
#define FBE_FDF_CFLAGS_DELAY_DRIVE_RESET   0x20  /* Manual Reset of drive required for firmware load    */
#define FBE_FDF_CFLAGS_ONLINE              0x40  /* Online firmware download                            */
#define FBE_FDF_CFLAGS_FAIL_NONQUALIFIED   0x80  /* Fail if there are non-qualified drives              */

#define FBE_FDF_CFLAGS2_CHECK_TLA          0x01  /* TLA # compared between the FDF and the drive when set */
#define FBE_FDF_CFLAGS2_FAST_DOWNLOAD      0x02  /* TLA # compared between the FDF and the drive when set */

#define FBE_FDF_FILENAME_LEN               30





/*****************************************************************/
/* These are the sizes for the trailer fields                    */
/*****************************************************************/

#define FBE_FDF_TLA_SIZE                 12

/* Note that the current flare revision size is 9 (01/06) */
/* 24 seems a reasonable expansion size (“dddd/bbbb”)     */
#define FBE_FDF_REVISION_SIZE            24 

/* Calculated size of trailer pad. Intent is to allow modest expansion of trailer before
 * users of trailer information need to do conversions over checking a Tflags value for 
 * new field addition
*/
#define FBE_FDF_PAD_SIZE (128-FBE_FDF_TLA_SIZE-FBE_FDF_REVISION_SIZE-sizeof(long)-sizeof(long)-sizeof(long))


/*****************************************************************/
/* This is a special marker used to identify a firmware image    */
/* trailer block embedded within the overall header              */
/*****************************************************************/

#define FBE_FDF_TRAILER_MARKER                   0x4E617669



/********************* 
 * DIEH defines
 ********************/
#define FBE_DIEH_PATH_LEN         256
#define FBE_DIEH_DEFAULT_FILENAME "DriveConfiguration.xml"
#define FBE_DIEH_PATH_REG_KEY     "DIEH_path"


#define FBE_LOGICAL_ERROR_ACTION_REG_KEY  "ActionOnCRCError"
#define FBE_LOGICAL_ERROR_ACTION_DEFAULT  FBE_PDO_ACTION_KILL_ON_MULTI_BIT_CRC

#define FBE_FUEL_GAUGE_POLL_INTERVAL_REG_KEY  "FuelGaugePollInterval"

/*------------------------------------------------------------------
 * Fuel Gauge Definition
 *------------------------------------------------------------------*/
#define DMO_FUEL_GAUGE_READ_BUFFER_LENGTH   55296    /* Define the biggest size of BMS data is retrieved from the drive.*/
#define DMO_FUEL_GAUGE_SESSION_HEADER_SIZE  17       /* 17 bytes for session header */
#define DMO_FUEL_GAUGE_DRIVE_HEADER_SIZE    63       /*  63 bytes for Drive header*/
#define DMO_FUEL_GAUGE_DATA_HEADER_SIZE     7        /* 7 bytes for log page data header*/
#define DMO_FUEL_GAUGE_MSG_HEADER_SIZE (DMO_FUEL_GAUGE_SESSION_HEADER_SIZE + DMO_FUEL_GAUGE_DRIVE_HEADER_SIZE + DMO_FUEL_GAUGE_DATA_HEADER_SIZE )
#define DMO_FUEL_GAUGE_WRITE_BUFFER_TRANSFTER_COUNT 1024        /* Length of Page 0x31 is 608 byes as now. Set it to 1024 for future grow.  */
#define DMO_FUEL_GAUGE_WRITE_BUFFER_LENGTH (DMO_FUEL_GAUGE_MSG_HEADER_SIZE + DMO_FUEL_GAUGE_WRITE_BUFFER_TRANSFTER_COUNT)
#define DMO_FUEL_GAUGE_FAILED_LBA_BUFFER_LENGTH 2048*64 /* Buffer size for the failed lba buffer. */


#define FBE_DMO_DRIVECONFIG_XML_MAX_VERSION_LEN 32
#define FBE_DMO_DRIVECONFIG_XML_MAX_DESCRIPTION_LEN 128

typedef struct fbe_esp_dmo_driveconfig_xml_info_s
{
    fbe_u8_t version[FBE_DMO_DRIVECONFIG_XML_MAX_VERSION_LEN+1];  // +1 for string terminator
    fbe_u8_t description[FBE_DMO_DRIVECONFIG_XML_MAX_DESCRIPTION_LEN+1];
    fbe_u8_t filepath[FBE_DIEH_PATH_LEN];
} fbe_esp_dmo_driveconfig_xml_info_t;


/*************************
 *   FUNCTION DEFINITIONS 
 *************************/

 /*! @defgroup esp_drive_mgmt_class ESP Drive MGMT Class
 *  @brief This is the set of definitions for the drive
 *  management class.
 *  @ingroup fbe_base_object
 */ 
/*! @defgroup esp_drive_mgmt_usurper_interface ESP Drive MGMT 
 *  Usurper Interface 
 *  @brief This is the set of definitions that comprise the
 *  drive management class usurper interface.
 *  @ingroup fbe_classes_usurper_interface
 * @{
 */


/*!********************************************************************* 
 * @enum fbe_esp_drive_mgmt_control_code_t 
 *  
 * @brief 
 * This enumeration lists all the ESP Drive Mgmt specific control
 * codes which the Drive Mgmt object will accept.
 *         
 **********************************************************************/
typedef enum
{
    FBE_ESP_DRIVE_MGMT_CONTROL_CODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_DRIVE_MGMT),

    // retrieve Drive Info from ESP
    FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_DRIVE_INFO,
    FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_POLICY,
    FBE_ESP_DRIVE_MGMT_CONTROL_CODE_CHANGE_POLICY,
    FBE_ESP_DRIVE_MGMT_CONTROL_CODE_ENABLE_FUEL_GAUGE,
    FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_FUEL_GAUGE,
    FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_FUEL_GAUGE_POLL_INTERVAL,
    FBE_ESP_DRIVE_MGMT_CONTROL_CODE_SET_FUEL_GAUGE_POLL_INTERVAL,
    FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_DRIVE_LOG,
    FBE_ESP_DRIVE_MGMT_CONTROL_CODE_DEBUG_COMMAND,
    FBE_ESP_DRIVE_MGMT_CONTROL_CODE_DIEH_LOAD_CONFIG,
    FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_ALL_DRIVES_INFO,/*one shot call to get FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_DRIVE_INFO of all drives*/
    FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_TOTAL_DRIVES_COUNT,/*how many drives are there, to be used for memory allocation for FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_ALL_DRIVES_INFO*/

    FBE_ESP_DRIVE_MGMT_CONTROL_SET_CRC_ACTION,

    FBE_ESP_DRIVE_MGMT_CONTROL_CODE_REDUCE_SYSTEM_DRIVE_QUEUE_DEPTH,
    FBE_ESP_DRIVE_MGMT_CONTROL_CODE_RESTORE_SYSTEM_DRIVE_QUEUE_DEPTH,
    FBE_ESP_DRIVE_MGMT_CONTROL_CODE_RESET_SYSTEM_DRIVE_QUEUE_DEPTH,
    FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_MAX_PLATFORM_DRIVE_COUNT, /* how many drives the platform allows */
    
    FBE_ESP_DRIVE_MGMT_CONTROL_CODE_WRITE_LOG_PAGE,
    FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_DRIVECONFIG_INFO,
    /* Insert new control codes here. */

    /* TESTING ONLY COMMANDS */
    FBE_ESP_DRIVE_MGMT_SIMULATION_COMMAND,

    FBE_ESP_DRIVE_MGMT_CONTROL_CODE_LAST
}
fbe_esp_drive_mgmt_control_code_t;


/*!********************************************************************* 
 * @enum fbe_esp_drive_mgmt_policy_id_t 
 *  
 * @brief 
 * This enumeration lists all the ESP Drive Mgmt policies
 *         
 **********************************************************************/
typedef enum fbe_drive_mgmt_policy_id_e 
{
    FBE_DRIVE_MGMT_POLICY_VERIFY_CAPACITY,    
    FBE_DRIVE_MGMT_POLICY_VERIFY_ENHANCED_QUEUING, /* <-- now handled in SEP via database_service*/
    FBE_DRIVE_MGMT_POLICY_SET_SYSTEM_DRIVE_IO_MAX,
	FBE_DRIVE_MGMT_POLICY_VERIFY_FOR_SED_COMPONENTS,
    FBE_DRIVE_MGMT_POLICY_LAST
} fbe_drive_mgmt_policy_id_t;

/*!********************************************************************* 
 * @enum fbe_drive_mgmt_policy_value_t 
 *  
 * @brief 
 * This enumeration lists the policies values indicating if the policy
 * is enforced.
 *         
 **********************************************************************/
typedef enum fbe_drive_mgmt_policy_value_e
{
    FBE_DRIVE_MGMT_POLICY_OFF,
    FBE_DRIVE_MGMT_POLICY_ON
} fbe_drive_mgmt_policy_value_t;


/* Public Interface Structures */

/*!********************************************************************* 
 * @struct fbe_drive_mgmt_drive_policy_t 
 *  
 * @brief 
 * This struct contains the policy information.
 *         
 **********************************************************************/
typedef struct fbe_drive_mgmt_policy_s
{
    fbe_drive_mgmt_policy_id_t      id;
    fbe_drive_mgmt_policy_value_t   value;
} fbe_drive_mgmt_policy_t;


/*!********************************************************************* 
 * @struct fbe_drive_mgmt_dieh_load_config_t 
 *  
 * @brief 
 *   Contains the status for the fw download process
 *
 * @ingroup fbe_api_drive_mgmt_interface
 **********************************************************************/
typedef struct fbe_drive_mgmt_dieh_load_config_s
{
    fbe_u8_t   file[FBE_DIEH_PATH_LEN];   /* xml file to load*/
} fbe_drive_mgmt_dieh_load_config_t;



/*!****************************************************************************
 *    
 * @struct dmo_event_log_drive_offline_reason_e
 *  
 * @brief 
 *   This is the definition of the enumeration for the reasons a drive
 *   was taken offline
 ******************************************************************************/
typedef enum dmo_event_log_drive_offline_reason_e{
    /* these values should not conflict with other death reasons, defined in fbe_physical_drive.h */
    DMO_DRIVE_OFFLINE_REASON_INVALID = FBE_OBJECT_DEATH_CODE_INVALID_DEF(FBE_CLASS_ID_DRIVE_MGMT),
    DMO_DRIVE_OFFLINE_REASON_DRIVE_REMOVED,
    DMO_DRIVE_OFFLINE_REASON_UNKNOWN_FAILURE,  
    DMO_DRIVE_OFFLINE_REASON_PERSISTED_EOL,
    DMO_DRIVE_OFFLINE_REASON_PERSISTED_DF,
    DMO_DRIVE_OFFLINE_REASON_NON_EQ,  /* drive doesn't support Enhance Queuing */
    DMO_DRIVE_OFFLINE_REASON_INVALID_ID,
    DMO_DRIVE_OFFLINE_REASON_SSD_LE,   /* Low endurance SSD not supported */
    DMO_DRIVE_OFFLINE_REASON_SSD_RI,   /* Read intensive SSD not supported */
    DMO_DRIVE_OFFLINE_REASON_HDD_520,  /* 520 block size HDD not supported */
    /* always add new death reasons to at end.  scripts and field support may depend on existing values */
    DMO_DRIVE_OFFLINE_REASON_LAST
}dmo_event_log_drive_offline_reason_t;

typedef enum fbe_dieh_load_status_e{
    FBE_DMO_THRSH_NO_ERROR = 0,
    FBE_DMO_THRSH_FILE_INVD_PATH_ERROR,
    FBE_DMO_THRSH_FILE_READ_ERROR,
    FBE_DMO_THRSH_XML_ELEMENT_PARSE_ERROR,
    FBE_DMO_THRSH_MEM_ERROR,
    FBE_DMO_THRSH_UPDATE_ALREADY_IN_PROGRESS,  /* DCS was already told to start and update, which hasn't finished.  */
    FBE_DMO_THRSH_ERROR,                       /* generic failure. */

} fbe_dieh_load_status_t;

/*!****************************************************************************
 *    
 * @struct dmo_get_all_drives_info_t
 *  
 * @brief 
 *   This is the structure to pass some data to FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_ALL_DRIVES_INFO
 *   The rest of the data is passed to a pointer in a sg list that is built in the FBE API itself
 ******************************************************************************/
typedef struct dmo_get_all_drives_info_s{
    fbe_u32_t   expected_count;/*IN*/
    fbe_u32_t   actual_count;/*OUT*/
}dmo_get_all_drives_info_t;

/*!****************************************************************************
 *    
 * @struct dmo_get_all_drives_info_t
 *  
 * @brief 
 *   This is the structure to pass some data to FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_TOTAL_DRIVES_COUNT
 ******************************************************************************/
typedef struct dmo_get_total_drives_count_s{
    fbe_u32_t   total_count;/*OUT*/
}dmo_get_total_drives_count_t;


#endif /* FBE_ESP_DRIVE_MGMT_H */

/*******************************
 * end file fbe_esp_drive_mgmt.h
 *******************************/
