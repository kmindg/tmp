/*!**************************************************************************
 * @file tcm_unit_test.c
 ****************************************************************************
 *
 * @brief
 *  This file contains unit test for the Terminator class management module.
 *
 * @date 11/25/2008
 * @author Alex Alanov
 *
 ***************************************************************************/

/**********************************/
/*        include files           */
/**********************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "fbe_terminator_class_management.h"
#include "terminator_test.h"

typedef enum {
    ENUM_VALUE_FIRST = 1,
    ENUM_VALUE_SECOND,
    ENUM_VALUE_THIRD
} enum_t;

#define MAKE_STR(a) #a

typedef struct some_structure_s
{
    char    a[64];
    long    b;
    enum_t  c;
} some_structure_t;

#define ATTR_A "attr_a"
#define ATTR_B "attr_b"
#define ATTR_C "attr_c"

#define ID_A 1
#define ID_B 2
#define ID_C 3

int allocator_flag = 0;
int destroyer_flag = 0;

/* handlers for allocating and destroying board */
static tcm_status_t some_allocator(const char * type, tcm_reference_t * handle)
{
    some_structure_t * pss = (some_structure_t *)malloc(sizeof(some_structure_t));
    if(pss == NULL)
    {
        return TCM_STATUS_GENERIC_FAILURE;
    }
    memset(pss, 0, sizeof(some_structure_t));
    strncpy(pss->a, type, 63);
    pss->b = 1L;
    pss->c = ENUM_VALUE_SECOND;
    *handle = (tcm_reference_t)pss;
    allocator_flag = 1;
    return TCM_STATUS_OK;
}

static tcm_status_t some_destroyer(const char * type, tcm_reference_t handle)
{
    free((void *)handle);
    destroyer_flag = 1;
    return TCM_STATUS_OK;
}

/* accessors */
static tcm_status_t string_string_setter(void *device, const char *name, const char *value)
{
    some_structure_t * pss = (some_structure_t *)device;
    if(!strcmp(name, ATTR_A))
    {
        strncpy(pss->a, value, 63);
        return TCM_STATUS_OK;
    }
    if(!strcmp(name, ATTR_B))
    {
        pss->b = atol(value);
        return TCM_STATUS_OK;
    }
    if(!strcmp(name, ATTR_C))
    {
        if(!strcmp(value, MAKE_STR(ENUM_VALUE_FIRST)))
        {
            pss->c = ENUM_VALUE_FIRST;
            return TCM_STATUS_OK;
        }
        if(!strcmp(value, MAKE_STR(ENUM_VALUE_SECOND)))
        {
            pss->c = ENUM_VALUE_SECOND;
            return TCM_STATUS_OK;
        }
        if(!strcmp(value, MAKE_STR(ENUM_VALUE_THIRD)))
        {
            pss->c = ENUM_VALUE_THIRD;
            return TCM_STATUS_OK;
        }
    }
    return TCM_STATUS_GENERIC_FAILURE;
}

static tcm_status_t string_string_getter(void *device, const char *name, char *buffer, tcm_uint_t buffer_len)
{
    some_structure_t * pss = (some_structure_t *)device;
    if(!strcmp(name, ATTR_A))
    {
        memcpy(buffer, pss->a, buffer_len);
        return TCM_STATUS_OK;
    }
    if(!strcmp(name, ATTR_B))
    {
        _snprintf(buffer, buffer_len, "%ld", pss->b);
        return TCM_STATUS_OK;
    }
    if(!strcmp(name, ATTR_C))
    {
        switch(pss->b)
        {
        case ENUM_VALUE_FIRST:
            memcpy(buffer, MAKE_STR(ENUM_VALUE_FIRST), buffer_len);
            return TCM_STATUS_OK;
        case ENUM_VALUE_SECOND:
            memcpy(buffer, MAKE_STR(ENUM_VALUE_SECOND), buffer_len);
            return TCM_STATUS_OK;
        case ENUM_VALUE_THIRD:
            memcpy(buffer, MAKE_STR(ENUM_VALUE_THIRD), buffer_len);
            return TCM_STATUS_OK;
        default:
            return TCM_STATUS_GENERIC_FAILURE;
        }
    }
    return TCM_STATUS_GENERIC_FAILURE;
}

static tcm_status_t uint_string_setter(void *device, tcm_uint_t key, const char *value)
{
    some_structure_t * pss = (some_structure_t *)device;
    if(key == ID_A)
    {
        strncpy(pss->a, value, 63);
        return TCM_STATUS_OK;
    }
    if(key == ID_B)
    {
        pss->b = atol(value);
        return TCM_STATUS_OK;
    }
    if(key == ID_C)
    {
        if(!strcmp(value, MAKE_STR(ENUM_VALUE_FIRST)))
        {
            pss->c = ENUM_VALUE_FIRST;
            return TCM_STATUS_OK;
        }
        if(!strcmp(value, MAKE_STR(ENUM_VALUE_SECOND)))
        {
            pss->c = ENUM_VALUE_SECOND;
            return TCM_STATUS_OK;
        }
        if(!strcmp(value, MAKE_STR(ENUM_VALUE_THIRD)))
        {
            pss->c = ENUM_VALUE_THIRD;
            return TCM_STATUS_OK;
        }
    }
    return TCM_STATUS_GENERIC_FAILURE;
}

static tcm_status_t uint_string_getter(void *device, tcm_uint_t key, char *buffer, tcm_uint_t buffer_len)
{
    some_structure_t * pss = (some_structure_t *)device;
    if(key == ID_A)
    {
        memcpy(buffer, pss->a, buffer_len);
        return TCM_STATUS_OK;
    }
    if(key == ID_B)
    {
        _snprintf(buffer, buffer_len, "%ld", pss->b);
        return TCM_STATUS_OK;
    }
    if(key == ID_C)
    {
        switch(pss->c)
        {
        case ENUM_VALUE_FIRST:
            memset(buffer, 0, buffer_len);
            memcpy(buffer, MAKE_STR(ENUM_VALUE_FIRST), buffer_len);
            return TCM_STATUS_OK;
        case ENUM_VALUE_SECOND:
            memset(buffer, 0, buffer_len);
            memcpy(buffer, MAKE_STR(ENUM_VALUE_SECOND), buffer_len);
            return TCM_STATUS_OK;
        case ENUM_VALUE_THIRD:
            memset(buffer, 0, buffer_len);
            memcpy(buffer, MAKE_STR(ENUM_VALUE_THIRD), buffer_len);
            return TCM_STATUS_OK;
        default:
            return TCM_STATUS_GENERIC_FAILURE;
        }
    }
    return TCM_STATUS_GENERIC_FAILURE;
}

static tcm_status_t uint_uint_getter(void *device, tcm_uint_t key, tcm_uint_t *value)
{
    some_structure_t * pss = (some_structure_t *)device;
    if(key == ID_A)
    {
        *value = strtoul(pss->a, NULL, 10);
        return TCM_STATUS_OK;
    }
    if(key == ID_B)
    {
        *value = (tcm_uint_t)(pss->b);
        return TCM_STATUS_OK;
    }
    if(key == ID_C)
    {
        *value = (tcm_uint_t)(pss->c);
        return TCM_STATUS_OK;
    }
    return TCM_STATUS_GENERIC_FAILURE;
}

#define BUFFER_LEN 64


void terminator_class_management_test(void)
{
    char buffer[BUFFER_LEN];
    tcm_status_t status;
    tcm_reference_t instance = 0;
    tcm_reference_t class_handle = 0;
    tcm_reference_t bclass = 0;
    terminator_attribute_accessor_t * string_accessor;
    terminator_attribute_accessor_t * id_accessor;

    terminator_attribute_accessor_t * string_accessor_c;

    /*initialize TCM and create class */
    status = fbe_terminator_class_management_init();
    MUT_ASSERT_INTEGER_EQUAL( TCM_STATUS_OK, status )
    status = fbe_terminator_class_management_class_create("Some", some_allocator, some_destroyer, &class_handle);
    MUT_ASSERT_INTEGER_EQUAL( TCM_STATUS_OK, status )

    /* add accessors */

    status = fbe_terminator_class_management_class_accessor_add(class_handle, ATTR_A, ID_A, 
        string_string_setter,
        string_string_getter,
        uint_string_setter,
        uint_string_getter,
        uint_uint_getter);
    MUT_ASSERT_INTEGER_EQUAL( TCM_STATUS_OK, status )

    status = fbe_terminator_class_management_class_accessor_add(class_handle, ATTR_B, ID_B, 
        string_string_setter,
        string_string_getter,
        uint_string_setter,
        uint_string_getter,
        uint_uint_getter);
    MUT_ASSERT_INTEGER_EQUAL( TCM_STATUS_OK, status )

    status = fbe_terminator_class_management_class_accessor_add(class_handle, ATTR_C, ID_C, 
        string_string_setter,
        string_string_getter,
        uint_string_setter,
        uint_string_getter,
        uint_uint_getter);
    MUT_ASSERT_INTEGER_EQUAL( TCM_STATUS_OK, status )
    /* cannot add accessor twice */
    status = fbe_terminator_class_management_class_accessor_add(class_handle, ATTR_A, ID_A, 
        string_string_setter,
        string_string_getter,
        uint_string_setter,
        uint_string_getter,
        uint_uint_getter);
    MUT_ASSERT_INTEGER_EQUAL( TCM_STATUS_GENERIC_FAILURE, status )
    
    /* there should not be any other classes */
    status = fbe_terminator_class_management_class_find("unknown_class", &bclass);
    MUT_ASSERT_INTEGER_EQUAL( TCM_STATUS_GENERIC_FAILURE, status )

    /* obtain class */
    status = fbe_terminator_class_management_class_find("Some", &bclass);
    MUT_ASSERT_INTEGER_EQUAL( TCM_STATUS_OK, status )
    MUT_ASSERT_TRUE( bclass == class_handle )

    /* obtain getters and setters by id and by string*/
    /*unknown ids and strings must produce an error*/
    status = fbe_terminator_class_management_class_accessor_string_lookup(bclass, "unknown_attribute", &string_accessor);
    MUT_ASSERT_INTEGER_EQUAL( TCM_STATUS_GENERIC_FAILURE, status )
    status = fbe_terminator_class_management_class_accessor_uint_lookup(bclass, 0x0BADF00D, &id_accessor);
    MUT_ASSERT_INTEGER_EQUAL( TCM_STATUS_GENERIC_FAILURE, status )

    /* known ids and strings must succeed*/
    status = fbe_terminator_class_management_class_accessor_string_lookup(bclass, ATTR_B, &string_accessor);
    MUT_ASSERT_INTEGER_EQUAL( TCM_STATUS_OK, status )
    status = fbe_terminator_class_management_class_accessor_uint_lookup(bclass, ID_B, &id_accessor);
    MUT_ASSERT_INTEGER_EQUAL( TCM_STATUS_OK, status )
    status = fbe_terminator_class_management_class_accessor_string_lookup(bclass, ATTR_C, &string_accessor);
    MUT_ASSERT_INTEGER_EQUAL( TCM_STATUS_OK, status )
    string_accessor_c = string_accessor;
    status = fbe_terminator_class_management_class_accessor_uint_lookup(bclass, ID_C, &id_accessor);
    MUT_ASSERT_INTEGER_EQUAL( TCM_STATUS_OK, status )
    status = fbe_terminator_class_management_class_accessor_string_lookup(bclass, ATTR_A, &string_accessor);
    MUT_ASSERT_INTEGER_EQUAL( TCM_STATUS_OK, status )
    status = fbe_terminator_class_management_class_accessor_uint_lookup(bclass, ID_A, &id_accessor);
    MUT_ASSERT_INTEGER_EQUAL( TCM_STATUS_OK, status )


    /* create an instance of device */
    status = fbe_terminator_class_management_create_instance(bclass, "some_type", &instance);
    MUT_ASSERT_INTEGER_EQUAL( TCM_STATUS_OK, status )
    MUT_ASSERT_TRUE( allocator_flag )

    /* set and get attributes of device */
    status = string_accessor->string_string_setter((void *)instance, ATTR_A, "attr_a_something_like_that");
    MUT_ASSERT_INTEGER_EQUAL( TCM_STATUS_OK, status )
    status = string_accessor->uint_string_getter(instance, ID_A, buffer, BUFFER_LEN);
    MUT_ASSERT_TRUE( !strcmp(buffer, "attr_a_something_like_that") )

    /*modify it manually */
    ((some_structure_t *)instance)->c = ENUM_VALUE_THIRD;

    status = string_accessor_c->uint_string_getter(instance, ID_C, buffer, BUFFER_LEN);
    MUT_ASSERT_INTEGER_EQUAL( TCM_STATUS_OK, status )

    MUT_ASSERT_TRUE( !strcmp(buffer, MAKE_STR(ENUM_VALUE_THIRD)))
    
    /* destroy an instance of device */
    status = fbe_terminator_class_management_delete_instance(bclass, "some_type", instance);
    MUT_ASSERT_INTEGER_EQUAL( TCM_STATUS_OK, status )
    MUT_ASSERT_TRUE( destroyer_flag )

    return;
}

