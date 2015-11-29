/*******************************************************************************
 * Copyright (C) EMC Corporation, 1997 - 2011
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

/*******************************************************************************
 * tcdshare.h
 *
 * This file defines the interface between a Target Enhanced Miniport, the
 * SCSI Target class driver (TCD), and the SCSI Target Disk Driver (TDD)
 *
 ******************************************************************************/

#ifndef _TCDSHARE_H_
#define _TCDSHARE_H_

// Mazur #include "scsi.h"
#include "EmcPAL_List.h"
//#include "storport.h"
#include "minipath.h"
#include "flare_sgl.h"
#include "SinglyLinkedList.h"
#include "LIFOSinglyLinkedList.h"
#include "InterlockedFIFOList.h"
#include "cpd_interface.h"
#include "cmm.h"

#include "EmcPAL_Irp.h"

//  CONTINUE CCB Flags - RO by miniport, RW by other drivers.
//  Modifies the transfer.
//      Legal Combinations:
//          CF_SCSI_STATUS_VALID|CF_SENSE_VALID
//          CF_SCSI_STATUS_VALID|CF_DATA_IN_VALID
//          CF_SCSI_STATUS_VALID
//          CF_DATA_OUT_VALID
//          CF_DATA_IN_VALID

typedef ULONG                   CCB_FLAGS;

#define CF_SCSI_STATUS_VALID    0x01L   // ContinueCcb->ScsiStatus is valid
#define CF_SENSE_VALID          0x02L   // ContinueCcb->ParentCcb->Sense is valid
#define CF_DATA_IN_VALID        0x04L   // Out from SCSI Target;  IN to Host
#define CF_DATA_OUT_VALID       0x08L   // In to SCSI Target;  OUT from Host
#define CF_DATA_PHASE           (CF_DATA_IN_VALID | CF_DATA_OUT_VALID)
#define CF_PHYS_SGL             0x10L   // SGL is in hardware HBA format
#define CF_SKIP_SECTOR_OVERHEAD_BYTES   \
                                0x20L   // Miniport skips metadata in SGL


//  CONTINUE CCB Status - Only set by the MiniPort or (sometimes) after the MiniPort
//               calls back into the TCD.  TxDs may not write to this.

typedef enum _CCB_STATUS
{

    CCBSTAT_SUCCESS = 0,  // Otherwise transfer failed

    // Set by miniports if xfer not attempted because
    // ParentCcb->Aborted is TRUE.
    CCBSTAT_ABORTED = 1,

    // The last transfer failed for this nexus, so this one
    // must also. No data was moved.
    CCBSTAT_PREVIOUS_XFER_FAILED =2,

    // The transfer failed while it was (or might have been)
    // in progress.
    CCBSTAT_XFER_ERROR = 3,
    
    // Could not get back on bus/loop.  The transfer 
    // did not take place.
    CCBSTAT_RESELECT_TIMEOUT = 4,

     // Parallel SCSI Transfer Errors
    CCBSTAT_INIT_DET_ERROR= 5,
    CCBSTAT_PROC_TERM = 6,
    CCBSTAT_CATASTROPHIC_ERROR = 10,

    // Temporary flag used by miniport
    CCBSTAT_CALLBACK_PENDING = 0x8000
}   CCB_STATUS;

// The state of ACCEPT_CCBs, to be used for debugging/crash analysis.

typedef enum _AcceptTcdState
{ 

    ACS_FREE                = 0x00,
    ACS_AT_ISR              = 0x01,
    ACS_TO_DPC              = 0x02,
    ACS_AT_DPC              = 0x03,
    ACS_ON_FAIRNESS_Q           = 0x04,
    ACS_WAIT_RESOURCES      = 0x05,		// Waiting in TCD due to lack of resources. 
    ACS_TO_PASSIVE          = 0x06,
    ACS_AT_PASSIVE          = 0x07,
    ACS_CONNECT_AT_TXD      = 0x08,
    ACS_TO_TXD              = 0x09,
    ACS_TCDSTATE_COUNT

} AcceptTcdState;

typedef enum _AcceptTxdState
{ 

    ACS_NONE                = 0x00,
    ACS_IN_HS               = 0x01,
    ACS_ON_CONTINUE_WAIT_Q  = 0x02,
    ACS_DCA_IRP_TO_DS       = 0x03,
    ACS_NON_DCA_IRP_TO_DS   = 0x04,
    ACS_OTHER_IRP_TO_DS     = 0x05,
    ACS_TXDSTATE_COUNT

} AcceptTxdState;

// The state of continue CCBs, to be used for debugging/crash analysis.

typedef enum _ContinueTcdState
{

    CCS_READY               = 0x00,
    CCS_AT_TCD              = 0x01,
    CCS_ON_CAC_Q            = 0x02,
    CCS_ON_MP_Q             = 0x03,
    CCS_BACK_FROM_MP        = 0x04,
    CCS_TO_PASSIVE          = 0x05,
    CCS_AT_PASSIVE          = 0x06,
    CCS_TO_TXD              = 0x07,
    CCS_TCDSTATE_COUNT

} ContinueTcdState;

typedef enum _ContinueTxdState
{ 

    CCS_FREE                 = 0x00,
    CCS_ALLOCATED            = 0x01,
    CCS_GET_BUFFER           = 0x02,
    CCS_ON_PASSIVE_LIST      = 0x03,
    CCS_ON_WITHHOLD_Q        = 0x04,
    CCS_EXECUTE_AT_ISR       = 0x05,
    CCS_DOING_CONFIG_UPDATE  = 0x06,
    CCS_ON_QUIESCE_QUEUE     = 0x07,
    CCS_ON_PRSTNT_RSRV_QUEUE = 0x08,
    CCS_TXDSTATE_COUNT

} ContinueTxdState;

#define PRINT_STATE_VIOLATION 1 // Must be 1 when checked in!!!

#if DBG

#if PRINT_STATE_VIOLATION

#define STATE_ASSERT(exp)                                           \
    if (!(exp)) {                                                   \
        KvPrintx(TRC_K_STD, "Unexpected state (%s)", #exp);         \
        KvPrintx(TRC_K_STD, "at %s:%d\n", __FILE__, __LINE__);      \
    }

#else

#define STATE_ASSERT(exp)                                           \
    if (!(exp)) {                                                   \
        KvPrintx(TRC_K_STD,                                         \
            "Assertion Failed:  (%s);  File %s;  Line %d.\n",       \
            #exp, __FILE__, __LINE__);                              \
        EmcpalDebugBreakPoint();                                            \
    }

#endif //PRINT_STATE_VIOLATION

#else

#define STATE_ASSERT(exp)

#endif //DBG


#define PROMOTE_TCD_STATE(AoC, St)              \
    (AoC)->PreviousTcdState = (AoC)->TcdState;  \
    (AoC)->TcdState = (St);

#define PROMOTE_TXD_STATE(AoC, St)              \
    (AoC)->PreviousTxdState = (AoC)->TxdState;  \
    (AoC)->TxdState = (St);


// For detailed descriptions of Scatter/Gather lists, see:
//    http://nterprise/K10Documents/Internal/Host/ScatterGatherLists.html
// All fields in this structure are ReadOnly by miniport drivers.
typedef struct _STORAGE_BUFFER
{

    // Buffer's Kernel Virtual Address. This field is for the convenience
    // of the driver initiating a transfer.  Miniport drivers must NOT 
    // access this field.
    PVOID                   Buffer; 
    
    // The number of bytes to transfer from the scatter/gather list below.
    // The sum of the SG list entry sizes MUST be >=
    // this value.
    ULONG                   BufferSize; 
    
    // The PhysAddress field in each SG entry should be treated as an
    // offset from some base physical address, i.e., 
    //          PA = Entry.PhysAddress + SglPhysOffset.
    // SglPhysOffset == 0 means Entry.PhysAddress is a true physical address.
    ULONG_PTR               SglPhysOffset;  
    PSGL                    VirtSgl;        // Pointer to untranslated SG List.

# if __cplusplus

    // Put the byte into Buffer[Position], ignore if past buffer end.
    // Always increment Position by 1.
    // Returns:  true - byte added and Position incremented.
    //           false - byte not added but  Position incremented.
    bool PushUCHAR(ULONG & Position, UCHAR Byte)
    {
        if (Position < BufferSize) {
            ((PUCHAR)Buffer)[Position++] = Byte;
            return true;

        }
        else {
            Position++;
            return false;
        }
    }

    // Put the ULONG into Buffer[Position] as big-endian, ignore if past buffer end.
    // Always increment Position by 4.
    // Returns:  true - byte added and Position incremented.
    //           false - byte not added but  Position incremented.
    bool PushULONG(ULONG & Position, ULONG ulong)
    {
        if (Position + sizeof(ULONG) <= BufferSize) {
            ((PUCHAR)Buffer)[Position++] = (UCHAR)(ulong >> 24);
            ((PUCHAR)Buffer)[Position++] = (UCHAR)(ulong >> 16);
            ((PUCHAR)Buffer)[Position++] = (UCHAR)(ulong >> 8);
            ((PUCHAR)Buffer)[Position++] = (UCHAR)ulong;
            return true;

        }
        else {
            Position += sizeof(ULONG);
            return false;
        }
    }

    // Put the ULONGLONG into Buffer[Position] as big-endian, ignore if past buffer end.
    // Always increment Position by 8.
    // Returns:  true - byte added and Position incremented.
    //           false - byte not added but  Position incremented.
    bool PushULONGLONG(ULONG & Position, ULONGLONG ulonglong)
    {
        if (Position + sizeof(ulonglong) <= BufferSize) {
            ((PUCHAR)Buffer)[Position++] = (UCHAR)(ulonglong >> 56);
            ((PUCHAR)Buffer)[Position++] = (UCHAR)(ulonglong >> 48);
            ((PUCHAR)Buffer)[Position++] = (UCHAR)(ulonglong >> 40);
            ((PUCHAR)Buffer)[Position++] = (UCHAR)(ulonglong >> 32);
            ((PUCHAR)Buffer)[Position++] = (UCHAR)(ulonglong >> 24);
            ((PUCHAR)Buffer)[Position++] = (UCHAR)(ulonglong >> 16);
            ((PUCHAR)Buffer)[Position++] = (UCHAR)(ulonglong >> 8);
            ((PUCHAR)Buffer)[Position++] = (UCHAR)ulonglong;
            return true;

        }
        else {
            Position += sizeof(ulonglong);
            return false;
        }
    }

# endif

} STORAGE_BUFFER, *PSTORAGE_BUFFER;



#define TOTAL_SENSE     48

typedef union _SENSE_BUFFER {

    SENSE_DATA          Data;
    UCHAR               Reserved[TOTAL_SENSE];

} SENSE_BUFFER, *PSENSE_BUFFER;


// Each continue CCB contains this many SG list entries, so that
// the SG list entries allocation can be avoided for small transfers.
// Must be at least 2 to handle small buffers that cross page boundaries.

#define SHORT_SGL_SIZE  2

// Each continue CCB contains a small buffer to allow small transfers
// to avoid buffer allocation.
# define CCB_SMALL_BUFFER_SIZE 32





// Each CCB contains a callback function, which takes the CCB itself
// as the only argument.

typedef BOOLEAN         (*PACCEPT_CALLBACK)(struct _ACCEPT_CCB *);
typedef BOOLEAN         (*PCONTINUE_CALLBACK)(struct _CCB *);

    
typedef struct _GIID {

    ULONGLONG   IdHigh;
    ULONGLONG   IdLow;

} GIID, *PGIID;

// An ACCEPT_CCB is associated with each arriving SCSI CDB.  The
// ACCEPT_CCB remains in existance until after the status has been sent
// to the host, until the request is cleaned up.
typedef struct _ACCEPT_CCB  ACCEPT_CCB, *PACCEPT_CCB;

SLListDefineListType(ACCEPT_CCB, ListOfACCEPT_CCB);

struct _ACCEPT_CCB {
    union {
    PACCEPT_CCB             nextAccept;
    EMCPAL_LIST_ENTRY              UNUSED_LINKS;   // Delete when we don't care about binary compatibility with FCDMTL
    };
    
    // For crash/analysis, debug.  Not for Miniport
    AcceptTcdState          TcdState;           // Tcd use
    AcceptTcdState          PreviousTcdState;
    AcceptTcdState          AbortTcdState;

    AcceptTxdState          TxdState;           // Tdd, CmiScd, etc. usage
    AcceptTxdState          PreviousTxdState;           // Tdd, CmiScd, etc. usage

    PVOID                   MiniPortContext;    // MiniPort-use only.

    // Callback function.  Used to notify class driver that Accept_CCB (Cbd)
    // arrived.  The only parameter is the address of the accept itself.
    PACCEPT_CALLBACK        Callback;           // Set by Tcd, RO by Miniport
    
    // The VLU corresponding to the Nexus. Set in ISR, never NULL.  May
    // point to the "dummy VLU".
    struct _VLU             *Vlu;               // TCD- and TxD-use only.

    // The Initiator context, LUN, and Tag.  
    CPD_PATH                Path;               // Set by miniport, RO by others.

    GIID                    Guid;               // Set at ISR by Tcd.  RO by all others.

    PVOID					TcdIrp;

    // TRUE if this request has been aborted, and no transfers may occur 
    // to the initiator for this request.
    BOOLEAN                 Aborted;            // Set at ISR by Tcd.  RO by all others.            
    UCHAR                   Cdb[16];            // The SCSI command.

    // One of:
    //  CPD_UNTAGGED                - Untagged operation
    //  CPD_SIMPLE_QUEUE_TAG        - normal tagged operation
    //  CPD_HEAD_OF_QUEUE_TAG       - head of q
    //  CPD_ORDERED_QUEUE_TAG       - ordered tag
    //  CPD_ACA_QUEUE_TAG           - ACA tag (considered untagged)
    CPD_TAG_TYPE            QueueAction;        // Set by Miniport, RO by others.

    // Set by Miniport, RO by others.  Specifies attributes associated with this 
    // particular CCB.
    CPD_ACF                 AcceptFlags;
    
    // The location of sense data that the miniport may read if 
    // continue->XferFlags & CF_SENSE_VALID
    SENSE_BUFFER            Sense;              // In accept rather than continue for ContingentAllegiance

    // The offset of the physical address of the sense buffer relative to its
    // logical address, i.e., 
    //          Sense Physical Address = &Sense + SensePhysOffset.
    ULONG_PTR               SensePhysOffset;    

    // The value of EmcpalTimeReadMilliseconds on arrival
    EMCPAL_TIME_MSECS           StartMsCount;

    // KeQueryPerformanceCounter value on arrival from Miniport.  
    UINT_64E                PerfCounter;

    // Holds core number on which DiskTarg!WO::Signaled() executed for this SCSI op
    int Core;


    // Spinlock for TDD_WORKORDER to avoid creation and destruction for every DCA and non-DCA IOs. 
    // For extended copy (0x83), EXT_COPY_REGISTER::EXT_COPY_INFO::DM_RESOURCES::DmrLock is used. 
    EMCPAL_SPINLOCK         WorkOrderLock;
};

// Continue CCBs are used to request data transfers between the initiator and 
// memory.  For a given Accept CCB, transfers are done in the order that the
// Continue CCBs are queued to the miniport.  If a transfer fails, the miniport 
// must reject subsequent Continues with data phases for the same Accept, along 
// with any attempt to send status.  Multiple Continue CCBs can be associated 
// with a single Accept CCB.

typedef struct _CCB {
    union {
    struct _CCB *           Flink;          // For use by current owner of CCB
    EMCPAL_LIST_ENTRY              UNUSED_LINKS;   // Still used by Reservation Thread and Miniport
    };

    // For crash/analysis, debug. Not for Miniport
    ContinueTcdState        TcdState;           // Tcd use
    ContinueTcdState        PreviousTcdState;

    ContinueTxdState        TxdState;           // Tdd, CmiScd, etc. usage
    ContinueTxdState        PreviousTxdState;

    struct _ACCEPT_CCB      *ParentCcb;         // Ptr. to parent accept ccb.
    PVOID					TcdIrp;

    // Called when transfer completes or fails.  RO by miniport.
    PCONTINUE_CALLBACK      Callback;

    // RO for miniport.  Describes what kind of xfers to do. See CCB_FLAGS
    CCB_FLAGS               XferFlags;

    // Success, or failure with reason. See CCB_STATUS.  Set by Miniport.
    CCB_STATUS              XferStatus;

    // The SCSI status byte to send to the initiator, if
    // XferFlags & CF_SCSI_STATUS_VALID.
    // If XferFlags & CF_SENSE_VALID, ParentCcb->Sense contains Sense data.
    UCHAR                   ScsiStatus;         // RO by miniport
    BOOLEAN                 EnterAca;           // If true this Ccb caused ACA

    // Describes where the transfer should take place to.  RO for miniport.
    // Used by miniport iff XferFlags & (CF_DATA_IN_VALID | CF_DATA_OUT_VALID)
    STORAGE_BUFFER          Mem;

    // Preallocated small SG List for use by Continue's originator. Should
    // not be directly referenced by miniport.
    SGL                     ShortSgl[SHORT_SGL_SIZE];

    SGL                     ShortVSgl[1];

    // Preallocated small buffer for use by Continue's originator. Should
    // not be directly referenced by miniport. Note that this buffer
    // may not be physically contiguous.
    UCHAR                   SmallBuffer[CCB_SMALL_BUFFER_SIZE];

    //TODO: THIS IS A HACK TO SATISFY A TESTING REQUIREMENT BY NSPD
    SGL                     PhysSgl[2];

    // KeQueryPerformanceCounter value before departure to Host or Diskside. 
    UINT_64E                PerfCounter;

} CCB, *PCCB;
 
SLListDefineListType(CCB, ListOfCCB);
LIFOSLListDefineListType(CCB, FreeListOfCCB);

SLListDefineInlineListTypeFunctions(CCB, ListOfCCB, Flink)

SLListDefineInlineListTypeFunctions(ACCEPT_CCB, ListOfACCEPT_CCB, nextAccept)

// For the specified virtual address/length, setup the scatter/gather
// entry fields in the CCB, using the preallocated scatter/gather list.
// The specified buffer must be physically contiguous.

# define CCB_SetupSinglePhysSGLFromVaddr(ccb, vaddr, len)           \
    CCB_SetupSinglePhysSGL(ccb, cmmGetPhysAddrFromVA(vaddr), len)

// For the virtual address/length specified in 
// (ccb)->Mem.Buffer/(ccb)->Mem.BufferSize,
// setup the scatter/gather entry fields in the CCB, using 
// the preallocated scatter/gather list.
// The specified buffer must be physically contiguous.

# define CCB_SetupSinglePhysSGLFromBuffer(ccb)                      \
    CCB_SetupSinglePhysSGLFromVaddr(ccb,                            \
                                    (ccb)->Mem.Buffer,              \
                                    (ccb)->Mem.BufferSize)


// Set the preallocted single entry S/G list in a ccb to
// the specified physicalAddress/length.  The paddr parameter
// must be a true physical address.

# define CCB_SetupSinglePhysSGL(ccb, paddr, len) {          \
    (ccb)->ShortSgl[0].PhysAddress = (PVOID)(paddr);        \
    (ccb)->ShortSgl[0].PhysLength = (len);                  \
    (ccb)->Mem.SglPhysOffset = 0;                           \
    (ccb)->Mem.VirtSgl = (ccb)->ShortSgl;                   \
    }


// Set the preallocted single entry S/G list in a ccb to
// reference the SmallBuffer in the CCB.
static __inline VOID CCB_SetupCCBForSmallBufferUsage(PCCB ccb, ULONG len) 
{   
    ULONG               bytesInPage;
    
    ccb->Mem.BufferSize = len;
    ccb->Mem.Buffer = ccb->SmallBuffer; 
    
    ccb->ShortVSgl[0].PhysAddress = (SGL_PADDR) (ULONG_PTR)(&ccb->SmallBuffer);
    ccb->ShortVSgl[0].PhysLength = len;
    
    (ccb)->ShortSgl[0].PhysAddress = cmmGetPhysAddrFromVA(&ccb->SmallBuffer[0]);
    
    bytesInPage = (ULONG)(PAGE_SIZE - ((ULONG_PTR)&ccb->SmallBuffer[0] & (PAGE_SIZE -1)));
    if (bytesInPage < len) {
        (ccb)->ShortSgl[0].PhysLength = bytesInPage;                    
        (ccb)->ShortSgl[1].PhysAddress = cmmGetPhysAddrFromVA(&ccb->SmallBuffer[bytesInPage]);
        (ccb)->ShortSgl[1].PhysLength = len - bytesInPage;
    }
    else {
        (ccb)->ShortSgl[0].PhysLength = len;
    }

    ccb->Mem.SglPhysOffset = 0;
 }

#ifdef C4_INTEGRATED
/* referenced by fakeport and smbtool  */
#define FAKEPORT_BLKSHIM_IOCTL_DEBUG    1
#define FAKEPORT_BLKSHIM_IOCTL_NO_DEBUG 2
#define FAKEPORT_BLKSHIM_IOCTL_GO       4
#endif /* C4_INTEGRATED - C4ARCH - blockshim_fakeport */

#endif /* _TCDSHARE_H_ */

