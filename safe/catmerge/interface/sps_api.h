#ifndef SPS_API_H
#define SPS_API_H

/************************************************************************************
 * Copyright (C) EMC Corporation  2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 * Unpublished - all rights reserved under the copyright laws of the United States
 ***********************************************************************************/

/************************************************************************************
 *  sps_api.h
 ************************************************************************************
 *
 * DESCRIPTION:
 *   This file is the header file that contains typedefs and defines used by
 *   other drivers to interface with the Serial driver controlling the SPS.
 *
 *  HISTORY:
 *      April 6, 2010   - Joe Ash - Created
 *
 ***********************************************************************************/

#ifdef ALAMOSA_WINDOWS_ENV
#include "ntddser.h"
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - NT serial driver only on Windows */

/* Specifics of SPS communication settings.
 */
#define SPS_SERIAL_BAUD                             9600
#define SPS_SERIAL_WIDTH                            8
#define SPS_SERIAL_PARITY                           NO_PARITY
#define SPS_SERIAL_STOP_BITS                        STOP_BITS_2

#define SPS_SERIAL_READ_INTERVAL_TIMEOUT            500
#define SPS_SERIAL_READ_TOTAL_TIMEOUT_CONSTANT      2000
#define SPS_SERIAL_READ_TOTAL_TIMEOUT_MULTIPLIER    10
#define	SPS_SERIAL_WRITE_TOTAL_TIMEOUT_CONSTANT     9500
#define SPS_SERIAL_WRITE_TOTAL_TIMEOUT_MULTIPLIER   2

#define SPS_SERIAL_DEVICE                           "\\Device\\Serial1"
#define SPS_SERIAL_DEVICE_DEBUGGER_ENABLED          "\\Device\\Serial0"


/*  Following is used to build a command to send to the SPS
 */
typedef struct _SPS_SERIAL_CMD
{
    PCHAR           command;        // Actual buffer must be one larger than commandLength to account for null terminator
    ULONG           commandLength;
    PCHAR           buffer;         // Actual buffer must be one larger than bufferLength to account for null terminator
    ULONG           bufferLength;
} SPS_SERIAL_CMD, *PSPS_SERIAL_CMD;

#define SPS_REQUEST_INIT(m_sps_req_p,m_cmd_p,m_cmd_len,m_buf_p,m_buf_len)\
        ((m_sps_req_p)->command         = (m_cmd_p),    \
         (m_sps_req_p)->commandLength   = (m_cmd_len),  \
         (m_sps_req_p)->buffer          = (m_buf_p),    \
         (m_sps_req_p)->bufferLength    = (m_buf_len))

#endif  /* SPS_API_H */

/* End of file  -  sps_api.h */
