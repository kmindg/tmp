/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

#include "fbe/fbe_types.h"
#include "spid_kernel_interface.h"

/*!***************************************************************
 * sep_set_degraded_mode_in_nvram()
 ****************************************************************
 * @brief
 *  This function is called when SEP is in degraded mode.
 *
 * @param void
 * 
 * @return fbe_status - Status for sep_set_degraded_mode_in_nvram.
 *
 * @author
 *  13-June-2012: Created. Zhipeng Hu
 *
 ****************************************************************/
fbe_status_t sep_set_degraded_mode_in_nvram(void)
{
    spidSetForceDegradedMode();
    return FBE_STATUS_OK;
}

