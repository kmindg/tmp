#ifndef SPS_MICR0_H
#define SPS_MICR0_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/****************************************************************
 *  SPS_micro.h
 ****************************************************************
 *
 *  Description:
 *  This header file contains structures which define
 *  the format of the communication with the SPS (Standby Power Supply)
 *  microcontroller, this communication happens over serial.
 *
 *  History:
 *      Apr-01-2010     Joe Ash - Created
 *
 ****************************************************************/


#include "generic_types.h"

typedef enum _SPS_COMMAND
{
    SPS_INVALID_COMMAND,

    /* SPS Commands */
    SPS_CONDITION_COMMAND,
    SPS_STATUS_COMMAND,
    SPS_BAT_TEST_COMMAND,
    SPS_STOP_COMMAND,
    SPS_MANUFACTURER_COMMAND,
    SPS_PART_NUM_COMMAND,
    SPS_REVISION_COMMAND,
    SPS_SERIAL_NUM_COMMAND,
    SPS_BAT_REP_DATE_COMMAND,
    SPS_BAT_TIME_COMMAND,
    SPS_TIME_COMMAND,
    SPS_TIME_OF_YEAR_COMMAND,
    SPS_DESCRIPTION_COMMAND,
    SPS_TRIP_AC_BREAKER_COMMAND,
    SPS_LATCH_OFF_COMMAND,
    SPS_LATCH_RESET_COMMAND,
    SPS_FAULT_TEST_COMMAND,

    /*New commands for 2.2kw Li-ion SPS*/
    SPS_LI_ION_2_2_KW_COMMAND_START,
    SPS_FFID_COMMAND = SPS_LI_ION_2_2_KW_COMMAND_START,
    SPS_MODE_COMMAND,
    SPS_SW_TIME_COMMAND,
    SPS_CABLE_TEST_COMMAND,
    SPS_FIRWMARE_DOWNLOAD_COMMAND,
    SPS_PRIMARY_FW_DL_COMMAND,
    SPS_SECONDARY_FW_DL_COMMAND,
    SPS_BATTERY_FW_DL_COMMAND,

    /* Below commands are specific to 2.2 kw Li-ion battery pack FRU.*/
    SPS_B_COMMAND_START,
    SPS_B_FFID_COMMAND = SPS_B_COMMAND_START,
    SPS_B_HW_VERSION_COMMAND,
    SPS_B_MANUFACTURE_COMMAND,
    SPS_B_MANUFACTURE_TIME_COMMAND,
    SPS_B_SN_COMMAND,
    SPS_B_PN_COMMAND,
    SPS_B_REVISION_COMMAND,
    SPS_B_TIME_COMMAND,
    SPS_B_PACK_COMMAND,
    SPS_B_TEMP_COMMAND,
    SPS_B_NOMINAL_CAP_COMMAND,
    SPS_B_REMAINING_CAP_COMMAND,
    SPS_B_BMS_CONDITION1_COMMAND,
    SPS_B_BMS_CONDITION2_COMMAND,
    SPS_B_BOOT_LOAD_COMMAND,
    SPS_B_COMMAND_END             = SPS_B_BOOT_LOAD_COMMAND,
    SPS_LI_ION_2_2_KW_COMMAND_END = SPS_B_COMMAND_END,
    /*End of 2.2 kw Li-ion specific commands. */

    SPS_TOTAL_COMMAND,
} SPS_COMMAND;

#if defined(__cplusplus)
/* use extern C++ here to trick other C++ files including this as extern C
 */
extern "C++"{
/* Define a postfix increment operator for _SPS_COMMAND
 */
inline _SPS_COMMAND operator++( SPS_COMMAND &command_id, int )
{
   return command_id = (SPS_COMMAND)((int)command_id + 1);
}
}
#endif

/* Virtual devices for logical polling
 */
typedef enum _SPS_DEV
{
    SPS_DEV_INVALID,

    /* SPS Commands */
    SPS_CONTROLLER,
    SPS_RESUME,
    SPS_BATTERY_CONTROLLER,
    SPS_BATTERY_RESUME,

    TOTAL_SPS_DEV,
} SPS_DEV;

#if defined(__cplusplus)
/* use extern C++ here to trick other C++ files including this as extern C
 */
extern "C++"{
/* Define a postfix increment operator for _SPS_DEV
 */
inline _SPS_DEV operator++( SPS_DEV &dev_id, int )
{
   return dev_id = (SPS_DEV)((int)dev_id + 1);
}
}
#endif

/* SPS battery command
 */
#define SPS_BAT_COMMAND_TERMINATOR  '\r'

/* SPS responses
 */
#define SPS_COMMAND_TERMINATOR  '\n'

#define SPS_NACK                '?'

#define SPS_BATTEST_PASS        "PASS"
#define SPS_BATTEST_FAIL        "FAIL"
#define SPS_BATTEST_TESTING     "TESTING"

#define SPS_MANUFACTURER_DELIM  ','

#define SPS_DESCRIPT_1_0_KW     "1000"
#define SPS_DESCRIPT_1_2_KW     "1200"
#define SPS_DESCRIPT_2_2_KW     "2200"

#define SPS_2_2_KW_WITHOUT_BATTERY_FFID     "0E08"
#define SPS_2_2_KW_BATTERY_PACK_FFID_082    "0E09"
#define SPS_2_2_KW_BATTERY_PACK_FFID_115    "0E0D"
#define SPS_PROGRAM_TIMEOUT_VALUE           300    /* Seconds */
#define SPS_MAX_BATTIME                     600
#define SPS_ISP_EXIT_COMMAND_STRING        "ISP\n00\n" /* command used to activate downloaded firmware and exit ISP mode.*/

/* XMODEM-CRC definitions
 */
#define SPS_XMODEM_PACKET_SIZE  128
#define SPS_XMODEM_SOH          0x01  /* 0x01 for 128 byte protocol.*/
#define SPS_XMODEM_ETX          0x03  /* 0x03 for 1024 byte protocol XMODEM-1K/YMODEM. Does the SPS support this?*/
#define SPS_XMODEM_EOT          0x04
#define SPS_XMODEM_EOB          0x17 
#define SPS_XMODEM_ACK          0x06 
#define SPS_XMODEM_NAK          0x15 
#define SPS_XMODEM_CAN          0x18 
#define SPS_XMODEM_C            0x43 /* 'C' */ 
#define SPS_XMODEM_SUB          0x1a /*CPMEOF ^Z */

/* Command Response Formats.
 *
 * COND
 *  Response in the format of five ASCII characters, a decimal value
 *  representing the two-byte binary value in the CONDITION register.
 *
 * BATTEST
 *  Response is one of the three values described above, describing the results
 *  of the most recent battery test.
 *
 * MFG
 *  Returns an ASCII string, up to 37 characters long,
 *   with the following information: 
 *      Field Name              Description/Syntax
 *      Supplier Name           Name of manufacturer of the queried unit
 *      ,                       Delimiter for field separation
 *      Supplier Model Number   Model number of queried unit
 *      ,                       Delimiter for field separation
 *      Firmware Version        Revision of firmware as XX.XX
 *      ,                       Delimiter for field separation
 *      Firmware Date           Date of firmware release, as mm/dd/yyyy
 *      <0Ah>                   End of line termination
 *
 * PARTNUM
 *  Responds with the information provided from a previous PARTNUM command.
 *  Normally either 9 numerical characters (original DGC style part number),
 *  or the standard 11 character EMC “3-3-3” format.
 *
 * REV
 *  Responds with the information provided from a previous REVISION command.
 *  Normally, the information is 3 characters; following the EMC “Axx” format.
 *
 * SN
 *  Responds with the information provided from a previous SERIAL command.
 *  Normally, the information is 14 alphanumeric characters long; following the
 *  EMC serial number format.  The format for the serial number is as follows:
 *      VVV     Assigned Manufacturer’s ID Code
 *      PP      Assigned Product Code
 *      YYMM    Year and Month of Manufacture
 *      #####   5-digit unique sequence for YYMM
 *
 * CD
 *  Responds with the date of the most recent battery installation, or battery
 *  change date, that was stored in the SPS EEPROM.
 *  The format of the response will be in the MM/DD/YY format (Month, Day, Year)
 *
 * BATTIME
 *  Responds with a positive, three digit, integer representing the available
 *  On-Battery time in seconds. Responses can be from 000 - 600.
 *
 * VALUE
 *  Responds with the current TOY value, or nacks if not in enhanced mode.
 *  TOY is in the format of YYWW, Year and Week.
 *
 * DESCRIPT
 *  Responds with the the wattage capability of the SPS as expressed with 4
 *  numeric characters, with space for 4 additional characters in the future.
 */

#pragma pack(push, SPS_micro, 1)

typedef struct _SPS_FAULT_REGISTER_GEN1
{
    UINT_16E    on_battery:1,           //common
                battery_available:1,    //bit 1
                battery_end_of_life:1,  //common
                charger_failure:1,      //common
                reserved1:3,
                testing:1,              //common
                internal_fault:1,       //common
                reserved2:7;
} SPS_FAULT_REGISTER_GEN1;

typedef struct _SPS_FAULT_REGISTER_GEN2
{
    UINT_16E    on_battery:1,           //common
                reserved1:1,
                battery_end_of_life:1,  //common
                charger_failure:1,      //common
                reserved2:1,
                latched_stop:1,         //common with GEN3
                reserved3:1,
                testing:1,              //common
                internal_fault:1,       //common
                battery_available:1,    //common with GEN3
                enhanced_mode:1,        //common with GEN3
                reserved4:5;
} SPS_FAULT_REGISTER_GEN2;

typedef struct _SPS_FAULT_REGISTER_GEN3
{
    UINT_16E    on_battery:1,           //common
                reserved1:1,
                battery_end_of_life:1,  //common
                charger_failure:1,      //common
                reserved2:1,
                latched_stop:1,         //common with GEN2
                shutdown:1,             //new for Li-ion 2.2kw
                testing:1,              //common
                internal_fault:1,       //common
                battery_available:1,    //common with GEN2
                enhanced_mode:1,        //common with GEN2
                cable_connectivity:1,        //new for Li-ion 2.2kw
                battery_not_engaged_fault:1, //new for Li-ion 2.2kw
                reserved3:3;
} SPS_FAULT_REGISTER_GEN3;

typedef struct _SPS_FAULT_REGISTER {
    union {
        SPS_FAULT_REGISTER_GEN1 gen1;
        SPS_FAULT_REGISTER_GEN2 gen2;
        SPS_FAULT_REGISTER_GEN3 gen3;
        UINT_16E reg;
    } generation;
} SPS_FAULT_REGISTER;

typedef struct
{
    UINT_16E    charger_voltage_present:1,
                discharging:1,
                reserved:1,
                full_charged:1,
                fan_request:1,
                discharge_stop:1,
                charge_stop:1,
                over_temperature_warning:1,
                cell_over_voltage_warning:1,
                cell_under_voltage_warning:1,
                over_temperature_fault:1,
                cell_over_voltage_fault:1,
                cell_under_voltage_fault:1,
                replace_fuse:1,
                check_battery:1,
                replace_battery:1;
}SPS_BMS_REGISTER_STATE1, *PSPS_BMS_REGISTER_STATE1;

typedef struct
{
    UINT_16E    installed1:1,
                installed2:1,
                discharge_stop_n_2:1,
                internal_fault:1,
                reserved2:11,
                battery_ready:1;
}SPS_BMS_REGISTER_STATE2, *PSPS_BMS_REGISTER_STATE2;

typedef struct _SPS_BMS_REGISTER
{
    union 
    {
        UINT_16E                state1;
        SPS_BMS_REGISTER_STATE1 fields1;
    };
    union 
    {
        UINT_16E                state2;
        SPS_BMS_REGISTER_STATE2 fields2;
    };
} SPS_BMS_REGISTER;

typedef enum _SPS_FW_TYPE
{
    INVALID_FW_TYPE      = 0,
    SECONDARY_FW_TYPE    = 1,
    PRIMARY_FW_TYPE      = 2,
    BATTERY_PACK_FW_TYPE = 3,
    BATTERY_PACK_BOOTLOADER_TYPE = 4,
    UNKNOWN_FW_TYPE      = 0xff,
} SPS_FW_TYPE;

/* SPS operating mode 
*/
typedef enum _SPS_MODE
{
    DEFAULT_LEGACY  = 0,  /* Default: Legacy linear approach. */
    REALTIME        = 1,  /* Real-time, more accurate reporting.*/
    LEGACY_NO_BMS   = 2,  /* Legacy linear approach but BMS info is NOT available to host.*/
    REALTIME_NO_BMS = 3,  /* Real-time, more accurate reporting but BMS info is NOT available to host.*/
    INVALID_MODE    = 0xff,
} SPS_MODE;

#pragma pack(pop, SPS_micro)

#endif //SPS_MICR0_H
