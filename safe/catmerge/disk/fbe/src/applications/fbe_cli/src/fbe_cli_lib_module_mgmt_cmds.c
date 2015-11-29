/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_module_mgmt_cmds.c
 ***************************************************************************
 *
 * @brief
 *  This file contains FBE cli functions for module management.
 *
 * @ingroup fbe_cli
 *
 * @revision
 *  01/17/2011:  Rashmi Sawale - created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
#include "fbe_cli_lib_module_mgmt_cmds.h"
#include "generic_utils_lib.h"
#include "fbe/fbe_api_esp_board_mgmt_interface.h"
#include "generic_utils_lib.h"


/*************************************************************************
 *                            @fn fbe_cli_cmd_ioports ()                                                           *
 *************************************************************************
 *
 *  @brief
 *   fbe_cli_cmd_ioports performs the execution of the IOPORTS command
 *   Display IOM and Port Information
 *
 * @param :
 *   argc     (INPUT) NUMBER OF ARGUMENTS
 *   argv     (INPUT) ARGUMENT LIST
 *
 * @return:
 *   None
 *
 * @author
 *   17-Jan-2011 Created - Rashmi Sawale
 *
 *************************************************************************/

void fbe_cli_cmd_ioports(fbe_s32_t argc,char ** argv)
{
    fbe_esp_module_mgmt_general_status_t general_status = {0};


    if ((argc == 1 )&&((strcmp(*argv, "-help") == 0) ||
        (strcmp(*argv, "-h") == 0)))
    {
        /* If they are asking for help, just display help and exit.
            */
        fbe_cli_printf("\n");
        fbe_cli_printf("%s", IOPORTS_USAGE);
        return;
    }
   
    if (argc > 1)
    {
        fbe_cli_printf("Invalid arguments .Please check the command \n");
        fbe_cli_print_usage();
        return;
    }
    fbe_api_esp_module_mgmt_general_status(&general_status);

    if(!general_status.port_configuration_loaded)
    {
         fbe_cli_printf("\nIO Port configurationw was not loaded from persistent storage.\n");
    }

    if(general_status.slic_upgrade_type == FBE_MODULE_MGMT_SLIC_UPGRADE_LOCAL_SP)
    {
        fbe_cli_printf("\nThere is a SLIC upgrade process running on the local SP.\n");
    }
    else if(general_status.slic_upgrade_type == FBE_MODULE_MGMT_SLIC_UPGRADE_PEER_SP)
    {
        fbe_cli_printf("\nThere is a SLIC upgrade process running on the peer SP.\n");
    }

    if(general_status.reboot_requested != REBOOT_NONE)
    {
        switch(general_status.reboot_requested)
        {
        case REBOOT_LOCAL:
            fbe_cli_printf("\nLocal SP reboot requested.\n");
            break;
        case REBOOT_PEER:
            fbe_cli_printf("\nPeer SP reboot requested.\n");
            break;
        case REBOOT_BOTH:
            fbe_cli_printf("\nDual SP reboot requested.\n");
            break;
        }
    }


    if((argc == 1) && (strcmp(*argv, "-physical") == 0))
    {
        fbe_cli_print_all_iom_physical_info();
    }
    else if((argc == 1) && (strcmp(*argv, "-logical") == 0))
    {
        fbe_cli_print_all_iom_logical_info();
    }
    else if((argc == 1) && (strcmp(*argv, "-iomenv") == 0))
    {
        fbe_cli_print_all_iomenv_info();
    }
    else if((argc == 1) && (strcmp(*argv, "-limits") == 0))
    {
        fbe_cli_print_all_port_limit_info();
    }
    else if((argc == 1) && (strcmp(*argv, "-affinity") == 0))
    {
        fbe_cli_print_all_port_affinity_settings();
    }
    else if ((argc == 1) && (strcmp(*argv, "-link") == 0)) 
    {
        fbe_cli_print_all_port_link_status();
    } 
    else if (argc == 0) 
    {
        fbe_cli_print_all_iom_and_ports_info();
    }
    else
    {
        fbe_cli_printf("Invalid arguments .Please check the command \n");
        fbe_cli_printf("%s", IOPORTS_USAGE);
        return;
    }
}

/************************************************************************
 **                       End of fbe_cli_cmd_ioports ()                **
 ************************************************************************/

/*!**************************************************************
 * fbe_cli_print_all_iom_physical_info()
 ****************************************************************
 * @brief
 *      Prints all physical info for local SP and peer SP.
 *      Called by FBE cli ioports command
 *
 *  @param :
 *      Nothing.
 *
 *  @return:
 *      Nothing.
 *
 * @author
 *  18-Jan-2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
static void fbe_cli_print_all_iom_physical_info(void)
{
    fbe_base_env_sp_t         local_sp = FBE_BASE_ENV_INVALID;
    fbe_status_t                                status = FBE_STATUS_OK;

    status = fbe_api_esp_common_get_sp_id(FBE_CLASS_ID_MODULE_MGMT,&local_sp);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:Failed to get SPID  status:%d\n", 
            __FUNCTION__, status);
        return;
    }

    fbe_cli_printf("\n******************* LOCAL INFO *******************\n");
    fbe_cli_print_physical_info(local_sp ? FBE_BASE_ENV_SPB : FBE_BASE_ENV_SPA,
                                                  FBE_DEVICE_TYPE_MEZZANINE);       /* LOCAL SP */
    fbe_cli_print_physical_info(local_sp ? FBE_BASE_ENV_SPB : FBE_BASE_ENV_SPA,
                                                  FBE_DEVICE_TYPE_BACK_END_MODULE);             /* LOCAL SP */
    fbe_cli_print_physical_info(local_sp ? FBE_BASE_ENV_SPB : FBE_BASE_ENV_SPA,
                                                  FBE_DEVICE_TYPE_IOMODULE);       /* LOCAL SP */

    fbe_cli_printf("\n******************* PEER INFO *******************\n");
    fbe_cli_print_physical_info(local_sp ? FBE_BASE_ENV_SPA : FBE_BASE_ENV_SPB,
                                                  FBE_DEVICE_TYPE_MEZZANINE);       /* PEER SP */
    fbe_cli_print_physical_info(local_sp ? FBE_BASE_ENV_SPA : FBE_BASE_ENV_SPB,
                                                  FBE_DEVICE_TYPE_BACK_END_MODULE);             /* PEER SP */
    fbe_cli_print_physical_info(local_sp ? FBE_BASE_ENV_SPA : FBE_BASE_ENV_SPB,
                                                  FBE_DEVICE_TYPE_IOMODULE);       /* PEER SP */
}
/******************************************************
 * end fbe_cli_print_all_iom_physical_info() 
 ******************************************************/

/*!**************************************************************
 * fbe_cli_print_all_iom_logical_info()
 ****************************************************************
 * @brief
 *      Prints all logical info for local SP and peer SP.
 *      Called by FBE cli ioports command
 *
 *  @param :
 *      Nothing.
 *
 *  @return:
 *      Nothing.
 *
 * @author
 *  18-Jan-2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
static void fbe_cli_print_all_iom_logical_info(void)
{
    fbe_base_env_sp_t         local_sp = FBE_BASE_ENV_INVALID;
    fbe_status_t                                status = FBE_STATUS_OK;

    status = fbe_api_esp_common_get_sp_id(FBE_CLASS_ID_MODULE_MGMT,&local_sp);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:Failed to get SPID  status:%d\n", 
            __FUNCTION__, status);
        return;
    }

    fbe_cli_printf("\n*********************** BE PORTS ***********************\n");
    fbe_cli_printf("\nIOM#   PLABEL#     LOGICAL#     ROLE      STATE       SUBSTATE    PROTOCOL    SUBROLE\n");
    fbe_cli_print_port_info_by_role(FBE_PORT_ROLE_BE,
                                                         local_sp ? FBE_BASE_ENV_SPB : FBE_BASE_ENV_SPA,
                                                         FBE_DEVICE_TYPE_MEZZANINE);
    fbe_cli_print_port_info_by_role(FBE_PORT_ROLE_BE,
                                                         local_sp ? FBE_BASE_ENV_SPB : FBE_BASE_ENV_SPA,
                                                         FBE_DEVICE_TYPE_BACK_END_MODULE);
    fbe_cli_print_port_info_by_role(FBE_PORT_ROLE_BE,
                                                         local_sp ? FBE_BASE_ENV_SPB : FBE_BASE_ENV_SPA,
                                                         FBE_DEVICE_TYPE_IOMODULE);

    fbe_cli_printf("\n*********************** FE PORTS ***********************\n");
    fbe_cli_printf("\nIOM#   PLABEL#     LOGICAL#     ROLE      STATE       SUBSTATE    PROTOCOL    SUBROLE\n");
    fbe_cli_print_port_info_by_role(FBE_PORT_ROLE_FE,
                                                         local_sp ? FBE_BASE_ENV_SPB : FBE_BASE_ENV_SPA,
                                                         FBE_DEVICE_TYPE_MEZZANINE);
    fbe_cli_print_port_info_by_role(FBE_PORT_ROLE_FE,
                                                         local_sp ? FBE_BASE_ENV_SPB : FBE_BASE_ENV_SPA,
                                                         FBE_DEVICE_TYPE_BACK_END_MODULE);
    fbe_cli_print_port_info_by_role(FBE_PORT_ROLE_FE,
                                                         local_sp ? FBE_BASE_ENV_SPB : FBE_BASE_ENV_SPA,
                                                         FBE_DEVICE_TYPE_IOMODULE);
}
/******************************************************
 * end fbe_cli_print_all_iom_logical_info() 
 ******************************************************/

/*!**************************************************************
 * fbe_cli_print_all_iomenv_info()
 ****************************************************************
 * @brief
 *      Prints all iomenv info for local SP and peer SP.
 *      Called by FBE cli ioports command
 *
 *  @param :
 *      Nothing.
 *
 *  @return:
 *      Nothing.
 *
 * @author
 *  18-Jan-2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
static void fbe_cli_print_all_iomenv_info(void)
{
    fbe_base_env_sp_t         local_sp = FBE_BASE_ENV_INVALID;
    fbe_status_t                                status = FBE_STATUS_OK;

    status = fbe_api_esp_common_get_sp_id(FBE_CLASS_ID_MODULE_MGMT,&local_sp);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:Failed to get SPID  status:%d\n", 
            __FUNCTION__, status);
        return;
    }

    fbe_cli_printf("\nLOCAL MEZZANINE:\n");
    fbe_cli_printf("\nCTRL#   PORT#   ENV STATE    UNIQUE ID    USES SFP        SUPP SPEEDS\n");
    fbe_cli_print_iomenv_info( local_sp ? FBE_BASE_ENV_SPB : FBE_BASE_ENV_SPA,
              FBE_DEVICE_TYPE_MEZZANINE);    /* LOCAL SP */

    fbe_cli_printf("\nLOCAL Base Module:\n");
    fbe_cli_printf("\nCTRL#   PORT#   ENV STATE    UNIQUE ID    USES SFP        SUPP SPEEDS\n");
    fbe_cli_print_iomenv_info( local_sp ? FBE_BASE_ENV_SPB : FBE_BASE_ENV_SPA,
              FBE_DEVICE_TYPE_BACK_END_MODULE);    /* LOCAL SP */

    fbe_cli_printf("\nLOCAL IOM:\n");
    fbe_cli_printf("\nCTRL#   PORT#   ENV STATE    UNIQUE ID    USES SFP        SUPP SPEEDS\n");
    fbe_cli_print_iomenv_info(local_sp ? FBE_BASE_ENV_SPB : FBE_BASE_ENV_SPA,
              FBE_DEVICE_TYPE_IOMODULE);    /* LOCAL SP */

}
/******************************************************
 * end fbe_cli_print_all_iomenv_info() 
 ******************************************************/

/*!**************************************************************
 * fbe_cli_print_all_iom_physical_info()
 ****************************************************************
 * @brief
 *      Prints all physical info for local SP and peer SP.
 *      Called by FBE cli ioports command
 *
 *  @param :
 *      Nothing.
 *
 *  @return:
 *      Nothing.
 *
 * @author
 *  18-Jan-2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
static void fbe_cli_print_all_port_limit_info(void)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_esp_module_limits_info_t                limits_info = {0};

    status = fbe_api_esp_module_mgmt_get_limits_info(&limits_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:Failed to get limit info  status:%d\n", 
            __FUNCTION__, status);
        return;
    }

    fbe_cli_printf("\nPROTOCOL     ROLE     #_PORTS\n");
    fbe_cli_printf("  SAS         BE         %u\n",limits_info.platform_port_limit.max_sas_be);
    fbe_cli_printf("  SAS         FE         %u\n",limits_info.platform_port_limit.max_sas_fe);
    fbe_cli_printf("  FCoE        FE         %u\n",limits_info.platform_port_limit.max_fcoe_fe);
    fbe_cli_printf("  FC          FE         %u\n",limits_info.platform_port_limit.max_fc_fe);
    fbe_cli_printf("iSCSI_10G     FE         %u\n",limits_info.platform_port_limit.max_iscsi_10g_fe);
    fbe_cli_printf("iSCSI_1G      FE         %u\n",limits_info.platform_port_limit.max_iscsi_1g_fe);
}

/*!**************************************************************
 * fbe_cli_print_all_port_affinity_settings()
 ****************************************************************
 * @brief
 *      Prints all port interrupt affinity settings.
 *      Called by FBE cli ioports command
 *
 *  @param:
 *      Nothing.
 *
 *  @return:
 *      Nothing.
 *
 * @author
 *  24-Apr-2012:  Created. Lin Lou
 *
 ****************************************************************/
static void fbe_cli_print_all_port_affinity_settings(void)
{
    fbe_base_env_sp_t         local_sp = FBE_BASE_ENV_INVALID;
    fbe_status_t              status = FBE_STATUS_OK;

    status = fbe_api_esp_common_get_sp_id(FBE_CLASS_ID_MODULE_MGMT,&local_sp);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
            "%s:Failed to get SPID  status:%d\n",
            __FUNCTION__, status);
        return;
    }

    fbe_cli_printf("\nLOCAL MEZZANINE:\n");
    fbe_cli_printf("\nCTRL#   PORT#   PCIDATA     CORE#\n");
    fbe_cli_print_port_affinity_settings(local_sp ? FBE_BASE_ENV_SPB : FBE_BASE_ENV_SPA,
              FBE_DEVICE_TYPE_MEZZANINE);    /* LOCAL SP */

    fbe_cli_printf("\nLOCAL Base Module:\n");
    fbe_cli_printf("\nCTRL#   PORT#   PCIDATA     CORE#\n");
    fbe_cli_print_port_affinity_settings(local_sp ? FBE_BASE_ENV_SPB : FBE_BASE_ENV_SPA,
              FBE_DEVICE_TYPE_BACK_END_MODULE);    /* LOCAL SP */

    fbe_cli_printf("\nLOCAL IOM:\n");
    fbe_cli_printf("\nCTRL#   PORT#   PCIDATA     CORE#\n");
    fbe_cli_print_port_affinity_settings(local_sp ? FBE_BASE_ENV_SPB : FBE_BASE_ENV_SPA,
              FBE_DEVICE_TYPE_IOMODULE);    /* LOCAL SP */
}
/******************************************************
 * end fbe_cli_print_all_port_affinity_settings()
 ******************************************************/

/*!**************************************************************
 * fbe_cli_print_all_port_link_status()
 ****************************************************************
 * @brief
 *      Prints the link status for all configured ports
 *
 *  @param:
 *      Nothing.
 *
 *  @return:
 *      Nothing.
 *
 * @author
 *  15-May-2015:  Created. Brion Philbin
 *
 ****************************************************************/
static void fbe_cli_print_all_port_link_status(void)
{
    fbe_base_env_sp_t         local_sp = FBE_BASE_ENV_INVALID;
    fbe_status_t              status = FBE_STATUS_OK;

    status = fbe_api_esp_common_get_sp_id(FBE_CLASS_ID_MODULE_MGMT,&local_sp);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
            "%s:Failed to get SPID  status:%d\n",
            __FUNCTION__, status);
        return;
    }

    
    fbe_cli_printf("\nROLE   PORT#   LINK\n");
    fbe_cli_print_port_link_status(local_sp ? FBE_BASE_ENV_SPB : FBE_BASE_ENV_SPA,
              FBE_DEVICE_TYPE_MEZZANINE);    /* LOCAL SP */

    fbe_cli_print_port_link_status(local_sp ? FBE_BASE_ENV_SPB : FBE_BASE_ENV_SPA,
              FBE_DEVICE_TYPE_BACK_END_MODULE);    /* LOCAL SP */

    fbe_cli_print_port_link_status(local_sp ? FBE_BASE_ENV_SPB : FBE_BASE_ENV_SPA,
              FBE_DEVICE_TYPE_IOMODULE);    /* LOCAL SP */

}

/*!**************************************************************
 * fbe_cli_print_all_iom_and_ports_info()
 ****************************************************************
 * @brief
 *      Prints all modules and ports info for local SP and peer SP.
 *      Called by FBE cli ioports command
 *
 *  @param :
 *      Nothing.
 *
 *  @return:
 *      Nothing.
 *
 * @author
 *  18-Jan-2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
static void fbe_cli_print_all_iom_and_ports_info(void)
{
    fbe_base_env_sp_t         local_sp = FBE_BASE_ENV_INVALID;
    fbe_status_t                                status = FBE_STATUS_OK;

    status = fbe_api_esp_common_get_sp_id(FBE_CLASS_ID_MODULE_MGMT,&local_sp);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:Failed to get SPID  status:%d\n", 
            __FUNCTION__, status);
        return;
    }

    fbe_cli_printf("\n******************* LOCAL INFO *******************\n");
    fbe_cli_print_all_iomodules_info(local_sp ? FBE_BASE_ENV_SPB : FBE_BASE_ENV_SPA,
                                                           FBE_DEVICE_TYPE_MEZZANINE);    /* LOCAL SP */
    fbe_cli_print_all_iomodules_info(local_sp ? FBE_BASE_ENV_SPB : FBE_BASE_ENV_SPA,
                                                           FBE_DEVICE_TYPE_BACK_END_MODULE);    /* LOCAL SP */
    fbe_cli_print_all_iomodules_info(local_sp ? FBE_BASE_ENV_SPB : FBE_BASE_ENV_SPA,
                                                           FBE_DEVICE_TYPE_IOMODULE);    /* LOCAL SP */

    fbe_cli_printf("\n******************* PEER INFO *******************\n");
    fbe_cli_print_physical_info(local_sp ? FBE_BASE_ENV_SPA : FBE_BASE_ENV_SPB,
                                                  FBE_DEVICE_TYPE_MEZZANINE);    /* PEER SP */
    fbe_cli_print_physical_info(local_sp ? FBE_BASE_ENV_SPA : FBE_BASE_ENV_SPB,
                                                  FBE_DEVICE_TYPE_BACK_END_MODULE);    /* PEER SP */
    fbe_cli_print_physical_info(local_sp ? FBE_BASE_ENV_SPA : FBE_BASE_ENV_SPB,
                                                  FBE_DEVICE_TYPE_IOMODULE);    /* PEER SP */
}

/******************************************************
 * end fbe_cli_print_all_iom_and_ports_info() 
 ******************************************************/

/*!**************************************************************
 * fbe_cli_print_all_iomodules_info()
 ****************************************************************
 * @brief
 *      Displays io modules and ports info
 *
 *  @param :
 *      fbe_u32_t                 which_sp
 *      fbe_u32_t                 device_type
 *
 *  @return:
 *      Nothing.
 *
 * @author
 *  18-Jan-2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
static void fbe_cli_print_all_iomodules_info(fbe_u32_t which_sp,fbe_u32_t device_type)
{
    fbe_u32_t                                   slot_num = 0;
    fbe_u32_t                                   max_slots = 0;
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_status_t                                module_info_status = FBE_STATUS_OK;
    fbe_status_t                                module_status = FBE_STATUS_OK;
    fbe_esp_module_io_module_info_t             iomodule_info = {0};
    fbe_esp_module_mgmt_module_status_t         module_status_info = {0};
    fbe_esp_module_limits_info_t                limits_info = {0};

    status = fbe_api_esp_module_mgmt_get_limits_info(&limits_info);

    if(status != FBE_STATUS_OK)
    {
        return;
    }

    if(device_type == FBE_DEVICE_TYPE_IOMODULE)
    {
        max_slots = limits_info.discovered_hw_limit.num_slic_slots;
    }
    else if(device_type == FBE_DEVICE_TYPE_MEZZANINE)
    {
        max_slots = limits_info.discovered_hw_limit.num_mezzanine_slots;
    }
    else if(device_type == FBE_DEVICE_TYPE_BACK_END_MODULE)
    {
        max_slots = limits_info.discovered_hw_limit.num_bem;
    }

    for(slot_num = 0; slot_num < max_slots; slot_num++)
    {
        iomodule_info.header.sp = which_sp;
        iomodule_info.header.type = device_type;
        iomodule_info.header.slot = slot_num;

        module_info_status = fbe_api_esp_module_mgmt_getIOModuleInfo(&iomodule_info);

        if(module_info_status == FBE_STATUS_GENERIC_FAILURE)
            continue;

        module_status_info.header.sp = which_sp;
        module_status_info.header.type = device_type;
        module_status_info.header.slot = slot_num;

        module_status = fbe_api_esp_module_mgmt_get_module_status(&module_status_info);

       if(module_status == FBE_STATUS_GENERIC_FAILURE)
            continue;

       if(device_type == FBE_DEVICE_TYPE_MEZZANINE)
       {
            fbe_cli_printf("\nMEZZANINE: %d\n", iomodule_info.io_module_phy_info.slotNumOnBlade);
            fbe_cli_printf("\nINSERTED   #_CTRLS     STATE           SUBSTATE          FFID\n");
            fbe_cli_print_mezzanine_info(&iomodule_info,module_status_info,limits_info.discovered_hw_limit.num_mezzanine_slots);
       }
       if(device_type == FBE_DEVICE_TYPE_BACK_END_MODULE)
       {
            fbe_cli_printf("\nBase Module: %d\n", iomodule_info.io_module_phy_info.slotNumOnBlade);
            fbe_cli_printf("\nINSERTED   #_CTRLS     STATE           SUBSTATE          FFID\n");
            fbe_cli_print_mezzanine_info(&iomodule_info,module_status_info,limits_info.discovered_hw_limit.num_bem);
       }
        else if (device_type == FBE_DEVICE_TYPE_IOMODULE)
       {
            fbe_cli_printf("\nLOCAL IOM: %d\n", iomodule_info.io_module_phy_info.slotNumOnBlade);
            fbe_cli_printf("\nIOM INS PORTS POWER     TYPE      STATE        SUBSTATE     LABEL       FFID\n");
        
            fbe_cli_print_iomodule_info(&iomodule_info,module_status_info,iomodule_info.io_module_phy_info.slotNumOnBlade);
        }

        fbe_cli_printf("\nPLABEL#   LOGICAL#     ROLE     STATE        SUBSTATE      PROTOCOL    SUBROLE\n");
        fbe_cli_print_all_ports_info(&limits_info, &iomodule_info, device_type, which_sp);
    }
}
/******************************************************
 * end fbe_cli_print_all_iomodules_info() 
 ******************************************************/

/*!**************************************************************
 * fbe_cli_print_port_affinity_settings()
 ****************************************************************
 * @brief
 *      Prints port interrupt affinity settings of specified
 *      SP & module type
 *
 *  @param :
 *      fbe_u32_t                 which_sp
 *      fbe_u32_t                 device_type
 *
 *  @return:
 *      Nothing.
 *
 * @author
 *  24-Apr-2012:  Created. Lin Lou
 *
 ****************************************************************/
static void fbe_cli_print_port_affinity_settings(fbe_u32_t which_sp, fbe_u32_t device_type)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_esp_module_mgmt_port_affinity_t         port_affinity = {0};
    fbe_u32_t                                   slot_num = 0;
    fbe_u32_t                                   port_num = 0;
    fbe_u32_t                                   max_slots = 0;
    fbe_esp_module_limits_info_t                limits_info = {0};
    fbe_esp_module_io_port_info_t               io_port_info = {0};

    status = fbe_api_esp_module_mgmt_get_limits_info(&limits_info);

    if(status != FBE_STATUS_OK)
    {
        return;
    }

    if(device_type == FBE_DEVICE_TYPE_IOMODULE)
    {
        max_slots = limits_info.discovered_hw_limit.num_slic_slots;
    }
    else if(device_type == FBE_DEVICE_TYPE_MEZZANINE)
    {
        max_slots = limits_info.discovered_hw_limit.num_mezzanine_slots;
    }

    for(slot_num = 0; slot_num < max_slots; slot_num++)
    {
        for(port_num = 0; port_num< limits_info.discovered_hw_limit.num_ports; port_num++)
        {
            io_port_info.header.sp =which_sp;
            io_port_info.header.type = device_type;
            io_port_info.header.slot = slot_num;
            io_port_info.header.port = port_num;

            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&io_port_info);
            if(status == FBE_STATUS_GENERIC_FAILURE)
            {
                 continue;
            }

            port_affinity.header.sp = which_sp;
            port_affinity.header.type = device_type;
            port_affinity.header.slot = slot_num;
            port_affinity.header.port = port_num;

            status = fbe_api_esp_module_mgmt_get_port_affinity(&port_affinity);

            if (status == FBE_STATUS_GENERIC_FAILURE)
                continue;

            fbe_cli_printf("%3u", io_port_info.io_port_info.slotNumOnBlade);
            fbe_cli_printf("%8u", io_port_info.io_port_info.portNumOnModule);
            fbe_cli_printf("     [%2u.%2u.%2u]", port_affinity.pciData.bus, port_affinity.pciData.device, port_affinity.pciData.function);

            if (port_affinity.processor_core != INVALID_CORE_NUMBER)
            {
                fbe_cli_printf("%5u\n", port_affinity.processor_core);
            }
            else
            {
                fbe_cli_printf("   NA\n");
            }
        }
    }
}
/******************************************************
 * end fbe_cli_print_port_affinity_settings()
 ******************************************************/

/*!**************************************************************
 * fbe_cli_print_port_link_status()
 ****************************************************************
 * @brief
 *      Prints port link status for all ports of specified
 *      SP & module type
 *
 *  @param :
 *      fbe_u32_t                 which_sp
 *      fbe_u32_t                 device_type
 *
 *  @return:
 *      Nothing.
 *
 * @author
 *  15-May-2015:  Created. Brion Philbin
 *
 ****************************************************************/
static void fbe_cli_print_port_link_status(fbe_u32_t which_sp, fbe_u32_t device_type)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_u32_t                                   slot_num = 0;
    fbe_u32_t                                   port_num = 0;
    fbe_u32_t                                   max_slots = 0;
    fbe_esp_module_limits_info_t                limits_info = {0};
    fbe_esp_module_io_port_info_t               io_port_info = {0};

    status = fbe_api_esp_module_mgmt_get_limits_info(&limits_info);

    if(status != FBE_STATUS_OK)
    {
        return;
    }

    if(device_type == FBE_DEVICE_TYPE_IOMODULE)
    {
        max_slots = limits_info.discovered_hw_limit.num_slic_slots;
    }
    else if(device_type == FBE_DEVICE_TYPE_MEZZANINE)
    {
        max_slots = limits_info.discovered_hw_limit.num_mezzanine_slots;
    }

    // loop through all slots and all ports
    for(slot_num = 0; slot_num < max_slots; slot_num++)
    {
        for(port_num = 0; port_num< limits_info.discovered_hw_limit.num_ports; port_num++)
        {
            io_port_info.header.sp =which_sp;
            io_port_info.header.type = device_type;
            io_port_info.header.slot = slot_num;
            io_port_info.header.port = port_num;

            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&io_port_info);
            if(status == FBE_STATUS_GENERIC_FAILURE)
            {
                 continue;
            }
            if (io_port_info.io_port_logical_info.logical_number == INVALID_PORT_U8) 
            {
                // not a configured port, skip it
                continue;
            }
            fbe_cli_printf("\n%4s   %2u       %s",
                           fbe_module_mgmt_port_role_to_string(io_port_info.io_port_logical_info.port_role),
                           io_port_info.io_port_logical_info.logical_number,
                           fbe_module_mgmt_convert_link_state_to_string(io_port_info.io_port_info.link_info.link_state));
        }
    }
    fbe_cli_printf("\n");
}

 /*!**************************************************************
 * fbe_cli_print_physical_info()
 ****************************************************************
 * @brief
 *      Prints logical information of local & Peer SP. 
 *      Called by FBE cli ioports command
 *
 *  @param :
 *      fbe_u32_t                       whichsp
 *      fbe_u32_t                       device_type
 *
 *  @return:
 *      Nothing.
 *
 * @author
 *  18-Jan-2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
static void fbe_cli_print_physical_info(fbe_u32_t which_sp,fbe_u32_t device_type)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_esp_module_limits_info_t                limits_info = {0};
    fbe_u32_t                                   slot_num = 0;
    fbe_u32_t                                   max_slots = 0;
    fbe_status_t                                module_info_status = FBE_STATUS_OK;
    fbe_status_t                                module_status = FBE_STATUS_OK;
    fbe_esp_module_io_module_info_t             iomodule_info = {0};
    fbe_esp_module_mgmt_module_status_t         module_status_info = {0};

    status = fbe_api_esp_module_mgmt_get_limits_info(&limits_info);

    if(status != FBE_STATUS_OK)
    {
        return;
    }

    if(device_type == FBE_DEVICE_TYPE_IOMODULE)
    {
        max_slots = limits_info.discovered_hw_limit.num_slic_slots;
    }
    else if(device_type == FBE_DEVICE_TYPE_MEZZANINE)
    {
        max_slots = limits_info.discovered_hw_limit.num_mezzanine_slots;
    }

    for(slot_num = 0; slot_num < max_slots; slot_num++)
    {
        fbe_zero_memory(&iomodule_info, sizeof(fbe_esp_module_io_module_info_t));
        iomodule_info.header.sp = which_sp;
        iomodule_info.header.type = device_type;
        iomodule_info.header.slot = slot_num;

        module_info_status =fbe_api_esp_module_mgmt_getIOModuleInfo(&iomodule_info);

        if(module_info_status == FBE_STATUS_GENERIC_FAILURE)
            continue;

        module_status_info.header.sp = which_sp;
        module_status_info.header.type = device_type;
        module_status_info.header.slot = slot_num;

        module_status = fbe_api_esp_module_mgmt_get_module_status(&module_status_info);

        if(module_status == FBE_STATUS_GENERIC_FAILURE)
            continue;

        if(device_type == FBE_DEVICE_TYPE_MEZZANINE)
        {
             fbe_cli_printf("\nMEZZANINE: %d\n", iomodule_info.io_module_phy_info.slotNumOnBlade);
             fbe_cli_printf("\nINSERTED   #_CTRLS     STATE           SUBSTATE          FFID\n");

            fbe_cli_print_mezzanine_info(&iomodule_info,
                                         module_status_info,
                                         limits_info.discovered_hw_limit.num_mezzanine_slots);
        }
        else if(device_type == FBE_DEVICE_TYPE_BACK_END_MODULE)
        {
             fbe_cli_printf("\nBase Module: %d\n", iomodule_info.io_module_phy_info.slotNumOnBlade);
             fbe_cli_printf("\nINSERTED   #_CTRLS     STATE           SUBSTATE          FFID\n");

            fbe_cli_print_mezzanine_info(&iomodule_info,
                                         module_status_info,
                                         limits_info.discovered_hw_limit.num_bem);
        }
        else if (device_type == FBE_DEVICE_TYPE_IOMODULE)
       {
            fbe_cli_printf("\nIOM: %d\n", slot_num);
            fbe_cli_printf("\nIOM INS PORTS POWER     TYPE      STATE        SUBSTATE     LABEL       FFID\n");
            fbe_cli_print_iomodule_info(&iomodule_info, module_status_info, slot_num);
       }
    }
}
/******************************************************
 * end fbe_cli_print_physical_info() 
 ******************************************************/
 
 /*!**************************************************************
 * fbe_cli_print_port_info_by_role()
 ****************************************************************
 * @brief
 *      Displays all information for all ports of BE/FE roles 
 *
 *  @param :
 *      fbe_port_role_t              port_role
 *      fbe_u32_t                    which_sp
 *      fbe_u32_t                       device_type
 *
 *  @return:
 *      Nothing.
 *
 * @author
 *  18-Jan-2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
static void fbe_cli_print_port_info_by_role(fbe_port_role_t port_role,fbe_u32_t which_sp,fbe_u32_t device_type)
{
    fbe_u32_t                         slot_num = 0;
    fbe_u32_t                         port_num = 0;
    fbe_u32_t                         max_slots = 0;
    fbe_esp_module_io_port_info_t     io_port_info = {0};
    fbe_status_t                      status = FBE_STATUS_OK;
    fbe_status_t                      port_status = FBE_STATUS_OK;
    fbe_esp_module_limits_info_t       limits_info = {0};
       
    status = fbe_api_esp_module_mgmt_get_limits_info(&limits_info);

    if(status != FBE_STATUS_OK)
    {
        return;
    }

    if(device_type == FBE_DEVICE_TYPE_IOMODULE)
    {
        max_slots = limits_info.discovered_hw_limit.num_slic_slots;
    }
    else if(device_type == FBE_DEVICE_TYPE_MEZZANINE)
    {
        max_slots = limits_info.discovered_hw_limit.num_mezzanine_slots;
    }
    else if(device_type == FBE_DEVICE_TYPE_BACK_END_MODULE)
    {
        max_slots = limits_info.discovered_hw_limit.num_bem;
    }


    for(slot_num = 0; slot_num <max_slots; slot_num++)
    {
        for(port_num = 0; port_num< limits_info.discovered_hw_limit.num_ports; port_num++)
        {
            io_port_info.header.sp = which_sp;
            io_port_info.header.type = device_type;
            io_port_info.header.slot = slot_num;
            io_port_info.header.port = port_num;

            port_status = fbe_api_esp_module_mgmt_getIOModulePortInfo (&io_port_info);
            if(port_status == FBE_STATUS_GENERIC_FAILURE)
            {
                 continue;
            }
            if(io_port_info.io_port_logical_info.port_role == port_role)
            {
                 fbe_cli_print_logical_info(&io_port_info, device_type);
            }
        }
    }
}
/******************************************************
 * end fbe_cli_print_port_info_by_role() 
 ******************************************************/

/*!**************************************************************
 * fbe_cli_print_iomenv_info()
 ****************************************************************
 * @brief
 *      Prints all iomenv info for local SP and peer SP.
 *      Called by FBE cli ioports command
 *
 *  @param :
 *      fbe_u32_t                which_sp
 *      fbe_u64_t    device_type
 *
 *  @return:
 *      Nothing.
 *
 * @author
 *  18-Jan-2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
static void fbe_cli_print_iomenv_info(fbe_u32_t which_sp,fbe_u64_t device_type)
{
    fbe_u32_t                                   slot_num = 0;
    fbe_u32_t                                   port_num = 0;
    fbe_u32_t                                   max_slots = 0;
    fbe_esp_module_io_port_info_t               io_port_info = {0};
    fbe_esp_module_io_module_info_t             module_info = {0};
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_esp_module_limits_info_t                limits_info = {0};

    status = fbe_api_esp_module_mgmt_get_limits_info(&limits_info);

    if(status != FBE_STATUS_OK)
    {
        return;
    }

    if(device_type == FBE_DEVICE_TYPE_IOMODULE)
    {
        max_slots = limits_info.discovered_hw_limit.num_slic_slots;
    }
    else if(device_type == FBE_DEVICE_TYPE_MEZZANINE)
    {
        max_slots = limits_info.discovered_hw_limit.num_mezzanine_slots;
    }
    else if(device_type == FBE_DEVICE_TYPE_BACK_END_MODULE)
    {
        max_slots = limits_info.discovered_hw_limit.num_bem;
    }

    for(slot_num = 0; slot_num < max_slots; slot_num++)
    {
        module_info.header.sp = which_sp;
        module_info.header.type = device_type;
        module_info.header.slot = slot_num;

        status = fbe_api_esp_module_mgmt_getIOModuleInfo(&module_info);

        for(port_num = 0; port_num< limits_info.discovered_hw_limit.num_ports; port_num++)
        {
            io_port_info.header.sp =which_sp;
            io_port_info.header.type = device_type;
            io_port_info.header.slot = slot_num;
            io_port_info.header.port = port_num;

            status = fbe_api_esp_module_mgmt_getIOModulePortInfo (&io_port_info);

            if(status == FBE_STATUS_OK)
            {
                fbe_cli_printf("%3u",module_info.io_module_phy_info.slotNumOnBlade);
                fbe_cli_printf("%8u",io_port_info.io_port_info.portNumOnModule);
                fbe_cli_printf("%10s", fbe_module_mgmt_get_env_status(module_info));
                fbe_cli_printf("        0x%-10x", module_info.io_module_phy_info.uniqueID);
                fbe_cli_printf("%7s", fbe_module_mgmt_mgmt_status_to_string(io_port_info.io_port_info.SFPcapable));
                fbe_cli_printf("%21s\n", fbe_module_mgmt_supported_speed_to_string(io_port_info.io_port_info.supportedSpeeds));

            }
        }
    }
}
/******************************************************
 * end fbe_cli_print_iomenv_info() 
 ******************************************************/
 

/*!**************************************************************
 * fbe_cli_print_mezzanine_info()
 ****************************************************************
 * @brief
 *      Prints mezzanine related information of local & Peer SP. 
 *      Called by FBE cli ioports command
 *
 *  @param :
 *      fbe_esp_module_io_module_info_t          mezzanine_info
 *      fbe_esp_module_mgmt_module_status_t      module_status_info
 *      fbe_u32_t                                num_mezzanine_slots
 *
 *  @return:
 *      Nothing.
 *
 * @author
 *  18-Jan-2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
 static void fbe_cli_print_mezzanine_info(fbe_esp_module_io_module_info_t *mezzanine_info,
                                          fbe_esp_module_mgmt_module_status_t module_status_info,
                                          fbe_u32_t num_mezzanine_slots)
{
    fbe_cli_printf("%3s", fbe_module_mgmt_mgmt_status_to_string(mezzanine_info->io_module_phy_info.inserted));
    fbe_cli_printf("%12u", num_mezzanine_slots);
    fbe_cli_printf("%14s", fbe_module_mgmt_module_state_to_string(module_status_info.state));
    fbe_cli_printf("%18s", fbe_module_mgmt_module_substate_to_string(module_status_info.substate));
    fbe_cli_printf("    %s\n", decodeFamilyFruID(mezzanine_info->io_module_phy_info.uniqueID));
}
/******************************************************
 * end fbe_cli_print_mezzanine_info() 
 ******************************************************/


/*!**************************************************************
 * fbe_cli_print_iomodule_info()
 ****************************************************************
 * @brief
 *      Prints mezzanine port related information of local & Peer SP. 
 *      Called by FBE cli ioports command
 *
 *  @param :
 *      fbe_esp_module_io_module_info_t          iomodule_info
 *      fbe_esp_module_mgmt_module_status_t      module_status_info
 *      fbe_u32_t                                slot_num
 *
 *  @return:
 *      Nothing.
 *
 * @author
 *  18-Jan-2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
 static void fbe_cli_print_iomodule_info(fbe_esp_module_io_module_info_t *iomodule_info,
                                         fbe_esp_module_mgmt_module_status_t module_status_info,
                                         fbe_u32_t slot_num)
{
    fbe_cli_printf("%3u ", slot_num);
    fbe_cli_printf("%3s", fbe_module_mgmt_mgmt_status_to_string(iomodule_info->io_module_phy_info.inserted));
    fbe_cli_printf("%3u", iomodule_info->io_module_phy_info.ioPortCount);
    fbe_cli_printf("%7s", fbe_module_mgmt_power_status_to_string(iomodule_info->io_module_phy_info.powerStatus));
    fbe_cli_printf("%9s", fbe_module_mgmt_device_type_to_string(iomodule_info->io_module_phy_info.deviceType));
    fbe_cli_printf("%12s", fbe_module_mgmt_module_state_to_string(module_status_info.state));
    fbe_cli_printf("%16s", fbe_module_mgmt_module_substate_to_string(module_status_info.substate));
    fbe_cli_printf("%15s", iomodule_info->label_name);
    fbe_cli_printf("  %s\n", decodeFamilyFruID(iomodule_info->io_module_phy_info.uniqueID));
}
/******************************************************
 * end fbe_cli_print_iomodule_info() 
 ******************************************************/

/*!**************************************************************
 * fbe_cli_print_all_ports_info()
 ****************************************************************
 * @brief
 *      Prints all port related information of local  SP. 
 *
 *  @param :
 *     fbe_esp_module_limits_info_t       limits_info,
 *     fbe_esp_module_io_module_info_t    module_info
 *     fbe_u64_t                           device_type,
 *     fbe_u32_t                                       which_sp
 *
 *  @return:
 *      Nothing.
 *
 * @author
 *  18-Jan-2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
static void fbe_cli_print_all_ports_info(fbe_esp_module_limits_info_t *limits_info,
                                         fbe_esp_module_io_module_info_t *module_info,
                                         fbe_u64_t device_type,
                                         fbe_u32_t which_sp)
{
    fbe_u32_t                                   port_num = 0;
    fbe_esp_module_io_port_info_t    io_port_info = {0};
    fbe_status_t                                status = FBE_STATUS_OK;

    for(port_num = 0;port_num < limits_info->discovered_hw_limit.num_ports; port_num++)
    {
         io_port_info.header.sp = which_sp;
         io_port_info.header.type = device_type;
         io_port_info.header.slot = module_info->io_module_phy_info.slotNumOnBlade;
         io_port_info.header.port = port_num;

         status = fbe_api_esp_module_mgmt_getIOModulePortInfo (&io_port_info);

         if(status == FBE_STATUS_GENERIC_FAILURE)
         {
             continue;
         }

         if(module_info->io_module_phy_info.inserted ==  FBE_MGMT_STATUS_NA ||
            module_info->io_module_phy_info.inserted ==  FBE_MGMT_STATUS_UNKNOWN)
         {
             continue;
         }

         fbe_cli_print_port_info(&io_port_info, port_num);
    }
}
/******************************************************
 * end fbe_cli_print_all_ports_info() 
 ******************************************************/
 
/*!**************************************************************
 * fbe_cli_print_port_info()
 ****************************************************************
 * @brief
 *      Prints port information of local & Peer SP. 
 *      Called by FBE cli ioports command
 *
 *  @param :
 *      fbe_esp_module_io_port_info_t   io_port_info
 *      fbe_u32_t                       port_num
 *
 *  @return:
 *      Nothing.
 *
 * @author
 *  18-Jan-2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
static void fbe_cli_print_port_info(fbe_esp_module_io_port_info_t *io_port_info, fbe_u32_t port_num)
{
    fbe_cli_printf("%3u",port_num);
    if(io_port_info->io_port_logical_info.logical_number == INVALID_PORT_U8)
    {
        fbe_cli_printf("%10s","UNC");
    }
    else
    {
        fbe_cli_printf("%10u",io_port_info->io_port_logical_info.logical_number);
    }
    fbe_cli_printf("%12s",fbe_module_mgmt_port_role_to_string(io_port_info->io_port_logical_info.port_role));
    fbe_cli_printf("%11s",fbe_module_mgmt_port_state_to_string(io_port_info->io_port_logical_info.port_state));
    fbe_cli_printf("%16s",fbe_module_mgmt_port_substate_to_string(io_port_info->io_port_logical_info.port_substate));
    fbe_cli_printf("%12s",fbe_module_mgmt_protocol_to_string(io_port_info->io_port_info.protocol));
    fbe_cli_printf("%11s\n",fbe_module_mgmt_port_subrole_to_string(io_port_info->io_port_logical_info.port_subrole));
}


/*!**************************************************************
 * fbe_cli_print_logical_info()
 ****************************************************************
 * @brief
 *      Prints logical information of local & Peer SP. 
 *      Called by FBE cli ioports command
 *
 *  @param :
 *      fbe_esp_module_io_port_info_t   io_port_info
 *      fbe_u64_t               device_type
 *  @return:
 *      Nothing.
 *
 * @author
 *  18-Jan-2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
static void fbe_cli_print_logical_info(fbe_esp_module_io_port_info_t *io_port_info, fbe_u64_t device_type)
{
    if(device_type == FBE_DEVICE_TYPE_MEZZANINE)
    {
        fbe_cli_printf("MEZZ%u",io_port_info->io_port_info.slotNumOnBlade);
    }
    if(device_type == FBE_DEVICE_TYPE_BACK_END_MODULE)
    {
        fbe_cli_printf("BEM %u",io_port_info->io_port_info.slotNumOnBlade);
    }
    else if(device_type == FBE_DEVICE_TYPE_IOMODULE)
    {
        fbe_cli_printf("SLIC%u",io_port_info->io_port_info.slotNumOnBlade);
    }
    fbe_cli_printf("%6u",io_port_info->io_port_info.portNumOnModule);
    if(io_port_info->io_port_logical_info.logical_number == INVALID_PORT_U8)
    {
        fbe_cli_printf("%13s","UNC");
    }
    else
    {
        fbe_cli_printf("%13u",io_port_info->io_port_logical_info.logical_number);
    }
    fbe_cli_printf("%11s",fbe_module_mgmt_port_role_to_string(io_port_info->io_port_logical_info.port_role));
    fbe_cli_printf("%12s",fbe_module_mgmt_port_state_to_string(io_port_info->io_port_logical_info.port_state));
    fbe_cli_printf("%15s",fbe_module_mgmt_port_substate_to_string(io_port_info->io_port_logical_info.port_substate));
    fbe_cli_printf("%10s",fbe_module_mgmt_protocol_to_string(io_port_info->io_port_info.protocol));
    fbe_cli_printf("%11s\n",fbe_module_mgmt_port_subrole_to_string(io_port_info->io_port_logical_info.port_subrole));
}
/******************************************************
 * end fbe_cli_print_logical_info() 
 ******************************************************/

/*!**************************************************************
 * fbe_module_mgmt_get_env_status()
 ****************************************************************
 * @brief
 *  This function converts the env interface status to a text string.
 *
 * @param 
 *        fbe_esp_module_io_module_info_t     module_info
 * 
 * @return
 *        fbe_char_t *string for env interface status
 * 
 * @author
 *  18-Jan-2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
static fbe_char_t * fbe_module_mgmt_get_env_status(fbe_esp_module_io_module_info_t  module_info)
{
    if((module_info.io_module_phy_info.envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_DATA_STALE)&&
       (module_info.io_module_phy_info.inserted == FBE_MGMT_STATUS_FALSE))
    {
        return "EMPTY";
    }

    if(module_info.io_module_phy_info.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL)
    {
        return "UNKNOWN";
    }

    if(module_info.io_module_phy_info.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD)
    {
        return "OK";
    }

    if(module_info.io_module_phy_info.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_NA)
    {
        return "NA";
    }

    return "NA"; /* what to return here? - prior code returned this by accident */
}


/*!**************************************************************
 * fbe_cli_module_mgmt_get_platform_port_limit()
 ****************************************************************
 * @brief
 *  This function takes a port protocol and role and returns a platform
 *  limit for the maximum number of those ports that are configurable.
 *
 * @param 
 *       port_protocol - protocol for the port
 *       port_role - FE, BE or Uncommitted
 *
 * @return 
 *       fbe_u32_t - port limit
 *
 * @author
 *  18-Jan-2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
static fbe_u32_t fbe_cli_module_mgmt_get_platform_port_limit(fbe_environment_limits_platform_port_limits_t *platform_port_limits,
                                                             IO_CONTROLLER_PROTOCOL port_protocol, 
                                                             fbe_port_role_t port_role)
{
    switch(port_protocol)
    {
    case IO_CONTROLLER_PROTOCOL_SAS:
        if(port_role == FBE_PORT_ROLE_BE)
        {
            return platform_port_limits->max_sas_be;
        }
        else if(port_role == FBE_PORT_ROLE_FE)
        {
            return platform_port_limits->max_sas_fe;
        }
        break;
    case IO_CONTROLLER_PROTOCOL_FCOE:
        return platform_port_limits->max_fcoe_fe;
        break;
    case IO_CONTROLLER_PROTOCOL_FIBRE:
        return platform_port_limits->max_fc_fe;
        break;
    case IO_CONTROLLER_PROTOCOL_ISCSI:
        return 8; // the iSCSI 1G/10G limits are shared and limited by driver capability for now
        break;
    default:
        return 0xFF;
        break;
    }
    return 0xFF;
}
/******************************************************
 * end fbe_cli_module_mgmt_get_platform_port_limit() 
 ******************************************************/
 /*!**************************************************************
 * fbe_cli_module_mgmt_config_mgmt_port()
 ****************************************************************
 * @brief
 *  This is entry function for fbecli command configmgmtport/cmp
 *
 * @param :
 *   argc     (INPUT) NUMBER OF ARGUMENTS
 *   argv     (INPUT) ARGUMENT LIST
 *
 * @return void
 *
 * @author
 *  15-April-2011: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
void
fbe_cli_module_mgmt_config_mgmt_port(fbe_s32_t argc,char ** argv)
{
    SP_ID           localSP;
    fbe_u32_t       spIndex;
    fbe_u32_t       slotIndex;
    fbe_u32_t       arg_index;
    fbe_status_t    status;
    fbe_base_env_sp_t   localSpid = FBE_BASE_ENV_INVALID;
    fbe_esp_module_limits_info_t        limits_info = {0};
    fbe_mgmt_port_speed_t           mgmt_port_speed = FBE_MGMT_PORT_SPEED_UNSPECIFIED;
    fbe_mgmt_port_auto_neg_t        mgmt_port_auto_neg = FBE_PORT_AUTO_NEG_UNSPECIFIED;
    fbe_mgmt_port_duplex_mode_t     mgmt_port_duplex = FBE_PORT_DUPLEX_MODE_UNSPECIFIED;
    fbe_esp_module_mgmt_get_mgmt_comp_info_t    mgmt_comp_info = {0};
    fbe_esp_module_mgmt_set_mgmt_port_config_t   set_mgmt_port_config = {0};
    fbe_esp_module_mgmt_get_mgmt_port_config_t     mgmt_port_config = {0}; 
    fbe_esp_board_mgmt_board_info_t   boardInfo = {0};
    fbe_u32_t                    spCount=SP_ID_MAX;

    /* Print the command help */
    if ((argc == 1)&&((fbe_compare_string(argv[0], 5, "-help", 5, FBE_FALSE) == 0) ||
        (fbe_compare_string(argv[0], 2,"-h", 2, FBE_FALSE) == 0)))
    {
        fbe_cli_printf("\n");
        fbe_cli_printf("%s", FBE_CONFIGMGMTPORT_USAGE);
        return;
    }

    /* Check the mgmt module count */
    status = fbe_api_esp_module_mgmt_get_limits_info(&limits_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("Failed to get MGMT module count \n");
        return;
    }
    if(limits_info.discovered_hw_limit.num_mgmt_modules == 0)
    {
        fbe_cli_printf("There is no mgmt module\n");
        return;
    }    

    /* Get the local SPID */
    status = fbe_api_esp_common_get_sp_id(FBE_CLASS_ID_MODULE_MGMT,
                                          &localSpid);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("Failed to get SPID \n");
        return;
    
    }
    localSP = (localSpid == FBE_BASE_ENV_SPA) ? SP_A : SP_B;


    status = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get boardInfo, status:%d\n", status);
        return;
    }

    if (boardInfo.isSingleSpSystem == TRUE)
    {
        spCount = SP_COUNT_SINGLE;
    }
    else
    {
        spCount = SP_ID_MAX;
    }
    
    if(argc == 1)
    {
        /* For display status */
        if(fbe_compare_string(argv[0], 7,"-status", 7, FBE_FALSE) == 0)
        {
            for(spIndex = SP_A; spIndex < spCount; spIndex++)
            {
                mgmt_comp_info.phys_location.sp = spIndex;
                for(slotIndex = 0; 
                    slotIndex < limits_info.discovered_hw_limit.num_mgmt_modules; 
                    slotIndex++)
                {
                    fbe_cli_printf("***** %s Management module %d management Port Status *****\n", 
                                    (spIndex == SP_A)? "SPA" : "SPB", slotIndex);
                    
                    mgmt_comp_info.phys_location.slot = slotIndex;
                    status = fbe_api_esp_module_mgmt_get_mgmt_comp_info(&mgmt_comp_info);
                    if(status != FBE_STATUS_OK)
                    {
                        fbe_cli_error("Failed to get MGMT module info.\n");
                        return;
                    }
                    if(!mgmt_comp_info.mgmt_module_comp_info.bInserted)
                    {
                        fbe_cli_printf("Management module is not inserted\n");
                    }
                    else
                    {
                        fbe_cli_printf("\tGeneral Fault : %s\n",
                            fbe_module_mgmt_convert_general_fault_to_string(mgmt_comp_info.mgmt_module_comp_info.generalFault));
                        fbe_cli_printf("\tEnvironment Interface Status : %s\n",
                            fbe_module_mgmt_convert_env_interface_status_to_string(mgmt_comp_info.mgmt_module_comp_info.envInterfaceStatus));
                        if(mgmt_comp_info.mgmt_module_comp_info.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD)
                        {
                            fbe_cli_printf("\tLink          : %s\n", 
                                           (mgmt_comp_info.mgmt_module_comp_info.managementPort.linkStatus)? "UP" : "DOWN");
                            if(mgmt_comp_info.mgmt_module_comp_info.managementPort.linkStatus)
                            {
                                /* The link is up. Print the auto neg, speed, duplex. */
                                fbe_cli_printf("\tAuto negotiate: %s\n", 
                                    fbe_module_mgmt_convert_mgmtPortAutoNeg_to_string(mgmt_comp_info.mgmt_module_comp_info.managementPort.mgmtPortAutoNeg));
                                fbe_cli_printf("\tSpeed         : %s\n", 
                                    fbe_module_mgmt_convert_externalMgmtPortSpeed_to_string(mgmt_comp_info.mgmt_module_comp_info.managementPort.mgmtPortSpeed));
                                fbe_cli_printf("\tDuplex Mode   : %s\n", 
                                    fbe_module_mgmt_convert_externalMgmtPortDuplexMode_to_string(mgmt_comp_info.mgmt_module_comp_info.managementPort.mgmtPortDuplex));
                            }
                            if(spIndex == localSP)
                            {
                                /*Requested speed change */
                                status = fbe_api_esp_module_mgmt_get_config_mgmt_port_info(&mgmt_port_config);
                                fbe_cli_printf("\tRequested Auto negotiate: %s\n", 
                                     fbe_module_mgmt_convert_mgmtPortAutoNeg_to_string(mgmt_port_config.mgmtPortConfig.mgmtPortAutoNeg));
                                fbe_cli_printf("\tRequested Speed         : %s\n", 
                                     fbe_module_mgmt_convert_externalMgmtPortSpeed_to_string(mgmt_port_config.mgmtPortConfig.mgmtPortSpeed));
                                fbe_cli_printf("\tRequested Duplex Mode   : %s\n", 
                                     fbe_module_mgmt_convert_externalMgmtPortDuplexMode_to_string(mgmt_port_config.mgmtPortConfig.mgmtPortDuplex));
                            }
                        }
                    }
                }
            }
        }        
        else
        {
            fbe_cli_printf("Invalid arguments.");
            fbe_cli_printf("%s", FBE_CONFIGMGMTPORT_USAGE);
            return;
        }
    }
    else if(argc >= 2)
    {
        /* Set revert by default */
        set_mgmt_port_config.revert = FBE_TRUE;
        set_mgmt_port_config.phys_location.sp = localSP;        
        for (arg_index = 0; arg_index < argc; arg_index ++)
        {
            if(fbe_compare_string(argv[arg_index], 5, "-auto", 5, FBE_FALSE) == 0)
            {
                arg_index++;
                if(arg_index >= argc)
                {
                    fbe_cli_printf("Argument missing for -auto\n");
                    return;
                }
                if(fbe_compare_string(argv[arg_index], 2, "on", 2, FBE_FALSE) == 0)
                {
                    mgmt_port_auto_neg = FBE_PORT_AUTO_NEG_ON;
                }
                else if(fbe_compare_string(argv[arg_index], 3, "off", 3, FBE_FALSE) == 0)
                {
                    mgmt_port_auto_neg = FBE_PORT_AUTO_NEG_OFF;
                }
                else
                {
                    fbe_cli_printf("Auto negotiate mode should be on or off.\n");
                    return;
                }
            }
            else if (fbe_compare_string(argv[arg_index], 6,"-speed", 6, FBE_FALSE) == 0)
            {
                arg_index++;
                if(arg_index >= argc)
                {
                    fbe_cli_printf("Argument missing for -speed\n");
                    return;
                }  
                if(fbe_compare_string(argv[arg_index], 3, "10m", 3, FBE_FALSE) == 0)
                {
                    mgmt_port_speed = FBE_MGMT_PORT_SPEED_10MBPS;
                }
                else if(fbe_compare_string(argv[arg_index], 4, "100m", 4, FBE_FALSE) == 0)
                {
                    mgmt_port_speed = FBE_MGMT_PORT_SPEED_100MBPS;
                }
                else if(fbe_compare_string(argv[arg_index], 5,"1000m", 5, FBE_FALSE) == 0)
                {
                    mgmt_port_speed = FBE_MGMT_PORT_SPEED_1000MBPS;
                }
                else
                {
                    fbe_cli_printf("management port speed should be 10m, 100m, or 1000m.\n");
                    return;
                }
            }
            else if(fbe_compare_string(argv[arg_index], 7,"-duplex", 7, FBE_FALSE) == 0)
            {
                arg_index++;
                if(arg_index >= argc)
                {
                    fbe_cli_printf("Argument missing for -duplex\n");
                    return;
                }  
                if(fbe_compare_string(argv[arg_index], 4,"half", 4, FBE_FALSE) == 0)
                {
                    mgmt_port_duplex = FBE_PORT_DUPLEX_MODE_HALF;
                }
                else if(fbe_compare_string(argv[arg_index], 4,"full", 4, FBE_FALSE) == 0)
                {
                    mgmt_port_duplex = FBE_PORT_DUPLEX_MODE_FULL;
                }
                else
                {
                    fbe_cli_printf("management port duplex mode should be full or half.\n");
                    return;
                }
            }
            else if(fbe_compare_string(argv[arg_index], 6,"-norevert", 6, FBE_FALSE) == 0) 
            {
                set_mgmt_port_config.revert = FBE_FALSE;
                fbe_cli_printf("Revert Disabled.\n");
            }
            else 
            {
                fbe_cli_printf("Invalid option: %s.\n", argv[arg_index]);
                fbe_cli_printf("%s", FBE_CONFIGMGMTPORT_USAGE);
                return;
            }
        }

        /* Initialized the port speed config command */
        set_mgmt_port_config.phys_location.slot = 0;
        set_mgmt_port_config.mgmtPortRequest.mgmtPortAutoNeg = mgmt_port_auto_neg;
        set_mgmt_port_config.mgmtPortRequest.mgmtPortSpeed = mgmt_port_speed;
        set_mgmt_port_config.mgmtPortRequest.mgmtPortDuplex = mgmt_port_duplex;
        status = fbe_api_esp_module_mgmt_set_mgmt_port_config(&set_mgmt_port_config);
        if(status == FBE_STATUS_OK)
        {
            fbe_cli_printf("\n**Management Port Configuration Successfully Completed**\n");
        }
        else
        {
            fbe_cli_printf("\n**Management Port Configuration Failed, Status: 0x%X**\n", status );
        }    
    }
    else
    {
        fbe_cli_printf("Invalid argument options\n\n");
        fbe_cli_printf("%s", FBE_CONFIGMGMTPORT_USAGE);
    }    
    return;
}
/******************************************************
 * end fbe_cli_module_mgmt_config_mgmt_port() 
 ******************************************************/

/*************************************************************************
*       fbe_cli_get_setport_inputs_from_user()
*************************************************************************
*
* DESCRIPTION:
*    This function prompts user for setport advance inputs from user
* and helps with default values.
*
* PARAMETERS:
*     fbe_esp_module_mgmt_set_port_t - pointer to adm port config data to hold
* data gathered from user.
*
* RETURN VALUES:
*     BOOL - TRUE if success, FALSE otherwise.
*
* NOTES:
*
* HISTORY:
*   31-May-12 dongz - Created
*
*
*************************************************************************/
fbe_status_t fbe_cli_get_setport_inputs_from_user(fbe_esp_module_mgmt_set_port_t *set_port_cfg_datap)
{
    fbe_u32_t port_index;

    fbe_u8_t response[30];
    fbe_u8_t *result;
    fbe_esp_module_limits_info_t                limits_info;
    fbe_status_t                                status;

    //get limits info
    fbe_zero_memory(&limits_info, sizeof(fbe_esp_module_limits_info_t));
    status = fbe_api_esp_module_mgmt_get_limits_info(&limits_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf ( "Failed to get SP limits info. status:%d\n", status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // number of ports
    fbe_cli_printf("Please enter number of ports to configure[1..%d]:\n",
            limits_info.discovered_hw_limit.num_ports);
    result = fgets(response, sizeof(response), stdin);
    if (!result)
    {
        fbe_cli_printf("Invalid number of ports.\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    set_port_cfg_datap->num_ports = atoi(response);
    if((set_port_cfg_datap->num_ports > limits_info.discovered_hw_limit.num_ports) || (set_port_cfg_datap->num_ports < 1))
    {
        fbe_cli_printf("Number of ports should be a number from 1 to %d\n", limits_info.discovered_hw_limit.num_ports);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for(port_index = 0;
            port_index < set_port_cfg_datap->num_ports;
            port_index++)
    {
        //io module class
        fbe_cli_printf("Please enter io module class 1-SLIC, 2-Mezzanine and 3-BM:\n");
        result = fgets(response, sizeof(response), stdin);
        if (!result)
        {
            fbe_cli_printf("Invalid io module class.\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if(atoi(response) == 1)
        {
            set_port_cfg_datap->port_config[port_index].port_location.type = FBE_DEVICE_TYPE_IOMODULE;
        }
        else if(atoi(response) == 2)
        {
            set_port_cfg_datap->port_config[port_index].port_location.type = FBE_DEVICE_TYPE_MEZZANINE;
        }
        else if(atoi(response) == 3)
        {
            set_port_cfg_datap->port_config[port_index].port_location.type = FBE_DEVICE_TYPE_BACK_END_MODULE;
        }
        else
        {
            fbe_cli_printf("Invalid io module class.\n");
            fbe_cli_printf("Io module class should be from 1 to 3.\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        // slot number
        if(set_port_cfg_datap->port_config[port_index].port_location.type == FBE_DEVICE_TYPE_MEZZANINE
                || set_port_cfg_datap->port_config[port_index].port_location.type == FBE_DEVICE_TYPE_BACK_END_MODULE)
        {
            fbe_cli_printf("IO Module class Mezzanine or BM, set slot to 0.\n");
            set_port_cfg_datap->port_config[port_index].port_location.slot = 0;
        }
        else
        {
            fbe_cli_printf("Please enter slot number:\n");
            result = fgets(response, sizeof(response), stdin);
            if (!result)
            {
                fbe_cli_printf("Invalid slot number.\n");
                return FBE_STATUS_GENERIC_FAILURE;
            }
            set_port_cfg_datap->port_config[port_index].port_location.slot = atoi(response);

            if(set_port_cfg_datap->port_config[port_index].port_location.type == FBE_DEVICE_TYPE_IOMODULE) // IOM
            {
                if((set_port_cfg_datap->port_config[port_index].port_location.slot > limits_info.discovered_hw_limit.num_slic_slots))
                {
                    fbe_cli_printf("slot number should be from 0 to %d\n", limits_info.discovered_hw_limit.num_slic_slots);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
            }
        }

        // port number
        fbe_cli_printf("Please enter port number[IOM:0..3 Mezzanine 0..7]:\n");
        result = fgets(response, sizeof(response), stdin);
        if (!result)
        {
            fbe_cli_printf("Invalid port number.\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        set_port_cfg_datap->port_config[port_index].port_location.port = atoi(response);
        if((set_port_cfg_datap->port_config[port_index].port_location.port > 7))
        {
            fbe_cli_printf("port number should be from 0 to 3 for IOM and 0..7 for Mezzanine\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        // logical number
        fbe_cli_printf("Please enter logical number:\n");
        result = fgets(response, sizeof(response), stdin);
        if (!result)
        {
            fbe_cli_printf("Invalid logical number.\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        set_port_cfg_datap->port_config[port_index].logical_number = atoi(response);

        // io module group
        fbe_cli_printf(IOM_GROUP_EXPECTED_VALUES);
        result = fgets(response, sizeof(response), stdin);
        if (!result)
        {
            fbe_cli_printf("Invalid io module group.\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        set_port_cfg_datap->port_config[port_index].iom_group = atoi(response);
        if((set_port_cfg_datap->port_config[port_index].iom_group > 11) ||
            (set_port_cfg_datap->port_config[port_index].iom_group < 0))
        {
            fbe_cli_printf(IOM_GROUP_EXPECTED_VALUES);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        // port role
        fbe_cli_printf("Please enter port role[0-UNC,1-FE,2-BE]:\n");
        result = fgets(response, sizeof(response), stdin);
        if (!result)
        {
            fbe_cli_printf("Invalid port role.\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        set_port_cfg_datap->port_config[port_index].port_role = atoi(response);
        if((set_port_cfg_datap->port_config[port_index].port_role > 2) ||
            (set_port_cfg_datap->port_config[port_index].port_role < 0))
        {
            fbe_cli_printf("port role should be from 0 to 2 [0-UNC,1-FE,2-BE]\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        // coldfront is FE only
        if((set_port_cfg_datap->port_config[port_index].iom_group == 8) &&
            (set_port_cfg_datap->port_config[port_index].port_role != 1))
        {
            fbe_cli_printf("Invalid iom group and port role combination\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        // port subrole
        fbe_cli_printf("Please enter port subrole[0-UNC,1-SPEC,2-NORM]:\n");
        result = fgets(response, sizeof(response), stdin);
        if (!result)
        {
            fbe_cli_printf("Invalid port subrole.\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        set_port_cfg_datap->port_config[port_index].port_subrole = atoi(response);
        if((set_port_cfg_datap->port_config[port_index].port_subrole > 2) ||
            (set_port_cfg_datap->port_config[port_index].port_subrole < 0))
        {
            fbe_cli_printf("port subrole should be from 0 to 2 [0-UNC,1-SPEC,2-NORM]\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }// for adm_p_num

    return FBE_STATUS_OK;
}

fbe_u64_t fbe_cli_convert_iom_class_to_device_type(fbe_u32_t iom_class)
{
    if(iom_class == 1)
    {
        return FBE_DEVICE_TYPE_IOMODULE;
    }
    if(iom_class == 2)
    {
        return FBE_DEVICE_TYPE_MEZZANINE;
    }
    return FBE_DEVICE_TYPE_INVALID;

}

/*************************************************************************
*       fbe_cli_validate_and_fill_setport_cmd_arg()
*************************************************************************
*
* DESCRIPTION:
*    This function validates the inputs received from user while
* removing or disabling ports.
*
* PARAMETERS:
*     fbe_esp_module_mgmt_set_port_t - pointer to port config data to hold
* data gathered from user.
*
* RETURN VALUES:
*     fbe_status_t - FBE_STATUS_OK if success.
*
* NOTES:
*
* HISTORY:
*   31-May-12 dongz - Created
*
*
*************************************************************************/
fbe_status_t fbe_cli_validate_and_fill_setport_cmd_arg(fbe_s32_t argc,
                                                          char ** argv,
                                                          fbe_esp_module_mgmt_set_port_t *set_port_cfg_datap)
{
    fbe_esp_module_limits_info_t                limits_info;
    fbe_esp_module_io_module_info_t             io_module_info;
    fbe_u32_t                                   offset;
    fbe_u32_t                                   iom_class;
    fbe_u32_t                                   slot;
    fbe_u32_t                                   port;
    fbe_u32_t                                   index;
    fbe_u32_t                                   port_num;
    fbe_u32_t                                   p_index;
    fbe_u32_t                                   logical_num;
    fbe_u32_t                                   iom_group;
    fbe_u32_t                                   port_role;
    fbe_u32_t                                   port_subrole;

    fbe_base_env_sp_t                           local_sp = FBE_BASE_ENV_INVALID;
    fbe_status_t                                status = FBE_STATUS_OK;

    if(set_port_cfg_datap->opcode == FBE_ESP_MODULE_MGMT_UPGRADE_PORTS
       || set_port_cfg_datap->opcode == FBE_ESP_MODULE_MGMT_REPLACE_PORTS
       || set_port_cfg_datap->opcode == FBE_ESP_MODULE_MGMT_REMOVE_ALL_PORTS_CONFIG
       || set_port_cfg_datap->opcode == FBE_ESP_MODULE_MGMT_PERSIST_ALL_PORTS_CONFIG)
    {
        return FBE_STATUS_OK;
    }

    if(set_port_cfg_datap->opcode == FBE_ESP_MODULE_MGMT_PERSIST_PORT_CONFIG_LIST)
    {
        offset = SET_PORT_CMD_FIRST_ARG_POS;
    }
    else if(set_port_cfg_datap->opcode == FBE_ESP_MODULE_MGMT_REMOVE_PORT_CONFIG_LIST)
    {
        offset = SET_PORT_REM_PORT_CMD_FIRST_ARG_POS;
    }
    else
    {
        fbe_cli_printf("Unsupport opcode\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //get limits info
    fbe_zero_memory(&limits_info, sizeof(fbe_esp_module_limits_info_t));
    status = fbe_api_esp_module_mgmt_get_limits_info(&limits_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf ( "Failed to get SP limits info. status:%d\n", status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    index = 0;

    //validate and fill req data
    do
    {
        //check the number of arg is correct
        if(set_port_cfg_datap->opcode == FBE_ESP_MODULE_MGMT_PERSIST_PORT_CONFIG_LIST)
        {
            if(offset + SET_PORT_PERSIST_ARG_NUM > argc)
            {
                fbe_cli_printf("Number of arg pre check fail! Too few arguments for persistent port!\n");
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        else if(set_port_cfg_datap->opcode == FBE_ESP_MODULE_MGMT_REMOVE_PORT_CONFIG_LIST)
        {
            if(offset + SET_PORT_REM_ARG_MIN > argc)
            {
                fbe_cli_printf("Number of arg pre check fail! Too few arguments for remove ports!\n");
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        //set iom_class start
        if(strcmp (argv[offset], "-c") != 0)
        {
            // bad, print expected
            fbe_cli_printf("Expected -c as arg %d\n", offset);
            fbe_cli_printf("Got %s\n", argv[offset]);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        iom_class = atoi(argv[offset+1]);

        set_port_cfg_datap->port_config[index].port_location.type
                = fbe_cli_convert_iom_class_to_device_type(iom_class);

        if(set_port_cfg_datap->port_config[index].port_location.type == FBE_DEVICE_TYPE_INVALID)
        {
            // bad, print actual
            fbe_cli_printf("io module class out of range (arg %d): %s\n", (offset+1), argv[offset+1]);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        //set iom_class end

        offset += 2;

        //set slot start
        if(strcmp(argv[offset], "-s") != 0)
        {
            // bad, print expected
            fbe_cli_printf("Expected -s as arg %d\n", offset);
            fbe_cli_printf("Got %s\n", argv[offset]);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        slot = atoi(argv[offset+1]);

        if((set_port_cfg_datap->port_config[index].port_location.type == FBE_DEVICE_TYPE_IOMODULE && slot >= limits_info.platform_hw_limit.max_slics)
                ||(set_port_cfg_datap->port_config[index].port_location.type == FBE_DEVICE_TYPE_MEZZANINE && slot >= limits_info.platform_hw_limit.max_mezzanines))
        {
            // bad, print actual
            fbe_cli_printf("io module class out of range (arg %d): %s\n", (offset+1), argv[offset+1]);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        set_port_cfg_datap->port_config[index].port_location.slot = slot;
        //set slot end

        offset += 2;

        //get number of ports on this slic/mezzanine
        status = fbe_api_esp_common_get_sp_id(FBE_CLASS_ID_MODULE_MGMT,&local_sp);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf ("Failed to get SPID  status:%d\n", status);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        fbe_zero_memory(&io_module_info, sizeof(fbe_esp_module_io_module_info_t));

        //TODO local sp is local sp?
        io_module_info.header.sp = local_sp;
        io_module_info.header.type = set_port_cfg_datap->port_config[index].port_location.type;
        io_module_info.header.slot = set_port_cfg_datap->port_config[index].port_location.slot;

        status = fbe_api_esp_module_mgmt_getIOModuleInfo(&io_module_info);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf ("Failed to get IO module info.  status:%d\n", status);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        //set port start
        if(offset >= argc || strcmp(argv[offset], "-p") != 0)
        {
            //for a "-rem" cmd, if the next argument is not "-p", we deem as remove all ports on this slic
            if(set_port_cfg_datap->opcode == FBE_ESP_MODULE_MGMT_REMOVE_PORT_CONFIG_LIST)
            {
                port_num = io_module_info.io_module_phy_info.ioPortCount;

                if(index + port_num >= FBE_ESP_IO_PORT_MAX_COUNT)
                {
                    fbe_cli_printf ("Number of ports exceed limit!\n");
                    return FBE_STATUS_GENERIC_FAILURE;
                }

                for(p_index = 0; p_index < port_num; p_index++)
                {
                    set_port_cfg_datap->port_config[index + p_index].port_location =
                            set_port_cfg_datap->port_config[index].port_location;
                    set_port_cfg_datap->port_config[index + p_index].port_location.port = p_index;
                }
                index += p_index;

                continue;
            }
            else
            {
                // bad, print expected
                fbe_cli_printf("Expected -p as arg %d\n", offset);
                fbe_cli_printf("Got %s\n", argv[offset]);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        else
        {
            port = atoi(argv[offset + 1]);

            if(port < 0 || port > io_module_info.io_module_phy_info.ioPortCount)
            {
                // bad, print actual
                fbe_cli_printf("io module class out of range (arg %d): %s\n", (offset+1), argv[offset+1]);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            set_port_cfg_datap->port_config[index].port_location.port = port;

            //set port end

            offset += 2;
        }

        //special info set for persist command
        if(set_port_cfg_datap->opcode == FBE_ESP_MODULE_MGMT_PERSIST_PORT_CONFIG_LIST)
        {
            //set logical start
            if(strcmp(argv[offset], "-l") != 0)
            {
                // bad, print expected
                fbe_cli_printf("Expected -l as arg %d\n", offset);
                fbe_cli_printf("Got %s\n", argv[offset]);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            logical_num = atoi(argv[offset+1]);

            //no limit of logical port number
            if(logical_num < 0 || logical_num > 0xfe)
            {
                // bad, print actual
                fbe_cli_printf("io module class out of range (arg %d): %s\n", (offset+1), argv[offset+1]);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            set_port_cfg_datap->port_config[index].logical_number = logical_num;
            //set logical end

            offset += 2;

            //set group start
            if(strcmp(argv[offset], "-g") != 0)
            {
                // bad, print expected
                fbe_cli_printf("Expected -g as arg %d\n", offset);
                fbe_cli_printf("Got %s\n", argv[offset]);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            iom_group = atoi(argv[offset+1]);

            if(iom_group <= FBE_IOM_GROUP_UNKNOWN || iom_group >= FBE_IOM_GROUP_END)
            {
                // bad, print actual
                fbe_cli_printf("io module class out of range (arg %d): %s\n", (offset+1), argv[offset+1]);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            set_port_cfg_datap->port_config[index].iom_group = iom_group;
            //set group end

            offset += 2;

            //set port role start
            if(strcmp(argv[offset], "-pr") != 0)
            {
                // bad, print expected
                fbe_cli_printf("Expected -pr as arg %d\n", offset);
                fbe_cli_printf("Got %s\n", argv[offset]);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            port_role = atoi(argv[offset+1]);

            if(port_role <= IOPORT_PORT_ROLE_UNINITIALIZED || port_role > IOPORT_PORT_ROLE_BE )
            {
                // bad, print actual
                fbe_cli_printf("io module class out of range (arg %d): %s\n", (offset+1), argv[offset+1]);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            set_port_cfg_datap->port_config[index].port_role = port_role;
            //set port role end

            offset += 2;

            //set port sub role start
            if(strcmp(argv[offset], "-psr") != 0)
            {
                // bad, print expected
                fbe_cli_printf("Expected -psr as arg %d\n", offset);
                fbe_cli_printf("Got %s\n", argv[offset]);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            port_subrole = atoi(argv[offset+1]);

            //TODO max of port_subrole?
            if(port_subrole <= FBE_PORT_SUBROLE_UNINTIALIZED || port_subrole > FBE_PORT_SUBROLE_NORMAL )
            {
                // bad, print actual
                fbe_cli_printf("io module class out of range (arg %d): %s\n", (offset+1), argv[offset+1]);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            set_port_cfg_datap->port_config[index].port_subrole = port_subrole;
            //set port sub role end

            offset += 2;
        }

        //update index
        index +=1;

    }while(offset < argc && index < FBE_ESP_IO_PORT_MAX_COUNT);

    //set number of ports
    set_port_cfg_datap->num_ports = index;

    return FBE_STATUS_OK;
}

/*************************************************************************
 *                             fbe_cli_cmd_set_port_cmd()
 *************************************************************************
 *
 * DESCRIPTION:
 *     fbe_cli_cmd_set_port_cmd - Allows user to have the ability to
 *                                set/modify the port information from fbecli
 *
 *
 * PARAMETERS:
 *   argc     NUMBER OF ARGUMENTS
 *   argv     ARGUMENT LIST
 *
 * RETURN VALUES:
 *   None
 *
 * NOTES:
 *
 * HISTORY:
 *   31-May-12 dongz -- Created
 *
 *************************************************************************/
void  fbe_cli_cmd_set_port_cmd(fbe_s32_t argc, char ** argv)
{
    fbe_esp_module_mgmt_set_port_t set_port_cfg_data;
    fbe_status_t status;

    fbe_zero_memory(&set_port_cfg_data, sizeof(fbe_esp_module_mgmt_set_port_t));

    if (argc == 0)
    {
        // no inputs specified, get those from user
        fbe_cli_printf("No command switches entered...\n");

        status = fbe_cli_get_setport_inputs_from_user(&set_port_cfg_data);
        if (status != FBE_STATUS_OK)
        {
            // input error, display command usage
            fbe_cli_printf(FBE_SETPORT_USAGE);
            return;
        }
        else
        {
            set_port_cfg_data.opcode = FBE_ESP_MODULE_MGMT_PERSIST_PORT_CONFIG_LIST;

            //send set port config cmd to module mgmt
            status = fbe_api_esp_module_mgmt_set_port_config(&set_port_cfg_data);
            if(status != FBE_STATUS_OK)
            {
                // print any errors
                fbe_cli_printf("\n**Operation failed, error: 0x%x**\n", status);
            }
            else
            {
                fbe_cli_printf("\n**Operation Completed**\n");
            }
            return;
        }
    }
    else if (strcmp (argv[0], "-h") == 0 )
    {
        fbe_cli_printf(FBE_SETPORT_USAGE);
        return;
    }
    else if (strcmp (argv[0], "-persist") == 0 )
    {
        set_port_cfg_data.opcode = FBE_ESP_MODULE_MGMT_PERSIST_ALL_PORTS_CONFIG;
    }
    else if (strcmp (argv[0], "-remove") == 0 )
    {
        set_port_cfg_data.opcode = FBE_ESP_MODULE_MGMT_REMOVE_ALL_PORTS_CONFIG;
    }

    else if(strcmp (argv[0], "-upgrade") == 0)
    {
        set_port_cfg_data.opcode = FBE_ESP_MODULE_MGMT_UPGRADE_PORTS;
    }
    else if(strcmp (argv[0], "-replace") == 0)
    {
        set_port_cfg_data.opcode = FBE_ESP_MODULE_MGMT_REPLACE_PORTS;
    }
    else if (strcmp (argv[0], "-remove_list") == 0 )
    {
        set_port_cfg_data.opcode = FBE_ESP_MODULE_MGMT_REMOVE_PORT_CONFIG_LIST;
    }
    else if (strcmp (argv[0], "-persist_list") == 0 )
    {
        set_port_cfg_data.opcode = FBE_ESP_MODULE_MGMT_PERSIST_PORT_CONFIG_LIST;
    }
    else
    {
        fbe_cli_printf(FBE_SETPORT_USAGE);
        return;
    }

    status = fbe_cli_validate_and_fill_setport_cmd_arg(argc, argv, &set_port_cfg_data);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf(FBE_SETPORT_USAGE);
        return;
    }

    //send set port config cmd to module mgmt
    status = fbe_api_esp_module_mgmt_set_port_config(&set_port_cfg_data);
    if(status != FBE_STATUS_OK)
    {
        // print any errors
        fbe_cli_printf("\n**Operation failed, error: 0x%x**\n", status);
    }
    else
    {
        fbe_cli_printf("\n**Operation Completed**\n");
    }

return;
}
