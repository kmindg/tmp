#ifndef FBE_ENVIRONMENT_LIMIT_H
#define FBE_ENVIRONMENT_LIMIT_H
/******************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *****************************************************************************/

/*!****************************************************************************
 * @file fbe_environment_limit.h
 ******************************************************************************
 *
 * @brief
 *  This file contain declaration of function which use for environment limit
 *  service.
 *
 * @version
 *   11-Nov-2010 - Wayne Garrett Created
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_module_info.h"
#include "fbe/fbe_limits.h"

/*!********************************************************************* 
 * @struct fbe_environment_limits_platform_hardware_limits_t 
 *  
 * @brief 
 *   Contains the platform limits information relevant to the physical
 *   characteristics of the hardware.  For example, a platform may have
 *   2 SLIC slots, but may have a limit of 0 max SLICs.
 *   These are all listed on a per SP basis.
 *
 **********************************************************************/
typedef struct fbe_environment_limits_platform_hardware_limits_s
{
    fbe_u32_t               max_slics;                  /*!< SLIC Maximum for this Platform */
    fbe_u32_t               max_mezzanines;             /*!< Mezzanine Maximum for this Platform */
    fbe_module_slic_type_t  supported_slic_types;       /*!< Bitmask of supported slic types for this Platform */
}fbe_environment_limits_platform_hardware_limits_t;

/*!********************************************************************* 
 * @struct fbe_environment_limits_platform_port_limits_t 
 *  
 * @brief 
 *   Contains the limits for port configurations for this platform.
 *   These are all listed on a per SP basis.
 *
 **********************************************************************/
typedef struct fbe_environment_limits_platform_port_limits_s
{
    fbe_u32_t           max_sas_be;                 /*!< SAS Back-End Port Maximum */
    fbe_u32_t           max_sas_fe;                 /*!< SAS Front-End Port Maximum */
    fbe_u32_t           max_fc_fe;                  /*!< Fibre Channel Front-End Port Maximum */
    fbe_u32_t           max_iscsi_1g_fe;            /*!< 1G iSCSI Front-End Port Maximum */
    fbe_u32_t           max_iscsi_10g_fe;           /*!< 10G iSCSI Front-End Port Maximum*/
    fbe_u32_t           max_fcoe_fe;                /*!< FCoE Front-End Port Maximum */   
    fbe_u32_t           max_combined_iscsi_fe;      /*!< Combined iSCSI limit, if 0 there is no combined limit */      
}fbe_environment_limits_platform_port_limits_t;

/*!********************************************************************* 
 * @struct fbe_environment_limits_s 
 *  
 * @brief 
 *    Contains the environment limits for platform.
 *
 * @ingroup fbe_api_environment_limit_service_interface
 * @ingroup fbe_environment_limit
 *
 **********************************************************************/
typedef struct fbe_environment_limits_s {
    fbe_u32_t platform_fru_count;    /*!< Number of frus supported on this platform */
    fbe_u32_t platform_max_be_count;    /*!<  Max number of back end buses supported on this platform */
    fbe_u32_t platform_max_encl_count_per_bus;  /*!< Max number of enclosures per bus supported. */
    fbe_u32_t platform_max_rg;    /*!<  Max number of RG we allow */
    fbe_u32_t platform_max_user_lun;    /*!<  Max number of user visible LUNs */
    fbe_u32_t platform_max_lun_per_rg;    /*!<  Max number of LUNs per RG */
    fbe_environment_limits_platform_hardware_limits_t platform_hardware_limits;
    fbe_environment_limits_platform_port_limits_t platform_port_limits;
}fbe_environment_limits_t;

/*!************************************************************
* @enum fbe_environment_limit_control_code_t
*
* @brief 
*    This contains the enum data info for environment limit service control code.
*
***************************************************************/
typedef enum fbe_environment_limit_control_code_e {
    FBE_ENVIRONMENT_LIMIT_CONTROL_CODE_INVALID = FBE_SERVICE_CONTROL_CODE_INVALID_DEF(FBE_SERVICE_ID_ENVIRONMENT_LIMIT),  /*!< Invalid Control code*/
    FBE_ENVIRONMENT_LIMIT_CONTROL_CODE_GET_LIMITS,    /*!<Get environment limits */
    FBE_ENVIRONMENT_LIMIT_CONTROL_CODE_SET_LIMITS         /*!<Set environment limits*/
} fbe_environment_limit_control_code_t;

fbe_u32_t fbe_environment_limit_get_platform_fru_count(void);
fbe_u32_t fbe_environment_limit_get_platform_max_be_count(void);
fbe_u32_t fbe_environment_limit_get_platform_max_encl_count_per_bus(void);
fbe_status_t fbe_environment_limit_get_board_object_id(fbe_object_id_t *board_id);
fbe_status_t fbe_environment_limit_get_platform_max_memory_mb(fbe_u32_t * platform_max_memory_mb);
void fbe_environment_limit_get_platform_hardware_limits(fbe_environment_limits_platform_hardware_limits_t * hardware_limits);
void fbe_environment_limit_get_platform_port_limits(fbe_environment_limits_platform_port_limits_t * port_limits);
fbe_status_t fbe_environment_limit_get_initiated_limits(fbe_environment_limits_t *env_limits);
void fbe_environment_limit_get_max_memory_mb(SPID_HW_TYPE hw_type,
                                             fbe_u32_t * platform_max_memory_mb);

#define PLATFORM_MAX_LUN_PER_RG   256

#define PLATFORM_MAX_ENCL_COUNT_PER_BUS FBE_ENCLOSURES_PER_BUS



#endif /* FBE_ENVIRONMENT_LIMIT_H */
