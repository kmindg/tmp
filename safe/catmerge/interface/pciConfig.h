#ifndef PCICONFIG_H
#define PCICONFIG_H

/***************************************************************************
 *  Copyright (C)  EMC Corporation 1992-2009
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. 
 ***************************************************************************/

/***************************************************************************
 * pciConfig.h
 ***************************************************************************
 *
 * File Description:
 *  Header file which copies the pci config information from ntddk.h
 *
 * Author:
 *  Joseph Ash
 *  
 * Revision History:
 *  November 16, 2009 - JSA - Created Inital Version
 *
 ***************************************************************************/

/* Check if PCI_TYPE0_ADDRESSES is defined, if it is, then we can assume that
 * everything else is defined as well
 */
#ifndef PCI_TYPE0_ADDRESSES

typedef struct _PCI_SLOT_NUMBER {
    union {
        struct {
            ULONG   DeviceNumber:5;
            ULONG   FunctionNumber:3;
            ULONG   Reserved:24;
        } bits;
        ULONG   AsULONG;
    } u;
} PCI_SLOT_NUMBER, *PPCI_SLOT_NUMBER;


#define PCI_TYPE0_ADDRESSES             6
#define PCI_TYPE1_ADDRESSES             2
#define PCI_TYPE2_ADDRESSES             5

typedef struct _PCI_COMMON_CONFIG {
    USHORT  VendorID;                   // (ro)
    USHORT  DeviceID;                   // (ro)
    USHORT  Command;                    // Device control
    USHORT  Status;
    UCHAR   RevisionID;                 // (ro)
    UCHAR   ProgIf;                     // (ro)
    UCHAR   SubClass;                   // (ro)
    UCHAR   BaseClass;                  // (ro)
    UCHAR   CacheLineSize;              // (ro+)
    UCHAR   LatencyTimer;               // (ro+)
    UCHAR   HeaderType;                 // (ro)
    UCHAR   BIST;                       // Built in self test

    union {
        struct _PCI_HEADER_TYPE_0 {
            ULONG   BaseAddresses[PCI_TYPE0_ADDRESSES];
            ULONG   CIS;
            USHORT  SubVendorID;
            USHORT  SubSystemID;
            ULONG   ROMBaseAddress;
            UCHAR   CapabilitiesPtr;
            UCHAR   Reserved1[3];
            ULONG   Reserved2;
            UCHAR   InterruptLine;      //
            UCHAR   InterruptPin;       // (ro)
            UCHAR   MinimumGrant;       // (ro)
            UCHAR   MaximumLatency;     // (ro)
        } type0;

// end_wdm end_ntminiport end_ntndis

        //
        // PCI to PCI Bridge
        //

        struct _PCI_HEADER_TYPE_1 {
            ULONG   BaseAddresses[PCI_TYPE1_ADDRESSES];
            UCHAR   PrimaryBus;
            UCHAR   SecondaryBus;
            UCHAR   SubordinateBus;
            UCHAR   SecondaryLatency;
            UCHAR   IOBase;
            UCHAR   IOLimit;
            USHORT  SecondaryStatus;
            USHORT  MemoryBase;
            USHORT  MemoryLimit;
            USHORT  PrefetchBase;
            USHORT  PrefetchLimit;
            ULONG   PrefetchBaseUpper32;
            ULONG   PrefetchLimitUpper32;
            USHORT  IOBaseUpper16;
            USHORT  IOLimitUpper16;
            UCHAR   CapabilitiesPtr;
            UCHAR   Reserved1[3];
            ULONG   ROMBaseAddress;
            UCHAR   InterruptLine;
            UCHAR   InterruptPin;
            USHORT  BridgeControl;
        } type1;

        //
        // PCI to CARDBUS Bridge
        //

        struct _PCI_HEADER_TYPE_2 {
            ULONG   SocketRegistersBaseAddress;
            UCHAR   CapabilitiesPtr;
            UCHAR   Reserved;
            USHORT  SecondaryStatus;
            UCHAR   PrimaryBus;
            UCHAR   SecondaryBus;
            UCHAR   SubordinateBus;
            UCHAR   SecondaryLatency;
            struct  {
                ULONG   Base;
                ULONG   Limit;
            }       Range[PCI_TYPE2_ADDRESSES-1];
            UCHAR   InterruptLine;
            UCHAR   InterruptPin;
            USHORT  BridgeControl;
        } type2;

// begin_wdm begin_ntminiport begin_ntndis

    } u;

    UCHAR   DeviceSpecific[192];

} PCI_COMMON_CONFIG, *PPCI_COMMON_CONFIG;


#define PCI_COMMON_HDR_LENGTH (CSX_OFFSET_OF (PCI_COMMON_CONFIG, DeviceSpecific))

#define PCI_MAX_DEVICES                     32
#define PCI_MAX_FUNCTION                    8
#define PCI_MAX_BRIDGE_NUMBER               0xFF

#define PCI_INVALID_VENDORID                0xFFFF

//
// Bit encodings for  PCI_COMMON_CONFIG.HeaderType
//

#define PCI_MULTIFUNCTION                   0x80
#define PCI_DEVICE_TYPE                     0x00
#define PCI_BRIDGE_TYPE                     0x01
#define PCI_CARDBUS_BRIDGE_TYPE             0x02

#define PCI_CONFIGURATION_TYPE(PciData) \
    (((PPCI_COMMON_CONFIG)(PciData))->HeaderType & ~PCI_MULTIFUNCTION)

#define PCI_MULTIFUNCTION_DEVICE(PciData) \
    ((((PPCI_COMMON_CONFIG)(PciData))->HeaderType & PCI_MULTIFUNCTION) != 0)

//
// Bit encodings for PCI_COMMON_CONFIG.Command
//

#define PCI_ENABLE_IO_SPACE                 0x0001
#define PCI_ENABLE_MEMORY_SPACE             0x0002
#define PCI_ENABLE_BUS_MASTER               0x0004
#define PCI_ENABLE_SPECIAL_CYCLES           0x0008
#define PCI_ENABLE_WRITE_AND_INVALIDATE     0x0010
#define PCI_ENABLE_VGA_COMPATIBLE_PALETTE   0x0020
#define PCI_ENABLE_PARITY                   0x0040  // (ro+)
#define PCI_ENABLE_WAIT_CYCLE               0x0080  // (ro+)
#define PCI_ENABLE_SERR                     0x0100  // (ro+)
#define PCI_ENABLE_FAST_BACK_TO_BACK        0x0200  // (ro)

//
// Bit encodings for PCI_COMMON_CONFIG.Status
//

#define PCI_STATUS_CAPABILITIES_LIST        0x0010  // (ro)
#define PCI_STATUS_66MHZ_CAPABLE            0x0020  // (ro)
#define PCI_STATUS_UDF_SUPPORTED            0x0040  // (ro)
#define PCI_STATUS_FAST_BACK_TO_BACK        0x0080  // (ro)
#define PCI_STATUS_DATA_PARITY_DETECTED     0x0100
#define PCI_STATUS_DEVSEL                   0x0600  // 2 bits wide
#define PCI_STATUS_SIGNALED_TARGET_ABORT    0x0800
#define PCI_STATUS_RECEIVED_TARGET_ABORT    0x1000
#define PCI_STATUS_RECEIVED_MASTER_ABORT    0x2000
#define PCI_STATUS_SIGNALED_SYSTEM_ERROR    0x4000
#define PCI_STATUS_DETECTED_PARITY_ERROR    0x8000

//
// The NT PCI Driver uses a WhichSpace parameter on its CONFIG_READ/WRITE
// routines.   The following values are defined-
//

#define PCI_WHICHSPACE_CONFIG               0x0
#define PCI_WHICHSPACE_ROM                  0x52696350

// end_wdm
//
// PCI Capability IDs
//

#define PCI_CAPABILITY_ID_POWER_MANAGEMENT  0x01
#define PCI_CAPABILITY_ID_AGP               0x02
#define PCI_CAPABILITY_ID_MSI               0x05
#define PCI_CAPABILITY_ID_AGP_TARGET        0x0E

//
// All PCI Capability structures have the following header.
//
// CapabilityID is used to identify the type of the structure (is
// one of the PCI_CAPABILITY_ID values above.
//
// Next is the offset in PCI Configuration space (0x40 - 0xfc) of the
// next capability structure in the list, or 0x00 if there are no more
// entries.
//
typedef struct _PCI_CAPABILITIES_HEADER {
    UCHAR   CapabilityID;
    UCHAR   Next;
} PCI_CAPABILITIES_HEADER, *PPCI_CAPABILITIES_HEADER;

//
// Power Management Capability
//

typedef struct _PCI_PMC {
    UCHAR       Version:3;
    UCHAR       PMEClock:1;
    UCHAR       Rsvd1:1;
    UCHAR       DeviceSpecificInitialization:1;
    UCHAR       Rsvd2:2;
    struct _PM_SUPPORT {
        UCHAR   Rsvd2:1;
        UCHAR   D1:1;
        UCHAR   D2:1;
        UCHAR   PMED0:1;
        UCHAR   PMED1:1;
        UCHAR   PMED2:1;
        UCHAR   PMED3Hot:1;
        UCHAR   PMED3Cold:1;
    } Support;
} PCI_PMC, *PPCI_PMC;

typedef struct _PCI_PMCSR {
    USHORT      PowerState:2;
    USHORT      Rsvd1:6;
    USHORT      PMEEnable:1;
    USHORT      DataSelect:4;
    USHORT      DataScale:2;
    USHORT      PMEStatus:1;
} PCI_PMCSR, *PPCI_PMCSR;


typedef struct _PCI_PMCSR_BSE {
    UCHAR       Rsvd1:6;
    UCHAR       D3HotSupportsStopClock:1;       // B2_B3#
    UCHAR       BusPowerClockControlEnabled:1;  // BPCC_EN
} PCI_PMCSR_BSE, *PPCI_PMCSR_BSE;


typedef struct _PCI_PM_CAPABILITY {

    PCI_CAPABILITIES_HEADER Header;

    //
    // Power Management Capabilities (Offset = 2)
    //

    union {
        PCI_PMC         Capabilities;
        USHORT          AsUSHORT;
    } PMC;

    //
    // Power Management Control/Status (Offset = 4)
    //

    union {
        PCI_PMCSR       ControlStatus;
        USHORT          AsUSHORT;
    } PMCSR;

    //
    // PMCSR PCI-PCI Bridge Support Extensions
    //

    union {
        PCI_PMCSR_BSE   BridgeSupport;
        UCHAR           AsUCHAR;
    } PMCSR_BSE;

    //
    // Optional read only 8 bit Data register.  Contents controlled by
    // DataSelect and DataScale in ControlStatus.
    //

    UCHAR   Data;

} PCI_PM_CAPABILITY, *PPCI_PM_CAPABILITY;

//
// AGP Capability
//
typedef struct _PCI_AGP_CAPABILITY {
    
    PCI_CAPABILITIES_HEADER Header;

    USHORT  Minor:4;
    USHORT  Major:4;
    USHORT  Rsvd1:8;

    struct _PCI_AGP_STATUS {
        ULONG   Rate:3;
        ULONG   Agp3Mode:1;
        ULONG   FastWrite:1;
        ULONG   FourGB:1;
        ULONG   HostTransDisable:1;
        ULONG   Gart64:1;
        ULONG   ITA_Coherent:1;
        ULONG   SideBandAddressing:1;                   // SBA
        ULONG   CalibrationCycle:3;
        ULONG   AsyncRequestSize:3;
        ULONG   Rsvd1:1;
        ULONG   Isoch:1;
        ULONG   Rsvd2:6;
        ULONG   RequestQueueDepthMaximum:8;             // RQ
    } AGPStatus;

    struct _PCI_AGP_COMMAND {
        ULONG   Rate:3;
        ULONG   Rsvd1:1;
        ULONG   FastWriteEnable:1;
        ULONG   FourGBEnable:1;
        ULONG   Rsvd2:1;
        ULONG   Gart64:1;
        ULONG   AGPEnable:1;
        ULONG   SBAEnable:1;
        ULONG   CalibrationCycle:3;
        ULONG   AsyncReqSize:3;
        ULONG   Rsvd3:8;
        ULONG   RequestQueueDepth:8;
    } AGPCommand;

} PCI_AGP_CAPABILITY, *PPCI_AGP_CAPABILITY;

//
// An AGPv3 Target must have an extended capability,
// but it's only present for a Master when the Isoch
// bit is set in its status register
//
typedef enum _EXTENDED_AGP_REGISTER {
    IsochStatus,
    AgpControl,
    ApertureSize,
    AperturePageSize,
    GartLow,
    GartHigh,
    IsochCommand
} EXTENDED_AGP_REGISTER, *PEXTENDED_AGP_REGISTER;

typedef struct _PCI_AGP_ISOCH_STATUS {
    ULONG ErrorCode: 2;
    ULONG Rsvd1: 1;
    ULONG Isoch_L: 3;
    ULONG Isoch_Y: 2;
    ULONG Isoch_N: 8;
    ULONG Rsvd2: 16;
} PCI_AGP_ISOCH_STATUS, *PPCI_AGP_ISOCH_STATUS;

typedef struct _PCI_AGP_CONTROL {
    ULONG Rsvd1: 7;
    ULONG GTLB_Enable: 1;
    ULONG AP_Enable: 1;
    ULONG CAL_Disable: 1;
    ULONG Rsvd2: 22;
} PCI_AGP_CONTROL, *PPCI_AGP_CONTROL;

typedef struct _PCI_AGP_APERTURE_PAGE_SIZE {
    USHORT PageSizeMask: 11;
    USHORT Rsvd1: 1;
    USHORT PageSizeSelect: 4;
} PCI_AGP_APERTURE_PAGE_SIZE, *PPCI_AGP_APERTURE_PAGE_SIZE;

typedef struct _PCI_AGP_ISOCH_COMMAND {
    USHORT Rsvd1: 6;
    USHORT Isoch_Y: 2;
    USHORT Isoch_N: 8;
} PCI_AGP_ISOCH_COMMAND, *PPCI_AGP_ISOCH_COMMAND;

typedef struct PCI_AGP_EXTENDED_CAPABILITY {

    PCI_AGP_ISOCH_STATUS IsochStatus;

//
// Target only ----------------<<-begin->>
//
    PCI_AGP_CONTROL AgpControl;
    USHORT ApertureSize;
    PCI_AGP_APERTURE_PAGE_SIZE AperturePageSize;
    ULONG GartLow;
    ULONG GartHigh;
//
// ------------------------------<<-end->>
//

    PCI_AGP_ISOCH_COMMAND IsochCommand;

} PCI_AGP_EXTENDED_CAPABILITY, *PPCI_AGP_EXTENDED_CAPABILITY;


#define PCI_AGP_RATE_1X     0x1
#define PCI_AGP_RATE_2X     0x2
#define PCI_AGP_RATE_4X     0x4

//
// MSI (Message Signalled Interrupts) Capability
//

typedef struct _PCI_MSI_CAPABILITY {

      PCI_CAPABILITIES_HEADER Header;

      struct _PCI_MSI_MESSAGE_CONTROL {
         USHORT  MSIEnable:1;
         USHORT  MultipleMessageCapable:3;
         USHORT  MultipleMessageEnable:3;
         USHORT  CapableOf64Bits:1;
         USHORT  Reserved:8;
      } MessageControl;

      union {
            struct _PCI_MSI_MESSAGE_ADDRESS {
               ULONG_PTR Reserved:2;              // always zero, DWORD aligned address
               ULONG_PTR Address:30;
            } Register;
            ULONG_PTR Raw;
      } MessageAddress;

      //
      // The rest of the Capability structure differs depending on whether
      // 32bit or 64bit addressing is being used.
      //
      // (The CapableOf64Bits bit above determines this)
      //

      union {

         // For 64 bit devices

         struct _PCI_MSI_64BIT_DATA {
            ULONG MessageUpperAddress;
            USHORT MessageData;
         } Bit64;

         // For 32 bit devices

         struct _PCI_MSI_32BIT_DATA {
            USHORT MessageData;
            ULONG Unused;
         } Bit32;
      } Data;

} PCI_MSI_CAPABILITY, *PPCI_PCI_CAPABILITY;

// begin_wdm
//
// Base Class Code encodings for Base Class (from PCI spec rev 2.1).
//

#define PCI_CLASS_PRE_20                    0x00
#define PCI_CLASS_MASS_STORAGE_CTLR         0x01
#define PCI_CLASS_NETWORK_CTLR              0x02
#define PCI_CLASS_DISPLAY_CTLR              0x03
#define PCI_CLASS_MULTIMEDIA_DEV            0x04
#define PCI_CLASS_MEMORY_CTLR               0x05
#define PCI_CLASS_BRIDGE_DEV                0x06
#define PCI_CLASS_SIMPLE_COMMS_CTLR         0x07
#define PCI_CLASS_BASE_SYSTEM_DEV           0x08
#define PCI_CLASS_INPUT_DEV                 0x09
#define PCI_CLASS_DOCKING_STATION           0x0a
#define PCI_CLASS_PROCESSOR                 0x0b
#define PCI_CLASS_SERIAL_BUS_CTLR           0x0c
#define PCI_CLASS_WIRELESS_CTLR             0x0d
#define PCI_CLASS_INTELLIGENT_IO_CTLR       0x0e
#define PCI_CLASS_SATELLITE_COMMS_CTLR      0x0f
#define PCI_CLASS_ENCRYPTION_DECRYPTION     0x10
#define PCI_CLASS_DATA_ACQ_SIGNAL_PROC      0x11

// 0d thru fe reserved

#ifdef PCI_CLASS_NOT_DEFINED
#undef PCI_CLASS_NOT_DEFINED
#endif
#define PCI_CLASS_NOT_DEFINED               0xff

//
// Sub Class Code encodings (PCI rev 2.1).
//

// Class 00 - PCI_CLASS_PRE_20

#define PCI_SUBCLASS_PRE_20_NON_VGA         0x00
#define PCI_SUBCLASS_PRE_20_VGA             0x01

// Class 01 - PCI_CLASS_MASS_STORAGE_CTLR

#define PCI_SUBCLASS_MSC_SCSI_BUS_CTLR      0x00
#define PCI_SUBCLASS_MSC_IDE_CTLR           0x01
#define PCI_SUBCLASS_MSC_FLOPPY_CTLR        0x02
#define PCI_SUBCLASS_MSC_IPI_CTLR           0x03
#define PCI_SUBCLASS_MSC_RAID_CTLR          0x04
#define PCI_SUBCLASS_MSC_ATA_CTLR           0x05
#define PCI_SUBCLASS_MSC_SATA_CTLR          0x06
#define PCI_SUBCLASS_MSC_SAS_CTLR           0x07
#define PCI_SUBCLASS_MSC_OTHER              0x80

// Class 02 - PCI_CLASS_NETWORK_CTLR

#define PCI_SUBCLASS_NET_ETHERNET_CTLR      0x00
#define PCI_SUBCLASS_NET_TOKEN_RING_CTLR    0x01
#define PCI_SUBCLASS_NET_FDDI_CTLR          0x02
#define PCI_SUBCLASS_NET_ATM_CTLR           0x03
#define PCI_SUBCLASS_NET_ISDN_CTLR          0x04
#define PCI_SUBCLASS_NET_OTHER              0x80

// Class 03 - PCI_CLASS_DISPLAY_CTLR

// N.B. Sub Class 00 could be VGA or 8514 depending on Interface byte

#define PCI_SUBCLASS_VID_VGA_CTLR           0x00
#define PCI_SUBCLASS_VID_XGA_CTLR           0x01
#define PCI_SUBLCASS_VID_3D_CTLR            0x02
#define PCI_SUBCLASS_VID_OTHER              0x80

// Class 04 - PCI_CLASS_MULTIMEDIA_DEV

#define PCI_SUBCLASS_MM_VIDEO_DEV           0x00
#define PCI_SUBCLASS_MM_AUDIO_DEV           0x01
#define PCI_SUBCLASS_MM_TELEPHONY_DEV       0x02
#define PCI_SUBCLASS_MM_OTHER               0x80

// Class 05 - PCI_CLASS_MEMORY_CTLR

#define PCI_SUBCLASS_MEM_RAM                0x00
#define PCI_SUBCLASS_MEM_FLASH              0x01
#define PCI_SUBCLASS_MEM_OTHER              0x80

// Class 06 - PCI_CLASS_BRIDGE_DEV

#define PCI_SUBCLASS_BR_HOST                0x00
#define PCI_SUBCLASS_BR_ISA                 0x01
#define PCI_SUBCLASS_BR_EISA                0x02
#define PCI_SUBCLASS_BR_MCA                 0x03
#define PCI_SUBCLASS_BR_PCI_TO_PCI          0x04
#define PCI_SUBCLASS_BR_PCMCIA              0x05
#define PCI_SUBCLASS_BR_NUBUS               0x06
#define PCI_SUBCLASS_BR_CARDBUS             0x07
#define PCI_SUBCLASS_BR_RACEWAY             0x08
#define PCI_SUBCLASS_BR_OTHER               0x80

// Class 07 - PCI_CLASS_SIMPLE_COMMS_CTLR

// N.B. Sub Class 00 and 01 additional info in Interface byte

#define PCI_SUBCLASS_COM_SERIAL             0x00
#define PCI_SUBCLASS_COM_PARALLEL           0x01
#define PCI_SUBCLASS_COM_MULTIPORT          0x02
#define PCI_SUBCLASS_COM_MODEM              0x03
#define PCI_SUBCLASS_COM_OTHER              0x80

// Class 08 - PCI_CLASS_BASE_SYSTEM_DEV

// N.B. See Interface byte for additional info.

#define PCI_SUBCLASS_SYS_INTERRUPT_CTLR     0x00
#define PCI_SUBCLASS_SYS_DMA_CTLR           0x01
#define PCI_SUBCLASS_SYS_SYSTEM_TIMER       0x02
#define PCI_SUBCLASS_SYS_REAL_TIME_CLOCK    0x03
#define PCI_SUBCLASS_SYS_GEN_HOTPLUG_CTLR   0x04
#define PCI_SUBCLASS_SYS_OTHER              0x80

// Class 09 - PCI_CLASS_INPUT_DEV

#define PCI_SUBCLASS_INP_KEYBOARD           0x00
#define PCI_SUBCLASS_INP_DIGITIZER          0x01
#define PCI_SUBCLASS_INP_MOUSE              0x02
#define PCI_SUBCLASS_INP_SCANNER            0x03
#define PCI_SUBCLASS_INP_GAMEPORT           0x04
#define PCI_SUBCLASS_INP_OTHER              0x80

// Class 0a - PCI_CLASS_DOCKING_STATION

#define PCI_SUBCLASS_DOC_GENERIC            0x00
#define PCI_SUBCLASS_DOC_OTHER              0x80

// Class 0b - PCI_CLASS_PROCESSOR

#define PCI_SUBCLASS_PROC_386               0x00
#define PCI_SUBCLASS_PROC_486               0x01
#define PCI_SUBCLASS_PROC_PENTIUM           0x02
#define PCI_SUBCLASS_PROC_ALPHA             0x10
#define PCI_SUBCLASS_PROC_POWERPC           0x20
#define PCI_SUBCLASS_PROC_COPROCESSOR       0x40

// Class 0c - PCI_CLASS_SERIAL_BUS_CTLR

#define PCI_SUBCLASS_SB_IEEE1394            0x00
#define PCI_SUBCLASS_SB_ACCESS              0x01
#define PCI_SUBCLASS_SB_SSA                 0x02
#define PCI_SUBCLASS_SB_USB                 0x03
#define PCI_SUBCLASS_SB_FIBRE_CHANNEL       0x04
#define PCI_SUBCLASS_SB_SMBUS               0x05

// Class 0d - PCI_CLASS_WIRELESS_CTLR

#define PCI_SUBCLASS_WIRELESS_IRDA          0x00
#define PCI_SUBCLASS_WIRELESS_CON_IR        0x01
#define PCI_SUBCLASS_WIRELESS_RF            0x10
#define PCI_SUBCLASS_WIRELESS_OTHER         0x80

// Class 0e - PCI_CLASS_INTELLIGENT_IO_CTLR

#define PCI_SUBCLASS_INTIO_I2O              0x00

// Class 0f - PCI_CLASS_SATELLITE_CTLR

#define PCI_SUBCLASS_SAT_TV                 0x01
#define PCI_SUBCLASS_SAT_AUDIO              0x02
#define PCI_SUBCLASS_SAT_VOICE              0x03
#define PCI_SUBCLASS_SAT_DATA               0x04

// Class 10 - PCI_CLASS_ENCRYPTION_DECRYPTION

#define PCI_SUBCLASS_CRYPTO_NET_COMP        0x00
#define PCI_SUBCLASS_CRYPTO_ENTERTAINMENT   0x10
#define PCI_SUBCLASS_CRYPTO_OTHER           0x80

// Class 11 - PCI_CLASS_DATA_ACQ_SIGNAL_PROC

#define PCI_SUBCLASS_DASP_DPIO              0x00
#define PCI_SUBCLASS_DASP_OTHER             0x80



// end_ntndis
#endif //PCI_TYPE0_ADDRESSES
#ifndef PCI_ADDRESS_IO_SPACE

//
// Bit encodes for PCI_COMMON_CONFIG.u.type0.BaseAddresses
//

#define PCI_ADDRESS_IO_SPACE                0x00000001  // (ro)
#define PCI_ADDRESS_MEMORY_TYPE_MASK        0x00000006  // (ro)
#define PCI_ADDRESS_MEMORY_PREFETCHABLE     0x00000008  // (ro)

#define PCI_ADDRESS_IO_ADDRESS_MASK         0xfffffffc
#define PCI_ADDRESS_MEMORY_ADDRESS_MASK     0xfffffff0
#define PCI_ADDRESS_ROM_ADDRESS_MASK        0xfffff800

#define PCI_TYPE_32BIT      0
#define PCI_TYPE_20BIT      2
#define PCI_TYPE_64BIT      4

//
// Bit encodes for PCI_COMMON_CONFIG.u.type0.ROMBaseAddresses
//

#define PCI_ROMADDRESS_ENABLED              0x00000001


//
// Reference notes for PCI configuration fields:
//
// ro   these field are read only.  changes to these fields are ignored
//
// ro+  these field are intended to be read only and should be initialized
//      by the system to their proper values.  However, driver may change
//      these settings.
//
// ---
//
//      All resources comsumed by a PCI device start as unitialized
//      under NT.  An uninitialized memory or I/O base address can be
//      determined by checking it's corrisponding enabled bit in the
//      PCI_COMMON_CONFIG.Command value.  An InterruptLine is unitialized
//      if it contains the value of -1.
//

//
// Another set of PCI related infrastructure (used by CMI)
//
typedef ULONG(*PGET_SET_DEVICE_DATA) (IN PVOID Context, IN ULONG DataType, IN PVOID Buffer, IN ULONG Offset, IN ULONG Length);                      
typedef struct {
    PVOID Context;
    ULONG Size;
    PGET_SET_DEVICE_DATA SetBusData;
    PGET_SET_DEVICE_DATA GetBusData;
} BUS_INTERFACE_STANDARD, *PBUS_INTERFACE_STANDARD;

#endif //PCI_ADDRESS_IO_SPACE
#endif //PCICONFIG_H
/***************************************************************************
 * END pciConfig.h
 ***************************************************************************/
