/**************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 **************************************************************************/

/*!**************************************************************************
 * @file fbe_terminator_class_management.h
 ****************************************************************************
 *
 * @brief
 *  This file contains public interface for the Terminator class management module.
 *
 * @date 11/12/2008
 * @author VG
 *
 ***************************************************************************/

/**********************************/
/*        include files           */
/**********************************/


/* 32-bit unsigned */
typedef unsigned int tcm_uint_t;

typedef void * tcm_reference_t;

typedef enum tcm_status_e 
{
    TCM_STATUS_OK = 1,
    TCM_STATUS_NOT_IMPLEMENTED,
    TCM_STATUS_GENERIC_FAILURE
} tcm_status_t;



#define TCM_MAX_ATTR_NAME_LEN   64

#define TCM_MAX_CLASSES     10
#define TCM_MAX_ATTRIBUTES  32


typedef	tcm_status_t (* fbe_terminator_device_allocator)(const char *type, tcm_reference_t *device);
typedef	tcm_status_t (* fbe_terminator_device_destroyer)(const char *type, tcm_reference_t device);

// at a minimum attributes need to be accessable using strings
typedef	tcm_status_t (* terminator_attribute_string_string_setter)(void *device, const char *name, const char *value);
typedef	tcm_status_t (* terminator_attribute_string_string_getter)(void *device, const char *name, char * buffer, tcm_uint_t buffer_len);


// convenience/performance attribute accessors
typedef	tcm_status_t (* terminator_attribute_uint_string_setter)(void *device, tcm_uint_t key, const char *value);
typedef	tcm_status_t (* terminator_attribute_uint_string_getter)(void *device, tcm_uint_t key, char * buffer, tcm_uint_t buffer_len);
typedef	tcm_status_t (* terminator_attribute_uint_uint_getter)(void *device, tcm_uint_t key, tcm_uint_t *value);

/*! @struct terminator_attribute_accessor_s
 *  
 * @brief This structure contains identifiers and accessors used to
 *        translate key/value pairs.  Each device type that the 
 *        Terminator is capable of managing will contain an
 *        instance of terminator_attribute_accessor_s/t that can
 *        be used to access one attribute within a terminator device
 *        of this type
 */
typedef struct terminator_attribute_accessor_s
{
    char attribute_name[TCM_MAX_ATTR_NAME_LEN];      
    tcm_uint_t attribute_id;                    
    terminator_attribute_string_string_setter string_string_setter;
    terminator_attribute_string_string_getter string_string_getter;
    terminator_attribute_uint_string_setter   uint_string_setter;
    terminator_attribute_uint_string_getter   uint_string_getter;
    terminator_attribute_uint_uint_getter     uint_uint_getter;
} terminator_attribute_accessor_t;


/*! @fn tdr_status_t fbe_terminator_class_management_init()
 *  
 *  @brief Function that initializes Terminator class management module.
 *  @return the status of the call.
 */
tcm_status_t fbe_terminator_class_management_init(void);


/*! @fn tdr_status_t fbe_terminator_class_management_destroy()
 *  
 *  @brief Function that destroys Terminator class management module.
 *  @return the status of the call.
 */
tcm_status_t fbe_terminator_class_management_destroy(void);


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
                                                           tcm_reference_t *class_handle);

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
                                                                        );

/*! @fn fbe_terminator_class_management_class_find(char *class_name, tcm_reference_t *class_handle)
 *  @param class_handle - handle of class accessor is added to
 *  @param class_handle - handle to the newly created class 
 *  @return the status of the call.
 */
tcm_status_t fbe_terminator_class_management_class_find(const char *class_name, tcm_reference_t *class_handle);

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
                                                                           terminator_attribute_accessor_t **accessor);

/*! @fn fbe_terminator_class_management_class_accessor_uint_lookup(tcm_reference_t class_handle,
                                                                           uint attribute_id, 
                                                                           terminator_attribute_accessor_t **accessor)
 *  @param class_handle - handle of class accessor is added to
 *  @param attribute_id - id of attribute 
 *  @param accessor     - return pointer to the attribute accessor
 *  @return the status of the call.
 */
tcm_status_t fbe_terminator_class_management_class_accessor_uint_lookup(tcm_reference_t class_handle,
                                                                           tcm_uint_t attribute_id,
                                                                           terminator_attribute_accessor_t **accessor);


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
                                                             tcm_reference_t * instance);

/*! @fn fbe_terminator_class_management_delete_instance(tcm_reference_t class_handle,
                                                             tcm_reference_t instance);
 *  
 *  @param  class_handle - handle of class for device, instance of which we want to delete (IN)
 *  @param  type         - type of device (IN)
 *  @param  instance     - instance of device we want to delete (IN)
*/
tcm_status_t fbe_terminator_class_management_delete_instance(tcm_reference_t class_handle,
                                                             const char * type,
                                                             tcm_reference_t instance);
