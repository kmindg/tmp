//***************************************************************************
// Copyright (C)  EMC Corporation 2007
// All rights reserved.
// Licensed material - property of EMC Corporation. * All rights reserved.
//**************************************************************************/
#ifndef _K10_PLX_H_
#define _K10_PLX_H_


//++
// File Name:
//      K10Plx.h
//
// Contents:
//      NVRAM Constants for the PLX Device
//
//      These constants do not have PLX prefix, as future
//      Longbows may use a different physical device. 
//      As long as "K10_NVRAM_PLX_DG_PART_NUMBER_OFFSET" always
//      refers to the offset of that Part Number in a given
//      device, code changes to access that Part Number in
//      a new device should be minimal
//
//      DO NOT CHANGE THE SIZES OF ANY REGIONS WITHOUT CONTACTING GPS
//
// Exported:
//
// Revision History:
//      11/7/00    MWagner   Created
//
//--

#ifdef C4_INTEGRATED
#include "environment.h"
#endif /* C4_INTEGRATED - C4HW */

//++
// Macro:
//      K10_NVRAM_PLX_DEVICE
//
// Description:
//      If K10_NVRAM_PLX_DEVICE is defined, then K10NvRam.h will use
//      Plx specific Data Offsets and Lengths.
//      
// Revision History:
//      11/10/00   MWagner    Created.
//
//      10/08/07 scz Alter NVRAM address for Memory Persistence to new location 
//
//--
#define K10_NVRAM_PLX_DEVICE                 1
//++
// .End K10_NVRAM_PLX_DEVICE
//--

//++
// Macro:
//      K10_NVRAM_PLX_PCI_VENDOR_ID
//
// Description:
//      PLX Vendor ID
//
// Revision History:
//      11/7/00   MWagner    Created.
//
//--
#define K10_NVRAM_PLX_PCI_VENDOR_ID                0x1089
// .End K10_NVRAM_PLX_PCI_VENDOR_ID

//--
//++
// Macro:
//      K10_NVRAM_PLX_DEVICE_DATA_ALIGN
//
// Description:
//      PLX Device reads need to be aligned on 8 byte
//      boundaries.
//
// Revision History:
//      11/7/00   MWagner    Created.
//
//--
#define K10_NVRAM_PLX_DEVICE_DATA_ALIGN          0x0008
// .End K10_NVRAM_PLX_PCI_DEVICE_ID


//--
//++
// Macro:
//      K10_NVRAM_PLX_PCI_DEVICE_ID
//
// Description:
//      PLX Device ID
//
// Revision History:
//      11/7/00   MWagner    Created.
//
//--
#define K10_NVRAM_PLX_PCI_DEVICE_ID                 0x0009
// .End K10_NVRAM_PLX_PCI_DEVICE_ID

//++
// Macro:
//      K10_NVRAM_PLX_PCI_BASE_ADDRESS_REGISTER
//
// Description:
//      The Base Address Register in the PCI Configuration
//      Space that contains the NVRAM Base Address.
//
// Revision History:
//      11/7/00   MWagner    Created.
//
//--
#define K10_NVRAM_PLX_PCI_BASE_ADDRESS_REGISTER     0x2
// .End K10_NVRAM_PLX_PCI_BASE_ADDRESS_REGISTER

//++
// Macro:
//      RESERVED OFFSETS
//
// Description:
//      These are reserved offsets in the Non-Volatile RAM.
//      The Non Volatile RAM fills up like graduate level
//      Comp Sci classes- from the back.
//
//      These offsets are described in the NvRam.doc document.
//
//
// Revision History:
//      11/7/00   MWagner    Created.
//      06/20/07  JAsh       Added NVRAM_HARDWARE_SECTION.
//
//--

/*
 * Define ID's for various sections of NvRam
 */
typedef enum _nvram_sections
{
    NVRAM_REBOOT_SECTION = 0,
    NVRAM_REBOOT_LOG_SECTION,
    NVRAM_FLARE_SECTION,
    NVRAM_BOOT_DIRECTIVE_SECTION,
    NVRAM_PANIC_LOG_SECTION,
    NVRAM_CMM_SECTION,
    NVRAM_MINIPORT_SECTION,
    NVRAM_MEMORY_PERSIST_HEADER_SECTION,
    NVRAM_MEMORY_PERSIST_APPLICATION_SECTION,
    NVRAM_C4_INTEGRATED_APPLICATION_OS_AREA,
    NVRAM_CNA_SECTION,
    NVRAM_KMS_CONFIG_SECTION,
    NVRAM_NUMBER_OF_SECTIONS    // this must always be last
}
NVRAM_SECTIONS;

/*
 * Structure of information for a NvRam Section
 */
typedef struct nvram_section_record
{
    char            *name;
    unsigned long   startOffset;
    unsigned long   size;
} NVRAM_SECTION_RECORD, *PNVRAM_SECTION_RECORD;


/* This part is owned by GPS, and any new regions or changes
 * in region size needs to be cleared with them.
 */

#define K10_NVRAM_SP_SERIAL_NUMBER_SIZE             0x10

// These are the Transformers definitions.  Note that these offsets are
// not global to the NVRAM address space, but relative to the OS region
// of NVRAM.

// Note that the UEFI driver has dependencies on the reboot section being at the
// top of the OS region.
#define K10_NVRAM_SIO_REBOOT_DATA_TRANSFORMERS                              0x00000
#define K10_NVRAM_SIO_REBOOT_DATA_SIZE_TRANSFORMERS                         0x00130

#define K10_NVRAM_SIO_REBOOT_LOG_DATA_TRANSFORMERS                          0x00130
#define K10_NVRAM_SIO_REBOOT_LOG_DATA_SIZE_TRANSFORMERS                     0x000D0

#define K10_NVRAM_SIO_MINIPORT_DATA_TRANSFORMERS                            0x00200
#define K10_NVRAM_SIO_MINIPORT_DATA_SIZE_TRANSFORMERS                       0x00D00

#define K10_NVRAM_SIO_PANIC_LOG_DATA_TRANSFORMERS                           0x00F00
#define K10_NVRAM_SIO_PANIC_LOG_DATA_SIZE_TRANSFORMERS                      0x01000

#define K10_NVRAM_SIO_FLARE_DATA_TRANSFORMERS                               0x01F00
#define K10_NVRAM_SIO_FLARE_DATA_SIZE_TRANSFORMERS                          0x0F100

#define K10_NVRAM_SIO_MEMORY_PERSISTENCE_APPLICATION_DATA_TRANSFORMERS      0x11000
#define K10_NVRAM_SIO_MEMORY_PERSISTENCE_APPLICATION_DATA_SIZE_TRANSFORMERS 0x00800    // C4_INTEGRATED - C4ARCH - memory_config

#define K10_NVRAM_SIO_C4_DATA_TRANSFORMERS                                  0x15000
#define K10_NVRAM_SIO_C4_DATA_SIZE_TRANSFORMERS                             0x04000

#define K10_NVRAM_SIO_BOOT_DIRECTIVE_DATA_TRANSFORMERS                      0x19600
#define K10_NVRAM_SIO_BOOT_DIRECTIVE_DATA_SIZE_TRANSFORMERS                 0x00400

#define K10_NVRAM_SIO_KMS_CONFIG_AREA                                       0x19A00
#define K10_NVRAM_SIO_KMS_CONFIG_AREA_SIZE                                  0x02000

// For Transformers, the CMM section is in the 'Common' region, not
// the OS region.  So these offsets are relative to the 'Common' region.    
#define K10_NVRAM_SIO_CMM_DATA_TRANSFORMERS                                 0x00060
#define K10_NVRAM_SIO_CMM_DATA_SIZE_TRANSFORMERS                            0x00668

// For Transformers, the MP header section is in the 'Common' region, not
// the OS region.  So these offsets are relative to the 'Common' region.
#define K10_NVRAM_SIO_MEMORY_PERSISTENCE_HEADER_TRANSFORMERS                0x00C00
#define K10_NVRAM_SIO_MEMORY_PERSISTENCE_HEADER_SIZE_TRANSFORMERS           0x00020

// For Moons, the CNA Personality section is in the 'Common' region.
// Offset is relative to the 'Common' region.
#define K10_NVRAM_CNA_PERSONALITY                                           0x00C30
#define K10_NVRAM_CNA_PERSONALITY_SIZE                                      0x00001

// .End RESERVED OFFSETS

#endif //  _K10_PLX_H_

// End K10Plx.h
