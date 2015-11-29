#ifndef FBE_CMS_BUFF_H
#define FBE_CMS_BUFF_H

/***************************************************************************/
/** @file fbe_cms_buff_tracker.h
***************************************************************************
*
* @brief
*  This file contains buffer operations related function definitions.   
*
* @version
*   2011-11-22 - Created. Vamsi V 
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe/fbe_types.h"
#include "fbe_cms_buff_tracker.h"

/* Alloc statemachine  */
fbe_status_t fbe_cms_buff_alloc_start(fbe_cms_buff_tracker_t * tracker_p);
fbe_status_t fbe_cms_buff_alloc_complete(fbe_cms_buff_tracker_t * tracker_p);


#endif /* FBE_CMS_BUFF_H */
