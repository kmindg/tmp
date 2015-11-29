/**************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 **************************************************************************/

/*!**************************************************************************
 * @file fbe_terminator_class_management.c
 ***************************************************************************
 *
 * @brief
 *  This file contains implementation of the Terminator class management module.
 *
 * @date 12/03/2008
 * @author Alanov Alex
 *
 ***************************************************************************/

/**********************************/
/*        include files           */
/**********************************/

#include "fbe/fbe_winddk.h"
#include "fbe_terminator_class_management.h"
#include <stdlib.h>
#include <string.h>
#include "fbe_terminator_common.h"

/*! @struct terminator_attribute_accessor_list_s
 *
 * @brief This structure contains a list of attributes accessors
 *        Each device type that the Terminator is capable of managing
 *        will contain an instance of terminator_attribute_accessor_list_s/t
 *        that can be used to access all available attributes within
 *        a terminator device of this type
 */
typedef struct terminator_attribute_accessor_list_s
{
    tcm_uint_t size;
    tcm_uint_t count;
    terminator_attribute_accessor_t * list;
} terminator_attribute_accessor_list_t;

/*! @struct terminator_device_class_s
 *
 * @brief This structure defines one class of objects that the Terminator
 *        can manage.  Every class of object that the Terminator can manage
 *        will have an instance of this structure registered within
 *        the terminator device management system.
 *        All terminator API requests to created, delete, query or
 *        modify objects contained within the terminator, will be routed
 *        through an instance of terminator_device_class_s/t
 */
typedef struct terminator_device_class_s
{
    char class_name[TCM_MAX_ATTR_NAME_LEN];
    fbe_terminator_device_allocator allocate;
    fbe_terminator_device_destroyer destroy;
    terminator_attribute_accessor_list_t accessors;
} terminator_device_class_t;

typedef struct terminator_device_class_list_s
{
    tcm_uint_t size;
    tcm_uint_t count;
    terminator_device_class_t * classlist;
} terminator_device_class_list_t;

/* list of Terminator device classes */
static terminator_device_class_list_t terminator_classes = EmcpalEmpty;


/*! @fn tcm_status_t fbe_terminator_class_management_init()
 *
 *  @brief Function that initializes Terminator class management module.
 *  @return the status of the call.
 */
tcm_status_t fbe_terminator_class_management_init(void)
{
    /* check that TCM has not been initialized yet */
    if(terminator_classes.classlist != NULL )
    {
        return TCM_STATUS_GENERIC_FAILURE;
    }
    else
    {
        tcm_uint_t i;
        terminator_classes.size = TCM_MAX_CLASSES;
        terminator_classes.count = 0;
        terminator_classes.classlist = (terminator_device_class_t *)fbe_terminator_allocate_memory(terminator_classes.size * sizeof(terminator_device_class_t));
        if(terminator_classes.classlist == NULL)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        for(i = 0; i < terminator_classes.size; i++)
        {
            fbe_zero_memory(&(terminator_classes.classlist[i]), sizeof(terminator_device_class_t));
        }
        return TCM_STATUS_OK;
    }
};



static void terminator_class_management_destroy_class(terminator_device_class_t * cls)
{
    if(cls->accessors.list != NULL)
    {
        fbe_terminator_free_memory(cls->accessors.list);
    }
}

/*! @fn tcm_status_t fbe_terminator_class_management_destroy()
 *
 *  @brief Function that destroys Terminator class management module.
 *  @return the status of the call.
 */
tcm_status_t fbe_terminator_class_management_destroy(void)
{
    tcm_uint_t i;
    if(terminator_classes.classlist != NULL)
    {
        for(i= 0; i < terminator_classes.count; i++)
        {
            terminator_class_management_destroy_class(&(terminator_classes.classlist[i]));
        }
        fbe_terminator_free_memory(terminator_classes.classlist);
        terminator_classes.classlist = NULL;
    }
    terminator_classes.count = 0;
    terminator_classes.size = 0;
    return TCM_STATUS_OK;
};


/*! @fn fbe_terminator_class_management_class_create(char *class_name,
                                                      fbe_terminator_device_allocator allocate,
                                                      fbe_terminator_device_destroyer destroy,
                                                      tcm_reference_t *class_handle);
 *  @brief Function registers a device translation
 *  @param class_name   - name of this class
 *  @param allocate     - function used to allocate instance of class
 *  @param destroy      - function used to destroy instance of class
 *  @param class_handle - handle to the newly created class
 *  @return the status of the call.
 */
tcm_status_t fbe_terminator_class_management_class_create(const char *class_name,
                                                           fbe_terminator_device_allocator allocate,
                                                           fbe_terminator_device_destroyer destroy,
                                                           tcm_reference_t *class_handle)
{
    tcm_uint_t i;

    /* check that there is enough space in class registry */
    if(terminator_classes.count >= terminator_classes.size)
    {
        return TCM_STATUS_GENERIC_FAILURE;
    }

    /* check parameters */
    if(class_name == NULL || class_handle == NULL || strlen(class_name) >= TCM_MAX_ATTR_NAME_LEN)
    {
        return TCM_STATUS_GENERIC_FAILURE;
    }

    /* check that there is not class with such name */
    for(i = 0; i < terminator_classes.count; i++)
    {
        if( !strcmp(terminator_classes.classlist[i].class_name, class_name) )
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
    }

    /* create new class */
    i = terminator_classes.count;
    terminator_classes.classlist[i].accessors.list = (terminator_attribute_accessor_t *)fbe_terminator_allocate_memory(
        TCM_MAX_ATTRIBUTES * sizeof(terminator_attribute_accessor_t));
    if (terminator_classes.classlist[i].accessors.list == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s failed to allocate memory for attribute accessor list at line %d.\n",
                         __FUNCTION__,
                         __LINE__);

        return TCM_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(terminator_classes.classlist[i].accessors.list, TCM_MAX_ATTRIBUTES * sizeof(terminator_attribute_accessor_t));

    terminator_classes.classlist[i].accessors.size = TCM_MAX_ATTRIBUTES;
    terminator_classes.classlist[i].accessors.count = 0;
    fbe_zero_memory(terminator_classes.classlist[i].class_name, sizeof(terminator_classes.classlist[i].class_name));
    strncpy(terminator_classes.classlist[i].class_name, class_name, sizeof(terminator_classes.classlist[i].class_name) - 1);
    terminator_classes.classlist[i].allocate = allocate;
    terminator_classes.classlist[i].destroy = destroy;
    terminator_classes.count++;
    *class_handle = (tcm_reference_t)&(terminator_classes.classlist[i]);
    return TCM_STATUS_OK;

}


/*! @fn fbe_terminator_class_management_class_accessor_add(tcm_reference_t class_handle,
                                                            char *attribute_name,
                                                            tcm_uint_t attribute_id,
                                                            terminator_attribute_string_string_setter string_string_setter,
                                                            terminator_attribute_string_string_getter string_string_getter,
                                                            terminator_attribute_uint_string_setter   uint_string_setter,
                                                            terminator_attribute_uint_string_getter   uint_string_getter);
 *  @brief Function registers an attribute accessor within a class
 *  @param class_handle   - handle of class accessor is added to
 *  @param attribute_name - new of attribute
 *  @param string_string_setter   - method used to update a device attribute that is passed String key/values
 *  @param string_string_getter   - method used to access a device attribute that is passed a String key and returns a String value
 *  @param uint_string_setter     - method used to update a device attribute that is passed a uint key and a String value
 *  @param uint_string_getter     - method used to access a device attribute that is passed a uint key and returns a String value
 *  @param uint_uint_getter       - method used to access a device attribute that is passed a uint key and returns a uint value
 *  @return the status of the call.
 */
tcm_status_t fbe_terminator_class_management_class_accessor_add(tcm_reference_t class_handle,
                                                                        const char *attribute_name,
                                                                        tcm_uint_t attribute_id,
                                                                        terminator_attribute_string_string_setter string_string_setter,
                                                                        terminator_attribute_string_string_getter string_string_getter,
                                                                        terminator_attribute_uint_string_setter   uint_string_setter,
                                                                        terminator_attribute_uint_string_getter   uint_string_getter,
                                                                        terminator_attribute_uint_uint_getter     uint_uint_getter
                                                                        )
{
    tcm_uint_t i;
    terminator_device_class_t * cls = (terminator_device_class_t *)class_handle;

    if(cls == NULL || attribute_name == NULL || strlen(attribute_name) >= TCM_MAX_ATTR_NAME_LEN )
    {
        return TCM_STATUS_GENERIC_FAILURE;
    }
    if( cls->accessors.count >= cls->accessors.size || cls->accessors.list == NULL )
    {
        return TCM_STATUS_GENERIC_FAILURE;
    }

    /* check if there is already an accessor for such attribute */
    for(i = 0; i < cls->accessors.count; i++)
    {
        if( !strcmp(attribute_name, cls->accessors.list[i].attribute_name)
            || attribute_id == cls->accessors.list[i].attribute_id )
        {
            /* there should not be two attribute accessors with same name or id */
            return TCM_STATUS_GENERIC_FAILURE;
        }
    }

    /* set the required fields */
    cls->accessors.list[cls->accessors.count].attribute_id = attribute_id;
    cls->accessors.list[cls->accessors.count].string_string_getter = string_string_getter;
    cls->accessors.list[cls->accessors.count].string_string_setter = string_string_setter;
    cls->accessors.list[cls->accessors.count].uint_string_getter = uint_string_getter;
    cls->accessors.list[cls->accessors.count].uint_string_setter = uint_string_setter;
    cls->accessors.list[cls->accessors.count].uint_uint_getter = uint_uint_getter;
    fbe_zero_memory(cls->accessors.list[cls->accessors.count].attribute_name, sizeof(cls->accessors.list[cls->accessors.count].attribute_name));
    strncpy(cls->accessors.list[cls->accessors.count].attribute_name, attribute_name, sizeof(cls->accessors.list[cls->accessors.count].attribute_name) - 1);
    cls->accessors.count++;
    return TCM_STATUS_OK;
}

/*! @fn fbe_terminator_class_management_class_find(char *class_name, tcm_reference_t *class_handle)
 *  @param class_handle - handle of class accessor is added to
 *  @param class_handle - handle to the newly created class
 *  @return the status of the call.
 */
tcm_status_t fbe_terminator_class_management_class_find(const char *class_name, tcm_reference_t *class_handle)
{
    tcm_uint_t i;
    for(i = 0; i < terminator_classes.count; i++)
    {
        if( !strcmp(terminator_classes.classlist[i].class_name, class_name) )
        {
            *class_handle = (tcm_reference_t)&(terminator_classes.classlist[i]);
            return TCM_STATUS_OK;
        }
    }
    return TCM_STATUS_GENERIC_FAILURE;
}

/*! @fn fbe_terminator_class_management_class_accessor_string_lookup(tcm_reference_t class_handle,
                                                                           char *attribute_name,
                                                                           terminator_attribute_accessor_t **accessor)
 *  @param class_handle   - handle of class accessor is added to
 *  @param attribute_name - name of attribute
 *  @param accessor       - return pointer to the attribute accessor
 *  @return the status of the call.
 */
tcm_status_t fbe_terminator_class_management_class_accessor_string_lookup(tcm_reference_t class_handle,
                                                                           const char *attribute_name,
                                                                           terminator_attribute_accessor_t **accessor)
{
    tcm_uint_t i;
    terminator_device_class_t * cls = (terminator_device_class_t *)class_handle;
    if(cls == NULL || attribute_name == NULL || accessor == NULL)
    {
        return TCM_STATUS_GENERIC_FAILURE;
    }
    /* check if there is an accessor for such attribute */
    for(i = 0; i < cls->accessors.count; i++)
    {
        if( !strcmp(attribute_name, cls->accessors.list[i].attribute_name) )
        {
            *accessor = &(cls->accessors.list[i]);
            return TCM_STATUS_OK;
        }
    }
    return TCM_STATUS_GENERIC_FAILURE;
}

/*! @fn fbe_terminator_class_management_class_accessor_uint_lookup(tcm_reference_t class_handle,
                                                                           uint attribute_id,
                                                                           terminator_attribute_accessor_t **accessor)
 *  @param class_handle - handle of class accessor is added to
 *  @param attribute_id - id of attribute
 *  @param accessor     - returd pointer to the attribute accessor
 *  @return the status of the call.
 */
tcm_status_t fbe_terminator_class_management_class_accessor_uint_lookup(tcm_reference_t class_handle,
                                                                           tcm_uint_t attribute_id,
                                                                           terminator_attribute_accessor_t **accessor)
{
    tcm_uint_t i;
    terminator_device_class_t * cls = (terminator_device_class_t *)class_handle;
    if(cls == NULL || accessor == NULL)
    {
        return TCM_STATUS_GENERIC_FAILURE;
    }
    /* check if there is an accessor for such attribute */
    for(i = 0; i < cls->accessors.count; i++)
    {
        if( attribute_id == cls->accessors.list[i].attribute_id )
        {
            *accessor = &(cls->accessors.list[i]);
            return TCM_STATUS_OK;
        }
    }
    return TCM_STATUS_GENERIC_FAILURE;
}

/*! @fn fbe_terminator_class_management_create_instance(tcm_reference_t class_handle,
                                                        const char * type,
                                                        tcm_reference_t * instance);
 *
 *  @param  class_handle - handle of class for device, instance of which we want to obtain (IN)
 *  @param  type         - type of device (IN)
 *  @param  instance     - created instance of device would be stored here in case of success (OUT)
 */
tcm_status_t fbe_terminator_class_management_create_instance(tcm_reference_t class_handle,
                                                             const char * type,
                                                             tcm_reference_t * instance)
{
    terminator_device_class_t * cls = (terminator_device_class_t *)class_handle;
    if(cls == NULL || instance == NULL)
    {
        return TCM_STATUS_GENERIC_FAILURE;
    }
    return cls->allocate(type, instance);
}

/*! @fn fbe_terminator_class_management_delete_instance(tcm_reference_t class_handle,
                                                             tcm_reference_t instance);
 *
 *  @param  class_handle - handle of class for device, instance of which we want to delete (IN)
 *  @param  type         - type of device (IN)
 *  @param  instance     - instance of device we want to delete (IN)
*/
tcm_status_t fbe_terminator_class_management_delete_instance(tcm_reference_t class_handle,
                                                             const char * type,
                                                             tcm_reference_t instance)
{
    terminator_device_class_t * cls = (terminator_device_class_t *)class_handle;
    if(cls == NULL || instance == 0)
    {
        return TCM_STATUS_GENERIC_FAILURE;
    }
    return cls->destroy(type, instance);
}

