/***************************************************************************
 * Copyright (C) EMC Corporation 2011 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/
#ifndef FBE_PEER_BOOT_UTILS_H
#define FBE_PEER_BOOT_UTILS_H

/*!**************************************************************************
 * @file fbe_peer_boot_utils.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the defination required for board_mgmt object functionality.
 *
 * @ingroup board_mgmt_class_files
 * 
 * @version
 *  04-Jan-2011: Created  Vaibhav Gaonkar
 *
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_types.h"
#include "fbe/fbe_board.h"
#include "spid_types.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe/fbe_devices.h"
#include "resume_prom.h"


#define FBE_MAX_SUBFRU_REPLACEMENTS            4       // max number of subfrus associated with a fault
#define FBE_BOARD_MGMT_TLA_FULL_LENGTH         (RESUME_PROM_EMC_TLA_PART_NUM_SIZE+1)
#define FBE_INVALID_FLT_EXP_ID 0XFF

/*!****************************************************************************
 * 
 * @enum fbe_boot_states_s
 *  
 * @brief 
 *  All boot codes get reduced to one of these boot 
 *  state symbols This is basically refer from  _cm_boot_states  of flare.
 *
 ******************************************************************************/
typedef enum fbe_boot_states_s
{
    FBE_PEER_SUCCESS,
    FBE_PEER_DEGRADED,
    FBE_PEER_BOOTING,
    FBE_PEER_FAILURE,
    FBE_PEER_HUNG,
    FBE_PEER_DUMPING,
    FBE_PEER_UNKNOWN,
    FBE_PEER_SW_LOADING,
    FBE_PEER_DRIVER_LOAD_ERROR,
    FBE_PEER_POST,
    FBE_PEER_INVALID_CODE = 255
} fbe_peer_boot_states_t;

/*!****************************************************************************
 * 
 * @enum fbe_subfru_replacements_s
 *  
 * @brief 
 *  This is enum for sub fru associated with each fault expander code.
 *
 ******************************************************************************/
typedef enum fbe_subfru_replacements_s
{
    FBE_SUBFRU_NONE,
    FBE_SUBFRU_CPU,
    // All the IO modules have to be in order. Do not change their orders.
    FBE_SUBFRU_IO_0, 
    FBE_SUBFRU_IO_1,
    FBE_SUBFRU_IO_2,
    FBE_SUBFRU_IO_3,
    FBE_SUBFRU_IO_4,
    FBE_SUBFRU_IO_5,
    FBE_SUBFRU_BEM,
    FBE_SUBFRU_ALL_DIMMS,
    FBE_SUBFRU_DIMM_0,
    FBE_SUBFRU_DIMM_1,
    FBE_SUBFRU_DIMM_2,
    FBE_SUBFRU_DIMM_3,
    FBE_SUBFRU_DIMM_4,
    FBE_SUBFRU_DIMM_5,
    FBE_SUBFRU_DIMM_6,
    FBE_SUBFRU_DIMM_7,
    FBE_SUBFRU_VRM_0,
    FBE_SUBFRU_VRM_1,
    FBE_SUBFRU_MANAGEMENT,
    FBE_SUBFRU_ENCL,
    FBE_SUBFRU_SFP,
    FBE_SUBFRU_CACHE_CARD,
    FBE_SUBFRU_MANAGEMENTA,
    FBE_SUBFRU_MANAGEMENTB,
    FBE_SUBFRU_CF,
    FBE_SUBFRU_UDOC,
    FBE_SUBFRU_BLADE,
    FBE_SUBFRU_DW_IO_0,
    FBE_SUBFRU_DW_IO_1
} fbe_subfru_replacements_t;

/*!****************************************************************************
 * 
 * @struct fbe_peer_boot_entry_s
 *  
 * @brief 
 *  The structure defined to hold the pre-defined fault expander status code and 
 *  there associated values.
 *
 ******************************************************************************/
typedef struct fbe_peer_boot_entry_s
{
    fbe_u8_t                    code;           /*!<  fault/status code */
    fbe_u8_t                    *description;   /*!<  fault/status code description */
    fbe_peer_boot_states_t      state;          /*!<  boot state (hang, fail, booting, etc) */
    fbe_u32_t                   timeout;        /*!<  Seconds allowed for status change */
    fbe_u32_t                   numberOfSubFrus;/*!<  number of fault subfrus with part numbers */
    fbe_subfru_replacements_t   replace[FBE_MAX_SUBFRU_REPLACEMENTS]; /*!<  subfru replacement code */
} fbe_peer_boot_entry_t;

 /*!****************************************************************************
 * 
 * @enum fbe_peer_boot_code_enum
 *  
 * @brief 
 *  Indicates the most recent stage the peer entered
 *
 ******************************************************************************/
typedef enum fbe_peer_boot_code_enum_e
{
    FBE_PEER_BOOT_FAULT_CPU        = 0,
    FBE_PEER_BOOT_FAULT_IO_0,        // 0x01
    FBE_PEER_BOOT_FAULT_IO_1,        // 0x02
    FBE_PEER_BOOT_FAULT_BEM,         // 0x03
    FBE_PEER_BOOT_FAULT_CPU_IO_0,    // 0x04
    FBE_PEER_BOOT_FAULT_CPU_IO_1,    // 0x05
    FBE_PEER_BOOT_FAULT_CPU_ANNEX,   // 0x06
    FBE_PEER_BOOT_FAULT_CPU_DIMMS,   // 0x07
    FBE_PEER_BOOT_FAULT_DIMMS,       // 0x08
    FBE_PEER_BOOT_FAULT_DIMM_0,      // 0x09
    FBE_PEER_BOOT_FAULT_DIMM_1,      // 0x0A
    FBE_PEER_BOOT_FAULT_DIMM_2,      // 0x0B
    FBE_PEER_BOOT_FAULT_DIMM_3,      // 0x0C
    FBE_PEER_BOOT_FAULT_DIMM_4,      // 0x0D
    FBE_PEER_BOOT_FAULT_DIMM_5,      // 0x0E
    FBE_PEER_BOOT_FAULT_DIMM_6,      // 0x0F
    FBE_PEER_BOOT_FAULT_DIMM_7,      // 0x10
    FBE_PEER_BOOT_FAULT_DIMM_0_1,    // 0x11
    FBE_PEER_BOOT_FAULT_DIMM_2_3,    // 0x12
    FBE_PEER_BOOT_FAULT_DIMM_4_5,    // 0x13
    FBE_PEER_BOOT_FAULT_DIMM_6_7,    // 0x14
    FBE_PEER_BOOT_FAULT_VRM_0,       // 0x15
    FBE_PEER_BOOT_FAULT_VRM_1,       // 0x16
    FBE_PEER_BOOT_FAULT_MANAGEMENT,  // 0x17
    FBE_PEER_BOOT_FAULT_ANNEX_MIDFUSE,   // 0x18
    FBE_PEER_BOOT_FAULT_CANT_ISOLATE,    // 0x19
    FBE_PEER_BOOT_FAULT_ENCL_MID,        // 0x1A
    FBE_PEER_BOOT_FAULT_SFP,             // 0x1B
    FBE_PEER_BOOT_FAULT_DEGRADED    = 0x25,
    FBE_PEER_BOOT_FAULT_CMI_PATH,        // 0x26
    FBE_PEER_BOOT_FAULT_ILL_CONFIG,      // 0x27
    FBE_PEER_BOOT_FAULT_SW_IMAGE,        // 0x28
    FBE_PEER_BOOT_FAULT_DISK_ACCESS,     // 0x29
    FBE_PEER_BOOT_LOCAL_BLADE_FW_UPDATE, // 0x2A
    FBE_PEER_BOOT_SW_PANIC_CORE_DUMP,    // 0x2B
    FBE_PEER_BOOT_SUCCESS,               // 0x2C
    FBE_PEER_BOOT_OS_RUNNING,            // 0x2D
    FBE_PEER_BOOT_REMOTE_FRU_FW_UPDATE,  // 0x2E
    FBE_PEER_BOOT_BLADE_SERVICED,        // 0x2F
    FBE_PEER_BOOT_POST_TEST_BLADE   = 0x34,
    FBE_PEER_BOOT_POST_TEST_CPU_IO_0,    // 0x35
    FBE_PEER_BOOT_POST_TEST_CPU_IO_1,    // 0x36
    FBE_PEER_BOOT_POST_TEST_CPU_ANNEX,   // 0x37
    FBE_PEER_BOOT_POST_TEST_CPU_DIMMS,   // 0x38
    FBE_PEER_BOOT_POST_TEST_CPU,         // 0x39
    FBE_PEER_BOOT_POST_TEST_COMPLETE,    // 0x3A
    FBE_PEER_BOOT_BIOS_TEST_CPU_IO_0,    // 0x3B
    FBE_PEER_BOOT_BIOS_TEST_CPU_IO_1,    // 0x3C
    FBE_PEER_BOOT_BIOS_TEST_CPU_ANNEX,   // 0x3D
    FBE_PEER_BOOT_BIOS_TEST_CPU_DIMMS,   // 0x3E
    FBE_PEER_BOOT_BIOS_TEST_CPU_CODE0,   // 0x3F
    FBE_PEER_BOOT_FAULT_IO_2,            // 0x40
    FBE_PEER_BOOT_FAULT_IO_3,            // 0x41
    FBE_PEER_BOOT_FAULT_IO_4,            // 0x42
    FBE_PEER_BOOT_FAULT_IO_5,            // 0x43
    FBE_PEER_BOOT_FAULT_DOUBLE_WIDE_IO_0,// 0x44
    FBE_PEER_BOOT_FAULT_DOUBLE_WIDE_IO_1,// 0x45
    FBE_PEER_BOOT_POST_TEST_CPU_IO_2,    // 0x46
    FBE_PEER_BOOT_POST_TEST_CPU_IO_3,    // 0x47
    FBE_PEER_BOOT_POST_TEST_CPU_IO_4,    // 0x48
    FBE_PEER_BOOT_POST_TEST_CPU_IO_5,    // 0x49
    FBE_PEER_BOOT_POST_TEST_CPU_DOUBLE_WIDE_IO_0, //0x4A
    FBE_PEER_BOOT_POST_TEST_CPU_DOUBLE_WIDE_IO_1, //0x4B
    FBE_PEER_BOOT_BIOS_TEST_CPU_IO_2,    // 0x4C
    FBE_PEER_BOOT_BIOS_TEST_CPU_IO_3,    // 0x4D
    FBE_PEER_BOOT_BIOS_TEST_CPU_IO_4,    // 0x4E
    FBE_PEER_BOOT_BIOS_TEST_CPU_IO_5,    // 0x4F
    FBE_PEER_BOOT_BIOS_TEST_CPU_DOUBLE_WIDE_IO_0,// 0x50
    FBE_PEER_BOOT_BIOS_TEST_CPU_DOUBLE_WIDE_IO_1,// 0x51
    FBE_PEER_BOOT_POST_TEST_ANNEX_IO_4,          // 0x52
    FBE_PEER_BOOT_POST_TEST_ANNEX_IO_5,          // 0x53
    FBE_PEER_BOOT_POST_TEST_ANNEX_IO_4_5,        // 0x54
    FBE_PEER_BOOT_POST_TEST_CPU_ANNEX_IO_4,      // 0x55
    FBE_PEER_BOOT_POST_TEST_CPU_ANNEX_IO_5,      // 0x56
    FBE_PEER_BOOT_POST_TEST_CPU_ANNEX_IO_4_5,    // 0x57
    FBE_PEER_BOOT_POST_TEST_CPU_MGMT_A,          // 0x58
    FBE_PEER_BOOT_POST_TEST_CPU_MGMT_B,          // 0x59
    FBE_PEER_BOOT_POST_TEST_CPU_MGMT_A_B,        // 0x5A
    FBE_PEER_BOOT_FAULT_COMPACT_FLASH,           // 0x5B
    FBE_PEER_BOOT_POST_TEST_COMPACT_FLASH,       // 0x5C
    FBE_PEER_BOOT_BIOS_TEST_COMPACT_FLASH,       // 0x5D
    FBE_PEER_BOOT_POST_TEST_CPU_COMPACT_FLASH,   // 0x5E
    FBE_PEER_BOOT_POST_TEST_CPU_MODULE_ASSEMBLY, // 0x5F
    FBE_PEER_BOOT_FAULT_UDOC,                    // 0x60
    FBE_PEER_BOOT_POST_TEST_UDOC,                // 0x61
    FBE_PEER_BOOT_BIOS_TEST_UDOC,                // 0x62
    FBE_PEER_BOOT_POST_TEST_CPU_VRM_0,           // 0x63
    FBE_PEER_BOOT_POST_TEST_CPU_VRM_1,           // 0x64
    FBE_PEER_BOOT_POST_TEST_CPU_VRM_1_2,         // 0x65
    FBE_PEER_BOOT_FAULT_CPU_BOOTED_TO_FLASH,     // 0x66
    FBE_PEER_BOOT_FAULT_CPU_IMAGE_INCORRECT,     // 0x67
    FBE_PEER_BOOT_FAULT_CPU_IO_2,                // 0x68
    FBE_PEER_BOOT_FAULT_CPU_IO_3,                // 0x69
    FBE_PEER_BOOT_FAULT_CPU_IO_4,                // 0x6A
    FBE_PEER_BOOT_FAULT_CPU_IO_5,                // 0x6B
    FBE_PEER_BOOT_FAULT_CPU_ANNEX_IO_4,          // 0x6C
    FBE_PEER_BOOT_FAULT_CPU_ANNEX_IO_5,          // 0x6D
    FBE_PEER_BOOT_FAULT_DIMM_0_2,                // 0x6E
    FBE_PEER_BOOT_FAULT_DIMM_1_3,                // 0x6F
    FBE_PEER_BOOT_FAULT_DIMM_4_6,                // 0x70
    FBE_PEER_BOOT_FAULT_DIMM_5_7,                // 0x71
    FBE_PEER_BOOT_FAULT_DIMM_0_1_2_3,            // 0x72
    FBE_PEER_BOOT_FAULT_DIMM_4_5_6_7,            // 0x73
    FBE_PEER_BOOT_FAULT_MRC,                     // 0x74
    FBE_PEER_BOOT_FAULT_CPU_ANNEX_IO_4_IO_5,     // 0x75
    FBE_PEER_BOOT_USB_START,                     // 0x76
    FBE_PEER_BOOT_USB_POST,                      // 0x77
    FBE_PEER_BOOT_USB_ENGINUITY,                 // 0x78
    FBE_PEER_BOOT_USB_SUCESS,                    // 0x79
    FBE_PEER_BOOT_USB_POST_FAIL,                 // 0x7A
    FBE_PEER_BOOT_USB_ENGINUITY_FAIL,            // 0x7B
    FBE_PEER_BOOT_INIT_PATH,                     // 0x7C
    FBE_PEER_BOOT_MEM_MISMATCH,                  // 0x7D
    FBE_PEER_BOOT_BIOS_TEST_CPU_CODE1  = 0xFF    // 0xFF
}fbe_peer_boot_code_t;
/*!********************************************************************* 
 * @struct fbe_peer_boot_fault_reg_info_s 
 *  
 * @brief 
 *   This struct use to store peer SP info in fault register, to send to upper layer software.
 *
 * @ingroup fbe_api_esp_board_mgmt_interface
 * @ingroup fbe_esp_board_mgmt_board_info
 **********************************************************************/
typedef struct fbe_peer_boot_fault_reg_info_s
{
    BOOL                    dimmFault[MAX_DIMM_FAULT];
    BOOL                    driveFault[MAX_DRIVE_FAULT];
    BOOL                    batteryFault[MAX_BATTERY_FAULT];
    BOOL                    superCapFault;
    BOOL                    i2cFault[MAX_I2C_FAULT];
    BOOL                    cpuFault;
    BOOL                    eFlashFault;
    BOOL                    cacheCardFault;
    BOOL                    midplaneFault;
    BOOL                    cmiFault;
    BOOL                    allFrusFault;
    BOOL                    externalFruFault;
    BOOL                    anyFaultsFound; /* Indicates that any one of the bits above is set */
    fbe_u32_t               cpuStatusRegister;
}fbe_peer_boot_fault_reg_info_t;

typedef enum fbe_peer_boot_fru_s
{
    PBL_FRU_INVALID = 0x00,
    PBL_FRU_DIMM,
    PBL_FRU_DRIVE,
    PBL_FRU_SLIC,
    PBL_FRU_POWER,
    PBL_FRU_BATTERY,
    PBL_FRU_SUPERCAP,
    PBL_FRU_FAN,
    PBL_FRU_I2C,
    PBL_FRU_CPU,
    PBL_FRU_MGMT,
    PBL_FRU_BM,
    PBL_FRU_EFLASH,
    PBL_FRU_CACHECARD,
    PBL_FRU_MIDPLANE,
    PBL_FRU_CMI,
    PBL_FRU_ALL_FRU,
    PBL_FRU_EXTERNAL_FRU,
    //Above fru types have fault value in FSR
    PBL_FRU_ALL_DIMM,
    PBL_FRU_ONBOARD_DRIVE,
    PBL_FRU_LAST
} fbe_peer_boot_fru_t;

typedef struct fbe_peer_boot_bad_fru_s
{
    fbe_peer_boot_fru_t         type;           
    fbe_u32_t                   id;             /*!<  index/slot of bad FRU */
} fbe_peer_boot_bad_fru_t;
/*!****************************************************************************
 * 
 * @struct fbe_peer_boot_cpu_reg_entry_s
 *  
 * @brief 
 *  The structure defined to hold the pre-defined CPU register status code and 
 *  their associated values.
 *
 ******************************************************************************/
typedef struct fbe_peer_boot_cpu_reg_entry_s
{
    fbe_u32_t                   status;                                     /*!<  CPU register status code */
    fbe_u32_t                   timeout;                                    /*!<  Seconds allowed for status change */
    fbe_peer_boot_states_t      bootState;                                  /*!<  boot state (hang, fail, booting, etc) */
    fbe_peer_boot_bad_fru_t     badFru[FBE_MAX_SUBFRU_REPLACEMENTS];        /*!<  bad FRU to replace while peer halted */
} fbe_peer_boot_cpu_reg_entry_t;

#define FBE_PEER_BOOT_CODE_LIMIT1     FBE_PEER_BOOT_MEM_MISMATCH         // 0x7D
#define FBE_PEER_BOOT_CODE_LAST       FBE_PEER_BOOT_BIOS_TEST_CPU_CODE1  // 0xFF
#define FBE_PEER_STATUS_DEGRADED_MODE 0x25
#define FBE_PEER_STATUS_UNKNOW        0xFE 
#define FBE_IS_USB_LOADER(m_code) (((m_code >= FBE_PEER_BOOT_USB_START)&&(m_code <= FBE_PEER_BOOT_USB_SUCESS)) ? TRUE : FALSE)

/*
 * board_mgmt utils function prototypes
 */
fbe_status_t fbe_board_mgmt_get_device_type_and_location(SP_ID sp, 
                                            fbe_subfru_replacements_t subFru, 
                                            fbe_device_physical_location_t *device_location, 
                                            fbe_u64_t *device_type);
fbe_status_t fbe_board_mgmt_get_tla_num(fbe_u64_t device_type,
                           fbe_device_physical_location_t *device_location,
                           fbe_u8_t *bufferPtr,
                           fbe_u32_t bufferSize);
fbe_u8_t * fbe_board_mgmt_utils_get_suFruMessage(fbe_subfru_replacements_t subFru);
const char *fbe_pbl_decodeFru(fbe_peer_boot_fru_t  fru);
fbe_u64_t fbe_pbl_mapFbeDeviceType(fbe_peer_boot_fru_t  fru);
fbe_bool_t fbe_pbl_BootPeerSuccessful(fbe_peer_boot_states_t bootState);
const char* fbe_pbl_decodeBootState(fbe_peer_boot_states_t bootState);
fbe_peer_boot_states_t fbe_pbl_getPeerBootState(SPECL_FAULT_REGISTER_STATUS  *fltRegStatus);
fbe_u32_t fbe_pbl_getPeerBootTimeout(SPECL_FAULT_REGISTER_STATUS  *fltRegStatus);
fbe_u32_t fbe_pbl_getBadFruNum(fbe_u32_t cpuStatus);
fbe_status_t fbe_pbl_getBadFru(fbe_u32_t                 cpuStatus,
                               fbe_u32_t                 fruIndex,
                               fbe_peer_boot_bad_fru_t * badFru);

extern fbe_peer_boot_entry_t            fbe_PeerBootEntries[];
#endif // FBE_PEER_BOOT_UTILS_H
