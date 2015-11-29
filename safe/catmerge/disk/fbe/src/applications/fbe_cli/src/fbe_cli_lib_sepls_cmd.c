/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_sepls_cmd.c
 ***************************************************************************
 *
 * @brief
 *  This file contains FBE CLI functions for sepls command.
 *
 * @ingroup fbe_cli
 *
 * @revision
 *  01/05/2012:  Trupi Ghate - created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_cli_lib_sepls_cmd.h"
#include "fbe_cli_private.h"
#include "fbe_trace.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_trace.h"
#include "fbe/fbe_api_utils.h"
#include "fbe_api_raid_group_interface.h"
#include "fbe_cli_luninfo.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"

static fbe_object_id_t  vd_ids[FBE_MAX_SPARE_OBJECTS] = {FBE_OBJECT_ID_INVALID};
static fbe_u32_t        total_vd_ids = 0;
static fbe_object_id_t  pvd_ids[FBE_MAX_SPARE_OBJECTS] = {FBE_OBJECT_ID_INVALID};
static fbe_u32_t        total_pvd_ids = 0;
static fbe_bool_t       b_display_allsep = FBE_FALSE;
static fbe_bool_t       b_display_default = FBE_TRUE;
static fbe_u32_t        display_format = FBE_SEPLS_DISPLAY_OBJECTS_NONE;
static fbe_lba_t         total_lun_capacity = 0;
static fbe_object_id_t object_id_arg = FBE_OBJECT_ID_INVALID;

/*************************************************************************
 * @fn      fbe_cli_lib_sepls_get_display_format()                       *
 *************************************************************************
 *
 * @brief   Get the desired display format.
 *
 * @param   None
 *
 * @return  current display format
 * 
 * @author
 *  11/05/2012  Ron Proulx  - Created
 *
 *************************************************************************/
fbe_u32_t fbe_cli_lib_sepls_get_display_format(void)
{
    return display_format;
}
/********************************************
 * end fbe_cli_lib_sepls_get_display_format()
 ********************************************/


/*************************************************************************
 * @fn      fbe_cli_lib_sepls_set_display_format()                       *
 *************************************************************************
 *
 * @brief   Set the desired display format.
 *
 * @param   selected_display_format - Display format to use.
 *
 * @return  None
 * 
 * @author
 *  11/05/2012  Ron Proulx  - Created
 *
 *************************************************************************/
void fbe_cli_lib_sepls_set_display_format(fbe_sepls_display_object_t selected_display_format)
{
    display_format = selected_display_format;

    return;
}
/********************************************
 * end fbe_cli_lib_sepls_set_display_format()
 ********************************************/

/*************************************************************************
 * @fn      fbe_cli_lib_sepls_get_current_object_id_to_display()         *
 *************************************************************************
 *
 * @brief   Get the current object id to display.
 *
 * @param   None
 *
 * @return  current display format
 * 
 * @author
 *  11/05/2012  Ron Proulx  - Created
 *
 *************************************************************************/
fbe_object_id_t fbe_cli_lib_sepls_get_current_object_id_to_display(void)
{
    return object_id_arg;
}
/**********************************************************
 * end fbe_cli_lib_sepls_get_current_object_id_to_display()
 **********************************************************/

/*************************************************************************
 * @fn      fbe_cli_lib_sepls_set_object_id_to_display()                 *
 *************************************************************************
 *
 * @brief   Set the desired object id to display.
 *
 * @param   object_id_to_display - Display format to use.
 *
 * @return  None
 * 
 * @author
 *  11/05/2012  Ron Proulx  - Created
 *
 *************************************************************************/
void fbe_cli_lib_sepls_set_object_id_to_display(fbe_object_id_t object_id_to_display)
{
    object_id_arg = object_id_to_display;

    return;
}
/**************************************************
 * end fbe_cli_lib_sepls_set_object_id_to_display()
 **************************************************/

/*************************************************************************
 *                            @fn fbe_cli_cmd_sepls ()                                                           *
 *************************************************************************
 *
 *  @brief
 *   fbe_cli_cmd_sepls performs the execution of the 'sepls' command
 *   which displays the sep objects information.
 *
 * @param :
 *   argc     (INPUT) NUMBER OF ARGUMENTS
 *   argv     (INPUT) ARGUMENT LIST
 *
 * @return:
 *   None
 *
 * @author
 *   05-Jan-2012 Created - Trupti Ghate
 *
 *************************************************************************/
void fbe_cli_cmd_sepls(fbe_s32_t argc,char ** argv)
{

    /*****************
    **    BEGIN    **
    *****************/
    fbe_u32_t level = 0, index = 0;
    fbe_status_t status;
    fbe_object_id_t *pvd_object_ids = NULL;
    fbe_u32_t num_objects = 0;
    fbe_api_provision_drive_info_t provision_drive_info;
    fbe_package_id_t package_id = FBE_PACKAGE_ID_SEP_0 ;

    b_display_allsep = FBE_FALSE;
    b_display_default = FBE_TRUE;
    display_format = FBE_SEPLS_DISPLAY_LEVEL_2;  /* Default display format */

    if(argc > 0)
    {
        /* Display all the objects */
        if (strlen(*argv) && strncmp(*argv, "-help", 5) == 0)
        {
            fbe_cli_printf("%s",SEPLS_CMD_USAGE);
            return;
        }
        /* Display all the objects */
        if (strlen(*argv) && strncmp(*argv, "-allsep", 7) == 0)
        {
            b_display_allsep = FBE_TRUE;
            b_display_default = FBE_FALSE;
            /* Increment past the allsep flag.
             */
            *argv += FBE_MIN(8, strlen(*argv));
        }
        /* Display only LUN objects */
        else if (strlen(*argv) && strncmp(*argv, "-lun", 4) == 0)
        {
            display_format = FBE_SEPLS_DISPLAY_ONLY_LUN_OBJECTS;
            b_display_default = FBE_FALSE;
            *argv += FBE_MIN(5, strlen(*argv));
        }
        /* Display only RG objects */
        else if (strlen(*argv) && strncmp(*argv, "-rg", 3) == 0)
        {
            display_format = FBE_SEPLS_DISPLAY_ONLY_RG_OBJECTS;
            b_display_default = FBE_FALSE;
            *argv += FBE_MIN(4, strlen(*argv));
        }
        /* Display only VD objects */
        else if (strlen(*argv) && strncmp(*argv, "-vd", 3) == 0)
        {
            display_format = FBE_SEPLS_DISPLAY_ONLY_VD_OBJECTS;
            b_display_default = FBE_FALSE;
            *argv += FBE_MIN(4, strlen(*argv));
        }
        /* Display only PVD objects */
        else if (strlen(*argv) && strncmp(*argv, "-pvd", 4) == 0)
        {
            display_format = FBE_SEPLS_DISPLAY_ONLY_PVD_OBJECTS;
            b_display_default = FBE_FALSE;
            *argv += FBE_MIN(5, strlen(*argv));
        }
        else if (strlen(*argv) && strncmp(*argv, "-level", 6) == 0)
        {
           argv++;
            if(*argv != NULL)
            {
                b_display_default = FBE_FALSE;
                level = strtoul(*argv, 0, 0);
                switch(level)
                {
                    case 1:      /* Level 1 will display only RG objects */
                    {
                        display_format = FBE_SEPLS_DISPLAY_LEVEL_1;
                        break;
                    }
                    case 2:     /* Level 2 will display only LUN and RG objects */
                    {
                        display_format = FBE_SEPLS_DISPLAY_LEVEL_2;
                        break;
                    }
                    case 3:    /* Level 3 will display  LUN, RG and VD objects */
                    {
                        display_format = FBE_SEPLS_DISPLAY_LEVEL_3;
                        break;
                    }
                    case 4:    /* Level 4 will display  LUN, RG, VD and PVD objects */
                    {
                        display_format = FBE_SEPLS_DISPLAY_LEVEL_4;
                        break;
                    }
                    default:
                    {
                        fbe_cli_printf("%-8s","sepls: Invalid level. Please specify the level between 1 to 4\n");
                        fbe_cli_printf("%s",SEPLS_CMD_USAGE);
                        return;
                    }
                }
            }
        }
        else if (strlen(*argv) && (strncmp(*argv, "-objtree", 6) == 0
                                   || strncmp(*argv, "-o",2)==0))
        {
           argv++;
            if(*argv != NULL)
            {
                b_display_default = FBE_FALSE;
                display_format = FBE_SEPLS_DISPLAY_OBJECT_TREE;
                object_id_arg = strtoul(*argv, 0, 0);
                argv++;
                if(*argv != NULL)
                {
                    if (strlen(*argv) && (strncmp(*argv, "-package", 6) == 0
                                          || strncmp(*argv, "-p",2)==0))
                    {
                        argv++;
                        if(*argv != NULL)
                        {
                            if((strcmp(*argv, "physical") == 0) ||
                               (strcmp(*argv, "pp") == 0))
                            {
                                package_id = FBE_PACKAGE_ID_PHYSICAL;
                            }
                            else if(strcmp(*argv, "sep") == 0)
                            {
                                package_id = FBE_PACKAGE_ID_SEP_0;
                            }
                            else
                            {
                                fbe_cli_printf("Please enter either physical or sep package name. \n");
                                return;
                            }
                        }
                    }
               }
               else
               {
                   package_id = FBE_PACKAGE_ID_SEP_0;
                   fbe_cli_printf("\nNo package name specified, hence assuming it as SEP package. \n");
                   fbe_cli_printf("For LDO and PDO objects you should specify physical package name.\n");
                   fbe_cli_printf("For example, sepls -o 0x5 -package physical\n\n");
               }
            }
        }
        else
        {
            fbe_cli_printf("%-8s","sepls: Invalid arguments specified.\n");
            fbe_cli_printf("%s",SEPLS_CMD_USAGE);
            return;
        }
    }
    fbe_cli_printf("Type    Obj ID       ID    Object Info    Lifecycle State  Downstream                        Drives\n");
    fbe_cli_printf("                                                           Objects\n");
    fbe_cli_printf("------------------------------------------------------------------------------------------------------------------------------------------------------\n");
  
  
    status = fbe_cli_sepls_rg_info(object_id_arg, package_id);
    if (status != FBE_STATUS_OK)
    {
     return;
    }
  
    if(display_format != FBE_SEPLS_DISPLAY_OBJECT_TREE)
    {
        if(b_display_allsep == FBE_TRUE)
        {
            fbe_cli_printf("\n");
            fbe_cli_printf("\n");
        }
        /* Display PVD objects that are not associated with RAID group */
        status = 
        fbe_api_enumerate_class(FBE_CLASS_ID_PROVISION_DRIVE, 
                                FBE_PACKAGE_ID_SEP_0, 
                                &pvd_object_ids, 
                                &num_objects);

        if(status != FBE_STATUS_OK)
        {
            fbe_cli_error("%s failed to get a list of PVDs\n",__FUNCTION__);

            if(pvd_object_ids != NULL)
            {
                fbe_api_free_memory(pvd_object_ids);
            }
        }
        else
        {
            for (index = 0; index < num_objects; index++)
            {
                  status = fbe_api_provision_drive_get_info(pvd_object_ids[index],&provision_drive_info);
                  if (status == FBE_STATUS_BUSY)
                  {
                      fbe_status_t          lifecycle_status = FBE_STATUS_INVALID;
                      fbe_lifecycle_state_t lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
                      
                      lifecycle_status = fbe_api_get_object_lifecycle_state(pvd_object_ids[index], 
                                                                  &lifecycle_state, FBE_PACKAGE_ID_SEP_0);
                      if (lifecycle_status != FBE_STATUS_OK)
                      {
                          fbe_cli_printf("%s Get lifecycle failed: index: %d obj id: 0x%x status: 0x%x \n",
                                         __FUNCTION__, index, pvd_object_ids[index], status);
                          return;
                      }
                      if (lifecycle_state != FBE_LIFECYCLE_STATE_SPECIALIZE)
                          continue;
                  } 
                  else if (status != FBE_STATUS_OK)
                  {
                      fbe_cli_error("\nFailed to get provision drive information for object id:0x%x status: 0x%x\n", pvd_object_ids[index], status);
                      return;
                  }
                 fbe_cli_sepls_print_pvd_details(pvd_object_ids[index],  &provision_drive_info);
            }
            if(pvd_object_ids != NULL)
            {
                fbe_api_free_memory(pvd_object_ids);
            }
        }
    }
    total_pvd_ids = 0;
    total_vd_ids = 0;
    display_format = FBE_SEPLS_DISPLAY_OBJECTS_NONE;
    object_id_arg = FBE_OBJECT_ID_INVALID;
}

/**************************************************************************
*                      fbe_cli_sepls_rg_info ()                 *
***************************************************************************
*
*  @brief
*    This function logic displays LUN-RG-VD-PVD-LDO-LDO.
*
*
*  @param
*          fbe_u32_t display_format
*          fbe_bool_t b_display_allsep
*  @return
*    fbe_u32_t status
*
*  @version
*    01/11/2012 - Created. Trupti Ghate
*
***************************************************************************/
fbe_u32_t fbe_cli_sepls_rg_info(fbe_object_id_t object_id_arg, fbe_package_id_t package_id)
{
    fbe_object_id_t           *object_ids;
    fbe_u32_t                  i,index,index_rgs;
    fbe_status_t               status = FBE_STATUS_OK;
    fbe_cli_lurg_rg_details_t rg_details;
    fbe_class_id_t                class_id;
    fbe_u32_t                      num_objects = 0;
    fbe_bool_t                     is_sys_vd_displayed = FBE_FALSE;
    fbe_bool_t                     is_vd_exists = FBE_FALSE;
    fbe_block_size_t            exported_block_size;
    fbe_bool_t                     is_system_rg = FBE_FALSE;
    fbe_u32_t                      total_objects = 0;
    fbe_object_id_t              object_id = FBE_OBJECT_ID_INVALID;
    fbe_status_t          lifecycle_status = FBE_STATUS_INVALID;
    fbe_lifecycle_state_t lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("Failed to get total number of objects\n");
        return status;
    }

    object_ids = (fbe_object_id_t *)fbe_api_allocate_memory(sizeof(fbe_object_id_t) * total_objects);
    if (object_ids == NULL) {
        fbe_cli_error("Failed to allocate memory");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Retrive list of objects_ids of SEP package 
      */
    fbe_set_memory(object_ids, (fbe_u8_t)FBE_OBJECT_ID_INVALID, sizeof(fbe_object_id_t) * total_objects);
    status = fbe_api_enumerate_objects (object_ids, total_objects, &num_objects ,FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        fbe_api_free_memory(object_ids);
        fbe_cli_error("Failed to retrieve objects of SEP package\n");
        return status;
    }

    /* Find out the associated RAID obejct ID 
      */
    if( display_format == FBE_SEPLS_DISPLAY_OBJECT_TREE)
    {
            total_vd_ids = 0;
            status = fbe_cli_sepls_get_rg_object_id(object_id_arg,&object_id, package_id);
            if(status == FBE_STATUS_GENERIC_FAILURE)
            {
                fbe_cli_error("\n\nInvalid object ID specified. \n");
                return status;
            }

            if(object_id == FBE_OBJECT_ID_INVALID)
            {
                return FBE_STATUS_OK;
            }
    }

    for (index_rgs = 0; index_rgs < num_objects; index_rgs++)
    {
        if( display_format != FBE_SEPLS_DISPLAY_OBJECT_TREE)
        {
            object_id = object_ids[index_rgs];
        }

        status = fbe_api_get_object_class_id(object_id, &class_id, FBE_PACKAGE_ID_SEP_0);
        if (status == FBE_STATUS_BUSY)
        {
            lifecycle_status = fbe_api_get_object_lifecycle_state(object_id, 
                                                        &lifecycle_state, FBE_PACKAGE_ID_SEP_0);
            if(lifecycle_status != FBE_STATUS_OK)
            {
                fbe_cli_error("Fail to get the Lifecycle State for object id:0x%x, status:0x%x\n",
                               object_id,lifecycle_status);
                continue;
            }
            if (lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE)
            {
	 	if(class_id != FBE_CLASS_ID_PROVISION_DRIVE)
                fbe_cli_printf("%-9s0x%-3x :%-6d%-28s\n", "Unknown", object_id, 
                               object_id, "SPECLZ");
		/* status is set to FBE_STATUS_OK so that it doesnt return immdeiatly outside this function and prints PVD info.*/
		status = FBE_STATUS_OK;
                continue;
            }
        }
        else if (status != FBE_STATUS_OK) 
        {
           fbe_cli_printf("failed to get class ID of object %d\n", object_id);
           return FBE_STATUS_GENERIC_FAILURE;
        }
        if(class_id < FBE_CLASS_ID_MIRROR || class_id > FBE_CLASS_ID_PARITY)
        {
            continue;
        }
        total_lun_capacity = 0;

         /* Display system VD-PVD-LDO-PDO,
           * after all system RGs and LUNs are displayed
           */
        is_system_rg = FBE_FALSE;
        status = fbe_api_database_is_system_object(object_id, &is_system_rg);
        if (status != FBE_STATUS_OK) 
        {
            fbe_api_free_memory(object_ids);
            fbe_cli_printf("failed to identify whether object(%d) is system object or not \n", object_ids[index_rgs]);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if(is_system_rg == FBE_FALSE)
        {
            if( is_sys_vd_displayed == FBE_FALSE)
           {
                for(i=0; i< total_vd_ids; i++)
                {
                    fbe_cli_sepls_print_vd_details(vd_ids[i], exported_block_size);
                    vd_ids[i] = FBE_OBJECT_ID_INVALID;
                }
                total_vd_ids =0;
                is_sys_vd_displayed = FBE_TRUE;
            }
        }
        fbe_zero_memory(&rg_details, sizeof(fbe_cli_lurg_rg_details_t));
        /* We want downstream as well as upstream objects for this RG indicate 
        * that by setting corresponding variable in structure
        */
        rg_details.direction_to_fetch_object = DIRECTION_BOTH;
    
        /* Get the details of RG associated with this LUN */
        status = fbe_cli_get_rg_details(object_id, &rg_details);
        /* return failure from here since we're not able to know objects related to this */
        if (status == FBE_STATUS_GENERIC_FAILURE)
        {
            fbe_api_free_memory(object_ids);
            fbe_cli_printf("failed to get Raid Group details of object %d\n", object_id);
            return status;
        }
        if(rg_details.rg_number == FBE_RAID_GROUP_INVALID)
        {
            continue;
        }

        for (index = 0; index < rg_details.upstream_object_list.number_of_upstream_objects; index++)
        {
            /* Find out if given obj ID is for RAID group or LUN */
            status = fbe_api_get_object_class_id(rg_details.upstream_object_list.upstream_object_list[index], &class_id, FBE_PACKAGE_ID_SEP_0);
            if(status == FBE_OBJECT_ID_INVALID)
            {
                break;
            }
            /* if status is generic failure then continue to next object */
            if (status == FBE_STATUS_GENERIC_FAILURE)
            {
                fbe_cli_printf("failed to get class ID of object %d\n", object_id);
                continue;
            }
            if(class_id == FBE_CLASS_ID_LUN)
            {
                 /*Print details of a current lun*/
                 fbe_cli_sepls_print_lun_details(rg_details.upstream_object_list.upstream_object_list[index], &rg_details);
            }
         }
         fbe_cli_sepls_print_rg_details(object_id, &rg_details,&exported_block_size);

        /* Since one VD can be associated with more than one RG,
         * maintain the list of VDs displayed to prevent multiple records 
         * displayed for same VD
         */
        for (index = 0; index < rg_details.downstream_vd_object_list.number_of_downstream_objects; index++)
        {
            is_vd_exists = FBE_FALSE;
             for(i=0; i< total_vd_ids; i++)
             {
                 if(vd_ids[i] == rg_details.downstream_vd_object_list.downstream_object_list[index])
                 {
                     is_vd_exists = FBE_TRUE;
                     break;
                 } 
             }
             if(is_vd_exists == FBE_FALSE)
             {
                 vd_ids[total_vd_ids] = rg_details.downstream_vd_object_list.downstream_object_list[index];
                 total_vd_ids++;
                 if( is_sys_vd_displayed == FBE_TRUE || display_format == FBE_SEPLS_DISPLAY_OBJECT_TREE)
                 {
                    fbe_cli_sepls_print_vd_details(rg_details.downstream_vd_object_list.downstream_object_list[index], exported_block_size);
                 }
             }
        }
        if( display_format == FBE_SEPLS_DISPLAY_OBJECT_TREE)
        {
            break;
        }
    }
    fbe_api_free_memory(object_ids);
    return status;
}

/*!**************************************************************
 *          fbe_cli_sepls_get_vd_mode_by_drive()
 ****************************************************************
 *
 * @brief   This function checks spare drive.
 *
 * @param   index : drive list index 
 *
 * @return  None.
 *
 * @author
 *  24/03/2014 - Created. Dong Jbing
 *
 ****************************************************************/
fbe_bool_t fbe_cli_sepls_get_vd_mode_by_drive(fbe_u32_t index, fbe_cli_lurg_rg_details_t * rg_details, fbe_virtual_drive_configuration_mode_t * mode)
{
    fbe_u32_t i;
    fbe_u32_t spare_index = -1;
    fbe_u32_t current_index = 0;
    fbe_virtual_drive_configuration_mode_t current_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN;

    for (i = 0; i < rg_details->downstream_vd_object_list.number_of_downstream_objects; i++)
    {
        current_mode = rg_details->downstream_vd_mode_list[i];
        if (current_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)
        {
            spare_index = current_index + 1;
            current_index += 2;
        }
        else if (current_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE)
        {
            spare_index = current_index;
            current_index += 2;
        }
        else
        {
            spare_index = -1;
            current_index++; 
        }

        if (current_index > index)
            break;
    }

    if (index == spare_index)
    {
        *mode = current_mode;
        return FBE_TRUE;
    }

    if (i < rg_details->downstream_vd_object_list.number_of_downstream_objects)
    {
        *mode = current_mode;
    }
    else
    {
        *mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN;
    }

    return FBE_FALSE;
}
/********************************************
 * end fbe_cli_sepls_get_vd_mode_by_drive
 ********************************************/

/**************************************************************************
*                      fbe_cli_sepls_print_lun_details ()                 *
***************************************************************************
*
*  @brief
*    This function prints the information of a single lun.
*
*
*  @param
*          lun_id - LUN Object id
*  @return
*    None
*
*  @version
*    01/11/2012 - Created. Trupti Ghate
*
***************************************************************************/
void fbe_cli_sepls_print_lun_details(fbe_object_id_t lun_id, fbe_cli_lurg_rg_details_t * rg_details)
{
    fbe_status_t               status;
    fbe_cli_lurg_lun_details_t lun_details;
    fbe_u32_t                  i;
    fbe_lifecycle_state_t lifecycle_state;
    fbe_block_size_t      exported_block_size;
    fbe_lba_t             capacity_of_lun;
    fbe_virtual_drive_configuration_mode_t  vd_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN;
    fbe_bool_t b_is_spare_drive = FBE_FALSE;
    fbe_bool_t b_display = FBE_FALSE;
    if( display_format == FBE_SEPLS_DISPLAY_OBJECT_TREE)
    {
        fbe_cli_printf("%s", lun_id == object_id_arg? "*":" ");
    }
    /*Get lun details*/
    status = fbe_cli_get_lun_details(lun_id,&lun_details);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("Failed to get details of Lun id %x, Status:0x%x\n",lun_id,status);
        return;
    }
    total_lun_capacity += lun_details.lun_info.capacity;
    fbe_cli_sepls_is_display_allowed(FBE_SEPLS_OBJECT_TYPE_LUN, &b_display);
    if(b_display == FBE_FALSE)
    {
        return ;
    }
   
    fbe_cli_printf("%-9s","LUN");
    fbe_cli_printf("0x%-3x :%-6d",lun_id, lun_id);
    fbe_cli_printf("%-6d", lun_details.lun_number);

    exported_block_size = lun_details.lun_info.raid_info.exported_block_size;
    capacity_of_lun = (fbe_u32_t)((lun_details.lun_info.capacity *exported_block_size)/(1024*1024));
    fbe_cli_printf("%-7llu", (unsigned long long)capacity_of_lun);
    fbe_cli_printf("%-8s","MB");
    fbe_cli_printf(" %-15s ", lun_details.p_lifecycle_state_str);
    fbe_cli_printf(" %-33d", lun_details.lun_info.raid_group_obj_id);

    /*Get drive objects related to a RG*/
    for (i = 0; i < rg_details->downstream_pvd_object_list.number_of_downstream_objects; i++)
    {
        status = fbe_api_get_object_lifecycle_state(rg_details->downstream_pvd_object_list.downstream_object_list[i], &lifecycle_state, FBE_PACKAGE_ID_SEP_0);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s Get lifecycle failed: index: %d obj id: 0x%x status: 0x%x \n",
                           __FUNCTION__, i, rg_details->downstream_pvd_object_list.downstream_object_list[i], status);
            return;
        }

        b_is_spare_drive = fbe_cli_sepls_get_vd_mode_by_drive(i, rg_details, &vd_mode);
        if (b_is_spare_drive && vd_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)
        {
            fbe_cli_printf(" ->");
        }

        if (!b_is_spare_drive && vd_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE)
        {
            fbe_cli_printf(" <-");
        }

        if(lifecycle_state == FBE_LIFECYCLE_STATE_FAIL)
        {
            fbe_cli_printf( "%s"," (FAIL)");
        }
        else
        {
            /*Print the info of fru*/
            fbe_cli_printf(" %d_%d_%d ",rg_details->drive_list[i].port_num,rg_details->drive_list[i].encl_num,rg_details->drive_list[i].slot_num);
        }
    }
    fbe_cli_printf("\n");
}
/******************************************
 * end fbe_cli_luninfo_print_lun_details()
 ******************************************/

/**************************************************************************
*                      fbe_cli_sepls_print_rg_details ()                 *
***************************************************************************
*
*  @brief
*    This function prints the information of a single RG.
*
*
*  @param
*           rg_object_id - RG Object id
*  @return
*    None
*
*  @version
*    01/11/2012 - Created. Trupti Ghate
*
***************************************************************************/
void fbe_cli_sepls_print_rg_details(fbe_object_id_t rg_object_id, fbe_cli_lurg_rg_details_t *rg_details,     fbe_block_size_t*    exported_block_size)
{
    fbe_status_t                 status = FBE_STATUS_OK;
    fbe_u32_t                 i = 0;
    fbe_lifecycle_state_t lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_api_raid_group_get_info_t   raid_group_info;
    const char *                             lifecycle_state_str;
    fbe_bool_t                                b_display = FBE_FALSE;
    fbe_api_base_config_downstream_object_list_t  downstream_raid_object_list;
    fbe_port_number_t                port_num = FBE_PORT_NUMBER_INVALID;
    fbe_enclosure_number_t         encl_num = FBE_ENCLOSURE_NUMBER_INVALID;
    fbe_enclosure_slot_number_t  slot_num = FBE_ENCLOSURE_SLOT_NUMBER_INVALID;
    fbe_u16_t                              reb_percentage = 0;
    fbe_u32_t                 rg_width = 0;
    fbe_object_id_t drive_obj_id = FBE_OBJECT_ID_INVALID;
    fbe_lba_t rebuild_capacity = 0;
    fbe_u32_t index = 0;
    fbe_virtual_drive_configuration_mode_t  vd_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN;
    fbe_bool_t b_is_spare_drive = FBE_FALSE;
    
   /* Initialize the raid_group_info*/
    fbe_zero_memory(&raid_group_info, sizeof(fbe_api_raid_group_get_info_t));

    /* Need to initialize rebuild checkpoint to FBE_LBA_INVALID explicitly*/
   for(i = 0; i < FBE_RAID_MAX_DISK_ARRAY_WIDTH ; i++){
        raid_group_info.rebuild_checkpoint[i] = FBE_LBA_INVALID;
   }

    /* initialize downstream_raid_object_list*/
    fbe_zero_memory(&downstream_raid_object_list, sizeof(fbe_api_base_config_downstream_object_list_t));

    /* Get the information for the RG */
    status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, 
                                         FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("rginfo: ERROR: Not able to get the information for RG object id 0x%x\n", 
                       rg_object_id);
        return;
    }

    /* Check whether user want to display RG object or not? 
      */
    fbe_cli_sepls_is_display_allowed(FBE_SEPLS_OBJECT_TYPE_RG, &b_display);
    if(b_display == FBE_FALSE)
    {
      *exported_block_size= raid_group_info.exported_block_size;
        return;
    }
    if( display_format == FBE_SEPLS_DISPLAY_OBJECT_TREE)
    {
        fbe_cli_printf("%s", rg_object_id == object_id_arg? "*":" ");
        fbe_cli_printf(" ");
        fbe_cli_printf("%-8s","RG");
    }
    else
    {
        fbe_cli_printf("%-9s","RG");
    }
    fbe_cli_printf("0x%-3x :%-6d",rg_object_id, rg_object_id);

    fbe_cli_printf("%-6d",rg_details->rg_number );
    fbe_cli_sepls_display_raid_type(raid_group_info.raid_type);
    status = fbe_api_get_object_lifecycle_state(rg_object_id, &lifecycle_state, FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("rginfo: ERROR: Not able to get the lifecycle for RG object id 0x%x\n", 
                       rg_object_id);
        return;
    }
    fbe_cli_get_state_name(lifecycle_state, &lifecycle_state_str);

    fbe_cli_printf("%-6s", lifecycle_state_str);
    //fbe_cli_printf("%-17s", lifecycle_state_str);

    if ((raid_group_info.error_verify_checkpoint != FBE_LBA_INVALID) ||
        (raid_group_info.system_verify_checkpoint != FBE_LBA_INVALID) ||
        (raid_group_info.incomplete_write_verify_checkpoint != FBE_LBA_INVALID) ||
        (raid_group_info.rw_verify_checkpoint != FBE_LBA_INVALID) ||
        (raid_group_info.ro_verify_checkpoint != FBE_LBA_INVALID)){
        fbe_u16_t vr_percent;
        
        if (raid_group_info.error_verify_checkpoint != FBE_LBA_INVALID){
             vr_percent = (fbe_u16_t) ((raid_group_info.error_verify_checkpoint * 100) / raid_group_info.imported_blocks_per_disk);
             fbe_cli_printf("ERRVR %3d%% ", vr_percent);
        }
        else if (raid_group_info.system_verify_checkpoint != FBE_LBA_INVALID){
             vr_percent = (fbe_u16_t) ((raid_group_info.system_verify_checkpoint * 100) / raid_group_info.imported_blocks_per_disk);
             fbe_cli_printf("SYSVR %3d%% ", vr_percent);
        }
        else if (raid_group_info.incomplete_write_verify_checkpoint != FBE_LBA_INVALID){
             vr_percent = (fbe_u16_t) ((raid_group_info.incomplete_write_verify_checkpoint * 100) / raid_group_info.imported_blocks_per_disk);
             fbe_cli_printf("IWRVR %3d%% ", vr_percent);
        }
        else if (raid_group_info.rw_verify_checkpoint != FBE_LBA_INVALID){
             vr_percent = (fbe_u16_t) ((raid_group_info.rw_verify_checkpoint * 100) / raid_group_info.imported_blocks_per_disk);
             fbe_cli_printf("RWRVR %3d%% ", vr_percent);
        }
        else if (raid_group_info.ro_verify_checkpoint != FBE_LBA_INVALID){
             vr_percent = (fbe_u16_t) ((raid_group_info.ro_verify_checkpoint * 100) / raid_group_info.imported_blocks_per_disk);
             fbe_cli_printf("RDOVR %3d%% ", vr_percent);
        }
    }
    else if ((raid_group_info.rekey_checkpoint != FBE_LBA_INVALID) &&
             (raid_group_info.rekey_checkpoint != 0))
    {
        fbe_u16_t encryption_percent;
        encryption_percent = (fbe_u16_t) ((raid_group_info.rekey_checkpoint * 100) / raid_group_info.imported_blocks_per_disk);
        if (encryption_percent > 100) {
            encryption_percent = 100;
        }
        fbe_cli_printf("ENC:%3d%% ", encryption_percent);
    }
    else
    {
        fbe_cli_printf("           ");
    }



    /*display the downstream objects of RG*/
    status = fbe_api_base_config_get_downstream_object_list(rg_object_id, &downstream_raid_object_list);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("rginfo: ERROR: Not able to get the downstream objects for RG object id 0x%x\n", 
                       rg_object_id);
        return;
    }
    for (i = 0; i <downstream_raid_object_list.number_of_downstream_objects; i++)
    {
        fbe_cli_printf("%-3d ",downstream_raid_object_list.downstream_object_list[i]);
    }
  
    // no padding whitespace if it's too long
    if (33>downstream_raid_object_list.number_of_downstream_objects*4)
    {
        for (i = 0; i < (33- downstream_raid_object_list.number_of_downstream_objects*4); i++)
        { 
            fbe_cli_printf(" ");
        }
    }
    rg_width = downstream_raid_object_list.number_of_downstream_objects;


    if(raid_group_info.raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        rg_width = rg_width *2;
    }
    if(raid_group_info.raid_type != FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER)
    {
        for (i = 0; i < rg_details->downstream_pvd_object_list.number_of_downstream_objects; i++)
        {
            b_is_spare_drive = fbe_cli_sepls_get_vd_mode_by_drive(i, rg_details, &vd_mode);

            if (b_is_spare_drive && vd_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)
            {
                fbe_cli_printf(" ->");
            }
            if (!b_is_spare_drive && vd_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE)
            {
                fbe_cli_printf(" <-");
            }

            fbe_cli_printf(" %d_%d_%d ",rg_details->drive_list[i].port_num,rg_details->drive_list[i].encl_num,rg_details->drive_list[i].slot_num);
            if (!b_is_spare_drive && index < rg_width)
            {
                if(lifecycle_state == FBE_LIFECYCLE_STATE_READY)
                {
                    /* Get the object id of the PVD */
                    status = fbe_api_provision_drive_get_obj_id_by_location(rg_details->drive_list[i].port_num,rg_details->drive_list[i].encl_num,
                                                                            rg_details->drive_list[i].slot_num, &drive_obj_id);

                    if (status == FBE_STATUS_OK)
                    {
                        if ((raid_group_info.rebuild_checkpoint[index] != FBE_LBA_INVALID))
                        {
                            rebuild_capacity = fbe_cli_sepls_calculate_rebuild_capacity(total_lun_capacity, rg_width, raid_group_info.raid_type);
                            if(rebuild_capacity != 0)
                            {
                                reb_percentage = (fbe_u16_t) ((raid_group_info.rebuild_checkpoint[index] * 100)/rebuild_capacity);
                            }
                            if(reb_percentage > 100)
                            {
                                //continue;
                            }
                            else
                            {
                                fbe_cli_printf( "(REB:%d%%) ", reb_percentage);
                            }
                        }
                    }
                }
                index++;
            }
         }        
    }
    else
    {   /* Retrieve and display the drive information of mirror objects */
         for (i = 0; i < downstream_raid_object_list.number_of_downstream_objects; i++)
         {
              status = fbe_cli_get_bus_enclosure_slot_info(downstream_raid_object_list.downstream_object_list[i], 
                                                      FBE_CLASS_ID_PROVISION_DRIVE,
                                                      &port_num,
                                                      &encl_num,
                                                      &slot_num,
                                                      FBE_PACKAGE_ID_SEP_0);
              if (status != FBE_STATUS_OK)
              {
                  fbe_cli_printf("sepls: ERROR: Not able to get the lifecycle for RG object id 0x%x\n", 
                                 rg_object_id);
                  return;
              }
              fbe_cli_printf(" %d_%d_%d ",port_num,encl_num,slot_num);
         }
    }
    fbe_cli_printf("\n");
   
    /* For RAID type RAID10, call same function recursively to display its mirror objects */
    if(raid_group_info.raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
            for (i = 0; i < downstream_raid_object_list.number_of_downstream_objects; i++)
           {
               fbe_cli_sepls_print_rg_details(downstream_raid_object_list.downstream_object_list[i], rg_details,exported_block_size);
           }
    }
    else
    {
       *exported_block_size= raid_group_info.exported_block_size;
    }
}
/******************************************
 * end fbe_cli_luninfo_print_lun_details()
 ******************************************/

/**************************************************************************
*                      fbe_cli_sepls_print_vd_details ()                 *
***************************************************************************
*
*  @brief
*    This function prints the information of a single VD.
*
*
*  @param   vd_object_id - Virtual drive object id
* 
*  @return
*    None
*
*  @version
*    01/11/2012 - Created. Trupti Ghate
*
***************************************************************************/
void fbe_cli_sepls_print_vd_details(fbe_object_id_t vd_object_id, fbe_block_size_t exported_block_size)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_raid_group_get_info_t                   virtual_drive_info;
    fbe_lba_t                                       exported_capacity;
    fbe_api_virtual_drive_get_configuration_t       configuration_mode;
    fbe_virtual_drive_configuration_mode_t          config_mode;
    fbe_u32_t                                       index;
    fbe_lifecycle_state_t                           lifecycle_state;
    const char                                     *lifecycle_state_str;
    fbe_port_number_t                               port_num;
    fbe_enclosure_number_t                          encl_num;
    fbe_enclosure_slot_number_t                     slot_num;
    fbe_api_provision_drive_info_t                  provision_drive_info;
    fbe_u32_t                                       capacity_of_vd;
    fbe_bool_t                                      b_display = FBE_FALSE;
    fbe_api_base_config_downstream_object_list_t    downstream_pvd_object_list;
    fbe_u32_t                                       primary_index;
    fbe_u32_t                                       secondary_index;

    /* Check user options to display VD is set or not 
     */
    fbe_cli_sepls_is_display_allowed(FBE_SEPLS_OBJECT_TYPE_VD, &b_display);
    if (b_display == FBE_FALSE)
    {
        /* Parent methods will always call the child method so simply ignore this.
         */
        return;
    }

    /* Get the parent class (i.e. raid group) information.
     */
    status = fbe_api_raid_group_get_info(vd_object_id, &virtual_drive_info, FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nFailed to get virtual drive info:status: 0x%x\n", status);
        return;
    }

    /* Get the virtual drive configuration mode.
     */
    status = fbe_api_virtual_drive_get_configuration(vd_object_id, &configuration_mode);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nFailed to get virtual drive config mode:status: 0x%x\n", status);
        return;
    }
    config_mode = configuration_mode.configuration_mode;

    /* Display the basic information.
     */
    if (display_format == FBE_SEPLS_DISPLAY_OBJECT_TREE)
    {
        fbe_cli_printf("%s", vd_object_id == object_id_arg? "*":" ");
        fbe_cli_printf("  ");
        fbe_cli_printf("%-7s","VD");
    }
    else
    {
        fbe_cli_printf("%-9s","VD");
    }
    fbe_cli_printf("0x%-3x :%-6d",vd_object_id, vd_object_id);
    fbe_cli_printf("%-6s","-");

    /* Calculate the `exported' (i.e. don't include the virtual drive metadata)
     * capacity.
     */
    exported_capacity = (virtual_drive_info.raid_capacity - virtual_drive_info.paged_metadata_capacity);

    /* Display capacity in MB */
    capacity_of_vd = (fbe_u32_t)((exported_capacity * exported_block_size)/(1024*1024));
    fbe_cli_printf("%-7d",capacity_of_vd);
    fbe_cli_printf("%-9s","MB");

    status = fbe_api_get_object_lifecycle_state(vd_object_id, &lifecycle_state, FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s Get lifecycle failed: obj id: 0x%x status: 0x%x \n",
                       __FUNCTION__, vd_object_id, status);
        return;
    }
    fbe_cli_get_state_name(lifecycle_state, &lifecycle_state_str);

    /* Now determine the primary index.
     */
    switch (config_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
            primary_index = 0;
            secondary_index = 1;
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            primary_index = 1;
            secondary_index = 0;
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
            primary_index = 0;
            secondary_index = 1;
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            primary_index = 1;
            secondary_index = 0;
            break;
    }

    /* Now get the downstream information for the primary.
     */
    status = fbe_api_base_config_get_downstream_object_list(vd_object_id, &downstream_pvd_object_list);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nFailed to get downstreamobject ids of object:0x%x status: 0x%x\n", vd_object_id, status);
        return;
    }

    /* If we are in pass-thru mode disply the single pvd info.
     */
    if ((config_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE) ||
        (config_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE)   )
    {
        /* Only (1) downstream object.  Display the first.
         */
        fbe_cli_printf("%-17s", lifecycle_state_str);
        fbe_cli_printf("%-33d ",downstream_pvd_object_list.downstream_object_list[0]);
        status = fbe_cli_get_bus_enclosure_slot_info(downstream_pvd_object_list.downstream_object_list[0], 
                                                FBE_CLASS_ID_PROVISION_DRIVE,
                                                &port_num,
                                                &encl_num,
                                                &slot_num, FBE_PACKAGE_ID_SEP_0);
        if (port_num == FBE_PORT_NUMBER_INVALID)
        {
           fbe_cli_printf("INVALID PORT");
        }
        else
        {
            fbe_cli_printf("%d_%d_%d  ",port_num,encl_num,slot_num);
        }
        fbe_cli_printf("\n");
    }
    else
    {
        /* Else the virtual drive is in copy mode.  Display the percentage complete.
         */
        fbe_u64_t percent = 0;
        fbe_u32_t percentage;
        fbe_lba_t rb_checkpoint = virtual_drive_info.rebuild_checkpoint[secondary_index];
        if (rb_checkpoint >= exported_capacity)
        {
            percentage = 100;
        }
        else
        {
            percent = (rb_checkpoint * 100) / exported_capacity;
            percentage = (fbe_u32_t)percent;
        }
        fbe_cli_printf("%-6s", lifecycle_state_str);
        fbe_cli_printf("COPY:%3d%%", percentage);
        fbe_cli_printf("%-2s"," ");

        /* Based on the copy direction print the source and destination locations.
         */
        if (config_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)
        {
            /* Display the source (i.e. primary) location first.
             */
            fbe_cli_printf("%-3d -> %-27d", 
                           downstream_pvd_object_list.downstream_object_list[primary_index], 
                           downstream_pvd_object_list.downstream_object_list[secondary_index]);
            status = fbe_cli_get_bus_enclosure_slot_info(downstream_pvd_object_list.downstream_object_list[primary_index], 
                                                FBE_CLASS_ID_PROVISION_DRIVE,
                                                &port_num,
                                                &encl_num,
                                                &slot_num, FBE_PACKAGE_ID_SEP_0);
            if (port_num == FBE_PORT_NUMBER_INVALID)
            {
                fbe_cli_printf("INVALID PORT");
            }
            else
            {
                fbe_cli_printf("%d_%d_%d",port_num,encl_num,slot_num);
            }
            fbe_cli_printf("  -> ");
            status = fbe_cli_get_bus_enclosure_slot_info(downstream_pvd_object_list.downstream_object_list[secondary_index], 
                                                FBE_CLASS_ID_PROVISION_DRIVE,
                                                &port_num,
                                                &encl_num,
                                                &slot_num, FBE_PACKAGE_ID_SEP_0);
            if (port_num == FBE_PORT_NUMBER_INVALID)
            {
                fbe_cli_printf("INVALID PORT");
            }
            else
            {
                fbe_cli_printf("%d_%d_%d",port_num,encl_num,slot_num);
            }
            fbe_cli_printf("\n");
        }
        else
        {
            /* Else display the secondary first.
             */
            fbe_cli_printf("%-3d <- %-27d", 
                           downstream_pvd_object_list.downstream_object_list[secondary_index], 
                           downstream_pvd_object_list.downstream_object_list[primary_index]);
            status = fbe_cli_get_bus_enclosure_slot_info(downstream_pvd_object_list.downstream_object_list[secondary_index], 
                                                FBE_CLASS_ID_PROVISION_DRIVE,
                                                &port_num,
                                                &encl_num,
                                                &slot_num, FBE_PACKAGE_ID_SEP_0);
            if (port_num == FBE_PORT_NUMBER_INVALID)
            {
                fbe_cli_printf("INVALID PORT");
            }
            else
            {
                fbe_cli_printf("%d_%d_%d",port_num,encl_num,slot_num);
            }
            fbe_cli_printf("  <- ");
            status = fbe_cli_get_bus_enclosure_slot_info(downstream_pvd_object_list.downstream_object_list[primary_index], 
                                                FBE_CLASS_ID_PROVISION_DRIVE,
                                                &port_num,
                                                &encl_num,
                                                &slot_num, FBE_PACKAGE_ID_SEP_0);
            if (port_num == FBE_PORT_NUMBER_INVALID)
            {
                fbe_cli_printf("INVALID PORT");
            }
            else
            {
                fbe_cli_printf("%d_%d_%d",port_num,encl_num,slot_num);
            }
            fbe_cli_printf("\n");
        }

    } /* end else in mirror mode */

    /* If configured, display the downstream object tree.
     */
    if (display_format == FBE_SEPLS_DISPLAY_OBJECT_TREE)
    {
        for (index = 0; index < downstream_pvd_object_list.number_of_downstream_objects; index++)
        {
            status = fbe_api_provision_drive_get_info(downstream_pvd_object_list.downstream_object_list[index],&provision_drive_info);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("\nFailed to get provision drive information for object id:0x%x status: 0x%x\n", vd_object_id, status);
                return;
            }
            fbe_cli_sepls_print_pvd_details(downstream_pvd_object_list.downstream_object_list[index], &provision_drive_info);
        }
    }

    return;
}

/************************************************************************
 **                       End of fbe_cli_sepls_print_vd_details ()                **
 ************************************************************************/
/**************************************************************************
*                      fbe_cli_sepls_print_pvd_details ()                 *
***************************************************************************
*
*  @brief
*    This function prints the information of a single PVD.
*
*
*  @param
*          pvd_id - PVD Object id
*  @return
*    None
*
*  @version
*    01/11/2012 - Created. Trupti Ghate
*    10/25/2012 - Added Serial Number with PVD slot location. Wenxuan Yin
*
***************************************************************************/
void fbe_cli_sepls_print_pvd_details(fbe_object_id_t pvd_id, fbe_api_provision_drive_info_t *provision_drive_info)
{
    fbe_status_t                 status;

    fbe_u32_t                 i=0  ;
    fbe_lifecycle_state_t lifecycle_state;
    const char *             lifecycle_state_str;
    fbe_u32_t                capacity_of_pvd = -1;
    fbe_lba_t                 zero_chkpt = 0;   
    fbe_u16_t                sniff_percent;
    fbe_u16_t                zero_percent;
    fbe_lba_t                 verify_invalidate_chkpt = 0;   
    fbe_u16_t                verify_invalidate_percent;
    fbe_provision_drive_get_verify_status_t  sniff_verify;
    fbe_provision_drive_set_priorities_t       pvd_get_priorities;
    fbe_object_id_t                                    pdo_obj_id = FBE_OBJECT_ID_INVALID;
    fbe_bool_t                                           b_display = FBE_FALSE;
    fbe_bool_t                                           b_is_invalid_block_size = FBE_FALSE;
    /* Check user options to display PVD is set or not 
     */
    fbe_cli_sepls_is_display_allowed(FBE_SEPLS_OBJECT_TYPE_PVD, &b_display);

    if(b_display == FBE_TRUE)
    {
        /* Return if PVD is already displayed 
         */
        for(i=0; i< total_pvd_ids; i++)
        {
            if(pvd_ids[i] == pvd_id)
            {
                return;
            } 
        }
        if( display_format == FBE_SEPLS_DISPLAY_OBJECT_TREE)
        {
            fbe_cli_printf("%s", pvd_id == object_id_arg? "*":" ");
            fbe_cli_printf("   ");
            fbe_cli_printf("%-6s","PVD");
        }
        else
        {
            fbe_cli_printf("%-9s","PVD");
        }
        fbe_cli_printf("0x%-3x :%-6d",pvd_id, pvd_id);
        fbe_cli_printf("%-6s","-");

        /* Calculate capacity of PVD in megabytes
         */
        if ((provision_drive_info->configured_physical_block_size == FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_520)  ||
            (provision_drive_info->configured_physical_block_size == FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_4160)    )
        {
            /* Capacity is always is terms of 520-bps */
            capacity_of_pvd = (fbe_u32_t)((provision_drive_info->capacity * 520)/(1024*1024));
            fbe_cli_printf("%-7d",capacity_of_pvd);
        }
        else
        {
            fbe_cli_printf("%-16s","INVALID BLOCK SIZE");
            b_is_invalid_block_size = FBE_TRUE;
        }

        if(provision_drive_info->config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE)
        {
            fbe_cli_printf("%-2s","MB");
            fbe_cli_printf("%s","(TEST)");
        }
        else if(b_is_invalid_block_size == FBE_FALSE)
        {
            fbe_cli_printf("%-9s","MB");
        }
        
        status = fbe_api_get_object_lifecycle_state(pvd_id, &lifecycle_state, FBE_PACKAGE_ID_SEP_0);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s Get lifecycle failed: obj id: 0x%x status: 0x%x \n",
                           __FUNCTION__, pvd_id, status);
            return;
        }
        fbe_cli_get_state_name(lifecycle_state, &lifecycle_state_str);
        fbe_cli_printf("%-6s", lifecycle_state_str);
        if (lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE)
        {
            fbe_cli_printf("\n");
            return;
        }

        status = fbe_api_provision_drive_get_zero_checkpoint(pvd_id, &zero_chkpt);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("Failed to get zero checkpoint for object %d\n", pvd_id);
            return;
        }

        status = fbe_api_provision_drive_get_verify_invalidate_checkpoint(pvd_id, &verify_invalidate_chkpt);
        if ((status != FBE_STATUS_OK))
        {
               fbe_cli_printf("Failed to get verify invalidate checkpoint for object %d\n", pvd_id);
               return;
        }

        status = fbe_api_provision_drive_get_verify_status(pvd_id, FBE_PACKET_FLAG_NO_ATTRIB, &sniff_verify);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("Failed to get verify status for object %d\n", pvd_id);
            return;
        }

        status = fbe_api_provision_drive_get_background_priorities(pvd_id, &pvd_get_priorities);
        if (status != FBE_STATUS_OK)
        {
               fbe_cli_printf("Failed to get background zero priorities for object %d\n", pvd_id);
               return;
        }
        if(lifecycle_state == FBE_LIFECYCLE_STATE_READY)
        {
            if ((verify_invalidate_chkpt != FBE_LBA_INVALID) && verify_invalidate_chkpt > 0 )
            {
                verify_invalidate_percent = (fbe_u16_t) ((verify_invalidate_chkpt * 100) / provision_drive_info->capacity);
                if(verify_invalidate_percent > 100)
                {
                    fbe_cli_printf("%-12s"," ");
                }
                else
                {
                    fbe_cli_printf("VRINV:%3d%%",verify_invalidate_percent);
                    fbe_cli_printf("%-3s"," ");
                }
            }
            else if ((zero_chkpt != FBE_LBA_INVALID) && zero_chkpt > 0 )
            {
                zero_percent = (fbe_u16_t) ((zero_chkpt * 100) / provision_drive_info->capacity);
                if(zero_percent > 100)
                {
                    fbe_cli_printf("%-12s"," ");
                }
                else
                {
                    fbe_cli_printf("ZER:%3d%%",zero_percent);
                    fbe_cli_printf("%-3s"," ");
                }
            }
            else if ((sniff_verify.verify_checkpoint!= FBE_LBA_INVALID) && sniff_verify.verify_checkpoint > 0 )
            {
                sniff_percent = (fbe_u16_t) ((sniff_verify.verify_checkpoint * 100) / provision_drive_info->capacity);
                if (sniff_percent > 100)
                {
                    fbe_cli_printf("%-12s"," ");

                } 
                else
                {
                    fbe_cli_printf("SV:%3d%%",sniff_percent);
                    fbe_cli_printf("%-4s"," ");
                }
            }
            else
            {
                fbe_cli_printf("%-11s"," ");
            }
            
        }
        else
        {
            fbe_cli_printf("%-11s"," ");
        }
       /* Display ID of downstream object */
        status = fbe_api_get_physical_drive_object_id_by_location(provision_drive_info->port_number, 
                                                              provision_drive_info->enclosure_number, 
                                                              provision_drive_info->slot_number,
                                                              &pdo_obj_id);
        if (status != FBE_STATUS_OK)
        {
               fbe_cli_printf("%-33s ","Failed to get PDO object ID \n");
               return;
        }
        if ((pdo_obj_id == FBE_OBJECT_ID_INVALID)||(lifecycle_state == FBE_LIFECYCLE_STATE_FAIL))
        {
               fbe_cli_printf("%-33s ","-");
        }
        else
        {
            fbe_cli_printf("%-33d ", pdo_obj_id);
        }

       fbe_cli_printf("%d_%d_%d  ",provision_drive_info->port_number,provision_drive_info->enclosure_number, provision_drive_info->slot_number);
       fbe_cli_printf("SN:%s", provision_drive_info->serial_num);

       pvd_ids[total_pvd_ids] = pvd_id;
       total_pvd_ids++;
       fbe_cli_printf("\n");

        if (pdo_obj_id != FBE_OBJECT_ID_INVALID)
        {
            fbe_cli_sepls_print_pdo_details(pdo_obj_id, provision_drive_info->capacity);
        }

   }
}
/************************************************************************
 **                       End of fbe_cli_sepls_print_pvd_details ()                **
 ************************************************************************/

/**************************************************************************
*                      fbe_cli_sepls_print_ldo_details ()                 *
***************************************************************************
*
*  @brief
*    This function prints the information of a LDO
*
*
*  @param
*          ldo_obj_id - LDO Object id
*  @return
*    None
*
*  @version
*    01/11/2012 - Created. Trupti Ghate
*
***************************************************************************/
void fbe_cli_sepls_print_ldo_details(fbe_object_id_t ldo_obj_id, fbe_u32_t capacity)
{
    fbe_status_t                 status;
    fbe_lifecycle_state_t      lifecycle_state;
    const char *                  lifecycle_state_str;
    fbe_port_number_t        port_num;
    fbe_enclosure_number_t       encl_num;
    fbe_enclosure_slot_number_t  slot_num;
    fbe_object_id_t                      pdo_obj_id = FBE_OBJECT_ID_INVALID;
    fbe_bool_t                             b_display = FBE_FALSE;
	
    /* Check user options to display PVD is set or not 
     */
    fbe_cli_sepls_is_display_allowed(FBE_SEPLS_OBJECT_TYPE_LDO, &b_display);
    if(b_display == FBE_TRUE)
    {
        if( display_format == FBE_SEPLS_DISPLAY_OBJECT_TREE)
        {
            fbe_cli_printf("%s", ldo_obj_id == object_id_arg? "*":" ");
            fbe_cli_printf("    ");
            fbe_cli_printf("%-5s","LDO");
        }
        else
        {
            fbe_cli_printf("%-9s","LDO");
        }
        fbe_cli_printf("0x%-3x :%-6d",ldo_obj_id, ldo_obj_id);
        fbe_cli_printf("%-6s","-");
        fbe_cli_printf("%-7d",capacity);
        fbe_cli_printf("%-9s","MB");
        status = fbe_api_get_object_lifecycle_state(ldo_obj_id, &lifecycle_state, FBE_PACKAGE_ID_PHYSICAL);
        if (status != FBE_STATUS_OK)
        {
               fbe_cli_printf("Failed to get lifecycle state of %d\n",ldo_obj_id);
               return;
        }

        fbe_cli_get_state_name(lifecycle_state, &lifecycle_state_str);
        fbe_cli_printf("%-17s", lifecycle_state_str);
    
        status = fbe_cli_get_bus_enclosure_slot_info(ldo_obj_id, 
                                            FBE_CLASS_ID_LOGICAL_DRIVE,
                                            &port_num,
                                            &encl_num,
                                            &slot_num,
                                            FBE_PACKAGE_ID_SEP_0);
        if (status != FBE_STATUS_OK)
        {
               fbe_cli_printf("\nFailed to get BES information of %d\n",ldo_obj_id);
               return;
        }
        status = fbe_api_get_physical_drive_object_id_by_location(port_num,encl_num,slot_num, &pdo_obj_id);
        if (status != FBE_STATUS_OK)
        {
               fbe_cli_printf("%-33s ","Failed to get PDO object ID \n");
               return;
        }

        fbe_cli_printf("%-33d ",pdo_obj_id);
        fbe_cli_printf("%d_%d_%d  ",port_num,encl_num,slot_num);
        fbe_cli_printf("\n");
        if (pdo_obj_id != FBE_OBJECT_ID_INVALID)
        {
            fbe_cli_sepls_print_pdo_details(pdo_obj_id, capacity);
        }
    }
}
/************************************************************************
 **                       End of fbe_cli_sepls_print_ldo_details ()                **
 ************************************************************************/
/**************************************************************************
*                      fbe_cli_sepls_print_pdo_details ()                 *
***************************************************************************
*
*  @brief
*    This function prints the information of a single physical drive object
*
*
*  @param
*          pdo_obj_id - PDO Object id
*  @return
*    None
*
*  @version
*    01/11/2012 - Created. Trupti Ghate
*
***************************************************************************/
void fbe_cli_sepls_print_pdo_details(fbe_object_id_t pdo_obj_id, fbe_lba_t capacity)
{
    fbe_status_t                 status;
    fbe_lifecycle_state_t lifecycle_state;
    const char *            lifecycle_state_str;
    fbe_port_number_t                port_num;
    fbe_enclosure_number_t        encl_num;
    fbe_enclosure_slot_number_t  slot_num;
    fbe_bool_t                             b_display = FBE_FALSE;
    /* Check user options to display PVD is set or not 
     */
    fbe_cli_sepls_is_display_allowed(FBE_SEPLS_OBJECT_TYPE_PDO, &b_display);
    if(b_display == FBE_FALSE)
    {
        return;
    }
    if( display_format == FBE_SEPLS_DISPLAY_OBJECT_TREE)
    {
        fbe_cli_printf("%s", pdo_obj_id == object_id_arg? "*":" ");
        fbe_cli_printf("     ");
        fbe_cli_printf("%-4s","PDO");
    }
    else
    {
        fbe_cli_printf("%-9s","PDO");
    }
    fbe_cli_printf("0x%-3x :%-6d",pdo_obj_id, pdo_obj_id);
    fbe_cli_printf("%-6s","-");
    fbe_cli_printf("%-7llx",capacity);
    fbe_cli_printf("%-9s","MB");

    status = fbe_api_get_object_lifecycle_state(pdo_obj_id, &lifecycle_state, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
           fbe_cli_printf("\nFailed to get lifecycle state  of %d\n",pdo_obj_id);
           return;
    }
    fbe_cli_get_state_name(lifecycle_state, &lifecycle_state_str);
    fbe_cli_printf("%-17s", lifecycle_state_str);

    status = fbe_cli_get_bus_enclosure_slot_info(pdo_obj_id, 
                                        FBE_CLASS_ID_SAS_PHYSICAL_DRIVE,
                                        &port_num,
                                        &encl_num,
                                        &slot_num,
                                        FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK)
    {
           fbe_cli_printf("Failed to get location of %d\n",pdo_obj_id);
           return;
    }

    fbe_cli_printf("%-33s ","-");
    fbe_cli_printf("%d_%d_%d  ",port_num,encl_num,slot_num);
    fbe_cli_printf("\n");
}
/************************************************************************
 **                       End of fbe_cli_sepls_print_pdo_details ()                **
 ************************************************************************/
/**************************************************************************
*                      fbe_cli_sepls_print_object_details ()                 *
***************************************************************************
*
*  @brief
*    This function prints the information of a single physical drive object
*
*
*  @param
*          pdo_obj_id - PDO Object id
*  @return
*    None
*
*  @version
*    01/11/2012 - Created. Trupti Ghate
*
***************************************************************************/
fbe_status_t fbe_cli_sepls_get_rg_object_id(fbe_object_id_t obj_id_arg, fbe_object_id_t* rg_object_id, fbe_package_id_t package_id)
{
    fbe_status_t status;
    fbe_class_id_t class_id;
    fbe_object_id_t obj_id = object_id_arg;
    fbe_cli_lurg_lun_details_t lun_details;
    fbe_api_base_config_upstream_object_list_t  upstream_object_list;
    fbe_api_provision_drive_info_t provision_drive_info;
    fbe_port_number_t            port_num;
    fbe_enclosure_number_t       encl_num;
    fbe_enclosure_slot_number_t  slot_num;
   fbe_api_raid_group_get_info_t raid_group_info;
    while(obj_id != FBE_OBJECT_ID_INVALID)
    {
        /* Get class ID of object */
        status = fbe_api_get_object_class_id(obj_id, &class_id, package_id);
        if (status != FBE_STATUS_OK) 
        {
           return FBE_STATUS_GENERIC_FAILURE;
        }
        /* Return if object is of type Mirror, Parity or striper */
        if(class_id >= FBE_CLASS_ID_MIRROR &&  class_id <= FBE_CLASS_ID_PARITY)
        {
            status = fbe_api_raid_group_get_info(obj_id, &raid_group_info, 
                                         FBE_PACKET_FLAG_NO_ATTRIB);
             if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("rginfo: ERROR: Not able to get the information for RG object id 0x%x\n", 
                               obj_id);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            if(raid_group_info.raid_type != FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER)
            {
                *rg_object_id = obj_id;
                return FBE_STATUS_OK;
            }
        }
    
        /* Find out the RG object ID of a LUN*/
        if(class_id == FBE_CLASS_ID_LUN)
        {
            /*Get lun details*/
            status = fbe_cli_get_lun_details(obj_id,&lun_details);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("\nFailed to get details of Lun id %x, Status:0x%x\n",obj_id,status);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            *rg_object_id = lun_details.lun_info.raid_group_obj_id;
            return FBE_STATUS_OK;
        }
        if(class_id > FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST && class_id < FBE_CLASS_ID_PHYSICAL_DRIVE_LAST)
        {
            status = fbe_cli_get_bus_enclosure_slot_info(obj_id, 
                                                class_id,
                                                &port_num,
                                                &encl_num,
                                                &slot_num, FBE_PACKAGE_ID_PHYSICAL);
            if(status != FBE_STATUS_OK)
            {
             fbe_cli_error("\nFail to get the location of the object id :0x%x ,status:0x%x",obj_id,status);
             return status;
            }
            /* physical drive connects to PVD directly now */
            /*
            if(class_id == FBE_CLASS_ID_SAS_PHYSICAL_DRIVE)
            {
                status = fbe_api_get_logical_drive_object_id_by_location(port_num, 
                                                                  encl_num, slot_num, &obj_id);
                if (status != FBE_STATUS_OK)
                {
                   fbe_cli_printf("%-33s ","Failed to get logical object ID\n");
                   return status;
                }
            }
            else 
            */
            {
                 status = fbe_api_provision_drive_get_obj_id_by_location(port_num,encl_num,
                                                          slot_num, &obj_id);
                if (status != FBE_STATUS_OK)
                {
                   fbe_cli_printf("%-33s ","Failed to get pvd object ID\n");
                   return status;
                }
                package_id = FBE_PACKAGE_ID_SEP_0;
            }
            continue;
         }
         status = fbe_api_base_config_get_upstream_object_list(obj_id, &upstream_object_list);
         if(status != FBE_STATUS_OK)
         {
             fbe_cli_error("\nFail to upstream  Object list for object id :0x%x ,status:0x%x",obj_id,status);
             return status;
         }
         if(upstream_object_list.number_of_upstream_objects > 0)
         {
             obj_id = upstream_object_list.upstream_object_list[0];
         }
         else
         {
             break;
         }
    }
    if(class_id == FBE_CLASS_ID_PROVISION_DRIVE)
    {
        status = fbe_api_provision_drive_get_info(obj_id,&provision_drive_info);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_error("\nFailed to get provision drive information for object id:0x%x status: 0x%x\n", obj_id, status);
            return FBE_STATUS_OK;
        }
        fbe_cli_sepls_print_pvd_details(obj_id,  &provision_drive_info);
    }
    return FBE_STATUS_OK;
}
/*!**************************************************************
 *          fbe_cli_sepls_display_raid_type()
 ****************************************************************
 *
 * @brief   Display raid group debug flag information
 *
 * @param   rg_debug_flag_info : library debug flag info 
 *
 * @return  None.
 *
 * @author
 *  01/01/2012 - Created. Trupti Ghate
 *
 ****************************************************************/
void fbe_cli_sepls_display_raid_type(
                       fbe_raid_group_type_t raid_type)
{
    fbe_u8_t *raid_type_str = NULL;
    switch(raid_type)
    {
    case FBE_RAID_GROUP_TYPE_RAID1:
        raid_type_str = "RAID1";
        break;
    case FBE_RAID_GROUP_TYPE_RAW_MIRROR:
        raid_type_str = "RAW-MIRROR";
        break;
    case FBE_RAID_GROUP_TYPE_RAID10:
        raid_type_str = "RAID10";
        break;
    case FBE_RAID_GROUP_TYPE_RAID3:
        raid_type_str = "RAID3";
        break;
    case FBE_RAID_GROUP_TYPE_RAID0:
        raid_type_str = "RAID0";
        break;
    case FBE_RAID_GROUP_TYPE_RAID5:
        raid_type_str = "RAID5";
        break;
    case FBE_RAID_GROUP_TYPE_RAID6:
        raid_type_str = "RAID6";
        break;
    case FBE_RAID_GROUP_TYPE_SPARE:
        raid_type_str = "SPARE";
        break;
    case FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER:
        raid_type_str = "MIRROR";
        break;
    default:
        raid_type_str = "UNKNOWN";
        break;

    }
    fbe_cli_printf("%-14s  ", raid_type_str);
    return;
}
/******************************************
 * end fbe_cli_rginfo_display_raid_type()
 ******************************************/
/*!**************************************************************
 *          fbe_cli_sepls_is_display_allowed()
 ****************************************************************
 *
 * @brief   This function checks the display option specified via command line args,
               and returns True if object of 'object_type' should be displayed or not.
 *
 * @param   rg_debug_flag_info : library debug flag info 
 *
 * @return  None.
 *
 * @author
 *  01/01/2012 - Created. Trupti Ghate
 *
 ****************************************************************/
void fbe_cli_sepls_is_display_allowed(fbe_sepls_object_type_t object_type, 
                                                                         fbe_bool_t* b_display)
{
    *b_display = FBE_FALSE;
    if(b_display_allsep == FBE_TRUE || display_format == FBE_SEPLS_DISPLAY_OBJECT_TREE)
    {
        *b_display = FBE_TRUE;
        return;
    }
   switch(object_type)
   {
       case  FBE_SEPLS_OBJECT_TYPE_RG:
           if(display_format == FBE_SEPLS_DISPLAY_ONLY_RG_OBJECTS ||
               display_format >= FBE_SEPLS_DISPLAY_LEVEL_1)  
          {
               *b_display = FBE_TRUE;
               return;
           }
           break;
       case FBE_SEPLS_OBJECT_TYPE_LUN:

           if(display_format == FBE_SEPLS_DISPLAY_ONLY_LUN_OBJECTS ||
               display_format >= FBE_SEPLS_DISPLAY_LEVEL_2)  
          {
               *b_display = FBE_TRUE;
               return;
           }
           break;
       case FBE_SEPLS_OBJECT_TYPE_VD:
           if(display_format == FBE_SEPLS_DISPLAY_ONLY_VD_OBJECTS ||
               display_format >= FBE_SEPLS_DISPLAY_LEVEL_3)  
          {
               *b_display = FBE_TRUE;
               return;
           }
           break;
       case FBE_SEPLS_OBJECT_TYPE_PVD:
           if(display_format == FBE_SEPLS_DISPLAY_ONLY_PVD_OBJECTS ||
               display_format >= FBE_SEPLS_DISPLAY_LEVEL_4)  
          {
               *b_display = FBE_TRUE;
               return;
           }
           break;
        default:
           {
               *b_display = FBE_FALSE;
           }
       }
}


/*!**************************************************************
 * fbe_cli_sepls_calculate_rebuild_capacity()
 ****************************************************************
 * @brief
 *  Calculate the rebuild capacity which will help in calculating the
 *  rebuild percentage.
 *  
 * @param fbe_sepls_rg_config_t - Pointer to the RG data structure where we 
 *                                 populate the information.
 * 
 * @return fbe_lba_t - Rebuild capacity
 *
 * @author
 *  02/03/2011 - Created. Trupti Ghate
 *
 ****************************************************************/
fbe_lba_t fbe_cli_sepls_calculate_rebuild_capacity(fbe_lba_t total_lun_capacity,  fbe_u32_t rg_width, fbe_raid_group_type_t raid_type)
{
    fbe_lba_t rebuild_capacity;
    switch(raid_type)
    {
        case FBE_RAID_GROUP_TYPE_RAID3:
        case FBE_RAID_GROUP_TYPE_RAID5:
        {
            rebuild_capacity = total_lun_capacity/(rg_width - 1);
            break;
        }
        case FBE_RAID_GROUP_TYPE_RAID1:
        case FBE_RAID_GROUP_TYPE_RAW_MIRROR:
        {
            rebuild_capacity = total_lun_capacity;
            break;
        }
        case FBE_RAID_GROUP_TYPE_RAID0:
        {
            rebuild_capacity = total_lun_capacity;
            break;
        }
        case FBE_RAID_GROUP_TYPE_RAID6:
        {
            rebuild_capacity = total_lun_capacity/(rg_width - 2);
            break;
        }
        case FBE_RAID_GROUP_TYPE_RAID10:
        case FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER:
        {
            rebuild_capacity = total_lun_capacity/(rg_width);
            break;
        }
        default:
            rebuild_capacity = FBE_LBA_INVALID;
    }
    return rebuild_capacity;
}
/********************************************
 * end fbe_sepls_calculate_rebuild_capacity
 ********************************************/


/************************************************************************
 **                       End of fbe_cli_cmd_sepls ()                **
 ************************************************************************/
