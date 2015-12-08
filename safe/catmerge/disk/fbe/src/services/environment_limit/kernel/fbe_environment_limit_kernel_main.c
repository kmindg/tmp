/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

 /*!**************************************************************************
 * @file fbe_environment_limit_kernel_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains kernel specifc functions for environment
 *  limit service.
 * 
 * @ingroup environment_limit_service_files
 * 
 * @version
 *   19-August-2010:  Created. Vishnu Sharma
 *
 ***************************************************************************/
 
 #include "fbe_environment_limit_private.h"
 #include "core_config_runtime.h"
 extern BOOL ccrGetPlatformType(SPID_HW_TYPE *pPlatformType);

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
            FBE_SLIC_TYPE_FC_8G|FBE_SLIC_TYPE_FC_8G_4S|FBE_SLIC_TYPE_FC_8G_1S3M|
            FBE_SLIC_TYPE_ISCSI_10G|FBE_SLIC_TYPE_FCOE|FBE_SLIC_TYPE_4PORT_ISCSI_1G|FBE_SLIC_TYPE_6G_SAS_1|  
            FBE_SLIC_TYPE_ISCSI_COPPER|FBE_SLIC_TYPE_ISCSI_10G_V2|FBE_SLIC_TYPE_6G_SAS_3|FBE_SLIC_TYPE_4PORT_ISCSI_10G|FBE_SLIC_TYPE_NA};
static fbe_environment_limits_platform_port_limits_t        bearcat_port_limit          = {2,0,6,6,6,0,6};

static fbe_environment_limits_platform_hardware_limits_t    oberon_s1_hw_limit      = {2,1,
            FBE_SLIC_TYPE_V2_4PORT_ISCSI_1G  |
            FBE_SLIC_TYPE_V2_4PORT_ISCSI_10G |
            FBE_SLIC_TYPE_FC_16G             |
            FBE_SLIC_TYPE_V3_2PORT_ISCSI_10G |
            FBE_SLIC_TYPE_V3_4PORT_ISCSI_10G |
            FBE_SLIC_TYPE_NA};
static fbe_environment_limits_platform_port_limits_t        oberon_s1_port_limit    = {2,0,10,12,12,0};

static fbe_environment_limits_platform_hardware_limits_t    triton_1_hw_limit      = {11,0,
            FBE_SLIC_TYPE_FC_8G|FBE_SLIC_TYPE_ISCSI_10G|FBE_SLIC_TYPE_FCOE|FBE_SLIC_TYPE_4PORT_ISCSI_1G|FBE_SLIC_TYPE_6G_SAS_1|
            FBE_SLIC_TYPE_ISCSI_COPPER|FBE_SLIC_TYPE_ISCSI_10G_V2|FBE_SLIC_TYPE_6G_SAS_3};
static fbe_environment_limits_platform_port_limits_t        triton_1_port_limit    = {16,0,36,8,8,18,0};


/*!****************************************************************************
 * @fn fbe_initialize_environment_limit_service()
 ******************************************************************************
 *
 * @brief
 *      This function initializes the environment limit service for
        kernel mode.
 *
 * @return fbe_status_t
 *
 * @author
 *      27-August-2010    Vishnu Sharma  Created.
 *
 ******************************************************************************/

fbe_status_t 
fbe_environment_limit_init_limits(fbe_environment_limits_t *env_limits)
{
    SPID_HW_TYPE platformType;
    if(ccrInit())
    {
        fbe_environment_limit_trace(FBE_TRACE_LEVEL_INFO,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "The Core Config Runtime library initialized successfully\n");

        env_limits->platform_fru_count = 
                    pCoreConfigRuntime_Global -> PlatformFruCount;
        
        env_limits->platform_max_encl_count_per_bus = PLATFORM_MAX_ENCL_COUNT_PER_BUS;

        env_limits->platform_max_lun_per_rg = PLATFORM_MAX_LUN_PER_RG; 
        env_limits->platform_max_rg = pCoreConfigRuntime_Global->RgUserCount;
        env_limits->platform_max_user_lun = pCoreConfigRuntime_Global->PlatformHostAccessibleFlareLuns;

        if (ccrGetPlatformType(&platformType))
        {

            switch(platformType)
            {
            case SPID_ENTERPRISE_HW_TYPE:
                env_limits->platform_hardware_limits    = enterprise_hw_limit;
                env_limits->platform_port_limits        = enterprise_port_limit;
                break;
            case SPID_PROMETHEUS_M1_HW_TYPE:
                env_limits->platform_hardware_limits    = prometheus_m1_hw_limit;
                env_limits->platform_port_limits        = prometheus_m1_port_limit;
                break;
            case SPID_PROMETHEUS_S1_HW_TYPE:
                env_limits->platform_hardware_limits    = prometheus_s1_hw_limit;
                env_limits->platform_port_limits        = prometheus_s1_port_limit;
                break;
            case SPID_DEFIANT_M1_HW_TYPE:
                env_limits->platform_hardware_limits    = defiant_m1_hw_limit;
                env_limits->platform_port_limits        = defiant_m1_port_limit;
                break;
            case SPID_DEFIANT_M2_HW_TYPE:
                env_limits->platform_hardware_limits    = defiant_m2_hw_limit;
                env_limits->platform_port_limits        = defiant_m2_port_limit;
                break;
            case SPID_DEFIANT_M3_HW_TYPE:
                env_limits->platform_hardware_limits    = defiant_m3_hw_limit;
                env_limits->platform_port_limits        = defiant_m3_port_limit;
                break;
            case SPID_DEFIANT_M4_HW_TYPE:
                env_limits->platform_hardware_limits    = defiant_m4_hw_limit;
                env_limits->platform_port_limits        = defiant_m4_port_limit;
                break;
            case SPID_DEFIANT_M5_HW_TYPE:
                env_limits->platform_hardware_limits    = defiant_m5_hw_limit;
                env_limits->platform_port_limits        = defiant_m5_port_limit;
                break;
            case SPID_DEFIANT_S1_HW_TYPE:
                env_limits->platform_hardware_limits    = defiant_s1_hw_limit;
                env_limits->platform_port_limits        = defiant_s1_port_limit;
                break;
            case SPID_DEFIANT_S4_HW_TYPE:
                env_limits->platform_hardware_limits    = defiant_s4_hw_limit;
                env_limits->platform_port_limits        = defiant_s4_port_limit;
                break;
            case SPID_DEFIANT_K2_HW_TYPE:
                env_limits->platform_hardware_limits    = defiant_k2_hw_limit;
                env_limits->platform_port_limits        = defiant_k2_port_limit;
                break;
            case SPID_DEFIANT_K3_HW_TYPE:
                env_limits->platform_hardware_limits    = defiant_k3_hw_limit;
                env_limits->platform_port_limits        = defiant_k3_port_limit;
                break;
            case SPID_NOVA1_HW_TYPE:
                env_limits->platform_hardware_limits    = nova_hw_limit;
                env_limits->platform_port_limits        = nova_port_limit;
                break;
            case SPID_NOVA3_HW_TYPE:
                env_limits->platform_hardware_limits    = nova3_hw_limit;
                env_limits->platform_port_limits        = nova3_port_limit;
                break;
            case SPID_NOVA3_XM_HW_TYPE:
                env_limits->platform_hardware_limits    = nova3_xm_hw_limit;
                env_limits->platform_port_limits        = nova3_xm_port_limit;
                break;
            case SPID_NOVA_S1_HW_TYPE:
                env_limits->platform_hardware_limits    = nova_s1_hw_limit;
                env_limits->platform_port_limits        = nova_s1_port_limit;
                break;
            case SPID_BEARCAT_HW_TYPE:
                env_limits->platform_hardware_limits    = bearcat_hw_limit;
                env_limits->platform_port_limits        = bearcat_port_limit;
                break;
            case SPID_OBERON_S1_HW_TYPE:
                env_limits->platform_hardware_limits    = oberon_s1_hw_limit;
                env_limits->platform_port_limits        = oberon_s1_port_limit;
                break;
            case SPID_OBERON_1_HW_TYPE:
            case SPID_OBERON_2_HW_TYPE:
            case SPID_OBERON_3_HW_TYPE:
            case SPID_OBERON_4_HW_TYPE:
            case SPID_HYPERION_1_HW_TYPE:
            case SPID_HYPERION_2_HW_TYPE:
            case SPID_HYPERION_3_HW_TYPE:
                //Fill in Environment Limits from Global Config Runtime
                env_limits->platform_max_encl_count_per_bus            = CoreConfigRuntime_Global.HwAttributes.PlatformMaxEnclPerBus;
                env_limits->platform_max_lun_per_rg                    = CoreConfigRuntime_Global.HwAttributes.PlatformMaxLunsPerRG; 
                env_limits->platform_hardware_limits.max_slics         = CoreConfigRuntime_Global.HwAttributes.PlatformMaxSlics;
                env_limits->platform_hardware_limits.max_mezzanines    = CoreConfigRuntime_Global.HwAttributes.PlatformMaxMezzanines;
                env_limits->platform_port_limits.max_sas_be            = CoreConfigRuntime_Global.PortLimits.PlatformMaxSasBeCount;
                env_limits->platform_port_limits.max_sas_fe            = CoreConfigRuntime_Global.PortLimits.PlatformMaxSasFeCount;
                env_limits->platform_port_limits.max_fc_fe             = CoreConfigRuntime_Global.PortLimits.PlatformMaxFcFeCount;
                env_limits->platform_port_limits.max_iscsi_1g_fe    = CoreConfigRuntime_Global.PortLimits.PlatformMaxiSCSI1GFeCount;
                env_limits->platform_port_limits.max_iscsi_10g_fe   = CoreConfigRuntime_Global.PortLimits.PlatformMaxiSCSI10GFeCount;
                env_limits->platform_port_limits.max_fcoe_fe        = CoreConfigRuntime_Global.PortLimits.PlatformMaxFcoeFeCount;
                env_limits->platform_port_limits.max_combined_iscsi_fe = CoreConfigRuntime_Global.PortLimits.PlatformMaxCombinediSCSIFeCount;


                if (platformType == SPID_OBERON_1_HW_TYPE) {
                    //12G SAS not supported on Oberon 1
                    env_limits->platform_hardware_limits.supported_slic_types = (FBE_SLIC_TYPE_V2_4PORT_ISCSI_1G|
                                                                                 FBE_SLIC_TYPE_V2_4PORT_ISCSI_10G|
                                                                                 FBE_SLIC_TYPE_V3_2PORT_ISCSI_10G|
                                                                                 FBE_SLIC_TYPE_V3_4PORT_ISCSI_10G|
                                                                                 FBE_SLIC_TYPE_ISCSI_10G_V2|
                                                                                 FBE_SLIC_TYPE_FC_16G|
                                                                                 FBE_SLIC_TYPE_FCOE|
                                                                                 FBE_SLIC_TYPE_NA);
                }
                else
                {
                    env_limits->platform_hardware_limits.supported_slic_types = (FBE_SLIC_TYPE_12G_SAS|
                                                                                 FBE_SLIC_TYPE_V2_4PORT_ISCSI_1G|
                                                                                 FBE_SLIC_TYPE_V2_4PORT_ISCSI_10G|
                                                                                 FBE_SLIC_TYPE_V3_2PORT_ISCSI_10G|
                                                                                 FBE_SLIC_TYPE_V3_4PORT_ISCSI_10G|
                                                                                 FBE_SLIC_TYPE_ISCSI_10G_V2|
                                                                                 FBE_SLIC_TYPE_FC_16G|
                                                                                 FBE_SLIC_TYPE_FCOE|
                                                                                 FBE_SLIC_TYPE_NA);
                }
                break;

            case SPID_TRITON_1_HW_TYPE:
                env_limits->platform_hardware_limits    = triton_1_hw_limit;
                env_limits->platform_port_limits        = triton_1_port_limit;
                break;


            /* VVNX Virtual; TODO: Revisit; Same as KH sim */
            case SPID_MERIDIAN_ESX_HW_TYPE:
            case SPID_TUNGSTEN_HW_TYPE:
                //Fill in Environment Limits from Global Config Runtime
                env_limits->platform_max_encl_count_per_bus            = CoreConfigRuntime_Global.HwAttributes.PlatformMaxEnclPerBus;
                env_limits->platform_max_lun_per_rg                    = CoreConfigRuntime_Global.HwAttributes.PlatformMaxLunsPerRG; 
                env_limits->platform_hardware_limits.max_slics         = CoreConfigRuntime_Global.HwAttributes.PlatformMaxSlics;
                env_limits->platform_hardware_limits.max_mezzanines    = CoreConfigRuntime_Global.HwAttributes.PlatformMaxMezzanines;
                env_limits->platform_port_limits.max_sas_be            = CoreConfigRuntime_Global.PortLimits.PlatformMaxSasBeCount;
                env_limits->platform_port_limits.max_sas_fe            = CoreConfigRuntime_Global.PortLimits.PlatformMaxSasFeCount;
                env_limits->platform_port_limits.max_fc_fe             = CoreConfigRuntime_Global.PortLimits.PlatformMaxFcFeCount;
                env_limits->platform_port_limits.max_iscsi_1g_fe    = CoreConfigRuntime_Global.PortLimits.PlatformMaxiSCSI1GFeCount;
                env_limits->platform_port_limits.max_iscsi_10g_fe   = CoreConfigRuntime_Global.PortLimits.PlatformMaxiSCSI10GFeCount;
                env_limits->platform_port_limits.max_fcoe_fe        = CoreConfigRuntime_Global.PortLimits.PlatformMaxFcoeFeCount;
                env_limits->platform_port_limits.max_combined_iscsi_fe = CoreConfigRuntime_Global.PortLimits.PlatformMaxCombinediSCSIFeCount;

                env_limits->platform_hardware_limits.supported_slic_types = (FBE_SLIC_TYPE_FC_8G |
									     FBE_SLIC_TYPE_FC_8G_4S |
									     FBE_SLIC_TYPE_FC_8G_1S3M |
            								     FBE_SLIC_TYPE_ISCSI_10G |
                                                                             FBE_SLIC_TYPE_FCOE |
									     FBE_SLIC_TYPE_4PORT_ISCSI_1G |
									     FBE_SLIC_TYPE_6G_SAS_1 |
            								     FBE_SLIC_TYPE_ISCSI_COPPER |
									     FBE_SLIC_TYPE_ISCSI_10G_V2 |
									     FBE_SLIC_TYPE_6G_SAS_3 |
									     FBE_SLIC_TYPE_4PORT_ISCSI_10G |
                                                                             FBE_SLIC_TYPE_NA);

                break;


            default: // making sim the default
                env_limits->platform_hardware_limits    = nova_s1_hw_limit;
                env_limits->platform_port_limits        = nova_s1_port_limit;
                break;
            }
        }

        env_limits->platform_max_be_count = 
                    env_limits->platform_port_limits.max_sas_be;

        return FBE_STATUS_OK;
    }
    else
    {
        fbe_environment_limit_trace(FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "\nThe Core Config Runtime library could not be initialized\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
}
/****************************************************************************
    end of fbe_environment_limit_init_limits()     
*****************************************************************************/
fbe_status_t fbe_environment_limit_clear_limits(void)
{
    return FBE_STATUS_OK;
}

fbe_status_t fbe_environment_limit_get_initiated_limits(fbe_environment_limits_t *env_limits)
{
    fbe_status_t status;
    status = fbe_environment_limit_init_limits(env_limits);
    return status;
}
//fbe_environment_limit_kernel_main.c
