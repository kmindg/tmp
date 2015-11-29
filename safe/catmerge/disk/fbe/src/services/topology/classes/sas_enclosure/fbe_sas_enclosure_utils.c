/**********************************************************************
 *  Copyright (C)  EMC Corporation 2008
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_sas_enclosure_utils.c
 ***************************************************************************
 *
 * @brief
 *  The routines in this file contains all the utility functions for the
 *  sas enclosure.
 *
 * @ingroup fbe_enclosure_class
 *
 * NOTE: 
 *
 * HISTORY
 *   07-Apr-2009 Dipak Patel - Created. 
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "fbe_sas_enclosure_debug.h"
#include "fbe_eses_enclosure_debug.h"
#include "fbe_base_enclosure_debug.h"
#include"sas_enclosure_private.h"

/*!**************************************************************
 * @fn fbe_sas_enclosure_print_prvt_data(
 *                         void * enclPrvtData, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print private data of sas
 *  enclosure.
 *
 * @param enclPrvtData - pointer to the void.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   04/02/2009:  Created. Dipak Patel
 *
 ****************************************************************/ 
void fbe_sas_enclosure_print_prvt_data(void * enclPrvtData,
	                                                                       fbe_trace_func_t trace_func,
	                                                                       fbe_trace_context_t trace_context)
{

    fbe_sas_enclosure_t * sasPrvtData = (fbe_sas_enclosure_t *)enclPrvtData;

    trace_func(trace_context, "\nSAS Private Data:\n");
	
    fbe_sas_enclosure_print_sasEnclosureSMPAddress(sasPrvtData->sasEnclosureSMPAddress, trace_func, trace_context);
    fbe_sas_enclosure_print_smp_address_generation_code(sasPrvtData->smp_address_generation_code, trace_func, trace_context);
    fbe_sas_enclosure_print_ses_port_address(sasPrvtData->ses_port_address, trace_func, trace_context);
    fbe_sas_enclosure_print_ses_port_address_generation_code(sasPrvtData->ses_port_address_generation_code, trace_func, trace_context);
    fbe_sas_enclosure_print_sasEnclosureProductType(sasPrvtData->sasEnclosureProductType, trace_func, trace_context);

    /* print base private data */	
    fbe_base_enclosure_print_prvt_data((void *)&(sasPrvtData->base_enclosure), trace_func, trace_context);

}


// End of file fbe_eses_enclosure_utils.c
