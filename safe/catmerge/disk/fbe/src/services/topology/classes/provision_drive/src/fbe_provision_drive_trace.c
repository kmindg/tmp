/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_base_object_trace.c
 ****************************************************************************
 *
 * @brief
 *  This file contains the trace functions for Base Object.
 *
 * @author
 *   02/13/2009: - Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_base_object.h"
#include "fbe_provision_drive_private.h"
#include "fbe_base_object_trace.h"
#include "fbe/fbe_raid_group.h"
#include "fbe_raid_geometry.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!***************************************************************************
 * fbe_provision_drive_trace_get_drive_type_string()
 ******************************************************************************
 * @brief
 *  Return a string for Drive Type.
 *
 * @param drive_type - drive type for the object.
 *
 * @return - char * string that represents drive type.
 *
 * @author
 *  28/12/2010 - Created. Prafull P
 *
 ****************************************************************/
char *
fbe_provision_drive_trace_get_drive_type_string(fbe_drive_type_t drive_type)
{
    char *drive_type_String;
	
    switch (drive_type)
    {
        case FBE_DRIVE_TYPE_INVALID:
            drive_type_String = "FBE_DRIVE_TYPE_INVALID";
            break;
        case FBE_DRIVE_TYPE_FIBRE:
            drive_type_String = "FBE_DRIVE_TYPE_FIBRE";
            break;
        case FBE_DRIVE_TYPE_SAS:
            drive_type_String = "FBE_DRIVE_TYPE_SAS";
            break;
        case FBE_DRIVE_TYPE_SATA:
            drive_type_String = "FBE_DRIVE_TYPE_SATA";
            break;
        case FBE_DRIVE_TYPE_SAS_FLASH_HE:
            drive_type_String = "FBE_DRIVE_TYPE_SAS_FLASH_HE";
            break;
        case FBE_DRIVE_TYPE_SATA_FLASH_HE:
            drive_type_String = "FBE_DRIVE_TYPE_SATA_FLASH_HE";
            break;
        case FBE_DRIVE_TYPE_SAS_NL:
            drive_type_String = "FBE_DRIVE_TYPE_SAS_NL";
            break;
        case FBE_DRIVE_TYPE_SAS_FLASH_ME:
            drive_type_String = "FBE_DRIVE_TYPE_SAS_FLASH_ME";
            break;            
        case FBE_DRIVE_TYPE_SAS_FLASH_LE:
            drive_type_String = "FBE_DRIVE_TYPE_SAS_FLASH_LE";
            break;            
        case FBE_DRIVE_TYPE_SAS_FLASH_RI:
            drive_type_String = "FBE_DRIVE_TYPE_SAS_FLASH_RI";
            break;            
	case FBE_DRIVE_TYPE_SATA_PADDLECARD:
	    drive_type_String = "FBE_DRIVE_TYPE_SATA_PADDLECARD";
	    break;
	case FBE_DRIVE_TYPE_SAS_SED:
	    drive_type_String = "FBE_DRIVE_TYPE_SAS_SED";
	    break;
	default:
            drive_type_String = "UNKNOWN LIFECYCLE STATE";
            break;      
    };

    return(drive_type_String);
}
/******************************************
 * end fbe_provision_drive_trace_get_drive_type_string()
 ******************************************/


char * fbe_provision_drive_trace_get_config_type_string(fbe_provision_drive_config_type_t config_type)
{
    char *config_type_String;

    switch(config_type)
    {
        case FBE_PROVISION_DRIVE_CONFIG_TYPE_INVALID:
            config_type_String = "INVALID";
            break;

        case FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED:
            config_type_String = "UNKNOWN";
            break;

        case FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE:
            config_type_String = "SPARE";
            break;

        case FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID:
            config_type_String = "RAID";
            break;

        default :
            config_type_String = "Unknown config_type";
            break;
    }
    return(config_type_String);
}
