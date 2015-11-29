/************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ************************************************************************/

/*!***********************************************************************
 * @file fbe_cli_lib_sfp_info_cmds.c
 *************************************************************************
 *
 * @brief
 *  This file contains FBE cli command for SFP information.
 *
 * @ingroup fbe_cli
 *
 * @revision
 *  25-July-2011:  Harshal Wanjari - created
 *  2-May-2012:    Feng Ling - Modified
 *
 ************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_cli_private.h"
#include "fbe_cli_lib_sfp_info_cmds.h"
#include "fbe/fbe_api_esp_board_mgmt_interface.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_esp_module_mgmt_interface.h"


static void fbe_cli_display_sfp_supported_protocols(fbe_sfp_protocol_t supported_protocols);
/*************************************************************************
 *                            @fn fbe_cli_cmd_sfp_info ()                                                           *
 *************************************************************************
 *
 *  @brief
 *   fbe_cli_cmd_sfp_info performs the execution of the SFPINFO command
 *   Display SFP Information
 *
 * @param :
 *   argc     (INPUT) NUMBER OF ARGUMENTS
 *   argv     (INPUT) ARGUMENT LIST
 *
 * @return:
 *   None
 *
 * @author
 *   25-July-2011:  Harshal Wanjari - created
 *   2-May-2012:    Feng Ling - Modified
 *
 *************************************************************************/

void fbe_cli_cmd_sfp_info(fbe_s32_t argc,char ** argv)
{
	fbe_u32_t                                   slot_num_index;
	fbe_u32_t                                   port_num_index;
	fbe_bool_t                                  device_flag =FBE_FALSE;
	fbe_bool_t                                  slot_flag =FBE_FALSE;
	fbe_bool_t                                  port_flag =FBE_FALSE;
	fbe_status_t                                status = FBE_STATUS_OK;
	fbe_base_env_sp_t                           local_sp = FBE_BASE_ENV_INVALID;
	fbe_u32_t                                   which_sp;
    fbe_esp_module_limits_info_t                limits_info = {0};
    fbe_u32_t                                   max_port = 0;
	fbe_u32_t                                   max_slot = 0;
	fbe_esp_module_io_module_info_t             esp_io_module_info = {0};
    fbe_u32_t                                   device_type;

    if ((argc == 1 )&&((strcmp(*argv, "-help") == 0) 
	   || (strcmp(*argv, "-h") == 0)))
    {
        // If they are asking for help, just display help and exit.          
        fbe_cli_printf("\n");
        fbe_cli_printf("%s", SFP_INFO_USAGE);
        return;
    }

    while (argc >0)
	{
		if((strcmp(*argv, "-d")) == 0)
		{
			argc--;
            argv++;             // move to the device type
            if(argc == 0)
            {
                fbe_cli_printf("Please provide device type.\n");
                return;
            }
			if(strcmp(*argv, "mezz") == 0)
            {
                device_type = FBE_DEVICE_TYPE_MEZZANINE;
            }
			else if(strcmp(*argv, "iom") == 0)
            {
                device_type = FBE_DEVICE_TYPE_IOMODULE;
            }
			else if(strcmp(*argv, "bem") == 0)
            {
                device_type = FBE_DEVICE_TYPE_BACK_END_MODULE;
            }
			else 
			{
				fbe_cli_printf("Invalid device type.\n");
				fbe_cli_printf("It should be mezz, iom or bem.\n");
				return;
			}
			device_flag = FBE_TRUE;
			argc--;
            argv++;
		}
		else if ((strcmp(*argv, "-s"))== 0) 
		{
			argc--;
            argv++;             // move to the slot number
            if(argc == 0)
            {
                fbe_cli_printf("Please provide slot number.\n");
                return;
            }
			slot_num_index = atoi(*argv);

			//get and check limits_info
	        status = fbe_api_esp_module_mgmt_get_limits_info(&limits_info);

            if(status != FBE_STATUS_OK)
            {
                fbe_cli_error("%s: Unable to get Limits info",__FUNCTION__);
                return;
            }
			if(device_type == FBE_DEVICE_TYPE_MEZZANINE)
	        {
                max_slot = limits_info.discovered_hw_limit.num_mezzanine_slots;
                if(max_slot == 0)
		        {
			        fbe_cli_printf("No slots for FBE_DEVICE_TYPE_MEZZANINE.\n");
			        return;
		        }             
		        if (slot_num_index > (max_slot - 1) )
		        {
			        if(max_slot == 1)
				    fbe_cli_printf("Invalid slot number, it should be 0.\n");
			        else
			        fbe_cli_printf("Invalid slot number, it should be 0 to %d.\n", 
				                   max_slot - 1);
			        return;
		        }               
	        }
	        else if(device_type == FBE_DEVICE_TYPE_IOMODULE)
	        {
                max_slot = limits_info.discovered_hw_limit.num_slic_slots;
                if(max_slot == 0)
		        {
			        fbe_cli_printf("No slots for IO Module.\n");
			        return;
		        }             
		        if (slot_num_index > (max_slot - 1))
		        {
			        fbe_cli_printf("Invalid slot number, it should be 0 to %d.\n", 
				                   max_slot - 1);
			        return;
		        }             
	        }
	        else
	        {
                max_slot = limits_info.discovered_hw_limit.num_bem;
                if(max_slot == 0)
		        {
			        fbe_cli_printf("No slots for BEM Module.\n");
			        return;
		        }             
		        if (slot_num_index > (max_slot - 1))
		        {
			        fbe_cli_printf("Invalid slot number, it should be 0 to %d.\n", 
				                   max_slot - 1);
			        return;
		        }             
	        }
			slot_flag = FBE_TRUE;
			argc--;
            argv++;
		}
		else if ((strcmp(*argv, "-p")) == 0)
		{
			argc--;
            argv++;             // move to the slot number
            if(argc == 0)
            {
                fbe_cli_printf("Please provide port number.\n");
                return;
            }
			port_num_index = atoi(*argv);

			if((device_flag == FBE_FALSE) || (slot_flag == FBE_FALSE))
			{
				fbe_cli_printf("Please provide both device type and slot number.\n");
				return;
			}
	        status = fbe_api_esp_common_get_sp_id(FBE_CLASS_ID_MODULE_MGMT,&local_sp);
            which_sp = (local_sp ? FBE_BASE_ENV_SPB : FBE_BASE_ENV_SPA);
			//get and check mezz or slic info
            esp_io_module_info.header.sp = which_sp;
            esp_io_module_info.header.type = device_type;
            esp_io_module_info.header.slot = slot_num_index;
			status = fbe_api_esp_module_mgmt_getIOModuleInfo(&esp_io_module_info);
            max_port = esp_io_module_info.io_module_phy_info.ioPortCount;
            if(max_port == 0)
	        {
		        fbe_cli_printf("No ports for slot %d\n",
			                   slot_num_index);
	            return;
	        }
	        if (port_num_index > (max_port - 1))
	        {
		        fbe_cli_printf("Invalid port number, it should be 0 to %d.\n",
			                   max_port - 1);
	            return;
	        }
			port_flag = FBE_TRUE;
			argc--;
            argv++;
		}
		else // invalid arguments
        {
            fbe_cli_printf("Invalid arguments. Please check the command. \n");
            fbe_cli_printf("%s", SFP_INFO_USAGE);
            return;
		}
	}

    //get and check SPID
	status = fbe_api_esp_common_get_sp_id(FBE_CLASS_ID_MODULE_MGMT,&local_sp);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error ( "%s:Failed to get SPID status:%d\n", 
            __FUNCTION__, status);
        return;
    }

    which_sp = (local_sp ? FBE_BASE_ENV_SPB : FBE_BASE_ENV_SPA);
    fbe_cli_printf("\nInformation on SFP for %s.\n",which_sp ? "SPB" : "SPA");

	if((device_flag == FBE_TRUE) && (slot_flag == FBE_TRUE) 
		&& (port_flag == FBE_TRUE))
    {
        fbe_cli_specific_port_sfp_info(device_type, 
			                           slot_num_index, 
								       port_num_index);
    }
    else if((device_flag == FBE_TRUE) && (slot_flag == FBE_TRUE))
    {
        fbe_cli_specific_slot_sfp_info(device_type, 
			                           slot_num_index);
    }
    else if(device_flag == FBE_TRUE)
    {
        fbe_cli_specific_device_sfp_info(device_type);
    }
    else 
    {
       fbe_cli_all_sfp_info();
    }
    return;
}

/*************************************************************************
 **                       End of fbe_cli_cmd_sfp_info ()                **
 ************************************************************************/
 
 /*!**********************************************************************
 * fbe_cli_specific_port_sfp_info()
 *************************************************************************
 * @brief
 *      Prints sfp information of a specific port. 
 *      Called by FBE cli sfpinfo command
 *
 *  @param :
 *      fbe_u32_t                       device_type
 *      fbe_u32_t                       slot_num_index
 *      fbe_u32_t                       port_num_index
 *
 *  @return:
 *      Nothing.
 *
 * @author
 *   2-May-2012:    Feng Ling - Created
 *
 ************************************************************************/
static void fbe_cli_specific_port_sfp_info(fbe_u32_t device_type,
								           fbe_u32_t slot_num_index,
								           fbe_u32_t port_num_index)
{
    fbe_status_t                             status = FBE_STATUS_OK;
	fbe_base_env_sp_t                        local_sp = FBE_BASE_ENV_INVALID;
	fbe_u32_t                                which_sp;
    fbe_esp_module_limits_info_t             limits_info = {0};
    fbe_u32_t                                max_port = 0;
	fbe_u32_t                                max_slot = 0;
    fbe_esp_module_sfp_info_t                sfp_info;
    fbe_status_t                             module_info_status = FBE_STATUS_OK;
	fbe_esp_module_io_module_info_t          esp_io_module_info = {0};
	
	//get SPID
	status = fbe_api_esp_common_get_sp_id(FBE_CLASS_ID_MODULE_MGMT,&local_sp);	
	which_sp = (local_sp ? FBE_BASE_ENV_SPB : FBE_BASE_ENV_SPA);

	//get limits info
	status = fbe_api_esp_module_mgmt_get_limits_info(&limits_info);

    //get max slot counts and max ports counts for a specific diveci type
    esp_io_module_info.header.sp = which_sp;
    esp_io_module_info.header.type = device_type;
    esp_io_module_info.header.slot = slot_num_index;
    status = fbe_api_esp_module_mgmt_getIOModuleInfo(&esp_io_module_info);
    max_port = esp_io_module_info.io_module_phy_info.ioPortCount;
    if(max_port == 0)
    {
        return;
    }
    if(device_type == FBE_DEVICE_TYPE_MEZZANINE)
	{
        max_slot = limits_info.discovered_hw_limit.num_mezzanine_slots;
		fbe_cli_printf("\nDevice Type: Mezzanine.\n");
	}
	else if(device_type == FBE_DEVICE_TYPE_IOMODULE)
	{
        max_slot = limits_info.discovered_hw_limit.num_slic_slots;
        fbe_cli_printf("\nDevice Type: IO Module.\n");
	}
	else
	{
        max_slot = limits_info.discovered_hw_limit.num_bem;
        fbe_cli_printf("\nDevice Type: BEM Module.\n");
	}
	if (slot_num_index < max_slot)
	{
		fbe_cli_printf("\nSlot Number: %d\n",slot_num_index);
		if (port_num_index < max_port)
		{
			fbe_cli_printf("\nPort Number: %d\n",port_num_index);
		    sfp_info.header.sp = which_sp;
            sfp_info.header.type = device_type;
            sfp_info.header.slot = slot_num_index;
            sfp_info.header.port = port_num_index;

            //collecting sfp information
            module_info_status = fbe_api_esp_module_mgmt_get_sfp_info(&sfp_info);
            fbe_cli_print_sfp_info(&sfp_info);
		}				   
	}
}
/*************************************************************************
 * end fbe_cli_specific_port_sfp_info() 
 ************************************************************************/
 
 /*!**********************************************************************
 * fbe_cli_specific_slot_sfp_info()
 *************************************************************************
 * @brief
 *      Prints all sfp information of a specific slot. 
 *      Called by FBE cli sfpinfo command
 *
 *  @param :
 *      fbe_u32_t                       device_type
 *      fbe_u32_t                       slot_num_index
 *
 *  @return:
 *      Nothing.
 *
 * @author
 *   2-May-2012:    Feng Ling - Created
 *
 ************************************************************************/

static void fbe_cli_specific_slot_sfp_info(fbe_u32_t device_type,
								           fbe_u32_t slot_num_index)
{
	fbe_status_t                             status = FBE_STATUS_OK;
	fbe_u32_t                                port_num_index;
	fbe_u32_t                                max_port = 0;
	fbe_esp_module_io_module_info_t          esp_io_module_info = {0};
	fbe_base_env_sp_t                        local_sp = FBE_BASE_ENV_INVALID;
	fbe_u32_t                                which_sp;

	status = fbe_api_esp_common_get_sp_id(FBE_CLASS_ID_MODULE_MGMT,&local_sp);	
	which_sp = (local_sp ? FBE_BASE_ENV_SPB : FBE_BASE_ENV_SPA);

    esp_io_module_info.header.sp = which_sp;
    esp_io_module_info.header.type = device_type;
    esp_io_module_info.header.slot = slot_num_index;
    status = fbe_api_esp_module_mgmt_getIOModuleInfo(&esp_io_module_info);
    max_port = esp_io_module_info.io_module_phy_info.ioPortCount;
	for(port_num_index = 0; port_num_index < max_port; port_num_index++)
	{
		fbe_cli_specific_port_sfp_info(device_type,
								       slot_num_index,
								       port_num_index);
	}
}

/*************************************************************************
 * end fbe_cli_specific_slot_sfp_info() 
 ************************************************************************/

 /*!**********************************************************************
 * fbe_cli_specific_device_sfp_info()
 *************************************************************************
 * @brief
 *      Prints all sfp information of a specific device type. 
 *      Called by FBE cli sfpinfo command
 *
 *  @param :
 *      fbe_u32_t                       device_type
 *
 *  @return:
 *      Nothing.
 *
 * @author
 *   2-May-2012:    Feng Ling - Created
 *
 ************************************************************************/

static void fbe_cli_specific_device_sfp_info(fbe_u32_t device_type)
{
	fbe_status_t                             status = FBE_STATUS_OK;
	fbe_u32_t                                max_slot = 0;
    fbe_u32_t                                slot_num_index;
	fbe_esp_module_limits_info_t             limits_info = {0};

	status = fbe_api_esp_module_mgmt_get_limits_info(&limits_info);

	if(device_type == FBE_DEVICE_TYPE_MEZZANINE)
        max_slot = limits_info.discovered_hw_limit.num_mezzanine_slots;
	else if(device_type == FBE_DEVICE_TYPE_IOMODULE)
        max_slot = limits_info.discovered_hw_limit.num_slic_slots;
	else
        max_slot = limits_info.discovered_hw_limit.num_bem;
	
	for(slot_num_index = 0; slot_num_index < max_slot; slot_num_index++)
	{
		fbe_cli_specific_slot_sfp_info(device_type,
								       slot_num_index);
	}
}
/*************************************************************************
 * end fbe_cli_specific_device_sfp_info() 
 ************************************************************************/

 /*!**********************************************************************
 * fbe_cli_all_sfp_info()
 *************************************************************************
 * @brief
 *      Prints all sfp information of all device types. 
 *      Called by FBE cli sfpinfo command
 *
 *  @param :
 *      None
 *
 *  @return:
 *      Nothing.
 *
 * @author
 *   2-May-2012:    Feng Ling - Created
 *
 ************************************************************************/

static void fbe_cli_all_sfp_info()
{
		fbe_cli_specific_device_sfp_info(FBE_DEVICE_TYPE_MEZZANINE);
		fbe_cli_specific_device_sfp_info(FBE_DEVICE_TYPE_IOMODULE);
		fbe_cli_specific_device_sfp_info(FBE_DEVICE_TYPE_BACK_END_MODULE);
}

/*************************************************************************
 * end fbe_cli_all_sfp_info() 
 ************************************************************************/

 
/*!***********************************************************************
 * fbe_cli_print_sfp_info()
 *************************************************************************
 * @brief
 *      Prints sfp related information of local SP. 
 *      Called by FBE cli sfpinfo command
 *
 *  @param :
 *      fbe_esp_module_sfp_info_t          sfp_info
 *
 *  @return:
 *      Nothing.
 *
 * @author
 *   25-July-2011:  Harshal Wanjari - created
 *
 ************************************************************************/
 static void fbe_cli_print_sfp_info(fbe_esp_module_sfp_info_t *sfp_info)
{
    fbe_status_t                            status = FBE_STATUS_OK;
	fbe_esp_module_io_port_info_t           esp_io_port_info = {0};

    
    esp_io_port_info.header.sp = sfp_info->header.sp;
    esp_io_port_info.header.type = sfp_info->header.type;
    esp_io_port_info.header.slot = sfp_info->header.slot;
    esp_io_port_info.header.port = sfp_info->header.port;
    status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);

    /*printing SFP information */
    fbe_cli_printf("\nPort Address: %d\n",
		           sfp_info->header.port);
    fbe_cli_printf("Port Role-logical number:           %s-%d\n",
		           fbe_cli_convert_ioport_role_id_to_name(&esp_io_port_info.io_port_logical_info.port_role),
                   esp_io_port_info.io_port_logical_info.logical_number);
    fbe_cli_printf("  SFP Inserted:                     %s\n", 
		           sfp_info->sfp_info.inserted ? "Yes" : "No");
    if (sfp_info->sfp_info.inserted)
    {
        fbe_cli_printf("  SFP cable length:                 %d m\n", 
                       sfp_info->sfp_info.cable_length);
        fbe_cli_printf("  SFP cable speed:                  0x%x\n", 
                       sfp_info->sfp_info.capable_speeds);
        fbe_cli_printf("  SFP Media type:                   ");
        fbe_cli_display_sfp_media_type(sfp_info->sfp_info.media_type);
        fbe_cli_printf("  SFP HW type:                      0x%xm\n", 
                       sfp_info->sfp_info.hw_type);
        fbe_cli_printf("  SFP emc part number:              %s\n", 
                       sfp_info->sfp_info.emc_part_number);
        fbe_cli_printf("  SFP emc serial number:            %s\n", 
                       sfp_info->sfp_info.emc_serial_number);
        fbe_cli_printf("  SFP vendor part number:           %s\n", 
                       sfp_info->sfp_info.vendor_part_number);
        fbe_cli_printf("  SFP vendor serial number:         %s\n", 
                       sfp_info->sfp_info.vendor_serial_number);
        fbe_cli_printf("  SFP condition additional info:    %d\n", 
                       sfp_info->sfp_info.condition_additional_info);
        fbe_cli_printf("  SFP condition type:               ");
        fbe_cli_display_sfp_condition_type(sfp_info->sfp_info.condition_type);
        fbe_cli_printf("  SFP sub condition type:           ");
        fbe_cli_display_sfp_sub_condition_type(sfp_info->sfp_info.sub_condition_type);
        fbe_cli_display_sfp_supported_protocols(sfp_info->sfp_info.supported_protocols);
    }
    return;
}
/*************************************************************************
 * end fbe_cli_print_sfp_info() 
 ************************************************************************/

 /*!**********************************************************************
 * fbe_cli_get_port_role()
 *************************************************************************
 * @brief
 *     This function gets the port role.
 *
 *  @param :
 *      fbe_u32_t                        port 
 *
 *  @return:
 *      fbe_u8_t* -port role name.
 *
 * @author
 *   25-July-2011:  Harshal Wanjari - created
 *
 ************************************************************************/
fbe_u8_t* fbe_cli_get_port_role(fbe_u32_t port )
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_object_id_t     object_id;
    fbe_port_role_t     port_role;
    fbe_u8_t *port_name;
    
    /*get port object id by location*/
    status = fbe_api_get_port_object_id_by_location(port,&object_id);
    if(status != FBE_STATUS_OK)
    {
        port_name = (fbe_u8_t*)("UNINIT");
        return port_name;
    }

    /*get port role by object id*/
    status = fbe_api_port_get_port_role(object_id,&port_role);
    if(status != FBE_STATUS_OK)
    {
        port_name = (fbe_u8_t*)("UNINIT");
        return port_name;
    }
    port_name = fbe_cli_convert_port_role_id_to_name(&port_role);
    
    return port_name;
}
/*************************************************************************
 * end fbe_cli_get_port_role() 
 ************************************************************************/
    
/*!***********************************************************************
 * fbe_cli_convert_port_role_id_to_name()
 *************************************************************************
 * @brief
 *  This function takes a port  role and returns a port role name.
 *
 * @param 
 *       port_role - FE, BE or Uncommitted
 *
 * @return 
 *       fbe_u8_t* - port role name
 *
 * @author
 *   25-July-2011:  Harshal Wanjari - created
 *
 ************************************************************************/
fbe_u8_t* fbe_cli_convert_port_role_id_to_name( fbe_port_role_t *port_role)
{
    fbe_u8_t *port_str = {0};

    /*converting port role id to name */
    switch(*port_role)
    {
        case FBE_PORT_ROLE_FE:
            port_str = (fbe_u8_t *)("FE");
            break;
        case FBE_PORT_ROLE_BE:
            port_str = (fbe_u8_t *)("BE");
            break;
        case FBE_PORT_ROLE_BOOT:
            port_str = (fbe_u8_t *)("BOOT");
            break;
        case FBE_PORT_ROLE_SPECIAL:
            port_str = (fbe_u8_t *)("SPECIAL"); 
            break;
        case FBE_PORT_ROLE_UNC:
            port_str = (fbe_u8_t *)("UNC");
            break;
        default:
            port_str = (fbe_u8_t *)("Invalid");
            break;
    }   
    return (port_str);
}
/*************************************************************************
 * end fbe_cli_convert_port_role_id_to_name() 
 ************************************************************************/
 
/*!***********************************************************************
 * fbe_cli_convert_ioport_role_id_to_name()
 *************************************************************************
 * @brief
 *  This function takes an io port  role and returns an io port role name.
 *
 * @param 
 *       ioport_role - FE, BE or Uncommitted
 *
 * @return 
 *       fbe_u8_t* - port role name
 *
 * @author
 *   25-July-2011:  Harshal Wanjari - created
 *
 ************************************************************************/
fbe_u8_t* fbe_cli_convert_ioport_role_id_to_name( fbe_ioport_role_t *port_role)
{
    fbe_u8_t *port_str = {0};

    /*converting port role id to name */
    switch(*port_role)
    {
        case IOPORT_PORT_ROLE_FE:
            port_str = (fbe_u8_t *)("FE");
            break;

        case IOPORT_PORT_ROLE_BE:
            port_str = (fbe_u8_t *)("BE");
            break;

        default:
            port_str = (fbe_u8_t *)("Invalid");
            break;

    }
    return (port_str);
}
/*************************************************************************
 * end fbe_cli_convert_ioport_role_id_to_name() 
 ************************************************************************/

/*!***********************************************************************
 * fbe_cli_display_sfp_supported_protocols()
 *************************************************************************
 * @brief
 *  This function takes the supported protocols bitmask and prints a
 *  concatenated string for protocols.
 *
 * @param 
 *       supported_protocols 
 *
 * @return 
 *       n/a
 *
 * @author
 *   31-July-2015:  Brion Philbin - created
 *
 ************************************************************************/
void fbe_cli_display_sfp_supported_protocols(fbe_sfp_protocol_t supported_protocols)
{
    int	OneAlreadyPrinted = 0;
    fbe_char_t stringBuf[255] = {0};

    if (supported_protocols  == FBE_SFP_PROTOCOL_UNKNOWN) 
    {
        strncat(stringBuf, "Unknown", strlen("Unknown"));
    }
    else 
    {
        if (supported_protocols & FBE_SFP_PROTOCOL_FC) {

                strncat(stringBuf, "FC", strlen("FC"));
                OneAlreadyPrinted++;
        }

        if (supported_protocols & FBE_SFP_PROTOCOL_ISCSI) {

                if (OneAlreadyPrinted) { strncat(stringBuf, ",", strlen(",")); }
                strncat(stringBuf, "iSCSI", strlen("iSCSI"));
                OneAlreadyPrinted++;
        }

        if (supported_protocols & FBE_SFP_PROTOCOL_FCOE) {

                if (OneAlreadyPrinted) { strncat(stringBuf, ",", strlen(",")); }
                strncat(stringBuf, "FCoE", strlen("FCoE"));
                OneAlreadyPrinted++;
        }
    }
    fbe_cli_printf("  Supported Protocols:              %s\n", stringBuf);
}



/*************************************************************************
 * end fbe_cli_lib_sfp_info_cmds() 
 ************************************************************************/
