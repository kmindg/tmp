#ifndef FBE_CMS_DEFINES_H
#define FBE_CMS_DEFINES_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cms_defines.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions for the cluster memory service.
 *
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"

/* Maximum supported CMS memory size: 512MB */
#define FBE_CMS_MAX_MEM_SIZE (0x20000000)
/* Buffer Size: 32k */
#define FBE_CMS_BUFFER_SIZE (0x8000)
#define FBE_CMS_BUFFER_SIZE_IN_SECTORS (64)

#define FBE_CMS_INVALID_ALLOC_ID (0xffffffffffffffff)

/* Hash Table related Defines */
#define FBE_CMS_AVG_HASH_CHAIN_SIZE (1)
#define FBE_CMS_MAX_HASH_SIZE (((FBE_CMS_MAX_MEM_SIZE/FBE_CMS_BUFFER_SIZE) * ((fbe_u64_t)sizeof(fbe_cms_cluster_ht_entry_t))) / FBE_CMS_AVG_HASH_CHAIN_SIZE)
#define FBE_CMS_MAX_HASH_ENTRIES ((FBE_CMS_MAX_MEM_SIZE/FBE_CMS_BUFFER_SIZE) / FBE_CMS_AVG_HASH_CHAIN_SIZE)
#define FBE_CMS_CMM_MEMORY_BLOCKING_SIZE (512 * 520 * 4) //TODO: use (CMM_MEMORY_BLOCKING_SIZE) 

/* Operations related Defines */
#define FBE_CMS_BUFF_TRACKER_CNT (1024)

/***********************************************
 * end file fbe_cms_defines.h
 ***********************************************/
#endif /* end FBE_CMS_DEFINES_H */

