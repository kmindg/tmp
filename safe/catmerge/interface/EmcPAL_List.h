//***************************************************************************
// Copyright (C) EMC Corporation 2011
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

/*!
 * @file EmcPAL_List.h
 * @addtogroup emcpal_misc
 * @{
 *
 * @brief
 *      This file contains List abstraction
 *
 * @version
 *      03/16/2011 Ashok Tamilarasan - Created.
 *
 */

#ifndef _EMCPAL_LIST_H_
#define _EMCPAL_LIST_H_

#include "EmcPAL.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined (EMCPAL_USE_CSX_LISTS)
typedef csx_dlist_entry_t EMCPAL_LIST_ENTRY, *PEMCPAL_LIST_ENTRY;
#define EmcpalInitializeListHead csx_dlist_head_init

#define EmcpalRemoveListEntry csx_dlist_remove_entry

#define EmcpalIsListEmpty     csx_dlist_is_empty

#define EmcpalRemoveListHead  csx_dlist_remove_entry_at_head

#define EmcpalRemoveListTail  csx_dlist_remove_entry_at_tail

#define EmcpalInsertListTail  csx_dlist_add_entry_tail

#define EmcpalInsertListHead  csx_dlist_add_entry_head
#else
/*! @brief EmcPAL list entry */
typedef LIST_ENTRY EMCPAL_LIST_ENTRY, *PEMCPAL_LIST_ENTRY;		/*!< EmcPAL Pointer to list entry */
#define EmcpalInitializeListHead InitializeListHead				/*!< EmcPAL initialize list head */

#define EmcpalRemoveListEntry RemoveEntryList					/*!< EmcPAL remove list entry */

#define EmcpalIsListEmpty     IsListEmpty						/*!< EmcPAL is list empty */

#define EmcpalRemoveListHead  RemoveHeadList					/*!< EmcPAL remove list head */

#define EmcpalRemoveListTail  RemoveTailList					/*!< EmcPAL remove list tail */

#define EmcpalInsertListTail  InsertTailList					/*!< EmcPAL insert list tail */

#define EmcpalInsertListHead  InsertHeadList					/*!< EmcPAL insert list head */
#endif


#ifdef __cplusplus
}
#endif

/*!
 * @} end group emcpal_misc
 * @} end file EmcPAL_List.h
 */

#endif /* _EMCPAL_LIST_H_ */

