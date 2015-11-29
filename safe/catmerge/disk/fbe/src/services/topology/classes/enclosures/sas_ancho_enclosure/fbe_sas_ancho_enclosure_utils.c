/**********************************************************************
 *  Copyright (C)  EMC Corporation 2014
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_sas_ancho_enclosure_utils.c
 ***************************************************************************
 *
 * @brief
 *  The routines in this file contains all the utility functions for the
 *  sas_ancho enclosure
 *
 * @ingroup fbe_enclosure_class
 *
 * NOTE: 
 *
 * HISTORY
 *   24-Mar-2014:  Created. Jaleel Kazi 
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "sas_ancho_enclosure_private.h"
#include "sas_ancho_enclosure_config.h"
#include "fbe_sas_enclosure_utils.h"
#include "fbe_eses_enclosure_debug.h"


/*!**************************************************************
 * @fn fbe_sas_ancho_enclosure_print_prvt_data(
 *                         void * enclPrvtData, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print private data of sas ancho
 *  enclosure.
 *
 * @param enclPrvtData - pointer to the void.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   24-Mar-2014:  Created. Jaleel Kazi
 *
 ****************************************************************/
void fbe_sas_ancho_enclosure_print_prvt_data(void * enclPrvtData , 
                                             fbe_trace_func_t trace_func, 
                                             fbe_trace_context_t trace_context)
{
    fbe_sas_ancho_enclosure_t * anchoPrvtData = (fbe_sas_ancho_enclosure_t *)enclPrvtData;

    /*print eses private data	*/  
    fbe_eses_enclosure_print_prvt_data((void *)&(anchoPrvtData->eses_enclosure), trace_func, trace_context);	  
}
// End of file fbe_sas_ancho_enclosure_utils.c
