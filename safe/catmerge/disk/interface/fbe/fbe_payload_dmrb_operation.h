#ifndef FBE_PAYLOAD_DMRB_OPERATION_H
#define FBE_PAYLOAD_DMRB_OPERATION_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_payload_dmrb_operation.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the definitions of the fbe_payload_dmrb_operation_t
 *  structure and the related structures, enums and definitions.
 * 
 *  The @ref fbe_payload_dmrb_operation_t is the payload for the communication
 *  with miniport driver.
 *
 * @version
 *   8/2008:  Created. Peter Puhov.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_time.h"

#include "fbe/fbe_payload_operation_header.h"
/*! @defgroup fbe_payload_dmrb_operation DMRB Payload
 *  
 *  This is the set of definitions that make up the DMRB payload.
 *  
 *  @ingroup fbe_transport_interfaces
 * @{
 */ 

/*! @enum fbe_payload_dmrb_operation_constants_e 
 *  
 *  @brief These are the supported qualifier values for the dmrb operation.
 *         Positive values are associated with the status
 *         FBE_PAYLOAD_DMRB_OPERATION_STATUS_SUCCESS, and negative values are
 *         associated with negative status values which have a bad status.
 */
enum fbe_payload_dmrb_operation_constants_e{

    /*! This is a size of opaque memory.
     */
#ifndef ALAMOSA_WINDOWS_ENV
    FBE_PAYLOAD_DMRB_MEMORY_SIZE = 480, /* This should be enough to contain CPD_DMRB and DPC structure */
#else
    FBE_PAYLOAD_DMRB_MEMORY_SIZE = 500, /* This should be enough to contain CPD_DMRB and DPC structure */
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - structure size mess */
};

/*! @struct fbe_payload_dmrb_operation_t 
 *
 *  @brief This is the payload which is used by pmc_shim to communicate with miniport.
 */
typedef struct fbe_payload_dmrb_operation_s{
    /*! This must be first in the payload since the functions which manipulate 
     * the payload will cast it to a payload operation header. 
     */
    fbe_payload_operation_header_t			operation_header;

    /*! This opaque memory will be mapped to the CPD_DMRB_TAG (CPD_DMRB) structure.
     */
	fbe_u8_t opaque[FBE_PAYLOAD_DMRB_MEMORY_SIZE];

}fbe_payload_dmrb_operation_t;

/*!**************************************************************
 * @fn fbe_payload_dmrb_get_memory(
 *          fbe_payload_dmrb_operation_t *payload_dmrb_operation,
 *          void ** opaque)
 ****************************************************************
 * @brief Returns the pointer to the opaque memory.
 *
 * @param payload_dmrb_operation - Ptr to the dmrb payload struct.
 * @param opaque - Ptr to the memory to return.
 *
 * @return Always FBE_STATUS_OK
 * 
 * @see fbe_payload_dmrb_operation_t 
 * @see fbe_payload_dmrb_operation_t::opaque
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_payload_dmrb_get_memory(fbe_payload_dmrb_operation_t * payload_dmrb_operation, void ** opaque)
{
	 * opaque = payload_dmrb_operation->opaque;
	 return FBE_STATUS_OK;
}

/*! @} */ /* end of group fbe_dmrb_operation_payload */

#endif /* FBE_PAYLOAD_DMRB_OPERATION_H */
