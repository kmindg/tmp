/**********************************************************************
 *  Copyright (C)  EMC Corporation 2008
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_sas_derringer_enclosure_utils.c
 ***************************************************************************
 *
 * @brief
 *  The routines in this file contains all the utility functions for the  
 *  sas_derringer enclosure
 *
 * @ingroup fbe_enclosure_class
 *
 * NOTE: 
 *
 * HISTORY
 *   12-July-2009 Dipak Patel - Created. 
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "sas_derringer_enclosure_private.h"
#include "fbe_sas_enclosure_utils.h"
#include "fbe_eses_enclosure_debug.h"


void fbe_sas_derringer_enclosure_print_prvt_data(void * enclPrvtData,
                                        fbe_trace_func_t trace_func, 
                                        fbe_trace_context_t trace_context)
{
    fbe_sas_derringer_enclosure_t * derringerPrvtData = (fbe_sas_derringer_enclosure_t *)enclPrvtData;
    fbe_eses_enclosure_print_prvt_data((void *)&(derringerPrvtData->eses_enclosure), trace_func, trace_context);
}
// End of file fbe_sas_derringer_enclosure_utils.c
