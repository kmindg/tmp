/**********************************************************************
 *  Copyright (C)  EMC Corporation 2011
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_sas_boxwood_enclosure_utils.c
 ***************************************************************************
 *
 * @brief
 *  The routines in this file contains all the utility functions for the  
 *  sas_boxwood enclosure
 *
 * @ingroup fbe_enclosure_class
 *
 * NOTE: 
 *
 * HISTORY
 *   02-Nov-2011 Created from fbe_sas_bunker_enclosure_print_prvt_data - Chang Rui
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "sas_boxwood_enclosure_private.h"
#include "fbe_sas_enclosure_utils.h"
#include "fbe_eses_enclosure_debug.h"


void fbe_sas_boxwood_enclosure_print_prvt_data(void * enclPrvtData,
                                        fbe_trace_func_t trace_func, 
                                        fbe_trace_context_t trace_context)
{
    fbe_sas_boxwood_enclosure_t * boxwoodPrvtData = (fbe_sas_boxwood_enclosure_t *)enclPrvtData;
    fbe_eses_enclosure_print_prvt_data((void *)&(boxwoodPrvtData->eses_enclosure), trace_func, trace_context);
}
// End of file fbe_sas_boxwood_enclosure_utils.c
