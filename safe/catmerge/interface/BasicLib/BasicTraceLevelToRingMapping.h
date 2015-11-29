/* $CCRev: ./array_sw/catmerge/FabricArray/interface/BasicTrace.h@@/main/daytona_mr1/1 */
/************************************************************************
 *
 *  Copyright (c) 2005-2009 EMC Corporation.
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
 ************************************************************************
 *
 *  2005-11-10 08:47pm  fabx-r5036       jahern    .../daytona_mr1/1
 *
 ************************************************************************/

#ifndef __BasicTraceLevelToRingMapping__
#define __BasicTraceLevelToRingMapping__

# include "BasicTraceLevel.h"
# include "ConfigDatabase.h"
# include "ktrace.h"

//++
// File Name:
//      BasicTraceLevelToRingMapping.h
//
// Contents:
//      This file contains the definition of the BasicTraceLevelToRingMapping
//   class.
//


// BasicTraceLevelToRingMapping defines the current mapping of BasicTraceLevel to KTrace
// ring buffers.  Mapping a level to TRC_K_NONE suppresses tracing for that level.
//
// NOTE: mapping to TRC_K_IO creates problems due to the fact that the IO buffer supports
// only 4 32 bit values as arguments, and those that are string pointers must be pointers
// to constant strings.  The IO buffer should not be used in its present form (An
// enhancement to allow more arguments and nested format strings would allow it to be
// used).
//
// FIX: Support object tracing to TRC_K_IO.  To do this reasonably, TRC_K_IO in ktrace.sys
// should support at least 6 arguments (perhaps 8), and allow two format string via some
// means, so that a pointer to each format string is saved in the I/O buffer, where the
// first format string would specify the prefix for the object, along with some arguments,
// and the second string would specify the details. See BasicVolume::TraceRing() for more
// details.
//
//
// Possible Enhancements:
//   - A debugger extension to change the mapping array (mMap) would allow run time
//     changes in what is logged, to disable or enable TraceLevels, or to change the trace
//     buffer for a TraceLevel.
//   - A constructor/Method to initialize the state taking a DatabaseKey as the root of a
//     tree, e.g., initialization via the registry.
//   - Allow a mapping to reference another mapping for mappings of levels that were not
//     explicitly specified.  This would allow objects to use, say, a class mapping by
//     default, but to override this on a per object basis.
//
// 
class BasicTraceLevelToRingMapping {
public:

    // The constructor defines the default mapping.
    // @param error - the ring buffer for BTRC_ERROR
    // @param warn - the ring buffer for BTRC_WARN
    // @param startInfo - the ring buffer for BTRC_START_INFO
    // @param info - the ring buffer for BTRC_INFO
    // @param debugTerse - the ring buffer for BTRC_DEBUG_TERSE
    // @param debugVerbose - the ring buffer for BTRC_DEBUG_VERBOSE
    BasicTraceLevelToRingMapping(
        KTRACE_ring_id_T error          = TRC_K_STD, 
        KTRACE_ring_id_T warn           = TRC_K_STD, 
        KTRACE_ring_id_T startInfo      = TRC_K_START,
        KTRACE_ring_id_T info           = TRC_K_STD,

        
#       if !DBG
        KTRACE_ring_id_T debugTerse     = TRC_K_NONE,
        KTRACE_ring_id_T debugVerbose   = TRC_K_NONE

#       else 
        KTRACE_ring_id_T debugTerse     = TRC_K_STD,
        KTRACE_ring_id_T debugVerbose   = TRC_K_STD
#       endif 
    )
    {
        for (int i = 0; i <= BTRC_MAX_LEVEL; i++) {
            mMap[i] = TRC_K_NONE;
        }

        mMap[BTRC_BUG] = TRC_K_STD;  // No choice!

        mMap[BTRC_ERROR] = error; 
        mMap[BTRC_START_INFO] = startInfo; 
        mMap[BTRC_WARN] = warn;
        mMap[BTRC_INFO] = info;
        mMap[BTRC_DEBUG_TERSE] = debugTerse;
        mMap[BTRC_DEBUG_VERBOSE] = debugVerbose;
    }

    // For a trace level, determine which KTrace buffer should be used at this time.
    // 
    // @param level - The trace level the query is about.
    //  
    //   Returns the ring buffer to use or TRC_K_NONE to indicate no logging.
    KTRACE_ring_id_T  DetermineTraceBuffer(BasicTraceLevel level) const
    {
        return mMap[level];
    }

    // For a trace level, change the KTrace ring buffer it is mapped to.
    // @param level - the trace level to change the mapping for
    // @param ringBuffer - the KTrace ring buffer to now use.  TRC_K_NONE indicates that
    //                     log messages of this level should be suppressed.
    void SetMapping(BasicTraceLevel level, KTRACE_ring_id_T ringBuffer)
    {
        mMap[level] = ringBuffer;
    }

    // Change the KTrace ring buffer mapping based on registry entries, if they exist.
    //
    // @param key - a node in the registry tree that may contain override values
    void SetMapping(DatabaseKey * key)
    {
        // Use the old value as default if the value does not exist.
        mMap[BTRC_ERROR] = (KTRACE_ring_id_T) key->GetULONGValue("ERROR", mMap[BTRC_ERROR]); 
        mMap[BTRC_START_INFO] = (KTRACE_ring_id_T)key->GetULONGValue("START", mMap[BTRC_START_INFO]); 
        mMap[BTRC_WARN] = (KTRACE_ring_id_T)key->GetULONGValue("WARN", mMap[BTRC_WARN]);
        mMap[BTRC_INFO] = (KTRACE_ring_id_T)key->GetULONGValue("INFO", mMap[BTRC_INFO]);
        mMap[BTRC_DEBUG_TERSE] = (KTRACE_ring_id_T)key->GetULONGValue("DebugTerse", mMap[BTRC_DEBUG_TERSE]);
        mMap[BTRC_DEBUG_VERBOSE] = (KTRACE_ring_id_T)key->GetULONGValue("DebugVerbose", mMap[BTRC_DEBUG_VERBOSE]);
    }

private:

    // A simple mapping of level to ring buffer.
    KTRACE_ring_id_T    mMap[BTRC_MAX_LEVEL+1];
};



#endif // __BasicTraceLevelToRingMapping__
