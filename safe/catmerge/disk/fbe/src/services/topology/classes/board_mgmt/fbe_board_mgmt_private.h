/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/
#ifndef BOARD_MGMT_PRIVATE_H
#define BOARD_MGMT_PRIVATE_H
/*!**************************************************************************
 * @file fbe_board_mgmt_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains declarations for the ESP Board Mgmt object.
 * 
 * @ingroup board_mgmt_class_files
 * 
 * @revision
 *  29-July-2010: Created  Vaibhav Gaonkar
 *
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_base_object.h"
#include "fbe/fbe_board.h"
#include "fbe/fbe_time.h"
#include "base_object_private.h"
#include "fbe_base_environment_private.h"
#include "fbe/fbe_peer_boot_utils.h"
#include "specl_types.h"
#include "fbe/fbe_transport.h"

/* FUP related macros */
#define FBE_BOARD_MGMT_CDES_FUP_IMAGE_PATH_KEY   "CDESLccImagePath"
#define FBE_BOARD_MGMT_CDES_FUP_IMAGE_FILE_NAME  "SXP36x6G_Bundled_Fw"
#define FBE_BOARD_MGMT_CDES_UNIFIED_FUP_IMAGE_PATH_KEY   "CDESUnifiedLccImagePath"
#define FBE_BOARD_MGMT_CDES_UNIFIED_FUP_IMAGE_FILE_NAME  "CDES_Bundled_FW"

#define FBE_BOARD_MGMT_MAX_PROGRAMMABLE_COUNT_PER_SP_SLOT  1

#define FBE_BOARD_MGMT_CDES2_FUP_IMAGE_PATH_KEY             "CDES2LccImagePath"
#define FBE_BOARD_MGMT_CDES2_FUP_CDEF_IMAGE_FILE_NAME       "cdef"
#define FBE_BOARD_MGMT_CDES2_FUP_CDEF_DUAL_IMAGE_FILE_NAME  "cdef_dual"
#define FBE_BOARD_MGMT_CDES2_FUP_ISTR_IMAGE_FILE_NAME       "istr"
#define FBE_BOARD_MGMT_CDES2_FUP_CDES_ROM_IMAGE_FILE_NAME   "cdes_rom"

#define FBE_BOARD_MGMT_CDES2_FUP_MANIFEST_FILE_PATH_KEY  "CDES2LccImagePath"
#define FBE_BOARD_MGMT_CDES2_FUP_MANIFEST_FILE_NAME  "manifest_lcc"
#define FBE_MAX_SSD_SLOTS_PER_SP 2


/* Lifecycle definitions
 * Define the Board management lifecycle's instance.
 */
extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(board_mgmt);

/* These are the lifecycle condition ids for Board
   Management object.*/

/*! @enum fbe_board_mgmt_lifecycle_cond_id_t  
 *  
 *  @brief These are the lifecycle conditions for the board
 *  management object.
 */
typedef enum fbe_board_mgmt_lifecycle_cond_id_e 
{
    /*! Processing of board
     */
    FBE_BOARD_MGMT_LIFECYCLE_COND_DISCOVER_BOARD = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_BOARD_MGMT),

    FBE_BOARD_MGMT_PEER_BOOT_CHECK_COND,

    FBE_BOARD_MGMT_LIFECYCLE_COND_LAST /* must be last */
} 
fbe_board_mgmt_lifecycle_cond_id_t;

/*!****************************************************************************
 * 
 * @struct fbe_board_mgmt_bmc_bist_bitmask_e
 *  
 * @brief 
 *  This enumeration defines a bitmask for BMC BIST status
 *
 ******************************************************************************/
typedef enum fbe_board_mgmt_bmc_bist_bitmask_e
{
    FBE_BOARD_MGMT_BMC_BIST_NO_FAULTS                   = 0x00000000,

    FBE_BOARD_MGMT_BMC_BIST_CPU_TEST_FAILURE            = 0x00000001,
    FBE_BOARD_MGMT_BMC_BIST_DRAM_TEST_FAILURE           = 0x00000002,
    FBE_BOARD_MGMT_BMC_BIST_SRAM_TEST_FAILURE           = 0x00000004,
    FBE_BOARD_MGMT_BMC_BIST_I2C_TEST_FAILURE            = 0x00000008,

    FBE_BOARD_MGMT_BMC_BIST_FAN0_TEST1_FAILURE          = 0x00000010,
    FBE_BOARD_MGMT_BMC_BIST_FAN0_TEST2_FAILURE          = 0x00000020,
    FBE_BOARD_MGMT_BMC_BIST_FAN0_TEST3_FAILURE          = 0x00000040,
    FBE_BOARD_MGMT_BMC_BIST_FAN1_TEST1_FAILURE          = 0x00000080,

    FBE_BOARD_MGMT_BMC_BIST_FAN1_TEST2_FAILURE          = 0x00000100,
    FBE_BOARD_MGMT_BMC_BIST_FAN1_TEST3_FAILURE          = 0x00000200,
    FBE_BOARD_MGMT_BMC_BIST_FAN2_TEST1_FAILURE          = 0x00000400,
    FBE_BOARD_MGMT_BMC_BIST_FAN2_TEST2_FAILURE          = 0x00000800,

    FBE_BOARD_MGMT_BMC_BIST_FAN2_TEST3_FAILURE          = 0x00001000,
    FBE_BOARD_MGMT_BMC_BIST_FAN3_TEST1_FAILURE          = 0x00002000,
    FBE_BOARD_MGMT_BMC_BIST_FAN3_TEST2_FAILURE          = 0x00004000,
    FBE_BOARD_MGMT_BMC_BIST_FAN3_TEST3_FAILURE          = 0x00008000,

    FBE_BOARD_MGMT_BMC_BIST_FAN4_TEST1_FAILURE          = 0x00010000,
    FBE_BOARD_MGMT_BMC_BIST_FAN4_TEST2_FAILURE          = 0x00020000,
    FBE_BOARD_MGMT_BMC_BIST_FAN4_TEST3_FAILURE          = 0x00040000,
    FBE_BOARD_MGMT_BMC_BIST_FAN5_TEST_FAILURE           = 0x00080000,

    FBE_BOARD_MGMT_BMC_BIST_BBU_TEST_FAILURE            = 0x00100000,
    FBE_BOARD_MGMT_BMC_BIST_SSP_TEST_FAILURE            = 0x00200000,
    FBE_BOARD_MGMT_BMC_BIST_NVRAM_TEST_FAILURE          = 0x00400000,
    FBE_BOARD_MGMT_BMC_BIST_SGPIO_TEST_FAILURE          = 0x00800000,

    FBE_BOARD_MGMT_BMC_BIST_USB_TEST_FAILURE            = 0x01000000,
    FBE_BOARD_MGMT_BMC_BIST_VIRT_UART0_TEST_FAILURE     = 0x02000000,
    FBE_BOARD_MGMT_BMC_BIST_VIRT_UART1_TEST_FAILURE     = 0x04000000,
    FBE_BOARD_MGMT_BMC_BIST_UART2_TEST_FAILURE          = 0x08000000,

    FBE_BOARD_MGMT_BMC_BIST_UART3_TEST_FAILURE          = 0x10000000,
    FBE_BOARD_MGMT_BMC_BIST_UART4_TEST_FAILURE          = 0x20000000,
} fbe_board_mgmt_bmc_bist_bitmask_t;

/*!****************************************************************************
 * 
 * @struct fbe_board_mgmt_peer_boot_info_s
 *  
 * @brief 
 *  This struct define the peer boot state recevied from Physical Package. 
 *  These status are filled by peer SP's BIOS and POST firmware.
 *
 ******************************************************************************/
typedef struct fbe_board_mgmt_peer_boot_info_s{
    SP_ID                   peerSp;                     /*!<  The fru associated SP ID */
    fbe_u64_t       faultHwType;                  /* fault expander or fault register */
    fbe_u8_t                peerBootCode;               /*!<  Fault Expander status recevied from Physical Package*/
    fbe_peer_boot_states_t  peerBootState;              /*!<  Peer SP boot state */                               
    fbe_env_inferface_status_t     fltExpEnvInterfaceStatus;
    fbe_env_inferface_status_t     fltRegEnvInterfaceStatus;
    /* Timer related variables */
    fbe_time_t              StateChangeStartTime;       /*!<  This is time at which fault expander state change */
    fbe_bool_t              stateChangeProcessRequired; /*!<  Set when flt exp status required to process*/

    /* Fault register part */
    SPECL_FAULT_REGISTER_STATUS fltRegStatus;
    fbe_system_time_t           lastFltRegClearTime;    /*!<  Timestamp of last clearance of peer fault register */
    SPECL_FAULT_REGISTER_STATUS lastFltRegStatus;       /*!<  Peer fault register values before cleared by local ESP */
    fbe_bool_t                  biosPostFail;
    fbe_bool_t                  isPeerSpApplicationRunning;
}fbe_board_mgmt_peer_boot_info_t;

/*!****************************************************************************
 * 
 * @struct fbe_board_mgmt_extended_peer_boot_info_s
 *  
 * @brief 
 *  This struct define the extended peer boot state recevied from Physical Package. 
 *  This part of status are filled by peer SP's software.
 *
 ******************************************************************************/
typedef struct fbe_board_mgmt_ext_peer_boot_info_s{
    SP_ID                   peerSp;                     /*!<  The fru associated SP ID */
    fbe_env_inferface_status_t     envInterfaceStatus;
    fbe_u8_t                generalStatus;
    fbe_u8_t                componentStatus;
    fbe_u8_t                componentExtStatus;
}fbe_board_mgmt_ext_peer_boot_info_t;

/*!****************************************************************************
 * 
 * @struct fbe_board_mgmt_s
 *  
 * @brief 
 *   This is the definition of the board mgmt object. This object
 *   deals with handling board related functions
 *
 ******************************************************************************/
typedef struct fbe_board_mgmt_s {
    /*! This must be first.  This is the base object we inherit from.
     */
    fbe_base_environment_t base_environment;

    fbe_object_id_t                     object_id;      // Board Object ID 
    fbe_object_id_t                     peObjectId; // If this is XPE, it is FBE_OBJECT_ID_INVALID.If this is DPE, it is valid.
    fbe_bool_t                          spInserted;  // Local SP inserted or not
    fbe_bool_t                          peerInserted; // Peer SP inserted or not
    FBE_TRI_STATE                       peerPriorityDriveAccess; // Peer SP has priority access to system drives
    fbe_bool_t                          lowBattery; //CPU Battery is low or not
    SPID_PLATFORM_INFO                  platform_info;
    fbe_u32_t                           suitcaseCountPerBlade;
    fbe_board_mgmt_suitcase_info_t      suitcase_info[SP_ID_MAX][MAX_SUITCASE_COUNT_PER_BLADE];
    fbe_board_mgmt_bmc_info_t           bmc_info[SP_ID_MAX][TOTAL_BMC_PER_BLADE];
    fbe_u32_t                           previousBmcBistBitmask[SP_ID_MAX][TOTAL_BMC_PER_BLADE];
    fbe_base_env_resume_prom_info_t    *resume_info; // [SP_ID_MAX] for SP resume prom.
    /* Maintain the peer SP boot state */
    fbe_board_mgmt_peer_boot_info_t     peerBootInfo;
    fbe_board_mgmt_ext_peer_boot_info_t extPeerBootInfo;
    
    fbe_common_cache_status_t           boardCacheStatus;
    fbe_bool_t                          peerSpDown;

    fbe_bool_t                          engineIdFault; 
    LED_BLINK_RATE                      UnsafeToRemoveLED; 
    fbe_u32_t                           spPhysicalMemorySize[SP_ID_MAX];

    fbe_u32_t                           lccCount;
    fbe_lcc_info_t                      lccInfo[SP_ID_MAX];  //Expander on SP board of DPE still report LCC status though there is no physical LCC component. 
    fbe_base_env_fup_persistent_info_t  board_fup_info[SP_ID_MAX]; // from SP board expander CDES firmeare upgrade. 

    fbe_u32_t                           cacheCardCount;
    fbe_board_mgmt_cache_card_info_t    cacheCardInfo[MAX_CACHE_CARD_COUNT];
    fbe_base_env_resume_prom_info_t     *cacheCardResumeInfo; //[MAX_CACHE_CARD_COUNT]
    fbe_bool_t                          cacheCardConfigFault; 
    
    fbe_u32_t                           dimmCount;
    fbe_board_mgmt_dimm_info_t          *dimmInfo; //TOTAL_DIMM_PER_BLADE * SP_ID_MAX

    fbe_u32_t                           ssdCount;
    fbe_board_mgmt_ssd_info_t           *ssdInfo; //FBE_MAX_SSD_SLOTS_PER_SP;

    fbe_cable_status_t                  fbeInternalCablePort1;

    /* Lifecycle defines. */
    FBE_LIFECYCLE_DEF_INST_DATA;
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_BOARD_MGMT_LIFECYCLE_COND_LAST));

}fbe_board_mgmt_t;

/*!****************************************************************************
 *    
 * @struct fbe_board_mgmt_cmi_msg_s
 *  
 * @brief 
 *   This is the definition of the BOARD mgmt object structure used for
 *   CMI (Peer-to-Peer) message communication.
 ******************************************************************************/
typedef struct fbe_board_mgmt_cmi_msg_s 
{
    fbe_base_environment_cmi_message_t  baseCmiMsg;
    fbe_u32_t                           spPhysicalMemorySize;
} fbe_board_mgmt_cmi_msg_t;

typedef void * fbe_board_mgmt_cmi_packet_t;
/*
 * board_mgmt function prototypes
 */
/* fbe_board_mgmt_main.c */
fbe_status_t fbe_board_mgmt_create_object(fbe_packet_t * packet, 
                                          fbe_object_handle_t * object_handle);
fbe_status_t fbe_board_mgmt_destroy_object(fbe_object_handle_t object_handle);
fbe_status_t fbe_board_mgmt_control_entry(fbe_object_handle_t object_handle, 
                                          fbe_packet_t * packet);
fbe_status_t fbe_board_mgmt_event_entry(fbe_object_handle_t object_handle, 
                                        fbe_event_type_t event_type, 
                                       fbe_event_context_t event_context);
fbe_status_t fbe_board_mgmt_monitor_entry(fbe_object_handle_t object_handle, 
                                          fbe_packet_t * packet);
fbe_status_t fbe_board_mgmt_monitor_entry(fbe_object_handle_t object_handle, 
                                          fbe_packet_t * packet);
fbe_status_t fbe_board_mgmt_monitor_load_verify(void);
fbe_status_t fbe_board_mgmt_init(fbe_board_mgmt_t * board_mgmt);

/* fbe_board_mgmt_resume_prom.c */
fbe_status_t fbe_board_mgmt_initiate_resume_prom_cmd(fbe_board_mgmt_t * pBoardMgmt,
                                                     fbe_u64_t deviceType,
                                                     fbe_device_physical_location_t * pLocation);

fbe_status_t fbe_board_mgmt_resume_prom_handle_board_status_change(fbe_board_mgmt_t * pBoardMgmt, 
                                                                   fbe_board_mgmt_misc_info_t *misc_info);

fbe_status_t fbe_board_mgmt_get_resume_prom_info_ptr(fbe_board_mgmt_t * board_mgmt,
                                                      fbe_u64_t deviceType,
                                                      fbe_device_physical_location_t * pLocation,
                                                      fbe_base_env_resume_prom_info_t ** pResumePromInfoPtr);
fbe_status_t fbe_board_mgmt_resume_prom_update_encl_fault_led(fbe_board_mgmt_t * board_mgmt,
                                                              fbe_u64_t deviceType,
                                                              fbe_device_physical_location_t *pLocation);
fbe_status_t fbe_board_mgmt_resume_prom_handle_resume_prom_info_change(fbe_board_mgmt_t * pBoardMgmt,
                                                                       fbe_u64_t deviceType,
                                                                       fbe_device_physical_location_t * pLocation);


fbe_status_t fbe_board_mgmt_resume_prom_handle_cache_card_status_change(fbe_board_mgmt_t * pBoardMgmt,
                                                 fbe_device_physical_location_t * pLocation,
                                                 fbe_board_mgmt_cache_card_info_t * pNewCacheCardInfo,
                                                 fbe_board_mgmt_cache_card_info_t * pOldCacheCardInfo);

/* Peer Boot log Functionality */
void fbe_board_mgmt_init_peerBootInfo(fbe_board_mgmt_peer_boot_info_t *peerBootInfo);
fbe_u32_t fbe_board_mgmt_getBootCodeSubFruCount(fbe_u8_t bootCode);
fbe_subfru_replacements_t fbe_board_mgmt_getBootCodeSubFruReplacements(fbe_u8_t bootCode, fbe_u32_t num);
fbe_bool_t fbe_board_mgmt_check_usb_loader(fbe_u8_t boot_code);
void fbe_board_mgmt_setPeerBootInfo(fbe_board_mgmt_t *board_mgmt,
                                    fbe_peer_boot_flt_exp_info_t  *fltRegInfoPtr);
fbe_bool_t fbe_board_mgmt_fltReg_checkBootTimeOut(fbe_board_mgmt_t              *board_mgmt);
fbe_bool_t fbe_board_mgmt_fltReg_checkBiosPostFault(fbe_board_mgmt_t             *board_mgmt,
                                                    fbe_peer_boot_flt_exp_info_t *fltRegInfoPtr);
fbe_bool_t fbe_board_mgmt_fltReg_checkNewFault(fbe_board_mgmt_t             *board_mgmt,
                                               fbe_peer_boot_flt_exp_info_t *fltRegInfoPtr);
fbe_status_t fbe_board_mgmt_fltReg_statusChange(fbe_board_mgmt_t              *board_mgmt,
                                                fbe_peer_boot_flt_exp_info_t  *fltRegInfoPtr);

fbe_status_t fbe_board_mgmt_slavePortStatusChange(fbe_board_mgmt_t                  *board_mgmt,
                                                  fbe_board_mgmt_slave_port_info_t  *slavePortInfoPtr);

fbe_status_t fbe_board_mgmt_update_encl_fault_led(fbe_board_mgmt_t *board_mgmt,
                                    fbe_device_physical_location_t *pLocation,
                                    fbe_encl_fault_led_reason_t enclFaultLedReason);

/* fbe_board_mgmt_kernel_main.c, fbe_board_mgmt_sim_main.c */
fbe_u32_t fbe_board_mgmt_get_local_sp_physical_memory(void);

/* fbe_board_mgmt_fup.c */
fbe_status_t fbe_board_mgmt_fup_handle_sp_lcc_status_change(fbe_board_mgmt_t * pBoardMgmt,
                                                 fbe_device_physical_location_t * pLocation,
                                                 fbe_lcc_info_t * pNewLccInfo,
                                                 fbe_lcc_info_t * pOldLccInfo);

fbe_status_t fbe_board_mgmt_fup_initiate_upgrade(void * pContext, 
                                              fbe_u64_t deviceType, 
                                              fbe_device_physical_location_t * pLocation,
                                              fbe_u32_t forceFlags,
                                              fbe_u32_t upgradeRetryCount);

fbe_status_t fbe_board_mgmt_fup_get_full_image_path(void * pContext,                               
                               fbe_base_env_fup_work_item_t * pWorkItem);

fbe_status_t fbe_board_mgmt_fup_get_manifest_file_full_path(void * pContext,
                                                           fbe_char_t * pManifestFileFullPath);

fbe_status_t fbe_board_mgmt_fup_check_env_status(void * pContext, 
                                 fbe_base_env_fup_work_item_t * pWorkItem);

fbe_status_t fbe_board_mgmt_fup_get_firmware_rev(void * pContext,
                          fbe_base_env_fup_work_item_t * pWorkItem,
                          fbe_u8_t * pFirmwareRev);

fbe_status_t fbe_board_mgmt_get_fup_info_ptr(void * pContext,
                                            fbe_u64_t deviceType,
                                            fbe_device_physical_location_t * pLocation,
                                            fbe_base_env_fup_persistent_info_t ** pLccFupInfoPtr);

fbe_status_t fbe_board_mgmt_get_fup_info(void * pContext,
                                            fbe_u64_t deviceType,
                                            fbe_device_physical_location_t * pLocation,
                                            fbe_esp_base_env_get_fup_info_t *  pGetFupInfo);

fbe_status_t fbe_board_mgmt_fup_resume_upgrade(void * pContext);

fbe_status_t fbe_board_mgmt_fup_new_contact_init_upgrade(fbe_board_mgmt_t * pBoardMgmt, 
                                                        fbe_u64_t deviceType);

fbe_status_t fbe_board_mgmt_fup_copy_fw_rev_to_resume_prom(fbe_board_mgmt_t *pBoardMgmt,
                                                        fbe_u64_t deviceType,
                                                        fbe_device_physical_location_t *pLocation);

/* fbe_board_mgmt_kernel_main.c and fbe_board_mgmt_sim_main.c */

fbe_status_t fbe_board_mgmt_fup_build_image_file_name(fbe_board_mgmt_t * pBoardMgmt,
                                                      fbe_base_env_fup_work_item_t * pWorkItem,
                                                      fbe_u8_t * pImageFileNameBuffer,
                                                      char * pImageFileNameConstantPortion,
                                                      fbe_u8_t bufferLen,
                                                      fbe_u32_t * pImageFileNameLen);
/* fbe_board_mgmt_monitor.c*/
fbe_status_t fbe_board_mgmt_get_dimm_index(fbe_board_mgmt_t *board_mgmt,
                                           fbe_device_physical_location_t *pLocation,
                                           fbe_u32_t * pDimmIndex);

fbe_status_t fbe_board_mgmt_get_ssd_index(fbe_board_mgmt_t *board_mgmt,
                                           fbe_device_physical_location_t *pLocation,
                                           fbe_u32_t * pSsdIndex);

#endif  /* BOARD_MGMT_PRIVATE_H*/
