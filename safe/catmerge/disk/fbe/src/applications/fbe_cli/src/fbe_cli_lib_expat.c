
/***************************************************************************
* Copyright (C) EMC Corporation 2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************
* fbe_cli_lib_expat.c
***************************************************************************
*
* DESCRIPTION
*  This file contains Expat parser, XML parsing and related functionality. 
*  
* TABLE OF CONTENTS
*  fbe_xml_parse_file
*  fbe_xml_save_file
*  fbe_xml_write_buffer
*
* NOTES
* 
* History:
*  05/10/2012  Created. kothal
***************************************************************************/

/*************************
 ** INCLUDE FILES
 *************************/
#include "fbe_cli_lib_expat.h"

/*********************************************************************
 *                      BEGIN FUNCTION DEFINITIONS                   *
 *********************************************************************/

/***************************************************
 * Redefine functions needed by the Expat library.
 * Since this is simulation and user mode,
 * we need to define these functions in terms of
 * available functions.
 ***************************************************/

/*********************************************************************
 * @var XmlLogTrace
 *********************************************************************
 * @brief Redefined function needed by the Expat library.
 *
 *  @param    LogString
 *  @param    arg1
 *  @param    arg2

 * @return - none.  
 *
 * @author March 01, 2011 - Created. Sandeep Chaudhari
 *********************************************************************/
void __stdcall XmlLogTrace(char *LogString, fbe_s32_t arg1, fbe_s32_t arg2)
{
    /* For user mode, this currently does nothing.
     */
    return;
}
/**************************************
 * end XmlLogTrace()
 **************************************/

/*********************************************************************
 * @var XmlMalloc
 *********************************************************************
 * @brief Redefined function needed by the Expat library. 
 *        This function allocates a block of size bytes of memory,
 *        returning a pointer to the beginning of the block.
 *
 *  @param   size - Size of the memory block, in bytes.
 *
 * @return - void*
 *
 * @author March 01, 2011 - Created. Sandeep Chaudhari
 *********************************************************************/
void * __stdcall XmlMalloc( size_t size )
{
    /* For user mode redefine in terms of malloc.
     */
    return malloc(size);
}
/**************************************
 * end XmlMalloc()
 **************************************/

/*********************************************************************
 * @var XmlCalloc
 *********************************************************************
 * @brief Redefined function needed by the Expat library.
 *        This function allocates a block of memory for an array of 
 *        num elements, each of them size bytes long, and initializes 
 *        all its bits to zero.
 *
 *  @param   num  - Number of elementes to be allocated.
 *  @param   size - Size of element.
 *
 * @return - void*
 *
 * @author March 01, 2011 - Created. Sandeep Chaudhari
 *********************************************************************/
void * __stdcall XmlCalloc( size_t num, size_t size )
{
    /* For user mode redefine in terms of XmlMalloc and memset.
     */
    void *ptr = XmlMalloc(size * num);

    if(NULL != ptr)
    {
        memset( ptr, 0, (size * num)); 
    }
    return ptr; 
}
/**************************************
 * end XmlCalloc()
 **************************************/

/*********************************************************************
 * @var XmlFree
 *********************************************************************
 * @brief Redefined function needed by the Expat library.
 *        This function deallocate a block of memory previously 
 *        allocated using a call to malloc, calloc or realloc.
 *
 *  @param   memblock - Pointer to a memory block previously allocated
 *                      with malloc, calloc or realloc to be deallocated.
 *
 * @return - void*
 *
 * @author March 01, 2011 - Created. Sandeep Chaudhari
 *********************************************************************/
void __stdcall XmlFree( void *memblock )
{
    if (memblock == NULL)
    {
        return;
    }
    else
    {
        free(memblock);
    }
    return;
}
/**************************************
 * end XmlFree()
 **************************************/

/***************************************************
 * Define the interface for the expat library to
 * use the statically linked Expat Library.
 ***************************************************/

/*********************************************************************
 * @var ExpatParserCreate
 *********************************************************************
 * @brief  This function calls XML_ParserCreate.
 *         XML_ParserCreate constructs a new parser.
 *
 *  @param   pEncoding - pEncoding is the encoding specified by the external
 *                       protocol or null if there is none specified.
 *
 * @return - XML_Parser
 *
 * @author March 08, 2011 - Created. Sandeep Chaudhari
 *********************************************************************/
XML_Parser  ExpatParserCreate ( const XML_Char *pEncoding )
{
    return XML_ParserCreate(pEncoding);
}
/**************************************
 * end ExpatParserCreate()
 **************************************/

/*********************************************************************
 * @var ExpatSetUserData
 *********************************************************************
 * @brief  This function calls XML_SetUserData to set user data for
 *         expat.
 *
 *  @param   pParser -  This is pointer to constructed parser.
 *           pUserData- User data for expat.
 *
 * @return - void
 *
 * @author March 08, 2011 - Created. Sandeep Chaudhari
 *********************************************************************/
void  ExpatSetUserData ( XML_Parser pParser, void *pUserData )
{
    XML_SetUserData(pParser, pUserData);
    return;
}
/**************************************
 * end ExpatSetUserData()
 **************************************/

/*********************************************************************
 * @var ExpatSetElementHandler
 *********************************************************************
 * @brief  This function calls XML_SetElementHandler which set 
 *         callbacks for expat - they will call handlers
 *         initialized before.
 *
 *  @param   pParser-This is pointer to constructed parser.
 *           start - This is pointer to function which is passed
 *                   to expat.
 *           end   - This is pointer to function which is passed
 *                   to expat.
 *
 * @return - void
 *
 * @author March 08, 2011 - Created. Sandeep Chaudhari
 *********************************************************************/
void ExpatSetElementHandler ( XML_Parser pParser,
                              XML_StartElementHandler start,
                              XML_EndElementHandler end )
{
    XML_SetElementHandler(pParser, start, end);
    return;
}
/**************************************
 * end ExpatSetElementHandler()
 **************************************/

/*********************************************************************
 * @var ExpatSetCharacterDataHandler
 *********************************************************************
 * @brief  This function calls XML_SetCharacterDataHandler which set 
 *         callback for expat data handler - this will call handler
 *         initialized before.
 *
 *  @param   pParser - This is pointer to constructed parser.
 *
 * @return - void
 *
 * @author March 08, 2011 - Created. Sandeep Chaudhari
 *********************************************************************/
void ExpatSetCharacterDataHandler ( XML_Parser pParser, XML_CharacterDataHandler handler )
{
    XML_SetCharacterDataHandler(pParser, handler);
    return;
}
/**************************************
 * end ExpatSetCharacterDataHandler()
 **************************************/

/*********************************************************************
 * @var ExpatParse
 *********************************************************************
 * @brief  This function calls XML_Parse to parse input. Returns 0 if 
 *         a fatal error is detected.
 *
 *  @param   pParser - This is pointer to constructed parser.
 *           s       - Pointer to constant string which is to be parsed.
 *           len     - Len may be zero for this call
 *           isFinal - The last call to XML_Parse must have isFinal true.
 *
 * @return - fbe_s32_t
 *
 * @author March 08, 2011 - Created. Sandeep Chaudhari
 *********************************************************************/
fbe_s32_t ExpatParse ( XML_Parser pParser, 
                 const char *s, 
                 fbe_s32_t len, 
                 fbe_s32_t isFinal )
{
    return XML_Parse(pParser, s, len, isFinal);
}
/**************************************
 * end ExpatParse()
 **************************************/

/*********************************************************************
 * @var ExpatGetCurrentLineNumber
 *********************************************************************
 * @brief  This function calls XML_GetCurrentLineNumber to get line
 *         number for current parse location.
 *
 *  @param   This is pointer to constructed parser.
 *
 * @return - fbe_s32_t
 *
 * @author March 08, 2011 - Created. Sandeep Chaudhari
 *********************************************************************/
fbe_s32_t ExpatGetCurrentLineNumber ( XML_Parser pParser )
{
    return XML_GetCurrentLineNumber(pParser);
}
/**************************************
 * end ExpatGetCurrentLineNumber()
 **************************************/

/*********************************************************************
 * @var ExpatGetErrorCode
 *********************************************************************
 * @brief  This function calls XML_GetErrorCode to get information 
 *         about the error.
 *
 *  @param   pParser - This is pointer to constructed parser.
 *
 * @return - XML_Error
 *
 * @author March 08, 2011 - Created. Sandeep Chaudhari
 *********************************************************************/
enum XML_Error  ExpatGetErrorCode( XML_Parser pParser )
{
    return XML_GetErrorCode(pParser) ;
}
/**************************************
 * end ExpatGetErrorCode()
 **************************************/

/*********************************************************************
 * @var ExpatErrorString
 *********************************************************************
 * @brief  This function calls XML_ErrorString to get string 
 *         describing the error.
 *
 *  @param   code - Error Code
 *
 * @return - PXML_LChar
 *
 * @author March 08, 2011 - Created. Sandeep Chaudhari
 *********************************************************************/
PXML_LChar ExpatErrorString ( fbe_s32_t code )
{
    return XML_ErrorString(code);
}
/**************************************
 * end ExpatErrorString()
 **************************************/

/*********************************************************************
 * @var ExpatParserFree
 *********************************************************************
 * @brief  This function calls XML_ParserFree to frees memory used 
 *         by the parser.
 *
 *  @param   pParser - This is pointer to constructed parser.
 *
 * @return - void
 *
 * @author March 08, 2011 - Created. Sandeep Chaudhari
 *********************************************************************/
void ExpatParserFree ( XML_Parser pParser )
{
    XML_ParserFree(pParser);
    return;
}
/**************************************
 * end ExpatParserFree()
 **************************************/

/*********************************************************************
 * @var ExpatSetUserData
 *********************************************************************
 * @brief  As of now this function do not do anything but return status.
 *
 *  @param   void
 *
 * @return - NTSTATUS
 *
 * @author March 08, 2011 - Created. Sandeep Chaudhari
 *********************************************************************/
EMCPAL_STATUS ExpatInitialize( void )
{
    /* As of now we do not do anything but return status.
     */

    return EMCPAL_STATUS_SUCCESS;
}
/**************************************
 * end ExpatInitialize()
 **************************************/

/**********************************************************************
 * fbe_xml_parse_file() 
 **********************************************************************
 *
 * DESCRIPTION:
 *  This function parses the provided XML file. File handlers are
 *  invoked and EXPAT parser is initialized. Then the configuration
 *  XML file is parsed. 
 *
 * PARAMETERS:
 *  pwFilename,   [I] - File to parse.
 *  start_handler, [I] - Function for Expat to use as callback.
 *  end_handler, [I] - Function for Expat to use as callback.
 *  data_handler, [I] - Function for Expat to use as callback.
 *
 * ASSUMPTIONS:
 *  None
 *  
 * RETURNS:   
 *   FBE_XML_MEMORY_ALLOCATION_ERROR- failed to allocate memory
 *   FBE_XML_FILE_READ_ERROR - not able to find the file or read it
 *   FBE_XML_PARSE_ERROR - error parsing the file
 *   FBE_XML_SUCCESS - no errors
 * 
 * HISTORY:
 *  17 Jan 2011 - Created. Sandeep Chaudhari
 **********************************************************************/

fbe_u32_t fbe_xml_parse_file(fbe_char_t *pwFilename,
                          XML_StartElementHandler start_handler,
                          XML_EndElementHandler end_handler,
                          XML_CharacterDataHandler data_handler )
{
    VOID *xml_parser;         /* Parser used by expat. */ 
    fbe_u8_t *buffer_p = NULL; /* Buffer to read parse data into. */
    fbe_bool_t done = FALSE;   /* Flag to check on the parsing progress. */
    fbe_file_handle_t file_handle;
    fbe_u32_t bytesRead;

    file_handle = fbe_file_open(pwFilename, FBE_FILE_RDWR, 0, NULL);
     

    if(file_handle == FBE_FILE_INVALID_HANDLE)
    {
        /* Failure opening file, return a read error.
         */
        return FBE_XML_FILE_READ_ERROR;
    }

    /* Initialize the EXPAT parser and set the user implemented callback 
     * functions to point to the EXPAT Callback.
     */
    ExpatInitialize(); 
    xml_parser = (XML_Parser) ExpatParserCreate(NULL);

    /* Set the callback functions for EXPAT handlers.
     */
    ExpatSetElementHandler( (XML_Parser)xml_parser, 
                            start_handler, 
                            end_handler);
    
    ExpatSetCharacterDataHandler( (XML_Parser)xml_parser, 
                                  data_handler);

    /* Initialize the buffer that is needed to read the XML file.
     */
    buffer_p = (fbe_u8_t*) malloc( sizeof(fbe_u8_t) * FBE_XML_BUFFER_BYTES);
    
    if (buffer_p == NULL)
    {
        /* Free the parser here since we will be returning.
         */
        ExpatParserFree( (XML_Parser)xml_parser ); 
        
        /* Close the file handle. 
         */
        fbe_file_close(file_handle);
        return FBE_XML_MEMORY_ALLOCATION_ERROR;
    }

    /* Now that the EXPAT is initialized and file is all set to be read,
     * start parsing the file.
     */
    while(!done)
    {
        bytesRead = fbe_file_read(file_handle, buffer_p, FBE_XML_BUFFER_BYTES, NULL);

        if (bytesRead < FBE_XML_BUFFER_BYTES)
        {
            /* We reached the end of the file, set done so we exit loop.
             */
           done = TRUE;
        }
        
        /* Parse the data we just read in from file.
         */
        if( !ExpatParse((XML_Parser)xml_parser, 
                        (fbe_u8_t*)buffer_p, 
                        (fbe_s32_t)bytesRead, 
                        done) )
        {
            /* An error was encountered while parsing.
             */
            printf("Error parsing XML file: %s at line %d\n", 
                    ExpatErrorString(ExpatGetErrorCode(
                                        (XML_Parser)xml_parser)),
                    ExpatGetCurrentLineNumber(
                                        (XML_Parser)xml_parser) );

            /* Free the parser, memory and close file handle,
             * since we will be returning here.
             */
            ExpatParserFree( (XML_Parser)xml_parser ); 
            free(buffer_p);
            fbe_file_close(file_handle);
            return FBE_XML_PARSE_ERROR;
            
        }/* End if parsing error. */

    } /* End while not done. */

    /* Free the parser, memory and close file handle,
     * since we are done.
     */
    ExpatParserFree( (XML_Parser)xml_parser );
    free(buffer_p);
    fbe_file_close(file_handle);
    return FBE_XML_SUCCESS;
}

/**********************************************
 * end of fbe_xml_parse_file
 *********************************************/


/**********************************************************************
 * fbe_xml_save_file() 
 **********************************************************************
 *
 * Description:
 *  Save an XML file.
 *  We open the file, write the header, call the save_func 
 *  callback to write the user's data, write the footer,
 *  and then close the file.
 *
 * PARAMETERS:
 *  pfilename, [I] - Filename to write the configuration to.
 *  header_p, [I] - Ptr to header to write to file, NULL if no
 *                  header should be written.
 *  footer_p, [I] - Ptr to footer to write to file, NULL if no
 *                  footer should be written.
 *  safe_func, [I] - Ptr to callback function to use to write the
 *                   configuration file.  The caller sets this up
 *                   so that it can write out caller specific
 *                   information after the file is opened.
 *                   NULL means no callback necessary.
 *
 * ASSUMPTIONS:
 *  None
 *
 * RETURNS:
 *   FBE_XML_FILE_WRITE_ERROR - If an error encountered writing the file.
 *   FBE_XML_SUCCESS - If writing is successful.
 *
 * History:
 *  05/10/2012  Created. kothal
 **********************************************************************/
fbe_u32_t fbe_xml_save_file(fbe_char_t* pfilename, const char *const header_p, const char *const footer_p, FBE_XML_ERROR_INFO (*save_func)(HANDLE handle, UCHAR *buffer_p) )
{	  
    fbe_file_handle_t file_handle;          /* File handle. */
    fbe_u8_t *buffer_p = NULL;      /* Buffer for writing to file. */
    FBE_XML_ERROR_INFO wr_status = FBE_XML_SUCCESS; /* Status of the write operation. */    
      
    file_handle = fbe_file_open(pfilename, FBE_FILE_RDWR|FBE_FILE_CREAT, 0,
                                NULL);
	
    if(file_handle == FBE_FILE_INVALID_HANDLE)
    {      
         /* Failure creating file, return error */
         printf( "%s Could not create file.\n", __FUNCTION__); 
         return FBE_XML_FILE_WRITE_ERROR;      
    }        
	
	/* Initialize the buffer that is needed to read the XML file.
	*/
    buffer_p = (fbe_u8_t*) malloc( sizeof(fbe_u8_t) * FBE_XML_BUFFER_BYTES);
    
    if (buffer_p == NULL)
    {        
        /* Close the file handle. 
         */
        fbe_file_close(file_handle);      
        return FBE_XML_MEMORY_ALLOCATION_ERROR;
    }
    
    if ( header_p != NULL )
    {
        /* Write the XML header into the file if we have one. 
         */    
        strcpy(buffer_p, header_p);
        
        /* Write the header into the file. 
         */
        wr_status = fbe_xml_write_buffer(file_handle, buffer_p);
    }
    
    if (wr_status == FBE_XML_SUCCESS && save_func != NULL )
    {
        /* Call the save records callback provided by the caller.
         * This allows the caller to save anything they wish to the file.
         * We assume they will not close the file.
         */
        wr_status = save_func(file_handle, buffer_p);
    }
    
    if ( wr_status == FBE_XML_SUCCESS && footer_p != NULL )
    {
        /* If we have a footer, write it now.
         */
        strcpy(buffer_p, footer_p);
        
        /* Write the closing tag to the file. 
         */
        wr_status = fbe_xml_write_buffer(file_handle, buffer_p);
    }
    
	/* Close the file handle, free memory and report status.
	*/

    free(buffer_p);
    fbe_file_close(file_handle); 
    return FBE_XML_SUCCESS;
}
/**********************************************************************
 * fbe_xml_save_file() 
 **********************************************************************/
/**********************************************************************
 * fl_xml_write_buffer()
 **********************************************************************
 *
 * Description:
 *  Write a single buffer to a given file handle.
 *
 * PARAMETERS:
 *  handle, [I] - Handle of file we are writing to.  Assumption is that
 *                this file was already opened and exists.
 *
 *  buffer_p, [I] - Buffer that we will write out to this file.
 *
 * ASSUMPTIONS:
 *  File is open, buffer contains a non zero length string.
 *
 * RETURNS:
 *   FBE_XML_FILE_WRITE_ERROR - If an error encountered writing the file.
 *   FBE_XML_SUCCESS - If writing is successful.
 * 
 * History:
 *  05/10/2012  Created. kothal
 **********************************************************************/
FBE_XML_ERROR_INFO fbe_xml_write_buffer( const fbe_file_handle_t handle,
                                        fbe_u8_t * buffer_p )
{    
    fbe_u32_t bytesWritten;

    /* Write out the buffer.
     * If an error occurs, then display the write error,
     * and return FBE_XML_WRITE_ERROR.
     */
	
    bytesWritten = fbe_file_write(handle, buffer_p,(unsigned int)strlen(buffer_p), NULL );
    
	/* We expect to read and write the number of bytes EXACTLY as it is in the fixed structure size */
    if((bytesWritten == 0) || (bytesWritten == FBE_FILE_ERROR))
    {
        printf("Log File Write Error: %d\n", bytesWritten);
        return FBE_XML_FILE_WRITE_ERROR;
    }
    return FBE_XML_SUCCESS;
}
/**************************************************
 * end fbe_xml_write_buffer()
 **************************************************/
