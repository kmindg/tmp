/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                xml_field.c
 ***************************************************************************
 *
 * DESCRIPTION: XML field functionality
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    09/29/2008   Alexander Alanov (alanoa) initial version
 *
 **************************************************************************/

#include "fbe/fbe_winddk.h"
#include <stdio.h>
//#include <ctype.h>
#include <stdlib.h>
//#include <limits.h>
#include <string.h>
#include "xml_field.h"
#include "fbe_xml_util.h"
#include "fbe/fbe_ctype.h"

//extern size_t strlen(const char * s);
//extern char * strdup(const char * s);
//extern int strncmp ( const char * str1, const char * str2, size_t num );

/* Attributes default handler */
static void process_attributes(void * field, void * object, const char ** attributes)
{
    /* do nothing */
}

/* end of element default handler*/
static void end_element(void * field, void * object, void * element)
{
    /* do nothing */
}

/*  default finalizer (function that will be used to destroy any data stored by
    user-specified callback functions. It would be called just before destroying
    xml_field)
*/
static void finalize_field(void * field)
{
    /* do nothing */
}

/* default handler for xml_field exception condition */
static void default_xfe_handler(void * field, xml_field_exception_code ecode, void * data)
{
    KvTrace("xml_field exception code = %d (%s)", ecode, data?(char*)data:"");
}

/* adds data chunk to array of data chunks in xml_field */
static void add_data_chunk(xml_field * f, const char * chunk)
{
    if(f == NULL || chunk == NULL) return;
    if(f->data_chunks == NULL)
    {
        f->data_chunks = (char **)fbe_api_allocate_memory(2*sizeof(char *));
        *(f->data_chunks) = fbe_strdup(chunk);
        *(f->data_chunks + 1) = NULL;
        f->data_chunks_counter = 1;
    }
    else
    {
        int chunks_count = f->data_chunks_counter;
        int new_size = (chunks_count + 2)*sizeof(char *);
        char ** new_chunks = (char **)fbe_reallocate_memory(
            f->data_chunks, chunks_count * sizeof(char*), new_size);
        if(new_chunks != NULL)
        {
            f->data_chunks = new_chunks;
            f->data_chunks[chunks_count] = fbe_strdup(chunk);
            f->data_chunks[chunks_count + 1] = NULL;
            f->data_chunks_counter++;
            /* if strdup failed */
            if(f->data_chunks[chunks_count] == NULL)
            {
                xf_exception_handler(f, XFE_NO_MEMORY, NULL);
            }
        }
        else
        {
            xf_exception_handler(f, XFE_NO_MEMORY, NULL);
        }
    }
}

/* default handler for storing data chunks */
static void store(void * field, void * unused, const char * data)
{
    xml_field * f = (xml_field *)field;
    add_data_chunk(f, data);
}

/* Adds subfield child_field to the parent_field, both must not be NULL */
void add_subfield(xml_field * parent_field, xml_field * child_field)
{
    if(parent_field == NULL) return;
    if(child_field == NULL)
    {
        parent_field->exception_handler(parent_field, XFE_INVALID_ARGUMENT, NULL);
        return;
    }

    /*  If user wants warnings about suspicious situantions, we must check that we haven't
        already got subfield with the same name*/
    if(parent_field->warning_handler != NULL)
    {
        int i;
        for(i = 0; i < child_field->names_counter; i++)
        {
            if(find_subfield(parent_field, child_field->names[i]) != NULL)
            {
                xf_warning_handler(parent_field, XFW_DUPLICATED_SUBFIELD, child_field->names[i]);
                break;
            }
        }
    }
    /* Should we prevent situation when two different parents have one child? */
    if(child_field->parent != NULL && child_field->parent != parent_field)
    {
        parent_field->exception_handler(parent_field, XFE_ALREADY_HAS_PARENT, NULL);
        return;
    }
    else child_field->parent = parent_field;

    if(parent_field->subfields == NULL)
    {
        parent_field->subfields = (xml_field **)fbe_api_allocate_memory(2*sizeof(xml_field *));
        *(parent_field->subfields) = child_field;
        *(parent_field->subfields + 1) = NULL;
        parent_field->subfields_counter = 1;
    }
    else
    {
        //xml_field ** sf = parent_field->subfields;

        int sf_count = parent_field->subfields_counter;
        int new_size = (sf_count + 2)*sizeof(xml_field *);
        xml_field ** new_subfields = (xml_field **)fbe_reallocate_memory(
            parent_field->subfields, sf_count * sizeof(xml_field*), new_size);
        if(new_subfields != NULL)
        {
            parent_field->subfields = new_subfields;
            parent_field->subfields[sf_count] = child_field;
            parent_field->subfields[sf_count+1] = NULL;
            parent_field->subfields_counter++;
        }
        else
        {
            xf_exception_handler(parent_field, XFE_NO_MEMORY, NULL);
        }
    }
}

/* Adds name to the list of names of parent_field, both arguments must not be NULL */
void add_name(xml_field * parent_field, const char * name)
{
    if(parent_field == NULL) return;
    if(name == NULL)
    {
        xf_exception_handler(parent_field, XFE_INVALID_ARGUMENT, NULL);
        return;
    }

    if(parent_field->names == NULL)
    {
        parent_field->names = (char **)fbe_api_allocate_memory(2*sizeof(char *));
        *(parent_field->names) = fbe_strdup(name);
        *(parent_field->names + 1) = NULL;
        parent_field->names_counter = 1;
        /* if strdup failed */
        if(*(parent_field->names) == NULL)
        {
            xf_exception_handler(parent_field, XFE_NO_MEMORY, NULL);
        }
    }
    else
    {
        int names_count = parent_field->names_counter;
        int new_size = (names_count + 2)*sizeof(char *);
        char ** new_names = (char **)fbe_reallocate_memory(
            parent_field->names, names_count * sizeof(char *), new_size);
        if(new_names != NULL)
        {
            parent_field->names = new_names;
            parent_field->names[names_count] = fbe_strdup(name);
            parent_field->names[names_count + 1] = NULL;
            parent_field->names_counter++;
            /* if strdup failed */
            if(parent_field->names[names_count] == NULL)
            {
                xf_exception_handler(parent_field, XFE_NO_MEMORY, NULL);
            }
        }
        else
        {
            xf_exception_handler(parent_field, XFE_NO_MEMORY, NULL);
        }
    }
}

/* allocates memory for new xml_field structure
  and assign default values to its fields     */
xml_field * create_xml_field(const char * name, int report_missing_fields)
{
    xml_field * new_field = (xml_field *)fbe_api_allocate_memory(sizeof(xml_field));
    new_field->allocate = NULL;
    new_field->store = store;
    new_field->process_attributes = process_attributes;
    new_field->end_element = end_element;
    new_field->exception_handler = default_xfe_handler;
    new_field->warning_handler = NULL;
    new_field->finalize = finalize_field;
    if(name != NULL)
    {
        new_field->names = (char **)fbe_api_allocate_memory(2*sizeof(char *));
        new_field->names[0] = fbe_strdup(name); /* ???? strdup !!!! */
        new_field->names[1] = NULL;
        new_field->current_name = new_field->names[0];
        new_field->names_counter = 1;
    }
    else
    {
        new_field->names = (char **)fbe_api_allocate_memory(sizeof(char *));
        new_field->names[0] = NULL;
        new_field->current_name = new_field->names[0];
        new_field->names_counter = 0;
    }
    new_field->report_missing_fields = report_missing_fields;
    new_field->obj = NULL;
    new_field->parent = NULL;
    new_field->subfields_counter = 0;
    new_field->subfields = NULL;
    new_field->data_chunks_counter = 0;
    new_field->data_chunks = NULL;
    new_field->user_data = NULL;
    new_field->attributes_counter = 0;
    new_field->attributes = NULL;
    return new_field;
}

/* remove subfield from the parent's list of subfields */
void delete_subfield(xml_field * parent, xml_field * subfield)
{
    xml_field ** sf = parent ? parent->subfields : NULL;
    if(sf == NULL) return;
    if(subfield == NULL)
    {
        xf_exception_handler(parent, XFE_INVALID_ARGUMENT, NULL);
        return;
    }
    while(*sf != NULL && *sf != subfield) sf++;
    if(*sf == NULL) return; /* nothing has been found */
    else
    {
        /* should we clear link to parent in the subfield??? */
        subfield->parent = NULL;
        /* shift items in array of subfields */
        do
        {
            *sf = *(sf + 1);
        }
        while(*++sf != NULL);
        parent->subfields_counter--;
        return;
    }
}

/* destroy xml_field structure f, which was previously allocated by create_xml_field */
void delete_xml_field(xml_field * f)
{
    if(f == NULL) return;
    else
    {
        /* Should we "notify" parent when deleting subfields? */
        /* if(f->parent != NULL) delete_subfield(f->parent, f); */
        xml_field ** sf = f->subfields;
        char ** ns = f->names;
        char ** dcs = f->data_chunks;
        char ** atts = f->attributes;
        /* Clean user data */
        xf_finalize(f);
        /* delete subfields */
        if(sf != NULL)
        {
            while(*sf != NULL)
            {
                delete_xml_field(*sf++);
                f->subfields_counter--;
            }
            fbe_api_free_memory(f->subfields);
        }
        /* delete all local copies of names*/
        if(ns != NULL)
        {
            while(*ns != NULL)
            {
                fbe_api_free_memory(*ns++);
            }
            fbe_api_free_memory(f->names);
        }
        /* delete all stored data chunks */
        if(dcs != NULL)
        {
            while(*dcs != NULL)
            {
                fbe_api_free_memory(*dcs++);
            }
            fbe_api_free_memory(f->data_chunks);
        }
        /* delete all stored attributes and their values */
        if(atts != NULL)
        {
            while(*atts != NULL)
            {
                fbe_api_free_memory(*atts++);
            }
            fbe_api_free_memory(f->attributes);
        }
        /* fbe_api_free_memory memory allocated for the structure */
        fbe_api_free_memory(f);
    }
}

/* returns 1 if at least one name of field is equal to the specified name */
/* returns 0 in any other case, even when some of arguments are NULL */
int matches(const xml_field * field, const char * name)
{
    if(field == NULL || name == NULL) return 0;
    else
    {
        char ** s = field->names;
        if(s == NULL || *s == NULL) return 0;
        else
        {
            do
            {
                if( !strcmp(*s,  name) ) return 1;
            }
            while (*++s != NULL);
        }
        return 0;
    }
}

/* Scans all subfields of the parent_field. If nth field from this set
   matches with name (see definition of matches() function), then pointer
   to this field is returned. Otherwise, this function return NULL */
xml_field * find_nth_subfield(const xml_field * parent_field, const char * name, int n)
{
    if(parent_field == NULL || name == NULL) return NULL;
    else
    {
        xml_field ** sf = parent_field->subfields;
        if(sf == NULL || *sf == NULL) return NULL;
        else
        {
            do
            {
                if(matches(*sf, name))
                    if(!n--) return *sf;
            }
            while(*++sf != NULL);
        }
        return NULL;
    }
}

xml_field * find_subfield(const xml_field * parent_field, const char * name)
{
    return find_nth_subfield(parent_field, name, 0);
}

/*
    Functions that invoke appropriate handlers.
    Instead of writing
        field->handler(field, arguments);
    you can use short version:
        xf_handler(field, arguments);
    Also these functions can check that handlers are not NULLs
    (not implemented yet)
*/
void * xf_allocate(xml_field * field, void * argument)
{
    return field->allocate(field, argument);
}

void xf_store(xml_field * field, void * object, const char * data)
{
    field->store(field, object, data);
}

void xf_end_element(xml_field * field, void * object, void * element)
{
    field->end_element(field, object, element);
}

void xf_process_attributes(xml_field * field, void * object, const char ** attributes)
{
    field->process_attributes(field, object, attributes);
}

void xf_exception_handler(xml_field * field, xml_field_exception_code code, void * data)
{
    field->exception_handler(field, code, data);
}

void xf_warning_handler(xml_field * field, xml_field_warning_code code, void * data)
{
    field->warning_handler(field, code, data);
}

void xf_finalize(xml_field * field)
{
    field->finalize(field);
}

/*  All these functions try to find subfield specified by name in the field, specified by
    first argument. If subfield is found, they will try to convert its data (actually, data
    chunk #0, all the following chunks are ignored) to specified type. If subfield is not
    found, exception XFE_CHILD_NOT_FOUND is raised. If subfield does not have data,
    exception XFE_DATA_MISSING would be raised. If conversion fails (data has incompatible
    format, or precision is lost) XFE_CONVERSION_ERROR exception is raised, and 0 (NULL) is
    returned.
*/
/* Does not convert data, just do checks */
const char * get_nth_string_subfield(const xml_field * parent, const char * name, int n)
{
    xml_field * child;
    if(parent == NULL) return NULL;
    if(name == NULL)
    {
        /* not the best way to remove const qualifier */
        xf_exception_handler((xml_field *)parent, XFE_INVALID_ARGUMENT, NULL);
        return NULL;
    }
    /*obtain subfield */
    child = find_nth_subfield(parent, name, n);
    /*make cheks*/
    if(child == NULL)
    {
        xf_exception_handler((xml_field *)parent, XFE_CHILD_NOT_FOUND, (void *)name);
        return NULL;
    }
    if(child->data_chunks == NULL || child->data_chunks[0] == NULL)
    {
        xf_exception_handler((xml_field *)parent, XFE_DATA_MISSING, (void *)name);
        return NULL;
    }
    else return child->data_chunks[0];
}

/* Do checks and convert data to long integer value */
long get_nth_long_subfield(const xml_field * parent, const char * name, int n)
{
    //long result = 0L;
    const char * s = get_nth_string_subfield(parent, name, n);
    //char * endofstr;
    if(s == NULL) return 0L; /* exception is already handled, see upper */
    /*omit spaces, tabs and \ns */
    while(csx_rt_ctype_isspace(*s)) s++;
    //result = strtol(s, &endofstr, 0);
    ///*check that there is nothing but spaces, tabs and \ns */
    ///*if strtol finds an error, endofstr = s, and *endofstr is not space */
    //while(isspace(*endofstr)) endofstr++;
    //if(*endofstr || s == endofstr)
    //{
    //    char buffer[100];
    //    memset(buffer, 0, sizeof(buffer));
    //    _snprintf(buffer, 100, "Could not convert \"%.24s\" to long integer", s);
    //    xf_exception_handler((xml_field *)parent, XFE_CONVERSION_ERROR, buffer);
    //    return 0L;
    //}
    //else return result;
    return atol(s);
}

/* Do checks and convert data to value of type int*/
int get_nth_int_subfield(const xml_field * parent, const char * name, int n)
{
    long resl = get_nth_long_subfield(parent, name, n);
    if(resl < (long)INT_MIN || resl > (long)INT_MAX)
    {
        char buffer[100];
        memset(buffer, 0, sizeof(buffer));
        _snprintf(buffer, 100, "Number %ld is out of int type range.", resl);
        xf_exception_handler((xml_field *)parent, XFE_CONVERSION_ERROR, buffer);
        return (int)resl;
    }
    return (int)resl;
}

const char * get_string_subfield(const xml_field * parent, const char * name)
{
    return get_nth_string_subfield(parent, name, 0);
}

long get_long_subfield(const xml_field * parent, const char * name)
{
    return get_nth_long_subfield(parent, name, 0);
}

int get_int_subfield(const xml_field * parent, const char * name)
{
    return get_nth_int_subfield(parent, name, 0);
}

/*  returns 1 if at least one name of field is equal to the specified name
    (not null-terminated string of length name_len)
    returns 0 in any other case, even when some of arguments are NULL */
static int matchesn(const xml_field * field, const char * name, int name_len)
{
    if(field == NULL || name == NULL) return 0;
    else
    {
        char ** s = field->names;
        if(s == NULL || *s == NULL) return 0;
        else
        {
            do
            {
                if( !strncmp(*s,  name, name_len) ) return 1;
            }
            while (*++s != NULL);
        }
        return 0;
    }
}

/*  Scans all subfields of the parent_field. If first name_len symbols
    of one of them matches with name (see definition of matchesn()
    function), then pointer to this field is returned. Otherwise, this
    function return NULL */
static xml_field * find_subfieldn(const xml_field * parent_field, const char * name, int name_len)
{
    if(parent_field == NULL || name == NULL) return NULL;
    else
    {
        xml_field ** sf = parent_field->subfields;
        if(sf == NULL || *sf == NULL) return NULL;
        else
        {
            do
            {
                if(matchesn(*sf, name, name_len)) return *sf;
            }
            while(*++sf != NULL);
        }
        return NULL;
    }
}

xml_field * get_subfield_by_path(const xml_field * root, const char * path)
{
    return NULL; /*Not implemented yet*/
}

/*  This function try to find attribute specified by name in the field,
    specified by first argument. If attribute is found, NULL is returned.
*/
const char * get_attribute_value(const xml_field * parent , const char * name)
{
    if(parent == NULL) return NULL;
    if(name == NULL)
    {
        xf_exception_handler((xml_field *)parent, XFE_INVALID_ARGUMENT, NULL);
    }
    else
    {
        char ** atts = parent->attributes;
        if(atts == NULL || *atts == NULL) return NULL;
        do
        {
            char * attr_name = *atts;
            char * value = *++atts;
            if(!strcmp(attr_name, name)) return value;
        }
        while(*++atts);
    }
    return NULL;
}

/*  All these functions try to find attribute specified by name in the field, specified by
    first argument. If attribute is found, they will try to convert its value to specified
    type. If attribute is not found, exception XFE_ATTR_NOT_FOUND is raised. If conversion
    fails (value has incompatible format, or precision is lost) XFE_CONVERSION_ERROR
    exception is raised, and 0 is returned.
*/
long get_long_attribute_value(const xml_field * parent, const char * name)
{
    /*long result = 0L;*/
    /*char * endofstr;*/
    const char * s = get_attribute_value(parent, name);
    if(s == NULL)
    {
        xf_exception_handler((xml_field *)parent, XFE_ATTRIBUTE_NOT_FOUND, (void *)name);
        return 0L;
    }
    /*omit spaces, tabs and \ns */
    while(csx_rt_ctype_isspace(*s)) s++;
    //result = strtol(s, &endofstr, 0);
    ///*check that there is nothing but spaces, tabs and \ns */
    ///*if strtol finds an error, endofstr = s, and *endofstr is not space */
    //while(isspace(*endofstr)) endofstr++;
    //if(*endofstr || s == endofstr)
    //{
    //    char buffer[100];
    //    memset(buffer, 0, sizeof(buffer));
    //    _snprintf(buffer, 100, "Could not convert \"%.24s\" to long integer", s);
    //    xf_exception_handler((xml_field *)parent, XFE_CONVERSION_ERROR, buffer);
    //    return 0L;
    //}
    //else return result;
    return atol(s);
}

int get_int_attribute_value(const xml_field * parent, const char * name)
{
    long resl = get_long_attribute_value(parent, name);
    if(resl < (long)INT_MIN || resl > (long)INT_MAX)
    {
        char buffer[100];
        memset(buffer, 0, sizeof(buffer));
        _snprintf(buffer, 100, "Number %ld is out of int type range.", resl);
        xf_exception_handler((xml_field *)parent, XFE_CONVERSION_ERROR, buffer);
        return (int)resl;
    }
    return (int)resl;
}

