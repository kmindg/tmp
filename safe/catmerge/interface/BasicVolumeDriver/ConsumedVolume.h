/************************************************************************
*
*  Copyright (c) 2002, 2005-2010, 2014 EMC Corporation.
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

#ifndef __ConsumedVolume__
#define __ConsumedVolume__

//++
// File Name:
//      ConsumedVolume.h
//
// Contents:
//      Defines the ConsumedVolume class.
//
// Revision History:
//--


# include "K10SpinLock.h"
# include "K10AnsiString.h"
# include "BasicVolumeDriver/BasicVolume.h"
# include "flare_ioctls.h"

# include "BasicLib/BasicBinarySemaphore.h"
# include "BasicLib/BasicThreadWaitEvent.h"
# include "BasicLib/BasicConsumedDevice.h"

// The ConsumedVolume class allows a WDM device object to be attached to (opened), and
// IRPs to be sent to that device object.
class ConsumedVolume: public BasicConsumedDevice
{
public:
    // Constructor.  
    ConsumedVolume(BasicObjectTracer* tracer);

    // Destructor.
    virtual ~ConsumedVolume();
    
    // Initialize access to the device.
    //
    // Must execute at PASSIVE_LEVEL
    //
    // @param CapabilitiesQuery - If open is successful, issue CAPABILITIES_QUERY if true,
    //                            otherwise mark as capabilities unknown.
    // @param RaidInfoQuery     - If open is successful, issue GET_RAID_INFO
    //                            if true, otherwise set stripe size to -1
    //
    // Returns success status if the device was opened, otherwise an error status.
    virtual EMCPAL_STATUS OpenConsumedVolume(bool CapabilitiesQuery = true,
                                        bool RaidInfoQuery = false);

    // Close the consumed device.
    //
    // Must execute at PASSIVE_LEVEL
    virtual VOID CloseConsumedVolume();


    // Take an IRP whose current stack location is for the current driver, and forward it
    // to the consumed device (via IoCallDriver()), with exactly the same parameters, and
    // no completion routine in this driver.
    //
    // @param Irp - the IRP to forward.
    virtual VOID ForwardIrp(PBasicIoRequest  Irp);

    // Take an IRP whose current stack location is for the current driver, and forward it
    // to the consumed device, with exactly the same parameters.  Control returns to this
    // driver.
    //
    // @param Irp - the IRP to forward.
    // @param completionRoutine - the routine to call when the lower driver completes the
    //                            IRP.
    // @param context - the parameter to give to the completion routine.
    virtual VOID ForwardIrp(PBasicIoRequest  Irp, PBasicIoCompletionRoutine completionRoutine, PVOID context);



    // Send an IRP_MJ_READ IRP to the consumed device.  The MDL for the buffer must
    // already be set up in the IRP.
    //
    // @param Irp - the IRP to send.
    // @param Length - the number of sectors to read.
    // @param Offset - the sector offset within the device.
    // @param Key - the value for IO_STACK_LOCATION::Parameters.Read.Key.
    // @param completionRoutine - the routine to call when the lower driver completes the
    //                            IRP.
    // @param context - the parameter to give to the completion routine.
    // @param IoStackFlags - the Flags fields from the IO_STACK_LOCATION
    virtual VOID SendIRP_MJ_READ(PBasicIoRequest  Irp, EMCPAL_IRP_STACK_FLAGS IoStackFlags, DiskSectorCount Length, 
        DiskSectorCount Offset, ULONG Key = 0, PBasicIoCompletionRoutine completionRoutine = NULL, 
        PVOID context = NULL);

    // Send an IRP_MJ_WRITE IRP to the consumed device.  The MDL for the buffer must
    // already be set up in the IRP.
    //
    // @param Irp - the IRP to send.
    // @param Length - the number of sectors to write.
    // @param Offset - the sector offset within the device.
    // @param Key - the value for IO_STACK_LOCATION::Parameters.Write.Key.
    // @param completionRoutine - the routine to call when the lower driver completes the
    //                            IRP.
    // @param context - the parameter to give to the completion routine.
    // @param IoStackFlags - the Flags fields from the IO_STACK_LOCATION
    virtual VOID SendIRP_MJ_WRITE(PBasicIoRequest  Irp, EMCPAL_IRP_STACK_FLAGS IoStackFlags, DiskSectorCount Length, 
        DiskSectorCount Offset, ULONG Key = 0, PBasicIoCompletionRoutine completionRoutine = NULL, 
        PVOID context = NULL);

    // Send a DCA_READ IRP to the consumed device.  
    //
    // @param Irp - the IRP to send.
    // @param ByteCount - the number of bytes to read.
    // @param ByteOffset - the byte offset within the device.
    // @param transferTable - the routine and parameters associated with the callback to
    //                        transfer DCA data.
    // @param completionRoutine - the routine to call when the lower driver completes the
    //                            IRP.
    // @param context - the parameter to give to the completion routine.
    // @param IoStackFlags - the Flags fields from the IO_STACK_LOCATION
    virtual VOID SendDCA_READ(PBasicIoRequest  Irp, EMCPAL_IRP_STACK_FLAGS IoStackFlags, ULONG ByteCount, 
        ULONGLONG ByteOffset, PDCA_TABLE transferTable, PBasicIoCompletionRoutine completionRoutine = NULL,
        PVOID context = NULL);

    // Send a DCA_WRITE IRP to the consumed device.  
    //
    // @param Irp - the IRP to send.
    // @param ByteCount - the number of bytes to write.
    // @param ByteOffset - the byte offset within the device.
    // @param transferTable - the routine and parameters associated with the callback to
    //                        transfer DCA data.
    // @param completionRoutine - the routine to call when the lower driver completes the
    //                            IRP.
    // @param context - the parameter to give to the completion routine.
    // @param IoStackFlags - the Flags fields from the IO_STACK_LOCATION
    virtual VOID SendDCA_WRITE(PBasicIoRequest  Irp, EMCPAL_IRP_STACK_FLAGS IoStackFlags, ULONG ByteCount, ULONGLONG ByteOffset,
        PDCA_TABLE transferTable, PBasicIoCompletionRoutine completionRoutine = NULL,
        PVOID context = NULL);

    // Send a SGL_READ IRP to the consumed device.  
    //
    // @param Irp - the IRP to send.
    // @param ByteCount - the number of bytes to read.
    // @param ByteOffset - the byte offset within the device.
    // @param SglInfo - describe backend native SGL.
    // @param IoPriority - sets the priority of the request. Read requests will have medium
    //                     priority, Write-thru & forced flush request will have high priority
    //                     idle flush request will have low priority.
    // @param completionRoutine - the routine to call when the lower driver completes the
    //                            IRP.
    // @param context - the parameter to give to the completion routine.
    // @param IoStackFlags - the Flags fields from the IO_STACK_LOCATION
    virtual VOID SendSGL_READ(PBasicIoRequest  Irp, EMCPAL_IRP_STACK_FLAGS IoStackFlags, ULONG ByteCount, 
        ULONGLONG ByteOffset, PFLARE_SGL_INFO SglInfo, UINT_32 IoPriority,
        PBasicIoCompletionRoutine completionRoutine = NULL, PVOID context = NULL);

    // Send a SGL_WRITE IRP to the consumed device.  
    //
    // @param Irp - the IRP to send.
    // @param ByteCount - the number of bytes to write.
    // @param ByteOffset - the byte offset within the device.
    // @param SglInfo - describe backend native SGL.
    // @param IoPriority - sets the priority of the request. Read requests will have medium
    //                     priority, Write-thru & forced flush request will have high priority
    //                     idle flush request will have low priority.
    // @param completionRoutine - the routine to call when the lower driver completes the
    //                            IRP.
    // @param context - the parameter to give to the completion routine.
    // @param IoStackFlags - the Flags fields from the IO_STACK_LOCATION
    virtual VOID SendSGL_WRITE(PBasicIoRequest  Irp, EMCPAL_IRP_STACK_FLAGS IoStackFlags, ULONG ByteCount, ULONGLONG ByteOffset,
        PFLARE_SGL_INFO SglInfo, UINT_32 IoPriority, PBasicIoCompletionRoutine completionRoutine = NULL,
        PVOID context = NULL);

    // Send an IRP_MJ_DEVICE_CONTROL IRP to the consumed device.  
    //
    // @param Irp - the IRP to use for the send.  It must be initialized, but no fields
    //              need to be set.
    // @param ioControlCode - the IoControlCode to use.
    // @param buffer - the buffer to use for both the input and output data.  Prior to the
    //                 call, the buffer must contain the input data, if inputBufferLength
    //                 != 0. This buffer will be overwritten with up to outputBufferLength
    //                 bytes of data prior to the IRP completion.  Buffer may be NULL if
    //                 both lengths are 0.
    // @param inputBufferLength - the number of bytes in buffer that should be passed in
    //                            the IRP.
    // @param outputBufferLength - the maximum number of bytes that may be returned in
    //                             buffer.
    // @param completionRoutine - the routine to call when the IRP completes.
    // @param context - the second parameter to the completion routine.
    virtual VOID SendDeviceControl(PBasicIoRequest  Irp,  ULONG ioControlCode, 
        PVOID inputBuffer, ULONG inputBufferLength, 
        PVOID outputBuffer, ULONG outputBufferLength,
        PBasicIoCompletionRoutine completionRoutine = NULL, PVOID context = NULL);

    // Send an IRP to the consumed device, specifying a completion routine. The next
    // IO_STACK_LOCATION may be already set.
    //
    // @param Irp - the IRP to send.
    // @param completionRoutine - the routine to call when the lower driver completes the
    //                            IRP.
    // @param context - the parameter to give to the completion routine. IRP.  If NULL,
    //                  the existing values in the next stack location must be set.
    //
    // Returns the value returned by IoCallDriver().
    // NOTE: This routine is deprecated and should not be used going forward.
    //          This is kept in the code to support the MLU driver.
    virtual EMCPAL_STATUS SendIrp(PBasicIoRequest  Irp, PBasicIoCompletionRoutine completionRoutine = NULL, PVOID context = NULL);

    // Send an IRP to the consumed device, specifying a completion routine. The next
    // IO_STACK_LOCATION may be already set.
    //
    // @param Irp - the IRP to send.
    // @param completionRoutine - the routine to call when the lower driver completes the
    //                            IRP.
    // @param context - the parameter to give to the completion routine. IRP.  If NULL,
    //                  the existing values in the next stack location must be set.
    virtual VOID SendIoRequest(PBasicIoRequest  Irp, PBasicIoCompletionRoutine completionRoutine = NULL, PVOID context = NULL); 

    // Returns true if the consumed device understands the capabilities protocol, at least
    // enough to have done the handshake.  Should not be called if the consumed device is
    // not open.
    bool KnowsAboutDca() const { return mKnowsAboutDca; }

    // Returns true if the consumed device supports DCA_READ IRPs.  Should not be called
    // if the consumed device is not open.
    bool SupportsDcaRead() const { return mSupportsDcaRead; }

    // Returns true if the consumed device supports DCA_WRITE IRPs.  Should not be called
    // if the consumed device is not open.
    bool SupportsDcaWrite() const { return mSupportsDcaWrite; }

    // Returns true if the consumed device supports ZERO_FILL IOCTLs.  Should not be
    // called if the consumed device is not open.
    bool SupportsZeroFill() const { return mSupportsZeroFill; }

    // Returns true if the consumed device supports sparse allocation. Should not be
    // called if the consumed device is not open.
    bool SupportsSparseAllocation() const { return mSupportsSparseAllocation; }

    // Updates the internal capabilities returned by the "SupportsXxx()" accessor methods.
    void UpdateCapabilities(FLARE_CAPABILITIES_QUERY * pCapabilities); 

    // Returns the IDM LUN number (User logical unit number) 
    VOID SynchronousGetLunNumber(ULONG & LunNumber, ULONG & FluNumber);

    enum {
        // Value of mLunNumber when it is not valid.
        INVALID_LUN = (~0)
    };

    // Returns true if the LUN number is valid.
    bool IsValidLun(ULONG LunNumber) const { return (LunNumber != INVALID_LUN); }

    // Initializes fields in an IRP, preparing to send an IRP_MJ_DEVICE_CONTROL IOCTL. The
    // completion handler is setup so that DeviceControlCompletion() will be called on
    // completion, so the caller of this function must either:
    //   - override DeviceControlCompletion
    //   - overwrite the completion routine in the IRP using IoSetCompletionRoutine
    //
    // @param Irp - the IRP to use to send the IOCTL
    // @param IoControlCode - the specific device control to send.
    // @param buffer - the buffer to use for both sending and receiving.
    // @param inputLen - the number of bytes being passed in in *buffer
    // @param outputLen - the maixmum number of bytes that can be returned in buffer.
    VOID  PrepareDeviceControl(PBasicIoRequest  Irp, ULONG IoControlCode, PVOID buffer_in, 
        ULONG inputLen, PVOID buffer_out, ULONG outpuLlen);

    // Completion handler for all device control IRPs.  Base class version will panic. If
    // PrepareDeviceControl() is used, this function should be overridden.
    // @param Irp - the IRP that just completed. 
    virtual void DeviceControlCompletion(PBasicIoRequest  Irp);

    // Updates the mSupportsDcaRead to either TRUE or FALSE.
    // @param Supported - TRUE if DcaRead is supported, FALSE otherwise.
    void UpdateDcaReadLockHeld(bool Supported);

    // Updates the mSupportsDcaWrite to either TRUE or FALSE.
    // @param Supported - TRUE if DcaWrite is supported, FALSE otherwise.
    void UpdateDcaWriteLockHeld(bool Supported);

    // Returns the number of bytes per sector.
    ULONG GetBytesPerSector() const
    {
        return mRaidInfo.BytesPerSector;
    }

    // Returns number of disks in volume (including parity)
    ULONG GetDisksPerStripe() const
    {
        return mRaidInfo.DisksPerStripe;
    }

    // Returns the RAID stripe size of the volume
    ULONG GetSectorsPerStripe() const
    {
        return mRaidInfo.SectorsPerStripe;
    }

    // Returns stripe count for the volume.
    ULONG GetStripeCount() const 
    {
        return mRaidInfo.StripeCount;
    }
    
    // Returns the RAID elements per parity stripe. Parity rotates every N stripes.
    ULONG GetElementsPerParityStripe() const
    {
        return mRaidInfo.ElementsPerParityStripe;
    }
    
    // Returns the number of bytes per element.
    ULONG GetBytesPerElement() const
    {
        return mRaidInfo.BytesPerElement;
    }

    // Returns the maximum number of requests that the RG can handle. Returns one if
    // GET_RAID_INFO IOCTL hasn't been issued.
    ULONG GetMaxQueueDepth() const 
    { 
        return mRaidInfo.MaxQueueDepth; 
    }
    
    // Returns the WWN from the RAID Info. Returns zero if the GET_RAID_INFO IOCTL
    // hasn't been issued.
    K10_WWID GetWWN() const { return mRaidInfo.WorldWideName; }
    
    // Returns the FLU number for the consumed volume. Returns INVALID_LUN if the
    // GET_RAID_INFO IOCTL hasn't been issued.
    // NOTE: This method doesn't cause the GET_RAID_INFO IOCTL to be issued.
    ULONG GetFluNumber() const { return mRaidInfo.Flun; }
    
    // Returns the group ID for the consumed volume. Returns zero if the GET_RAID_INFO 
    // IOCTL hasn't been issued.
    // NOTE: This method doesn't cause the GET_RAID_INFO IOCTL to be issued.
    ULONG GetGroupId() const { return mRaidInfo.GroupId; }
    
    // Returns the rotational rate for the consumed volume. Returns zero if the  
    // GET_RAID_INFO IOCTL hasn't been issued.
    // NOTE: This method doesn't cause the GET_RAID_INFO IOCTL to be issued.
    USHORT GetRotationalRate() const { return mRaidInfo.RotationalRate; }

    // Returns pointer of the tracer object.
    BasicObjectTracer * GetTracer() const {
        return mTracer;
    }

private:

    // Get the Open/Close semaphore.  The calling thread may block, therefore it must be
    // at PASSIVE_LEVEL. The Open/Close semaphore is used to serialize volume open and
    // closes..
    VOID AcquireOpenCloseSemaphore();

    // Release the Open/Close semaphore.  
    VOID ReleaseOpenCloseSemaphore();

    // WDM IRP completion routine for IOCTLs issued to consumed volumes. Calls the member
    // function of the same name.
    //
    // @param DeviceObject - the lower level driver's device object.
    // @param Irp - the IRP that just completed. 
    // @param Context - RedirectorConsumedVolume *
    // Returns: STATUS_MORE_PROCESSING_REQUIRED
    static EMCPAL_STATUS StaticDeviceControlCompletion(IN PVOID Unused,
        IN PBasicIoRequest  pIrp,
        IN PVOID            pContext
        );

    // Do the CAPABILITIES_QUERY negotiation (after the device is open).
    void SynchronousCapabilitiesQuery();

    // Get the RAID info (after the device is open).
    EMCPAL_STATUS SynchronousGetRaidInfo();

    // Device understood IOCTL_FLARE_CAPABILITIES_QUERY.
    bool           mKnowsAboutDca; 

    // Device understood IOCTL_FLARE_CAPABILITIES_QUERY and supports DCA_WRITE.
    bool           mSupportsDcaWrite;

    // Device understood IOCTL_FLARE_CAPABILITIES_QUERY and supports DCA_READ.
    bool           mSupportsDcaRead;

    // Device understood IOCTL_FLARE_CAPABILITIES_QUERY and supports zero-fill.
    bool           mSupportsZeroFill;

    // Device understood IOCTL_FLARE_CAPABILITIES_QUERY and supports sparse allocations.
    bool           mSupportsSparseAllocation;

    // For serializing open/close
    BasicBinarySemaphore     mOpenCloseSemaphore;

    // Number of times the consumed volume has logically been opened.
    ULONG          mOpenCount;

    BasicObjectTracer*          mTracer;

    // Stores RAID info which we get when we send IOCTL_FLARE_GET_RAID_INFO.
    FLARE_RAID_INFO             mRaidInfo;      

    enum { 

        // How long to wait on the Open/Close semaphore before bug-checking? We want to
        // bug check on the wait event first, since it is probably closer to the root of
        // the problem.
        OpenCloseSemaphoreBugCheckSeconds = 4*60
    };

public:
    VOID ForwardIrp(PEMCPAL_IRP Irp, PEMCPAL_IRP_COMPLETION_ROUTINE completionRoutine, PVOID context) {
        ForwardIrp(static_cast<PBasicIoRequest>(Irp), reinterpret_cast<PBasicIoCompletionRoutine>(completionRoutine), context);
    }

    VOID ForwardIrp(PEMCPAL_IRP Irp) {
        ForwardIrp(static_cast<PBasicIoRequest>(Irp));
    }

    // Send an IRP_MJ_READ IRP to the consumed device.  The MDL for the buffer must
    // already be set up in the IRP.
    //
    // @param Irp - the IRP to send.
    // @param Length - the number of sectors to read.
    // @param Offset - the sector offset within the device.
    // @param Key - the value for IO_STACK_LOCATION::Parameters.Read.Key.
    // @param completionRoutine - the routine to call when the lower driver completes the
    //                            IRP.
    // @param context - the parameter to give to the completion routine.
    // @param IoStackFlags - the Flags fields from the IO_STACK_LOCATION
    //
    // Returns none
    void SendIRP_MJ_READ(PEMCPAL_IRP  Irp, EMCPAL_IRP_STACK_FLAGS IoStackFlags, DiskSectorCount Length, 
        DiskSectorCount Offset, ULONG Key = 0, PEMCPAL_IRP_COMPLETION_ROUTINE completionRoutine = NULL, 
        PVOID context = NULL)
    {
        SendIRP_MJ_READ(static_cast<PBasicIoRequest>(Irp), IoStackFlags, Length, Offset, Key, reinterpret_cast<PBasicIoCompletionRoutine>(completionRoutine), context);
    }

    EMCPAL_STATUS SendIrp(PEMCPAL_IRP  Irp, PEMCPAL_IRP_COMPLETION_ROUTINE completionRoutine = NULL, PVOID context = NULL)
    {
        return SendIrp(static_cast<PBasicIoRequest>(Irp), reinterpret_cast<PBasicIoCompletionRoutine>(completionRoutine), context);
    }

    PFLARE_RAID_INFO GetFlareRaidInfo ()
    {
        return (&mRaidInfo);
    }

};


#endif  // __ConsumedVolume__

