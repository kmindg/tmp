/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_sps_mgmt_utils.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the SPS Management utils
 *  code.
 * 
 *  This file contains some common utility functions for sps object management.
 * 
 * @ingroup sps_mgmt_class_files
 * 
 * @version
 *   05-August-2010:  Created. Vishnu Sharma
 *
 ***************************************************************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_sps_info.h"

const char *fbe_sps_mgmt_getSpsFaultString (fbe_sps_fault_info_t *spsFaultInfo)
{
    const char *spsFaultString = "Undefined";

    if (!spsFaultInfo->spsModuleFault && !spsFaultInfo->spsBatteryFault)
    {
        spsFaultString = "NoFault" ;
    }
    else
    {
    
        if (spsFaultInfo->spsModuleFault && !spsFaultInfo->spsBatteryFault)
        {
            if (spsFaultInfo->spsChargerFailure && spsFaultInfo->spsInternalFault)
            {
                spsFaultString = "InternalFlt ChargerFail" ;
            }
            else if (spsFaultInfo->spsChargerFailure)
            {
                spsFaultString = "ChargerFail" ;
            }
            else if (spsFaultInfo->spsInternalFault)
            {
                spsFaultString = "InternalFlt" ;
            }
            else
            {
                spsFaultString = "ModuleFlt" ;
            }
        }
        else if (spsFaultInfo->spsBatteryFault && !spsFaultInfo->spsModuleFault)
        {
            if (spsFaultInfo->spsBatteryEOL)
            {
                spsFaultString = "BatteryEol" ;
            }
            else
            {
                spsFaultString = "BatteryFlt" ;
            }
        }
        else
        {
            spsFaultString = "ModuleAndBatteryFlt" ;
        }
    }

    return spsFaultString;
}


const char *fbe_sps_mgmt_getSpsStatusString (SPS_STATE spsStatus)
{
    const char *spsStatusString = "Undefined";

    switch (spsStatus)
    {
/*
        case FBE_SPS_STATUS_NOT_PRESENT:
            spsStatusString = "NotPresent" ;
            break;
*/
        case SPS_STATE_AVAILABLE:
            spsStatusString = "Available" ;
            break;
        case SPS_STATE_CHARGING:
            spsStatusString = "Charging" ;
            break;
        case SPS_STATE_TESTING:
            spsStatusString = "Testing" ;
            break;
        case SPS_STATE_ON_BATTERY:
            spsStatusString = "BatteryOnline" ;
            break;
        case SPS_STATE_FAULTED:
            spsStatusString = "Faulted" ;
            break;
        case SPS_STATE_UNKNOWN:
            spsStatusString = "Unknown" ;
            break;
        default:
            break;
    }

    return spsStatusString;
}


const char *fbe_sps_mgmt_getSpsCablingStatusString (fbe_sps_cabling_status_t spsCablingStatus)
{
    const char *spsCablingStatusString = "Undefined";

    switch (spsCablingStatus)
    {
        case FBE_SPS_CABLING_VALID:
            spsCablingStatusString = "Valid" ;
            break;
        case FBE_SPS_CABLING_UNKNOWN:
            spsCablingStatusString = "Unknown" ;
            break;
        case FBE_SPS_CABLING_INVALID_PE:
            spsCablingStatusString = "xPE-DPECable" ;
            break;
        case FBE_SPS_CABLING_INVALID_DAE0:
            spsCablingStatusString = "DAE0Cable" ;
            break;
        case FBE_SPS_CABLING_INVALID_SERIAL:
            spsCablingStatusString = "SerialCable" ;
            break;
        case FBE_SPS_CABLING_INVALID_MULTI:
            spsCablingStatusString = "MulitpleCables" ;
            break;
        default:
            break;
    }

    return spsCablingStatusString;
}

const char *fbe_sps_mgmt_getSpsString (fbe_device_physical_location_t *spsLocation)
{
    const char *spsString = "Undefined";

    if ((spsLocation->bus == FBE_XPE_PSEUDO_BUS_NUM) && (spsLocation->enclosure == FBE_XPE_PSEUDO_BUS_NUM))
    {
        if (spsLocation->slot == 0)
        {
            spsString = "PE SPS 0";
        }
        else if (spsLocation->slot == 1)
        {
            spsString = "PE SPS 1";
        }
        else
        {
            spsString = "Invalid SPS slot";
        }
    }
    else if ((spsLocation->bus == 0) && (spsLocation->enclosure == 0))
    {
        if (spsLocation->slot == 0)
        {
            spsString = "0_0 SPS 0";
        }
        else if (spsLocation->slot == 1)
        {
            spsString = "0_0 SPS 1";
        }
        else
        {
            spsString = "Invalid SPS slot";
        }
    }
    else
    {
        spsString = "Invalid SPS slot";
    }

    return spsString;
}

const char *fbe_sps_mgmt_getSpsModelString (SPS_TYPE spsModelType)
{
    const char *spsModelString = "Undefined";
    switch (spsModelType)
    {
        case SPS_TYPE_1_0_KW:
            spsModelString = "1.0 KW";
            break;
        case SPS_TYPE_1_2_KW:
            spsModelString = "1.2 KW";
            break;
        case SPS_TYPE_2_2_KW:
            spsModelString = "2.2 KW";
            break;
        case SPS_TYPE_LI_ION_2_2_KW:
            spsModelString = "2.2 LI-ION";
            break;
        default:
            break;
    }
    return spsModelString;
}

const char *fbe_sps_mgmt_getSpsActionTypeString (fbe_sps_action_type_t spsActionType)
{
    const char *spsActionString = "Undefined";
    switch (spsActionType)
    {
        case FBE_SPS_ACTION_NONE:
            spsActionString = "None";
            break;
        case FBE_SPS_ACTION_START_TEST:
            spsActionString = "START_TEST";
            break;
        case FBE_SPS_ACTION_STOP_TEST:
            spsActionString = "STOP_TEST";
            break;
        case FBE_SPS_ACTION_SHUTDOWN:
            spsActionString = "SHUTDOWN";
            break;
        case FBE_SPS_ACTION_SET_SPS_PRESENT:
            spsActionString = "SET_SPS_PRESENT";
            break;
        case FBE_SPS_ACTION_CLEAR_SPS_PRESENT:
            spsActionString = "CLEAR_SPS_PRESENT";
            break;
        case FBE_SPS_ACTION_ENABLE_SPS_FAST_SWITCHOVER:
            spsActionString = "ENABLE_SPS_FAST_SWITCHOVER";
            break;
        case FBE_SPS_ACTION_DISABLE_SPS_FAST_SWITCHOVER:
            spsActionString = "DISABLE_SPS_FAST_SWITCHOVER";
            break;
        default:
            break;
    }
    return spsActionString;
}


const char *fbe_sps_mgmt_getBobStateString (fbe_battery_status_t bobStatus)
{
    const char *bobStateString = "Undefined";

    switch (bobStatus)
    {
        case FBE_BATTERY_STATUS_REMOVED:
            bobStateString = "BatteryRemoved" ;
            break;
        case FBE_BATTERY_STATUS_BATTERY_READY:
            bobStateString = "BatteryReady" ;
            break;
        case FBE_BATTERY_STATUS_ON_BATTERY:
            bobStateString = "OnBattery" ;
            break;
        case FBE_BATTERY_STATUS_CHARGING:
            bobStateString = "Charging" ;
            break;
        case FBE_BATTERY_STATUS_FULL_CHARGED:
            bobStateString = "FullCharged" ;
            break;
        case FBE_BATTERY_STATUS_UNKNOWN:
            bobStateString = "Unknown" ;
            break;
        case FBE_BATTERY_STATUS_TESTING:
            bobStateString = "Testing" ;
            break;
        default:
            break;
    }

    return bobStateString;
}

const char *fbe_sps_mgmt_getBobFaultString (fbe_battery_fault_t bobFault)
{
    const char *bobFaultString = "Undefined";

    switch (bobFault)
    {
        case FBE_BATTERY_FAULT_NO_FAULT:
            bobFaultString = "NoFault" ;
            break;
        case FBE_BATTERY_FAULT_INTERNAL_FAILURE:
            bobFaultString = "InternalFail" ;
            break;
        case FBE_BATTERY_FAULT_COMM_FAILURE:
            bobFaultString = "CommFail" ;
            break;
        case FBE_BATTERY_FAULT_LOW_CHARGE:
            bobFaultString = "LowCharge" ;
            break;
        case FBE_BATTERY_FAULT_TEST_FAILED:
            bobFaultString = "TestFailed" ;
            break;
        case FBE_BATTERY_FAULT_CANNOT_CHARGE:
            bobFaultString = "CannotCharge" ;
            break;
        case FBE_BATTERY_FAULT_NOT_READY:
            bobFaultString = "Not Ready";
            break;
        case FBE_BATTERY_FAULT_FLT_STATUS_REG:
            bobFaultString = "Flt Status Reg";
            break;
        default:
            break;
    }

    return bobFaultString;
}

const char *fbe_sps_mgmt_getBobTestStatusString (fbe_battery_test_status_t testStatus)
{
    const char *bobTestStatusString = "Undefined";

    switch (testStatus)
    {
        case FBE_BATTERY_TEST_STATUS_NONE:
            bobTestStatusString = "NoTest";
            break;
        case FBE_BATTERY_TEST_STATUS_STARTED:
            bobTestStatusString = "TestStarted";
            break;
        case FBE_BATTERY_TEST_STATUS_COMPLETED:
            bobTestStatusString = "TestCompleted";
            break;
        case FBE_BATTERY_TEST_STATUS_FAILED:
            bobTestStatusString = "TestFailed";
            break;
        case FBE_BATTERY_TEST_STATUS_INSUFFICIENT_LOAD:
            bobTestStatusString = "Insufficient Load";
            break;
        default:
            break;
    }

    return bobTestStatusString;
}

const char *fbe_sps_mgmt_decode_bbu_state(fbe_battery_state_t testStatus)
{
    const char *bobStateString = "UNDEFINED";

    switch (testStatus)
    {
        case FBE_BATTERY_STATE_UNKNOWN:
            bobStateString = "UNKNOWN";
            break;
        case FBE_BATTERY_STATE_OK:
            bobStateString = "OK";
            break;
        case FBE_BATTERY_STATE_DEGRADED:
            bobStateString = "DEGRADED";
            break;
        case FBE_BATTERY_STATE_REMOVED:
            bobStateString = "REMOVED";
            break;
        case FBE_BATTERY_STATE_FAULTED:
            bobStateString = "FAULTED";
            break;
        default:
            break;
    }

    return bobStateString;
}

const char *fbe_sps_mgmt_decode_bbu_subState(fbe_battery_subState_t testStatus)
{
    const char *bobSubStateString = "UNDEFINED";

    switch (testStatus)
    {
        case FBE_BATTERY_SUBSTATE_NO_FAULT:
            bobSubStateString = "NO FAULT";
            break;
        case FBE_BATTERY_SUBSTATE_GEN_FAULT:
            bobSubStateString = "GEN FAULT";
            break;
        case FBE_BATTERY_SUBSTATE_FAN_FAULT:
            bobSubStateString = "FAN FAULT";
            break;
        case FBE_BATTERY_SUBSTATE_UNSUPPORTED:
            bobSubStateString = "UNSUPPORTED";
            break;
        case FBE_BATTERY_SUBSTATE_FLT_STATUS_REG_FAULT:
            bobSubStateString = "FLT REG FAULT";
            break;
        case FBE_BATTERY_SUBSTATE_RP_READ_FAILURE:
            bobSubStateString = "RP READ FAIL";
            break;
        case FBE_BATTERY_SUBSTATE_ENV_INTF_FAILURE:
            bobSubStateString = "ENV INTF FAIL";
            break;
        default:
            break;
    }

    return bobSubStateString;
}

const char *fbe_getWeekDayString (fbe_u16_t   weekday)
{
    const char *weekDayString = "Undefined";
    switch (weekday)
    {
        case 0:
            weekDayString = "Sunday";
            break;
        case 1:
            weekDayString = "Monday";
            break;
        case 2:
            weekDayString = "Tuesday";
            break;
        case 3:
            weekDayString = "Wednesday";
            break;
        case 4:
            weekDayString = "Thursday";
            break;
        case 5:
            weekDayString = "Friday";
            break;
        case 6:
            weekDayString = "Saturday";
            break;
        default:
            break;
    }
    return weekDayString;
}


void fbe_getTimeString (fbe_system_time_t *time, fbe_s8_t *timeString)
{
    fbe_s8_t buffer[3] = {'\0'};
    if(time->hour < 10)
    {
        _snprintf(timeString, 8, "0%d:", time->hour);
    }
    else if(time->hour >= 10 && time->hour < 24)
    {
        _snprintf(timeString, 8, "%d:", time->hour);
    }
    if(time->minute < 10)
    {
        _snprintf(buffer, 3, "0%d", time->minute);
    }
    else if (time->minute >= 10 && time->minute < 60)
    {
        _snprintf(buffer, 3, "%d", time->minute);
    }
    strncat(timeString, buffer, 8-strlen(buffer)-1);
    return;
}

//end of fbe_sps_mgmt_utils.c
