/***************************************************************************
 * Copyright (C) EMC Corp 2003 - 2014
 *
 * All rights reserved.
 * 
 * This software contains the intellectual property of EMC Corporation or
 * is licensed to EMC Corporation from third parties. Use of this software
 * and the intellectual property contained therein is expressly limited to
 * the terms and conditions of the License Agreement under which it is
 * provided by or on behalf of EMC
 **************************************************************************/

/*******************************************************************************
 *  cpd_sata.h
 *******************************************************************************/
#ifndef CPD_SATA_H
#define CPD_SATA_H

/* Includes */
#ifndef UEFI
#include "generic_types.h"
#else
#include "cpd_uefi_types.h"
#endif

#define COMMAND_BIT                              0x80
#define COMMAND_FIS                              0x27
#define COMMAND_PAYLOAD                          0x46
#define LBA_ADDRESS                              0x40 /*LAB bit need to be set to specify the address is an LBA*/
#define ATA_COMMAND_READ_SECTORS                 0x20
#define ATA_COMMAND_READ_SECTORS_EXT             0x24
#define ATA_COMMAND_READ_LOG_EXT                 0x2F
#define ATA_COMMAND_READ_VERIFY_SECTORS          0x40
#define ATA_COMMAND_READ_VERIFY_SECTORS_EXT      0x42
#define ATA_COMMAND_READ_BUFFER                  0xE4
#define ATA_COMMAND_WRITE_BUFFER                 0xE8
#define ATA_COMMAND_WRITE_SECTORS                0x30
#define ATA_COMMAND_WRITE_SECTORS_EXT            0x34
#define ATA_COMMAND_DIAGNOSTIC                   0x90
#define ATA_COMMAND_SMART                        0xb0
#define ATA_COMMAND_READ_MULTIPLE                0xc4
#define ATA_COMMAND_WRITE_MULTIPLE               0xc5
#define ATA_COMMAND_STANDBY_IMMEDIATE            0xe0
#define ATA_COMMAND_IDLE_IMMEDIATE               0xe1
#define ATA_COMMAND_STANDBY                      0xe2
#define ATA_COMMAND_IDLE                         0xe3
#define ATA_COMMAND_CHECK_POWER_MODE             0xE5
#define ATA_COMMAND_SLEEP                        0xe6
#define ATA_COMMAND_IDENTIFY                     0xec
#define ATA_COMMAND_DEVICE_CONFIG                0xb1
#define ATA_COMMAND_SET_FEATURES                 0xef
#define ATA_COMMAND_SET_ADVANCED_FEATURES        0xc3
#define ATA_COMMAND_WRITE_DMA                    0xca
#define ATA_COMMAND_WRITE_DMA_EXT                0x35
#define ATA_COMMAND_WRITE_DMA_QUEUED             0xcc
#define ATA_COMMAND_WRITE_DMA_QUEUED_EXT         0x36
#define ATA_COMMAND_WRITE_FPDMA_QUEUED_EXT       0x61
#define ATA_COMMAND_READ_DMA                     0xc8
#define ATA_COMMAND_READ_DMA_EXT                 0x25
#define ATA_COMMAND_READ_DMA_QUEUED              0xc7
#define ATA_COMMAND_READ_DMA_QUEUED_EXT          0x26
#define ATA_COMMAND_READ_FPDMA_QUEUED_EXT        0x60
#define ATA_COMMAND_FLUSH_CACHE                  0xe7
#define ATA_COMMAND_FLUSH_CACHE_EXT              0xea
#define ATA_COMMAND_READ_NATIVE_MAX_ADDRESS_EXT  0x27
#define ATA_COMMAND_SET_MAX_ADDRESS_EXT          0x37
#define ATA_WRITE_SAME_EXT                       0xfd
#define ATA_COMMAND_READ_PORT_MUX                0xc2
#define ATA_COMMAND_WRITE_PORT_MUX               0xc3

#define ATA_LOG_PAGE_10                          0x10

#define SATA_ABORT_NO_AIB_NEEDED                 0x80000000

#define ATA_READ_LOG_EXT_BUFFER_SIZE             512

#define NQ      0x80
#define TAG     0x1F

typedef struct ATA_READ_LOG_PAGE_10_TAG
{
    UINT_8E     tag;
    UINT_8E     rsvd0;
    UINT_8E     status;
    UINT_8E     error;
    UINT_8E     lba_low;
    UINT_8E     lba_mid;
    UINT_8E     lba_high;
    UINT_8E     device;
    UINT_8E     lba_low_exp;
    UINT_8E     lba_mid_exp;
    UINT_8E     lba_high_exp;
    UINT_8E     rsvd;
    UINT_8E     sector_count;
    UINT_8E     sector_count_exp;
    UINT_8E     reserved[242];
    UINT_8E     vendor_specific[255];
    UINT_8E     checksum;

}ATA_READ_LOG_PAGE_10,*PATA_READ_LOG_PAGE_10;


// SATA Register - Host to Device FIS
typedef struct SATA_CMD_FIS_TAG
{
    UINT_8E fis_type; // 0X27
    UINT_8E pm_port_and_cbit; // |C|Res|Res|Res|PM PORT(4 Bits)|
    UINT_8E command;
    UINT_8E features;
    UINT_8E lba_low;
    UINT_8E lba_mid;
    UINT_8E lba_high;
    UINT_8E device;
    UINT_8E lba_low_exp;
    UINT_8E lba_mid_exp;
    UINT_8E lba_high_exp;
    UINT_8E features_exp;
    UINT_8E sector_count;
    UINT_8E sector_count_exp;
    UINT_8E reserved;
    UINT_8E control;
    UINT_32E reserved2;
} SATA_CMD_FIS, *PSATA_CMD_FIS;

CPD_COMPILE_TIME_ASSERT(30, sizeof(SATA_CMD_FIS) == 20);


// SATA Register - Device  to Host FIS
typedef struct SATA_DEVICE_FIS_TAG
{
    UINT_8E fis_type; // 0X34
    UINT_8E pm_port_and_ibit; // |Res|I|Res|Res|PM PORT(4 Bits)|
    UINT_8E status;
    UINT_8E error;
    UINT_8E lba_low;
    UINT_8E lba_mid;
    UINT_8E lba_high;
    UINT_8E device;
    UINT_8E lba_low_exp;
    UINT_8E lba_mid_exp;
    UINT_8E lba_high_exp;
    UINT_8E reserved0;
    UINT_8E sector_count;
    UINT_8E sector_count_exp;
    UINT_16E reserved;
    UINT_32E reserved2;
} SATA_DEVICE_FIS, *PSATA_DEVICE_FIS;

#endif  /* CPD_SATA_H */
