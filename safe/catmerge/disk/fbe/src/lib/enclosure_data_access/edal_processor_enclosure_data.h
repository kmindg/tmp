#ifndef EDAL_BASE_BOARD_ENCLOSURE_DATA_H
#define EDAL_BASE_BOARD_ENCLOSURE_DATA_H

/***************************************************************************
 * Copyright (C) EMC Corporation  2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 * Unpublished - all rights reserved under the copyright laws
 * of the United States
 ***************************************************************************/

/*!**************************************************************************
 *  @file edal_processor_enclosure_data.h
 ***************************************************************************
 * @brief
 *      This module is a header file for Enclosure Data Access Library (EDAL)
 *      and it defined data for the process enclosure components.
 *
 * @version
 *      22-Feb-2010: PHE - Created.
 *
 **************************************************************************/

#include "fbe/fbe_enclosure_data_access_public.h"
#include "edal_base_enclosure_data.h"
#include "specl_types.h"
#include "fbe/fbe_pe_types.h"
#include "fbe/fbe_devices.h"
#include "fbe/fbe_eir_info.h"
#include "fbe/fbe_sps_info.h"


/*
 * EDAL Processor Enclosure Platform Info.
 */
typedef struct edal_pe_platform_info_s
{  
    edal_base_platform_info_t           basePlatformInfo;
    fbe_board_mgmt_platform_info_t      pePlatformSubInfo; 
}edal_pe_platform_info_t;

/*
 * EDAL Processor Enclosure Miscellaneous Info.
 */
typedef struct edal_pe_misc_info_s
{  
    edal_base_misc_info_t       baseMiscInfo;
    fbe_board_mgmt_misc_info_t  peMiscSubInfo;   
} edal_pe_misc_info_t;

/*
 * EDAL Processor Enclosure IO Module/IO Annex/Mezzanine Info.
 */
typedef struct edal_pe_io_comp_sub_info_s 
{
    /* The following data is from SPECL */
    HW_MODULE_TYPE                 uniqueID;
    BOOL                           inserted;
    /* Save the timestamp that the transactionStatus changes from good to bad. */
    fbe_u64_t                      timeStampForFirstBadStatus; 
    /* The info exposed to the client. */
    fbe_board_mgmt_io_comp_info_t  externalIoCompInfo;
    fbe_pe_resume_prom_id_t        resumePromId;
} edal_pe_io_comp_sub_info_t;

typedef struct edal_pe_io_comp_info_s
{
    edal_base_io_comp_info_t                 baseIoCompInfo;
    edal_pe_io_comp_sub_info_t               peIoCompSubInfo;  
} edal_pe_io_comp_info_t;

/*
 * EDAL Processor Enclosure IO Port Info.
 */
typedef struct edal_pe_io_port_sub_info_s 
{
    /* The info exposed to the client. */
    fbe_board_mgmt_io_port_info_t  externalIoPortInfo;   
} edal_pe_io_port_sub_info_t;

typedef struct edal_pe_io_port_info_s
{
    edal_base_io_port_info_t                 baseIoPortInfo;
    edal_pe_io_port_sub_info_t               peIoPortSubInfo;  
} edal_pe_io_port_info_t;

/*
 * EDAL Processor Enclosure Power Supply Info.
 */
typedef struct edal_pe_power_supply_sub_info_s 
{
    /* Save the timestamp that the transactionStatus changes from good to bad. */
    fbe_u64_t                      timeStampForFirstBadStatus; 
    fbe_env_inferface_status_t     peerEnvInterfaceStatus; 
    /* The info exposed to the client. */
    fbe_power_supply_info_t        externalPowerSupplyInfo;   
} edal_pe_power_supply_sub_info_t;

typedef struct edal_pe_power_supply_info_s
{
    fbe_encl_power_supply_info_t             basePowerSupplyInfo;   
    edal_pe_power_supply_sub_info_t          pePowerSupplySubInfo;
} edal_pe_power_supply_info_t;

/*
 * EDAL Processor Enclosure SPS Info.
 */
typedef struct edal_pe_sps_sub_info_s 
{
    /* The following data is from SPECL */
    SPECL_ERROR                     transactionStatus;
    /* sps Module Resume transactionStatus, not sps battery resume transaction status */
    SPECL_ERROR                     resumeTransactionStatus;  
    /* previous sps Module Resume transactionStatus, not previous sps battery resume transaction status */
    SPECL_ERROR                     prevResumeTransactionStatus;
    /* Save the timestamp that the transactionStatus changes from good to bad. */
    fbe_u64_t                       timeStampForFirstBadStatus; 
    fbe_env_inferface_status_t      peerEnvInterfaceStatus; 
    SPS_ISP_STATE                   ispState;
    EMCPAL_LARGE_INTEGER            lastIspStateTimeStamp;
    fbe_enclosure_firmware_status_t     prevFupStatus; 
    fbe_enclosure_firmware_ext_status_t prevFupExtStatus;
    fbe_enclosure_firmware_status_t     fupStatus; 
    fbe_enclosure_firmware_ext_status_t fupExtStatus; 
    
    /* The info exposed to the client. */
    fbe_base_sps_info_t             externalSpsInfo;   
    fbe_sps_manuf_info_t            externalSpsManufInfo;
} edal_pe_sps_sub_info_t;

typedef struct edal_pe_sps_info_s
{
    edal_base_sps_info_t            baseSpsInfo;
    edal_pe_sps_sub_info_t          peSpsSubInfo;
} edal_pe_sps_info_t;

/*
 * EDAL Processor Enclosure Battery Info.
 */
typedef struct edal_pe_battery_sub_info_s 
{
    /* Save the timestamp that the transactionStatus changes from good to bad. */
    fbe_u64_t                       timeStampForFirstBadStatus; 
    fbe_env_inferface_status_t      peerEnvInterfaceStatus; 
    /* The info exposed to the client. */
    fbe_base_battery_info_t         externalBatteryInfo;   
} edal_pe_battery_sub_info_t;

typedef struct edal_pe_battery_info_s
{
    edal_base_battery_info_t        baseBatteryInfo;
    edal_pe_battery_sub_info_t      peBatterySubInfo;
} edal_pe_battery_info_t;

/*
 * EDAL Processor Enclosure Cooling Info.
 */
typedef struct edal_pe_cooling_sub_info_s
{
    /* Save the timestamp that the transactionStatus changes from good to bad. */
    fbe_u64_t                      timeStampForFirstBadStatus; 
    /* Save the timestamp to remember that start time that the multiple fan modules are faulted. */
    fbe_u64_t                      timeStampForMultiFaultedStateStart;
    /* The info exposed to the client. */
    fbe_cooling_info_t             externalCoolingInfo;
} edal_pe_cooling_sub_info_t;

typedef struct edal_pe_cooling_info_s
{
    fbe_encl_cooling_info_t                 baseCoolingInfo;   
    edal_pe_cooling_sub_info_t              peCoolingSubInfo;
} edal_pe_cooling_info_t;

/*
 * EDAL Processor Enclosure Management Module Info.
 */
typedef struct edal_pe_mgmt_module_sub_info_s 
{
    /* Save the timestamp that the transactionStatus changes from good to bad. */
    fbe_u64_t                              timeStampForFirstBadStatus; 
    /* The info exposed to the client. */
    fbe_board_mgmt_mgmt_module_info_t      externalMgmtModuleInfo;   
} edal_pe_mgmt_module_sub_info_t;

typedef struct edal_pe_mgmt_module_info_s
{
    edal_base_mgmt_module_info_t           baseMgmtModuleInfo;   
    edal_pe_mgmt_module_sub_info_t         peMgmtModuleSubInfo;    
} edal_pe_mgmt_module_info_t;

/*
 * EDAL Processor Enclosure Fault Expander Info.
 */
typedef struct edal_pe_flt_exp_sub_info_s
{
    /* Save the timestamp that the transactionStatus changes from good to bad. */
    fbe_u64_t                              timeStampForFirstBadStatus; 
    /* The info exposed to the client. */
    fbe_peer_boot_flt_exp_info_t  externalFltExpInfo;
} edal_pe_flt_exp_sub_info_t;

typedef struct edal_pe_flt_exp_info_s
{  
    edal_base_flt_exp_info_t    baseFltExpInfo;
    edal_pe_flt_exp_sub_info_t  peFltExpSubInfo;   
} edal_pe_flt_exp_info_t;

/*
 * Processor Enclosure Slave Port Info.
 */
typedef struct edal_pe_slave_port_sub_info_s
{
    /* Save the timestamp that the transactionStatus changes from good to bad. */
    fbe_u64_t                              timeStampForFirstBadStatus; 
    /* The info exposed to the client. */
    fbe_board_mgmt_slave_port_info_t  externalSlavePortInfo;
} edal_pe_slave_port_sub_info_t;

typedef struct edal_pe_slave_port_info_s
{  
    edal_base_slave_port_info_t    baseSlavePortInfo;
    edal_pe_slave_port_sub_info_t  peSlavePortSubInfo;   
} edal_pe_slave_port_info_t;

/*
 * EDAL Processor Enclosure Suitcase Info.
 */

typedef struct edal_pe_suitcase_sub_info_s
{
    /* Save the timestamp that the transactionStatus changes from good to bad. */
    fbe_u64_t                      timeStampForFirstBadStatus; 
    /* The info exposed to the client. */
    fbe_board_mgmt_suitcase_info_t externalSuitcaseInfo;
} edal_pe_suitcase_sub_info_t;

typedef struct edal_pe_suitcase_info_s
{
    edal_base_suitcase_info_t             baseSuitcaseInfo; 
    edal_pe_suitcase_sub_info_t           peSuitcaseSubInfo;
} edal_pe_suitcase_info_t;

/*
 * EDAL Processor Enclosure BMC Info.
 */
typedef struct edal_pe_bmc_sub_info_s
{
    /* Save the timestamp that the transactionStatus changes from good to bad. */
    fbe_u64_t                      timeStampForFirstBadStatus; 
    /* The info exposed to the client. */
    fbe_board_mgmt_bmc_info_t      externalBmcInfo;
} edal_pe_bmc_sub_info_t;

typedef struct edal_pe_bmc_info_s
{
    edal_base_bmc_info_t                  baseBmcInfo; 
    edal_pe_bmc_sub_info_t                peBmcSubInfo;
} edal_pe_bmc_info_t;

/*
 * EDAL Processor Enclosure Cache Card Info.
 */
typedef struct edal_pe_cache_card_sub_info_s
{
    /* The following data is from SPECL */
    SPECL_ERROR                    transactionStatus;
    /* Save the timestamp that the transactionStatus changes from good to bad. */
    fbe_u64_t                      timeStampForFirstBadStatus; 
    /* The info exposed to the client. */
    fbe_board_mgmt_cache_card_info_t      externalCacheCardInfo;
} edal_pe_cache_card_sub_info_t;

typedef struct edal_pe_cache_card_info_s
{
    edal_base_cache_card_info_t                  baseCacheCardInfo; 
    edal_pe_cache_card_sub_info_t                peCacheCardSubInfo;
} edal_pe_cache_card_info_t;

/*
 * EDAL Processor Enclosure DIMM Info.
 */
typedef struct edal_pe_dimm_sub_info_s
{
    /* The following data is from SPECL */
    SPECL_ERROR                    transactionStatus;
    /* Save the timestamp that the transactionStatus changes from good to bad. */
    fbe_u64_t                      timeStampForFirstBadStatus; 
    /* The info exposed to the client. */
    fbe_board_mgmt_dimm_info_t      externalDimmInfo;
} edal_pe_dimm_sub_info_t;

typedef struct edal_pe_dimm_info_s
{
    edal_base_dimm_info_t                  baseDimmInfo; 
    edal_pe_dimm_sub_info_t                peDimmSubInfo;
} edal_pe_dimm_info_t;

/*
 * EDAL Processor Enclosure SSD Info.
 */
typedef struct edal_pe_ssd_info_s
{
    edal_base_ssd_info_t            baseSSDInfo;
    fbe_board_mgmt_ssd_info_t       ssdInfo;
}edal_pe_ssd_info_t;

/*
 * EDAL Processor Enclosure Resume Prom Info.
 */
typedef struct edal_pe_resume_prom_sub_info_s
{
    /*  Which SP the FRU is associated with. */
    SP_ID                                   associatedSp;
    /* Each Blade based slot number. */
    fbe_u32_t                               slotNumOnBlade;  
    fbe_u64_t                       deviceType;
    SMB_DEVICE                              smbDevice;
    /* The info exposed to the client. */
    fbe_board_mgmt_resume_prom_info_t       externalResumePromInfo;
} edal_pe_resume_prom_sub_info_t;

typedef struct edal_pe_resume_prom_info_s
{
    edal_base_resume_prom_info_t            baseResumePromInfo; 
    edal_pe_resume_prom_sub_info_t          peResumePromSubInfo;
} edal_pe_resume_prom_info_t;

/*
 * EDAL Processor Enclosure Temperature Info.
 */
typedef struct edal_pe_temperature_sub_info_s 
{
    /* The following data is from SPECL */
    SPECL_ERROR                    transactionStatus;
    /* Save the timestamp that the transactionStatus changes from good to bad. */
    fbe_u64_t                      timeStampForFirstBadStatus; 
    fbe_env_inferface_status_t     peerEnvInterfaceStatus; 
    /* The info exposed to the client. */
    fbe_eir_temp_sample_t           externalTempInfo;
} edal_pe_temperature_sub_info_t;

typedef struct edal_pe_temperature_info_s
{
    fbe_encl_temp_sensor_info_t             baseTempInfo;   
    edal_pe_temperature_sub_info_t          peTempSubInfo;
} edal_pe_temperature_info_t;


/*
 * Processor Enclosure function prototypes
 */
fbe_bool_t processor_enclosure_is_component_type_supported(fbe_enclosure_component_types_t componentType);

/* Get function prototypes */
fbe_edal_status_t
processor_enclosure_get_component_size(fbe_enclosure_component_types_t component_type,
                              fbe_u32_t * component_size_p);

fbe_edal_status_t 
processor_enclosure_access_getBuffer(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t component,
                              fbe_u32_t buffer_length,
                              fbe_u8_t *buffer_p);

fbe_edal_status_t 
processor_enclosure_access_getBool(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t component,
                              fbe_bool_t *returnDataPtr);

fbe_edal_status_t 
processor_enclosure_access_getU8(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t component,
                              fbe_u8_t *returnDataPtr);

fbe_edal_status_t 
processor_enclosure_access_getU32(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t component,
                              fbe_u32_t *returnDataPtr);

fbe_edal_status_t 
processor_enclosure_access_getU64(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t component,
                              fbe_u64_t *returnDataPtr);

fbe_edal_status_t 
processor_enclosure_access_getStr(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t componentType,
                              fbe_u32_t length,
                              char *returnStringPtr);


/* Set function prototypes */
fbe_edal_status_t 
processor_enclosure_access_setBuffer(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t componentType,
                              fbe_u32_t buffer_length,
                              fbe_u8_t *buffer_p);

fbe_edal_status_t 
processor_enclosure_access_setBool(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t component,
                              fbe_bool_t newValue);

fbe_edal_status_t 
processor_enclosure_access_setU8(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t component,
                              fbe_u8_t newValue);

fbe_edal_status_t 
processor_enclosure_access_setU32(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t component,
                              fbe_u32_t newValue);

fbe_edal_status_t 
processor_enclosure_access_setU64(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t component,
                              fbe_u64_t newValue);

fbe_edal_status_t 
processor_enclosure_access_setStr(fbe_edal_general_comp_handle_t generalDataPtr,
                                    fbe_base_enclosure_attributes_t attribute,
                                    fbe_enclosure_component_types_t componentType,
                                    fbe_u32_t length,
                                    char *newValue);


#endif  // ifndef EDAL_BASE_BOARD_ENCLOSURE_DATA_H
