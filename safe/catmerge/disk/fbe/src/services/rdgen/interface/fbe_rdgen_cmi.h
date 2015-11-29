#ifndef FBE_RDGEN_CMI_H
#define FBE_RDGEN_CMI_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_rdgen_cmi.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions related to rdgen's cmi connection.
 *
 * @version
 *   9/27/2010:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_rdgen.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

enum fbe_rdgen_cmi_constants_e
{
    FBE_METADATA_CMI_MEMORY_UPDATE_DATA_SIZE = 256, /* in bytes */
};

/*!*******************************************************************
 * @enum fbe_rdgen_cmi_message_type_t
 *********************************************************************
 * @brief Describes the kind of message being received.
 *
 *********************************************************************/
typedef enum fbe_rdgen_cmi_message_type_e 
{
    FBE_RDGEN_CMI_MESSAGE_TYPE_INVALID,
    FBE_RDGEN_CMI_MESSAGE_TYPE_START_REQUEST, /*!< Object metadata memory update */
    FBE_RDGEN_CMI_MESSAGE_TYPE_PEER_ALIVE,    /*!< Peer is now up. */
    FBE_RDGEN_CMI_MESSAGE_TYPE_REQUEST_RESP_NO_RESOURCES, /*!< Peer waiting for resources. */
    FBE_RDGEN_CMI_MESSAGE_TYPE_REQUEST_RESPONSE, /*!< This is the response to the start request msg. */
	FBE_RDGEN_CMI_MESSAGE_TYPE_CMI_PERFORMANCE_TEST, /*!< used to send varying size messages  for cmi performance testing*/

    FBE_RDGEN_CMI_MESSAGE_TYPE_LAST, /*!< Always the last msg. */
}
fbe_rdgen_cmi_message_type_t;

/*!*************************************************
 * @typedef fbe_rdgen_cmi_start_request_t
 ***************************************************
 * @brief
 *   This is the standard payload of a message.
 *
 ***************************************************/
typedef struct fbe_rdgen_cmi_start_request_s
{
    fbe_rdgen_control_start_io_t start_io;
}
fbe_rdgen_cmi_start_request_t;

/*!*************************************************
 * @typedef fbe_rdgen_cmi_message_t
 ***************************************************
 * @brief
 *   This is our rdgen message transported over CMI.
 *
 ***************************************************/
typedef struct fbe_rdgen_cmi_message_s 
{
    fbe_rdgen_cmi_message_type_t metadata_cmi_message_type; /*!< Identifies purpose. PLEASE LEAVE AS FIRST ARGUMENT IN STRUCTURE */
    void *peer_ts_ptr; /*!< Ptr to the peer's context.  Peer gets this back in response. */
    fbe_status_t status; /*!< Status of the packet sent on the peer. */
    
    union
    {
        fbe_rdgen_cmi_start_request_t start_request;
    }
    msg;
}
fbe_rdgen_cmi_message_t;


static __forceinline fbe_rdgen_cmi_message_t *
fbe_rdgen_start_request_to_cmi_message(fbe_rdgen_cmi_start_request_t * start_request_p)
{
    fbe_rdgen_cmi_message_t * rdgen_cmi_message_p;
    rdgen_cmi_message_p = (fbe_rdgen_cmi_message_t  *)((fbe_u8_t *)start_request_p - (fbe_u8_t *)(&((fbe_rdgen_cmi_message_t  *)0)->msg.start_request));
    return rdgen_cmi_message_p;
}


fbe_status_t fbe_rdgen_cmi_init(void);

fbe_status_t fbe_rdgen_cmi_destroy(void);

fbe_status_t fbe_rdgen_cmi_send_message(fbe_rdgen_cmi_message_t * rdgen_cmi_message, fbe_packet_t * packet);

#endif /* FBE_RDGEN_CMI_H */

/*************************
 * end file fbe_rdgen_cmi.h
 *************************/