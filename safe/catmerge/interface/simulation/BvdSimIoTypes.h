/***************************************************************************
 * Copyright (C) EMC Corporation 2013-2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               BvdSimIoTypes.h
 ***************************************************************************
 *
 * DESCRIPTION:     
 *       
 *       
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY: 03/09/2011  Komal Padia Initial Version
 *     
 *
 **************************************************************************/
#ifndef __BVDSIMIOTYPES__
#define __BVDSIMIOTYPES__

// The following stream types determine how I/O is performed on the disk.  Currently the code supports 2 methods: 
// 1) Every thread is performing IO to its own private region of a LUN or 
// 2) multiple threads are performing overlapping IO to this LUN.
//
// For method 1, the code divides the disk up into a number of regions based upon the the size of the disk and the
//  number of threads that will be accessing the disk.   Each thread is assigned a unique startblock and endblock 
//  within the limits of the disk size and each thread then performs I/O to the region assigned to.
//
// For method 2, each thread utilizes the same startblock and endblock, and is free to access any sector within the
// limits of the disk.   Currently the only methods that use this method are \
// SIMIO_STREAM_MULTI_THD_TA_RANDOM_LBA_FIXED_LENGTH and SIMIO_STREAM_MULTI_THD_TA_RANDOM_LBA_RANDOM_LENGTH.
//


typedef enum BvdSimIo_stream_type_e {
    SIMIO_STREAM_SEQUENTIAL = 1,                                                 // Each thread assigned a range of LBAs
    SIMIO_STREAM_SEQ_LBA_RANDOM_LENGTH = 1,                                      // Each Thread assigned a range of LBAs
    SIMIO_STREAM_RANDOM_LBA_FIXED_LENGTH,                                        // Each Thread assigned a range of LBAs
    SIMIO_STREAM_SEQ_LBA_FIXED_LENGTH,                                           // Each Thread assigned a range of LBAs
    SIMIO_STREAM_RANDOM_LBA_LENGTH,                                              // Each Thread assigned a range of LBAs
    SIMIO_STREAM_MULTI_THD_TA_RANDOM_LBA_FIXED_LENGTH,                           // All threads run over the entire disk
    SIMIO_STREAM_MULTI_THD_TA_RANDOM_LBA_RANDOM_LENGTH,                          // All threads run over the entire disk
    SIMIO_STREAM_SEQUENTIAL_NO_TAG,                                              // Each Thread assigned a range of LBAs
    SIMIO_STREAM_SEQ_LBA_RANDOM_LENGTH_NO_TAG = SIMIO_STREAM_SEQUENTIAL_NO_TAG,  // Each Thread assigned a range of LBAs
    SIMIO_STREAM_RANDOM_LBA_FIXED_LENGTH_NO_TAG,                                 // Each Thread assigned a range of LBAs
    SIMIO_STREAM_SEQ_LBA_FIXED_LENGTH_NO_TAG,                                    // Each Thread assigned a range of LBAs
    SIMIO_STREAM_RANDOM_LBA_LENGTH_NO_TAG,                                       // Each Thread assigned a range of LBAs
    SIMIO_STREAM_MERGE_FIXED_LENGTH_NO_TAG,                                      // Each Thread assigned a range of LBAs
} BvdSimIo_stream_type_t;

typedef enum BvdSimIo_method_type_e {
    SIMIO_METHOD_NONE = 1,
    SIMIO_METHOD_READFILE,
    SIMIO_METHOD_DCAREADFILE,
    SIMIO_METHOD_DCAREADFILE_LAYOUTDONTCARE,
    SIMIO_METHOD_WRITEFILE,
    SIMIO_METHOD_DCAWRITEFILE,
    SIMIO_METHOD_ZEROFILL,
    SIMIO_METHOD_SGLREADFILE,
    SIMIO_METHOD_SGLWRITEFILE,
    SIMIO_METHOD_READWRITEZERO_MIXED,       // Used with AsychronousConcurrentIO
    SIMIO_METHOD_DCAREADWRITEZERO_MIXED,    // Used with AsychronousConcurrentIO
    SIMIO_METHOD_SGLREADWRITEZERO_MIXED,    // Used with AsychronousConcurrentIO
    SIMIO_METHOD_READWRITE_MIXED,       // Used with AsychronousConcurrentIO
    SIMIO_METHOD_DCAREADWRITE_MIXED,    // Used with AsychronousConcurrentIO
    SIMIO_METHOD_SGLREADWRITE_MIXED,    // Used with AsychronousConcurrentIO

    // For Disparate Write Operations only
    SIMIO_METHOD_DW_WRITE,
    SIMIO_METHOD_DW_DCAWRITE,
    SIMIO_METHOD_DW_ALLWRITES_MIXED,
    SIMIO_METHOD_DW_WRITES_ZERO_MIXED,
    SIMIO_METHOD_DW_WRITES_DISCARD_MIXED,
    SIMIO_METHOD_DW_WRITES_ZERO_DISCARD_MIXED,
    SIMIO_METHOD_DW_DCAWRITES_ZERO_MIXED,
    SIMIO_METHOD_DW_DCAWRITES_DISCARD_MIXED,
    SIMIO_METHOD_DW_DCAWRITES_ZERO_DISCARD_MIXED,
    SIMIO_METHOD_DW_ALLWRITES_ZERO_MIXED,
    SIMIO_METHOD_DW_ALLWRITES_DISCARD_MIXED,
    SIMIO_METHOD_DW_ALLWRITES_ZERO_DISCARD_MIXED,
} BvdSimIo_type_t;

typedef enum IoStyle_e {
        BVD_SIM_IO, 
        DAQ_STYLE_IO,
        NONE
    }IoStyle_t;

#endif
