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

#ifndef __FlareIoRequest_h__
#define __FlareIoRequest_h__

#include "BasicLib/BasicIoRequest.h"
#include "flare_ioctls.h"

//This class provides FLARE_SGL_(READ|WRITE) use case on top of BasicIoRequest
class SglRequest : public BasicIoRequest
{
public:
    // Set the parameters to pass to the next driver for read/write operations
    // @param Minor - {FLARE_SGL_READ, FLARE_SGL_WRITE }
    // @param Flags - WDM: IO_STACK_LOCATION::Flags
    // @param ByteOffset - WDM: IO_STACK_LOCATION::Parameters.Read/Write.ByteOffset
    // @param TransferLen - WDM: IO_STACK_LOCATION::Parameters.Read/Write.TransferLen
    // @param SglInfo - WDM: IO_STACK_LOCATION::Parameters.Others.Argument4
    // @param Key - WDM: IO_STACK_LOCATION::Parameters.Read/Write.Key
    inline void SetupNextSGLReadWrite(
        unsigned char Minor, 
        EMCPAL_IRP_STACK_FLAGS Flags, 
        TRANSFER_OFFSET ByteOffset, 
        ULONG Key, 
        TRANSFER_LENGTH TransferLen, 
        PFLARE_SGL_INFO SglInfo)
    {
        DBG_ASSERT(Minor == FLARE_SGL_READ || Minor == FLARE_SGL_WRITE);
        SetupNextReadWrite(
            EMCPAL_IRP_TYPE_CODE_INTERNAL_DEVICE_CONTROL, Minor, Flags, 
            ByteOffset, Key, TransferLen);

        PEMCPAL_IO_STACK_LOCATION nextIrpStack = EmcpalIrpGetNextStackLocation(this);
        SET_PSGL_INFO_IN_IRP(nextIrpStack,(void*)SglInfo);

    }

    // Method to get a FLARE_SGL_INFO for FLARE_SGL_{READ,WRITE}
    // WDM: Argument4.
    inline PFLARE_SGL_INFO GetCurrentSglInfo()
    {
        DBG_ASSERT(GetCurrentRequestType() == CODE_INT_IOCTL && 
            (GetCurrentRequestSubtype() == FLARE_SGL_READ || GetCurrentRequestSubtype() == FLARE_SGL_WRITE)); //Invalid operation
        PEMCPAL_IO_STACK_LOCATION current = EmcpalIrpGetCurrentStackLocation(this);
        PFLARE_SGL_INFO pInfo = (PFLARE_SGL_INFO) GET_PSGL_INFO_FROM_IRP(current);
        return pInfo;
    }
};

//This class provides FLARE_DCA_(READ|WRITE|DATALESS_WRITE) use case on top of BasicIoRequest
class DcaRequest : public BasicIoRequest
{
public:
    // Only for IRP_MJ_INTERNAL_DEVICE_CONTROL: FLARE_DCA_READ/WRITE/FLARE_NOTIFY_EXTENT_CHANGE (Dataless DCA_WRITE).
    inline DCA_TABLE* GetCurrentDcaTable()
    {
        DBG_ASSERT(GetCurrentRequestType() == CODE_INT_IOCTL && 
            (GetCurrentRequestSubtype() == FLARE_DCA_READ || GetCurrentRequestSubtype() == FLARE_DCA_WRITE ||
             GetCurrentRequestSubtype() == FLARE_NOTIFY_EXTENT_CHANGE)); //Invalid operation
        // The DCA protocol passes the callback function in the "Argument4" field.
        PEMCPAL_IO_STACK_LOCATION current = EmcpalIrpGetCurrentStackLocation(this);
        struct DCA_TABLE * dca = (struct DCA_TABLE *) GET_PDCA_TABLE_FROM_IRP(current);
        return dca;
    }

    // @param dcaArg - the parameter structure which is passed to the originator.
    inline void DoCurrentDcaTransferCallback( struct FLARE_DCA_ARG * dcaArg)
    {
        DCA_TABLE* dca = GetCurrentDcaTable();
        PDCA_XFER_FUNCTION callback = dca->XferFunction;

        // Really start the transfer.
        (*callback)(this, dcaArg);
    }

    // Returns TRUE if the DCA request wants an SGL with virtual address, not physical.
    inline bool GetCurrentDcaRequestReportVirtualAddressesInSGL()
    {
        DCA_TABLE* dca = GetCurrentDcaTable();
        return dca->ReportVirtualAddressesInSGL != FALSE;
    }

    // Returns TRUE if the DCA requestor indicates that it wants and SGL where each entry
    // covers more than a 512 bytes.
    inline ULONG GetCurrentDcaRequestAvoidSkipOverheadBytes()
    {
        struct DCA_TABLE* dca = GetCurrentDcaTable();
        return dca->AvoidSkipOverheadBytes;
    }

    // Set the value for DCA request.
    // @param NewValue - {ASOB_FALSE, ASOB_TRUE, ASOB_DONT_CARE}
    inline void SetCurrentDcaRequestAvoidSkipOverheadBytes(ULONG NewValue)
    {
        struct DCA_TABLE* dca = GetCurrentDcaTable();
        dca->AvoidSkipOverheadBytes = NewValue;
    }

    // Set the parameters to pass to the next driver for read/write(-like) operations
    // @param Minor - {FLARE_DCA_READ, FLARE_DCA_WRITE, FLARE_NOTIFY_EXTENT_CHANGE}
    // @param Flags - WDM: IO_STACK_LOCATION::Flags
    // @param ByteOffset - WDM: IO_STACK_LOCATION::Parameters.Read/Write.ByteOffset
    // @param TransferLen - WDM: IO_STACK_LOCATION::Parameters.Read/Write.TransferLen
    // @param TransferTable - WDM: IO_STACK_LOCATION::Parameters.Others.Argument4
    // @param Key - WDM: IO_STACK_LOCATION::Parameters.Read/Write.Key
    inline void SetupNextDCAReadWrite(
        unsigned char Minor, 
        EMCPAL_IRP_STACK_FLAGS Flags, 
        TRANSFER_OFFSET ByteOffset, 
        ULONG Key, 
        TRANSFER_LENGTH TransferLen, 
        PDCA_TABLE TransferTable)
    {
        DBG_ASSERT(Minor == FLARE_DCA_READ || Minor == FLARE_DCA_WRITE || Minor == FLARE_NOTIFY_EXTENT_CHANGE);
        SetupNextReadWrite(
            EMCPAL_IRP_TYPE_CODE_INTERNAL_DEVICE_CONTROL, Minor, Flags, 
            ByteOffset, Key, TransferLen);

        PEMCPAL_IO_STACK_LOCATION nextIrpStack = EmcpalIrpGetNextStackLocation(this);
        SET_PDCA_TABLE_IN_IRP(nextIrpStack,(void*)TransferTable);
    }

    // These access field in the first stack location in the IRP.  These are useful on a
    // DCA callback to determine whether the IRP sou...
    inline PEMCPAL_IRP_COMPLETION_ROUTINE OriginatorsCompletionRoutine()
    {
        PEMCPAL_IO_STACK_LOCATION irpStack = EmcpalIrpGetFirstIrpStackLocation(this);
        return EmcpalIrpStackGetCompletionRoutine(irpStack);
    }
    inline void* OriginatorsContext()
    {
        PEMCPAL_IO_STACK_LOCATION irpStack = EmcpalIrpGetFirstIrpStackLocation(this);
        return EmcpalIrpStackGetCompletionContext(irpStack);
    }
};

typedef DcaRequest * PDcaRequest;
typedef SglRequest * PSglRequest;

#endif //#ifndef __FlareIoRequest_h__
