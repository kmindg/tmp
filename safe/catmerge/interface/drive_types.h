#ifndef DRIVE_TYPES_H
#define DRIVE_TYPES_H 0x00000001 /* from common dir */
/**********************************************************************
 *  Copyright (C)  EMC Corporation 2009
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*
 * Define the various drive types outside of the Flare environment.
 * Other subsystems, such as DDBS and POST may desire to use them,
 * but they should not have to pull in definitions for the rest of Flare.
 *
 * Created: 9/1/2009, J. Cook
 */

typedef enum flare_drive_type_tag
{
    UNKNOWN_DRIVE_TYPE =                0x00,
    VALID_DRIVE_TYPE_MIN =              0x01,
    FC_DRIVE_TYPE      =                0x01,
    KLONDIKE_ATA_DRIVE_TYPE =           0x02,
    SATA_DRIVE_TYPE =                   0x03,
    NORTHSTAR_ATA_DRIVE_TYPE =          0x04,
    SAS_DRIVE_TYPE   =                  0x05,
    FSSD_FC_DRIVE_TYPE      =           0x06,
    FSSD_SATA_DRIVE_TYPE    =           0x07,
    FLASH_SATA_DRIVE_TYPE    =          0x08,
    FLASH_SAS_DRIVE_TYPE    =           0x09, /*For SAS EFD drive */
    NL_SAS_DRIVE_TYPE    =              0x0A,
    FLASH_SAS_ME_DRIVE_TYPE  =          0x0B, /*For SAS ME SSD drive */
    FLASH_SAS_LE_DRIVE_TYPE  =          0x0C, /*For SAS LE SSD drive */
    FLASH_SAS_RI_DRIVE_TYPE  =          0x0D, /*For SAS RI SSD drive */
    VALID_DRIVE_TYPE_MAX =              FLASH_SAS_RI_DRIVE_TYPE,
    ENCL_DRIVE_TYPE_MISMATCH =          0xFF
    // should another drive type be added, we need to update
    // the polling routine in adm_poll_encl_drive()
}
FLARE_DRIVE_TYPE;

#define IS_DRIVE_TYPE_VALID(drive_type) ((drive_type >= VALID_DRIVE_TYPE_MIN) && (drive_type <= VALID_DRIVE_TYPE_MAX)) 

typedef enum flare_drive_price_class_tag
{
    DRIVE_PRICE_CLASS_J = 0x00,
    DRIVE_PRICE_CLASS_B,
    DRIVE_PRICE_CLASS_UNKNOWN,
    DRIVE_PRICE_CLASS_LAST = 0xFF
}
FLARE_DRIVE_PRICE_CLASS;


#endif