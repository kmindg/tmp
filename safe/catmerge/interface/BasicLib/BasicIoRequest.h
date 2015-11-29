/************************************************************************
*
*  Copyright (c) 2002, 2005-2006, 2014 EMC Corporation.
*  All rights reserved.
*
*  ALL RIGHTS ARE RESERVED BY EMC CORPORATION.  ACCESS TO THIS
*  SOURCE CODE IS STRICTLY RESTRICTED UNDER CONTRACT. THIS CODE IS TO
*  BE KEPT STRICTLY CONFIDENTIAL.
*
*  UNAUTHORIZED MODIFICATIONS OF THIS FILE WILL VOID YOUR SUPPORT
*  CONTRACT WITH EMC CORPORATION.  IF SUCH MODIFICATIONS ARE FOR
*  THE PURPOSE OF CIRCUMVENTING LICENSING LIMITATIONS, LEGAL ACTION
*  MAY RESULT.
*
************************************************************************/

#ifndef __BasicIoRequest__
#define __BasicIoRequest__
//++
// File Name:
//      BasicIoRequest.h
//
// Contents:
//  Defines the BasicIoRequest class.
//
//
// Revision History:
//--



#include "ListOfCancelableIrps.h"
#include "EmcPAL_Irp.h"

class BasicIoRequest;
class BasicDeviceObject;

// Prototype for function that is used for handling the callback when an IoRequest
// completes some processing.
typedef EMCPAL_STATUS (*PBasicIoCompletionRoutine) (
    IN PVOID Unused,
    IN BasicIoRequest * Irp,
    IN PVOID Context
    );

// Prototype for function that is used for handling the callback when an IoRequest is
// cancelled.  Every cancel routine must contain a call to CancelRoutinePreamble().
typedef
VOID (*PBasicIoRequestCancelRoutine) (
                                      IN PVOID Unused,
                                      IN BasicIoRequest *Irp
                                      );

// Use COMPLETE_REQUEST_NO_INFO instead of "NULL" for "info" in CompleteRequest*
// Failure causes linux build warnings with newer compiler
#define COMPLETE_REQUEST_NO_INFO (ULONG_PTR)0

//  A BasicIoRequest provides an abstraction above the Windows Driver Model IRP. A
//  BasicIoRequest has an Originator, the driver which created it.  The Orginator fills in
//  "Next" parameters, and passes control ("CallDriver") to another driver. When that
//  driver has control of the IoRequest, the Next parameters of the Originator become the
//  "Current" parameters which the driver in control of the IoRequest can query. The
//  driver with control can set Next parameters also, and pass control on to another
//  driver, where those next parameters become current, and the old current become
//  completely hidden.
//
//  A driver which completes the IoRequest will return control back to the previous
//  driver, and previously hidden parameters will become current.
//
//  The orginiator of an IoRequest may cancel the IoRequest, so long as that originator
//  has ensured that the IoRequest cannot complete and be freed prior to the cancel
//  completing. Only the driver in control of the IoRequest that may call methods on
//  IoRequest, except for methods that explicitly state they allow others to call them.
//
//  "WDM:" in the description below describes the intent of the abstraction from the point
//  of view of the Windows Driver Model.
class BasicIoRequest : public EMCPAL_IRP
{
public:
    //Convenience typedefs
    enum MajorCodes
    {
        CODE_CREATE = EMCPAL_IRP_TYPE_CODE_CREATE,
        CODE_CLOSE = EMCPAL_IRP_TYPE_CODE_CLOSE,
        CODE_CLEANUP = EMCPAL_IRP_TYPE_CODE_CLEANUP,
        CODE_READ = EMCPAL_IRP_TYPE_CODE_READ,
        CODE_WRITE = EMCPAL_IRP_TYPE_CODE_WRITE,
        CODE_IOCTL = EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL,
        CODE_INT_IOCTL = EMCPAL_IRP_TYPE_CODE_INTERNAL_DEVICE_CONTROL
    };

    typedef EMCPAL_IRP_TRANSFER_LENGTH TRANSFER_LENGTH;
    typedef EMCPAL_IRP_TRANSFER_OFFSET TRANSFER_OFFSET;
    typedef EMCPAL_IRP_IOCTL_CODE IOCTL_CODE;
    typedef EMCPAL_IRP_TRANSFER_METHOD TRANSFER_METHOD;

    // Initialize the IO request.  An newly initialized request has no Current parameters.
    // @param NumberOfDriverLayers - The number of different versions of "next" parameters
    //                               this request can support.
    // WDM: IoInitializeIrp(this, IoSizeOfIrp(NumberOfDriverLayers),
    // NumberOfDriverLayers);
    // CSX: csx_p_dcall_initialize(this, ...)
    void InitializeRequest(UCHAR NumberOfDriverLayers);

    // Allocate and initialize the IO request.  An newly initialized request has no
    // Current parameters.
    // @param NumberOfDriverLayers - The number of different versions of "next" parameters
    //                               this request can support.
    // WDM: IoAllocateIrp(NumberOfDriverLayers);
    // CSX: csx_p_dcall_allocate();
    // Returns: The new request, or NULL if the memory allocation failed.
    static BasicIoRequest* Allocate(UCHAR StackSize);

    // Method to tell how much memory should be allocated for each request.  Used when
    // allocating a large chunk of memory and slicing it up into a pool of individual
    // requests
    // @param NumberOfDriverLayers - The number of different versions of "next" parameters
    //                               this request should support.
    // WDM: IoSizeOfIrp(NumberOfDriverLayers);
    // CSX: csx_p_dcall_size
    // Returns: The size.
    static unsigned GetRequestSize(UCHAR StackSize);

    // Inverse of "Allocate", but executed on the object itself rather than static.
    // WDM: IoFreeIrp(this)
    // CSX: csx_p_dcall_free(this)
    void Free();

    // If a previously initialized request had control returned to the current driver, 
    // this function may be called rather than InitializeRequest to prepare the request
    // for reuse.  InitializeRequest or ReinitializeRequest must be called before reusing
    // a completed request.  This function will be somewhat less expensive than a full
    // initialization.
    // WDM: 
    // CSX: csx_p_dcall_recycle();
    void ReinitializeRequest(EMCPAL_STATUS status);

    // Returns the status of the IoRequest reported on its last completion.  
    // WDM: this corresponds to IRP::IoStatus.Status.
    // CSX: dcall_body_t::status_ptr->dcall_status_actual
    EMCPAL_STATUS GetCompletionStatus() const;

    // Returns whether the Completion status indicated a successful outcome.
    // WDM: this corresponds to NT_SUCCEEDED(IRP::IoStatus.Status).
    // CSX: CSX_SUCCESS(dcall_body_t::status_ptr->dcall_status_actual).
    bool Successful() const;

    // Returns data associated with a specific request, e.g., the amount of data returned.
    // WDM: this corresponds to IRP::IoStatus.Information
    // CSX: dcall_body_t::status_ptr->io_size_actual
    ULONG_PTR GetCompletionInformation() const;

    // Returns TRUE if IoRequest::Cancel() or equivalent has been called for this
    // IoRequest.
    // WDM: Corresponds to Irp::Cancel
    // CSX: dcall_cancel_in_progress
    bool IsCancelPending() const;
    
    // Returns TRUE if the IRP is a type that will have to be matched to another
    // "sibling" IRP before processing can begin.
    // WDM:
    // CSX:
    bool IsPairableRequest() const;

    // Returns the type of IoRequest this receiver of the IoRequest is expected to
    // perform.
    // WDM: Corresponds to IoGetCurrentIrpStackLocation(Irp)->MajorFunction.
    // CSX: csx_p_dcall_get_current_level(this)->dcall_type_code
    UCHAR GetCurrentRequestType() const;

    // Returns the type of IoRequest this receiver of the IoRequest is expected to
    // perform.
    // WDM: Corresponds to IoGetCurrentIrpStackLocation(Irp)->MinorFunction.
    // CSX: csx_p_dcall_get_current_level(this)->dcall_sub_type_code
    UCHAR GetCurrentRequestSubtype() const;

    // Returns the type of IoRequest this receiver of the IoRequest is expected to
    // perform.
    // WDM: Corresponds to IoGetCurrentIrpStackLocation(Irp)->Flags.
    // CSX: csx_p_dcall_get_current_level(this)->level_flags
    EMCPAL_IRP_STACK_FLAGS GetCurrentFlags() const;

    // Only for IRP_MJ_DEVICE_CONTROL. Returns the typeof IOCTL.
    // WDM: IoGetCurrentIrpStackLocation()->Parameters.DeviceIoControl.IoControlCode
    // CSX: csx_p_dcall_get_current_level(this)->level_parameters.ioctl.ioctl_code
    IOCTL_CODE GetCurrentIoctlCode() const;

    // Only for IRP_MJ_DEVICE_CONTROL.
    // @param offset - return value displacement
    // @param minimum_size - the minimum amount of amount of buffer space that must have
    //                       been left after displacement. Pass zero to ignore.
    // Returns: 
    //   - a pointer to the output buffer where data is to be returned for the IOCTL.
    //   - NULL, if the size expected is larger than that provided.
    // WDM:
    // buffer = AssociatedIrp.SystemBuffer -or- MmGetSystemAddressForMdlSafe(MdlAddress)
    // -or- Type3InputBuffer
    // IoGetCurrentIrpStackLocation()->Parameters.DeviceIoControl.InputBufferLength >=
    // expectedSize?
    //          buffer: NULL;
    PVOID GetCurrentIoctlOutputBuffer(TRANSFER_LENGTH offset, TRANSFER_LENGTH minimum_size);
    PVOID GetCurrentIoctlOutputBuffer(TRANSFER_LENGTH minimum_size = 0)
    {
        return GetCurrentIoctlOutputBuffer(0, minimum_size);
    }

    // Only for IRP_MJ_DEVICE_CONTROL.
    // @param minimum_size - the minimum amount of amount of buffer space that must have
    //                       been provided by the sender.
    // Example: 
    //    SOME_OUT_STRUCT *out =
    //    irp->GetCurrentIoctlOutput<SOME_OUT_STRUCT>(out_minimum_size);
    // Returns: 
    //    - a pointer to the output buffer where typed data is to be returned for the
    // IOCTL.
    //    - NULL, if the size expected is larger than requested by the caller.
    template<class TData>
    TData* GetCurrentIoctlOutput(TRANSFER_LENGTH minimum_size)
    {
        if(PVOID rval = GetCurrentIoctlOutputBuffer(minimum_size)) {
            return reinterpret_cast<TData*>(rval);
        }
        return NULL;
    }

    // Only for IRP_MJ_DEVICE_CONTROL.
    // Example: 
    //    SOME_OUT_STRUCT *out = irp->GetCurrentIoctlOutput<SOME_OUT_STRUCT>();
    // Returns: a pointer to the output buffer where typed data is to be returned for the
    // IOCTL. Returns NULL, if the size sizeof(TData) is larger than requested by the
    // caller.
    template<class TData>
    TData* GetCurrentIoctlOutput()
    {
        return GetCurrentIoctlOutput<TData>(sizeof(TData));
    }


    // Only for IRP_MJ_DEVICE_CONTROL.
    // @param offset - return value displacement
    // @param minimum_size - the minimum amount of amount of buffer space that must have
    //                       been left after displacement. Pass zero to ignore.
    // Returns: a pointer to the input buffer where data is to be read for the IOCTL.
    // Returns NULL, if the size expected is larger than that provided.
    // WDM:
    // buffer = AssociatedIrp.SystemBuffer -or- MmGetSystemAddressForMdlSafe(MdlAddress)
    // -or- Type3InputBuffer
    // IoGetCurrentIrpStackLocation()->Parameters.DeviceIoControl.InputBufferLength >=
    // expectedSize?
    //          buffer: NULL;
    PVOID GetCurrentIoctlInputBuffer(TRANSFER_LENGTH offset, TRANSFER_LENGTH minimum_size);
    PVOID GetCurrentIoctlInputBuffer(TRANSFER_LENGTH minimum_size = 0)
    {
        return GetCurrentIoctlInputBuffer(0, minimum_size);
    }

     // Only for IRP_MJ_DEVICE_CONTROL.
    // @param minimum_size - the minimum amount of amount of buffer space that must have
    //                       been provided by the sender.
    // Example: 
    //    SOME_OUT_STRUCT *out =
    //    irp->GetCurrentIoctlInput<SOME_OUT_STRUCT>(in_minimum_size);
    // Returns: a pointer to the output buffer where typed data is to be returned for the
    // IOCTL. Returns NULL, if the size expected is larger than provided by the caller.
    template<class TData>
    TData* GetCurrentIoctlInput(TRANSFER_LENGTH minimum_size)
    {
        if(PVOID rval = GetCurrentIoctlInputBuffer(minimum_size)) {
            return reinterpret_cast<TData*>(rval);
        }
        return NULL;
    }

    // Only for IRP_MJ_DEVICE_CONTROL.
    // Example: 
    //    SOME_OUT_STRUCT *out = irp->GetCurrentIoctlInput<SOME_OUT_STRUCT>();
    // Returns: a pointer to the output buffer where typed data is to be returned for the
    // IOCTL. Returns 0, if the size sizeof(TData) is larger than provided by the caller.
    template<class TData>
    TData* GetCurrentIoctlInput()
    {
        return GetCurrentIoctlInput<TData>(sizeof(TData));
    }

    // Only for IRP_MJ_READ.
    // @param offset - return value displacement
    // @param minimum_size - the minimum amount of amount of buffer space that must have
    //                       been left after displacement. Pass zero to ignore.
    // Returns:  pointer to buffer where data to be placed or zero if size is
    //                       less than expected
    // WDM: 
    // buffer = AssociatedIrp.SystemBuffer -or- MmGetSystemAddressForMdlSafe(MdlAddress)
    // IoGetCurrentIrpStackLocation()->Parameters.Read.Length >= expectedSize?
    //          buffer: NULL;
    PVOID GetCurrentReadBuffer(TRANSFER_LENGTH offset, TRANSFER_LENGTH minimum_size);
    PVOID GetCurrentReadBuffer(TRANSFER_LENGTH minimum_size = 0)
    {
        return GetCurrentReadBuffer(0, minimum_size);
    }

    // Only for IRP_MJ_READ.
    // @param offset - return value displacement
    // @param minimum_size - the minimum amount of amount of buffer space that must have
    //                       been left after displacement. Pass zero to ignore.
    // Returns: pointer to buffer where data to be read from or zero if size is less than
    // expected
    // WDM: 
    // buffer = AssociatedIrp.SystemBuffer -or- MmGetSystemAddressForMdlSafe(MdlAddress)
    // IoGetCurrentIrpStackLocation()->Parameters.Read.Length >= expectedSize?
    //          buffer: NULL;
    PVOID GetCurrentWriteBuffer(TRANSFER_LENGTH offset, TRANSFER_LENGTH minimum_size);
    PVOID GetCurrentWriteBuffer(TRANSFER_LENGTH minimum_size = 0)
    {
        return GetCurrentWriteBuffer(0, minimum_size);
    }

    //Only for READ, WRITE, IOCTL
    // Returns transfer method for current stack frame
    EMCPAL_IRP_TRANSFER_METHOD GetCurrentTransferMethod();

    // Only for IRP_MJ_DEVICE_CONTROL.
    //
    // Returns the number of bytes provided by the Ioctl.
    // WDM: IoGetCurrentIrpStackLocation()->Parameters.DeviceIoControl.InputBufferLength
    // CSX: current_level->level_parameters.ioctl.io_in_size_requested
    TRANSFER_LENGTH GetCurrentIoctlInputLength() const;

    // Only for IRP_MJ_DEVICE_CONTROL. UNSAFE CurrentIoctlInputLength override. Used for
    // binary compatibility hacks.
    void OverrideCurrentIoctlInputLength(TRANSFER_LENGTH length);

    // Only for IRP_MJ_DEVICE_CONTROL.
    //
    // Returns the number of bytes expected by the Ioctl.
    // WDM: IoGetCurrentIrpStackLocation()->Parameters.DeviceIoControl.OutputBufferLength
    // CSX: current_level->level_parameters.ioctl.io_out_size_requested
    TRANSFER_LENGTH GetCurrentIoctlOutputLength() const;

    // For IRP_MJ_READ/WRITE and IRP_MJ_INTERNAL_DEVICE_CONTROL: FLARE_DCA_READ/WRITE
    // Returns the number of bytes that were requested to be transferred.
    // WDM: IoGetCurrentIrpStackLocation(this)->Parameters.Read/Write.Length
    // CSX: current_level->level_parameters.read/write.io_size_requested
    TRANSFER_LENGTH GetCurrentReadWriteLength();

    // For IRP_MJ_READ/WRITE and IRP_MJ_INTERNAL_DEVICE_CONTROL: FLARE_DCA_READ/WRITE
    // Returns the number of bytes that were requested to be transferred.
    // WDM: IoGetCurrentIrpStackLocation(this)->Parameters.Read/Write.Length
    TRANSFER_OFFSET GetCurrentReadWriteByteOffset();

    // For IRP_MJ_READ/WRITE and IRP_MJ_INTERNAL_DEVICE_CONTROL: FLARE_DCA_READ/WRITE
    //
    ULONG GetCurrentReadWriteKey();

    //WDM: current()->file_object;
    //CSX: current()->level_device_reference;
    PEMCPAL_FILE_OBJECT GetCurrentFileObject();

    //WDM: if(current()->file_object) {file_object->FsContext = context;}
    //CSX: csx_p_device_reference_set_private_data(current()->level_device_reference,
    //context);
    void SetCurrentFileObjectContext(PVOID context);

    //WDM: if(current()->file_object) {return file_object->FsContext;} return NULL;
    //CSX return
    //csx_p_device_reference_get_private_data(current()->level_device_reference);
    PVOID GetCurrentFileObjectContext();

    // Returns the device object that is serviving this IoRequest.
    // WDM: IoGetCurrentIrpStackLocation(this)->DeviceObject
    BasicDeviceObject* GetCurrentDeviceObject();
    PEMCPAL_DEVICE_OBJECT GetCurrentNativeDeviceObject();

    PVOID GetCurrentPDCATable();
    void SetNextPDCATable(PVOID arg);
    void ClearCurrentPDCATable();

    void SetDmIoctlContext(PVOID Context);
    PVOID GetDmIoctlContext();
    

    // WDM:  IoMarkIrpPending(this)
    void MarkPending();

    // Atomically sets the cancel routine for this IRP in preparation for holding the IRP.
    // @param routine - The routine the cancel handler should call.  Must not be NULL.
    // Returns whether the IRP should be held.
    //   * If true, either the IRP is now cancelable (the cancel routine is set), or
    //     it has already been canceled and its cancel routine has already started
    //     running.  In either case, the IRP should be held.  In the latter case,
    //     once the cancel routine obtains the lock protecting held IRPs, it will
    //     unhold the IRP and complete it with STATUS_CANCELLED.
    //   * If false, the caller should complete the IRP with STATUS_CANCELLED.
    bool SetCancelRoutine(PBasicIoRequestCancelRoutine routine)
    {
        return SetCancelRoutine(reinterpret_cast<PEMCPAL_IRP_CANCEL_FUNCTION>(routine));
    }

    bool SetCancelRoutine(PEMCPAL_IRP_CANCEL_FUNCTION routine);

    // Atomically clears the cancel routine in preparation for unholding the IRP.
    // Returns whether the IRP should be unheld.
    //  * If true, the IRP is now not cancelable and hence may be unheld and processed.
    //  * If false, the IRP's cancel routine has already started running.  The IRP
    //    should be left in the held state for the cancel routine to unhold and
    //    complete with STATUS_CANCELLED.
    bool ClearCancelRoutine();


    void SetNextCompletionRoutine(PBasicIoCompletionRoutine routine, PVOID context)
    {
        SetNextCompletionRoutine(reinterpret_cast<PEMCPAL_IRP_COMPLETION_ROUTINE>(routine), context);
    }

    void SetNextCompletionRoutine(PEMCPAL_IRP_COMPLETION_ROUTINE routine, PVOID context);

    // Set default completion status and info.
    // WDM: IRP::IoStatus.Status = STATUS_UNSUCCESSFUL; IRP::IoStatus.Information = NULL;
    void ClearCompletionStatus();

    // Set the completion status for this request, but do not complete it now.
    // @param status - the status to report for the command.
    // WDM: IRP::IoStatus.Status = status;
    void SetCompletionStatus(EMCPAL_STATUS status);

    // Set the completion information for this request, but do not complete it now.
    // @param info - 64 bits of information which is dependent on the command type.
    // WDM: IRP::IoStatus.Information = info;
    void SetCompletionInformation(ULONG_PTR info);

    // Set the completion status and information for this request, but do not complete it now.
    // @param status - the status to report for the command.
    // @param info - 64 bits of information which is dependent on the command type.
    // WDM: IRP::IoStatus.Status = status; IRP::IoStatus.Information = info;
    void SetCompletionStatus(EMCPAL_STATUS status, ULONG_PTR info);

    // Complete the request with the status and information specified. If this is
    // flagged as a paired request, we must also complete the sibling request with
    // the same status.
    // @param status - the status os to report for the command.
    // @param info - 64 bits of information which is dependent on the command type.
    // WDM: IRP::IoStatus.Status = status; IRP::IoStatus.Information = info; 
    //      IoCompleteRequest(this, IO_NO_INCREMENT);
    void CompleteRequest(EMCPAL_STATUS status, ULONG_PTR info = COMPLETE_REQUEST_NO_INFO);
    void CompleteRequestDisk(EMCPAL_STATUS status, ULONG_PTR info = COMPLETE_REQUEST_NO_INFO);


    // Complete the request, assuming the status and inforamtion fields are already filled
    // in.  This will not change Status or info fields set via SetCompletionStatus. If this
    // is flagged as a paired request, we must also complete the sibling request with
    // the same status.
    // WDM: IoCompleteRequest(this, IO_NO_INCREMENT);
    void CompleteRequestCompletionStatusAlreadySet();

    // Setup IRP with operation, parameters and buffers to send. SetupTop* calls are
    // intended for a use case where outgoing IRP is being initially constructed. I.e.
    // preparing to be sent to the "top" (according to WDM) device.
    // @param Major - BasicIoRequest::CODE_READ, BasicIoRequest::CODE_WRITE,
    //                EMCPAL_IRP_TYPE_CODE_READ or EMCPAL_IRP_TYPE_CODE_WRITE
    // @param Minor - should be zero
    // @param Flags
    // @param ByteOffset
    // @param Key
    // @param buffer - byte buffer to pass. Can be zero (both, with length)
    // @param length - requested length
    // @param method - Transfer method for operation. 
    //        NT:
    //            EMCPAL_IRP_TRANSFER_BUFFERED: AssocIrp.SystemBuffer = buffer
    //            EMCPAL_IRP_TRANSFER_DIRECT: MDL object will be built inside, remember to
    //            call CleanupDirectIo afterwards
    //        CSX:
    //            No one cares
    // \retval false - unable to build request
    bool SetupTopReadWrite(
        UCHAR Major, 
        UCHAR Minor, 
        EMCPAL_IRP_STACK_FLAGS Flags,
        TRANSFER_OFFSET ByteOffset,
        ULONG Key,
        PVOID buffer, 
        TRANSFER_LENGTH length, 
        TRANSFER_METHOD method);
    
    // Setup IRP with operation, parameters and length to send. The main point of this method is
    // to create an mdl for the provided buffer, but set the size of the overall operation to a
	// different value (also provided).
    // @param Major - EMCPAL_IRP_TYPE_CODE_INTERNAL_DEVICE_CONTROL
    // @param Minor - FLARE_DM_SOURCE
    // @param Flags
    // @param ByteOffset
    // @param Key
    // @param buffer - byte buffer to pass. Can be zero (both, with length)
    // @param ioSize - size of data movement
    // @param mdlLength - size of zero-detect buffer
    // @param method - Transfer method for operation. 
    //            EMCPAL_IRP_TRANSFER_BUFFERED: AssocIrp.SystemBuffer = buffer
    //            EMCPAL_IRP_TRANSFER_DIRECT: MDL object will be built inside, remember to
    //            call CleanupDirectIo afterwards
    //        CSX:
    //            EMCPAL_IRP_TRANSFER_BUFFERED: dcall_io_buffer = buffer
    //            EMCPAL_IRP_TRANSFER_DIRECT: MDL object will be built inside, remember to
    //            call CleanupDirectIo afterwards
    // \retval false - unable to build request
    bool SetupTopDmSrcIoctl(
        UCHAR Major, 
        UCHAR Minor, 
        EMCPAL_IRP_STACK_FLAGS Flags,
        TRANSFER_OFFSET ByteOffset,
        ULONG Key,
        PVOID buffer, 
        TRANSFER_LENGTH ioSize, 
        TRANSFER_LENGTH mdlLength, 
        TRANSFER_METHOD method);
    
    // User should call this after irp set with SetupTopReadWrite(..., METHOD_DIRECT); is
    // going to be reused or deallocated Note that this is obligation of IRP owner to
    // cleanup.
    // WDM: if(MdlAddress) {IoFreeMdl(MdlAddress); MdlAddress = NULL;}
    void CleanupDirectIo();

    // Set another new buffer pointer into the I/O Request
    // WDM: This is the Irp's "UserBuffer" member which apparently only has
    //  meaning in specific circumstances
    void SetNewBufferPtr(PVOID buffer);

    // Get the buffer pointer from the I/O Request
    // WDM: This is the Irp's "UserBuffer" member which apparently only has
    //  meaning in specific circumstances
    void * GetCurrentBufferPtr();

    // Setup IRP with operation and parameters to send. The main point of this method is
    // to hide passing "direct" buffers between IRPs. But others work as well. SetupTop*
    // calls are intended for a use case where outgoing IRP is being initially
    // constructed. I.e. preparing to be sent to the "top" (according to WDM) device.
    // @req ASSERT(forward_buffer_from->GetCurrentReadWriteLength() >= inbuffer_offset +
    // length);
    // @param Major - BasicIoRequest::CODE_READ, BasicIoRequest::CODE_WRITE,
    //                EMCPAL_IRP_TYPE_CODE_READ or EMCPAL_IRP_TYPE_CODE_WRITE
    // @param Minor - should be zero
    // @param Flags
    // @param ByteOffset
    // @param Key
    // @param forward_buffer_from
    // @param inbuffer_offset, length - sub-buffer range from original irp 
    // if(forward_buffer_from->GetCurrentTransferMethod() == EMCPAL_IRP_TRANSFER_DIRECT) 
    //        { /*Don't forget to call CleanupDirectIo before reusing this irp object*/ }
    // \retval false - unable to build request
    bool SetupTopReadWrite(
        UCHAR Major, 
        UCHAR Minor, 
        EMCPAL_IRP_STACK_FLAGS Flags,
        TRANSFER_OFFSET ByteOffset,
        ULONG Key,
        BasicIoRequest* forward_buffer_from, 
        TRANSFER_LENGTH inbuffer_offset,
        TRANSFER_LENGTH length);

    // Setup IRP with IOCTL operation and parameters to send where buffers are local
    // adressable C-structures SetupTop* calls are intended for a use case where outgoing
    // IRP is being initially constructed. I.e. preparing to be sent to the "top"
    // (according to WDM) device.
    // @param Major - BasicIoRequest::CODE_IOCTL, EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL
    // @param Minor - should be zero
    // @param Flags
    // @param code - IOCTL code
    // @param in - input structure or (char*)(NULL)  there is a problem passing void* to
    //             this template method
    // @param out - output structure or (char*)(NULL)
    template<class TIn, class TOut>
    void SetupTopIoctl(
        UCHAR Major, 
        UCHAR Minor, 
        EMCPAL_IRP_STACK_FLAGS Flags,
        IOCTL_CODE code,
        TIn* in,
        TOut* out)
    {
        if(in && out) {
            SetupTopIoctl(Major, Minor, Flags, code, in, sizeof(TIn), out, sizeof(TOut));
        }
        else if(in && !out) {
            SetupTopIoctl(Major, Minor, Flags, code, in, sizeof(TIn), NULL, NULL);
        }
        else if(!in && out) {
            SetupTopIoctl(Major, Minor, Flags, code, NULL, 0, out, sizeof(TOut));
        }
    }

    // Setup IRP with IOCTL operation and parameters to send where buffers are local
    // adressable C-structures SetupTop* calls are intended for a use case where outgoing
    // IRP is being initially constructed. I.e. preparing to be sent to the "top"
    // (according to WDM) device.
    // @param Major - BasicIoRequest::CODE_IOCTL, EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL
    // @param Minor - should be zero
    // @param Flags
    // @param code - IOCTL code
    // @param in - non-null pointer to the local structure to be sent as input buffer
    // @param out - non-null pointer to the local structure where output to be stored
    template<class TIn, class TOut>
    void SetupTopIoctl_InOut(
        UCHAR Major, 
        UCHAR Minor, 
        EMCPAL_IRP_STACK_FLAGS Flags,
        IOCTL_CODE code,
        TIn* in,
        TOut* out)
    {
        DBG_ASSERT(in && out);
        SetupTopIoctl(Major, Minor, Flags, code, in, sizeof(TIn), out, sizeof(TOut));
    }

    // Setup IRP with IOCTL operation and parameters to send where input buffer is local
    // adressable C-structure SetupTop* calls are intended for a use case where outgoing
    // IRP is being initially constructed. I.e. preparing to be sent to the "top"
    // (according to WDM) device.
    // @param Major - BasicIoRequest::CODE_IOCTL, EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL
    // @param Minor - should be zero
    // @param Flags
    // @param code - IOCTL code
    // @param in - non-null pointer to the local structure to be sent as input buffer
    template<class TIn>
    void SetupTopIoctl_In(
        UCHAR Major, 
        UCHAR Minor, 
        EMCPAL_IRP_STACK_FLAGS Flags,
        IOCTL_CODE code,
        TIn* in)
    {
        DBG_ASSERT(in);
        SetupTopIoctl(Major, Minor, Flags, code, in, sizeof(TIn), NULL, 0);
    }

    // Setup IRP with IOCTL operation and parameters to send where output buffer is local
    // adressable C-structure SetupTop* calls are intended for a use case where outgoing
    // IRP is being initially constructed. I.e. preparing to be sent to the "top"
    // (according to WDM) device.
    // @param Major - BasicIoRequest::CODE_IOCTL, EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL
    // @param Minor - should be zero
    // @param Flags
    // @param code - IOCTL code
    // @param out - non-null pointer to the local structure where output to be stored
    template<class TOut>
    void SetupTopIoctl_Out(
        UCHAR Major, 
        UCHAR Minor, 
        EMCPAL_IRP_STACK_FLAGS Flags,
        IOCTL_CODE code,
        TOut* out)
    {
        DBG_ASSERT(out);
        SetupTopIoctl(Major, Minor, Flags, code, NULL, 0, out, sizeof(TOut));
    }

    // Setup IRP with IOCTL operation and parameters to send where no buffers required
    // SetupTop* calls are intended for a use case where outgoing IRP is being initially
    // constructed. I.e. preparing to be sent to the "top" (according to WDM) device.
    // @param Major - BasicIoRequest::CODE_IOCTL, EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL
    // @param Minor - should be zero
    // @param Flags
    // @param code - IOCTL code
    void SetupTopIoctl(
        UCHAR Major, 
        UCHAR Minor, 
        EMCPAL_IRP_STACK_FLAGS Flags,
        IOCTL_CODE code)
    {
        SetupTopIoctl(Major, Minor, Flags, code, NULL, 0, NULL, 0);
    }

    // Setup IRP with IOCTL operation, parameters and buffers to send SetupTop* calls are
    // intended for a use case where outgoing IRP is being initially constructed. I.e.
    // preparing to be sent to the "top" (according to WDM) device.
    // @req code should specify METHOD_BUFFERED or METHOD_NEITHER as transfer method. It's
    // the only methods we support, now at least
    // @param Major - BasicIoRequest::CODE_IOCTL, EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL
    // @param Minor - should be zero
    // @param Flags
    // @param code - IOCTL code
    // @param input_buffer, input_length - both nulls, or both not
    // @param output_buffer, output_length - both nulls, or both not
    void SetupTopIoctl(
        UCHAR Major, 
        UCHAR Minor, 
        EMCPAL_IRP_STACK_FLAGS Flags,
        IOCTL_CODE code,
        PVOID input_buffer, 
        TRANSFER_LENGTH input_length, 
        PVOID output_buffer, 
        TRANSFER_LENGTH output_length);

    // Initially contruct an IRP for FLARE_NOTIFY_EXTENT_CHANGE.
    // @param ByteOffset - the byte offset at which the extent begins
    // @param Length - the length of the extent
    void SetupTopFlareNotifyExtentChange(
        TRANSFER_OFFSET ByteOffset,
        TRANSFER_LENGTH Length);

    // Setup next operation flags. Will overwrite flags previously set with Setup*(...)
    // call
    void SetNextFlags(EMCPAL_IRP_STACK_FLAGS flags);

    // Setup IRP with IOCTL operation and parameters without touching the buffers. Should
    // be used in filter drivers only,
    //   as original irp sender should call SetupTop*(...).
    void SetupNextIoctl(
        UCHAR Major, 
        UCHAR Minor, 
        EMCPAL_IRP_STACK_FLAGS Flags,
        IOCTL_CODE code,
        TRANSFER_LENGTH input_length,
        TRANSFER_LENGTH output_length);

    // Setup IRP with READ/WRITE operation and parameters without touching the buffers.
    // Should be used in filter drivers only,
    //   as original irp sender should call SetupTop*(...).
    void SetupNextReadWrite(
        UCHAR Major, 
        UCHAR Minor, 
        EMCPAL_IRP_STACK_FLAGS Flags,
        TRANSFER_OFFSET ByteOffset,
        ULONG Key,
        TRANSFER_LENGTH length);


    // Indicates:
    //  - Current parameters are copied to Next Parameters
    void CopyCurrentToNextAllowCompletionHandling();

    // Indicates:
    //  - About to invoke "CallDriver"
    //  - Current parameters are used to set Next Parameters
    //  - Caller will not touch current parameters after this call, or set a completion
    //    handler
    //  - control will not return to this driver.
    void CopyCurrentToNextNoCompletionHandling();

    operator EMCPAL_IRP & ()
    {
        return *this;
    }

    // All Cancel routines must contain a call to this. Should not be called elsewhere.
    // WDM:  IoReleaseCancelSpinLock(this->CancelIrql); 
    void CancelRoutinePreamble();

    // The creator of the IoRequest can request that the driver in control of this request
    // cancel the IRP soon.  Only the creator may call this.
    // WDM: IoCancelIrp()
    void CancelRequest();

    // Returns strings representing the kind and subkind of operation, e.g., for trace
    // messages.
    void IRPFunctionToString( PCHAR& major, PCHAR & minor);

    // These access field in the first stack location in the IRP.  These are useful on a
    // DCA callback to determine whether the IRP sou...
    PVOID OriginatorsCompletionRoutine();
    PVOID OriginatorsContext();

    // An IoRequest can store several pointers for its current owner.  These values are
    // lost across a "CallDriver". 
    void SavePointerWhileRequestOwner(PVOID pointer0)
    {
        OwnerContext()[0] = pointer0;
    }
    
    void SavePointerWhileRequestOwner(PVOID pointer0, PVOID pointer1)
    {
        OwnerContext()[0] = pointer0;
        OwnerContext()[1] = pointer1;
    }
    
    void SavePointerWhileRequestOwner(PVOID pointer0, PVOID pointer1, PVOID pointer2)
    {
        // OwnerContext()[2] is used for storing a pointer to the siblingIrp.
        // Keeping a debug assert here to make sure that drivers never try
        // to write directly to this field after a pair of IRPs is linked.
        // Note that it is assumed that whatever the request type is, it will
        // never be forwarded after it has been linked.
        DBG_ASSERT(!IsLinkedPair());
        OwnerContext()[0] = pointer0;
        OwnerContext()[1] = pointer1;
        OwnerContext()[2] = pointer2;
    }

    // Method to retrieve first pointer saved by SavePointerWhileRequestOwner
    PVOID RetrievePointerWhileRequestOwner()
    {
        return OwnerContext()[0];
    }

    // Method to retrieve 1-3 pointers saved by SavePointerWhileRequestOwner
    void RetrievePointerWhileRequestOwner(PVOID* pointer0, PVOID* pointer1 = NULL, PVOID* pointer2 = NULL)
    {
        if(pointer0) {
            *pointer0 = OwnerContext()[0];
        }
        if(pointer1) {
            *pointer1 = OwnerContext()[1];
        }
        if(pointer2) {
            *pointer2 = OwnerContext()[2];
        }
    }

    // Grabs pointer to the sibling request that was saved off previously.
    BasicIoRequest * GetSiblingOfPair();
    
    // Links this request to sibling so one can be accessed by the other.
    void CreatePairing(BasicIoRequest * sibling);
    
    // Breaks links between a pair of requests.
    BasicIoRequest * BreakPairing();

    // Returns TRUE if this IRP is currently linked to a sibling request.
    bool IsLinkedPair();

    // Returns:
    //    UserMode - if request was allocated and filed by UserMode process.
    //    KernelMode - if request was allocated and filed by KernelMode process
    //  WDM: this->RequestorMode
    char GetOriginatorProcessorMode();

    // for extended paranoid checks
    bool IsNextCompletionRoutineSet();

    PEMCPAL_IRP_LIST_ENTRY UserListEntry()
    {
#ifdef EMCPAL_USE_CSX_DCALL
        return &this->dcall_user_list_entry_1;
#else
        return &this->Tail.Overlay.ListEntry;
#endif
    }
private:
    // An IoRequest can store several pointers for its current owner.  These values are
    // lost across a "CallDriver". Not user accessible. There are three pointers available
    // for owner's use.
    PVOID* OwnerContext();

private:
    PEMCPAL_IO_STACK_LOCATION Current() const { 
#ifdef EMCPAL_USE_CSX_DCALL
        DBG_ASSERT(dcall_level_idx_current < dcall_levels_available); 
#else
#ifdef ALAMOSA_WINDOWS_ENV
        DBG_ASSERT(CurrentLocation > 0 && CurrentLocation <= StackCount); 
#else
        DBG_ASSERT(CurrentLocation >= 0 && CurrentLocation < StackCount); 
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - CSX uses a different scheme for CurrentLocation */
#endif
        return EmcpalIrpGetCurrentStackLocation((PEMCPAL_IRP)this); 
    }
    PEMCPAL_IO_STACK_LOCATION Next() const  { 
#ifdef EMCPAL_USE_CSX_DCALL
        DBG_ASSERT(dcall_level_idx_current >= 1 && dcall_level_idx_current < (dcall_levels_available+1));
#else
#ifdef ALAMOSA_WINDOWS_ENV
        DBG_ASSERT(CurrentLocation > 1 && CurrentLocation <= (StackCount+1));
#else
        DBG_ASSERT(CurrentLocation >= 1 && CurrentLocation < (StackCount+1));
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - CSX uses a different scheme for CurrentLocation */
#endif
        return EmcpalIrpGetNextStackLocation((PEMCPAL_IRP)this); 
    }

};

typedef BasicIoRequest * PBasicIoRequest;

class InterlockedListOfBasicIoRequests {
public:
    bool Enqueue(PBasicIoRequest Irp);
    void Requeue(PBasicIoRequest Irp);
    PBasicIoRequest  Dequeue();

private:
    InterlockedListOfIRPs   mList;
};

class ListOfBasicIoRequest {
public:
    PBasicIoRequest RemoveHead();

    bool IsEmpty() const;

    void AddTail(PBasicIoRequest Irp);

    void AddHead(PBasicIoRequest Irp);

    bool Remove(PBasicIoRequest Irp);

    void Move(ListOfBasicIoRequest & from) {
        for(;;) {
            BasicIoRequest * irp = from.RemoveHead();
            if(!irp) break;
            AddTail(irp);
        }
    }
private:
    ListOfIRPs  mList;
};

// A ListOfCancelableBasicIoRequests is a ListOfBasicIoRequests that allows a cancel
// handler to be specified which will allow the IRP to be canceled while it is on this
// list.
//
// The user is required to single thread all calls to an instance of this class, typically
// by making all calls with a lock held. (see also LockedListOfCancelableBasicIoRequest.)
//
// The cancel handler will only be called if the IRP is on this list, and it must do the
// following:
//
// - Release the cancel spin lock
// - Use the IRP and/or DeviceObject find the relevant instance of this class.
// - Acquire the lock that protects the particular instance of the queue
// - Call ListOfCancelableIrps::CancelHandler on with the IRP.    
// - Release the lock
// - Complete the IRP with STATUS_CANCELLED
//
// ListOfCancelableIrps::CancelHandler MUST be called on the correct queue for the IRP, or
// the IRP will be lost. ListOfCancelableIrps::CancelHandler can also be called on the
// wrong queue, so one cancel handler is try multiple queues, or there can be a separate
// cancel handler for multiple types of queues.
class ListOfCancelableBasicIoRequests : public ListOfCancelableIrps {
public:
    PBasicIoRequest RemoveHead();

    bool Remove(PBasicIoRequest Irp);

    // Add an IRP to the head of the list
    // @param Irp - the IRP to add
    // @param cancelRoutine - the routine which will be used in the IoSetCancelRoutine()
    //                        call, see requirements above. Must not be NULL.
    //
    // Returns true if the IRP was added to the list, false if the IRP was not added
    // because the Irp->Cancel was already set (IoStatus is set to STATUS_CANCELLED).
    bool AddHead(PBasicIoRequest Irp, PBasicIoRequestCancelRoutine cancelRoutine);

    // Add an IRP to the tail of the list
    // @param Irp - the IRP to add
    // @param cancelRoutine - the routine which will be used in the IoSetCancelRoutine()
    //                        call, see requirements above. Must not be NULL.
    //
    // Returns true if the IRP was added to the list, false if the IRP was not added
    // because the Irp->Cancel was already set (IoStatus is set to STATUS_CANCELLED).
    bool AddTail(PBasicIoRequest Irp, PBasicIoRequestCancelRoutine cancelRoutine) ;

    bool CancelIrp(PBasicIoRequest Irp);
};



inline void CompleteIrp(PEMCPAL_IRP irp, EMCPAL_STATUS status, ULONG_PTR info = 0)
{
    ((PBasicIoRequest) irp)->CompleteRequest(status, info);
}

#endif  // __BasicDeviceObject__

