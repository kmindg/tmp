
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * NAME:
 *      speclcli_env_stats
 *
 * DESCRIPTION:
 *      This file contains miscellaneous utilities for the speclcli
 *
 * NOTES:
 *      None
 *
 **************************************************************************/


/************************
 * INCLUDE FILES
 ************************/

#ifdef C4_INTEGRATED

#include <windows.h>
#include "csx_ext.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include "specl_types.h"
#include "specl_interface.h"
#include "swap_exports.h"
#include "generic_types.h"
#include "generic_utils.h"
#include "generic_utils_lib.h"
#include "fbe_envstats.h"

BOOL getSpTemp (Temperature * temp)     
{


    SPECL_TEMPERATURE_SUMMARY   temperatureStatus;
    PSPECL_TEMPERATURE_STATUS   pTemperatureInfo;
    BOOL                        negativeTemp = FALSE;

    if (speclGetTemperatureStatus(&temperatureStatus) != 0x0)
    {
        return (FALSE);
    }
    pTemperatureInfo = &(temperatureStatus.temperatureStatus[0]);
    if (pTemperatureInfo->transactionStatus == 0x0)
    {
        if ((pTemperatureInfo->TemperatureMSB & BIT7) == BIT7)
        {
            /* The temperature is negative, indicate this and fix the
             * twos compliment number so its positive and correct.
             */
            negativeTemp = TRUE;
            pTemperatureInfo->TemperatureMSB = (~(pTemperatureInfo->TemperatureMSB) + 1);
	}
    }
    temp->negativeTemp = negativeTemp;
    temp->TemperatureMSB = pTemperatureInfo->TemperatureMSB;
    temp->TemperatureLSB = pTemperatureInfo->TemperatureLSB;

    return (TRUE);
}
#endif



