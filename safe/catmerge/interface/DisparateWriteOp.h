/*******************************************************************************
 * Copyright (C) EMC Corporation, 2014 - 2015.
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/
#ifndef __DisparateWriteOp__
#define __DisparateWriteOp__

// DisparateWriteOp : MLU to MCC : perform multiple write as atomic operation.
// Position is specified as ByteOffset/Length must be multiple of 512 sector size, similar to MJ_WRITE, DCA_WRITE.
// Use cases:
//  * Single host write to datalog(s) as logically sequential DcaWrite(s), and MjWrite(s) of meta-data.
//    DCA_TABLE and HostIrp used to gather host request in new cache pages.  Any subsequent DcaWrite must start page aligned.
//  * As above plus DwCopy during slice migration or TDX DCA write split. 
//  * Flush datalog: set of DwRename or DwCopy to backing store, and MjWrite or DwZero of metadata.

typedef struct DisparateWrite_WriteZeroOrDiscard {
    void *                  Volume;        // Device Object's DeviceExtension (opaque to caller)
    unsigned long long      ByteOffset;    // Logical bytes offset: subsequent writes must be page aligned.
    unsigned long long      ByteLength;    // Length in bytes (multiple of 512)
    unsigned long long      Flags;         // Relevant flag settings for operation
    unsigned long long      Keys;          // Relevant key values for operation
    void *                  Buffer;        // Virtual address, only for MjWrite
    void *                  MluContext;    // Context specific to MLU      
} DisparateWrite_WriteZeroOrDiscard;   /* Single Volume Operations */

typedef struct DisparateWrite_CopyOrRename {
    void *                  SrcVolume;     // Src Device Object's DeviceExtension (opaque to caller)
    unsigned long long      SrcByteOffset; // Src Logical bytes offset
    void *                  DstVolume;     // Dst Device Object's DeviceExtension (opaque to caller)
    unsigned long long      DstByteOffset; // Dst Logical bytes offset
    unsigned long long      ByteLength;    // Length in bytes (multiple of 512)
    unsigned long long      Flags;         // Relevant flag settings for operation
    unsigned long long      Keys;          // Relevant key values for operation
    void *                  MluContext1;   // Context specific to MLU      
    void *                  MluContext2;   // Context2 specific to MLU      
} DisparateWrite_CopyOrRename;     /* Two Volume Operations */


typedef struct DisparateWriteOp {
    struct DisparateWriteOp *Next;        // Link to next or NULL
    
    // Note: DwDiscard is last single-volume op: class DwSortOps compares with DwDiscard.
    enum WriteType { InvalidType, MjWrite, DcaWrite, DwZero, DwDiscard, DwCopy, DwRename } 
                           Operation;     // Type of write operation.

    unsigned int           SubSetTransactionOrder;      // Lowest order commit first.  Must be sorted lowest to highest.

    union {
        DisparateWrite_WriteZeroOrDiscard WriteZeroOrDiscard;   /* Single Volume Operations */
        DisparateWrite_CopyOrRename       CopyOrRename;     /* Two Volume Operations */
    };
} DisparateWriteOp;

#endif /* __DisparateWriteOp__ */
