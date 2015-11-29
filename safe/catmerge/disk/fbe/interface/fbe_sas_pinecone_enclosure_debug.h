#ifndef FBE_SAS_PINECONE_ENCLOSURE_DEBUG_H
#define FBE_SAS_PINECONE_ENCLOSURE_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_sas_pinecone_enclosure_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the sas pinecone enclosure debug library.
 *
 * @author
 *   4-Feb-2010 Created Rahul Pradhan
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_trace.h"
#include "fbe/fbe_dbgext.h"

/*************************
 *   FUNCTION DECLARATION
 *************************/

fbe_status_t fbe_sas_pinecone_enclosure_debug_prvt_data(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr sas_pinecone_enclosure_p,
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context);

#endif /* FBE_SAS_PINECONE_ENCLOSURE_DEBUG_H */

/*************************
 * end file fbe_sas_pinecone_enclosure_debug.h
 *************************/
