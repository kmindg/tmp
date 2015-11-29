/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_test_sep_raid_group.c
 ***************************************************************************
 *
 * @brief
 *  This file contains raid group testing related functions.
 *
 * @version
 *   9/5/2012:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "sep_utils.h"
#include "mut.h"
/*************************
 *   FUNCTION DEFINITIONS
 *************************/


/*!**************************************************************
 * fbe_test_raid_group_get_info()
 ****************************************************************
 * @brief
 *  Validate the parameters of rg get info for active raid groups.
 *
 * @param rg_config_p - Current active config.               
 *
 * @return None.  
 *
 * @author
 *  9/5/2012 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_raid_group_get_info(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index = 0;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);
    
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index];

        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Make sure we have the correct sectors per stripe.
         */
        if (rg_info.sectors_per_stripe != (rg_info.num_data_disk * rg_info.element_size))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "rg_info.sectors_per_stripe 0x%x != (rg_info.num_data_disk %d * rg_info.element_size %d)",
                       (unsigned int)rg_info.sectors_per_stripe, rg_info.num_data_disk, rg_info.element_size);
            MUT_FAIL_MSG("rg_info sectors per stripe unexpected");
        }

         /* Make sure we have the correct stripe count.
         */
        if (rg_info.stripe_count != (rg_info.capacity / rg_info.sectors_per_stripe))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "rg_info.stripe_count 0x%llx != (rg_info.capacity %llu / rg_info.sectors_per_stripe %llu)",
                       rg_info.stripe_count, rg_info.capacity, rg_info.sectors_per_stripe);
            MUT_FAIL_MSG("rg_info stripe count unexpected");
        }
        /* In the future we can add more cases here as needed to validate other parts of the rg info.
         */
        current_rg_config_p++;
    }
}
/******************************************
 * end fbe_test_raid_group_get_info()
 ******************************************/

/*!**************************************************************************
 *  fbe_test_sep_raid_group_find_mirror_obj_and_adj_pos_for_r10
 ***************************************************************************
 * @brief
 *   This function returns the object ID for the mirror object in the given
 *   R10 RG that corresponds to the given disk position.  Both the object
 *   ID of the mirror object and the position within that mirror object
 *   are returned.
 *
 * @param in_rg_config_p                    - pointer to the RG configuration info
 * @param in_position                       - disk position in the RG
 * @param out_mirror_raid_group_object_id_p - pointer to object ID of R10 mirror object
 * @param out_position_p                    - pointer to disk position in the R10 mirror RG
 *
 * @return void
 *
 ***************************************************************************/
void fbe_test_sep_raid_group_find_mirror_obj_and_adj_pos_for_r10(
                                                    fbe_test_rg_configuration_t*    in_rg_config_p,
                                                    fbe_u32_t                       in_position,
                                                    fbe_object_id_t*                out_mirror_raid_group_object_id_p,
                                                    fbe_u32_t*                      out_position_p)
{
    fbe_status_t                                    status;                     //  fbe status
    fbe_object_id_t                                 raid_group_object_id;       //  raid group's object id
    fbe_api_base_config_downstream_object_list_t    downstream_object_list;     //  object list for R10 RG
    fbe_u32_t                                       index;                      //  index into object list for R10


    //  Confirm the RG type is R10
    MUT_ASSERT_INT_EQUAL(FBE_RAID_GROUP_TYPE_RAID10, in_rg_config_p->raid_type);

    //  Get the object ID of the R10 RG; this is the striper RG object ID
    status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, raid_group_object_id);

    //  Get the downstream object list; this is the list of mirror objects for this R10 RG
    fbe_test_sep_util_get_ds_object_list(raid_group_object_id, &downstream_object_list);

    //  Get the index into the downstream object list for the mirror object corresponding
    //  to the incoming position
    index = in_position / 2;

    //  Get the object ID of the corresponding mirror object
    *out_mirror_raid_group_object_id_p = downstream_object_list.downstream_object_list[index];
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, *out_mirror_raid_group_object_id_p);

    //  Get the position in mirror object
    *out_position_p = in_position % 2;

    //  Return
    return;

}   // End fbe_test_sep_raid_group_find_mirror_obj_and_adj_pos_for_r10()

/*!***************************************************************************
 *          fbe_test_sep_raid_group_class_set_emeh_values()
 ***************************************************************************** 
 * 
 * @brief   This function changes the Extended Media Error Handling (EMEH)
 *          values as described for the raid group class (i.e for all raid
 *          groups). On BOTH SPs.
 *
 * @param   set_mode - The new mode (i.e. disable emeh, increase threshold etc.)
 * @param   b_change_threshold - FBE_TRUE - Use percent_threshold_increase to increase
 *              the media error `shoot drive' threshold by the percent indicated
 * @param   percent_threshold_increase - Percent (i.e. 100 etc) to increase the media
 *              error threshold from shooting drive.
 * @param   b_persist - FBE_TRUE - Persist the changes
 *
 * @return  status
 * 
 * @author
 *  03/11/2015  Ron Proulx  - Created.
 * 
 *****************************************************************************/
fbe_status_t fbe_test_sep_raid_group_class_set_emeh_values(fbe_u32_t set_mode,
                                                           fbe_bool_t b_change_threshold,
                                                           fbe_u32_t percent_threshold_increase,
                                                           fbe_bool_t b_persist)
{
    fbe_status_t                        status;
    fbe_bool_t                          b_is_dualsp_mode;
    fbe_bool_t                          b_send_to_both = FBE_FALSE;
    fbe_transport_connection_target_t   this_sp;
    fbe_transport_connection_target_t   other_sp;
    fbe_cmi_service_get_info_t          cmi_info;
    fbe_raid_group_class_get_extended_media_error_handling_t get_emeh;

    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();
    status = fbe_api_cmi_service_get_info(&cmi_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if ((b_is_dualsp_mode == FBE_TRUE)    &&
        (cmi_info.peer_alive == FBE_TRUE)    ) 
    {
        b_send_to_both = FBE_TRUE;
    }

    /* Get the current thresholds */
    status = fbe_api_raid_group_class_get_extended_media_error_handling(&get_emeh);
    if (status != FBE_STATUS_OK) {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "set class emeh: failed to get current emeh values - status: %d",
                   status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Send to the active sp.
     */
    /* Trace some information.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "set class emeh: SP: %d persist: %d change mode from: %d to: %d change threshold: %d current threshold: %d",
               this_sp, b_persist, get_emeh.current_mode, set_mode, b_change_threshold, get_emeh.current_threshold_increase);
    if (b_change_threshold) {
        mut_printf(MUT_LOG_TEST_STATUS, 
               "set class emeh: change threshold from: %d to: %d",
               get_emeh.current_threshold_increase, percent_threshold_increase);
    }

    /* Send the request to the raid group class. */
    status = fbe_api_raid_group_class_set_extended_media_error_handling(set_mode,
                                                                        b_change_threshold,
                                                                        percent_threshold_increase,
                                                                        b_persist);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* If we are in dualsp just validate it on the peer
     */
    if (b_send_to_both)
    {
        /* Should have automatically updated the timer on peer
         */
        fbe_api_sim_transport_set_target_server(other_sp);
        status = fbe_api_raid_group_class_get_extended_media_error_handling(&get_emeh);
        if (status != FBE_STATUS_OK) {
            mut_printf(MUT_LOG_TEST_STATUS, 
                   "set class emeh: failed to get current emeh values - status: %d",
                   status);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
        }
        /* Trace some information.
        */
        mut_printf(MUT_LOG_TEST_STATUS, 
               "set class emeh: SP: %d persist: %d change mode from: %d to: %d change threshold: %d current threshold: %d",
               other_sp, b_persist, get_emeh.current_mode, set_mode, b_change_threshold, get_emeh.current_threshold_increase);
        if (b_change_threshold) {
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "set class emeh: change threshold from: %d to: %d",
                       get_emeh.current_threshold_increase, percent_threshold_increase);
        }

        /* Send the request to the raid group class. */
        status = fbe_api_raid_group_class_set_extended_media_error_handling(set_mode,
                                                                            b_change_threshold,
                                                                            percent_threshold_increase,
                                                                            b_persist);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        fbe_api_sim_transport_set_target_server(this_sp);
    }
    return status;
}
/********************************************************* 
 * end fbe_test_sep_raid_group_class_set_emeh_values()
 *********************************************************/

/*!***************************************************************************
 *          fbe_test_sep_raid_group_class_get_emeh_values()
 ***************************************************************************** 
 * 
 * @brief   This function gets the Extended Media Error Handling (EMEH)
 *          values as described for the raid group class (i.e for all raid
 *          groups).
 *
 * @param   get_class_emeh_p - Pointer to class values to populate
 *
 * @return  status
 * 
 * @author
 *  03/11/2015  Ron Proulx  - Created.
 * 
 *****************************************************************************/
fbe_status_t fbe_test_sep_raid_group_class_get_emeh_values(fbe_raid_group_class_get_extended_media_error_handling_t *get_class_emeh_p)
{
    fbe_status_t                        status;
    fbe_bool_t                          b_is_dualsp_mode;
    fbe_bool_t                          b_send_to_both = FBE_FALSE;
    fbe_transport_connection_target_t   this_sp;
    fbe_transport_connection_target_t   other_sp;
    fbe_cmi_service_get_info_t          cmi_info;

    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();
    status = fbe_api_cmi_service_get_info(&cmi_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if ((b_is_dualsp_mode == FBE_TRUE)    &&
        (cmi_info.peer_alive == FBE_TRUE)    ) 
    {
        b_send_to_both = FBE_TRUE;
    }

    /* Send to the active sp.
     */
    status = fbe_api_raid_group_class_get_extended_media_error_handling(get_class_emeh_p);
    if (status != FBE_STATUS_OK) {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "get class emeh: failed to get current emeh values - status: %d",
                   status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Trace some information.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "get class emeh: SP: %d default mode: %d current mode: %d default inc threshold: %d current inc threshold: %d",
               this_sp, get_class_emeh_p->default_mode, get_class_emeh_p->current_mode, 
               get_class_emeh_p->default_threshold_increase, get_class_emeh_p->current_threshold_increase);

    /* If we are in dualsp just validate it on the peer
     */
    if (b_send_to_both)
    {
        /* Read form each SP
         */
        fbe_api_sim_transport_set_target_server(other_sp);
        status = fbe_api_raid_group_class_get_extended_media_error_handling(get_class_emeh_p);
        if (status != FBE_STATUS_OK) {
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "get class emeh: failed to get current emeh values - status: %d",
                       status);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
        }

        /* Trace some information.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "get class emeh: SP: %d default mode: %d current control: %d default inc threshold: %d current inc threshold: %d",
                   this_sp, get_class_emeh_p->default_mode, get_class_emeh_p->current_mode, 
                   get_class_emeh_p->default_threshold_increase, get_class_emeh_p->current_threshold_increase);

        fbe_api_sim_transport_set_target_server(this_sp);
    }
    return status;
}
/********************************************************* 
 * end fbe_test_sep_raid_group_class_get_emeh_values()
 *********************************************************/

/*!***************************************************************************
 *          fbe_test_sep_raid_group_set_emeh_values()
 ***************************************************************************** 
 * 
 * @brief   This function changes the Extended Media Error Handling (EMEH)
 *          values as described for the raid group.
 *
 * @param   rg_object_id - The object id of the raid group to set the values for
 * @param   set_control - The new control (i.e. disable emeh, increase threshold etc.)
 * @param   b_change_threshold - FBE_TRUE - Use percent_threshold_increase to increase
 *              the media error `shoot drive' threshold by the percent indicated
 * @param   percent_threshold_increase - Percent (i.e. 100 etc) to increase the media
 *              error threshold from shooting drive.
 * @param   b_persist - FBE_TRUE - Persist the changes
 *
 * @return  status
 * 
 * @author
 *  05/13/2015  Ron Proulx  - Created.
 * 
 *****************************************************************************/
fbe_status_t fbe_test_sep_raid_group_set_emeh_values(fbe_object_id_t rg_object_id,
                                                     fbe_u32_t set_control,
                                                     fbe_bool_t b_change_threshold,
                                                     fbe_u32_t percent_threshold_increase,
                                                     fbe_bool_t b_persist)
{
    fbe_status_t                        status;
    fbe_bool_t                          b_is_dualsp_mode;
    fbe_bool_t                          b_send_to_both = FBE_FALSE;
    fbe_transport_connection_target_t   this_sp;
    fbe_transport_connection_target_t   other_sp;
    fbe_cmi_service_get_info_t          cmi_info;
    fbe_raid_group_get_extended_media_error_handling_t get_emeh;

    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();
    status = fbe_api_cmi_service_get_info(&cmi_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if ((b_is_dualsp_mode == FBE_TRUE)    &&
        (cmi_info.peer_alive == FBE_TRUE)    ) 
    {
        b_send_to_both = FBE_TRUE;
    }

    /* Get the current thresholds */
    status = fbe_api_raid_group_get_extended_media_error_handling(rg_object_id, &get_emeh);
    if (status != FBE_STATUS_OK) {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "set emeh: rg obj: 0x%x failed to get current emeh values - status: %d",
                   rg_object_id, status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Send to the active sp.
     */
    /* Trace some information.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "set emeh: SP: %d rg obj: 0x%x change mode from: %d to: %d change threshold: %d",
               this_sp, rg_object_id, get_emeh.current_mode, set_control, b_change_threshold);
    if (b_change_threshold) {
        mut_printf(MUT_LOG_TEST_STATUS, 
               "set emeh: change threshold from: %d to: %d",
               get_emeh.default_threshold_increase, percent_threshold_increase);
    }

    /* Send the request to the raid group class. */
    status = fbe_api_raid_group_set_extended_media_error_handling(set_control,
                                                                  b_change_threshold,
                                                                  percent_threshold_increase,
                                                                  b_persist);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* If we are in dualsp just validate it on the peer
     */
    if (b_send_to_both)
    {
        /* Should have automatically updated the timer on peer
         */
        fbe_api_sim_transport_set_target_server(other_sp);
        status = fbe_api_raid_group_get_extended_media_error_handling(rg_object_id, &get_emeh);
        if (status != FBE_STATUS_OK) {
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "set emeh: rg obj: 0x%x failed to get current emeh values - status: %d",
                       rg_object_id, status);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
        }
        /* Trace some information.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
               "set emeh: SP: %d rg obj: 0x%x change mode from: %d to: %d change threshold: %d",
               other_sp, rg_object_id, get_emeh.current_mode, set_control, b_change_threshold);
        if (b_change_threshold) {
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "set class emeh: change threshold from: %d to: %d",
                       get_emeh.default_threshold_increase, percent_threshold_increase);
        }

        /* Send the request to the raid group class. */
        status = fbe_api_raid_group_set_extended_media_error_handling(set_control,
                                                                      b_change_threshold,
                                                                      percent_threshold_increase,
                                                                      b_persist);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        fbe_api_sim_transport_set_target_server(this_sp);
    }
    return status;
}
/********************************************************* 
 * end fbe_test_sep_raid_group_set_emeh_values()
 *********************************************************/

/*!***************************************************************************
 *          fbe_test_sep_raid_group_display_emeh_values()
 ***************************************************************************** 
 * 
 * @brief   This function gets the Extended Media Error Handling (EMEH)
 *          values as described for the raid group specified.
 *
 * @param   this_sp - The SP that emeh occurred on
 * @param   rg_object_id - The object id of the raid group to fetch the values for
 * @param   get_emeh_p - Pointer to EMEH values for this raid group
 *
 * @return  status
 * 
 * @author
 *  05/13/2015  Ron Proulx  - Created.
 * 
 *****************************************************************************/
static fbe_status_t fbe_test_sep_raid_group_display_emeh_values(fbe_transport_connection_target_t this_sp,
                                                                fbe_object_id_t rg_object_id,
                                                                fbe_raid_group_get_extended_media_error_handling_t *get_emeh_p)
{
    fbe_u32_t   position;

    mut_printf(MUT_LOG_TEST_STATUS, 
               "get emeh: SP: %d rg obj: 0x%x enabled mode: %d current mode: %d reliability: %d",
               this_sp, rg_object_id, get_emeh_p->enabled_mode, get_emeh_p->current_mode, get_emeh_p->reliability);
    mut_printf(MUT_LOG_TEST_STATUS, 
               "                              paco pos: %d active command: %d default inc: %d",
               get_emeh_p->paco_position, get_emeh_p->active_command, get_emeh_p->default_threshold_increase);
    mut_printf(MUT_LOG_TEST_STATUS,
               "emeh:  pos   dieh reliability     dieh mode      media weight adjust  paco position");
    for (position = 0; position < get_emeh_p->width; position++)
    {
        mut_printf(MUT_LOG_TEST_STATUS,
               "      [%2d]        %2d                %2d                %2d              %2d", 
                   position, get_emeh_p->dieh_reliability[position], 
                   get_emeh_p->dieh_mode[position], 
                   get_emeh_p->media_weight_adjust[position],
                   get_emeh_p->vd_paco_position[position]);
    }

    return FBE_STATUS_OK;
}
/********************************************************* 
 * end fbe_test_sep_raid_group_display_emeh_values()
 *********************************************************/

/*!***************************************************************************
 *          fbe_test_sep_raid_group_get_emeh_values()
 ***************************************************************************** 
 * 
 * @brief   This function gets the Extended Media Error Handling (EMEH)
 *          values as described for the raid group specified.
 *
 * @param   rg_object_id - The object id of the raid group to fetch the values
 *              for.
 * @param   get_emeh_p - Pointer to structure to update
 * @param   b_display_values - FBE_TRUE - Display values for each SP
 * @param   b_send_to_both_sps - FBE_TRUE - Send request to both SPs
 *
 * @return  status
 * 
 * @author
 *  05/13/2015  Ron Proulx  - Created.
 * 
 *****************************************************************************/
fbe_status_t fbe_test_sep_raid_group_get_emeh_values(fbe_object_id_t rg_object_id,
                                                     fbe_raid_group_get_extended_media_error_handling_t *get_emeh_p,
                                                     fbe_bool_t b_display_values,
                                                     fbe_bool_t b_send_to_both_sps)
{
    fbe_status_t                        status;
    fbe_bool_t                          b_is_dualsp_mode;
    fbe_bool_t                          b_send_to_both = FBE_FALSE;
    fbe_transport_connection_target_t   this_sp;
    fbe_transport_connection_target_t   other_sp;
    fbe_cmi_service_get_info_t          cmi_info;

    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();
    status = fbe_api_cmi_service_get_info(&cmi_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if ((b_is_dualsp_mode == FBE_TRUE)    &&
        (cmi_info.peer_alive == FBE_TRUE) &&
        (b_send_to_both_sps == FBE_TRUE)     )
    {
        b_send_to_both = FBE_TRUE;
    }

    /* Send to the active sp
     */
    status = fbe_api_raid_group_get_extended_media_error_handling(rg_object_id, get_emeh_p);
    if (status != FBE_STATUS_OK) {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "get emeh: failed to get current emeh values - status: %d",
                   status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* If requested display the current values on this SP
     */
    if (b_display_values)
    {
        fbe_test_sep_raid_group_display_emeh_values(this_sp, rg_object_id, get_emeh_p);
    }

    /* If we are in dualsp just validate it on the peer
     */
    if (b_send_to_both)
    {
        /* Read form each SP
         */
        fbe_api_sim_transport_set_target_server(other_sp);
        status = fbe_api_raid_group_get_extended_media_error_handling(rg_object_id, get_emeh_p);
        if (status != FBE_STATUS_OK) {
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "get emeh: failed to get current emeh values - status: %d",
                       status);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
        }

        /* If requested display the current values on this SP
         */
        if (b_display_values)
        {
            fbe_test_sep_raid_group_display_emeh_values(other_sp, rg_object_id, get_emeh_p);
        }
        fbe_api_sim_transport_set_target_server(this_sp);
    }
    return status;
}
/********************************************************* 
 * end fbe_test_sep_raid_group_get_emeh_values()
 *********************************************************/

/*!***************************************************************************
 *          fbe_test_sep_raid_group_validate_emeh_values_using_class_data()
 *****************************************************************************
 *
 * @brief   Validate that the current EMEH (Extended Media Error Handling) are
 *          as expected.
 *
 * @param   get_class_emeh_p - Pointer to class data
 * @param   get_emeh_p - Pointer to raid group data
 * @param   rg_object_id - Object id of raid group being validated
 * @param   b_expect_default - FBE_TRUE - Expect the default values
 * @param   b_expect_disabled - FBE_TRUE - Expect the thresholds to be disabled
 * @param   b_expect_increase - FBE_TRUE - Expect the media error thresholds
 *              to be increased.
 *
 * @return  status
 *
 * @author
 *  05/13/2015  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void fbe_test_sep_raid_group_validate_emeh_values_using_class_data(fbe_raid_group_class_get_extended_media_error_handling_t *get_class_emeh_p,
                                                          fbe_raid_group_get_extended_media_error_handling_t *get_emeh_p,
                                                          fbe_object_id_t rg_object_id,
                                                          fbe_bool_t b_expect_default,
                                                          fbe_bool_t b_expect_disabled,
                                                          fbe_bool_t b_expect_increase)
{
    fbe_u32_t   default_media_adjust = 100;
    fbe_u32_t   disabled_media_adjust = 100;
    fbe_u32_t   increase_media_adjust = 50;
    fbe_u32_t   position;

    /* Now validate the values.
     */
    MUT_ASSERT_INT_EQUAL(get_class_emeh_p->current_mode, get_emeh_p->enabled_mode);
    MUT_ASSERT_INT_EQUAL(get_class_emeh_p->default_threshold_increase, get_emeh_p->default_threshold_increase);
    if (b_expect_default)
    {
        MUT_ASSERT_INT_EQUAL(get_class_emeh_p->current_mode, get_emeh_p->current_mode);
        MUT_ASSERT_INT_EQUAL(FBE_RAID_EMEH_COMMAND_INVALID, get_emeh_p->active_command);
        MUT_ASSERT_TRUE((FBE_RAID_EMEH_RELIABILITY_UNKNOWN == get_emeh_p->reliability)   ||
                        (FBE_RAID_EMEH_RELIABILITY_LOW == get_emeh_p->reliability)    );
        for (position = 0; position < get_emeh_p->width; position++)
        {
            /*! @note A value of FBE_U32_MAX means that this position is 
             *        degraded or missing.  Do not compare to expected.
             */
            if (get_emeh_p->dieh_reliability[position] != FBE_U32_MAX)
            {
                MUT_ASSERT_INT_EQUAL(FBE_DRIVE_MEDIA_RELIABILITY_VERY_LOW, get_emeh_p->dieh_reliability[position]);
                MUT_ASSERT_INT_EQUAL(FBE_DRIVE_MEDIA_MODE_THRESHOLDS_DEFAULT, get_emeh_p->dieh_mode[position]);
                MUT_ASSERT_INT_EQUAL(default_media_adjust, get_emeh_p->media_weight_adjust[position]);
                MUT_ASSERT_INT_EQUAL(FBE_EDGE_INDEX_INVALID, get_emeh_p->vd_paco_position[position]);
            }
        }
    }
    else if (b_expect_disabled)
    {
        if (get_emeh_p->paco_position != FBE_EDGE_INDEX_INVALID) 
        {
            MUT_ASSERT_INT_EQUAL(FBE_RAID_EMEH_MODE_PACO_HA, get_emeh_p->current_mode);
        }
        else
        {
            MUT_ASSERT_INT_EQUAL(FBE_RAID_EMEH_MODE_DEGRADED_HA, get_emeh_p->current_mode);
        }
        MUT_ASSERT_INT_EQUAL(FBE_RAID_EMEH_COMMAND_INVALID, get_emeh_p->active_command);
        MUT_ASSERT_INT_EQUAL(FBE_RAID_EMEH_RELIABILITY_LOW, get_emeh_p->reliability);
        for (position = 0; position < get_emeh_p->width; position++)
        {
            /*! @note A value of FBE_U32_MAX means that this position is 
             *        degraded or missing.  Do not compare to expected.
             */
            if (get_emeh_p->dieh_reliability[position] != FBE_U32_MAX)
            {
                MUT_ASSERT_INT_EQUAL(FBE_DRIVE_MEDIA_RELIABILITY_VERY_LOW, get_emeh_p->dieh_reliability[position]);

                /* If this is the proactive copy position we expecte NO modifications.
                 * I.E. We expect the defaults.
                 */
                if ((get_emeh_p->paco_position != FBE_EDGE_INDEX_INVALID) &&
                    (position == get_emeh_p->paco_position)                  )
                {
                    /* This is the proactive copy position AND all the drives 
                     * in the raid group are `not reliable'.  Therefore there
                     * should be no changes to the proactive copy position.
                     */
                    MUT_ASSERT_INT_EQUAL(FBE_DRIVE_MEDIA_MODE_THRESHOLDS_DISABLED, get_emeh_p->dieh_mode[position]);
                    MUT_ASSERT_INT_EQUAL(default_media_adjust, get_emeh_p->media_weight_adjust[position]);
                    MUT_ASSERT_INT_EQUAL(position, get_emeh_p->vd_paco_position[position]);
                }
                else
                {
                    /* Else the thresholds should have been disabled.
                     */
                    if (get_emeh_p->dieh_mode[position] != FBE_DRIVE_MEDIA_MODE_THRESHOLDS_DISABLED)
                    {
                        fbe_test_sep_util_raid_group_generate_error_trace(rg_object_id);
                    }
                    MUT_ASSERT_INT_EQUAL(FBE_DRIVE_MEDIA_MODE_THRESHOLDS_DISABLED, get_emeh_p->dieh_mode[position]);
                    MUT_ASSERT_INT_EQUAL(disabled_media_adjust, get_emeh_p->media_weight_adjust[position]);
                    MUT_ASSERT_INT_EQUAL(FBE_EDGE_INDEX_INVALID, get_emeh_p->vd_paco_position[position]);
                }
            }
        }
    }
    else if (b_expect_increase)
    {
        MUT_ASSERT_INT_EQUAL(FBE_RAID_EMEH_MODE_THRESHOLDS_INCREASED, get_emeh_p->current_mode);
        for (position = 0; position < get_emeh_p->width; position++)
        {
            /*! @note A value of FBE_U32_MAX means that this position is 
             *        degraded or missing.  Do not compare to expected.
             */
            if (get_emeh_p->dieh_reliability[position] != FBE_U32_MAX)
            {
                MUT_ASSERT_INT_EQUAL(FBE_DRIVE_MEDIA_RELIABILITY_VERY_HIGH, get_emeh_p->dieh_reliability[position]);

                /* If this is the proactive copy position we expecte NO modifications.
                 * I.E. We expect the defaults.
                 */
                if ((get_emeh_p->paco_position != FBE_EDGE_INDEX_INVALID) &&
                    (position == get_emeh_p->paco_position)                  )
                {
                    /* This is the proactive copy position AND all the drives 
                     * in the raid group are `highly reliable'.  Therefore there
                     * should be no changes to the proactive copy position.
                     */
                    MUT_ASSERT_INT_EQUAL(FBE_DRIVE_MEDIA_MODE_THRESHOLDS_DEFAULT, get_emeh_p->dieh_mode[position]);
                    MUT_ASSERT_INT_EQUAL(default_media_adjust, get_emeh_p->media_weight_adjust[position]);
                    MUT_ASSERT_INT_EQUAL(position, get_emeh_p->vd_paco_position[position]);
                }
                else
                {
                    /* Else the thresholds should have been increased.
                     */
                    if (get_emeh_p->dieh_mode[position] != FBE_DRIVE_MEDIA_MODE_THRESHOLDS_INCREASED)
                    {
                        fbe_test_sep_util_raid_group_generate_error_trace(rg_object_id);
                    }
                    MUT_ASSERT_INT_EQUAL(FBE_DRIVE_MEDIA_MODE_THRESHOLDS_INCREASED, get_emeh_p->dieh_mode[position]);
                    MUT_ASSERT_INT_EQUAL(increase_media_adjust, get_emeh_p->media_weight_adjust[position]);
                    MUT_ASSERT_INT_EQUAL(FBE_EDGE_INDEX_INVALID, get_emeh_p->vd_paco_position[position]);
                }
            }
        }
    }
    else
    {
        MUT_ASSERT_TRUE_MSG((b_expect_default == FBE_TRUE), "Either b_expect_default or b_expect_disabled or b_expect_increase must be True");
    }

    return;
}
/********************************************************************** 
 * end fbe_test_sep_raid_group_validate_emeh_values_using_class_data()
 **********************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_raid_group_validate_emeh_values()
 *****************************************************************************
 *
 * @brief   Validate that the current EMEH (Extended Media Error Handling) are
 *          set for all teh raid groups.
 *
 * @param   rg_config_p - Pointer to raid group configuraiton to wait for
 * @param   raid_group_count - Count of raid groups wait for rebuild
 * @param   position_to_evaluate - Position in raid group that is of interest
 * @param   b_expect_default - FBE_TRUE - Expect the default values
 * @param   b_expect_disabled - FBE_TRUE - Expect the thresholds to be disabled
 * @param   b_expect_increase - FBE_TRUE - Expect the media error thresholds
 *              to be increased.
 *
 * @return  status
 *
 * @author
 *  05/13/2015  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_raid_group_validate_emeh_values(fbe_test_rg_configuration_t * const rg_config_p,
                                                          fbe_u32_t raid_group_count,
                                                          fbe_u32_t position_to_evaluate,
                                                          fbe_bool_t b_expect_default,
                                                          fbe_bool_t b_expect_disabled,
                                                          fbe_bool_t b_expect_increase)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t            *current_rg_config_p = rg_config_p;
    fbe_u32_t                               rg_index;
    fbe_object_id_t                         rg_object_id;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_raid_group_class_get_extended_media_error_handling_t get_class_emeh;
    fbe_raid_group_get_extended_media_error_handling_t get_emeh;

    /* Get the current target server
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    /* Get the class values.
     */
    status = fbe_test_sep_raid_group_class_get_emeh_values(&get_class_emeh);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* For all enabled raid groups.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        fbe_u32_t       adjusted_postion;
        fbe_object_id_t mirror_object_id = FBE_OBJECT_ID_INVALID;

        /* There is no need to wait if the rg_config is not enabled
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {        
            current_rg_config_p++;    
            continue;
        }

        /* Get the raid group object id
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

        /* Get the values
         */
        status = fbe_test_sep_raid_group_get_emeh_values(rg_object_id, &get_emeh,
                                                         FBE_TRUE,  /* Display the values */
                                                         FBE_FALSE  /* Only display on current SP*/);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* If it is a RAID-10 get EMEH values for the associated mirror pair.
         */
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            fbe_test_sep_raid_group_find_mirror_obj_and_adj_pos_for_r10(current_rg_config_p, position_to_evaluate, 
                                                                        &mirror_object_id, &adjusted_postion);
            status = fbe_test_sep_raid_group_get_emeh_values(mirror_object_id, &get_emeh,
                                                             FBE_TRUE,  /* Display the values */
                                                             FBE_FALSE  /* Only display on current SP*/);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Validate the data.
             */
            fbe_test_sep_raid_group_validate_emeh_values_using_class_data(&get_class_emeh, &get_emeh,
                                                                      mirror_object_id,
                                                                      b_expect_default,
                                                                      b_expect_disabled,
                                                                      b_expect_increase);
        }
        else
        {
            /* Validate the data.
             */
            fbe_test_sep_raid_group_validate_emeh_values_using_class_data(&get_class_emeh, &get_emeh,
                                                                          rg_object_id,
                                                                          b_expect_default,
                                                                          b_expect_disabled,
                                                                          b_expect_increase);
        }

        /* If dualsp */
        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            fbe_api_sim_transport_set_target_server(other_sp);

            /* Get the values
             */
            status = fbe_test_sep_raid_group_get_emeh_values(rg_object_id, &get_emeh,
                                                             FBE_TRUE,  /* Display the values */
                                                             FBE_FALSE  /* Only display on current SP*/);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* If it is a RAID-10 get EMEH values for the first mirror pair.
             */
            if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
            {
                status = fbe_test_sep_raid_group_get_emeh_values(mirror_object_id, &get_emeh,
                                                                 FBE_TRUE,  /* Display the values */
                                                                 FBE_FALSE  /* Only display on current SP*/);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                /* Validate the data.
                 */
                fbe_test_sep_raid_group_validate_emeh_values_using_class_data(&get_class_emeh, &get_emeh,
                                                                      mirror_object_id,
                                                                      b_expect_default,
                                                                      b_expect_disabled,
                                                                      b_expect_increase);
            }
            else
            {
                /* Validate the data.
                 */
                fbe_test_sep_raid_group_validate_emeh_values_using_class_data(&get_class_emeh, &get_emeh,
                                                                              rg_object_id,
                                                                              b_expect_default,
                                                                              b_expect_disabled,
                                                                              b_expect_increase);
            }
            fbe_api_sim_transport_set_target_server(this_sp);
        }

        /* Goto the next raid group*/
        current_rg_config_p++;

    } /* end for all raid groups that need to be rebuilt */

    return status;
}
/****************************************************
 * end fbe_test_sep_raid_group_validate_emeh_values()
 ****************************************************/

/*!***************************************************************************
 *          fbe_test_sep_raid_group_set_debug_state()
 ***************************************************************************** 
 * 
 * @brief   This function allows the changing of the following raid group values
 *          for testing purposes:
 *              o Local state mask
 *              o Clustered raid groups flags
 *
 * @param   rg_object_id - The object id of the raid group to set the values for
 * @param   set_local_state_mask - Set one or more of the local state flags
 * @param   set_clustered_flags - Set one or more of the clustered raid group
 *              flags
 * @param   set_raid_group_condition - Set a raid group condition (value should
 *              not include the class_id)
 *
 * @return  status
 * 
 * @author
 *  08/05/2015  Ron Proulx  - Created.
 * 
 *****************************************************************************/
fbe_status_t fbe_test_sep_raid_group_set_debug_state(fbe_object_id_t rg_object_id,
                                                     fbe_u64_t set_local_state_mask,     
                                                     fbe_u64_t set_clustered_flags,      
                                                     fbe_u32_t set_raid_group_condition) 
{
    fbe_status_t                        status;
    fbe_transport_connection_target_t   this_sp;

    /* Get `this' SP
     */
    this_sp = fbe_api_sim_transport_get_target_server();

    /* Trace some information.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "set rg debug state: SP: %d rg obj: 0x%x local state mask: 0x%llx clustered flags: 0x%llx raid group condition: %d",
               this_sp, rg_object_id, set_local_state_mask,
               set_clustered_flags, set_raid_group_condition);


    /* Send to the active sp.
     */
    status = fbe_api_raid_group_set_raid_group_debug_state(rg_object_id,
                                                           set_local_state_mask,
                                                           set_clustered_flags,
                                                           set_raid_group_condition);
    if (status != FBE_STATUS_OK) 
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "set rg debug state: rg obj: 0x%x failed - status: %d",
                   rg_object_id, status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }
    return status;
}
/********************************************************* 
 * end fbe_test_sep_raid_group_set_debug_state()
 *********************************************************/


/*************************************
 * end file fbe_test_sep_raid_group.c
 *************************************/
