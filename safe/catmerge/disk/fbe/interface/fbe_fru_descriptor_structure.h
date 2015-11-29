/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_fru_descriptor_structure.h
***************************************************************************
*
* @brief
*  This file contains the basic data structures of fru descriptor
*  Homewrecker and some other logics may use this structure
*
* @version
*	11/24/2011 - Created. zphu
*
***************************************************************************/


#ifndef FBE_FRU_DESCRIPTOR_STRUCTURES_H
#define FBE_FRU_DESCRIPTOR_STRUCTURES_H

#define     FBE_FRU_DESCRIPTOR_MAGIC_STRING  "$FBE_FRU_DESCRIPTOR_STRUCTURE_OHMY$"
#define     FBE_FRU_DESCRIPTOR_MAGIC_STRING_LENGTH	0x23
#define     FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER  4          
#define     FBE_FRU_DESCRIPTOR_DB_DRIVE_NUMBER      3          /*the first 3 system drives have fru descriptor private space region*/
/* the first number in FRU_Desc should be 233391
  * for Michael + Scottie + Dennis = BULLS in 95 to 98
  */
#define     FBE_FRU_DESCRIPTOR_DEFAULT_SEQ_NUMBER   233390 
#define     FBE_FRU_DESCRIPTOR_INVALID_SEQ_NUMBER   0
#define     FBE_FRU_DESCRIPTOR_STRUCTURE_VERSION    0x1

#define     FBE_CHASSIS_REPLACEMENT_FLAG_TRUE       0xFBEEDC    /*when set the chassis replacement flag, this value should be used */
#define     FBE_CHASSIS_REPLACEMENT_FLAG_INVALID    0x0         /*when unset/clear the chassis replacement flag, this value is used */

typedef     fbe_u8_t serial_num_t[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1];

/* pack these structures on 1-byte boundaries,
  * which is standard for wire protocols */

/* Modified this structure @2013 Feb 26th
  * Adding sequence number into FRU Descriptor
  * Because, raw_mirror can not be trusted before Homewrecker
  * does system drives validation.
  * Then Homewrecker needs to do FRU Descriptor validation by
  * itself, so sequence number is necessary, besides sequence number
  * we already have wwn_seed in FRU Descriptor. It would be also used
  * when Homewrecker does FRU Descritor validation.
*/
#pragma pack(1)
typedef struct
{
    fbe_char_t      magic_string[FBE_FRU_DESCRIPTOR_MAGIC_STRING_LENGTH + 1];           /*label whether the region on this drive is initialized*/
    fbe_u32_t       wwn_seed;                                                           /*the wwn from the midplane PROM.*/
    serial_num_t    system_drive_serial_number[FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER];
    fbe_u32_t       chassis_replacement_movement;
    fbe_u32_t       sequence_number;
    fbe_u32_t       structure_version;
} fbe_homewrecker_fru_descriptor_t;
#pragma pack()

#endif
