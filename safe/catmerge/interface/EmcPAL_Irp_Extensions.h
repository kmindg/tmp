#ifndef _EMCPAL_IRP_EXTENSIONS_H_
#define _EMCPAL_IRP_EXTENSIONS_H_
/*!
 * @file EmcPAL_Irp_Extensions.h
 * @addtogroup emcpal_irps
 * @{
 */

//PZPZ
#define EMCPAL_MY_MODULE_CONTEXT() CSX_MY_MODULE_CONTEXT()

//=======================
// EMCPAL IRP Structures

#ifdef EMCPAL_USE_CSX_DCALL
#define EMCPAL_IRP_KPROCESSOR_MODE csx_nuint_t
#define EMCPAL_IRP_LOCK_OPERATION csx_nuint_t
#define EMCPAL_IRP_MEMORY_CACHING_TYPE csx_nuint_t
#define EMCPAL_IRP_MM_PAGE_PRIORITY csx_nuint_t
#define EmcpalUserMode 1
#define EmcpalKernelMode 0
#define EmcpalIoReadAccess 0
#define EmcpalMmCached 1
#define EmcpalMmNonCached 0
#else
#define EMCPAL_IRP_KPROCESSOR_MODE KPROCESSOR_MODE
#define EMCPAL_IRP_LOCK_OPERATION LOCK_OPERATION
#define EMCPAL_IRP_MEMORY_CACHING_TYPE MEMORY_CACHING_TYPE
#define EMCPAL_IRP_MM_PAGE_PRIORITY MM_PAGE_PRIORITY
#define EmcpalUserMode UserMode
#define EmcpalKernelMode KernelMode
#define EmcpalIoReadAccess IoReadAccess
#define EmcpalMmCached MmCached
#define EmcpalMmNonCached MmNonCached
#endif

//=======================
// EMCPAL IRP Structures

#ifdef EMCPAL_USE_CSX_DCALL
#define EMCPAL_IRP_KPROCESSOR_MODE csx_nuint_t
#define EMCPAL_IRP_MINOR_TYPE_QUERY_INTERFACE     0x71
#define EMCPAL_IRP_MINOR_TYPE_QUERY_RESOURCES     0x72
#define EMCPAL_IRP_MINOR_TYPE_QUERY_REMOVE_DEVICE 0x73
#define EMCPAL_IRP_MINOR_TYPE_CANCEL_REMOVE_DEVICE 0x75
#define EMCPAL_IRP_MINOR_TYPE_REMOVE_DEVICE       0x76
#define EMCPAL_IRP_MINOR_TYPE_START_DEVICE        0x74
#define EMCPAL_IRP_DEVICE_CHARS_REMOVABLE_MEDIA   0x8
#define EMCPAL_IRP_DEVICE_CHARS_SECURE_OPEN       0x4
#define EMCPAL_IRP_DEVICE_ALIGNMENT_WORD          4
typedef int EMCPAL_IRP_DEVICE_TYPE;
#else                                      /* EMCPAL_USE_CSX_DCALL */
#define EMCPAL_IRP_KPROCESSOR_MODE KPROCESSOR_MODE
#define EMCPAL_IRP_MINOR_TYPE_QUERY_INTERFACE     IRP_MN_QUERY_INTERFACE
#define EMCPAL_IRP_MINOR_TYPE_QUERY_RESOURCES     IRP_MN_QUERY_RESOURCES
#define EMCPAL_IRP_MINOR_TYPE_QUERY_REMOVE_DEVICE IRP_MN_QUERY_REMOVE_DEVICE
#define EMCPAL_IRP_MINOR_TYPE_CANCEL_REMOVE_DEVICE IRP_MN_CANCEL_REMOVE_DEVICE
#define EMCPAL_IRP_MINOR_TYPE_REMOVE_DEVICE       IRP_MN_REMOVE_DEVICE
#define EMCPAL_IRP_MINOR_TYPE_START_DEVICE        IRP_MN_START_DEVICE
typedef DEVICE_TYPE EMCPAL_IRP_DEVICE_TYPE;
#define EMCPAL_IRP_DEVICE_CHARS_REMOVABLE_MEDIA   FILE_REMOVABLE_MEDIA
#define EMCPAL_IRP_DEVICE_CHARS_SECURE_OPEN       FILE_DEVICE_SECURE_OPEN
#define EMCPAL_IRP_DEVICE_ALIGNMENT_WORD          FILE_WORD_ALIGNMENT
#endif                                     /* EMCPAL_USE_CSX_DCALL */

//=======================
// EMCPAL IRP Stack Location Manipulation

#ifdef EMCPAL_USE_CSX_DCALL
#define EMCPAL_IRP_CURRENT_LOCATION_SANE(_Irp) \
    ((_Irp)->dcall_level_idx_current < EmcpalExtIrpStackCountGet((_Irp)))
    //((_Irp)->dcall_level_idx_current != 0 && (_Irp)->dcall_level_idx_current <= EmcpalExtIrpStackCountGet((_Irp))) //PZPZ revisit
#else
#define EMCPAL_IRP_CURRENT_LOCATION_SANE(_Irp) \
    ((_Irp)->CurrentLocation != 0 && (_Irp)->CurrentLocation <= EmcpalExtIrpStackCountGet((_Irp)))
#endif

CSX_STATIC_INLINE PVOID
EmcpalExtIrpStackParamArg1Get(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return (PVOID) IrpStackLocation->level_parameters.user_defined.user_defined_params[0];
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IrpStackLocation->Parameters.Others.Argument1;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PVOID
EmcpalExtIrpStackParamArg2Get(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return (PVOID) IrpStackLocation->level_parameters.user_defined.user_defined_params[1];
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IrpStackLocation->Parameters.Others.Argument2;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PVOID
EmcpalExtIrpStackParamArg3Get(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return (PVOID) IrpStackLocation->level_parameters.user_defined.user_defined_params[2];
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IrpStackLocation->Parameters.Others.Argument3;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PVOID
EmcpalExtIrpStackParamArg4Get(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return (PVOID) IrpStackLocation->level_parameters.user_defined.user_defined_params[3];
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IrpStackLocation->Parameters.Others.Argument4;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=======================
// EMCPAL IRP Stack Location Manipulation

CSX_STATIC_INLINE PVOID *
EmcpalExtIrpStackParamArg1PtrGet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return (PVOID *) & IrpStackLocation->level_parameters.user_defined.user_defined_params[0];
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return &IrpStackLocation->Parameters.Others.Argument1;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PVOID *
EmcpalExtIrpStackParamArg2PtrGet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return (PVOID *) & IrpStackLocation->level_parameters.user_defined.user_defined_params[1];
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return &IrpStackLocation->Parameters.Others.Argument2;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PVOID *
EmcpalExtIrpStackParamArg3PtrGet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return (PVOID *) & IrpStackLocation->level_parameters.user_defined.user_defined_params[2];
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return &IrpStackLocation->Parameters.Others.Argument3;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PVOID *
EmcpalExtIrpStackParamArg4PtrGet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return (PVOID *) & IrpStackLocation->level_parameters.user_defined.user_defined_params[3];
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return &IrpStackLocation->Parameters.Others.Argument4;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PVOID *
EmcpalExtIrpStackParamArgBlockPtrGet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return (PVOID *) & IrpStackLocation->level_parameters.user_defined.user_defined_params[0];
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return &IrpStackLocation->Parameters.Others.Argument1;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=======================
// EMCPAL IRP Stack Location Manipulation

CSX_STATIC_INLINE VOID
EmcpalExtIrpStackParamArg1Set(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    PVOID Value)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->level_parameters.user_defined.user_defined_params[0] = (csx_ptrhld_t) Value;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->Parameters.Others.Argument1 = Value;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpStackParamArg2Set(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    PVOID Value)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->level_parameters.user_defined.user_defined_params[1] = (csx_ptrhld_t) Value;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->Parameters.Others.Argument2 = Value;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpStackParamArg3Set(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    PVOID Value)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->level_parameters.user_defined.user_defined_params[2] = (csx_ptrhld_t) Value;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->Parameters.Others.Argument3 = Value;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpStackParamArg4Set(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    PVOID Value)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->level_parameters.user_defined.user_defined_params[3] = (csx_ptrhld_t) Value;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->Parameters.Others.Argument4 = Value;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=======================
// EMCPAL IRP Stack Location Manipulation

CSX_STATIC_INLINE VOID
EmcpalExtIrpStackParamReadByteOffsetValSet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    EMCPAL_LARGE_INTEGER Offset)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->level_parameters.read.io_offset = Offset.QuadPart;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->Parameters.Read.ByteOffset.QuadPart = Offset.QuadPart;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpStackParamReadByteOffsetSet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    ULONGLONG Offset)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->level_parameters.read.io_offset = Offset;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->Parameters.Read.ByteOffset.QuadPart = Offset;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpStackParamReadByteOffsetHiSet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    ULONG OffsetHi)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->level_parameters.read.io_offset =
        CSX_LO_32(IrpStackLocation->level_parameters.read.io_offset) | (((ULONGLONG) OffsetHi) << 32);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->Parameters.Read.ByteOffset.QuadPart = CSX_LO_32(IrpStackLocation->Parameters.Read.ByteOffset.QuadPart) | (((ULONGLONG) OffsetHi) << 32);
        /* line auto-joined with above */
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpStackParamReadByteOffsetLoSet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    ULONG OffsetLo)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->level_parameters.read.io_offset = CSX_HI_32(IrpStackLocation->level_parameters.read.io_offset) | OffsetLo;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->Parameters.Read.ByteOffset.QuadPart = CSX_HI_32(IrpStackLocation->Parameters.Read.ByteOffset.QuadPart) | OffsetLo;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=======================
// EMCPAL IRP Stack Location Manipulation

CSX_STATIC_INLINE VOID
EmcpalExtIrpStackParamWriteByteOffsetValSet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    EMCPAL_LARGE_INTEGER Offset)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->level_parameters.write.io_offset = Offset.QuadPart;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->Parameters.Write.ByteOffset.QuadPart = Offset.QuadPart;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpStackParamWriteByteOffsetSet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    ULONGLONG Offset)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->level_parameters.write.io_offset = Offset;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->Parameters.Write.ByteOffset.QuadPart = Offset;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpStackParamWriteByteOffsetHiSet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    ULONG OffsetHi)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->level_parameters.write.io_offset =
        CSX_LO_32(IrpStackLocation->level_parameters.write.io_offset) | (((ULONGLONG) OffsetHi) << 32);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->Parameters.Write.ByteOffset.QuadPart = CSX_LO_32(IrpStackLocation->Parameters.Write.ByteOffset.QuadPart) | (((ULONGLONG) OffsetHi) << 32);
        /* line auto-joined with above */
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpStackParamWriteByteOffsetLoSet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    ULONG OffsetLo)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->level_parameters.write.io_offset = CSX_HI_32(IrpStackLocation->level_parameters.write.io_offset) | OffsetLo;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->Parameters.Write.ByteOffset.QuadPart = CSX_HI_32(IrpStackLocation->Parameters.Write.ByteOffset.QuadPart) | OffsetLo;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=======================
// EMCPAL IRP Stack Location Manipulation

CSX_STATIC_INLINE EMCPAL_LARGE_INTEGER
EmcpalExtIrpStackParamReadByteOffsetValGet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
    EMCPAL_LARGE_INTEGER rv;
#ifdef EMCPAL_USE_CSX_DCALL
    rv.QuadPart = IrpStackLocation->level_parameters.read.io_offset;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    rv = IrpStackLocation->Parameters.Read.ByteOffset;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
    return rv;
}

CSX_STATIC_INLINE ULONG
EmcpalExtIrpStackParamReadByteOffsetHiGet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return CSX_HI_32(IrpStackLocation->level_parameters.read.io_offset);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return CSX_HI_32(IrpStackLocation->Parameters.Read.ByteOffset.QuadPart);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE ULONG
EmcpalExtIrpStackParamReadByteOffsetLoGet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return CSX_LO_32(IrpStackLocation->level_parameters.read.io_offset);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return CSX_LO_32(IrpStackLocation->Parameters.Read.ByteOffset.QuadPart);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=======================
// EMCPAL IRP Stack Location Manipulation

CSX_STATIC_INLINE EMCPAL_LARGE_INTEGER
EmcpalExtIrpStackParamWriteByteOffsetValGet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
    EMCPAL_LARGE_INTEGER rv;
#ifdef EMCPAL_USE_CSX_DCALL
    rv.QuadPart = IrpStackLocation->level_parameters.write.io_offset;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    rv = IrpStackLocation->Parameters.Write.ByteOffset;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
    return rv;
}

CSX_STATIC_INLINE ULONG
EmcpalExtIrpStackParamWriteByteOffsetHiGet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return CSX_HI_32(IrpStackLocation->level_parameters.write.io_offset);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return CSX_HI_32(IrpStackLocation->Parameters.Write.ByteOffset.QuadPart);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE ULONG
EmcpalExtIrpStackParamWriteByteOffsetLoGet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return CSX_LO_32(IrpStackLocation->level_parameters.write.io_offset);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return CSX_LO_32(IrpStackLocation->Parameters.Write.ByteOffset.QuadPart);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=======================
// EMCPAL IRP Stack Location Manipulation

CSX_STATIC_INLINE VOID
EmcpalExtIrpStackParamReadLengthSet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    ULONG Length)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->level_parameters.read.io_size_requested = Length;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->Parameters.Read.Length = Length;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpStackParamWriteLengthSet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    ULONG Length)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->level_parameters.write.io_size_requested = Length;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->Parameters.Write.Length = Length;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=======================
// EMCPAL IRP Stack Location Manipulation

CSX_STATIC_INLINE VOID
EmcpalExtIrpStackParamReadKeySet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    ULONG Key)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->level_parameters.read.user_context = Key;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->Parameters.Read.Key = Key;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpStackParamWriteKeySet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    ULONG Key)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->level_parameters.write.user_context = Key;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->Parameters.Write.Key = Key;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=======================
// EMCPAL IRP Stack Location Manipulation

#ifdef EMCPAL_SCSI_STUFF_AVAILABLE

#include "cpd_interface_extension.h"

CSX_STATIC_INLINE VOID
EmcpalExtIrpStackParamScsiSrbSet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    PCPD_SCSI_REQ_BLK ScsiSrb)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->level_parameters.scsi.srb = (struct _csx_p_scsi_request_block_s *) ScsiSrb;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->Parameters.Scsi.Srb = ScsiSrb;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PCPD_SCSI_REQ_BLK
EmcpalExtIrpStackParamScsiSrbGet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return (PCPD_SCSI_REQ_BLK) IrpStackLocation->level_parameters.scsi.srb;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IrpStackLocation->Parameters.Scsi.Srb;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PVOID
EmcpalExtIrpStackParamScsiSrbGetDataBuffer(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return cpd_os_io_get_data_buf(((PCPD_SCSI_REQ_BLK) IrpStackLocation->level_parameters.scsi.srb));
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return cpd_os_io_get_data_buf(IrpStackLocation->Parameters.Scsi.Srb);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE UCHAR
EmcpalExtIrpStackParamScsiSrbGetFunction(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return cpd_os_io_get_command(((PCPD_SCSI_REQ_BLK) IrpStackLocation->level_parameters.scsi.srb));
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return cpd_os_io_get_command(IrpStackLocation->Parameters.Scsi.Srb);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

#endif                                     /* PAL_SCSI_STUFF_AVAILABLE */

//=======================
// EMCPAL IRP Stack Location Manipulation

CSX_STATIC_INLINE ULONG
EmcpalExtIrpStackParamIoctlCodeGet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return IrpStackLocation->level_parameters.ioctl.ioctl_code;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IrpStackLocation->Parameters.DeviceIoControl.IoControlCode;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE ULONG
EmcpalExtIrpStackParamIoctlInputSizeGet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return (ULONG) IrpStackLocation->level_parameters.ioctl.io_in_size_requested;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IrpStackLocation->Parameters.DeviceIoControl.InputBufferLength;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE ULONG
EmcpalExtIrpStackParamIoctlOutputSizeGet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return (ULONG) IrpStackLocation->level_parameters.ioctl.io_out_size_requested;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IrpStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpStackParamIoctlCodeSet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    ULONG Code)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->level_parameters.ioctl.ioctl_code = Code;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->Parameters.DeviceIoControl.IoControlCode = Code;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpStackParamIoctlInputSizeSet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    ULONG Size)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->level_parameters.ioctl.io_in_size_requested = Size;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->Parameters.DeviceIoControl.InputBufferLength = Size;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpStackParamIoctlOutputSizeSet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    ULONG Size)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->level_parameters.ioctl.io_out_size_requested = Size;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->Parameters.DeviceIoControl.OutputBufferLength = Size;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=======================
// EMCPAL IRP Stack Location Manipulation

#ifdef EMCPAL_USE_CSX_DCALL
CSX_STATIC_INLINE PVOID
EmcpalExtIrpStackParamIoctlOutputBufferGet(
    PEMCPAL_IO_STACK_LOCATION pIrpStack)
{
    return pIrpStack->level_parameters.ioctl.io_out_buffer; /* CSXBUFFCOMPAT */
} /* C4_INTEGRATED - C4BUG - any use of this is IOCTL model mixing */

CSX_STATIC_INLINE PVOID
EmcpalExtIrpStackParamIoctlInputBufferGet(
    PEMCPAL_IO_STACK_LOCATION pIrpStack)
{
    return CSX_CAST_TO_STD(pIrpStack->level_parameters.ioctl.io_in_buffer); /* CSXBUFFCOMPAT */
} /* C4_INTEGRATED - C4BUG - any use of this is IOCTL model mixing */

#endif // EMCPAL_USE_CSX_DCALL

#ifdef EMCPAL_USE_CSX_DCALL
#endif                                     /* EMCPAL_USE_CSX_DCALL */

//=======================
// EMCPAL IRP Stack Location Manipulation

CSX_STATIC_INLINE PVOID
EmcpalExtIrpStackParamType3InputBufferGet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return (PVOID) IrpStackLocation->level_parameters.ioctl.io_in_buffer;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IrpStackLocation->Parameters.DeviceIoControl.Type3InputBuffer;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpStackParamType3InputBufferSet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    PVOID Buffer)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->level_parameters.ioctl.io_in_buffer = Buffer;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->Parameters.DeviceIoControl.Type3InputBuffer = Buffer;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

// EMCPAL IRP Stack Location Manipulation

CSX_STATIC_INLINE PEMCPAL_FILE_OBJECT
EmcpalExtIrpStackFileObjectGet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return IrpStackLocation->level_device_reference;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IrpStackLocation->FileObject;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpStackFileObjectSet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    PEMCPAL_FILE_OBJECT FileObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->level_device_reference = FileObject;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->FileObject = FileObject;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

#if 0 /* SAFEKILL */
CSX_STATIC_INLINE PEMCPAL_DEVICE_OBJECT
EmcpalExtIrpStackDeviceObjectGet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return IrpStackLocation->level_device_handle;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IrpStackLocation->DeviceObject;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}
#endif

#if 0 /* SAFEKILL */
CSX_STATIC_INLINE VOID
EmcpalExtIrpStackDeviceObjectSet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->level_device_handle = DeviceObject;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->DeviceObject = DeviceObject;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}
#endif

//=======================
// EMCPAL IRP Stack Location Manipulation

CSX_STATIC_INLINE UCHAR
EmcpalExtIrpStackMajorFunctionGet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return IrpStackLocation->dcall_type_code;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IrpStackLocation->MajorFunction;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE UCHAR
EmcpalExtIrpStackMinorFunctionGet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return IrpStackLocation->dcall_type_sub_code;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IrpStackLocation->MinorFunction;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpStackMajorFunctionSet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    UCHAR MajorFunction)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->dcall_type_code = MajorFunction;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->MajorFunction = MajorFunction;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpStackMinorFunctionSet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    UCHAR MinorFunction)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->dcall_type_sub_code = MinorFunction;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->MinorFunction = MinorFunction;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=======================
// EMCPAL IRP Stack Location Manipulation

#if 0 /* SAFEKILL */
CSX_STATIC_INLINE VOID
EmcpalExtIrpStackFlagsGet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return IrpStackLocation->level_flags;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IrpStackLocation->Flags;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}
#endif

#if 0 /* SAFEKILL */
CSX_STATIC_INLINE VOID
EmcpalExtIrpStackFlagsSet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    EMCPAL_IRP_STACK_FLAGS Flags)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->level_flags = Flags;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->Flags = Flags;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}
#endif

CSX_STATIC_INLINE VOID
EmcpalExtIrpStackFlagsAnd(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    EMCPAL_IRP_STACK_FLAGS Flags)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->level_flags &= Flags;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->Flags &= Flags;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpStackFlagsOr(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    EMCPAL_IRP_STACK_FLAGS Flags)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->level_flags |= Flags;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->Flags |= Flags;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=======================
// EMCPAL IRP Stack Location Manipulation

#if 0 /* SAFEKILL */
CSX_STATIC_INLINE PVOID
EmcpalExtIrpStackContextGet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return IrpStackLocation->level_completion_context;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IrpStackLocation->Context;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}
#endif

#if 0 /* SAFEKILL */
CSX_STATIC_INLINE VOID
EmcpalExtIrpStackContextSet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    PVOID Ptr)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->level_completion_context = Ptr;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->Context = Ptr;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}
#endif

//=======================
// EMCPAL IRP Stack Location Manipulation

CSX_STATIC_INLINE PVOID
EmcpalExtIrpStackFsContextGet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_device_user_context_get(IrpStackLocation->level_device_reference);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IrpStackLocation->FileObject->FsContext;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PVOID *
EmcpalExtIrpStackFsContextPtrGet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_device_user_context_get_ptr(IrpStackLocation->level_device_reference);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return &IrpStackLocation->FileObject->FsContext;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpStackFsContextSet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    PVOID FsContext)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_device_user_context_set(IrpStackLocation->level_device_reference, FsContext);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->FileObject->FsContext = FsContext;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=======================
// EMCPAL IRP Stack Location Manipulation

#if 0 /* SAFEKILL */
/* SAFECRAZY */
CSX_STATIC_INLINE PEMCPAL_IRP_COMPLETION_ROUTINE
EmcpalExtIrpStackCompletionRoutineGet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return IrpStackLocation->level_completion_function;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IrpStackLocation->CompletionRoutine;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}
#endif

#if 0 /* SAFEKILL */
/* SAFECRAZY */
CSX_STATIC_INLINE VOID
EmcpalExtIrpStackCompletionRoutineSet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation,
    PEMCPAL_IRP_COMPLETION_ROUTINE CompletionRoutine)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->level_completion_function = CompletionRoutine;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->CompletionRoutine = CompletionRoutine;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}
#endif

CSX_STATIC_INLINE BOOL
EmcpalExtIrpStackCompletionRoutineIsSet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return (IrpStackLocation->
        level_control_flags & (CSX_P_LEVEL_CONTROL_FLAG_INVOKE_CF_ON_SUCCESS | CSX_P_LEVEL_CONTROL_FLAG_INVOKE_CF_ON_ERROR |
            CSX_P_LEVEL_CONTROL_FLAG_INVOKE_CF_ON_CANCEL));
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return (IrpStackLocation->Control & (SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_CANCEL));
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpStackParamCopy(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocationDst,
    PEMCPAL_IO_STACK_LOCATION IrpStackLocationSrc)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocationDst->level_parameters = IrpStackLocationSrc->level_parameters;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocationDst->Parameters = IrpStackLocationSrc->Parameters;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE BOOL
EmcpalExtIrpStackPendingIsSet(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return (IrpStackLocation->level_control_flags & CSX_P_LEVEL_CONTROL_FLAG_PENDING_RETURNED);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return (IrpStackLocation->Control & SL_PENDING_RETURNED);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpStackPendingClear(
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStackLocation->level_control_flags &= ~CSX_P_LEVEL_CONTROL_FLAG_PENDING_RETURNED;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStackLocation->Control &= ~SL_PENDING_RETURNED;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpPendingReturnedClear(
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    Irp->dcall_pending_returned = CSX_FALSE;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    Irp->PendingReturned = CSX_FALSE;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=======================
// EMCPAL IRP Manipulation

/* SAFECRAZY */
#ifdef EMCPAL_USE_CSX_DCALL
#define EmcpalExtIrpFlagsGet(_Irp) ((_Irp)->dcall_flags)
#define EmcpalExtIrpFlagsSet(_Irp, _Flags) ((_Irp)->dcall_flags) = (_Flags)
#else                                      /* EMCPAL_USE_CSX_DCALL */
#define EmcpalExtIrpFlagsGet(_Irp) ((_Irp)->Flags)
#define EmcpalExtIrpFlagsSet(_Irp, _Flags) ((_Irp)->Flags) = (_Flags)
#endif                                     /* EMCPAL_USE_CSX_DCALL */

//=======================
// EMCPAL IRP Manipulation

#define EmcpalExtIrpCompletionRoutineSet(_Irp, _Routine, _Context, _InvokeOnSuccess, _InvokeOnError, _InvokeOnCancel) \
    EmcpalExtIrpCompletionRoutineSetImpl(_Irp, (PEMCPAL_IRP_COMPLETION_ROUTINE)(_Routine), _Context, _InvokeOnSuccess, _InvokeOnError, _InvokeOnCancel)

CSX_STATIC_INLINE VOID
EmcpalExtIrpCompletionRoutineSetImpl(
    PEMCPAL_IRP Irp,
    PEMCPAL_IRP_COMPLETION_ROUTINE CompletionRoutine,
    PVOID Context,
    BOOL InvokeOnSuccess,
    BOOL InvokeOnError,
    BOOL InvokeOnCancel)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_dcall_completion_function_set(Irp, (csx_p_dcall_completion_function_t)CompletionRoutine, Context, InvokeOnSuccess, InvokeOnError, InvokeOnCancel);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE)CompletionRoutine, Context, InvokeOnSuccess, InvokeOnError, InvokeOnCancel);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=======================
// EMCPAL IRP Manipulation

CSX_STATIC_INLINE BOOLEAN
EmcpalExtIrpIsFrom32BitProcess(
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_dcall_from_32_bit_process(Irp);
#else                                      /* EMCPAL_USE_CSX_DCALL */
#ifndef _WIN64
    return 1;
#else                                      /* _WIN64 */
    return IoIs32bitProcess(Irp);
#endif                                     /* _WIN64 */
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=========

CSX_STATIC_INLINE EMCPAL_IRP_KPROCESSOR_MODE
EmcpalExtIrpRequestorModeGet(
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
/* SAFENOEQUIV */
    CSX_UNREFERENCED_PARAMETER(Irp);
/* SAFENOEQUIV */
    return 0;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return Irp->RequestorMode;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpRequestorModeSet(
    PEMCPAL_IRP Irp,
    EMCPAL_IRP_KPROCESSOR_MODE RequestorMode)
{
#ifdef EMCPAL_USE_CSX_DCALL
/* SAFENOEQUIV */
    CSX_UNREFERENCED_PARAMETER(Irp);
    CSX_UNREFERENCED_PARAMETER(RequestorMode);
/* SAFENOEQUIV */
#else                                      /* EMCPAL_USE_CSX_DCALL */
    Irp->RequestorMode = RequestorMode;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=========

CSX_STATIC_INLINE PEMCPAL_MDL
EmcpalExtIrpMdlAddressGet(
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return Irp->dcall_buffer_list;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return Irp->MdlAddress;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PEMCPAL_MDL *
EmcpalExtIrpMdlAddressPtrGet(
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return &Irp->dcall_buffer_list;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return &Irp->MdlAddress;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpMdlAddressSet(
    PEMCPAL_IRP Irp,
    PEMCPAL_MDL Mdl)
{
#ifdef EMCPAL_USE_CSX_DCALL
    Irp->dcall_buffer_list = Mdl;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    Irp->MdlAddress = Mdl;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=========

CSX_STATIC_INLINE PEMCPAL_IRP
EmcpalExtIrpMasterIrpGet(
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return Irp->dcall_master_ptr;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return Irp->AssociatedIrp.MasterIrp;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpMasterIrpSet(
    PEMCPAL_IRP NewIrp,
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    NewIrp->dcall_master_ptr = Irp;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    NewIrp->AssociatedIrp.MasterIrp = Irp;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=========

CSX_STATIC_INLINE ULONG_PTR
EmcpalExtIrpInformationFieldGet(
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return Irp->dcall_status_ptr->io_size_actual;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return Irp->IoStatus.Information;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE ULONG_PTR *
EmcpalExtIrpInformationFieldPtrGet(
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return (ULONG_PTR *) & Irp->dcall_status_ptr->io_size_actual;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return (ULONG_PTR *) & Irp->IoStatus.Information;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpInformationFieldSet(
    PEMCPAL_IRP Irp,
    ULONG_PTR Information)
{
#ifdef EMCPAL_USE_CSX_DCALL
    Irp->dcall_status_ptr->io_size_actual = Information;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    Irp->IoStatus.Information = Information;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=========

CSX_STATIC_INLINE EMCPAL_STATUS
EmcpalExtIrpStatusFieldGet(
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return Irp->dcall_status_ptr->dcall_status_actual;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return Irp->IoStatus.Status;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE EMCPAL_STATUS *
EmcpalExtIrpStatusFieldPtrGet(
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return (EMCPAL_STATUS *)&Irp->dcall_status_ptr->dcall_status_actual;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return (EMCPAL_STATUS *)&Irp->IoStatus.Status;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpStatusFieldSet(
    PEMCPAL_IRP Irp,
    EMCPAL_STATUS Status)
{
#ifdef EMCPAL_USE_CSX_DCALL
    Irp->dcall_status_ptr->dcall_status_actual = (csx_status_e)Status;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    Irp->IoStatus.Status = Status;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PEMCPAL_IRP_STATUS_BLOCK
EmcpalExtIrpStatusPtrGet(
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return &Irp->dcall_status_local;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return &Irp->IoStatus;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=========

CSX_STATIC_INLINE PVOID
EmcpalExtIrpSystemBufferGet(
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
#ifdef C4_INTEGRATED
    if (Irp->dcall_io_buffer != NULL) {
        return Irp->dcall_io_buffer;
    } else {
        // PZPZ revisit this - hack to allow intermixing of IOCTL operation models
        PEMCPAL_IO_STACK_LOCATION pIrpStack = EmcpalIrpGetCurrentStackLocation(Irp);
        return CSX_NOT_NULL(pIrpStack->level_parameters.ioctl.io_out_buffer) ?
                            pIrpStack->level_parameters.ioctl.io_out_buffer :
                            (PVOID)pIrpStack->level_parameters.ioctl.io_in_buffer; /* CSXBUFFCOMPAT */
    }
#else
    return Irp->dcall_io_buffer;                 /* SAFECSXODD */
#endif /* C4_INTEGRATED - C4BUG - any use of this is IOCTL model mixing */
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return Irp->AssociatedIrp.SystemBuffer;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpSystemBufferSet(
    PEMCPAL_IRP Irp,
    PVOID Buffer)
{
#ifdef EMCPAL_USE_CSX_DCALL
    Irp->dcall_io_buffer = Buffer;               /* SAFECSXODD */
#else                                      /* EMCPAL_USE_CSX_DCALL */
    Irp->AssociatedIrp.SystemBuffer = Buffer;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=========

CSX_STATIC_INLINE PVOID
EmcpalExtIrpUserBufferGet(
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return Irp->dcall_user_buffer;         /* SAFECSXODD */
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return Irp->UserBuffer;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpUserBufferSet(
    PEMCPAL_IRP Irp,
    PVOID Buffer)
{
#ifdef EMCPAL_USE_CSX_DCALL
    Irp->dcall_user_buffer = Buffer;       /* SAFECSXODD */
#else                                      /* EMCPAL_USE_CSX_DCALL */
    Irp->UserBuffer = Buffer;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=========

CSX_STATIC_INLINE ULONG
EmcpalExtIrpStackCountGet(
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return Irp->dcall_levels_available;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return Irp->StackCount;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PVOID *
EmcpalExtIrpCurrentStackDriverContextGet(
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return Irp->dcall_user_pointers;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return Irp->Tail.Overlay.DriverContext;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=======================
// EMCPAL IRP Manipulation

CSX_STATIC_INLINE PEMCPAL_LIST_ENTRY
EmcpalExtIrpListEntryPtrGet(
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return &Irp->dcall_user_list_entry_1;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return &Irp->Tail.Overlay.ListEntry;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpListEntryBlinkSet(
    PEMCPAL_IRP Irp,
    PEMCPAL_LIST_ENTRY Ptr)
{
#ifdef EMCPAL_USE_CSX_DCALL
    Irp->dcall_user_list_entry_1.Blink = Ptr;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    Irp->Tail.Overlay.ListEntry.Blink = Ptr;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpListEntryFlinkSet(
    PEMCPAL_IRP Irp,
    PEMCPAL_LIST_ENTRY Ptr)
{
#ifdef EMCPAL_USE_CSX_DCALL
    Irp->dcall_user_list_entry_1.Flink = Ptr;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    Irp->Tail.Overlay.ListEntry.Flink = Ptr;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpListEntryBlinkTrash(
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    Irp->dcall_user_list_entry_1.Blink = CSX_NULL;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    Irp->Tail.Overlay.ListEntry.Blink = CSX_NULL;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpListEntryFlinkTrash(
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    Irp->dcall_user_list_entry_1.Flink = CSX_NULL;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    Irp->Tail.Overlay.ListEntry.Flink = CSX_NULL;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PEMCPAL_LIST_ENTRY
EmcpalExtIrpListEntryBlinkGet(
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return Irp->dcall_user_list_entry_1.Blink;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return Irp->Tail.Overlay.ListEntry.Blink;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PEMCPAL_LIST_ENTRY
EmcpalExtIrpListEntryFlinkGet(
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return Irp->dcall_user_list_entry_1.Flink;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return Irp->Tail.Overlay.ListEntry.Flink;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=======================
// EMCPAL IRP Manipulation

CSX_STATIC_INLINE VOID
EmcpalExtIrpUserInformationFieldSet(
    PEMCPAL_IRP Irp,
    ULONG_PTR Information)
{
#ifdef EMCPAL_USE_CSX_DCALL
    Irp->dcall_status_ptr->io_size_actual = Information;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    Irp->UserIosb->Information = Information;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpUserStatusFieldSet(
    PEMCPAL_IRP Irp,
    EMCPAL_STATUS Status)
{
#ifdef EMCPAL_USE_CSX_DCALL
    Irp->dcall_status_ptr->dcall_status_actual = (csx_status_e)Status;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    Irp->UserIosb->Status = Status;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=========

CSX_STATIC_INLINE PEMCPAL_IRP_CANCEL_FUNCTION
EmcpalExtIrpCancelRoutineGet(
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return Irp->dcall_cancel_function;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return Irp->CancelRoutine;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpCancelRoutineSet(
    PEMCPAL_IRP Irp,
    PEMCPAL_IRP_CANCEL_FUNCTION CancelRoutine)
{
#ifdef EMCPAL_USE_CSX_DCALL
    Irp->dcall_cancel_function = CancelRoutine;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    Irp->CancelRoutine = CancelRoutine;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpCancelInProgressClear(
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    Irp->dcall_cancel_in_progress = CSX_FALSE;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    Irp->Cancel = FALSE;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE EMCPAL_KIRQL
EmcpalExtIrpCancelIrqlGet(
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return (EMCPAL_KIRQL)Irp->CancelIrql;                //PFPF
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return Irp->CancelIrql;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=========

/* SAFECRAZY */
CSX_STATIC_INLINE VOID
EmcpalExtIrpCurrentStackLocationSet(
    PEMCPAL_IRP Irp,
    PEMCPAL_IO_STACK_LOCATION IrpStackLocation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    Irp->dcall_level_ptr_current = IrpStackLocation;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    Irp->Tail.Overlay.CurrentStackLocation = IrpStackLocation;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

#ifdef EMCPAL_KERNEL_STUFF_AVAILABLE

CSX_STATIC_INLINE VOID
EmcpalExtIrpThreadSet(
    PEMCPAL_IRP Irp,
    PVOID Thread)
{
#ifdef EMCPAL_USE_CSX_DCALL
    Irp->dcall_thread = (csx_p_thr_handle_t *) Thread;  /* SAFECSXODD */
#else                                      /* EMCPAL_USE_CSX_DCALL */
    Irp->Tail.Overlay.Thread = (PETHREAD)Thread;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PVOID
EmcpalExtIrpThreadGet(
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return (PVOID) Irp->dcall_thread;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return (PVOID) Irp->Tail.Overlay.Thread;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

#endif                                     /* EMCPAL_KERNEL_STUFF_AVAILABLE */

//=======================
// EMCPAL IRP Status Block Manipulation

CSX_STATIC_INLINE EMCPAL_STATUS
EmcpalExtIrpStatusBlockStatusFieldGet(
    PCEMCPAL_IRP_STATUS_BLOCK IrpStatusBlock)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return IrpStatusBlock->dcall_status_actual;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IrpStatusBlock->Status;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE ULONG_PTR
EmcpalExtIrpStatusBlockInformationFieldGet(
    PCEMCPAL_IRP_STATUS_BLOCK IrpStatusBlock)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return IrpStatusBlock->io_size_actual;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IrpStatusBlock->Information;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpStatusBlockStatusFieldSet(
    PEMCPAL_IRP_STATUS_BLOCK IrpStatusBlock,
    EMCPAL_STATUS Status)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStatusBlock->dcall_status_actual = (csx_status_e)Status;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStatusBlock->Status = Status;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpStatusBlockInformationFieldSet(
    PEMCPAL_IRP_STATUS_BLOCK IrpStatusBlock,
    ULONG_PTR Information)
{
#ifdef EMCPAL_USE_CSX_DCALL
    IrpStatusBlock->io_size_actual = Information;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IrpStatusBlock->Information = Information;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=======================
// EMCPAL IRP Manipulation

/* SAFECRAZY */
CSX_STATIC_INLINE PVOID
EmcpalExtIrpCrazyPtrGet(
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return &Irp->dcall_flags;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return &Irp->Flags;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=======================
// EMCPAL IRP Manipulation

CSX_STATIC_INLINE PEMCPAL_IRP_STATUS_BLOCK
EmcpalExtIrpUserStatusPtrGet(
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return Irp->dcall_status_ptr;          /* SAFECSXODD */
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return Irp->UserIosb;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PEMCPAL_IRP_STATUS_BLOCK *
EmcpalExtIrpUserStatusPtrPtrGet(
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return &Irp->dcall_status_ptr;         /* SAFECSXODD */
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return &Irp->UserIosb;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpUserStatusPtrSet(
    PEMCPAL_IRP Irp,
    PEMCPAL_IRP_STATUS_BLOCK UserStatusPtr)
{
#ifdef EMCPAL_USE_CSX_DCALL
    Irp->dcall_status_ptr = UserStatusPtr; /* SAFECSXODD */
#else                                      /* EMCPAL_USE_CSX_DCALL */
    Irp->UserIosb = UserStatusPtr;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

#ifdef EMCPAL_KERNEL_STUFF_AVAILABLE

CSX_STATIC_INLINE VOID
EmcpalExtIrpUserEventPtrClear(
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    CSX_UNREFERENCED_PARAMETER(Irp);       /* SAFECSXODD */
#else                                      /* EMCPAL_USE_CSX_DCALL */
    Irp->UserEvent = NULL;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

#endif                                     /* EMCPAL_KERNEL_STUFF_AVAILABLE */

//=======================
// EMCPAL IRP Manipulation

#ifdef EMCPAL_USE_CSX_DCALL
#define EMCPAL_DCALL_FLAGS_WE_ALLOCATED_BUFFER 0x1000
#endif                                     /* EMCPAL_USE_CSX_DCALL */

#if defined(EMCPAL_USE_CSX_DCALL) && defined(EMCPAL_USE_CSX_MRE)
#define EMCPAL_IRP_COMPLETION_EVENT_PTR EMCPAL_RENDEZVOUS_EVENT *
#define EmcpalIrpCompletionEventSet(_e) EmcpalRendezvousEventSet(_e)
#else
#define EMCPAL_IRP_COMPLETION_EVENT_PTR PKEVENT
#define EmcpalIrpCompletionEventSet(_e) KeSetEvent(_e, 0, FALSE)
#endif

#ifdef EMCPAL_USE_CSX_DCALL

EMCPAL_API csx_status_e CSX_GX_CI_DEFCC
EmcpalExtIrpBuildSyncIoCompletion(
    csx_p_device_handle_t device_handle,
    csx_p_dcall_body_t * dcall_body,
    csx_p_dcall_completion_context_t level_completion_context);

EMCPAL_API csx_status_e CSX_GX_CI_DEFCC
EmcpalExtIrpBuildIoctlCompletion(
    csx_p_device_handle_t device_handle,
    csx_p_dcall_body_t * dcall_body,
    csx_p_dcall_completion_context_t level_completion_context);

#endif                                     /* EMCPAL_USE_CSX_DCALL */

//=======================
// EMCPAL IRP Manipulation

CSX_STATIC_INLINE PEMCPAL_IRP
EmcpalExtIrpBuildIoctl(
    ULONG IoControlCode,
    PEMCPAL_DEVICE_OBJECT DeviceObject,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength,
    BOOLEAN InternalDeviceIoControl,
    EMCPAL_IRP_COMPLETION_EVENT_PTR Event,
    PEMCPAL_IRP_STATUS_BLOCK IoStatusBlock)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_dcall_body_t *dcall_body;
    csx_p_dcall_level_t *dcall_level;
    dcall_body = csx_p_dcall_allocate(csx_p_device_get_dcall_levels_required(DeviceObject));
    if (CSX_IS_NULL(dcall_body)) {
        return CSX_NULL;
    }
    if (CSX_NOT_NULL(IoStatusBlock)) {
        dcall_body->dcall_status_ptr = IoStatusBlock;
    }
    dcall_body->dcall_io_buffer = CSX_NULL;
    dcall_body->dcall_user_buffer = CSX_NULL;
    dcall_level = csx_p_dcall_get_next_level(dcall_body);
    if (CSX_NOT_FALSE(InternalDeviceIoControl)) {
        dcall_level->dcall_type_code = EMCPAL_IRP_TYPE_CODE_INTERNAL_DEVICE_CONTROL;
    } else {
        dcall_level->dcall_type_code = EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL;
    }
    dcall_level->level_parameters.ioctl.io_in_size_requested = InputBufferLength;
    dcall_level->level_parameters.ioctl.io_out_size_requested = OutputBufferLength;
    dcall_level->level_parameters.ioctl.io_in_buffer = CSX_NULL;
    dcall_level->level_parameters.ioctl.io_out_buffer = CSX_NULL;
    dcall_level->level_parameters.ioctl.ioctl_code = IoControlCode;
    {
        csx_nuint_t Method = CSX_P_IOCTL_METHOD_GET(IoControlCode);

        if (Method == CSX_P_IOCTL_METHOD_BUFFERED) {
            csx_nuint_t BytesNeeded = CSX_MAX(InputBufferLength, OutputBufferLength);
            if (CSX_IS_ZERO(BytesNeeded)) {
                dcall_body->dcall_io_buffer = CSX_NULL;
            } else {
                dcall_body->dcall_io_buffer = csx_p_util_mem_tagged_alloc_maybe(CSX_P_UTIL_MEM_TYPE_WIRED, CSX_TAG("_sb_"), BytesNeeded);
                if (CSX_IS_NULL(dcall_body->dcall_io_buffer)) {
                    csx_p_dcall_free(dcall_body);
                    return CSX_NULL;
                }
                if (InputBufferLength != 0) {
                    csx_p_memcpy(dcall_body->dcall_io_buffer, InputBuffer, InputBufferLength);
                }
                dcall_body->dcall_flags |= EMCPAL_DCALL_FLAGS_WE_ALLOCATED_BUFFER;
            }
            dcall_level->level_parameters.ioctl.io_in_buffer = dcall_body->dcall_io_buffer; /* CSXBUFFCOMPAT */
            dcall_level->level_parameters.ioctl.io_out_buffer = dcall_body->dcall_io_buffer; /* CSXBUFFCOMPAT */
            dcall_body->dcall_user_buffer = OutputBuffer;
        } else if (Method == CSX_P_IOCTL_METHOD_NEITHER) {
            dcall_level->level_parameters.ioctl.io_in_buffer = InputBuffer; /* CSXBUFFCOMPAT */
            dcall_level->level_parameters.ioctl.io_out_buffer = OutputBuffer; /* CSXBUFFCOMPAT */
            dcall_body->dcall_user_buffer = OutputBuffer;
        } else {
            CSX_PANIC();
        }
    }
    csx_p_dcall_completion_function_set(dcall_body, EmcpalExtIrpBuildIoctlCompletion, (csx_pvoid_t) Event, CSX_TRUE, CSX_TRUE, CSX_TRUE);
    return dcall_body;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IoBuildDeviceIoControlRequest(IoControlCode, DeviceObject,
        InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, InternalDeviceIoControl, Event, IoStatusBlock);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PEMCPAL_IRP
EmcpalExtIrpBuildAsyncIo(
    ULONG MajorFunction,
    PEMCPAL_DEVICE_OBJECT DeviceObject,
    PVOID Buffer,
    ULONG Length,
    EMCPAL_PLARGE_INTEGER StartingOffset,
    PEMCPAL_IRP_STATUS_BLOCK IoStatusBlock)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_dcall_body_t *dcall_body;
    csx_p_dcall_level_t *dcall_level;
    dcall_body = csx_p_dcall_allocate(csx_p_device_get_dcall_levels_required(DeviceObject));
    if (CSX_IS_NULL(dcall_body)) {
        return CSX_NULL;
    }
    if (CSX_NOT_NULL(IoStatusBlock)) {
        dcall_body->dcall_status_ptr = IoStatusBlock;
    }
    dcall_body->dcall_user_buffer = CSX_NULL;
    dcall_level = csx_p_dcall_get_next_level(dcall_body);
    dcall_level->dcall_type_code = (csx_u8_t)MajorFunction;
    if (csx_p_device_io_style_is_direct(DeviceObject) && CSX_NOT_ZERO(Length)) {
        dcall_body->dcall_buffer_list = csx_p_dcall_buffer_allocate(Buffer, Length);
        if (CSX_IS_NULL(dcall_body->dcall_buffer_list)) {
            csx_p_dcall_free(dcall_body);
            return CSX_NULL;
        }
    }
    if (MajorFunction == EMCPAL_IRP_TYPE_CODE_READ) {
        dcall_level->level_parameters.read.io_offset = StartingOffset->QuadPart;
        dcall_level->level_parameters.read.io_size_requested = Length;
        dcall_level->level_parameters.read.io_buffer = Buffer; /* CSXBUFFCOMPAT */
    } else if (MajorFunction == EMCPAL_IRP_TYPE_CODE_WRITE) {
        dcall_level->level_parameters.write.io_offset = StartingOffset->QuadPart;
        dcall_level->level_parameters.write.io_size_requested = Length;
        dcall_level->level_parameters.write.io_buffer = Buffer; /* CSXBUFFCOMPAT */
    } else if (MajorFunction == EMCPAL_IRP_TYPE_CODE_PNP) {
        CSX_ASSERT_H_RDC(CSX_IS_NULL(Buffer));
        CSX_ASSERT_H_RDC(CSX_IS_ZERO(Length));
    } else {
        CSX_ASSERT_H_RDC(CSX_IS_NULL(Buffer));
        CSX_ASSERT_H_RDC(CSX_IS_ZERO(Length));
        CSX_PANIC();
    }
    return dcall_body;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IoBuildAsynchronousFsdRequest(MajorFunction, DeviceObject, Buffer, Length, StartingOffset, IoStatusBlock);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PEMCPAL_IRP
EmcpalExtIrpBuildSyncIo(
    ULONG MajorFunction,
    PEMCPAL_DEVICE_OBJECT DeviceObject,
    PVOID Buffer,
    ULONG Length,
    EMCPAL_PLARGE_INTEGER StartingOffset,
    EMCPAL_IRP_COMPLETION_EVENT_PTR Event,
    PEMCPAL_IRP_STATUS_BLOCK IoStatusBlock)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_dcall_body_t *dcall_body;
    dcall_body = EmcpalExtIrpBuildAsyncIo(MajorFunction, DeviceObject, Buffer, Length, StartingOffset, IoStatusBlock);
    if (CSX_NOT_NULL(dcall_body)) {
        csx_p_dcall_completion_function_set(dcall_body, EmcpalExtIrpBuildSyncIoCompletion, (csx_pvoid_t) Event, CSX_TRUE, CSX_TRUE,
            CSX_TRUE);
    }
    return dcall_body;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IoBuildSynchronousFsdRequest(MajorFunction, DeviceObject, Buffer, Length, StartingOffset, Event, IoStatusBlock);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=======================
// EMCPAL Device Object Manipulation

CSX_STATIC_INLINE EMCPAL_STATUS
EmcpalExtDeviceReference(
    PEMCPAL_DEVICE_OBJECT DeviceObject,
    ULONG DesiredAccess,
    PVOID ObjectType,
    EMCPAL_IRP_KPROCESSOR_MODE AccessMode)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_status_e Status;
    CSX_UNREFERENCED_PARAMETER(DesiredAccess);
    CSX_UNREFERENCED_PARAMETER(ObjectType);
    CSX_UNREFERENCED_PARAMETER(AccessMode);
    Status = csx_p_device_reference(DeviceObject);
    return Status;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return ObReferenceObjectByPointer(DeviceObject, DesiredAccess, (POBJECT_TYPE) ObjectType, AccessMode);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtDeviceDereference(
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_status_e Status;
    Status = csx_p_device_dereference(DeviceObject);
    CSX_ASSERT_SUCCESS_H_RDC(Status);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    ObDereferenceObject(DeviceObject);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtDeviceClose(
    PEMCPAL_FILE_OBJECT FileObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_status_e Status;
    Status = csx_p_device_close(FileObject);
    CSX_ASSERT_SUCCESS_H_RDC(Status);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    ObDereferenceObject(FileObject);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

#ifdef EMCPAL_USE_CSX_DCALL

#define EmcpalExtDeviceOpen( \
    DeviceName, \
    DesiredAccess, \
    FileObjectRv, \
    DeviceObjectRv) \
    EmcpalExtDeviceOpenImplA( \
        EmcpalDriverGetCurrentClientObject(), \
        DeviceName, \
        DesiredAccess, \
        FileObjectRv, \
        DeviceObjectRv)

#define EmcpalExtDeviceOpenW(\
	DeviceName, \
	DesiredAccess, \
	FileObjectRv, \
	DeviceObjectRv)\
	EmcpalExtDeviceOpenImplW(\
	    EmcpalDriverGetCurrentClientObject(), \
	    DeviceName, \
	    DesiredAccess, \
           FileObjectRv, \
            DeviceObjectRv)

#else

#define EmcpalExtDeviceOpen( \
    DeviceName, \
    DesiredAccess, \
    FileObjectRv, \
    DeviceObjectRv) \
    EmcpalExtDeviceOpenImplA( \
        CSX_NULL, \
        DeviceName, \
        DesiredAccess, \
        FileObjectRv, \
        DeviceObjectRv)

#define EmcpalExtDeviceOpenW(\
	DeviceName, \
	DesiredAccess, \
	FileObjectRv, \
	DeviceObjectRv)\
	EmcpalExtDeviceOpenImplW(\
	    CSX_NULL, \
	    DeviceName, \
	    DesiredAccess, \
           FileObjectRv, \
            DeviceObjectRv)

#endif

#ifdef SIMMODE_ENV
#define EmcpalExtDeviceOpen2( \
    DeviceName, \
    DesiredAccess, \
    FileObjectRv, \
    DeviceObjectRv) \
    EmcpalExtDeviceOpenImplA( \
        EmcpalDriverGetCurrentClientObject(), \
        DeviceName, \
        DesiredAccess, \
        FileObjectRv, \
        DeviceObjectRv)
#else
#define EmcpalExtDeviceOpen2( \
    DeviceName, \
    DesiredAccess, \
    FileObjectRv, \
    DeviceObjectRv) \
    EmcpalExtDeviceOpenImplA( \
        EmcPALDllGetClient(), \
        DeviceName, \
        DesiredAccess, \
        FileObjectRv, \
        DeviceObjectRv)
#endif

CSX_STATIC_INLINE EMCPAL_STATUS
EmcpalExtDeviceOpenImplW(
    PEMCPAL_CLIENT pPalClient,
    EMCPAL_PUNICODE_STRING DeviceName,
    EMCPAL_ACCESS_MASK DesiredAccess,
    PEMCPAL_FILE_OBJECT * FileObjectRv,
    PEMCPAL_DEVICE_OBJECT * DeviceObjectRv)
{
#ifdef EMCPAL_USE_CSX_DCALL
    EMCPAL_STATUS Status;
    csx_string_t CsxDeviceName = CSX_NULL;
    csx_p_device_reference_t CsxDeviceReference;
    csx_p_device_handle_t CsxDeviceHandle;

    CSX_UNREFERENCED_PARAMETER(DesiredAccess);

    Status = csx_rt_mx_nt_RtlUnicodeStringToAsciiString(DeviceName, &CsxDeviceName);
    if (CSX_FAILURE(Status)) {
        goto bad;
    }
    Status = csx_p_device_open(EmcpalClientGetCsxModuleContext(pPalClient), CsxDeviceName, &CsxDeviceReference);
    if (CSX_FAILURE(Status)) {
        goto bad;
    }
    CsxDeviceHandle = csx_p_device_get_handle(CsxDeviceReference);
    *FileObjectRv = CsxDeviceReference;
    *DeviceObjectRv = CsxDeviceHandle;
    Status = CSX_STATUS_SUCCESS;
  bad:
    if (CSX_NOT_NULL(CsxDeviceName)) {
        csx_rt_mx_nt_RtlFreeAsciiString(CsxDeviceName);
    }
    return Status;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IoGetDeviceObjectPointer(DeviceName, DesiredAccess, FileObjectRv, DeviceObjectRv);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE EMCPAL_STATUS
EmcpalExtDeviceOpenImplA(
    PEMCPAL_CLIENT pPalClient,
    csx_cstring_t CsxDeviceName,
    EMCPAL_ACCESS_MASK DesiredAccess,
    PEMCPAL_FILE_OBJECT * FileObjectRv,
    PEMCPAL_DEVICE_OBJECT * DeviceObjectRv)
{
	EMCPAL_STATUS Status;
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_device_reference_t CsxDeviceReference;
    csx_p_device_handle_t    CsxDeviceHandle;

    CSX_UNREFERENCED_PARAMETER(DesiredAccess);

    Status = csx_p_device_open(EmcpalClientGetCsxModuleContext(pPalClient), CsxDeviceName, &CsxDeviceReference);
    if (CSX_FAILURE(Status)) {
        return Status;
    }
    
    CsxDeviceHandle = csx_p_device_get_handle(CsxDeviceReference);
    *FileObjectRv = CsxDeviceReference;
    *DeviceObjectRv = CsxDeviceHandle;
    Status = CSX_STATUS_SUCCESS;
    return Status;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    EMCPAL_UNICODE_STRING   DeviceUnicodeName;
    EMCPAL_ANSI_STRING      DeviceStringName;

    EmcpalInitAnsiString( &DeviceStringName, CsxDeviceName); 
    Status = EmcpalAnsiStringToUnicodeString( &DeviceUnicodeName, &DeviceStringName, TRUE);

    if (CSX_FAILURE(Status))
    {
        goto bad;
    }

    Status = IoGetDeviceObjectPointer(&DeviceUnicodeName, DesiredAccess, FileObjectRv, DeviceObjectRv);

  bad:
    if(CSX_NOT_NULL(DeviceUnicodeName.Buffer)) {
        EmcpalFreeUnicodeString(&DeviceUnicodeName);
    }
    return Status;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}


CSX_STATIC_INLINE VOID
EmcpalExtDeviceEnable(
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_device_enable(DeviceObject);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtDeviceIoBufferedSet(
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_device_io_style_set_buffered(DeviceObject);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    DeviceObject->Flags |= DO_BUFFERED_IO;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtDeviceIoDirectSet(
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_device_io_style_set_direct(DeviceObject);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    DeviceObject->Flags |= DO_DIRECT_IO;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtDeviceIoDirectClear(
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_device_io_style_unset_direct(DeviceObject);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    DeviceObject->Flags &= ~DO_DIRECT_IO;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE BOOL
EmcpalExtDeviceIoIsDirectOrBuffered(
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_device_io_style_is_buffered(DeviceObject) || csx_p_device_io_style_is_direct(DeviceObject);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return DeviceObject->Flags & (DO_DIRECT_IO | DO_BUFFERED_IO);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE BOOL
EmcpalExtDeviceIoIsDirect(
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_device_io_style_is_direct(DeviceObject);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return DeviceObject->Flags & (DO_DIRECT_IO);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE BOOL
EmcpalExtDeviceIoIsBuffered(
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_device_io_style_is_buffered(DeviceObject);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return DeviceObject->Flags & (DO_BUFFERED_IO);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtDeviceIoFlagsCopy(
    PEMCPAL_DEVICE_OBJECT DstDeviceObject,
    PEMCPAL_DEVICE_OBJECT SrcDeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_device_io_style_unset_direct(DstDeviceObject);
    csx_p_device_io_style_unset_buffered(DstDeviceObject);
    if (csx_p_device_io_style_is_direct(SrcDeviceObject)) {
        csx_p_device_io_style_set_direct(DstDeviceObject);
    }
    if (csx_p_device_io_style_is_buffered(SrcDeviceObject)) {
        csx_p_device_io_style_set_buffered(DstDeviceObject);
    }
#else                                      /* EMCPAL_USE_CSX_DCALL */
    DstDeviceObject->Flags &= ~(DO_DIRECT_IO | DO_BUFFERED_IO);
    if (SrcDeviceObject->Flags & DO_DIRECT_IO) {
        DstDeviceObject->Flags |= DO_DIRECT_IO;
    }
    if (SrcDeviceObject->Flags & DO_BUFFERED_IO) {
        DstDeviceObject->Flags |= DO_BUFFERED_IO;
    }
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=======================
// EMCPAL Device Object Manipulation

CSX_STATIC_INLINE VOID
EmcpalExtDevicePowerPagableSet(
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
/* SAFENOEQUIV */
    CSX_UNREFERENCED_PARAMETER(DeviceObject);
/* SAFENOEQUIV */
    return;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    DeviceObject->Flags |= DO_POWER_PAGABLE;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtDevicePowerPagableClear(
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
/* SAFENOEQUIV */
    CSX_UNREFERENCED_PARAMETER(DeviceObject);
/* SAFENOEQUIV */
    return;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    DeviceObject->Flags &= ~DO_POWER_PAGABLE;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtDeviceVerifyVolumeSet(
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
/* SAFENOEQUIV */
    CSX_UNREFERENCED_PARAMETER(DeviceObject);
/* SAFENOEQUIV */
    return;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    DeviceObject->Flags |= DO_VERIFY_VOLUME;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtDeviceVerifyVolumeClear(
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
/* SAFENOEQUIV */
    CSX_UNREFERENCED_PARAMETER(DeviceObject);
/* SAFENOEQUIV */
    return;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    DeviceObject->Flags &= ~DO_VERIFY_VOLUME;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE BOOL
EmcpalExtDeviceVerifyVolumeIsSet(
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
/* SAFENOEQUIV */
    CSX_UNREFERENCED_PARAMETER(DeviceObject);
/* SAFENOEQUIV */
    return FALSE;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return DeviceObject->Flags & DO_VERIFY_VOLUME;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE BOOL
EmcpalExtDevicePowerInrushIsSet(
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
/* SAFENOEQUIV */
    CSX_UNREFERENCED_PARAMETER(DeviceObject);
/* SAFENOEQUIV */
    return FALSE;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return DeviceObject->Flags & DO_POWER_INRUSH;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=======================
// EMCPAL Device Object Manipulation

CSX_STATIC_INLINE ULONG
EmcpalExtDeviceAlignmentRequirementGet(
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
/* SAFENOEQUIV */
    return sizeof(csx_u64_t);
/* SAFENOEQUIV */
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return DeviceObject->AlignmentRequirement;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtDeviceAlignmentRequirementSet(
    PEMCPAL_DEVICE_OBJECT DeviceObject,
    ULONG AlignmentRequirement)
{
#ifdef EMCPAL_USE_CSX_DCALL
/* SAFENOEQUIV */
    CSX_UNREFERENCED_PARAMETER(AlignmentRequirement);
/* SAFENOEQUIV */
#else                                      /* EMCPAL_USE_CSX_DCALL */
    DeviceObject->AlignmentRequirement = AlignmentRequirement;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=======================
// EMCPAL Device Object Manipulation

CSX_STATIC_INLINE BOOL
EmcpalExtDeviceRemoveableMediaIsSet(
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
/* SAFENOEQUIV */
    CSX_UNREFERENCED_PARAMETER(DeviceObject);
/* SAFENOEQUIV */
    return CSX_FALSE;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtDeviceRemoveableMediaSet(
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
/* SAFENOEQUIV */
    CSX_UNREFERENCED_PARAMETER(DeviceObject);
/* SAFENOEQUIV */
#else                                      /* EMCPAL_USE_CSX_DCALL */
    DeviceObject->Characteristics |= FILE_REMOVABLE_MEDIA;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE ULONG
EmcpalExtDeviceTypeGet(
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_device_type_get(DeviceObject);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return DeviceObject->DeviceType;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

#if 0 /* SAFEKILL */
CSX_STATIC_INLINE ULONG
EmcpalExtDeviceReferenceCountGet(
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_device_get_open_count(DeviceObject);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return DeviceObject->ReferenceCount;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}
#endif

#if 0 /* SAFEKILL */
CSX_STATIC_INLINE VOID
EmcpalExtDeviceReferenceCountInc(
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
/* SAFEBUG - no sane users of this */
    CSX_BREAK();
    CSX_UNREFERENCED_PARAMETER(DeviceObject);
/* SAFEBUG - no sane users of this */
#else
    DeviceObject->ReferenceCount++;
#endif
}
#endif

#if 0 /* SAFEKILL */
/* SAFECRAZY */
CSX_STATIC_INLINE VOID
EmcpalExtDeviceReferenceCountDec(
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
/* SAFEBUG - no sane users of this */
    CSX_BREAK();
    CSX_UNREFERENCED_PARAMETER(DeviceObject);
/* SAFEBUG - no sane users of this */
#else
    DeviceObject->ReferenceCount--;
#endif
}
#endif

//=======================
// EMCPAL Device Object Manipulation

#if 0 /* SAFEKILL */
CSX_STATIC_INLINE PVOID
EmcpalExtDeviceExtensionGet(
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_device_get_private_data(DeviceObject);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return DeviceObject->DeviceExtension;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}
#endif

CSX_STATIC_INLINE VOID
EmcpalExtDeviceExtensionSet(
    PEMCPAL_DEVICE_OBJECT DeviceObject,
    PVOID Extension)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_device_set_private_data(DeviceObject, Extension);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    DeviceObject->DeviceExtension = Extension;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

#if 0 /* SAFEKILL */
CSX_STATIC_INLINE PEMCPAL_DEVICE_OBJECT
EmcpalExtDeviceObjectFromFileObject(
    PEMCPAL_FILE_OBJECT FileObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_device_get_handle(FileObject);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IoGetRelatedDeviceObject(FileObject);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}
#endif

CSX_STATIC_INLINE PEMCPAL_NATIVE_DRIVER_OBJECT
EmcpalExtDeviceDriverObjectGet(
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_device_get_driver(DeviceObject);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return DeviceObject->DriverObject;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=======================
// EMCPAL Device Object Manipulation

#ifdef EMCPAL_KERNEL_STUFF_AVAILABLE_WIN_ONLY

/* *INDENT-OFF* */
typedef VOID (*EMCPAL_IO_DPC_ROUTINE) (PKDPC Dpc, PEMCPAL_DEVICE_OBJECT DeviceObject, PEMCPAL_IRP Irp, PVOID Context);
/* *INDENT-ON* */

#endif                                     /* EMCPAL_KERNEL_STUFF_AVAILABLE */

//=======================
// EMCPAL Device Object Manipulation

#ifdef EMCPAL_KERNEL_STUFF_AVAILABLE

CSX_STATIC_INLINE PEMCPAL_DEVICE_OBJECT
EmcpalExtDeviceAttachCreate(
    PEMCPAL_DEVICE_OBJECT SourceDeviceObject,
    PEMCPAL_DEVICE_OBJECT TargetDeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_status_e status;
    PEMCPAL_DEVICE_OBJECT PreviousTopDeviceObject;
    status = csx_p_query_filter_device(TargetDeviceObject, &PreviousTopDeviceObject);
    if (CSX_FAILURE(status)) {
        return NULL;
    }
    status = csx_p_attach_filter_device(SourceDeviceObject, TargetDeviceObject);
    if (CSX_FAILURE(status)) {
        return NULL;
    }
    return PreviousTopDeviceObject;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IoAttachDeviceToDeviceStack(SourceDeviceObject, TargetDeviceObject);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PEMCPAL_DEVICE_OBJECT
EmcpalExtDeviceAttachGet(
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_status_e status;
    PEMCPAL_DEVICE_OBJECT PreviousTopDeviceObject;
    status = csx_p_device_reference(DeviceObject);
    if (CSX_FAILURE(status)) {
        return NULL;
    }
    status = csx_p_query_filter_device(DeviceObject, &PreviousTopDeviceObject);
    if (CSX_FAILURE(status)) {
        return NULL;
    }
    return PreviousTopDeviceObject;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IoGetAttachedDeviceReference(DeviceObject);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

#ifndef ALAMOSA_WINDOWS_ENV
CSX_STATIC_INLINE PEMCPAL_DEVICE_OBJECT
EmcpalExtDeviceAttachQuery(
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_status_e status;
    PEMCPAL_DEVICE_OBJECT PreviousTopDeviceObject;
    status = csx_p_query_filter_device(DeviceObject, &PreviousTopDeviceObject);
    if (CSX_FAILURE(status)) {
        return NULL;
    }
    return PreviousTopDeviceObject;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IoGetAttachedDevice(DeviceObject);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - only needed in SAFE world */

CSX_STATIC_INLINE VOID
EmcpalExtDeviceAttachDestroy(
    PEMCPAL_DEVICE_OBJECT TargetDeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_status_e status;
    status = csx_p_detach_filter_device(TargetDeviceObject);
    CSX_ASSERT_SUCCESS_H_RDC(status);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IoDetachDevice(TargetDeviceObject);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

#endif                                     /* EMCPAL_KERNEL_STUFF_AVAILABLE */

//=======================
// EMCPAL Device Object Manipulation

CSX_STATIC_INLINE EMCPAL_STATUS
EmcpalExtIrpSendAsync(
    PEMCPAL_DEVICE_OBJECT DeviceObject,
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_priv_dcall_send(DeviceObject, Irp);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IoCallDriver(DeviceObject, Irp);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

#ifdef EMCPAL_KERNEL_STUFF_AVAILABLE

CSX_STATIC_INLINE EMCPAL_STATUS
EmcpalExtIrpSendPoAsync(
    PEMCPAL_DEVICE_OBJECT DeviceObject,
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_priv_dcall_send(DeviceObject, Irp);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return PoCallDriver(DeviceObject, Irp);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE BOOLEAN
EmcpalExtIrpSendSync(
    PEMCPAL_DEVICE_OBJECT DeviceObject,
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_status_e status;
    status = csx_p_dcall_forward(EMCPAL_MY_MODULE_CONTEXT(), DeviceObject, Irp);
    return (CSX_FAILURE(status)) ? FALSE : TRUE;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IoForwardIrpSynchronously(DeviceObject, Irp);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtDeviceIrpQueueStartNext(
    PEMCPAL_DEVICE_OBJECT DeviceObject,
    BOOLEAN Cancelable)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return;                                //PFPF
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IoStartNextPacket(DeviceObject, Cancelable);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PEMCPAL_IRP
EmcpalExtDeviceIrpQueueCurrentIrpGet(
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return CSX_NULL;                       //PFPF
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return DeviceObject->CurrentIrp;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpStartPoNext(
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
/* SAFENOEQUIV */
    CSX_UNREFERENCED_PARAMETER(Irp);
/* SAFENOEQUIV */
    return;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    PoStartNextPowerIrp(Irp);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

#endif                                     /* EMCPAL_KERNEL_STUFF_AVAILABLE */

//=======================
// EMCPAL Device Object Manipulation

CSX_STATIC_INLINE EMCPAL_STATUS
EmcpalExtIrpDoIoctl(
    PEMCPAL_CLIENT pPalClient,
    ULONG IoControlCode,
    PEMCPAL_DEVICE_OBJECT DeviceObject,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength)
{
    EMCPAL_STATUS Status;
    PEMCPAL_IRP pIrp;
    EMCPAL_RENDEZVOUS_EVENT Event;
    EMCPAL_IRP_STATUS_BLOCK IoStatusBlock;
    EmcpalRendezvousEventCreate(pPalClient, &Event, "do_ioctl", FALSE);
    pIrp = EmcpalExtIrpBuildIoctl(
        IoControlCode,
        DeviceObject,
        InputBuffer,
        InputBufferLength,
        OutputBuffer,
        OutputBufferLength,
        FALSE,
        &Event,
        &IoStatusBlock);
    if  (NULL == pIrp) {
        EmcpalRendezvousEventDestroy(&Event);
        return EMCPAL_STATUS_INSUFFICIENT_RESOURCES;
    }
    Status = EmcpalExtIrpSendAsync(DeviceObject, pIrp);
    if  (Status == EMCPAL_STATUS_PENDING) {
        EmcpalRendezvousEventWait(&Event, EMCPAL_TIMEOUT_INFINITE_WAIT);
        Status = EmcpalExtIrpStatusBlockStatusFieldGet(&IoStatusBlock);
    }
    EmcpalRendezvousEventDestroy(&Event);
    return Status;
}

CSX_STATIC_INLINE EMCPAL_STATUS
EmcpalExtIrpDoIoctlWithTimeout(
    PEMCPAL_CLIENT pPalClient,
    ULONG IoControlCode,
    PEMCPAL_DEVICE_OBJECT DeviceObject,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength,
    EMCPAL_TIMEOUT_MSECS TimeoutMsecs) /* SAFEBUG - every use of this is a bug */
{
    EMCPAL_STATUS Status;
    PEMCPAL_IRP pIrp;
    EMCPAL_RENDEZVOUS_EVENT Event;
    EMCPAL_IRP_STATUS_BLOCK IoStatusBlock;
    EmcpalRendezvousEventCreate(pPalClient, &Event, "do_ioctl", FALSE);
    pIrp = EmcpalExtIrpBuildIoctl(
        IoControlCode,
        DeviceObject,
        InputBuffer,
        InputBufferLength,
        OutputBuffer,
        OutputBufferLength,
        FALSE,
        &Event,
        &IoStatusBlock);
    if  (NULL == pIrp) {
        EmcpalRendezvousEventDestroy(&Event);
        return EMCPAL_STATUS_INSUFFICIENT_RESOURCES;
    }
    Status = EmcpalExtIrpSendAsync(DeviceObject, pIrp);
    if  (Status == EMCPAL_STATUS_PENDING) {
        Status = EmcpalRendezvousEventWait(&Event, TimeoutMsecs);
        if (Status != EMCPAL_STATUS_TIMEOUT) {
            Status = EmcpalExtIrpStatusBlockStatusFieldGet(&IoStatusBlock);
        }
    }
    EmcpalRendezvousEventDestroy(&Event);
    return Status;
}

//=======================
// EMCPAL Device Object Manipulation

CSX_STATIC_INLINE PEMCPAL_DEVICE_OBJECT
EmcpalExtDeviceListNextGet(
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_device_get_next(DeviceObject);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return DeviceObject->NextDevice;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PEMCPAL_DEVICE_OBJECT
EmcpalExtDeviceListFirstGet(
    PEMCPAL_NATIVE_DRIVER_OBJECT DriverObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_device_get_first(DriverObject);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return DriverObject->DeviceObject;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtDeviceListFirstSet(
    PEMCPAL_NATIVE_DRIVER_OBJECT DriverObject,
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
/* SAFEBUG - no sane users of this */
    CSX_BREAK();
    CSX_UNREFERENCED_PARAMETER(DriverObject);
    CSX_UNREFERENCED_PARAMETER(DeviceObject);
/* SAFEBUG - no sane users of this */
#else                                      /* EMCPAL_USE_CSX_DCALL */
    DriverObject->DeviceObject = DeviceObject;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=======================
// EMCPAL MDL Object Manipulation

CSX_STATIC_INLINE VOID
EmcpalExtMdlFree(
    PEMCPAL_MDL Mdl)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_dcall_buffer_free(Mdl);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IoFreeMdl(Mdl);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PEMCPAL_MDL
EmcpalExtMdlAllocate(
    PVOID VirtualAddress,
    ULONG Length,
    BOOLEAN SecondaryBuffer,
    BOOLEAN ChargeQuota,
    PEMCPAL_IRP Irp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_dcall_buffer_t *dcall_buffer;
    CSX_UNREFERENCED_PARAMETER(ChargeQuota);    /* SAFEKILL */
    dcall_buffer = csx_p_dcall_buffer_allocate(VirtualAddress, Length);
    if (CSX_IS_NULL(dcall_buffer)) {
        return dcall_buffer;
    }
    if (CSX_IS_NULL(Irp)) {
        return dcall_buffer;
    }
    if (CSX_IS_FALSE(SecondaryBuffer)) {
        CSX_ASSERT_H_RDC(CSX_IS_NULL(Irp->dcall_buffer_list));
        Irp->dcall_buffer_list = dcall_buffer;
    } else {
        csx_p_dcall_buffer_t *dcall_buffer_last = Irp->dcall_buffer_list;
        while (CSX_NOT_NULL(dcall_buffer_last->buffer_next)) {
            dcall_buffer_last = dcall_buffer_last->buffer_next;
        }
        dcall_buffer_last->buffer_next = dcall_buffer;
    }
    return dcall_buffer;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IoAllocateMdl(VirtualAddress, Length, SecondaryBuffer, ChargeQuota, Irp);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtMdlInitialize(
    PEMCPAL_MDL Mdl,
    PVOID VirtualAddress,
    SIZE_T Length)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_dcall_buffer_populate(Mdl, VirtualAddress, Length);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    MmInitializeMdl(Mdl, VirtualAddress, Length);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtMdlBuildPartial(
    PEMCPAL_MDL SourceMdl,
    PEMCPAL_MDL TargetMdl,
    PVOID VirtualAddress,
    ULONG Length)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_dcall_buffer_create_from_existing(SourceMdl, TargetMdl, VirtualAddress, Length);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IoBuildPartialMdl(SourceMdl, TargetMdl, VirtualAddress, Length);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtMdlPrepareForReuse(
    PEMCPAL_MDL Mdl)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_dcall_buffer_recycle(Mdl);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    MmPrepareMdlForReuse(Mdl);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtMdlBuildForNonPagedMemory(
    PEMCPAL_MDL Mdl)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_dcall_buffer_set_nonpaged(Mdl);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    MmBuildMdlForNonPagedPool(Mdl);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtMdlProbeAndLockPages(
    PEMCPAL_MDL Mdl,
    EMCPAL_IRP_KPROCESSOR_MODE AccessMode,
    EMCPAL_IRP_LOCK_OPERATION Operation)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_status_e status;
    status = csx_p_dcall_buffer_pin(Mdl, AccessMode == EmcpalUserMode, Operation == EmcpalIoReadAccess);
    CSX_ASSERT_SUCCESS_H_RDC(status);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    MmProbeAndLockPages(Mdl, AccessMode, Operation);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtMdlUnlockPages(
    PEMCPAL_MDL Mdl)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_status_e status;
    status = csx_p_dcall_buffer_unpin(Mdl);
    CSX_ASSERT_SUCCESS_H_RDC(status);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    MmUnlockPages(Mdl);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PVOID
EmcpalExtMdlMapLockedPages(
    PEMCPAL_MDL Mdl,
    EMCPAL_IRP_KPROCESSOR_MODE AccessMode,
    EMCPAL_IRP_MEMORY_CACHING_TYPE CacheType,
    PVOID BaseAddress,
    ULONG BugCheckOnFailure,
    EMCPAL_IRP_MM_PAGE_PRIORITY Priority)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_status_e status;
    csx_pvoid_t buffer_base_mapped;
    CSX_UNREFERENCED_PARAMETER(Priority);
    CSX_ASSERT_H_RDC(CSX_IS_NULL(BaseAddress)); /* SAFEKILL */
    CSX_ASSERT_H_RDC(CSX_IS_FALSE(BugCheckOnFailure));  /* SAFEKILL */
    //CSX_ASSERT_S_RDC(CacheType == EmcpalMmCached);    /* SAFEKILL */
    status = csx_p_dcall_buffer_map(Mdl, AccessMode == EmcpalUserMode, &buffer_base_mapped);
    CSX_ASSERT_SUCCESS_H_RDC(status);
    return buffer_base_mapped;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return MmMapLockedPagesSpecifyCache(Mdl, AccessMode, CacheType, BaseAddress, BugCheckOnFailure, Priority);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtMdlUnmapLockedPages(
    PVOID BaseAddress,
    PEMCPAL_MDL Mdl)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_dcall_buffer_unmap(Mdl, BaseAddress);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    MmUnmapLockedPages(BaseAddress, Mdl);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE SIZE_T
EmcpalExtMdlSizeCalculate(
    PVOID VirtualAddress,
    SIZE_T Length)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_dcall_buffer_get_size_needed(VirtualAddress, Length);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return MmSizeOfMdl(VirtualAddress, Length);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE ULONG
EmcpalExtMdlByteOffsetValGet(
    PEMCPAL_MDL Mdl)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_dcall_buffer_get_buffer_offset(Mdl);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return MmGetMdlByteOffset(Mdl);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE ULONG
EmcpalExtMdlByteCountValGet(
    PEMCPAL_MDL Mdl)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return (ULONG)csx_p_dcall_buffer_get_buffer_size(Mdl);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return MmGetMdlByteCount(Mdl);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PVOID
EmcpalExtMdlVirtualAddressGet(
    PEMCPAL_MDL Mdl)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_dcall_buffer_get_buffer_base_given_raw(Mdl);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return MmGetMdlVirtualAddress(Mdl);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PVOID
EmcpalExtMdlGetSystemAddressStd(
    PEMCPAL_MDL Mdl)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_dcall_buffer_get_mapped_address(Mdl);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return MmGetSystemAddressForMdl(Mdl);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PVOID
EmcpalExtMdlGetSystemAddressSafe(
    PEMCPAL_MDL Mdl,
    EMCPAL_IRP_MM_PAGE_PRIORITY Priority)
{
#ifdef EMCPAL_USE_CSX_DCALL
    CSX_UNREFERENCED_PARAMETER(Priority);
    return csx_p_dcall_buffer_get_mapped_address(Mdl);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return MmGetSystemAddressForMdlSafe(Mdl, Priority);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE EMCPAL_STATUS
EmcpalExtMdlProtectSystemAddress(
    PEMCPAL_MDL Mdl,
    ULONG NewProtect)
{
#ifdef EMCPAL_USE_CSX_DCALL
/* SAFEBUG - no sane users of this */
    CSX_BREAK();
    CSX_UNREFERENCED_PARAMETER(Mdl);
    CSX_UNREFERENCED_PARAMETER(NewProtect);
/* SAFEBUG - no sane users of this */
    return EMCPAL_STATUS_NOT_IMPLEMENTED;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return MmProtectMdlSystemAddress(Mdl, NewProtect);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PEMCPAL_MDL
EmcpalExtMdlNextGet(
    PEMCPAL_MDL Mdl)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_dcall_buffer_get_next(Mdl);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return Mdl->Next;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtMdlNextSet(
    PEMCPAL_MDL Mdl,
    PEMCPAL_MDL MdlNext)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_dcall_buffer_set_next(Mdl, MdlNext);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    Mdl->Next = MdlNext;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtMdlMappedAddressSet(
    PEMCPAL_MDL Mdl)
{
#ifdef EMCPAL_USE_CSX_DCALL
/* SAFEBUG - no sane users of this */
    CSX_BREAK();
    CSX_UNREFERENCED_PARAMETER(Mdl);
/* SAFEBUG - no sane users of this */
#else                                      /* EMCPAL_USE_CSX_DCALL */
    Mdl->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE ULONG
EmcpalExtMdlSizeGet(
    PEMCPAL_MDL Mdl)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_dcall_buffer_get_size_inuse(Mdl);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return Mdl->Size;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE ULONG
EmcpalExtMdlFlagsGet(
    PEMCPAL_MDL Mdl)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_dcall_buffer_get_buffer_flags(Mdl);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return Mdl->MdlFlags;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE ULONG
EmcpalExtMdlByteCountGet(
    PEMCPAL_MDL Mdl)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return (ULONG)csx_p_dcall_buffer_get_buffer_size(Mdl);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return Mdl->ByteCount;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE ULONG
EmcpalExtMdlByteOffsetGet(
    PEMCPAL_MDL Mdl)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_dcall_buffer_get_buffer_offset(Mdl);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return Mdl->ByteOffset;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PVOID
EmcpalExtMdlMappedSystemVaGet(
    PEMCPAL_MDL Mdl)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_dcall_buffer_get_buffer_base_mapped_raw(Mdl);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return Mdl->MappedSystemVa;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PVOID
EmcpalExtMdlStartVaGet(
    PEMCPAL_MDL Mdl)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_dcall_buffer_get_buffer_base_given_page_aligned(Mdl);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return Mdl->StartVa;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//=======================
// Cancel Safe IRP Queue Manipulation

#ifdef EMCPAL_KERNEL_STUFF_AVAILABLE

#ifdef EMCPAL_USE_CSX_DCALL
typedef csx_p_dcall_csq_body_t EMCPAL_IRP_CSQ, *PEMCPAL_IRP_CSQ;
typedef csx_p_dcall_csq_context_t EMCPAL_IRP_CSQ_IRP_CONTEXT, *PEMCPAL_IRP_CSQ_IRP_CONTEXT;
typedef csx_p_dcall_csq_insert_handler_t PEMCPAL_IRP_CSQ_INSERT_IRP;
typedef csx_p_dcall_csq_remove_handler_t PEMCPAL_IRP_CSQ_REMOVE_IRP;
typedef csx_p_dcall_csq_peek_handler_t PEMCPAL_IRP_CSQ_PEEK_NEXT_IRP;
typedef csx_p_dcall_csq_lock_acquire_handler_t PEMCPAL_IRP_CSQ_ACQUIRE_LOCK;
typedef csx_p_dcall_csq_lock_release_handler_t PEMCPAL_IRP_CSQ_RELEASE_LOCK;
typedef csx_p_dcall_csq_complete_cancelled_handler_t PEMCPAL_IRP_CSQ_COMPLETE_CANCELED_IRP;
#else                                      /* EMCPAL_USE_CSX_DCALL */
typedef IO_CSQ EMCPAL_IRP_CSQ, *PEMCPAL_IRP_CSQ;
typedef IO_CSQ_IRP_CONTEXT EMCPAL_IRP_CSQ_IRP_CONTEXT, *PEMCPAL_IRP_CSQ_IRP_CONTEXT;
typedef PIO_CSQ_INSERT_IRP PEMCPAL_IRP_CSQ_INSERT_IRP;
typedef PIO_CSQ_REMOVE_IRP PEMCPAL_IRP_CSQ_REMOVE_IRP;
typedef PIO_CSQ_PEEK_NEXT_IRP PEMCPAL_IRP_CSQ_PEEK_NEXT_IRP;
typedef PIO_CSQ_ACQUIRE_LOCK PEMCPAL_IRP_CSQ_ACQUIRE_LOCK;
typedef PIO_CSQ_RELEASE_LOCK PEMCPAL_IRP_CSQ_RELEASE_LOCK;
typedef PIO_CSQ_COMPLETE_CANCELED_IRP PEMCPAL_IRP_CSQ_COMPLETE_CANCELED_IRP;
#endif                                     /* EMCPAL_USE_CSX_DCALL */

CSX_STATIC_INLINE EMCPAL_STATUS
EmcpalExtIrpCsqInitialize(
    PEMCPAL_IRP_CSQ IrpCsq,
    PEMCPAL_IRP_CSQ_INSERT_IRP CsqInsertIrp,
    PEMCPAL_IRP_CSQ_REMOVE_IRP CsqRemoveIrp,
    PEMCPAL_IRP_CSQ_PEEK_NEXT_IRP CsqPeekNextIrp,
    PEMCPAL_IRP_CSQ_ACQUIRE_LOCK CsqAcquireLock,
    PEMCPAL_IRP_CSQ_RELEASE_LOCK CsqReleaseLock,
    PEMCPAL_IRP_CSQ_COMPLETE_CANCELED_IRP CsqCompleteCanceledIrp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_dcall_csq_create(IrpCsq, CsqInsertIrp, CsqRemoveIrp, CsqPeekNextIrp, CsqAcquireLock, CsqReleaseLock,
        CsqCompleteCanceledIrp);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IoCsqInitialize(IrpCsq, CsqInsertIrp, CsqRemoveIrp, CsqPeekNextIrp, CsqAcquireLock, CsqReleaseLock, CsqCompleteCanceledIrp);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtIrpCsqInsert(
    PEMCPAL_IRP_CSQ IrpCsq,
    PEMCPAL_IRP Irp,
    PEMCPAL_IRP_CSQ_IRP_CONTEXT Context)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_dcall_csq_insert_dcall(IrpCsq, Irp, Context);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IoCsqInsertIrp(IrpCsq, Irp, Context);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PEMCPAL_IRP
EmcpalExtIrpCsqRemove(
    PEMCPAL_IRP_CSQ IrpCsq,
    PEMCPAL_IRP_CSQ_IRP_CONTEXT Context)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_dcall_csq_remove_dcall(IrpCsq, Context);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IoCsqRemoveIrp(IrpCsq, Context);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE PEMCPAL_IRP
EmcpalExtIrpCsqRemoveNext(
    PEMCPAL_IRP_CSQ IrpCsq,
    PVOID PeekContext)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_dcall_csq_remove_dcall_next(IrpCsq, PeekContext);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IoCsqRemoveNextIrp(IrpCsq, PeekContext);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

#endif                                     /* EMCPAL_KERNEL_STUFF_AVAILABLE */

//=======================
// I/O REMOVE LOCK Manipulation

#ifdef EMCPAL_KERNEL_STUFF_AVAILABLE

#ifdef EMCPAL_USE_CSX_DCALL
typedef csx_p_device_remove_lock_body_t EMCPAL_IRP_IO_REMOVE_LOCK, *PEMCPAL_IRP_IO_REMOVE_LOCK;
#else                                      /* EMCPAL_USE_CSX_DCALL */
typedef IO_REMOVE_LOCK EMCPAL_IRP_IO_REMOVE_LOCK, *PEMCPAL_IRP_IO_REMOVE_LOCK;
#endif                                     /* EMCPAL_USE_CSX_DCALL */

CSX_STATIC_INLINE VOID
EmcpalExtDeviceRemoveLockInitialize(
    PEMCPAL_IRP_IO_REMOVE_LOCK RemoveLock,
    ULONG Tag,
    ULONG MaxLockedMinutes,
    ULONG HighWatermark)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_device_remove_lock_create(EMCPAL_MY_MODULE_CONTEXT(), RemoveLock);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IoInitializeRemoveLock(RemoveLock, Tag, MaxLockedMinutes, HighWatermark);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE EMCPAL_STATUS
EmcpalExtDeviceRemoveLockAcquire(
    PEMCPAL_IRP_IO_REMOVE_LOCK RemoveLock,
    PVOID Tag)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_device_remove_lock_acquire(RemoveLock);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IoAcquireRemoveLock(RemoveLock, Tag);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtDeviceRemoveLockRelease(
    PEMCPAL_IRP_IO_REMOVE_LOCK RemoveLock,
    PVOID Tag)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_device_remove_lock_release(RemoveLock);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IoReleaseRemoveLock(RemoveLock, Tag);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtDeviceRemoveLockReleaseAndWait(
    PEMCPAL_IRP_IO_REMOVE_LOCK RemoveLock,
    PVOID Tag)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_device_remove_lock_release_and_wait(RemoveLock);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IoReleaseRemoveLockAndWait(RemoveLock, Tag);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE VOID
EmcpalExtDeviceRemoveLockDestroy(
    PEMCPAL_IRP_IO_REMOVE_LOCK RemoveLock)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_device_remove_lock_destroy(RemoveLock);
#else                                      /* EMCPAL_USE_CSX_DCALL */
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

#endif                                     /* EMCPAL_KERNEL_STUFF_AVAILABLE */

//=======================

/*!
 * @} end group emcpal_irps
 * @} end file EmcPAL_Irp.h
 */
#endif                                     /* _EMCPAL_IRP_EXTENSIONS_H_ */
