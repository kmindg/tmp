/*******************************************************************************
 * Copyright (C) EMC Corporation, 2014 - 2015.
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/
#ifndef __DisparateWriteIoctl__
#define __DisparateWriteIoctl__

#include "DisparateWriteOp.h"

// IOCTL_CACHE_DISPARATE_WRITE : MLU to MCC : perform multiple write as atomic operation.
// Position is specified as ByteOffset/Length must be multiple of 512 sector size, similar to MJ_WRITE, DCA_WRITE.
// Use cases:
//  * Single host write to datalog(s) as logically sequential DcaWrite(s), and MjWrite(s) of meta-data.
//    DCA_TABLE and HostIrp used to gather host request in new cache pages.  Any subsequent DcaWrite must start page aligned.
//  * As above plus DwCopy during slice migration or TDX DCA write split. 
//  * Flush datalog: set of DwRename or DwCopy to backing store, and MjWrite or DwZero of metadata.

typedef struct IoctlCacheDisparateWriteReq {
    void *              DcaTable;                     // PDCA_TABLE to transfer logically sequential host data
    void *              HostIrp;                      // PEMCPAL_IRP context for DcaTable->XferFunction.
    unsigned long long  StartingByteOffsetinDcaTable; // From which offset in Dca Table to process IO
    DisparateWriteOp*   Ops;                          // Null terminated linked list of Ops.
    unsigned long long  ReqId;                        // Identify this request in RBA traces
} IoctlCacheDisparateWriteReq;

#endif /* __DisparateWriteIoctl__ */
