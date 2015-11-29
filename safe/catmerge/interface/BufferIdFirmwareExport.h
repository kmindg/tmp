/*******************************************************************************
 * Copyright (C) EMC Corporation, 2000.
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

/*******************************************************************************
 * BufferIdFirmwareExport.h
 *
 * This header file defines structs used in the Read/Write buffer information 
 * for Flare.
 *
 *	Revision History
 *	----------------
 *	11 Aug 00	D. Zeryck	Initial version.
  *	14 Aug 00	D. Zeryck	Add FILE_MICROCODE_BEGIN_OFFSET.
 ******************************************************************************/

#if !defined (BUFFERIDFIRMWARE_EXPORT_H)
#define BUFFERIDFIRMWARE_EXPORT_H

#if !defined(GM_INSTALL_EXPORT_H)
#include "gm_install_export.h"
#endif


/*
 * The firmware metadata and data must follow the firmware update data structure
 * immediately in memory and must be accounted for as part of the input
 * size of the write buffer IOCTL.
 */
typedef struct firmware_update_data_struct_rev1
{
    ULONG                   revision;           // Revision number
    unsigned char           sequenceNumber;     // Sequence number for those requiring it
    ULONG                   firmwareDataSize;   // Size of firmware metadata and data
} FIRMWARE_UPDATE_DATA_STRUCT_REV1;

typedef union
{ 
    HEADER_BLOCK_STRUCT headerBlock;        // Controls what FRUs do and don't get updated
                                            // Must be last in structure to form a
                                            //  contiguous buffer with the actual firmware
    char                headerBlockBuffer[INSTALL_HEADER_SIZE];
                                            // Because it's expected to have room to grow
} FIRMWARE_UPDATE_HEADER_BLOCK_UNION;

typedef FIRMWARE_UPDATE_DATA_STRUCT_REV1    FIRMWARE_UPDATE_DATA, *PFIRMWARE_UPDATE_DATA;

#define FIRMWARE_UPDATE_DATA_REV1           0x19990921

#define FIRMWARE_UPDATE_DATA_REVISION       FIRMWARE_UPDATE_DATA_REV1



/*
 * Firmware metadata in the file delivered by Navi to Admin has the HEADER_BLOCK_STRUCT and some reserved space at the beginning
 */
#define FILE_MICROCODE_BEGIN_OFFSET		0x2000

#endif //BUFFERIDFIRMWARE_EXPORT_H
