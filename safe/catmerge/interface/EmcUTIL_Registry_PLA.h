#ifndef EMCUTIL_REGISTRY_PLA_H_
#define EMCUTIL_REGISTRY_PLA_H_

//***************************************************************************
// Copyright (C) EMC Corporation 2011
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

/*! @file  EmcUTIL_Registry_PLA.h
 *
 */
// Header file for the utility Registry-like APIs - PAL Look-Alike (PLA) variants.

#include "EmcUTIL.h"

CSX_CDECLS_BEGIN

/*! @addtogroup emcutil_registry_pla
 *  @{
 *
 * @brief
 *
 * =====================================================================================\n
 * <b>Registry GET routines: EmcutilGetRegistryValueAs...()</b> where ... = String, MultiStr, Uint32, Uint64, Binary\n
 * =====================================================================================\n
 *
 * Description of the GET Routines: 
 * There is a "Get" function to read the types of values found in the Windows Registry:\n
 * - EmcutilGetRegistryValueAsString()
 * - EmcutilGetRegistryValueAsMultiStr()
 * - EmcutilGetRegistryValueAsUint32()
 * - EmcutilGetRegistryValueAsUint64()
 * - EmcutilGetRegistryValueAsBinary()
 *
 * In addition, there are versions for the same five value types that return a
 * default if the value is missing from the registry:\n
 * - EmcutilGetRegistryValueAsStringDefault()
 * - EmcutilGetRegistryValueAsMultiStrDefault()
 * - EmcutilGetRegistryValueAsUint32Default()
 * - EmcutilGetRegistryValueAsUint64Default()
 * - EmcutilGetRegistryValueAsBinaryDefault()
 *
 * Key assumptions:
 * -# The input path parameter is the full path to the key or parameter in question
 *     E.g., for Windows Services keys the first part of the path would be\n
 *		\ Registry \ Machine \ System \ CurrentControlSet \ Services\n
 *		Note that the leading "\" seems to be required.\n
 * -# The caller knows the value "type" when calling a Get or Set function.\n
 * -# No Default value is entered into a value with a Set routine.\n
 * -# In the EmcutilGetRegistryValueAsString and EmcutilGetRegistryValueAsMultiStr cases
 *     the user must call EmcutilFreeRegistryMem() to free the buffer that is returned.
 *
 * Arguments Input to these functions:
 * - pRegPath       Pointer to the full path (see assumption # 1 above).
 * - pParmName      Pointer to the Name (ID) of the parameter.
 * - xRet...		   Caller defined pointer to a ui32, ui64, char-string, or binary-string
 * - xxxLen         This parameter is used as input for the EmcutilGetRegistryValueAsBinary
 *				   function to specify the length of the caller defined storage space
 *                 This input parameter is required for EmcutilGetRegistryValueAsBinary
 * - Default...     This parameter is used to specify the default that should be returned
 *                 if the value is not found.
 * - defLen         The length in bytes of the default value (including the NUL terminator).
 *
 * Argument Output from these functions:
 * - xRet...		   Buffer or UINTxx containing the return value.
 *
 * ADDITIONAL RETURN VALUES IN THE EmcutilGetRegistryValueAsString,
 *                                 EmcutilGetRegistryValueAsMultiStr, and
 *                                 EmcutilGetRegistryValueAsBinary FUNCTIONS:
 * - xxxLen         The length of the xRet buffer including the NUL terminator
 *                 This output parameter is optional for EmcutilGetRegistryValueAsString and
 *                 EmcutilGetRegistryValueAsMultiStr, it may be specified as NULL
 *
 * Return Value: EMCUTIL_STATUS code
 *
 *
 * =====================================================================================\n
 * <b>Registry SET routines: EmcutilSetRegistryValueAs...()</b> where ... = String, MultiStr, Uint32, Uint64, Binary\n
 * =====================================================================================\n
 *
 * Description of the four SET Routines:
 * There is a "Set" routine to write the types of values found in the Windows Registry:
 * - EmcutilSetRegistryValueAsString()
 * - EmcutilSetRegistryValueAsMultiStr()
 * - EmcutilSetRegistryValueAsUint32()
 * - EmcutilSetRegistryValueAsUint64()
 * - EmcutilSetRegistryValueAsBinary()
 *
 * Key assumptions:
 * -# The input path parameter is the full path to the key or parameter in question
 *     E.g., for Windows Services keys the first part of the path would be\n
 *		\ Registry \ Machine \ System \ CurrentControlSet \ Services\n
 *		Note that the leading "\" seems to be required.
 * -# The caller knows the value "type" when calling a Set function (ie, AsString, AsUint32, etc.).
 * -# No Default value is entered into a value with a Set routine.
 *
 * Arguments Input to these functions:
 * - pRegPath       Pointer to the absolute path value of the Key entry.
 * - pParmName      Pointer to the Name (ID) of the value.
 * - xSet...		   Pointer to the string or value to be Set into the Registry
 * - xxxLen         Length of the char or binary string in xSet...
 *                 the length INCLUDES the terminating null characters, one
 *                 for strings, 2 for multiStrings
 *
 * Return Value: EMCUTIL_STATUS code
 *
 **************************************************/


/*!
 * @brief Get registry string value
 * @param pRegPath Pointer to the full path of registry key
 * @param pParmName Value name
 * @param ppRetString [OUT] Pointer to returned value (string). Must be freed using EmcutilFreeRegistryMem()
 * @param pStringLen [OUT] Optional pointer to lenght of returned string incl. terminating NUL
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilGetRegistryValueAsString(
    csx_cstring_t pRegPath,
    csx_cstring_t pParmName,
    csx_string_t * ppRetString,
    csx_wulong_t * pStringLen);

/*!
 * @brief Get registry multistring value
 * @param pRegPath Pointer to the full path of registry key
 * @param pParmName Value name
 * @param ppRetString [OUT] Pointer to returned value (multistring). Must be freed using EmcutilFreeRegistryMem()
 * @param pStringLen [OUT] Optional pointer to lenght of returned string incl. terminating NUL
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilGetRegistryValueAsMultiStr(
    csx_cstring_t pRegPath,
    csx_cstring_t pParmName,
    csx_string_t * ppRetString,
    csx_wulong_t * pStringLen);

/*!
 * @brief Get registry 32-bit value
 * @param pRegPath Pointer to the full path of registry key
 * @param pParmName Value name
 * @param pRetValue [OUT] Pointer to retyrned 32-bit value
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilGetRegistryValueAsUint32(
    csx_cstring_t pRegPath,
    csx_cstring_t pParmName,
    csx_u32_t * pRetValue);

/*!
 * @brief Get registry 64-bit value
 * @param pRegPath Pointer to the full path of registry key
 * @param pParmName Value name
 * @param pRetValue [OUT] Pointer to retyrned 64-bit value
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilGetRegistryValueAsUint64(
    csx_cstring_t pRegPath,
    csx_cstring_t pParmName,
    csx_u64_t * pRetValue);

/*!
 * @brief Get registry binary blob value
 * @param pRegPath Pointer to the full path of registry key
 * @param pParmName Value name
 * @param pBinBuffer Pointer to buffer where the binary blob is returned
 * @param pBinLen [OUT] Optional pointer to lenght of returned binary blob
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilGetRegistryValueAsBinary(
    csx_cstring_t pRegPath,
    csx_cstring_t pParmName,
    csx_u8_t * pBinBuffer,
    csx_wulong_t * pBinLen);

/*!
 * @brief Get registry string value, providing default string if target the value not found.
 * @param pRegPath Pointer to the full path of registry key
 * @param pParmName Value name
 * @param ppRetString [OUT] Pointer to returned value (string). Must be freed using EmcutilFreeRegistryMem()
 * @param pStringLen [OUT] Optional pointer to lenght of returned string incl. terminating NUL
 * @param pDefaultStr Default string value
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilGetRegistryValueAsStringDefault(
    csx_cstring_t pRegPath,
    csx_cstring_t pParmName,
    csx_string_t * ppRetString,
    csx_wulong_t * pStringLen,
    csx_cstring_t pDefaultStr);

/*!
 * @brief Get registry multistring value, providing default multistring if target the value not found.
 * @param pRegPath Pointer to the full path of registry key
 * @param pParmName Value name
 * @param ppRetString [OUT] Pointer to returned value (multistring). Must be freed using EmcutilFreeRegistryMem()
 * @param pStringLen [OUT] Optional pointer to lenght of returned string incl. terminating NUL
 * @param pDefaultStr Default multistring value
 * @param defLen Length of default multistring
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilGetRegistryValueAsMultiStrDefault(
    csx_cstring_t pRegPath,
    csx_cstring_t pParmName,
    csx_string_t * ppRetString,
    csx_wulong_t * pStringLen,
    csx_cstring_t pDefaultStr,
    csx_wulong_t defLen);

/*!
 * @brief Get registry 32-bit value, providing default 32-bit value if target the value not found.
 * @param pRegPath Pointer to the full path of registry key
 * @param pParmName Value name
 * @param pReturnValue [OUT] Pointer to returned 32-bit value
 * @param defaultValue Default 32-bit value
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilGetRegistryValueAsUint32Default(
    csx_cstring_t pRegPath,
    csx_cstring_t pParmName,
    csx_u32_t * pReturnValue,
    csx_u32_t defaultValue);

/*!
 * @brief Get registry 64-bit value, providing default 64-bit value if target the value not found.
 * @param pRegPath Pointer to the full path of registry key
 * @param pParmName Value name
 * @param pReturnValue [OUT] Pointer to returned 64-bit value
 * @param defaultValue Default 64-bit value
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilGetRegistryValueAsUint64Default(
    csx_cstring_t pRegPath,
    csx_cstring_t pParmName,
    csx_u64_t * pReturnValue,
    csx_u64_t defaultValue);

/*!
 * @brief Get registry binary blob value, providing default binary blob value if target the value not found.
 * @param pRegPath Pointer to the full path of registry key
 * @param pParmName Value name
 * @param pBinBuffer Pointer to buffer where the binary blob is returned
 * @param pBinLen [OUT] Optional pointer to lenght of returned binary blob
 * @param pDefault Default binary blob
 * @param defLen Size of default binary blob in bytes
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilGetRegistryValueAsBinaryDefault(
    csx_cstring_t pRegPath,
    csx_cstring_t pParmName,
    csx_u8_t * pBinBuffer,
    csx_wulong_t * pBinLen,
    csx_u8_t * pDefault,
    csx_wulong_t defLen);

/*!
 * @brief Set registry string value
 * @param pRegPath Pointer to the full path of registry key
 * @param pParmName Value name
 * @param pSetString String value to be set
 * @param stringLen Length of string value to be set (must include terminating NUL)
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilSetRegistryValueAsString(
    csx_cstring_t pRegPath,
    csx_cstring_t pParmName,
    csx_cstring_t pSetString,
    csx_wulong_t stringLen);

/*!
 * @brief Set registry multistring value
 * @param pRegPath Pointer to the full path of registry key
 * @param pParmName Value name
 * @param pSetString Multistring value to be set
 * @param stringLen Length of multistring value to be set (must include terminating NUL)
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilSetRegistryValueAsMultiStr(
    csx_cstring_t pRegPath,
    csx_cstring_t pParmName,
    csx_cstring_t pSetString,
    csx_wulong_t stringLen);

/*!
 * @brief Set registry 32-bit value
 * @param pRegPath Pointer to the full path of registry key
 * @param pParmName Value name
 * @param setValue 32-bit value to be set
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilSetRegistryValueAsUint32(
    csx_cstring_t pRegPath,
    csx_cstring_t pParmName,
    csx_u32_t setValue);

/*!
 * @brief Set registry 64-bit value
 * @param pRegPath Pointer to the full path of registry key
 * @param pParmName Value name
 * @param setValue 64-bit value to be set
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilSetRegistryValueAsUint64(
    csx_cstring_t pRegPath,
    csx_cstring_t pParmName,
    csx_u64_t setValue);

/*!
 * @brief Set registry binary blob value
 * @param pRegPath Pointer to the full path of registry key
 * @param pParmName Value name
 * @param pSetBinStr Pointer to binary blob to be set
 * @param setBinBytes Size of binary blob in bytes
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilSetRegistryValueAsBinary(
    csx_cstring_t pRegPath,
    csx_cstring_t pParmName,
    const csx_u8_t * pSetBinStr,
    csx_wulong_t setBinBytes);

/*!
 * @brief Routine to free the area that contains the returned string which was created from
 *        within an Emcutil function.
 * @remark Specifically for use with EmcutilGetRegistryValueAsString,
 *          EmcutilGetRegistryValueAsMultiStr, and EmcutilRegistryEnumerateKey
 *          functions which return a buffer.
 * @param p Pointer to memory to be released
 * @return No value returned
 */
EMCUTIL_API csx_void_t EMCUTIL_CC EmcutilFreeRegistryMem(
    csx_pvoid_t p);

/*!
 * @brief Routine to allocate from the same pool that EmcutilFreeRegistryMem frees to.
 *        For convenience of users.
 * @param n Size to be allocated
 * @return Pointer to allocated memory
 */
EMCUTIL_API csx_pvoid_t EMCUTIL_CC EmcutilAllocRegistryMem(
    EMCUTIL_SIZE_T n);

/*!
 * @brief This function deletes a value from the OS's Registry.
 * @param pRegPath Pointer to the full path of registry key
 * @param pParmName Value name
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilDeleteRegistryValue(
    csx_cstring_t pRegPath,
    csx_cstring_t pParmName);

/*!
 * @brief Check if the registy key exists
 * @param pRegPath Pointer to the full path of registry key
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if the key exists, EMCUTIL_STATUS_NAME_NOT_FOUND otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilCheckRegistryKey(
    csx_cstring_t pRegPath);

/*!
 * @brief Create registry key
 * @param pRegPath Pointer to the full path of registry key to be created
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilCreateRegistryKey(
    csx_cstring_t pRegPath);

/*!
 * @brief This function is used to flush a registry key to disk.  This should be used
 *        if it is absolutely necessary to ensure that a key is flushed to disk
 * 	      immediately after it is written.
 * @param pRegPath Pointer to the full path of registry key
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilFlushRegistryKey(
    csx_cstring_t pRegPath);

/*!
 * @brief  Open a handle to the registry key for later use with the enumerate function.
 * @param pRegPath Pointer to the relative or full path of the Key to be opened.
 * @param parentHandle Open Emcutil key handle to the parent key for pRegPath.
 *						This is optional and may be specified as NULL.  If the
 *						parameter is specified pRegPath must be relative to the
 *						path used to open the parent handle, otherwise pRegPath
 *                      must be the full path to the Key plus the Key itself
 *		                (e.g., if the key is "Tcpip" in the Service registry the path is\n
 *		                \ Registry \ Machine \ System \ CurrentControlSet \ Services \ Tcpip\n
 *		                Note that the leading "\" seems to be required).
 * @param accessFlags Access flags:
 *					  - EMCUTIL_REG_KEY_QUERY_VALUE			-- Read Key Values
 *					  - EMCUTIL_REG_KEY_SET_VALUE			-- Write Key Values
 *					  - EMCUTIL_REG_KEY_CREATE_SUB_KEY		-- Create Subkeys for the key
 *					  - EMCUTIL_REG_KEY_ENUMERATE_SUB_KEYS	-- Read the key's subkeys
 *					  - EMCUTIL_REG_KEY_READ				-- EMCUTIL_REG_KEY_QUERY_VALUE | EMCUTIL_REG_KEY_ENUMERATE_SUB_KEYS
 *					  - EMCUTIL_REG_KEY_WRITE				-- EMCUTIL_REG_KEY_SET_VALUE | EMCUTIL_REG_KEY_CREATE_SUB_KEY
 *					  - EMCUTIL_REG_KEY_EXECUTE				-- same as EMCUTIL_REG_KEY_READ
 *					  - EMCUTIL_REG_KEY_ALL_ACCESS			-- all access flags set
 * @param pRetKeyHandle	[OUT] Address to store the returned key handle
 * @return EMCUTIL_STATUS code:
 *       - EMCUTIL_STATUS_OK            -- the key is opened and the handle created
 *       - EMCUTIL_STATUS_NO_RESOURCES  -- insufficient memory is available to complete the request
 *       - EMCUTIL_STATUS_INVALID_PARAMETER
 *       - EMCUTIL_STATUS_NAME_NOT_FOUND
 * @remark The returned handle must be closed with the EmcutilRegistryCloseKey function to
 *	       avoid a memory leak.
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegistryOpenKey(
    csx_cstring_t pRegPath,
    EMCUTIL_REG_HANDLE parentHandle,
    csx_wulong_t accessFlags,
    EMCUTIL_REG_HANDLE * pRetKeyHandle);

/*!
 * @brief Close a handle to the registry key.
 * @param EmcutilRegHandle Open handle to a registry key 
 * @return EMCUTIL_STATUS code:
 *         - EMCUTIL_STATUS_OK   -- the key is closed and the handle deleted
 *         - EMCUTIL_STATUS_INVALID_PARAMETER
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegistryCloseKey(
    EMCUTIL_REG_HANDLE EmcutilRegHandle);

/*!
 * @brief Provide information about the number and sizes of subkeys for a key
 * @param EmcutilRegHandle A registry key handle
 * @param pNumValues [OUT] Address to return the number of values. Optional, may be specified as NULL
 * @param pMaxValueNameLen [OUT] Address to return the max value name length in chars, does not include terminating NUL.
 *        Optional, may be specified as NULL
 * @param pMaxValueDataLen [OUT] Address to return the max value data length in bytes.
 *        Optional, may be specified as NULL
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegistryQueryKeyInfo(
    EMCUTIL_REG_HANDLE EmcutilRegHandle,
    csx_wulong_t * pNumValues,
    csx_wulong_t * pMaxValueNameLen,
    csx_wulong_t * pMaxValueDataLen);

/*!
 * @brief Provide information about the value
 * @param EmcutilRegHandle A registry key handle
 * @param pValueName The name of the value under the open key that you want to query
 * @param pDataLength [OUT] Address to return the length (in bytes) of the returned data string.
 *        Optional, may be specified as NULL
 * @param ppData [OUT] Address to return data buffer address. This buffer will contain an ascii character representation
 *        of the value's data. Optional, may be specified as NULL
 * @param pRegPathLength [OUT] Address to return the length of the returned path (length includes terminating NUL).
 *        Optional, may be specified as NULL
 * @param ppRegPath	[OUT] Address to return path buffer address. This name is suitable for use in a call to one of
 *        the Emcutil registry get or set functions. Optional, may be specified as NULL
 * @param pParmNameLength [OUT] Address to return the length of the returned param name (length includes terminating NUL).
 *        Optional, may be specified as NULL
 * @param ppParmName [OUT] Address to return param name buffer address This name is suitable for use in a call to one of
 *        the Emcutil registry get or set functions, Optional, may be specified as NULL
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise)
 * @remark The caller must use EmcutilFreeRegistryMem() to free the ppData, ppRegPath, and ppParmName buffers
 *         after a successful call to this function.
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegistryQueryValueInfo(
    EMCUTIL_REG_HANDLE EmcutilRegHandle,
    csx_char_t * pValueName,
    csx_wulong_t * pDataLength,
    csx_pchar_t * ppData,
    csx_wulong_t * pRegPathLength,
    csx_pchar_t * ppRegPath,
    csx_wulong_t * pParmNameLength,
    csx_pchar_t * ppParmName);

/*!
 * @brief Provide information about the subkeys of an open registry key
 * @param EmcutilRegHandle A registry key handle
 * @param SubkeyIndex The index of the desired subkey
 * @param pNameLen [OUT] Address to return the name length in chars (does not include terminating NUL).
 *        Optional, may be specified as NULL
 * @param ppName [OUT] Address to return the address of the subkey name string. Optional, may be specified as NULL
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise):
 *       - EMCUTIL_STATUS_OK -- The data has been returned
 *       - EMCUTIL_STATUS_NO_MORE_ENTRIES -- The Index value is out of range for the registry key
 *	           specified by EmcutilRegHandle. For example, if a key has n subkeys, then for any value
 *             greater than n-1 the routine returns STATUS_NO_MORE_ENTRIES.
 *       - EMCUTIL_STATUS_INVALID_PARAMETER -- Invalid argument
 *       - EMCUTIL_STATUS_NO_RESOURCES -- Failed to allocate resources (memory)
 * @remark The caller must call EmcutilFreeRegistryMem() when done with ppName.
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegistryEnumerateKey(
    EMCUTIL_REG_HANDLE EmcutilRegHandle,
    csx_wulong_t SubkeyIndex,
    csx_wulong_t * pNameLen,
    csx_pchar_t * ppName);

/*!
 * @brief Provide information about the value keys of an open registry key
 * @param EmcutilRegHandle A registry key handle
 * @param SubkeyIndex The index of the desired value key
 * @param pDataLength [OUT] Address to return the length of the returned data. Optional, may be specified as NULL
 * @param ppData [OUT] Address to return data buffer address. Optional, may be specified as NULL
 * @param pNameLength [OUT] Address to return the name length in chars, does not include terminating NUL.
 *        Optional, may be specified as NULL
 * @param ppName [OUT] Address to return the address of the value key name string. Optional, may be specified as NULL
 * @return EMCUTIL_STATUS code (EMCUTIL_STATUS_OK if success, error code otherwise):
 *       - EMCUTIL_STATUS_OK -- The data has been returned
 *       - EMCUTIL_STATUS_NO_MORE_ENTRIES -- The Index value is out of range for the registry key
 *	           specified by EmcutilRegHandle. For example, if a key has n subkeys, then for any value
 *             greater than n-1 the routine returns STATUS_NO_MORE_ENTRIES.
 *       - EMCUTIL_STATUS_INVALID_PARAMETER -- Invalid argument
 *       - EMCUTIL_STATUS_NO_RESOURCES -- Failed to allocate resources (memory)
 * @remark The caller must call EmcutilFreeRegistryMem() when done with ppName and ppData
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilRegistryEnumerateValueKey(
    EMCUTIL_REG_HANDLE EmcutilRegHandle,
    csx_wulong_t SubkeyIndex,
    csx_wulong_t * pDataLength,
    csx_pvoid_t * ppData,
    csx_wulong_t * pNameLength,
    csx_pchar_t * ppName);


/*! @} END emcutil_registry_pla */

CSX_CDECLS_END

//++
//.End file EmcUTIL_Registry_PLA.h
//--

#endif                                     /* EMCUTIL_REGISTRY_PLA_H_ */
