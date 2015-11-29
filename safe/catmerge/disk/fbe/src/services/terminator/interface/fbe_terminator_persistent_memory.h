/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_terminator_persistent_memory.h
 ***************************************************************************
 *
 * @brief
 *  
 *
 * @version
 *   2011-10-31 - Created. Matthew Ferson
 *
 ***************************************************************************/

#include "fbe/fbe_types.h"
#include "fbe_terminator_common.h"
#include "flare_sgl.h"
#include "fbe_cms.h"

#define FBE_TERMINATOR_PERSISTENT_MEMORY_SIZE_BYTES 0x800000

/*! @enum fbe_terminator_persistent_memory_state_s
 *  @brief Enumerates all of the possible memory persistence states
 */
typedef enum fbe_terminator_persistent_memory_state_s
{
    FBE_TERMINATOR_PERSISTENT_MEMORY_STATE_INVALID      = 0,
    FBE_TERMINATOR_PERSISTENT_MEMORY_STATE_INITIALIZED,
    FBE_TERMINATOR_PERSISTENT_MEMORY_STATE_DESTROYED,

} fbe_terminator_persistent_memory_state_t;

typedef struct fbe_terminator_persistent_memory_global_data_s
{
    SGL                                         sgl[FBE_CMS_MAXIMUM_SYSTEM_SGL_LENGTH];
    fbe_bool_t                                  persistence_request;
    fbe_cms_memory_persist_status_t             persistence_status;
    fbe_terminator_persistent_memory_state_t    memory_state;

} fbe_terminator_persistent_memory_global_data_t;


/* Public Function Prototypes*/
fbe_status_t fbe_terminator_persistent_memory_init(void);
fbe_status_t fbe_terminator_persistent_memory_destroy(void);
fbe_status_t fbe_terminator_persistent_memory_set_persistence_request(fbe_bool_t);
fbe_status_t fbe_terminator_persistent_memory_get_persistence_request(fbe_bool_t *);
fbe_status_t fbe_terminator_persistent_memory_set_persistence_status(fbe_cms_memory_persist_status_t);
fbe_status_t fbe_terminator_persistent_memory_get_persistence_status(fbe_cms_memory_persist_status_t *);
fbe_status_t fbe_terminator_persistent_memory_get_sgl(SGL *);
fbe_status_t fbe_terminator_persistent_memory_zero_memory(void);
