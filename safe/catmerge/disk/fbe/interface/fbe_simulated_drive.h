#ifndef FBE_SIMULATED_DRIVE_H
#define FBE_SIMULATED_DRIVE_H

#include "fbe/fbe_types.h"
/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_simulated_drive.h
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains the api for the simulated drive access functions.
 *
 ***************************************************************************/

#define SIMULATED_DRIVE_INVALID_SIZE ((simulated_drive_size_t)-1)
#define SIMULATED_DRIVE_INVALID_HANDLE (-1)

#define SIMULATED_DRIVE_DEFAULT_SERVER_NAME "127.0.0.1"
#define SIMULATED_DRIVE_DEFAULT_PORT_NUMBER 2717

#define SIMULATED_DRIVE_MAX_NUM 960
#define SIMULATED_DRIVE_IDENTITY_LENGTH 20

#define SIMULATED_DRIVE_TRUE 1
#define SIMULATED_DRIVE_FALSE 0

#define FBE_SIMULATED_DRIVE_MAX_NUMBER_OF_DRIVES 1024

typedef fbe_u8_t simulated_drive_identity_t[SIMULATED_DRIVE_IDENTITY_LENGTH];
typedef fbe_u64_t simulated_drive_handle_t;
typedef unsigned short simulated_drive_port_t;

/* 
 * Must be called when the Virtual Drive Simulation starting up. 
 * It will open the virtual drive file and build all header index and block index. 
 *  
 * Parameters: 
 * serverName: server name; 
 * port: server port. 
 * sp: 0 - SPA, 1 - SPB 
 * Return: 
 * TRUE if succeeds, FALSE if fails. 
 */
fbe_bool_t fbe_simulated_drive_init(const fbe_u8_t * server_name, fbe_u16_t port, fbe_u32_t sp);

/*
 * Get the process id of the drive simulation server
 *  
 * Return: 
 * PID of drive simulation server
 */
fbe_u64_t fbe_simulated_drive_get_server_pid(void);

/*
 * Must be called before the Virtual Drive Simulation shutting down.
 * Close the virtual drive file and clean up all indices.
 *  
 * Return: 
 * TRUE if succeeds, FALSE if fails. 
 */
fbe_bool_t fbe_simulated_drive_cleanup(void);

/*
 * Create a new simulated drive handle for an EXISTING drive. 
 *  
 * Parameters: 
 * drive_identity: drive identity.
 *  
 * Return: 
 * Simulated drive handle if succeeds, SIMULATED_DRIVE_INVALID_HANDLE if no such drive existing. 
 */
simulated_drive_handle_t simulated_drive_open(simulated_drive_identity_t drive_identity);

/*
 * Close the simulated drive handle. 
 *  
 * Parameters: 
 * handle: simulated drive handle, should be returned by simulated_drive_open().
 *  
 * Return: 
 * TRUE if succeeds, FALSE if fails.
 */
fbe_bool_t simulated_drive_close(simulated_drive_handle_t handle);

/*
 * Create a new simulated drive handle for an NEW drive. 
 * Note that creation for an existing drive will lose all the current data. 
 * The recommended practice is trying to open the drive first, create it if opening fails.
 *  
 * Parameters: 
 * drive_identity: drive identity.
 * drive_block_size: size of the drive block. 
 * max_lba: number of the max lba. 
 *  
 * Return: 
 * Simulated drive handle if succeeds, SIMULATED_DRIVE_INVALID_HANDLE if fails. 
 */
simulated_drive_handle_t simulated_drive_create(simulated_drive_identity_t drive_identity, fbe_u32_t drive_block_size, fbe_lba_t max_lba);

/*
 * Remove an existing simulated drive entry.
 * Note that all data of the the drive will be lost.
 *
 * Parameters:
 * drive_identity: drive identity.
 *
 * Return:
 * TRUE if succeeds, FALSE if fails.
 */
fbe_bool_t simulated_drive_remove(simulated_drive_identity_t drive_identity);

/*
 * Remove all simulated drives.
 *
 * Return:
 * TRUE if succeeds, FALSE if fails.
 */
fbe_bool_t simulated_drive_remove_all(void);


/*
 * Read nblocks_to_read drive blocks data into the buffer. 
 *  
 * Parameters: 
 * handle: simulated drive handle, should be returned by simulated_drive_open() or simulated_drive_create().
 * buffer: buffer to store the result. 
 * nbytes_to_read: number of bytes to be read. 
 *  
 * Return: 
 * Bytes of read data if succeeds, else SIMULATED_DRIVE_ERROR.
 */
fbe_u32_t simulated_drive_read(simulated_drive_identity_t drive_identity,
										  void *buffer, 
										  fbe_lba_t lba, 
										  fbe_u32_t nbytes_to_read,
										  void * context);

/*
 * Write nblocks_to_write drive blocks data to the drive from the buffer. 
 *  
 * Parameters: 
 * handle: simulated drive handle, should be returned by simulated_drive_open() or simulated_drive_create().
 * buffer: buffer of data to be written. 
 * nbytes_to_write: number of bytes to be written. 
 *  
 * Return: 
 * Bytes of written data if succeeds, else SIMULATED_DRIVE_ERROR.
 */
fbe_u32_t simulated_drive_write(simulated_drive_identity_t drive_identity,
										   void *buffer, 
										   fbe_lba_t lba, 
										   fbe_u32_t nbytes_to_write,
										   void * context);

/*
 * Write nblocks_to_write drive blocks data repeat_count times to the drive from the buffer.
 *
 * Parameters:
 * handle: simulated drive handle, should be returned by simulated_drive_open() or simulated_drive_create().
 * buffer: buffer of data to be written.
 * nbytes_to_write: number of bytes to be written.
 * repeat_count: repeat count.
 *
 * Return:
 * Bytes of written data if succeeds, else SIMULATED_DRIVE_ERROR.
 */
fbe_u32_t simulated_drive_write_same(simulated_drive_identity_t drive_identity,
												void *buffer, 
												fbe_lba_t lba, 
												fbe_u32_t nbytes_to_write,
												fbe_u32_t repeat_count,
												void * context);

#endif  // FBE_SIMULATED_DRIVE_H
