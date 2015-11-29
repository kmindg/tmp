/**************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 **************************************************************************/

/*!**************************************************************************
 * @file fbe_terminator_device_classes.h
 ***************************************************************************
 *
 * @brief
 *  This file contains an interface for TCM classes representing main Terminator devices.
 *
 * @version
 *   01/26/2009:  Alanov Alexander created
 * 
 *
 ***************************************************************************/

/* Initialization stuff */
/* function used to initialize class management */
fbe_status_t terminator_device_classes_initialize(void);

/* Allocators */
/* allocator for board */
tcm_status_t terminator_board_allocator(const char * type, tcm_reference_t * device);

/* allocator for port */
tcm_status_t terminator_port_allocator(const char * type, tcm_reference_t * device);

/* allocator for enclosure */
tcm_status_t terminator_enclosure_allocator(const char * type, tcm_reference_t * device);

/* allocator for drive */
tcm_status_t terminator_drive_allocator(const char * type, tcm_reference_t * device);
/* destroyer for drive */
tcm_status_t terminator_drive_destroyer(const char *type, tcm_reference_t device);

/* Getters and Setters */
/* currently only "string-string accessors" are implemented */

/* setters and getters for board */
tcm_status_t terminator_board_attribute_setter(fbe_terminator_device_ptr_t board_handle,
                                                    const char * attribute_name,
                                                    const char * attribute_value);

tcm_status_t terminator_board_attribute_getter(fbe_terminator_device_ptr_t board_handle,
                                                    const char * attribute_name,
                                                    char * buffer,
                                                    tcm_uint_t buffer_len);

/* for port */
tcm_status_t terminator_port_attribute_setter(fbe_terminator_device_ptr_t port_handle,
                                                    const char * attribute_name,
                                                    const char * attribute_value);

tcm_status_t terminator_port_attribute_getter(fbe_terminator_device_ptr_t port_handle,
                                                    const char * attribute_name,
                                                    char * buffer,
                                                    tcm_uint_t buffer_len);

/* for enclosure */
tcm_status_t terminator_enclosure_attribute_setter(fbe_terminator_device_ptr_t enclosure_handle,
                                                    const char * attribute_name,
                                                    const char * attribute_value);

tcm_status_t terminator_enclosure_attribute_getter(fbe_terminator_device_ptr_t enclosure_handle,
                                                    const char * attribute_name,
                                                    char * buffer,
                                                    tcm_uint_t buffer_len);

/* for drive */
tcm_status_t terminator_drive_attribute_setter(fbe_terminator_device_ptr_t drive_handle,
                                                    const char * attribute_name,
                                                    const char * attribute_value);

tcm_status_t terminator_drive_attribute_getter(fbe_terminator_device_ptr_t drive_handle,
                                                    const char * attribute_name,
                                                    char * buffer,
                                                    tcm_uint_t buffer_len);

/* Convenient (and slower) way to invoke a setter (requires many lookups) */
tcm_status_t terminator_set_device_attribute(fbe_terminator_device_ptr_t device_handle,
                                             const char * attribute_name,
                                             const char * attribute_value);

/* Convenient (and slower) way to invoke a getter (requires many lookups) */
tcm_status_t terminator_get_device_attribute(fbe_terminator_device_ptr_t device_handle,
                                                    const char * attribute_name,
                                                    char * attribute_value_buffer,
                                                    fbe_u32_t buffer_length);
