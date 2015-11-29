/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!*************************************************************************
 * @file fbe_raid_group_metadata_memory.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions related to the raid group's metadata memory.
 * 
 *  This includes the local metadata memory that we update and share with the peer,
 *  and the peer metadata memory which the local SP inspects.
 *
 * @version
 *   6/16/2011:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_raid_group.h"
#include "fbe_base_config_private.h"
#include "fbe_raid_group_object.h"
#include "fbe_raid_group_bitmap.h"    
#include "fbe/fbe_notification_interface.h"
#include "fbe_notification.h"
#include "fbe_raid_library.h"
#include "fbe_transport_memory.h"
#include "fbe_cmi.h"
/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!****************************************************************************
 * fbe_raid_group_is_peer_clustered_flag_set()
 ******************************************************************************
 * @brief
 *  This function checks to see if the peer has set this flag.
 * 
 * @param raid_group_p - a pointer to the base config
 * @param flags - flag to check in the clustered flags.
 *
 * @return fbe_status_t
 *
 * @author
 *  6/15/2011 - Created. Rob Foley
 ******************************************************************************/
fbe_bool_t fbe_raid_group_is_peer_clustered_flag_set(fbe_raid_group_t * raid_group_p, fbe_raid_group_clustered_flags_t flags)
{
    fbe_bool_t is_flag_set = FBE_FALSE;
    fbe_raid_group_metadata_memory_t * raid_group_metadata_memory_p = NULL;
    
    if (raid_group_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_FALSE;
    }

    raid_group_metadata_memory_p = (fbe_raid_group_metadata_memory_t *)raid_group_p->base_config.metadata_element.metadata_memory_peer.memory_ptr;

    if (raid_group_metadata_memory_p == NULL) 
    {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s NULL pointer\n", __FUNCTION__);
        return FBE_FALSE;
    }

    if(((raid_group_metadata_memory_p->flags & flags) == flags)) {
        is_flag_set = FBE_TRUE;
    }
    return is_flag_set;
}
/**************************************
 * end fbe_raid_group_is_peer_clustered_flag_set()
 **************************************/


/*!****************************************************************************
 * fbe_raid_group_is_peer_clust_flag_set_and_get_peer_bitmap()
 ******************************************************************************
 * @brief
 *  This function checks to see if the peer has set this flag.
 * 
 * @param raid_group_p - a pointer to the base config
 * @param flags - flag to check in the clustered flags.
 *
 * @return fbe_status_t
 *
 * @author
 *  6/15/2011 - Created. Rob Foley
 ******************************************************************************/
fbe_bool_t fbe_raid_group_is_peer_clust_flag_set_and_get_peer_bitmap(fbe_raid_group_t * raid_group_p, fbe_raid_group_clustered_flags_t flags,
                                                                     fbe_raid_position_bitmask_t *failed_drives_bitmap_p,
                                                                     fbe_raid_position_bitmask_t *failed_ios_bitmap_p)
{
    fbe_bool_t is_flag_set = FBE_FALSE;
    fbe_raid_group_metadata_memory_t * raid_group_metadata_memory_p = NULL;
    
    if (raid_group_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_FALSE;
    }

    raid_group_metadata_memory_p = (fbe_raid_group_metadata_memory_t *)raid_group_p->base_config.metadata_element.metadata_memory_peer.memory_ptr;

    if (raid_group_metadata_memory_p == NULL) 
    {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s NULL pointer\n", __FUNCTION__);
        return FBE_FALSE;
    }

    if(((raid_group_metadata_memory_p->flags & flags) == flags)) {
        is_flag_set = FBE_TRUE;
        *failed_drives_bitmap_p = raid_group_metadata_memory_p->failed_drives_bitmap;
        *failed_ios_bitmap_p = raid_group_metadata_memory_p->failed_ios_pos_bitmap;
    }
    return is_flag_set;
}
/****************************************************************
 * end fbe_raid_group_is_peer_clust_flag_set_and_get_peer_bitmap()
 *****************************************************************/

/*!****************************************************************************
 * fbe_raid_group_is_clustered_flag_set()
 ******************************************************************************
 * @brief
 *  This function checks to see if this flag is set locally.
 * 
 * @param raid_group_p - a pointer to the base config
 * @param flags - flag to check in the clustered flags.
 *
 * @return fbe_status_t
 *
 * @author
 *  6/15/2011 - Created. Rob Foley
 ******************************************************************************/
fbe_bool_t fbe_raid_group_is_clustered_flag_set(fbe_raid_group_t * raid_group_p, fbe_raid_group_clustered_flags_t flags)
{
    fbe_bool_t is_flag_set = FBE_FALSE;
    fbe_raid_group_metadata_memory_t * raid_group_metadata_memory_p = NULL;
    
    if (raid_group_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_FALSE;
    }

    raid_group_metadata_memory_p = (fbe_raid_group_metadata_memory_t *)raid_group_p->base_config.metadata_element.metadata_memory.memory_ptr;

    if (raid_group_metadata_memory_p == NULL) 
    {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s NULL pointer\n", __FUNCTION__);
        return FBE_FALSE;
    }

    if ((raid_group_metadata_memory_p->flags & flags) == flags) {
        is_flag_set = FBE_TRUE;
    }
    return is_flag_set;
}
/**************************************
 * end fbe_raid_group_is_clustered_flag_set()
 **************************************/
/*!****************************************************************************
 * fbe_raid_group_is_any_clustered_flag_set()
 ******************************************************************************
 * @brief
 *  This function checks to see if any of the input flags
 *  are set locally.
 * 
 * @param raid_group_p - a pointer to the base config
 * @param flags - flag to check in the clustered flags.
 *
 * @return fbe_status_t
 *
 * @author
 *  7/25/2011 - Created. Rob Foley
 ******************************************************************************/
fbe_bool_t fbe_raid_group_is_any_clustered_flag_set(fbe_raid_group_t * raid_group_p, fbe_raid_group_clustered_flags_t flags)
{
    fbe_bool_t is_flag_set = FBE_FALSE;
    fbe_raid_group_metadata_memory_t * raid_group_metadata_memory_p = NULL;
    
    if (raid_group_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_FALSE;
    }

    raid_group_metadata_memory_p = (fbe_raid_group_metadata_memory_t *)raid_group_p->base_config.metadata_element.metadata_memory.memory_ptr;

    if (raid_group_metadata_memory_p == NULL) 
    {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s NULL pointer\n", __FUNCTION__);
        return FBE_FALSE;
    }

    if ((raid_group_metadata_memory_p->flags & flags) != 0) {
        is_flag_set = FBE_TRUE;
    }
    return is_flag_set;
}
/**************************************
 * end fbe_raid_group_is_any_clustered_flag_set()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_get_local_bitmap()
 ******************************************************************************
 * @brief
 *  This function checks to see if this flag is set locally.
 * 
 * @param raid_group_p - a pointer to the base config
 * @param flags - flag to check in the clustered flags.
 *
 * @return fbe_status_t
 *
 * @author
 *  6/28/2011 - Created. Ashwin Tamilarasan
 ******************************************************************************/
void fbe_raid_group_get_local_bitmap(fbe_raid_group_t * raid_group_p, fbe_raid_position_bitmask_t *failed_drives_bitmap_p,
                                     fbe_raid_position_bitmask_t *failed_ios_pos_bitmap_p)
{
        
    fbe_raid_group_metadata_memory_t * raid_group_metadata_memory_p = NULL;
    
    if (raid_group_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return;
    }

    raid_group_metadata_memory_p = (fbe_raid_group_metadata_memory_t *)raid_group_p->base_config.metadata_element.metadata_memory.memory_ptr;

    if (raid_group_metadata_memory_p == NULL) 
    {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s NULL pointer\n", __FUNCTION__);
        return;
    }

    *failed_drives_bitmap_p = raid_group_metadata_memory_p->failed_drives_bitmap;
    *failed_ios_pos_bitmap_p = raid_group_metadata_memory_p->failed_ios_pos_bitmap;
    return; 
}
/**************************************
 * end fbe_raid_group_get_local_bitmap()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_get_peer_bitmap()
 ******************************************************************************
 * @brief
 *  This function checks to see if this flag is set locally.
 * 
 * @param raid_group_p - a pointer to the base config
 * @param flags - flag to check in the clustered flags.
 *
 * @return fbe_status_t
 *
 * @author
 *  6/28/2011 - Created. Ashwin Tamilarasan
 ******************************************************************************/
void fbe_raid_group_get_peer_bitmap(fbe_raid_group_t * raid_group_p, 
                                    fbe_lifecycle_state_t expected_state,
                                    fbe_raid_position_bitmask_t *failed_drives_bitmap_p,
                                    fbe_raid_position_bitmask_t *failed_ios_pos_bitmap_p)
{  
    
    if (raid_group_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return;
    }    

    /* Check if we have joined the peer. 
     */
    if (fbe_raid_group_is_peer_present(raid_group_p))
    {
        fbe_raid_group_get_clustered_peer_bitmap(raid_group_p, failed_drives_bitmap_p,failed_ios_pos_bitmap_p);
        return;
    }
    else
    {
        /* If the peer is not there, we should not wait for it*/         
        *failed_drives_bitmap_p = 0;
        *failed_ios_pos_bitmap_p = 0; 
    }
    return;    
}
/***************************************
 * end fbe_raid_group_get_peer_bitmap()
 ***************************************/

/*!****************************************************************************
 * fbe_raid_group_get_clustered_peer_bitmap()
 ******************************************************************************
 * @brief
 *  This function checks to see if this flag is set locally.
 * 
 * @param raid_group_p - a pointer to the base config
 * @param flags - flag to check in the clustered flags.
 *
 * @return fbe_status_t
 *
 * @author
 *  6/28/2011 - Created. Ashwin Tamilarasan
 ******************************************************************************/
void fbe_raid_group_get_clustered_peer_bitmap(fbe_raid_group_t * raid_group_p,
                                              fbe_raid_position_bitmask_t *failed_drives_bitmap_p,
                                              fbe_raid_position_bitmask_t *failed_ios_pos_bitmap_p)
{
        
    fbe_raid_group_metadata_memory_t * raid_group_metadata_memory_p = NULL;
    
    if (raid_group_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return;
    }

    raid_group_metadata_memory_p = (fbe_raid_group_metadata_memory_t *)raid_group_p->base_config.metadata_element.metadata_memory_peer.memory_ptr;

    if (raid_group_metadata_memory_p == NULL) 
    {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s NULL pointer\n", __FUNCTION__);
        return;
    }

    *failed_drives_bitmap_p = raid_group_metadata_memory_p->failed_drives_bitmap;
    *failed_ios_pos_bitmap_p = raid_group_metadata_memory_p->failed_ios_pos_bitmap;
    return; 
}
/************************************************
 * end fbe_raid_group_get_clustered_peer_bitmap()
 ************************************************/

/*!****************************************************************************
 * fbe_raid_group_get_disabled_peer_bitmap()
 ******************************************************************************
 * @brief
 *  This function gets the peer's disabled bits.
 * 
 * @param raid_group_p - a pointer to the base config
 * @param disabled_pos_bitmap_p - 
 *
 * @return fbe_status_t
 *
 * @author
 *  1/29/2013 - Created. Rob Foley
 ******************************************************************************/
void fbe_raid_group_get_disabled_peer_bitmap(fbe_raid_group_t * raid_group_p,
                                             fbe_raid_position_bitmask_t *disabled_pos_bitmap_p)
{
    fbe_raid_group_metadata_memory_t * raid_group_metadata_memory_p = NULL;
    
    if (raid_group_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return;
    }

    raid_group_metadata_memory_p = (fbe_raid_group_metadata_memory_t *)raid_group_p->base_config.metadata_element.metadata_memory_peer.memory_ptr;

    if (raid_group_metadata_memory_p == NULL) 
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                              "%s NULL pointer\n", __FUNCTION__);
        return;
    }

    *disabled_pos_bitmap_p = raid_group_metadata_memory_p->disabled_pos_bitmap;
    return; 
}
/************************************************
 * end fbe_raid_group_get_disabled_peer_bitmap()
 ************************************************/
/*!****************************************************************************
 * fbe_raid_group_get_disabled_local_bitmap()
 ******************************************************************************
 * @brief
 *  This function gets the peer's disabled bits.
 * 
 * @param raid_group_p - a pointer to the base config
 * @param disabled_pos_bitmap_p - pointer to value to return.
 *
 * @return fbe_status_t
 *
 * @author
 *  2/8/2013 - Created. Rob Foley
 ******************************************************************************/
void fbe_raid_group_get_disabled_local_bitmap(fbe_raid_group_t * raid_group_p,
                                              fbe_raid_position_bitmask_t *disabled_pos_bitmap_p)
{
    fbe_raid_group_metadata_memory_t * raid_group_metadata_memory_p = NULL;
    
    if (raid_group_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return;
    }

    raid_group_metadata_memory_p = (fbe_raid_group_metadata_memory_t *)raid_group_p->base_config.metadata_element.metadata_memory.memory_ptr;

    if (raid_group_metadata_memory_p == NULL) 
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                              "%s NULL pointer\n", __FUNCTION__);
        return;
    }

    *disabled_pos_bitmap_p = raid_group_metadata_memory_p->disabled_pos_bitmap;
    return; 
}
/************************************************
 * end fbe_raid_group_get_disabled_local_bitmap()
 ************************************************/


/*!****************************************************************************
 * fbe_raid_group_is_peer_flag_set()
 ******************************************************************************
 * @brief
 *  This function checks to see if the peer has set this flag.
 * 
 *  If the peer is not there or the object is not in the expected state,
 *  then we will consider the flag set so that this object does not wait for it.
 * 
 * @param raid_group_p - a pointer to the base config
 * @param b_peer_set_p - Pointer to the boolean that is an output parameter.
 *                       TRUE means that the peer has the flag set
 *                       FALSE means the peer does not have this bit set.
 * @param peer_flag - flag to check in the clustered flags.
 *
 * @return fbe_status_t
 *
 * @author
 *  6/15/2011 - Created. Rob Foley
 ******************************************************************************/
fbe_status_t fbe_raid_group_is_peer_flag_set(fbe_raid_group_t* raid_group_p, 
                                             fbe_bool_t* b_peer_set_p,
                                             fbe_raid_group_clustered_flags_t peer_flag)
{
    /* Check if the peer has joined. 
     */
    if (fbe_raid_group_is_peer_present(raid_group_p))
    {
        *b_peer_set_p = fbe_raid_group_is_peer_clustered_flag_set(raid_group_p, peer_flag);
    }
    else
    {
        /* If the peer is not there, we should not wait for it,
         * assume the bit is set. 
         */
        *b_peer_set_p = FBE_TRUE;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_is_peer_flag_set()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_is_peer_flag_set_and_get_peer_bitmap()
 ******************************************************************************
 * @brief
 *  This function checks to see if the peer has set this flag.
 * 
 *  If the peer is not there or the object is not in the expected state,
 *  then we will consider the flag set so that this object does not wait for it.
 * 
 * @param raid_group_p - a pointer to the base config
 * @param b_peer_set_p - Pointer to the boolean that is an output parameter.
 *                       TRUE means that the peer has the flag set
 *                       FALSE means the peer does not have this bit set.
 * @param expected_state - State we expect peer object to be in.
 * @param peer_flag - flag to check in the clustered flags.
 *
 * @return fbe_status_t
 *
 * @author
 *  6/28/2011 - Created. Ashwin Tamilarasan
 ******************************************************************************/
fbe_status_t fbe_raid_group_is_peer_flag_set_and_get_peer_bitmap(fbe_raid_group_t* raid_group_p, 
                                                                 fbe_bool_t* b_peer_set_p,
                                                                 fbe_lifecycle_state_t expected_state,
                                                                 fbe_raid_group_clustered_flags_t peer_flag,
                                                                 fbe_raid_position_bitmask_t   *failed_drives_bitmap_p,
                                                                 fbe_raid_position_bitmask_t   *failed_ios_bitmap_p)
{
    /* Check if we have joined the peer.
     */
    if (fbe_raid_group_is_peer_present(raid_group_p))
    {
        *b_peer_set_p = fbe_raid_group_is_peer_clust_flag_set_and_get_peer_bitmap(raid_group_p, peer_flag, failed_drives_bitmap_p,
                                                                                  failed_ios_bitmap_p);
    }
    else
    {
        /* If the peer is not there, we should not wait for it,
         * assume the bit is set. 
         */
        *b_peer_set_p = FBE_TRUE;
    }
    return FBE_STATUS_OK;
}
/***********************************************************
 * end fbe_raid_group_is_peer_flag_set_and_get_peer_bitmap()
 ***********************************************************/

/*!**************************************************************
 * fbe_raid_group_set_clustered_flag()
 ****************************************************************
 * @brief
 *  Set the local clustered flags and update peer if needed.
 *
 * @param raid_group_p - The object.
 * @param flags - The flags to clear.
 *
 * @return fbe_status_t
 *
 * @author
 *  6/16/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t 
fbe_raid_group_set_clustered_flag(fbe_raid_group_t * raid_group_p, fbe_raid_group_clustered_flags_t flags)
{
    fbe_raid_group_metadata_memory_t * raid_group_metadata_memory = NULL;

    if(raid_group_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID){
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    raid_group_metadata_memory = (fbe_raid_group_metadata_memory_t *)raid_group_p->base_config.metadata_element.metadata_memory.memory_ptr;

    if (raid_group_metadata_memory == NULL) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s NULL pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (!((raid_group_metadata_memory->flags & flags) == flags))
    {
        raid_group_metadata_memory->flags |= flags;
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*)raid_group_p, FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY);
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_group_set_clustered_flag()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_set_clustered_flag_and_bitmap()
 ****************************************************************
 * @brief
 *  Set the local clustered flags and bitmaps and update peer if needed.
 *
 * @param raid_group_p - The object.
 * @param flags - The flags to clear.
 * @param failed_drives_bitmap - 
 * @param failed_ios_bitmap -
 * @param disabled_pos_bitmap - 
 *
 * @return fbe_status_t
 *
 * @author
 *  6/28/2011 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
fbe_status_t 
fbe_raid_group_set_clustered_flag_and_bitmap(fbe_raid_group_t * raid_group_p, fbe_raid_group_clustered_flags_t flags,
                                             fbe_raid_position_bitmask_t failed_drives_bitmap, 
                                             fbe_raid_position_bitmask_t failed_ios_pos_bitmap, 
                                             fbe_raid_position_bitmask_t disabled_pos_bitmap)
{
    fbe_raid_group_metadata_memory_t * raid_group_metadata_memory = NULL;

    if(raid_group_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID){
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    raid_group_metadata_memory = (fbe_raid_group_metadata_memory_t *)raid_group_p->base_config.metadata_element.metadata_memory.memory_ptr;

    if (raid_group_metadata_memory == NULL) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s NULL pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (!(((raid_group_metadata_memory->flags & flags) == flags) &&
          ((raid_group_metadata_memory->failed_drives_bitmap & failed_drives_bitmap) == failed_drives_bitmap) &&
          ((raid_group_metadata_memory->failed_ios_pos_bitmap & failed_ios_pos_bitmap) == failed_ios_pos_bitmap) &&
          ((raid_group_metadata_memory->disabled_pos_bitmap & disabled_pos_bitmap) == disabled_pos_bitmap)) )
    {
        raid_group_metadata_memory->flags |= flags;
        raid_group_metadata_memory->failed_drives_bitmap |= failed_drives_bitmap;
        raid_group_metadata_memory->failed_ios_pos_bitmap |= failed_ios_pos_bitmap;  
        raid_group_metadata_memory->disabled_pos_bitmap |= disabled_pos_bitmap;  
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*)raid_group_p, FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY);
    }

    return FBE_STATUS_OK;
}
/****************************************************
 * end fbe_raid_group_set_clustered_flag_and_bitmap()
 ****************************************************/

/*!**************************************************************
 * fbe_raid_group_set_clustered_bitmap()
 ****************************************************************
 * @brief
 *  Set the local bitmaps and update peer if needed.
 *
 * @param raid_group_p - The object.
 * @param failed_drives_bitmap - Bitmap to update
 *
 * @return fbe_status_t
 *
 * @author
 *  7/22/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t 
fbe_raid_group_set_clustered_bitmap(fbe_raid_group_t * raid_group_p,
                                    fbe_raid_position_bitmask_t failed_drives_bitmap)
{
    fbe_raid_group_metadata_memory_t * raid_group_metadata_memory = NULL;

    if(raid_group_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID){
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    raid_group_metadata_memory = (fbe_raid_group_metadata_memory_t *)raid_group_p->base_config.metadata_element.metadata_memory.memory_ptr;

    if (raid_group_metadata_memory == NULL) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s NULL pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (failed_drives_bitmap != raid_group_metadata_memory->failed_drives_bitmap)
    {
        raid_group_metadata_memory->failed_drives_bitmap = failed_drives_bitmap;
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*)raid_group_p, 
                               FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY);
    }

    return FBE_STATUS_OK;
}
/****************************************************
 * end fbe_raid_group_set_clustered_bitmap()
 ****************************************************/

/*!**************************************************************
 * fbe_raid_group_set_clustered_disabled_bitmap()
 ****************************************************************
 * @brief
 *  Set the local disabled bitmask and update peer if needed.
 *
 * @param raid_group_p - The object.
 * @param disabled_drives_bitmap - Bitmap to update
 *
 * @return fbe_status_t
 *
 * @author
 *  2/8/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t 
fbe_raid_group_set_clustered_disabled_bitmap(fbe_raid_group_t * raid_group_p,
                                             fbe_raid_position_bitmask_t disabled_pos_bitmap)
{
    fbe_raid_group_metadata_memory_t * raid_group_metadata_memory = NULL;

    if(raid_group_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID){
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    raid_group_metadata_memory = (fbe_raid_group_metadata_memory_t *)raid_group_p->base_config.metadata_element.metadata_memory.memory_ptr;

    if (raid_group_metadata_memory == NULL) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s NULL pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (disabled_pos_bitmap != raid_group_metadata_memory->disabled_pos_bitmap)
    {
        raid_group_metadata_memory->disabled_pos_bitmap = disabled_pos_bitmap;
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*)raid_group_p, 
                               FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY);
    }

    return FBE_STATUS_OK;
}
/****************************************************
 * end fbe_raid_group_set_clustered_disabled_bitmap()
 ****************************************************/

/*!**************************************************************
 * fbe_raid_group_clear_clustered_flag_and_bitmap()
 ****************************************************************
 * @brief
 *  Clear the local clustered flags and update peer if needed.
 *
 * @param raid_group_p - The object.
 * @param flags - The flags to clear.
 *
 * @return fbe_status_t
 *
 * @author
 *  6/16/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t 
fbe_raid_group_clear_clustered_flag_and_bitmap(fbe_raid_group_t * raid_group_p, fbe_raid_group_clustered_flags_t flags)
{
    fbe_raid_group_metadata_memory_t * raid_group_metadata_memory = NULL;

    if(raid_group_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID){
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    raid_group_metadata_memory = (fbe_raid_group_metadata_memory_t *)raid_group_p->base_config.metadata_element.metadata_memory.memory_ptr;

    if (raid_group_metadata_memory == NULL) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s NULL pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (((raid_group_metadata_memory->flags & flags) != 0)||
        (raid_group_metadata_memory->failed_drives_bitmap != 0) ||
        (raid_group_metadata_memory->failed_ios_pos_bitmap != 0) ||
        (raid_group_metadata_memory->disabled_pos_bitmap != 0))
    {
        raid_group_metadata_memory->flags &= ~flags;
        raid_group_metadata_memory->failed_drives_bitmap = 0;
        raid_group_metadata_memory->failed_ios_pos_bitmap = 0; 
        raid_group_metadata_memory->disabled_pos_bitmap = 0; 
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, 
                               (fbe_base_object_t*)raid_group_p, 
                               FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY);
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_group_clear_clustered_flag_and_bitmap()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_clear_clustered_flag()
 ****************************************************************
 * @brief
 *  Clear the local clustered flags and update peer if needed.
 *
 * @param raid_group_p - The object.
 * @param flags - The flags to clear.
 *
 * @return fbe_status_t
 *
 * @author
 *  6/16/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t 
fbe_raid_group_clear_clustered_flag(fbe_raid_group_t * raid_group_p, fbe_raid_group_clustered_flags_t flags)
{
    fbe_raid_group_metadata_memory_t * raid_group_metadata_memory = NULL;

    if(raid_group_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID){
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    raid_group_metadata_memory = (fbe_raid_group_metadata_memory_t *)raid_group_p->base_config.metadata_element.metadata_memory.memory_ptr;

    if (raid_group_metadata_memory == NULL) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s NULL pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ((raid_group_metadata_memory->flags & flags) != 0)
    {
        raid_group_metadata_memory->flags &= ~flags;        
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, 
                               (fbe_base_object_t*)raid_group_p, 
                               FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY);
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_group_clear_clustered_flag()
 ******************************************/

/*!****************************************************************************
 * fbe_raid_group_get_local_slf_drives()
 ******************************************************************************
 * @brief
 *  This function gets the number of local slf drives.
 * 
 * @param raid_group_p - a pointer to the raid group
 *
 * @return number of slf drives
 *
 * @author
 *  5/02/2012 - Created. Lili Chen
 ******************************************************************************/
fbe_u16_t fbe_raid_group_get_local_slf_drives(fbe_raid_group_t * raid_group_p)
{
    fbe_u16_t num_drives = 0;
    fbe_raid_group_metadata_memory_t * raid_group_metadata_memory_p = NULL;
    
    if (raid_group_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID)
    {
        return num_drives;
    }

    raid_group_metadata_memory_p = (fbe_raid_group_metadata_memory_t *)raid_group_p->base_config.metadata_element.metadata_memory.memory_ptr;

    if (raid_group_metadata_memory_p == NULL) 
    {
        return num_drives;
    }

    return (raid_group_metadata_memory_p->num_slf_drives);
}
/**************************************
 * end fbe_raid_group_get_local_slf_drives()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_get_peer_slf_drives()
 ******************************************************************************
 * @brief
 *  This function gets the number of peer slf drives.
 * 
 * @param raid_group_p - a pointer to the raid group
 *
 * @return number of slf drives
 *
 * @author
 *  5/02/2012 - Created. Lili Chen
 ******************************************************************************/
fbe_u16_t fbe_raid_group_get_peer_slf_drives(fbe_raid_group_t * raid_group_p)
{
    fbe_u16_t num_drives = ((fbe_base_config_t *)raid_group_p)->width;
    fbe_raid_group_metadata_memory_t * raid_group_metadata_memory_p = NULL;

    if (!fbe_base_config_is_peer_present((fbe_base_config_t *)raid_group_p))
    {
        return num_drives;
    }

    if (raid_group_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID)
    {
        return num_drives;
    }

    raid_group_metadata_memory_p = (fbe_raid_group_metadata_memory_t *)raid_group_p->base_config.metadata_element.metadata_memory_peer.memory_ptr;

    if (raid_group_metadata_memory_p == NULL) 
    {
        return num_drives;
    }

    return (raid_group_metadata_memory_p->num_slf_drives);
}
/**************************************
 * end fbe_raid_group_get_peer_slf_drives()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_set_local_slf_drives()
 ******************************************************************************
 * @brief
 *  This function sets the number of local slf drives.
 * 
 * @param raid_group_p - a pointer to the raid group
 * @param num_slf_drives - number of slf drives
 *
 * @return number of slf drives
 *
 * @author
 *  5/02/2012 - Created. Lili Chen
 ******************************************************************************/
void fbe_raid_group_set_local_slf_drives(fbe_raid_group_t * raid_group_p, fbe_u16_t num_slf_drives)
{
    fbe_raid_group_metadata_memory_t * raid_group_metadata_memory_p = NULL;
    
    if (raid_group_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return;
    }

    raid_group_metadata_memory_p = (fbe_raid_group_metadata_memory_t *)raid_group_p->base_config.metadata_element.metadata_memory.memory_ptr;

    if (raid_group_metadata_memory_p == NULL) 
    {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s NULL pointer\n", __FUNCTION__);
        return;
    }

    raid_group_metadata_memory_p->num_slf_drives = num_slf_drives;
    fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*)raid_group_p, FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY);

    return;
}
/**************************************
 * end fbe_raid_group_get_local_slf_drives()
 **************************************/

/*!**************************************************************
 * fbe_raid_group_metadata_memory_set_capacity()
 ****************************************************************
 * @brief
 *  Set the new capacity into the metadata memory.
 *
 * @param raid_group_p - The object.
 * @param new_cap
 *
 * @return fbe_status_t
 *
 * @author
 *  10/18/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t 
fbe_raid_group_metadata_memory_set_capacity(fbe_raid_group_t * raid_group_p,    
                                            fbe_lba_t new_cap)
{
    fbe_raid_group_metadata_memory_t * raid_group_metadata_memory = NULL;

    if(raid_group_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID){
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    raid_group_metadata_memory = (fbe_raid_group_metadata_memory_t *)raid_group_p->base_config.metadata_element.metadata_memory.memory_ptr;

    if (raid_group_metadata_memory == NULL) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s NULL pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (raid_group_metadata_memory->capacity_expansion_blocks != new_cap)
    {
        raid_group_metadata_memory->capacity_expansion_blocks = new_cap;
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*)raid_group_p, FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY);
    }

    return FBE_STATUS_OK;
}
/****************************************************
 * end fbe_raid_group_metadata_memory_set_capacity()
 ****************************************************/

/*!****************************************************************************
 * fbe_raid_group_is_any_peer_clustered_flag_set()
 ******************************************************************************
 * @brief
 *  This function checks to see if any of the input flags
 *  are set on the peer.
 * 
 * @param raid_group_p - a pointer to the base config
 * @param flags - flag to check in the clustered flags.
 *
 * @return fbe_status_t
 *
 * @author
 *  5/11/2015 - Created. Ron Proulx
 ******************************************************************************/
fbe_bool_t fbe_raid_group_is_any_peer_clustered_flag_set(fbe_raid_group_t * raid_group_p, fbe_raid_group_clustered_flags_t flags)
{
    fbe_raid_group_metadata_memory_t * raid_group_metadata_memory_p = NULL;
    
    if (raid_group_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_FALSE;
    }

    raid_group_metadata_memory_p = (fbe_raid_group_metadata_memory_t *)raid_group_p->base_config.metadata_element.metadata_memory_peer.memory_ptr;

    if (raid_group_metadata_memory_p == NULL) 
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                              "%s NULL pointer\n", __FUNCTION__);
        return FBE_FALSE;
    }

    if ((raid_group_metadata_memory_p->flags & flags) != 0) 
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}
/**************************************
 * end fbe_raid_group_is_any_peer_clustered_flag_set()
 **************************************/

/*************************
 * end file fbe_raid_group_metadata_memory.c
 *************************/


