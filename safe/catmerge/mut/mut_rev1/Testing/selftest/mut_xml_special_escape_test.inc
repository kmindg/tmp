/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_xml_special_escape_test.inc
 ***************************************************************************
 *
 * DESCRIPTION: MUT special xml character escape test definition
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    6/12/2009   Bo Gao(gaob3) initial version
 *
 **************************************************************************/
//MUT_XML_SPECIAL_ESCAPE("&", "&amp;", and)
//MUT_XML_SPECIAL_ESCAPE(">", "&gt;", greater)
//MUT_XML_SPECIAL_ESCAPE("<", "&lt;", less)
//MUT_XML_SPECIAL_ESCAPE("\"", "&quot;", double_quote)
//MUT_XML_SPECIAL_ESCAPE("'", "&apos;", single_quote)

#undef MUT_XML_SPECIAL_ESCAPE
