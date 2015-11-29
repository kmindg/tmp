/***********************************************************************
*
*  Copyright (c) 2002, 2005-2015 EMC Corporation. 
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
#ifndef __IOIRPTracker__
#define __IOIRPTracker__

//++
// File Name:
//      IOIRPTracker.h
//
// Contents:
//      Declares the IOIRPTracker class. 
//

//
// Revision History:
//--

# include "BasicVolumeDriver/BasicVolume.h"  // need many defs in this header file
# include "flare_sgl.h"
# include "flare_ioctls.h"
# include "LIFOSinglyLinkedList.h"
# include "BasicLib/BasicIoRequest.h"
# include "FlareIoRequest.h"

// IOIRPTracker is a abstract base class intended to make handling of READ and WRITE IRPs
// easier, along with DEVICE_CONTROL IRPs.
//
// Subclasses of IOIRPTracker are used to have per IRP data either for IRPs that have been
// forwarded below, or when the per IRP data cannot be represented in the IRP itself. An
// IrpTracker's association with an IRP is limited to the tie the IRP enters the driver
// and when the driver calls IoCompleteRequest().
//
// This base class understands both the DCA and standard IO IRP protocols, and how to
// transfer data to/from the orginator of the IRP.
//
// A IOIRPTracker is associated with an IRP when its Initialize() function is called.
// Initialize() classifies the IRP into {MJ,DCA}_{READ,WRITE}, DEVICE_CONTROL, 
// INTERNAL_DEVICE_CONTROL (excluding DCA READ/WRITE}, or OTHER, and sets this
// in the field mIrpType.  Initialize also sets mCurrentByte to be the byte offset
// of the start of the {MJ,DCA}_{READ,WRITE}, and mBytesLeft to the number of bytes
// to read/write.  For other types of IRPs, mBytesLeft and mCurrentByte are set to zero.
// Initialize() also initializes the other fields in IOIRPTracker.
//
// A Read or Write write request will require 1 or more data transfers. Data transfers
// must always be requested in order. When a Read or Write has a buffer and is ready to
// transfer data, that buffer is described to the tracker by calling the
// AddRangeToTransfer() function for each contiguous piece of the buffer. If the buffer is
// non-contiguous, multiple calls will be required.
//
// After the buffer has been described, the transfer must be started by calling
// StartFrontEndTransfer().  Once the transfer is complete, the virtual function
// FrontEndTransferComplete() will be called.  For DCA operations, the data movement is
// initiated by the call to StartFrontEndTransfer().  For MJ_{READ,WRITE} the data is
// actually copied during the AddRangeToTransfer() call, and StartFrontEndTransfer()
// immediately calls back to FrontEndTransferComplete(), but the interface functions the
// same for both DCA and non-DCA transfers.
//
// AddRangeToTransfer() must always succeed on the first call of a new transfer, but it
// may fail on subsequent calls, indicating that the client should start a transfer with
// the previously added buffers, then retry the range that failed, etc.
//
// Overlapped front end transfers are not supported. Once StartFrontEndTransfer()  has
// been called, AddRangeToTransfer() and StartFrontEndTransfer() must not be called again
// until FrontEndTransferComplete() is called.
//
class IOIRPTracker {
public:

    // Defines the type of IRP that is being tracked.
    enum IRPType 
    { 
        // Used to catch zeroed values.
        InvalidIrpType,

        // Tracker for IRP_MJ_READ
        MJ_READ,

        // Tracker for IRP_INTERNAL_DEVICE_CONTROL - DCA_READ
        DCA_READ, 

        // Tracker for IRP_INTERNAL_DEVICE_CONTROL - SGL_READ
        SGL_READ, 

        // Tracker for IRP_INTERNAL_DEVICE_CONTROL - DCA_READ
        MJ_WRITE, 

        // Tracker for IRP_INTERNAL_DEVICE_CONTROL - DCA_WRITE
        DCA_WRITE,

        // Tracker for IRP_INTERNAL_DEVICE_CONTROL - SGL_WRITE
        SGL_WRITE,

        // Tracker for IRP_DEVICE_CONTROL 
        MJ_DEVICE_CONTROL, 

        // Tracker for IRP_DEVICE_CONTROL other than DCA_READ/DCA_WRITE
        MJ_INTERNAL_DEVICE_CONTROL,

        // Tracker for IRP_INTERNAL_DEVICE_CONTROL - DataLess Write (Notify Extent Change)
        DCA_DATALESS_WRITE,

        // Other IRP types
        MJ_OTHER
    } ;
    
    // Associate an IRP with this Tracker. Sets mIrpType, mCurrentByte, mBytesLeft based
    // on type of IRP.
    // @param Irp - the IRP to associate
    inline virtual void Initialize(PBasicIoRequest  Irp);
    

    //  Append this buffer range to the end of the next transfer. Move the current sector
    //  address forward, and reduce the count of bytes outstanding.
    //
    //  If the range contains more data than the client requested, it uses a smaller
    //  range.
    // @param paddr - the physical address of the next byte to add to the transfer.
    // @param offset - the logical offset from the start of the host request.
    // @param len - the number of bytes at paddr that should be transferred.
    // @param skipOverheadBytes - if true, after every 512 bytes at paddr, there are 8
    //                            bytes that must be skipped. len does not include these 8
    //                            bytes
    //
    // Return Value:
    //    true - range appended
    //    false - there was no room to append the range.
    bool                AddRangeToTransfer(ULONG offset, SGL_PADDR paddr, SGL_LENGTH len, 
        BOOLEAN skipOverheadBytes = false);

   
    // Append this buffer range to the end of the next transfer. Move the current sector
    // address forward, and reduce the count of bytes outstanding.
    //
    // If the original request is a DCA_READ or DCA_WRITE, the buffer locations are
    // recorded in the internal scatter/gather list, to be used when the transfer is
    // initiated via StartFrontEndTransfer().  If it is a MJ_READ or MJ_WRITE, where the
    // buffer is available, then the data is immediately moved (and StartFrontEndTransfer
    // will immediately complete).
    //
    // If the range contains more data than the client requested, it uses a smaller range.
    // 
    // @param offset - the logical offset from the start of the host request.
    // @param vaddr - the virtual address of the next byte to add to the transfer.
    // @param len - the number of bytes at paddr that should be transferred.
    // @param skipOverheadBytes - if true, after every 512 bytes at paddr, there are 8
    //                            bytes that must be skipped. len does not include these 8
    //                            bytes
    //
    // Return Value:
    //    true - range appended
    //    false - there was no room to append the range.
    bool                AddRangeToTransfer(ULONG offset, PVOID vaddr, SGL_LENGTH len,
        BOOLEAN skipOverheadBytes = false);
    

    // Initiate a transfer to/from the ranges specified via "AddRangeToTransfer".
    //

    // Note: the FrontEndTransferComplete function may be called BEFORE this function
    // returns.
    VOID                StartFrontEndTransfer();

    // Callback when a transfer that started actually finishes.
    virtual  VOID       FrontEndTransferComplete(EMCPAL_STATUS Status, ULONG bytes) = 0;

    // Return the number of bytes requested.
    ULONG               GetTotalRequestSize() const; 

    // Return the number of bytes beyound the point of the highest sucessful transfer.
    ULONGLONG           GetFirstByteOffset() const; 

    // Return the first sector beyond the point of the highest sucessful transfer.
    DiskSectorCount     GetCurrentSector() const { return DiskBytesToSectors(GetFirstByteOffset()+mMaxTransferByte); }

    // Return the number of bytes beyound the point of the highest sucessful transfer.
    ULONG               BytesLeft() const { return GetTotalRequestSize()-mMaxTransferByte; }

    // Return the number of bytes that were successfully transferred, assuming no holes.
    ULONG               TotalBytesTransferred() const { return mMaxTransferByte; }

    // Set the status, and extract the IRP from the tracker.
    //
    // @param status - the status to return in the IRP
    // @param info - the info value to return in the IRP
    //
    // Returns the IRP pointer that was in the tracker, and clears that pointer from the
    // tracker. A NULL return indicates that the IRP was no longer associated with the
    // tracker, i.e., it was previously aborted.
    PBasicIoRequest  PrepareToCompleteIrp(EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS, ULONG info = 0);

    // Returns the IRP associated with the tracker, or NULL if none.
    PBasicIoRequest                 GetIrp() const { return mIrp; }

    // Returns the type of IRP associated with the tracker.
    IRPType   GetIrpType() const { return mIrpType; }
    bool      IsReadIrp() const {return (mIrpType == SGL_READ || mIrpType == DCA_READ || mIrpType == MJ_READ);}
    bool      IsWriteIrp() const {return (mIrpType == SGL_WRITE || mIrpType == DCA_WRITE || mIrpType == MJ_WRITE);}
    bool      IsZeroFillIrp() const { return ((mIrpType == MJ_DEVICE_CONTROL) && (mIrp->GetCurrentIoctlCode() == IOCTL_FLARE_ZERO_FILL)); }

protected:

    // The IRP associated with this tracker.
    PBasicIoRequest                 mIrp;

    // The type of IRP we are tracking.
    IRPType             mIrpType;


    // The number of valid SGL entries. Initialize() sets this to zero,
    // AddRangeToTransfer() increases this count, transfer completion sets this to 0.
    ULONG               mNumSGLs;

    // The tracker has an internal SGL list of a fixed size to allow it to accumulate
    // non-contiguous I/O.  A single front end transfer cannot increase this size.
    enum Constants { MaxSGLEntries = 64 };

    // Tracker local storage for "MaxSGLEntries" SGL entries.
    // ENHANCE: substantial space could be saved in the tracker by having a very small
    // embedded SGL array, and attempting allocation of larger arrays only on transfers
    // that require them.  If such an allocation fails, then the transfer would be done
    // with the short list, forcing multiple transfers.  This would also allow transfers
    // with very large SGLs.
    SGL                 mSgl[MaxSGLEntries];

    // The highest byte successfully moved +1.
    ULONG               mMaxTransferByte;

    // Used as part of the DCA protocol when our driver has received IRPs.
    FLARE_DCA_ARG       mDcaArg;

private:
    // Internal, static callback function for when the upper driver completes a DCA
    // transfer that we requested on the front end. Will call FrontEndTransferComplete()
    // after doing some bookkeeping.
    // @param Status - whether the transfer succeeded or not
    // @param context - the IOIRPTracker * whose transfer just completed.
    static VOID         DcaTransferCompleteHandler(EMCPAL_STATUS Status, PVOID context);

};

inline VOID  IOIRPTracker::Initialize(PBasicIoRequest  Irp)
{
    mIrp = Irp;

    // Initialize some fields to standard values.
    mNumSGLs = 0;
    mDcaArg.Offset = 0;
    mDcaArg.SGList = &mSgl[0];
    mDcaArg.SglType = SGL_NORMAL;
    mDcaArg.TransferBytes = 0;
    mMaxTransferByte = 0;

    // Initialize other fields depending upon IRP types.

    switch (Irp->GetCurrentRequestType()) 
    {

    case EMCPAL_IRP_TYPE_CODE_READ: 
        mIrpType = MJ_READ;
        break;

    case EMCPAL_IRP_TYPE_CODE_WRITE: 
        mIrpType = MJ_WRITE;
        break;

    case EMCPAL_IRP_TYPE_CODE_INTERNAL_DEVICE_CONTROL:
        switch (Irp->GetCurrentRequestSubtype())
        {
        case FLARE_DCA_READ:  
            {
                mIrpType = DCA_READ;
                if(PDcaRequest(Irp)->GetCurrentDcaRequestReportVirtualAddressesInSGL()) {
                    mDcaArg.SglType |= SGL_VIRTUAL_ADDRESSES; 
                }
                break;
            }

        case FLARE_DCA_WRITE: 
            {
                mIrpType = DCA_WRITE;
                if(PDcaRequest(Irp)->GetCurrentDcaRequestReportVirtualAddressesInSGL()) {
                    mDcaArg.SglType |= SGL_VIRTUAL_ADDRESSES; 
                }
            break;
            }

        case FLARE_SGL_READ:  
            mIrpType = SGL_READ;
            break;

        case FLARE_SGL_WRITE: 
            mIrpType = SGL_WRITE;
            break;

        case FLARE_NOTIFY_EXTENT_CHANGE: 
            {
                mIrpType = DCA_DATALESS_WRITE;

            break;
            }

        default: 
            mIrpType = MJ_INTERNAL_DEVICE_CONTROL;
            break;
        }
        break;

    case EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL:
        mIrpType = MJ_DEVICE_CONTROL;   
        break;

    default:
        mIrpType = MJ_OTHER;    
        break;
    }
}

inline ULONG  IOIRPTracker::GetTotalRequestSize() const
{
    // Initialize other fields depending upon IRP types.
    switch (mIrpType) 
    {

    case DCA_READ:
    case MJ_READ: 
    case SGL_READ:
        return mIrp->GetCurrentReadWriteLength();

    case DCA_WRITE:
    case MJ_WRITE: 
    case SGL_WRITE:
    case DCA_DATALESS_WRITE:
        return mIrp->GetCurrentReadWriteLength();

    default: 
        return 0;

    }
}

inline ULONGLONG  IOIRPTracker::GetFirstByteOffset() const
{
    // Initialize other fields depending upon IRP types.

    switch (mIrpType) 
    {

    case DCA_READ:
    case MJ_READ: 
    case SGL_READ:
        return mIrp->GetCurrentReadWriteByteOffset();

    case DCA_WRITE:
    case MJ_WRITE: 
    case SGL_WRITE:
    case DCA_DATALESS_WRITE:
        return mIrp->GetCurrentReadWriteByteOffset();

    default: 
        return 0;

    }
}

inline PBasicIoRequest  IOIRPTracker::PrepareToCompleteIrp(EMCPAL_STATUS status, ULONG info)
{
    PBasicIoRequest     Irp = mIrp;

    if(!Irp) { 
        // IRP was probably aborted...
        return NULL;
    }

    mIrp = NULL;

    Irp->SetCompletionStatus(status, info);


    return Irp;

    // Caller is expected to call:   IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

#endif  // __IOIRPTracker__

