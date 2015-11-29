/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_data_pattern_print.c
 ***************************************************************************
 *
 * @brief
 *  This file contains data pattern sg list functions.
 *
 * @version
 *   8/30/2010:  Created. Swati Fursule
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_data_pattern.h"
#include "fbe/fbe_xor_api.h"
#include "fbe_trace.h"
#include "fbe_base.h"
/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!**************************************************************
 * fbe_data_pattern_trace()
 ****************************************************************
 * @brief
 *  Trace function for use by data pattern library.
 *  Takes into account the current global trace level and
 *  the locally set trace level.
 *
 * @param trace_level - trace level of this message.
 * @param message_id - generic identifier.
 * @param fmt... - variable argument list starting with format.
 *
 * @return None.  
 *
 * @author
 *  8/30/2010 - Created. Swati Fursule
 *
 ****************************************************************/

void fbe_data_pattern_trace(fbe_trace_level_t trace_level,
                         fbe_trace_message_id_t message_id,
                         const char * fmt, ...)
{
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t service_trace_level;

    va_list args;

    /* Assume we are using the default trace level.
     */
    service_trace_level = default_trace_level = fbe_trace_get_default_trace_level();
    /* If the service trace level passed in is greater than the
     * current chosen trace level then just return.
     */
    if (trace_level > service_trace_level) 
    {
        return;
    }

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_LIBRARY,
                     FBE_LIBRARY_ID_DATA_PATTERN,
                     trace_level,
                     message_id,
                     fmt, 
                     args);
    va_end(args);
    return;
}
/******************************************
 * end fbe_data_pattern_trace()
 ******************************************/


/*************************
 * end file fbe_data_pattern_print.c
 *************************/

