/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

 /*!*************************************************************************
 * @file fbe_environment_limit_for_sep_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains simulation mode implementation which will be used by SEP package
 *
 * @ingroup environment_limit_service_files
 * 
 *
 ***************************************************************************/
#include "fbe_environment_limit_private.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_pe_types.h"
#include "core_config_runtime.h"
#include "fbe/fbe_board.h"
#include "fbe/fbe_transport.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_service_manager_interface.h"

extern BOOL ccrInitCoreConfig_impl(SPID_HW_TYPE PlatformType);
static fbe_status_t fbe_environment_limit_get_platform_type(fbe_object_id_t board_id, SPID_HW_TYPE *platform_type);
//static fbe_status_t fbe_environment_limit_get_board_object_id(fbe_object_id_t *board_id);

/* 
 * Port limits and supported SLIC types for the various platforms
 */
static fbe_environment_limits_platform_hardware_limits_t    prometheus_m1_hw_limit      = {11,0,
            FBE_SLIC_TYPE_FC_8G|FBE_SLIC_TYPE_ISCSI_10G|FBE_SLIC_TYPE_FCOE|FBE_SLIC_TYPE_4PORT_ISCSI_1G| FBE_SLIC_TYPE_FC_16G | 
            FBE_SLIC_TYPE_6G_SAS_1|FBE_SLIC_TYPE_ISCSI_COPPER|FBE_SLIC_TYPE_ISCSI_10G_V2|FBE_SLIC_TYPE_6G_SAS_3|FBE_SLIC_TYPE_12G_SAS};
static fbe_environment_limits_platform_port_limits_t        prometheus_m1_port_limit    = {16,0,36,8,8,18,16};

static fbe_environment_limits_platform_hardware_limits_t    enterprise_hw_limit         = {10,0,
            FBE_SLIC_TYPE_FC_8G|FBE_SLIC_TYPE_FCOE|FBE_SLIC_TYPE_4PORT_ISCSI_1G| FBE_SLIC_TYPE_FC_16G |
            FBE_SLIC_TYPE_ISCSI_COPPER|FBE_SLIC_TYPE_ISCSI_10G_V2|FBE_SLIC_TYPE_6G_SAS_3};
static fbe_environment_limits_platform_port_limits_t        enterprise_port_limit       = {16,0,36,8,8,18};

static fbe_environment_limits_platform_hardware_limits_t    prometheus_s1_hw_limit      = {11,0,
            FBE_SLIC_TYPE_FC_8G|FBE_SLIC_TYPE_FCOE|FBE_SLIC_TYPE_4PORT_ISCSI_1G| FBE_SLIC_TYPE_FC_16G |
            FBE_SLIC_TYPE_ISCSI_COPPER|FBE_SLIC_TYPE_ISCSI_10G_V2|FBE_SLIC_TYPE_6G_SAS_3|FBE_SLIC_TYPE_12G_SAS};
static fbe_environment_limits_platform_port_limits_t        prometheus_s1_port_limit    = {8,0,16,12,6,8,16};

static fbe_environment_limits_platform_hardware_limits_t    defiant_m1_hw_limit         = {5,1,
            FBE_SLIC_TYPE_FC_8G|FBE_SLIC_TYPE_FCOE|FBE_SLIC_TYPE_4PORT_ISCSI_1G| FBE_SLIC_TYPE_FC_16G |
            FBE_SLIC_TYPE_ISCSI_COPPER|FBE_SLIC_TYPE_ISCSI_10G_V2|FBE_SLIC_TYPE_6G_SAS_3|FBE_SLIC_TYPE_12G_SAS|FBE_SLIC_TYPE_NA};
static fbe_environment_limits_platform_port_limits_t        defiant_m1_port_limit       = {6,0,20,8,8,10,16};

static fbe_environment_limits_platform_hardware_limits_t    defiant_m2_hw_limit         = {5,1,
            FBE_SLIC_TYPE_FC_8G|FBE_SLIC_TYPE_FCOE|FBE_SLIC_TYPE_4PORT_ISCSI_1G| FBE_SLIC_TYPE_FC_16G |
            FBE_SLIC_TYPE_ISCSI_COPPER|FBE_SLIC_TYPE_ISCSI_10G_V2|FBE_SLIC_TYPE_6G_SAS_3|FBE_SLIC_TYPE_12G_SAS|FBE_SLIC_TYPE_NA};
static fbe_environment_limits_platform_port_limits_t        defiant_m2_port_limit       = {6,0,20,8,8,10,16};

static fbe_environment_limits_platform_hardware_limits_t    defiant_m3_hw_limit         = {5,1,
            FBE_SLIC_TYPE_FC_8G|FBE_SLIC_TYPE_FCOE|FBE_SLIC_TYPE_4PORT_ISCSI_1G| FBE_SLIC_TYPE_FC_16G |
            FBE_SLIC_TYPE_ISCSI_COPPER|FBE_SLIC_TYPE_ISCSI_10G_V2|FBE_SLIC_TYPE_6G_SAS_3|FBE_SLIC_TYPE_12G_SAS|FBE_SLIC_TYPE_NA};
static fbe_environment_limits_platform_port_limits_t        defiant_m3_port_limit       = {6,0,20,8,8,10,16};

static fbe_environment_limits_platform_hardware_limits_t    defiant_m4_hw_limit         = {4,1,
            FBE_SLIC_TYPE_FC_8G|FBE_SLIC_TYPE_FCOE|FBE_SLIC_TYPE_4PORT_ISCSI_1G| FBE_SLIC_TYPE_FC_16G |
            FBE_SLIC_TYPE_ISCSI_COPPER|FBE_SLIC_TYPE_ISCSI_10G_V2|FBE_SLIC_TYPE_NA};
static fbe_environment_limits_platform_port_limits_t        defiant_m4_port_limit       = {2,0,16,8,8,8,16};

static fbe_environment_limits_platform_hardware_limits_t    defiant_m5_hw_limit         = {3,1,
            FBE_SLIC_TYPE_FC_8G|FBE_SLIC_TYPE_FCOE|FBE_SLIC_TYPE_4PORT_ISCSI_1G| FBE_SLIC_TYPE_FC_16G |
            FBE_SLIC_TYPE_ISCSI_COPPER|FBE_SLIC_TYPE_ISCSI_10G_V2|FBE_SLIC_TYPE_NA};
static fbe_environment_limits_platform_port_limits_t        defiant_m5_port_limit       = {2,0,16,8,8,8,16};

static fbe_environment_limits_platform_hardware_limits_t    defiant_s1_hw_limit         = {2,1,
            FBE_SLIC_TYPE_FC_8G|FBE_SLIC_TYPE_FCOE|FBE_SLIC_TYPE_4PORT_ISCSI_1G| FBE_SLIC_TYPE_FC_16G |
            FBE_SLIC_TYPE_ISCSI_COPPER|FBE_SLIC_TYPE_ISCSI_10G_V2|FBE_SLIC_TYPE_6G_SAS_3|FBE_SLIC_TYPE_NA};
static fbe_environment_limits_platform_port_limits_t        defiant_s1_port_limit       = {6,0,16,12,6,8,16};

static fbe_environment_limits_platform_hardware_limits_t    defiant_s4_hw_limit         = {2,1,
            FBE_SLIC_TYPE_FC_8G|FBE_SLIC_TYPE_FCOE|FBE_SLIC_TYPE_4PORT_ISCSI_1G| FBE_SLIC_TYPE_FC_16G |
            FBE_SLIC_TYPE_ISCSI_COPPER|FBE_SLIC_TYPE_ISCSI_10G_V2|FBE_SLIC_TYPE_6G_SAS_3|FBE_SLIC_TYPE_NA};
static fbe_environment_limits_platform_port_limits_t        defiant_s4_port_limit       = {6,0,16,12,6,8,16};


static fbe_environment_limits_platform_hardware_limits_t    defiant_k2_hw_limit         = {5,0,
            FBE_SLIC_TYPE_FC_8G|FBE_SLIC_TYPE_4PORT_ISCSI_1G|
            FBE_SLIC_TYPE_6G_SAS_3|FBE_SLIC_TYPE_NA};
static fbe_environment_limits_platform_port_limits_t        defiant_k2_port_limit       = {6,0,20,16,16,10,0};

static fbe_environment_limits_platform_hardware_limits_t    defiant_k3_hw_limit         = {5,0,
            FBE_SLIC_TYPE_FC_8G|FBE_SLIC_TYPE_4PORT_ISCSI_1G|
            FBE_SLIC_TYPE_6G_SAS_3|FBE_SLIC_TYPE_NA};
static fbe_environment_limits_platform_port_limits_t        defiant_k3_port_limit       = {6,0,20,16,16,10,0};

static fbe_environment_limits_platform_hardware_limits_t    nova_hw_limit               = {1,1,
            FBE_SLIC_TYPE_FC_8G|FBE_SLIC_TYPE_FC_8G_4S|FBE_SLIC_TYPE_FC_8G_1S3M|
            FBE_SLIC_TYPE_ISCSI_10G|FBE_SLIC_TYPE_FCOE|FBE_SLIC_TYPE_4PORT_ISCSI_1G|FBE_SLIC_TYPE_6G_SAS_1|
            FBE_SLIC_TYPE_ISCSI_COPPER|FBE_SLIC_TYPE_ISCSI_10G_V2|FBE_SLIC_TYPE_6G_SAS_3|FBE_SLIC_TYPE_4PORT_ISCSI_10G|FBE_SLIC_TYPE_NA};
static fbe_environment_limits_platform_port_limits_t        nova_port_limit             = {2,0,4,8,8,0,0};

//nova3_hw_limit={1-eSLIC,1-mezz,eGlacier|eSupercell|eEruption|Mezzanine}

static fbe_environment_limits_platform_hardware_limits_t    nova3_hw_limit              = {1,1,
             FBE_SLIC_TYPE_FC_8G|FBE_SLIC_TYPE_FC_8G_4S|FBE_SLIC_TYPE_FC_8G_1S3M|
             FBE_SLIC_TYPE_4PORT_ISCSI_1G|FBE_SLIC_TYPE_ISCSI_COPPER|FBE_SLIC_TYPE_NA};
static fbe_environment_limits_platform_port_limits_t        nova3_port_limit            = {2,0,4,8,8,0,0};


//nova3_xm_hw_limit={1-eSLIC,1-mezz,eGlacier|eSupercell|eEruption|Mezzanine}

static fbe_environment_limits_platform_hardware_limits_t    nova3_xm_hw_limit           = {1,1,
            FBE_SLIC_TYPE_FC_8G|FBE_SLIC_TYPE_FC_8G_4S|FBE_SLIC_TYPE_FC_8G_1S3M|
            FBE_SLIC_TYPE_4PORT_ISCSI_1G|FBE_SLIC_TYPE_ISCSI_COPPER|FBE_SLIC_TYPE_NA};
static fbe_environment_limits_platform_port_limits_t        nova3_xm_port_limit         = {2,0,4,8,6,0,0};


static fbe_environment_limits_platform_hardware_limits_t    nova_s1_hw_limit            = {1,1,
            FBE_SLIC_TYPE_FC_8G|FBE_SLIC_TYPE_FC_8G_4S|FBE_SLIC_TYPE_FC_8G_1S3M|
            FBE_SLIC_TYPE_ISCSI_10G|FBE_SLIC_TYPE_FCOE|FBE_SLIC_TYPE_4PORT_ISCSI_1G|FBE_SLIC_TYPE_6G_SAS_1|
            FBE_SLIC_TYPE_ISCSI_COPPER|FBE_SLIC_TYPE_ISCSI_10G_V2|FBE_SLIC_TYPE_6G_SAS_3|FBE_SLIC_TYPE_4PORT_ISCSI_10G|FBE_SLIC_TYPE_NA};
static fbe_environment_limits_platform_port_limits_t        nova_s1_port_limit          = {2,0,4,8,8,0,0};

static fbe_environment_limits_platform_hardware_limits_t    bearcat_hw_limit            = {1,1,
            FBE_SLIC_TYPE_FC_8G|FBE_SLIC_TYPE_ISCSI_10G|FBE_SLIC_TYPE_FCOE|FBE_SLIC_TYPE_4PORT_ISCSI_1G|FBE_SLIC_TYPE_6G_SAS_1|  
            FBE_SLIC_TYPE_ISCSI_COPPER|FBE_SLIC_TYPE_ISCSI_10G_V2|FBE_SLIC_TYPE_6G_SAS_3|FBE_SLIC_TYPE_4PORT_ISCSI_10G|FBE_SLIC_TYPE_NA};
static fbe_environment_limits_platform_port_limits_t        bearcat_port_limit          = {2,0,6,6,6,0,6};

//oberon_1_hw_limit={2-SLIC,1-mezz,ThunderheadX|RockslideX|Downburst|Vortex|Maelstrom|MaelstromX|DownburstX|Onboard}
static fbe_environment_limits_platform_hardware_limits_t    oberon_1_hw_limit      = {2,1,
            FBE_SLIC_TYPE_V2_4PORT_ISCSI_1G|
            FBE_SLIC_TYPE_V2_4PORT_ISCSI_10G|
            FBE_SLIC_TYPE_ISCSI_10G_V2|
            FBE_SLIC_TYPE_FC_16G|
            FBE_SLIC_TYPE_FCOE|
            FBE_SLIC_TYPE_V3_2PORT_ISCSI_10G|
            FBE_SLIC_TYPE_V3_4PORT_ISCSI_10G|
            FBE_SLIC_TYPE_NA};
static fbe_environment_limits_platform_port_limits_t        oberon_1_port_limit    = {2,0,10,8,12,10,0};

//oberon_2_hw_limit={2-SLIC,1-mezz,Snowdevil/Dustdevil|ThunderheadX|RockslideX|Downburst|Vortex|Maelstrom|MaelstromX|DownburstX|Onboard}
static fbe_environment_limits_platform_hardware_limits_t    oberon_2_hw_limit      = {2,1,
            FBE_SLIC_TYPE_12G_SAS|
            FBE_SLIC_TYPE_V2_4PORT_ISCSI_1G|
            FBE_SLIC_TYPE_V2_4PORT_ISCSI_10G|
            FBE_SLIC_TYPE_ISCSI_10G_V2|
            FBE_SLIC_TYPE_FC_16G|
            FBE_SLIC_TYPE_FCOE|
            FBE_SLIC_TYPE_V3_2PORT_ISCSI_10G|
            FBE_SLIC_TYPE_V3_4PORT_ISCSI_10G|
            FBE_SLIC_TYPE_NA};
static fbe_environment_limits_platform_port_limits_t        oberon_2_port_limit    = {6,0,10,8,12,10,0};

//oberon_3_hw_limit={2-SLIC,1-mezz,Snowdevil/Dustdevil|ThunderheadX|RockslideX|Downburst|Vortex|Maelstrom|MaelstromX|DownburstX|Onboard}
static fbe_environment_limits_platform_hardware_limits_t    oberon_3_hw_limit      = {2,1,
            FBE_SLIC_TYPE_12G_SAS|
            FBE_SLIC_TYPE_V2_4PORT_ISCSI_1G|
            FBE_SLIC_TYPE_V2_4PORT_ISCSI_10G|
            FBE_SLIC_TYPE_ISCSI_10G_V2|
            FBE_SLIC_TYPE_FC_16G|
            FBE_SLIC_TYPE_FCOE|
            FBE_SLIC_TYPE_V3_2PORT_ISCSI_10G|
            FBE_SLIC_TYPE_V3_4PORT_ISCSI_10G|
            FBE_SLIC_TYPE_NA};
static fbe_environment_limits_platform_port_limits_t        oberon_3_port_limit    = {6,0,10,8,12,10,0};

//oberon_4_hw_limit={2-SLIC,1-mezz,Snowdevil/Dustdevil|ThunderheadX|RockslideX|Downburst|Vortex|Maelstrom|MaelstromX|DownburstX|Onboard}
static fbe_environment_limits_platform_hardware_limits_t    oberon_4_hw_limit      = {2,1,
            FBE_SLIC_TYPE_12G_SAS|
            FBE_SLIC_TYPE_V2_4PORT_ISCSI_1G|
            FBE_SLIC_TYPE_V2_4PORT_ISCSI_10G|
            FBE_SLIC_TYPE_ISCSI_10G_V2|
            FBE_SLIC_TYPE_FC_16G|
            FBE_SLIC_TYPE_FCOE|
            FBE_SLIC_TYPE_V3_2PORT_ISCSI_10G|
            FBE_SLIC_TYPE_V3_4PORT_ISCSI_10G|
            FBE_SLIC_TYPE_NA};
static fbe_environment_limits_platform_port_limits_t        oberon_4_port_limit    = {6,0,10,8,12,10,0};

static fbe_environment_limits_platform_hardware_limits_t    oberon_s1_hw_limit      = {2,1,
            FBE_SLIC_TYPE_V2_4PORT_ISCSI_1G  |
            FBE_SLIC_TYPE_V2_4PORT_ISCSI_10G |
            FBE_SLIC_TYPE_FC_16G             |
            FBE_SLIC_TYPE_V3_2PORT_ISCSI_10G|
            FBE_SLIC_TYPE_V3_4PORT_ISCSI_10G|
            FBE_SLIC_TYPE_NA};
static fbe_environment_limits_platform_port_limits_t        oberon_s1_port_limit    = {2,0,10,12,12,0};

//hyperion_1_hw_limit={5-SLIC,1-mezz,Snowdevil/Dustdevil|ThunderheadX|RockslideX|Downburst|Vortex|Maelstrom|Lapetus}
static fbe_environment_limits_platform_hardware_limits_t    hyperion_1_hw_limit         = {5,1,
            FBE_SLIC_TYPE_12G_SAS|
            FBE_SLIC_TYPE_V2_4PORT_ISCSI_1G|
            FBE_SLIC_TYPE_V2_4PORT_ISCSI_10G|
            FBE_SLIC_TYPE_ISCSI_10G_V2|
            FBE_SLIC_TYPE_FC_16G|
            FBE_SLIC_TYPE_FCOE|
            FBE_SLIC_TYPE_NA};
static fbe_environment_limits_platform_port_limits_t        hyperion_1_port_limit       = {6,0,20,20,20,20,0};

//hyperion_2_hw_limit={5-SLIC,1-mezz,Snowdevil/Dustdevil|ThunderheadX|RockslideX|Downburst|Vortex|Maelstrom|Lapetus}
static fbe_environment_limits_platform_hardware_limits_t    hyperion_2_hw_limit         = {5,1,
            FBE_SLIC_TYPE_12G_SAS|
            FBE_SLIC_TYPE_V2_4PORT_ISCSI_1G|
            FBE_SLIC_TYPE_V2_4PORT_ISCSI_10G|
            FBE_SLIC_TYPE_ISCSI_10G_V2|
            FBE_SLIC_TYPE_FC_16G|
            FBE_SLIC_TYPE_FCOE|
            FBE_SLIC_TYPE_NA};
static fbe_environment_limits_platform_port_limits_t        hyperion_2_port_limit       = {10,0,20,20,20,20,0};

//hyperion_3_hw_limit={5-SLIC,1-mezz,Snowdevil/Dustdevil|ThunderheadX|RockslideX|Downburst|Vortex|Maelstrom|Lapetus}
static fbe_environment_limits_platform_hardware_limits_t    hyperion_3_hw_limit         = {5,1,
            FBE_SLIC_TYPE_12G_SAS|
            FBE_SLIC_TYPE_V2_4PORT_ISCSI_1G|
            FBE_SLIC_TYPE_V2_4PORT_ISCSI_10G|
            FBE_SLIC_TYPE_ISCSI_10G_V2|
            FBE_SLIC_TYPE_FC_16G|
            FBE_SLIC_TYPE_FCOE|
            FBE_SLIC_TYPE_NA};
static fbe_environment_limits_platform_port_limits_t        hyperion_3_port_limit       = {18,0,20,20,20,20,0};

static fbe_environment_limits_platform_hardware_limits_t    triton_1_hw_limit      = {11,0,
            FBE_SLIC_TYPE_FC_8G|FBE_SLIC_TYPE_ISCSI_10G|FBE_SLIC_TYPE_FCOE|FBE_SLIC_TYPE_4PORT_ISCSI_1G|FBE_SLIC_TYPE_6G_SAS_1|
            FBE_SLIC_TYPE_ISCSI_COPPER|FBE_SLIC_TYPE_ISCSI_10G_V2|FBE_SLIC_TYPE_6G_SAS_3};
static fbe_environment_limits_platform_port_limits_t        triton_1_port_limit    = {16,0,36,8,8,18,0};

/*!****************************************************************************
 * @fn fbe_environment_limit_init_limits()
 ******************************************************************************
 *
 * @brief
 *      This function initializes the environment limit service for 
 *       user mode
 *
 *
 * @return fbe_status_t
 *
 *
 ******************************************************************************/
fbe_status_t 
fbe_environment_limit_init_limits(fbe_environment_limits_t *env_limits)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_object_id_t                 board_object_id;
    SPID_HW_TYPE                    platform_type;

    status = fbe_environment_limit_get_board_object_id(&board_object_id);
    if (status != FBE_STATUS_OK)
    {
        /* trace the failure */
        fbe_environment_limit_trace(FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "\nUnable to obtain the object id for the board\n");

        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        status = fbe_environment_limit_get_platform_type(board_object_id, &platform_type);
        if (status != FBE_STATUS_OK)
        {
             fbe_environment_limit_trace(FBE_TRACE_LEVEL_ERROR,
                                         FBE_TRACE_MESSAGE_ID_INFO,
                                         "\nUnable to obtain platform information\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }
/*        
        if (!ccrInitCoreConfig_impl(platform_type)) 
        {
            fbe_environment_limit_trace(FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "\nThe Core Config Runtime library could not be initialized for this harware type\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }
*/
        switch(platform_type)
        {
        case SPID_ENTERPRISE_HW_TYPE:
            env_limits->platform_hardware_limits    = enterprise_hw_limit;
            env_limits->platform_port_limits        = enterprise_port_limit;
            env_limits->platform_fru_count = 1000;
            break;
// ENCL_CLEANUP - cases for Moons
        case SPID_PROMETHEUS_M1_HW_TYPE:
            env_limits->platform_hardware_limits    = prometheus_m1_hw_limit;
            env_limits->platform_port_limits        = prometheus_m1_port_limit;
            env_limits->platform_fru_count = 1000;
            break;
        case SPID_PROMETHEUS_S1_HW_TYPE:
            env_limits->platform_hardware_limits    = prometheus_s1_hw_limit;
            env_limits->platform_port_limits        = prometheus_s1_port_limit;
            env_limits->platform_fru_count = 1000;
            break;
        case SPID_DEFIANT_M1_HW_TYPE:
            env_limits->platform_hardware_limits    = defiant_m1_hw_limit;
            env_limits->platform_port_limits        = defiant_m1_port_limit;
            env_limits->platform_fru_count = 1000;
            break;
        case SPID_DEFIANT_M2_HW_TYPE:
            env_limits->platform_hardware_limits    = defiant_m2_hw_limit;
            env_limits->platform_port_limits        = defiant_m2_port_limit;
            env_limits->platform_fru_count = 750;
            break;
        case SPID_DEFIANT_M3_HW_TYPE:
            env_limits->platform_hardware_limits    = defiant_m3_hw_limit;
            env_limits->platform_port_limits        = defiant_m3_port_limit;
            env_limits->platform_fru_count = 500;
            break;
        case SPID_DEFIANT_M4_HW_TYPE:
            env_limits->platform_hardware_limits    = defiant_m4_hw_limit;
            env_limits->platform_port_limits        = defiant_m4_port_limit;
            env_limits->platform_fru_count = 250;
            break;
        case SPID_DEFIANT_M5_HW_TYPE:
            env_limits->platform_hardware_limits    = defiant_m5_hw_limit;
            env_limits->platform_port_limits        = defiant_m5_port_limit;
            env_limits->platform_fru_count = 125;
            break;
        case SPID_DEFIANT_S1_HW_TYPE:
            env_limits->platform_hardware_limits    = defiant_s1_hw_limit;
            env_limits->platform_port_limits        = defiant_s1_port_limit;
            env_limits->platform_fru_count = 500;
            break;
        case SPID_DEFIANT_S4_HW_TYPE:
            env_limits->platform_hardware_limits    = defiant_s4_hw_limit;
            env_limits->platform_port_limits        = defiant_s4_port_limit;
            env_limits->platform_fru_count = 250;
            break;
        case SPID_DEFIANT_K2_HW_TYPE:
            env_limits->platform_hardware_limits    = defiant_k2_hw_limit;
            env_limits->platform_port_limits        = defiant_k2_port_limit;
            env_limits->platform_fru_count = 750;
            break;
        case SPID_DEFIANT_K3_HW_TYPE:
            env_limits->platform_hardware_limits    = defiant_k3_hw_limit;
            env_limits->platform_port_limits        = defiant_k3_port_limit;
            env_limits->platform_fru_count = 500;
            break;
        case SPID_NOVA3_HW_TYPE:
            env_limits->platform_hardware_limits    = nova3_hw_limit;
            env_limits->platform_port_limits        = nova3_port_limit;
            env_limits->platform_fru_count = 250;
            break;
        case SPID_NOVA3_XM_HW_TYPE:
            env_limits->platform_hardware_limits    = nova3_xm_hw_limit;
            env_limits->platform_port_limits        = nova3_xm_port_limit;
            env_limits->platform_fru_count = 250;
            break;
        case SPID_NOVA1_HW_TYPE:
            env_limits->platform_hardware_limits    = nova_hw_limit;
            env_limits->platform_port_limits        = nova_port_limit;
            env_limits->platform_fru_count = 200;
            break;
        case SPID_NOVA_S1_HW_TYPE:
            env_limits->platform_hardware_limits    = nova_s1_hw_limit;
            env_limits->platform_port_limits        = nova_s1_port_limit;
            env_limits->platform_fru_count = 200;
            break;
        case SPID_BEARCAT_HW_TYPE:   
            env_limits->platform_hardware_limits    = bearcat_hw_limit;
            env_limits->platform_port_limits        = bearcat_port_limit;
            env_limits->platform_fru_count = 200;
            break;
        case SPID_OBERON_S1_HW_TYPE:
            env_limits->platform_hardware_limits    = oberon_s1_hw_limit;
            env_limits->platform_port_limits        = oberon_s1_port_limit;
            env_limits->platform_fru_count = 200;
            break;

        case SPID_OBERON_1_HW_TYPE:
            env_limits->platform_hardware_limits    = oberon_1_hw_limit;
            env_limits->platform_port_limits        = oberon_1_port_limit;
            break;
        case SPID_OBERON_2_HW_TYPE:
            env_limits->platform_hardware_limits    = oberon_2_hw_limit;
            env_limits->platform_port_limits        = oberon_2_port_limit;
            break;
        case SPID_OBERON_3_HW_TYPE:
            env_limits->platform_hardware_limits    = oberon_3_hw_limit;
            env_limits->platform_port_limits        = oberon_3_port_limit;
            break;
        case SPID_OBERON_4_HW_TYPE:
            env_limits->platform_hardware_limits    = oberon_4_hw_limit;
            env_limits->platform_port_limits        = oberon_4_port_limit;
            break;
        
        case SPID_HYPERION_1_HW_TYPE:
            env_limits->platform_hardware_limits    = hyperion_1_hw_limit;
            env_limits->platform_port_limits        = hyperion_1_port_limit;
            break;
        case SPID_HYPERION_2_HW_TYPE:
            env_limits->platform_hardware_limits    = hyperion_2_hw_limit;
            env_limits->platform_port_limits        = hyperion_2_port_limit;
            break;
        case SPID_HYPERION_3_HW_TYPE:
            env_limits->platform_hardware_limits    = hyperion_3_hw_limit;
            env_limits->platform_port_limits        = hyperion_3_port_limit;
            break;
        case SPID_TRITON_1_HW_TYPE:
            env_limits->platform_hardware_limits    = triton_1_hw_limit;
            env_limits->platform_port_limits        = triton_1_port_limit;
            break;
        /* VVNX Virtual; TODO: Revisit; Same as KH sim */
        case SPID_MERIDIAN_ESX_HW_TYPE:
        case SPID_TUNGSTEN_HW_TYPE:
        default: // making sim the default
            env_limits->platform_hardware_limits    = nova_s1_hw_limit;
            env_limits->platform_port_limits        = nova_s1_port_limit;
            env_limits->platform_fru_count = 50;
            break;
        }
      
        env_limits->platform_max_be_count = env_limits->platform_port_limits.max_sas_be;;
        env_limits->platform_max_encl_count_per_bus = PLATFORM_MAX_ENCL_COUNT_PER_BUS;
        env_limits->platform_max_lun_per_rg = PLATFORM_MAX_LUN_PER_RG;
        env_limits->platform_max_rg = 1000;//pCoreConfigRuntime_Global->RgUserCount;
        env_limits->platform_max_user_lun = 2048;//pCoreConfigRuntime_Global->PlatformHostAccessibleFlareLuns;               

        #if defined(SIMMODE_ENV)
            env_limits->platform_fru_count = 150;//pCoreConfigRuntime_Global -> PlatformFruCount;
            env_limits->platform_max_be_count = 8;//pCoreConfigRuntime_Global -> PlatformMaxBeCount;
            env_limits->platform_max_lun_per_rg = PLATFORM_MAX_LUN_PER_RG;
            env_limits->platform_max_encl_count_per_bus = PLATFORM_MAX_ENCL_COUNT_PER_BUS;
            env_limits->platform_max_rg = 150;//pCoreConfigRuntime_Global->RgUserCount;
            env_limits->platform_max_user_lun = 500;//pCoreConfigRuntime_Global->PlatformHostAccessibleFlareLuns;       
        #endif
               
    }
    return FBE_STATUS_OK;
}
/****************************************************************************
    end of fbe_environment_limit_init_limits()     
*****************************************************************************/

fbe_status_t fbe_environment_limit_clear_limits()
{
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_environment_limit_get_platform_type(fbe_object_id_t board_id, SPID_HW_TYPE *platform_type)
{
	fbe_status_t                  		status = FBE_STATUS_GENERIC_FAILURE;
	fbe_board_mgmt_platform_info_t 		platform_info;
    fbe_packet_t *                  	packet_p = NULL;
    fbe_payload_ex_t *             		payload = NULL;
    fbe_payload_control_operation_t *	control_operation = NULL;
    fbe_payload_control_status_t    	payload_control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    
    /* The memory service may not be available yet */
    packet_p = fbe_allocate_nonpaged_pool_with_tag ( sizeof (fbe_packet_t), 'pfbe');

    if (packet_p == NULL) {
        fbe_environment_limit_trace (FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s:can't allocate packet\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_transport_initialize_packet(packet_p);
    payload = fbe_transport_get_payload_ex (packet_p);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    fbe_payload_control_build_operation (control_operation,
                                         FBE_BASE_BOARD_CONTROL_CODE_GET_PLATFORM_INFO,
                                         &platform_info,
                                         sizeof(fbe_board_mgmt_platform_info_t));

    /* Set packet address.*/
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_PHYSICAL,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              board_id); 

    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    /* Control packets should be sent via service manager. */
    status = fbe_service_manager_send_control_packet(packet_p);
    if (status != FBE_STATUS_OK){
            fbe_environment_limit_trace (FBE_TRACE_LEVEL_ERROR,
										 FBE_TRACE_MESSAGE_ID_INFO,
										 "%s:failed to send control packet\n",__FUNCTION__);
        
    }

    fbe_transport_wait_completion(packet_p);

    fbe_payload_control_get_status(control_operation, &payload_control_status);
    if(payload_control_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_environment_limit_trace (FBE_TRACE_LEVEL_ERROR,
									 FBE_TRACE_MESSAGE_ID_INFO,
									 "%s:bad return status\n",__FUNCTION__);
    }

    status  = fbe_transport_get_status_code(packet_p);
    if(status != FBE_STATUS_OK){
        fbe_environment_limit_trace (FBE_TRACE_LEVEL_ERROR,
									 FBE_TRACE_MESSAGE_ID_INFO,
									 "%s:bad return status\n",__FUNCTION__);
    }

    fbe_payload_ex_release_control_operation(payload, control_operation);
    fbe_transport_destroy_packet(packet_p);
    fbe_release_nonpaged_pool(packet_p);

	*platform_type = platform_info.hw_platform_info.platformType;

    return status;
}

fbe_status_t fbe_environment_limit_get_initiated_limits(fbe_environment_limits_t *env_limits)
{
    fbe_status_t status;
    status = fbe_environment_limit_init_limits(env_limits);
    return status;
}

 //end fbe_environment_limit_user_main.c

