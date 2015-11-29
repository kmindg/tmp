/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_module_mgmt_mapping.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Module Management object relationship mapping.
 * 
 ***************************************************************************/
#include "fbe/fbe_module_mgmt_mapping.h"

fbe_ffid_property_t fbe_ffid_property_map[] = 
{
    {TOMAHAWK_C6                         , FBE_SLIC_TYPE_FC_4G             , FBE_IO_MODULE_PROTOCOL_FIBRE,  "4 Port FC 4G"}   , //Obsolete
    {TOMAHAWK                            , FBE_SLIC_TYPE_FC_4G             , FBE_IO_MODULE_PROTOCOL_FIBRE,  "4 Port FC 4G"}   , //Obsolete
    {GLACIER                             , FBE_SLIC_TYPE_FC_8G             , FBE_IO_MODULE_PROTOCOL_FIBRE,  "4 Port FC 8G"}   ,
    {GLACIER_REV_C                       , FBE_SLIC_TYPE_FC_8G             , FBE_IO_MODULE_PROTOCOL_FIBRE,  "4 Port FC 8G"}   ,
    {EGLACIER                            , FBE_SLIC_TYPE_FC_8G             , FBE_IO_MODULE_PROTOCOL_FIBRE,  "4 Port FC 8G"}   ,
    {EGLACIER_4S                         , FBE_SLIC_TYPE_FC_8G_4S          , FBE_IO_MODULE_PROTOCOL_FIBRE,  "4 Port FC 8G"}   ,
    {EGLACIER_1S3M                       , FBE_SLIC_TYPE_FC_8G_1S3M        , FBE_IO_MODULE_PROTOCOL_FIBRE,  "4 Port FC 8G"}   ,
    {VORTEX16                            , FBE_SLIC_TYPE_FC_16G            , FBE_IO_MODULE_PROTOCOL_FIBRE,  "2 Port FC 16G"}   , //Obsolete
    {VORTEX16Q                           , FBE_SLIC_TYPE_FC_16G            , FBE_IO_MODULE_PROTOCOL_FIBRE,  "16G Fibre/FICON v3"}   ,

    {SUPERCELL                           , FBE_SLIC_TYPE_4PORT_ISCSI_1G    , FBE_IO_MODULE_PROTOCOL_ISCSI,  "4 Port iSCSI 1G"}   ,
    {ESUPERCELL                          , FBE_SLIC_TYPE_4PORT_ISCSI_1G    , FBE_IO_MODULE_PROTOCOL_ISCSI,  "4 Port iSCSI 1G"}   ,
    {ELANDSLIDE                          , FBE_SLIC_TYPE_4PORT_ISCSI_10G   , FBE_IO_MODULE_PROTOCOL_ISCSI,  "4 Port iSCSI 10G"}   ,
    {ERUPTION_COPPER_2PORT_REV_B         , FBE_SLIC_TYPE_ISCSI_COPPER      , FBE_IO_MODULE_PROTOCOL_ISCSI,  "2 Port iSCSI 10G Copper"}   , //Obsolete
    {ERUPTION_2PORT_REV_C_84823          , FBE_SLIC_TYPE_ISCSI_COPPER      , FBE_IO_MODULE_PROTOCOL_ISCSI,  "2 Port iSCSI 10G Copper"}   , //Obsolete
    {ERUPTION_2PORT_REV_C_84833          , FBE_SLIC_TYPE_ISCSI_COPPER      , FBE_IO_MODULE_PROTOCOL_ISCSI,  "2 Port iSCSI 10G Copper"}   , //Obsolete
    {ERUPTION_REV_D                      , FBE_SLIC_TYPE_ISCSI_COPPER      , FBE_IO_MODULE_PROTOCOL_ISCSI,  "2 Port iSCSI 10G Copper"}   ,
    {EERUPTION                           , FBE_SLIC_TYPE_ISCSI_COPPER      , FBE_IO_MODULE_PROTOCOL_ISCSI,  "2 Port iSCSI 10G Copper"}   , //Obsolete
    {EERUPTION_REV_B                     , FBE_SLIC_TYPE_ISCSI_COPPER      , FBE_IO_MODULE_PROTOCOL_ISCSI,  "2 Port iSCSI 10G Copper"}   ,
    {EERUPTION_10GB_W_BCM84833           , FBE_SLIC_TYPE_ISCSI_COPPER      , FBE_IO_MODULE_PROTOCOL_ISCSI,  "2 Port iSCSI 10G Copper"}   , //Obsolete
    {ELNINO                              , FBE_SLIC_TYPE_ISCSI_10G_V2      , FBE_IO_MODULE_PROTOCOL_ISCSI,  "2 Port iSCSI 10G Optical"}   , //Obsolete
    {ELNINO_REV_B                        , FBE_SLIC_TYPE_ISCSI_10G_V2      , FBE_IO_MODULE_PROTOCOL_ISCSI,  "2 Port iSCSI 10G Optical"}   ,
    {HARPOON                             , FBE_SLIC_TYPE_UNSUPPORTED       , FBE_IO_MODULE_PROTOCOL_ISCSI,  "2 Port iSCSI 1G"}   , //Obsolete

    {HEATWAVE                            , FBE_SLIC_TYPE_FCOE              , FBE_IO_MODULE_PROTOCOL_FCOE,   "2 Port FCoE"}    ,
    {MAELSTROM                           , FBE_SLIC_TYPE_FCOE              , FBE_IO_MODULE_PROTOCOL_FCOE,   "10GbE V6"}    , 

    {COROMANDEL                          , FBE_SLIC_TYPE_SAS               , FBE_IO_MODULE_PROTOCOL_SAS,    "SAS"}     , //Obsolete
    {HYPERNOVA                           , FBE_SLIC_TYPE_6G_SAS_1          , FBE_IO_MODULE_PROTOCOL_SAS,    "6Gb SAS V1"}     , //Obsolete
    {COLDFRONT                           , FBE_SLIC_TYPE_6G_SAS_2          , FBE_IO_MODULE_PROTOCOL_SAS,    "6Gb SAS V2"}     , //Obsolete
    {MOONSHADOW                          , FBE_SLIC_TYPE_6G_SAS_3          , FBE_IO_MODULE_PROTOCOL_SAS,    "6Gb SAS v3"}     , //Unsupported
    {MOONQUAKE                           , FBE_SLIC_TYPE_6G_SAS_3          , FBE_IO_MODULE_PROTOCOL_SAS,    "6Gb SAS v3"}     , //Unsupported
    {MOONSHINE                           , FBE_SLIC_TYPE_6G_SAS_3          , FBE_IO_MODULE_PROTOCOL_SAS,    "6Gb SAS v3"}     , //Unsupported
    {MOONLITE                            , FBE_SLIC_TYPE_6G_SAS_3          , FBE_IO_MODULE_PROTOCOL_SAS,    "6Gb SAS v3"}     ,
    {SNOWDEVIL                           , FBE_SLIC_TYPE_12G_SAS           , FBE_IO_MODULE_PROTOCOL_SAS,    "12Gb SAS v1"}     ,
    {DUSTDEVIL                           , FBE_SLIC_TYPE_12G_SAS           , FBE_IO_MODULE_PROTOCOL_SAS,    "12Gb SAS v1"}     , //Unsupported

    {GROWLER                             , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "Mezzanine"}   , //Obsolete
    {GROWLER_DVT                         , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "Mezzanine"}   , //Obsolete

    {PEACEMAKER_WITH_SAS                 , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "Mezzanine"}   , //Obsolete
    {PHANTOM                             , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "Mezzanine"}   , //Obsolete
    {MUZZLE_WITH_SAS                     , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "Mezzanine"}   , //Obsolete

    {IRONHIDE_6G_SAS                     , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "6Gb SAS Base Module"}   ,
    {IRONHIDE_6G_SAS_REV_B               , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "6Gb SAS Base Module"}   ,
    {IRONHIDE_6G_SAS_REV_C               , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "6Gb SAS Base Module"}   ,

    {LAPETUS_12G_SAS_REV_A               , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "12Gb SAS Base Module"}   ,
    {LAPETUS_12G_SAS_REV_B               , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "12Gb SAS Base Module"}   ,

    {BEACHCOMBER_PCB                     , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "Onboard"}   ,
    {BEACHCOMBER_PCB_REV_B               , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "Onboard"}   ,
    {BEACHCOMBER_PCB_REV_B_CPU_6_CORE    , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "Onboard"}   ,
    {BEACHCOMBER_PCB_REV_B_BCM57840_16GB , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "Onboard"}   ,
    {BEACHCOMBER_PCB_REV_B_BCM57840_24GB , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "Onboard"}   ,
    {BEACHCOMBER_PCB_REV_C_4C            , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "Onboard"}   ,
    {BEACHCOMBER_PCB_REV_C_6C            , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "Onboard"}   ,
    {BEACHCOMBER_PCB_REV_D_4C            , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "Onboard"}   ,
    {BEACHCOMBER_PCB_REV_D_6C            , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "Onboard"}   ,
    {BEACHCOMBER_PCB_SIM                 , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "Onboard"}   ,

    {BEACHCOMBER_PCB_FC_2C               , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "Onboard"}   ,
    {OBERON_SP_85W_REV_A                 , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "Onboard"}   ,
    {OBERON_SP_105W_REV_A                , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "Onboard"}   ,
    {OBERON_SP_120W_REV_A                , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "Onboard"}   ,
    {OBERON_SP_E5_2603V3_REV_B           , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "Onboard"}   ,
    {OBERON_SP_E5_2630V3_REV_B           , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "Onboard"}   ,
    {OBERON_SP_E5_2609V3_REV_B           , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "Onboard"}   ,
    {OBERON_SP_E5_2660V3_REV_B           , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "Onboard"}   ,

    {MERIDIAN_SP_ESX                     , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "Onboard"}   ,
    {TUNGSTEN_SP                         , FBE_SLIC_TYPE_NA                , FBE_IO_MODULE_PROTOCOL_MULTI,  "Onboard"}   ,

    {THUNDERHEAD_X                       , FBE_SLIC_TYPE_V2_4PORT_ISCSI_1G , FBE_IO_MODULE_PROTOCOL_ISCSI,  "1GbE BaseT v3"}   ,
    {ROCKSLIDE_X                         , FBE_SLIC_TYPE_V2_4PORT_ISCSI_10G, FBE_IO_MODULE_PROTOCOL_ISCSI,  "10GbE BaseT v2"}   ,

    {DOWNBURST_X_RDMA                    , FBE_SLIC_TYPE_V3_4PORT_ISCSI_10G, FBE_IO_MODULE_PROTOCOL_ISCSI,  "10GbE v5"}   ,
    {MAELSTROM_X                         , FBE_SLIC_TYPE_V3_2PORT_ISCSI_10G, FBE_IO_MODULE_PROTOCOL_FCOE,   "10GbE V6"}    ,
    {VORTEXQ_X                           , FBE_SLIC_TYPE_FC_16G            , FBE_IO_MODULE_PROTOCOL_FIBRE,  "16Gb Fibre/FICON V3"}   , 
    {SNOWDEVIL_X                         , FBE_SLIC_TYPE_12G_SAS           , FBE_IO_MODULE_PROTOCOL_SAS,    "12Gb SAS V1 (e)"}     ,
    {DUSTDEVIL_X                         , FBE_SLIC_TYPE_12G_SAS           , FBE_IO_MODULE_PROTOCOL_SAS,    "12Gb SAS (e)"}     , // maybe? V2? //Unsupported
    {NO_MODULE                           , FBE_SLIC_TYPE_UNKNOWN           , FBE_IO_MODULE_PROTOCOL_UNKNOWN, "Unknown"}
};

fbe_u32_t fbe_ffid_property_map_size = sizeof(fbe_ffid_property_map) / sizeof(fbe_ffid_property_t);

fbe_slic_type_property_t fbe_slic_type_property_map[] = 
{
    {FBE_SLIC_TYPE_FC_4G             , "FC 4G"                            , FBE_IOM_GROUP_A       , FBE_PORT_ROLE_FE}               ,
    {FBE_SLIC_TYPE_ISCSI_1G          , "2Port iSCSI 1G"                   , FBE_IOM_GROUP_B       , FBE_PORT_ROLE_FE}               ,
    {FBE_SLIC_TYPE_6G_SAS_1          , "6G SAS v.1"                       , FBE_IOM_GROUP_C       , FBE_PORT_ROLE_BE}               ,
    {FBE_SLIC_TYPE_SAS               , "6G SAS"                           , FBE_IOM_GROUP_C       , FBE_PORT_ROLE_BE}               ,
    {FBE_SLIC_TYPE_FC_8G             , "FC 8G"                            , FBE_IOM_GROUP_D       , FBE_PORT_ROLE_FE}               ,
    {FBE_SLIC_TYPE_FC_8G_4S          , "FC 8G 4 Single Mode"              , FBE_IOM_GROUP_D       , FBE_PORT_ROLE_FE}               ,
    {FBE_SLIC_TYPE_FC_8G_1S3M        , "FC 8G 1 Single Mode 3 Multi Mode" , FBE_IOM_GROUP_D       , FBE_PORT_ROLE_FE}               ,
    {FBE_SLIC_TYPE_ISCSI_10G         , "iSCSI 10G"                        , FBE_IOM_GROUP_E       , FBE_PORT_ROLE_FE}               ,
    {FBE_SLIC_TYPE_FCOE              , "FCoE"                             , FBE_IOM_GROUP_F       , FBE_PORT_ROLE_FE}               ,
    {FBE_SLIC_TYPE_4PORT_ISCSI_1G    , "4Port iSCSI 1G"                   , FBE_IOM_GROUP_G       , FBE_PORT_ROLE_FE}               ,
    {FBE_SLIC_TYPE_6G_SAS_2          , "6G SAS v.2"                       , FBE_IOM_GROUP_H       , FBE_PORT_ROLE_BE}               ,
    {FBE_SLIC_TYPE_ISCSI_COPPER      , "iSCSI 10G Copper"                 , FBE_IOM_GROUP_I       , FBE_PORT_ROLE_FE}               ,
    {FBE_SLIC_TYPE_ISCSI_10G_V2      , "iSCSI 10G v.2"                    , FBE_IOM_GROUP_J       , FBE_PORT_ROLE_FE}               ,
    {FBE_SLIC_TYPE_6G_SAS_3          , "6G SAS v.3"                       , FBE_IOM_GROUP_K       , FBE_PORT_ROLE_BE}               ,
    {FBE_SLIC_TYPE_4PORT_ISCSI_10G   , "4Port iSCSI 10G"                  , FBE_IOM_GROUP_L       , FBE_PORT_ROLE_FE}               ,
    {FBE_SLIC_TYPE_12G_SAS           , "12G SAS"                          , FBE_IOM_GROUP_N       , FBE_PORT_ROLE_BE}               ,
    {FBE_SLIC_TYPE_V2_4PORT_ISCSI_1G , "SLIC 2.0 4Port iSCSI 1G"          , FBE_IOM_GROUP_O       , FBE_PORT_ROLE_FE}               ,
    {FBE_SLIC_TYPE_V2_4PORT_ISCSI_10G, "SLIC 2.0 4Port iSCSI 10G"         , FBE_IOM_GROUP_Q       , FBE_PORT_ROLE_FE}               ,
    {FBE_SLIC_TYPE_FC_16G            , "FC 16G"                           , FBE_IOM_GROUP_M       , FBE_PORT_ROLE_FE}               ,
    {FBE_SLIC_TYPE_V3_2PORT_ISCSI_10G, "SLIC 2Port iSCSI 10G Gen 3"       , FBE_IOM_GROUP_R       , FBE_PORT_ROLE_FE}               ,
    {FBE_SLIC_TYPE_V3_4PORT_ISCSI_10G, "SLIC 4Port iSCSI 10G Gen 3"       , FBE_IOM_GROUP_S       , FBE_PORT_ROLE_FE}               ,
    {FBE_SLIC_TYPE_NA                , "NA"                               , FBE_IOM_GROUP_UNKNOWN , FBE_PORT_ROLE_INVALID} , // Need additional logic for this type
    {FBE_SLIC_TYPE_UNKNOWN           , "Unknown"                          , FBE_IOM_GROUP_UNKNOWN , FBE_PORT_ROLE_INVALID} ,
    {FBE_SLIC_TYPE_UNSUPPORTED       , "Unsupported"                      , FBE_IOM_GROUP_UNKNOWN , FBE_PORT_ROLE_INVALID}
};

fbe_u32_t fbe_slic_type_property_map_size = sizeof(fbe_slic_type_property_map) / sizeof(fbe_slic_type_property_t);

fbe_iom_group_property_t fbe_iom_group_property_map[] =
{
    {FBE_IOM_GROUP_B , ISCSI_CPDISQL_STRING} ,
    {FBE_IOM_GROUP_C , SAS_STRING}           ,
    {FBE_IOM_GROUP_D , FC_STRING}            ,
    {FBE_IOM_GROUP_E , ISCSI_GENERAL_STRING} ,
    {FBE_IOM_GROUP_F , FCOE_STRING}          ,
    {FBE_IOM_GROUP_G , ISCSI_GENERAL_STRING} ,
    {FBE_IOM_GROUP_H , SAS_FE_STRING}        ,
    {FBE_IOM_GROUP_I , ISCSI_GENERAL_STRING} ,
    {FBE_IOM_GROUP_J , ISCSI_GENERAL_STRING} ,
    {FBE_IOM_GROUP_K , SAS_STRING}           ,
    {FBE_IOM_GROUP_L , ISCSI_GENERAL_STRING} ,
    {FBE_IOM_GROUP_M , FC_16G_STRING}           ,
    {FBE_IOM_GROUP_N , SAS_STRING} ,
    {FBE_IOM_GROUP_O , ISCSI_GENERAL_STRING} ,
    {FBE_IOM_GROUP_P , ISCSI_QL_STRING}        ,
    {FBE_IOM_GROUP_Q , ISCSI_GENERAL_STRING} ,
    {FBE_IOM_GROUP_R , ISCSI_QL_STRING} ,
    {FBE_IOM_GROUP_S , ISCSI_GENERAL_STRING}
};

fbe_u32_t fbe_iom_group_property_map_size = sizeof(fbe_iom_group_property_map) / sizeof(fbe_iom_group_property_t);

fbe_driver_property_t fbe_driver_property_map[] =
{
    {FC_STRING            , FBE_IOM_GROUP_D} ,
    {ISCSI_CPDISQL_STRING , FBE_IOM_GROUP_B} ,
    {ISCSI_GENERAL_STRING , FBE_IOM_GROUP_E} ,
    {SAS_STRING           , FBE_IOM_GROUP_C} ,
    {FCOE_STRING          , FBE_IOM_GROUP_F} ,
    {SAS_FE_STRING        , FBE_IOM_GROUP_H} ,
    {FC_16G_STRING        , FBE_IOM_GROUP_M} ,
    {ISCSI_QL_STRING      , FBE_IOM_GROUP_P}
};

fbe_u32_t fbe_driver_property_map_size = sizeof(fbe_driver_property_map) / sizeof(fbe_driver_property_t);

