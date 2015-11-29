#ifndef FBE_CMS_OPERATIONS_H
#define FBE_CMS_OPERATIONS_H

/***************************************************************************/
/** @file fbe_cms_operations.h
***************************************************************************
*
* @brief
*  This file contains function definition for buffer operation interfaces.   
*
* @version
*   2011-11-22 - Created. Vamsi V 
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe/fbe_types.h"
#include "fbe_cms.h"

fbe_status_t fbe_cms_operations_init();
fbe_status_t fbe_cms_operations_destroy();

fbe_status_t fbe_cms_operations_handle_incoming_req(fbe_cms_buff_req_t * req_p);
fbe_status_t fbe_cms_operations_complete_req(fbe_cms_buff_req_t * req_p);

#endif /* FBE_CMS_OPERATIONS_H */
