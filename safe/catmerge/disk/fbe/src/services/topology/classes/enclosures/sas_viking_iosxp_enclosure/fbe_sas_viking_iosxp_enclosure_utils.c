/**********************************************************************
 *  Copyright (C)  EMC Corporation 2008
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_sas_viking_enclosure_utils.c
 ***************************************************************************
 *
 * @brief
 *  The routines in this file contain all the utility functions for the  
 *  viking enclosure
 *
 * @ingroup fbe_enclosure_class
 *
 * NOTE: 
 *
 * HISTORY
 *   22-Nov-2013:  Greg Bailey - Created.
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "sas_viking_iosxp_enclosure_private.h"
#include "fbe_eses_enclosure_debug.h"


void fbe_sas_viking_iosxp_enclosure_print_prvt_data(void * pEnclPrvtData,
                                        fbe_trace_func_t trace_func, 
                                        fbe_trace_context_t trace_context)
{
    fbe_sas_viking_iosxp_enclosure_t * pVikingIosxpPrvtData = (fbe_sas_viking_iosxp_enclosure_t *)pEnclPrvtData;
    fbe_eses_enclosure_print_prvt_data((void *)&(pVikingIosxpPrvtData->eses_enclosure), trace_func, trace_context);
}

// End of file fbe_sas_viking_enclosure_utils.c
