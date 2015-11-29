/***************************************************************************
* Copyright (C) EMC Corporation 2011 
* All rights reserved. 
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!*************************************************************************
*                 @file fbe_err_sniff_file_access.c
****************************************************************************
*
* @brief
*  This file contains file access functions for fbe_err_sniff.
*
* @ingroup fbe_err_sniff
*
* @date
*  08/08/2011 - Created. Vera Wang
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_err_sniff_file_access.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_time.h"

/*!**********************************************************************
*             fbe_err_sniff_open_file()               
*************************************************************************
*
*  @brief
*    fbe_err_sniff_open_file - Open a file for fbe_err_sniff
	
*  @param    * file_name - the file name pointer
*  @param    fMode - open file mode(clean or append)
*
*  @return
*			 File*
*************************************************************************/
FILE * fbe_err_sniff_open_file(const char* file_name, char fMode)                                                                      
{                                                                                                                          
    FILE * fp;                                                                                                             
    if (fMode == 'c') 
    {
        if((fp = fopen(file_name, "w"))==NULL)                                                                                 
        {                                                                                                                      
            return NULL;                                                                                                       
        }
        else return fp;   
    }
    else if (fMode == 'm') 
    {
        if((fp = fopen(file_name, "a"))==NULL)                                                                                 
        {                                                                                                                      
            return NULL;                                                                                                       
        }
        else return fp;
    }
    else return NULL;                                                                                                                                                                                                                             
}                                                                                                                          
/******************************************
 * end fbe_err_sniff_open_file()
 ******************************************/                                                                                                                           
    
/*!**********************************************************************
*             fbe_err_sniff_write_to_file()               
*************************************************************************
*
*  @brief
*    fbe_err_sniff_write_to_file - Write content into a file
	
*  @param    * fp - the file pointer
*  @param    content - content to be writen to the file
*
*  @return
*			 fbe_status_t
*************************************************************************/                                                                                                                       
fbe_status_t fbe_err_sniff_write_to_file(FILE * fp,const char* content)                                                    
{                                                                                                                          
    fbe_system_time_t  currentTime;                                                                                        
    fbe_char_t timeStamp[20]={0};                                                                                          
                                                                                                                           
    fbe_get_system_time(&currentTime);                                                                                     
    //Construct timeStamp                                                                                                  
    fbe_sprintf(timeStamp, 20, "%04d-%02d-%02d %02d:%02d:%02d",                                                            
                currentTime.year,currentTime.month,currentTime.day,currentTime.hour,currentTime.minute,currentTime.second);
                                                                                                                           
    fprintf(fp, "%s   %s %s", timeStamp, content, "\n");                                                                    
                                                                                                                           
    return FBE_STATUS_OK;                                                                                                  
}                                                                                                                          
/******************************************
 * end fbe_err_sniff_write_to_file()
 ******************************************/      
                                                                                                                      
/*!**********************************************************************
*             fbe_err_sniff_close_file()               
*************************************************************************
*
*  @brief
*    fbe_err_sniff_close_file - Close the file for fbe_err_sniff
	
*  @param    * fp - the file pointerr
*
*  @return
*			 fbe_status_t
*************************************************************************/                                                                                                                      
fbe_status_t fbe_err_sniff_close_file(FILE * fp)                                                                           
{                                                                                                                          
    fclose(fp);                                                                                                            
    return FBE_STATUS_OK;                                                                                                  
}          
/******************************************
 * end fbe_err_sniff_close_file()
 ******************************************/
                                                                                                                 
/******************************************
* end file fbe_err_sniff_file_access.c
*******************************************/
