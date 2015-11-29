/***************************************************************************
 * Copyright (C) EMC Corporation 2007
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_log_private.h
 ***************************************************************************
 *
 * DESCRIPTION: MUT Framework logging
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    09/18/2007   Buckley, Martin  initial version
 *
 **************************************************************************/

#ifndef MUT_LOG_PRIVATE_H
#define MUT_LOG_PRIVATE_H
#include "mut_testsuite.h"
#include "mut_test_entry.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "stdarg.h"

/** \def TIMESTAMP_SIZEOF
 *  \details
 *  This macro defines the size of the buffer for time stamp
 */
#define TIMESTAMP_SIZEOF 32

/** \def DATESTAMP_SIZEOF
 *  \details
 *  This macro defines the size of the buffer for date stamp
 */
#define DATESTAMP_SIZEOF 32

/** \def MSG_BUFFER_SIZEOF
 *  \details
 *  This macro defines the size of the buffer for log message
 */
#define MSG_BUFFER_SIZEOF 2048

/** \def TOKEN_SEP
 *  \details
 *  This macro defines the token separator for multiline message in log
 */
#define TOKEN_SEP "\n"



/** \enum mut_log_type_e
 *  \details
 *  Log can be either in plain text (MUT_LOG_TEXT) or in xml (MUT_LOG_XML)
 *  if -xml command option was used.
 *
 *  NOTE, MUT_LOG_TEXT and MUT_LOG_XML are used as index into array
 *  mut_log_format_spec_t.format.
 *    As such, there Text needs a value of 0 and XML needs a value of 1
 */
typedef enum mut_log_type_e {
    MUT_LOG_TEXT = 0,
    MUT_LOG_XML  = 1,
    MUT_LOG_NONE = 2,
    MUT_LOG_BOTH = 4
} mut_log_type_t;


/** \enum mut_log_routing_e
 *
 *
 */
typedef enum mut_log_routing_e {
    MUT_CONSOLE_OUTPUT_ENABLED  = 1,
    MUT_TEXT_LOG_OUTPUT_ENABLED = 2,
    MUT_XML_LOG_OUTPUT_ENABLED  = 4,
    MUT_CONSOLE_AND_TEXT_LOG_OUTPUT_ENABLED = 3,
    MUT_CONSOLE_AND_XML_LOG_OUTPUT_ENABLED  = 5,
    MUT_ALL_OUTPUT_ENABLED      = 7
} mut_log_routing_t;


extern char console_suite_summary[MSG_BUFFER_SIZEOF*4];

#ifdef __cplusplus
}
#endif
#endif /* MUT_LOG_PRIVATE_H */
