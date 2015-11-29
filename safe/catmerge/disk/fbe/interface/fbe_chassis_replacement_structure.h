/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_chassis_replacement_structure.h
***************************************************************************
*
* @brief
*  This file contains the basic data structures of chassis replacement flags
*  Homewrecker and some other logics may use this structure
*
* @version
*	11/23/2011 - Created. zphu
*
***************************************************************************/


#ifndef FBE_CHASSIS_REPLACEMENT_STRUCTURES_H
#define FBE_CHASSIS_REPLACEMENT_STRUCTURES_H


#define     FBE_CHASSIS_REPLACEMENT_FLAG_VERSION	0x1
#define     FBE_CHASSIS_REPLACEMENT_FLAG_MAGIC_STRING	"$FBE_CHASSIS_REPLACEMENT_FLAG_ON_DISK_STRUCTURE$"
#define     FBE_CHASSIS_REPLACEMENT_FLAG_MAGIC_STRING_LENGTH	0x30
#define     FBE_CHASSIS_REPLACEMENT_FLAG_TRUE	0xFBEEDC    /*when set the chassis replacement flag, this value should be used */
#define     FBE_CHASSIS_REPLACEMENT_FLAG_INVALID	0x0    /*when unset/clear the chassis replacement flag, this value is used */

#pragma pack(1)

/* fbe_chassis_replacement_flags_t */
typedef struct
{
    fbe_char_t  magic_string[FBE_CHASSIS_REPLACEMENT_FLAG_MAGIC_STRING_LENGTH + 1];
    fbe_u32_t   structure_version;
    fbe_u32_t   chassis_replacement_movement;  /* only two values are permitted: FBE_CHASSIS_REPLACEMENT_FLAG_TRUE and FBE_CHASSIS_REPLACEMENT_FLAG_INVALID */
    fbe_u32_t   seq_no;
} fbe_chassis_replacement_flags_t;

#pragma pack()

#endif
