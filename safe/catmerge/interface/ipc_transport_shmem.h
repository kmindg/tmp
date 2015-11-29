/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 **************************************************************************/

/***************************************************************************
 *                                ipc_transport_shmem.h
 ***************************************************************************
 *
 * DESCRIPTION: Inter Process Communication (IPC) Transport implementation with Shared Memory
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    01/15/2009   Igor Bobrovnikov  Initial version
 *
 **************************************************************************/

#if !defined(_IPC_TRANSPORT_SHMEM_H_)
#define _IPC_TRANSPORT_SHMEM_H_

/*****************************************
 *       Inclides                        *
 *****************************************/

#include "ipc_transport_base.h"

/*****************************************
 *       Function declarations           *
 *****************************************/

/** \fn ipc_transport_status_t ipc_transport_shmem_create(ipc_transport_base_t ** transport, const char * segment_id, int offset, int size, ipc_transport_mode_t mode, int sema_read, int sema_write);
 *  \details
 *  this function creates (constructor) IPC transport vith shared memory mechanism
 *  \param transport - the pointer to the transport instance pointer that will be created
 *  \param segment_id - transport identifier
 *  \param size - available size within shared memory segment
 *  \param mode - ipc transport instance mode (server or client)
 *  \return status
 */
ipc_transport_status_t ipc_transport_shmem_create(ipc_transport_base_t ** transport, const char * segment_id, int size, ipc_transport_mode_t mode);

/** \fn ipc_transport_status_t ipc_transport_shmem_create(ipc_transport_base_t ** transport, const char * segment_id, int offset, int size, ipc_transport_mode_t mode, int sema_read, int sema_write);
 *  \details
 *  this function creates (constructor) IPC transport vith shared memory mechanism
 *  \param transport - the pointer to the transport instance pointer that will be created
 *  \param segment_id - shared memory segment id
 *  \param offset - offset within the shared memory segment
 *  \param size - available size within shared memory segment
 *  \param mode - ipc transport instance mode (server or client)
 *  \param sema_server - semaphore index responsible for server side access
 *  \param sema_client - semaphore index responsible for client side access
 *  \return status
 */
ipc_transport_status_t ipc_transport_shmem_create_deprecated(ipc_transport_base_t ** transport, const char * segment_id, int offset, int size, ipc_transport_mode_t mode, int sema_server, int sema_client);

#endif /* !defined(_IPC_TRANSPORT_SHMEM_H_) */