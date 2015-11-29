#ifndef NTMIRROR_USER_INTERFACE_H
#define NTMIRROR_USER_INTERFACE_H

/***************************************************************************
 *  Copyright (C)  EMC Corporation 2003
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. * All rights reserved.
 ***************************************************************************/

/***************************************************************************
 * ntmirror_user_interface.h
 ***************************************************************************
 *
 * Description:
 *
 *  Contains defines that user mode applications need in order to interface
 *  with the NtMirror Driver.  This includes device types and custom IOCTLs,
 *  etc.  (See ntmirror_interface.h for ODBS and MDB related #defines which
 *  are mostly shared between kernel mode drivers.)
 *
 * History:
 *  
 *  29 Jul 03  MWH  Created from ntmirror_interface.h
 *
 ***************************************************************************/

/* Utility partition device name. The following partition names are
 * followed by an ordinal partition number. 
 */
#define NTMIRROR_UTILITY_DEVICE_OBJECT_DIRECTORY "\\Device\\Utility"
#define NTMIRROR_UTILITY_DEVICE_NAME     "\\Device\\Utility\\UtilityPartition%d"
#define NTMIRROR_UTILITY_DOS_DEVICE_NAME "\\??\\UtilityPartition%d"

/* A custom device type for the Utility partition type. Note that devioctl.h
 * in the DDK states that the range 0-32767 (0-0x7FFF) is reserved for use
 * by Microsoft and the range 32768-65535 (0x8000-0xFFFF) are reserved for
 * use by customers.
 *
 * NOTE: FILE_DEVICE_ASIDC is defined as 0x8000 in asidc_interface.h
 * ICA will access the Utility partition device via this device type.
 */
#define FILE_DEVICE_UTILITY  0x00008001

/* Special device type for HiddenFdo's created by NtMirror.
 */
#define FILE_DEVICE_HIDDEN_MIRROR 0x00008002

/* IOCTL Codes for NtMirror. Custom function codes (2nd argument) may
 * be in the range 2048-4095 (0x800-0xFFF). The METHOD_BUFFERED argument indicates
 * that Buffered IO will be the mechanism for transfering data to and from the
 * caller of these IOCTLs. Application level code will maintain seperate buffers
 * for input and output data. Input data is copied to a system buffer for use
 * by the NtMirror driver. This system buffer is overwritten with output data.
 * Output data is copied to the application level's output buffer.
 *
 * We borrow the FILE_DEVICE_UTILITY device type so ICA can distinguish between
 * these IOCTLs and those that should be directed at ASIDC (FILE_DEVICE_ASIDC).
 */
#define IOCTL_NTMIRROR_BASE             FILE_DEVICE_UTILITY
#define IOCTL_NTMIRROR_VERSION          EMCPAL_IOCTL_CTL_CODE(IOCTL_NTMIRROR_BASE, 0x800, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)
#define IOCTL_NTMIRROR_REBUILD_STATUS   EMCPAL_IOCTL_CTL_CODE(IOCTL_NTMIRROR_BASE, 0x801, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)
#define IOCTL_NTMIRROR_UTILITY_IO       EMCPAL_IOCTL_CTL_CODE(IOCTL_NTMIRROR_BASE, 0x802, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

/* Opcodes for NTMIRROR_VERSION_DATA
 */
#define NTMIRROR_VERSION_READ   (0)
#define NTMIRROR_VERSION_WRITE  (1)

#if defined(__cplusplus)
#pragma warning( push )
#pragma warning( disable : 4200 ) // The other option is to change to DataBuffer[1]
#endif
typedef struct _NTMIRROR_VERSION_DATA
{
    UCHAR Operation;        /* 0=read, 1=write */
    ULONG LengthInSectors;  /* only 1 currently supported */
    ULONG OffsetInSectors;  /* only 0 currently supported */
    ULONG Reserved1;
    ULONG Reserved2;
    UCHAR DataBuffer[];     /* data buffer, sized appropriately by caller */
} NTMIRROR_VERSION_DATA;
#if defined(__cplusplus)
#pragma warning( pop ) 
#endif

typedef struct _NTMIRROR_REBUILD_STATUS
{
    /* Reebuild could be required, but may not be in progress at this precise 
     * moment in time because of backend loop events. 
     * If both of these flags are FALSE, caller should ignore the rest of this 
     * structure.
     */
    BOOLEAN RebuildRequired;
    BOOLEAN RebuildInProgress;

    /* For now, these disks always reside in the first enclosure on backend 
     * zero.  (On AX platforms we would have to decode the true physical disk 
     * number from the loop id (because the aliasing is SP-specific).)
     * Since the purpose of this IOCTL is to provide upper-level visibility
     * to our rebuilds, we do not want to expose our aliases to them.
     */
    ULONG SourcePhysicalDisk;
    ULONG DestinationPhysicalDisk;

    /* The current rebuild checkpoint and percentage rebuilt so far.
     * The percentage rebuilt normally applies only if a rebuild is
     * currently in progress.
     */
    ULONG Checkpoint;
    UCHAR PercentageRebuilt;

    /* For future expansion.
     */
    ULONG Reserved1;
    ULONG Reserved2;
    ULONG Reserved3;
    ULONG Reserved4;

} NTMIRROR_REBUILD_STATUS;

typedef struct _NTMIRROR_UTILITY_IO_PARAMETERS
{
    /* Setting this to TRUE essentially "opens" the Utility Partition for 
     * full read/write access. If set to FALSE, NtMirror will fail any attempt
     * to read the MBR on the partition in order to hide it from the OS.
     */
    BOOLEAN AllowIo;

    /* For future expansion.
     */
    ULONG Reserved1;
    ULONG Reserved2;
    ULONG Reserved3;
    ULONG Reserved4;

} NTMIRROR_UTILITY_IO_PARAMETERS;

#endif /* NTMIRROR_USER_INTERFACE_H */
