/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_registry_kernel.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the registry operation functions.
 * 
 * @version
 *   12 -April -2010: Vaibhav Gaonkar Created.
 *
 ***************************************************************************/

 /*************************
 *   INCLUDE FILES
 *************************/
#include <ntddk.h>
#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_library_interface.h"
#include "fbe/fbe_registry.h"
#include "EmcUTIL.h"

/****************************
 * MACRO DEFINATIONS
 ****************************/
#define REGISTRY_LENGTH   256
#define noOfBytes         (REGISTRY_LENGTH)

/*************************************
 * LOCAL FUNCTIONS
 *************************************/
static void 
fbe_convWstrToStr(fbe_u8_t *str_p,
                  WCHAR *wStr_p,
                  fbe_u32_t maxLength);
static void 
fbe_convStrToWstr(fbe_u8_t *str_p,
                  WCHAR *wStr_p,
                  fbe_u32_t maxLength);
static
void fbe_convRegValueType(fbe_registry_value_type_t ValueType,
                          EMCUTIL_REG_VALUE_TYPE *regType);

/*!***************************************************************
 * fbe_registry_read()
 ****************************************************************
 * @brief
 *  This function use for reading the specified registry key
 *  value.
 *
 * @param filePathExtraInfo Only used in sim mode, ignore in kernel mode
 * @param pRegPath       Registry path
 * @param pKey           Registry key
 * @param pBuffer        Pointer to buffer to store key value
 * @param bufferLength   Lenght of buffer
 * @param ValueType      Type of registry key
 * @param defaultValue_p Default value for key
 * @param createKey      TRUE, creates the key upon failure
 * 
 * @return fbe_status_t FBE_STATUS_OK  Successfully read registry
 *                                     value
 *
 * @author
 *  12-April-2010: Vaibhav Gaonkar Created.
 *
 ****************************************************************/
fbe_status_t 
fbe_registry_read(fbe_u8_t* filePathExtraInfo,
                  fbe_u8_t* pRegPath,
                  fbe_u8_t* pKey,
                  void* pBuffer,
                  fbe_u32_t bufferLength,
                  fbe_registry_value_type_t ValueType,
                  void* defaultValue_p,
                  fbe_u32_t defaultLength,
                  fbe_bool_t createKey)
{
    fbe_status_t fbe_status = FBE_STATUS_OK;
    EMCUTIL_STATUS Status;
    EMCUTIL_REG_VALUE_TYPE regValueType;
    EMCUTIL_REG_HANDLE keyHandle;
    EMCUTIL_REG_VALUE_SIZE bufferLengthRv;

    if(pBuffer == NULL ||
       defaultValue_p == NULL ||
       pRegPath == NULL ||
       pKey == NULL)
    {
            
       fbe_base_library_trace(FBE_LIBRARY_ID_REGISTRY,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Invalid argument for function  %s\n",__FUNCTION__); 
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    Status = EmcutilRegOpenKeyA(NULL, 
                                pRegPath, 
                                EMCUTIL_REG_KEY_QUERY_VALUE, 
                                &keyHandle);

    if (EMCUTIL_SUCCESS(Status))
    {
        Status = EmcutilRegQueryValueA(keyHandle,
                                        pKey,
                                        &regValueType,
                                        pBuffer,
                                        bufferLength,
                                        &bufferLengthRv);
        EmcutilRegCloseKey(keyHandle);
    }

    if (EMCUTIL_SUCCESS(Status))
    {
        return FBE_STATUS_OK;
    }
    else
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_REGISTRY,
                               FBE_TRACE_LEVEL_DEBUG_LOW,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: Could not get/read the reg key. Picking up the default.\n",
                               __FUNCTION__); 

        fbe_base_library_trace(FBE_LIBRARY_ID_REGISTRY,
                               FBE_TRACE_LEVEL_DEBUG_LOW,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "Path: %s\n", pRegPath); 

        fbe_base_library_trace(FBE_LIBRARY_ID_REGISTRY,
                               FBE_TRACE_LEVEL_DEBUG_LOW,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: Reg key: %s, Status: 0x%08x, createKey: %d\n",
                               __FUNCTION__, pKey, Status, createKey); 
        fbe_status = FBE_STATUS_GENERIC_FAILURE;
    }

    // fail if input buffer is too small
    if ((createKey == TRUE) && (Status != EMCUTIL_STATUS_MORE_DATA))
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_REGISTRY,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: Writing default registry.\n",
                               __FUNCTION__); 

        /* Create the Key when the read fails */
        fbe_status = fbe_registry_write(NULL,
                                        pRegPath,
                                        pKey, 
                                        ValueType,
                                        defaultValue_p, 
                                        defaultLength);
        if (fbe_status == FBE_STATUS_OK)
        {
            // copy default value if buffer is big enough
            if (bufferLength >= defaultLength)
            {
                csx_p_memcpy(pBuffer, defaultValue_p, defaultLength);

                fbe_base_library_trace(FBE_LIBRARY_ID_REGISTRY,
                                       FBE_TRACE_LEVEL_INFO,
                                       FBE_TRACE_MESSAGE_ID_INFO,
                                       "%s: Copied the default. Status: 0x%x\n",
                                       __FUNCTION__, fbe_status); 
            }
            else
            {
                fbe_base_library_trace(FBE_LIBRARY_ID_REGISTRY,
                                       FBE_TRACE_LEVEL_WARNING,
                                       FBE_TRACE_MESSAGE_ID_INFO,
                                       "%s: Input buffer is too small. bufferLen: %d, defaultLen: %d\n",
                                       __FUNCTION__, bufferLength, defaultLength); 

                fbe_status = FBE_STATUS_GENERIC_FAILURE;
            }
        }

        if (fbe_status != FBE_STATUS_OK)
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_REGISTRY,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s: Registry write failed. Key: 0x%08llx, status 0x%x\n",
                                   __FUNCTION__, (unsigned long long)pKey, fbe_status); 
        }
    }

    return fbe_status;
}
/******************************
    end fbe_registry_read()     
*******************************/

/*!***************************************************************
 * fbe_registry_write()
 ****************************************************************
 * @brief
 *  .This function is for write the speificed registry key
 *   value
 *
 * @param filePathExtraInfo Only used in sim mode, ignore in kernel mode
 * @param pRegPath  Registry path
 * @param pKey      Registry key
 * @param valueType Type of registry key
 * @param value_p   Pointer to name of subkey
 * @param length    Specify number of bytes.
 * 
 * @return fbe_status_t
 *
 * @author
 *  12-April-2010: Vaibhav Gaonkar Created.
 *
 ****************************************************************/
 fbe_status_t 
 fbe_registry_write(fbe_u8_t* filePathExtraInfo,
                    fbe_u8_t* pRegPath,
                    fbe_u8_t* pKey, 
                    fbe_registry_value_type_t valueType,
                    void *value_p, 
                    fbe_u32_t length)
{
    EMCUTIL_STATUS Status;
    EMCUTIL_REG_VALUE_TYPE regValueType;
    EMCUTIL_REG_HANDLE keyHandle;

    if(pRegPath == NULL ||
       pKey == NULL ||
       value_p == NULL)
    {
       fbe_base_library_trace(FBE_LIBRARY_ID_REGISTRY,
                          FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "Invalid argument for function  %s\n",__FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_convRegValueType(valueType, &regValueType);

    Status = EmcutilCheckRegistryKey(pRegPath);
 
    if (Status != EMCUTIL_STATUS_OK) 
    {
        Status = EmcutilCreateRegistryKey(pRegPath);
        if (Status != EMCUTIL_STATUS_OK) 
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_REGISTRY,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s Create parameter key 0x%08x\n",__FUNCTION__, Status);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    Status = EmcutilRegOpenKeyA(NULL, 
                                pRegPath, 
                                EMCUTIL_REG_KEY_WRITE, 
                                &keyHandle);

    if (EMCUTIL_SUCCESS(Status))
    {
        // Write data to the registry.
        // adjust length if necessary 
        if ((valueType == FBE_REGISTRY_VALUE_SZ) ||
            (valueType == FBE_REGISTRY_VALUE_MULTISTRING))
        {
            length = (fbe_u32_t)(strlen(value_p) + 1);
        }

        Status = EmcutilRegSetValueA(keyHandle,
                                     pKey,     
                                     regValueType,
                                     value_p,
                                     length);
    }
    if (!EMCPAL_IS_SUCCESS(Status))
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_REGISTRY,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s RtlWriteRegistryValue for key (%s) failed.. Status: 0x%08x",__FUNCTION__, pKey,Status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_REGISTRY,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s Update of key (%s) succeeded",__FUNCTION__, pKey);
        return FBE_STATUS_OK;
    }
}
/******************************
    end fbe_registry_write()    
*******************************/
/*!***************************************************************
 * fbe_registry_check()
 ****************************************************************
 * @brief
 *     This is to check the registy for specified key.
 *
 * @param RelativeTo
 * @param Path  Registry  path
 * 
 * @return NTSTATUS
 *
 * @author
 *  12-April-2010: Vaibhav Gaonkar Created.
 *
 ****************************************************************/
 fbe_status_t 
 fbe_registry_check(fbe_u32_t RelativeTo,
                    fbe_u8_t  *Path)
{
    EMCUTIL_STATUS Status;
    EMCUTIL_REG_HANDLE handle = NULL;

    Status = EmcutilRegOpenKeyA(NULL, Path, EMCUTIL_REG_KEY_ALL_ACCESS, &handle);
    if(!EMCUTIL_SUCCESS(Status))
    {    
        fbe_base_library_trace(FBE_LIBRARY_ID_REGISTRY,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s Registry checked failed",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    } else {
        EmcutilRegCloseKey(handle);
    }
    return FBE_STATUS_OK;
}
/******************************
    end fbe_registry_check()    
*******************************/
/*!***************************************************************
 * fbe_convWstrToStr()
 ****************************************************************
 * @brief 
 *  .This function convert the WCHAR to char string.
 *
 * @param str_p  CHAR string for saved output
 * @param wStr_p WCHAR string to convert
 * @param maxLength Maximum length of string
 * 
  * @author
 *  12-April-2010: Vaibhav Gaonkar Created.
 *
 ****************************************************************/
static 
void fbe_convWstrToStr(fbe_u8_t *str_p,
                       WCHAR  *wStr_p,
                       fbe_u32_t maxLength)
{
    fbe_u32_t len = 0;
    while(len < (maxLength - 1))
    {
        if(*wStr_p)
        {
            *str_p++ = (unsigned char)*wStr_p++;
            len++;
        }
        else
            break;
    }
    *str_p = '\0';
    return;
}
/******************************
    end fbe_convWstrToStr()    
*******************************/
/*!***************************************************************
 * fbe_convStrToWstr()
 ****************************************************************
 * @brief 
 *  .This function convert the WCHAR to char string.
 *
 * @param str_p  CHAR string to convert
 * @param wStr_p WCHAR string for saved output
 * @param maxLength Maximum length of string
 * 
  * @author
 *  12-April-2010: Vaibhav Gaonkar Created.
 *
 ****************************************************************/
static
void fbe_convStrToWstr(fbe_u8_t *str_p,
                       WCHAR *wStr_p,  
                       fbe_u32_t maxLength)
{
    fbe_u32_t   len = 0;
    while(len < (maxLength -1))
    {
        if(*str_p)
        {
            *wStr_p++ = (WCHAR) (*str_p++);
            len++;
        }
        else
            break;
    }
    *wStr_p = '\0';
    return;
}
/******************************
    end fbe_convStrToWstr()    
*******************************/
/*!***************************************************************
 * fbe_convRegValueType()
 ****************************************************************
 * @brief 
 *  .This function convert FBE registry type to EmcUTIL registy type
 *
 * @param ValueType  FBE registry type
 * @param regType retrun EmcUTIL registry value
 * 
 * @author
 *  12-April-2010: Vaibhav Gaonkar Created.
 *  10-November-2011: Chris Lin - convert to EmcUTIL type
 *
 ****************************************************************/
static
void fbe_convRegValueType(fbe_registry_value_type_t ValueType,
                          EMCUTIL_REG_VALUE_TYPE *regType)
{
    switch(ValueType)
    {

        case FBE_REGISTRY_VALUE_SZ:
            *regType = EMCUTIL_REG_TYPE_STRING;
        break;
        case FBE_REGISTRY_VALUE_DWORD:
            *regType = EMCUTIL_REG_TYPE_DWORD;
        break;
        case FBE_REGISTRY_VALUE_BINARY:
            *regType = EMCUTIL_REG_TYPE_BINARY;
        break;
        case FBE_REGISTRY_VALUE_MULTISTRING:
             *regType = EMCUTIL_REG_TYPE_MSTRING;
        break;
    }
}
/******************************
    end fbe_convRegValueType()    
*******************************/
#define FBE_REGISTRY_SYSTEM_PATH "\\Registry\\Machine\\system"
/*!***************************************************************
 * fbe_flushRegistry()
 ****************************************************************
 * @brief 
 *  This function issues a request to flush the Registry from
 *  the \\HKLM\SYSTEM Key and below
 *
 * @param None
 * 
  * @author
 *  06-Jan-2011: Brion Philbin Created.
 *
 ****************************************************************/
fbe_status_t fbe_flushRegistry()
{
    EMCUTIL_STATUS          Status;

    Status = EmcutilFlushRegistryKey(FBE_REGISTRY_SYSTEM_PATH);

    if (EMCUTIL_SUCCESS(Status))
    {
        return FBE_STATUS_OK;
    }
    else
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
}


/*!***************************************************************
 * fbe_registry_delete_key()
 ****************************************************************
 * @brief 
 *  This function deletes the specified Registry key.
 *
 * @param None
 * 
  * @author
 *  28-Mar-2011: Brion Philbin Created.
 *
 ****************************************************************/
fbe_status_t fbe_registry_delete_key(fbe_u8_t* pRegPath,
                                     fbe_u8_t* pKey)
{
    EMCPAL_STATUS Status;
    EMCUTIL_REG_HANDLE handle;

    Status = EmcutilRegOpenKeyA(NULL, pRegPath, EMCUTIL_REG_KEY_ALL_ACCESS, &handle);
    if (EMCUTIL_SUCCESS(Status)) {
        Status = EmcutilRegDeleteValueA(handle, pKey);
        EmcutilRegCloseKey(handle);
    }
    if (!EMCUTIL_SUCCESS(Status))
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_REGISTRY,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s EmcutilRegDeleteValue for key (%s) failed.. Status: 0x%08x",__FUNCTION__, pKey, Status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        return FBE_STATUS_OK;
    }

}
