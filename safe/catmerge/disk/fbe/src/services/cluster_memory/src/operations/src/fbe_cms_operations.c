/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************/
/** @file fbe_cms_operations.c
***************************************************************************
*
* @brief
*  This file contains implementation of buffer operations interface.
*
* @version
*   2011-11-30 - Created. Vamsi V
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cms_operations.h"
#include "fbe_cms_buff.h"
#include "fbe_cms_buff_tracker_pool.h"

fbe_status_t fbe_cms_operations_init()
{
    fbe_cms_buff_tracker_pool_init();

    return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_operations_destroy()
{
    fbe_cms_buff_tracker_pool_destroy();

    return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_operations_handle_incoming_req(fbe_cms_buff_req_t * req_p)
{
    fbe_cms_buff_tracker_t * tracker_p = NULL; 

    /* TODO: Check with CMS statemachine if we are allowed to process the request */

    if(tracker_p = fbe_cms_buff_tracker_allocate(req_p))
    {
        switch(req_p->m_opcode)
        {
        case FBE_CMS_BUFF_ALLOC:
            return fbe_cms_buff_alloc_start(tracker_p);
            break;

        default:
            break;
        }
    }
    else
    {
        /* No tracker available. It will be restarted when one frees
           Queued for us by fbe_cms_buff_tracker_allocate */
    }

    return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_operations_complete_req(fbe_cms_buff_req_t * req_p)
{
	/* Invoke completion function */
	/* When CMS is seperate driver, we would need to send IOCTL to client */
	req_p->m_comp_func(req_p, req_p->m_context);
	return FBE_STATUS_OK;
}



