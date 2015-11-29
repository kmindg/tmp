/**************************************************************************
 * Copyright (C) EMC Corporation, 2000.
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 *************************************************************************/

/**************************************************************************
 * BufferIdFirmwareBios.h
 *
 * This header file defines structs used in the Read/Write buffer
 * information for Flare.
 *
 *	Revision History
 *	----------------
 *	26 Dec 00	J. Shimkus	Initial version.
 *************************************************************************/

#if !defined (BUFFERIDBIOS_EXPORT_H)
#define BUFFERIDBIOS_EXPORT_H

#if !defined(GM_INSTALL_EXPORT_H)
#include "gm_install_export.h"
#endif

/* The following is a header immediately preceding the 
 * image located in the data buffer.
 */
typedef struct base_fw_update_data_struct_rev1
{
    unsigned int revision;           // Revision number
    unsigned char sequenceNumber;    // Sequence number if needed
    unsigned int biosDataSize;       // Size of BIOS metadata and data
} BASE_FW_UPDATE_DATA_STRUCT_REV1;

typedef BASE_FW_UPDATE_DATA_STRUCT_REV1 BASE_FW_UPDATE_DATA;

/* This is a revision number for the makeup of the buffer
 * being passed between the Admin and Flare drivers.  This
 * revision does not change for every new version of the
 * image.
 */
#define BIOS_UPDATE_DATA_REVISION   0x01082002
#define POST_UPDATE_DATA_REVISION   0x01082002
#define DDBS_UPDATE_DATA_REVISION   0x01082002 
#define GPS_FW_UPDATE_DATA_REVISION   0x01082002 

#endif // BUFFERIDBIOS_EXPORT_H
