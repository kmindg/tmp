/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                xml_field.h
 ***************************************************************************
 *
 * DESCRIPTION: Header for XML field functionality
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    09/29/2008   Alexander Alanov (alanoa) initial version
 *
 **************************************************************************/


#ifndef XML_FIELD_INCLUDED
#define XML_FIELD_INCLUDED 1

#ifdef __cplusplus
extern "C" {
#endif

typedef enum tag_xml_field_warning_code
{
    XFW_DUPLICATED_SUBFIELD = 1, 
    /*  Framework detected that some field would have two subfields with the same name.
        Name is passed to handler as null-terminated string in third argument (void * data)
    */
    XFW_MAX
} xml_field_warning_code;

typedef enum tag_xml_field_exception_code
{   XFE_NO_MEMORY = 1,      /*Could not allocate memory for additional data */
    XFE_INVALID_ARGUMENT,   /*Invalid argument passed to function */
    XFE_ALREADY_HAS_PARENT, /*Field already has a parent ()*/
    XFE_CHILD_NOT_FOUND,    /*Field does not have a child required to proceed*/
    XFE_ATTRIBUTE_NOT_FOUND,/*Field does not have a attribute required to proceed*/
    XFE_PATH_NOT_FOUND,     /*get_subfield_by_path could not find subfield specified by path */
    XFE_DATA_MISSING,       /*Required data is not present in field (appropriate pointer is NULL)*/
    XFE_CONVERSION_ERROR,   /*Could not convert character data to specified type */
    XFE_MAX                 /*Last member of enumeration */
} xml_field_exception_code;

/*  First argument to all these callback functions is pointer to structure
    through which they are called. It is a kind of C++ "this" pointer.
 */
typedef void * (* xml_field_allocate_object_t)(void *, void *);
/*  store() handler, second argument is reserved for future use, and third is null-terminated
    string containing the chunk of data to store */
typedef void (* xml_field_store_t)(void *, void *, const char *);
/*  process_attributes() handler, second argument is reserved for future use, and third is NULL-terminated
    array of null-terminated strings. The structure of attributes is following:
    {attribute1, value1, attribute2, value2, ..., attributeN, valueN, NULL}
*/
typedef void (* xml_field_process_attributes_t)(void *, void *, const char ** attributes);
/*  Handler for exceptions. Second argument is exception code (see enum upper), third - additional 
    data related to the exception (depends on code, can be NULL)*/
typedef void (* xml_field_exception_handler)(void *, xml_field_exception_code, void *);
/*  Handler for warnings. Second argument is warning code (see enum upper), third - additional 
    data related to the exception (depends on code, can be NULL)*/
typedef void (* xml_field_warning_handler)(void *, xml_field_warning_code, void *);
/* end of element handler. Called when the end of field is reached. Second and third arguments are 
    reserved for future use and are not used at this moment*/
typedef void (* xml_field_end_element)(void *, void *, void *);
/* finalizer */
typedef void (* xml_field_finalize)(void *);

/*  
    I know that it is not a good idea to do callback functions in this way,
    because it becomes annoying to write pointer to the structure two times.
    But we can define an invokation function like this:

    void * allocate(xml_field * f, void * object)
    {
        return f->allocate(f, object);
    }

 */

typedef struct _tag_xml_field
{
    struct _tag_xml_field * parent;     /* parent node */
    int report_missing_fields;          /* unused, reserved for future use*/
    int names_counter;                  /* counter of names of this node */
    char ** names;                      /* NULL-terminated array of zero-terminated names of this node */
    char * current_name;                /* current name of this node */    
    int attributes_counter;             /* counter of pairs attribute = value of this node */
    char ** attributes;                 /* NULL-terminated array of attributes of this node
                                           The structure of attributes is following:
                                            {attribute1, value1, attribute2, value2, 
                                            ..., attributeN, valueN, NULL}  
                                           Attributes and values are zero-terminated strings */
    int subfields_counter;              /* counter of child nodes of this node */
    struct _tag_xml_field ** subfields; /* NULL-terminated array of pointers to child nodes*/
    int data_chunks_counter;             /* counter of data chunks */
    char ** data_chunks;                /* NULL-terminated array of character data chunks */
                                        /* consider following xml data:
                                            <a>
                                            data1
                                            <b>...</b>
                                            data2
                                            <c>...</c>
                                            data3
                                            </a>
                                        In this case data_chunks for field a will be
                                        {"\ndata1\n", "\ndata2\n", "\ndata3\n", NULL}
                                        and data_chunks_counter = 3 */
    xml_field_allocate_object_t allocate; 
    /* allocate() callback function, unused, reserved for future use */
    xml_field_store_t store;            
    /* store() callback function, used to store chunk of character data related to this field*/
    xml_field_process_attributes_t process_attributes; 
    /*process_attributes() callback function, used to process attributes of field */
    xml_field_exception_handler exception_handler; 
    /* exception handler callback function */
    xml_field_warning_handler warning_handler; 
    /* Warning handler callback function, by default it is NULL. If you want to check the
    suspicious situations, you must set warning handler, in other case additional check 
    would not happen.
    */
    xml_field_end_element end_element;  
    /* end_element() callback function, used to indicate the end of the field*/
    xml_field_finalize finalize;        
    /* finalize() callback function, called just before the structure is deleted */
    void * obj;                         /* unused, reserved for future use */
    void * user_data;                   
    /* pointer to user-defined data. Any resources allocated by user and stored 
       in this data should be freed by finalize() callback function*/
} xml_field;

/* Adds subfield child_field to the parent_field, both must not be NULL */
void add_subfield(xml_field *, xml_field *);

/* Adds name to the list of names of parent_field, both arguments must not be NULL */
void add_name(xml_field *, const char *);

/* allocates memory for new xml_field structure 
  and assign default values to its fields     */
xml_field * create_xml_field(const char *, int);

/* remove subfield from the parent's list of subfields */
void delete_subfield(xml_field * , xml_field * );

/*  remove field from the parent's list of subfields and delete it with all its
    subfields */
void delete_xml_field_from_tree(xml_field *);

/* Destroy xml_field structure f, which was previously allocated by create_xml_field
   This function should be used only for deleting whole tree, or standalone nodes.
   In other case you should use delete_xml_field_from_tree(&field), in that case 
   parent's list of subfields would be updated and tree would remain consistent. 
   In this function parent's list of subfields is not updated for performance reasons.
*/
void delete_xml_field(xml_field * );

/* returns 1 if at least one name of field is equal to the specified name */
/* returns 0 in any other case, even when some of arguments are NULL */
int matches(const xml_field *, const char * );

/* Scans all subfields of the parent_field. If one of them matches with
   name (see definition of matches() function), then pointer to this 
   field is returned. Otherwise, this function return NULL. 
   find_sub_field(foo, bar) is equivalent to find_nth_subfield(foo, bar, 0) */
xml_field * find_subfield(const xml_field *, const char * );

/* Scans all subfields of the parent_field. If some of them match with
   name (see definition of matches() function), then pointer to n-th field
   in this set is returned. Otherwise, this function return NULL */
xml_field * find_nth_subfield(const xml_field *, const char *, int  );

/*  Finds xml_field structure by its relative path (null-terminated string 
    specified in second argument) from the root(first argument) field. Path
    contains the names of all subfields we must visit in order to get to the
    target field, separated by slash, for example "field/subfield/target"
*/
xml_field * get_subfield_by_path(const xml_field *, const char * );

/*  All these functions try to find nth subfield specified by name in the field, specified 
    by first argument. Index n is specified by third argument. If subfield is found, they
    will try to convert its data (actually, data chunk #0, all the following chunks are
    ignored) to specified type. If subfield is not found, exception XFE_CHILD_NOT_FOUND is
    raised. If subfield does not have data, exception XFE_DATA_MISSING would be raised. If
    conversion fails (data has incompatible format, or precision is lost) 
    XFE_CONVERSION_ERROR exception is raised, and 0 (or NULL) is returned.
    get_xxx_subfield(foo, bar) functions actually call get_nth_xxx_subfield(foo, bar, 0).
*/
const char * get_nth_string_subfield(const xml_field *, const char *, int );

long get_nth_long_subfield(const xml_field *, const char *, int );

int get_nth_int_subfield(const xml_field *, const char *, int );

double get_nth_double_subfield(const xml_field *, const char *, int );

const char * get_string_subfield(const xml_field *, const char * );

long get_long_subfield(const xml_field *, const char * );

int get_int_subfield(const xml_field *, const char * );

double get_double_subfield(const xml_field *, const char * );


/*  This function try to find attribute specified by name in the field, 
    specified by first argument. If attribute is found, NULL is returned.
*/
const char * get_attribute_value(const xml_field *, const char * );

/*  All these functions try to find attribute specified by name in the field, specified by
    first argument. If attribute is found, they will try to convert its value to specified
    type. If attribute is not found, exception XFE_ATTR_NOT_FOUND is raised. If conversion
    fails (value has incompatible format, or precision is lost) XFE_CONVERSION_ERROR 
    exception is raised, and 0 is returned.
*/
long get_long_attribute_value(const xml_field *, const char * );

int get_int_attribute_value(const xml_field *, const char * );

double get_double_attribute_value(const xml_field *, const char * );

/* invokes callback function in xml_field*/
/* alias for field->allocate(field, argument) */
void * xf_allocate(xml_field * field, void * argument);

/* alias for field->store(field, object, data)*/
void xf_store(xml_field * field, void * object, const char * data);

/* alias for field->end_element(field, object, element)*/
void xf_end_element(xml_field * field, void * object, void * element);

/* alias for field->process_attributes(field, object, attributes) */
void xf_process_attributes(xml_field * field, void * object, const char ** attributes);

/* alias for field->exception_handler(field, code, data) */
void xf_exception_handler(xml_field * field, xml_field_exception_code code, void * data);
 
/* alias for field->warning_handler(field, code, data) */
void xf_warning_handler(xml_field * field, xml_field_warning_code code, void * data);

/* alias for field->finalize(field) */
void xf_finalize(xml_field * field);

#ifdef __cplusplus
}
#endif

#endif