#ifndef Data_Movement_Stats_H
#define Data_Movement_Stats_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2000-2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * $Id:$
 ***************************************************************************
 *
 * DESCRIPTION:
 *   This header file contains the structure definition for the DM stats
 *
 * NOTES:
 *
 * HISTORY:
 *      28-Feb-2005 CHH Created
 *
 * $Log:$
 ***************************************************************************/

/*
 * LOCAL TEXT unique_file_id[] = "$Header: $"
 */

/*************************
 * INCLUDE FILES
 *************************/

/*******************************
 * LITERAL DEFINITIONS
 *******************************/

/*********************************
 * STRUCTURE DEFINITIONS
 *********************************/

typedef struct _DM_stats
{
    ULONG total_requests;          // Total number of requests handled
    ULONG successful_dm;           // Number of successful DM copies
    ULONG dca_xfers;               // Number of DCA transfers
    ULONG buffered_copy;           // Number of buffered copies
    ULONG cancelled_requests;      // Number of initiator cancelled requests 
    ULONG deadlocks;               // Number of deadlocks detected.
    ULONGLONG total_bytes;         // Total number of bytes copied
    ULONG buffq_max_waiting;       // Maximum number of requests waiting.
    ULONG buffq_arrivals;          // Number of requests waiting for buffer.
    ULONG buffq_busy;              // Number of non-zero arrivals for buffer.
    INT_32 active_req_count;       // Count of active DM requests
    ULONG active_req_count_HWM;    // Highest number of active requests seen.
    ULONGLONG bad_blocks_copied;   // Number of bad blocks marked on destination
    ULONG unaligned_requests;      // Number of unaligned requests.
} DM_stats, *PDM_stats;

//
//  The following defines the number of histogram buckets to record the response
//  time.  1 << 28 == 1099511627776 usec == 1099511+ sec == 305+ hours.  Should
//  be enough resolution.
//
#define DM_BULK_STATS_RESPONSE_TIME_HISTOGRAM_MAX                  28


//
//  The following defines the number of histogram buckets to record the size of
//  bulk requests.
//
#define DM_BULK_STATS_REQUEST_SIZE_HISTOGRAM_MAX                   32

// The maximum number of SGL elements allowed by a DM bulk request.
#define DM_MAX_BULK_SGL_ELEMENTS                                   32

typedef struct _DM_bulk_stats
{
    UINT_64E frequency;                     // The system's clock frequency
    UINT_64 max_request_response_time_usec; // HWM for bulk requests
    ULONG64 request_copy_rt_histogram[DM_BULK_STATS_RESPONSE_TIME_HISTOGRAM_MAX];
                                            // Histogram of response times for
                                            // bulk requests.
    ULONG64 request_zero_rt_histogram[DM_BULK_STATS_RESPONSE_TIME_HISTOGRAM_MAX];
                                            // Histogram of response times for
                                            // bulk zero requests.
    UINT_64 max_piece_copy_response_time_usec;// HWM for bulk requests    
    ULONG64 piece_copy_rt_histogram[DM_BULK_STATS_RESPONSE_TIME_HISTOGRAM_MAX];
                                            // Histogram of response times for
                                            // bulk request pieces.
    UINT_64 max_piece_zero_response_time_usec;// HWM for bulk zero pieces
    ULONG64 piece_zero_rt_histogram[DM_BULK_STATS_RESPONSE_TIME_HISTOGRAM_MAX];
                                            // Histogram of response times for
                                            // bulk zero pieces.
    ULONG cancelled_requests;               // Number of initiator cancelled requests 
    ULONG total_zf_requests;                // Total number of zero fill requets
    ULONG total_bulk_requests;              // Total number of zero fill requets
    ULONGLONG total_bytes_copied;           // Total number of bytes copied
    ULONGLONG total_bytes_zeroed;           // Total number of bytes zeroed
    ULONGLONG total_copy_pieces;            // Total number of copy pieces
    ULONGLONG total_zero_pieces;            // Total number of zero pieces
    INT_32 active_bulk_copy_req_count;      // Count of active DM bulk requests
    INT_32 active_bulk_zero_req_count;      // Count of active ZF requests
    UINT_64 src_sgl_copy_histogram[DM_MAX_BULK_SGL_ELEMENTS];
                                            // Histogram for number of src elements
    UINT_64 dst_sgl_copy_histogram[DM_MAX_BULK_SGL_ELEMENTS];
                                            // Histogram for number of dst elements
    UINT_64 src_len_copy_histogram[DM_BULK_STATS_REQUEST_SIZE_HISTOGRAM_MAX];
                                            // Histogram for src size length        
    UINT_64 dst_len_copy_histogram[DM_BULK_STATS_REQUEST_SIZE_HISTOGRAM_MAX];
                                            // Histogram for dst size length
	UINT_64 dst_sgl_zero_histogram[DM_MAX_BULK_SGL_ELEMENTS];
	                                        // Histogram for number of dst elements
    UINT_64 src_len_zero_histogram[DM_BULK_STATS_REQUEST_SIZE_HISTOGRAM_MAX];
                                            // Histogram for src size length        
    UINT_64 dst_len_zero_histogram[DM_BULK_STATS_REQUEST_SIZE_HISTOGRAM_MAX];
                                            // Histogram for dst size length
} DM_bulk_stats, *PDM_bulk_stats;

/*********************************
 * EXTERNAL REFERENCE DEFINITIONS
 *********************************/

/************************
 * PROTOTYPE DEFINITIONS
 ************************/

/*
 * End $Id:$
 */

#endif
