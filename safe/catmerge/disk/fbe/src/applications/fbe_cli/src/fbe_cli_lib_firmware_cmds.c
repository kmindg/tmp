/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_firmware_cmds.c
 ***************************************************************************
 *
 * @brief
 *  This file contains FBE cli functions for the Firmware.
 *
 * @ingroup fbe_cli
 *
 * @revision
 *   07/09/2010:  Vishnu Sharma - created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_cli_firmware_cmd.h"
#include "fbe/fbe_api_common.h"
#include "fbe_cli_private.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_shim_flare_interface.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fbe_api_enclosure_interface.h"
#include "fbe_base_environment_debug.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_api_esp_ps_mgmt_interface.h"
#include "fbe/fbe_api_esp_board_mgmt_interface.h"
#include "fbe/fbe_api_esp_module_mgmt_interface.h"
#include "fbe/fbe_api_esp_sps_mgmt_interface.h"
#include "fbe/fbe_api_esp_cooling_mgmt_interface.h"

static fbe_status_t fbe_cli_print_fup_manifest_info(fbe_base_env_fup_manifest_info_t * pManifestInfo);

/*************************************************************************
 *                            @fn fbe_cli_cmd_firmware_upgrade ()                 *
 *************************************************************************
 *
 *  @brief
 *   fbe_cli_cmd_firmware_upgrade performs the execution of the fup command
 *   Display Firmware Upgrade related information
 *
 * @param :
 *   argc     (INPUT) NUMBER OF ARGUMENTS
 *   argv     (INPUT) ARGUMENT LIST
 *
 * @return:
 *   None
 *
 * @author
 *   07-September-2010 Created - Vishnu Sharma
 *
 *************************************************************************/

void fbe_cli_cmd_firmware_upgrade(fbe_s32_t argc,char ** argv)
{

    /*********************************
    **    VARIABLE DECLARATIONS    **
    *********************************/
    fbe_u64_t deviceType = FBE_DEVICE_TYPE_INVALID;
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_bool_t   initiate_fup = FBE_FALSE;
    fbe_bool_t   status_of_upgrade = FBE_FALSE;
    fbe_bool_t   firmware_revision = FBE_FALSE;
    fbe_bool_t   eng_option = FBE_FALSE;
    fbe_bool_t   abort_upgrade = FBE_FALSE;
    fbe_bool_t   resume_upgrade = FBE_FALSE;
    fbe_bool_t   terminate_upgrade = FBE_FALSE;
    fbe_bool_t   get_manifest_info = FBE_FALSE;
    fbe_bool_t   invalid_option = FBE_FALSE;
    fbe_base_env_fup_force_flags_t forceFlags = 0;
    fbe_device_physical_location_t location = {0};
    fbe_bool_t   location_flag = FBE_FALSE,temp_flag = FBE_FALSE;
    fbe_bool_t   sp_flag = FBE_FALSE;
    fbe_esp_encl_mgmt_encl_info_t encl_info = {0};
    fbe_u32_t    enclCntOnBus = 0;
    fbe_u32_t    slotCount = 0;
    fbe_u32_t    bus =0, encl = 0, slot =0; // for iteration.
    fbe_u32_t    componentCount=0;
    fbe_esp_board_mgmt_board_info_t   boardInfo = {0};
    fbe_esp_module_limits_info_t limits_info = {0};
    fbe_esp_base_env_get_fup_info_t getFupInfo = {0};
    fbe_u8_t idx;
    fbe_u32_t    spCount=SP_ID_MAX;
    fbe_base_env_fup_manifest_info_t *pManifestInfo = NULL;
    fbe_bool_t   modify_time = FBE_FALSE;
    fbe_u32_t    delayInSec = 0;

    /*****************
    **    BEGIN    **
    *****************/
    if ((argc == 1 )&&((strcmp(*argv, "-help") == 0) ||
        (strcmp(*argv, "-h") == 0)))
    {
        /* If they are asking for help, just display help and exit.
            */
        fbe_cli_printf("\n");
        fbe_cli_printf("%s", FUP_USAGE);
        return;
    }
   
    if (argc < 1)
    {
        fbe_cli_printf("\n");
        fbe_cli_printf("%s", FUP_USAGE);
        return;
    }
    while(argc > 0)
    {
        if(strcmp(*argv, "i") == 0)
        {
            initiate_fup = FBE_TRUE;
        }
        else if(strcmp(*argv, "s") == 0)
        {
            status_of_upgrade = FBE_TRUE;
        }
        else if(strcmp(*argv, "r") == 0)
        {
            firmware_revision = FBE_TRUE;
        }
        else if(strcmp(*argv, "terminate") == 0)
        {
            terminate_upgrade = FBE_TRUE;
        }
        else if(strcmp(*argv, "abort") == 0)
        {
            abort_upgrade = FBE_TRUE;
        }
        else if(strcmp(*argv, "resume") == 0)
        {
            resume_upgrade = FBE_TRUE;
        }
        else if(strcmp(*argv, "m") == 0)
        {
            get_manifest_info = FBE_TRUE;
        }
        else if(strcmp(*argv, "t") == 0)
        {
            modify_time = FBE_TRUE;
        }
        else if(strcmp(*argv, "-eng") == 0)
        {
            eng_option = FBE_TRUE;
        }
        else if(strcmp(*argv, "-d") == 0)
        {
            argc--;
            argv++;     
            if(argc == 0)
            {
                fbe_cli_printf(" Please provide device type #.\n");
                return;
            }

            if(strcmp(*argv, "lcc") == 0)
            {
                deviceType = FBE_DEVICE_TYPE_LCC;
            }
            else if(strcmp(*argv, "ps") == 0) 
            {
                deviceType = FBE_DEVICE_TYPE_PS;
            }
            else if(strcmp(*argv, "fan") == 0)  
            {
                deviceType = FBE_DEVICE_TYPE_FAN;
            }
            else if(strcmp(*argv, "sps") == 0)  
            {
                deviceType = FBE_DEVICE_TYPE_SPS;
            }
            else if(strcmp(*argv, "bm") == 0) 
            {
                deviceType = FBE_DEVICE_TYPE_BACK_END_MODULE;
            }
            else if(strcmp(*argv, "sp") == 0) 
            {
                deviceType = FBE_DEVICE_TYPE_SP;
            }
            else
            {
                fbe_cli_printf("Invalid device type\n");
                fbe_cli_printf("Please provide valid device type: lcc, ps or fan.\n");
                fbe_cli_printf("%s", FUP_USAGE);
                return;
            }
        }
        else if(strcmp(*argv, "-b") == 0)
        {
            argc--;
            argv++;     // move to the BUS number
            if(argc == 0)
            {
                fbe_cli_printf(" Please provide bus #.\n");
                return;
            }
            location.bus = atoi(*argv);
            if(location.bus >= FBE_PHYSICAL_BUS_COUNT && location.bus != FBE_XPE_PSEUDO_BUS_NUM)
            {
                fbe_cli_printf("Invalid Bus number\n");
                fbe_cli_printf("%s", FUP_USAGE);
                return;
            }
            location_flag = FBE_TRUE;
        }
        else if(strcmp(*argv, "-e") == 0)
        {
            argc--;
            argv++;     // move to the ENCLOSURE number
            if(argc == 0)
            {
                fbe_cli_printf(" Please provide enclosure position #.\n");
                return;
            }
            location.enclosure = atoi(*argv);
            if((location.enclosure >=FBE_ENCLOSURES_PER_BUS) &&
               (location.enclosure != FBE_XPE_PSEUDO_ENCL_NUM))
            {
                fbe_cli_printf("Invalid Enclosure number\n");
                fbe_cli_printf("%s", FUP_USAGE);
                return;
            }
        }
        else if(strcmp(*argv, "-s") == 0)
        {
            argc--;
            argv++;     // move to the SLOT number
            if(argc == 0)
            {
                fbe_cli_printf(" Please provide slot number #.\n");
                return;
            }
            location.slot = atoi(*argv);
            if(location.slot < 0 || location.slot >=FBE_ENCLOSURE_SLOTS)
            {
                fbe_cli_printf("Invalid Slot number\n");
                fbe_cli_printf("%s", FUP_USAGE);
                return;
            }
        }
        else if((strcmp(*argv, "-cid") == 0) ||
                (strcmp(*argv, "-c") == 0))
        {
            argc--;
            argv++;     // move to the component ID
            if(argc == 0)
            {
                fbe_cli_printf(" Please provide component id #.\n");
                return;
            }
            location.componentId = atoi(*argv);
        }
        else if(strcmp(*argv, "-sp") == 0)
        {
            argc--;
            argv++;     // move to the SP
            if(argc == 0)
            {
                fbe_cli_printf(" Please provide SP number #.\n");
                return;
            }
            location.sp = atoi(*argv);
            sp_flag=FBE_TRUE;
            location_flag = FBE_TRUE;
        }
        else if(strcmp(*argv, "-delay") == 0)
        {
            argc--;
            argv++;     // move to the delay time
            if(argc == 0)
            {
                fbe_cli_printf(" Please provide the delay time in seconds.\n");
                return;
            }
            delayInSec = atoi(*argv);
        }
        else if(strcmp(*argv, "-f") == 0)
        {
            argc--;
            argv++;     
            if(argc == 0)
            {
                fbe_cli_printf(" Please provide force option.\n");
                return;
            }

            if(strcmp(*argv, "norevcheck") == 0)
            {
                forceFlags |= FBE_BASE_ENV_FUP_FORCE_FLAG_NO_REV_CHECK;
            }
            else if(strcmp(*argv, "singlesp") == 0) 
            {
                forceFlags |= FBE_BASE_ENV_FUP_FORCE_FLAG_SINGLE_SP_MODE;
            }
            else if(strcmp(*argv, "noenvcheck") == 0)  
            {
                forceFlags |= FBE_BASE_ENV_FUP_FORCE_FLAG_NO_ENV_CHECK;
            }
            else if(strcmp(*argv, "readimage") == 0)  
            {
                forceFlags |= FBE_BASE_ENV_FUP_FORCE_FLAG_READ_IMAGE;
            }
            else if(strcmp(*argv, "readmanifestfile") == 0)  
            {
                forceFlags |= FBE_BASE_ENV_FUP_FORCE_FLAG_READ_MANIFEST_FILE;
            }
            else if(strcmp(*argv, "noprioritycheck") == 0)  
            {
                forceFlags |= FBE_BASE_ENV_FUP_FORCE_FLAG_NO_PRIORITY_CHECK;
            }
            else if(strcmp(*argv, "nobadimagecheck") == 0)  
            {
                forceFlags |= FBE_BASE_ENV_FUP_FORCE_FLAG_NO_BAD_IMAGE_CHECK;
            }
            else if(strcmp(*argv, "all") == 0)  
            {
                forceFlags |= (FBE_BASE_ENV_FUP_FORCE_FLAG_NO_REV_CHECK |
                               FBE_BASE_ENV_FUP_FORCE_FLAG_SINGLE_SP_MODE |
                               FBE_BASE_ENV_FUP_FORCE_FLAG_NO_ENV_CHECK |
                               FBE_BASE_ENV_FUP_FORCE_FLAG_READ_IMAGE |
                               FBE_BASE_ENV_FUP_FORCE_FLAG_NO_BAD_IMAGE_CHECK |
                               FBE_BASE_ENV_FUP_FORCE_FLAG_NO_PRIORITY_CHECK |
                               FBE_BASE_ENV_FUP_FORCE_FLAG_READ_MANIFEST_FILE);
            }
            else
            {
                fbe_cli_printf("Invalid force option.\n");
                fbe_cli_printf("Please provide valid force option: norevcheck, singlesp, noenvcheck, readimage, readmanifestfile, noprioritycheck.\n");
                fbe_cli_printf("%s", FUP_USAGE);
                return;
            }
        }
        else
        {
            invalid_option = FBE_TRUE;
            fbe_cli_printf("Invalid option:%s\n",*argv);
            break;
        }
        
        argc--;
        argv++;
    }
    //Check for valid command
    if(!initiate_fup && !status_of_upgrade && !firmware_revision &&
       !abort_upgrade && !resume_upgrade && !terminate_upgrade && !get_manifest_info && !modify_time)
    {
        fbe_cli_error("\nInvalid Command.\n");
        fbe_cli_printf("%s", FUP_USAGE);
        return ;
    }
    
    if(invalid_option)
    {
        fbe_cli_error("\nInvalid Command Option\n");
        fbe_cli_printf("%s", FUP_USAGE);
        return ;
    }
    
    if(deviceType != FBE_DEVICE_TYPE_INVALID)
    {
        if(get_manifest_info)
        {
            pManifestInfo = (fbe_base_env_fup_manifest_info_t *)malloc(sizeof(fbe_base_env_fup_manifest_info_t) * FBE_BASE_ENV_FUP_MAX_SUBENCL_COUNT_IN_MANIFEST);
            if(pManifestInfo == NULL) 
            {
                fbe_cli_error("\nFailed to alloc memory to get the %s manifest info!\n",
                              fbe_base_env_decode_device_type(deviceType));

                return;
            }
            status = fbe_api_esp_common_get_fup_manifest_info(deviceType, pManifestInfo);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("\nFailed to get %s manifest info, status: 0x%x\n",
                                fbe_base_env_decode_device_type(deviceType), status);

                free(pManifestInfo);
                return ;
            }
            status = fbe_cli_print_fup_manifest_info(pManifestInfo);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("\nFailed to print out %s manifest info, status: 0x%x\n",
                                fbe_base_env_decode_device_type(deviceType), status);

            }

            free(pManifestInfo);
            return;
        }
        else if(modify_time) 
        {
            status = fbe_api_esp_common_modify_fup_delay(deviceType, delayInSec);
            if (status == FBE_STATUS_OK)
            {
                fbe_cli_printf("\nThe %s firmware upgrade will start in %d seconds.\n",
                                fbe_base_env_decode_device_type(deviceType), delayInSec);
            }
            else
            {
                fbe_cli_error("\nCan not modify %s fup delay, delay may expire, status: 0x%x.\n",
                                fbe_base_env_decode_device_type(deviceType), status);
            }
            return;
        }
        if(!location_flag)            // If the location is not given
        {
            status = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);

            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("Failed to get boardInfo, status:%d\n", status);
                return;
            }

            status = fbe_api_esp_module_mgmt_get_limits_info(&limits_info);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("Failed to get module limits_info, status:%d\n", status);
                return;
            }

            /* xPE SPS/FAN firmware upgrade */
            if (deviceType == FBE_DEVICE_TYPE_SPS || deviceType == FBE_DEVICE_TYPE_FAN)
            {
                location.bus = FBE_XPE_PSEUDO_BUS_NUM;
                location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
                status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &encl_info);
                if (status != FBE_STATUS_OK)
                {
                    fbe_cli_error("\nFailed to get encl info for  bus: %d   encl: %d  status: 0x%x\n",
                                    bus,encl,status);
                    return;
                }
                /* if not xPE, enclPresent returns false */
                if(encl_info.enclPresent)
                {
                    if(deviceType == FBE_DEVICE_TYPE_SPS)
                    {
                        slotCount = encl_info.spsCount;
                    }
                    else if(deviceType == FBE_DEVICE_TYPE_FAN)
                    {
                        slotCount = encl_info.fanCount;
                    }
                    else
                    {
                        slotCount = 0;
                    }
                }
                for ( slot = 0; slot < slotCount; slot++ )
                {
                    location.slot = slot;
                    if(initiate_fup)
                    {
                        if(!temp_flag)
                        {
                            fbe_cli_printf("\n Firmware upgrade is initiated for the devices at following locations \n");
                            fbe_cli_printf("\n Bus Encl Slot Comp  Status\n");
                            fbe_cli_printf(" ====================================\n");
                            temp_flag = FBE_TRUE;
                        }
                        getFupInfo.deviceType = deviceType;
                        getFupInfo.location = location;
                        status = fbe_api_esp_common_get_fup_info(&getFupInfo);
                        if (status != FBE_STATUS_OK){
                            fbe_cli_error("\nFailed to get fup info, status: 0x%x\n",status);
                            return ;
                        }
                        for(idx = 0; idx < getFupInfo.programmableCount; idx++)
                        {
                            location.componentId = idx;
                            status = fbe_api_esp_common_initiate_upgrade(deviceType,&location,forceFlags);
                            fbe_cli_printf(" %3d  %3d    %d    %c     %s\n",location.bus,location.enclosure,location.slot, '0'+location.componentId, (status==FBE_STATUS_OK)?"OK":"FAIL");
                        }
                    }
                    else if(status_of_upgrade)
                    {
                        if(!temp_flag)
                        {
                            fbe_cli_printf("\n Bus Encl Slot Comp Status                         Img Rev  Prev Rev  Curr Rev  Workstate\n");
                            fbe_cli_printf(" ==================================================================================== \n");
                            temp_flag = FBE_TRUE;
                        }
                        status = fbe_cli_get_fup_status(deviceType,&location);
                        //If fail to get status return the error
                        if(status != FBE_STATUS_OK)
                            return;
                    }
                    else if(firmware_revision)
                    {
                        if(!temp_flag)
                        {
                            fbe_cli_printf("\n Bus Encl Slot Comp  Revision \n");
                            fbe_cli_printf(" ===================================\n");
                            temp_flag = FBE_TRUE;
                        }
                        status = fbe_cli_get_fup_revision(deviceType,&location,eng_option);
                        //If fail to get revision return the error
                        if (status != FBE_STATUS_OK)
                            return;
                    }
                }
            }

            /* DPE or DAE firmware upgrade */
            for (bus = 0; bus < FBE_PHYSICAL_BUS_COUNT; bus ++)
            {
                location.bus = bus;

                //only DPE enclosure 0_0 has SP board
                if(deviceType == FBE_DEVICE_TYPE_SP && bus>0)
                {
                    continue;
                }

                if(deviceType == FBE_DEVICE_TYPE_BACK_END_MODULE && bus>0) //only enclosure 0_0 has bem
                {
                    continue;
                }
                status = fbe_api_esp_encl_mgmt_get_encl_count_on_bus(&location, &enclCntOnBus);
                if(status != FBE_STATUS_OK)
                {
                    fbe_cli_error("\nFailed to get encl count on bus %d status 0x%x.\n",
                                        bus, status);
                    return;    
                }

                for ( encl = 0; encl < enclCntOnBus; encl++ )
                {
                    location.bus = bus;
                    location.enclosure = encl;
            
                    status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &encl_info);
                    if (status != FBE_STATUS_OK)
                    {
                        fbe_cli_error("\nFailed to get encl info for  bus: %d   encl: %d  status: 0x%x\n",
                                        bus,encl,status);
                        return;
                    }

                    if(encl_info.enclPresent) 
                    {
                        if(deviceType == FBE_DEVICE_TYPE_LCC)
                        {
                            slotCount = encl_info.lccCount;
                            componentCount = encl_info.subEnclCount;
                        }
                        else if(deviceType == FBE_DEVICE_TYPE_PS) 
                        {
                            slotCount = encl_info.psCount;
                        }
                        else if(deviceType == FBE_DEVICE_TYPE_SPS)
                        {
                            slotCount = encl_info.spsCount;
                        }
                        else if(deviceType == FBE_DEVICE_TYPE_FAN) 
                        {
                            slotCount = encl_info.fanCount;
                        }
                        else if(deviceType == FBE_DEVICE_TYPE_BACK_END_MODULE) 
                        {
                            if (encl>0) //only enclosure 0_0 has bem
                            {
                                continue;
                            }
                            slotCount = limits_info.discovered_hw_limit.num_bem;
                        }
                        else if(deviceType == FBE_DEVICE_TYPE_SP) 
                        {
                            if (encl>0) //only enclosure 0_0 has bem
                            {
                                continue;
                            }
                            slotCount = 1;
                        }
                        else
                        {
                            slotCount = 0;
                        }

                        for ( slot = 0; slot < slotCount; slot++ )
                        {
                            location.bus = bus;
                            location.enclosure = encl;
                            location.slot = slot;
                            if ((deviceType == FBE_DEVICE_TYPE_BACK_END_MODULE) ||
                               (deviceType == FBE_DEVICE_TYPE_SP))
                            {
                                location.sp = boardInfo.sp_id;
                            }

                            if(initiate_fup)
                            {
                                if((boardInfo.platform_info.enclosureType == DPE_ENCLOSURE_TYPE) &&
                                    (deviceType != FBE_DEVICE_TYPE_BACK_END_MODULE && deviceType != FBE_DEVICE_TYPE_SP) &&
                                    (bus == 0 && encl == 0))
                                {
                                    continue; 
                                }

                                if(!temp_flag)
                                {
                                    fbe_cli_printf("\n Firmware upgrade is initiated for the devices at following locations \n");
                                    fbe_cli_printf("\n Bus Encl Slot Comp SP         Status\n");
                                    fbe_cli_printf(" ====================================\n");
                                    temp_flag = FBE_TRUE;
                                }

                                getFupInfo.deviceType = deviceType;
                                getFupInfo.location = location;
                                status = fbe_api_esp_common_get_fup_info(&getFupInfo);
                                if (status != FBE_STATUS_OK){
                                    fbe_cli_error("\nFailed to get fup info, status: 0x%x\n",status);
                                    return ;
                                }
                                       
                                if (deviceType == FBE_DEVICE_TYPE_LCC)
                                {
                                    for(idx=0;idx<getFupInfo.programmableCount;idx++)
                                    {
                                        /* Get the component id */
                                        location.componentId = getFupInfo.fupInfo[idx].componentId;

                                        if (location.componentId != 0) 
                                        {
                                            status = fbe_api_esp_common_initiate_upgrade(deviceType, &location, forceFlags); 
                                            fbe_cli_printf(" %3d  %3d  %3d   %c             %s\n",location.bus,location.enclosure,location.slot, ((location.componentId==0)?' ':('0'+location.componentId)), (status==FBE_STATUS_OK)?"OK":"FAIL");
                                        }
                                    }
                                    /* We would like to do the upgrade for DRVSXP or EE first and then IOSXP or ICM.
                                     * So queuing up the LCC upgrade request for IOSXP or ICM after DRVSXP or EE can reduce the chance for bugs.
                                     */
                                    location.componentId = 0;
                                    status = fbe_api_esp_common_initiate_upgrade(deviceType, &location, forceFlags); 
                                    fbe_cli_printf(" %3d  %3d  %3d   %c             %s\n",location.bus,location.enclosure,location.slot, ((location.componentId==0)?' ':('0'+location.componentId)), (status==FBE_STATUS_OK)?"OK":"FAIL");
                                }
                                else if (deviceType == FBE_DEVICE_TYPE_SPS)
                                {
                                    for(idx = 0; idx < getFupInfo.programmableCount; idx++)
                                    {
                                        location.componentId = idx;
                                        status = fbe_api_esp_common_initiate_upgrade(deviceType,&location,forceFlags);
                                        fbe_cli_printf(" %3d  %3d  %3d   %c             %s\n",location.bus,location.enclosure,location.slot, '0'+location.componentId, (status==FBE_STATUS_OK)?"OK":"FAIL");
                                    }
                                }
                                else if (deviceType == FBE_DEVICE_TYPE_SP || deviceType == FBE_DEVICE_TYPE_BACK_END_MODULE)
                                {
                                    status = fbe_api_esp_common_initiate_upgrade(deviceType,&location,forceFlags);
                                    fbe_cli_printf(" %3d  %3d  %3d       %d         %s\n",location.bus,location.enclosure,location.slot, location.sp, (status==FBE_STATUS_OK)?"OK":"FAIL");
                                }
                                else
                                {
                                    status = fbe_api_esp_common_initiate_upgrade(deviceType,&location,forceFlags);
                                    fbe_cli_printf(" %3d  %3d  %3d   %c             %s\n",location.bus,location.enclosure,location.slot, ((location.componentId==0)?' ':('0'+location.componentId)), (status==FBE_STATUS_OK)?"OK":"FAIL");
                                }
                            }
                            else if(status_of_upgrade)
                            {
                                if(!temp_flag)
                                {
                                    fbe_cli_printf("\n Bus Encl Slot Comp SP Status                         Img Rev  Prev Rev  Curr Rev  Workstate\n");
                                    fbe_cli_printf(" =========================================================================================== \n");
                                    temp_flag = FBE_TRUE;
                                }
                                status = fbe_cli_get_fup_status(deviceType,&location);
                                //If fail to get status return the error
                                if(status != FBE_STATUS_OK)
                                    return;
                            }
                            else if(firmware_revision)
                            {
                                if(!temp_flag)
                                {
                                    fbe_cli_printf("\n Bus Encl Slot Comp  SP   Revision \n");
                                    fbe_cli_printf(" =================================\n");
                                    temp_flag = FBE_TRUE;
                                }


                                if (deviceType == FBE_DEVICE_TYPE_SP || deviceType == FBE_DEVICE_TYPE_BACK_END_MODULE)
                                {
                                    if (boardInfo.isSingleSpSystem == TRUE)
                                    {
                                        spCount = SP_COUNT_SINGLE;
                                    }
                                    else
                                    {
                                        spCount = SP_ID_MAX;
                                    }

                                    for (idx=0;idx<spCount;idx++)
                                    {
                                        location.sp=idx;
                                        status = fbe_cli_get_fup_revision(deviceType,&location,eng_option);
                                        //If fail to get revision return the error
                                        if (status != FBE_STATUS_OK)
                                            return;
                                    }
                                }
                                else
                                {
                                    status = fbe_cli_get_fup_revision(deviceType,&location,eng_option);
                                    //If fail to get revision return the error
                                    if (status != FBE_STATUS_OK)
                                        return;
                                }
                            }
                        }//End of loop for slot
                    }
                }//End of loop for encl
            }//End of loop for bus
           
        }//End of location not given
        else // Location is given
        {
            if ((deviceType == FBE_DEVICE_TYPE_SP || deviceType == FBE_DEVICE_TYPE_BACK_END_MODULE) && sp_flag == FBE_FALSE)
            {
                fbe_cli_error("\n-sp is needed for device type sp and bm.\n");
                return ;
            }

            status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &encl_info);
            
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("\nFailed to get encl info for bus: %d  encl: %d  status: 0x%x\n",
                               location.bus, location.enclosure, status);
                return ;
            }
            if(encl_info.enclState == FBE_ESP_ENCL_STATE_OK)// If the state is ready perform the operation
            {
                if(initiate_fup)
                {
                    fbe_cli_printf("\n Firmware upgrade is initiated for the devices at following locations \n");
                    fbe_cli_printf("\n Bus Encl Slot Comp SP         Status\n");
                    fbe_cli_printf(" ====================================\n");

                    if (deviceType == FBE_DEVICE_TYPE_LCC)
                    {
                        location.sp=0;
                        status = fbe_api_esp_common_initiate_upgrade(deviceType,&location,forceFlags);
                        fbe_cli_printf(" %3d  %3d  %3d   %c             %s\n",location.bus,location.enclosure,location.slot, ((location.componentId==0)?' ':('0'+location.componentId)), (status==FBE_STATUS_OK)?"OK":"FAIL");
                    }
                    else if (deviceType == FBE_DEVICE_TYPE_SP || deviceType == FBE_DEVICE_TYPE_BACK_END_MODULE)
                    {
                        location.bus = 0;
                        location.enclosure = 0;
                        location.slot= 0;

                        status = fbe_api_esp_common_initiate_upgrade(deviceType,&location,forceFlags);
                        fbe_cli_printf(" %3d  %3d  %3d       %d         %s\n",location.bus,location.enclosure,location.slot, location.sp, (status==FBE_STATUS_OK)?"OK":"FAIL");
                    }
                    else
                    {
                        status = fbe_api_esp_common_initiate_upgrade(deviceType,&location,forceFlags);
                        fbe_cli_printf(" %3d  %3d  %3d   %c             %s\n",location.bus,location.enclosure,location.slot, ((location.componentId==0)?' ':('0'+location.componentId)), (status==FBE_STATUS_OK)?"OK":"FAIL");
                    }
                }
                else if(status_of_upgrade)
                {
                    fbe_cli_printf(" \nFirmware upgrade status for the device at the given location \n");
                    fbe_cli_printf("\n Bus Encl Slot Comp SP Status                         Img Rev  Prev Rev  Curr Rev  Workstate\n");
                    fbe_cli_printf(" =========================================================================================== \n");
                    status = fbe_cli_get_fup_status(deviceType,&location);
                }
                else if(firmware_revision)
                {
                    fbe_cli_printf("\n Firmware revision for the device at the given location \n");
                    fbe_cli_printf("\n Bus Encl Slot Comp  SP   Revision \n");
                    fbe_cli_printf(" =================================\n");
                    status = fbe_cli_get_fup_revision(deviceType,&location,eng_option);
                }
            }
            else
            {
                fbe_cli_error("\n Enclosure object is not in ready state\n");
                return ;
            }
            
        }//End of location given
        
        
    }// valid device type is given
    else if(!abort_upgrade && !resume_upgrade && !terminate_upgrade)// valid device type is not given
    {
        fbe_cli_printf("Please provide valid device type: lcc, ps or fan.\n");
        fbe_cli_printf("%s", FUP_USAGE);
        return;
    
    }
    if(abort_upgrade)
    {
        status = fbe_api_esp_common_abort_upgrade();
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_error("\nFailed to abort firmware upgrade status: 0x%x\n",
                            status);
            return ;
        }
        fbe_cli_printf(" \n Firmware upgrade is aborted for all devices \n");
    }
    else if(resume_upgrade)
    {
       status = fbe_api_esp_common_resume_upgrade();
       if (status != FBE_STATUS_OK)
        {
            fbe_cli_error("\nFailed to resume firmware upgrade status: 0x%x\n",
                            status);
            return ;
        }
        fbe_cli_printf(" \nFirmware upgrade is resumed for all devices \n");
    }
    else if(terminate_upgrade)
    {
       status = fbe_api_esp_common_terminate_upgrade();
       if (status != FBE_STATUS_OK)
        {
            fbe_cli_error("\nFailed to terminate upgrade, status: 0x%x\n",
                            status);
            return ;
        }
        fbe_cli_printf(" \nFirmware upgrade is terminated for all devices \n");
    }

    return ;
}

/*!***************************************************************
 * @fn fbe_cli_get_fup_status(fbe_u64_t deviceType,
 *                                      fbe_device_physical_location_t * pLocation)
 ****************************************************************
 * @brief
 *  This function gets the firmware upgrade status on the device type.
 *
 * @param deviceType - 
 * @param pLocation (OUTPUT) -
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  04-October-2010: Vishnu Sharma - Created.
 *
 ****************************************************************/

fbe_status_t fbe_cli_get_fup_status(fbe_u64_t deviceType, fbe_device_physical_location_t *pLocation)
{
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_base_env_fup_work_state_t    workState = FBE_BASE_ENV_FUP_WORK_STATE_NONE;
    fbe_esp_base_env_get_fup_info_t getFupInfo = {0};
    fbe_u8_t idx;
    fbe_u8_t componentId = 0;
    fbe_u8_t *imageRev, *preFirmwareRev, *currentFirmwareRev;

    getFupInfo.deviceType = deviceType;
    getFupInfo.location = *pLocation;
    status = fbe_api_esp_common_get_fup_info(&getFupInfo);
    if (status != FBE_STATUS_OK){
        fbe_cli_error("\nFailed to get fup info, status: 0x%x\n",status);
        return status;
    }

    for(idx=0;idx<getFupInfo.programmableCount;idx++)
    {
        /* Get the component id */
        componentId = getFupInfo.fupInfo[idx].componentId;

        imageRev = getFupInfo.fupInfo[idx].imageRev;
        preFirmwareRev = getFupInfo.fupInfo[idx].preFirmwareRev;
        currentFirmwareRev = getFupInfo.fupInfo[idx].currentFirmwareRev;
        if(strcmp(imageRev, "") == 0)
        {
            imageRev = "-- ";
        }
        if(strcmp(preFirmwareRev, "") == 0)
        {
            preFirmwareRev = "-- ";
        }
        if(strcmp(currentFirmwareRev, "") == 0)
        {
            currentFirmwareRev = "-- ";
        }
        if (deviceType == FBE_DEVICE_TYPE_SP || deviceType == FBE_DEVICE_TYPE_BACK_END_MODULE)
        {
            fbe_cli_printf(" %3d  %3d  %3d       %d %-33s%6s   %6s   %6s  ",
                        pLocation->bus,pLocation->enclosure,pLocation->slot, pLocation->sp,
                        fbe_base_env_decode_fup_completion_status(getFupInfo.fupInfo[idx].completionStatus),
                        imageRev, preFirmwareRev, currentFirmwareRev);
        }
        else
        {
            fbe_cli_printf(" %3d  %3d  %3d   %c     %-33s%6s   %6s   %6s  ",
                        pLocation->bus,pLocation->enclosure,pLocation->slot, '0'+ componentId,
                        fbe_base_env_decode_fup_completion_status(getFupInfo.fupInfo[idx].completionStatus),
                        imageRev, preFirmwareRev, currentFirmwareRev);
        }

        workState = getFupInfo.fupInfo[idx].workState;
        if(workState == FBE_BASE_ENV_FUP_WORK_STATE_WAIT_BEFORE_UPGRADE) 
        {
            fbe_cli_printf(" %s(%d seconds left)",fbe_base_env_decode_fup_work_state(workState), getFupInfo.fupInfo[idx].waitTimeBeforeUpgrade);
        }
        else
        {
            fbe_cli_printf(" %s",fbe_base_env_decode_fup_work_state(workState));
        }
        fbe_cli_printf(" \n");
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_cli_get_fup_revision(fbe_u64_t deviceType,
 *                                      fbe_device_physical_location_t * pLocation)
 ****************************************************************
 * @brief
 *  This function gets the firmware revision  of the device type.
 *
 * @param deviceType - 
 * @param pLocation (OUTPUT) -
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  04-October-2010: Vishnu Sharma - Created.
 *
 ****************************************************************/

fbe_status_t fbe_cli_get_fup_revision(fbe_u64_t deviceType, fbe_device_physical_location_t *pLocation, fbe_bool_t eng_option)
{
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_esp_encl_mgmt_lcc_info_t lccInfo = {0};
    fbe_esp_ps_mgmt_ps_info_t psInfo = {0};
    fbe_esp_board_mgmt_board_info_t   boardInfo = {0};
    fbe_esp_module_io_module_info_t  io_module_info = {0};
    fbe_esp_base_env_get_fup_info_t getFupInfo = {0};
    fbe_esp_sps_mgmt_spsStatus_t spsStatusInfo = {0};
    fbe_esp_cooling_mgmt_fan_info_t fanInfo = {0};
    fbe_u8_t idx;

    getFupInfo.deviceType = deviceType;
    getFupInfo.location = *pLocation;
    status = fbe_api_esp_common_get_fup_info(&getFupInfo);
    if (status != FBE_STATUS_OK){
        fbe_cli_error("\nFailed to get fup info, status: 0x%x\n",status);
        return status;
    }

    if(deviceType == FBE_DEVICE_TYPE_PS)
    {
        psInfo.location = *pLocation;
        status = fbe_api_esp_ps_mgmt_getPsInfo(&psInfo);
        if (status != FBE_STATUS_OK){
            fbe_cli_error("\nFailed to get PS info for its rev  status: 0x%x\n",status);
            return status ;
        }
        fbe_cli_printf(" %3d  %3d  %3d   %d        %s\n",
                       pLocation->bus,pLocation->enclosure,pLocation->slot,pLocation->componentId, psInfo.psInfo.firmwareRev);
    }
    else if(deviceType == FBE_DEVICE_TYPE_LCC)
    {
        for(idx=0;idx<getFupInfo.programmableCount;idx++)
        {
            /* Get the component id */
            pLocation->componentId = getFupInfo.fupInfo[idx].componentId;

            status = fbe_api_esp_encl_mgmt_get_lcc_info(pLocation, &lccInfo);
            if (status != FBE_STATUS_OK){
                fbe_cli_error("\nFailed to get LCC info for its rev  status: 0x%x\n",status);
                return status;
            }
            if(lccInfo.inserted)
            {
                if((lccInfo.esesRevision == ESES_REVISION_2_0) && eng_option)
                {
                    fbe_cli_printf(" %3d  %3d  %3d   %c      (CDES)%s (CDEF)%s (IStr)%s\n",
                                   pLocation->bus,
                                   pLocation->enclosure,
                                   pLocation->slot, 
                                   '0'+pLocation->componentId,
                                   lccInfo.firmwareRev,
                                   lccInfo.fpgaFirmwareRev,
                                   lccInfo.initStrFirmwareRev);
                }
                else
                {
                    fbe_cli_printf(" %3d  %3d  %3d   %c        %s\n",
                                pLocation->bus,pLocation->enclosure,pLocation->slot, '0'+pLocation->componentId, lccInfo.firmwareRev);
                }
            }
        }
    }
    else if (deviceType == FBE_DEVICE_TYPE_SP)
    {
        status = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_error("\nFailed to get SP board CDES firmware rev, status: 0x%x\n",status);
            return status;
        }
        if(boardInfo.lccInfo[pLocation->sp].inserted && !boardInfo.lccInfo[pLocation->sp].faulted)
        {
            if((boardInfo.lccInfo[pLocation->sp].eses_version_desc == ESES_REVISION_2_0) && eng_option)
            {
                fbe_cli_printf(" %3d  %3d  %3d       %d    (CDES)%s (CDEF)%s (IStr)%s\n",
                               pLocation->bus,
                               pLocation->enclosure,
                               pLocation->slot,
                               pLocation->sp,
                               boardInfo.lccInfo[pLocation->sp].firmwareRev,
                               boardInfo.lccInfo[pLocation->sp].fpgaFirmwareRev,
                               boardInfo.lccInfo[pLocation->sp].initStrFirmwareRev);
            }
            else
            {
                fbe_cli_printf(" %3d  %3d  %3d       %d    %s\n",
                            pLocation->bus,pLocation->enclosure,pLocation->slot, pLocation->sp, boardInfo.lccInfo[pLocation->sp].firmwareRev);
            }
        }
    }
    else if (deviceType == FBE_DEVICE_TYPE_BACK_END_MODULE)
    {
        io_module_info.header.sp = pLocation->sp;
        io_module_info.header.type = FBE_DEVICE_TYPE_BACK_END_MODULE;
        io_module_info.header.slot = pLocation->slot;

        status = fbe_api_esp_module_mgmt_getIOModuleInfo(&io_module_info);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_error("\nFailed to get Base Module CDES rev  status: 0x%x\n",status);
            return status;
        }

        if(io_module_info.io_module_phy_info.lccInfo.inserted && !io_module_info.io_module_phy_info.lccInfo.faulted)
        {
            if((io_module_info.io_module_phy_info.lccInfo.eses_version_desc == ESES_REVISION_2_0) && eng_option)
            {
                fbe_cli_printf(" %3d  %3d  %3d       %d    (CDES)%s (CDEF)%s (IStr)%s\n",
                               pLocation->bus,
                               pLocation->enclosure,
                               pLocation->slot,
                               pLocation->sp,
                               io_module_info.io_module_phy_info.lccInfo.firmwareRev,
                               io_module_info.io_module_phy_info.lccInfo.fpgaFirmwareRev,
                               io_module_info.io_module_phy_info.lccInfo.initStrFirmwareRev);
            }
            else
            {
                fbe_cli_printf(" %3d  %3d  %3d       %d    %s\n",
                            pLocation->bus,pLocation->enclosure,pLocation->slot, pLocation->sp, io_module_info.io_module_phy_info.lccInfo.firmwareRev);
            }
        }
    }
    else if(deviceType == FBE_DEVICE_TYPE_SPS)
    {
        spsStatusInfo.spsLocation = *pLocation;
        status = fbe_api_esp_sps_mgmt_getSpsStatus(&spsStatusInfo);
        if (status != FBE_STATUS_OK){
            fbe_cli_error("\nFailed to get SPS info for its rev status: 0x%x\n",status);
            return status;
        }
        if(spsStatusInfo.spsModuleInserted)
        {
            fbe_cli_printf(" %3d  %3d    %d    %d    %s\n",
                        pLocation->bus,pLocation->enclosure,pLocation->slot, 0, spsStatusInfo.primaryFirmwareRev);
            fbe_cli_printf(" %3d  %3d    %d    %d    %s\n",
                        pLocation->bus,pLocation->enclosure,pLocation->slot, 1, spsStatusInfo.secondaryFirmwareRev);
        }
        if(spsStatusInfo.spsBatteryInserted)
        {
            fbe_cli_printf(" %3d  %3d    %d    %d    %s\n",
                        pLocation->bus,pLocation->enclosure,pLocation->slot, 2, spsStatusInfo.batteryFirmwareRev);
        }
    }
    else if(deviceType == FBE_DEVICE_TYPE_FAN)
    {
        status = fbe_api_esp_cooling_mgmt_get_fan_info(pLocation, &fanInfo);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_error("\nFailed to get FAN info for its rev status: 0x%x\n",status);
        }
        if(fanInfo.inserted == FBE_MGMT_STATUS_TRUE)
        {
            fbe_cli_printf(" %3d  %3d    %d    %d    %s\n",
                        pLocation->bus,pLocation->enclosure,pLocation->slot, pLocation->componentId, fanInfo.firmwareRev);
        }
    }
    return status;
}

/*!***************************************************************
 * @fn fbe_cli_print_fup_manifest_info(fbe_base_env_fup_manifest_info_t * pManifestInfo)
 ****************************************************************
 * @brief
 *  This function prints out the manifest info.
 *
 * @param pManifestInfo - 
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  18-Sep-2014: PHE - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_cli_print_fup_manifest_info(fbe_base_env_fup_manifest_info_t * pManifestInfo)
{
    fbe_u32_t subEnclIndex = 0;
    fbe_u32_t imageIndex = 0;

    if(pManifestInfo->subenclProductId[0] == ' ')
    {
        fbe_cli_printf(" CDES-2 manifest file is not present or the parsing of the file failed!\n");

        return FBE_STATUS_OK;
    }

    fbe_cli_printf(" subEnclProdId      \tImageFileName\timageRev\tfwCompType\tfwTarget\n");

    fbe_cli_printf(" ===============================================================================\n");

    for(subEnclIndex = 0; subEnclIndex < FBE_BASE_ENV_FUP_MAX_SUBENCL_COUNT_IN_MANIFEST; subEnclIndex ++) 
    {
        if(pManifestInfo->subenclProductId[0] == ' ')
        {
            break;
        }

        for(imageIndex = 0; imageIndex < pManifestInfo->imageFileCount; imageIndex ++)
        {
            fbe_cli_printf(" %-16s\t%s\t%-16s\t%d\t%s\n",
                           &pManifestInfo->subenclProductId[0],
                           &pManifestInfo->imageFileName[imageIndex][0],
                           &pManifestInfo->imageRev[imageIndex][0],
                           pManifestInfo->firmwareCompType[imageIndex],
                           fbe_base_env_decode_firmware_target(pManifestInfo->firmwareTarget[imageIndex]));
        }
        fbe_cli_printf("\n");
        pManifestInfo ++;
    }

    return FBE_STATUS_OK;
}

/************************************************************************
 **                       End of fbe_cli_cmd_firmware_upgrade ()                **
 ************************************************************************/
