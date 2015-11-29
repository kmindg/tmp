/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 **************************************************************************/

/***************************************************************************
 *                                ipc_transport_base.h
 ***************************************************************************
 *
 * DESCRIPTION: Inter Process Communication (IPC) Transport Base types and functionality
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    01/15/2009   Igor Bobrovnikov  Initial version
 *
 **************************************************************************/

#if !defined(_IPC_TRANSPORT_BASE_H_)
#define _IPC_TRANSPORT_BASE_H_

#include "csx_ext.h"

/*****************************************
 *       Type declarations               *
 *****************************************/

/** \enum ipc_transport_status_t
 *  \details
 *  This enum defines the transport status / error codes
 */
typedef enum ipc_transport_status_e {
    IPC_TRANSPORT_STATUS_GENERIC_ERROR = 0, /**< Unknown error */
    IPC_TRANSPORT_STATUS_OK = 1,            /**< Success */
    IPC_TRANSPORT_STATUS_RESOURCE_ERROR,    /**< Unavailability of resource */
    IPC_TRANSPORT_STATUS_METHOD_ERROR,      /**< No method available */
    IPC_TRANSPORT_STATUS_BOUNDARY_ERROR,    /**< Out of boundary */
    IPC_TRANSPORT_STATUS_TIMEOUT,           /**< Timeout */
} ipc_transport_status_t;

/** \enum ipc_transport_mode_t
 *  \details
 *  This enum defines the operation mode of transport instance
 */
typedef enum ipc_transport_mode_e {
    TRANSPORT_MODE_SERVER = 0,   /**< Transport instance operates on server side */
    TRANSPORT_MODE_CLIENT = 1,   /**< Transport instance operates on client side */
    TRANSPORT_MODE_LAST 
        = TRANSPORT_MODE_CLIENT, /**< The last valid mode (highest index) */
    TRANSPORT_MODE_COUNT,        /**< The count of transport modes */
} ipc_transport_mode_t;

/* Various integers supported by IPC transport */
typedef unsigned char ipc_transport_byte_t; /*  8 bits */
typedef long ipc_transport_long_t;          /* 32 bits */
typedef __int64 ipc_transport_integer_t;    /* 64 bits integer */

enum {
    STR_BUFFER_SIZE = 128, /* Short strings such as method name */
};

/* IPC Transport implements several base functions. Here is declaration of function types for table of virtual methods */
typedef ipc_transport_status_t ipc_transport_control_func_t(void *); /* Destroy, syncronization, call and process */
typedef ipc_transport_status_t ipc_transport_integer_write_func_t(void *, ipc_transport_integer_t val, int len); /* Write integer */
typedef ipc_transport_status_t ipc_transport_integer_read_func_t(void *, ipc_transport_integer_t * val, int len); /* Read integer */
typedef ipc_transport_status_t ipc_transport_string_write_func_t(void *, const char * val); /* Write string */
typedef ipc_transport_status_t ipc_transport_string_read_func_t(void *, char * val, int len); /* Read string */
typedef ipc_transport_status_t ipc_transport_buffer_write_funct_t(void *, const void * buf, long len); /* Write buffer */
typedef ipc_transport_status_t ipc_transport_buffer_read_funct_t(void *, void * buf, long max_len); /* Read buffer */

/* */
struct ipc_transport_base_s;
typedef ipc_transport_status_t ipc_transport_process_func_t(struct ipc_transport_base_s *, void * data); /* Read buffer */

/** \ struct ipc_transport_base_t
 *  \details
 *  The base transport structure used for transport access virtualization.
 *  Any transport implementation is abstracted from this structure.
 */
typedef struct ipc_transport_base_s {
    /* The IPC transport base virtual methods - pointers to functions */
    ipc_transport_control_func_t * destroy; /* Destroy (destructor) of the transport */
    ipc_transport_control_func_t * acquire; /* Acquire the exclusive access to the transport */
    ipc_transport_control_func_t * release; /* Release the exclusive access to the transport */
    ipc_transport_control_func_t * call; /* Call to another side */
    ipc_transport_integer_write_func_t * integer_write; /* Write integer */
    ipc_transport_integer_read_func_t * integer_read; /* Read integer */
    ipc_transport_string_write_func_t * string_write; /* Write string */
    ipc_transport_string_read_func_t * string_read; /* Read string */
    ipc_transport_buffer_write_funct_t * buffer_write; /* Write buffer */
    ipc_transport_buffer_read_funct_t * buffer_read; /* Read buffer */
} ipc_transport_base_t;

/*****************************************
 *       Function declarations           *
 *****************************************/

/** \fn ipc_transport_status_t ipc_transport_destroy(ipc_transport_base_t * transport);
 *  \details
 *  this function destroys the transport instance
 *  \param transport - the pointer to the transport instance
 *  \return status
 */
ipc_transport_status_t ipc_transport_destroy(ipc_transport_base_t * transport);

/** \fn ipc_transport_status_t ipc_transport_acquire(ipc_transport_base_t * transport);
 *  \details
 *  this function acquires transport for exclusive read/write access
 *  \param transport - the pointer to the transport instance
 *  \return status
 */
ipc_transport_status_t ipc_transport_acquire(ipc_transport_base_t * transport);

/** \fn ipc_transport_status_t ipc_transport_release(ipc_transport_base_t * transport);
 *  \details
 *  this function releases transport so other parties may acquire it
 *  \param transport - the pointer to the transport instance
 *  \return status
 */
ipc_transport_status_t ipc_transport_release(ipc_transport_base_t * transport);

/** \fn ipc_transport_status_t ipc_transport_call(ipc_transport_base_t * transport);
 *  \details
 *  this function implements an inter process call
 *  \param transport - the pointer to the transport instance
 *  \return status
 */
ipc_transport_status_t ipc_transport_call(ipc_transport_base_t * transport);

/** \fn ipc_transport_status_t ipc_transport_integer_write(ipc_transport_base_t * transport, ipc_transport_integer_t val, int len);
 *  \details
 *  this function writes an integer value into the transport
 *  \param transport - the pointer to the transport instance
 *  \param val - integer value (any width up to 64 bits)
 *  \len - the length in bytes to be written
 *  \return status
 */
ipc_transport_status_t ipc_transport_integer_write(ipc_transport_base_t * transport, ipc_transport_integer_t val, int len);

/** \fn ipc_transport_status_t ipc_transport_integer_read(ipc_transport_base_t * transport, ipc_transport_integer_t * val, int len);
 *  \details
 *  this function writes an integer value into the transport
 *  \param transport - the pointer to the transport instance
 *  \param val - pointer integer value (64 bits strongly)
 *  \len - the length in bytes to be read
 *  \return status
 */
ipc_transport_status_t ipc_transport_integer_read(ipc_transport_base_t * transport, ipc_transport_integer_t * val, int len);

/** \fn ipc_transport_status_t ipc_transport_string_write(ipc_transport_base_t * transport, const char * val);
 *  \details
 *  this function writes an integer value into the transport
 *  \param transport - the pointer to the transport instance
 *  \param val - string to be written
 *  \return status
 */
ipc_transport_status_t ipc_transport_string_write(ipc_transport_base_t * transport, const char * val);

/** \fn ipc_transport_status_t ipc_transport_string_read(ipc_transport_base_t * transport, char * val, int len);
 *  \details
 *  this function reads a string value via the transport
 *  \param transport - the pointer to the transport instance
 *  \param val - string buffer to be read to
 *  \param len - length of buffer
 *  \return status
 */
ipc_transport_status_t ipc_transport_string_read(ipc_transport_base_t * transport, char * val, int len);

/** \fn ipc_transport_status_t ipc_transport_buffer_write(ipc_transport_base_t * transport, const void * buf, long len);
 *  \details
 *  this function writes data buffer into transport
 *  \param transport - the pointer to the transport instance
 *  \param buf - pointer to data buffer to be written
 *  \param len - length of buffer
 *  \return status
 */
ipc_transport_status_t ipc_transport_buffer_write(ipc_transport_base_t * transport, const void * buf, long len);

/** \fn ipc_transport_status_t ipc_transport_buffer_read(ipc_transport_base_t * transport, void * buf, long max_len);
 *  \details
 *  this function reads data buffer from transport
 *  \param transport - the pointer to the transport instance
 *  \param buf - pointer to data buffer to be written
 *  \param max_len - max length of buffer can be read
 *  \return status
 */
ipc_transport_status_t ipc_transport_buffer_read(ipc_transport_base_t * transport, void * buf, long max_len);

/** \struct ipc_transport_thread_s
 *  \details platform independent structure for multithreading support
 */
typedef void* ipc_transport_thread_t;

/** \struct ipc_transport_thread_context_s
 *  \details platform independent structure for multithreading support
 */
typedef struct ipc_transport_thread_context_s {
    void (*startroutine)(void *startcontext);
    void *startcontext;
} ipc_transport_thread_context_t;

/** \fn ipc_transport_status_t ipc_transport_thread_init(MUT_THREAD thread,
 *      void (*startroutine)(void *startcontext), void *startcontext)
 *  \param thread - the newly created thread
 *  \param startroutine - function we want to start in separate thread
 *  \param startcontext - pointer which would be given as an argument to
 *  the startroutine
 */
ipc_transport_status_t ipc_transport_thread_init(ipc_transport_thread_t * thread,
    void (*startroutine)(void *startcontext), void *startcontext);

/** \fn MUT_NTSTATUS ipc_transport_thread_wait(void *thread, DWORD timeout)
 *  \param thread - pointer to the ipc_transport_thread_t structure
 *  \param timeout - max time to wait
 *  \return operation status
 */
ipc_transport_status_t ipc_transport_thread_wait(ipc_transport_thread_t *thread,
    unsigned long timeout);

/** \fn void ipc_transport_terminate_thread(void *thread)
 *  \param thread - pointer to the thread structure
 */
ipc_transport_status_t ipc_transport_terminate_thread(ipc_transport_thread_t *thread);

/** \fn ipc_transport_status_t ipc_transport_server_process(ipc_transport_base_t * transport, ipc_transport_process_func_t processor, void * data)
 *  \param transport - the pointer to the transport instance
 *  \param processor - pointer to the function that will process client calls
 *  \param data - user data will be accessible by processor function
 */
ipc_transport_status_t ipc_transport_server_process(ipc_transport_base_t * transport, ipc_transport_process_func_t process, void * data);

/** \fn const char * ipc_transport_string_get(ipc_transport_base_t * transport)
 *  \details
 *  Retrieves string values from the transport
 *  \param transport - the pointer to the transport instance
 *  \return string value
 */
const char * ipc_transport_string_get(ipc_transport_base_t * transport);

/** \fn ipc_transport_integer_t ipc_transport_integer_get(ipc_transport_base_t * transport);
 *  \details
 *  Retrieves integer from the transport
 *  \param transport - the pointer to the transport instance
 *  \return integer value
 */
ipc_transport_integer_t ipc_transport_integer_get(ipc_transport_base_t * transport);

/** \fn void* ipc_transport_pointer_get(ipc_transport_base_t * transport)
 *  \details
 *  Retrieves pointer from the transport
 *  \param transport - the pointer to the transport instance
 *  \return abstract pointer value
 */
void* ipc_transport_pointer_get(ipc_transport_base_t * transport);

/** \fn ipc_transport_integer_t poke_integer(ipc_transport_integer_t address, ipc_transport_integer_t size, ipc_transport_integer_t value);
 *  \details
 *  Stores integer values within simulator's address space
 *  \param transport - the pointer to the transport instance pointer that will be created
 *  \param size - values size in bytes
 *  \param value - integer value
 *  \return status
 */
ipc_transport_integer_t poke_integer(ipc_transport_integer_t address, ipc_transport_integer_t size, ipc_transport_integer_t value);

/** \fn ipc_transport_integer_t peek_integer(ipc_transport_integer_t address, ipc_transport_integer_t size);
 *  \details
 *  Retrieves integer values from simulator's address space
 *  \param transport - the pointer to the transport instance pointer that will be created
 *  \param size - values size in bytes
 *  \return status
 */
ipc_transport_integer_t peek_integer(ipc_transport_integer_t address, ipc_transport_integer_t size, ipc_transport_integer_t* value);

/** \fn ipc_transport_integer_t poke_bitfield(ipc_transport_integer_t address, ipc_transport_integer_t bit_offset, ipc_transport_integer_t bit_length, ipc_transport_integer_t value);
 *  \details
 *  Stores integer values within simulator's address space
 *  \param transport - the pointer to the transport instance pointer that will be created
 *  \bit_offset - offset of the field in bits
 *  \bit_length - length of the field in bits
 *  \param value - integer value
 *  \return status
 */
ipc_transport_integer_t poke_bitfield(ipc_transport_integer_t address, ipc_transport_integer_t bit_offset, ipc_transport_integer_t bit_length, ipc_transport_integer_t value);

/** \fn ipc_transport_integer_t peek_bitfield(ipc_transport_integer_t address, ipc_transport_integer_t bit_offset, ipc_transport_integer_t bit_length, ipc_transport_integer_t* value);
 *  \details
 *  Retrieves integer values from simulator's address space
 *  \param transport - the pointer to the transport instance pointer that will be created
 *  \bit_offset - offset of the field in bits
 *  \bit_length - length of the field in bits
 *  \param value - return value
 *  \return status
 */
ipc_transport_integer_t peek_bitfield(ipc_transport_integer_t address, ipc_transport_integer_t bit_offset, ipc_transport_integer_t bit_length, ipc_transport_integer_t* value);

/** \fn ipc_transport_integer_t block_alloc(ipc_transport_integer_t address, ipc_transport_integer_t size, ipc_transport_integer_t * value);
 *  \details
 *  Retrieves integer values from simulator's address space
 *  \param transport - the pointer to the transport instance pointer that will be created
 *  \param size - size of block to be allocated in bytes
 *  \param value - return value
 *  \return status
 */
ipc_transport_integer_t alloc_block(ipc_transport_integer_t size, ipc_transport_integer_t * value);

#endif /* !defined(_IPC_TRANSPORT_BASE_H_) */
