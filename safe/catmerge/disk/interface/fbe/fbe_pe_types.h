#ifndef FBE_PE_TYPES_H
#define FBE_PE_TYPES_H

/***************************************************************************
 * Copyright (C) EMC Corporation  2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 * Unpublished - all rights reserved under the copyright laws
 * of the United States
 ***************************************************************************/

/*!**************************************************************************
 *  @file fbe_pe_types_public.h
 ***************************************************************************
 * @brief
 *      This header file defines the data structure 
 *      for the process enclosure components in the physical package.
 *
 * @version
 *      22-Feb-2010: PHE - Created.
 *
 **************************************************************************/

#include "specl_types.h"
#include "fbe/fbe_enclosure_interface.h"
#include "fbe/fbe_devices.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe/fbe_resume_prom_info.h"
#include "speeds.h"

#define MAX_IO_PORTS_PER_MODULE  (MAX_IO_PORTS_PER_CONTROLLER * MAX_IO_CONTROLLERS_PER_MODULE)

#define MAX_ISCSI_PORTS_PER_IOM  2
#define MAX_FC_PORTS_PER_IOM  4
#define MAX_SAS_PORTS_PER_IOM  4
#define FBE_ENCLOSURE_MAX_PS_INTERNAL_FAN_COUNT 2

#define FBE_SSD_SERIAL_NUMBER_SIZE 20
#define FBE_SSD_PART_NUMBER_SIZE 100
#define FBE_SSD_ASSEMBLY_NAME_SIZE 100
#define FBE_SSD_FIRMWARE_REVISION_SIZE 20

#ifndef SPS_ID_DEFINED
#define SPS_ID_DEFINED
typedef enum { 
    FBE_SPS_A = 0,
    FBE_SPS_B = 1,
    FBE_SPS_INVALID = 0xFF
} SPS_ID;
#endif
#ifndef FAN_MODULE_ID_DEFINED
#define FAN_MODULE_ID_DEFINED
typedef enum { 
    FAN_MODULE_A = 0,
    FAN_MODULE_B = 1,
    FAN_MODULE_C = 2,
    FAN_MODULE_D = 3,
    FAN_MODULE_INVALID = 0xFF
} FAN_MODULE_ID;
#endif

#define    FBE_SPS_MAX  2

typedef enum fbe_mgmt_status_e
{
    FBE_MGMT_STATUS_FALSE    = 0,
    FBE_MGMT_STATUS_TRUE     = 1,
    FBE_MGMT_STATUS_UNKNOWN  = 254,
    FBE_MGMT_STATUS_NA       = 255
}fbe_mgmt_status_t;

typedef enum fbe_power_status_e
{
    /* Power is good.*/
    FBE_POWER_STATUS_POWER_ON             = 0,
    /* Tried to power up but failed. */
    FBE_POWER_STATUS_POWERUP_FAILED       = 1,
    /* Power is disabled. */
    FBE_POWER_STATUS_POWER_OFF            = 2, 
    /* Power status is not available */
    FBE_POWER_STATUS_UNKNOWN              = 254, 
    /* Power status is not applicable for the IO component which is not inserted. */
    FBE_POWER_STATUS_NA                   = 255
}fbe_power_status_t;

typedef enum fbe_env_inferface_status_e
{
    FBE_ENV_INTERFACE_STATUS_GOOD                = 0,
    /* SEPCL transaction status failed.*/
    FBE_ENV_INTERFACE_STATUS_XACTION_FAIL        = 1,
    /* SPECL data stale. */
    FBE_ENV_INTERFACE_STATUS_DATA_STALE          = 2,
    /* Not applicable. */
    FBE_ENV_INTERFACE_STATUS_NA                  = 255
}fbe_env_inferface_status_t;

typedef enum fbe_io_module_location_e
{
    FBE_IO_MODULE_LOC_ONBOARD_SLOT0    = 0,   // IO module is plugged in the SP IO slot0.
    FBE_IO_MODULE_LOC_ONBOARD_SLOT1,          // IO module is plugged in the SP IO slot1.
    FBE_IO_MODULE_LOC_ONBOARD_SLOT2,          // IO module is plugged in the SP IO slot2.
    FBE_IO_MODULE_LOC_ONBOARD_SLOT3,          // IO module is plugged in the SP IO slot3.
    FBE_IO_MODULE_LOC_ONBOARD_SLOT4,          // IO module is plugged in the SP IO slot0.
    FBE_IO_MODULE_LOC_ONBOARD_SLOT_MAX,
    FBE_IO_MODULE_LOC_ANNEX_SLOT0,            // IO module is plugged into io annex IO slot 0.
    FBE_IO_MODULE_LOC_ANNEX_SLOT1,            // IO module is plugged into io annex IO slot 0.
    FBE_IO_MODULE_LOC_ANNEX_SLOT_MAX,
    FBE_IO_MODULE_LOC_UNKNOWN          = 254,  
    FBE_IO_MODULE_LOC_NA               = 255
}fbe_io_module_location_t;

/*
 * FBE_BASE_BOARD_CONTROL_CODE_GET_PLATFORM_INFO
 */
typedef struct fbe_board_mgmt_platform_info_s{
    SPID_PLATFORM_INFO  hw_platform_info;
    fbe_bool_t          is_simulation;
    fbe_bool_t          is_single_sp_system;
}fbe_board_mgmt_platform_info_t;

/*
 * FBE_BASE_BOARD_CONTROL_CODE_GET_MISC_INFO
 */
typedef struct fbe_board_mgmt_misc_info_s
{  
    SP_ID                         spid;         /* For local SP */
    fbe_bool_t                    engineIdFault; /* for local SP, NOTE: This is sticky (it will remain true after the fault clears) */
    fbe_bool_t                    peerInserted;  /* Indicate whether the peer CPU is inserted or not. */
    fbe_bool_t                    lowBattery;  /* Indicate whether CPU is low battery*/
    fbe_bool_t                    smbSelectBitsFailed;  /* For local SP */
    fbe_bool_t                    peerHoldInReset;
    fbe_bool_t                    peerHoldInPost;
    fbe_bool_t                    localHoldInPost;
    LED_BLINK_RATE                SPFaultLED;         /* LED_BLINK_RATE */
    LED_BLINK_RATE                UnsafeToRemoveLED;  /* LED_BLINK_RATE */
    LED_BLINK_RATE                EnclosureFaultLED;  /* LED_BLINK_RATE */      
    LED_BLINK_RATE                ManagementPortLED;
    LED_BLINK_RATE                ServicePortLED;
    fbe_bool_t                    SPFaultLEDColor;
    fbe_u64_t                     enclFaultLedReason;
    BOOL                          localPowerECBFault; /* Indicate if local Electric Circuit Breaker is tripped */
    BOOL                          peerPowerECBFault;  /* Indicate if peer Electric Circuit Breaker is tripped */
    SPECL_CNA_MODE                cnaMode;
    fbe_cable_status_t            fbeInternalCablePort1;
} fbe_board_mgmt_misc_info_t;


/*
 * FBE_BASE_BOARD_CONTROL_CODE_GET_IO_MODULE_INFO/ 
 * FBE_BASE_BOARD_CONTROL_CODE_GET_BEM_INFO/
 * FBE_BASE_BOARD_CONTROL_CODE_GET_MEZZANINE_INFO
 */
typedef struct fbe_board_mgmt_io_comp_info_s 
{     
    /*  Which SP should change parameters on this IO FRU. */
    SP_ID                           associatedSp;
    /* Each Blade based slot number. */
    fbe_u32_t                       slotNumOnBlade;  
    fbe_u64_t               deviceType;  
    fbe_env_inferface_status_t      envInterfaceStatus;
    SPECL_ERROR                     transactionStatus;
    /* Whether the fru is associated to the running SP. */
    fbe_bool_t                      isLocalFru;
    fbe_mgmt_status_t               inserted;    
    fbe_power_status_t              powerStatus; 
    HW_MODULE_TYPE                  uniqueID;
    fbe_u32_t                       ioPortCount;
    LED_BLINK_RATE                  faultLEDStatus; 
    fbe_io_module_location_t        location;
    fbe_bool_t                      isFaultRegFail;
    fbe_bool_t                      internalFanFault;
    fbe_bool_t                      powerEnabled;
    fbe_bool_t                     hwErrMonFault;          // fault reported by HwErrMon
    /* Jetfire BEM is also treated as lcc in eses enclosure object,
     * So we need to get this virtual lcc's info which is get from eses/expander.
     * Only valid for Jetfire BEM. 
     */
    fbe_lcc_info_t                  lccInfo;
    fbe_bool_t                      localPowerECBFault;
    fbe_bool_t                      peerPowerECBFault;
} fbe_board_mgmt_io_comp_info_t;


/*
 * FBE_BASE_BOARD_CONTROL_CODE_GET_IO_PORT_INFO
 */
typedef struct fbe_board_mgmt_io_port_info_s 
{
    /*  Which SP should change parameters on this IO FRU. */
    SP_ID                          associatedSp;
    /* Each Blade based slot number. */
    fbe_u32_t                      slotNumOnBlade;  
    /* Each Module based port number. */
    fbe_u32_t                      portNumOnModule;  
    fbe_u64_t              deviceType;
    /* Whether the fru is associated to the running SP. */
    fbe_bool_t                     isLocalFru;  
    fbe_mgmt_status_t              present;
    fbe_power_status_t             powerStatus;
    IO_CONTROLLER_PROTOCOL         protocol; // current port protocol
    IO_CONTROLLER_PROTOCOL         controller_protocol; // protocol for the controller to this port
    fbe_mgmt_status_t              SFPcapable;
    fbe_u32_t                      supportedSpeeds;
    fbe_u32_t                      pciDataCount;
    SPECL_IO_PORT_PCI_DATA         pciData[PCI_MAX_FUNCTION];
    SPECL_IO_PORT_PCI_DATA         portPciData[PCI_MAX_FUNCTION]; 
    LED_BLINK_RATE                 ioPortLED;
    LED_COLOR_TYPE                 ioPortLEDColor;
    fbe_u32_t                      portal_number;
    fbe_bool_t                     boot_device;
    fbe_u32_t                      phyMapping;
    SPECL_SFP_STATUS               sfpStatus;
} fbe_board_mgmt_io_port_info_t;


typedef enum
{
    FBE_PS_STATE_UNKNOWN = 0,
    FBE_PS_STATE_OK,
    FBE_PS_STATE_REMOVED,
    FBE_PS_STATE_FAULTED,
    FBE_PS_STATE_DEGRADED,
} fbe_ps_state_t;

typedef enum
{
    FBE_PS_SUBSTATE_NO_FAULT = 0,
    FBE_PS_SUBSTATE_GEN_FAULT,
    FBE_PS_SUBSTATE_FAN_FAULT,
    FBE_PS_SUBSTATE_UNSUPPORTED,
    FBE_PS_SUBSTATE_FLT_STATUS_REG_FAULT,
    FBE_PS_SUBSTATE_FUP_FAILURE,
    FBE_PS_SUBSTATE_RP_READ_FAILURE,
    FBE_PS_SUBSTATE_ENV_INTF_FAILURE,
} fbe_ps_subState_t;

typedef enum
{
    FBE_PS_MGMT_EXPECTED_PS_TYPE_NONE  = 0,
    FBE_PS_MGMT_EXPECTED_PS_TYPE_OCTANE, 
    FBE_PS_MGMT_EXPECTED_PS_TYPE_OTHER
} fbe_ps_mgmt_expected_ps_type_t;
/*
 * FBE_BASE_BOARD_CONTROL_CODE_GET_POWER_SUPPLY_INFO
 */
typedef struct fbe_power_supply_info_s
{
    /* Whether the platform support this PS. */
    fbe_bool_t                     psSupported;
    /* Which SP this power supply supplies the power to.
     * If the PS supplies the power to both SPA and SPB, associatdSp is NA.
     */
    SP_ID                          associatedSp;
    /* Enclosure based slot number. */
    fbe_u32_t                      slotNumOnEncl;  
    fbe_u32_t                      slotNumOnSpBlade;
    fbe_env_inferface_status_t     envInterfaceStatus;
    SPECL_ERROR                    transactionStatus;
    fbe_bool_t                     resumePromReadFailed;
    fbe_bool_t                     inProcessorEncl;
    /* use ffid as uniqueId */
    HW_MODULE_TYPE                 uniqueId;  
    fbe_bool_t                     uniqueIDFinal;
    /* Whether the fru is supplying power to the running SP. */
    fbe_bool_t                     isLocalFru;   
    /* Which SPS this PS is connected to. */
    SPS_ID                         associatedSps;
    fbe_bool_t                     bInserted;
    fbe_ps_state_t                 state;
    fbe_ps_subState_t              subState;
    fbe_mgmt_status_t              generalFault;
    fbe_mgmt_status_t              internalFanFault;
    fbe_mgmt_status_t              ambientOvertempFault;
    fbe_mgmt_status_t              DCPresent;
    fbe_mgmt_status_t              ACFail;    
    fbe_mgmt_status_t              ACFailDetails[FBE_SPS_MAX];      // used for Voyager
    AC_DC_INPUT                    ACDCInput;
    fbe_u8_t                       firmwareRevValid;
    fbe_bool_t                     bDownloadable;
    fbe_u16_t                      esesVersionDesc;     // enclosure level variable to know if cdes1 or cdes2
    // The lowest rev of the PS MCUs
    fbe_u8_t                       firmwareRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1]; 
    fbe_rev_info_t                 psRevInfo[FBE_ENCLOSURE_MAX_REV_COUNT_PER_PS];

    fbe_u8_t                       subenclProductId[FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE + 1];
    fbe_bool_t                     fupFailure;
    /* The following two fields are generated by physical package based on the inputPower from SPECL/ESES. 
     * Do not trust inputPower while inputPowerValid is FALSE. 
     */
    fbe_u32_t                      inputPowerStatus;
    fbe_u32_t                      inputPower;     
    fbe_u16_t                      psInputPower;
    fbe_u8_t                       psMarginTestMode;  
    fbe_u8_t                       psMarginTestResults; 
    /* Only for Processor enclosure PS */
    fbe_bool_t                     isFaultRegFail;
} fbe_power_supply_info_t;

/*
 * FBE_BASE_BOARD_CONTROL_CODE_GET_COOLING_INFO
 */
typedef enum fbe_fan_location_e
{
    FBE_FAN_LOCATION_FRONT = 0,
    FBE_FAN_LOCATION_BACK,
    FBE_FAN_LOCATION_SP,
} fbe_fan_location_t;

typedef enum
{
    FBE_FAN_STATE_UNKNOWN = 0,
    FBE_FAN_STATE_OK,
    FBE_FAN_STATE_REMOVED,
    FBE_FAN_STATE_FAULTED,
    FBE_FAN_STATE_DEGRADED,
} fbe_fan_state_t;

typedef enum
{
    FBE_FAN_SUBSTATE_NO_FAULT = 0,
    FBE_FAN_SUBSTATE_FLT_STATUS_REG_FAULT,
    FBE_FAN_SUBSTATE_FUP_FAILURE,
    FBE_FAN_SUBSTATE_RP_READ_FAILURE,
    FBE_FAN_SUBSTATE_ENV_INTF_FAILURE,
} fbe_fan_subState_t;

typedef struct fbe_cooling_info_s
{
    /* Enclosure based slot number. */
    fbe_u32_t                      slotNumOnEncl; 
    fbe_u32_t                      slotNumOnSpBlade; 
    fbe_fan_state_t                state;
    fbe_fan_subState_t             subState;
    fbe_env_inferface_status_t     envInterfaceStatus;
    SPECL_ERROR                    transactionStatus;
    fbe_bool_t                     dataStale;
    fbe_bool_t                     environmentalInterfaceFault;
    fbe_bool_t                     inProcessorEncl;
    fbe_mgmt_status_t              inserted;
    fbe_mgmt_status_t              fanFaulted;  // Multiple fault if the fan includes multiples elements.
    fbe_mgmt_status_t              fanDegraded; // Single fault.
    fbe_bool_t                     bDownloadable;
    fbe_bool_t                     hasResumeProm;
    fbe_bool_t                     resumePromReadFailed;
    fbe_bool_t                     fupFailure;
    fbe_u8_t                       firmwareRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1];
    fbe_u8_t                       subenclProductId[FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE + 1];
    HW_MODULE_TYPE                 uniqueId;
    SP_ID                          associatedSp;
    fbe_fan_location_t             fanLocation;
    /***************************************
     * Applies only to XPE and DPE fans    *
     ***************************************/
    fbe_mgmt_status_t              multiFanModuleFault;
    /* Multiple Fan Modules are faulted for a certain amount of time. */
    fbe_mgmt_status_t              persistentMultiFanModuleFault;   
    /* Only for Processor enclosure fans */
    fbe_bool_t                     isFaultRegFail;
} fbe_cooling_info_t;

/*
 * FBE_BASE_BOARD_CONTROL_CODE_SET_MGMT_PORT
 */

/*!********************************************************************* 
 * @enum fbe_mgmt_port_auto_neg_t 
 *  
 * @brief 
 *  This contains the enumeration for the port auto config.
 **********************************************************************/
typedef enum fbe_mgmt_port_auto_neg_e
{
    FBE_PORT_AUTO_NEG_OFF            = 0,
    FBE_PORT_AUTO_NEG_ON             = 1,
    FBE_PORT_AUTO_NEG_UNSPECIFIED    = 252,
    FBE_PORT_AUTO_NEG_INVALID        = 253,
    FBE_PORT_AUTO_NEG_INDETERMIN     = 254,
    FBE_PORT_AUTO_NEG_NA             = 255 
} fbe_mgmt_port_auto_neg_t;

/*!********************************************************************* 
 * @enum fbe_mgmt_port_speed_e 
 *  
 * @brief 
 *  This contains the enumeration for the port speed.
 **********************************************************************/
typedef enum fbe_mgmt_port_speed_e
{
    FBE_MGMT_PORT_SPEED_1000MBPS                 = SPEED_ONE_GIGABIT,  // The value matches the standard speed value SPEED_ONE_GIGABIT
    FBE_MGMT_PORT_SPEED_10MBPS                   = SPEED_TEN_MEGABIT,  // The value matches the standard speed value SPEED_TEN_MEGABIT 
    FBE_MGMT_PORT_SPEED_100MBPS                  = SPEED_100_MEGABIT,  // The value matches the standard speed value SPEED_100_MEGABIT
    FBE_MGMT_PORT_SPEED_UNSPECIFIED              = 252,  //  For Write;
    FBE_MGMT_PORT_SPEED_INVALID                  = 253,
    FBE_MGMT_PORT_SPEED_INDETERMIN               = 254,  //  For Read.
    FBE_MGMT_PORT_SPEED_NA                       = 255   //  For Read.
}fbe_mgmt_port_speed_t;

/*!********************************************************************* 
 * @enum fbe_mgmt_port_duplex_mode_e 
 *  
 * @brief 
 *  This contains the enumeration for the duplex mode.
 **********************************************************************/
typedef enum fbe_mgmt_port_duplex_mode_e
{
    FBE_PORT_DUPLEX_MODE_HALF               = 0,
    FBE_PORT_DUPLEX_MODE_FULL               = 1,
    FBE_PORT_DUPLEX_MODE_UNSPECIFIED        = 252,  //  For Write;  
    FBE_PORT_DUPLEX_MODE_INVALID            = 253,
    FBE_PORT_DUPLEX_MODE_INDETERMIN         = 254,  //  For Read.
    FBE_PORT_DUPLEX_MODE_NA                 = 255
} fbe_mgmt_port_duplex_mode_t;

/*!****************************************************************************
 *    
 * @struct fbe_mgmt_port_config_info_s
 *  
 * @brief 
 *   This structure use during read/write persistence data for management port 
 *   speed configuration.
 ******************************************************************************/
typedef struct fbe_mgmt_port_config_info_s
{
    fbe_mgmt_port_auto_neg_t        mgmtPortAutoNeg;
    fbe_mgmt_port_speed_t           mgmtPortSpeed;
    fbe_mgmt_port_duplex_mode_t     mgmtPortDuplex;
}fbe_mgmt_port_config_info_t;

typedef struct fbe_mgmt_port_status_s
{
    fbe_mgmt_port_auto_neg_t        mgmtPortAutoNeg;
    fbe_mgmt_port_speed_t           mgmtPortSpeed;
    fbe_mgmt_port_duplex_mode_t     mgmtPortDuplex;
    fbe_bool_t                      linkStatus;

}fbe_mgmt_port_status_t;

typedef struct fbe_board_mgmt_set_mgmt_port_s
{
    SP_ID                       sp_id;
    fbe_u32_t                   mgmtID; 
    PORT_ID_TYPE                portIDType;
    fbe_mgmt_port_config_info_t mgmtPortConfig;
}fbe_board_mgmt_set_mgmt_port_t;

/*
 * FBE_BASE_BOARD_CONTROL_CODE_GET_MGMT_MODULE_INFO
 */
typedef struct fbe_board_mgmt_mgmt_module_info_s
{
    /*  Which SP the FRU is associated with. */
    SP_ID                          associatedSp;
    /* Each Blade based slot number. */
    fbe_u32_t                      mgmtID;  
    fbe_env_inferface_status_t     envInterfaceStatus;
    SPECL_ERROR                    transactionStatus;
    /* Whether the fru is associated to the running SP. */
    fbe_bool_t                     isLocalFru;      
    HW_MODULE_TYPE                 uniqueID;
    fbe_bool_t                     bInserted;
    fbe_mgmt_status_t              generalFault; 
    fbe_u8_t                       faultInfoStatus;
    LED_BLINK_RATE                 faultLEDStatus;
    VLAN_CONFIG_MODE               vLanConfigMode;
    fbe_mgmt_port_status_t         managementPort;
    PORT_STS_CONFIG                servicePort;
    fbe_u8_t                       firmwareRevMajor;
    fbe_u8_t                       firmwareRevMinor;
    fbe_bool_t                     isFaultRegFail;
} fbe_board_mgmt_mgmt_module_info_t;

typedef struct fbe_peer_boot_flt_exp_info_s
{
    /*  Which SP the FRU is associated with. */
    SP_ID                          associatedSp; 
    /*  Each blade based mezzanine ID. */
    FLT_REG_IDS                    fltExpID; 
    fbe_env_inferface_status_t     envInterfaceStatus;
    SPECL_ERROR                    transactionStatus;
    /* Hardware type store fault status */
    fbe_u64_t              fltHwType;  
    SPECL_FAULT_REGISTER_STATUS faultRegisterStatus;
} fbe_peer_boot_flt_exp_info_t; //old type: fbe_board_mgmt_flt_exp_info_t

/*
 * FBE_BASE_BOARD_CONTROL_CODE_GET_SLAVE_PORT_INFO
 */
typedef struct fbe_board_mgmt_slave_port_info_s
{
    /*  Which SP the FRU is associated with. */
    SP_ID                          associatedSp; 
    /*  Each blade based mezzanine ID. */
    FLT_REG_IDS                    slavePortID;  
    fbe_env_inferface_status_t     envInterfaceStatus;
    SPECL_ERROR                    transactionStatus;
    /* Whether the fru is associated to the running SP. */
    fbe_bool_t                     isLocalFru;      
    fbe_u8_t                       generalStatus;
    fbe_u8_t                       componentStatus;
    fbe_u8_t                       componentExtStatus;
} fbe_board_mgmt_slave_port_info_t;

/*
 * FBE_BASE_BOARD_CONTROL_CODE_GET_SUITCASE_INFO
 */

typedef enum
{
    FBE_SUITCASE_STATE_UNKNOWN = 0,
    FBE_SUITCASE_STATE_OK,
    FBE_SUITCASE_STATE_FAULTED,
    FBE_SUITCASE_STATE_DEGRADED,
    FBE_SUITCASE_STATE_REMOVED,
}fbe_suitcase_state_t;

typedef enum
{
    FBE_SUITCASE_SUBSTATE_NO_FAULT = 0,
    FBE_SUITCASE_SUBSTATE_FLT_STATUS_REG_FAULT,
    FBE_SUITCASE_SUBSTATE_HW_ERR_MON_FAULT,
    FBE_SUITCASE_SUBSTATE_LOW_BATTERY,
    FBE_SUITCASE_SUBSTATE_RP_READ_FAILURE,
    FBE_SUITCASE_SUBSTATE_ENV_INTF_FAILURE,
    FBE_SUITCASE_SUBSTATE_INTERNAL_CABLE_MISSING,
    FBE_SUITCASE_SUBSTATE_INTERNAL_CABLE_CROSSED,
    FBE_SUITCASE_SUBSTATE_BIST_FAILURE,
} fbe_suitcase_subState_t;

typedef struct fbe_board_mgmt_suitcase_info_s
{
    /*  Which SP the FRU is associated with. */
    SP_ID                          associatedSp; 
    /*  Each blade based Suitcase ID.  */
    fbe_u32_t                      suitcaseID;  
    fbe_env_inferface_status_t     envInterfaceStatus;
    SPECL_ERROR                    transactionStatus;
    /* Whether the fru is associated to the running SP. */
    fbe_bool_t                     isLocalFru;      

    fbe_suitcase_state_t           state;
    fbe_suitcase_subState_t        subState;
    /* The following data is from SPECL */
    fbe_mgmt_status_t              Tap12VMissing;
    fbe_mgmt_status_t              shutdownWarning;
    fbe_mgmt_status_t              ambientOvertempFault;
    fbe_mgmt_status_t              ambientOvertempWarning;
    fbe_mgmt_status_t              fanPackFailure;
    fbe_u8_t                       tempSensor;
    /* cpu Fault */

    fbe_bool_t                     resumePromReadFailed;

    fbe_bool_t                     isCPUFaultRegFail;
    fbe_bool_t                     hwErrMonFault;           // fault reported by HwErrMon
} fbe_board_mgmt_suitcase_info_t;

typedef struct fbe_board_mgmt_bist_info_s
{
    BMC_BIST_RESULT         cpuTest;
    BMC_BIST_RESULT         dramTest;
    BMC_BIST_RESULT         sramTest;
    BMC_BIST_RESULT         i2cTests[I2C_TEST_MAX];
    BMC_BIST_RESULT         uartTests[UART_TEST_MAX];
    BMC_BIST_RESULT         sspTest;
    BMC_BIST_RESULT         bbuTests[BBU_TEST_MAX];
    BMC_BIST_RESULT         nvramTest;
    BMC_BIST_RESULT         sgpioTest;
    BMC_BIST_RESULT         fanTests[FAN_TEST_MAX];
    BMC_BIST_RESULT         arbTests[ARB_TEST_MAX];
    BMC_BIST_RESULT         ncsiLanTest;
    BMC_BIST_RESULT         dedicatedLanTest;
} fbe_board_mgmt_bist_info_t;

/*
 * FBE_BASE_BOARD_CONTROL_CODE_GET_BMC_INFO
 */
#define FBE_BMC_RIDE_THROUGH_TIMER_DISABLE_VALUE        0
#define FBE_BMC_RIDE_THROUGH_TIMER_ENABLE_VALUE         10

typedef struct fbe_board_mgmt_bmc_info_s
{
    /*  Which SP the FRU is associated with. */
    SP_ID                           associatedSp;
    /*  Each blade based BMC ID */
    fbe_u32_t                       bmcID;
    fbe_env_inferface_status_t      envInterfaceStatus;
    SPECL_ERROR                     transactionStatus;
    /* Whether the fru is associated to the running SP. */
    fbe_bool_t                      isLocalFru;
    /* The following data is from SPECL */
    fbe_bool_t                      shutdownWarning;
    SHUTDOWN_REASON                 shutdownReason;
    fbe_u32_t                       shutdownTimeRemaining;
    fbe_bool_t                      bistFailureDetected;
    fbe_board_mgmt_bist_info_t      bistReport;
    SPECL_BMC_FW_UPDATE_STATUS      firmwareStatus;
    fbe_bool_t                      rideThroughTimeSupported;
    fbe_bool_t                      lowPowerMode;               // only valid if rideThroughTimeSupported True
    fbe_u8_t                        rideThroughTime;            // only valid if rideThroughTimeSupported True
    fbe_u8_t                        vaultTimeout;               // only valid if rideThroughTimeSupported True
} fbe_board_mgmt_bmc_info_t;

/*
 * FBE_BASE_BOARD_CONTROL_CODE_GET_CACHE_CARD_INFO
 */
typedef struct fbe_board_mgmt_cache_card_info_s
{
    /*  Which SP the FRU is associated with. */
    SP_ID                           associatedSp;
    /*  Each blade based cacheCard ID */
    fbe_u32_t                       cacheCardID;
    fbe_env_inferface_status_t      envInterfaceStatus;
    /* Whether the fru is associated to the running SP. */
    fbe_bool_t                      isLocalFru;
    /* The following data is from SPECL */
    HW_MODULE_TYPE                  uniqueID;
    fbe_mgmt_status_t               inserted;
    LED_BLINK_RATE                  faultLEDStatus;
    fbe_mgmt_status_t               powerGood;
    fbe_mgmt_status_t               powerUpFailure;
    fbe_mgmt_status_t               powerUpEnable;
    fbe_mgmt_status_t               reset;
    fbe_mgmt_status_t               powerEnable;
} fbe_board_mgmt_cache_card_info_t;

/*
 * FBE_BASE_BOARD_CONTROL_CODE_GET_DIMM_INFO
 */
typedef enum
{
    FBE_DIMM_STATE_UNKNOWN = 0,
    FBE_DIMM_STATE_OK,
    FBE_DIMM_STATE_FAULTED,
    FBE_DIMM_STATE_EMPTY,
    FBE_DIMM_STATE_MISSING,
    FBE_DIMM_STATE_NEED_TO_REMOVE,
    FBE_DIMM_STATE_DEGRADED,
}fbe_dimm_state_t;

typedef enum
{
    FBE_DIMM_SUBSTATE_NO_FAULT = 0,
    FBE_DIMM_SUBSTATE_GEN_FAULT,
    FBE_DIMM_SUBSTATE_FLT_STATUS_REG_FAULT,
    FBE_DIMM_SUBSTATE_HW_ERR_MON_FAULT,
    FBE_DIMM_SUBSTATE_ENV_INTF_FAILURE,
} fbe_dimm_subState_t;

typedef struct fbe_board_mgmt_dimm_info_s
{
    /*  Which SP the FRU is associated with. */
    SP_ID                           associatedSp;
    /*  Each blade based dimm ID */
    fbe_u32_t                       dimmID;
    fbe_env_inferface_status_t      envInterfaceStatus;
    /* Whether the fru is associated to the running SP. */
    fbe_bool_t                      isLocalFru;
    /* The following data is from SPECL */
    fbe_mgmt_status_t       inserted;
    fbe_mgmt_status_t       faulted;  
    fbe_dimm_state_t        state;
    fbe_dimm_subState_t     subState;
    SPD_DEVICE_TYPE         dimmType;
    SPD_REVISION            dimmRevision;
    fbe_u32_t               dimmDensity;
    ULONG                   numOfBanks;
    ULONG                   deviceWidth;
    ULONG                   numberOfRanks;
    BOOL                    vendorRegionValid;
    csx_uchar_t             jedecIdCode             [JEDEC_ID_CODE_SIZE+1];
    csx_uchar_t             vendorName              [EMC_DIMM_VENDOR_NAME_SIZE]; //decode from JedecIDCode[0]
    csx_uchar_t             assemblyName            [EMC_DIMM_MODULE_NAME_SIZE]; //decode from dimmType
    csx_uchar_t             manufacturingLocation;
    csx_uchar_t             manufacturingDate       [MANUFACTURING_DATE_SIZE+1];
    csx_uchar_t             moduleSerialNumber      [MODULE_SERIAL_NUMBER_SIZE+1];
    csx_uchar_t             partNumber              [PART_NUMBER_SIZE_2+1];
    csx_uchar_t             EMCDimmPartNumber       [EMC_DIMM_PART_NUMBER_SIZE+1];
    csx_uchar_t             EMCDimmSerialNumber     [EMC_DIMM_SERIAL_NUMBER_SIZE+1];
    BYTE                    OldEMCDimmSerialNumber  [OLD_EMC_DIMM_SERIAL_NUMBER_SIZE+1];
    fbe_bool_t              faultRegFault;          
    /*
     * EDAL currently only saves the local DIMM data. 
     * So we need to add this field to save the fault reported by the fault register for the peer DIMM.
     */
    fbe_bool_t              peerFaultRegFault;  
    fbe_bool_t              hwErrMonFault;          // fault reported by HwErrMon
} fbe_board_mgmt_dimm_info_t;


/* FBE_BASE_BOARD_CONTROL_CODE_GET_RESUME */
typedef struct fbe_board_mgmt_resume_prom_info_s
{
    fbe_bool_t                     retriesExhausted;
    SPECL_ERROR                    lastWriteStatus;
    fbe_resume_prom_status_t       resumePromStatus;
}fbe_board_mgmt_resume_prom_info_t;

/*
 * FBE_BASE_BOARD_CONTROL_GET_SSD_INFO  
 */

typedef enum
{
    FBE_SSD_STATE_UNKNOWN = 0,
    FBE_SSD_STATE_OK,
    FBE_SSD_STATE_FAULTED,
    FBE_SSD_STATE_NOT_PRESENT,
    FBE_SSD_STATE_DEGRADED,
}fbe_ssd_state_t;

typedef enum
{
    FBE_SSD_SUBSTATE_NO_FAULT = 0,
    FBE_SSD_SUBSTATE_GEN_FAULT,
    FBE_SSD_SUBSTATE_FLT_STATUS_REG_FAULT,
} fbe_ssd_subState_t;

typedef struct fbe_board_mgmt_ssd_info_s
{
    /*  Which SP the FRU is associated with. */
    SP_ID                  associatedSp;
    fbe_u32_t              slot;
    /* Whether the fru is associated to the running SP. */
    fbe_bool_t             isLocalFru;
    /* This field is populated in ESP, not in the physical package.
     * Ideally, the ESP should use its own struct for its implementation.
     */
    fbe_ssd_state_t        ssdState;
    fbe_ssd_subState_t     ssdSubState;   
    /* The following fields are only available for a local SSD, not available for a peer SSD. */
    fbe_u32_t              remainingSpareBlkCount;
    fbe_u32_t              ssdLifeUsed;
    fbe_bool_t             ssdSelfTestPassed;
    fbe_u32_t              ssdTemperature;
    /* Resume related info */
    fbe_char_t             ssdSerialNumber      [FBE_SSD_SERIAL_NUMBER_SIZE + 1];
    fbe_char_t             ssdPartNumber        [FBE_SSD_PART_NUMBER_SIZE +1];
    fbe_char_t             ssdAssemblyName      [FBE_SSD_ASSEMBLY_NAME_SIZE +1];
    fbe_char_t             ssdFirmwareRevision  [FBE_SSD_FIRMWARE_REVISION_SIZE + 1];
    fbe_bool_t             isPeerFaultRegFail;
    fbe_bool_t             isLocalFaultRegFail;
}fbe_board_mgmt_ssd_info_t;


#endif  // ifndef FBE_PE_TYPES_H
