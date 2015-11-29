/* $CCRev: ./array_sw/catmerge/FabricArray/interface/BasicTrace.h@@/main/daytona_mr1/1 */
/************************************************************************
 *
 *  Copyright (c) 2005 EMC Corporation.
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
 *
 ************************************************************************/

#ifndef __BasicObjectTracer__
#define __BasicObjectTracer__


#include "csx_ext.h"
# include <stdarg.h>
# include "BasicLib/BasicTraceLevel.h"
# include "ktrace_structs.h"
#include "ktrace.h"


//++
// File Name:
//      BasicObjectTracer.h
//
// Contents:
//      This file contains the definition of the BasicObjectTracer
//    abstract base class.
//


// A BasicObjectTracer is an abstract base class to be included in classes to allow Ktrace
// logging on a per instance basis.  This defines a standard interface for tracing
// information about the current object.
//
// The subclass must implement the TraceRing method, which will implement the details of
// the tracing.  Typically this implementation will prefix all trace messages with the
// driver name, an indication of object type, and some unique object identifier (a key or
// the value of "this").
class BasicObjectTracer {
public:
    // Default constructor
    BasicObjectTracer() : mTrafficLogStateInfo(NULL), mTraceIdentifier(0), mTraceEnableBit(0) {};

    // Log to ktrace
    // @param level - the trace level this message is for
    // @param format - the ktrace format string.
    // @param ... - format string parameters.
    VOID Trace(BasicTraceLevel level, const char * format, ...) const __attribute__((format(__printf_func__,3,4)));

    // Log Bug to ktrace.  If such a trace occurs, a bug report should be filed, and
    // engineering should analyze.
    // @param format - the ktrace format string.
    // @param ... - format string parameters.
    VOID TraceBug(const char * format, ...) const __attribute__((format(__printf_func__,2,3)));

    // Log Error to ktrace
    // @param format - the ktrace format string.
    // @param ... - format string parameters.
    VOID TraceError(const char * format, ...) const __attribute__((format(__printf_func__,2,3)));

    // Log warning to ktrace
    // @param format - the ktrace format string.
    // @param ... - format string parameters.
    VOID TraceWarn(const char * format, ...) const __attribute__((format(__printf_func__,2,3)));

    // Log information to ktrace
    // @param format - the ktrace format string.
    // @param ... - format string parameters.
    VOID TraceInfo(const char * format, ...) const __attribute__((format(__printf_func__,2,3)));

    // Log object creation/destruction information to ktrace
    // @param format - the ktrace format string.
    // @param ... - format string parameters.
    VOID TraceCreate(const char * format, ...) const __attribute__((format(__printf_func__,2,3)));

    // Log startup information to ktrace
    // @param format - the ktrace format string.
    // @param ... - format string parameters.
    VOID TraceStartup(const char * format, ...) const __attribute__((format(__printf_func__,2,3)));

    // Log verbose startup information to ktrace
    // @param format - the ktrace format string.
    // @param ... - format string parameters.
    VOID TraceVerboseStartup(const char * format, ...) const __attribute__((format(__printf_func__,2,3)));

    // Log debugging information to ktrace
    // @param format - the ktrace format string.
    // @param ... - format string parameters.
    VOID TraceDebug(const char * format, ...) const __attribute__((format(__printf_func__,2,3)));

    // Log verbose debugging information to ktrace
    // @param format - the ktrace format string.
    // @param ... - format string parameters.
    VOID TraceVerboseDebug(const char * format, ...) const __attribute__((format(__printf_func__,2,3)));

    // Initialize the RBA member variables
    // @param traceIdentifier - Ktrace ID for the layer being traced.
    // @param traceEnableBit - Bit mask for layer being traced
    VOID InitializeRBA(UINT_32 traceIdentifier, UINT_32 traceEnableBit)
    {
        mTrafficLogStateInfo = KtraceInitN(TRC_K_TRAFFIC);
        mTraceIdentifier = traceIdentifier;
        mTraceEnableBit = traceEnableBit;
    }

    bool TraceRbaEnabled() const {
        return (mTrafficLogStateInfo != NULL) && (((TRACE_INFO *) mTrafficLogStateInfo)->ti_tflag & mTraceEnableBit);
    }
    
    // Log the start of a read IO to ktrace
    // @param lun - LUN associated with the IO
    // @param lba - Target LBA for IO
    // @param length - Length of IO
    VOID TraceRbaReadStart(ULONGLONG lun, LBA_T lba, ULONGLONG length) const;

    // Log the end of a read IO to ktrace
    // @param lun - LUN associated with the IO
    // @param lba - Target LBA for IO
    // @param length - Length of IO
    // @param status - Status of the request being completed
    VOID TraceRbaReadStop(ULONGLONG lun, LBA_T lba, ULONGLONG length, ULONG status=0) const;

    // Log the start of a write IO to ktrace
    // @param lun - LUN associated with the IO
    // @param lba - Target LBA for IO
    // @param length - Length of IO
    VOID TraceRbaWriteStart(ULONGLONG lun, LBA_T lba, ULONGLONG length) const;

    // Log the end of a write IO to ktrace
    // @param lun - LUN associated with the IO
    // @param lba - Target LBA for IO
    // @param length - Length of IO
    // @param status - Status of the request being completed
    VOID TraceRbaWriteStop(ULONGLONG lun, LBA_T lba, ULONGLONG length, ULONG status=0) const;

    // Log the start of a zero IO to ktrace
    // @param lun - LUN associated with the IO
    // @param lba - Target LBA for IO
    // @param length - Length of IO
    VOID TraceRbaZeroStart(ULONGLONG lun, LBA_T lba, ULONGLONG length) const;

    // Log the end of a zero IO to ktrace
    // @param lun - LUN associated with the IO
    // @param lba - Target LBA for IO
    // @param length - Length of IO
    // @param status - Status of the request being completed
    VOID TraceRbaZeroStop(ULONGLONG lun, LBA_T lba, ULONGLONG length, ULONG status=0) const;

    // Log the start of a DateMover IO to ktrace
    // @param srcLun - Source LUN associated with the IO
    // @param srcLba - Source LBA for IO
    // @param dstLun - Destination LUN associated with the IO
    // @param dstLba - Destination LBA for IO
    // @param length - Length of IO
    VOID TraceRbaDMStart(ULONGLONG srcLun, LBA_T srcLba, ULONGLONG dstLun, LBA_T dstLba, ULONGLONG length) const;

    // Log the end of a DateMover IO to ktrace
    // @param srcLun - Source LUN associated with the IO
    // @param srcLba - Source LBA for IO
    // @param dstLun - Destination LUN associated with the IO
    // @param dstLba - Destination LBA for IO
    // @param length - Length of IO
    // @param status - Status of the request being completed
    VOID TraceRbaDMStop(ULONGLONG srcLun, LBA_T srcLba, ULONGLONG dstLun, LBA_T dstLba, ULONGLONG length, ULONG status=0) const;

protected:

    // Generic RBA start tracing where the caller can specify the command.
    // @param command - Trace command, DM_START will be added to value provided.
    // @param srcLun - Source LUN associated with the IO
    // @param srcLba - Source LBA for IO
    // @param dstLun - Destination LUN associated with the IO
    // @param dstLba - Destination LBA for IO
    // @param length - Length of IO
    VOID TraceRbaGenericStart(ULONG command,
                              ULONGLONG srcLun,
                              LBA_T srcLba,
                              ULONGLONG dstLun,
                              LBA_T dstLba,
                              ULONGLONG length) const;

    // Generic RBA stop tracing where the caller can specify the command.
    // @param command - Trace command, DM_DONE will be added to value provided.
    // @param srcLun - Source LUN associated with the IO
    // @param srcLba - Source LBA for IO
    // @param dstLun - Destination LUN associated with the IO
    // @param dstLba - Destination LBA for IO
    // @param length - Length of IO
    // @param status - The status of the request that is being completed.
    VOID TraceRbaGenericStop(ULONG command,
                             ULONGLONG srcLun,
                             LBA_T srcLba,
                             ULONGLONG dstLun,
                             LBA_T dstLba,
                             ULONGLONG length,
                             ULONG status=0) const;

    // Extended generic RBA start tracing where the caller can specify the command.
    // @param command - Trace command, DM_START will be added to value provided.
    // @param srcLun - Source LUN associated with the IO
    // @param srcLba - Source LBA for IO
    // @param dstLun - Destination LUN associated with the IO
    // @param dstLba - Destination LBA for IO
    // @param length - Length of IO
    // @param extended - shifts extended info into tr_id (above mIndentifer)
    VOID TraceRbaExtendedStart(ULONG command,
                              ULONGLONG srcLun,
                              LBA_T srcLba,
                              ULONGLONG dstLun,
                              LBA_T dstLba,
                              ULONGLONG length,
                              ULONGLONG extended) const;

    // Extended generic RBA stop tracing where the caller can specify the command.
    // @param command - Trace command, DM_DONE will be added to value provided.
    // @param srcLun - Source LUN associated with the IO
    // @param srcLba - Source LBA for IO
    // @param dstLun - Destination LUN associated with the IO
    // @param dstLba - Destination LBA for IO
    // @param length - Length of IO
    // @param extended - shifts extended info into tr_id (above mIndentifer)    
    // @param status - The status of the request that is being completed.
    VOID TraceRbaExtendedStop(ULONG command,
                             ULONGLONG srcLun,
                             LBA_T srcLba,
                             ULONGLONG dstLun,
                             LBA_T dstLba,
                             ULONGLONG length,
                             ULONGLONG extended,
                             ULONG status=0) const;
    
    // Internal logging function, must be defined in the subclass.
    // @param ring - the ktrace ring buffer to log to
    // @param format - the printf format string
    // @param args - the format arguments.
   __attribute__ ((format(__printf_func__, 3, 0))) virtual VOID TraceRing(KTRACE_ring_id_T ring, const char * format, va_list args)  const = 0;

    // Decide whether a certain level of tracing is required.  The default is to delegate
    // this to BasicVolumeDriver::Driver->DetermineTraceBuffer(level).
    // @param level - the trace level to test.
    virtual KTRACE_ring_id_T  DetermineTraceBuffer(BasicTraceLevel level) const = 0;

private:
    // Which RBA trace buffers are enabled
    PVOID mTrafficLogStateInfo;

    // Ktrace ID for layer being traced
    UINT_32 mTraceIdentifier;

    // Bit to identify whether this instance should be traced
    UINT_32 mTraceEnableBit;
};


inline VOID BasicObjectTracer::Trace(BasicTraceLevel level, const char * format, ...) const
{
    KTRACE_ring_id_T ring = DetermineTraceBuffer(level);
    if (ring != TRC_K_NONE) {
        va_list args;

        va_start( args, format );     /* Initialize variable arguments. */
        TraceRing(ring, format, args);
        va_end( args );
    }
}

inline VOID BasicObjectTracer::TraceBug(const char * format, ...) const
{
    KTRACE_ring_id_T ring = DetermineTraceBuffer(BTRC_BUG);
    if (ring != TRC_K_NONE) {
        va_list args;

        va_start( args, format );     /* Initialize variable arguments. */
        TraceRing(ring, format, args);
        va_end( args );
    }
}

inline VOID BasicObjectTracer::TraceError(const char * format, ...) const
{
    KTRACE_ring_id_T ring = DetermineTraceBuffer(BTRC_ERROR);
    if (ring != TRC_K_NONE) {
        va_list args;

        va_start( args, format );     /* Initialize variable arguments. */
        TraceRing(ring, format, args);
        va_end( args );
    }
}

inline VOID BasicObjectTracer::TraceWarn(const char * format, ...) const
{
    KTRACE_ring_id_T ring = DetermineTraceBuffer(BTRC_WARN);
    if (ring != TRC_K_NONE) {
        va_list args;

        va_start( args, format );     /* Initialize variable arguments. */
        TraceRing(ring, format, args);
        va_end( args );
    }
}

inline VOID BasicObjectTracer::TraceStartup(const char * format, ...) const
{
    KTRACE_ring_id_T ring = DetermineTraceBuffer(BTRC_START_INFO);
    if (ring != TRC_K_NONE) {
        va_list args;

        va_start( args, format );     /* Initialize variable arguments. */
        TraceRing(ring, format, args);
        va_end( args );
    }
}

inline VOID BasicObjectTracer::TraceVerboseStartup(const char * format, ...) const
{
#ifndef C4_INTEGRATED
#if DBG || defined(SIMMODE_ENV)
    KTRACE_ring_id_T ring = DetermineTraceBuffer(BTRC_START_INFO);
    if (ring != TRC_K_NONE) {
        va_list args;

        va_start( args, format );     /* Initialize variable arguments. */
        TraceRing(ring, format, args);
        va_end( args );
    }
#endif // DBG || defined(SIMMODE_ENV)
#endif // C4_INTEGRATED
}

inline VOID BasicObjectTracer::TraceInfo(const char * format, ...) const
{
    KTRACE_ring_id_T ring = DetermineTraceBuffer(BTRC_INFO);
    if (ring != TRC_K_NONE) {
        va_list args;

        va_start( args, format );     /* Initialize variable arguments. */
        TraceRing(ring, format, args);
        va_end( args );
    }
}

inline VOID BasicObjectTracer::TraceCreate(const char * format, ...) const
{
    KTRACE_ring_id_T ring = DetermineTraceBuffer(BTRC_INFO);
    if (ring != TRC_K_NONE) {
        va_list args;

        va_start( args, format );     /* Initialize variable arguments. */
        TraceRing(ring, format, args);
        va_end( args );
    }
}

inline VOID BasicObjectTracer::TraceDebug(const char * format, ...) const
{
    KTRACE_ring_id_T ring = DetermineTraceBuffer(BTRC_DEBUG_TERSE);
    if (ring != TRC_K_NONE) {
        va_list args;

        va_start( args, format );     /* Initialize variable arguments. */
        TraceRing(ring, format, args);
        va_end( args );
    }
}

inline VOID BasicObjectTracer::TraceVerboseDebug(const char * format, ...) const
{
#ifndef C4_INTEGRATED
#if DBG || defined(SIMMODE_ENV)
    KTRACE_ring_id_T ring = DetermineTraceBuffer(BTRC_DEBUG_VERBOSE);
    if (ring != TRC_K_NONE) {
        va_list args;

        va_start( args, format );     /* Initialize variable arguments. */
        TraceRing(ring, format, args);
        va_end( args );
    }
#endif
#endif // C4_INTEGRATED
}

inline VOID BasicObjectTracer::TraceRbaGenericStart(ULONG command,
                                                    ULONGLONG srcLun,
                                                    LBA_T srcLba,
                                                    ULONGLONG dstLun,
                                                    LBA_T dstLba,
                                                    ULONGLONG length) const
{
    if ( (mTrafficLogStateInfo != NULL) && (((TRACE_INFO *) mTrafficLogStateInfo)->ti_tflag & mTraceEnableBit) ) {
        KTRACEX(TRC_K_TRAFFIC, mTraceIdentifier,
                srcLba,
                dstLba,
                KT_TRAFFIC_LU1(srcLun)| KT_TRAFFIC_LU2(dstLun) | KT_TRAFFIC_BLOCKS(length),
                (command | KT_START));
    }

}

inline VOID BasicObjectTracer::TraceRbaGenericStop(ULONG command,
                                                   ULONGLONG srcLun,
                                                   LBA_T srcLba,
                                                   ULONGLONG dstLun,
                                                   LBA_T dstLba,
                                                   ULONGLONG length,
                                                   ULONG status) const
{
    if ( (mTrafficLogStateInfo != NULL) && (((TRACE_INFO *) mTrafficLogStateInfo)->ti_tflag & mTraceEnableBit) ) {
        KTRACEX(TRC_K_TRAFFIC, mTraceIdentifier,
                srcLba,
                dstLba,
                KT_TRAFFIC_LU1(srcLun)| KT_TRAFFIC_LU2(dstLun) | KT_TRAFFIC_BLOCKS(length),
                ((ULONGLONG)(command | KT_DONE))|((ULONGLONG)status << 32));
    }

}

inline VOID BasicObjectTracer::TraceRbaExtendedStart(ULONG command,
        ULONGLONG srcLun,
        LBA_T srcLba,
        ULONGLONG dstLun,
        LBA_T dstLba,
        ULONGLONG length,
        ULONGLONG extended) const
{
    if ( (mTrafficLogStateInfo != NULL) && (((TRACE_INFO *) mTrafficLogStateInfo)->ti_tflag & mTraceEnableBit) ) {
        KTRACEX(TRC_K_TRAFFIC, (ULONGLONG)mTraceIdentifier | (extended << 16),
                srcLba,
                dstLba,
                KT_TRAFFIC_LU1(srcLun)| KT_TRAFFIC_LU2(dstLun) | KT_TRAFFIC_BLOCKS(length),
                (command | KT_START));
    }

}

inline VOID BasicObjectTracer::TraceRbaExtendedStop(ULONG command,
        ULONGLONG srcLun,
        LBA_T srcLba,
        ULONGLONG dstLun,
        LBA_T dstLba,
        ULONGLONG length,
        ULONGLONG extended,
        ULONG status) const
{
    if ( (mTrafficLogStateInfo != NULL) && (((TRACE_INFO *) mTrafficLogStateInfo)->ti_tflag & mTraceEnableBit) ) {
        KTRACEX(TRC_K_TRAFFIC, (ULONGLONG)mTraceIdentifier | (extended << 16),
                srcLba,
                dstLba,
                KT_TRAFFIC_LU1(srcLun)| KT_TRAFFIC_LU2(dstLun) | KT_TRAFFIC_BLOCKS(length),
                ((ULONGLONG)(command | KT_DONE))|((ULONGLONG)status << 32));
    }

}

inline VOID BasicObjectTracer::TraceRbaReadStart(ULONGLONG lun, LBA_T lba, ULONGLONG length) const
{
    TraceRbaGenericStart(KT_READ, lun, lba, 0, 0, length);
}

inline VOID BasicObjectTracer::TraceRbaReadStop(ULONGLONG lun, LBA_T lba, ULONGLONG length, ULONG status) const
{
    TraceRbaGenericStop(KT_READ, lun, lba, 0, 0, length, status);
}

inline VOID BasicObjectTracer::TraceRbaWriteStart(ULONGLONG lun, LBA_T lba, ULONGLONG length) const
{
    TraceRbaGenericStart(KT_WRITE, lun, lba, 0, 0, length);
}

inline VOID BasicObjectTracer::TraceRbaWriteStop(ULONGLONG lun, LBA_T lba, ULONGLONG length, ULONG status) const
{
    TraceRbaGenericStop(KT_WRITE, lun, lba, 0, 0, length, status);
}

inline VOID BasicObjectTracer::TraceRbaZeroStart(ULONGLONG lun, LBA_T lba, ULONGLONG length) const
{
    TraceRbaGenericStart(KT_ZERO, lun, lba, 0, 0, length);
}

inline VOID BasicObjectTracer::TraceRbaZeroStop(ULONGLONG lun, LBA_T lba, ULONGLONG length, ULONG status) const
{
    TraceRbaGenericStop(KT_ZERO, lun, lba, 0, 0, length, status);
}

inline VOID BasicObjectTracer::TraceRbaDMStart(ULONGLONG srcLun,
                                               LBA_T srcLba,
                                               ULONGLONG dstLun,
                                               LBA_T dstLba,
                                               ULONGLONG length) const
{
    TraceRbaGenericStart(KT_MOVE, srcLun, srcLba, dstLun, dstLba, length);
}

inline VOID BasicObjectTracer::TraceRbaDMStop(ULONGLONG srcLun,
                                              LBA_T srcLba,
                                              ULONGLONG dstLun,
                                              LBA_T dstLba,
                                              ULONGLONG length,
                                              ULONG status) const
{
    TraceRbaGenericStop(KT_MOVE, srcLun, srcLba, dstLun, dstLba, length, status);
}

#endif // __BasicObjectTracer__
