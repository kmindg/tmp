/* odb_dd_utils.c */

/***************************************************************************
* Copyright (C) EMC Corporation 2000-2012
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************
*
* DESCRIPTION:
*   This file contains OBJECT DATABASE Data Directory utilities 
*   -- ALL SERVICES.
*    
*   The following layout is implemented by this file:
*   DD_MAIN_SERVICE:
*   
* ------------------------------------------------------------
*    AUX SPACE:
* -----------------------
*       Data directory boot service code (Flare Bootloader)
* ------------------------------------------------------------
*    MANAGEMENT SPACE:
* -----------------------
*    CURRENT REV DATA DIRECTORY
*    --------------------
*       Master DDE                    (Primary Copy)
*       DD Info                       (Primary Copy)
*       User DB DDE Array             (Primary Copy)
*       External Use DB DDE Array     (Primary Copy)
*       Primary Reserved Space
*       -----------------               1/2 of total allocated space
*       Master DDE                    (Secondary Copy)
*       DD Info                       (Secondary Copy)
*       User DB DDE Array             (Secondary Copy)
*       External Use DB DDE Array     (Secondary Copy)
*       Secondary Reserved Space
* ------------------------------------------------------------
*    PREVIOUS REVISON DD MANAGEMNT SPACE:
* -----------------------
*       Master DDE                    (Primary Copy)
*       ...
*       -----------------               1/2 of total allocated space
*       Master DDE                    (Secondary Copy)
*       ...
* ------------------------------------------------------------
*    USER DB SPACE
* --------------------
*       USER DB Type 0                (If present -- space allocated)
*       ...
*       USER DB Type N                (If present -- space allocated)
*       USER SPACE Reserved Space     (If present -- space allocated)
*
* ------------------------------------------------------------
*    EXTERNAL USE SPACE
* --------------------
*       EXTERNAL USE Type 0           (If present)
*       ...
*       EXTERNAL USE Type M           (If present)
*       EXTERNAL USE Reserved Space
*
* ------------------------------------------------------------
*    NON-DATA DIRECTORY SPACE : PUBLIC BIND SPACE
* --------------------
*      |
*     \/
*    . . .
*     /\
*     |
* --------------------
*    END OF FRU
* ------------------------------------------------------------
*   
* FUNCTIONS:
*
*   dd_WhatIsDdAuxStartAddr()
*   dd_WhatIsDdAuxSpace()
*   dd_WhatIsDdAuxSetStartAddr()
*   dd_WhatIsDdAuxSetSpace()
*
*   dd_WhatIsDataDirectoryStartAddr()
*   dd_WhatIsDataDirectorySpace()
*   dd_WhatIsDbSpace()
*   dd_WhatIsDbStartAddr()
*
*   dd_WhatIsManagementDboAddr()
*   dd_WhatIsManagementDboBpe()
*   dd_WhatIsManagementDboCurrentRev()
*   dd_WhatIsManagementDboSpace()
*
*   dd_CreateManufactureZeroMark()
*   dd_GetDefaultMasterDde               
*   dd_GetDefault_User_ExtUse_SetInfo()
*   dd_Is_User_ExtUse_SetOnEachFru()  
*   dd_Is_User_ExtUse_SetOnFru()
*   dd_WhatIsDefault_User_ExtUse_SetAddr()
*   dd_WhatIsDefault_User_ExtUse_SetRev()
*   dd_WhatIsNumUserExtUseTypes()
*
*   dd_ChooseMasterDde()
*   dd_Choose_User_ExtUse_Dde()
*   dd_CrcManagementData()
*
*   dd_DboType2Index()
*   dd_Index2DboType()
*   dd_GetNextDboType()
*   
* NOTES:
*   All functions in this file return BOOL: TRUE - error, FALSE otherwise.
*   The caller should always check for returned status.
*
* HISTORY:
*   09-Aug-00: Created.                                    Mike Zelikov (MZ)
*   09-Aug-01: Added dd_ChooseMasterDde(), dd_Choose_User_ExtUse_Dde(). (MZ)
*   07-Nov-01: Added support for Stand-Alone code.                      (MZ)
*   10-Jan-02: Added support for Data Directory Boot Service.           (MZ)
*   24-Oct-02: Added support for Klondike type enclosures  Aparna V     (AV)
*   22-Apr-08: Added DBConvert error printouts   Gary Peterson         (GSP)
*   01-May-10: The last guy to touch ODBS.                    Jim Cook (CJC)
*
**************************************************************************/


/*************************
* INCLUDE FILES
*************************/
#include "drive_types.h"
#include "dbo_crc32.h"
#include "dd_main_service.h"
#include "SharedSizes.h"
#include "ostypes.h"


/**************************
* EXPORTS 
**************************/
EXPORT BOOLEAN dd_read_layout_on_disk = FALSE;
EXPORT BOOLEAN vmSimulationMode = FALSE;
/**************************
* LOCAL_DEFINITIONS 
**************************/

/**************************
* LOCAL FUNCTION PROTOTYPES 
**************************/

/**************************
* INITIALIZATIONS 
**************************/
EXPORT DD_MS_AUX_SET_INFO dd_ms_aux_set_info[DD_NUM_AUX_TYPES] = 
{
    /* DBO_TYPE                     SIZE */
    {DD_BOOTSERVICE_TYPE,           DD_BOOTSERVICE_BLK_SIZE},
};


EXPORT DD_MS_MANAGEMENT_SET_INFO dd_ms_man_set_info[DD_NUM_MANAGEMENT_TYPES] = 
{ 
    /* DBO_TYPE                      IS_DBO     IS_SET    REV                           DBO_DATA_SIZE          NUM_ENTRIES (in MAN space) */

    /* Management types in order      */

    /* This should alway be the first one */
    {DD_MASTER_DDE_TYPE,             TRUE,      TRUE,     MASTER_DDE_CURRENT_REVISION,  MASTER_DDE_DATA_SIZE,  1},

    {DD_INFO_TYPE,                   TRUE,      FALSE,    DD_INFO_CURRENT_REVISION,     DD_INFO_DATA_SIZE,     1},
    {DD_PREV_REV_MASTER_DDE_TYPE,    FALSE,     TRUE,     DBO_INVALID_REVISION,         0,                     0},
    {DD_USER_DDE_TYPE,               TRUE,      TRUE,     DDE_CURRENT_REVISION,         DDE_DATA_SIZE,         DD_MS_NUM_USER_TYPES},
    {DD_EXTERNAL_USE_DDE_TYPE,       TRUE,      TRUE,     DDE_CURRENT_REVISION,         DDE_DATA_SIZE,         DD_MS_NUM_EXTERNAL_USE_TYPES},

    /* This should always be the last one */
    {DD_PUBLIC_BIND_TYPE,            FALSE,     TRUE,     DBO_INVALID_REVISION,         0,                     0},
};

/* 
* For User types allowed algorithm are: 3-DISK MIRROR, SINGLE DISK
* If MIN_FRU and MAX_FRU are DBO_INVALID_FRU then this set is 
* present on each FRU.
*/
EXPORT DD_MS_USER_SET_INFO dd_ms_user_set_info[DD_MS_NUM_USER_TYPES] = 
{
    /* DB TYPE                      DB ALGORITHM                     NUM ENTRIES                             BLOCKS PER ENTRY                             MIN_FRU                                MAX_FRU */

    /* User Types -- In order */
    {DD_MS_FRU_SIGNATURE_TYPE,        DD_MS_SINGLE_DISK_ALG,         DD_MS_FRU_SIGNATURE_NUM_ENTRIES,        DD_MS_FRU_SIGNATURE_BLOCKS_PER_ENTRY,        DBO_INVALID_FRU,                       DBO_INVALID_FRU,					    DD_MS_USER_REGION_1},  /* Present on each FRU */
    {DD_MS_FRU_SIGNATURE_PAD_TYPE,    DD_MS_SINGLE_DISK_ALG,         DD_MS_FRU_SIGNATURE_PAD_NUM_ENTRIES,    DD_MS_FRU_SIGNATURE_PAD_BLOCKS_PER_ENTRY,    DBO_INVALID_FRU,                       DBO_INVALID_FRU,					    DD_MS_USER_REGION_1},  /* Present on each FRU */
    {DD_MS_HW_FRU_VERIFY_TYPE,        DD_MS_SINGLE_DISK_ALG,         DD_MS_HW_FRU_VERIFY_NUM_ENTRIES,        DD_MS_HW_FRU_VERIFY_BLOCKS_PER_ENTRY,        DBO_INVALID_FRU,                       DBO_INVALID_FRU,                       DD_MS_USER_REGION_1},  /* Present on each FRU */
    {DD_MS_HW_FRU_VERIFY_PAD_TYPE,    DD_MS_SINGLE_DISK_ALG,         DD_MS_HW_FRU_VERIFY_PAD_NUM_ENTRIES,    DD_MS_HW_FRU_VERIFY_PAD_BLOCKS_PER_ENTRY,    DBO_INVALID_FRU,                       DBO_INVALID_FRU,					    DD_MS_USER_REGION_1},  /* Present on each FRU */
    {DD_MS_EXPANSION_CHKPT_TYPE,      DD_MS_SINGLE_DISK_ALG,         DD_MS_EXPANSION_CHKPT_NUM_ENTRIES,      DD_MS_EXPANSION_CHKPT_BLOCKS_PER_ENTRY,      DBO_INVALID_FRU,                       DBO_INVALID_FRU,                       DD_MS_USER_REGION_1},  /* Present on each FRU */
    {DD_MS_EXPANSION_CHKPT_PAD_TYPE,  DD_MS_SINGLE_DISK_ALG,         DD_MS_EXPANSION_CHKPT_PAD_NUM_ENTRIES,  DD_MS_EXPANSION_CHKPT_PAD_BLOCKS_PER_ENTRY,  DBO_INVALID_FRU,                       DBO_INVALID_FRU,					    DD_MS_USER_REGION_1},  /* Present on each FRU */
    {DD_MS_CLEAN_DIRTY_TYPE,          DD_MS_SINGLE_DISK_ALG,         DD_MS_CLEAN_DIRTY_NUM_ENTRIES,          DD_MS_CLEAN_DIRTY_BLOCKS_PER_ENTRY,          DBO_INVALID_FRU,                       DBO_INVALID_FRU,                       DD_MS_USER_REGION_1},  /* Present on each FRU */
    {DD_MS_PADDER_TYPE,               DD_MS_SINGLE_DISK_ALG,         DD_MS_PADDER_NUM_ENTRIES,               DD_MS_PADDER_BLOCKS_PER_ENTRY,               DBO_INVALID_FRU,						 DBO_INVALID_FRU,					    DD_MS_USER_REGION_1},  /* Present on each FRU */
    {DD_MS_EXTERNAL_DUMMY_TYPE,       DD_MS_THREE_DISK_MIRROR_ALG,   DD_MS_EXTERNAL_DUMMY_NUM_ENTRIES,       DD_MS_EXTERNAL_DUMMY_BLK_SIZE,			      DD_MS_THREE_DISK_MIRROR_ALG_MIN_FRU,	 DD_MS_THREE_DISK_MIRROR_ALG_MAX_FRU,   DD_MS_USER_REGION_1},
	{DD_MS_FLARE_PAD_TYPE,            DD_MS_THREE_DISK_MIRROR_ALG,   DD_MS_FLARE_PAD_NUM_ENTRIES,            DD_MS_FLARE_PAD_BLOCKS_PER_ENTRY,            DD_MS_THREE_DISK_MIRROR_ALG_MIN_FRU,   DD_MS_THREE_DISK_MIRROR_ALG_MAX_FRU,   DD_MS_USER_REGION_2},
	{DD_MS_SYSCONFIG_TYPE,            DD_MS_THREE_DISK_MIRROR_ALG,   DD_MS_SYSCONFIG_NUM_ENTRIES,            DD_MS_SYSCONFIG_BLOCKS_PER_ENTRY,            DD_MS_THREE_DISK_MIRROR_ALG_MIN_FRU,   DD_MS_THREE_DISK_MIRROR_ALG_MAX_FRU,   DD_MS_USER_REGION_2},
	{DD_MS_SYSCONFIG_PAD_TYPE,        DD_MS_THREE_DISK_MIRROR_ALG,   DD_MS_SYSCONFIG_PAD_NUM_ENTRIES,        DD_MS_SYSCONFIG_PAD_BLOCKS_PER_ENTRY,        DD_MS_THREE_DISK_MIRROR_ALG_MIN_FRU,   DD_MS_THREE_DISK_MIRROR_ALG_MAX_FRU,   DD_MS_USER_REGION_2},
    {DD_MS_GLUT_TYPE,                 DD_MS_THREE_DISK_MIRROR_ALG,   DD_MS_GLUT_NUM_ENTRIES,                 DD_MS_GLUT_BLOCKS_PER_ENTRY,                 DD_MS_THREE_DISK_MIRROR_ALG_MIN_FRU,   DD_MS_THREE_DISK_MIRROR_ALG_MAX_FRU,   DD_MS_USER_REGION_2},
    {DD_MS_GLUT_PAD_TYPE,             DD_MS_THREE_DISK_MIRROR_ALG,   DD_MS_GLUT_PAD_NUM_ENTRIES,             DD_MS_GLUT_PAD_BLOCKS_PER_ENTRY,             DD_MS_THREE_DISK_MIRROR_ALG_MIN_FRU,   DD_MS_THREE_DISK_MIRROR_ALG_MAX_FRU,   DD_MS_USER_REGION_2},
    {DD_MS_FRU_TYPE,                  DD_MS_THREE_DISK_MIRROR_ALG,   DD_MS_FRU_NUM_ENTRIES,                  DD_MS_FRU_BLOCKS_PER_ENTRY,                  DD_MS_THREE_DISK_MIRROR_ALG_MIN_FRU,   DD_MS_THREE_DISK_MIRROR_ALG_MAX_FRU,   DD_MS_USER_REGION_2},
    {DD_MS_FRU_PAD_TYPE,              DD_MS_THREE_DISK_MIRROR_ALG,   DD_MS_FRU_PAD_NUM_ENTRIES,              DD_MS_FRU_PAD_BLOCKS_PER_ENTRY,              DD_MS_THREE_DISK_MIRROR_ALG_MIN_FRU,   DD_MS_THREE_DISK_MIRROR_ALG_MAX_FRU,   DD_MS_USER_REGION_2},
    {DD_MS_RAID_GROUP_TYPE,           DD_MS_THREE_DISK_MIRROR_ALG,   DD_MS_RAID_GROUP_NUM_ENTRIES,           DD_MS_RAID_GROUP_BLOCKS_PER_ENTRY,           DD_MS_THREE_DISK_MIRROR_ALG_MIN_FRU,   DD_MS_THREE_DISK_MIRROR_ALG_MAX_FRU,   DD_MS_USER_REGION_2},
    {DD_MS_RAID_GROUP_PAD_TYPE,       DD_MS_THREE_DISK_MIRROR_ALG,   DD_MS_RAID_GROUP_PAD_NUM_ENTRIES,       DD_MS_RAID_GROUP_PAD_BLOCKS_PER_ENTRY,       DD_MS_THREE_DISK_MIRROR_ALG_MIN_FRU,   DD_MS_THREE_DISK_MIRROR_ALG_MAX_FRU,   DD_MS_USER_REGION_2},
    {DD_MS_VERIFY_RESULTS_TYPE,       DD_MS_THREE_DISK_MIRROR_ALG,   DD_MS_VERIFY_RESULTS_NUM_ENTRIES,       DD_MS_VERIFY_RESULTS_BLOCKS_PER_ENTRY,       DD_MS_THREE_DISK_MIRROR_ALG_MIN_FRU,   DD_MS_THREE_DISK_MIRROR_ALG_MAX_FRU,   DD_MS_USER_REGION_2},
    {DD_MS_VERIFY_RESULTS_PAD_TYPE,   DD_MS_THREE_DISK_MIRROR_ALG,   DD_MS_VERIFY_RESULTS_PAD_NUM_ENTRIES,   DD_MS_VERIFY_RESULTS_PAD_BLOCKS_PER_ENTRY,   DD_MS_THREE_DISK_MIRROR_ALG_MIN_FRU,   DD_MS_THREE_DISK_MIRROR_ALG_MAX_FRU,   DD_MS_USER_REGION_2},
	{DD_MS_ENCL_TBL_TYPE,             DD_MS_THREE_DISK_MIRROR_ALG,   DD_MS_ENCL_TBL_NUM_ENTRIES,             DD_MS_ENCL_TBL_BLOCKS_PER_ENTRY,             DD_MS_THREE_DISK_MIRROR_ALG_MIN_FRU,   DD_MS_THREE_DISK_MIRROR_ALG_MAX_FRU,   DD_MS_USER_REGION_2},
	{DD_MS_ENCL_TBL_PAD_TYPE,         DD_MS_THREE_DISK_MIRROR_ALG,   DD_MS_ENCL_TBL_PAD_NUM_ENTRIES,         DD_MS_ENCL_TBL_PAD_BLOCKS_PER_ENTRY,         DD_MS_THREE_DISK_MIRROR_ALG_MIN_FRU,   DD_MS_THREE_DISK_MIRROR_ALG_MAX_FRU,   DD_MS_USER_REGION_2},
	{DD_MS_RESERVED_EXP_TYPE,         DD_MS_THREE_DISK_MIRROR_ALG,   DD_MS_RESERVED_EXP_NUM_ENTRIES,         DD_MS_RESERVED_EXP_BLOCKS_PER_ENTRY,         DD_MS_THREE_DISK_MIRROR_ALG_MIN_FRU,   DD_MS_THREE_DISK_MIRROR_ALG_MAX_FRU,   DD_MS_USER_REGION_2},
};

EXPORT DD_MS_EXTERNAL_USE_SET_INFO_STRUCT dd_ms_ext_use_set_info =
{
    DEFAULT_EXT_USE_SET, /* The default Extternal Use Set is approptiate for Hardware or UMODE Simulation. */

    /* The default External Use set for physical hardware or UMODE simulation. */
    {
  	    /* Exact layout of the disk */
        /* DB TYPE                        SIZE IN BLOCKS                      MIN_FRU                              MAX_FRU                                 REGION_TYPE */

        /* EXTERNALLY MANAGED TYPES - in LBA order */
        {DD_MS_DISK_DIAGNOSTIC_TYPE,      DD_MS_DISK_DIAGNOSTIC_BLK_SIZE,     DBO_INVALID_FRU,                     DBO_INVALID_FRU,                        DD_MS_EXTERNAL_USE_REGION_1}, /* Present on each FRU */
        {DD_MS_DISK_DIAGNOSTIC_PAD_TYPE,  DD_MS_DISK_DIAGNOSTIC_PAD_BLK_SIZE, DBO_INVALID_FRU,                     DBO_INVALID_FRU,                        DD_MS_EXTERNAL_USE_REGION_1}, /* Present on each FRU */
        {DD_MS_DDBS_TYPE,                 DD_MS_DDBS_BLK_SIZE,                DD_MS_DDBS_MIN_FRU,                  DD_MS_DDBS_MAX_FRU,                     DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_DDBS_LUN_OBJ_TYPE,         DD_MS_DDBS_LUN_OBJ_BLK_SIZE,        DD_MS_DDBS_LUN_OBJ_MIN_FRU,          DD_MS_DDBS_LUN_OBJ_MAX_FRU,             DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_BIOS_UCODE_TYPE,           DD_MS_BIOS_UCODE_BLK_SIZE,          DD_MS_BIOS_UCODE_MIN_FRU,            DD_MS_BIOS_UCODE_MAX_FRU,               DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_BIOS_LUN_OBJ_TYPE,         DD_MS_BIOS_LUN_OBJ_BLK_SIZE,        DD_MS_BIOS_LUN_OBJ_MIN_FRU,          DD_MS_BIOS_LUN_OBJ_MAX_FRU,             DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_PROM_UCODE_TYPE,           DD_MS_PROM_UCODE_BLK_SIZE,          DD_MS_PROM_UCODE_MIN_FRU,            DD_MS_PROM_UCODE_MAX_FRU,               DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_PROM_LUN_OBJ_TYPE,         DD_MS_PROM_LUN_OBJ_BLK_SIZE,        DD_MS_PROM_LUN_OBJ_MIN_FRU,          DD_MS_PROM_LUN_OBJ_MAX_FRU,             DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_GPS_FW_TYPE,               DD_MS_GPS_FW_BLK_SIZE,              DD_MS_GPS_FW_MIN_FRU,                DD_MS_GPS_FW_MAX_FRU,                   DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_GPS_FW_LUN_OBJ_TYPE,       DD_MS_GPS_FW_LUN_OBJ_BLK_SIZE,      DD_MS_GPS_FW_LUN_OBJ_MIN_FRU,        DD_MS_GPS_FW_LUN_OBJ_MAX_FRU,           DD_MS_EXTERNAL_USE_REGION_1},

        /* The following defines the area occupied by the Flare "3 disk" database in the middle of this "external use" space layout. */
        {DD_MS_EXP_USER_DUMMY_TYPE,       DD_MS_FLARE_LEGACY_DB_BLK_SIZE,     DD_MS_EXP_USER_DUMMY_MIN_FRU,        DD_MS_EXP_USER_DUMMY_MAX_FRU,           DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_FLARE_DB_TYPE,             DD_MS_FLARE_DB_BLK_SIZE,            DD_MS_FLARE_DB_MIN_FRU,              DD_MS_FLARE_DB_MAX_FRU,                 DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_FLARE_DB_LUN_OBJ_TYPE,     DD_MS_FLARE_DB_LUN_OBJ_BLK_SIZE,    DD_MS_FLARE_DB_LUN_OBJ_MIN_FRU,      DD_MS_FLARE_DB_LUN_OBJ_MAX_FRU,         DD_MS_EXTERNAL_USE_REGION_1},

        {DD_MS_ESP_LUN_TYPE,              DD_MS_ESP_LUN_BLK_SIZE,             DD_MS_ESP_LUN_MIN_FRU,               DD_MS_ESP_LUN_MAX_FRU,                  DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_ESP_LUN_OBJ_TYPE,          DD_MS_ESP_LUN_OBJ_BLK_SIZE,         DD_MS_ESP_LUN_OBJ_MIN_FRU,           DD_MS_ESP_LUN_OBJ_MAX_FRU,              DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_VCX_LUN_0_TYPE,            DD_MS_VCX_LUN_0_BLK_SIZE,           DD_MS_VCX_LUN_0_MIN_FRU,             DD_MS_VCX_LUN_0_MAX_FRU,                DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_VCX_LUN_0_OBJ_TYPE,        DD_MS_VCX_LUN_0_OBJ_BLK_SIZE,       DD_MS_VCX_LUN_0_OBJ_MIN_FRU,         DD_MS_VCX_LUN_0_OBJ_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_VCX_LUN_1_TYPE,            DD_MS_VCX_LUN_1_BLK_SIZE,           DD_MS_VCX_LUN_1_MIN_FRU,             DD_MS_VCX_LUN_1_MAX_FRU,                DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_VCX_LUN_1_OBJ_TYPE,        DD_MS_VCX_LUN_1_OBJ_BLK_SIZE,       DD_MS_VCX_LUN_1_OBJ_MIN_FRU,         DD_MS_VCX_LUN_1_OBJ_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_VCX_LUN_4_TYPE,            DD_MS_VCX_LUN_4_BLK_SIZE,           DD_MS_VCX_LUN_4_MIN_FRU,             DD_MS_VCX_LUN_4_MAX_FRU,                DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_VCX_LUN_4_OBJ_TYPE,        DD_MS_VCX_LUN_4_OBJ_BLK_SIZE,       DD_MS_VCX_LUN_4_OBJ_MIN_FRU,         DD_MS_VCX_LUN_4_OBJ_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_CENTERA_LUN_TYPE,          DD_MS_CENTERA_LUN_BLK_SIZE,         DD_MS_CENTERA_LUN_MIN_FRU,           DD_MS_CENTERA_LUN_MAX_FRU,              DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_CENTERA_LUN_OBJ_TYPE,      DD_MS_CENTERA_LUN_OBJ_BLK_SIZE,     DD_MS_CENTERA_LUN_OBJ_MIN_FRU,       DD_MS_CENTERA_LUN_OBJ_MAX_FRU,          DD_MS_EXTERNAL_USE_REGION_1},

        {DD_MS_PSM_RESERVED_TYPE,         DD_MS_PSM_RESERVED_BLK_SIZE,        DD_MS_PSM_RESERVED_MIN_FRU,          DD_MS_PSM_RESERVED_MAX_FRU,             DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_PSM_LUN_OBJ_TYPE,          DD_MS_PSM_LUN_OBJ_BLK_SIZE,         DD_MS_PSM_LUN_OBJ_MIN_FRU,           DD_MS_PSM_LUN_OBJ_MAX_FRU,              DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_DRAM_CACHE_LUN_TYPE,       DD_MS_DRAM_CACHE_LUN_BLK_SIZE,      DD_MS_DRAM_CACHE_LUN_MIN_FRU,        DD_MS_DRAM_CACHE_LUN_MAX_FRU,           DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_DRAM_CACHE_LUN_OBJ_TYPE,   DD_MS_DRAM_CACHE_LUN_OBJ_BLK_SIZE,  DD_MS_DRAM_CACHE_LUN_OBJ_MIN_FRU,    DD_MS_DRAM_CACHE_LUN_OBJ_MAX_FRU,       DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_EFD_CACHE_LUN_TYPE,        DD_MS_EFD_CACHE_LUN_BLK_SIZE,       DD_MS_EFD_CACHE_LUN_MIN_FRU,         DD_MS_EFD_CACHE_LUN_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_EFD_CACHE_LUN_OBJ_TYPE,    DD_MS_EFD_CACHE_LUN_OBJ_BLK_SIZE,   DD_MS_EFD_CACHE_LUN_OBJ_MIN_FRU,     DD_MS_EFD_CACHE_LUN_OBJ_MAX_FRU,        DD_MS_EXTERNAL_USE_REGION_1},

        {DD_MS_FUTURE_EXP_TYPE,           DD_MS_FUTURE_EXP_BLK_SIZE,          DD_MS_FUTURE_EXP_MIN_FRU,            DD_MS_FUTURE_EXP_MAX_FRU,               DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_PSM_RAID_GROUP_OBJ_TYPE,   DD_MS_PSM_RAID_GROUP_OBJ_BLK_SIZE,  DD_MS_PSM_RAID_GROUP_OBJ_MIN_FRU,    DD_MS_PSM_RAID_GROUP_OBJ_MAX_FRU,       DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_ALIGN_BUFFER1_TYPE,        DD_MS_ALIGN_BUFFER1_BLK_SIZE,       DD_MS_ALIGN_BUFFER1_MIN_FRU,         DD_MS_ALIGN_BUFFER1_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_1},

        /* Dummy area for third disk so the following areas all start at the same address */
	    {DD_MS_EXT_NEW_DB_DUMMY_TYPE,	  DD_MS_EXT_NEW_DB_DUMMY_BLK_SIZE,	  DD_MS_EXT_NEW_DB_DUMMY_MIN_FRU,	   DD_MS_EXT_NEW_DB_DUMMY_MAX_FRU,         DD_MS_EXTERNAL_USE_REGION_2},

        {DD_MS_WIL_A_LUN_TYPE,            DD_MS_WIL_A_LUN_BLK_SIZE,           DD_MS_WIL_A_LUN_MIN_FRU,             DD_MS_WIL_A_LUN_MAX_FRU,                DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_WIL_A_LUN_OBJ_TYPE,        DD_MS_WIL_A_LUN_OBJ_BLK_SIZE,       DD_MS_WIL_A_LUN_OBJ_MIN_FRU,         DD_MS_WIL_A_LUN_OBJ_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_WIL_B_LUN_TYPE,            DD_MS_WIL_B_LUN_BLK_SIZE,           DD_MS_WIL_B_LUN_MIN_FRU,             DD_MS_WIL_B_LUN_MAX_FRU,                DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_WIL_B_LUN_OBJ_TYPE,        DD_MS_WIL_B_LUN_OBJ_BLK_SIZE,       DD_MS_WIL_B_LUN_OBJ_MIN_FRU,         DD_MS_WIL_B_LUN_OBJ_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_CPL_A_LUN_TYPE,            DD_MS_CPL_A_LUN_BLK_SIZE,           DD_MS_CPL_A_LUN_MIN_FRU,             DD_MS_CPL_A_LUN_MAX_FRU,                DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_CPL_A_LUN_OBJ_TYPE,        DD_MS_CPL_A_LUN_OBJ_BLK_SIZE,       DD_MS_CPL_A_LUN_OBJ_MIN_FRU,         DD_MS_CPL_A_LUN_OBJ_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_CPL_B_LUN_TYPE,            DD_MS_CPL_B_LUN_BLK_SIZE,           DD_MS_CPL_B_LUN_MIN_FRU,             DD_MS_CPL_B_LUN_MAX_FRU,                DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_CPL_B_LUN_OBJ_TYPE,        DD_MS_CPL_B_LUN_OBJ_BLK_SIZE,       DD_MS_CPL_B_LUN_OBJ_MIN_FRU,         DD_MS_CPL_B_LUN_OBJ_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_2},

        {DD_MS_VCX_LUN_2_TYPE,            DD_MS_VCX_LUN_2_BLK_SIZE,           DD_MS_VCX_LUN_2_MIN_FRU,             DD_MS_VCX_LUN_2_MAX_FRU,                DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_VCX_LUN_2_OBJ_TYPE,        DD_MS_VCX_LUN_2_OBJ_BLK_SIZE,       DD_MS_VCX_LUN_2_OBJ_MIN_FRU,         DD_MS_VCX_LUN_2_OBJ_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_VCX_LUN_3_TYPE,            DD_MS_VCX_LUN_3_BLK_SIZE,           DD_MS_VCX_LUN_3_MIN_FRU,             DD_MS_VCX_LUN_3_MAX_FRU,                DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_VCX_LUN_3_OBJ_TYPE,        DD_MS_VCX_LUN_3_OBJ_BLK_SIZE,       DD_MS_VCX_LUN_3_OBJ_MIN_FRU,         DD_MS_VCX_LUN_3_OBJ_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_VCX_LUN_5_TYPE,            DD_MS_VCX_LUN_5_BLK_SIZE,           DD_MS_VCX_LUN_5_MIN_FRU,             DD_MS_VCX_LUN_5_MAX_FRU,                DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_VCX_LUN_5_OBJ_TYPE,        DD_MS_VCX_LUN_5_OBJ_BLK_SIZE,       DD_MS_VCX_LUN_5_OBJ_MIN_FRU,         DD_MS_VCX_LUN_5_OBJ_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_2},

        {DD_MS_RAID_METADATA_LUN_TYPE,    DD_MS_RAID_METADATA_LUN_BLK_SIZE,   DD_MS_RAID_METADATA_LUN_MIN_FRU,     DD_MS_RAID_METADATA_LUN_MAX_FRU,        DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_RAID_METADATA_LUN_OBJ_TYPE, DD_MS_RAID_METADATA_LUN_OBJ_BLK_SIZE, DD_MS_RAID_METADATA_LUN_OBJ_MIN_FRU, DD_MS_RAID_METADATA_LUN_OBJ_MAX_FRU, DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_RAID_METASTATS_LUN_TYPE,   DD_MS_RAID_METASTATS_LUN_BLK_SIZE,  DD_MS_RAID_METASTATS_LUN_MIN_FRU,    DD_MS_RAID_METASTATS_LUN_MAX_FRU,       DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_RAID_METASTATS_LUN_OBJ_TYPE, DD_MS_RAID_METASTATS_LUN_OBJ_BLK_SIZE, DD_MS_RAID_METASTATS_LUN_OBJ_MIN_FRU, DD_MS_RAID_METASTATS_LUN_OBJ_MAX_FRU, DD_MS_EXTERNAL_USE_REGION_2},

        {DD_MS_VAULT_RESERVED_TYPE,       DD_MS_VAULT_RESERVED_BLK_SIZE,      DD_MS_VAULT_RESERVED_MIN_FRU,        DD_MS_VAULT_RESERVED_MAX_FRU,           DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_VAULT_LUN_OBJECT_TYPE,     DD_MS_VAULT_LUN_OBJECT_BLK_SIZE,    DD_MS_VAULT_LUN_OBJECT_MIN_FRU,      DD_MS_VAULT_LUN_OBJECT_MAX_FRU,         DD_MS_EXTERNAL_USE_REGION_2},

        {DD_MS_VAULT_FUTURE_EXP_TYPE,     DD_MS_VAULT_FUTURE_EXP_BLK_SIZE,    DD_MS_VAULT_FUTURE_EXP_MIN_FRU,      DD_MS_VAULT_FUTURE_EXP_MAX_FRU,         DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_VAULT_RAID_GROUP_OBJ_TYPE, DD_MS_VAULT_RAID_GROUP_OBJ_BLK_SIZE, DD_MS_VAULT_RAID_GROUP_OBJ_MIN_FRU, DD_MS_VAULT_RAID_GROUP_OBJ_MAX_FRU,     DD_MS_EXTERNAL_USE_REGION_2},

	    {DD_MS_NT_BOOT_SPA_PRIM_TYPE,     DD_MS_NT_BOOT_SPA_PRIM_BLK_SIZE,    DD_MS_NT_BOOT_SPA_PRIM_MIN_FRU,      DD_MS_NT_BOOT_SPA_PRIM_MAX_FRU,         DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_NT_BOOT_SPA_SEC_TYPE,      DD_MS_NT_BOOT_SPA_SEC_BLK_SIZE,     DD_MS_NT_BOOT_SPA_SEC_MIN_FRU,       DD_MS_NT_BOOT_SPA_SEC_MAX_FRU,          DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_NT_BOOT_SPB_PRIM_TYPE,     DD_MS_NT_BOOT_SPB_PRIM_BLK_SIZE,    DD_MS_NT_BOOT_SPB_PRIM_MIN_FRU,      DD_MS_NT_BOOT_SPB_PRIM_MAX_FRU,         DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_NT_BOOT_SPB_SEC_TYPE,      DD_MS_NT_BOOT_SPB_SEC_BLK_SIZE,     DD_MS_NT_BOOT_SPB_SEC_MIN_FRU,       DD_MS_NT_BOOT_SPB_SEC_MAX_FRU,          DD_MS_EXTERNAL_USE_REGION_2},
	    {DD_MS_NT_BOOT_SPA_PRIM_EXP_TYPE, DD_MS_NT_BOOT_SPA_PRIM_EXP_BLK_SIZE,DD_MS_NT_BOOT_SPA_PRIM_EXP_MIN_FRU,  DD_MS_NT_BOOT_SPA_PRIM_EXP_MAX_FRU,     DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_NT_BOOT_SPA_SEC_EXP_TYPE,  DD_MS_NT_BOOT_SPA_SEC_EXP_BLK_SIZE, DD_MS_NT_BOOT_SPA_SEC_EXP_MIN_FRU,   DD_MS_NT_BOOT_SPA_SEC_EXP_MAX_FRU,      DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_NT_BOOT_SPB_PRIM_EXP_TYPE, DD_MS_NT_BOOT_SPB_PRIM_EXP_BLK_SIZE,DD_MS_NT_BOOT_SPB_PRIM_EXP_MIN_FRU,  DD_MS_NT_BOOT_SPB_PRIM_EXP_MAX_FRU,     DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_NT_BOOT_SPB_SEC_EXP_TYPE,  DD_MS_NT_BOOT_SPB_SEC_EXP_BLK_SIZE, DD_MS_NT_BOOT_SPB_SEC_EXP_MIN_FRU,   DD_MS_NT_BOOT_SPB_SEC_EXP_MAX_FRU,      DD_MS_EXTERNAL_USE_REGION_2},

	    {DD_MS_UTIL_SPA_PRIM_TYPE,        DD_MS_UTIL_SPA_PRIM_BLK_SIZE,       DD_MS_UTIL_SPA_PRIM_MIN_FRU,         DD_MS_UTIL_SPA_PRIM_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_UTIL_SPA_SEC_TYPE,         DD_MS_UTIL_SPA_SEC_BLK_SIZE,        DD_MS_UTIL_SPA_SEC_MIN_FRU,          DD_MS_UTIL_SPA_SEC_MAX_FRU,             DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_UTIL_SPB_PRIM_TYPE,        DD_MS_UTIL_SPB_PRIM_BLK_SIZE,       DD_MS_UTIL_SPB_PRIM_MIN_FRU,         DD_MS_UTIL_SPB_PRIM_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_UTIL_SPB_SEC_TYPE,         DD_MS_UTIL_SPB_SEC_BLK_SIZE,        DD_MS_UTIL_SPB_SEC_MIN_FRU,          DD_MS_UTIL_SPB_SEC_MAX_FRU,             DD_MS_EXTERNAL_USE_REGION_2},
	    {DD_MS_UTIL_SPA_PRIM_EXP_TYPE,    DD_MS_UTIL_SPA_PRIM_EXP_BLK_SIZE,   DD_MS_UTIL_SPA_PRIM_EXP_MIN_FRU,     DD_MS_UTIL_SPA_PRIM_EXP_MAX_FRU,        DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_UTIL_SPA_SEC_EXP_TYPE,     DD_MS_UTIL_SPA_SEC_EXP_BLK_SIZE,    DD_MS_UTIL_SPA_SEC_EXP_MIN_FRU,      DD_MS_UTIL_SPA_SEC_EXP_MAX_FRU,         DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_UTIL_SPB_PRIM_EXP_TYPE,    DD_MS_UTIL_SPB_PRIM_EXP_BLK_SIZE,   DD_MS_UTIL_SPB_PRIM_EXP_MIN_FRU,     DD_MS_UTIL_SPB_PRIM_EXP_MAX_FRU,        DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_UTIL_SPB_SEC_EXP_TYPE,     DD_MS_UTIL_SPB_SEC_EXP_BLK_SIZE,    DD_MS_UTIL_SPB_SEC_EXP_MIN_FRU,      DD_MS_UTIL_SPB_SEC_EXP_MAX_FRU,         DD_MS_EXTERNAL_USE_REGION_2},

	    {DD_MS_ICA_IMAGE_REP_TYPE,		  DD_MS_ICA_IMAGE_REP_BLK_SIZE,		  DD_MS_ICA_IMAGE_REP_MIN_FRU,		   DD_MS_ICA_IMAGE_REP_MAX_FRU,			   DD_MS_EXTERNAL_USE_REGION_2},
	    {DD_MS_ICA_IMAGE_REP_EXP_TYPE,	  DD_MS_ICA_IMAGE_REP_EXP_BLK_SIZE,	  DD_MS_ICA_IMAGE_REP_EXP_MIN_FRU,     DD_MS_ICA_IMAGE_REP_EXP_MAX_FRU,		   DD_MS_EXTERNAL_USE_REGION_2},

        /* Dummy area for third disk so the following areas all start at the same address */
        {DD_MS_UTIL_FLARE_DUMMY_TYPE,     DD_MS_UTIL_FLARE_DUMMY_BLK_SIZE,    DD_MS_UTIL_FLARE_DUMMY_MIN_FRU,      DD_MS_UTIL_FLARE_DUMMY_MAX_FRU,         DD_MS_EXTERNAL_USE_REGION_2},

        {DD_MS_FUTURE_GROWTH_TYPE,        DD_MS_FUTURE_GROWTH_BLK_SIZE,       DD_MS_FUTURE_GROWTH_MIN_FRU,         DD_MS_FUTURE_GROWTH_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_NON_LUN_RAID_GROUP_OBJ_TYPE, DD_MS_NON_LUN_RAID_GROUP_OBJ_BLK_SIZE, DD_MS_NON_LUN_RAID_GROUP_OBJ_MIN_FRU, DD_MS_NON_LUN_RAID_GROUP_OBJ_MAX_FRU, DD_MS_EXTERNAL_USE_REGION_2}
    },

    /* External Use DB set for VM Simulation. */     
    {
  	    /* Exact layout of the disk */
        /* DB TYPE                        SIZE IN BLOCKS                      MIN_FRU                              MAX_FRU                                 REGION_TYPE */

        /* EXTERNALLY MANAGED TYPES - in LBA order */
        {DD_MS_DISK_DIAGNOSTIC_TYPE,      DD_MS_VSIM_DISK_DIAGNOSTIC_BLK_SIZE,     DBO_INVALID_FRU,                     DBO_INVALID_FRU,                        DD_MS_EXTERNAL_USE_REGION_1}, /* Present on each FRU */
        {DD_MS_DISK_DIAGNOSTIC_PAD_TYPE,  DD_MS_VSIM_DISK_DIAGNOSTIC_PAD_BLK_SIZE, DBO_INVALID_FRU,                     DBO_INVALID_FRU,                        DD_MS_EXTERNAL_USE_REGION_1}, /* Present on each FRU */
        {DD_MS_DDBS_TYPE,                 DD_MS_VSIM_DDBS_BLK_SIZE,                DD_MS_DDBS_MIN_FRU,                  DD_MS_DDBS_MAX_FRU,                     DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_DDBS_LUN_OBJ_TYPE,         DD_MS_VSIM_DDBS_LUN_OBJ_BLK_SIZE,        DD_MS_DDBS_LUN_OBJ_MIN_FRU,          DD_MS_DDBS_LUN_OBJ_MAX_FRU,             DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_BIOS_UCODE_TYPE,           DD_MS_VSIM_BIOS_UCODE_BLK_SIZE,          DD_MS_BIOS_UCODE_MIN_FRU,            DD_MS_BIOS_UCODE_MAX_FRU,               DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_BIOS_LUN_OBJ_TYPE,         DD_MS_VSIM_BIOS_LUN_OBJ_BLK_SIZE,        DD_MS_BIOS_LUN_OBJ_MIN_FRU,          DD_MS_BIOS_LUN_OBJ_MAX_FRU,             DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_PROM_UCODE_TYPE,           DD_MS_VSIM_PROM_UCODE_BLK_SIZE,          DD_MS_PROM_UCODE_MIN_FRU,            DD_MS_PROM_UCODE_MAX_FRU,               DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_PROM_LUN_OBJ_TYPE,         DD_MS_VSIM_PROM_LUN_OBJ_BLK_SIZE,        DD_MS_PROM_LUN_OBJ_MIN_FRU,          DD_MS_PROM_LUN_OBJ_MAX_FRU,             DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_GPS_FW_TYPE,               DD_MS_VSIM_GPS_FW_BLK_SIZE,              DD_MS_GPS_FW_MIN_FRU,                DD_MS_GPS_FW_MAX_FRU,                   DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_GPS_FW_LUN_OBJ_TYPE,       DD_MS_VSIM_GPS_FW_LUN_OBJ_BLK_SIZE,      DD_MS_GPS_FW_LUN_OBJ_MIN_FRU,        DD_MS_GPS_FW_LUN_OBJ_MAX_FRU,           DD_MS_EXTERNAL_USE_REGION_1},

        /* The following defines the area occupied by the Flare "3 disk" database in the middle of this "external use" space layout. */
        {DD_MS_EXP_USER_DUMMY_TYPE,       DD_MS_VSIM_FLARE_LEGACY_DB_BLK_SIZE,     DD_MS_EXP_USER_DUMMY_MIN_FRU,        DD_MS_EXP_USER_DUMMY_MAX_FRU,           DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_FLARE_DB_TYPE,             DD_MS_VSIM_FLARE_DB_BLK_SIZE,            DD_MS_FLARE_DB_MIN_FRU,              DD_MS_FLARE_DB_MAX_FRU,                 DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_FLARE_DB_LUN_OBJ_TYPE,     DD_MS_VSIM_FLARE_DB_LUN_OBJ_BLK_SIZE,    DD_MS_FLARE_DB_LUN_OBJ_MIN_FRU,      DD_MS_FLARE_DB_LUN_OBJ_MAX_FRU,         DD_MS_EXTERNAL_USE_REGION_1},

        {DD_MS_ESP_LUN_TYPE,              DD_MS_VSIM_ESP_LUN_BLK_SIZE,             DD_MS_ESP_LUN_MIN_FRU,               DD_MS_ESP_LUN_MAX_FRU,                  DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_ESP_LUN_OBJ_TYPE,          DD_MS_VSIM_ESP_LUN_OBJ_BLK_SIZE,         DD_MS_ESP_LUN_OBJ_MIN_FRU,           DD_MS_ESP_LUN_OBJ_MAX_FRU,              DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_VCX_LUN_0_TYPE,            DD_MS_VSIM_VCX_LUN_0_BLK_SIZE,           DD_MS_VCX_LUN_0_MIN_FRU,             DD_MS_VCX_LUN_0_MAX_FRU,                DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_VCX_LUN_0_OBJ_TYPE,        DD_MS_VSIM_VCX_LUN_0_OBJ_BLK_SIZE,       DD_MS_VCX_LUN_0_OBJ_MIN_FRU,         DD_MS_VCX_LUN_0_OBJ_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_VCX_LUN_1_TYPE,            DD_MS_VSIM_VCX_LUN_1_BLK_SIZE,           DD_MS_VCX_LUN_1_MIN_FRU,             DD_MS_VCX_LUN_1_MAX_FRU,                DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_VCX_LUN_1_OBJ_TYPE,        DD_MS_VSIM_VCX_LUN_1_OBJ_BLK_SIZE,       DD_MS_VCX_LUN_1_OBJ_MIN_FRU,         DD_MS_VCX_LUN_1_OBJ_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_VCX_LUN_4_TYPE,            DD_MS_VSIM_VCX_LUN_4_BLK_SIZE,           DD_MS_VCX_LUN_4_MIN_FRU,             DD_MS_VCX_LUN_4_MAX_FRU,                DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_VCX_LUN_4_OBJ_TYPE,        DD_MS_VSIM_VCX_LUN_4_OBJ_BLK_SIZE,       DD_MS_VCX_LUN_4_OBJ_MIN_FRU,         DD_MS_VCX_LUN_4_OBJ_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_CENTERA_LUN_TYPE,          DD_MS_VSIM_CENTERA_LUN_BLK_SIZE,         DD_MS_CENTERA_LUN_MIN_FRU,           DD_MS_CENTERA_LUN_MAX_FRU,              DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_CENTERA_LUN_OBJ_TYPE,      DD_MS_VSIM_CENTERA_LUN_OBJ_BLK_SIZE,     DD_MS_CENTERA_LUN_OBJ_MIN_FRU,       DD_MS_CENTERA_LUN_OBJ_MAX_FRU,          DD_MS_EXTERNAL_USE_REGION_1},

        {DD_MS_PSM_RESERVED_TYPE,         DD_MS_VSIM_PSM_RESERVED_BLK_SIZE,        DD_MS_PSM_RESERVED_MIN_FRU,          DD_MS_PSM_RESERVED_MAX_FRU,             DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_PSM_LUN_OBJ_TYPE,          DD_MS_VSIM_PSM_LUN_OBJ_BLK_SIZE,         DD_MS_PSM_LUN_OBJ_MIN_FRU,           DD_MS_PSM_LUN_OBJ_MAX_FRU,              DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_DRAM_CACHE_LUN_TYPE,       DD_MS_VSIM_DRAM_CACHE_LUN_BLK_SIZE,      DD_MS_DRAM_CACHE_LUN_MIN_FRU,        DD_MS_DRAM_CACHE_LUN_MAX_FRU,           DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_DRAM_CACHE_LUN_OBJ_TYPE,   DD_MS_VSIM_DRAM_CACHE_LUN_OBJ_BLK_SIZE,  DD_MS_DRAM_CACHE_LUN_OBJ_MIN_FRU,    DD_MS_DRAM_CACHE_LUN_OBJ_MAX_FRU,       DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_EFD_CACHE_LUN_TYPE,        DD_MS_VSIM_EFD_CACHE_LUN_BLK_SIZE,       DD_MS_EFD_CACHE_LUN_MIN_FRU,         DD_MS_EFD_CACHE_LUN_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_EFD_CACHE_LUN_OBJ_TYPE,    DD_MS_VSIM_EFD_CACHE_LUN_OBJ_BLK_SIZE,   DD_MS_EFD_CACHE_LUN_OBJ_MIN_FRU,     DD_MS_EFD_CACHE_LUN_OBJ_MAX_FRU,        DD_MS_EXTERNAL_USE_REGION_1},

        {DD_MS_FUTURE_EXP_TYPE,           DD_MS_VSIM_FUTURE_EXP_BLK_SIZE,          DD_MS_FUTURE_EXP_MIN_FRU,            DD_MS_FUTURE_EXP_MAX_FRU,               DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_PSM_RAID_GROUP_OBJ_TYPE,   DD_MS_VSIM_PSM_RAID_GROUP_OBJ_BLK_SIZE,  DD_MS_PSM_RAID_GROUP_OBJ_MIN_FRU,    DD_MS_PSM_RAID_GROUP_OBJ_MAX_FRU,       DD_MS_EXTERNAL_USE_REGION_1},
        {DD_MS_ALIGN_BUFFER1_TYPE,        DD_MS_VSIM_ALIGN_BUFFER1_BLK_SIZE,       DD_MS_ALIGN_BUFFER1_MIN_FRU,         DD_MS_ALIGN_BUFFER1_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_1},

        /* Dummy area for third disk so the following areas all start at the same address */
	    {DD_MS_EXT_NEW_DB_DUMMY_TYPE,	  DD_MS_VSIM_EXT_NEW_DB_DUMMY_BLK_SIZE,	  DD_MS_EXT_NEW_DB_DUMMY_MIN_FRU,	   DD_MS_EXT_NEW_DB_DUMMY_MAX_FRU,         DD_MS_EXTERNAL_USE_REGION_2},

        {DD_MS_WIL_A_LUN_TYPE,            DD_MS_VSIM_WIL_A_LUN_BLK_SIZE,           DD_MS_WIL_A_LUN_MIN_FRU,             DD_MS_WIL_A_LUN_MAX_FRU,                DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_WIL_A_LUN_OBJ_TYPE,        DD_MS_VSIM_WIL_A_LUN_OBJ_BLK_SIZE,       DD_MS_WIL_A_LUN_OBJ_MIN_FRU,         DD_MS_WIL_A_LUN_OBJ_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_WIL_B_LUN_TYPE,            DD_MS_VSIM_WIL_B_LUN_BLK_SIZE,           DD_MS_WIL_B_LUN_MIN_FRU,             DD_MS_WIL_B_LUN_MAX_FRU,                DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_WIL_B_LUN_OBJ_TYPE,        DD_MS_VSIM_WIL_B_LUN_OBJ_BLK_SIZE,       DD_MS_WIL_B_LUN_OBJ_MIN_FRU,         DD_MS_WIL_B_LUN_OBJ_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_CPL_A_LUN_TYPE,            DD_MS_VSIM_CPL_A_LUN_BLK_SIZE,           DD_MS_CPL_A_LUN_MIN_FRU,             DD_MS_CPL_A_LUN_MAX_FRU,                DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_CPL_A_LUN_OBJ_TYPE,        DD_MS_VSIM_CPL_A_LUN_OBJ_BLK_SIZE,       DD_MS_CPL_A_LUN_OBJ_MIN_FRU,         DD_MS_CPL_A_LUN_OBJ_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_CPL_B_LUN_TYPE,            DD_MS_VSIM_CPL_B_LUN_BLK_SIZE,           DD_MS_CPL_B_LUN_MIN_FRU,             DD_MS_CPL_B_LUN_MAX_FRU,                DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_CPL_B_LUN_OBJ_TYPE,        DD_MS_VSIM_CPL_B_LUN_OBJ_BLK_SIZE,       DD_MS_CPL_B_LUN_OBJ_MIN_FRU,         DD_MS_CPL_B_LUN_OBJ_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_2},

        {DD_MS_VCX_LUN_2_TYPE,            DD_MS_VSIM_VCX_LUN_2_BLK_SIZE,           DD_MS_VCX_LUN_2_MIN_FRU,             DD_MS_VCX_LUN_2_MAX_FRU,                DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_VCX_LUN_2_OBJ_TYPE,        DD_MS_VSIM_VCX_LUN_2_OBJ_BLK_SIZE,       DD_MS_VCX_LUN_2_OBJ_MIN_FRU,         DD_MS_VCX_LUN_2_OBJ_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_VCX_LUN_3_TYPE,            DD_MS_VSIM_VCX_LUN_3_BLK_SIZE,           DD_MS_VCX_LUN_3_MIN_FRU,             DD_MS_VCX_LUN_3_MAX_FRU,                DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_VCX_LUN_3_OBJ_TYPE,        DD_MS_VSIM_VCX_LUN_3_OBJ_BLK_SIZE,       DD_MS_VCX_LUN_3_OBJ_MIN_FRU,         DD_MS_VCX_LUN_3_OBJ_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_VCX_LUN_5_TYPE,            DD_MS_VSIM_VCX_LUN_5_BLK_SIZE,           DD_MS_VCX_LUN_5_MIN_FRU,             DD_MS_VCX_LUN_5_MAX_FRU,                DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_VCX_LUN_5_OBJ_TYPE,        DD_MS_VSIM_VCX_LUN_5_OBJ_BLK_SIZE,       DD_MS_VCX_LUN_5_OBJ_MIN_FRU,         DD_MS_VCX_LUN_5_OBJ_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_2},

        {DD_MS_RAID_METADATA_LUN_TYPE,    DD_MS_VSIM_RAID_METADATA_LUN_BLK_SIZE,   DD_MS_RAID_METADATA_LUN_MIN_FRU,     DD_MS_RAID_METADATA_LUN_MAX_FRU,        DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_RAID_METADATA_LUN_OBJ_TYPE, DD_MS_VSIM_RAID_METADATA_LUN_OBJ_BLK_SIZE, DD_MS_RAID_METADATA_LUN_OBJ_MIN_FRU, DD_MS_RAID_METADATA_LUN_OBJ_MAX_FRU, DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_RAID_METASTATS_LUN_TYPE,   DD_MS_VSIM_RAID_METASTATS_LUN_BLK_SIZE,  DD_MS_RAID_METASTATS_LUN_MIN_FRU,    DD_MS_RAID_METASTATS_LUN_MAX_FRU,       DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_RAID_METASTATS_LUN_OBJ_TYPE, DD_MS_VSIM_RAID_METASTATS_LUN_OBJ_BLK_SIZE, DD_MS_RAID_METASTATS_LUN_OBJ_MIN_FRU, DD_MS_RAID_METASTATS_LUN_OBJ_MAX_FRU, DD_MS_EXTERNAL_USE_REGION_2},

        {DD_MS_VAULT_RESERVED_TYPE,       DD_MS_VSIM_VAULT_RESERVED_BLK_SIZE,      DD_MS_VAULT_RESERVED_MIN_FRU,        DD_MS_VAULT_RESERVED_MAX_FRU,           DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_VAULT_LUN_OBJECT_TYPE,     DD_MS_VSIM_VAULT_LUN_OBJECT_BLK_SIZE,    DD_MS_VAULT_LUN_OBJECT_MIN_FRU,      DD_MS_VAULT_LUN_OBJECT_MAX_FRU,         DD_MS_EXTERNAL_USE_REGION_2},

        {DD_MS_VAULT_FUTURE_EXP_TYPE,     DD_MS_VSIM_VAULT_FUTURE_EXP_BLK_SIZE,    DD_MS_VAULT_FUTURE_EXP_MIN_FRU,      DD_MS_VAULT_FUTURE_EXP_MAX_FRU,         DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_VAULT_RAID_GROUP_OBJ_TYPE, DD_MS_VSIM_VAULT_RAID_GROUP_OBJ_BLK_SIZE, DD_MS_VAULT_RAID_GROUP_OBJ_MIN_FRU, DD_MS_VAULT_RAID_GROUP_OBJ_MAX_FRU,     DD_MS_EXTERNAL_USE_REGION_2},

	    {DD_MS_NT_BOOT_SPA_PRIM_TYPE,     DD_MS_VSIM_NT_BOOT_SPA_PRIM_BLK_SIZE,    DD_MS_NT_BOOT_SPA_PRIM_MIN_FRU,      DD_MS_NT_BOOT_SPA_PRIM_MAX_FRU,         DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_NT_BOOT_SPA_SEC_TYPE,      DD_MS_VSIM_NT_BOOT_SPA_SEC_BLK_SIZE,     DD_MS_NT_BOOT_SPA_SEC_MIN_FRU,       DD_MS_NT_BOOT_SPA_SEC_MAX_FRU,          DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_NT_BOOT_SPB_PRIM_TYPE,     DD_MS_VSIM_NT_BOOT_SPB_PRIM_BLK_SIZE,    DD_MS_NT_BOOT_SPB_PRIM_MIN_FRU,      DD_MS_NT_BOOT_SPB_PRIM_MAX_FRU,         DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_NT_BOOT_SPB_SEC_TYPE,      DD_MS_VSIM_NT_BOOT_SPB_SEC_BLK_SIZE,     DD_MS_NT_BOOT_SPB_SEC_MIN_FRU,       DD_MS_NT_BOOT_SPB_SEC_MAX_FRU,          DD_MS_EXTERNAL_USE_REGION_2},
	    {DD_MS_NT_BOOT_SPA_PRIM_EXP_TYPE, DD_MS_VSIM_NT_BOOT_SPA_PRIM_EXP_BLK_SIZE,DD_MS_NT_BOOT_SPA_PRIM_EXP_MIN_FRU,  DD_MS_NT_BOOT_SPA_PRIM_EXP_MAX_FRU,     DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_NT_BOOT_SPA_SEC_EXP_TYPE,  DD_MS_VSIM_NT_BOOT_SPA_SEC_EXP_BLK_SIZE, DD_MS_NT_BOOT_SPA_SEC_EXP_MIN_FRU,   DD_MS_NT_BOOT_SPA_SEC_EXP_MAX_FRU,      DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_NT_BOOT_SPB_PRIM_EXP_TYPE, DD_MS_VSIM_NT_BOOT_SPB_PRIM_EXP_BLK_SIZE,DD_MS_NT_BOOT_SPB_PRIM_EXP_MIN_FRU,  DD_MS_NT_BOOT_SPB_PRIM_EXP_MAX_FRU,     DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_NT_BOOT_SPB_SEC_EXP_TYPE,  DD_MS_VSIM_NT_BOOT_SPB_SEC_EXP_BLK_SIZE, DD_MS_NT_BOOT_SPB_SEC_EXP_MIN_FRU,   DD_MS_NT_BOOT_SPB_SEC_EXP_MAX_FRU,      DD_MS_EXTERNAL_USE_REGION_2},

	    {DD_MS_UTIL_SPA_PRIM_TYPE,        DD_MS_VSIM_UTIL_SPA_PRIM_BLK_SIZE,       DD_MS_UTIL_SPA_PRIM_MIN_FRU,         DD_MS_UTIL_SPA_PRIM_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_UTIL_SPA_SEC_TYPE,         DD_MS_VSIM_UTIL_SPA_SEC_BLK_SIZE,        DD_MS_UTIL_SPA_SEC_MIN_FRU,          DD_MS_UTIL_SPA_SEC_MAX_FRU,             DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_UTIL_SPB_PRIM_TYPE,        DD_MS_VSIM_UTIL_SPB_PRIM_BLK_SIZE,       DD_MS_UTIL_SPB_PRIM_MIN_FRU,         DD_MS_UTIL_SPB_PRIM_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_UTIL_SPB_SEC_TYPE,         DD_MS_VSIM_UTIL_SPB_SEC_BLK_SIZE,        DD_MS_UTIL_SPB_SEC_MIN_FRU,          DD_MS_UTIL_SPB_SEC_MAX_FRU,             DD_MS_EXTERNAL_USE_REGION_2},
	    {DD_MS_UTIL_SPA_PRIM_EXP_TYPE,    DD_MS_VSIM_UTIL_SPA_PRIM_EXP_BLK_SIZE,   DD_MS_UTIL_SPA_PRIM_EXP_MIN_FRU,     DD_MS_UTIL_SPA_PRIM_EXP_MAX_FRU,        DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_UTIL_SPA_SEC_EXP_TYPE,     DD_MS_VSIM_UTIL_SPA_SEC_EXP_BLK_SIZE,    DD_MS_UTIL_SPA_SEC_EXP_MIN_FRU,      DD_MS_UTIL_SPA_SEC_EXP_MAX_FRU,         DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_UTIL_SPB_PRIM_EXP_TYPE,    DD_MS_VSIM_UTIL_SPB_PRIM_EXP_BLK_SIZE,   DD_MS_UTIL_SPB_PRIM_EXP_MIN_FRU,     DD_MS_UTIL_SPB_PRIM_EXP_MAX_FRU,        DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_UTIL_SPB_SEC_EXP_TYPE,     DD_MS_VSIM_UTIL_SPB_SEC_EXP_BLK_SIZE,    DD_MS_UTIL_SPB_SEC_EXP_MIN_FRU,      DD_MS_UTIL_SPB_SEC_EXP_MAX_FRU,         DD_MS_EXTERNAL_USE_REGION_2},

	    {DD_MS_ICA_IMAGE_REP_TYPE,		  DD_MS_VSIM_ICA_IMAGE_REP_BLK_SIZE,		  DD_MS_ICA_IMAGE_REP_MIN_FRU,		   DD_MS_ICA_IMAGE_REP_MAX_FRU,			   DD_MS_EXTERNAL_USE_REGION_2},
	    {DD_MS_ICA_IMAGE_REP_EXP_TYPE,	  DD_MS_VSIM_ICA_IMAGE_REP_EXP_BLK_SIZE,	  DD_MS_ICA_IMAGE_REP_EXP_MIN_FRU,     DD_MS_ICA_IMAGE_REP_EXP_MAX_FRU,		   DD_MS_EXTERNAL_USE_REGION_2},

        /* Dummy area for third disk so the following areas all start at the same address */
        {DD_MS_UTIL_FLARE_DUMMY_TYPE,     DD_MS_VSIM_UTIL_FLARE_DUMMY_BLK_SIZE,    DD_MS_UTIL_FLARE_DUMMY_MIN_FRU,      DD_MS_UTIL_FLARE_DUMMY_MAX_FRU,         DD_MS_EXTERNAL_USE_REGION_2},

        {DD_MS_FUTURE_GROWTH_TYPE,        DD_MS_VSIM_FUTURE_GROWTH_BLK_SIZE,       DD_MS_FUTURE_GROWTH_MIN_FRU,         DD_MS_FUTURE_GROWTH_MAX_FRU,            DD_MS_EXTERNAL_USE_REGION_2},
        {DD_MS_NON_LUN_RAID_GROUP_OBJ_TYPE, DD_MS_VSIM_NON_LUN_RAID_GROUP_OBJ_BLK_SIZE, DD_MS_NON_LUN_RAID_GROUP_OBJ_MIN_FRU, DD_MS_NON_LUN_RAID_GROUP_OBJ_MAX_FRU, DD_MS_EXTERNAL_USE_REGION_2}
    }
};

#if defined(LAB_DEBUG) || defined(UMODE_ENV)
/* global variables for DD RB test */
typedef enum _ODBS_DD_RB_TEST_CASE_enum
{
    ODBS_DD_RB_TEST_CASE_NONE       = 0,
    ODBS_DD_RB_TEST_CASE_1          = 1,
    ODBS_DD_RB_TEST_CASE_2          = 2,
    ODBS_DD_RB_TEST_CASE_3          = 3,
    ODBS_DD_RB_TEST_CASE_4          = 4,
    ODBS_DD_RB_TEST_CASE_5          = 5,
    ODBS_DD_RB_TEST_CASE_6          = 6,
    ODBS_DD_RB_TEST_CASE_7          = 7,
    ODBS_DD_RB_TEST_CASE_8          = 8,

    ODBS_DD_RB_TEST_CASE_MAX        = 9
} ODBS_DD_RB_TEST_CASE;

#define ODBS_DD_RB_TEST_INVALID_TARGET_FRU   -1

UINT_32 odbs_dd_rb_test_case_max    = ODBS_DD_RB_TEST_CASE_MAX;
UINT_32 odbs_dd_rb_test_flag        = ODBS_DD_RB_TEST_CASE_NONE;
UINT_32 odbs_dd_rb_test_target_fru  = ODBS_DD_RB_TEST_INVALID_TARGET_FRU;

#endif 

/**************************
* MACROS
**************************/

/* The follow defines a "multi-platform ktrace" that compiles correctly in any
 * of the environments into which this module may be compiled: 
 * ddbs.lib, ddbs_info.exe, dbconvert.exe, ntmirror.sys.
 *
 * ARS 415450: This macro was implemented for this ARS. Only key functions
 * use it, but other functions can be converted as time/need permits.
 * 
 */

#if DBCONVERT_ENV
/* DBconvert traces vis CTprint macros */
#define MP_ktracev(...) CTprint(PRINT_MASK_ERROR, __VA_ARGS__)

#elif ODB_FLARE_ENVIRONMENT
/* Flare traces via ktraces */
#define MP_ktracev KvTrace

#else
/* The remainder are environments for which tracing is not offered (e.g. ddbs.lib) or 
 * not desired (e.g. ddbs_info.exe).
 */
#define MP_ktracev(...)
#endif

/* ****** GENERAL DATA DIRECTORY UTILITY FUNCTIONS ***** */

/***********************************************************************
*                  dd_GetExtUseSetInfoPtr()
***********************************************************************
*
* DESCRIPTION:
*   This function returns a pointer to an element in the external 
*   use DB set which is valid for the current operating 
*   environment: Hardware, User Simulation,or VM Simulation.
*
* PARAMETERS:
*   i - index for the element in the external use DB set desired. 
*
* RETURN VALUES/ERRORS:
*   Pointer to the external use DB set info referenced by the supplied
*   index.
*
* NOTES:
*    These three external use DB sets are initialized at compile
*    time, but the decision as to which is valid is made at run time.
*    We default to the hardware disk layout being valid.
*
* HISTORY:
*   3-March-11: Created.                              Greg Mogavero
*
***********************************************************************/

DD_MS_EXTERNAL_USE_SET_INFO* dd_GetExtUseSetInfoPtr(int i)
{
    DD_MS_EXTERNAL_USE_SET_INFO  *pExtUseSetInfo;

    switch (dd_ms_ext_use_set_info.Selected_Ext_Use_Set) {
        case DEFAULT_EXT_USE_SET:
        default:
            pExtUseSetInfo = &dd_ms_ext_use_set_info.Default_Ext_Use_Set[i];
            break;
        case VMSIM_EXT_USE_SET:
            pExtUseSetInfo = &dd_ms_ext_use_set_info.VmSim_Ext_Use_Set[i];
            break;
    }
    return (pExtUseSetInfo);
}

/***********************************************************************
*                  dd_GetVmSimExtUseSetInfoPtr()
***********************************************************************
*
* DESCRIPTION:
*   This function returns a pointer to an element in the external
*   use DB set for the VM Simulation operating environment. Note that
*   this may not be the current operating environment with other 
*   options being Hardware or User Simulation. This intent is for
*   this function to be used by the ddbs_info utility when the 
*   user wants to display the private space layout for VM Simulation
*   even when operating in another environment. 
*
*
* PARAMETERS:
*   i - index for the element in the external use DB set desired. 
*
* RETURN VALUES/ERRORS:
*   Pointer to the external use DB set info referenced by the supplied
*   index.
*
* NOTES:
*    These three external use DB sets are initialized at compile
*    time, but the decision as to which is valid is made at run time.
*    We default to the hardware disk layout being valid.
*
* HISTORY:
*   11-January-12: Created.                              Greg Mogavero
*
***********************************************************************/

DD_MS_EXTERNAL_USE_SET_INFO* dd_GetVmSimExtUseSetInfoPtr(int i)
{
    DD_MS_EXTERNAL_USE_SET_INFO  *pExtUseSetInfo;
    pExtUseSetInfo = &dd_ms_ext_use_set_info.VmSim_Ext_Use_Set[i];
    return (pExtUseSetInfo);
}

/***********************************************************************
*                  dd_CrcManagementData()
***********************************************************************
*
* DESCRIPTION:
*   *GENERAL DATA DIRECTORY UTILITY FUNCTION*
*   This function computes crc on MANAGEMENT data buffer.
*
* PARAMETERS:
*   data       - Ptr to management data;
*   size       - Total size of management data;
*   crc_ptr    - Ptr to place where crc should be placed;
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*   Value of *crc_ptr is set to -1 in case of an error;
*
* NOTES:
*    Calling layer should check for the returned value.
*
* HISTORY:
*   09-Aug-00: Created.                               Mike Zelikov (MZ)
*
***********************************************************************/
DD_ERROR_CODE dd_CrcManagementData(UINT_8E *data, UINT_32 man_size, UINT_32 *crc_ptr) 
{
    UINT_8E *crc_data=NULL;

    odb_assert(crc_ptr != NULL);
    *crc_ptr = DBO_CRC32_DEFAULT_SEED;

    odb_assert(data != NULL);
    odb_assert(man_size > DD_MAN_DATA_CRC_OFFSET);

    /* All management Data structures on a fru
    * contain rev_num and crc as the first
    * fields. These fields do not participate in
    * process of computing the crc.
    */
    crc_data = &(data[DD_MAN_DATA_CRC_OFFSET]);
    *crc_ptr = dbo_Crc32Buffer(DBO_CRC32_DEFAULT_SEED,
        crc_data,
        man_size - DD_MAN_DATA_CRC_OFFSET);

    return DD_OK;

} /* dd_CrcManagementData() */


/***********************************************************************
*                  dd_DboType2Index()
***********************************************************************
*
* DESCRIPTION:
*   *GENERAL DATA DIRECTORY UTILITY FUNCTION*
*   This function returns the order position of a type.
*
* PARAMETERS:
*   service    - ODB service type;
*   rev        - Revision;
*   dbo_type   - DBO Type;
*   index_ptr  - Ptr to index;
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*   Value of *index_ptr is set to DBO_INVALID_INDEX in case of an error;
*
* NOTES:
*    Calling layer should check for the returned value.
*
* HISTORY:
*   07-Nov-01: Created.                               Mike Zelikov (MZ)
*   22-Apr-08: Added DBConvert error printouts      Gary Peterson (GSP)
*
***********************************************************************/
DD_ERROR_CODE dd_DboType2Index(const DD_SERVICE service,
                      const DBO_REVISION rev,
                      const DBO_TYPE dbo_type,
                      UINT_32 *index_ptr)
{
    BOOL  found = FALSE;
    UINT_32 i = 0;
    DD_ERROR_CODE error_code = DD_OK;
    DD_MS_EXTERNAL_USE_SET_INFO *pExtUseSetInfo;

    odb_assert(index_ptr != NULL);
    *index_ptr = DBO_INVALID_INDEX;

    switch (service)
    {
    case DD_MAIN_SERVICE :
        if (rev == DD_MS_REVISION_ONE)
        {
            /* AUX TYPE */
            if (DBO_TYPE_IS_AUX(dbo_type))
            {
                for (i=0; i<DD_NUM_AUX_TYPES; i++)
                {
                    if (dbo_type == dd_ms_aux_set_info[i].dbo_type)
                    {
                        found = TRUE;
                        break;
                    }
                }                    
            }
            /* MANAGEMENT TYPE */
            else if (DBO_TYPE_IS_MANAGEMENT(dbo_type))
            {
                for (i=0; i<DD_NUM_MANAGEMENT_TYPES; i++)
                {
                    if (dbo_type == dd_ms_man_set_info[i].dbo_type)
                    {
                        found = TRUE;
                        break;
                    }
                }
            }
            /* USER TYPE */
            else if (DBO_TYPE_IS_USER(dbo_type))
            {
                for (i=0; i<DD_MS_NUM_USER_TYPES; i++)
                {
                    if (dbo_type == dd_ms_user_set_info[i].dbo_type)
                    {
                        found = TRUE;
                        break;
                    }
                }
            }
            /* EXTERNAL USE TYPE */
            else if (DBO_TYPE_IS_EXTERNAL_USE(dbo_type))
            {
                for (i=0; i<DD_MS_NUM_EXTERNAL_USE_TYPES; i++)
                {
                    pExtUseSetInfo = dd_GetExtUseSetInfoPtr(i);

                    if (dbo_type == pExtUseSetInfo->dbo_type)
                    {
                        found = TRUE;
                        break;
                    }
                }    
            }

            if (found)
            {
                *index_ptr = i;
            }
            else
            {
                error_code = DD_E_INVALID_DBO_TYPE;
            }
        }
        else
        {
            error_code = DD_E_INVALID_REVISION;
        }
        break;

    default:
        error_code = DD_E_INVALID_SERVICE;
    }

#ifdef DBCONVERT_ENV
    if (error_code != DD_OK)
    {
	CTprint(PRINT_MASK_ERROR,
		"dd_DboType2Index: requested data not found");

	CTprint(PRINT_MASK_ERROR,
		"\tService: 0x%x, Rev: 0x%x, DboType: 0x%x",
		service,
		rev,
		dbo_type);
    }
#endif	

    return (error_code);

} /* dd_DboType2Index() */

/***********************************************************************
*                  dd_ManType2Region()
***********************************************************************
*
* DESCRIPTION:
*   *GENERAL DATA DIRECTORY UTILITY FUNCTION*
*   This function returns region corresponding to a DBO type.
*
* PARAMETERS:
*   service    - ODB service type;
*   rev        - Revision;
*   man_type   - Management DB type;
*   region_ptr  - Ptr to region;
*
* RETURN VALUES/ERRORS:
*  DD_ERROR_CODE - Error_code;
*   Value of *region_ptr is set to DD_INVALID_REGION in case of an error;
*
* NOTES:
*    Calling layer should check for the returned value.
*
* HISTORY:
*
*   23-Apr-08: Added DBConvert error printouts      Gary Peterson (GSP)
*
***********************************************************************/
DD_ERROR_CODE dd_ManType2Region(const DD_SERVICE service,
                       const DBO_REVISION rev,
                       const DBO_TYPE man_type,
                       DD_REGION_TYPE *region_ptr)
{
    DD_ERROR_CODE error_code = DD_OK;


    odb_assert(region_ptr != NULL);
    *region_ptr = DD_INVALID_REGION;

    switch (service)
    {
    case DD_MAIN_SERVICE :
        if (rev == DD_MS_REVISION_ONE)
        {
            switch(man_type)
            {
            case DD_MASTER_DDE_TYPE:
                *region_ptr = DD_MS_MGMT_REGION;
                break;

            case DD_INFO_TYPE:
                *region_ptr = DD_MS_INFO_REGION;
                break;

            case DD_PREV_REV_MASTER_DDE_TYPE:
                *region_ptr = DD_MS_PREV_REV_MGMT_REGION;
                break;

            case DD_USER_DDE_TYPE:
                *region_ptr = DD_MS_USER_REGION_1;
                break;

            case DD_EXTERNAL_USE_DDE_TYPE:
                *region_ptr = DD_MS_EXTERNAL_USE_REGION_1;
                break;

            case DD_PUBLIC_BIND_TYPE:
                *region_ptr = DD_MS_PUBLIC_BIND_REGION;
                break;

                /* EXTERNAL USE TYPE */
                /*               else if (DBO_TYPE_IS_EXTERNAL_USE(dbo_type))
                {
                for (i=0; i<DD_MS_NUM_EXTERNAL_USE_TYPES; i++)
                {
                if (dbo_type == dd_ms_ext_use_set_info[i].dbo_type)
                {
                *region_ptr = dd_ms_ext_use_set_info[i].region_type;
                break;
                }
                }    
                }
                }*/

            default:
#ifdef DBCONVERT_ENV
		CTprint(PRINT_MASK_ERROR,
			"dd_ManType2Region: Unsupported region (0x%x) at line %d",
			man_type, __LINE__);
#endif
                error_code = DD_E_INVALID_MAN_TYPE;
            }

            if (*region_ptr == DD_INVALID_REGION)
            {
#ifdef DBCONVERT_ENV
		CTprint(PRINT_MASK_ERROR,
			"dd_ManType2Region: Region (0x%x) translates as INVALID at line %d",
			man_type, __LINE__);
#endif
                error_code = DD_E_INVALID_REGION_TYPE;
            }
            break;
        }

    default:
#ifdef DBCONVERT_ENV
	CTprint(PRINT_MASK_ERROR,
		"dd_ManType2Region: Unsupported service (0x%x) at line %d",
		service, __LINE__);
#endif
        error_code = DD_E_INVALID_SERVICE;
    }

    return (error_code);

} /* dd_ManType2Region() */


/***********************************************************************
*                  dd_Index2DboType()
***********************************************************************
*
* DESCRIPTION:
*   *GENERAL DATA DIRECTORY UTILITY FUNCTION*
*   This function returns DBO TYPE based on the order position;
*
* PARAMETERS:
*   service       - ODB service type;
*   rev           - Revision;
*   dd_class      - DD type class to be produced based on index;
*   index_ptr     - Ptr to index;
*   dbo_type_ptr  - Ptr tot DBO Type to be returned;
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*   Value of *dbo_type_ptr is set to DBO_INVALID_TYPE in case of an error;
*
* NOTES:
*    Calling layer should check for the returned value.
*
* HISTORY:
*   07-Nov-01: Created.                               Mike Zelikov (MZ)
*   22-Apr-08: Added DBConvert error printouts      Gary Peterson (GSP)
*
***********************************************************************/
DD_ERROR_CODE dd_Index2DboType(const DD_SERVICE service,
                      const DBO_REVISION rev,
                      const DD_TYPE_CLASS dd_class,
                      const UINT_32 index,
                      DBO_TYPE *dbo_type_ptr)
{
    DD_ERROR_CODE error_code = DD_OK;
    DD_MS_EXTERNAL_USE_SET_INFO  *pExtUseSetInfo;

    odb_assert(dbo_type_ptr != NULL);
    *dbo_type_ptr = DBO_INVALID_INDEX;

    switch (service)
    {
    case DD_MAIN_SERVICE :
        if (rev == DD_MS_REVISION_ONE)
        {
            switch (dd_class)
            {
            case DBO_TYPE_AUX_CLASS :
                if (index < DD_NUM_AUX_TYPES)
                {
                    odb_assert(DBO_TYPE_IS_AUX(dd_ms_aux_set_info[index].dbo_type));
                    *dbo_type_ptr = dd_ms_aux_set_info[index].dbo_type;
                }
                else
                {
#ifdef DBCONVERT_ENV
		    CTprint(PRINT_MASK_ERROR,
			    "dd_Index2DboType: Invalid AUX_CLASS index at line %d",
			    __LINE__);
		    CTprint(PRINT_MASK_ERROR,
			    "\tdd_class: 0x%x, index: 0x%x (0x%x)",
			    dd_class, index, DD_NUM_AUX_TYPES);
#endif 
                    error_code = DD_E_INVALID_DBO_CLASS;
                }
                break;

            case DBO_TYPE_MANAGEMENT_CLASS :
                if (index < DD_NUM_MANAGEMENT_TYPES)
                {
                    odb_assert(DBO_TYPE_IS_MANAGEMENT(dd_ms_man_set_info[index].dbo_type));
                    *dbo_type_ptr = dd_ms_man_set_info[index].dbo_type;
                }
                else
                {
#ifdef DBCONVERT_ENV
		    CTprint(PRINT_MASK_ERROR,
			    "dd_Index2DboType: Invalid MANAGEMENT_CLASS index at line %d",
			    __LINE__);
		    CTprint(PRINT_MASK_ERROR,
			    "\tdd_class: 0x%x, index: 0x%x (0x%x)",
			    dd_class, index, DD_NUM_MANAGEMENT_TYPES);
#endif 
                    error_code = DD_E_INVALID_DBO_CLASS;
                }
                break;

            case DBO_TYPE_USER_CLASS :
                if (index < DD_MS_NUM_USER_TYPES)
                {
                    odb_assert(DBO_TYPE_IS_USER(dd_ms_user_set_info[index].dbo_type));
                    *dbo_type_ptr = dd_ms_user_set_info[index].dbo_type;
                }
                else
                {
#ifdef DBCONVERT_ENV
		    CTprint(PRINT_MASK_ERROR,
			    "dd_Index2DboType: Invalid USER_CLASS index at line %d",
			    __LINE__);
		    CTprint(PRINT_MASK_ERROR,
			    "\tdd_class: 0x%x, index: 0x%x (0x%x)",
			    dd_class, index, DD_MS_NUM_USER_TYPES);
#endif 
                    error_code = DD_E_INVALID_DBO_CLASS;
                }
                break;

            case DBO_TYPE_EXTERNAL_USE_CLASS :
                if (index < DD_MS_NUM_EXTERNAL_USE_TYPES)
                {
                    pExtUseSetInfo = dd_GetExtUseSetInfoPtr(index);

                    odb_assert(DBO_TYPE_IS_EXTERNAL_USE(pExtUseSetInfo->dbo_type));
                    *dbo_type_ptr = pExtUseSetInfo->dbo_type;
                }
                else
                {
#ifdef DBCONVERT_ENV
		    CTprint(PRINT_MASK_ERROR,
			    "dd_Index2DboType: Invalid EXT_USE_CLASS index at line %d",
			    __LINE__);
		    CTprint(PRINT_MASK_ERROR,
			    "\tdd_class: 0x%x, index: 0x%x (0x%x)",
			    dd_class, index, DD_MS_NUM_EXTERNAL_USE_TYPES);
#endif 
                    error_code = DD_E_INVALID_DBO_CLASS;
                }
                break;

            default:
#ifdef DBCONVERT_ENV
		CTprint(PRINT_MASK_ERROR,
			"dd_Index2DboType: Invalid class value (0x%x) at line %d",
			dd_class, __LINE__);
#endif
                error_code = DD_E_INVALID_DBO_CLASS;
            }
        }
        else
        {
#ifdef DBCONVERT_ENV
		CTprint(PRINT_MASK_ERROR,
			"dd_Index2DboType: Invalid rev value (0x%x) at line %d",
			rev, __LINE__);
#endif
            error_code = DD_E_INVALID_REVISION;
        }
        break;

    default:
#ifdef DBCONVERT_ENV
	CTprint(PRINT_MASK_ERROR,
		"dd_Index2DboType: Invalid service value (0x%x) at line %d",
		service, __LINE__);
#endif
        error_code = DD_E_INVALID_SERVICE;
    }

    return (error_code);

} /* dd_Index2DboType() */


/***********************************************************************
*                  dd_GetNextDboType()
***********************************************************************
*
* DESCRIPTION:
*   *GENERAL DATA DIRECTORY UTILITY FUNCTION*
*   This function returns next DBO TYPE in order
*
* PARAMETERS:
*   service        - ODB service type;
*   rev            - Revision;
*   dbo_type_ptr   - Ptr to DBO Type;
*   mdde_ptr       - Ptr to MDDE;
*   curr_mdde_rev - Current revision of Master DDE
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*   Value of *dbo_type_ptr is set DBO_INVALID_TYPE in case of an error.
*
* NOTES:
*    Calling layer should check for the returned value.
*
* HISTORY:
*   07-Nov-01: Created.                               Mike Zelikov (MZ)
*   23-Apr-08: Added DBConvert error printouts      Gary Peterson (GSP)
*
***********************************************************************/
DD_ERROR_CODE dd_GetNextDboType(const DD_SERVICE service,
                       const DBO_REVISION rev,
                       DBO_TYPE *dbo_type_ptr,
                       MASTER_DDE_DATA *mdde_ptr,
                       const DBO_REVISION curr_mdde_rev)
{
    UINT_32 index = 0, num_external_use_types = 0, num_user_types=0;
    DBO_TYPE dbo_type=DBO_INVALID_TYPE;
    DD_ERROR_CODE error_code = DD_OK;
    DD_MS_EXTERNAL_USE_SET_INFO  *pExtUseSetInfo;

    odb_assert(dbo_type_ptr != NULL);
    dbo_type = *dbo_type_ptr;
    *dbo_type_ptr = DBO_INVALID_INDEX;

    switch (service)
    {
    case DD_MAIN_SERVICE :
        if (rev == DD_MS_REVISION_ONE)
        {
            error_code = dd_DboType2Index(service, rev, dbo_type, &index);
            if (error_code != DD_OK)
	    {
#ifdef DBCONVERT_ENV
		CTprint(PRINT_MASK_ERROR,
			"dd_GetNextDboType: error from dd_DboType2Index at line %d",
			__LINE__);
#endif       
        return (error_code);
	    }

            /* AUX TYPE */
            if (DBO_TYPE_IS_AUX(dbo_type))
            {
                if (index < (DD_NUM_AUX_TYPES - 1))
                {
                    *dbo_type_ptr = dd_ms_aux_set_info[index+1].dbo_type;
                }
            }
            /* MANAGEMENT TYPE */
            else if (DBO_TYPE_IS_MANAGEMENT(dbo_type))
            {
                if (index < (DD_NUM_MANAGEMENT_TYPES - 1))
                {
                    *dbo_type_ptr = dd_ms_man_set_info[index+1].dbo_type;
                }
            }
            /* USER TYPE */
            else if (DBO_TYPE_IS_USER(dbo_type))
            {
                error_code = dd_WhatIsNumUserExtUseTypes(service,
                    rev,
                    DD_USER_DDE_TYPE,
                    curr_mdde_rev,
                    &num_user_types);
                odb_assert(!error_code);
                if (index < (num_user_types - 1))
                {
                    *dbo_type_ptr = dd_ms_user_set_info[index+1].dbo_type;
                }
            }
            /* EXTERNAL USE TYPE */
            else if (DBO_TYPE_IS_EXTERNAL_USE(dbo_type))
            {
                error_code = dd_WhatIsNumUserExtUseTypes(service,
                    rev,
                    DD_EXTERNAL_USE_DDE_TYPE,
                    curr_mdde_rev,
                    &num_external_use_types);
                odb_assert(!error_code);

                if (index < (num_external_use_types - 1))
                {
                    pExtUseSetInfo = dd_GetExtUseSetInfoPtr(index+1);
                    *dbo_type_ptr = pExtUseSetInfo->dbo_type;
                }
            }
            else
            {
#ifdef DBCONVERT_ENV
		CTprint(PRINT_MASK_ERROR,
			"dd_GetNextDboType: Unknown Dbo Type for 0x%x at line %d",
			dbo_type, __LINE__);
#endif       

                error_code = DD_E_INVALID_DBO_TYPE;
            }
        }
        else
        {
#ifdef DBCONVERT_ENV
	    CTprint(PRINT_MASK_ERROR,
		    "dd_GetNextDboType: Unknown rev (0x%x) at line %d",
		    rev, __LINE__);
#endif       
            error_code = DD_E_INVALID_REVISION;
        }
        break;

    default:
#ifdef DBCONVERT_ENV
	CTprint(PRINT_MASK_ERROR,
		"dd_GetNextDboType: Unknown service (0x%x) at line %d",
		service, __LINE__);
#endif       
        error_code = DD_E_INVALID_SERVICE;
    }

    return (error_code);

} /* dd_GetNextDboType() */


/***********************************************************************
*                  dd_WhatIsDdAuxStartAddr()
***********************************************************************
*
* DESCRIPTION:
*   *GENERAL DATA DIRECTORY UTILITY FUNCTION*
*   This function returns Data Directory AUX space start address.
*
* PARAMETERS:
*   service    - ODB service type;
*   rev        - Revision;
*   addr       - Ptr to Address value to be returned;
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*   Value of *addr is set to DBOLB_INVALID_ADDRESS in case of an error;
*
* NOTES:
*    Calling layer should check for the returned value.
*
* HISTORY:
*   10-Jan-02: Created.                               Mike Zelikov (MZ)
*   23-Apr-08: Added DBConvert error printouts      Gary Peterson (GSP)
*
***********************************************************************/
DD_ERROR_CODE dd_WhatIsDdAuxStartAddr(const DD_SERVICE service,
                             const DBO_REVISION rev,
                             DBOLB_ADDR *addr)
{
    DD_ERROR_CODE error_code = DD_OK;


    odb_assert(addr != NULL);
    odb_assert(DD_NUM_AUX_TYPES != 0);
    *addr = DBOLB_INVALID_ADDRESS;

    switch (service)
    {
    case DD_MAIN_SERVICE :
        if (rev == DD_MS_REVISION_ONE)
        {
            *addr = DD_START_ADDRESS;
        }
        else
        {
#ifdef DBCONVERT_ENV
	    CTprint(PRINT_MASK_ERROR,
		    "dd_WhatIsDdAuxStartAddr: Unknown rev (0x%x) at line %d",
		    rev, __LINE__);
#endif       
            error_code = DD_E_INVALID_REVISION;
        }
        break;

    default:
#ifdef DBCONVERT_ENV
	CTprint(PRINT_MASK_ERROR,
		"dd_WhatIsDdAuxStartAddr: Unknown service (0x%x) at line %d",
		service, __LINE__);
#endif       
        error_code = DD_E_INVALID_SERVICE;
    }

    return (error_code);

} /* dd_WhatIsDdAuxStartAddr() */


/***********************************************************************
*                  dd_WhatIsDdAuxSpace()
***********************************************************************
*
* DESCRIPTION:
*   *GENERAL DATA DIRECTORY UTILITY FUNCTION*
*   This function returns Data Directory AUX space size.
*
* PARAMETERS:
*   service    - ODB service type;
*   rev        - Revision;
*   blk_size   - Ptr to block size to be returned;
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*   Value of *blk_size is set to 0 in case of an error;
*
* NOTES:
*    Calling layer should check for the returned value.
*
* HISTORY:
*   10-Jan-02: Created.                               Mike Zelikov (MZ)
*   23-Apr-08: Added DBConvert error printouts      Gary Peterson (GSP)
*
***********************************************************************/
DD_ERROR_CODE dd_WhatIsDdAuxSpace(const DD_SERVICE service,
                         const DBO_REVISION rev,
                         UINT_32 *blk_size)
{
    UINT_32 i=0;
    DD_ERROR_CODE error_code = DD_OK;


    odb_assert(blk_size != NULL);
    odb_assert(DD_NUM_AUX_TYPES != 0);
    *blk_size = 0;

    switch (service)
    {
    case DD_MAIN_SERVICE :
        if (rev == DD_MS_REVISION_ONE)
        {
            for (i=0; i < DD_NUM_AUX_TYPES; i++) 
            {
                *blk_size +=  dd_ms_aux_set_info[i].blk_size;
            }
        }
        else
        {
#ifdef DBCONVERT_ENV
	    CTprint(PRINT_MASK_ERROR,
		    "dd_WhatIsDdAuxSetSpace: Unknown rev (0x%x) at line %d",
		    rev, __LINE__);
#endif       
            error_code = DD_E_INVALID_REVISION;
        }
        break;

    default:
#ifdef DBCONVERT_ENV
	CTprint(PRINT_MASK_ERROR,
		"dd_WhatIsDdAuxSetSpace: Unknown service (0x%x) at line %d",
		service, __LINE__);
#endif       
        error_code = DD_E_INVALID_SERVICE;
    }

    return (error_code);

} /* dd_WhatIsDdAuxSpace() */


/***********************************************************************
*                  dd_WhatIsDdAuxSetSpace()
***********************************************************************
*
* DESCRIPTION:
*   *GENERAL DATA DIRECTORY UTILITY FUNCTION*
*   This function returns Data Directory AUX SET space size.
*
* PARAMETERS:
*   service    - ODB service type;
*   rev        - Revision;
*   aux_type   - AUX space set dbo type;
*   blk_size   - Ptr to block size to be returned;
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*   Value of *blk_size is set to 0 in case of an error;
*
* NOTES:
*    Calling layer should check for the returned value.
*
* HISTORY:
*   10-Jan-02: Created.                               Mike Zelikov (MZ)
*   23-Apr-08: Added DBConvert error printouts      Gary Peterson (GSP)
*
***********************************************************************/
DD_ERROR_CODE dd_WhatIsDdAuxSetSpace(const DD_SERVICE service,
                            const DBO_REVISION rev,
                            const DBO_TYPE aux_type,                            
                            UINT_32 *blk_size)
{
    UINT_32 index = 0;
    DD_ERROR_CODE error_code = DD_OK;


    odb_assert(blk_size != NULL);
    odb_assert(DD_NUM_AUX_TYPES != 0);
    odb_assert(DBO_TYPE_IS_AUX(aux_type));
    *blk_size = 0;

    switch (service)
    {
    case DD_MAIN_SERVICE :
        if (rev == DD_MS_REVISION_ONE)
        {
            error_code = dd_DboType2Index(service, rev, aux_type, &index);
            if (error_code != DD_OK)
	    {
#ifdef DBCONVERT_ENV
		CTprint(PRINT_MASK_ERROR,
			"dd_WhatIsDdAuxSetStartAddr: error from dd_DboType2Index at line %d",
			__LINE__);
#endif       
        return (error_code);
	    }

            odb_assert (index < DD_NUM_AUX_TYPES);
            odb_assert (dd_ms_aux_set_info[index].dbo_type == aux_type);

            *blk_size = dd_ms_aux_set_info[index].blk_size;
        }
        else
        {
#ifdef DBCONVERT_ENV
	    CTprint(PRINT_MASK_ERROR,
		    "dd_WhatIsDdAuxSetStartAddr: Unknown rev (0x%x) at line %d",
		    rev, __LINE__);
#endif       
            error_code = DD_E_INVALID_REVISION;
        }
        break;

    default:
#ifdef DBCONVERT_ENV
	CTprint(PRINT_MASK_ERROR,
		"dd_WhatIsDdAuxSetStartAddr: Unknown service (0x%x) at line %d",
		service, __LINE__);
#endif       
        error_code = DD_E_INVALID_SERVICE;
    }

    return (error_code);

} /* dd_WhatIsDdAuxSetSpace() */


/***********************************************************************
*                  dd_WhatIsDdAuxSetStartAddr()
***********************************************************************
*
* DESCRIPTION:
*   *GENERAL DATA DIRECTORY UTILITY FUNCTION*
*   This function returns Data Directory AUX space SET start address.
*
* PARAMETERS:
*   service    - ODB service type;
*   rev        - Revision;
*   aux_type   - AUX space set dbo type;
*   addr       - Ptr to Address value to be returned;
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*   Value of *addr is set to DBOLB_INVALID_ADDRESS in case of an error;
*
* NOTES:
*    Calling layer should check for the returned value.
*
* HISTORY:
*   10-Jan-02: Created.                               Mike Zelikov (MZ)
*   23-Apr-08: Added DBConvert error printouts      Gary Peterson (GSP)
*
***********************************************************************/
DD_ERROR_CODE dd_WhatIsDdAuxSetStartAddr(const DD_SERVICE service,
                                const DBO_REVISION rev,
                                const DBO_TYPE aux_type,
                                DBOLB_ADDR *addr)
{
    UINT_32 temp_space = 0, index=0, aux_index=0;
    DBOLB_ADDR temp_addr=DBOLB_INVALID_ADDRESS;
    DBO_TYPE t=DBO_INVALID_TYPE;
    DD_ERROR_CODE error_code = DD_OK;


    odb_assert(addr != NULL);
    odb_assert(DD_NUM_AUX_TYPES != 0);
    odb_assert(DBO_TYPE_IS_AUX(aux_type));
    *addr = DBOLB_INVALID_ADDRESS;

    switch (service)
    {
    case DD_MAIN_SERVICE :
        if (rev == DD_MS_REVISION_ONE)
        {
            error_code = dd_WhatIsDdAuxStartAddr(service, rev, &temp_addr);
            if (error_code != DD_OK)
	    {
#ifdef DBCONVERT_ENV
		CTprint(PRINT_MASK_ERROR,
			"dd_WhatIsDdAuxSetStartAddr: error from dd_WhatIsDdAuxStartAddr at line %d",
			__LINE__);
#endif
        return (error_code); 
	    }

            error_code = dd_DboType2Index(service, rev, aux_type, &aux_index);
            if (error_code != DD_OK)
        {
#ifdef DBCONVERT_ENV
        CTprint(PRINT_MASK_ERROR,
            "dd_WhatIsDdAuxSetStartAddr: error from dd_DboType2Index at line %d",
            __LINE__);
#endif
        return(error_code); 
	    }

            odb_assert(index < DD_NUM_AUX_TYPES);
            odb_assert(dd_ms_aux_set_info[index].dbo_type == aux_type);

            for (index=0; index < aux_index; index++) 
            {
                error_code = dd_Index2DboType(service, rev,
                    DBO_TYPE_AUX_CLASS,
                    index, &t);
                if (error_code != DD_OK)
		{
#ifdef DBCONVERT_ENV
		    CTprint(PRINT_MASK_ERROR,
			    "dd_WhatIsDdAuxSetStartAddr: error from dd_Index2DboType at line %d",
			    __LINE__);
#endif
            return (error_code);
		}

                odb_assert(dd_ms_aux_set_info[index].dbo_type == t);

                error_code = dd_WhatIsDdAuxSetSpace(service, rev,
                    t, &temp_space);
                if (error_code != DD_OK)
		{
#ifdef DBCONVERT_ENV
		    CTprint(PRINT_MASK_ERROR,
			    "dd_WhatIsDdAuxSetStartAddr: error from dd_WhatIsDdAuxSetSpace at line %d",
			    __LINE__);
#endif
            return (error_code);
		}
                temp_addr += temp_space;
            }

            *addr = temp_addr;
        }
        else
        {
#ifdef DBCONVERT_ENV
	    CTprint(PRINT_MASK_ERROR,
		    "dd_WhatIsDdAuxSetStartAddr: Unsupported rev (0x%x) at line %d",
		    rev, __LINE__);
#endif
            error_code = DD_E_INVALID_REVISION;
        }
        break;

    default:
#ifdef DBCONVERT_ENV
	CTprint(PRINT_MASK_ERROR,
		"dd_WhatIsDdAuxSetStartAddr: Unsupported service (0x%x) at line %d",
		service, __LINE__);
#endif
        error_code = DD_E_INVALID_SERVICE;
    }

    return (error_code);

} /* dd_WhatIsDdAuxSetStartAddr() */


/***********************************************************************
*                  dd_WhatIsDataDirectoryStartAddr()
***********************************************************************
*
* DESCRIPTION:
*   *GENERAL DATA DIRECTORY UTILITY FUNCTION*
*   This function returns start address of database starting from
*   MANAGEMENT space (MDDE).
*
* PARAMETERS:
*   service    - ODB service type;
*   rev        - Revision;
*   addr       - Ptr to Address value to be returned;
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*   Value of *addr is set to DBOLB_INVALID_ADDRESS in case of an error;
*
* NOTES:
*    Calling layer should check for the returned value.
*
* HISTORY:
*   09-Aug-00: Created.                               Mike Zelikov (MZ)
*   23-Apr-08: Added DBConvert error printouts      Gary Peterson (GSP)
*
***********************************************************************/
DD_ERROR_CODE dd_WhatIsDataDirectoryStartAddr(const DD_SERVICE service,
                                     const DBO_REVISION rev,
                                     DBOLB_ADDR *addr)
{
    UINT_32 blk_size=0;
    DBOLB_ADDR temp_addr=DBOLB_INVALID_ADDRESS;
    DD_ERROR_CODE error_code = DD_OK;


    odb_assert(addr != NULL);
    *addr = DBOLB_INVALID_ADDRESS;

    switch (service)
    {
    case DD_MAIN_SERVICE :
        if (rev == DD_MS_REVISION_ONE)
        {
            error_code = dd_WhatIsDdAuxStartAddr(service, rev, &temp_addr);
            if (error_code != DD_OK) 
	    {
#ifdef DBCONVERT_ENV
		CTprint(PRINT_MASK_ERROR,
			"dd_WhatIsDataDirectoryStartAddr: error from dd_WhatIsDdAuxStartAddr at line %d",
			__LINE__);
#endif 
        return(error_code); 
	    }

            error_code = dd_WhatIsDdAuxSpace(service, rev, &blk_size);
            if (error_code != DD_OK) 
	    { 
#ifdef DBCONVERT_ENV
		CTprint(PRINT_MASK_ERROR,
			"dd_WhatIsDataDirectoryStartAddr: error from dd_WhatIsDdAuxSpace at line %d",
			__LINE__);
#endif 
        return(error_code); 
	    }

            *addr = temp_addr + blk_size + DD_MS_REV_ONE_START_ADDRESS;
        }
        else
        {
#ifdef DBCONVERT_ENV
	CTprint(PRINT_MASK_ERROR,
		"dd_WhatIsDataDirectoryStartAddr: Unsupported rev (0x%x) at line %d",
		rev, __LINE__);
#endif
            error_code = DD_E_INVALID_REVISION;
        }
        break;

    default:
#ifdef DBCONVERT_ENV
	CTprint(PRINT_MASK_ERROR,
		"dd_WhatIsDataDirectoryStartAddr: Unsupported service (0x%x) at line %d",
		service, __LINE__);
#endif
        error_code = DD_E_INVALID_SERVICE;
    }

    return (error_code);

} /* dd_WhatIsDataDirectoryStartAddr() */


/***********************************************************************
*                  dd_WhatIsDbSpace()
***********************************************************************
*
* DESCRIPTION:
*   *GENERAL DATA DIRECTORY UTILITY FUNCTION*
*    This function returns the space required for a specific DB 
*    area. This area is identified by MANAGEMENT DBO TYPE.
*
* PARAMETERS:
*   service    - ODB service type;
*   rev        - Revision;
*   man_type   - Management DBO Type;
*   fru        - Fru index;
*   blk_size   - Ptr to block size to be returned;
*   DiskLayout - Default or VM Simulation layout
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*   Value of *blk_size is set to 0 in case of an error;
*
* NOTES:
*    Calling layer should check for the returned value.
*    Use DD_MASTER_DDE_TYPE          -- for identifying MANAGEMENT SPACE
*    Use DD_PREV_REV_MASTER_DDE_TYPE -- for idnetifying PREV REV MANAGEMENT SPACE
*    Use DD_USER_DDE_TYPE            -- for idnetifying USER DB SPACE
*    Use DD_EXTERNAL_USE_DDE_TYPE    -- for idnetifying EXTERNAL USE DB SPACE
*    Use DD_PUBLIC_BIND_TYPE         -- for identifying PUBLIC BIND SPACE
*
*    Returned size for Management Types that do not represent a DD DB area
*    should be 0;
*
* HISTORY:
*   09-Aug-00: Created.                               Mike Zelikov (MZ)
*   23-Apr-08: Added DBConvert error printouts        Gary Peterson (GSP)
*   03-Feb-12: Support for VM Simulation layout       Greg Mogavero             
*
***********************************************************************/

DD_ERROR_CODE dd_WhatIsDbSpace(const DD_SERVICE service,
                      const DBO_REVISION rev,
                      const UINT_32 fru,
                      const DBO_TYPE man_type,
                      const DISK_LAYOUT_TYPE DiskLayout,
                      UINT_32 *blk_size)
{
    UINT_32 index = 0;
    DD_ERROR_CODE error_code = DD_OK;
    
    odb_assert(DDE_CURRENT_REVISION == DDE_REVISION_ONE);
    odb_assert(DBO_TYPE_IS_MANAGEMENT(man_type));

    odb_assert(blk_size != NULL);
    *blk_size = 0;

    switch (service)
    {
    case DD_MAIN_SERVICE :
        if (rev == DD_MS_REVISION_ONE)
        {
            odb_assert((fru == DBO_INVALID_FRU) || (fru < FRU_COUNT));

            error_code = dd_DboType2Index(service, rev, man_type, &index);
            if (error_code != DD_OK)
            {
#ifdef DBCONVERT_ENV
                CTprint(PRINT_MASK_ERROR,
                        "dd_WhatIsDbSpace: error from dd_DboType2Index at line %d",
                        __LINE__);
#endif		
                return (error_code);
            }

            odb_assert (index < DD_NUM_MANAGEMENT_TYPES);
            odb_assert (dd_ms_man_set_info[index].dbo_type == man_type);

            if (dd_ms_man_set_info[index].is_set != TRUE)
            {
                switch (man_type)
                {
                case DD_INFO_TYPE :
                    *blk_size = 0;
                    break;

                default:
#ifdef DBCONVERT_ENV
                    CTprint(PRINT_MASK_ERROR,
                            "dd_WhatIsDbSpace: Invlaid man_type (0x%x) at line %d",
                            man_type, __LINE__);
#endif		
                    error_code = DD_E_INVALID_MAN_TYPE;
                }
            }
            else
            {
                switch(man_type)
                {
                case DD_MASTER_DDE_TYPE :
                    *blk_size = DD_MS_REV_ONE_MAN_SPACE_BLK_SIZE;
                    break;

                case DD_PREV_REV_MASTER_DDE_TYPE :
                    *blk_size = DD_MS_REV_ONE_PREV_REV_MAN_SPACE_BLK_SIZE;
                    break;

                case DD_USER_DDE_TYPE :          
                    if (vmSimulationMode || (DiskLayout == VmSimLayout))
                    {
                        *blk_size = DD_MS_VSIM_USER_DB_BLK_SIZE;
                    }
                    else
                    {
                        *blk_size = DD_MS_USER_DB_BLK_SIZE;
                    }
                    break;

                case DD_EXTERNAL_USE_DDE_TYPE :
                    // Verify that we have a valid fru 
                    // fru >= FRU_COUNT - fru starts at 0 
                    if ((fru == DBO_INVALID_FRU) || (fru >= FRU_COUNT))
                    {
#ifdef DBCONVERT_ENV
                        CTprint(PRINT_MASK_ERROR,
                                "dd_WhatIsDbSpace: Invlaid fru value at line %d",
                                __LINE__);
                        CTprint(PRINT_MASK_ERROR,
                                "\tman_type: 0x%x, fru: 0x%x (0x%x)",
                                man_type, fru, FRU_COUNT);
#endif		
                        error_code = DD_E_INVALID_MAN_SET_INFO;
                    }
                    else
                    {
                        if (fru <= DD_MS_MAX_VAULT_FRU)
                        {
                            // The space is the same for ALL VAULT DISKS 
                            if (vmSimulationMode || (DiskLayout == VmSimLayout))
                            {
                                *blk_size = DD_MS_VSIM_EXTERNAL_USE_VAULT_FRUS_TOTAL_BLK_SIZE;
                            }
                            else
                            {
                               *blk_size = DD_MS_EXTERNAL_USE_VAULT_FRUS_TOTAL_BLK_SIZE;
                            }
                        }
                        else
                        {
                            if (vmSimulationMode || (DiskLayout == VmSimLayout))
                            {
                                *blk_size = DD_MS_VSIM_EXTERNAL_USE_ALL_FRUS_BLK_SIZE;
                            }
                            else
                            {
                                *blk_size = DD_MS_EXTERNAL_USE_ALL_FRUS_BLK_SIZE;
                            }
                        }
                    }
                    break;

                case DD_PUBLIC_BIND_TYPE :
                    // We do not care about this size -- set to zero
                    // HR: blk_size value is zero on 'error' also
                    *blk_size = 0;
                    break;

                default:
#ifdef DBCONVERT_ENV
                    CTprint(PRINT_MASK_ERROR,
                            "dd_WhatIsDbSpace: Invlaid man_type (0x%x) at line %d",
                            man_type, __LINE__);
#endif		
                    error_code = DD_E_INVALID_MAN_TYPE;
                }
            }
        }
        else
        {
#ifdef DBCONVERT_ENV
            CTprint(PRINT_MASK_ERROR,
                    "dd_WhatIsDbSpace: Invlaid rev (0x%x) at line %d",
                    rev, __LINE__);
#endif		
            error_code = DD_E_INVALID_REVISION;
        }
        break;

    default:
#ifdef DBCONVERT_ENV
        CTprint(PRINT_MASK_ERROR,
                "dd_WhatIsDbSpace: Invlaid service (0x%x) at line %d",
                service, __LINE__);
#endif		
        error_code = DD_E_INVALID_SERVICE;
    }

    return (error_code);

} /* dd_WhatIsDbSpace() */


/***********************************************************************
*                  dd_WhatIsRegionSpace()
***********************************************************************
*
* DESCRIPTION:
*   *GENERAL DATA DIRECTORY UTILITY FUNCTION*
*    This function returns the space required for a specific DB 
*    region. 
*
* PARAMETERS:
*   service    - ODB service type;
*   rev        - Revision;
*   region     - Management region;
*   fru        - Fru index;
*   blk_size   - Ptr to block size to be returned;
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*   Value of *blk_size is set to 0 in case of an error;
*
* NOTES:
*    Calling layer should check for the returned value.
*    Use DD_MS_MGMT_REGION          -- for identifying MANAGEMENT REGION
*    Use DD_MS_USER_REGION_1        -- for idnetifying USER DB REGION 1
*    Use DD_MS_USER_REGION_2        -- for idnetifying USER DB REGION 2
*    Use DD_EXTERNAL_USE_REGION_1   -- for idnetifying EXTERNAL USE DB REGION 1
*    Use DD_EXTERNAL_USE_REGION_2   -- for idnetifying EXTERNAL USE DB REGION 2
*    Use DD_PUBLIC_BIND_REGION      -- for identifying PUBLIC BIND REGION
*
*    Returned size for Management Types that do not represent a DD region
*    should be 0;
*
* HISTORY:
*
*   23-Apr-08: Added DBConvert error printouts      Gary Peterson (GSP)
*
***********************************************************************/
DD_ERROR_CODE dd_WhatIsRegionSpace(const DD_SERVICE service,
                          const DBO_REVISION rev,
                          const UINT_32 fru,
                          const DD_REGION_TYPE region,
                          const DISK_LAYOUT_TYPE DiskLayout,
                          UINT_32 *blk_size)
{
    UINT_32 index = 0;
    DD_ERROR_CODE error_code = DD_OK;


    odb_assert(DDE_CURRENT_REVISION == DDE_REVISION_ONE);

    odb_assert(blk_size != NULL);
    *blk_size = 0;

    switch (service)
    {
    case DD_MAIN_SERVICE :
        if (rev == DD_MS_REVISION_ONE)
        {
            odb_assert((fru == DBO_INVALID_FRU) || (fru < FRU_COUNT));

            switch(region)
            {
            case DD_MS_MGMT_REGION :
                *blk_size = DD_MS_REV_ONE_MAN_SPACE_BLK_SIZE;
                break;

            case DD_MS_INFO_REGION:
                *blk_size = 0;
                break;

            case DD_MS_PREV_REV_MGMT_REGION :
                *blk_size = DD_MS_REV_ONE_PREV_REV_MAN_SPACE_BLK_SIZE;
                break;

            case DD_MS_USER_REGION_1 :
                if (vmSimulationMode || (DiskLayout == VmSimLayout))
                {
                    *blk_size = DD_MS_VSIM_USER_DB1_BLK_SIZE;
                }
                else
                {
                    *blk_size = DD_MS_USER_DB1_BLK_SIZE;
                }
                break;

            case DD_MS_USER_REGION_2 :
                /* Verify that we have a valid fru */
                if ((fru == DBO_INVALID_FRU) || (fru >= FRU_COUNT))
                {
#ifdef DBCONVERT_ENV
		    CTprint(PRINT_MASK_ERROR,
			    "dd_WhatIsRegionSpace: Invlaid fru (0x%x) at line %d",
			    fru, __LINE__);
#endif

                    error_code = DD_E_INVALID_FRU;
                }
                else
                {
                    if (fru <= DD_MS_MAX_VAULT_FRU)
                    {
                        /* The space is the same for ALL vault DISKS */
                        if (vmSimulationMode || (DiskLayout == VmSimLayout))
                        {
                            *blk_size = DD_MS_VSIM_USER_DB2_BLK_SIZE;
                        }
                        else
                        {
                            *blk_size = DD_MS_USER_DB2_BLK_SIZE;
                        }
                    }
                    else
                    {
                        /* This region is on disks 0-4 only */
                        *blk_size = 0;
                    }
                }
                break;

            case DD_MS_EXTERNAL_USE_REGION_1 :
                /* Verify that we have a valid fru */
                if ((fru == DBO_INVALID_FRU) || (fru >= FRU_COUNT))
                {
#ifdef DBCONVERT_ENV
		    CTprint(PRINT_MASK_ERROR,
			    "dd_WhatIsRegionSpace: Invlaid fru (0x%x) at line %d",
			    fru, __LINE__);
#endif
                    error_code = DD_E_INVALID_FRU;
                }
                else
                {
                    if (fru <= DD_MS_MAX_VAULT_FRU)
                    {
                        /* The space is the same for ALL VAULT DISKS */
                        if (vmSimulationMode || (DiskLayout == VmSimLayout))
                        {
                            *blk_size = DD_MS_VSIM_EXTERNAL_USE_VAULT_FRUS_TOTAL_BLK_SIZE;
                        }
                        else
                        {
                            *blk_size = DD_MS_EXTERNAL_USE_VAULT_FRUS_TOTAL_BLK_SIZE;
                        }
                    }
                    else
                    {
                        if (vmSimulationMode || (DiskLayout == VmSimLayout))
                        {
                            *blk_size = DD_MS_VSIM_EXTERNAL_USE_ALL_FRUS_BLK_SIZE;
                        }
                        else
                        {
                            *blk_size = DD_MS_EXTERNAL_USE_ALL_FRUS_BLK_SIZE;
                        }
                    }
                }
                break;

            case DD_MS_EXTERNAL_USE_REGION_2 :
                /* Verify that we have a valid fru */
                if ((fru == DBO_INVALID_FRU) || (fru >= FRU_COUNT))
                {
#ifdef DBCONVERT_ENV
		    CTprint(PRINT_MASK_ERROR,
			    "dd_WhatIsRegionSpace: Invlaid fru (0x%x) at line %d",
			    fru, __LINE__);
#endif
                    error_code = DD_E_INVALID_FRU;
                }
                else
                {
                    *blk_size = 0 ;
                }
                break;

            case DD_MS_PUBLIC_BIND_REGION :
                /* We do not care about this size -- set to zero */
                *blk_size = 0;
                break;

            default:
#ifdef DBCONVERT_ENV
		CTprint(PRINT_MASK_ERROR,
			"dd_WhatIsRegionSpace: Unsupported rev (0x%x) at line %d",
			rev, __LINE__);
#endif
                error_code = DD_E_INVALID_REVISION;
            }
            break;
        }
        error_code = DD_E_INVALID_REVISION;;
        break;

    default:
#ifdef DBCONVERT_ENV
	CTprint(PRINT_MASK_ERROR,
		"dd_WhatIsRegionSpace: Unsupported service (0x%x) at line %d",
		service, __LINE__);
#endif
        error_code = DD_E_INVALID_SERVICE;
    }

    return (error_code);
}

/* dd_WhatIsRegionSpace() */



/***********************************************************************
*                  dd_WhatIsDbStartAddr()
***********************************************************************
*
* DESCRIPTION:
*   *GENERAL DATA DIRECTORY UTILITY FUNCTION*
*   This function returns starting address of a specific DB area.
*   Start address is calculated by adding AUX and MANAGEMENT sizes.
*   This area is identified by MANAGEMENT DBO TYPE.
*
* PARAMETERS:
*   service    - ODB service type;
*   rev        - Revision;
*   fru        - Fru index;
*   man_type   - Dbo type that identifies Db region;
*   addr       - Ptr to Address value to be returned;
*   drive_type - Fru drive type
*   DiskLayout - Select the default or VM Simulation layout.
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*   Value of *addr is set to DBOLB_INVALID_ADDRESS in case of an error;
*
* NOTES:
*    Calling layer should check for the returned value.
*    Use DD_MASTER_DDE_TYPE          -- for identifying MANAGEMENT SPACE
*    Use DD_PREV_REV_MASTER_DDE_TYPE -- for idnetifying PREV REV MANAGEMENT SPACE
*    Use DD_USER_DDE_TYPE            -- for idnetifying USER DB SPACE
*    Use DD_EXTERNAL_USE_DDE_TYPE    -- for idnetifying EXTERNAL USE DB SPACE
*    Use DD_PUBLIC_BIND_TYPE         -- for identifying PUBLIC BIND SPACE
*
*    Returned start address for Management Types that do not represent a 
*    DD DB area should be DBOLB_INVALID_ADDRESS;
*
* HISTORY:
*   09-Aug-00: Created.                               Mike Zelikov (MZ)
*   24-Oct-02: Added argument encl_type               Aparna V
*   05-Jul-05: Changed encl_type to encl_drive_type   Paul Motora
*   23-Apr-08: Added DBConvert error printouts        Gary Peterson (GSP)
*
***********************************************************************/
DD_ERROR_CODE dd_WhatIsDbStartAddr(const DD_SERVICE service,
                                   const DBO_REVISION rev,
                                   const UINT_32 fru,
                                   const DBO_TYPE man_type,
                                   DBOLB_ADDR *addr,
                                   FLARE_DRIVE_TYPE drive_type,
                                   const DISK_LAYOUT_TYPE DiskLayout)
{
    UINT_32 temp_space = 0, index = 0;
    DBOLB_ADDR temp_addr = DBOLB_INVALID_ADDRESS;
    DD_REGION_TYPE region = DD_INVALID_REGION;
    DD_ERROR_CODE error_code = DD_OK;


    odb_assert(addr != NULL);
    *addr = DBOLB_INVALID_ADDRESS;

    switch (service)
    {
    case DD_MAIN_SERVICE :
        if (rev == DD_MS_REVISION_ONE)
        {
            if (!DBO_TYPE_IS_MANAGEMENT(man_type) ||
                (fru == DBO_INVALID_FRU) || 
                (fru >= FRU_COUNT))
            {
#ifdef DBCONVERT_ENV
                CTprint(PRINT_MASK_ERROR,
                        "dd_WhatIsDbStartAddr: Parameter error at line %d",
                        __LINE__);
                CTprint(PRINT_MASK_ERROR,
                        "\tman_type: 0x%x, IsMgmt: %s, fru: 0x%x (0x%x)",
                        man_type,
                        DBO_TYPE_IS_MANAGEMENT(man_type) ? "TRUE" : "FALSE",
                        fru, FRU_COUNT);
#endif
                return  DD_E_INVALID_ARG;
            }

            error_code = dd_WhatIsDataDirectoryStartAddr(service, rev, &temp_addr);
            if (error_code != DD_OK)
            {
#ifdef DBCONVERT_ENV
                CTprint(PRINT_MASK_ERROR,
                        "dd_WhatIsDbStartAddr: error from dd_WhatIsDataDirectoryStartAddr at line %d",
                        __LINE__);
#endif
                break;
            }

                    error_code = dd_DboType2Index(service, rev, man_type, &index);
                    if (error_code != DD_OK)
		    {
#ifdef DBCONVERT_ENV
			CTprint(PRINT_MASK_ERROR,
				"dd_WhatIsDbStartAddr: error from dd_DboType2Index at line %d",
				__LINE__);
#endif
            return (error_code);
		    }

            odb_assert (index < DD_NUM_MANAGEMENT_TYPES);
            odb_assert (dd_ms_man_set_info[index].dbo_type == man_type);

            if (dd_ms_man_set_info[index].is_set != TRUE)
            {
                switch (man_type)
                {
                   case DD_INFO_TYPE :
                        *addr = DBOLB_INVALID_ADDRESS;
                        break;

                    default:
#ifdef DBCONVERT_ENV
                        CTprint(PRINT_MASK_ERROR,
                                "dd_WhatIsDbStartAddr: Invalid man_type (0x%x) at line %d",
                                 man_type, __LINE__);
#endif
                        error_code = DD_E_INVALID_MAN_TYPE;
                }
            }
            else
            {
                switch(man_type)
                {
                    case DD_MASTER_DDE_TYPE :
                    case DD_PREV_REV_MASTER_DDE_TYPE :
                    case DD_USER_DDE_TYPE :
                    case DD_EXTERNAL_USE_DDE_TYPE :
                    case DD_PUBLIC_BIND_TYPE :
                        break;

                    default:
#ifdef DBCONVERT_ENV
                        CTprint(PRINT_MASK_ERROR,
                                "dd_WhatIsDbStartAddr: Invalid man_type (0x%x) at line %d",
                                man_type, __LINE__);
#endif
                        error_code = DD_E_INVALID_MAN_TYPE;
                        return (error_code);
                }

                error_code = dd_ManType2Region(service, rev, man_type, &region);
                if (error_code != DD_OK)
                {
#ifdef DBCONVERT_ENV
                    CTprint(PRINT_MASK_ERROR,
                            "dd_WhatIsDbStartAddr: error from dd_ManType2Region at line %d",
                            __LINE__);
#endif
                    return (error_code);
                }

                /* We start going through the Management Types,
                 * since only these identify entry point to DB
                 * areas of different types.
                 */
                for (index = 0; index < (UINT_32)region; index++)
                {
                    error_code = dd_WhatIsRegionSpace(service, rev, fru,   
                                 index, DiskLayout, &temp_space);
                    if (error_code != DD_OK)                
                    {
#ifdef DBCONVERT_ENV
                        CTprint(PRINT_MASK_ERROR,
                                "dd_WhatIsDbStartAddr: error from dd_WhatIsRegionSpace at line %d",
                                __LINE__);
#endif
                        return (error_code);
                    }

                    temp_addr += temp_space;
                }

                /* User space address must be aligned for some drive types. */
                if (man_type == DD_PUBLIC_BIND_TYPE)
                {
                    if (drive_type == KLONDIKE_ATA_DRIVE_TYPE)
                    {
                        if (temp_addr % DD_KLONDIKE_USER_SPACE_BOUNDARY) 
                        {
                            temp_addr += DD_KLONDIKE_USER_SPACE_BOUNDARY - (temp_addr % DD_KLONDIKE_USER_SPACE_BOUNDARY);
                        }
                    }
                    else if (drive_type == SATA_DRIVE_TYPE ||
                             drive_type == SAS_DRIVE_TYPE ||
                             drive_type == FLASH_SAS_DRIVE_TYPE ||
                             drive_type == FLASH_SATA_DRIVE_TYPE ||
                             drive_type == NL_SAS_DRIVE_TYPE )
                    {
                        if (temp_addr % LOGICAL_MIN_520_BLOCKS_FOR_DRIVES_SUPPORTED)
                        {
                            temp_addr += LOGICAL_MIN_520_BLOCKS_FOR_DRIVES_SUPPORTED - (temp_addr % LOGICAL_MIN_520_BLOCKS_FOR_DRIVES_SUPPORTED);
                        }
                    }
                }

                *addr = temp_addr;
            }
        }
        else
        {
#ifdef DBCONVERT_ENV
            CTprint(PRINT_MASK_ERROR,
                    "dd_WhatIsDbStartAddr: Unsupported rev (0x%x) at line %d",
                    rev, __LINE__);
#endif
            error_code = DD_E_INVALID_REVISION;
        }
        break;

    default:
#ifdef DBCONVERT_ENV
        CTprint(PRINT_MASK_ERROR,
                "dd_WhatIsDbStartAddr: Unsupported service (0x%x) at line %d",
                service, __LINE__);
#endif
        error_code = DD_E_INVALID_SERVICE;
    }

    return (error_code);

} /* dd_WhatIsDbStartAddr() */


/***********************************************************************
*                  dd_WhatIsDataDirectorySpace()
***********************************************************************
*
* DESCRIPTION:
*   *UTILITY FUNCTION*
*    This function returns total DD DB size;
*
* PARAMETERS:
*   service    - ODB service type;
*   rev        - Revision;
*   fru        - Fru index;
*   blk_size   - Ptr to block size to be returned;
*   drive_type - Fru drive type;
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*   Value of *blk_size is set to 0 in case of an error;
*
* NOTES:
*    Calling layer should check for the returned value.
*
* HISTORY:
*   09-Aug-00: Created.                               Mike Zelikov (MZ)
*   24-Oct-02: Added argument encl_type               Aparna V
*   05-Jul-05: Changed encl_type to encl_drive_type   Paul Motora
*   23-Apr-08: Added DBConvert error printouts      Gary Peterson (GSP)
*
***********************************************************************/
DD_ERROR_CODE dd_WhatIsDataDirectorySpace(const DD_SERVICE service,
                                         const DBO_REVISION rev,
                                         const UINT_32 fru,
                                         UINT_32 *blk_size,
                                         FLARE_DRIVE_TYPE drive_type)
{
    DBOLB_ADDR dd_start_addr = DBOLB_INVALID_ADDRESS, 
        public_bind_addr = DBOLB_INVALID_ADDRESS;
    UINT_32 index=0;
    DD_ERROR_CODE error_code = DD_OK;


    odb_assert(blk_size != NULL);
    *blk_size = 0;

    error_code = dd_WhatIsDataDirectoryStartAddr(service, rev, &dd_start_addr);

    if (error_code != DD_OK)
    {
#ifdef DBCONVERT_ENV
	CTprint(PRINT_MASK_ERROR,
		"dd_WhatIsManagementDboCurrentRev: error from dd_WhatIsDataDirectoryStartAddr at line %d",
		__LINE__);
#endif
    return (error_code);
    }

    /* We assume in this implementation that PUBLIC_BIND 
    * area is always the last one!
    */
    error_code = dd_DboType2Index(service, rev, DD_PUBLIC_BIND_TYPE, &index);
    if (error_code != DD_OK)
    {
#ifdef DBCONVERT_ENV
	CTprint(PRINT_MASK_ERROR,
		"dd_WhatIsManagementDboCurrentRev: error from dd_DboType2Index at line %d",
		__LINE__);
#endif
    return (error_code);
    }

    odb_assert(dd_ms_man_set_info[index].dbo_type == DD_PUBLIC_BIND_TYPE);
    odb_assert(index == (DD_NUM_MANAGEMENT_TYPES - 1));

    error_code = dd_WhatIsDbStartAddr(service, rev, fru, 
                                      DD_PUBLIC_BIND_TYPE,
                                      &public_bind_addr,
                                      drive_type,
                                      DefaultLayout);

    if (error_code != DD_OK)
    {
#ifdef DBCONVERT_ENV
	CTprint(PRINT_MASK_ERROR,
		"dd_WhatIsManagementDboCurrentRev: error from dd_WhatIsDbStartAddr at line %d",
		__LINE__);
#endif
    return (error_code);
    }

    *blk_size = (UINT_32) (public_bind_addr - dd_start_addr);

    return (error_code);

} /* dd_WhatIsDataDirectorySpace() */


/* ****** MANAGEMENT SPACE DATA DIRECTORY UTILITY FUNCTIONS ***** */

/***********************************************************************
*                  dd_WhatIsManagementDboCurrentRev()
***********************************************************************
*
* DESCRIPTION:
*   *MANAGEMENT SPACE DD UTILITY FUNCTION*
*   This function returns current Management Dbo Revision.
*
* PARAMETERS:
*   service            - ODB service type;
*   rev                - ODBS revision;
*   man_dbo_type       - Managemnt Dbo Type;
*   man_dbo_rev        - Management Dbo Revision;
*   man_dbo_bpe        - Ptr to BPE to be returned;
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*   Value of *man_dbo_rev is set to DBO_INVALID_REVISION in 
*   case of an error;
*
*   If there is no management entry associated with this type
*   on managemnt space then the rev should be returned 
*   DBO_INVALID_REVISION.
*
* NOTES:
*    Calling layer should check for the returned value.
*
* HISTORY:
*   02-May-01: Created.                               Mike Zelikov (MZ)
*   22-Apr-08: Added DBConvert error printouts      Gary Peterson (GSP)
*
***********************************************************************/
DD_ERROR_CODE dd_WhatIsManagementDboCurrentRev(const DD_SERVICE service, 
                                      const DBO_REVISION rev,
                                      const DBO_TYPE man_dbo_type, 
                                      DBO_REVISION *man_dbo_rev)
{
    UINT_32 index = 0;
    DD_ERROR_CODE error_code = DD_OK;


    odb_assert(man_dbo_rev != NULL);
    odb_assert(DBO_TYPE_IS_MANAGEMENT(man_dbo_type));

    *man_dbo_rev = DBO_INVALID_REVISION;

    switch (service)
    {
    case DD_MAIN_SERVICE :
        if (rev == DD_MS_REVISION_ONE)
        {
            if (DBO_TYPE_IS_MANAGEMENT(man_dbo_type))
            {
                error_code = dd_DboType2Index(service, rev, man_dbo_type, &index);
                if (error_code != DD_OK)
		{
#ifdef DBCONVERT_ENV
		    CTprint(PRINT_MASK_ERROR,
			    "dd_WhatIsManagementDboCurrentRev: error from dd_DboType2Index at line %d",
			    __LINE__);
#endif		
            return (error_code);
		}
                odb_assert(index < DD_NUM_MANAGEMENT_TYPES);
                if (dd_ms_man_set_info[index].dbo_type != man_dbo_type)
                {
#ifdef DBCONVERT_ENV
		    CTprint(PRINT_MASK_ERROR,
			    "dd_WhatIsManagementDboCurrentRev: Dbo Type mismatch at line %d",
			    __LINE__);
		    CTprint(PRINT_MASK_ERROR,
			    "\tmax_set_info[%d].dbo_type: 0x%x,  man_dbo_type: 0x%x",
			    index, dd_ms_man_set_info[index].dbo_type, man_dbo_type);
#endif		
                    return (error_code);
                }

                *man_dbo_rev = dd_ms_man_set_info[index].rev;
            }
            else
            {
#ifdef DBCONVERT_ENV
		CTprint(PRINT_MASK_ERROR,
			"dd_WhatIsManagementDboCurrentRev: Dbo Type (0x%x) Not Management at line %d",
			man_dbo_type, __LINE__);
#endif
                error_code = DD_E_INVALID_MAN_TYPE;
            }
        }
        else
        {
#ifdef DBCONVERT_ENV
	    CTprint(PRINT_MASK_ERROR,
		    "dd_WhatIsManagementDboCurrentRev: Unsupported rev (0x%x) at line %d",
		    rev, __LINE__);
#endif
            error_code = DD_E_INVALID_REVISION;
        }
        break;

    default:
#ifdef DBCONVERT_ENV
	CTprint(PRINT_MASK_ERROR,
		"dd_WhatIsManagementDboCurrentRev: Unsupported service (0x%x) at line %d",
		service, __LINE__);
#endif
        error_code = DD_E_INVALID_SERVICE;
    }

    return (error_code);

} /* dd_WhatIsManagementDboCurrentRev() */


/***********************************************************************
*                  dd_WhatIsManagementDboBpe()
***********************************************************************
*
* DESCRIPTION:
*   *MANAGEMENT SPACE DD UTILITY FUNCTION*
*   This function returns default value for Blocks Per entry for 
*   Management Dbo.
*
* PARAMETERS:
*   service            - ODB service type;
*   rev                - ODBS revision;
*   man_dbo_type       - Managemnt Dbo Type;
*   man_dbo_rev        - Management Dbo Revision;
*   man_dbo_bpe        - Ptr to BPE to be returned;
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*   Value of *man_dbo_bpe is set to 0 in case of an error;
*
*   If there is no management entry associated with this type
*   on managemnt space then the BPE should be returned 0.
*
* NOTES:
*    Calling layer should check for the returned value.
*
* HISTORY:
*   09-Aug-00: Created.                               Mike Zelikov (MZ)
*   22-Apr-08: Added DBConvert error printouts      Gary Peterson (GSP)
*
***********************************************************************/
DD_ERROR_CODE dd_WhatIsManagementDboBpe(const DD_SERVICE service, 
                               const DBO_REVISION rev,
                               const DBO_TYPE man_dbo_type, 
                               const DBO_REVISION man_dbo_rev,
                               UINT_32 *man_dbo_bpe)
{
    UINT_32 dbo_size = 0, index = DBO_INVALID_INDEX;
    DD_ERROR_CODE error_code = DD_OK;


    odb_assert(man_dbo_bpe != NULL);
    odb_assert(DBO_TYPE_IS_MANAGEMENT(man_dbo_type));

    *man_dbo_bpe = 0;

    switch (service)
    {
    case DD_MAIN_SERVICE :
        if (rev == DD_MS_REVISION_ONE)
        {
            if (!DBO_TYPE_IS_MANAGEMENT(man_dbo_type))
            {
#ifdef DBCONVERT_ENV
		CTprint(PRINT_MASK_ERROR,
			"dd_WhatIsManagementDboBpe: DboType 0x%x not a Management type error at line %d",
			man_dbo_type, __LINE__);
#endif 
                return DD_E_INVALID_MAN_TYPE;
            }

            error_code = dd_DboType2Index(service, rev, man_dbo_type, &index);
            if (error_code != DD_OK)
	    {
#ifdef DBCONVERT_ENV
		CTprint(PRINT_MASK_ERROR,
			"dd_WhatIsManagementDboBpe: error from dd_DboType2Index at line %d",
			__LINE__);
#endif 
        return (error_code);
	    }
            odb_assert(index < DD_NUM_MANAGEMENT_TYPES);
            odb_assert(dd_ms_man_set_info[index].dbo_type == man_dbo_type);

            dbo_size = dd_ms_man_set_info[index].dbo_data_size;

            if (dbo_size != 0)
            {
                *man_dbo_bpe = dbo_UserDataCount2DbolbCount(dbo_size);
            }
            else
            {
                if (dd_ms_man_set_info[index].is_dbo == TRUE)
                {
#ifdef DBCONVERT_ENV
		    CTprint(PRINT_MASK_ERROR,
			    "dd_WhatIsManagementDboBpe: IsDbo error (index %d) at line %d",
			    index, __LINE__);   
#endif 
                    return  DD_E_INVALID_MAN_SET_INFO;
                }
                else
                {
                    *man_dbo_bpe = 0;
                }
            }
        }
        else
        {
#ifdef DBCONVERT_ENV
	    CTprint(PRINT_MASK_ERROR,
		    "dd_WhatIsManagementDboBpe: Unknown rev (0x%x) at line %d",
		    rev, __LINE__);   
#endif 
            error_code = DD_E_INVALID_REVISION;
        }
        break;

    default:
#ifdef DBCONVERT_ENV
	CTprint(PRINT_MASK_ERROR,
		"dd_WhatIsManagementDboBpe: Unknown service (0x%x) at line %d",
		service, __LINE__);   
#endif 
        error_code = DD_E_INVALID_SERVICE;
    }

    return (error_code);

} /* dd_WhatIsManagementDboBpe() */


/***********************************************************************
*                  dd_WhatIsManagementDboSpace()
***********************************************************************
*
* DESCRIPTION:
*   *MANAGEMENT SPACE DD UTILITY FUNCTION*
*   This function returns default value for Management Dbo allocated Space
*
* PARAMETERS:
*   service            - ODB service type;
*   rev                - ODBS revision;
*   man_dbo_type       - Managemnt Dbo Type;
*   man_dbo_rev        - Management Dbo Revision;
*   man_dbo_space      - Ptr to space allocated by Dbo to be returned;
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*   Value of *man_space is set to 0 in case of an error;
*
* NOTES:
*    Calling layer should check for the returned value.
*
* HISTORY:
*   09-Aug-00: Created.                               Mike Zelikov (MZ)
*   23-Apr-08: Added DBConvert error printouts      Gary Peterson (GSP)
*
***********************************************************************/
DD_ERROR_CODE dd_WhatIsManagementDboSpace(const DD_SERVICE service, 
                                 const DBO_REVISION rev,
                                 const DBO_TYPE man_dbo_type, 
                                 const DBO_REVISION man_dbo_rev,
                                 UINT_32 *man_dbo_space)
{
    UINT_32 bpe = 0, index = DBO_INVALID_INDEX;
    DD_ERROR_CODE error_code = DD_OK;

    odb_assert(man_dbo_space != NULL);
    odb_assert(DBO_TYPE_IS_MANAGEMENT(man_dbo_type));

    switch (service)
    {
    case DD_MAIN_SERVICE :
        if (rev == DD_MS_REVISION_ONE)
        {
            if (!DBO_TYPE_IS_MANAGEMENT(man_dbo_type))
            {
#ifdef DBCONVERT_ENV
		CTprint(PRINT_MASK_ERROR,
			"dd_WhatIsManagementDboSpace: Dbo Type (0x%x) Not Management at line %d",
			man_dbo_type, __LINE__);
#endif
                return DD_E_INVALID_MAN_TYPE;
            }

            error_code = dd_DboType2Index(service, rev, man_dbo_type, &index);
            if (error_code != DD_OK)
	    {
#ifdef DBCONVERT_ENV
		CTprint(PRINT_MASK_ERROR,
			"dd_WhatIsManagementDboSpace: error from dd_DboType2Index at line %d",
			__LINE__);
#endif		
        return (error_code);
	    }

            odb_assert(index < DD_NUM_MANAGEMENT_TYPES);
            odb_assert(dd_ms_man_set_info[index].dbo_type == man_dbo_type);

            if (dd_ms_man_set_info[index].is_dbo != TRUE)
            {
                switch (man_dbo_type)
                {
                case DD_PREV_REV_MASTER_DDE_TYPE :
                case DD_PUBLIC_BIND_TYPE :
                    if (man_dbo_rev != DBO_INVALID_REVISION)
                    {
#ifdef DBCONVERT_ENV
			CTprint(PRINT_MASK_ERROR,
				"dd_WhatIsManagementDboSpace: Dbo Revision error at line %d",
				__LINE__);
			CTprint(PRINT_MASK_ERROR,
				"\tman_dbo_type: 0x%x, man_dbo_rev: 0x%x",
				man_dbo_type, man_dbo_rev);
#endif
                        error_code = DD_E_INVALID_REVISION;
                    }
                    break;

                default:
#ifdef DBCONVERT_ENV
		    CTprint(PRINT_MASK_ERROR,
			    "dd_WhatIsManagementDboSpace: Invalid Dbo Type (0x%x) error at line %d",
			    man_dbo_type, __LINE__);
#endif
                    error_code = DD_E_INVALID_DBO_TYPE;
                }
            }
            else
            {
                error_code = dd_WhatIsManagementDboBpe(service, rev, man_dbo_type, 
                    man_dbo_rev, &bpe);
                if (error_code != DD_OK)
		{
#ifdef DBCONVERT_ENV
		    CTprint(PRINT_MASK_ERROR,
			    "dd_WhatIsManagementDboSpace: error from dd_WhatIsManagementDboBpe at line %d",
			    __LINE__);
#endif
            return (error_code);
		}

                if (dd_ms_man_set_info[index].num_entries == 0) 
		{
#ifdef DBCONVERT_ENV
		    CTprint(PRINT_MASK_ERROR,
			    "dd_WhatIsManagementDboSpace: Zero Entries error at line %d",
			    __LINE__);
		    CTprint(PRINT_MASK_ERROR,
			    "\tman_dbo_type: 0x%x, index: %d, man_set[index].num_entries: 0x%x",
			    man_dbo_type, index, dd_ms_man_set_info[index].num_entries);
#endif
            return DD_E_INVALID_MAN_SET_INFO; 
		}

                /* Total Size = Bpe * Number of Entries */
                *man_dbo_space = bpe * dd_ms_man_set_info[index].num_entries;
            }
        } /* if (rev == DD_MS_REVISION_ONE) */
        else
        {
#ifdef DBCONVERT_ENV
	    CTprint(PRINT_MASK_ERROR,
		    "dd_WhatIsManagementDboSpace: Unknown rev (0x%x) error at line %d",
		    rev, __LINE__);
#endif 
            error_code = DD_E_INVALID_REVISION;
        }
        break;

    default:
#ifdef DBCONVERT_ENV
	CTprint(PRINT_MASK_ERROR,
		"dd_WhatIsManagementDboSpace: Unknown service (0x%x) error at line %d",
		service, __LINE__);
#endif 
        error_code = DD_E_INVALID_SERVICE;
    }

    return (error_code);

} /* dd_WhatIsManagementDboSpace() */


/***********************************************************************
*                  dd_WhatIsManagementDboAddr()
***********************************************************************
*
* DESCRIPTION:
*   *MANAGEMENT SPACE DD UTILITY FUNCTION*
*    This function returns address of Management Object: MDDE, DD_INFO, ...
*
* PARAMETERS:
*   service    - ODB service type;
*   rev        - ODBS revision;
*   dbo_type   - Object type;
*   flag       - Shows type of the address;
*   addr       - Ptr to Address value to be returned;
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*   Value of *addr is set to DBOLB_INVALID_ADDRESS in case of an error;
*
* NOTES:
*    Calling layer should check for the returned value.
*
* HISTORY:
*   09-Aug-00: Created.                               Mike Zelikov (MZ)
*   22-Apr-08: Added DBConvert error printouts      Gary Peterson (GSP)
*
***********************************************************************/
DD_ERROR_CODE dd_WhatIsManagementDboAddr(const DD_SERVICE service, 
                                const DBO_REVISION rev,
                                const DBO_TYPE dbo_type,
                                const DBO_INDEX flag,
                                DBOLB_ADDR *addr)
{
    DBOLB_ADDR t_addr=DBOLB_INVALID_ADDRESS;
    UINT_32 t_space = 0, man_space = 0;
    UINT_32 index = DBO_INVALID_INDEX, man_dbo_index = DBO_INVALID_INDEX;
    DBO_TYPE t = DBO_INVALID_TYPE;
    DBO_REVISION t_rev = DBO_INVALID_REVISION;
	DD_REGION_TYPE region = DD_INVALID_REGION;
    DD_ERROR_CODE error_code = DD_OK;

    odb_assert(DD_MANAGEMENT_SPACE_REDUNDANCY == 2);

    odb_assert(addr != NULL);
    *addr = DBOLB_INVALID_ADDRESS;

    odb_assert(DBO_TYPE_IS_MANAGEMENT(dbo_type));
    odb_assert((DBO_SECONDARY_INDEX == flag) || (DBO_PRIMARY_INDEX == flag));

    switch (service)
    {
    case DD_MAIN_SERVICE :
        if (rev == DD_MS_REVISION_ONE)
        {
            if (!DBO_TYPE_IS_MANAGEMENT(dbo_type))
            {
#ifdef DBCONVERT_ENV
		CTprint(PRINT_MASK_ERROR,
			"dd_WhatIsManagementDboAddr: DboType 0x%x not a Management type error at line %d",
			dbo_type, __LINE__);
#endif 
                return DD_E_INVALID_MAN_TYPE;
            }

            /* In this function we do not need to analyze the revisons
            * since this function calls other functions, which
            * will check individual revisions.
            */

            /* Get start of Databases -- MDDE start address */
            error_code = dd_WhatIsDataDirectoryStartAddr(service, rev, &t_addr);
            if (error_code != DD_OK)
	    {
#ifdef DBCONVERT_ENV
		CTprint(PRINT_MASK_ERROR,
			"dd_WhatIsManagementDboAddr: error from dd_WhatIsDataDirectoryStartAddr at line %d",
			__LINE__);
#endif 
        return (error_code);
	    }

            /* Get total Managememnt space size */
        error_code = dd_ManType2Region(service, rev, DD_MASTER_DDE_TYPE, &region);
        error_code = dd_WhatIsRegionSpace(service, rev, DBO_INVALID_FRU, region, DefaultLayout, &man_space);

            //error = dd_WhatIsDbSpace(service, rev, DBO_INVALID_FRU, 
            //    DD_MASTER_DDE_TYPE, DefaultLayout, &man_space);

            if (error_code != DD_OK)
	    {
#ifdef DBCONVERT_ENV
		CTprint(PRINT_MASK_ERROR,
			"dd_WhatIsManagementDboAddr: error from dd_WhatIsRegionSpace at line %d",
			__LINE__);
#endif 
        return (error_code);
	    }

            /* We got space size for PRIMARY and SECONDARY copies */
            man_space /= DD_MANAGEMENT_SPACE_REDUNDANCY;

            error_code = dd_DboType2Index(service, rev, dbo_type, &man_dbo_index);
            if (error_code != DD_OK)
	    {
#ifdef DBCONVERT_ENV
		CTprint(PRINT_MASK_ERROR,
			"dd_WhatIsManagementDboAddr: error from dd_DboType2Index at line %d",
			__LINE__);
#endif 
        return (error_code);
	    }

            odb_assert(man_dbo_index < DD_NUM_MANAGEMENT_TYPES);
            odb_assert(dd_ms_man_set_info[man_dbo_index].dbo_type == dbo_type);

            if (dd_ms_man_set_info[man_dbo_index].is_dbo != TRUE)
            {
                switch (dbo_type)
                {
                case DD_PREV_REV_MASTER_DDE_TYPE :
                case DD_PUBLIC_BIND_TYPE :
                    if (dd_ms_man_set_info[man_dbo_index].rev != DBO_INVALID_REVISION)
                    {
#ifdef DBCONVERT_ENV
			CTprint(PRINT_MASK_ERROR,
				"dd_WhatIsManagementDboAddr: Invalid Revision error at line %d",
				__LINE__);
			CTprint(PRINT_MASK_ERROR,
				"\tman_dbo_index: 0x%x, man_set_info[index].rev: 0x%x",
				man_dbo_index,
				dd_ms_man_set_info[man_dbo_index].rev);
#endif 
                        return DD_E_INVALID_REVISION;
                    }
                    break;

                default:
#ifdef DBCONVERT_ENV
		    CTprint(PRINT_MASK_ERROR,
			    "dd_WhatIsManagementDboAddr: Invalid DboType (0x%x) error at line %d",
			    dbo_type, __LINE__);
#endif
                    return DD_E_INVALID_DBO_TYPE;
                }
            }
            else
            {
                if (dd_ms_man_set_info[man_dbo_index].rev == DBO_INVALID_REVISION)
                {
#ifdef DBCONVERT_ENV
		    CTprint(PRINT_MASK_ERROR,
			    "dd_WhatIsManagementDboAddr: Invalid Revision error at line %d",
			    __LINE__);
		    CTprint(PRINT_MASK_ERROR,
			    "\tman_dbo_index: 0x%x, man_set_info[index].rev: 0x%x",
			    man_dbo_index,
			    dd_ms_man_set_info[man_dbo_index].rev);
#endif 
                    return DD_E_INVALID_MAN_SET_INFO;
                }

                /* We start going through the Management Types,
                * since only these identify entry point to DB
                * areas of different types.
                */
                for (index = 0; index < man_dbo_index; index++)
                {
                    error_code = dd_Index2DboType(service, rev,
                        DBO_TYPE_MANAGEMENT_CLASS,
                        index, &t);
                    if (error_code != DD_OK)
		    {
#ifdef DBCONVERT_ENV
			CTprint(PRINT_MASK_ERROR,
				"dd_WhatIsManagementDboAddr: error from dd_Index2DboType at line %d",
				__LINE__);
#endif 
            return (error_code);
		    }

                    odb_assert (dd_ms_man_set_info[index].dbo_type == t);

                    t_rev = dd_ms_man_set_info[index].rev;

                    error_code = dd_WhatIsManagementDboSpace(service, rev, t,
                        t_rev, &t_space);
                    if (error_code != DD_OK)
		    {
#ifdef DBCONVERT_ENV
			CTprint(PRINT_MASK_ERROR,
				"dd_WhatIsManagementDboAddr: error from dd_WhatIsManagementDboSpace at line %d",
				__LINE__);
#endif 
            return (error_code);
		    }

                    t_addr += t_space;
                }

                *addr = (DBO_SECONDARY_INDEX == flag) ?
                    (t_addr + man_space) : t_addr;
            }
        }
        else
        {
#ifdef DBCONVERT_ENV
	    CTprint(PRINT_MASK_ERROR,
		    "dd_WhatIsManagementDboAddr: Unknown rev (0x%x) error at line %d",
		    rev, __LINE__);
#endif 
            error_code = DD_E_INVALID_REVISION;
        }
        break;

    default:
#ifdef DBCONVERT_ENV
	    CTprint(PRINT_MASK_ERROR,
		    "dd_WhatIsManagementDboAddr: Unknown service (0x%x) error at line %d",
		    service, __LINE__);
#endif 
        error_code = DD_E_INVALID_SERVICE;
    }

    return (error_code);

} /* dd_WhatIsManagementDboAddr() */


/***********************************************************************
*                  dd_GetDefaultMasterDde()
***********************************************************************
*
* DESCRIPTION:
*    *MANAGEMENT SPACE DD UTILITY FUNCTION*
*    This function will populate Master DDE with default values.
*
* PARAMETERS:
*    service   - ODBS service type;
*    rev       - Odbs revision;
*    mdde_rev  - Master DDE revision;
*    fru       - Fru for which we are building MDDE;
*    mdde      - Ptr to Master DDE data to be populated;
*    drive_type - Fru drive type
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*
* NOTES:
*    Calling layer should check for the returned value.
*
* HISTORY:
*   09-Aug-00: Created.                               Mike Zelikov (MZ)
*   24-Oct-02: Added argument encl_type               Aparna V
*   05-Jul-05: Changed encl_type to encl_drive_type   Paul Motora
*   22-Apr-08: Added DBConvert error printouts        Gary Peterson
*
***********************************************************************/
DD_ERROR_CODE dd_GetDefaultMasterDde(const DD_SERVICE service,
                                     const DBO_REVISION rev,
                                     const DBO_REVISION mdde_rev,
                                     UINT_32 fru,
                                     MASTER_DDE_DATA *mdde,
                                     FLARE_DRIVE_TYPE drive_type)
{
    UINT_32 i=0, mdde_index=0, index=0;
    DBO_TYPE man_type=DBO_INVALID_TYPE;
    DD_MAN_DATA_TBL_ENTRY *mtbl_entry = NULL;
    DBO_REVISION man_rev = DBO_INVALID_REVISION;
    BOOL error = FALSE;
    DD_ERROR_CODE error_code = DD_OK;

    odb_assert(mdde != NULL);

    switch (service)
    {
    case DD_MAIN_SERVICE :
        if (rev == DD_MS_REVISION_ONE)
        {
            /* Get the type corresponding MDDE index */
            error_code = dd_DboType2Index(service, rev, DD_MASTER_DDE_TYPE, &mdde_index);
            if (error_code != DD_OK)
	    {
#ifdef DBCONVERT_ENV
		CTprint(PRINT_MASK_ERROR,
			"dd_GetDefaultMasterDde: error from dd_DboType2Index at line %d",
			__LINE__);
#endif		
		break;
	    }
            odb_assert (dd_ms_man_set_info[mdde_index].dbo_type == DD_MASTER_DDE_TYPE);

            // Zero the structure
            error = dbo_memset((UINT_8 *)mdde, 0, dd_ms_man_set_info[mdde_index].dbo_data_size);
            if (error)
	    {
#ifdef DBCONVERT_ENV
		CTprint(PRINT_MASK_ERROR,
			"dd_GetDefualtMasterDde: error from dbo_memset -- unable to init memory at line %d",
			__LINE__);
#endif	
        return DD_E_INVALID_MEMSET;
	    }

            // Fill in the initial values
            if (mdde->rev_num == DBO_INVALID_REVISION)
            {
                mdde->rev_num = MASTER_DDE_CURRENT_REVISION;
            }
            else
            {
                mdde->rev_num = mdde_rev;
            }
            mdde->crc               = 0;

            mdde->dd_service        = service;
            mdde->dd_rev            = rev;

            mdde->update_count      = DD_MS_DEFAULT_UPDATE_COUNT;
            mdde->flags             = DD_INVALID_FLAGS;
            mdde->logical_id        = DBO_INVALID_FRU;

            mdde->mtbl_quota        = DD_MAN_DATA_TBL_QUOTA;
            mdde->mtbl_num_entries  = DD_NUM_MANAGEMENT_TYPES;

            mdde->layout_rev        = DD_MS_DEFAULT_LAYOUT_REV;

            odb_assert(DD_MAN_DATA_TBL_QUOTA >= DD_NUM_MANAGEMENT_TYPES);

            /* Fill in the table  */
            for (i=0; (i < DD_NUM_MANAGEMENT_TYPES) && !error; i++)
            {
                /* Get the pointer to MTBL entry */
                mtbl_entry = &(mdde->mtbl[i]);

                /* Get the type corresponding index i */
                error_code = dd_Index2DboType(service, rev,
                    DBO_TYPE_MANAGEMENT_CLASS,
                    i, &man_type);
                if (error_code != DD_OK)
		{
#ifdef DBCONVERT_ENV
		    CTprint(PRINT_MASK_ERROR,
			    "dd_GetDefualtMasterDde: error from dd_IndexDboType (index %d) at line %d",
			    i, __LINE__);
#endif	
		    break;
		}
                odb_assert (dd_ms_man_set_info[i].dbo_type == man_type);

                /* Get the current type revision  */
                man_rev = dd_ms_man_set_info[i].rev;

                /* Fill in the details */
                mtbl_entry->type = man_type;

                error_code = dd_WhatIsManagementDboBpe(service, rev, man_type,
                    man_rev, &(mtbl_entry->bpe));
                if (error_code != DD_OK)
		{
#ifdef DBCONVERT_ENV
		    CTprint(PRINT_MASK_ERROR,
			    "dd_GetDefaultMasterDde: error from dd_WhatIsManagementDboBpe at line %d",
			    __LINE__);
#endif		
		    break;
		}
                mtbl_entry->num_entries = dd_ms_man_set_info[i].num_entries;

                /* If a DBO for this type present we should have num_entries != 0 */
                if (((0 == dd_ms_man_set_info[i].num_entries) &&
                    (TRUE == dd_ms_man_set_info[i].is_dbo)) ||
                    ((0 != dd_ms_man_set_info[i].num_entries) &&
                    (TRUE != dd_ms_man_set_info[i].is_dbo)))
                {
                    error_code = DD_E_INVALID_DBO_TYPE;
#ifdef DBCONVERT_ENV
		    CTprint(PRINT_MASK_ERROR,
			    "dd_GetDefaultMasterDde: num_entires mismatch for DBO type at line %d",
			    __LINE__);
		    CTprint(PRINT_MASK_ERROR,
			    "\tindex: %d, IsDbo: %s, num_entries: 0x%x",
			    i,
			    dd_ms_man_set_info[i].is_dbo ? "TRUE" : "FALSE",
			    dd_ms_man_set_info[i].num_entries);
#endif		
                    break;
                }

                error_code = dd_WhatIsManagementDboAddr(service, rev, man_type,
                    DBO_PRIMARY_INDEX, 
                    &(mtbl_entry->prim_copy));
                if (error_code != DD_OK)
		{
#ifdef DBCONVERT_ENV
		    CTprint(PRINT_MASK_ERROR,
			    "dd_GetDefaultMasterDde: error from dd_WhatIsManagementDboAddr at line %d",
			    __LINE__);
#endif		
		    break;
		}
                error_code = dd_WhatIsManagementDboAddr(service, rev, man_type,
                    DBO_SECONDARY_INDEX, 
                    &(mtbl_entry->sec_copy));
                if (error_code != DD_OK)
		{
#ifdef DBCONVERT_ENV
		    CTprint(PRINT_MASK_ERROR,
			    "dd_GetDefaultMasterDde: error from dd_WhatIsManagementDboAddr at line %d",
			    __LINE__);
#endif				    
		    break;
		}

		//err = dd_ManType2Region(service, rev, DD_USER_DDE_TYPE, &region);
		//error = dd_WhatIsDbSpace(service, rev, fru, DD_USER_DDE_TYPE, DefaultLayout, &(mtbl_entry->region_quota));
	      		
                error_code = dd_WhatIsDbSpace(service, rev, fru,
                    man_type, DefaultLayout, &(mtbl_entry->region_quota));
                if (error_code != DD_OK)
		{
#ifdef DBCONVERT_ENV
		    CTprint(PRINT_MASK_ERROR,
			    "dd_GetDefaultMasterDde: error from dd_WhatIsDbSpace at line %d",
			    __LINE__);
#endif				    
		    break;
		}
				

        error_code = dd_WhatIsDbStartAddr(service, rev, fru,
                                          man_type, &(mtbl_entry->region_addr), drive_type, DefaultLayout);
        if (error_code != DD_OK)
		{
#ifdef DBCONVERT_ENV
		    CTprint(PRINT_MASK_ERROR,
			    "dd_GetDefaultMasterDde: error from dd_WhatIsDbStartAddr at line %d",
			    __LINE__);
#endif				    
		    break;
		}
            }

            /* Now modify entries in the Master DDE according to the revision of the Master DDE */
            switch(mdde->rev_num)
            {
            case MASTER_DDE_REVISION_ONE:
                error_code = dd_DboType2Index(service, rev, 
                    DD_EXTERNAL_USE_DDE_TYPE, 
                    &index);
                odb_assert (!error_code);
                mtbl_entry = &(mdde->mtbl[index]);
                mtbl_entry->num_entries = DD_MS_NUM_EXTERNAL_USE_TYPES_REV_ONE;
                break;

            case MASTER_DDE_REVISION_TWO:
            case MASTER_DDE_REVISION_ONE_HUNDRED:
            default:
                break;
            }
        }
        else
        {
#ifdef DBCONVERT_ENV
	    CTprint(PRINT_MASK_ERROR,
		    "dd_GetDefaultMasterDde: Invalid revision (0x%x) for DD_MAIN_SERVICE at line %d",
		    rev, __LINE__);
#endif 			    
            error_code = DD_E_INVALID_REVISION;
        }
        break;

    default:
#ifdef DBCONVERT_ENV
	CTprint(PRINT_MASK_ERROR,
		"dd_GetDefaultMasterDde: Invalid Service request (0x%x) at line %d",
		service, __LINE__);
#endif 			    
        error_code = DD_E_INVALID_SERVICE;
    }

    if (error_code != DD_OK)
    {
        (void)dbo_memset((UINT_8 *)mdde, 0, dd_ms_man_set_info[mdde_index].dbo_data_size);
    }

    return (error_code);

} /* dd_GetDefaultMasterDde() */


/***********************************************************************
*                  dd_GetDefault_User_ExtUse_SetInfo()
***********************************************************************
*
* DESCRIPTION:
*    *MANAGEMENT SPACE DD UTILITY FUNCTION*
*    This function will populate DDE with default values,
*    or values that are specific to the VM Simulation 
*    layout. 
*
* PARAMETERS:
*    service   - ODBS service type;
*    rev       - Odbs revision;
*    dde_rev   - DDE revision;
*    fru       - fru index;
*    dbo_type  - DDE DBO Type;
*    dde       - DDE to be populated;
*    drive_type - Fru drive type;
*    DiskLayout - Layout to be displayed: default or VM Simulation
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*
* NOTES:
*    Calling layer should check for the returned value. 
*    It's expected that populating the DDE with VM Simulation specific 
*    values will only be done by ddbs_info.exe.
*
* HISTORY:
*   09-Aug-00: Created.                               Mike Zelikov (MZ)
*   29-May-02: Revision 2 support - Disk Swap.        Mike Zelikov (MZ)
*   24-Oct-02: Added argument encl_type               Aparna V
*   05-Jul-05: Changed encl_type to encl_drive_type   Paul Motora
*   22-Apr-08: Added DBConvert error printouts        Gary Peterson
*   11-Jan-12: Added Support for VM Simulation Layout Greg Mogavero
*
***********************************************************************/
DD_ERROR_CODE dd_GetDefault_User_ExtUse_SetInfo(const DD_SERVICE service,
                                                const DBO_REVISION rev,
                                                const DBO_REVISION dde_rev, 
                                                const UINT_32 fru,
                                                const DBO_TYPE dbo_type,
                                                DDE_DATA *dde,
                                                FLARE_DRIVE_TYPE drive_type,
                                                const DBO_REVISION current_dde_rev,
                                                const DISK_LAYOUT_TYPE DiskLayout)
{
    BOOL  set_present = FALSE;
    DD_MS_USER_SET_INFO *user_info_ptr=NULL;
    DD_MS_EXTERNAL_USE_SET_INFO *ext_use_info_ptr=NULL;
    UINT_32 i=0, dde_index = 0, min_fru = DBO_INVALID_FRU, max_fru = DBO_INVALID_FRU;
    UINT_32 dde_data_size = 0;
    DD_ERROR_CODE error_code = DD_OK;


    odb_assert(dde_rev == DDE_REVISION_ONE);
    odb_assert(dde != NULL);
    odb_assert(DDE_CURRENT_REVISION == DDE_REVISION_ONE);

    switch (service)
    {
    case DD_MAIN_SERVICE :
        if (rev == DD_MS_REVISION_ONE) 
        {
            if ((!DBO_TYPE_IS_EXTERNAL_USE(dbo_type) &&
                !(DBO_TYPE_IS_USER(dbo_type))) ||
                (fru >= FRU_COUNT))
            {
#ifdef DBCONVERT_ENV
                CTprint(PRINT_MASK_ERROR,
                    "dd_GetDefault_User_ExtUse_SetInfo: DboType error at line %d",
                    __LINE__);
                CTprint(PRINT_MASK_ERROR,
                    "\tdbo_type: 0x%x, ExtUse: %s, User: %s, Fru: 0x%x (0x%x)",
                    dbo_type,
                    DBO_TYPE_IS_EXTERNAL_USE(dbo_type) ? "TRUE" : "FALSE",
                    DBO_TYPE_IS_USER(dbo_type) ? "TRUE" : "FALSE",
                    fru, FRU_COUNT);		   
#endif
                error_code = DD_E_INVALID_EXT_DBO_TYPE;
                break;
            }

            if (DBO_TYPE_IS_USER(dbo_type))
            {
                error_code = dd_DboType2Index(service, rev, DD_USER_DDE_TYPE, &dde_index);                                          
                if (error_code != DD_OK)
                {
#ifdef DBCONVERT_ENV
                    CTprint(PRINT_MASK_ERROR,
                        "dd_GetDefault_User_ExtUse_SetInfo: error from dd_DboType2Index at line %d",
                        __LINE__);
#endif
                    return (error_code);
                }

                dde_data_size = dd_ms_man_set_info[dde_index].dbo_data_size;
            }
            else
            {
                error_code  = dd_DboType2Index(service, rev, DD_EXTERNAL_USE_DDE_TYPE, &dde_index);
                if (error_code != DD_OK)
                {
#ifdef DBCONVERT_ENV
                    CTprint(PRINT_MASK_ERROR,
                        "dd_GetDefault_User_ExtUse_SetInfo: error from dd_DboType2Index at line %d",
                        __LINE__);
#endif
                    return (error_code);
                }
                dde_data_size = dd_ms_man_set_info[dde_index].dbo_data_size;
            }

            /* Structure size should not be zero */
            if (dde_data_size == 0)
            {
#ifdef DBCONVERT_ENV
                CTprint(PRINT_MASK_ERROR,
                    "dd_GetDefault_User_ExtUse_SetInfo: Structure size zero error at line %d",
                    __LINE__);
#endif
                return (error_code);
            }

            error_code = dbo_memset((UINT_8 *)dde, 0, dde_data_size);
            if (error_code != DD_OK)
            {
#ifdef DBCONVERT_ENV
                CTprint(PRINT_MASK_ERROR,
                    "dd_GetDefault_User_ExtUse_SetInfo: error from dbo_memset at line %d",
                    __LINE__);
#endif
                return (error_code);
            }

            /* Set initial values */
            dde->rev_num      = DBO_INVALID_REVISION;
            dde->crc          = 0;

            dde->update_count = UNSIGNED_MINUS_1;
            dde->type         = DBO_INVALID_TYPE;
            dde->start_addr   = DBOLB_INVALID_ADDRESS;

            /* Invalidate frus in the set */
            dde->num_disks = 0;
            for (i=0; i<MAX_DISK_ARRAY_WIDTH; i++)
            {
                dde->disk_set[i] = DBO_INVALID_FRU;
            }

            error_code = dd_Is_User_ExtUse_SetOnFru(service, rev, fru, dbo_type, &set_present);
            if ((error_code == DD_OK) && set_present)
            {

                error_code = dd_DboType2Index(service, rev, dbo_type, &dde_index);
                if (error_code != DD_OK)
                {
#ifdef DBCONVERT_ENV
                    CTprint(PRINT_MASK_ERROR,
                        "dd_GetDefault_User_ExtUse_SetInfo: error from dd_DboType2Index at line %d",
                        __LINE__);
#endif
                    break;
                }

                if (DBO_TYPE_IS_USER(dbo_type))
                {
                    user_info_ptr = &(dd_ms_user_set_info[dde_index]);
                    odb_assert(user_info_ptr->dbo_type == dbo_type);

                    dde->algorithm   = user_info_ptr->algorithm;
                    dde->num_entries = user_info_ptr->num_entries;
                    dde->info_bpe    = user_info_ptr->info_bpe;

                    min_fru = user_info_ptr->min_fru;
                    max_fru = user_info_ptr->max_fru;

                    if (dde->algorithm == DD_MS_SINGLE_DISK_ALG)
                    {
                        odb_assert((min_fru == DBO_INVALID_FRU) && 
                            (max_fru == DBO_INVALID_FRU));
                    }
                }
                else
                {
                    if (DiskLayout == DefaultLayout)
                    {
                        // Default layout
                        ext_use_info_ptr = dd_GetExtUseSetInfoPtr(dde_index);
                    }
                    else
                    {
                        // VM Simulation Layout
                        ext_use_info_ptr = dd_GetVmSimExtUseSetInfoPtr(dde_index);

                    }

                    odb_assert(ext_use_info_ptr->dbo_type == dbo_type);

                    dde->algorithm   = DD_MS_EXTERNAL_ALG;
                    dde->num_entries = ext_use_info_ptr->blk_size;
                    dde->info_bpe    = 1;

                    min_fru = ext_use_info_ptr->min_fru;
                    max_fru = ext_use_info_ptr->max_fru;

                    if ((dde->type == DD_MS_DISK_DIAGNOSTIC_TYPE) || (dde->type == DD_MS_DISK_DIAGNOSTIC_PAD_TYPE))
                    {
                        odb_assert((min_fru == DBO_INVALID_FRU) && 
                            (max_fru == DBO_INVALID_FRU));
                    }
                }

                dde->rev_num      = dde_rev;
                dde->update_count = DD_MS_DEFAULT_UPDATE_COUNT;
                dde->type         = dbo_type;

                error_code = dd_WhatIsDefault_User_ExtUse_SetRev(service, 
                    rev, 
                    dbo_type,
                    &(dde->info_rev));
                if (error_code != DD_OK)
                {
#ifdef DBCONVERT_ENV
                    CTprint(PRINT_MASK_ERROR,
                        "dd_GetDefault_User_ExtUse_SetInfo: error from dd_WhatIsDefault_User_ExtUse_SetRev at line %d",
                        __LINE__);
#endif
                    break;
                }

                if ((DBO_INVALID_REVISION != current_dde_rev) &&
                    (current_dde_rev > dde->info_rev))
                {
                    dde->info_rev = current_dde_rev;
                }

                error_code = dd_WhatIsDefault_User_ExtUse_SetAddr(service, rev, fru, 
                                                                  dbo_type, &(dde->start_addr), drive_type, DiskLayout);
                if (error_code != DD_OK)
                {
#ifdef DBCONVERT_ENV
                    CTprint(PRINT_MASK_ERROR,
                        "dd_GetDefault_User_ExtUse_SetInfo: error from dd_WhatIsDefault_User_ExtUse_SetAddr at line %d",
                        __LINE__);
#endif
                    break;
                }

                /* Fill in the fru set information */
                if (min_fru == DBO_INVALID_FRU)
                {
                    odb_assert(max_fru == DBO_INVALID_FRU);

                    dde->num_disks = 1;
                    dde->disk_set[0] = fru;
                }
                else
                {
                    odb_assert(max_fru != DBO_INVALID_FRU);

                    dde->num_disks = max_fru - min_fru + 1;
                    odb_assert((dde->num_disks != 0) && (dde->num_disks <= MAX_DISK_ARRAY_WIDTH));

                    for (i=0; i < dde->num_disks; i++)
                    {
                        dde->disk_set[i] = min_fru + i;
                    }
                }

            } /* if (!error && set_present) */
        }
        else
        {
#ifdef DBCONVERT_ENV
            CTprint(PRINT_MASK_ERROR,
                "dd_GetDefault_User_ExtUse_SetInfo: Unsupported rev (0x%x) at line %d",
                rev, __LINE__);
#endif
            error_code = DD_E_INVALID_REVISION;
        }
        break;

    default:
#ifdef DBCONVERT_ENV
        CTprint(PRINT_MASK_ERROR,
            "dd_GetDefault_User_ExtUse_SetInfo: Unsupported service (0x%x) at line %d",
            service, __LINE__);
#endif
        error_code = DD_E_INVALID_SERVICE;
    }

    if (error_code != DD_OK)
    {
        (void)dbo_memset((UINT_8 *)dde, 0, dde_data_size);
    }

    return (error_code);

} /* dd_GetDefault_User_ExtUse_SetInfo() */


/***********************************************************************
*                  dd_WhatIsDefault_User_ExtUse_SetAddr()
***********************************************************************
*
* DESCRIPTION:
*   *MANAGEMENT SPACE DD UTILITY FUNCTION*
*    This function will return the SET's default 
*    or VM Simulation address.
*
* PARAMETERS:
*   service    - ODB service type;
*   rev        - Revision;
*   fru        - Fru index;
*   dbo_type   - DBO TYPE;
*   addr       - Ptr to address to be populated;
*   drive_type - Fru drive type;
*   DiskLayout - Specifies whether we're the SET's default 
*                or VM Simulation address
*
* RETURN VALUES/ERRORS:
*    DD_ERROR_CODE - Error_code;
*    In case of an error *addr is set to DBOLB_INVALID_ADDRESS.
*
* NOTES:
*    Calling layer should check for the returned value.
*
* HISTORY:
*   09-Aug-00: Created.                               Mike Zelikov (MZ)
*   24-Oct-02: Added argument encl_type               Aparna V
*   05-Jul-05: Changed encl_type to encl_drive_type   Paul Motora
*   23-Apr-08: Added DBConvert error printouts        Gary Peterson (GSP)
*   31-Jan-12: Added support for VM Simulatin
*              layout.                                Greg Mogavero
*
***********************************************************************/
DD_ERROR_CODE dd_WhatIsDefault_User_ExtUse_SetAddr(const DD_SERVICE service, 
                                                   const DBO_REVISION rev, 
                                                   const UINT_32 fru, 
                                                   const DBO_TYPE dbo_type,
                                                   DBOLB_ADDR *addr,
                                                   FLARE_DRIVE_TYPE drive_type,
                                                   const DISK_LAYOUT_TYPE DiskLayout)
{
    UINT_32 i=0;
    DBO_TYPE t=DBO_INVALID_TYPE;
    BOOL found = FALSE, set_present = FALSE;
    DBOLB_ADDR temp_addr = DBOLB_INVALID_ADDRESS;
    DD_ERROR_CODE error_code = DD_OK;
    DD_MS_EXTERNAL_USE_SET_INFO  *pExtUseSetInfo;

    odb_assert(addr != NULL);
    *addr = DBOLB_INVALID_ADDRESS;

    switch (service)
    {
    case DD_MAIN_SERVICE :
        if (rev == DD_MS_REVISION_ONE)
        {
            odb_assert(DBO_TYPE_IS_EXTERNAL_USE(dbo_type) || DBO_TYPE_IS_USER(dbo_type));
            odb_assert(fru != DBO_INVALID_FRU);

            error_code = dd_Is_User_ExtUse_SetOnFru(service, rev, fru, dbo_type, &set_present);
            if (error_code != DD_OK)
            {
#ifdef DBCONVERT_ENV
                CTprint(PRINT_MASK_ERROR,
                    "dd_WhatIsDefault_User_ExtUse_SetAddr: error from dd_Is_User_ExtUse_SetOnFru at line %d",
                    __LINE__);
#endif
                break;
            }

            if (set_present)
            {
                if (DBO_TYPE_IS_USER(dbo_type))
                {
                    /* Get starting point of USER DB area */
                    error_code = dd_WhatIsDbStartAddr(service, rev, fru, 
                                                      DD_USER_DDE_TYPE, 
                                                      &temp_addr,
                                                      drive_type,
                                                      DefaultLayout);
                    if (error_code != DD_OK)
                    {
#ifdef DBCONVERT_ENV
                        CTprint(PRINT_MASK_ERROR,
                            "dd_WhatIsDefault_User_ExtUse_SetAddr: error from dd_WhatIsDbStartAddr at line %d",
                            __LINE__);
#endif
                        break;
                    }
                    odb_assert(temp_addr != DBOLB_INVALID_ADDRESS);

                    for (i=0; i<DD_MS_NUM_USER_TYPES; i++)
                    {
                        error_code = dd_Index2DboType(service, rev, 
                            DBO_TYPE_USER_CLASS, 
                            i, &t);
                        if (error_code != DD_OK)
                        {
#ifdef DBCONVERT_ENV
                            CTprint(PRINT_MASK_ERROR,
                                "dd_WhatIsDefault_User_ExtUse_SetAddr: error from dd_Index2DboType at line %d",
                                __LINE__);
#endif
                            break;
                        }
                        odb_assert(dd_ms_user_set_info[i].dbo_type == t);

                        if (t == dbo_type)
                        {
                            odb_assert(dd_ms_user_set_info[i].dbo_type == dbo_type);
                            found = TRUE;
                            break;
                        }
                        else
                        {
                            /* Every USER space is allocated on each FRU */
                            temp_addr += (dd_ms_user_set_info[i].num_entries * 
                                dd_ms_user_set_info[i].info_bpe);
                        }
                    }                         
                }
                else
                {
                    /* Get starting point of EXTERNAL USE DB area */
                    error_code = dd_WhatIsDbStartAddr(service, rev, fru,
                                                      DD_EXTERNAL_USE_DDE_TYPE, 
                                                      &temp_addr,
                                                      drive_type, 
                                                      DefaultLayout);
                    if (error_code != DD_OK)
                    {
#ifdef DBCONVERT_ENV
                        CTprint(PRINT_MASK_ERROR,
                            "dd_WhatIsDefault_User_ExtUse_SetAddr: error from dd_WhatIsDbStartAddr at line %d",
                            __LINE__);
#endif
                        break;
                    }

                    odb_assert(temp_addr != DBOLB_INVALID_ADDRESS);

                    for (i=0; i<DD_MS_NUM_EXTERNAL_USE_TYPES; i++)
                    {
                        error_code = dd_Index2DboType(service, rev, 
                            DBO_TYPE_EXTERNAL_USE_CLASS, 
                            i, &t);
                        if (error_code != DD_OK)
                        {
#ifdef DBCONVERT_ENV
                            CTprint(PRINT_MASK_ERROR,
                                "dd_WhatIsDefault_User_ExtUse_SetAddr: error from dd_Index2DboType at line %d",
                                __LINE__);
#endif
                            break;
                        }

                        if (DiskLayout == DefaultLayout)
                        {
                            // Default layout
                            pExtUseSetInfo = dd_GetExtUseSetInfoPtr(i);
                        }
                        else
                        {
                            // VM Simulation Layout
                            pExtUseSetInfo = dd_GetVmSimExtUseSetInfoPtr(i);
                        }

                        odb_assert(pExtUseSetInfo->dbo_type == t);

                        if (t == dbo_type)
                        {
                            odb_assert(pExtUseSetInfo->dbo_type == t);                            
                            found = TRUE;
                            break;
                        }
                        else
                        {
                            /* Not every EXTERNAL USE set space is present on each FRU */
                            error_code = dd_Is_User_ExtUse_SetOnFru(service, rev, fru,
                                t, &set_present);
                            if (error_code != DD_OK)
                            {
#ifdef DBCONVERT_ENV
                                CTprint(PRINT_MASK_ERROR,
                                    "dd_WhatIsDefault_User_ExtUse_SetAddr: error from dd_Is_User_ExtUse_SetOnFru at line %d",
                                    __LINE__);
#endif
                                break;
                            }

                            if (set_present)
                            {
                                /* Since bpe is always 1, use size directly */
                                if (DiskLayout == DefaultLayout)
                                {
                                    // Default layout
                                    pExtUseSetInfo = dd_GetExtUseSetInfoPtr(i);
                                }
                                else
                                {
                                    // VM Simulation Layout
                                    pExtUseSetInfo = dd_GetVmSimExtUseSetInfoPtr(i);
                                }
                                temp_addr += pExtUseSetInfo->blk_size; 
                            }
                        }
                    }
                } /* else on if (set_present) */

                if (error_code == DD_OK)
                {
                    if (!found)
                    {
#ifdef DBCONVERT_ENV
                        CTprint(PRINT_MASK_ERROR,
                            "dd_WhatIsDefault_User_ExtUse_SetAdde: DboType 0x%x not found at line %d",
                            dbo_type, __LINE__);
#endif
                        error_code = DD_E_INVALID_EXT_DBO_TYPE;
                    }
                    else
                    {
                        *addr = temp_addr;
                    }
                }
            }
            else
            {
                /* This set is not present on this FRU */
                *addr = DBOLB_INVALID_ADDRESS;
            }
        }
        else
        {
#ifdef DBCONVERT_ENV
            CTprint(PRINT_MASK_ERROR,
                "dd_WhatIsDefault_User_ExtUse_SetAddr: unknown rev (0x%x) at line %d",
                rev, __LINE__);
#endif
            error_code = DD_E_INVALID_REVISION;
        }
        break;

    default:
#ifdef DBCONVERT_ENV
        CTprint(PRINT_MASK_ERROR,
            "dd_WhatIsDefault_User_ExtUse_SetAddr: unknown service (0x%x) at line %d",
            service, __LINE__);
#endif
        error_code = DD_E_INVALID_SERVICE;
    }

    return (error_code);

} /* dd_WhatIsDefault_User_ExtUse_SetAddr() */


/***********************************************************************
*                  dd_ChooseMasterDde()
***********************************************************************
*
* DESCRIPTION:
*   *MANAGEMENT SPACE DD UTILITY FUNCTION*
*    This function verifies copies of Master DDE and choose the correct 
*    one.
*
* PARAMETERS:
*   service           - ODB service type;
*   rev               - ODBS revision;
*   mdde_buf          - MDDE data to be filled in;
*   fru               - Fru from which data was read;
*   mdde_raw_arr      - Array of pointers to RAW MDDE data from the disk;
*   mdde_addr_arr     - Array of Master DDE addresses;
*   validity_mask_ptr - Ptr to Address value to be returned;
*   drive_type        - Fru  drive type
*   current_mdde_rev  - Current revision of MDDE
*   mdde_rev_ptr      - Pointer to Revision of MDDE which was validated
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*   Validity mask set to DBO_ALL_COPIES_INVALID in the case of an
*   error.
*
* NOTES:
*    Calling layer should check for the returned value.
*    Even that we pass service and revision arguments, this information
*    is stored in MDDE itself. If at least one valid MDDE found
*    the caller can should use values directly from the MDDE data.
*
* WARNING:
*    There is a parallel function (dd_ChooseValidMasterDde) which performs
*    a similar function, but is called by the DBConvert (Database Conversions,
*    run in the Utility Partition) utility. Any fixes that are made there may
*    also need to be made there.
*
* HISTORY:
*   08-Aug-01: Created.                               Mike Zelikov (MZ)
*   24-Oct-02: Added argument encl_type               Aparna V
*   05-Jul-05: Changed encl_type to encl_drive_type   Paul Motora
*
***********************************************************************/
DD_ERROR_CODE dd_ChooseMasterDde(const DD_SERVICE service, 
                                 const DBO_REVISION rev,
                                 MASTER_DDE_DATA *mdde_buf,
                                 const UINT_32 fru,
                                 const UINT_32 blk_count,
                                 UINT_8E *mdde_raw_arr[DD_NUM_MASTER_DDE_COPIES],
                                 DBOLB_ADDR mdde_addr_arr[DD_NUM_MASTER_DDE_COPIES],
                                 UINT_32 *validity_mask_ptr,
                                 FLARE_DRIVE_TYPE drive_type,
                                 const DBO_REVISION current_mdde_rev)
{
    DD_SERVICE dd_service=UNSIGNED_MINUS_1;
    BOOL error=FALSE, zero_mark[DD_NUM_MASTER_DDE_COPIES]={FALSE, FALSE}, status=FALSE;
    UINT_32 val_mask=DBO_ALL_COPIES_INVALID, i=0;
    DBO_REVISION mdde_rev=DBO_INVALID_REVISION, dd_rev=DBO_INVALID_REVISION;
    UINT_32 logical_id=DBO_INVALID_FRU, mzm_size=0, count=0, layout_id=DBO_INVALID_FRU;
    UINT_32 update_count=0, mtbl_index=0, crc=0;
    MASTER_DDE_DATA *curr_mdde=NULL, *mdde_arr[DD_NUM_MASTER_DDE_COPIES]={NULL, NULL};
    DD_MAN_ZERO_MARK mzm;
    DBO_TYPE dbo_type = DBO_INVALID_TYPE;
    DBO_ID dbo_id = DBO_INVALID_ID;
    BOOL use_default_values = FALSE;
    DD_ERROR_CODE error_code = DD_OK;

    odb_assert(DD_NUM_MASTER_DDE_COPIES == 2);

    odb_assert(mdde_buf != NULL);
    odb_assert(validity_mask_ptr != NULL);
    odb_assert(mdde_addr_arr != NULL);
    odb_assert(mdde_raw_arr != NULL);
    odb_assert(blk_count == 1);


    /* ************ STEP 0: Prepare the data */
    *validity_mask_ptr = DBO_ALL_COPIES_INVALID;
    /*
    *status_ptr = FALSE;
    */
    error = dbo_memset((UINT_8E *)mdde_buf, 0, MASTER_DDE_DATA_SIZE);
    if (error) { return DD_E_INVALID_MEMSET; }

    if ((dd_ms_man_set_info[DD_MAIN_SERVICE].dbo_type != DD_MASTER_DDE_TYPE) ||
        (dd_ms_man_set_info[DD_MAIN_SERVICE].dbo_data_size != MASTER_DDE_DATA_SIZE))
    {
        return DD_E_INVALID_MDDE;
    }
    /* ************ END of STEP 0 */


    /* ************ STEP 1: Convert data from RAW DBOLB format to MDDE DATA */
    for (i=0; i< DD_NUM_MASTER_DDE_COPIES; i++)
    {
        if (mdde_raw_arr[i] != NULL)
        {
            count = blk_count;
            error_code = dbo_DbolbData2UserData(mdde_raw_arr[i],
                &count,
                mdde_addr_arr[i],
                &dbo_type,
                &dbo_id,
                &status);
#if defined(LAB_DEBUG) || defined(UMODE_ENV)
            if((odbs_dd_rb_test_flag == ODBS_DD_RB_TEST_CASE_1) && 
                (fru == odbs_dd_rb_test_target_fru ))
            {
                error_code = DD_E_INVALID_MDDE;
            }
#endif 
            if (error_code != DD_OK)
            {
                return (error_code);
            }

            if (status != TRUE) 
            { 
                /* Invalidate this copy since the data was bad */
                mdde_arr[i] = NULL;
            }
            else
            {
#if defined(LAB_DEBUG) || defined(UMODE_ENV)
                if((odbs_dd_rb_test_flag == ODBS_DD_RB_TEST_CASE_2) && 
                    (fru == odbs_dd_rb_test_target_fru))
                {
                    dbo_type = DD_PUBLIC_BIND_TYPE;
                }
#endif
                if ((dbo_type != DD_MASTER_DDE_TYPE) ||
                    ((!DD_ID_IS_MASTER_DDE(&dbo_id)) &&
                    (!DD_ID_IS_MZM(&dbo_id))))
                { 
                    /* The data was good  --  We expect MASTER DDE
                    * with MDDE data or MZM data.
                    * !!!MZ -- In general we should not return error in this case
                    *          but rather set:
                    *          mdde_arr[i] = NULL;
                    */
                    
                    return DD_E_INVALID_MDDE; 
                }

                mdde_arr[i] = (MASTER_DDE_DATA *) mdde_raw_arr[i];

                if (DD_ID_IS_MZM(&dbo_id))
                {
                    zero_mark[i] = TRUE;
                }
                else
                {
                    zero_mark[i] = FALSE;
                }
            }
        }
        else
        {
            mdde_arr[i]  = NULL;
            zero_mark[i] = FALSE;
        }
    } 
    /* ************ END of STEP 1 */


    /* ************ STEP 2: Verify service, revision number, crc32, update count ... */
    /* NOTE: We do not take PREV REV pointer into considiratrion
    *       This needs to be changed once we support DownGrades. (!!!MZ)
    */
    for (i=0; i<DD_NUM_MASTER_DDE_COPIES; i++)
    {
        if ((curr_mdde = mdde_arr[i]) != NULL)
        {
            /* Notice that update_count is 0, we never update
            * anything on the disk with the update count 0,
            * so if we discover a bad data we will continue
            * with invalid update_count set!
            */

            if (TRUE == zero_mark[i])
            {
                /* Create manufacture zero mark data for the comparison */
                error_code = dd_CreateManufactureZeroMark(service, rev,
                    curr_mdde->rev_num,
                    (UINT_8E *)&mzm,
                    &mzm_size);
          #ifdef DBCONVERT_ENV
                CTprint(PRINT_MASK_ERROR,
                        "dd_CreateManufactureZeroMark() returned = 0x%x \n",error_code);
         #endif
                
                if (error_code != DD_OK) 
                {
                    return (error_code); 
                }
                odb_assert(mzm_size == DD_MAN_ZERO_MARK_SIZE);

                /* Check CRC */
                error_code = dd_CrcManagementData((UINT_8E *)curr_mdde,
                    mzm_size,
                    &crc);
                if (error_code != DD_OK) 
                {
                    return (error_code); 
                }

                if ((curr_mdde->crc ^= crc) != 0)
                {
                    /* CRC does not match - corrupted data! */
                    mdde_arr[i]  = NULL;
                    zero_mark[i] = FALSE;
                    continue;
                }

                /* Check the data */
                error = dbo_memcmp(curr_mdde, &mzm, mzm_size, &status);
                if (error) 
                {
                    return DD_E_INVALID_MEMSET; 
                }

                if (status != TRUE)
                {
                    /* Oops -- we have good CRC/rev but invalid data??? */
                    
                    return DD_E_INVALID_MDDE;
                }

                /* We found VALID Manufacture Zero Mark */
                zero_mark[i] = TRUE;
                mdde_arr[i] = NULL;
            }
            else /* If not ZERO MARK */
            {

                error_code = dd_CrcManagementData((UINT_8E *)curr_mdde, 
                    MASTER_DDE_DATA_SIZE,
                    &crc);
                if (error_code != DD_OK) 
                {
                    return(error_code); 
                }

                if ((curr_mdde->crc ^= crc) != 0)
                {
                    /* CRC does not macth! */
                    mdde_arr[i] = NULL;
                    continue;
                }

                /* Check DATA DIRECTORY Service:
                * if service created MDDE does not match
                * the caller we consider the data to be
                * invalid.
                */
                if (curr_mdde->dd_service != (UINT_32)service)
                {
                    /* !!!MZ - We do not recognize service created this data */
                    mdde_arr[i] = NULL;
                    continue;
                }

                if (i == DBO_PRIMARY_INDEX)
                {
                    /* Set the initilal values for future comparison */
                    dd_rev       = curr_mdde->dd_rev;
                    update_count = curr_mdde->update_count;
                    logical_id   = curr_mdde->logical_id;
                }
                else
                {
                    odb_assert(i == DBO_SECONDARY_INDEX);

                    /* First check the Data Directory revision */
                    if ((mdde_arr[DBO_PRIMARY_INDEX] != NULL) &&
                        (curr_mdde->dd_rev != dd_rev))
                    {
                        if (curr_mdde->dd_rev > dd_rev)
                        {
                            /* Invalidate primary copy */
                            mdde_arr[DBO_PRIMARY_INDEX] = NULL;
                        }
                        else
                        {
                            /* Invalidate secondary (this) copy */
                            mdde_arr[DBO_SECONDARY_INDEX] = NULL;
                        }
                    }

                    if ((mdde_arr[DBO_PRIMARY_INDEX] != NULL) &&
                        (mdde_arr[DBO_SECONDARY_INDEX] != NULL))
                    {
                        /* Notice that if PRIMARY copy is invalid
                        * update_count will be 0, and the smallest
                        * valid update count is 1.
                        */
                        if (curr_mdde->update_count == update_count)
                        {
                            odb_assert(update_count != 0);

                            if (logical_id != curr_mdde->logical_id)
                            {
                                /* We have the same revision and
                                 * the same update count, but
                                 * different logical_id --> ERROR
                                 * But wait a minute: DIMS186480/184075. I will save you from the panic
                                 * however you need to pay for it by experiencing a rebuild. Though I can pretty much
                                 * help you skip most of the rebuild cases, I don't want to do it.
                                 */
                                // Invalidate both copies.
                                mdde_arr[DBO_PRIMARY_INDEX] = NULL;
                                mdde_arr[DBO_SECONDARY_INDEX] = NULL;
                            }
                        }
                        else if (curr_mdde->update_count > update_count)
                        {
                            /* Invalidate primary copy */
                            mdde_arr[DBO_PRIMARY_INDEX] = NULL;
                        }
                        else
                        {
                            mdde_arr[DBO_SECONDARY_INDEX] = NULL;
                        }
                    }
                } /* else on if Primary copy */
            } /* else on if MZM data */
        } /* if (curr_mdde != NULL) */
        else
        {
            zero_mark[i] = FALSE;
        }
    } 
    /* ************ END of STEP 2 */


    /* ************ STEP 3: Set the Data Directory service, revision; update_count */
    /* Notice that if both copies are not NULL then
    * dd_service, dd_rev, mdde_rev, update_count should be the same
    */
    if ((mdde_arr[DBO_PRIMARY_INDEX] != NULL) &&
        (mdde_arr[DBO_SECONDARY_INDEX] != NULL))
    {
        odb_assert((mdde_arr[DBO_PRIMARY_INDEX]->rev_num == 
            mdde_arr[DBO_SECONDARY_INDEX]->rev_num) &&
            (mdde_arr[DBO_PRIMARY_INDEX]->dd_service == 
            mdde_arr[DBO_SECONDARY_INDEX]->dd_service) &&
            (mdde_arr[DBO_PRIMARY_INDEX]->dd_rev == 
            mdde_arr[DBO_SECONDARY_INDEX]->dd_rev) &&
            (mdde_arr[DBO_PRIMARY_INDEX]->logical_id == 
            mdde_arr[DBO_SECONDARY_INDEX]->logical_id) &&
            (mdde_arr[DBO_PRIMARY_INDEX]->update_count == 
            mdde_arr[DBO_SECONDARY_INDEX]->update_count));
    }

    dd_service = service;

    use_default_values = FALSE;

    if (mdde_arr[DBO_PRIMARY_INDEX] != NULL)
    {
        mdde_rev     = mdde_arr[DBO_PRIMARY_INDEX]->rev_num;
        dd_rev       = mdde_arr[DBO_PRIMARY_INDEX]->dd_rev;
        logical_id   = mdde_arr[DBO_PRIMARY_INDEX]->logical_id;
    }
    else if (mdde_arr[DBO_SECONDARY_INDEX] != NULL)
    {
        mdde_rev     = mdde_arr[DBO_SECONDARY_INDEX]->rev_num;
        dd_rev       = mdde_arr[DBO_SECONDARY_INDEX]->dd_rev;
        logical_id   = mdde_arr[DBO_SECONDARY_INDEX]->logical_id;
    }
    else
    {
        use_default_values = TRUE;
    }

    if ((DBO_INVALID_FRU != logical_id) &&
        (logical_id >= FRU_COUNT))
    {
        // This drive might have come from a bigger system with a
        // larger number of drives. Rebuild the drive.
        mdde_arr[DBO_PRIMARY_INDEX] = NULL;
        mdde_arr[DBO_SECONDARY_INDEX] = NULL;
	use_default_values = TRUE;
    }

    if (mdde_rev > MASTER_DDE_CURRENT_REVISION)
    {
        /* We do not know about the layout for mdde_rev */
        use_default_values = TRUE;
    }

    if (use_default_values)
    {
        /* Use default values */
        error_code = dd_WhatIsManagementDboCurrentRev(service, rev,
            DD_MASTER_DDE_TYPE,
            &mdde_rev);
        #ifdef DBCONVERT_ENV
                CTprint(PRINT_MASK_ERROR,
                        "dd_WhatIsManagementDboCurrentRev() returned = 0x%x \n",error_code);
        #endif
        
        if (error_code != DD_OK)
        {
            return(error_code);
        }

        dd_rev = rev;
        logical_id = DBO_INVALID_FRU;
    }

    /* !!!MZ - We must have the caller revision 
    * compatible with the revision on the disk.
    */
    odb_assert(dd_rev == rev);
    /*
    if (dd_rev > rev)
    {
    *status_ptr = FALSE;
    return (FALSE);
    }
    */
    /* ************ END of STEP 3 */


    /* ************ STEP 4: Get the default MDDE for comparison */
    if (logical_id == DBO_INVALID_FRU)
    {
        /* This fru is not HOT SPARE */
        layout_id = logical_id = fru;
    }
    else
    {
        /* It is a HOT SPARE - use logical_id as the "real" fru */
        layout_id = logical_id;
    }

    /* Notice that we use dd_service and dd_rev obtained 
    * from the MDDE (if any).
    */
    error_code = dd_GetDefaultMasterDde(dd_service, dd_rev,
                                        mdde_rev, logical_id, mdde_buf, drive_type);
    #ifdef DBCONVERT_ENV
                CTprint(PRINT_MASK_ERROR,
                             "dd_GetDefaultMasterDde() returned = 0x%x \n",error_code);
    #endif
    
    if (error_code != DD_OK) 
    {
        return (error_code); 
    }

    /* ************ END of STEP 4 */


    /* ************ STEP 5: Verify passed addresses */
    error_code = dd_DboType2Index(dd_service, dd_rev, DD_MASTER_DDE_TYPE, &mtbl_index);
    #ifdef DBCONVERT_ENV
                CTprint(PRINT_MASK_ERROR,
                             "dd_DboType2Index() returned = 0x%x \n",error_code);
    #endif
    odb_assert(0 == mtbl_index);

    if ((error_code != DD_OK) ||
        (mdde_buf->mtbl[mtbl_index].prim_copy != mdde_addr_arr[DBO_PRIMARY_INDEX]) ||
        (mdde_buf->mtbl[mtbl_index].sec_copy != mdde_addr_arr[DBO_SECONDARY_INDEX]))
    {
        /* Addresses do not match */
        (void) dbo_memset((UINT_8E *)mdde_buf, 0, MASTER_DDE_DATA_SIZE);
        return (error_code);
    }
    /* ************ END of STEP 5 */


    /* ************ STEP 6: Compare the values */
    for (i=0; i<DD_NUM_MASTER_DDE_COPIES; i++)
    {
        if ((curr_mdde = mdde_arr[i]) != NULL)
        {
            if ((fru <= DD_MS_MAX_VAULT_FRU) &&
                (curr_mdde->logical_id != DBO_INVALID_FRU) &&
                (curr_mdde->logical_id != fru))
            {
                /* VAULT fru can not have layout of some other fru */
                mdde_arr[i] = NULL;
                continue;
            }

            /* Place the default value for logical_id, update_count, and zero mark */
            logical_id   = curr_mdde->logical_id;
            update_count = curr_mdde->update_count;

            curr_mdde->logical_id   = mdde_buf->logical_id;
            curr_mdde->update_count = mdde_buf->update_count;

            mdde_buf->flags = curr_mdde->flags;

#ifndef ODB_FLARE_ENVIRONMENT
            // The disk_type is invalid when we are compiled into DDBS (for POST), NTMIRROR, or ASIDC. 
            // As the start of the user bind area differs based on the disk type, we must take the start
            // from the MDDE we are to verify, and not use the default MDDE value.

            {
                UINT_32     bind_region_index;

                if (dd_DboType2Index(service, rev, DD_PUBLIC_BIND_TYPE, &bind_region_index) == DD_OK)
                {
                    mdde_buf->mtbl[bind_region_index].region_addr = curr_mdde->mtbl[bind_region_index].region_addr;
                }
            }
#endif

            /*odb_assert((curr_mdde->flags == DD_INVALID_FLAGS) ||
            (curr_mdde->flags == DD_ZERO_MARK_FLAG));*/

            error = dbo_memcmp(curr_mdde, mdde_buf, MASTER_DDE_DATA_SIZE, &status);

#if defined(LAB_DEBUG) || defined(UMODE_ENV)
            if((odbs_dd_rb_test_flag == ODBS_DD_RB_TEST_CASE_5) && 
                (fru == odbs_dd_rb_test_target_fru))
            {
                error = TRUE;
            }
#endif 
            if (error) { return DD_E_INVALID_MEMSET; }

            if (status == TRUE)
            {
                if (i == 0)
                {
                    DBO_SET_COPY_VALID(val_mask, DBO_PRIMARY_INDEX);
                }
                else
                {
                    odb_assert(i == DBO_SECONDARY_INDEX);
                    DBO_SET_COPY_VALID(val_mask, DBO_SECONDARY_INDEX);
                }

                /* Restore the logical_id and update_count values */
                curr_mdde->logical_id   = logical_id;
                curr_mdde->update_count = update_count;
            }
            else
            {
                mdde_arr[i] = NULL;
            }
        }
    } 
    /* ************ END of STEP 6 */


    /* ************ STEP 7: Get the valid MDDE */
    if (val_mask != DBO_ALL_COPIES_INVALID)
    {
        odb_assert((mdde_arr[DBO_PRIMARY_INDEX] != NULL) ||
            (mdde_arr[DBO_SECONDARY_INDEX] != NULL));

        /* Verify that we have identical flags */
        if (DBO_IS_COPY_VALID(val_mask, DBO_PRIMARY_INDEX) &&
            DBO_IS_COPY_VALID(val_mask, DBO_SECONDARY_INDEX))
        {
#if defined(LAB_DEBUG) || defined(UMODE_ENV)
            if((odbs_dd_rb_test_flag == ODBS_DD_RB_TEST_CASE_6) && 
                (fru == odbs_dd_rb_test_target_fru))
            {
                return DD_E_INVALID_MDDE;
            }
#endif 
            odb_assert(mdde_arr[DBO_PRIMARY_INDEX]->flags ==
                mdde_arr[DBO_SECONDARY_INDEX]->flags);
        }

        for (i=0; i<DD_NUM_MASTER_DDE_COPIES; i++)
        {
            if (DBO_IS_COPY_VALID(val_mask, i))
            {
                odb_assert(mdde_arr[i] != NULL);
                odb_assert(!zero_mark[i]);

#if defined(LAB_DEBUG) || defined(UMODE_ENV)
                if((odbs_dd_rb_test_flag == ODBS_DD_RB_TEST_CASE_8) && 
                    (fru == odbs_dd_rb_test_target_fru))
                {
                    mdde_arr[i]->flags |= DD_ZERO_MARK_FLAG;
                    (mdde_arr[i]->update_count)++;
                }
#endif 
                if ((mdde_arr[i]->flags & DD_ZERO_MARK_FLAG) != 0)
                {
                    odb_assert(mdde_arr[i]->update_count == 
                        DD_MS_DEFAULT_UPDATE_COUNT);
                }

                *mdde_buf = *mdde_arr[i];
            }
            else
            {
                odb_assert(mdde_arr[i] == NULL);
            }
        }
    }
    else
    {
        if (layout_id != fru)
        {
            /* We have created the default structure for 
            * incorrect fru: using logical_id from mdde that was 
            * invalid in the first place. Create the default info for 
            * the passed fru.
            */
            error_code = dd_GetDefaultMasterDde(service, rev,
                                                mdde_rev, fru, mdde_buf, drive_type);
            if (error_code != DD_OK) 
            {
                return (error_code); 
            }
        }

        /* Check zero mark */
        if (zero_mark[DBO_PRIMARY_INDEX] ||
            zero_mark[DBO_SECONDARY_INDEX])
        {
            /* Disk was zeroed  */
            mdde_buf->flags |= DD_ZERO_MARK_FLAG;
        }
    }
    /* ************ END of STEP 7 */

    /* Check if new layout has been committed AV 10/15/02 */
    if ( (fru >= DD_MS_VAULT_RESERVED_MIN_FRU) &&
        (fru <= DD_MS_VAULT_RESERVED_MAX_FRU) )
    {
        dd_read_layout_on_disk = TRUE;
        if (DBO_IS_COPY_VALID(val_mask, DBO_PRIMARY_INDEX))
        {
            curr_mdde = mdde_arr[DBO_PRIMARY_INDEX];
        }
        else if (DBO_IS_COPY_VALID(val_mask, DBO_SECONDARY_INDEX))
        {
            curr_mdde = mdde_arr[DBO_SECONDARY_INDEX];
        }
        else
        {
            curr_mdde = NULL;
        }
    }

    /* ************ STEP 8: Set the return validity mask */
    *validity_mask_ptr = val_mask;

    return (error_code);

} /* dd_ChooseMasterDde() */


/***********************************************************************
*                  dd_ChooseValidMasterDde()
***********************************************************************
*
* DESCRIPTION:
*   *MANAGEMENT SPACE DD UTILITY FUNCTION*
*    This function verifies copies of Master DDE and choose the correct 
*    one.
*
* PARAMETERS:
*   service           - ODB service type;
*   rev               - ODBS revision;
*   mdde_buf          - MDDE data to be filled in;
*   fru               - Fru from which data was read;
*   mdde_raw_arr      - Array of pointers to RAW MDDE data from the disk;
*   mdde_addr_arr     - Array of Master DDE addresses;
*   validity_mask_ptr - Ptr to Address value to be returned;
*   drive_type        - Fru drive type
*   current_mdde_rev  - Current revision of MDDE
*   mdde_rev_ptr      - Pointer to Revision of MDDE which was validated
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*   Validity mask set to DBO_ALL_COPIES_INVALID in the case of an
*   error.
*
* NOTES:
*    Calling layer should check for the returned value.
*    Even that we pass service and revision arguments, this information
*    is stored in MDDE itself. If at least one valid MDDE found
*    the caller can should use values directly from the MDDE data.
*
*    This function is the same as dd_ChooseMasterDde(), but is less
*    restrictive. It is intended to eventually replace 
*    dd_ChooseMasterDde().
*
*    This function is NOT called from Flare, at present, only DBConvert
*    (Flare Database Conversions) uses it. (DBConvert is run in the
*    Utility Partition, which is outside the Flare stack).
*
* HISTORY:
*   
*   26-Aug-05: Created from dd_ChooseMasterDde               Aparna V
*   22-Apr-08: Added DBConvert error printouts          Gary Peterson
*
***********************************************************************/
DD_ERROR_CODE dd_ChooseValidMasterDde(const DD_SERVICE service, 
                                      const DBO_REVISION rev,
                                      MASTER_DDE_DATA *mdde_buf,
                                      const UINT_32 fru,
                                      const UINT_32 blk_count,
                                      UINT_8E *mdde_raw_arr[DD_NUM_MASTER_DDE_COPIES],
                                      DBOLB_ADDR mdde_addr_arr[DD_NUM_MASTER_DDE_COPIES],
                                      UINT_32 *validity_mask_ptr,
			                          BOOL check_crc_only,
                                      FLARE_DRIVE_TYPE drive_type)
{
    DD_SERVICE dd_service=UNSIGNED_MINUS_1;
    BOOL error=FALSE, zero_mark[DD_NUM_MASTER_DDE_COPIES]={FALSE, FALSE}, status=FALSE;
    UINT_32 val_mask=DBO_ALL_COPIES_INVALID, i=0;
    DBO_REVISION mdde_rev=DBO_INVALID_REVISION, dd_rev=DBO_INVALID_REVISION;
    UINT_32 logical_id=DBO_INVALID_FRU, mzm_size=0, count=0, layout_id=DBO_INVALID_FRU;
    UINT_32 update_count=0, crc=0;
    MASTER_DDE_DATA *curr_mdde=NULL, *mdde_arr[DD_NUM_MASTER_DDE_COPIES]={NULL, NULL};
    DD_MAN_ZERO_MARK mzm;
    DBO_TYPE dbo_type = DBO_INVALID_TYPE;
    DBO_ID dbo_id = DBO_INVALID_ID;
    BOOL use_default_values = FALSE;
    UINT_32 mdde_size;
    DD_ERROR_CODE error_code = DD_OK;

    /* ************ STEP 0: Prepare the data */
    *validity_mask_ptr = DBO_ALL_COPIES_INVALID;

    error = dbo_memset((UINT_8E *)mdde_buf, 0, MASTER_DDE_DATA_SIZE);
    if (error) 
    { 
#ifdef DBCONVERT_ENV
	CTprint(PRINT_MASK_ERROR,
		"dd_ChooseValidMasterDde: error from dbo_memset -- unable to init memory");
#endif	
    return DD_E_INVALID_MEMSET; 
    }

    if ((dd_ms_man_set_info[DD_MAIN_SERVICE].dbo_type != DD_MASTER_DDE_TYPE) ||
        (dd_ms_man_set_info[DD_MAIN_SERVICE].dbo_data_size != MASTER_DDE_DATA_SIZE))
    {
#ifdef DBCONVERT_ENV
	CTprint(PRINT_MASK_ERROR,
		"dd_ChooseValidMasterDde: Step 0 - DBO type/size mismatch error");
	CTprint(PRINT_MASK_ERROR,
		"\tExpected Type: 0x%x\tActual Type: 0x%x",
		DD_MASTER_DDE_TYPE,
		dd_ms_man_set_info[DD_MAIN_SERVICE].dbo_type);
	CTprint(PRINT_MASK_ERROR,
		"\tExpected Size: 0x%x\tActual Size: 0x%x",
		MASTER_DDE_DATA_SIZE,
		dd_ms_man_set_info[DD_MAIN_SERVICE].dbo_data_size);
#endif	
        return DD_E_INVALID_DBO_TYPE;
    }
    /* ************ END of STEP 0 */


    /* ************ STEP 1: Convert data from RAW DBOLB format to MDDE DATA */
    for (i=0; i< DD_NUM_MASTER_DDE_COPIES; i++)
    {
        if (mdde_raw_arr[i] != NULL)
        {
            count = blk_count;
            error = dbo_DbolbData2UserData(mdde_raw_arr[i],
					   &count,
					   mdde_addr_arr[i],
					   &dbo_type,
					   &dbo_id,
					   &status);
            if (error) 
	    {
#ifdef DBCONVERT_ENV
		CTprint(PRINT_MASK_ERROR,
			"dd_ChooseValidMasterDde: Step 1 - error from dbo_DbolbData2UserData at line %d",
			__LINE__);
#endif
                return DD_E_INVALID_MDDE;
            }

            if (status != TRUE) 
            { 
                /* Invalidate this copy since the data was bad */
                mdde_arr[i] = NULL;
            }
            else
            {
                if ((dbo_type != DD_MASTER_DDE_TYPE) ||
                    ((!DD_ID_IS_MASTER_DDE(&dbo_id)) &&
		     (!DD_ID_IS_MZM(&dbo_id))))
                { 
                    mdde_arr[i] = NULL;
                    zero_mark[i] = FALSE;
                }
                else
                {
                    mdde_arr[i] = (MASTER_DDE_DATA *) mdde_raw_arr[i];

                    if (DD_ID_IS_MZM(&dbo_id))
                    {
                        zero_mark[i] = TRUE;
                    }
                    else
                    {
                        zero_mark[i] = FALSE;
                    }
                }
            }
        }
        else
        {
            mdde_arr[i]  = NULL;
	    DBO_SET_COPY_INVALID(val_mask, i);
            zero_mark[i] = FALSE;
        }
    } 
    /* ************ END of STEP 1 */


    /* ************ STEP 2: Verify service, revision number, crc32, update count ... */
    /* NOTE: We do not take PREV REV pointer into considiratrion
     *       This needs to be changed once we support DownGrades. (!!!MZ)
     */
    for (i=0; i<DD_NUM_MASTER_DDE_COPIES; i++)
    {
        if ((curr_mdde = mdde_arr[i]) != NULL)
        {
            /* Notice that update_count is 0, we never update
	     * anything on the disk with the update count 0,
	     * so if we discover a bad data we will continue
	     * with invalid update_count set!
	     */

            if (TRUE == zero_mark[i])
            {
                /* Create manufacture zero mark data for the comparison */
                error_code = dd_CreateManufactureZeroMark(service, rev,
						     curr_mdde->rev_num,
						     (UINT_8E *)&mzm,
						     &mzm_size);
                if (error_code != DD_OK) 
                {
#ifdef DBCONVERT_ENV
		    CTprint(PRINT_MASK_ERROR,
			    "dd_ChooseValidMasterDde: Step 2 - error from dd_CreateManufactureZeroMark at line %d",
			    __LINE__);
#endif
                    return (error_code); 
                }
                odb_assert(mzm_size == DD_MAN_ZERO_MARK_SIZE);

                /* Check CRC */
                error_code = dd_CrcManagementData((UINT_8E *)curr_mdde,
					     mzm_size,
					     &crc);
                if (error_code != DD_OK) 
                {
#ifdef DBCONVERT_ENV
		    CTprint(PRINT_MASK_ERROR,
                    "dd_ChooseValidMasterDde: Step 2 - error from dd_CrcManagementData at line 0x%x \n",
                    error_code);
#endif
                    return (error_code); 
                }

                if ((curr_mdde->crc ^= crc) != 0)
                {
                    /* CRC does not match - corrupted data! */
                    mdde_arr[i]  = NULL;
		    DBO_SET_COPY_INVALID(val_mask, i);
                    zero_mark[i] = FALSE;
                    continue;
                }

                /* Check the data */
                error = dbo_memcmp(curr_mdde, &mzm, mzm_size, &status);
                if (error) 
                {
#ifdef DBCONVERT_ENV
		    CTprint(PRINT_MASK_ERROR,
			    "dd_ChooseValidMasterDde: Step 2 - error from dbo_memcmp at line %d",
			    __LINE__);
#endif
                    return DD_E_INVALID_MEMSET; 
                }

                if (status != TRUE)
                {
                    /* Oops -- we have good CRC/rev but invalid data??? */
                    
#ifdef DBCONVERT_ENV
		    CTprint(PRINT_MASK_ERROR,
			    "dd_ChooseValidMasterDde: Step 2 error - good CRC, but invalid DDE data");
#endif
                    return DD_E_INVALID_DDE_TYPE;
                }

                /* We found VALID Manufacture Zero Mark */
                zero_mark[i] = TRUE;
                mdde_arr[i] = NULL;
		DBO_SET_COPY_INVALID(val_mask, i);
            }
            else /* If not ZERO MARK */
            {

		if ((check_crc_only) &&
		    ((curr_mdde->rev_num == MASTER_DDE_REVISION_ONE) ||
		     (curr_mdde->rev_num == MASTER_DDE_REVISION_TWO) ||
                     (curr_mdde->rev_num == MASTER_DDE_REVISION_ONE_HUNDRED)))
		{
		    mdde_size = NT_FISH_MASTER_DDE_DATA_SIZE;
		}
		else
		{
		    mdde_size = MASTER_DDE_DATA_SIZE;
		}

                error_code = dd_CrcManagementData((UINT_8E *)curr_mdde, 
					     mdde_size,
					     &crc);
                if (error_code != DD_OK)
		{ 
#ifdef DBCONVERT_ENV
		    CTprint(PRINT_MASK_ERROR,
			    "dd_ChooseValidMasterDde: Step 2 - error from dd_CrcManagementData at line %d",
			    __LINE__);
#endif
            return (error_code); 
		}

                if ((curr_mdde->crc ^= crc) != 0)
                {
                    /* CRC does not macth! */
                    mdde_arr[i] = NULL;
		    DBO_SET_COPY_INVALID(val_mask, i);
                    continue;
                }
		else
		{
                    if ((check_crc_only) &&
                        ((curr_mdde->rev_num == MASTER_DDE_REVISION_ONE) ||
			 (curr_mdde->rev_num == MASTER_DDE_REVISION_TWO) ||
			 (curr_mdde->rev_num == MASTER_DDE_REVISION_ONE_HUNDRED)))
                    {
                        error_code = dd_FormatMDDE(mdde_arr[i]);
                        if (error_code != DD_OK)
			{
#ifdef DBCONVERT_ENV
			    CTprint(PRINT_MASK_ERROR,
				    "dd_ChooseValidMasterDde: Step 2 - error from dd_FormatMDDE at line %d",
				    __LINE__);
#endif
                return (error_code);
			}
                    }

		    DBO_SET_COPY_VALID(val_mask, i);
		}

                /* Check DATA DIRECTORY Service:
		 * if service created MDDE does not match
		 * the caller we consider the data to be
		 * invalid.
		 */
                if (curr_mdde->dd_service != (UINT_32)service)
                {
                    /* !!!MZ - We do not recognize service created this data */
                    mdde_arr[i] = NULL;
		    DBO_SET_COPY_INVALID(val_mask, i);
                    continue;
                }

                if (i == DBO_PRIMARY_INDEX)
                {
                    /* Set the initial values for future comparison */
                    dd_rev       = curr_mdde->dd_rev;
                    update_count = curr_mdde->update_count;
                    logical_id   = curr_mdde->logical_id;
                }
                else
                {
                    odb_assert(i == DBO_SECONDARY_INDEX);

                    /* First check the Data Directory revision */
                    if ((mdde_arr[DBO_PRIMARY_INDEX] != NULL) &&
                        (curr_mdde->dd_rev != dd_rev))
                    {
                        if (curr_mdde->dd_rev > dd_rev)
                        {
                            /* Invalidate primary copy */
                            mdde_arr[DBO_PRIMARY_INDEX] = NULL;
			    DBO_SET_COPY_INVALID(val_mask, DBO_PRIMARY_INDEX);
                        }
                        else
                        {
                            /* Invalidate secondary (this) copy */
                            mdde_arr[DBO_SECONDARY_INDEX] = NULL;
			    DBO_SET_COPY_INVALID(val_mask, DBO_SECONDARY_INDEX);
                        }
                    }

                    if ((mdde_arr[DBO_PRIMARY_INDEX] != NULL) &&
                        (mdde_arr[DBO_SECONDARY_INDEX] != NULL))
                    {
                        /* Notice that if PRIMARY copy is invalid
			 * update_count will be 0, and the smallest
			 * valid update count is 1.
			 */
                        if (curr_mdde->update_count == update_count)
                        {
                            odb_assert(update_count != 0);

                            if (logical_id != curr_mdde->logical_id)
                            {
                                /* We have the same revision and
				 * the same update count, but
				 * different logical_id --> ERROR
				 */
#ifdef DBCONVERT_ENV
				CTprint(PRINT_MASK_ERROR,
					"dd_ChooseValidMasterDde: Step 2 - logical id mismatch error");
				CTprint(PRINT_MASK_ERROR,
					"\tMDDE[0]: 0x%x\tMDDE[1]: 0x%x",
					logical_id,
					curr_mdde->logical_id);
#endif
                                return DD_E_INVALID_MDDE;
                            }
                        }
                        else if (curr_mdde->update_count > update_count)
                        {
                            /* Invalidate primary copy */
                            mdde_arr[DBO_PRIMARY_INDEX] = NULL;
			    DBO_SET_COPY_INVALID(val_mask, DBO_PRIMARY_INDEX);
                        }
                        else
                        {
                            mdde_arr[DBO_SECONDARY_INDEX] = NULL;
			    DBO_SET_COPY_INVALID(val_mask, DBO_SECONDARY_INDEX);
                        }
                    }
                } /* else on if Primary copy */
            } /* else on if MZM data */
        } /* if (curr_mdde != NULL) */
        else
        {
            zero_mark[i] = FALSE;
        }
    } 
    /* ************ END of STEP 2 */


    /* ************ STEP 3: Set the Data Directory service, revision; update_count */
    /* Notice that if both copies are not NULL then
     * dd_service, dd_rev, mdde_rev, update_count should be the same
     */
    if ((mdde_arr[DBO_PRIMARY_INDEX] != NULL) &&
        (mdde_arr[DBO_SECONDARY_INDEX] != NULL))
    {
#ifdef DBCONVERT_ENV
        if ((mdde_arr[DBO_PRIMARY_INDEX]->rev_num != mdde_arr[DBO_SECONDARY_INDEX]->rev_num) ||
	    (mdde_arr[DBO_PRIMARY_INDEX]->dd_service != mdde_arr[DBO_SECONDARY_INDEX]->dd_service) ||
	    (mdde_arr[DBO_PRIMARY_INDEX]->dd_rev != mdde_arr[DBO_SECONDARY_INDEX]->dd_rev) ||
	    (mdde_arr[DBO_PRIMARY_INDEX]->logical_id != mdde_arr[DBO_SECONDARY_INDEX]->logical_id) ||
	    (mdde_arr[DBO_PRIMARY_INDEX]->update_count != mdde_arr[DBO_SECONDARY_INDEX]->update_count))
	{
	    CTprint(PRINT_MASK_ERROR,
		    "dd_ChooseValidMasterDde: Step 3 - MDDE mismatch error");
	    CTprint(PRINT_MASK_ERROR,
		    "\trev_num\t\tMDDE[0]: 0x%8x\tMDDE[1]: 0x%8x",
		    mdde_arr[DBO_PRIMARY_INDEX]->rev_num,
		    mdde_arr[DBO_SECONDARY_INDEX]->rev_num);		    
	    CTprint(PRINT_MASK_ERROR,
		    "\tdd_service\tMDDE[0]: 0x%8x\tMDDE[1]: 0x%8x",
		    mdde_arr[DBO_PRIMARY_INDEX]->dd_service,
		    mdde_arr[DBO_SECONDARY_INDEX]->dd_service);
	    CTprint(PRINT_MASK_ERROR,
		    "\tdd_rev\t\tMDDE[0]: 0x%8x\tMDDE[1]: 0x%8x",
		    mdde_arr[DBO_PRIMARY_INDEX]->dd_rev,
		    mdde_arr[DBO_SECONDARY_INDEX]->dd_rev);
	    CTprint(PRINT_MASK_ERROR,
		    "\tlogical_id\tMDDE[0]: 0x%8x\tMDDE[1]: 0x%8x",
		    mdde_arr[DBO_PRIMARY_INDEX]->logical_id,
		    mdde_arr[DBO_SECONDARY_INDEX]->logical_id);
	    CTprint(PRINT_MASK_ERROR,
		    "\tupdate_cnt\tMDDE[0]: 0x%8x\tMDDE[1]: 0x%8x",
		    mdde_arr[DBO_PRIMARY_INDEX]->update_count,
		    mdde_arr[DBO_SECONDARY_INDEX]->update_count);
	}
#endif
        odb_assert(mdde_arr[DBO_PRIMARY_INDEX]->rev_num == mdde_arr[DBO_SECONDARY_INDEX]->rev_num);
	odb_assert(mdde_arr[DBO_PRIMARY_INDEX]->dd_service == mdde_arr[DBO_SECONDARY_INDEX]->dd_service);
	odb_assert(mdde_arr[DBO_PRIMARY_INDEX]->dd_rev == mdde_arr[DBO_SECONDARY_INDEX]->dd_rev);
	odb_assert(mdde_arr[DBO_PRIMARY_INDEX]->logical_id == mdde_arr[DBO_SECONDARY_INDEX]->logical_id);
	odb_assert(mdde_arr[DBO_PRIMARY_INDEX]->update_count == mdde_arr[DBO_SECONDARY_INDEX]->update_count);
    }

    dd_service = service;

    use_default_values = FALSE;

    if (mdde_arr[DBO_PRIMARY_INDEX] != NULL)
    {
        mdde_rev     = mdde_arr[DBO_PRIMARY_INDEX]->rev_num;
        dd_rev       = mdde_arr[DBO_PRIMARY_INDEX]->dd_rev;
        logical_id   = mdde_arr[DBO_PRIMARY_INDEX]->logical_id;
    }
    else if (mdde_arr[DBO_SECONDARY_INDEX] != NULL)
    {
        mdde_rev     = mdde_arr[DBO_SECONDARY_INDEX]->rev_num;
        dd_rev       = mdde_arr[DBO_SECONDARY_INDEX]->dd_rev;
        logical_id   = mdde_arr[DBO_SECONDARY_INDEX]->logical_id;
    }
    else
    {
        use_default_values = TRUE;
    }

    if ((DBO_INVALID_FRU != logical_id) &&
        (logical_id >= FRU_COUNT))
    {
        // This drive might have come from a bigger system with a
        // larger number of drives. Rebuild the drive.
        mdde_arr[DBO_PRIMARY_INDEX] = NULL;
	DBO_SET_COPY_INVALID(val_mask, DBO_PRIMARY_INDEX);
        mdde_arr[DBO_SECONDARY_INDEX] = NULL;
	DBO_SET_COPY_INVALID(val_mask, DBO_SECONDARY_INDEX);
        use_default_values = TRUE;
    }

    if (use_default_values)
    {
        /* Use values of the caller */
        error_code = dd_WhatIsManagementDboCurrentRev(service, rev,
						 DD_MASTER_DDE_TYPE,
						 &mdde_rev);
        if (error_code != DD_OK) {
#ifdef DBCONVERT_ENV
	    CTprint(PRINT_MASK_ERROR,
		    "dd_ChooseValidMasterDde: Step 3 - error from dd_WhatIsManagementDboCurentRev at line %d",
		    __LINE__);
#endif
            return (error_code); 
        }

        dd_rev = rev;
        logical_id = DBO_INVALID_FRU;
    }

    /* !!!MZ - We must have the caller revision 
     * compatible with the revision on the disk.
     */
    odb_assert(dd_rev == rev);

    /* ************ END of STEP 3 */


    /* ************ STEP 4: Get the valid MDDE */
    if (val_mask != DBO_ALL_COPIES_INVALID)
    {
        odb_assert((mdde_arr[DBO_PRIMARY_INDEX] != NULL) ||
		   (mdde_arr[DBO_SECONDARY_INDEX] != NULL));

        /* Verify that we have identical flags */
        if (DBO_IS_COPY_VALID(val_mask, DBO_PRIMARY_INDEX) &&
            DBO_IS_COPY_VALID(val_mask, DBO_SECONDARY_INDEX))
        {
            /*db_assert(mdde_arr[DBO_PRIMARY_INDEX]->flags ==
	      mdde_arr[DBO_SECONDARY_INDEX]->flags);*/
        }

        for (i=0; i<DD_NUM_MASTER_DDE_COPIES; i++)
        {
            if (DBO_IS_COPY_VALID(val_mask, i))
            {
                odb_assert(mdde_arr[i] != NULL);
                odb_assert(!zero_mark[i]);

                if ((mdde_arr[i]->flags & DD_ZERO_MARK_FLAG) != 0)
                {
                    odb_assert(mdde_arr[i]->update_count == 
			       DD_MS_DEFAULT_UPDATE_COUNT);
                }

                *mdde_buf = *mdde_arr[i];
            }
            else
            {
                odb_assert(mdde_arr[i] == NULL);
            }
        }
    }
    else
    {
        if (layout_id != fru)
        {
            /* We have created the default structure for 
             * incorrect fru: using logical_id from mdde that was 
             * invalid in the first place. Create the default info for 
             * the passed fru.
             */
            error_code = dd_GetDefaultMasterDde(service, rev,
                                                mdde_rev, fru, mdde_buf, drive_type);
            if (error_code != DD_OK) 
            {
#ifdef DBCONVERT_ENV
		CTprint(PRINT_MASK_ERROR,
			"dd_ChooseValidMasterDde: Step 4 - error from dd_GetDefaultMasterDde at line %d",
			__LINE__);
#endif
                return(error_code); 
            }
        }

        /* Check zero mark */
        if (zero_mark[DBO_PRIMARY_INDEX] ||
            zero_mark[DBO_SECONDARY_INDEX])
        {
            /* Disk was zeroed  */
            mdde_buf->flags |= DD_ZERO_MARK_FLAG;
        }
    }
    /* ************ END of STEP 4 */

    /* ************ STEP 5: Set the return validity mask */
    *validity_mask_ptr = val_mask;
    /*
     *status_ptr = TRUE;
     */
    /* ************ END of STEP 5 */

    return (error_code);

} /* dd_ChooseValidMasterDde() */


/***********************************************************************
*                                  dd_FormatMDDE
***********************************************************************
*
* DESCRIPTION:
*   *MANAGEMENT SPACE DD UTILITY FUNCTION*
*
* PARAMETERS:
*   mdde_ptr - Ptr to MDDE;
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*   Validity mask set to DBO_ALL_COPIES_INVALID in the case of an
*   error.
*
* NOTES:
*
* HISTORY:
*
*   22-Apr-08: Added DBConvert error printouts      Gary Peterson (GSP)
*
***********************************************************************/
DD_ERROR_CODE dd_FormatMDDE(MASTER_DDE_DATA *mdde_ptr)
{
    NT_FISH_MASTER_DDE_DATA *nt_fish_mdde_ptr;
    MASTER_DDE_DATA temp_mdde;
    UINT_32 index = 0;
    DD_ERROR_CODE error_code = DD_OK;

    nt_fish_mdde_ptr = (NT_FISH_MASTER_DDE_DATA *)mdde_ptr;

    temp_mdde.rev_num = nt_fish_mdde_ptr->rev_num;
    temp_mdde.dd_service = nt_fish_mdde_ptr->dd_service;
    temp_mdde.dd_rev = nt_fish_mdde_ptr->dd_rev;
    temp_mdde.update_count = nt_fish_mdde_ptr->update_count;
    temp_mdde.flags = nt_fish_mdde_ptr->flags;
    temp_mdde.logical_id = nt_fish_mdde_ptr->logical_id;
    temp_mdde.mtbl_quota = nt_fish_mdde_ptr->mtbl_quota;
    temp_mdde.mtbl_num_entries = nt_fish_mdde_ptr->mtbl_num_entries;
    temp_mdde.layout_rev = DD_MS_DEFAULT_LAYOUT_REV;
   
    for (index = 0; index < DD_MAN_DATA_TBL_QUOTA; index++)
    {
        temp_mdde.mtbl[index].type = nt_fish_mdde_ptr->mtbl[index].type;
        temp_mdde.mtbl[index].bpe = nt_fish_mdde_ptr->mtbl[index].bpe;
        temp_mdde.mtbl[index].num_entries = nt_fish_mdde_ptr->mtbl[index].num_entries;
        temp_mdde.mtbl[index].prim_copy = (UINT_32E)(nt_fish_mdde_ptr->mtbl[index].prim_copy);
        temp_mdde.mtbl[index].sec_copy = (UINT_32E)(nt_fish_mdde_ptr->mtbl[index].sec_copy);
        temp_mdde.mtbl[index].region_quota = nt_fish_mdde_ptr->mtbl[index].region_quota;
        temp_mdde.mtbl[index].region_addr = (UINT_32E)(nt_fish_mdde_ptr->mtbl[index].region_addr);
    }

    error_code = dd_CrcManagementData((UINT_8E *)(&temp_mdde), 
                    NT_FISH_MASTER_DDE_DATA_SIZE,
                    &(temp_mdde.crc));
    if (error_code != DD_OK)
    {
#ifdef DBCONVERT_ENV
	CTprint(PRINT_MASK_ERROR,
            "dd_ForamtMDDE: error from dd_CrcManagementData at line 0x%x \n",
            error_code);
#endif 
    return(error_code); 
    }

    *mdde_ptr = temp_mdde;

    return DD_OK;

} // dd_FormatMDDE()

/***********************************************************************
*                  dd_Choose_User_ExtUse_Dde()
***********************************************************************
*
* DESCRIPTION:
*   *MANAGEMENT SPACE DD UTILITY FUNCTION*
*    This function verifies copies of USER/EXT USE DDE and choose the 
*    correct ne.
*
* PARAMETERS:
*   service            - ODB service type;
*   rev                - ODBS revision;
*   dbo_type           - USER or EXT USE dbo type;
*   dde_buf            - USER DDE data to be filled in;
*   fru                - Fru from which data was read;
*   dde_raw_arr        - Array of pointers to obtained Master DDEs raw DBOLB data;
*   dde_addr_arr       - Array of DDE addresses;
*   validity_mask_ptr  - Ptr to Address value to be returned;
*   dd_swap_ptr        - Ptr to fru position where this disk came from.
*   drive_type         - Fru drive type
*   current_dde_rev    - Current revision of DDE
*   dde_rev_ptr        - Pointer to revision of DDE which was validated
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*   Validity mask set to DBO_ALL_COPIES_INVALID in the case of an
*   error.
*
* NOTES:
*    If a buffer is NULL pointer we do not look at address and blk_count
*    values -- One can pass all buffers as NULL ptrs and get the default 
*    DDE.
*
*    The caller should pass service/rev obtained from MDDE on the fru 
*    not running values of ODB service when parsing data from a live fru.
*
*    Calling layer should check for the returned value.
*
* HISTORY:
*   08-Aug-01: Created.                               Mike Zelikov (MZ)
*   24-Oct-02: Added argument encl_type               Aparna V
*   05-Jul-05: Changed encl_type to encl_drive_type   Paul Motora
*   23-Apr-08: Added DBConvert error printouts        Gary Peterson
*
***********************************************************************/
DD_ERROR_CODE dd_Choose_User_ExtUse_Dde(const DD_SERVICE service,
                                        const DBO_REVISION rev,
                                        const DBO_TYPE dbo_type,
                                        DDE_DATA *dde_buf,
                                        const UINT_32 fru,
                                        const UINT_32 blk_count,
                                        UINT_8E *dde_raw_arr[DD_NUM_DDE_COPIES],
                                        DBOLB_ADDR dde_addr_arr[DD_NUM_DDE_COPIES],
                                        UINT_32 *validity_mask_ptr,
                                        UINT_32 *dd_swap_ptr,
                                        FLARE_DRIVE_TYPE drive_type,
                                        const DBO_REVISION current_dde_rev)
{
    BOOL error = FALSE, status = FALSE, set_present=FALSE;
    UINT_32 val_mask=DBO_ALL_COPIES_INVALID, i=0, crc=0, count=0, update_count=0;
    DBO_REVISION dde_rev=DBO_INVALID_REVISION, dde_info_rev = DBO_INVALID_REVISION;
    DDE_DATA *curr_dde=NULL, *dde_arr[DD_NUM_DDE_COPIES]={NULL, NULL};
    DBO_TYPE dde_type = DBO_INVALID_TYPE;
    DBO_ID dbo_id = DBO_INVALID_ID;
    DD_ERROR_CODE error_code = DD_OK;


    /* This function supports DDE_REVISION_ONE only */
    odb_assert(DD_NUM_DDE_COPIES == 2);

    odb_assert(dde_buf != NULL);
    odb_assert(validity_mask_ptr != NULL);
    odb_assert(dde_raw_arr != NULL);
    odb_assert(DBO_TYPE_IS_USER(dbo_type) || DBO_TYPE_IS_EXTERNAL_USE(dbo_type));

    /* ************ STEP 0: Prepare the data */
    *validity_mask_ptr = DBO_ALL_COPIES_INVALID;
    *dd_swap_ptr = DBO_INVALID_FRU;
    /*
    *status_ptr = FALSE;
    */
    error = dbo_memset((UINT_8E *)dde_buf, 0, DDE_DATA_SIZE);
    if (error) 
    { 
        MP_ktracev("ODBS: dd_Choose_User_ExtUse_Dde: dbo_memset returned error (0x%x), line %d\n", error, __LINE__);
        return DD_E_INVALID_MEMSET; 
    }
    /* ************ END of STEP 0 */


    switch (service)
    {
    case DD_MAIN_SERVICE :
        if (rev == DD_MS_REVISION_ONE)
        {
            /* ************ STEP 1: Initial validations */
            dde_type = ((DBO_TYPE_IS_USER(dbo_type)) ? DD_USER_DDE_TYPE : DD_EXTERNAL_USE_DDE_TYPE);

            error_code = dd_DboType2Index(service, rev, dde_type, &i);
            if (error_code != DD_OK)
            {
                MP_ktracev("ODBS: dd_Choose_User_ExtUse_Dde: dd_DboType2Index error 0x%x type 0x%x rev 0x%x, line %d\n", error_code, dde_type, rev, __LINE__);
                return (error_code);
            }

            odb_assert(dd_ms_man_set_info[i].dbo_type == dde_type);

            if (dd_ms_man_set_info[i].dbo_data_size != DDE_DATA_SIZE)
            {
                MP_ktracev("ODBS: dd_Choose_User_ExtUse_Dde: dbo_data_size mismatch, line %d\n", __LINE__);
                MP_ktracev("\tdbo_type: 0x%x, dde_type: 0x%x, index: %d, man[i].data_size: %d,\n", 
                           dbo_type, dde_type, i, dd_ms_man_set_info[i].dbo_data_size);
                MP_ktracev("\texpected size %llu)\n",
			   (unsigned long long)DDE_DATA_SIZE);

                return DD_E_INVALID_MAN_SET_INFO;
            }
            /* ************ END of STEP 1 */


            /* ************ STEP 2: Convert data from RAW DBOLB format to DDE DATA */
            for (i=0; i< DD_NUM_MASTER_DDE_COPIES; i++)
            {
                if (dde_raw_arr[i] != NULL)
                {
                    count = blk_count;
                    error = dbo_DbolbData2UserData(dde_raw_arr[i], &count, 
                                                   dde_addr_arr[i], &dde_type, &dbo_id, &status);
                    if (error) 
                    {
                        MP_ktracev("ODBS: dd_Choose_User_ExtUse_Dde: dbo_DbolbData2UserData returned error (0x%x), index %d, line %d\n", error, i, __LINE__);
                        return DD_E_INVALID_DDE_TYPE; 
                    }
		    
                    if (status != TRUE) 
                    { 
                        /* Invalidate this copy since the data was bad */
                        dde_arr[i] = NULL;
                    }
                    else
                    {
                        dde_arr[i] = (DDE_DATA *) dde_raw_arr[i];

                        if ((dde_type != DD_USER_DDE_TYPE &&
                             dde_type != DD_EXTERNAL_USE_DDE_TYPE) ||
                            count != DDE_DATA_SIZE ||
                            dbo_id.low != 0 ||
                            ((DBO_TYPE) dbo_id.high != DBO_INVALID_TYPE &&
                             (DBO_TYPE) dbo_id.high != dbo_type))
                        {
                            MP_ktracev("ODBS: dd_Choose_User_ExtUse_Dde: Step 2 dde_type error(s) for index %d, line %d\n", i, __LINE__);
                            MP_ktracev("\tdde_type: 0x%x (0x%x, 0x%x)\n", 
                                       dde_type, DD_USER_DDE_TYPE, DD_EXTERNAL_USE_DDE_TYPE); 
                            MP_ktracev("\tcount %d (%llu)\n", 
                                       count, (unsigned long long)DDE_DATA_SIZE); 
                            MP_ktracev("\tdbo_id: 0x%x:0x%x, dbo_type: 0x%x\n", 
                                       dbo_id.high, dbo_id.low, dbo_type);

                            error_code = DD_E_INVALID_DDE_TYPE;
                            return error_code;
                        }

                        dde_arr[i] = (DDE_DATA *) dde_raw_arr[i];
                    }
                }
                else
                {
                    dde_arr[i] = NULL;
                }
            }
            /* ************ END of STEP 2 */


            /* ************ STEP 3: Validate dde revision, CRC, and update_count */
            for (i=0; i< DD_NUM_MASTER_DDE_COPIES; i++)
            {
                if ((curr_dde = dde_arr[i]) != NULL)
                {
                    /* !!!MZ - we should really write correct revision
                    * since we CRC the data - however do not bother
                    * right now do it later when first upgrade is required.
                    */
                    if ((curr_dde->rev_num != DBO_INVALID_REVISION) &&
                        (curr_dde->rev_num != DDE_REVISION_ONE))
                    {
                        /* We do not support this revision - bad data! */
                        dde_arr[i] = NULL;
                        continue;
                    }

                    odb_assert(curr_dde->crc != 0);
                    error_code = dd_CrcManagementData((UINT_8E *)curr_dde, DDE_DATA_SIZE, &crc);
                    if (error_code != DD_OK) 
                    {
                        MP_ktracev("ODBS: dd_Choose_User_ExtUse_Dde: dd_CrcManagementData returned error (0x%x), line %d\n", error_code, __LINE__);
                        return error_code; 
                    }

                    if ((curr_dde->crc ^= crc) != 0)
                    {
                        /* CRC does not macth! */
                        dde_arr[i] = NULL;
                        continue;
                    }

                    odb_assert(curr_dde->update_count != 0);

                    if (i == DBO_PRIMARY_INDEX)
                    {
                        /* Set the initilal values for future comparison */
                        update_count = curr_dde->update_count;
                    }
                    else
                    {
                        odb_assert(i == DBO_SECONDARY_INDEX);

                        if ((dde_arr[DBO_PRIMARY_INDEX] != NULL) &&
                            (curr_dde->update_count != update_count))
                        {
                            (curr_dde->update_count > update_count) ?
                                (dde_arr[DBO_PRIMARY_INDEX] = NULL) :
                            (dde_arr[DBO_SECONDARY_INDEX] = NULL);
                        }
                    }
                }
            } 
            /* ************ END of STEP 3 */


            /* ************ STEP 4: Get the default DDE value */
            if ((dde_arr[DBO_PRIMARY_INDEX] != NULL) &&
                (dde_arr[DBO_SECONDARY_INDEX] != NULL))
            {
                odb_assert((dde_arr[DBO_PRIMARY_INDEX]->rev_num == 
                    dde_arr[DBO_SECONDARY_INDEX]->rev_num) &&
                    (dde_arr[DBO_PRIMARY_INDEX]->update_count == 
                    dde_arr[DBO_SECONDARY_INDEX]->update_count));
            }

            if (dde_arr[DBO_PRIMARY_INDEX] != NULL)
            {
                dde_rev = dde_arr[DBO_PRIMARY_INDEX]->rev_num;
                dde_info_rev = dde_arr[DBO_PRIMARY_INDEX]->info_rev;
            }
            else if (dde_arr[DBO_SECONDARY_INDEX] != NULL)
            {
                dde_rev = dde_arr[DBO_SECONDARY_INDEX]->rev_num;
                dde_info_rev = dde_arr[DBO_SECONDARY_INDEX]->info_rev;
            }
            else
            {
                /* Use values of the caller */
                dde_type = ((DBO_TYPE_IS_USER(dbo_type)) ? DD_USER_DDE_TYPE : DD_EXTERNAL_USE_DDE_TYPE);

                error_code = dd_WhatIsManagementDboCurrentRev(service, rev, dde_type, &dde_rev);
                if (error_code != DD_OK) 
                {
                    MP_ktracev("ODBS: dd_Choose_User_ExtUse_Dde: dd_WhatIsManagementDboCurrentRev returned error (0x%x), line %d\n", error_code, __LINE__);
                    return error_code; 
                }

                odb_assert(dde_rev != DBO_INVALID_REVISION );

                error_code = dd_WhatIsDefault_User_ExtUse_SetRev(service, rev, dbo_type,&(dde_info_rev));
                if (error_code != DD_OK) 
                {
                    MP_ktracev("ODBS: dd_Choose_User_ExtUse_Dde: dd_WhatIsDefault_User_ExtUse_SetRev returned error (0x%x), line %d\n", error_code, __LINE__);
                    return error_code; 
                }

                if ((DBO_INVALID_REVISION != current_dde_rev) &&
                    (dde_info_rev < current_dde_rev))
                {
                    dde_info_rev = current_dde_rev;
                }

                odb_assert(dde_info_rev != DBO_INVALID_REVISION);
            }

            if (dde_rev == DBO_INVALID_REVISION)
            {
                dde_rev = DDE_REVISION_ONE;
            }

            error_code = dd_GetDefault_User_ExtUse_SetInfo(service, rev, dde_rev, 
                                                           fru, dbo_type, dde_buf, drive_type, dde_info_rev, DefaultLayout);
            if (error_code != DD_OK) 
            {
                MP_ktracev("ODBS: dd_Choose_User_ExtUse_Dde: dd_GetDefault_User_ExtUse_SetInfo returned error (0x%x), line %d\n", error_code, __LINE__);
                return error_code; 
            }

            /* If this set is present on each fru - replace the value in */
            error_code = dd_Is_User_ExtUse_SetOnEachFru(service, rev, dbo_type, &set_present);
            if (error_code != DD_OK) 
            {
                MP_ktracev("ODBS: dd_Choose_User_ExtUse_Dde: dd_Is_User_ExtUse_SetOnEachFru returned error (0x%x), line %d\n", error_code, __LINE__);
                return error_code; 
            }
            /* ************ END of STEP 4 */


            /* ************ STEP 5: Compare the values */
            update_count = dde_buf->update_count;
            for (i=0; i<DD_NUM_DDE_COPIES; i++)
            {
                if ((curr_dde = dde_arr[i]) != NULL)
                {
                    /* Replace dynamic values */
                    if ((set_present) && 
                        (1 == curr_dde->num_disks) &&
                        (curr_dde->disk_set[0] != fru))
                    {
                        if (0 == i)
                        {
                            *dd_swap_ptr = curr_dde->disk_set[0];
                        }
                        else if ((dde_arr[DBO_PRIMARY_INDEX] != NULL) &&
                            (*dd_swap_ptr != curr_dde->disk_set[0]))
                        {
                            /* !!!MZ - We have different information for 
                            * both copies - cannot make the decision
                            */
                            *validity_mask_ptr = DBO_ALL_COPIES_INVALID;
                            dde_arr[DBO_PRIMARY_INDEX]   = NULL;
                            dde_arr[DBO_SECONDARY_INDEX] = NULL;
                            continue;
                        }

                        /* We expect this disk present in the defaultd data */
                        odb_assert((1 == dde_buf->num_disks) &&
                            (dde_buf->disk_set[0] == fru));

                        curr_dde->disk_set[0] = fru;
                    }

                    dde_buf->update_count = curr_dde->update_count;
                    dde_buf->info_rev = curr_dde->info_rev;

                    /* Notice that this will verify REV, TYPE, ... */
                    error = dbo_memcmp(curr_dde, dde_buf, DDE_DATA_SIZE, &status);
                    if (error) 
                    {
                        MP_ktracev("ODBS: dd_Choose_User_ExtUse_Dde: dbo_memcmp returned error (0x%x), line %d\n", error, __LINE__);
                        return DD_E_INVALID_MEMSET; 
                    }

                    if (status == TRUE)
                    {
                        if ((i % 2) == 0)
                        {
                            DBO_SET_COPY_VALID(val_mask, DBO_PRIMARY_INDEX);
                        }
                        else
                        {
                            DBO_SET_COPY_VALID(val_mask, DBO_SECONDARY_INDEX);
                        }
                    }
                }
            } 

            if (DBO_ALL_COPIES_INVALID == val_mask)
            {
                if (dde_buf->start_addr != DBOLB_INVALID_ADDRESS)
                {
                    /* Restore the original update count */
                    odb_assert(update_count == DD_MS_DEFAULT_UPDATE_COUNT);
                    dde_buf->update_count = update_count;
                }

                *dd_swap_ptr = DBO_INVALID_FRU;
            }
            /* ************ END of STEP 5 */


            /* ************ STEP 6: Set the validity mask */
            *validity_mask_ptr = val_mask;
            /* *status_ptr = TRUE; */
            /* ************ END of STEP 6 */
        }
        else
        {
            MP_ktracev("ODBS: dd_Choose_User_ExtUse_Dde: invalid rev (0x%x), line %d\n", rev, __LINE__);
            error_code = DD_E_INVALID_REVISION;
        }
        break;

    default:
        MP_ktracev("ODBS: dd_Choose_User_ExtUse_Dde: invalid service (0x%x), line %d\n", service, __LINE__);
        error_code = DD_E_INVALID_REVISION;
    }

    return(error_code);

} /* dd_Choose_User_ExtUse_Dde() */


/***********************************************************************
*                         dd_FormatDDE
***********************************************************************
*
* DESCRIPTION:
*   *MANAGEMENT SPACE DD UTILITY FUNCTION*
*
* PARAMETERS:
*   dde_ptr            - ptr to DDE to format
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*
* NOTES:
*
*    Calling layer should check for the returned value.
*
* HISTORY:
*
*   23-Apr-08: Added DBConvert error printouts        Gary Peterson
*
***********************************************************************/

DD_ERROR_CODE dd_FormatDDE(DDE_DATA *dde_ptr)
{
    NT_FISH_DDE_DATA *nt_fish_dde_ptr;
    DDE_DATA temp_dde;
    UINT_32 index = 0;
    DD_ERROR_CODE error_code = DD_OK;

    nt_fish_dde_ptr = (NT_FISH_DDE_DATA *)dde_ptr;

    temp_dde.rev_num = nt_fish_dde_ptr->rev_num;
    temp_dde.crc;
    temp_dde.update_count = nt_fish_dde_ptr->update_count;
    temp_dde.type = nt_fish_dde_ptr->type;
    temp_dde.algorithm = nt_fish_dde_ptr->algorithm;
    temp_dde.num_entries = nt_fish_dde_ptr->num_entries;
    temp_dde.info_rev = nt_fish_dde_ptr->info_rev;
    temp_dde.info_bpe = nt_fish_dde_ptr->info_bpe;
    temp_dde.start_addr = (UINT_32E)(nt_fish_dde_ptr->start_addr);
    temp_dde.num_disks = nt_fish_dde_ptr->num_disks;
    for (index = 0; index < MAX_DISK_ARRAY_WIDTH; index++)
    {
        temp_dde.disk_set[index] = nt_fish_dde_ptr->disk_set[index];
    }

    // Calculate CRC
    error_code = dd_CrcManagementData((UINT_8E *)(&temp_dde), 
                    NT_FISH_DDE_DATA_SIZE,
                    &(temp_dde.crc));
    if (error_code != DD_OK) 
    {
#ifdef DBCONVERT_ENV
	CTprint(PRINT_MASK_ERROR,
		"dd_FormatDDE: error from dd_CrcManagementData at line %d",
		__LINE__);
#endif	
    return error_code;
    }

    *dde_ptr = temp_dde;

    return DD_OK;


} // dd_FormatDDE()


/***********************************************************************
*                  dd_ChooseValidDde()
***********************************************************************
*
* DESCRIPTION:
*   *MANAGEMENT SPACE DD UTILITY FUNCTION*
*    This function verifies copies of USER/EXT USE DDE and choose the 
*    correct one.
*
* PARAMETERS:
*   service            - ODB service type;
*   rev                - ODBS revision;
*   dde_buf            - USER DDE data to be filled in;
*   fru                - Fru from which data was read;
*   dde_raw_arr        - Array of pointers to obtained Master DDEs raw DBOLB data;
*   dde_addr_arr       - Array of DDE addresses;
*   validity_mask_ptr  - Ptr to Address value to be returned;
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*   Validity mask set to DBO_ALL_COPIES_INVALID in the case of an
*   error.
*
* NOTES:
*    If a buffer is NULL pointer we do not look at address and blk_count
*    values -- One can pass all buffers as NULL ptrs and get the default 
*    DDE.
*
*    The caller should pass service/rev obtained from MDDE on the fru 
*    not running values of ODB service when parsing data from a live fru.
*
*    Calling layer should check for the returned value.
*
*    This function is the same as dd_Choose_User_External_Use_Dde(), but is less
*    restrictive. It is intended to eventually replace 
*    dd_ChooseMasterDde().
*
* HISTORY:
*   31-Aug-05: Added argument encl_type               Aparna V
*   22-Apr-08: Added DBConvert error printouts      Gary Peterson (GSP)
*
***********************************************************************/
DD_ERROR_CODE dd_ChooseValidDde(const DD_SERVICE service,
                                const DBO_REVISION rev,
                                DBO_TYPE dde_type,
                                DDE_DATA *dde_buf,
                                const UINT_32 fru,
                                const UINT_32 blk_count,
                                UINT_8E *dde_raw_arr[DD_NUM_DDE_COPIES],
                                DBOLB_ADDR dde_addr_arr[DD_NUM_DDE_COPIES],
                                UINT_32 *validity_mask_ptr,
                                BOOL check_crc_only,
                                DBO_REVISION mdde_rev)
{
    BOOL error = FALSE, status = FALSE, set_present=FALSE;
    UINT_32 val_mask=DBO_ALL_COPIES_INVALID, i=0, crc=0, count=0, update_count=0;
    DDE_DATA *curr_dde=NULL, *dde_arr[DD_NUM_DDE_COPIES]={NULL, NULL};
    DBO_ID dbo_id = DBO_INVALID_ID;
    DD_ERROR_CODE error_code = DD_OK;


    /* This function supports DDE_REVISION_ONE only */
    odb_assert(dde_buf != NULL);
    odb_assert(validity_mask_ptr != NULL);
    odb_assert(dde_raw_arr != NULL);

    /* ************ STEP 0: Prepare the data */
    *validity_mask_ptr = DBO_ALL_COPIES_INVALID;

    error = dbo_memset((UINT_8E *)dde_buf, 0, DDE_DATA_SIZE);
    if (error) 
    {
#ifdef DBCONVERT_ENV
	CTprint(PRINT_MASK_ERROR,
		"dd_ChooseValidDde: error from dbo_memset -- unable to init memory");
#endif	 
    return DD_E_INVALID_MEMSET; 
    }
    /* ************ END of STEP 0 */


    switch (service)
    {
    case DD_MAIN_SERVICE :
        if (rev == DD_MS_REVISION_ONE)
        {
            /* ************ STEP 1: Convert data from RAW DBOLB format to DDE DATA */

            for (i=0; i< DD_NUM_MASTER_DDE_COPIES; i++)
            {
                if (dde_raw_arr[i] != NULL)
                {
                    count = blk_count;
                    error = dbo_DbolbData2UserData(dde_raw_arr[i],
						   &count,
						   dde_addr_arr[i],
						   &dde_type,
						   &dbo_id,
						   &status);
                    if (error) 
		    {
#ifdef DBCONVERT_ENV
			CTprint(PRINT_MASK_ERROR,
				"dd_ChooseValidDde: Step 1 - error from dbo_DbolbData2UserData at line %d",
				__LINE__);
#endif 
            return DD_E_INVALID_DDE_TYPE; 
		    }

                    if (status != TRUE) 
                    { 
                        /* Invalidate this copy since the data was bad */
                        dde_arr[i] = NULL;
                    }
                    else
                    {
                        dde_arr[i] = (DDE_DATA *) dde_raw_arr[i];

                        if (((dde_type != DD_USER_DDE_TYPE) &&
			     (dde_type != DD_EXTERNAL_USE_DDE_TYPE)) ||
                            ((count != DDE_DATA_SIZE) && (count != NT_FISH_DDE_DATA_SIZE))||
                            ((dbo_id.low != 0)))
                        {
                            error_code = DD_E_INVALID_DDE_TYPE;
#ifdef DBCONVERT_ENV
			    CTprint(PRINT_MASK_ERROR,
				    "dd_ChooseValidDde: Step 1 - dde_type error(s) at line %d",
				    __LINE__);
			    CTprint(PRINT_MASK_ERROR,
				    "\tdde_type: 0x%x (0x%x, 0x%x), count: 0x%x (0x%x, 0x%x), dbo_id: 0x%x:0x%x",
				    dde_type, DD_USER_DDE_TYPE, DD_EXTERNAL_USE_DDE_TYPE,
				    count, DDE_DATA_SIZE, NT_FISH_DDE_DATA_SIZE,
				    dbo_id.high, dbo_id.low);
#endif

                            return(error_code);
                        }

                        dde_arr[i] = (DDE_DATA *) dde_raw_arr[i];
                    }
                }
                else
                {
                    dde_arr[i] = NULL;
                }
            }
            /* ************ END of STEP 1 */


            /* ************ STEP 2: Validate dde revision, CRC, and update_count */
            for (i=0; i< DD_NUM_MASTER_DDE_COPIES; i++)
            {
                if ((curr_dde = dde_arr[i]) != NULL)
                {
                    UINT_32 dde_size = 0;

                    if ((curr_dde->rev_num != DBO_INVALID_REVISION) &&
                        (curr_dde->rev_num != DDE_REVISION_ONE))
                    {
                        /* We do not support this revision - bad data! */
                        dde_arr[i] = NULL;
                        continue;
                    }

                    if (check_crc_only &&
                        ((mdde_rev == MASTER_DDE_REVISION_ONE) ||
                         (mdde_rev == MASTER_DDE_REVISION_TWO) ||
                         (mdde_rev == MASTER_DDE_REVISION_ONE_HUNDRED)))
                    {
                        dde_size = NT_FISH_DDE_DATA_SIZE;
                    }
                    else
                    {
                        dde_size = DDE_DATA_SIZE;
                    }

                    odb_assert(curr_dde->crc != 0);
                    error_code = dd_CrcManagementData((UINT_8E *)curr_dde,
						 dde_size,
						 &crc);
                    if (error_code != DD_OK) 
		    {
#ifdef DBCONVERT_ENV
			CTprint(PRINT_MASK_ERROR,
				"dd_ChooseValidDde: Step 2 - error from dd_CrcManagementData at line %d",
				__LINE__);
#endif 
            return(error_code); 
		    }

                    if ((curr_dde->crc ^= crc) != 0)
                    {
                        /* CRC does not macth! */
                        dde_arr[i] = NULL;
                        continue;
                    }
                    else if (check_crc_only &&
                             ((mdde_rev == MASTER_DDE_REVISION_ONE) ||
                              (mdde_rev == MASTER_DDE_REVISION_TWO) ||
                              (mdde_rev == MASTER_DDE_REVISION_ONE_HUNDRED)))
                    {
                        error_code = dd_FormatDDE(dde_arr[i]);
                        if (error_code != DD_OK)
			{
#ifdef DBCONVERT_ENV
			    CTprint(PRINT_MASK_ERROR,
				    "dd_ChooseValidDde: Step 2 - error from dd_FormatDDE at line %d",
				    __LINE__);
#endif 			    
                return(error_code);
			}
                        DBO_SET_COPY_VALID(val_mask, i);
                    }
                    else
                    {
                        DBO_SET_COPY_VALID(val_mask, i);
                    }

                    odb_assert(curr_dde->update_count != 0);

                    if (i == DBO_PRIMARY_INDEX)
                    {
                        /* Set the initilal values for future comparison */
                        update_count = curr_dde->update_count;
                    }
                    else
                    {
                        odb_assert(i == DBO_SECONDARY_INDEX);

                        if ((dde_arr[DBO_PRIMARY_INDEX] != NULL) &&
                            (curr_dde->update_count != update_count))
                        {
                            (curr_dde->update_count > update_count) ?
                                (dde_arr[DBO_PRIMARY_INDEX] = NULL) :
				(dde_arr[DBO_SECONDARY_INDEX] = NULL);
                        }
                    }
                }
            } 
            /* ************ END of STEP 2 */


	    /* ************ STEP 3: Get the valid DDE */
	    if (val_mask != DBO_ALL_COPIES_INVALID)
	    {
		odb_assert((dde_arr[DBO_PRIMARY_INDEX] != NULL) ||
			   (dde_arr[DBO_SECONDARY_INDEX] != NULL));

		for (i=0; i<DD_NUM_DDE_COPIES; i++)
		{
		    if (DBO_IS_COPY_VALID(val_mask, i))
		    {
			odb_assert(dde_arr[i] != NULL);
			*dde_buf = *dde_arr[i];
		    }
		    else
		    {
			odb_assert(dde_arr[i] == NULL);
		    }
		}
	    }
	    /* ************ END of STEP 3 */

	    /* ************ STEP 4: Set the validity mask */
	    *validity_mask_ptr = val_mask;
            /* *status_ptr = TRUE; */
            /* ************ END of STEP 4 */
        }
        else
        {
#ifdef DBCONVERT_ENV
	    CTprint(PRINT_MASK_ERROR,
		    "dd_ChooseValidDde: Invalid revision (0x%x) for DD_MAIN_SERVICE at line %d",
		    rev, __LINE__);
#endif 			    
            error_code = DD_E_INVALID_REVISION;
        }
        break;

    default:
#ifdef DBCONVERT_ENV
	CTprint(PRINT_MASK_ERROR,
		"dd_ChooseValidDde: Invalid Service request (0x%x) at line %d",
		service, __LINE__);
#endif 			    
        error_code = DD_E_INVALID_SERVICE;
    }

    return(error_code);

} /* dd_ChooseValidDde() */

/***********************************************************************
*                  dd_CreateManufactureZeroMark()
***********************************************************************
*
* DESCRIPTION:
*   *MANAGEMENT SPACE DD UTILITY FUNCTION*
*    This function creates the manufature zero mark based on the revision
*    requested.
*
* PARAMETERS:
*   service           - ODB service type;
*   rev               - ODBS revision;
*   mzm_rev           - format indicator for mzm to be created;
*   buff              - Buffer where zero marked will be placed;
*   size_ptr          - Ptr to size of Zero Mark Data;
*
* RETURN VALUES/ERRORS:
*  DD_ERROR_CODE - Error_code;
*   
* NOTES:
*    Calling layer should check for the returned value.
*
* HISTORY:
*   08-Sep-01: Created.                               Mike Zelikov (MZ)
*   22-Apr-08: Added DBConvert error printouts      Gary Peterson (GSP)
*
***********************************************************************/
DD_ERROR_CODE dd_CreateManufactureZeroMark(const DD_SERVICE service, 
                                  const DBO_REVISION rev,
                                  const DBO_REVISION mzm_rev,
                                  UINT_8E *buff,
                                  UINT_32 *size_ptr)
{
    DD_MAN_ZERO_MARK *mzm=NULL;
    DD_ERROR_CODE error_code = DD_OK;


    /* This function supports REV 1 only */
    odb_assert(buff != NULL);
    odb_assert(size_ptr != NULL);

    *size_ptr = 0;

    switch (service)
    {
    case DD_MAIN_SERVICE :
        if (rev == DD_MS_REVISION_ONE)
        {
            switch (mzm_rev)
            {
            case  DD_MAN_ZERO_MARK_REVISION_ONE :
                mzm = (DD_MAN_ZERO_MARK *)buff;

                mzm->rev_num = DD_MAN_ZERO_MARK_CURRENT_REVISION;
                mzm->crc = 0;

                dbo_memcpy(mzm->raw_data,
                    DD_MANUFACTURE_ZERO_MARK_DATA,
                    DD_MANUFACTURE_ZERO_MARK_DATA_SIZE);

                *size_ptr = DD_MAN_ZERO_MARK_SIZE;
                break;

            default:
#ifdef DBCONVERT_ENV
		CTprint(PRINT_MASK_ERROR,
			"dd_CreateManufactureZeroMark: unknown mzm-rev (0x%x) at line %d",
			mzm_rev, __LINE__);
#endif	 

                error_code = DD_E_FAILED_MZM_CREATION;
            }
        }
        else
        {
#ifdef DBCONVERT_ENV
	    CTprint(PRINT_MASK_ERROR,
		    "dd_CreateManufactureZeroMark: unknown rev (0x%x) at line %d",
		    rev, __LINE__);
#endif	 
            error_code = DD_E_INVALID_REVISION;
        }
        break;

    default:
#ifdef DBCONVERT_ENV
	CTprint(PRINT_MASK_ERROR,
		"dd_CreateManufactureZeroMark: unknown service (0x%x) at line %d",
		service, __LINE__);
#endif	 
        error_code = DD_E_INVALID_SERVICE;
    }

    return(error_code);

} /* dd_CreateManufactureZeroMark() */


/* ****** USER/EXTERNAL USE SPACE DATA DIRECTORY UTILITY FUNCTIONS ***** */

/***********************************************************************
*                  dd_Is_User_ExtUse_SetOnFru()
***********************************************************************
*
* DESCRIPTION:
*   *USER/EXTERNAL USE SPACE DD UTILITY FUNCTION*
*    This function checks whether a requeted Set is present on a 
*    given FRU.
*
* PARAMETERS:
*   service     - ODB service type;
*   rev         - Revision;
*   fru         - fru index;
*   dbo_type    - DDE DBO type;
*   set_present - Ptr to value that will be set to TRUE if set is on FRU.
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*
* NOTES:
*    Calling layer should check for the returned value.
*
* HISTORY:
*   09-Aug-00: Created.                               Mike Zelikov (MZ)
*   23-Apr-08: Added DBConvert error printouts      Gary Peterson (GSP)
*
***********************************************************************/
DD_ERROR_CODE dd_Is_User_ExtUse_SetOnFru(const DD_SERVICE service, 
                                const DBO_REVISION rev,
                                const UINT_32 fru,
                                const DBO_TYPE dbo_type,
                                BOOL *set_present)
{
    UINT_32 index = 0;
    UINT_32 min_fru = DBO_INVALID_FRU, max_fru = DBO_INVALID_FRU;
    DBO_ALGORITHM algorithm = DD_MS_EXTERNAL_ALG;
    DD_ERROR_CODE error_code=DD_OK;
    DD_MS_EXTERNAL_USE_SET_INFO  *pExtUseSetInfo;

    odb_assert(set_present != NULL);
    *set_present = FALSE;

    switch (service)
    {
    case DD_MAIN_SERVICE :

        if (rev == DD_MS_REVISION_ONE)
        {
            odb_assert(DBO_TYPE_IS_EXTERNAL_USE(dbo_type) || DBO_TYPE_IS_USER(dbo_type));
            odb_assert(fru != DBO_INVALID_FRU);

            error_code = dd_DboType2Index(service, rev, dbo_type, &index);
            if (error_code != DD_OK)
	    {
#ifdef DBCONVERT_ENV
		CTprint(PRINT_MASK_ERROR,
			"dd_Is_User_ExtUse_SetOnFru: error from dd_DboType2Index at line %d",
			__LINE__);
#endif
        return (error_code);
	    }

            if (DBO_TYPE_IS_USER(dbo_type))
            {
                odb_assert(index < DD_MS_NUM_USER_TYPES);
                odb_assert(dd_ms_user_set_info[index].dbo_type == dbo_type);

                min_fru = dd_ms_user_set_info[index].min_fru;
                max_fru = dd_ms_user_set_info[index].max_fru;
                algorithm = dd_ms_user_set_info[index].algorithm;
            }
            else
            {
                odb_assert(index < DD_MS_NUM_EXTERNAL_USE_TYPES);
                pExtUseSetInfo = dd_GetExtUseSetInfoPtr(index);
                odb_assert(pExtUseSetInfo->dbo_type == dbo_type);

                min_fru = pExtUseSetInfo->min_fru;
                max_fru = pExtUseSetInfo->max_fru;
            }

            if (min_fru == DBO_INVALID_FRU)
            {
                /* This set is present on each fru */
                odb_assert(max_fru == DBO_INVALID_FRU);
                odb_assert((algorithm == DD_MS_SINGLE_DISK_ALG) ||
                    ((algorithm == DD_MS_EXTERNAL_ALG) && (dbo_type == DD_MS_DISK_DIAGNOSTIC_TYPE)) ||
					((algorithm == DD_MS_EXTERNAL_ALG) && (dbo_type == DD_MS_DISK_DIAGNOSTIC_PAD_TYPE)));

                *set_present = TRUE;
            }
            else
            {
                odb_assert(max_fru != DBO_INVALID_FRU);

                if ((fru >= min_fru) && (fru <= max_fru))
                {
                    *set_present = TRUE;
                }
            }
        }
        else
        {
#ifdef DBCONVERT_ENV
	    CTprint(PRINT_MASK_ERROR,
		    "dd_Is_User_ExtUse_SetOnFru: invalid rev (0x%x) at line %d",
		    rev, __LINE__);
#endif
            error_code = DD_E_INVALID_REVISION;
        }
        break;

    default:
#ifdef DBCONVERT_ENV
	CTprint(PRINT_MASK_ERROR,
		"dd_Is_User_ExtUse_SetOnFru: invalid service (0x%x) at line %d",
		service, __LINE__);
#endif
        error_code = DD_E_INVALID_SERVICE;
    }

    return (error_code);

} /* dd_Is_User_ExtUse_SetOnFru() */


/***********************************************************************
*                  dd_Is_User_ExtUse_SetOnEachFru()
***********************************************************************
*
* DESCRIPTION:
*   *USER/EXTERNAL USE SPACE DD UTILITY FUNCTION*
*    This function checks whether a requested Set is present on a 
*    all FRUs.
*
* PARAMETERS:
*   service      - ODB service type;
*   rev          - Revision;
*   dbo_type     - DDE DBO type;
*   set_present  - Ptr to value that shows whether a set is present on each
*                  fru. TRUE if set is present on all frus; FALSE otherwise.
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*
* NOTES:
*   Calling layer should check for the returned value.
*
* HISTORY:
*   15-Feb-01: Created.                                Mike Zelikov (MZ)
*   23-Apr-08: Added DBConvert error printouts      Gary Peterson (GSP)
*
***********************************************************************/
DD_ERROR_CODE dd_Is_User_ExtUse_SetOnEachFru(const DD_SERVICE service, 
                                    const DBO_REVISION rev,
                                    const DBO_TYPE dbo_type,
                                    BOOL *set_present)
{
    UINT_32 index = 0;
    UINT_32 min_fru = DBO_INVALID_FRU, max_fru = DBO_INVALID_FRU;
    DBO_ALGORITHM algorithm = DD_MS_EXTERNAL_ALG;
    DD_ERROR_CODE error_code = DD_OK;
    DD_MS_EXTERNAL_USE_SET_INFO  *pExtUseSetInfo;

    switch (service)
    {
    case DD_MAIN_SERVICE :

        if (rev == DD_MS_REVISION_ONE)
        {
            odb_assert(DBO_TYPE_IS_EXTERNAL_USE(dbo_type) || DBO_TYPE_IS_USER(dbo_type));

            error_code = dd_DboType2Index(service, rev, dbo_type, &index);
            if (error_code != DD_OK)
	    {
#ifdef DBCONVERT_ENV
		CTprint(PRINT_MASK_ERROR,
			"dd_Is_User_ExtUse_SetOnEachFru: error from dd_DboType2Index at line %d",
			__LINE__);
#endif
        return (error_code);
	    }

            if (DBO_TYPE_IS_USER(dbo_type))
            {
                odb_assert(index < DD_MS_NUM_USER_TYPES);
                odb_assert(dd_ms_user_set_info[index].dbo_type == dbo_type);

                min_fru = dd_ms_user_set_info[index].min_fru;
                max_fru = dd_ms_user_set_info[index].max_fru;
                algorithm = dd_ms_user_set_info[index].algorithm;
            }
            else
            {
                odb_assert(index < DD_MS_NUM_EXTERNAL_USE_TYPES);

                pExtUseSetInfo = dd_GetExtUseSetInfoPtr(index);
                odb_assert(pExtUseSetInfo->dbo_type == dbo_type);

                min_fru = pExtUseSetInfo->min_fru;
                max_fru = pExtUseSetInfo->max_fru;
            }

            if (min_fru == DBO_INVALID_FRU)
            {
                /* This set is present on each fru */
                odb_assert(max_fru == DBO_INVALID_FRU);
                odb_assert((algorithm == DD_MS_SINGLE_DISK_ALG) ||
                    ((algorithm == DD_MS_EXTERNAL_ALG) && (dbo_type == DD_MS_DISK_DIAGNOSTIC_TYPE)) ||
                    ((algorithm == DD_MS_EXTERNAL_ALG) && (dbo_type == DD_MS_DISK_DIAGNOSTIC_PAD_TYPE)));

                *set_present = TRUE;
            }
            else
            {
                odb_assert(max_fru != DBO_INVALID_FRU);
            }
        }
        else
        {
#ifdef DBCONVERT_ENV
	    CTprint(PRINT_MASK_ERROR,
		    "dd_Is_User_ExtUse_SetOnEachFru: invalid rev (0x%x) at line %d",
		    rev, __LINE__);
#endif
            error_code = DD_E_INVALID_REVISION;
        }
        break;

    default:
#ifdef DBCONVERT_ENV
	CTprint(PRINT_MASK_ERROR,
		"dd_Is_User_ExtUse_SetOnEachFru: invalid service (0x%x) at line %d",
		service, __LINE__);
#endif
        error_code = DD_E_INVALID_SERVICE;
    }

    return (error_code);

} /* odbs_Is_User_ExtUse_SetOnEachFru() */


/***********************************************************************
*                  dd_WhatIs_User_ExtUse_DiskSet()
***********************************************************************
*
* DESCRIPTION:
*   *USER/EXTERNAL USE SPACE DD UTILITY FUNCTION*
*    This function returns MIN and MAX fru that contain the specified 
*    User or External Use set.
*
* PARAMETERS:
*   service     - ODB service type;
*   rev         - Revision;
*   dbo_type    - DDE DBO type;
*   min_fru     - Minimum Fru value to be populated;
*   max_fru     - Maximum Fru value to be populated;
*     Note that if the set is present on every disk, DBO_INVALID_FRU is 
*     returned as max_fru and min_fru.
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*
* NOTES:
*    Calling layer should check for the returned value.
*
* HISTORY:
*   01-May-01: Created.                               Mike Zelikov (MZ)
*   23-Apr-08: Added DBConvert error printouts      Gary Peterson (GSP)
*
***********************************************************************/
DD_ERROR_CODE dd_WhatIs_User_ExtUse_DiskSet(const DD_SERVICE service, 
                                   const DBO_REVISION rev,
                                   const DBO_TYPE dbo_type,
                                   UINT_32 *min_fru,
                                   UINT_32 *max_fru)
{
    UINT_32 index = 0;
    DBO_ALGORITHM algorithm = DD_MS_EXTERNAL_ALG;
    DD_ERROR_CODE error_code = DD_OK;
    DD_MS_EXTERNAL_USE_SET_INFO  *pExtUseSetInfo;

    odb_assert(min_fru != NULL);
    odb_assert(max_fru != NULL);

    *min_fru = DBO_INVALID_FRU;
    *max_fru = DBO_INVALID_FRU;

    switch (service)
    {
    case DD_MAIN_SERVICE :

        if (rev == DD_MS_REVISION_ONE)
        {
            odb_assert(DBO_TYPE_IS_EXTERNAL_USE(dbo_type) || DBO_TYPE_IS_USER(dbo_type));

            error_code = dd_DboType2Index(service, rev, dbo_type, &index);
            if (error_code != DD_OK)
	    {
#ifdef DBCONVERT_ENV
		CTprint(PRINT_MASK_ERROR,
			"dd_WhatIs_User_ExtUse_DiskSet: error from dd_DboType2Index at line %d",
			__LINE__);
#endif		
        return (error_code);
	    }

            if (DBO_TYPE_IS_USER(dbo_type))
            {
                odb_assert(index < DD_MS_NUM_USER_TYPES);
                odb_assert(dd_ms_user_set_info[index].dbo_type == dbo_type);

                *min_fru = dd_ms_user_set_info[index].min_fru;
                *max_fru = dd_ms_user_set_info[index].max_fru;
                algorithm = dd_ms_user_set_info[index].algorithm;
            }
            else
            {
                odb_assert(index < DD_MS_NUM_EXTERNAL_USE_TYPES);
                pExtUseSetInfo = dd_GetExtUseSetInfoPtr(index);
                odb_assert(pExtUseSetInfo->dbo_type == dbo_type);

                *min_fru = pExtUseSetInfo->min_fru;
                *max_fru = pExtUseSetInfo->max_fru;
            }

            if (*min_fru == DBO_INVALID_FRU)
            {
                /* This set is present on each fru */
                odb_assert(*max_fru == DBO_INVALID_FRU);
                odb_assert((algorithm == DD_MS_SINGLE_DISK_ALG) ||
                    ((algorithm == DD_MS_EXTERNAL_ALG) && (dbo_type == DD_MS_DISK_DIAGNOSTIC_TYPE)) ||
                    ((algorithm == DD_MS_EXTERNAL_ALG) && (dbo_type == DD_MS_DISK_DIAGNOSTIC_PAD_TYPE)));
            }
            else
            {
                odb_assert(*max_fru != DBO_INVALID_FRU);
            }
        }
        else
        {
#ifdef DBCONVERT_ENV
	    CTprint(PRINT_MASK_ERROR,
		    "dd_WhatIs_User_ExtUse_DiskSet: invalid rev (0x%x) at line %d",
		    rev, __LINE__);
#endif
            error_code = DD_E_INVALID_REVISION;
        }
        break;

    default:
#ifdef DBCONVERT_ENV
	CTprint(PRINT_MASK_ERROR,
		"dd_WhatIs_User_ExtUse_DiskSet: invalid service (0x%x) at line %d",
		service, __LINE__);
#endif
        error_code = DD_E_INVALID_SERVICE;
    }

    return (error_code);

} /* dd_WhatIs_User_ExtUse_DiskSet() */


/***********************************************************************
*                  dd_WhatIsDefault_User_ExtUse_SetRev()
***********************************************************************
*
* DESCRIPTION:
*   *USER/EXTERNAL USE SPACE DD UTILITY FUNCTION*
*    This function returns the current revision number for the
*    given USER/EXTERNAL USE DB type.
*
* PARAMETERS:
*   service      - ODB service type;
*   rev          - Revision;
*   dbo_type     - DDE DBO type;
*   dbo_rev      - Ptr to revision to be set;
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*
* NOTES:
*   Calling layer should check for the returned value.
*
* HISTORY:
*   27-Apr-01: Created.                                         (DDD/MZ)
*   22-Apr-08: Added DBConvert error printouts             Gary Peterson
*
***********************************************************************/
DD_ERROR_CODE dd_WhatIsDefault_User_ExtUse_SetRev(const DD_SERVICE service, 
                                         const DBO_REVISION rev,
                                         const DBO_TYPE dbo_type,
                                         DBO_REVISION *dbo_rev)
{
    DD_ERROR_CODE error_code = DD_OK;


    odb_assert(dbo_rev != NULL);
    odb_assert(DBO_TYPE_IS_EXTERNAL_USE(dbo_type) || DBO_TYPE_IS_USER(dbo_type));

    *dbo_rev = DBO_INVALID_REVISION;

    if (DBO_TYPE_IS_USER(dbo_type))
    {
        switch(dbo_type)
        {
        case DD_MS_32K_ALIGNING_PAD_TYPE :
            *dbo_rev = DD_MS_32K_ALIGNING_PAD_REVISION;
            break;

        case DD_MS_FRU_SIGNATURE_PAD_TYPE :
            *dbo_rev = DD_MS_FRU_SIGNATURE_PAD_REVISION;
            break;

        case DD_MS_HW_FRU_VERIFY_PAD_TYPE :
            *dbo_rev = DD_MS_HW_FRU_VERIFY_PAD_REVISION;
            break;

        case DD_MS_EXPANSION_CHKPT_PAD_TYPE :
            *dbo_rev = DD_MS_EXPANSION_CHKPT_PAD_REVISION;
            break;

        case DD_MS_FRU_SIGNATURE_TYPE :
            *dbo_rev = DD_MS_FRU_SIGNATURE_REVISION;
            break;

        case DD_MS_HW_FRU_VERIFY_TYPE :
            *dbo_rev = DD_MS_HW_FRU_VERIFY_REVISION;
            break;

        case DD_MS_EXPANSION_CHKPT_TYPE :
            *dbo_rev = DD_MS_EXPANSION_CHKPT_REVISION;
            break;

        case DD_MS_CLEAN_DIRTY_TYPE :
            *dbo_rev = DD_MS_CLEAN_DIRTY_REVISION;
            break;

        case DD_MS_PADDER_TYPE:
            *dbo_rev = DD_MS_PADDER_REVISION;
            break;

        case DD_MS_EXTERNAL_DUMMY_TYPE:
            *dbo_rev = DD_MS_EXTERNAL_DUMMY_REVISION;
            break;

        case DD_MS_FLARE_PAD_TYPE :
            *dbo_rev = DD_MS_FLARE_PAD_REVISION;
            break;

        case DD_MS_SYSCONFIG_TYPE :
            *dbo_rev = DD_MS_SYSCONFIG_REVISION;
            break;

        case DD_MS_SYSCONFIG_PAD_TYPE :
            *dbo_rev = DD_MS_SYSCONFIG_PAD_REVISION;
            break;

        case DD_MS_GLUT_TYPE :
            *dbo_rev = DD_MS_GLUT_REVISION;
            break;

        case DD_MS_GLUT_PAD_TYPE :
            *dbo_rev = DD_MS_GLUT_PAD_REVISION;
            break;

        case DD_MS_FRU_TYPE :
            *dbo_rev = DD_MS_FRU_REVISION;
            break;

        case DD_MS_FRU_PAD_TYPE :
            *dbo_rev = DD_MS_FRU_PAD_REVISION;
            break;

        case DD_MS_RAID_GROUP_TYPE :
            *dbo_rev = DD_MS_RAID_GROUP_REVISION;
            break;

        case DD_MS_RAID_GROUP_PAD_TYPE :
            *dbo_rev = DD_MS_RAID_GROUP_PAD_REVISION;
            break;

        case DD_MS_VERIFY_RESULTS_TYPE :
            *dbo_rev = DD_MS_VERIFY_RESULTS_REVISION;
            break;

        case DD_MS_VERIFY_RESULTS_PAD_TYPE :
            *dbo_rev = DD_MS_VERIFY_RESULTS_PAD_REVISION;
            break;

        case DD_MS_ENCL_TBL_TYPE :
            *dbo_rev = DD_MS_ENCL_TBL_REVISION;
            break;

        case DD_MS_ENCL_TBL_PAD_TYPE :
            *dbo_rev = DD_MS_ENCL_TBL_PAD_REVISION;
            break;

        case DD_MS_RESERVED_EXP_TYPE :
            *dbo_rev = DD_MS_RESERVED_EXP_REVISION;
            break;


        default:
#ifdef DBCONVERT_ENV
	    CTprint(PRINT_MASK_ERROR,
		    "dd_WhatIsDefualt_User_ExtUse_SetRev: Invalid User DBO Type (0x%x) at line %d",
		    dbo_type, __LINE__);
#endif 			    
            error_code = DD_E_INVALID_REVISION;
            break;
        }
    }
    else
    {
        switch(dbo_type)
        {
        case DD_MS_DISK_DIAGNOSTIC_TYPE:
            *dbo_rev = DD_MS_DISK_DIAGNOSTIC_REVISION;
            break;

        case DD_MS_DISK_DIAGNOSTIC_PAD_TYPE:
            *dbo_rev = DD_MS_DISK_DIAGNOSTIC_PAD_REVISION;
            break;

        case DD_MS_DDBS_TYPE:
            *dbo_rev = DD_MS_DDBS_REVISION;
            break;

        case DD_MS_BIOS_UCODE_TYPE:
            *dbo_rev = DD_MS_BIOS_UCODE_REVISION;
            break;

        case DD_MS_PROM_UCODE_TYPE:
            *dbo_rev = DD_MS_PROM_UCODE_REVISION;
            break;

        case DD_MS_FUTURE_EXP_TYPE:
            *dbo_rev = DD_MS_FUTURE_EXP_REVISION;
            break;

        case DD_MS_GPS_FW_TYPE:
            *dbo_rev = DD_MS_GPS_FW_REVISION;
            break;

        case DD_MS_BOOT_LOADER_TYPE:
            *dbo_rev = DD_MS_BOOT_LOADER_REVISION;
            break;

        case DD_MS_EXP_USER_DUMMY_TYPE:
            *dbo_rev = DD_MS_EXP_USER_DUMMY_REVISION;
            break;

        case DD_MS_VAULT_RESERVED_TYPE:
            *dbo_rev = DD_MS_VAULT_RESERVED_REVISION;
            break;

        case DD_MS_VAULT_FUTURE_EXP_TYPE:
            *dbo_rev = DD_MS_VAULT_FUTURE_EXP_REVISION;
            break;

        case DD_MS_UTIL_SPA_PRIM_TYPE:
            *dbo_rev = DD_MS_UTIL_SPA_PRIM_REVISION;
            break;

        case DD_MS_UTIL_SPA_SEC_TYPE:
            *dbo_rev = DD_MS_UTIL_SPA_SEC_REVISION;
            break;

        case DD_MS_UTIL_SPB_PRIM_TYPE:
            *dbo_rev = DD_MS_UTIL_SPB_PRIM_REVISION;
            break;

        case DD_MS_UTIL_SPB_SEC_TYPE:
            *dbo_rev = DD_MS_UTIL_SPB_SEC_REVISION;
            break;

        case DD_MS_UTIL_SPA_PRIM_EXP_TYPE:
            *dbo_rev = DD_MS_UTIL_SPA_PRIM_EXP_REVISION;
            break;

        case DD_MS_UTIL_SPA_SEC_EXP_TYPE:
            *dbo_rev = DD_MS_UTIL_SPA_SEC_EXP_REVISION;
            break;

        case DD_MS_UTIL_SPB_PRIM_EXP_TYPE:
            *dbo_rev = DD_MS_UTIL_SPB_PRIM_EXP_REVISION;
            break;

        case DD_MS_UTIL_SPB_SEC_EXP_TYPE:
            *dbo_rev = DD_MS_UTIL_SPB_SEC_EXP_REVISION;
            break;
		
        case DD_MS_NT_BOOT_SPA_PRIM_TYPE:
            *dbo_rev = DD_MS_NT_BOOT_SPA_PRIM_REVISION;
            break;

        case DD_MS_NT_BOOT_SPA_SEC_TYPE:
            *dbo_rev = DD_MS_NT_BOOT_SPA_SEC_REVISION;
            break;

        case DD_MS_NT_BOOT_SPB_PRIM_TYPE:
            *dbo_rev = DD_MS_NT_BOOT_SPB_PRIM_REVISION;
            break;

        case DD_MS_NT_BOOT_SPB_SEC_TYPE:
            *dbo_rev = DD_MS_NT_BOOT_SPB_SEC_REVISION;
            break;

        case DD_MS_NT_BOOT_SPA_PRIM_EXP_TYPE:
            *dbo_rev = DD_MS_NT_BOOT_SPA_PRIM_EXP_REVISION;
            break;

        case DD_MS_NT_BOOT_SPA_SEC_EXP_TYPE:
            *dbo_rev = DD_MS_NT_BOOT_SPA_SEC_EXP_REVISION;
            break;

        case DD_MS_NT_BOOT_SPB_PRIM_EXP_TYPE:
            *dbo_rev = DD_MS_NT_BOOT_SPB_PRIM_EXP_REVISION;
            break;

        case DD_MS_NT_BOOT_SPB_SEC_EXP_TYPE:
            *dbo_rev = DD_MS_NT_BOOT_SPB_SEC_EXP_REVISION;
            break;

        case DD_MS_ICA_IMAGE_REP_EXP_TYPE:
            *dbo_rev = DD_MS_ICA_IMAGE_REP_EXP_REVISION;
            break;

        case DD_MS_ICA_IMAGE_REP_TYPE:
            *dbo_rev = DD_MS_ICA_IMAGE_REP_REVISION;
            break;

        case DD_MS_ICA_IMAGE_REP_PAD_TYPE:
            *dbo_rev = DD_MS_ICA_IMAGE_REP_PAD_REVISION;
            break;

        case DD_MS_PSM_RESERVED_TYPE:
            *dbo_rev = DD_MS_PSM_RESERVED_REVISION;
            break;

        case DD_MS_BOOT_DIRECTIVES_TYPE:
            *dbo_rev = DD_MS_BOOT_DIRECTIVES_REVISION;
            break;

        case DD_MS_FUTURE_GROWTH_TYPE:
            *dbo_rev = DD_MS_FUTURE_GROWTH_REVISION;
            break;

        case DD_MS_EXT_NEW_DB_DUMMY_TYPE :
            *dbo_rev = DD_MS_EXT_NEW_DB_DUMMY_REVISION;
            break;

        case DD_MS_UTIL_FLARE_DUMMY_TYPE :
            *dbo_rev = DD_MS_UTIL_FLARE_DUMMY_REVISION;
            break;

        case DD_MS_PSM_BOOT_DUMMY_TYPE :
            *dbo_rev = DD_MS_PSM_BOOT_DUMMY_REVISION;
            break;

        case DD_MS_DDBS_LUN_OBJ_TYPE :
            *dbo_rev = DD_MS_DDBS_LUN_OBJ_REVISION;
            break;

        case DD_MS_BIOS_LUN_OBJ_TYPE :
            *dbo_rev = DD_MS_BIOS_LUN_OBJ_REVISION;
            break;

        case DD_MS_PROM_LUN_OBJ_TYPE :
            *dbo_rev = DD_MS_PROM_LUN_OBJ_REVISION;
            break;

        case DD_MS_GPS_FW_LUN_OBJ_TYPE :
            *dbo_rev = DD_MS_GPS_FW_LUN_OBJ_REVISION;
            break;

        case DD_MS_FLARE_DB_TYPE :
            *dbo_rev = DD_MS_FLARE_DB_REVISION;
            break;

        case DD_MS_FLARE_DB_LUN_OBJ_TYPE :
            *dbo_rev = DD_MS_FLARE_DB_LUN_OBJ_REVISION;
            break;

        case DD_MS_ESP_LUN_TYPE :
            *dbo_rev = DD_MS_ESP_LUN_REVISION;
            break;

        case DD_MS_ESP_LUN_OBJ_TYPE :
            *dbo_rev = DD_MS_ESP_LUN_OBJ_REVISION;
            break;

        case DD_MS_VCX_LUN_0_TYPE :
            *dbo_rev = DD_MS_VCX_LUN_0_REVISION;
            break;

        case DD_MS_VCX_LUN_0_OBJ_TYPE :
            *dbo_rev = DD_MS_VCX_LUN_0_OBJ_REVISION;
            break;

        case DD_MS_VCX_LUN_1_TYPE :
            *dbo_rev = DD_MS_VCX_LUN_1_REVISION;
            break;

        case DD_MS_VCX_LUN_1_OBJ_TYPE :
            *dbo_rev = DD_MS_VCX_LUN_1_OBJ_REVISION;
            break;

        case DD_MS_VCX_LUN_2_TYPE :
            *dbo_rev = DD_MS_VCX_LUN_2_REVISION;
            break;

        case DD_MS_VCX_LUN_2_OBJ_TYPE :
            *dbo_rev = DD_MS_VCX_LUN_2_OBJ_REVISION;
            break;

        case DD_MS_VCX_LUN_3_TYPE :
            *dbo_rev = DD_MS_VCX_LUN_3_REVISION;
            break;

        case DD_MS_VCX_LUN_3_OBJ_TYPE :
            *dbo_rev = DD_MS_VCX_LUN_3_OBJ_REVISION;
            break;

        case DD_MS_VCX_LUN_4_TYPE :
            *dbo_rev = DD_MS_VCX_LUN_4_REVISION;
            break;

        case DD_MS_VCX_LUN_4_OBJ_TYPE :
            *dbo_rev = DD_MS_VCX_LUN_4_OBJ_REVISION;
            break;

        case DD_MS_VCX_LUN_5_TYPE :
            *dbo_rev = DD_MS_VCX_LUN_5_REVISION;
            break;

        case DD_MS_VCX_LUN_5_OBJ_TYPE :
            *dbo_rev = DD_MS_VCX_LUN_5_OBJ_REVISION;
            break;

        case DD_MS_CENTERA_LUN_TYPE :
            *dbo_rev = DD_MS_CENTERA_LUN_REVISION;
            break;

        case DD_MS_CENTERA_LUN_OBJ_TYPE :
            *dbo_rev = DD_MS_CENTERA_LUN_OBJ_REVISION;
            break;

        case DD_MS_PSM_LUN_OBJ_TYPE :
            *dbo_rev = DD_MS_PSM_LUN_OBJ_REVISION;
            break;

        case DD_MS_DRAM_CACHE_LUN_TYPE :
            *dbo_rev = DD_MS_DRAM_CACHE_LUN_REVISION;
            break;

        case DD_MS_DRAM_CACHE_LUN_OBJ_TYPE :
            *dbo_rev = DD_MS_DRAM_CACHE_LUN_OBJ_REVISION;
            break;

        case DD_MS_EFD_CACHE_LUN_TYPE :
            *dbo_rev = DD_MS_EFD_CACHE_LUN_REVISION;
            break;

        case DD_MS_EFD_CACHE_LUN_OBJ_TYPE :
            *dbo_rev = DD_MS_EFD_CACHE_LUN_OBJ_REVISION;
            break;

        case DD_MS_PSM_RAID_GROUP_OBJ_TYPE :
            *dbo_rev = DD_MS_PSM_RAID_GROUP_OBJ_REVISION;
            break;

        case DD_MS_ALIGN_BUFFER1_TYPE :
            *dbo_rev = DD_MS_ALIGN_BUFFER1_REVISION;
            break;

        case DD_MS_WIL_A_LUN_TYPE :
            *dbo_rev = DD_MS_WIL_A_LUN_REVISION;
            break;

        case DD_MS_WIL_A_LUN_OBJ_TYPE :
            *dbo_rev = DD_MS_WIL_A_LUN_OBJ_REVISION;
            break;

        case DD_MS_WIL_B_LUN_TYPE :
            *dbo_rev = DD_MS_WIL_B_LUN_REVISION;
            break;

        case DD_MS_WIL_B_LUN_OBJ_TYPE :
            *dbo_rev = DD_MS_WIL_B_LUN_OBJ_REVISION;
            break;

        case DD_MS_CPL_A_LUN_TYPE :
            *dbo_rev = DD_MS_CPL_A_LUN_REVISION;
            break;

        case DD_MS_CPL_A_LUN_OBJ_TYPE :
            *dbo_rev = DD_MS_CPL_A_LUN_OBJ_REVISION;
            break;

        case DD_MS_CPL_B_LUN_TYPE :
            *dbo_rev = DD_MS_CPL_B_LUN_REVISION;
            break;

        case DD_MS_CPL_B_LUN_OBJ_TYPE :
            *dbo_rev = DD_MS_CPL_B_LUN_OBJ_REVISION;
            break;

        case DD_MS_RAID_METADATA_LUN_TYPE :
            *dbo_rev = DD_MS_RAID_METADATA_LUN_REVISION;
            break;

        case DD_MS_RAID_METADATA_LUN_OBJ_TYPE :
            *dbo_rev = DD_MS_RAID_METADATA_LUN_OBJ_REVISION;
            break;

        case DD_MS_RAID_METASTATS_LUN_TYPE :
            *dbo_rev = DD_MS_RAID_METASTATS_LUN_REVISION;
            break;

        case DD_MS_RAID_METASTATS_LUN_OBJ_TYPE :
            *dbo_rev = DD_MS_RAID_METASTATS_LUN_OBJ_REVISION;
            break;

        case DD_MS_VAULT_LUN_OBJECT_TYPE :
            *dbo_rev = DD_MS_VAULT_LUN_OBJECT_REVISION;
            break;

        case DD_MS_VAULT_RAID_GROUP_OBJ_TYPE :
            *dbo_rev = DD_MS_VAULT_RAID_GROUP_OBJ_REVISION;
            break;

        case DD_MS_NON_LUN_RAID_GROUP_OBJ_TYPE :
            *dbo_rev = DD_MS_NON_LUN_RAID_GROUP_OBJ_REVISION;
            break;

        default:
#ifdef DBCONVERT_ENV
	    CTprint(PRINT_MASK_ERROR,
		    "dd_WhatIsDefualt_User_ExtUse_SetRev: Invalid ExtUse DBO Type (0x%x) at line %d",
		    dbo_type, __LINE__);
#endif 			    
         error_code = DD_E_INVALID_REVISION;

#if UMODE_ENV
            odb_assert(FALSE);
#endif
            break;
        }
    }

    return (error_code);

} /* dd_WhatIsDefault_User_ExtUse_SetRev() */

/**********************************************************************
*                 dd_WhatIsNumUserExtUseTypes()
**********************************************************************
*
* DESCRIPTION:   This function returns the number of user or 
*                external use types present on the fru passed in. 
*                If the new layout was not yet committed, it is 
*                possible for different frus to have different layouts. 
*
* PARAMETERS:
*   service     - ODB service type
*   rev         - Revision
*   dbo_type    - dbo type
*   curr_mdde_rev - current revision of Master DDE
*   num_types   - Ptr to the number of types (to be returned)
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*
* NOTES:
*   Calling layer should check for the returned value.
*
* HISTORY:
*   11-Oct-02: Created.                             Aparna Vankamamidi
*   28-Jun-03: Modified to be used for User
*              as well as External Use types.       Aparna V
*   23-Apr-08: Added DBConvert error printouts      Gary Peterson (GSP)
*
**********************************************************************/
DD_ERROR_CODE dd_WhatIsNumUserExtUseTypes(const DD_SERVICE service,
                                 const DBO_REVISION rev,
                                 const DBO_TYPE dbo_type,
                                 const DBO_REVISION curr_mdde_rev,
                                 UINT_32 *num_types)
{

    DD_ERROR_CODE error_code = DD_OK;

    odb_assert(num_types != NULL);
    odb_assert(dd_read_layout_on_disk);

    switch (service)
    {
    case DD_MAIN_SERVICE:
        if (rev == DD_MS_REVISION_ONE)
        {
            if (DD_USER_DDE_TYPE == dbo_type)
            {
                switch (curr_mdde_rev)
                {
                case MASTER_DDE_REVISION_ONE:
                case MASTER_DDE_REVISION_TWO:
                case MASTER_DDE_REVISION_ONE_HUNDRED:
				default:
                    *num_types = DD_MS_NUM_USER_TYPES;
                    break;

                } // switch (curr_mdde_rev)
            }
            else
            {
                odb_assert(DD_EXTERNAL_USE_DDE_TYPE == dbo_type);
                switch (curr_mdde_rev)
                {
                case MASTER_DDE_REVISION_ONE:
                    *num_types = DD_MS_NUM_EXTERNAL_USE_TYPES_REV_ONE;
                    break;

                case MASTER_DDE_REVISION_TWO:
                case MASTER_DDE_REVISION_ONE_HUNDRED:
                default:
                    *num_types = DD_MS_NUM_EXTERNAL_USE_TYPES;
                    break;


                } // switch (current_mdde_rev)
            } // if (DD_USER_DDE_TYPE == dbo_type)
        } // if rev == DD_MS_REVISION_ONE
        else
        {
#ifdef DBCONVERT_ENV
	    CTprint(PRINT_MASK_ERROR,
		    "dd_WhatIsNumUserExtUseTypes: Invalid rev (0x%x) at line %d",
		    rev, __LINE__);
#endif 			    
            error_code = DD_E_INVALID_REVISION;
        }
        break;

    default:
#ifdef DBCONVERT_ENV
	CTprint(PRINT_MASK_ERROR,
		"dd_WhatIsNumUserExtUseTypes: Invalid service (0x%x) at line %d",
		service, __LINE__);
#endif 			    
        error_code = DD_E_INVALID_SERVICE;
    }
    return (error_code); 
}


/* ****** MISCELANEOUS DATA DIRECTORY UTILITY FUNCTIONS ***** */

/***********************************************************************
*                  dd_DboType2String()
***********************************************************************
*
* DESCRIPTION:
*   *MISCELANEOUS DATA DIRECTORY UTILITY FUNCTION*
*    This function converts DBO Type to text string.
*
* PARAMETERS:
*   service      - ODB service type;
*   rev          - Revision;
*   dbo_type     - Dbo Type to be converted;
*   str          - string to be filled with text value;
*
* RETURN VALUES/ERRORS:
*   TRUE -- error, FALSE otherwise;
*
* NOTES:
*    Calling layer should check for the returned value.
*
* HISTORY:
*   17-Dec-01: Created.                               Mike Zelikov (MZ)
*
***********************************************************************/
BOOL dd_DboType2String(const DD_SERVICE service, 
                       const DBO_REVISION rev,
                       const DBO_TYPE dbo_type, 
                       char str[50])
{
    BOOL error = FALSE;


    switch (service)
    {
    case DD_MAIN_SERVICE :

        if (rev == DD_MS_REVISION_ONE)
        {
            switch (dbo_type)
            {
            /* AUX TYPES */
            case DD_BOOTSERVICE_TYPE :
                dbo_strcpy(str, "DD_BOOTSERVICE_TYPE        ");
                break;

            /* MANAGEMENT TYPES */
            case DD_MASTER_DDE_TYPE :
                dbo_strcpy(str, "DD_MASTER_DDE_TYPE         ");
                break;

            case DD_INFO_TYPE :
                dbo_strcpy(str, "DD_INFO_TYPE               ");
                break;

            case DD_PREV_REV_MASTER_DDE_TYPE :
                dbo_strcpy(str, "DD_PREV_REV_MASTER_DDE_TYPE");
                break;

            case DD_USER_DDE_TYPE :
                dbo_strcpy(str, "DD_USER_DDE_TYPE           ");
                break;

            case DD_EXTERNAL_USE_DDE_TYPE :
                dbo_strcpy(str, "DD_EXTERNAL_USE_TYPE       ");
                break;

            case DD_PUBLIC_BIND_TYPE :
                dbo_strcpy(str, "DD_PUBLIC_BIND_TYPE        ");
                break;

            /* USER TYPES */
			case DD_MS_32K_ALIGNING_PAD_TYPE :
                dbo_strcpy(str, "DD_MS_32K_ALIGNING_PAD_TYPE        ");
                break;

            case DD_MS_FRU_SIGNATURE_PAD_TYPE :
                dbo_strcpy(str, "DD_MS_FRU_SIGNATURE_PAD_TYPE       ");
                break;

            case DD_MS_HW_FRU_VERIFY_PAD_TYPE :
                dbo_strcpy(str, "DD_MS_HW_FRU_VERIFY_PAD_TYPE       ");
                break;

            case DD_MS_EXPANSION_CHKPT_PAD_TYPE :
                dbo_strcpy(str, "DD_MS_EXPANSION_CHKPT_PAD_TYPE     ");
                break;

            case DD_MS_FRU_SIGNATURE_TYPE :
                dbo_strcpy(str, "DD_MS_FRU_SIGNATURE_TYPE           ");
                break;

            case DD_MS_HW_FRU_VERIFY_TYPE :
                dbo_strcpy(str, "DD_MS_HW_FRU_VERIFY_TYPE           ");
                break;

            case DD_MS_EXPANSION_CHKPT_TYPE :
                dbo_strcpy(str, "DD_MS_EXPANSION_CHKPT_TYPE         ");
                break;

            case DD_MS_CLEAN_DIRTY_TYPE :
                dbo_strcpy(str, "DD_MS_CLEAN_DIRTY_TYPE             ");
                break;

			case DD_MS_PADDER_TYPE:
                dbo_strcpy(str, "DD_MS_PADDER_TYPE                  ");
                break;

			case DD_MS_FLARE_PAD_TYPE :
                dbo_strcpy(str, "DD_MS_FLARE_PAD_TYPE               ");
                break;

            case DD_MS_SYSCONFIG_TYPE :
                dbo_strcpy(str, "DD_MS_SYSCONFIG_TYPE               ");
                break;

            case DD_MS_SYSCONFIG_PAD_TYPE :
                dbo_strcpy(str, "DD_MS_SYSCONFIG_PAD_TYPE           ");
                break;

            case DD_MS_GLUT_TYPE :
                dbo_strcpy(str, "DD_MS_GLUT_TYPE                    ");
                break;

            case DD_MS_GLUT_PAD_TYPE :
                dbo_strcpy(str, "DD_MS_GLUT_PAD_TYPE                ");
                break;

            case DD_MS_FRU_TYPE :
                dbo_strcpy(str, "DD_MS_FRU_TYPE                     ");
                break;

            case DD_MS_FRU_PAD_TYPE :
                dbo_strcpy(str, "DD_MS_FRU_PAD_TYPE                 ");
                break;

            case DD_MS_RAID_GROUP_TYPE :
                dbo_strcpy(str, "DD_MS_RAID_GROUP_TYPE              ");
                break;

            case DD_MS_RAID_GROUP_PAD_TYPE :
                dbo_strcpy(str, "DD_MS_RAID_GROUP_PAD_TYPE          ");
                break;

            case DD_MS_SPA_ULOG_TYPE :
                dbo_strcpy(str, "DD_MS_SPA_ULOG_TYPE                ");
                break;

            case DD_MS_SPA_ULOG_PAD_TYPE :
                dbo_strcpy(str, "DD_MS_SPA_ULOG_PAD_TYPE            ");
                break;

            case DD_MS_SPB_ULOG_TYPE :
                dbo_strcpy(str, "DD_MS_SPB_ULOG_TYPE                ");
                break;

            case DD_MS_SPB_ULOG_PAD_TYPE :
                dbo_strcpy(str, "DD_MS_SPB_ULOG_PAD_TYPE            ");
                break;

            case DD_MS_INIT_SPA_ULOG_TYPE :
                dbo_strcpy(str, "DD_MS_INIT_SPA_ULOG_TYPE           ");
                break;

            case DD_MS_INIT_SPA_ULOG_PAD_TYPE :
                dbo_strcpy(str, "DD_MS_INIT_SPA_ULOG_PAD_TYPE       ");
                break;

            case DD_MS_INIT_SPB_ULOG_TYPE :
                dbo_strcpy(str, "DD_MS_INIT_SPB_ULOG_TYPE           ");
                break;

            case DD_MS_INIT_SPB_ULOG_PAD_TYPE :
                dbo_strcpy(str, "DD_MS_INIT_SPB_ULOG_PAD_TYPE       ");
                break;

            case DD_MS_VERIFY_RESULTS_TYPE :
                dbo_strcpy(str, "DD_MS_VERIFY_RESULTS_TYPE          ");
                break;

            case DD_MS_VERIFY_RESULTS_PAD_TYPE :
                dbo_strcpy(str, "DD_MS_VERIFY_RESULTS_PAD_TYPE      ");
                break;

            case DD_MS_ENCL_TBL_TYPE :
                dbo_strcpy(str, "DD_MS_ENCL_TBL_TYPE                ");
                break;

            case DD_MS_ENCL_TBL_PAD_TYPE :
                dbo_strcpy(str, "DD_MS_ENCL_TBL_PAD_TYPE            ");
                break;

			case DD_MS_RESERVED_EXP_TYPE :
				dbo_strcpy(str, "DD_MS_RESERVED_EXP_TYPE            ");
				break;

            case DD_MS_EXTERNAL_DUMMY_TYPE:
                dbo_strcpy(str, "DD_MS_EXTERNAL_DUMMY_TYPE          ");
                break;


            /* EXTERNAL USE TYPES */
            case DD_MS_DISK_DIAGNOSTIC_TYPE :
                dbo_strcpy(str, "DD_MS_DISK_DIAGNOSTIC_TYPE         ");
                break;

            case DD_MS_DISK_DIAGNOSTIC_PAD_TYPE :
                dbo_strcpy(str, "DD_MS_DISK_DIAGNOSTIC_PAD_TYPE     ");
                break;

            case DD_MS_DDBS_TYPE:
                dbo_strcpy(str, "DD_MS_DDBS_TYPE                    ");
                break;

            case DD_MS_BIOS_UCODE_TYPE :
                dbo_strcpy(str, "DD_MS_BIOS_UCODE_TYPE              ");
                break;

            case DD_MS_PROM_UCODE_TYPE :
                dbo_strcpy(str, "DD_MS_PROM_UCODE_TYPE              ");
                break;

            case DD_MS_FUTURE_EXP_TYPE:
                dbo_strcpy(str, "DD_MS_FUTURE_EXP_TYPE              ");
				break;

            case DD_MS_GPS_FW_TYPE :
                dbo_strcpy(str, "DD_MS_GPS_FW_TYPE                  ");
                break;

            case DD_MS_BOOT_LOADER_TYPE:
                dbo_strcpy(str, "DD_MS_BOOT_LOADER_TYPE             ");
                break;

            case DD_MS_VAULT_RESERVED_TYPE :
                dbo_strcpy(str, "DD_MS_VAULT_RESERVED_TYPE          ");
                break;

            case DD_MS_VAULT_FUTURE_EXP_TYPE :
                dbo_strcpy(str, "DD_MS_VAULT_FUTURE_EXP_TYPE        ");
                break;

            case DD_MS_UTIL_SPA_PRIM_TYPE:
                dbo_strcpy(str, "DD_MS_UTIL_SPA_PRIM_TYPE           ");
                break;

            case DD_MS_UTIL_SPA_SEC_TYPE:
                dbo_strcpy(str, "DD_MS_UTIL_SPA_SEC_TYPE            ");
                break;

            case DD_MS_UTIL_SPB_PRIM_TYPE:
                dbo_strcpy(str, "DD_MS_UTIL_SPB_PRIM_TYPE           ");
                break;

            case DD_MS_UTIL_SPB_SEC_TYPE:
                dbo_strcpy(str, "DD_MS_UTIL_SPB_SEC_TYPE            ");
                break;

            case DD_MS_UTIL_SPA_PRIM_EXP_TYPE:
                dbo_strcpy(str, "DD_MS_UTIL_SPA_PRIM_EXP_TYPE       ");
                break;

            case DD_MS_UTIL_SPA_SEC_EXP_TYPE:
                dbo_strcpy(str, "DD_MS_UTIL_SPA_SEC_EXP_TYPE        ");
                break;

            case DD_MS_UTIL_SPB_PRIM_EXP_TYPE:
                dbo_strcpy(str, "DD_MS_UTIL_SPB_PRIM_EXP_TYPE       ");
                break;

            case DD_MS_UTIL_SPB_SEC_EXP_TYPE:
                dbo_strcpy(str, "DD_MS_UTIL_SPB_SEC_EXP_TYPE        ");
                break;

            case DD_MS_NT_BOOT_SPA_PRIM_TYPE :
                dbo_strcpy(str, "DD_MS_NT_BOOT_SPA_PRIM_TYPE        ");
                break;

            case DD_MS_NT_BOOT_SPA_SEC_TYPE :
                dbo_strcpy(str, "DD_MS_NT_BOOT_SPA_SEC_TYPE         ");
                break;

            case DD_MS_NT_BOOT_SPB_PRIM_TYPE :
                dbo_strcpy(str, "DD_MS_NT_BOOT_SPB_PRIM_TYPE        ");
                break;

            case DD_MS_NT_BOOT_SPB_SEC_TYPE :
                dbo_strcpy(str, "DD_MS_NT_BOOT_SPB_SEC_TYPE         ");
                break;

            case DD_MS_NT_BOOT_SPA_PRIM_EXP_TYPE :
                dbo_strcpy(str, "DD_MS_NT_BOOT_SPA_PRIM_EXP_TYPE    ");
                break;

            case DD_MS_NT_BOOT_SPA_SEC_EXP_TYPE :
                dbo_strcpy(str, "DD_MS_NT_BOOT_SPA_SEC_EXP_TYPE     ");
                break;

            case DD_MS_NT_BOOT_SPB_PRIM_EXP_TYPE :
                dbo_strcpy(str, "DD_MS_NT_BOOT_SPB_PRIM_EXP_TYPE    ");
                break;

            case DD_MS_NT_BOOT_SPB_SEC_EXP_TYPE :
                dbo_strcpy(str, "DD_MS_NT_BOOT_SPB_SEC_EXP_TYPE     ");
                break;

			case DD_MS_ICA_IMAGE_REP_EXP_TYPE:
                dbo_strcpy(str, "DD_MS_ICA_IMAGE_REP_EXP_TYPE       ");
                break;

			case DD_MS_ICA_IMAGE_REP_TYPE:
                dbo_strcpy(str, "DD_MS_ICA_IMAGE_REP_TYPE           ");
                break;

            case DD_MS_ICA_IMAGE_REP_PAD_TYPE:
                dbo_strcpy(str, "DD_MS_ICA_IMAGE_REP_PAD_TYPE       ");
                break;

            case DD_MS_PSM_RESERVED_TYPE:
                dbo_strcpy(str, "DD_MS_PSM_RESERVED_TYPE            ");
                break;

            case DD_MS_BOOT_DIRECTIVES_TYPE:
                dbo_strcpy(str, "DD_MS_BOOT_DIRECTIVES_TYPE         ");
                break;

            case DD_MS_FUTURE_GROWTH_TYPE:
                dbo_strcpy(str, "DD_MS_FUTURE_GROWTH_TYPE           ");
                break;

            case DD_MS_EXT_NEW_DB_DUMMY_TYPE:
                dbo_strcpy(str, "DD_MS_EXT_NEW_DB_DUMMY_TYPE        ");
                break;

            case DD_MS_UTIL_FLARE_DUMMY_TYPE:
                dbo_strcpy(str, "DD_MS_UTIL_FLARE_DUMMY_TYPE        ");
                break;

            case DD_MS_EXP_USER_DUMMY_TYPE:
                dbo_strcpy(str, "DD_MS_EXP_USER_DUMMY_TYPE          ");
                break;

            case DD_MS_PSM_BOOT_DUMMY_TYPE:
                dbo_strcpy(str, "DD_MS_PSM_BOOT_DUMMY_TYPE          ");
                break;

            case DD_MS_DDBS_LUN_OBJ_TYPE:
                dbo_strcpy(str, "DD_MS_DDBS_LUN_OBJ_TYPE            ");
                break;

            case DD_MS_BIOS_LUN_OBJ_TYPE:
                dbo_strcpy(str, "DD_MS_BIOS_LUN_OBJ_TYPE            ");
                break;

            case DD_MS_PROM_LUN_OBJ_TYPE:
                dbo_strcpy(str, "DD_MS_PROM_LUN_OBJ_TYPE            ");
                break;

            case DD_MS_GPS_FW_LUN_OBJ_TYPE:
                dbo_strcpy(str, "DD_MS_GPS_FW_LUN_OBJ_TYPE          ");
                break;

            case DD_MS_FLARE_DB_TYPE:
                dbo_strcpy(str, "DD_MS_FLARE_DB_TYPE                ");
                break;

            case DD_MS_FLARE_DB_LUN_OBJ_TYPE:
                dbo_strcpy(str, "DD_MS_FLARE_DB_LUN_OBJ_TYPE        ");
                break;

            case DD_MS_ESP_LUN_TYPE:
                dbo_strcpy(str, "DD_MS_ESP_LUN_TYPE                 ");
                break;

            case DD_MS_ESP_LUN_OBJ_TYPE:
                dbo_strcpy(str, "DD_MS_ESP_LUN_OBJ_TYPE             ");
                break;

            case DD_MS_VCX_LUN_0_TYPE:
                dbo_strcpy(str, "DD_MS_VCX_LUN_0_TYPE               ");
                break;

            case DD_MS_VCX_LUN_0_OBJ_TYPE:
                dbo_strcpy(str, "DD_MS_VCX_LUN_0_OBJ_TYPE           ");
                break;

            case DD_MS_VCX_LUN_1_TYPE:
                dbo_strcpy(str, "DD_MS_VCX_LUN_1_TYPE               ");
                break;

            case DD_MS_VCX_LUN_1_OBJ_TYPE:
                dbo_strcpy(str, "DD_MS_VCX_LUN_1_OBJ_TYPE           ");
                break;

            case DD_MS_VCX_LUN_2_TYPE:
                dbo_strcpy(str, "DD_MS_VCX_LUN_2_TYPE               ");
                break;

            case DD_MS_VCX_LUN_2_OBJ_TYPE:
                dbo_strcpy(str, "DD_MS_VCX_LUN_2_OBJ_TYPE           ");
                break;

            case DD_MS_VCX_LUN_3_TYPE:
                dbo_strcpy(str, "DD_MS_VCX_LUN_3_TYPE               ");
                break;

            case DD_MS_VCX_LUN_3_OBJ_TYPE:
                dbo_strcpy(str, "DD_MS_VCX_LUN_3_OBJ_TYPE           ");
                break;

            case DD_MS_VCX_LUN_4_TYPE:
                dbo_strcpy(str, "DD_MS_VCX_LUN_4_TYPE               ");
                break;

            case DD_MS_VCX_LUN_4_OBJ_TYPE:
                dbo_strcpy(str, "DD_MS_VCX_LUN_4_OBJ_TYPE           ");
                break;

            case DD_MS_VCX_LUN_5_TYPE:
                dbo_strcpy(str, "DD_MS_VCX_LUN_5_TYPE               ");
                break;

            case DD_MS_VCX_LUN_5_OBJ_TYPE:
                dbo_strcpy(str, "DD_MS_VCX_LUN_5_OBJ_TYPE           ");
                break;

            case DD_MS_CENTERA_LUN_TYPE:
                dbo_strcpy(str, "DD_MS_CENTERA_LUN_TYPE             ");
                break;

            case DD_MS_CENTERA_LUN_OBJ_TYPE:
                dbo_strcpy(str, "DD_MS_CENTERA_LUN_OBJ_TYPE         ");
                break;

            case DD_MS_PSM_LUN_OBJ_TYPE:
                dbo_strcpy(str, "DD_MS_PSM_LUN_OBJ_TYPE             ");
                break;

            case DD_MS_DRAM_CACHE_LUN_TYPE:
                dbo_strcpy(str, "DD_MS_DRAM_CACHE_LUN_TYPE          ");
                break;

            case DD_MS_DRAM_CACHE_LUN_OBJ_TYPE:
                dbo_strcpy(str, "DD_MS_DRAM_CACHE_LUN_OBJ_TYPE      ");
                break;

            case DD_MS_EFD_CACHE_LUN_TYPE:
                dbo_strcpy(str, "DD_MS_EFD_CACHE_LUN_TYPE           ");
                break;

            case DD_MS_EFD_CACHE_LUN_OBJ_TYPE:
                dbo_strcpy(str, "DD_MS_EFD_CACHE_LUN_OBJ_TYPE       ");
                break;

            case DD_MS_PSM_RAID_GROUP_OBJ_TYPE:
                dbo_strcpy(str, "DD_MS_PSM_RAID_GROUP_OBJ_TYPE      ");
                break;

            case DD_MS_ALIGN_BUFFER1_TYPE:
                dbo_strcpy(str, "DD_MS_ALIGN_BUFFER1_TYPE           ");
                break;

            case DD_MS_WIL_A_LUN_TYPE:
                dbo_strcpy(str, "DD_MS_WIL_A_LUN_TYPE               ");
                break;

            case DD_MS_WIL_A_LUN_OBJ_TYPE:
                dbo_strcpy(str, "DD_MS_WIL_A_LUN_OBJ_TYPE           ");
                break;

            case DD_MS_WIL_B_LUN_TYPE:
                dbo_strcpy(str, "DD_MS_WIL_B_LUN_TYPE               ");
                break;

            case DD_MS_WIL_B_LUN_OBJ_TYPE:
                dbo_strcpy(str, "DD_MS_WIL_B_LUN_OBJ_TYPE           ");
                break;

            case DD_MS_CPL_A_LUN_TYPE:
                dbo_strcpy(str, "DD_MS_CPL_A_LUN_TYPE               ");
                break;

            case DD_MS_CPL_A_LUN_OBJ_TYPE:
                dbo_strcpy(str, "DD_MS_CPL_A_LUN_OBJ_TYPE           ");
                break;

            case DD_MS_CPL_B_LUN_TYPE:
                dbo_strcpy(str, "DD_MS_CPL_B_LUN_TYPE               ");
                break;

            case DD_MS_CPL_B_LUN_OBJ_TYPE:
                dbo_strcpy(str, "DD_MS_CPL_B_LUN_OBJ_TYPE           ");
                break;

            case DD_MS_RAID_METADATA_LUN_TYPE:
                dbo_strcpy(str, "DD_MS_RAID_METADATA_LUN_TYPE       ");
                break;

            case DD_MS_RAID_METADATA_LUN_OBJ_TYPE:
                dbo_strcpy(str, "DD_MS_RAID_METADATA_LUN_OBJ_TYPE   ");
                break;

            case DD_MS_RAID_METASTATS_LUN_TYPE:
                dbo_strcpy(str, "DD_MS_RAID_METASTATS_LUN_TYPE      ");
                break;

            case DD_MS_RAID_METASTATS_LUN_OBJ_TYPE:
                dbo_strcpy(str, "DD_MS_RAID_METASTATS_LUN_OBJ_TYPE  ");
                break;

            case DD_MS_VAULT_LUN_OBJECT_TYPE:
                dbo_strcpy(str, "DD_MS_VAULT_LUN_OBJECT_TYPE        ");
                break;

            case DD_MS_VAULT_RAID_GROUP_OBJ_TYPE:
                dbo_strcpy(str, "DD_MS_VAULT_RAID_GROUP_OBJ_TYPE    ");
                break;

            case DD_MS_NON_LUN_RAID_GROUP_OBJ_TYPE:
                dbo_strcpy(str, "DD_MS_NON_LUN_RAID_GROUP_OBJ_TYPE  ");
                break;

            default:
                error = TRUE;
            }
        }
        else
        {
            error = TRUE;
        }
        break;

    default:
        error = TRUE;
    }

    return (error);

} /* dd_DboType2String() */


/***********************************************************************
*                  dd_String2DboType()
***********************************************************************
*
* DESCRIPTION:
*   *MISCELANEOUS DATA DIRECTORY UTILITY FUNCTION*
*    This function converts text string to DBO Type.
*
* PARAMETERS:
*   service      - ODB service type;
*   rev          - Revision;
*   str          - String to be converted
*   dbo_type     - Dbo Type to be filled;
*
* RETURN VALUES/ERRORS:
*   TRUE -- error, FALSE otherwise;
*
* NOTES:
*    Calling layer should check for the returned value.
*
* HISTORY:
*   17-Dec-01: Created.                               Mike Zelikov (MZ)
*
***********************************************************************/
BOOL dd_String2DboType(const DD_SERVICE service, 
                       const DBO_REVISION rev,
                       const char *str,
                       DBO_TYPE *dbo_type)
{
    BOOL error = FALSE, status=FALSE;



    switch (service)
    {
    case DD_MAIN_SERVICE :          
        if (rev == DD_MS_REVISION_ONE)
        {
            /* AUX TYPES */
            error = dbo_strcmp(str, "DD_BOOTSERVICE_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MASTER_DDE_TYPE;
                break;
            }

            /* MANAGEMENT TYPES */
            error = dbo_strcmp(str, "DD_MASTER_DDE_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MASTER_DDE_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_INFO_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_INFO_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_PREV_REV_MASTER_DDE_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_PREV_REV_MASTER_DDE_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_USER_DDE_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_USER_DDE_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_EXTERNAL_USE_DDE_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_EXTERNAL_USE_DDE_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_PUBLIC_BIND_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_PUBLIC_BIND_TYPE;
                break;
            }

            /* USER TYPES */

			error = dbo_strcmp(str, "DD_MS_32K_ALIGNING_PAD_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_32K_ALIGNING_PAD_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_FRU_SIGNATURE_PAD_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_FRU_SIGNATURE_PAD_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_HW_FRU_VERIFY_PAD_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_HW_FRU_VERIFY_PAD_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_EXPANSION_CHKPT_PAD_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_EXPANSION_CHKPT_PAD_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_FRU_SIGNATURE_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_FRU_SIGNATURE_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_HW_FRU_VERIFY_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_HW_FRU_VERIFY_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_EXPANSION_CHKPT_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_EXPANSION_CHKPT_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_CLEAN_DIRTY_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_CLEAN_DIRTY_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_PADDER_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_PADDER_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_FLARE_PAD_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_FLARE_PAD_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_SYSCONFIG_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_SYSCONFIG_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_SYSCONFIG_PAD_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_SYSCONFIG_PAD_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_GLUT_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_GLUT_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_GLUT_PAD_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_GLUT_PAD_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_FRU_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_FRU_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_FRU_PAD_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_FRU_PAD_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_RAID_GROUP_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_RAID_GROUP_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_RAID_GROUP_PAD_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_RAID_GROUP_PAD_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_VERIFY_RESULTS_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_VERIFY_RESULTS_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_VERIFY_RESULTS_PAD_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_VERIFY_RESULTS_PAD_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_ENCL_TBL_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_ENCL_TBL_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_ENCL_TBL_PAD_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_ENCL_TBL_PAD_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_RESERVED_EXP_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_RESERVED_EXP_TYPE;
                break;
            }
			
            error = dbo_strcmp(str, "DD_MS_EXTERNAL_DUMMY_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_EXTERNAL_DUMMY_TYPE;
                break;
            }

            /* EXTERNAL USE TYPES */
            error = dbo_strcmp(str, "DD_MS_DISK_DIAGNOSTIC_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_DISK_DIAGNOSTIC_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_DISK_DIAGNOSTIC_PAD_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_DISK_DIAGNOSTIC_PAD_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_DDBS_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_DDBS_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_BIOS_UCODE_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_BIOS_UCODE_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_PROM_UCODE_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_PROM_UCODE_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_FUTURE_EXP_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_FUTURE_EXP_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_GPS_FW_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_GPS_FW_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_BOOT_LOADER_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_BOOT_LOADER_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_EXP_USER_DUMMY_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_EXP_USER_DUMMY_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_VAULT_RESERVED_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_VAULT_RESERVED_TYPE;
                break;
            }

			error = dbo_strcmp(str, "DD_MS_VAULT_FUTURE_EXP_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_VAULT_FUTURE_EXP_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_UTIL_SPA_PRIM_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_UTIL_SPA_PRIM_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_UTIL_SPA_SEC_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_UTIL_SPA_SEC_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_UTIL_SPB_PRIM_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_UTIL_SPB_PRIM_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_UTIL_SPB_SEC_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_UTIL_SPB_SEC_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_UTIL_SPA_PRIM_EXP_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_UTIL_SPA_PRIM_EXP_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_UTIL_SPA_SEC_EXP_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_UTIL_SPA_SEC_EXP_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_UTIL_SPB_PRIM_EXP_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_UTIL_SPB_PRIM_EXP_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_UTIL_SPB_SEC_EXP_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_UTIL_SPB_SEC_EXP_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_NT_BOOT_SPA_PRIM_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_NT_BOOT_SPA_PRIM_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_NT_BOOT_SPA_SEC_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_NT_BOOT_SPA_SEC_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_NT_BOOT_SPB_PRIM_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_NT_BOOT_SPB_PRIM_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_NT_BOOT_SPB_SEC_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_NT_BOOT_SPB_SEC_TYPE;
                break;
            }


            error = dbo_strcmp(str, "DD_MS_NT_BOOT_SPA_PRIM_EXP_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_NT_BOOT_SPA_PRIM_EXP_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_NT_BOOT_SPA_SEC_EXP_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_NT_BOOT_SPA_SEC_EXP_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_NT_BOOT_SPB_PRIM_EXP_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_NT_BOOT_SPB_PRIM_EXP_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_NT_BOOT_SPB_SEC_EXP_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_NT_BOOT_SPB_SEC_EXP_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_ICA_IMAGE_REP_EXP_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_ICA_IMAGE_REP_EXP_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_ICA_IMAGE_REP_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_ICA_IMAGE_REP_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_ICA_IMAGE_REP_PAD_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_ICA_IMAGE_REP_PAD_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_PSM_RESERVED_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_PSM_RESERVED_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_DRAM_CACHE_LUN_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_DRAM_CACHE_LUN_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_EFD_CACHE_LUN_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_EFD_CACHE_LUN_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_BOOT_DIRECTIVES_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_BOOT_DIRECTIVES_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_FUTURE_GROWTH_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_FUTURE_GROWTH_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_EXT_NEW_DB_DUMMY_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_EXT_NEW_DB_DUMMY_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_UTIL_FLARE_DUMMY_TYPE", &status);
			if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_UTIL_FLARE_DUMMY_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_PSM_BOOT_DUMMY_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_PSM_BOOT_DUMMY_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_DDBS_LUN_OBJ_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_DDBS_LUN_OBJ_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_BIOS_LUN_OBJ_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_BIOS_LUN_OBJ_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_PROM_LUN_OBJ_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_PROM_LUN_OBJ_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_GPS_FW_LUN_OBJ_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_GPS_FW_LUN_OBJ_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_FLARE_DB_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_FLARE_DB_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_FLARE_DB_LUN_OBJ_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_FLARE_DB_LUN_OBJ_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_ESP_LUN_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_ESP_LUN_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_ESP_LUN_OBJ_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_ESP_LUN_OBJ_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_VCX_LUN_0_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_VCX_LUN_0_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_VCX_LUN_0_OBJ_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_VCX_LUN_0_OBJ_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_VCX_LUN_1_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_VCX_LUN_1_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_VCX_LUN_1_OBJ_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_VCX_LUN_1_OBJ_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_VCX_LUN_2_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_VCX_LUN_2_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_VCX_LUN_2_OBJ_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_VCX_LUN_2_OBJ_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_VCX_LUN_3_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_VCX_LUN_3_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_VCX_LUN_3_OBJ_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_VCX_LUN_3_OBJ_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_VCX_LUN_4_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_VCX_LUN_4_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_VCX_LUN_4_OBJ_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_VCX_LUN_4_OBJ_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_VCX_LUN_5_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_VCX_LUN_5_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_VCX_LUN_5_OBJ_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_VCX_LUN_5_OBJ_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_CENTERA_LUN_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_CENTERA_LUN_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_CENTERA_LUN_OBJ_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_CENTERA_LUN_OBJ_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_PSM_LUN_OBJ_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_PSM_LUN_OBJ_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_DRAM_CACHE_LUN_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_DRAM_CACHE_LUN_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_DRAM_CACHE_LUN_OBJ_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_DRAM_CACHE_LUN_OBJ_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_EFD_CACHE_LUN_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_EFD_CACHE_LUN_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_EFD_CACHE_LUN_OBJ_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_EFD_CACHE_LUN_OBJ_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_PSM_RAID_GROUP_OBJ_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_PSM_RAID_GROUP_OBJ_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_ALIGN_BUFFER1_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_ALIGN_BUFFER1_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_WIL_A_LUN_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_WIL_A_LUN_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_WIL_A_LUN_OBJ_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_WIL_A_LUN_OBJ_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_WIL_B_LUN_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_WIL_B_LUN_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_WIL_B_LUN_OBJ_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_WIL_B_LUN_OBJ_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_CPL_A_LUN_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_CPL_A_LUN_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_CPL_A_LUN_OBJ_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_CPL_A_LUN_OBJ_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_CPL_B_LUN_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_CPL_B_LUN_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_CPL_B_LUN_OBJ_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_CPL_B_LUN_OBJ_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_RAID_METADATA_LUN_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_RAID_METADATA_LUN_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_RAID_METADATA_LUN_OBJ_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_RAID_METADATA_LUN_OBJ_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_RAID_METASTATS_LUN_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_RAID_METASTATS_LUN_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_RAID_METASTATS_LUN_OBJ_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_RAID_METASTATS_LUN_OBJ_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_VAULT_LUN_OBJECT_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_VAULT_LUN_OBJECT_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_VAULT_RAID_GROUP_OBJ_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_VAULT_RAID_GROUP_OBJ_TYPE;
                break;
            }

            error = dbo_strcmp(str, "DD_MS_NON_LUN_RAID_GROUP_OBJ_TYPE", &status);
            if (error) return (error);
            if (status)
            {
                *dbo_type = DD_MS_NON_LUN_RAID_GROUP_OBJ_TYPE;
                break;
            }

            error = TRUE;
            break;
        }
        // fallthrough when !(rev == DD_MS_REVISION_ONE)

    default:
        error = TRUE;
    }     

    return(error);

} /* dd_String2DboType() */


/***********************************************************************
*                  dd_OdbRegion2String()
***********************************************************************
*
* DESCRIPTION:
*   *MISCELANEOUS DATA DIRECTORY UTILITY FUNCTION*
*    This function converts Odb region to text string.
*
* PARAMETERS:
*   dbo_type   - Dbo Type that represents Odb region to be converted;
*   str        - string to be filled with text value;
*
* RETURN VALUES/ERRORS:
*   TRUE -- error, FALSE otherwise;
*
* NOTES:
*    This function must be called from FCLI context.
*    Maxim string length is 14 characters + for '\0'
*
* HISTORY:
*   03-Jan-02: Created.                               Mike Zelikov (MZ)
*
***********************************************************************/
BOOL dd_OdbRegion2String(const DD_SERVICE service, 
                         const DBO_REVISION rev,
                         const DBO_TYPE dbo_type, 
                         char str[15])
{
    BOOL error = FALSE;


    switch (service)
    {
    case DD_MAIN_SERVICE :          
        if (rev == DD_MS_REVISION_ONE)
        {
            switch (dbo_type)
            {
            case DD_BOOTSERVICE_TYPE :
                dbo_strcpy(str, "DD BOOT SERVC");
                break;

            case DD_MASTER_DDE_TYPE :
                dbo_strcpy(str, "MANAGEMENT   ");
                break;

            case DD_PREV_REV_MASTER_DDE_TYPE :
                dbo_strcpy(str, "PREV REV DB  ");
                break;

            case DD_USER_DDE_TYPE :
                dbo_strcpy(str, "USER DB      ");
                break;

            case DD_EXTERNAL_USE_DDE_TYPE :
                dbo_strcpy(str, "EXTERNAL USE ");
                break;

            case DD_PUBLIC_BIND_TYPE :
                dbo_strcpy(str, "PUBLIC BIND  ");
                break;

            default:
                error = TRUE;
            }
        }
        else
        {
            error = TRUE;
        }
        break;

    default:
        error = TRUE;
    }

    return(FALSE);

} /* dd_DbRegion2String() */


/***********************************************************************
*                  dd_ChooseDde()
***********************************************************************
*
* DESCRIPTION:
*    This function acts as a wrapper for dd_Choose_User_ExtUse_Dde
*    for ease of use for modules like NtMirror and DDBS.
*
* PARAMETERS:
*   service            - ODB service type;
*   rev                - ODBS revision;
*   dbo_type           - USER or EXT USE dbo type;
*   dde_buf            - USER DDE data to be filled in;
*   fru                - Fru from which data was read;
*   dde_raw_arr        - Array of pointers to obtained Master DDEs raw DBOLB data;
*   dde_addr_arr       - Array of DDE addresses;
*   validity_mask_ptr  - Ptr to Address value to be returned;
*   dd_swap_ptr        - Ptr to fru position where this disk came from.
*   drive_type         - Fru drive type
*   current_mdde_rev   - Current revision of MDDE
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*   Validity mask set to DBO_ALL_COPIES_INVALID in the case of an
*   error.
*
* NOTES:
*    If a buffer is NULL pointer we do not look at address and blk_count
*    values -- One can pass all buffers as NULL ptrs and get the default 
*    DDE.
*
* HISTORY:
*   29-Jun-03: Created.                               Aparna V
*   05-Jul-05: Changed encl_type to encl_drive_type   Paul Motora
*
***********************************************************************/
DD_ERROR_CODE dd_ChooseDde(const DD_SERVICE service,
                  const DBO_REVISION rev,
                  const DBO_TYPE dbo_type,
                  DDE_DATA *dde_buf,
                  const UINT_32 fru,
                  const UINT_32 blk_count,
                  UINT_8E *dde_raw_arr[DD_NUM_DDE_COPIES],
                  DBOLB_ADDR dde_addr_arr[DD_NUM_DDE_COPIES],
                  UINT_32 *validity_mask_ptr,
                  UINT_32 *dd_swap_ptr,
                  FLARE_DRIVE_TYPE drive_type,
                  const DBO_REVISION current_mdde_rev)
{
    DBO_REVISION dde_info_rev = DBO_INVALID_REVISION;
    DD_ERROR_CODE error_code = DD_OK;

    /* Setting dde_rev to DBO_INVALID_REVISION will cause 
    * dd_Choose_User_ExtUse_Dde() to validate DDE based on
    * the info_rev contained in the DDE itself.*/

    switch(current_mdde_rev)
    {
    case MASTER_DDE_REVISION_ONE:
    case MASTER_DDE_REVISION_TWO:
        switch(dbo_type)
        {
        case DD_MS_UTIL_SPA_PRIM_TYPE:
        case DD_MS_UTIL_SPA_SEC_TYPE:
        case DD_MS_UTIL_SPB_PRIM_TYPE:
        case DD_MS_UTIL_SPB_SEC_TYPE:
            dde_info_rev = ((DBO_REVISION) 1);
            break;
        default:
            error_code = dd_WhatIsDefault_User_ExtUse_SetRev(service, rev, dbo_type, &dde_info_rev);
            if (error_code != DD_OK)
            {
                MP_ktracev("ODBS: dd_ChooseDde: dd_WhatIsDefault_User_ExtUse_SetRev returned error 0x%x, line %d\n", error_code, __LINE__);
            }
        }
        break;

    case MASTER_DDE_REVISION_ONE_HUNDRED:
        error_code = dd_WhatIsDefault_User_ExtUse_SetRev(service, rev, dbo_type, &dde_info_rev);
        if (error_code != DD_OK)
        {
            MP_ktracev("ODBS: dd_ChooseDde: dd_WhatIsDefault_User_ExtUse_SetRev returned error 0x%x, line %d\n", error_code, __LINE__);
        }
        
        break;

    case DBO_INVALID_REVISION:
        dde_info_rev = DBO_INVALID_REVISION;
        break;

    default:
        error_code = DD_E_INVALID_EXT_USE_DDE;
    }

    if (error_code == DD_OK)
    {
        error_code = dd_Choose_User_ExtUse_Dde(service, rev, dbo_type, dde_buf, fru, blk_count, dde_raw_arr, 
                                               dde_addr_arr, validity_mask_ptr, dd_swap_ptr, drive_type, dde_info_rev);
        if (error_code != DD_OK)
        {
            MP_ktracev("ODBS: dd_ChooseDde: dd_Choose_User_ExtUse_Dde returned error 0x%x, line %d\n", error_code, __LINE__);
        }
    }

    return(error_code);
}

/***********************************************************************
*                  dd_GetDefaultDde()
***********************************************************************
*
* DESCRIPTION:
*    This function acts as a wrapper for dd_GetDefault_User_ExtUse_SetInfo
*    for ease of use for modules like NtMirror and DDBS.
*
* PARAMETERS:
*    service   - ODBS service type;
*    rev       - Odbs revision;
*    fru       - fru index;
*    dbo_type  - DDE DBO Type;
*    dde       - DDE to be populated;
*    drive_type - Fru drive type;
*
* RETURN VALUES/ERRORS:
*   DD_ERROR_CODE - Error_code;
*
* NOTES:
*    Calling layer should check for the returned value.
*
* HISTORY:
*   29-Jun-03: Created.                               Aparna V
*   05-Jul-05: Changed encl_type to encl_drive_type   Paul Motora
*
***********************************************************************/
DD_ERROR_CODE dd_GetDefaultDde(const DD_SERVICE service,
                      const DBO_REVISION rev,
                      const UINT_32 fru,
                      const DBO_TYPE dbo_type,
                      DDE_DATA *dde,
                      FLARE_DRIVE_TYPE drive_type,
                      const DBO_REVISION current_mdde_rev)
{
    DBO_REVISION dde_info_rev = DBO_INVALID_REVISION;
    DBO_REVISION dde_rev = DBO_INVALID_REVISION;
    DBO_TYPE dde_type = DBO_INVALID_TYPE;
    DD_ERROR_CODE error_code = DD_OK;

    switch(current_mdde_rev)
    {
    case MASTER_DDE_REVISION_ONE:
    case MASTER_DDE_REVISION_TWO:
        switch(dbo_type)
        {
        case DD_MS_UTIL_SPA_PRIM_TYPE:
        case DD_MS_UTIL_SPA_SEC_TYPE:
        case DD_MS_UTIL_SPB_PRIM_TYPE:
        case DD_MS_UTIL_SPB_SEC_TYPE:
            dde_info_rev = ((DBO_REVISION) 1);
            break;
        default:
            error_code = dd_WhatIsDefault_User_ExtUse_SetRev(service,
                rev,
                dbo_type,
                &dde_info_rev);
            #ifdef DBCONVERT_ENV
                CTprint(PRINT_MASK_ERROR,
                             "dd_WhatIsDefault_User_ExtUse_SetRev() returned = 0x%x \n",error_code);
            #endif
           
        }
        break;

    case MASTER_DDE_REVISION_ONE_HUNDRED:
    default:
        error_code = dd_WhatIsDefault_User_ExtUse_SetRev(service,
            rev,
            dbo_type,
            &dde_info_rev);
        #ifdef DBCONVERT_ENV
                CTprint(PRINT_MASK_ERROR,
                             "dd_WhatIsDefault_User_ExtUse_SetRev() returned = 0x%x \n",error_code);
         #endif
        
        break;

    case DBO_INVALID_REVISION:
        dde_info_rev = DBO_INVALID_REVISION;
        break;

    }

    if (error_code == DD_OK)
    {
        dde_type = ((DBO_TYPE_IS_USER(dbo_type)) ?
        DD_USER_DDE_TYPE : DD_EXTERNAL_USE_DDE_TYPE);
        error_code = dd_WhatIsManagementDboCurrentRev(service,
            rev,
            dde_type, 
            &dde_rev);
        odb_assert(!error_code);
        error_code = dd_GetDefault_User_ExtUse_SetInfo(service, rev, dde_rev, fru,
                                                       dbo_type, dde, drive_type, dde_info_rev, DefaultLayout);
    }

    return(error_code);
}
