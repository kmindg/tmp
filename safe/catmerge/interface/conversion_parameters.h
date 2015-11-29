#ifndef CONVERSION_PARAMETERS_H
#define CONVERSION_PARAMETERS_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2005-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  FILE NAME: 
 *      conversion_parameters.h
 *
 *  DESCRIPTION
 *      This is the main header file for conversion tools.  It includes the
 *      interface header file and defines global/shared parameters.
 *
 *  HISTORY
 *      12-Aug-2005      Created
 *      01-Sep-2005      Added "rollback" info to CONVERSION_DISK_STATUS.
 *      09-Sep-2008	 Added CT_DM_GetIOModuleSummary_ParameterString.
 * 
 ***************************************************************************/
#include "ct_interface.h"

//  Source and target platforms for conversions.
#define FromModel_ParameterString         "CONV_FromModel"
#define ToModel_ParameterString           "CONV_ToModel"
#define FromModelType_ParameterString     "CONV_FromModelType"

//  Path to the RAM disk.
#define RamDiskPrefix_ParameterString     "UFE_RamDiskPrefix"

//  Number of private disks with flare boot images.
#define CONV_NUM_FLARE_BOOT_DISKS         4

/***************************************************************************
 *  NAME: 
 *      CONVERSION_DISK_*
 *
 * DESCRIPTION
 *      Supporting definitions for conversion structures.
 * 
 ***************************************************************************/

//  An enum used in CONVERSION_DISK_STATUS to indicate the state of an
//  individual disk with respect to conversion or reversion operations.
typedef enum _CONVERSION_DISK_STATE
{
    DISK_STATUS_UNKNOWN = 0, // The state of the disk is unknown.
    DISK_OK,                 // The disk is OK; all conversion/reversion
                             // operations so far on this have succeeded.
    DISK_INVALID,            // An operation on this disk failed due to a
                             // non-I/O error.
    DISK_FAILED,             // An operation on this disk failed due to an
                             // I/O error.
    DISK_MISSING             // The disk is not present. (No operation was done.)
} CONVERSION_DISK_STATE;

//  Array containing status information records about the private disks.
#define DiskHealth_ParameterString        "CONV_DiskHealth"

//  Max length of the failure message in CONVERSION_DISK_STATUS.
#define CONV_MAX_FAILURE_MSG_LENGTH       60

//  Number of private space disks.
#define CONV_NUM_PRIVATE_DISKS            5

/***************************************************************************
 * NAME: 
 *   CONVERSION_DISK_STATUS
 *
 * DESCRIPTION
 *      This structure defines the location of a disk and information 
 *      about the disk's health and conversion status.
 *
 *      cds_ConversionXYZ fields have status as of the most recent
 *      conversion operation (going forward).
 *
 *      cds_ReversionXYZ fields have status of the most recent reversion
 *      operation (going backward), if any. If no reversion has been done,
 *      the cds_ReversionDiskState should be DISK_STATUS_UNKNOWN.
 * 
 ***************************************************************************/
typedef struct _CONVERSION_DISK_STATUS
{
    UINT_16E                    cds_BusNumber;
    UINT_16E                    cds_DiskNumber;
    CONVERSION_DISK_STATE       cds_ConversionDiskState;        // Conversion Status
    CHAR                        cds_ConversionFailureMessage[CONV_MAX_FAILURE_MSG_LENGTH];
    CONVERSION_DISK_STATE       cds_ReversionDiskState;         // Reversion Status
    CHAR                        cds_ReversionFailureMessage[CONV_MAX_FAILURE_MSG_LENGTH];
} CONVERSION_DISK_STATUS;

/***************************************************************************
 *  NAME: 
 *      CONVERSION_DISK_HEALTH
 *
 *  DESCRIPTION
 *      This structure contains status records for each private space disk.
 * 
 ***************************************************************************/
typedef struct _CONVERSION_DISK_HEALTH
{
    CONVERSION_DISK_STATUS      cdh_DiskStatus[CONV_NUM_PRIVATE_DISKS];
} CONVERSION_DISK_HEALTH;

/***************************************************************************
 *  NAME: 
 *      CONV_PSM*
 *
 *  DESCRIPTION
 *      Supporting definitions for PSM related structures.
 * 
 ***************************************************************************/

//  Current and new PSM LU locations on private disks.
#define PSMLunCurrentRegionInfo_ParameterString   "CONV_PSMLunCurrentRegionInfo"
#define PSMLunNewRegionInfo_ParameterString       "CONV_PSMLunNewRegionInfo"

//  Boolean indicating if a conversion is required.
#define PSMLunConversionNeeded_ParameterString    "CONV_PSMLunConversionNeeded"

//  Number of disks containig the PSM LU.
#define CONV_NUM_PSM_DISKS                        3

/***************************************************************************
 *  NAME: 
 *      CONV_PSM_LUN_REGION_INFO
 *
 *  DESCRIPTION
 *      This structure contains region information for each PSM disk.
 *      CT_REGION_DESCRIPTOR_TYPE comes from ct_interface.h.
 * 
 *  NOTE
 *      At present, the only difference between the various region values
 *      is the disk number. All other fields should be the same value.
 *      (The PSM lun is a triple mirror, at the same address on the disks).
 *
 ***************************************************************************/
typedef struct _CONV_PSM_LUN_REGION_INFO
{
    CT_REGION_DESCRIPTOR_TYPE   cpl_PSMLunRegionInfo[CONV_NUM_PSM_DISKS];
} CONV_PSM_LUN_REGION_INFO;


/***************************************************************************
 *  NAME: 
 *      CT_DM...
 *
 *  DESCRIPTION
 *      This series of parameters provide simulation support for
 *      the CT_DM_ CTLib routines. In the normal (user) environent,
 *      these routines call the appropiate driver (SPECL, SPID, etc.)
 *      for execution. In the simulation environment, these drivers are
 *      not available, so the data returned is pulled from these parameters.
 * 
 *  NOTE
 *      If the 'required' simulation parameter isn't available or does not
 *      exist, an error is returned by CTLib.
 *
 ***************************************************************************/

#define CTDMIOStatusThisBlade_ParameterString   "CT_DM_IoStatusThisBlade"
#define CTDMIOSummary_ParameterString		"CT_DM_IoSummary"
#define CTDMSpId_ParameterString		"CT_DM_SpId"
#define CTDMHwType_ParameterString		"CT_DM_HwType"
#define CTDMPeerHwType_ParameterString		"CT_DM_PeerHwType"
#define CTDMGetHwName_ParameterString		"CT_DM_HwName"

#endif
