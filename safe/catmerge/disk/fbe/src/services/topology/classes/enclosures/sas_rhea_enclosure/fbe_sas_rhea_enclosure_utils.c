/**********************************************************************
 *  Copyright (C)  EMC Corporation 2014
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_sas_rhea_enclosure_utils.c
 ***************************************************************************
 *
 * @brief
 *  The routines in this file contains all the utility functions for the  
 *  sas_rhea enclosure
 *
 * @ingroup fbe_enclosure_class
 *
 * NOTE: 
 *
 * HISTORY
 *   03/28/2014: Joe Perry - Created from fbe_sas_bunker_enclosure_utils.c
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "sas_rhea_enclosure_private.h"
#include "fbe_sas_enclosure_utils.h"
#include "fbe_eses_enclosure_debug.h"


void fbe_sas_rhea_enclosure_print_prvt_data(void * enclPrvtData,
                                        fbe_trace_func_t trace_func, 
                                        fbe_trace_context_t trace_context)
{
    fbe_sas_rhea_enclosure_t * rheaPrvtData = (fbe_sas_rhea_enclosure_t *)enclPrvtData;
    fbe_eses_enclosure_print_prvt_data((void *)&(rheaPrvtData->eses_enclosure), trace_func, trace_context);
}
// End of file fbe_sas_rhea_enclosure_utils.c
