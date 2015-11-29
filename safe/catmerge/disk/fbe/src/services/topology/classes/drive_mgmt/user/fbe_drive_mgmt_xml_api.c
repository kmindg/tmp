/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_drive_mgmt_xml_api.c
 ***************************************************************************
 *
 *  Description
 *      Provides a user/kernel mode abstraction for the Expat library.
 *
 *
 *  History:
 *      03/20/12    Wayne Garrett - Copied from fbe_xml_api.c
 *
 ***************************************************************************/
#include <stdlib.h>
#include <string.h>
#include "fbe_drive_mgmt_xml_api.h"

/*TODO:  User fbe_xml lib instead of reimplementing the following */

XML_Parser  fbe_parser_create ( const XML_Char *pEncoding )
{
    return XML_ParserCreate(pEncoding);
}

void  fbe_set_user_data ( XML_Parser pParser, void *pUserData )
{
    XML_SetUserData(pParser, pUserData);
}

void fbe_set_element_handler ( XML_Parser pParser,
                              XML_StartElementHandler start,
                              XML_EndElementHandler end )
{
    XML_SetElementHandler(pParser, start, end);
}

void fbe_set_character_data_handler ( XML_Parser pParser,
                                                           XML_CharacterDataHandler handler )
{
    XML_SetCharacterDataHandler(pParser, handler);
}

int fbe_expat_parse ( XML_Parser pParser,
                 const char *s,
                 int len,
                 int isFinal )
{
    return XML_Parse(pParser, s, len, isFinal);
}

int fbe_get_current_line_number ( XML_Parser pParser )
{
    return XML_GetCurrentLineNumber(pParser);
}


enum XML_Error  fbe_get_error_code( XML_Parser pParser )
{
    return XML_GetErrorCode(pParser) ;
}

PXML_LChar fbe_error_string ( int code )
{
    return XML_ErrorString(code);
}

void fbe_parser_free ( XML_Parser pParser )
{
    XML_ParserFree(pParser);
}


/* The following was copied from user/fbe_xml_mem.c*/

/***************************************************
 * Define functions needed by the Expat library.
 * Since this is simulation and user mode,
 * we need to define these functions in terms of
 * available functions like malloc and free.
 ***************************************************/

void __stdcall XmlLogTrace(char *LogString, int arg1, int arg2)
{
    /* For user mode, this currently does nothing.
     */
    return;
}

void * __stdcall XmlMalloc( size_t size )
{
    /* For user mode redefine in terms of malloc.
     */
    return malloc(size);
}

void * __stdcall XmlCalloc( size_t num, size_t size )
{
    /* For user mode redefine in terms of XmlMalloc and memset.
     */
    void *ptr = XmlMalloc(size * num);
    memset( ptr, 0, (size * num));
    return ptr;
}
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

