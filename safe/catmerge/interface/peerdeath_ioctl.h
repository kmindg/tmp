#ifndef __peerdeath_ioctl_interface_h
#define __peerdeath_ioctl_interface_h
/*@LB************************************************************************
 ****************************************************************************
 ****************************************************************************
 ****
 ****  Copyright (c) 2004-2007 EMC Corporation.
 ****  All rights reserved.
 ****
 ****  ALL RIGHTS ARE RESERVED BY EMC CORPORATION.
 ****  ACCESS TO THIS SOURCE CODE IS STRICTLY RESTRICTED UNDER CONTRACT.
 ****  THIS CODE IS TO BE KEPT STRICTLY CONFIDENTIAL.
 ****
 ****************************************************************************
 ****************************************************************************
 *@LE************************************************************************/

/*@FB************************************************************************
 ****************************************************************************
 ****************************************************************************
 ****
 **** FILE: peerdeath_ioctl.h
 ****
 **** DESCRIPTION:
 **** 
 **** This file contains the definitions required for the ioctl interface to
 **** STONITH.
 ****
 **** NOTES:
 ****    <TBS>
 ****
 ****************************************************************************
 ****************************************************************************
 *@FE************************************************************************/

#define PDN_DEVICE_NAME        "PeerDeathNotificationDevice"
#define PDN_DRIVER_NAME        "PeerDeathNotificationDriver"


typedef enum 
{
    PDN_IOCTL_GET_CURRENT_STATE=0,
    PDN_IOCTL_CLEAR_CURRENT_STATE,

} pdn_ioctl_code_e;

typedef enum 
{
    PDN_INITIALIZE=0,             // Initial state.  Have not seen death notice.
    PDN_STONITH_RESET,            // STONITH reset the peer.  Assume it will come up.
    PDN_PEER_FLARE_DOWN,          // CMID has notified pdn of peer death.
    PDN_PEER_PRESENT,             // We have not been notified of peer death.  Peer is up or coming up.
    PDN_PEER_NOT_PRESENT,         // We have not been notified of peer death.  Peer is not present (other side not booted).
    PDN_PEER_REQUEST_ERROR,       // Our attempt to determine the peer state failed.
} pdn_state_e;


// pitars -- placeholder, do we need this?
typedef struct _pdn_ioctl_in_t
{
    pdn_ioctl_code_e request;

} pdn_ioctl_in_t;

typedef struct _pdn_ioctl_out_t
{
    csx_u64_t   stateSetTick;
    pdn_state_e state;

} pdn_ioctl_out_t;

typedef enum 
{
    PDN_STATUS_SUCCESS=0,
    PDN_STATUS_INVALID_ARG,

} pdn_status_e;


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

/*@TB************************************************************************
 ****************************************************************************
 ****************************************************************************
 ****
 **** END: peerdeath_ioctl.h
 ****
 ****************************************************************************
 ****************************************************************************
 *@TE************************************************************************/
#endif                                   /* __peerdeath_ioctl_interface_h */

