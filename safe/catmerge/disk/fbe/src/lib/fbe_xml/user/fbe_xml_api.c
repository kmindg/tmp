/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_xml_api.c
 ***************************************************************************
 *
 *  Description
 *      Provides a user/kernel mode abstraction for the Expat library.
 *
 *
 *  History:
 *      05/19/09    tayloj5 - Created
 *
 ***************************************************************************/

#include "fbe_xml_api.h"

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

