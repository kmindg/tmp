/**********************************************************************
 *  Copyright (C)  EMC Corporation 2008
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_sas_viper_enclosure_utils.c
 ***************************************************************************
 *
 * @brief
 *  The routines in this file contains all the utility functions for the
 *  sas_viper enclosure
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
#include "sas_viper_enclosure_private.h"
#include "sas_viper_enclosure_config.h"
#include "fbe_sas_enclosure_utils.h"
#include "fbe_eses_enclosure_debug.h"


/*!**************************************************************
 * @fn fbe_sas_viper_enclosure_print_prvt_data(
 *                         void * enclPrvtData, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print private data of sas viper
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
void fbe_sas_viper_enclosure_print_prvt_data(void * enclPrvtData , 
                                                                                       fbe_trace_func_t trace_func, 
                                                                                       fbe_trace_context_t trace_context)
{
    fbe_sas_viper_enclosure_t * viperPrvtData = (fbe_sas_viper_enclosure_t *)enclPrvtData;

    /*print eses private data	*/  
    fbe_eses_enclosure_print_prvt_data((void *)&(viperPrvtData->eses_enclosure), trace_func, trace_context);	  
}
// End of file fbe_sas_viper_enclosure_utils.c
