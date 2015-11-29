#ifndef FBE_CLI_LIB_EXPAT_H
#define FBE_CLI_LIB_EXPAT_H

/*************************
 ** INCLUDE FILES
 *************************/
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "generics.h"
#include "fbe/fbe_xmlparse.h"
#include "fbe_api_physical_drive_interface.h"
#include "fbe_file.h"
#include "fbe_cli_lurg.h"

#define FBE_XML_BUFFER_BYTES       2048
#define SUCCESS 1

/**************************************
 * FBE_XML_ERROR_INFO Error definitions
 *************************************/
typedef enum FBE_XML_ERROR_INFO
{
    FBE_XML_SUCCESS = SUCCESS,
    
    /* Generic failure error. 
     */
    FBE_XML_FAILURE,

    /* This error is used to denote error in parsing XML file.
     */
    FBE_XML_PARSE_ERROR,

    /* This is error return for failure on reading a file.
     */
    FBE_XML_FILE_READ_ERROR, 

    /* Returns the following when there is a error in writing to file.
     */
    FBE_XML_FILE_WRITE_ERROR,  

    /* When there is a memory allocation error, the following error
     * is returned. 
     */
    FBE_XML_MEMORY_ALLOCATION_ERROR, 
}FBE_XML_ERROR_INFO;

void __stdcall XmlLogTrace(char *LogString, fbe_s32_t arg1, fbe_s32_t arg2);
void * __stdcall XmlMalloc( size_t size );
void * __stdcall XmlCalloc( size_t num, size_t size );
void __stdcall XmlFree( void *memblock );
XML_Parser  ExpatParserCreate ( const XML_Char *pEncoding );
void  ExpatSetUserData ( XML_Parser pParser, void *pUserData );
void ExpatSetElementHandler ( XML_Parser pParser,
                              XML_StartElementHandler start,
                              XML_EndElementHandler end );
void ExpatSetCharacterDataHandler ( XML_Parser pParser, XML_CharacterDataHandler handler );
fbe_s32_t ExpatParse ( XML_Parser pParser, 
					   const char *s, 
					   fbe_s32_t len, 
					   fbe_s32_t isFinal );
fbe_s32_t ExpatGetCurrentLineNumber ( XML_Parser pParser );
enum XML_Error  ExpatGetErrorCode( XML_Parser pParser );
PXML_LChar ExpatErrorString ( fbe_s32_t code );
void ExpatParserFree ( XML_Parser pParser );
EMCPAL_STATUS ExpatInitialize( void );

fbe_u32_t fbe_xml_parse_file(fbe_char_t* pwFilename, XML_StartElementHandler start_handler, XML_EndElementHandler end_handler, XML_CharacterDataHandler data_handler );

fbe_u32_t fbe_xml_save_file(fbe_char_t* pfilename, const char *const header_p, const char *const footer_p, FBE_XML_ERROR_INFO (*save_func)(HANDLE handle, UCHAR *buffer_p) );

FBE_XML_ERROR_INFO fbe_xml_write_buffer( const fbe_file_handle_t handle, fbe_u8_t * buffer_p );

#endif /* end __FBE_CLI_LIB_EXPAT_H__ */
