#ifndef RAID_CALC_CLI_H
#define RAID_CALC_CLI_H
/**********************************************************************
 * Copyright (C) EMC Corporation 2006-2007
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 **********************************************************************/

/********************************************************************
 * RaidCalcCli.h
 ********************************************************************
 *
 * DESCRIPTION:
 *   This file contains the macro declaration, function prototypes 
 *   and global variables used by the CLI version of the Raid Calculator.
 *
 * Table of Contents:
 *
 * HISTORY:
 *   27-Mar-07:  Created by Pradnya Patankar
 *
 *********************************************************************/

/*************************
 * INCLUDE FILES
 *************************/

#include "fbe/fbe_raid_calc.h"
#include "generic_types.h"

/*******************************
 * MACRO DECLARATION
 *******************************/

/* It is the default sectors per element. 
 * Its value should be 128.
 */
#define DEFAULT_SECTORS_PER_ELEMENT 128

/* Defines the default array width for different RAID type.
 */
#define DEFAULT_ARRAY_WIDTH_RAID6 10
#define DEFAULT_ARRAY_WIDTH_RAID1 2
#define DEFAULT_ARRAY_WIDTH_ID 1
#define DEFAULT_ARRAY_WIDTH_RAID3_1 5 
#define DEFAULT_ARRAY_WIDTH_RAID3_2 9
#define DEFAULT_ARRAY_WIDTH_RAID10 2

/* Defines the maximum and minimum array width for the Raid 0,
 * Raid 5 and Raid 6.
 */
#define MIN_ARRAY_WIDTH 3
#define MAX_ARRAY_WIDTH 16

/* Maximum element size.
 */
#define MAX_ELSZ 0xffffffff

/* Maximum value of LBA.
 */
#define MAX_LBA RAID_CALC_LBA_MAX

/* Maximum value of PBA.
 */
#define MAX_PBA RAID_CALC_LBA_MAX

/* Maxumum value of the address offset.
 */
#define MAX_ADDR_OFFSET RAID_CALC_LBA_MAX

/* It is the size of the raid type string array.
 */
#define RAID_T_STR_ARR_SIZE 8

/* It is the raid type string size.
 */
#define RAID_TYPE_STR_SIZE 10

/* read_and_validate_input() validates the input string
 * and if an input string is invalid, then it return 
 * 0xFFFFffffFFFFffff for the invalid input.
 */
#define INVALID_INPUT CSX_CONST_U64(0xFFFFffffFFFFffff)

/* It is the size of the array used to read the input string.
 */
#define INPUT_ARRAY_SIZE 100

/* It is the size of the array used to read the -output(HEX/DEC) string.
 */
#define OUTPUT_STR_SIZE 5

/* Checks whether given character is valid hex or not.
 */
#define CHAR_HEX_VALID(InChar)          \
 ((InChar >= '0' && InChar <= '9') ||   \
 (InChar >= 'a' && InChar <= 'f') ||    \
 (InChar >= 'A' && InChar <= 'F'))

/* The maximum bytes for the input is 4.
 * Therefore the maximum string length will be 8.
 */
#define INPUT_MAX_STRLEN 16

/* It is size of the string which is passed to the hex_to_long().
 */
#define BUFFER_SIZE 1024

/* Minimum value of the element size and default_elemenat_per_parity.
 */
#define MIN_VALUE 1

typedef enum {
    LOG_TO_PHYS,
    PHYS_TO_LOG
} CALC_FLAG;
static CALC_FLAG calc_flag;

void out_hex(RAID_CALC_STRUCT& CalcStruct, CALC_FLAG& calc_flag );
void out_dec(RAID_CALC_STRUCT& CalcStruct, CALC_FLAG& calc_flag );
void display_help();
void calc_logical_to_physical(RAID_CALC_STRUCT& CalcStruct);
void calc_physical_to_logical(RAID_CALC_STRUCT& CalcStruct);
void determine_raid_type(PCHAR raid_t,RAID_CALC_STRUCT& CalcStruct);
void calculate_elements_per_parity_stripe(RAID_CALC_STRUCT& CalcStruct);
BOOL validate_input(CONST PCHAR string);
ULONGLONG read_and_validate_input(PCHAR argv);
ULONGLONG hex_to_long(CONST PCHAR String);
extern void RaidCalcCli_I(void);


//**************************************************
// Global Variables
//**************************************************
/* Structure is filled with the input values.
 */
static RAID_CALC_STRUCT CalcStruct;

/* It is the default sectors per element.
 */
static UINT_32 default_sectors = 128;

/* Flag to display the ouput in the specific(HEX/DEC)format.
 */
typedef enum{
    HEX,
    DEC
} OUTPUT_FORMAT;
static OUTPUT_FORMAT output_form;

/* Sets when array width for perticular raid type is not valid. 
 */
static BOOL array_wd_error = FALSE;

/* raid_input_string [] is made parallel to the RAID_TYPE of the CalcStruct
 * even though we are not using the HI5 so that it will be easier to
 * determine the raid type from the input string.
 */

static PCHAR raid_input_string[RAID_T_STR_ARR_SIZE] = 
{
    "R0","R1","R1_0","R3","R5","HI5","ID","R6"
};

/* raid_output_string [] is made parallel to the RAID_TYPE of the CalcStruct
 * even though we are not using the HI5 so that it will be easier to
 * retrieve the raid type from the CalcStruct.raidType.
 */

static PCHAR raid_output_string[RAID_T_STR_ARR_SIZE] = 
{
    "RAID0",
    "RAID1",
    "RAID10",
    "RAID3",
    "RAID5",
    "HI5",
    "IND_DISK",
    "RAID6"
} ;

/***************************************************
 * END RaidCalcCli.h
 ***************************************************/

#endif // RAID_CALC_CLI_H
