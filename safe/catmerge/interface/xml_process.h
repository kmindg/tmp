/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                xml_process.h
 ***************************************************************************
 *
 * DESCRIPTION: Header for XML process functionality
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    09/29/2008   Alexander Alanov (alanoa) initial version
 *
 **************************************************************************/

#ifndef XML_PROCESS_INCLUDED
#define XML_PROCESS_INCLUDED 1
#include "xml_field.h"

#ifdef __cplusplus
extern "C" {
#endif

/*  From expat's xmlparse.h */

typedef void * expat_xml_parser;

/*  First argument for all callback functions of following types is the pointer to xml_process 
    structure describing the context they are calling from. This is something like C++ this pointer.*/
/*  Called when expat returns an error. line_number indicates a line where it have occured, error_code is
    Expat code for the error, and error_description is brief null-terminated string describing the error.
    The description is also obtained from Expat. */
typedef void (* xp_error_handler)(void * xp_ptr, int line_number, int error_code, const char * error_description);
/*  Handler for beginning of an element of xml. name is null-terminated name of element, attributes
    is NULL-terminated array of null-terminated strings. The structure of attributes is following:
    {attribute1, value1, attribute2, value2, ..., attributeN, valueN, NULL}
*/
typedef void (* xp_start_element_handler)(void * xp_ptr, const char * name, const char ** attributes);
/*  Handler for end of element. name is the name of finished element */
typedef void (* xp_end_element_handler)(void * xp_ptr, const char * name);
/*  Handler for character data. data is array of characters of specified in third argument lenght.
    Take note that data is NOT null-terminated */
typedef void (* xp_character_data_handler)(void * xp_ptr, const char * data, int length);

typedef struct tag_xml_process
{
    expat_xml_parser parser;    /* Expat parser used by this process*/
    void * xml_object;          /* unused member, reserved for future use */
    xml_field * current_node;   /* current field being processed */
    char * current_value;       /* chunk of character data received before storing it in appropriate field */
    char * processed_file;      /* file being processed */
    xp_error_handler error;     /* error handler*/
    xp_start_element_handler start_element; /* start element handler */
    xp_end_element_handler end_element;     /* end element hanlder */
    xp_character_data_handler characters;   /* handler for character data */
} xml_process;

/* creates and dynamically initializes a new xml_process structure */
xml_process * create_xml_process(void);

/* parses file specified by path, using parser specified in structure pointed by xp*/
/* return a root of document tree, represented as tree of xml_fields */
xml_field * parse(xml_process * xp, const char * path);

/* parses stream specified by stream, using parser specified in structure pointed by xp*/
/* return a root of document tree, represented as tree of xml_fields */
xml_field * parse_stream(xml_process * xp, FILE * stream);

/*  Destroy xml_process which was previously created by create_xml_process
    If keep_tree = 0, this method also will destroy tree of nodes, otherwise
    it would keep it in memory, and user will have to destroy it manually 
    using delete_xml_field to prevent memory leaks. 
*/
void delete_xml_process(xml_process * xp, int keep_tree);

#ifdef __cplusplus
}
#endif

#endif
