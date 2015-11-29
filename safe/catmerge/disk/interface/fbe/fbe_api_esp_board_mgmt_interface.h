#ifndef FBE_API_ESP_BOARD_MGMT_INTERFACE_H
#define FBE_API_ESP_BOARD_MGMT_INTERFACE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_esp_board_mgmt_interface.h
 ***************************************************************************
 *
 * @brief 
 *  This header file defines the FBE API for the ESP Board Mgmt object.  
 *   
 * @ingroup fbe_api_esp_interface_class_files  
 *   
 * @version  
 *  05-Aug-2010 : Created  Vaibhav Gaonkar
 *
 ****************************************************************************/

/*************************  
 *   INCLUDE FILES  
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_board.h"
#include "spid_types.h"
#include "fbe/fbe_peer_boot_utils.h"
#include "specl_types.h"


#define BOARD_MGMT_PEER_BOOT_DESCRIPTION_LIMIT      100
#define FBE_API_ESP_BOARD_MGMT_PEER_BOOT_DESCRIPTION BOARD_MGMT_PEER_BOOT_DESCRIPTION_LIMIT + 1

FBE_API_CPP_EXPORT_START

//---------------------------------------------------------------- 
// Define the top level group for the FBE Environment Service Package APIs
// ----------------------------------------------------------------
/*! @defgroup fbe_api_esp_interface_class FBE ESP APIs 
 *
 *  @brief 
 *    This is the set of definitions for FBE ESP APIs.
 * 
 *  @ingroup fbe_api_esp_interface
 */ 
//----------------------------------------------------------------

//---------------------------------------------------------------- 
// Define the FBE API ESP BOARD Mgmt Interface for the Usurper Interface. 
// This is where all the data structures defined.  
//---------------------------------------------------------------- 
/*! @defgroup fbe_api_esp_board_mgmt_interface_usurper_interface FBE API ESP BOARD Mgmt Interface Usurper Interface  
 *  @brief   
 *    This is the set of definitions that comprise the FBE API ESP BOARD Mgmt Interface class  
 *    usurper interface.  
 *  
 *  @ingroup fbe_api_classes_usurper_interface  
 *  @{  
 */ 
//----------------------------------------------------------------

// FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_BOARD_INFO
/*!********************************************************************* 
 * @struct fbe_esp_board_mgmt_peer_boot_info_s 
 *  
 * @brief 
 *   This struct use to get peer SP booting info.
 *
 * @ingroup fbe_api_esp_board_mgmt_interface
 * @ingroup fbe_esp_board_mgmt_board_info
 **********************************************************************/
typedef struct fbe_esp_board_mgmt_peer_boot_info_s
{
    fbe_peer_boot_states_t      peerBootState;  /*!< OUT peer SP boot state */
    fbe_u64_t           faultHwType;    /* fault expander or fault register */
    /* fault expander part */
    fbe_peer_boot_code_t        bootCode;       /*!< OUT peer SP boot code */
    fbe_subfru_replacements_t   replace[FBE_MAX_SUBFRU_REPLACEMENTS];   /*!< OUT subfru replacement */
    /* fault register part */
    SPECL_FAULT_REGISTER_STATUS fltRegStatus;
    fbe_system_time_t           lastFltRegClearTime;    /*!<  Timestamp of last clearance of peer fault register */
    SPECL_FAULT_REGISTER_STATUS lastFltRegStatus;       /*!<  Peer fault register values before cleared by local ESP */
    fbe_bool_t                  biosPostFail;
    fbe_bool_t                  dataStale;
    fbe_bool_t                  environmentalInterfaceFault;
    fbe_bool_t                  isPeerSpApplicationRunning;
}fbe_esp_board_mgmt_peer_boot_info_t;


/*!********************************************************************* 
 * @struct fbe_esp_board_mgmt_board_info_s 
 *  
 * @brief 
 *   This struct use to get board info.
 *
 * @ingroup fbe_api_esp_board_mgmt_interface
 * @ingroup fbe_esp_board_mgmt_board_info
 **********************************************************************/
typedef struct fbe_esp_board_mgmt_board_info_s
{
    SPID_PLATFORM_INFO platform_info;   /*!< OUT platform info */
    SP_ID              sp_id;           /*!< OUT SP ID */
    fbe_bool_t         peerInserted;    /* OUT Peer SP inserted or not */
    fbe_bool_t         lowBattery;      /* CPU Battery low battery or not*/
    fbe_cable_status_t internalCableStatus; 
    fbe_bool_t         is_active_sp;    /* OUT Active SP or Passive SP */
    fbe_bool_t         engineIdFault; 
    fbe_bool_t         peerPresent;    /* OUT Peer SP inserted, alive, and successfully booted */
    fbe_bool_t         isSingleSpSystem;    /* OUT Single SP array or not */
    fbe_bool_t         isSimulation;   // OUT whether this is a simulated system.
    LED_BLINK_RATE     UnsafeToRemoveLED;
    fbe_u32_t          spPhysicalMemorySize[SP_ID_MAX];
    fbe_lcc_info_t     lccInfo[SP_ID_MAX];
} fbe_esp_board_mgmt_board_info_t;


/*!********************************************************************* 
 * @struct fbe_esp_board_mgmt_suitcaseInfo_s 
 *  
 * @brief 
 *   This struct use to get Suitcase info.
 * @param location (INPUT)- sp= SP_A, slot = 0 for SPA suitcase.
 *                        - sp= SP_B, slot = 0 for SPB suitcase.
 * @param suitcaseInfo (OUTPUT)
 *
 * @ingroup fbe_api_esp_board_mgmt_interface
 * @ingroup fbe_esp_board_mgmt_board_info
 **********************************************************************/
typedef struct fbe_esp_board_mgmt_suitcaseInfo_s
{
    fbe_device_physical_location_t location;  // Input
    fbe_u32_t               suitcaseCount;    //Output
    fbe_board_mgmt_suitcase_info_t   suitcaseInfo;     // Output
    fbe_bool_t                       dataStale;
    fbe_bool_t                       environmentalInterfaceFault;
}fbe_esp_board_mgmt_suitcaseInfo_t;

/*!********************************************************************* 
 * @struct fbe_esp_board_mgmt_bmcInfo_s 
 *  
 * @brief 
 *   This struct use to get BMC info.
 * @param location (INPUT)- sp= SP_A, slot = 0 for SPA BMC.
 *                        - sp= SP_B, slot = 0 for SPB BMC.
 * @param bmcInfo (OUTPUT)
 *
 * @ingroup fbe_api_esp_board_mgmt_interface
 * @ingroup fbe_esp_board_mgmt_board_info
 **********************************************************************/
typedef struct fbe_esp_board_mgmt_bmcInfo_s
{
    fbe_device_physical_location_t location;  // Input
    fbe_board_mgmt_bmc_info_t      bmcInfo;   // Output
    fbe_bool_t                     dataStale;
    fbe_bool_t                     environmentalInterfaceFault;

}fbe_esp_board_mgmt_bmcInfo_t;

/*!********************************************************************* 
 * @struct fbe_esp_board_mgmt_dimm_info_s 
 *  
 * @brief 
 *   This struct is used to expose the FBE DIMM info.
 *
 * @ingroup fbe_api_esp_board_mgmt_interface
 * @ingroup fbe_esp_board_mgmt_board_info
 **********************************************************************/
typedef struct fbe_esp_board_mgmt_dimm_info_s
{
    /*  Which SP the FRU is associated with. */
    SP_ID                           associatedSp;
    /*  Each blade based dimm ID */
    fbe_u32_t                       dimmID;
    fbe_bool_t                      isLocalDimm;
    fbe_env_inferface_status_t      envInterfaceStatus;

    /* The following data is from SPECL */
    fbe_mgmt_status_t       inserted;
    fbe_mgmt_status_t       faulted;   /* Set from ASEhwWdog driver for memory faults */  
    fbe_dimm_state_t        state; /* General status */
    fbe_dimm_subState_t     subState;
    SPD_DEVICE_TYPE         dimmType;
    
    SPD_REVISION            dimmRevision;
    fbe_u32_t               dimmDensity;
    ULONG                   numOfBanks;
    ULONG                   deviceWidth;
    ULONG                   numberOfRanks;
    BOOL                    vendorRegionValid;
    csx_uchar_t             jedecIdCode             [JEDEC_ID_CODE_SIZE+1];

    /* Resume related info */
    csx_uchar_t             EMCDimmSerialNumber     [2*EMC_DIMM_SERIAL_NUMBER_SIZE+1];
    csx_uchar_t             EMCDimmPartNumber       [EMC_DIMM_PART_NUMBER_SIZE+1];
    BYTE                    OldEMCDimmSerialNumber  [2*OLD_EMC_DIMM_SERIAL_NUMBER_SIZE+1];

    /* Get vendorName is decoded from jedecIdCode */
    csx_uchar_t             vendorName              [EMC_DIMM_VENDOR_NAME_SIZE+1]; 
    csx_uchar_t             vendorSerialNumber      [2*MODULE_SERIAL_NUMBER_SIZE+1];
    csx_uchar_t             vendorPartNumber        [PART_NUMBER_SIZE_2+1];

    /* assemblyName is decoded from deviceType */
    csx_uchar_t             assemblyName            [EMC_DIMM_MODULE_NAME_SIZE+1];      
    csx_uchar_t             manufacturingLocation;  // It is a byte code.
    csx_uchar_t             manufacturingDate       [MANUFACTURING_DATE_SIZE+1];
}fbe_esp_board_mgmt_dimm_info_t;

/*!********************************************************************* 
 * @struct fbe_esp_board_mgmt_ssd_info_s 
 *  
 * @brief 
 *   This struct is used to expose the FBE SSD info.
 *
 * @ingroup fbe_api_esp_board_mgmt_interface
 * @ingroup fbe_esp_board_mgmt_board_info
 **********************************************************************/
typedef struct fbe_esp_board_mgmt_ssd_info_s
{
    /*  Which SP the FRU is associated with. */
    SP_ID                  associatedSp;
    fbe_u32_t              slot;
    fbe_bool_t             isLocalSsd;
    fbe_ssd_state_t        ssdState;
    fbe_ssd_subState_t     ssdSubState;
    /* The following fields are only available for a local SSD, not available for a peer SSD. */
    fbe_mgmt_status_t      ssdFaulted;
    fbe_u32_t              remainingSpareBlkCount;
    fbe_u32_t              ssdLifeUsed;
    fbe_bool_t             ssdSelfTestPassed;
    /* Resume related info */
    fbe_char_t             ssdSerialNumber      [FBE_SSD_SERIAL_NUMBER_SIZE + 1];
    fbe_char_t             ssdPartNumber        [FBE_SSD_PART_NUMBER_SIZE +1];
    fbe_char_t             ssdAssemblyName      [FBE_SSD_ASSEMBLY_NAME_SIZE +1];
    fbe_char_t             ssdFirmwareRevision  [FBE_SSD_FIRMWARE_REVISION_SIZE + 1];
}fbe_esp_board_mgmt_ssd_info_t;


/*! @} */ /* end of group fbe_api_esp_board_mgmt_interface_usurper_interface */

//---------------------------------------------------------------- 
// Define the group for the FBE API ESP BOARD Mgmt Interface.   
// This is where all the function prototypes for the FBE API ESP BOARD Mgmt. 
//---------------------------------------------------------------- 
/*! @defgroup fbe_api_esp_board_mgmt_interface FBE API ESP BOARD Mgmt Interface  
 *  @brief   
 *    This is the set of FBE API ESP BOARD Mgmt Interface.   
 *  
 *  @details   
 *    In order to access this library, please include fbe_api_esp_board_mgmt_interface.h.  
 *  
 *  @ingroup fbe_api_esp_interface_class  
 *  @{  
 */
//---------------------------------------------------------------- 
fbe_status_t FBE_API_CALL fbe_api_esp_board_mgmt_getBoardInfo(fbe_esp_board_mgmt_board_info_t *board_info);
fbe_status_t FBE_API_CALL fbe_api_esp_board_mgmt_reboot_sp(fbe_board_mgmt_set_PostAndOrReset_t *post_reset);
fbe_status_t FBE_API_CALL fbe_api_esp_board_mgmt_getCacheStatus(fbe_common_cache_status_t *cacheStatus);
fbe_status_t FBE_API_CALL fbe_api_esp_board_mgmt_getPeerBootInfo(fbe_esp_board_mgmt_peer_boot_info_t *peerBootInfo);
fbe_status_t FBE_API_CALL fbe_api_esp_board_mgmt_getPeerCpuStatus(fbe_bool_t *cpuStatus);
fbe_status_t FBE_API_CALL fbe_api_esp_board_mgmt_getPeerDimmStatus(fbe_bool_t *dimmStatus, fbe_u32_t size);
fbe_status_t FBE_API_CALL fbe_api_esp_board_mgmt_getSuitcaseInfo(fbe_esp_board_mgmt_suitcaseInfo_t *suitcaseInfo);
fbe_status_t FBE_API_CALL fbe_api_esp_board_mgmt_getBmcInfo(fbe_esp_board_mgmt_bmcInfo_t *bmcInfo);
fbe_status_t FBE_API_CALL fbe_api_esp_board_mgmt_getCacheCardCount(fbe_u32_t * pCount);
fbe_status_t FBE_API_CALL fbe_api_esp_board_mgmt_getCacheCardStatus(fbe_board_mgmt_cache_card_info_t *cacheCardStatus);
fbe_status_t FBE_API_CALL fbe_api_esp_board_mgmt_flushSystem(void);
fbe_status_t FBE_API_CALL fbe_api_esp_board_mgmt_getDimmCount(fbe_u32_t * pCount);
fbe_status_t FBE_API_CALL fbe_api_esp_board_mgmt_getSSDCount(fbe_u32_t * pCount);
fbe_status_t FBE_API_CALL fbe_api_esp_board_mgmt_getSSDInfo(fbe_device_physical_location_t * pLocation, 
                                                            fbe_esp_board_mgmt_ssd_info_t *ssdStatus);
fbe_status_t FBE_API_CALL fbe_api_esp_board_mgmt_getDimmInfo(fbe_device_physical_location_t * pLocation,
                                                             fbe_esp_board_mgmt_dimm_info_t * pDimmInfo);

/*! @} */ /* end of group fbe_api_esp_encl_mgmt_interface */

FBE_API_CPP_EXPORT_END

//----------------------------------------------------------------
// Define the group for all FBE ESP Package APIs Interface class files.  
// This is where all the class files that belong to the FBE API ESP 
// Package define. In addition, this group is displayed in the FBE Classes
// module.
//----------------------------------------------------------------
/*! @defgroup fbe_api_esp_interface_class_files FBE ESP APIs Interface Class Files 
 *  @brief 
 *    This is the set of files for the FBE ESP Interface.
 *
 *  @ingroup fbe_api_classes
 */
//----------------------------------------------------------------

#endif /* FBE_API_ESP_BOARD_MGMT_INTERFACE_H */

/**********************************************
 * end file fbe_api_esp_board_mgmt_interface.h
 **********************************************/
