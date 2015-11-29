/***************************************************************************
 * Copyright (C) EMC Corporation 2011 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
* @file fbe_cli_lib_luninfo_cmds.c
 ***************************************************************************
 *
 * @brief
*  This file contains cli functions displays lun information in FBE CLI.
 *
 * @ingroup fbe_cli
 *
 * @date
 * 5/5/2011 - Created. Harshal Wanjari 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
//#include <math.h>
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
#include "fbe_cli_rginfo.h"
#include "fbe_bvd_interface.h"
#include "fbe_api_bvd_interface.h"

/*!**************************************************************
 * fbe_cli_lib_lun_map_lba()
 ****************************************************************
 * @brief
 *  Map a lun's lba to a physical address.
 *
 * @param lun_object_id - object_id.
 * @param lba - Logical address to map.
 *
 * @return None.
 *
 * @author
 *  7/10/2012 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_cli_lib_lun_map_lba(fbe_object_id_t lun_object_id, fbe_lba_t lba)
{

    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_raid_group_map_info_t map_info;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_database_lun_info_t db_lun_info;

    db_lun_info.lun_object_id = lun_object_id;
    status = fbe_api_database_get_lun_info(&db_lun_info);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_cli_error("luninfo: map lba to pba:  db get lun info error status: 0x%x\n", status);
        return; 
    }
    /* Get the current value of the raid group debug flags.
     */
    map_info.lba = lba;
    status = fbe_api_lun_map_lba(lun_object_id, &map_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("luninfo: ERROR: error doing map "
                       "for lun obj id %d lba: 0x%llx\n", lun_object_id, (unsigned long long)lba);
        return;
    }

    status = fbe_api_raid_group_get_info(db_lun_info.raid_group_obj_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("luninfo: ERROR: error getting rg info "
                       "for lun obj id %d lba: 0x%llx\n", lun_object_id, (unsigned long long)lba);
        return;
    }
    fbe_cli_printf("\n");
    fbe_cli_printf("LUN number 0x%x (%d) LUNlun object id 0x%x (%d)\n",
                   db_lun_info.lun_number, db_lun_info.lun_number, db_lun_info.lun_object_id, db_lun_info.lun_object_id);
    fbe_cli_printf("Raid Group Number: 0x%x (%d) object id: 0x%x (%d)\n", 
                   db_lun_info.rg_number, db_lun_info.rg_number, 
                   db_lun_info.raid_group_obj_id, db_lun_info.raid_group_obj_id);
    fbe_cli_printf("LUN lba: 0x%llx\n", (unsigned long long)lba);
    fbe_cli_printf("LUN offset: 0x%llx\n", (unsigned long long)db_lun_info.offset);
    fbe_cli_printf("RG  lba: 0x%llx\n", (unsigned long long)map_info.lba);
    fbe_cli_printf("pba: 0x%llx\n", (unsigned long long)map_info.pba);
    fbe_cli_printf("data/primary pos:      %d\n", map_info.data_pos);
    fbe_cli_printf("parity/secondary pos:  %d\n", map_info.parity_pos);
    fbe_cli_printf("chunk_index:  %d\n", (int)map_info.chunk_index);
    fbe_cli_printf("metadata lba: 0x%llx\n", (unsigned long long)map_info.metadata_lba);
    fbe_cli_printf("\n");
    fbe_cli_rginfo_display_raid_type(rg_info.raid_type);
    fbe_cli_printf("width:               %d\n", rg_info.width);
    fbe_cli_printf("element size:        %d\n", rg_info.element_size);
    fbe_cli_printf("elements per parity: %d\n", rg_info.elements_per_parity_stripe);
    fbe_cli_printf("offset:              0x%llx\n", (unsigned long long)map_info.offset);
}
/******************************************
 * end fbe_cli_lib_lun_map_lba()
 ******************************************/

/*!**************************************************************
 * fbe_cli_lib_lun_display_raid_info()
 ****************************************************************
 * @brief
 *  Display the LUN's information.
 *
 * @param lun_object_id - object_id.
 *
 * @return None.
 *
 * @author
 *  4/17/2013 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_cli_lib_lun_display_raid_info(fbe_object_id_t lun_object_id)
{

    fbe_status_t status = FBE_STATUS_OK;
    fbe_database_get_sep_shim_raid_info_t raid_info;
    status = fbe_api_database_get_lun_raid_info(lun_object_id, &raid_info);

    if (status != FBE_STATUS_OK) 
    { 
        fbe_cli_error("luninfo: get_lun_raid_info error 0x%x\n", status);
        return; 
    }
    fbe_cli_printf("object_id:                  0x%x\n", raid_info.lun_object_id);
    fbe_cli_printf("max_queue_depth:            %u\n", raid_info.max_queue_depth);
    fbe_cli_printf("width:                      %u\n", raid_info.width);
    fbe_cli_printf("element_size:               0x%x\n", raid_info.element_size);
    fbe_cli_printf("elements_per_party_stripe:  0x%x\n", raid_info.elements_per_parity_stripe);
    fbe_cli_printf("sectors_per_stripe:  0x%llx\n", raid_info.sectors_per_stripe);
    fbe_cli_printf("capacity:                   0x%llx\n", raid_info.capacity);
    fbe_cli_printf("exported_block_size:        0x%x\n", raid_info.exported_block_size);
    fbe_cli_printf("raid_group_number:          %u\n", raid_info.raid_group_number);
    fbe_cli_rginfo_display_raid_type(raid_info.raid_type);
    fbe_cli_printf("rotational_rate:            %u\n", raid_info.rotational_rate);
}
/******************************************
 * end fbe_cli_lib_lun_display_raid_info()
 ******************************************/
/*!*******************************************************************
 * @var fbe_cli_cmd_luninfo
 *********************************************************************
 * @brief Function to implement Lun info commands.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author
 *  04/01/2011 - Created. Vinay Patki
 *********************************************************************/
void fbe_cli_cmd_luninfo(int argc, char** argv)
{
    fbe_status_t                 status;
    fbe_object_id_t         lun_id, rg_id;
    fbe_u32_t                    lun_num, rg_num;
    fbe_bool_t b_clear_fault = FBE_FALSE;
    fbe_bool_t b_disable_fail_on_error = FBE_FALSE;
    fbe_bool_t b_enable_fail_on_error = FBE_FALSE;
    fbe_lba_t lba = FBE_LBA_INVALID;
    fbe_bool_t b_get_raid_info = FBE_FALSE;
    fbe_bool_t b_disable_loopback = FBE_FALSE;
    fbe_bool_t b_enable_loopback = FBE_FALSE;
    fbe_bool_t b_get_loopback = FBE_FALSE;
    fbe_bool_t b_unexport = FBE_FALSE;

  
    if (argc == 0)
    {
        fbe_cli_printf("Luninfo: ERROR: Too few args.\n");
        fbe_cli_printf("%s", LUNINFO_CMD_USAGE);
        return;
    }
    if ((strcmp(*argv, "-help") == 0) ||
        (strcmp(*argv, "-h") == 0))
    {
        /* If they are asking for help, just display help and exit.
        */
        fbe_cli_printf("%s", LUNINFO_CMD_USAGE);
        return;
    }

    /* Parse args
     */
    if(argc > 0)
    {
        if((strcmp(*argv, "-clear_fault") == 0))
        {
            argc--;
            argv++;
            b_clear_fault = FBE_TRUE;
        }
        else if((strcmp(*argv, "-disable_fail_on_error") == 0))
        {
            argc--;
            argv++;
            b_disable_fail_on_error = FBE_TRUE;
        }
        else if((strcmp(*argv, "-enable_fail_on_error") == 0))
        {
            argc--;
            argv++;
            b_enable_fail_on_error = FBE_TRUE;
        }
        else if((strcmp(*argv, "-disable_loopback") == 0))
        {
            argc--;
            argv++;
            b_disable_loopback = FBE_TRUE;
        }
        else if((strcmp(*argv, "-enable_loopback") == 0))
        {
            argc--;
            argv++;
            b_enable_loopback = FBE_TRUE;
        }
        else if((strcmp(*argv, "-get_loopback") == 0))
        {
            argc--;
            argv++;
            b_get_loopback = FBE_TRUE;
        }
        else if ((strcmp(*argv, "-unexport") == 0))
        {
            argc--;
            argv++;
            b_unexport = FBE_TRUE;
        }
        else if((strcmp(*argv, "-map_lba") == 0))
        {
            fbe_char_t *tmp_ptr = NULL;
            argc--;
            argv++;
            /* argc should not be less than 0 */
            if (argc == 0)
            {
                fbe_cli_printf("luninfo: ERROR: -map_lba lba expected\n");
                fbe_cli_printf("%s", LUNINFO_CMD_USAGE);
                return;
            }
            
            /* get the offset*/
            tmp_ptr = *argv;
            if ((*tmp_ptr == '0') && 
                (*(tmp_ptr + 1) == 'x' || *(tmp_ptr + 1) == 'X'))
            {
                tmp_ptr += 2;
            }
            status = fbe_atoh64(tmp_ptr, &lba);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("luninfo: error processing -map_lba lba\n");
                return;
            }
            argc--;
            argv++;
        }
        else if((strcmp(*argv, "-raid_info") == 0))
        {
            argc--;
            argv++;
	     b_get_raid_info = FBE_TRUE;
        }
        if ( ((argc == 0) || (strcmp(*argv, "-lun") != 0)) && 
             (b_get_raid_info||b_clear_fault || b_disable_fail_on_error || b_enable_fail_on_error ||
              b_get_loopback || b_disable_loopback || b_enable_loopback ||
             (lba != FBE_LBA_INVALID)) )
        {
            fbe_cli_error("-lun argument expected\n");
            return;
        }
        if ( ((argc == 0) || (strcmp(*argv, "-lun") != 0 && strcmp(*argv, "-all") != 0)) && (b_unexport) )
        {
            fbe_cli_error("-lun or -all argument expected\n");
            return;
        }
        if ((argc > 0) && ((strcmp(*argv, "-lun") == 0)))
        {
            argc--;
            argv++;
            if (argc == 0)
            {
                fbe_cli_error("LUN number argument is expected.\n");
                return;
            }

            lun_num = atoi(*argv);

            status = fbe_api_database_lookup_lun_by_number(lun_num, &lun_id);
            if (status == FBE_STATUS_GENERIC_FAILURE)
            {
                fbe_cli_error("LUN %d does not exist or unable to retrieve information for this LUN.\n",lun_num);
                return;
            }
            if (lba != FBE_LBA_INVALID)
            {
                fbe_cli_lib_lun_map_lba(lun_id, lba);
                return;
            }
            if (b_get_raid_info)
            {
                fbe_cli_lib_lun_display_raid_info(lun_id);
                return;
            }
            else if (b_clear_fault)
            {
                status = fbe_api_lun_clear_unexpected_errors(lun_id);
                if (status == FBE_STATUS_OK)
                {
                    fbe_cli_printf("LUN %d (obj 0x%x) fault cleared\n", lun_num, lun_id);
                    return;
                }
                else
                {
                    fbe_cli_error("LUN %d (obj 0x%x) failure %d on clearing fault\n", lun_num, lun_id, status);
                    return;
                }
            }
            else if (b_disable_fail_on_error)
            {
                status = fbe_api_lun_disable_unexpected_errors(lun_id);
                if (status == FBE_STATUS_OK)
                {
                    fbe_cli_printf("LUN %d (obj 0x%x) unexpected errors disabled\n", lun_num, lun_id);
                    return;
                }
                else
                {
                    fbe_cli_error("LUN %d (obj 0x%x) failure %d on disable unexpected errors\n", lun_num, lun_id, status);
                    return;
                }
            }
            else if (b_enable_fail_on_error)
            {
                status = fbe_api_lun_enable_unexpected_errors(lun_id);
                if (status == FBE_STATUS_OK)
                {
                    fbe_cli_printf("LUN %d (obj 0x%x) unexpected errors enabled\n", lun_num, lun_id);
                    return;
                }
                else
                {
                    fbe_cli_error("LUN %d (obj 0x%x) failure %d on enable unexpected errors\n", lun_num, lun_id, status);
                    return;
                }
            }
            else if (b_get_loopback)
            {
                fbe_bool_t enabled;
                status = fbe_api_lun_get_io_loopback(lun_id, &enabled);
                if (status == FBE_STATUS_OK)
                {
                    fbe_cli_printf("LUN %d (obj 0x%x) loopback %s\n", lun_num, lun_id, enabled ? "enabled" : "disabled");
                    return;
                }
                else
                {
                    fbe_cli_error("LUN %d (obj 0x%x) failure %d on getting loopbackinfo\n", lun_num, lun_id, status);
                    return;
                }
            }
            else if (b_enable_loopback)
            {
                status = fbe_api_lun_enable_io_loopback(lun_id);
                if (status == FBE_STATUS_OK)
                {
                    fbe_cli_printf("LUN %d (obj 0x%x) loopback enabled\n", lun_num, lun_id);
                    return;
                }
                else
                {
                    fbe_cli_error("LUN %d (obj 0x%x) failure %d on enabling loopback\n", lun_num, lun_id, status);
                    return;
                }
            }
            else if (b_disable_loopback)
            {
                status = fbe_api_lun_disable_io_loopback(lun_id);
                if (status == FBE_STATUS_OK)
                {
                    fbe_cli_printf("LUN %d (obj 0x%x) loopback disabled\n", lun_num, lun_id);
                    return;
                }
                else
                {
                    fbe_cli_error("LUN %d (obj 0x%x) failure %d on disabling loopback\n", lun_num, lun_id, status);
                    return;
                }
            }
            else if (b_unexport)
            {
                status = fbe_api_bvd_interface_unexport_lun(lun_id);
                if (status == FBE_STATUS_OK)
                {
                    fbe_cli_printf("LUN %d (obj 0x%x) device unexported\n", lun_num, lun_id);
                    return;
                }
                else if (status == FBE_STATUS_NO_DEVICE)
                {
                    fbe_cli_printf("LUN %d (obj 0x%x) device not present\n", lun_num, lun_id);
                    return;
                }
                else
                {
                    fbe_cli_error("LUN %d (obj 0x%x) failure %d on unexporting device\n", lun_num, lun_id, status);
                    return;
                }
            }

            /*Print information of a single lun*/
            fbe_cli_luninfo_print_lun_details(lun_id);
            return;
        }
        if((strcmp(*argv, "-rg") == 0))
        {
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("RG number argument is expected\n");
                return;
            }

            rg_num = atoi(*argv);

            /*Print the information only if the RG is valid.*/
            status = fbe_api_database_lookup_raid_group_by_number(rg_num, &rg_id);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_error("RG number argument is expected.\n");
                return;
            }
            /*Print information of all luns in RG*/
            fbe_cli_luninfo_print_lun_in_rg(rg_id);

            return;
        }
        if((strcmp(*argv, "-all") == 0))
        {
            if (b_unexport)
            {
                fbe_cli_luninfo_unexport_all_luns();
                return;
            }
            /*Print information of all luns*/
            fbe_cli_luninfo_print_all_luns();
        }
    }

    return;
}
/******************************************
 * end fbe_cli_cmd_luninfo()
 ******************************************/

/**************************************************************************
*                      fbe_cli_luninfo_print_lun_in_rg ()                 *
***************************************************************************
*
*  @brief
*    This prints the information of luns of RG mentioned by user.
*
*
*  @param
*          rg_id - RG Object id
*
*  @return
*    None
*
*  @version
*    04/05/2011 - Created. Vinay Patki
*
***************************************************************************/
void fbe_cli_luninfo_print_lun_in_rg(fbe_object_id_t rg_id)
{
    fbe_status_t                                 status;
    fbe_u32_t                                    index;
    fbe_cli_lurg_rg_details_t                    rg_details;
    fbe_class_id_t                               class_id;
    fbe_status_t                                 lifecycle_status = FBE_STATUS_INVALID;
    fbe_lifecycle_state_t                        lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;

    /* We want downstream as well as upstream objects for this RG indicate 
    * that by setting corresponding variable in structure
    */
    rg_details.direction_to_fetch_object = DIRECTION_BOTH;

    /* Get the details of RG associated with this LUN */
    status = fbe_cli_get_rg_details(rg_id, &rg_details);
    /* return failure from here since we're not able to know objects related to this */
    if(status == FBE_STATUS_GENERIC_FAILURE)
    {
        return;
    }
    else if(status == FBE_STATUS_BUSY)
    {
        lifecycle_status = fbe_api_get_object_lifecycle_state(rg_id, 
                                                    &lifecycle_state, FBE_PACKAGE_ID_SEP_0);
        if(lifecycle_status != FBE_STATUS_OK)
        {
            fbe_cli_error("Fail to get the Lifecycle State for object id:%d, status:0x%x\n",
                           rg_id,lifecycle_status);
            return;
        }
        else if (lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE)
        {
            fbe_cli_printf("Raid Group is in SPECIALIZE, Object ID 0x%x\n", rg_id);
            return;
        }
    }

    fbe_cli_printf("\nRG %d - Lun information:\n",rg_details.rg_number);
    fbe_cli_printf("==========================\n\n");

    if(rg_details.upstream_object_list.number_of_upstream_objects == 0)
    {
        fbe_cli_printf("RG %d has no luns!!\n\n",rg_details.rg_number);
        return;
    }

    for (index = 0; index < rg_details.upstream_object_list.number_of_upstream_objects; index++)
    {
        /* Find out if given obj ID is for RAID group or LUN */
        status = fbe_api_get_object_class_id(rg_details.upstream_object_list.upstream_object_list[index],
            &class_id, FBE_PACKAGE_ID_SEP_0);
        /* if status is generic failure then continue to next object */
        if (status == FBE_STATUS_GENERIC_FAILURE)
        {
            continue;
        }

        /* Now that we know that the object correspond to LUN 
        */
        fbe_cli_luninfo_print_lun_details(rg_details.upstream_object_list.upstream_object_list[index]);
    }
    return;
}
/******************************************
 * end fbe_cli_luninfo_print_lun_in_rg()
 ******************************************/
/**************************************************************************
*                      fbe_cli_luninfo_print_all_luns ()                 *
***************************************************************************
*
*  @brief
*    This prints the information of all the lun on system, except system luns.
*
*
*  @param
*    None
*
*  @return
*    None
*
*  @version
*    04/05/2011 - Created. Vinay Patki
*
***************************************************************************/
void fbe_cli_luninfo_print_all_luns(void)
{

    fbe_status_t              status;
    fbe_u32_t                 i;
    fbe_u32_t                 total_luns_in_system;
    fbe_object_id_t           *object_ids;
    fbe_u32_t                  total_system_objects = 0;
    fbe_object_id_t            *system_object_list_p = NULL;
    fbe_u32_t                  max_system_objects = 512 / sizeof(fbe_object_id_t);

    /* get the number of LUNs in the system */
    status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &object_ids, &total_luns_in_system);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("Failed to enumerate classes. Status: %x\n", status);
        return;
    }

    /* Now enumerate all the system objects.
    */
    system_object_list_p = (fbe_object_id_t *)fbe_api_allocate_memory(sizeof(fbe_object_id_t) * max_system_objects);
    if (system_object_list_p == NULL) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
            "%s: cannot allocate memory for enumerating system id object. Status: %x\n", 
            __FUNCTION__, status);
        return;
    }
    fbe_set_memory(system_object_list_p, (fbe_u8_t)FBE_OBJECT_ID_INVALID, sizeof(fbe_object_id_t) * max_system_objects);


    status = fbe_api_database_get_system_objects(FBE_CLASS_ID_INVALID, 
                                                 system_object_list_p, 
                                                 max_system_objects, 
                                                 &total_system_objects);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
            "%s: cannot allocate memory for enumerating system id object. Status: %x\n", 
            __FUNCTION__, status);
        fbe_api_free_memory(object_ids);
        fbe_api_free_memory(system_object_list_p);
        return;
    }
    for (i = 0; i < total_luns_in_system; i++)
    {
        /* check to make sure that LUN is not a system LUN */
        if (fbe_cli_is_system_lun(object_ids[i], system_object_list_p,
            total_system_objects) == FBE_TRUE)
        {
            continue;
        }

        /*Print details of a current lun*/
        fbe_cli_luninfo_print_lun_details(object_ids[i]);
        /* There should not be any error in displaying the LUN status 
         * We can ignore the error for now
         */
    }
    fbe_api_free_memory(object_ids);
    fbe_api_free_memory(system_object_list_p);
}
/******************************************
 * end fbe_cli_luninfo_print_all_luns()
 ******************************************/

/**************************************************************************
*                      fbe_cli_luninfo_print_lun_details ()                 *
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
*    04/05/2011 - Created. Vinay Patki
*
***************************************************************************/
void fbe_cli_luninfo_print_lun_details(fbe_object_id_t lun_id)
{
    fbe_status_t                    status;
    fbe_cli_lurg_lun_details_t      lun_details;
    fbe_cli_lurg_rg_details_t       rg_details;
    fbe_u32_t                       i;
    fbe_status_t                    lifecycle_status = FBE_STATUS_INVALID;
    fbe_lifecycle_state_t           lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_u32_t                       wwn_index = 0;
    fbe_u32_t                       wwn_size = 0;
    fbe_object_id_t                 bvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_volume_attributes_flags     bvd_attribute;
    /* We want downstream as well as upstream objects for this RG indicate 
    * that by setting corresponding variable in structure
    */
    rg_details.direction_to_fetch_object = DIRECTION_BOTH;

    /* Get the details of RG associated with this LUN */
    status = fbe_cli_get_rg_details(lun_id, &rg_details);
    /* return failure from here since we're not able to know objects related to this */
    if(status == FBE_STATUS_GENERIC_FAILURE)
    {
        fbe_cli_error("Failed to get details of Raid group for Lun id %x, Status:0x%x",
                      lun_id,status);
        return;
    }
    else if(status == FBE_STATUS_BUSY)
    {
        lifecycle_status = fbe_api_get_object_lifecycle_state(lun_id, 
                                                    &lifecycle_state, FBE_PACKAGE_ID_SEP_0);
        if(lifecycle_status != FBE_STATUS_OK)
        {
            fbe_cli_error("Fail to get the Lifecycle State for object id:0x%x, status:0x%x\n",
                           lun_id,lifecycle_status);
        }
        else if (lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE)
        {
            fbe_cli_printf("LUN is in SPECIALIZE, Object ID 0x%x\n", lun_id);
            return;
        }
    }

    /*Get lun details*/
    status = fbe_cli_get_lun_details(lun_id,&lun_details);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("Failed to get details of Lun id %x, Status:0x%x",lun_id,status);
        return;
    }  

    /* Get BVD object id */
    status = fbe_api_bvd_interface_get_bvd_object_id(&bvd_object_id);
    if (status != FBE_STATUS_OK ){
        fbe_cli_error ("Failed to get BVD ID, status : 0x%X \n", status);
        return;
    }

    /* Get attribute from BVD object */
    status = fbe_api_bvd_interface_get_downstream_attr(bvd_object_id, &bvd_attribute, lun_id);
    if (status != FBE_STATUS_OK ){
        fbe_cli_error ("Failed to get attribute from BVD object!, status : 0x%X \n", status);
    }
    
    fbe_cli_printf("Lun information:\n");
    fbe_cli_printf("----------------\n");
    fbe_cli_printf("Logical Unit:    %d\n", lun_details.lun_number);
    fbe_cli_printf("  Lun Name:        %s\n", lun_details.lun_info.user_defined_name.name);
    fbe_cli_printf("  Raid type:       0x%X [%s]\n", lun_details.lun_info.raid_info.raid_type,lun_details.p_rg_str);
    fbe_cli_printf("  Lifecycle State: %d [%s]\n", lun_details.lifecycle_state, lun_details.p_lifecycle_state_str);
    fbe_cli_printf("  Lun Object-id:   0x%x\n", lun_id);
    fbe_cli_printf("  Offset:          0x%llx\n",(unsigned long long)lun_details.lun_info.offset);
    fbe_cli_printf("  Capacity:        0x%llx\n",(unsigned long long)lun_details.lun_info.capacity);

    wwn_size = sizeof(lun_details.lun_info.world_wide_name.bytes);
    fbe_cli_printf("  world_wide_name: ");
    for(wwn_index = 0; wwn_index < wwn_size; ++wwn_index)
    {
        fbe_cli_printf("%02x",(fbe_u32_t)(lun_details.lun_info.world_wide_name.bytes[wwn_index]));
        fbe_cli_printf(":");    
    }
    fbe_cli_printf("\n");

    fbe_cli_printf("  Bind Time :      0x%llx\n",(unsigned long long)lun_details.lun_info.bind_time);
    fbe_cli_printf("  User Private :   %s\n",(lun_details.lun_info.user_private==FBE_TRUE)?"TRUE":"FALSE");
    fbe_cli_printf("  RG:              %d\n",rg_details.rg_number);
    fbe_cli_printf("  Attributes:      0x%X\n",lun_details.lun_info.attributes);
    fbe_cli_printf("  Rotational_rate: %d\n",lun_details.lun_info.rotational_rate);
    fbe_cli_printf("  Rebuild status:  percentage_complete %d\n",
                   lun_details.lun_info.rebuild_status.rebuild_percentage);
    fbe_cli_printf("                   checkpoint 0x%llx\n",
                   (unsigned long long)lun_details.lun_info.rebuild_status.rebuild_checkpoint);
    fbe_cli_printf("  BVD Attributes : 0x%X\n", bvd_attribute);
    fbe_cli_printf("\n");
    fbe_cli_printf("  FRUs (%d):\n",rg_details.downstream_pvd_object_list.number_of_downstream_objects);

    /*Get drive objects related to a RG*/
    for (i = 0; i < rg_details.downstream_pvd_object_list.number_of_downstream_objects; i++)
    {
        /*Print the info of fru*/
        fbe_cli_printf("    FRU - %d_%d_%d [Obj id - 0x%X]",rg_details.drive_list[i].port_num,rg_details.drive_list[i].encl_num,rg_details.drive_list[i].slot_num,
        rg_details.downstream_pvd_object_list.downstream_object_list[i]);
        
        status = fbe_api_get_object_lifecycle_state(rg_details.downstream_pvd_object_list.downstream_object_list[i], &lifecycle_state, FBE_PACKAGE_ID_SEP_0);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s Get lifecycle failed: index: %d obj id: 0x%x status: 0x%x \n",
                           __FUNCTION__, i, rg_details.downstream_pvd_object_list.downstream_object_list[i], status);
            return;
        }
        if(lifecycle_state == FBE_LIFECYCLE_STATE_FAIL)
        {
            fbe_cli_printf( "%s","(DEAD)");
        }
        fbe_cli_printf("\n");
    }

    fbe_cli_printf("\n");
    fbe_cli_printf("  Lun Zeroing:\n");
    fbe_cli_printf("    Percentage Zeroed: %d%%\n",lun_details.fbe_api_lun_get_zero_status.zero_percentage);
    fbe_cli_printf("    Checkpoint: 0x%llx\n",(unsigned long long)lun_details.fbe_api_lun_get_zero_status.zero_checkpoint);

    fbe_cli_display_metadata_element_state(lun_id);

    fbe_cli_printf("\n");
}
/******************************************
 * end fbe_cli_luninfo_print_lun_details()
 ******************************************/

/*!*******************************************************************
 * @var fbe_cli_lun_get_cache_zero_bitmap_block_count()
 *********************************************************************
 * @brief Function to get user lun capacity from the total lun capacity.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author
 *  06/03/2013 - Created. Sandeep Chaudhari
 *********************************************************************/
void fbe_cli_lun_get_cache_zero_bitmap_block_count(fbe_s32_t argc, fbe_s8_t ** argv)
{
    fbe_status_t                                        status = FBE_STATUS_INVALID;
    fbe_lba_t                                           lun_capacity = FBE_LBA_INVALID;
    fbe_lba_t                                           user_lun_capacity = FBE_LBA_INVALID;
    fbe_api_lun_calculate_cache_zero_bitmap_blocks_t    cache_zero_bitmap_blocks_info = {0};
        
    if (argc < 1)
    {
        fbe_cli_error("%s Not enough arguments !\n", __FUNCTION__);
        fbe_cli_printf("%s", CACHE_ZERO_BITMAP_COUNT_USAGE);
        return;
    }
    while (argc > 0)
    {
        if (strcmp(*argv, "-cap") == 0)
        {
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("LUN capacity argument is expected\n");
                return;
            }
            lun_capacity = fbe_cli_convert_str_to_lba(*argv);

            if((0 == lun_capacity) || (FBE_LBA_INVALID == lun_capacity))
            {
                fbe_cli_error("Invalid input capacity!\n ");
                return;
            }
            
            argc--;
            argv++;
        }
        else
        {
            /* The command line parameters should be properly entered */
            fbe_cli_printf("%s", CACHE_ZERO_BITMAP_COUNT_USAGE);
            return;
        }
    }

    cache_zero_bitmap_blocks_info.lun_capacity_in_blocks = lun_capacity;
    cache_zero_bitmap_blocks_info.cache_zero_bitmap_block_count = 0;
    
    status = fbe_api_lun_calculate_cache_zero_bit_map_size_to_remove(&cache_zero_bitmap_blocks_info);

    if(FBE_STATUS_OK != status)
    {
        fbe_cli_printf("Failed to get the cache zero bimap count ! \n");
        return;
    }

    /* Calculate the user lun capacity by subtracting cache zero bitmap from lun capacity */
    user_lun_capacity = cache_zero_bitmap_blocks_info.lun_capacity_in_blocks - cache_zero_bitmap_blocks_info.cache_zero_bitmap_block_count;

    /* Print the information */
    fbe_cli_printf("Lun Capacity                  : 0x%X \n", (unsigned int)cache_zero_bitmap_blocks_info.lun_capacity_in_blocks);
    fbe_cli_printf("Cache zero bitmap block count : 0x%X \n", (unsigned int)cache_zero_bitmap_blocks_info.cache_zero_bitmap_block_count);
    fbe_cli_printf("User Lun Capacity (In Hex)    : 0x%X \n", (unsigned int)user_lun_capacity);
    fbe_cli_printf("User Lun Capacity (In Dec)    : %u \n", (unsigned int)user_lun_capacity);
}
/******************************************
 * end fbe_cli_lun_get_cache_zero_bitmap_block_count()
 ******************************************/

/**************************************************************************
*                      fbe_cli_luninfo_unexport_all_luns ()                 *
***************************************************************************
*
*  @brief
*    This unexport all the blockshim device on system, except system luns.
*
*
*  @param
*    None
*
*  @return
*    None
*
*  @version
*    11/05/2014 - Created. Jibing Dong 
*
***************************************************************************/
void fbe_cli_luninfo_unexport_all_luns(void)
{

    fbe_status_t              status;
    fbe_u32_t                 i;
    fbe_u32_t                 total_luns_in_system;
    fbe_object_id_t           *object_ids;
    fbe_u32_t                  total_system_objects = 0;
    fbe_object_id_t            *system_object_list_p = NULL;
    fbe_u32_t                  max_system_objects = 512 / sizeof(fbe_object_id_t);

    /* get the number of LUNs in the system */
    status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &object_ids, &total_luns_in_system);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("Failed to enumerate classes. Status: %x\n", status);
        return;
    }

    /* Now enumerate all the system objects.
    */
    system_object_list_p = (fbe_object_id_t *)fbe_api_allocate_memory(sizeof(fbe_object_id_t) * max_system_objects);
    if (system_object_list_p == NULL) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
            "%s: cannot allocate memory for enumerating system id object. Status: %x\n", 
            __FUNCTION__, status);
        return;
    }
    fbe_set_memory(system_object_list_p, (fbe_u8_t)FBE_OBJECT_ID_INVALID, sizeof(fbe_object_id_t) * max_system_objects);


    status = fbe_api_database_get_system_objects(FBE_CLASS_ID_INVALID, 
                                                 system_object_list_p, 
                                                 max_system_objects, 
                                                 &total_system_objects);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
            "%s: cannot allocate memory for enumerating system id object. Status: %x\n", 
            __FUNCTION__, status);
        fbe_api_free_memory(object_ids);
        fbe_api_free_memory(system_object_list_p);
        return;
    }
    for (i = 0; i < total_luns_in_system; i++)
    {
        /* check to make sure that LUN is not a system LUN */
        if (fbe_cli_is_system_lun(object_ids[i], system_object_list_p,
            total_system_objects) == FBE_TRUE)
        {
            continue;
        }

        /*Uexport the current lun*/
        status = fbe_api_bvd_interface_unexport_lun(object_ids[i]);
        /* There should not be any error in unexporting blockshim device 
         * We can ignore the error for now
         */
        if (status != FBE_STATUS_OK && status != FBE_STATUS_NO_DEVICE)
        {
            fbe_cli_error("Failed to unexport LUN (obj 0x%x). Status: %x\n", object_ids[i], status);
            return;
        }
    }
    fbe_api_free_memory(object_ids);
    fbe_api_free_memory(system_object_list_p);

    fbe_cli_printf("LUN unexported successfully.\n");
}
/******************************************
 * end fbe_cli_luninfo_unexport_all_luns()
 ******************************************/

/*************************
 * end file fbe_cli_lib_luninfo_cmds.c
 *************************/
