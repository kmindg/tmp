#ifndef FBE_XML_API
#define FBE_XML_API

/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_xml_api.h
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

#undef XMLPARSEAPI
#define XMLPARSEAPI __stdcall
#include "../../../layered/Generic/krnl/interface/xmlparse.h"


XML_Parser fbe_parser_create( const XML_Char *encoding );
void fbe_set_user_data( XML_Parser parser, void *userData );
void fbe_set_element_handler( XML_Parser parser, XML_StartElementHandler start, XML_EndElementHandler end );
void fbe_set_character_data_handler( XML_Parser parser, XML_CharacterDataHandler handler );
int fbe_expat_parse( XML_Parser parser, const char *s, int len, int isFinal );
enum XML_Error fbe_get_error_code( XML_Parser parser );
PXML_LChar fbe_error_string (int code);
int fbe_get_current_line_number ( XML_Parser parser );
void fbe_parser_free ( XML_Parser parser );

#endif /* FBE_XML_API */
