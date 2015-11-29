#ifndef __SIMULATED_DRIVE_H__
#define __SIMULATED_DRIVE_H__

#include "k10ntddk.h"

#define SIMULATED_DRIVE_INVALID_SIZE (csx_size_t)-1
#define SIMULATED_DRIVE_INVALID_HANDLE -1

#define SIMULATED_DRIVE_DEFAULT_SERVER_NAME "127.0.0.1"
#define SIMULATED_DRIVE_DEFAULT_PORT_NUMBER 2717

#define SIMULATED_DRIVE_MAX_NUM 960
#define SIMULATED_DRIVE_IDENTITY_LENGTH 20

typedef unsigned long long    simulated_drive_u64_t;
typedef DWORD                 simulated_drive_u32_t;
typedef int                   simulated_drive_bool;
typedef simulated_drive_u64_t simulated_drive_size;
typedef char                  simulated_drive_byte;
typedef simulated_drive_size  simulated_drive_location;
typedef simulated_drive_byte  simulated_drive_identity[SIMULATED_DRIVE_IDENTITY_LENGTH];
typedef int                   simulated_drive_handle;

typedef enum
{
    SIMULATED_DRIVE_MODE_PERMANENT,
    SIMULATED_DRIVE_MODE_TEMPORARY // in temporary mode the server will clear all drives when the first client connects and the last client disconnects.
} simulated_drive_mode_e;

typedef enum
{
    SIMULATED_DRIVE_BACKEND_TYPE_MEMORY,
    SIMULATED_DRIVE_BACKEND_TYPE_FILE
} simulated_drive_backend_type_e;

typedef struct simulated_drive_s
{
    simulated_drive_identity drive_identity;
    simulated_drive_size max_lba;
    simulated_drive_size drive_block_size;
} simulated_drive_t;

typedef struct simulated_drive_list_s
{
    simulated_drive_size size;
    simulated_drive_t drives[SIMULATED_DRIVE_MAX_NUM];
} simulated_drive_list_t;

/* 
 * Must be called when the Virtual Drive Simulation starting up. 
 * It will open the virtual drive file and build all header index and block index. 
 *  
 * Parameters: 
 * serverName: server name; 
 * port: server port. 
 *  
 * Return: 
 * TRUE if succeeds, FALSE if fails. 
 */
simulated_drive_bool simulated_drive_init(const char* server_name, unsigned short port);

/*
 * Must be called before the Virtual Drive Simulation shutting down.
 * Close the virtual drive file and clean up all indices.
 *  
 * Return: 
 * TRUE if succeeds, FALSE if fails. 
 */
simulated_drive_bool simulated_drive_cleanup(void);

/* 
 * Get a list of drives existing in the Simulated Drive even when no handle is opened. 
 *  
 * Return: 
 * List of drives.
 */ 
simulated_drive_list_t simulated_drive_get_drive_list(void);

/*
 * Get process ID of Simulated Drive server.
 *
 * Return:
 * Disk server PID.
 */
simulated_drive_u32_t simulated_drive_get_server_pid(void);

/*
 * Create a new simulated drive handle for an EXISTING drive. 
 *  
 * Parameters: 
 * drive_identity: drive identity.
 *  
 * Return: 
 * Simulated drive handle if succeeds, SIMULATED_DRIVE_INVALID_HANDLE if no such drive existing. 
 */
simulated_drive_handle simulated_drive_open(simulated_drive_identity drive_identity);

/*
 * Close the simulated drive handle. 
 *  
 * Parameters: 
 * handle: simulated drive handle, should be returned by simulated_drive_open().
 *  
 * Return: 
 * TRUE if succeeds, FALSE if fails.
 */
simulated_drive_bool simulated_drive_close(simulated_drive_handle handle);

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
simulated_drive_handle simulated_drive_create(simulated_drive_identity drive_identity, simulated_drive_size drive_block_size, simulated_drive_size max_lba);

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
simulated_drive_bool simulated_drive_remove(simulated_drive_identity drive_identity);

/*
 * Remove all simulated drives.
 *
 * Return:
 * TRUE if succeeds, FALSE if fails.
 */
simulated_drive_bool simulated_drive_remove_all(void);

/*
 * Set the capacity of the simulated drive FILE(MB). 
 * the defaut value is 4 * 1024 MB = 4GB.
 *                                                 
 * Parameters:                                                
 * capacity: max size of the file. 
 *  
 * Return: 
 * TRUE if succeeds, FASLE if the capacity is invalid(<= 0). 
 */
simulated_drive_bool simulated_drive_set_total_capacity(double capacity);

/*
 * Set the mode of the simulated drive.
 *
 * Parameters:
 * mode: Simulated Drive server mode:
 *           SIMULATED_DRIVE_MODE_PERMANENT: data will be kept in file system permanently.
 *           SIMULATED_DRIVE_MODE_TEMPORARY: all drives will be cleared when first client connects or last client disconnects.
 *
 */
void simulated_drive_set_mode(simulated_drive_mode_e mode);

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
simulated_drive_size simulated_drive_read(simulated_drive_handle handle, void *buffer, simulated_drive_location lba, simulated_drive_size nbytes_to_read);

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
simulated_drive_size simulated_drive_write(simulated_drive_handle handle, void *buffer, simulated_drive_location lba, simulated_drive_size nbytes_to_write);

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
simulated_drive_size simulated_drive_write_same(simulated_drive_handle handle, void *buffer, simulated_drive_location lba, simulated_drive_size nbytes_to_write, simulated_drive_size repeat_count);

#endif  /* __SIMULATED_DRIVE_H__ */
