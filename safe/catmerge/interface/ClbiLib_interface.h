/***************************************************************************
 *  Copyright (C)  EMC Corporation 1992-2010
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. 
 ***************************************************************************/

/***************************************************************************
 * ClbiLib_interface.h
 ***************************************************************************
 *
 * File Description:
 *  Declarations for functions exported by CLBI library.
 * 
 * Author:
 *  Ashwin Tidke
 *  
 * Revision History:
 *  November 6 , 2008 - ART - Created Inital Version
 *
 ***************************************************************************/

#ifndef __CLBI_LIB_INTERFACE_H_INCLUDE_
#define __CLBI_LIB_INTERFACE_H_INCLUDE_

#include "ClbiLib_types.h"

/* CLBI Library API
 */
/*
 * ClbiInitalize():
 *  This function performs all the intialization required for CLBI library. 
 *  This includes:
 *  1. Checking existence and validity of data areas used to store the Stable 
 *     Values (SVs) and the instance name. If these data areas do not exist 
 *     or contain invalid data, they would be created/recreated.
 *  2. Initialize the Critical Section object used to synchronize access 
 *     to global AMS variables.
 *  3. Perform the initial registration of Lockbox Plugin Interface (LPI) with
 *     the CST lockbox library.
 *
 * Notes:
 *  Any application that intends to use the CLBI Library must call this 
 *  function first, to initialize the library. If successful it must 
 *  not be called again.
 *
 * Arguments: 
 *  None
 *
 * Return value: 
 *  0 on success.
 *  non-zero on failure.
 */
CLBI_API CLBI_STATUS ClbiInitalize (void);

/*
 * ClbiCreate():
 *  Create a new lockbox, if a revert is not possible.
 *
 * Arguments:
 *  LockboxName     [in]:   Null terminated string to identify the lockbox.
 *                          Must not exceed MAX_LB_NAME_LENGTH (including the 
 *                          null terminator).
 *                          The name must include the name of the 
 *                          feature/process that owns the lockbox.
 *  Overwrite       [in]:   true: overwrite an existing lockbox.
 *                          false: if the specified lockbox already exists, 
 *                          return error.
 *  LockboxHandle   [out]:  On success this will contain the handle to open 
 *                          lockbox.
 *
 * Return:  
 *  CLBI_SUCCESS[0]                 :   Success
 *  CLBI_ERR_INVALID_PARAMETER      :   One or more argument(s) passed to this 
 *                                      function was invalid.
 *  CLBI_ERR_COMMIT_REQUIRED        :   The current bundle needs to be committed.
 *  CLBI_ERR_ALREADY_EXISTS         :   The overwrite argument was clear and a 
 *                                      lockbox with given name already exists.
 */
CLBI_API CLBI_STATUS ClbiCreate(const char* LockboxName, 
                int Overwrite, 
                lbHandle* LockboxHandle);

/*
 * ClbiOpen():
 *  Open an existing lockbox.
 *
 * Arguments:
 *  LockboxName     [in]:   Null terminated string to identify the lockbox.
 *                          Must not exceed MAX_LB_NAME_LENGTH (including the 
 *                          null terminator).
 *  LockboxHandle   [out]:  On success this will contain the handle to open 
 *                          lockbox.
 *
 * Return:  
 *  CLBI_SUCCESS[0]                 :   Success
 *  CLBI_ERR_INVALID_PARAMETER      :   One or more argument(s) passed to this 
 *                                      function was invalid.
 *  CLBI_ERR_LB_FILE_NOT_FOUND      :   The specified lockbox does not exist.
 */
CLBI_API CLBI_STATUS ClbiOpen(const char* LockboxName, 
            lbHandle* LockboxHandle);

/*
 * ClbiRetrieveItem():
 *  Retrieve a single item from the lockbox.
 *
 * Arguments:
 *  LockboxHandle   [in]:   Handle to an open lockbox. 
 *                          A value obtained by calling ClbiCreate()/ClbiOpen().
 *  ItemName        [in]:   Null terminated string to identify the item to be 
 *                          retrieved. Must not exceed MAX_ITEM_NAME_LENGTH 
 *                          including Null terminator.
 *  ItemType        [in]:   Binary (Item may contain Null character) 
 *                          Text   (Item is Null terminated string)
 *                          It is the application's responsibility to keep
 *                          track of the item's type. The value would be returned
 *                          in the requested form without an error.
 *  Item            [out]:  Address of pointer to the buffer containing the 
 *                          retrieved item. Memory for the buffer will be 
 *                          allocated by CLBI. Must call ClbiFree() to free 
 *                          the buffer.
 *  ItemLen         [out]:  Length of the item buffer in bytes.
 *  AccessPsm       [in]:   true (default argument): Item would be retreived 
 *                          from PSM copy of lockbox. 
 *                          false: Item would be retrieved from in-memory 
 *                          copy of lockbox. Use only if sure that the there 
 *                          have been no updates to the lockbox since the
 *                          lockbox was opened or the last PSM read.
 *                          
 * Return:  
 *  CLBI_SUCCESS[0]                 :   Success
 *  CLBI_ERR_INVALID_PARAMETER      :   One or more argument(s) passed to this 
 *                                      function was invalid.
 *  CLBI_ERR_INVALID_HANDLE       :     The specified handle does not match 
 *                                      any open lockbox.
 *  CLBI_ERR_ITEM_NOT_FOUND         :   The specified item was not found in 
 *                                      the lockbox.
 *  CLBI_ERR_MEM_ALLOC_FAILED       :   Memory could not be allocated for 
 *                                      item buffer.
 */
CLBI_API CLBI_STATUS ClbiRetrieveItem(lbHandle LockboxHandle, 
                     const char* ItemName, 
                     CLBI_ITEM_TYPE ItemType, 
                     unsigned char** Item, 
                     unsigned int* ItemLen,
                     bool AccessPsm = true);

/*
 * ClbiFree():
 *  Shreds and frees a buffer allocated by CST Lockbox library during a call to
 *  ClbiRetrieveItem().
 *
 * Arguments:
 *  Item            [in]:   Pointer to the buffer to be freed.
 *  ItemLen         [in]:   Length of the buffer.
 *
 * Return:  
 *  None
 */
CLBI_API void ClbiFree(unsigned char* Item, 
             unsigned int ItemLen);

/*
 * ClbiStoreItem():
 *  Stores a single item to the lockbox, if a revert is not possible.
 *
 * Arguments:
 *  LockboxHandle   [in]:   Handle to an open lockbox.
 *                          A value obtained by calling ClbiCreate()/ClbiOpen()
 *  ItemName        [in]:   Null terminated string to identify the item to be 
 *                          retrieved. Must not exceed MAX_ITEM_NAME_LENGTH 
 *                          including Null terminator.
 *  ItemType        [in]:   Binary (Item may contain Null character) 
 *                          Text   (Item is Null terminated string)
 *                          It is the application's responsibility to keep
 *                          track of the item's type.
 *  Item            [in]:   Pointer to the buffer containing the item to be 
 *                          stored.
 *  ItemLen         [in]:   Length of the item buffer. Must not exceed 
 *                          MAX_ITEM_LENGTH. Includes Null terminator for 
 *                          text items.
 *
 * Return:  
 *  CLBI_SUCCESS[0]                 :   Success
 *  CLBI_ERR_INVALID_PARAMETER      :   One or more argument(s) passed to this 
 *                                      function was invalid.
 *  CLBI_ERR_INVALID_HANDLE       :     The specified handle does not match 
 *                                      any open lockbox.
 *  CLBI_ERR_COMMIT_REQUIRED        :   The current bundle needs to be committed.
 */
CLBI_API CLBI_STATUS ClbiStoreItem(lbHandle LockboxHandle, 
                  const char* ItemName, 
                  CLBI_ITEM_TYPE ItemType, 
                  const unsigned char* Item, 
                  const unsigned int ItemLen);

/*
 * ClbiRemoveItem():
 *  Deletes a single item from the lockbox, if a revert is not possible.
 *
 * Arguments:
 *  LockboxHandle   [in]:   Handle to an open lockbox. 
 *                          A value obtained by calling ClbiCreate()/ClbiOpen().
 *  ItemName        [in]:   Null terminated string to identify the item to be 
 *                          retrieved. Must not exceed MAX_ITEM_NAME_LENGTH 
 *                          including Null terminator.
 *  
 * Return:  
 *  CLBI_SUCCESS[0]                 :   Success
 *  CLBI_ERR_INVALID_PARAMETER      :   One or more argument(s) passed to this 
 *                                      function was invalid.
 *  CLBI_ERR_INVALID_HANDLE       :     The specified handle does not match 
 *                                      any open lockbox.
 *  CLBI_ERR_ITEM_NOT_FOUND         :   The specified item was not found in 
 *                                      the lockbox.
 *  CLBI_ERR_COMMIT_REQUIRED        :   The current bundle needs to be committed.
 */
CLBI_API CLBI_STATUS ClbiRemoveItem(lbHandle LockboxHandle,
                   const char* ItemName);

/*
 * ClbiStoreRemoveItems():
 *  Updates the lockbox with a given set of items in a transactional manner.
 *  The persistent copy of lockbox is updated with either all elements in the 
 *  set or none. A DLS lock is held by the calling thread for the entire 
 *  duration of this call. Hence, this should be used only for items with 
 *  above requirement. For single item store or remove use 
 *  ClbiStore() and ClbiRemove(). It is the application's responsibility to keep
 *  track of item types.
 *
 * Arguments:
 *  LockboxHandle   [in]:   Handle to an open lockbox. 
 *                          A value obtained by calling ClbiCreate()/ClbiOpen().
 *  ItemArray       [in]:   An array of structures of type CLBI_ITEM, that 
 *                          describes each item and the operation to be 
 *                          performed on it.
 *  ItemCount       [in]:   Number elements in ItemArray. Must not be zero.
 *
 * Return:  
 *  CLBI_SUCCESS[0]                 :   Success
 *  CLBI_ERR_INVALID_PARAMETER      :   One or more argument(s) passed to this 
 *                                      function was invalid.
 *  CLBI_ERR_INVALID_HANDLE       :     The specified handle does not match 
 *                                      any open lockbox.
 *  CLBI_ERR_COMMIT_REQUIRED        :   The current bundle needs to be committed.
 */
CLBI_API CLBI_STATUS ClbiStoreRemoveItems(lbHandle LockboxHandle,
                        CLBI_ITEM* ItemArray,
                        unsigned int ItemCount);

/*
 * ClbiClose():
 *  Closes an open lockbox. 
 *  The handle should not be used any further after returning from this call.
 *
 * Arguments:
 *  LockboxHandle   [in]:   Handle to an open lockbox. 
 *                          A value obtained by calling ClbiCreate()/ClbiOpen()
 *  
 * Return:  
 *  CLBI_SUCCESS[0]                 :   Success
 *  CLBI_ERR_INVALID_PARAMETER      :   One or more argument(s) passed to this 
 *                                      function was invalid.
 *  CLBI_ERR_INVALID_HANDLE       :     The specified handle does not match any 
 *                                      open lockbox.
 */
CLBI_API CLBI_STATUS ClbiClose(lbHandle LockboxHandle);

/*
 * ClbiGetErrorMessage():
 *  Get Clbi's error message for the error code returned in one of the APIs 
 *  above.
 *  
 * Arguments:
 *  ErrorCode       [in]:   The error code returned by one of the Clbi 
 *                          functions.
 *  ErrorMessage    [out]:  A Null terminated string describing the above 
 *                          error code.
 *                          The memory for the message will be allocated by 
 *                          CLBI. NULL if memory allocation fails.
 *                          The client needs to call ClbiFreeErrorMessage() 
 *                          to free this memory.
 *
 * Return:
 *  None
 */
CLBI_API void ClbiGetErrorMessage(CLBI_STATUS ErrorCode,
                         char** ErrorMessage);

/* ClbiFreeErrorMessage():
 *  Shreds and frees a buffer allocated by CLBI during a call to 
 *  ClbiGetErrorMessage().
 *
 * Arguments:
 *  ErrorMessage    [in]:   Pointer to a Nul terminated string to be freed.
 *
 * Return:  
 *  None
 */
CLBI_API void ClbiFreeErrorMessage(char* ErrorMessage);

/* ClbiUninitialize():
 *  Releases all resources allocated by the CLBI library for the process.
 *  Closes all lockboxes opened by the process and deletes 
 *  the global critical sections.
 *  Once this function is called, the process must call ClbiInitialize() before
 *  using the library again.
 *
 * Arguments:
 *  None
 *
 * Return:
 *  None.
 */
CLBI_API void ClbiUninitialize(void);


/* ClbiGetLBFileName():
 *  Provides the file name of lock box file in PSM.
 *
 * Arguments:
 *  LockBoxName     [in]:  Lock box name.
 *  LockBoxFileName [out]: Lock box file name.
 *
 * Return:
 *  CLBI_STATUS - Status code.
 */
CLBI_API CLBI_STATUS ClbiGetLBFileName(const char *LockBoxName,
                                       char *LockBoxFileName);

#endif