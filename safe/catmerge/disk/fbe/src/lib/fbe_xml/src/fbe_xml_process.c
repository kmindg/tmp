/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                xml_process.c
 ***************************************************************************
 *
 * DESCRIPTION: XML process functionality
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    09/29/2008   Alexander Alanov (alanoa) initial version
 *
 **************************************************************************/

#include <stdio.h>

//#include "k10ntddk.h"
//#undef XMLPARSEAPI
//#define XMLPARSEAPI __stdcall
//#include "..\..\..\..\..\..\layered\Generic\krnl\interface\xmlparse.h"

//#include "..\..\..\..\..\..\layered\Generic\krnl\interface\ExpatDllApi.h"
#include "fbe_xml_api.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_file.h"
//#include "xmlrole.h"
//#include "xmldef.h"
//#include "xmltok.h"
#include "xml_field.h"
#include "xml_process.h"
#include "fbe_xml_util.h"

#include <stdio.h>
//#include <ctype.h>
#include <stdlib.h>
#include <string.h>
//#include "../expat/interface/xmlparse.h"
//#include "../expat/interface/xmlrole.h"
//#include "../expat/interface/xmldef.h"
//#include "../expat/interface/xmltok.h"

/*
    These expat_* functions are handlers passed to expat. They are used to
    decorate handlers that are defined in xml_process. If some handler in
    xml_process is NULL, these functions don't call it. Also you can change
    any handler field in xml_process structure without registering it again.
*/

//extern size_t strlen(const char * s);
//extern char * strdup(const char * s);

static void expat_start_element(void *user_data, const char *name, const char **atts)
{
    xml_process * xp = (xml_process *)user_data;
    if(xp != NULL && xp->start_element != NULL)
    {
        xp->start_element(xp, name, atts);
    }
}

static void expat_end_element(void *user_data, const char *name)
{
    xml_process * xp = (xml_process *)user_data;
    if(xp != NULL && xp->end_element != NULL)
    {
        xp->end_element(xp, name);
    }
}

static void expat_data_handler(void * user_data, const char * data, int len)
{
    xml_process * xp = (xml_process *)user_data;
    if(xp != NULL && xp->characters != NULL)
    {
        xp->characters(xp, data, len);
    }
}

/* default error handler to parse() function */
static void xp_default_error_handler(void * xp_ptr, int line_number, int code, const char * description)
{
    KvTrace("Error parsing file %s at line %u: %s (code %u)\n",
        ((xml_process *)xp_ptr)->processed_file,
        line_number, description,
        code);
    return;
}

/* Empty handlers for allocated xml_field structures*/
/* empty allocate() */
static void * RETURN_NULL(void * field, void * argument)
{
    return NULL;
}


/*  Default handler for character data. Its default action is to
    append received data to the current_value field of xml_process
    it was called from. This field will be stored in appropriate
    xml_field structure using its store() method
*/
static void dataHandler(void * userData, const char * data, int len)
{
    xml_process * xp = (xml_process *)userData;
    if(data == NULL || len == 0) return; /* nothing to do */
    if(xp->current_value == NULL)
    {
        xp->current_value = (char *)fbe_api_allocate_memory(len + 1);
        fbe_copy_memory(xp->current_value, data, len);
        (xp->current_value)[len] = '\0';
    }
    else
    {
        int current_len = (int)strlen(xp->current_value);
        int new_len = current_len + len;
        char * new_value = fbe_reallocate_memory(xp->current_value, current_len, new_len  + 1);
        if(new_value == NULL)
        {
            xp->error(xp, -1, XML_ERROR_NO_MEMORY, "Could not allocate memory for character data");
        }
        else
        {
            /* append data and terminating null character to current_value */
            fbe_copy_memory(new_value + current_len, data, len);
            new_value[new_len] = '\0';
            xp->current_value = new_value;
        }
    }
    return;
}

/*  copies attributes and their values from attr - NULL-terminated array of
    zero-terminated strings to the xml_field f, and sets the proper value
    of attributes counter
*/
static void set_xml_field_attributes(xml_field * f, const char ** attr)
{
    int i = 0;
    if(attr == NULL) return;
    if(*attr == NULL) return;
    while(*(attr + i)) i++;
    f->attributes = (char **)fbe_api_allocate_memory((i + 1) * sizeof(char *));
    if(f->attributes == NULL) return;
    f->attributes_counter = i / 2;
    for(i = 0; i < 2*(f->attributes_counter); i++)
    {
        f->attributes[i] = fbe_strdup(attr[i]);
    }
    f->attributes[i] = NULL;
}

/*  Defauld handler for new element
    It will create new xml_field, initialize all its handlers using parent's,
    add new field to parent's subfields, call its handler for attributes (if
    there are any attributes), store chunk of collected character data in
    parent xml_field and deallocate memory used for them.
*/
static void startElement(void *userData, const char *name, const char **atts)
{
    xml_process * xp = (xml_process *)userData;
    /*
        If current node is NULL, it means that we are in the root of xml document
        and user has not provided any root field, and handlers for store() and
        allocate(), so the default handlers would be used (they actually do nothing)
    */
    if(xp->current_node == NULL)
    {
        xp->current_node = create_xml_field(name, 0);
        /* specify required callback functions */
        xp->current_node->allocate = RETURN_NULL;
    }
    else
    {
        /* create new node */
        xml_field * new_node = create_xml_field(name,
            xp->current_node->report_missing_fields);
        /* copy handlers from parent node to child node */
        new_node->allocate = xp->current_node->allocate;
        new_node->end_element = xp->current_node->end_element;
        new_node->exception_handler = xp->current_node->exception_handler;
        new_node->store = xp->current_node->store;
        new_node->process_attributes = xp->current_node->process_attributes;
        new_node->finalize = xp->current_node->finalize;
        new_node->warning_handler = xp->current_node->warning_handler;
        /* */
        add_subfield(xp->current_node, new_node);
        /* store parent character data that has already been collected */
        if(xp->current_value != NULL)
        {
            /*should we keep all character data in this field?*/
            xp->current_node->store(xp->current_node, NULL, xp->current_value);
            fbe_api_free_memory(xp->current_value);
            xp->current_value = NULL;
        }
        xp->current_node = new_node;
    }
    /* process attributes */
    if(atts != NULL)
    {
        set_xml_field_attributes(xp->current_node, atts);
        xp->current_node->process_attributes(xp->current_node, NULL, atts);
    }
}

/*  Default handler for end of element
    This function will call store() method to store all collected character data
    in current xml_field, and deallocate memory used to store this data in xml_process
*/
static void endElement(void *userData, const char *name)
{
    xml_process * xp = (xml_process *)userData;
    if(xp->current_value != NULL)
    {
        if(xp->current_node != NULL)
            xp->current_node->store(xp->current_node, NULL, xp->current_value);
        fbe_api_free_memory(xp->current_value);
        xp->current_value = NULL;
    }
    if(xp->current_node != NULL)
    {
        xml_field * parent = xp->current_node->parent;
        xp->current_node->end_element(xp->current_node, NULL, NULL);
        /* if we are not in the root */
        if(parent != NULL)
            xp->current_node = parent;

    }
}


/* creates and dynamically initializes a new xml_process structure */
xml_process * create_xml_process(void)
{
    xml_process * new_xp = (xml_process *)fbe_api_allocate_memory(sizeof(xml_process));
    new_xp->parser = fbe_parser_create(NULL);
    new_xp->current_value = NULL;
    /* set default handlers for supported events */
    new_xp->characters = dataHandler;
    new_xp->start_element = startElement;
    new_xp->end_element = endElement;
    new_xp->error = xp_default_error_handler;
    /*
        set user data for expat - this will be pointer to xml_process structure,
        which is being created, and this is needed to pass information about parser
        into handlers (something like "this" pointer in C++)
    */
    fbe_set_user_data(new_xp->parser, new_xp);
    /* set callbacks for expat - they will call handlers initialized before */
    fbe_set_element_handler(new_xp->parser, expat_start_element, expat_end_element);
    fbe_set_character_data_handler(new_xp->parser, expat_data_handler);
    /*  current node, application can change it to provide its own allocate() and
        store() procedures
    */
    new_xp->current_node = NULL;
    return new_xp;
}

/* parses file specified by path, using parser specified in structure pointed by xp*/
/* return a root of document tree, represented as tree of xml_fields */
xml_field * parse(xml_process * xp, const char * path)
{
    fbe_file_handle_t fp = NULL;
    xml_field * result;
    if(xp == NULL || path == NULL) return NULL;
    fp = fbe_file_open(path, FBE_FILE_RDONLY, 0, NULL);
    if(FBE_FILE_INVALID_HANDLE == fp) return NULL;
    /* store the processed file for call-back functions */
    xp->processed_file = (char *)path;
    result = parse_stream(xp, (FILE*)fp);
    xp->processed_file = NULL;
    return result;
}

/* parses stream specified by stream, using parser specified in structure pointed by xp*/
/* return a root of document tree, represented as tree of xml_fields */
xml_field * parse_stream(xml_process * xp, FILE* stream)
{
    int done = 0;
    if(xp == NULL || stream == NULL) return NULL;
    do
    {
        char buffer[513];
        size_t length = fbe_file_read((fbe_file_handle_t)stream, buffer, (sizeof(buffer)-1), NULL);
        if((int)length < 0 )
        {
            return NULL;
        }
        done = (length < (sizeof(buffer)-1)) ? 1 : 0;
        buffer[length]= 0;
        if (!fbe_expat_parse(xp->parser, buffer, (int)length, done))
        {
            int error_code = fbe_get_error_code(xp->parser);
            xp->error(xp, fbe_get_current_line_number(xp->parser),
                error_code, fbe_error_string(error_code));
            /*
            To prevent memory leaks, we must ensure that after error
            current_node points to the root of the already parsed tree,
            so that delete_xml_process with keep_tree = 0 would be able
            to delete it from memory. To do that we have to go to the
            root of the tree.
            */
            if(xp->current_node != NULL && xp->current_node->parent != NULL)
            {
                /*  If current node is NULL, no other node was allocated.
                    It is important that user-specified root has parent = NULL,
                    in other case our tree is subtree of some other tree which
                    would be deleted when user calls to delete_xml_process with
                    keep_tree = 0;
                */
                do
                {
                    xp->current_node = xp->current_node->parent;
                }
                while(xp->current_node->parent != NULL);
            }
            return NULL;
        }
    }
    while (!done);
    return xp->current_node;
}
/*  Destroy xml_process which was previously created by create_xml_process
    If keep_tree = 0, this method also will destroy tree of nodes, otherwise
    it would keep it in memory, and user will have to destroy it manually
    using delete_xml_field to prevent memory leaks.
*/
void delete_xml_process(xml_process * xp, int keep_tree)
{
    if(xp == NULL) return;
    fbe_parser_free(xp->parser);
    if(!keep_tree && xp->current_node != NULL) delete_xml_field(xp->current_node);
    fbe_api_free_memory(xp);
}

