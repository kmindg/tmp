#ifndef RESUME_PROM_H
#define RESUME_PROM_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2003
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/****************************************************************
 *  resume_prom.h
 ****************************************************************
 *
 *  Description:
 *  This header file contains structures which define
 *  the format of the resume PROM information.
 *
 *  Seperated from cm_resume_prom_api.h for the benefit
 *  of drivers outside of FLARE.
 *
 *  History:
 *      Nov-19-2001 . Ashok Tamilarasan - Created
 *      Sept-25-2003  Matt Yellen - Moved into this file from cm_resume_prom_api.h
 *      Sept-25-2003  Joe Ash - Renamed to resume_prom.h and removed CM_ from
 *                               all of the #defines and enums.
 *      May-04-2012   J Blaney - Add IPMI resume defines and structures
 *
 ****************************************************************/

#include "generic_types.h"
#include "csx_ext.h"

/* NOTE !!! Whenever a new field is added to the layout of Resume PROM.
 * Please do the following:
 * 1. Make an entry in RESUME_PROM_STRUCTURE structure(in resume_prom.h)
 * 2. Add the new field to the RESUME_PROM_FIELD enum.
 * 3. Create a #define for the size of the field. If space for this new field
 *     is taken from any of the empty spaces, then update the size for that
 *     empty space.
 * 4. Add the field to the various helper functions in the generic_utils_lib.
 *      getResumeFieldOffset
 *      getResumeFieldSize
 *      getResumeFieldName
 *      initResumeHelper
 */

/* Note : as of IPMI supported hardware platforms a corresponding IPMI structure
 * has been created. The BMC has the responsibility of reading older FRU resume's 
 * with the resume written in the EMC format and storing them in it's cache inthe IPMI format.
 * As support is added for newer FRU's they will be programmed with the IMPI format.
 * When the IPMI NetFN 0x0A is used to read the resume it is returned in the IPMI format.
 * The IPMI logic in Specl will do the mapping of the data from the IPMI format back into the EMC
 * format so as not to affect any of the upper layers.
 * ANY work in resume data structures sizes or data fields needs to take into account both
 * the IPMI data returned from the BMC and the data returned to upperlayers into account.
 */

/* The size of each data field in the resume prom (in bytes) */
#define RESUME_PROM_EMC_TLA_PART_NUM_SIZE           16
#define RESUME_PROM_EMC_TLA_ARTWORK_REV_SIZE        3
#define RESUME_PROM_EMC_TLA_ASSEMBLY_REV_SIZE       3
#define RESUME_PROM_EMPTY_SPACE_1_SIZE              2

#define RESUME_PROM_SEMAPHORE_SIZE                  1
#define RESUME_PROM_EMPTY_SPACE_2_SIZE              7

#define RESUME_PROM_EMC_TLA_SERIAL_NUM_SIZE         16
#define RESUME_PROM_EMPTY_SPACE_3_SIZE              3

#define RESUME_PROM_EMC_SYSTEM_HW_PN_SIZE           16
#define RESUME_PROM_EMC_SYSTEM_HW_SN_SIZE           16
#define RESUME_PROM_EMC_SYSTEM_HW_REV_SIZE          3
#define RESUME_PROM_EMPTY_SPACE_18_SIZE             3

#define RESUME_PROM_PRODUCT_PN_SIZE                 16
#define RESUME_PROM_PRODUCT_SN_SIZE                 16
#define RESUME_PROM_PRODUCT_REV_SIZE                3
#define RESUME_PROM_EMPTY_SPACE_19_SIZE             4

#define RESUME_PROM_VENDOR_PART_NUM_SIZE            32
#define RESUME_PROM_VENDOR_ARTWORK_REV_SIZE         3
#define RESUME_PROM_VENDOR_ASSEMBLY_REV_SIZE        3
#define RESUME_PROM_VENDOR_UNIQUE_REV_SIZE          4
#define RESUME_PROM_VENDOR_ACDC_INPUT_TYPE_SIZE     2
#define RESUME_PROM_EMPTY_SPACE_4_SIZE              4

#define RESUME_PROM_VENDOR_SERIAL_NUM_SIZE          32
#define RESUME_PROM_EMPTY_SPACE_5_SIZE              2

#define RESUME_PROM_PCIE_CONFIGURATION_SIZE         1
#define RESUME_PROM_BOARD_POWER_SIZE                2
#define RESUME_PROM_THERMAL_TARGET_SIZE             1
#define RESUME_PROM_THERMAL_SHUTDOWN_LIMIT_SIZE     1
#define RESUME_PROM_EMPTY_SPACE_20_SIZE             41

#define RESUME_PROM_VENDOR_NAME_SIZE                32
#define RESUME_PROM_LOCATION_MANUFACTURE_SIZE       32
#define RESUME_PROM_YEAR_MANUFACTURE_SIZE           4
#define RESUME_PROM_MONTH_MANUFACTURE_SIZE          2
#define RESUME_PROM_DAY_MANUFACTURE_SIZE            2
#define RESUME_PROM_EMPTY_SPACE_6_SIZE              8

#define RESUME_PROM_TLA_ASSEMBLY_NAME_SIZE          32
#define RESUME_PROM_CONTACT_INFORMATION_SIZE        128
#define RESUME_PROM_EMPTY_SPACE_7_SIZE              16

#define RESUME_PROM_ESES_NUM_PROG                   1
#define RESUME_PROM_NUM_PROG_SIZE                   1
#define RESUME_PROM_EMPTY_SPACE_8_SIZE              15

#define RESUME_PROM_PROG_NAME_SIZE                  8
#define RESUME_PROM_PROG_REV_SIZE                   4
#define RESUME_PROM_PROG_CHECKSUM_SIZE              4
#define RESUME_PROM_EMPTY_SPACE_9_SIZE              12

#define RESUME_PROM_WWN_SEED_SIZE                   4
#define RESUME_PROM_SAS_ADDRESS_SIZE                4
#define RESUME_PROM_EMPTY_SPACE_10_SIZE             12

#define RESUME_PROM_RESERVED_REGION_1_SIZE          1
#define RESUME_PROM_EMPTY_SPACE_11_SIZE             4

#define RESUME_PROM_PCBA_PART_NUM_SIZE              16
#define RESUME_PROM_PCBA_ASSEMBLY_REV_SIZE          3
#define RESUME_PROM_PCBA_SERIAL_NUM_SIZE            16
#define RESUME_PROM_EMPTY_SPACE_12_SIZE             2

#define RESUME_PROM_CONFIGURATION_TYPE_SIZE         2
#define RESUME_PROM_EMPTY_SPACE_13_SIZE             2

#define RESUME_PROM_EMC_ALT_MB_PART_SIZE            16
#define RESUME_PROM_EMPTY_SPACE_14_SIZE             4

#define RESUME_PROM_CHANNEL_SPEED_SIZE              2
#define RESUME_PROM_SYSTEM_TYPE_SIZE                2
#define RESUME_PROM_DAE_ENCL_ID_SIZE                1
#define RESUME_PROM_RACK_ID_SIZE                    1
#define RESUME_PROM_SLOT_ID_SIZE                    1
#define RESUME_PROM_EMPTY_SPACE_15_SIZE             5

#define RESUME_PROM_DRIVE_SPIN_UP_SELECT_SIZE       2
#define RESUME_PROM_SP_FAMILY_FRU_ID_SIZE           4
#define RESUME_PROM_FRU_CAPABILITY_SIZE             2
#define RESUME_PROM_EMPTY_SPACE_16_SIZE             2

#define RESUME_PROM_EMC_SUB_ASSY_PART_NUM_SIZE      16
#define RESUME_PROM_EMC_SUB_ASSY_ARTWORK_REV_SIZE   3
#define RESUME_PROM_EMC_SUB_ASSY_REV_SIZE           3
#define RESUME_PROM_EMC_SUB_ASSY_SERIAL_NUM_SIZE    16
#define RESUME_PROM_EMPTY_SPACE_17_SIZE             14

#define RESUME_PROM_CHECKSUM_SIZE                   4

#define RESUME_PROM_PS_MAX_FREQ_SIZE                1


/* Maximum programmables count as defined in the resume PROM Spec. */
#define     RESUME_PROM_MAX_PROG_COUNT              84
#define     IPMI_RESUME_PROM_MAX_PROG_COUNT         84

/* Maximum MAC Addresses count as defined in the resume PROM Spec. */
#define     RESUME_PROM_MAX_MAC_ADDR_COUNT          13

#define     RESUME_PROM_PS_MAX_FREQ_START_ADDR      0x81C

/* The size of each data field in the IPMI resume format as mapped by EMC(in bytes) */
#define IPMI_RESUME_COMMON_HEADER_SIZE              8

/* The 8 bytes of the IPMI Common header */
#define IPMI_RESUME_COMMON_HEADER_FORMAT_VER_SIZE   1
#define IPMI_RESUME_INTERNAL_USE_AREA_SIZE          1
#define IPMI_RESUME_CHASSIS_INFO_AREA_SIZE          1
#define IPMI_RESUME_BOARD_INFO_AREA_SIZE            1
#define IPMI_RESUME_PRODUCT_INFO_AREA_SIZE          1
#define IPMI_RESUME_MULTI_REC_INFO_AREA_SIZE        1
#define IPMI_RESUME_COMMON_HEADER_PAD_AREA_SIZE     1
#define IPMI_RESUME_COMMON_HEADER_CKSUM_SIZE        1

/* The IPMI type / length byte, a 1 byte pad between fields */
#define IPMI_RESUME_TYPE_LENGTH_SIZE                1

/* The IPMI Chassis Information Area */
/* This area is only populated for the Midplane FRU */
#define IPMI_RESUME_CHASSIS_INFO_AREA_FORMAT_VER_SIZE           1
#define IPMI_RESUME_CHASSIS_INFO_AREA_LENGTH_SIZE               1
#define IPMI_RESUME_CHASSIS_INFO_CHASSIS_TYPE_SIZE              1

#define IPMI_RESUME_CHASSIS_INFO_CHASSIS_PN_BYTES_SIZE          16
#define IPMI_RESUME_CHASSIS_INFO_CHASSIS_SN_BYTES_SIZE          16

#define IPMI_RESUME_CHASSIS_INFO_EMC_ARTWORK_REV_SIZE           3
#define IPMI_RESUME_CHASSIS_INFO_EMC_TLA_ASSEMBLY_REV_SIZE      3

#define IPMI_RESUME_CHASSIS_INFO_EMC_SYSTEM_HW_PN_SIZE          16
#define IPMI_RESUME_CHASSIS_INFO_EMC_SYSTEM_HW_SN_SIZE          16
#define IPMI_RESUME_CHASSIS_INFO_EMC_SYSTEM_HW_REV_SIZE         3

#define IPMI_RESUME_CHASSIS_INFO_EMC_ENCL_ID_SIZE               1
#define IPMI_RESUME_CHASSIS_INFO_EMC_RACK_ID_SIZE               1
#define IPMI_RESUME_CHASSIS_INFO_EMC_CONFIGURATION_TYPE_SIZE    2
#define IPMI_RESUME_CHASSIS_INFO_EMC_NAS_ENCLOSURE_ID_SIZE      16

#define IPMI_RESUME_CHASSIS_INFO_EMPTY_SPACE_1_SIZE             3
#define IPMI_RESUME_CHASSIS_INFO_CKSUM_SIZE                     1

/* The IPMI Board Information Area */
#define IPMI_RESUME_BOARD_INFO_AREA_FORMAT_VER_SIZE             1
#define IPMI_RESUME_BOARD_INFO_AREA_LENGTH_SIZE                 1
#define IPMI_RESUME_BOARD_INFO_LANGUAGE_CODE_SIZE               1

#define IPMI_RESUME_BOARD_INFO_MFG_YEAR_SIZE                    1
#define IPMI_RESUME_BOARD_INFO_MFG_MONTH_SIZE                   1
#define IPMI_RESUME_BOARD_INFO_MFG_DAY_SIZE                     1
#define IPMI_RESUME_BOARD_INFO_MFG_BYTES_SIZE                   32

#define IPMI_RESUME_BOARD_INFO_PROD_NAME_BYTES_SIZE             32
#define IPMI_RESUME_BOARD_INFO_SN_BYTES_SIZE                    16
#define IPMI_RESUME_BOARD_INFO_PN_BYTES_SIZE                    16

#define IPMI_RESUME_BOARD_INFO_FRU_FILE_BYTES_SIZE              0

#define IPMI_RESUME_BOARD_INFO_EMC_TLA_ARTWORK_REV_SIZE         3
#define IPMI_RESUME_BOARD_INFO_EMC_TLA_ASSEMBLY_REV_SIZE        3

#define IPMI_RESUME_BOARD_INFO_EMC_PCBA_PART_NUM_SIZE           16
#define IPMI_RESUME_BOARD_INFO_EMC_PCBA_ASSEMBLY_REV_SIZE       3
#define IPMI_RESUME_BOARD_INFO_EMC_PCBA_SERIAL_NUM_SIZE         16

#define IPMI_RESUME_BOARD_INFO_EMC_VENDOR_PART_NUM_SIZE         32
#define IPMI_RESUME_BOARD_INFO_EMC_VENDOR_ARTWORK_REV_SIZE      3
#define IPMI_RESUME_BOARD_INFO_EMC_VENDOR_ASSEMBLY_REV_SIZE     3
#define IPMI_RESUME_BOARD_INFO_EMC_VENDOR_UNIQUE_REV_SIZE       4
#define IPMI_RESUME_BOARD_INFO_EMC_VENDOR_ACDC_INPUT_TYPE_SIZE  2
#define IPMI_RESUME_BOARD_INFO_EMC_VENDOR_SERIAL_NUM_SIZE       32
#define IPMI_RESUME_BOARD_INFO_EMC_LOCATION_MANUFACTURE_SIZE    32

#define IPMI_RESUME_BOARD_INFO_EMC_ALT_MB_PART_SIZE             16
#define IPMI_RESUME_BOARD_INFO_EMC_SUB_ASSY_PART_NUM_SIZE       16
#define IPMI_RESUME_BOARD_INFO_EMC_SUB_ASSY_ARTWORK_REV_SIZE    3
#define IPMI_RESUME_BOARD_INFO_EMC_SUB_ASSY_REV_SIZE            3
#define IPMI_RESUME_BOARD_INFO_EMC_SUB_ASSY_SERIAL_NUM_SIZE     16

#define IPMI_RESUME_BOARD_INFO_EMC_WWN_SEED_SIZE                4
#define IPMI_RESUME_BOARD_INFO_EMC_SAS_ADDRESS_SIZE             4

#define IPMI_RESUME_BOARD_INFO_EMC_FAMILY_FRU_ID_SIZE           4
#define IPMI_RESUME_BOARD_INFO_EMC_CHANNEL_SPEED_SIZE           4
#define IPMI_RESUME_BOARD_INFO_EMC_SLOT_ID_SIZE                 1
#define IPMI_RESUME_BOARD_INFO_EMC_DRIVE_SPIN_UP_SELECT_SIZE    2

#define IPMI_RESUME_BOARD_INFO_EMC_SEMAPHORE_SIZE               1

#define IPMI_RESUME_BOARD_INFO_PCIE_CONFIGURATION_SIZE          1
#define IPMI_RESUME_BOARD_INFO_BOARD_POWER_SIZE                 2
#define IPMI_RESUME_BOARD_INFO_THERMAL_TARGET_SIZE              1
#define IPMI_RESUME_BOARD_INFO_THERMAL_SHUTDOWN_LIMIT_SIZE      1
#define IPMI_RESUME_BOARD_INFO_FRU_CAPABILITY_SIZE              2

#define IPMI_RESUME_BOARD_INFO_CKSUM_SIZE                       1

/* The IPMI Product Information Area */
#define IPMI_RESUME_PRODUCT_INFO_AREA_FORMAT_VER_SIZE   1
#define IPMI_RESUME_PRODUCT_INFO_AREA_LENGTH_SIZE       1
#define IPMI_RESUME_PRODUCT_INFO_LANGUAGE_CODE_SIZE     1

#define IPMI_RESUME_PRODUCT_INFO_MFG_NAME_BYTES_SIZE    0
#define IPMI_RESUME_PRODUCT_INFO_PROD_NAME_BYTES_SIZE   0
#define IPMI_RESUME_PRODUCT_INFO_PROD_PN_BYTES_SIZE     16
#define IPMI_RESUME_PRODUCT_INFO_PROD_VER_BYTES_SIZE    3
#define IPMI_RESUME_PRODUCT_INFO_PROD_SN_BYTES_SIZE     16
#define IPMI_RESUME_PRODUCT_INFO_ASSET_TAG_BYTES_SIZE   0
#define IPMI_RESUME_PRODUCT_INFO_FRU_ID_BYTES_SIZE      0

#define IPMI_RESUME_PRODUCT_INFO_EMC_SYSTEM_TYPE_SIZE   2

#define IPMI_RESUME_PRODUCT_INFO_EMPTY_SPACE_1_SIZE     6

#define IPMI_RESUME_PRODUCT_INFO_CKSUM_SIZE             1

/* The IPMI Internal Information Area */
#define IPMI_RESUME_INTERNAL_INFO_AREA_FORMAT_VER_SIZE      1

#define IPMI_RESUME_INTERNAL_INFO_EMPTY_SPACE_1_SIZE        15

#define IPMI_RESUME_INTERNAL_INFO_EMC_PROGRAMABLES_SIZE     1

#define IPMI_RESUME_INTERNAL_INFO_EMPTY_SPACE_2_SIZE        7

#define IPMI_RESUME_INTERNAL_INFO_EMC_PROG_NAME_SIZE        8
#define IPMI_RESUME_INTERNAL_INFO_EMC_PROG_REV_SIZE         4
#define IPMI_RESUME_INTERNAL_INFO_EMC_PROG_CHECKSUM_SIZE    4

#define IPMI_RESUME_INTERNAL_INFO_EMC_CONTACT_INFO_SIZE     128

#define IPMI_RESUME_INTERNAL_INFO_EMPTY_SPACE_3_SIZE        8

#define IPMI_RESUME_INTERNAL_INFO_EXTENDED_RESUME_SIZE      2048

/* Fields on the Resume PROMs */
typedef enum  resume_prom_field_tag
{
    RESUME_PROM_EMC_TLA_PART_NUM = 1,
    RESUME_PROM_EMC_TLA_ARTWORK_REV,
    RESUME_PROM_EMC_TLA_ASSEMBLY_REV,
    RESUME_PROM_SEMAPHORE,
    RESUME_PROM_EMC_TLA_SERIAL_NUM,
    RESUME_PROM_EMC_SYSTEM_HW_PN,
    RESUME_PROM_EMC_SYSTEM_HW_SN,
    RESUME_PROM_EMC_SYSTEM_HW_REV,
    RESUME_PROM_PRODUCT_PN,
    RESUME_PROM_PRODUCT_SN,
    RESUME_PROM_PRODUCT_REV,
    RESUME_PROM_VENDOR_PART_NUM,
    RESUME_PROM_VENDOR_ARTWORK_REV,
    RESUME_PROM_VENDOR_ASSEMBLY_REV,
    RESUME_PROM_VENDOR_UNIQUE_REV,
    RESUME_PROM_VENDOR_ACDC_INPUT_TYPE,
    RESUME_PROM_VENDOR_SERIAL_NUM,
    RESUME_PROM_PCIE_CONFIGURATION,
    RESUME_PROM_BOARD_POWER,
    RESUME_PROM_THERMAL_TARGET,
    RESUME_PROM_THERMAL_SHUTDOWN_LIMIT,
    RESUME_PROM_VENDOR_NAME,
    RESUME_PROM_LOCATION_MANUFACTURE,
    RESUME_PROM_YEAR_MANUFACTURE,
    RESUME_PROM_MONTH_MANUFACTURE,
    RESUME_PROM_DAY_MANUFACTURE,
    RESUME_PROM_TLA_ASSEMBLY_NAME,
    RESUME_PROM_CONTACT_INFORMATION,
    RESUME_PROM_NUM_PROG,
    RESUME_PROM_PROG_NAME,
    RESUME_PROM_PROG_REV,
    RESUME_PROM_PROG_CHECKSUM,
    RESUME_PROM_CHECKSUM,
    RESUME_PROM_WWN_SEED,
    RESUME_PROM_SAS_ADDRESS,
    RESUME_PROM_RESERVED_REGION_1,
    RESUME_PROM_PCBA_PART_NUM,
    RESUME_PROM_PCBA_ASSEMBLY_REV,
    RESUME_PROM_PCBA_SERIAL_NUM,
    RESUME_PROM_CONFIGURATION_TYPE,
    RESUME_PROM_EMC_ALT_MB_PART,
    RESUME_PROM_CHANNEL_SPEED,
    RESUME_PROM_DAE_ENCL_ID,
    RESUME_PROM_RACK_ID,
    RESUME_PROM_SLOT_ID,
    RESUME_PROM_SYSTEM_TYPE,
    RESUME_PROM_DRIVE_SPIN_UP_SELECT,
    RESUME_PROM_SP_FAMILY_FRU_ID,
    RESUME_PROM_FRU_CAPABILITY,
    RESUME_PROM_EMC_SUB_ASSY_PART_NUM,
    RESUME_PROM_EMC_SUB_ASSY_ARTWORK_REV,
    RESUME_PROM_EMC_SUB_ASSY_REV,
    RESUME_PROM_EMC_SUB_ASSY_SERIAL_NUM,
    RESUME_PROM_PROGRAMMABLES,
    RESUME_PROM_PS_MAX_FREQ,
    RESUME_PROM_FIELD_EOT
}RESUME_PROM_FIELD;

/* Fields on the Resume PROMs */
typedef enum  ipmi_resume_prom_field_tag
{
    IPMI_RESUME_COMMON_HEADER_FORMAT_VER =1,
    IPMI_RESUME_INTERNAL_USE_AREA,
    IPMI_RESUME_CHASSIS_INFO_AREA,
    IPMI_RESUME_BOARD_INFO_AREA,
    IPMI_RESUME_PRODUCT_INFO_AREA,
    IPMI_RESUME_MULTI_RECORD_INFO_AREA,
    IPMI_RESUME_COMMON_HEADER_PAD_AREA,
    IPMI_RESUME_COMMON_HEADER_CKSUM,
    IPMI_RESUME_CHASSIS_INFO_AREA_FORMAT_VER,
    IPMI_RESUME_CHASSIS_INFO_AREA_LENGTH,
    IPMI_RESUME_CHASSIS_INFO_CHASSIS_TYPE,
    IPMI_RESUME_CHASSIS_INFO_CHASSIS_PN_LEN,
    IPMI_RESUME_CHASSIS_INFO_CHASSIS_PN_BYTES,
    IPMI_RESUME_CHASSIS_INFO_CHASSIS_SN_LEN,
    IPMI_RESUME_CHASSIS_INFO_CHASSIS_SN_BYTES,
    IPMI_RESUME_CHASSIS_INFO_EMC_ARTWORK_REV,
    IPMI_RESUME_CHASSIS_INFO_EMC_TLA_ASSEMBLY,
    IPMI_RESUME_CHASSIS_INFO_EMC_SYSTEM_HW_PN,
    IPMI_RESUME_CHASSIS_INFO_EMC_SYSTEM_HW_SN,
    IPMI_RESUME_CHASSIS_INFO_EMC_SYSTEM_HW_REV,
    IPMI_RESUME_CHASSIS_INFO_EMC_ENCL_ID,
    IPMI_RESUME_CHASSIS_INFO_EMC_RACK_ID,
    IPMI_RESUME_CHASSIS_INFO_EMC_CONFIGURATION_TYPE,
    IPMI_RESUME_CHASSIS_INFO_EMC_NAS_ENCLOSURE_ID,
    IPMI_RESUME_CHASSIS_INFO_CKSUM,
    IPMI_RESUME_BOARD_INFO_AREA_FORMAT_VER,
    IPMI_RESUME_BOARD_INFO_AREA_LENGTH,
    IPMI_RESUME_BOARD_INFO_LANGUAGE_CODE,
    IPMI_RESUME_BOARD_INFO_MFG_YEAR,
    IPMI_RESUME_BOARD_INFO_MFG_MONTH,
    IPMI_RESUME_BOARD_INFO_MFG_DAY,
    IPMI_RESUME_BOARD_INFO_MFG_LEN,
    IPMI_RESUME_BOARD_INFO_MFG_BYTES,
    IPMI_RESUME_BOARD_INFO_PROD_NAME_LEN,
    IPMI_RESUME_BOARD_INFO_PROD_NAME_BYTES,
    IPMI_RESUME_BOARD_INFO_SN_LEN,
    IPMI_RESUME_BOARD_INFO_SN_BYTES,
    IPMI_RESUME_BOARD_INFO_PN_LEN,
    IPMI_RESUME_BOARD_INFO_PN_BYTES,
    IPMI_RESUME_BOARD_INFO_FRU_FILE_LEN,
    IPMI_RESUME_BOARD_INFO_EMC_TLA_ARTWORK_REV,
    IPMI_RESUME_BOARD_INFO_EMC_TLA_ASSEMBLY_REV,
    IPMI_RESUME_BOARD_INFO_EMC_PCBA_PART_NUM,
    IPMI_RESUME_BOARD_INFO_EMC_PCBA_ASSEMBLY_REV,
    IPMI_RESUME_BOARD_INFO_EMC_PCBA_SERIAL_NUM,
    IPMI_RESUME_BOARD_INFO_EMC_VENDOR_PART_NUM,
    IPMI_RESUME_BOARD_INFO_EMC_VENDOR_ARTWORK_REV,
    IPMI_RESUME_BOARD_INFO_EMC_VENDOR_ASSEMBLY_REV,
    IPMI_RESUME_BOARD_INFO_EMC_VENDOR_UNIQUE_REV,
    IPMI_RESUME_BOARD_INFO_EMC_VENDOR_ACDC_INPUT_TYPE,
    IPMI_RESUME_BOARD_INFO_EMC_VENDOR_SERIAL_NUM,
    IPMI_RESUME_BOARD_INFO_EMC_LOCATION_MANUFACTURE,
    IPMI_RESUME_BOARD_INFO_EMC_ALT_MB_PART,
    IPMI_RESUME_BOARD_INFO_EMC_SUB_ASSY_PART_NUM,
    IPMI_RESUME_BOARD_INFO_EMC_SUB_ASSY_ARTWORK_REV,
    IPMI_RESUME_BOARD_INFO_EMC_SUB_ASSY_REV,
    IPMI_RESUME_BOARD_INFO_EMC_SUB_ASSY_SERIAL_NUM,
    IPMI_RESUME_BOARD_INFO_EMC_WWN_SEED,
    IPMI_RESUME_BOARD_INFO_EMC_SAS_ADDRESS,
    IPMI_RESUME_BOARD_INFO_EMC_FAMILY_FRU_ID,
    IPMI_RESUME_BOARD_INFO_EMC_CHANNEL_SPEED,
    IPMI_RESUME_BOARD_INFO_EMC_SLOT_ID,
    IPMI_RESUME_BOARD_INFO_EMC_DRIVE_SPIN_UP_SELECT,
    IPMI_RESUME_BOARD_INFO_EMC_SEMAPHORE,
    IPMI_RESUME_BOARD_INFO_EMC_PCIE_CONFIGURATION,
    IPMI_RESUME_BOARD_INFO_EMC_BOARD_POWER,
    IPMI_RESUME_BOARD_INFO_EMC_THERMAL_TARGET,
    IPMI_RESUME_BOARD_INFO_EMC_THERMAL_SHUTDOWN_LIMIT,
    IPMI_RESUME_BOARD_INFO_EMC_FRU_CAPABILITIY,
    IPMI_RESUME_BOARD_INFO_CKSUM,
    IPMI_RESUME_PRODUCT_INFO_AREA_FORMAT_VER,
    IPMI_RESUME_PRODUCT_INFO_AREA_LENGTH,
    IPMI_RESUME_PRODUCT_INFO_LANGUAGE_CODE,
    IPMI_RESUME_PRODUCT_INFO_MFG_NAME_LEN,
    IPMI_RESUME_PRODUCT_INFO_PROD_NAME_LEN,
    IPMI_RESUME_PRODUCT_INFO_PROD_PN_LEN,
    IPMI_RESUME_PRODUCT_INFO_PROD_PN_BYTES,
    IPMI_RESUME_PRODUCT_INFO_PROD_VER,
    IPMI_RESUME_PRODUCT_INFO_PROD_VER_BYTES,
    IPMI_RESUME_PRODUCT_INFO_PROD_SN_LEN,
    IPMI_RESUME_PRODUCT_INFO_PROD_SN_BYTES,
    IPMI_RESUME_PRODUCT_INFO_ASSET_TAG_LEN,
    IPMI_RESUME_PRODUCT_INFO_FRU_ID_LEN,
    IPMI_RESUME_PRODUCT_INFO_EMC_SYSTEM_TYPE,
    IPMI_RESUME_PRODUCT_INFO_CKSUM,
    IPMI_RESUME_INTERNAL_INFO_AREA_FORMAT_VER,
    IPMI_RESUME_INTERNAL_INFO_EMC_PROGRAMABLES,
    IPMI_RESUME_INTERNAL_INFO_EMC_PROG_NAME,
    IPMI_RESUME_INTERNAL_INFO_EMC_PROG_REV,
    IPMI_RESUME_INTERNAL_INFO_EMC_PROG_CHECKSUM,
    IPMI_RESUME_INTERNAL_INFO_EMC_CONTACT_INFO
}IPMI_RESUME_PROM_FIELD;

#define RESUME_PROM_ALL_FIELDS  0xFE

#if defined(__cplusplus)
/* use extern C++ here to trick other C++ files including this as extern C
 */
extern "C++"{
/* Define a postfix increment operator for _RESUME_PROM_FIELD 
 */
inline resume_prom_field_tag operator++( resume_prom_field_tag &resumePromField, int )
{
   return resumePromField = (resume_prom_field_tag)((int)resumePromField + 1);
}
/* and _IPMI_RESUME_PROM_FIELD */
inline ipmi_resume_prom_field_tag operator++( ipmi_resume_prom_field_tag &resumePromField, int )
{
   return resumePromField = (ipmi_resume_prom_field_tag)((int)resumePromField + 1);
}

}

#endif


/* Checksum seed used to check the checksum */
#define     RESUME_PROM_CHECKSUM_SEED       0x64656573 /* ASCII FOR "DEES"
                                                        * which is "SEED" if byte
                                                        * swapped
                                                        */

/* Pack the structure in byte boundary because all resume PROM
 * information is read in single byte by the LCC and copied to
 * an array. The data is then transferred to a buffer by doing a
 * memory copy. So the structure should be aligned in byte boundary
 */
#pragma pack(push, resume_prom_info, 1)

/*************************************************************
 * RESUME_PROM_PROG_DETAILS
 *   This structure is used to keep information about
 *   Programmables on the resume PROM
 *
 *************************************************************
 */
typedef struct _RESUME_PROM_PROG_DETAILS
{
    CHAR        prog_name       [RESUME_PROM_PROG_NAME_SIZE];
    CHAR        prog_rev        [RESUME_PROM_PROG_REV_SIZE];
    UINT_32E    prog_checksum;
} RESUME_PROM_PROG_DETAILS;

typedef struct _RESUME_PROM_RESUME_PRODUCT_NUMS
{
    CHAR product_part_number    [RESUME_PROM_PRODUCT_PN_SIZE];
    CHAR product_serial_number  [RESUME_PROM_PRODUCT_SN_SIZE];
    CHAR product_revision       [RESUME_PROM_PRODUCT_REV_SIZE];
} RESUME_PROM_RESUME_PRODUCT_NUMS;

/***********************************************************************
 * RESUME_PROM_STRUCTURE
 *   This structure is used to keep information about
 *   Mapping of Resume PROMs
 *
 * !!!NOTE!!! 1. The structure exactly depicts the layout of the resume PROM.
 *               If a new field is added to resume PROM, the new element
 *               should be added to the structure at the same position
 *               If the space for the new field is allocated from one of
 *               the empty spaces, then the empty space size should also
 *               by updated. This change will also be necessary to the 
 *               IPMI_RESUME_PROM_STRUCTURE.
 *
 ***********************************************************************
 */
typedef struct _RESUME_PROM_STRUCTURE
{
    CHAR            emc_tla_part_num            [RESUME_PROM_EMC_TLA_PART_NUM_SIZE];
    CHAR            emc_tla_artwork_rev         [RESUME_PROM_EMC_TLA_ARTWORK_REV_SIZE];
    CHAR            emc_tla_assembly_rev        [RESUME_PROM_EMC_TLA_ASSEMBLY_REV_SIZE];
    csx_uchar_t     empty_space_1               [RESUME_PROM_EMPTY_SPACE_1_SIZE];

    UINT_8E         rp_semaphore                [RESUME_PROM_SEMAPHORE_SIZE];
    csx_uchar_t     empty_space_2               [RESUME_PROM_EMPTY_SPACE_2_SIZE];

    CHAR            emc_tla_serial_num          [RESUME_PROM_EMC_TLA_SERIAL_NUM_SIZE];
    csx_uchar_t     empty_space_3               [RESUME_PROM_EMPTY_SPACE_3_SIZE];

    CHAR            emc_system_hw_pn            [RESUME_PROM_EMC_SYSTEM_HW_PN_SIZE];
    CHAR            emc_system_hw_sn            [RESUME_PROM_EMC_SYSTEM_HW_SN_SIZE];
    CHAR            emc_system_hw_rev           [RESUME_PROM_EMC_SYSTEM_HW_REV_SIZE];
    csx_uchar_t     empty_space_18              [RESUME_PROM_EMPTY_SPACE_18_SIZE];

    CHAR            product_part_number         [RESUME_PROM_PRODUCT_PN_SIZE];
    CHAR            product_serial_number       [RESUME_PROM_PRODUCT_SN_SIZE];
    CHAR            product_revision            [RESUME_PROM_PRODUCT_REV_SIZE];
    csx_uchar_t     empty_space_19              [RESUME_PROM_EMPTY_SPACE_19_SIZE];

    CHAR            vendor_part_num             [RESUME_PROM_VENDOR_PART_NUM_SIZE];
    CHAR            vendor_artwork_rev          [RESUME_PROM_VENDOR_ARTWORK_REV_SIZE];
    CHAR            vendor_assembly_rev         [RESUME_PROM_VENDOR_ASSEMBLY_REV_SIZE];
    CHAR            vendor_unique_rev           [RESUME_PROM_VENDOR_UNIQUE_REV_SIZE];
    CHAR            vendor_acdc_input_type      [RESUME_PROM_VENDOR_ACDC_INPUT_TYPE_SIZE];
    csx_uchar_t     empty_space_4               [RESUME_PROM_EMPTY_SPACE_4_SIZE];

    CHAR            vendor_serial_num           [RESUME_PROM_VENDOR_SERIAL_NUM_SIZE];
    csx_uchar_t     empty_space_5               [RESUME_PROM_EMPTY_SPACE_5_SIZE];

    UINT_8E         pcie_configuration          [RESUME_PROM_PCIE_CONFIGURATION_SIZE];
    UINT_8E         board_power                 [RESUME_PROM_BOARD_POWER_SIZE];
    UINT_8E         thermal_target              [RESUME_PROM_THERMAL_TARGET_SIZE];
    UINT_8E         thermal_shutdown_limit      [RESUME_PROM_THERMAL_SHUTDOWN_LIMIT_SIZE];
    csx_uchar_t     empty_space_20              [RESUME_PROM_EMPTY_SPACE_20_SIZE];

    CHAR            vendor_name                 [RESUME_PROM_VENDOR_NAME_SIZE];
    CHAR            loc_mft                     [RESUME_PROM_LOCATION_MANUFACTURE_SIZE];
    CHAR            year_mft                    [RESUME_PROM_YEAR_MANUFACTURE_SIZE];
    CHAR            month_mft                   [RESUME_PROM_MONTH_MANUFACTURE_SIZE];
    CHAR            day_mft                     [RESUME_PROM_DAY_MANUFACTURE_SIZE];
    csx_uchar_t     empty_space_6               [RESUME_PROM_EMPTY_SPACE_6_SIZE];

    CHAR            tla_assembly_name           [RESUME_PROM_TLA_ASSEMBLY_NAME_SIZE];
    CHAR            contact_information         [RESUME_PROM_CONTACT_INFORMATION_SIZE];
    csx_uchar_t     empty_space_7               [RESUME_PROM_EMPTY_SPACE_7_SIZE];

    UINT_8E         num_prog                    [RESUME_PROM_NUM_PROG_SIZE];
    csx_uchar_t     empty_space_8               [RESUME_PROM_EMPTY_SPACE_8_SIZE];

    RESUME_PROM_PROG_DETAILS    prog_details    [RESUME_PROM_MAX_PROG_COUNT];
    csx_uchar_t     empty_space_9               [RESUME_PROM_EMPTY_SPACE_9_SIZE];

    UINT_32E        wwn_seed;
    UINT_8E         sas_address                 [RESUME_PROM_SAS_ADDRESS_SIZE];
    csx_uchar_t     empty_space_10              [RESUME_PROM_EMPTY_SPACE_10_SIZE];

    csx_uchar_t     reserved_region_1           [RESUME_PROM_RESERVED_REGION_1_SIZE];
    csx_uchar_t     empty_space_11              [RESUME_PROM_EMPTY_SPACE_11_SIZE];

    CHAR            pcba_part_num               [RESUME_PROM_PCBA_PART_NUM_SIZE];
    CHAR            pcba_assembly_rev           [RESUME_PROM_PCBA_ASSEMBLY_REV_SIZE];
    CHAR            pcba_serial_num             [RESUME_PROM_PCBA_SERIAL_NUM_SIZE];
    csx_uchar_t     empty_space_12              [RESUME_PROM_EMPTY_SPACE_12_SIZE];

    UINT_8E         configuration_type          [RESUME_PROM_CONFIGURATION_TYPE_SIZE];
    csx_uchar_t     empty_space_13              [RESUME_PROM_EMPTY_SPACE_13_SIZE];

    CHAR            emc_alt_mb_part_num         [RESUME_PROM_EMC_ALT_MB_PART_SIZE];
    csx_uchar_t     empty_space_14              [RESUME_PROM_EMPTY_SPACE_14_SIZE];

    UINT_8E         channel_speed               [RESUME_PROM_CHANNEL_SPEED_SIZE];
    UINT_8E         system_type                 [RESUME_PROM_SYSTEM_TYPE_SIZE];
    UINT_8E         dae_encl_id                 [RESUME_PROM_DAE_ENCL_ID_SIZE];
    UINT_8E         rack_id                     [RESUME_PROM_RACK_ID_SIZE];
    UINT_8E         slot_id                     [RESUME_PROM_SLOT_ID_SIZE];
    csx_uchar_t     empty_space_15              [RESUME_PROM_EMPTY_SPACE_15_SIZE];

    UINT_8E         drive_spin_up_select        [RESUME_PROM_DRIVE_SPIN_UP_SELECT_SIZE];
    UINT_8E         sp_family_fru_id            [RESUME_PROM_SP_FAMILY_FRU_ID_SIZE];
    UINT_8E         fru_capability              [RESUME_PROM_FRU_CAPABILITY_SIZE];
    csx_uchar_t     empty_space_16              [RESUME_PROM_EMPTY_SPACE_16_SIZE];

    CHAR            emc_sub_assy_part_num       [RESUME_PROM_EMC_SUB_ASSY_PART_NUM_SIZE];
    CHAR            emc_sub_assy_artwork_rev    [RESUME_PROM_EMC_SUB_ASSY_ARTWORK_REV_SIZE];
    CHAR            emc_sub_assy_rev            [RESUME_PROM_EMC_SUB_ASSY_REV_SIZE];
    CHAR            emc_sub_assy_serial_num     [RESUME_PROM_EMC_SUB_ASSY_SERIAL_NUM_SIZE];
    csx_uchar_t     empty_space_17              [RESUME_PROM_EMPTY_SPACE_17_SIZE];

    UINT_32E        resume_prom_checksum;

} RESUME_PROM_STRUCTURE;
#pragma pack(pop, resume_prom_info)

#pragma pack(push, ipmi_resume_prom_info, 1)

/*************************************************************
 * IPMI_RESUME_PROM_PROG_DETAILS
 *   This structure is used to keep information about
 *   Programmables on the resume PROMs
 *
 *************************************************************
 */
typedef struct _IPMI_RESUME_PROM_PROG_DETAILS
{
    CHAR        prog_name       [IPMI_RESUME_INTERNAL_INFO_EMC_PROG_NAME_SIZE];
    CHAR        prog_rev        [IPMI_RESUME_INTERNAL_INFO_EMC_PROG_REV_SIZE];
    UINT_32E    prog_checksum;
} IPMI_RESUME_PROM_PROG_DETAILS;

typedef struct _IPMI_RESUME_PROM_RESUME_PRODUCT_NUMS
{
    CHAR product_part_number    [IPMI_RESUME_PRODUCT_INFO_PROD_PN_BYTES_SIZE];
    CHAR product_serial_number  [IPMI_RESUME_PRODUCT_INFO_PROD_SN_BYTES_SIZE];
    CHAR product_revision       [IPMI_RESUME_PRODUCT_INFO_PROD_VER_BYTES_SIZE];
} IPMI_RESUME_PROM_RESUME_PRODUCT_NUMS;

/***********************************************************************
 * IPMI_RESUME_PROM_STRUCTURE
 *   This structure is used to keep information about IPMI Mapping of Resume PROMs
 *   The BMC is mapping existing supported FRU resume's from the EMC format to the new 
 *	 IPMI format.
 *
 * !!!NOTE!!! 1. The structure exactly depicts the layout of the IPMI resume PROM.
 *               If a new field is added to IPMI resume PROM, the new element
 *               should be added to the structure at the same position
 *               If the space for the new field is allocated from one of
 *               the empty spaces, then the empty space size should also
 *               by updated.
  *
 * ! NOTE !      The elements of the IPMI_RESUME_PROM_STRUCTURE match the elemements
 *               EMC resume prom structures if they share the same element name. 
 *               The only difference is the location of the element in the structure. 
 *
 ***********************************************************************
 */
typedef struct _IPMI_RESUME_PROM_STRUCTURE
{
    UINT_8E         header_format_version           [IPMI_RESUME_COMMON_HEADER_FORMAT_VER_SIZE];
    UINT_8E         internal_use_area               [IPMI_RESUME_INTERNAL_USE_AREA_SIZE];
    UINT_8E         chassis_info_area               [IPMI_RESUME_CHASSIS_INFO_AREA_SIZE];
    UINT_8E         board_info_area                 [IPMI_RESUME_BOARD_INFO_AREA_SIZE];
    UINT_8E         product_info_area               [IPMI_RESUME_PRODUCT_INFO_AREA_SIZE];
    UINT_8E         multi_rec_info_area             [IPMI_RESUME_MULTI_REC_INFO_AREA_SIZE];
    UINT_8E         common_header_pad               [IPMI_RESUME_COMMON_HEADER_PAD_AREA_SIZE];
    UINT_8E         common_header_cksum             [IPMI_RESUME_COMMON_HEADER_CKSUM_SIZE];

    UINT_8E         chassis_info_format             [IPMI_RESUME_CHASSIS_INFO_AREA_FORMAT_VER_SIZE];
    UINT_8E         chassis_info_area_len           [IPMI_RESUME_CHASSIS_INFO_AREA_LENGTH_SIZE];
    UINT_8E         chassis_type                    [IPMI_RESUME_CHASSIS_INFO_CHASSIS_TYPE_SIZE];

    UINT_8E         ipmi_type_length_40             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            emc_tla_part_num_chassis        [IPMI_RESUME_CHASSIS_INFO_CHASSIS_PN_BYTES_SIZE];

    UINT_8E         ipmi_type_length_41             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            emc_tla_serial_num_chassis      [IPMI_RESUME_CHASSIS_INFO_CHASSIS_SN_BYTES_SIZE];

    UINT_8E         ipmi_type_length_01             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            emc_tla_artwork_rev_chassis     [IPMI_RESUME_CHASSIS_INFO_EMC_ARTWORK_REV_SIZE];

    UINT_8E         ipmi_type_length_02             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            emc_tla_assembly_rev_chassis    [IPMI_RESUME_CHASSIS_INFO_EMC_TLA_ASSEMBLY_REV_SIZE];

    UINT_8E         ipmi_type_length_03             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            emc_system_hw_pn                [IPMI_RESUME_CHASSIS_INFO_EMC_SYSTEM_HW_PN_SIZE];

    UINT_8E         ipmi_type_length_04             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            emc_system_hw_sn                [IPMI_RESUME_CHASSIS_INFO_EMC_SYSTEM_HW_SN_SIZE];

    UINT_8E         ipmi_type_length_05             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            emc_system_hw_rev               [IPMI_RESUME_CHASSIS_INFO_EMC_SYSTEM_HW_REV_SIZE];

    UINT_8E         ipmi_type_length_06             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    UINT_8E         dae_encl_id                     [IPMI_RESUME_CHASSIS_INFO_EMC_ENCL_ID_SIZE];

    UINT_8E         ipmi_type_length_07             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    UINT_8E         rack_id                         [IPMI_RESUME_CHASSIS_INFO_EMC_RACK_ID_SIZE];

    UINT_8E         ipmi_type_length_08             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    UINT_8E         configuration_type              [IPMI_RESUME_CHASSIS_INFO_EMC_CONFIGURATION_TYPE_SIZE];

    UINT_8E         ipmi_type_length_09             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            nas_enclosure_id                [IPMI_RESUME_CHASSIS_INFO_EMC_NAS_ENCLOSURE_ID_SIZE];

    UINT_8E         ipmi_type_length_10             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    csx_uchar_t     ipmi_empty_space_1              [IPMI_RESUME_CHASSIS_INFO_EMPTY_SPACE_1_SIZE];

    UINT_8E         ipmi_type_length_11             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    UINT_8E         chassis_info_area_cksum         [IPMI_RESUME_CHASSIS_INFO_CKSUM_SIZE];

    UINT_8E         board_info_area_version         [IPMI_RESUME_BOARD_INFO_AREA_FORMAT_VER_SIZE];
    UINT_8E         board_info_area_len             [IPMI_RESUME_BOARD_INFO_AREA_LENGTH_SIZE];
    UINT_8E         board_language_code             [IPMI_RESUME_BOARD_INFO_LANGUAGE_CODE_SIZE];

    CHAR            year_mft                        [IPMI_RESUME_BOARD_INFO_MFG_YEAR_SIZE];
    CHAR            month_mft                       [IPMI_RESUME_BOARD_INFO_MFG_MONTH_SIZE];
    CHAR            day_mft                         [IPMI_RESUME_BOARD_INFO_MFG_DAY_SIZE];

    UINT_8E         ipmi_type_length_42             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            vendor_name                     [IPMI_RESUME_BOARD_INFO_MFG_BYTES_SIZE];

    UINT_8E         ipmi_type_length_43             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            tla_assembly_name               [IPMI_RESUME_BOARD_INFO_PROD_NAME_BYTES_SIZE];

    UINT_8E         ipmi_type_length_44             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            emc_tla_serial_num              [IPMI_RESUME_BOARD_INFO_SN_BYTES_SIZE];

    UINT_8E         ipmi_type_length_45             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            emc_tla_part_num                [IPMI_RESUME_BOARD_INFO_PN_BYTES_SIZE];

    UINT_8E         ipmi_type_length_46             [IPMI_RESUME_TYPE_LENGTH_SIZE]; //FRU File ID, EMPTY

    UINT_8E         ipmi_type_length_12             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            emc_tla_artwork_rev             [IPMI_RESUME_BOARD_INFO_EMC_TLA_ARTWORK_REV_SIZE];

    UINT_8E         ipmi_type_length_13             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            emc_tla_assembly_rev            [IPMI_RESUME_BOARD_INFO_EMC_TLA_ASSEMBLY_REV_SIZE];

    UINT_8E         ipmi_type_length_14             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            pcba_part_num                   [IPMI_RESUME_BOARD_INFO_EMC_PCBA_PART_NUM_SIZE];

    UINT_8E         ipmi_type_length_15             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            pcba_assembly_rev               [IPMI_RESUME_BOARD_INFO_EMC_PCBA_ASSEMBLY_REV_SIZE];

    UINT_8E         ipmi_type_length_16             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            pcba_serial_num                 [IPMI_RESUME_BOARD_INFO_EMC_PCBA_SERIAL_NUM_SIZE];

    UINT_8E         ipmi_type_length_17             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            vendor_part_num                 [IPMI_RESUME_BOARD_INFO_EMC_VENDOR_PART_NUM_SIZE];

    UINT_8E         ipmi_type_length_18             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            vendor_artwork_rev              [IPMI_RESUME_BOARD_INFO_EMC_VENDOR_ARTWORK_REV_SIZE];

    UINT_8E         ipmi_type_length_19             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            vendor_assembly_rev             [IPMI_RESUME_BOARD_INFO_EMC_VENDOR_ASSEMBLY_REV_SIZE];

    UINT_8E         ipmi_type_length_20             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            vendor_unique_rev               [IPMI_RESUME_BOARD_INFO_EMC_VENDOR_UNIQUE_REV_SIZE];

    UINT_8E         ipmi_type_length_21             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            vendor_acdc_input_type          [IPMI_RESUME_BOARD_INFO_EMC_VENDOR_ACDC_INPUT_TYPE_SIZE];

    UINT_8E         ipmi_type_length_22             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            vendor_serial_num               [IPMI_RESUME_BOARD_INFO_EMC_VENDOR_SERIAL_NUM_SIZE];

    UINT_8E         ipmi_type_length_23             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            loc_mft                         [IPMI_RESUME_BOARD_INFO_EMC_LOCATION_MANUFACTURE_SIZE];

    UINT_8E         ipmi_type_length_24             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            emc_alt_mb_part_num             [IPMI_RESUME_BOARD_INFO_EMC_ALT_MB_PART_SIZE];

    UINT_8E         ipmi_type_length_25             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            emc_sub_assy_part_num           [IPMI_RESUME_BOARD_INFO_EMC_SUB_ASSY_PART_NUM_SIZE];

    UINT_8E         ipmi_type_length_26             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            emc_sub_assy_artwork_rev        [IPMI_RESUME_BOARD_INFO_EMC_SUB_ASSY_ARTWORK_REV_SIZE];

    UINT_8E         ipmi_type_length_27             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            emc_sub_assy_rev                [IPMI_RESUME_BOARD_INFO_EMC_SUB_ASSY_REV_SIZE];

    UINT_8E         ipmi_type_length_28             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            emc_sub_assy_serial_num         [IPMI_RESUME_BOARD_INFO_EMC_SUB_ASSY_SERIAL_NUM_SIZE];

    UINT_8E         ipmi_type_length_29             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    UINT_32E        wwn_seed;

    UINT_8E         ipmi_type_length_30             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    UINT_8E         sas_address                     [IPMI_RESUME_BOARD_INFO_EMC_SAS_ADDRESS_SIZE];

    UINT_8E         ipmi_type_length_31             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    UINT_8E         sp_family_fru_id                [IPMI_RESUME_BOARD_INFO_EMC_FAMILY_FRU_ID_SIZE];

    UINT_8E         ipmi_type_length_32             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    UINT_8E         channel_speed                   [IPMI_RESUME_BOARD_INFO_EMC_CHANNEL_SPEED_SIZE];

    UINT_8E         ipmi_type_length_33             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    UINT_8E         slot_id                         [IPMI_RESUME_BOARD_INFO_EMC_SLOT_ID_SIZE];

    UINT_8E         ipmi_type_length_34             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    UINT_8E         drive_spin_up_select            [IPMI_RESUME_BOARD_INFO_EMC_DRIVE_SPIN_UP_SELECT_SIZE];

    UINT_8E         ipmi_type_length_35             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    UINT_8E         rp_semaphore                    [IPMI_RESUME_BOARD_INFO_EMC_SEMAPHORE_SIZE];

    UINT_8E         ipmi_type_length_36             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    UINT_8E         pcie_configuration              [IPMI_RESUME_BOARD_INFO_PCIE_CONFIGURATION_SIZE];

    UINT_8E         ipmi_type_length_47             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    UINT_8E         board_power                     [IPMI_RESUME_BOARD_INFO_BOARD_POWER_SIZE];

    UINT_8E         ipmi_type_length_48             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    UINT_8E         thermal_target                  [IPMI_RESUME_BOARD_INFO_THERMAL_TARGET_SIZE];

    UINT_8E         ipmi_type_length_49             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    UINT_8E         thermal_shutdown_limit          [IPMI_RESUME_BOARD_INFO_THERMAL_SHUTDOWN_LIMIT_SIZE];

    UINT_8E         ipmi_type_length_50             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    UINT_8E         fru_capability                  [IPMI_RESUME_BOARD_INFO_FRU_CAPABILITY_SIZE];

    UINT_8E         ipmi_type_length_37             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    UINT_8E         board_info_area_cksum           [IPMI_RESUME_BOARD_INFO_CKSUM_SIZE];

    UINT_8E         product_info_area_format        [IPMI_RESUME_PRODUCT_INFO_AREA_FORMAT_VER_SIZE];
    UINT_8E         product_info_area_length        [IPMI_RESUME_PRODUCT_INFO_AREA_LENGTH_SIZE];
    UINT_8E         product_language_code           [IPMI_RESUME_PRODUCT_INFO_LANGUAGE_CODE_SIZE];

    UINT_8E         ipmi_type_length_51             [IPMI_RESUME_TYPE_LENGTH_SIZE]; //Manufacturer Name, EMPTY
    UINT_8E         ipmi_type_length_52             [IPMI_RESUME_TYPE_LENGTH_SIZE]; //Product Name, EMPTY

    UINT_8E         ipmi_type_length_53             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            product_part_number             [IPMI_RESUME_PRODUCT_INFO_PROD_PN_BYTES_SIZE];

    UINT_8E         ipmi_type_length_54             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            product_revision                [IPMI_RESUME_PRODUCT_INFO_PROD_VER_BYTES_SIZE];

    UINT_8E         ipmi_type_length_55             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    CHAR            product_serial_number           [IPMI_RESUME_PRODUCT_INFO_PROD_SN_BYTES_SIZE];

    UINT_8E         ipmi_type_length_56             [IPMI_RESUME_TYPE_LENGTH_SIZE]; //Asset Tag, EMPTY
    UINT_8E         ipmi_type_length_57             [IPMI_RESUME_TYPE_LENGTH_SIZE]; //FRU File ID, EMPTY

    UINT_8E         ipmi_type_length_38             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    UINT_8E         system_type                     [IPMI_RESUME_PRODUCT_INFO_EMC_SYSTEM_TYPE_SIZE];

    UINT_8E         ipmi_type_length_39             [IPMI_RESUME_TYPE_LENGTH_SIZE];
    csx_uchar_t     ipmi_empty_space_2              [IPMI_RESUME_PRODUCT_INFO_EMPTY_SPACE_1_SIZE];

    UINT_8E         product_info_area_cksum         [IPMI_RESUME_PRODUCT_INFO_CKSUM_SIZE];

    UINT_8E         internal_info_format            [IPMI_RESUME_INTERNAL_INFO_AREA_FORMAT_VER_SIZE];
    csx_uchar_t     ipmi_empty_space_3              [IPMI_RESUME_INTERNAL_INFO_EMPTY_SPACE_1_SIZE];

    UINT_8E         num_prog                        [IPMI_RESUME_INTERNAL_INFO_EMC_PROGRAMABLES_SIZE];
    csx_uchar_t     ipmi_empty_space_4              [IPMI_RESUME_INTERNAL_INFO_EMPTY_SPACE_2_SIZE];

    IPMI_RESUME_PROM_PROG_DETAILS prog_details      [IPMI_RESUME_PROM_MAX_PROG_COUNT];

    CHAR            contact_information             [IPMI_RESUME_INTERNAL_INFO_EMC_CONTACT_INFO_SIZE];
    csx_uchar_t     ipmi_empty_space_5              [IPMI_RESUME_INTERNAL_INFO_EMPTY_SPACE_3_SIZE];

    //UINT_8E         extended_resume                 [IPMI_RESUME_INTERNAL_INFO_EXTENDED_RESUME_SIZE]; // do we want to read this?
} IPMI_RESUME_PROM_STRUCTURE;
#pragma pack(pop, ipmi_resume_prom_info)

#endif /* RESUME_PROM_H */

