/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_registry_sim.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the registry operation functions.
 * 
 * @remark
 *  The registry is simulated as file.The file is generated when esp loads
 *  The name of file is esp_reg_pid<sp_pid>.txt,the format of file is  "key_name   key_value".
 *  e.g.  EspFlexportkey    1  
 *    
 * @version
 *   12 -April -2010: Vaibhav Gaonkar Created.
 *
 ***************************************************************************/
 
/*************************
 *   INCLUDE FILES
 *************************/
#include <windows.h>
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_file.h"
#include "fbe/fbe_library_interface.h"
#include "fbe/fbe_registry.h"

/****************************
 * MACRO DEFINATIONS
 ****************************/
#define REGISTRY_LENGTH   256
#define REGISTRY_SIM_FILE_PATH_LENGTH 64
#define REGISTRY_SIM_FILE_PATH_FORMAT "./sp_sim_pid%d/esp_reg_pid%d.txt"
#define REGISTRY_SIM_TEMP_FILE_PATH_FORMAT "./sp_sim_pid%d/esp_reg_temp_pid%d.txt"

/*************************************
 * LOCAL FUNCTIONS
 *************************************/
fbe_status_t 
fbe_registry_get_key_value(fbe_file_handle_t file_p,
                           fbe_u8_t *buffer,
                           fbe_u8_t *pKey,
                           fbe_u8_t *registry_key_value);



/*!***************************************************************
 * fbe_registry_read()
 ****************************************************************
 * @brief
 *  This function use for reading the specified registry key
 *  value.
 *
 * @param filePathExtraInfo It is only for sim  mode. Pass in SP PID to append to
 *             the end of the register file path. If NULL this function will use current
 *             PID.
 * @param pRegPath       Registry path (This is path to file)
 * @param pKey           Registry key
 * @param pBuffer        Pointer to buffer to store key value
 * @param bufferLength   Length of buffer
 * @param ValueType      Type of registry key
 * @param defaultValue_p Default value for key
 * @param defaultLength  Default value length
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
    fbe_file_handle_t   fp = NULL;
    fbe_u8_t *registry_key_value;
    fbe_status_t status;
    fbe_u8_t *buffer; 
    fbe_u8_t *full_registry_path_and_key;
    fbe_u8_t *registry_file_path;
    fbe_u64_t pid = 0;
    fbe_u32_t retry_count = 0;
    const fbe_u32_t retry_count_max = 100;

    FBE_UNREFERENCED_PARAMETER(defaultValue_p);
    FBE_UNREFERENCED_PARAMETER(defaultLength);

    if(pBuffer == NULL ||
       pRegPath == NULL ||
       pKey == NULL)
    {
            
       fbe_base_library_trace(FBE_LIBRARY_ID_REGISTRY,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Invalid argument for function  %s\n",__FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    registry_key_value = malloc(REGISTRY_LENGTH);
    buffer = malloc(REGISTRY_LENGTH);
    full_registry_path_and_key = malloc(REGISTRY_LENGTH);
    registry_file_path = malloc(REGISTRY_LENGTH);
    fbe_zero_memory(registry_key_value, REGISTRY_LENGTH);
    fbe_zero_memory(buffer, REGISTRY_LENGTH);
    fbe_zero_memory(full_registry_path_and_key, REGISTRY_LENGTH);
    fbe_zero_memory(registry_file_path, REGISTRY_LENGTH);

    if(filePathExtraInfo != NULL)
    {
        pid = atoi(filePathExtraInfo);
    }
    else
    {
        pid = csx_p_get_process_id();
    }
    fbe_sprintf(registry_file_path, REGISTRY_SIM_FILE_PATH_LENGTH, REGISTRY_SIM_FILE_PATH_FORMAT, (int)pid, (int)pid);

    do
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_REGISTRY,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "opening the registry file, try %d\n",
                               retry_count);
        /* Open the file for read operation */
        fp = fbe_file_open(registry_file_path, FBE_FILE_RDONLY, 0, NULL);
        retry_count++;
        if(fp == FBE_FILE_INVALID_HANDLE)
        {
            fbe_thread_delay(100);
        }
        else
        {
            break;
        }
    }while(retry_count < retry_count_max);

    if(FBE_FILE_INVALID_HANDLE == fp)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_REGISTRY,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "Error in opening the registry file [%s] %s\n", (char*)registry_file_path, (char*)pRegPath);

        free(registry_key_value);
        free(buffer);
        free(full_registry_path_and_key);
        free(registry_file_path);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Concatenate the key and the path into one pathname */
    fbe_sprintf(full_registry_path_and_key, REGISTRY_LENGTH, "%s\\%s", pRegPath, pKey);

    /*  Get the registry key value */
    status = fbe_registry_get_key_value(fp, buffer, full_registry_path_and_key, 
                                        registry_key_value);

    if(FBE_STATUS_OK == status)
    {
        /* Successfully read the key value */
        switch(ValueType)
        {
            case FBE_REGISTRY_VALUE_BINARY:
            case FBE_REGISTRY_VALUE_DWORD:
                *((fbe_u32_t *)pBuffer) = atoi(registry_key_value);
            break;   
            default:                
                if(bufferLength >= strlen(registry_key_value))
                {
                    strcpy((fbe_u8_t *)pBuffer,registry_key_value);
                }
                else
                {
                    fbe_base_library_trace(FBE_LIBRARY_ID_REGISTRY,
                                           FBE_TRACE_LEVEL_ERROR,
                                           FBE_TRACE_MESSAGE_ID_INFO,
                                           "Insufficient buffer size %s\n",__FUNCTION__);
                    fbe_file_close(fp);
                    free(registry_key_value);
                    free(buffer);
                    free(full_registry_path_and_key);
                    free(registry_file_path);                    
                    return FBE_STATUS_GENERIC_FAILURE;
                }
            break;                
        }
        fbe_file_close(fp);
        free(registry_key_value);
        free(buffer);
        free(full_registry_path_and_key);
        free(registry_file_path);
        return FBE_STATUS_OK;
    }
    else if(createKey == TRUE)
    {
        /* We are to create a new key upon failure */
        fbe_file_close(fp);

        status = fbe_registry_write(NULL,
                   pRegPath,
                   pKey, 
                   ValueType,
                   defaultValue_p, 
                   defaultLength);
        free(registry_key_value);
        free(buffer);
        free(full_registry_path_and_key);
        free(registry_file_path);
        return status;

    }
    fbe_file_close(fp);
    free(registry_key_value);
    free(buffer);
    free(full_registry_path_and_key);
    free(registry_file_path);
    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************
*   end fbe_registry_read()
*******************************/

/*!***************************************************************
 * fbe_registry_get_key_value()
 ****************************************************************
 * @brief
 *  This function for split the string into registry key 
 *  and registry key value. 
 *
 * @param file_p                File pointer  
 * @param buffer                Buffer to hold the string
 * @param pKey                  Search key
 * @param registry_key_value    Place to hold the registy key value
 * 
 * @return fbe_status_t FBE_STATUS_OK  Successfully read registry key
 *                                     and registy key value.
 *
 * @author
 *  20 -April-2010: Vaibhav Gaonkar Created.
 *
 ****************************************************************/
fbe_status_t 
fbe_registry_get_key_value(fbe_file_handle_t file_p,
                           fbe_u8_t *buffer,
                           fbe_u8_t *pKey,
                           fbe_u8_t *registry_key_value)
{    
    fbe_u8_t registry_key[REGISTRY_LENGTH] = {'\0'};

    while(1)
    {
        /*  Read the line from opened file*/
        if(fbe_file_fgets(file_p, buffer, REGISTRY_LENGTH) == 0)
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_REGISTRY,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "Error in retrieving string from registry file %s\n",__FUNCTION__);
              return FBE_STATUS_GENERIC_FAILURE;
        }
        fbe_zero_memory(registry_key, REGISTRY_LENGTH);
        fbe_zero_memory(registry_key_value, REGISTRY_LENGTH);

        /* Split the line into registry key and key value. */
        sscanf(buffer, "%s\t%[^\n]", registry_key, registry_key_value);
#if 0
        registry_key = strtok(buffer, " \t\f\n");
        if(registry_key != NULL)
        {
            *registry_key_value = strtok(NULL," \t\f\n" );
        }
#endif
        if(strlen(registry_key) == 0)
        {
            /* Reach the end of file
             * Reg file does not contain required key
             */
             break;
        }
        if(!(strcmp(pKey,registry_key)))
        {           
            if(strlen(registry_key_value) == 0)
            {
                /* Reg key is there in file but it does NOT have value */
                return FBE_STATUS_GENERIC_FAILURE;
            }else
            {
                return FBE_STATUS_OK;
            }
        }
    }
    return FBE_STATUS_GENERIC_FAILURE;
}
/***************************************
*   end fbe_registry_get_key_value()
****************************************/

/*!***************************************************************
 * fbe_registry_write()
 ****************************************************************
 * @brief
 *  This function use for write the registy key and it value 
 *  speificed file
 *
 * @param filePathExtraInfo It is only for sim  mode. Pass in SP PID to append to
 *             the end of the register file path. If NULL this function will use current
 *             PID.
 * @param pRegPath       Registry path (This is path to file)
 * @param pKey           Registry key
 * @param ValueType      Type of registry key
 * @param value_p        Key value to write
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
    fbe_u8_t *buffer;
    fbe_u8_t *temp_buffer;
    fbe_u8_t *temp_key_value;
    fbe_file_handle_t fp = NULL;
    fbe_file_handle_t fp_temp = NULL;
    fbe_u8_t *local_buffer_value;
    fbe_u8_t *full_registry_path_and_key;
    fbe_u8_t *registry_file_path;
    fbe_u8_t *registry_temp_file_path;
    fbe_u64_t pid = 0;
    fbe_status_t status;
    fbe_u8_t *registry_key_value;
    fbe_u32_t file_status = 0;
    fbe_u8_t retry_count = 0;

    FBE_UNREFERENCED_PARAMETER(length);

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

    buffer = malloc(REGISTRY_LENGTH);
    temp_buffer = malloc(REGISTRY_LENGTH);
    temp_key_value = malloc(REGISTRY_LENGTH);
    local_buffer_value = malloc(REGISTRY_LENGTH);
    full_registry_path_and_key = malloc(REGISTRY_LENGTH);
    registry_file_path = malloc(REGISTRY_LENGTH);
    registry_temp_file_path = malloc(REGISTRY_LENGTH);
    registry_key_value = malloc(REGISTRY_LENGTH);

    fbe_zero_memory(buffer, REGISTRY_LENGTH);
    fbe_zero_memory(temp_buffer, REGISTRY_LENGTH);
    fbe_zero_memory(temp_key_value, REGISTRY_LENGTH);
    fbe_zero_memory(local_buffer_value, REGISTRY_LENGTH);
    fbe_zero_memory(full_registry_path_and_key, REGISTRY_LENGTH);
    fbe_zero_memory(registry_file_path, REGISTRY_LENGTH);
    fbe_zero_memory(registry_temp_file_path, REGISTRY_LENGTH);
    fbe_zero_memory(registry_key_value, REGISTRY_LENGTH);

    if(filePathExtraInfo != NULL)
    {
        pid = atoi(filePathExtraInfo);
    }
    else
    {
        pid = csx_p_get_process_id();
    }
    fbe_sprintf(registry_file_path, REGISTRY_SIM_FILE_PATH_LENGTH, REGISTRY_SIM_FILE_PATH_FORMAT, (int)pid, (int)pid);
    fp= fbe_file_open(registry_file_path, FBE_FILE_RDWR, 0, NULL);
    if(FBE_FILE_INVALID_HANDLE == fp)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_REGISTRY,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "Error in opening the file [%s] %s \n", registry_file_path, __FUNCTION__);
        free(buffer);
        free(temp_buffer);
        free(temp_key_value);
        free(local_buffer_value);
        free(full_registry_path_and_key);
        free(registry_file_path);
        free(registry_temp_file_path);
        free(registry_key_value);
        return FBE_STATUS_GENERIC_FAILURE;
    }    
    
    /* Concatenate the key and the path into one pathname */
    fbe_sprintf(full_registry_path_and_key, REGISTRY_LENGTH, "%s\\%s", pRegPath, pKey);

    /* Initialized the buffer */
    switch(valueType)
    {
        case FBE_REGISTRY_VALUE_DWORD:
        case FBE_REGISTRY_VALUE_BINARY:
            fbe_sprintf(buffer,(REGISTRY_LENGTH - 1),"%s\t%d\n",full_registry_path_and_key,*((fbe_u32_t *)value_p));
        break;
        default:
            strncpy(local_buffer_value, (fbe_u8_t *)value_p, REGISTRY_LENGTH);	
            fbe_sprintf(buffer , (REGISTRY_LENGTH - 1) ,"%s\t%s\n",full_registry_path_and_key,local_buffer_value);
        break;
    }

    
    /*
     * Locate the registry entry so that if one already exists we can overwrite it.
     * If the entry is not found then seek to the end of the file.
     */
    status = fbe_registry_get_key_value(fp, temp_buffer, full_registry_path_and_key, 
                                    registry_key_value);
    fbe_file_lseek(fp, 0, SEEK_SET);
    if(status == FBE_STATUS_OK)
    {
        fbe_u8_t temp_registry_key[REGISTRY_LENGTH];
        fbe_u8_t temp_registry_key_value[REGISTRY_LENGTH];
        /* 
         * The entry exists, to overwrite we must copy the registry file one line
         * at a time to a new file, when we reach the matching line, write out the 
         * new line and then continue copying the file.  When the copy is complete
         * delete the original registry file and rename the temporary file. 
         */

        fbe_sprintf(registry_temp_file_path, REGISTRY_SIM_FILE_PATH_LENGTH, REGISTRY_SIM_TEMP_FILE_PATH_FORMAT, (int)pid, (int)pid);
        fp_temp = fbe_file_creat(registry_temp_file_path, FBE_FILE_RDWR);

        while(1)
        {
            fbe_zero_memory(temp_registry_key, REGISTRY_LENGTH);
            fbe_zero_memory(temp_registry_key_value, REGISTRY_LENGTH);
            /*  Read the line from original registry file*/
            if(fbe_file_fgets(fp, temp_buffer, REGISTRY_LENGTH) == 0)
            {
                fbe_base_library_trace(FBE_LIBRARY_ID_REGISTRY,
                                       FBE_TRACE_LEVEL_ERROR,
                                       FBE_TRACE_MESSAGE_ID_INFO,
                                       "Error in retrieving string from registry file %s\n",__FUNCTION__);
                  break;
            }
            /* Split the line into registry key and key value. */
            //registry_key = strtok(temp_buffer, " \t\f\n");
            sscanf(temp_buffer, "%s\t%[^\n]", temp_registry_key, temp_registry_key_value);
#if 0
            if(registry_key != NULL)
            {
                registry_key_value = strtok(NULL," \t\f\n" );
            }
#endif
            if(strlen(temp_registry_key) == 0)
            {
                // Reach the end of file
                 break;
            }
            if(!(strcmp(full_registry_path_and_key,temp_registry_key)))
            {   
                // Replace this line with the new data
                fbe_file_write(fp_temp, buffer, (unsigned int)strlen(buffer), NULL); 
            }
            else
            {

                /*
                 * For some reason reading in the line and then simply writing it back out results
                 * in null characters instead of tabs being inserted here.  So to make sure we
                 * get the data on one line I'm re-creating the string here.
                 */
                fbe_zero_memory(temp_key_value, REGISTRY_LENGTH);
                fbe_sprintf(temp_key_value, (REGISTRY_LENGTH - 1) ,"\t%s\n", temp_registry_key_value);
                strcat(temp_registry_key, temp_key_value);


                // Copy this line to the temp file
                fbe_file_write(fp_temp, temp_registry_key, (unsigned int)strlen(temp_registry_key), NULL);
            }
        }
        /* Close and delete the original registry file */
        file_status = fbe_file_close(fp);
        if(file_status != 0)
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_REGISTRY,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s Failed to close %s status:0x%x\n",__FUNCTION__,registry_file_path,file_status);

        }
        file_status = fbe_file_delete(registry_file_path);
        while( (file_status == FBE_FILE_ERROR) && (retry_count < 50) )
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_REGISTRY,
                                   FBE_TRACE_LEVEL_INFO,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s Failed to delete %s status:0x%x count:%d\n",__FUNCTION__,registry_file_path,file_status,retry_count);
            EmcutilSleep(100);
            retry_count++;
            file_status = fbe_file_delete(registry_file_path);
        }
        if(retry_count == 5)
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_REGISTRY,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s Failed to delete %s status:0x%x retries exceeded.\n",__FUNCTION__,registry_file_path,file_status);

        }
        
        /* Rename the temporary file back to the original registry file */
        fbe_file_close(fp_temp);
        fbe_file_rename(registry_temp_file_path, registry_file_path);
        fbe_file_delete(registry_temp_file_path);
    }
    else
    {
        /*
         * The entry does not exist, just append this to the end of the file.
         */
        fbe_file_lseek(fp, 0, SEEK_END);
        fbe_file_write(fp, buffer, (unsigned int)strlen(buffer), NULL);
        fbe_file_close(fp); 
    }

    free(buffer);
    free(temp_buffer);
    free(temp_key_value);
    free(local_buffer_value);
    free(full_registry_path_and_key);
    free(registry_file_path);
    free(registry_temp_file_path);
    free(registry_key_value);
       
    return FBE_STATUS_OK;
}
/******************************
*   end fbe_registry_write()
*******************************/
/*!***************************************************************
 * fbe_registry_check()
 ****************************************************************
 * @brief
 *     This is to check the registy for specified key.
 *
 * @param RelativeTo
 * @param Path  Key for which checking the registy file
 * 
 * @return fbe_status_t
 *
 * @author
 *  12-April-2010: Vaibhav Gaonkar Created.
 *
 ****************************************************************/
 fbe_status_t 
 fbe_registry_check(fbe_u32_t RelativeTo,
                    fbe_u8_t *Path)
{
    fbe_file_handle_t fp = NULL;
    fbe_u8_t registry_key_value[REGISTRY_LENGTH] = {'\0'};    
    fbe_status_t status;
    fbe_u8_t   buffer[REGISTRY_LENGTH] = {'\0'};

    /* Open the file for read operation*/
    fp = fbe_file_open(Path, FBE_FILE_RDONLY, 0, NULL);
    if(FBE_FILE_INVALID_HANDLE == fp)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_REGISTRY,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               " Error in opening the registry file\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_registry_get_key_value(fp, buffer, Path, registry_key_value);
    fbe_file_close(fp);
    if(FBE_STATUS_OK == status)
    {        
        return FBE_STATUS_OK;
    }
    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************
*   end fbe_registry_check()
*******************************/

fbe_status_t fbe_flushRegistry()
{
    return FBE_STATUS_OK;
}

fbe_status_t fbe_registry_delete_key(fbe_u8_t* pRegPath,
                                     fbe_u8_t* pKey)
{
    // not yet implemented for simulation

    return FBE_STATUS_OK;
}
