/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_doxygen_howto.h
 ***************************************************************************
 *
 * @brief
 *  This file contains some general information about how to use doxygen.
 *
 * @version
 *   10/29/2008:  Created. RPF
 *
 ***************************************************************************/

/*! @page doxygen_source_file_templates_page Source File Templates 
   
  Below are examples of source files. 
  These can be found in disk\\fbe\\doc\\Doxygen 
   
  We have templates for: 
  - @ref doxygen_class_template_sec can be used when creating Doxygen 
    documentation for a new FBE Class object.
  - @ref doxygen_source_file_template_sec is an example of a C source file and 
    contains example file, function header and global varible Doxygen
    documentation.
  - @ref doxygen_header_file_template_sec of a C header file and contains 
    example file, \#define, structure, and enumeration Doxygen documentation.
   
  @section doxygen_class_template_sec FBE Class Template 
  This is a template of an FBE Class file.  These are the kinds of definitions 
  that need to be included with an FBE Class in order to properly document the 
  FBE Class. 
   
  The Doxygen Documentation for this file can be found here: @ref fbe_template_class.h 
  @include fbe_template_class.h 
   
  @section doxygen_source_file_template_sec FBE C Source File Template 
  This is an example C source file which illustrates the doxygen file header and
  example C function header. 
  
  The Doxygen Documentation for this file can be found here: @ref fbe_template_source_file.c
  @include fbe_template_source_file.c  
   
  @section doxygen_header_file_template_sec FBE C Header File Template 
  This is an example C Header file with examples for \#define, structure and 
  enumeration.  
  
  The Doxygen Documentation for this file can be found here: @ref fbe_template_header_file.h
  @include fbe_template_header_file.h
 
  */

/*! @page doxygen_groups_training How do I add Doxygen Documentation? 

   - @ref doxygen_groups_training_intro_sec
   - @ref doxygen_groups_training_detail_sec
  <hr> 
  @section doxygen_groups_training_intro_sec Adding Doxgen Groups for an FBE Object 
   
   -# First add a few new groups for your FBE Object in your object's .h file.
      So for example with the logical drive we added these group definitions
      to the file disk\interface\fbe\fbe_logical_drive.h
   -# Add a "Class group", which will show up on the modules page in the correct place of the hierarchy.
      Note how we add the ingroup for our base class, to put this object just below our base class in the hierarchy.
@verbatim
    /*! @defgroup logical_drive_class Logical Drive Class
     *  This is the set of definitions for the logical drive.
     *  @ingroup fbe_base_discovered
     */ 
@endverbatim
   -# Next add a group for your object's userper interfaces.  This will show up on the modules page under
      the list of all available usurper interfaces.
@verbatim
    /*! @defgroup logical_drive_usurper_interface Logical Drive Usurper Interface
     *  This is the set of definitions that comprise the logical drive class
     *  usurper interface.
     *  @ingroup fbe_classes_usurper_interface
     * @{
     */
       /* This is where to put the usurper interface definitions. */

    /*! @} */ /* end of group logical_drive_usurper_interface */
@endverbatim
    It is also possible to define the group separately, and then add to the group with the @@addtogroup tag.
@verbatim
    /*! @addtogroup logical_drive_usurper_interface Logical Drive Usurper Interface
     * @{ 
     */
       /* This is where to put the usurper interface definitions. */

    /*! @} */ /* end of group logical_drive_usurper_interface */
@endverbatim
   -# Next, add a group definition for all your files.
      Note that we add this group to two separate classes so that this shows up
      under both our object and all objects.
@verbatim
    /*! @defgroup logical_drive_class_files Logical Drive Class Files
     *  This is the set of files for the logical drive.
     *  @ingroup fbe_classes
     *  @ingroup logical_drive_class
     */
@endverbatim
   -# For each of your files, you can add an @@ingroup tag for each of your files.
      An example of this for the logical drive is below.
@verbatim
 /*!**************************************************************************
  * @file fbe_logical_drive_executor.c
  ***************************************************************************
  *
  * @brief
  *  This file contains the executer functions for the logical drive.
  * 
  * @ingroup logical_drive_class
  *
  ***************************************************************************/
@endverbatim

  <hr> 
  @section doxygen_training_detail_sec Adding Doxygen comments in your files.
   -# As shown above, each file should have a header, which has a @@file tag.  
      This tell Doxygen to extract the documentation for your file.
   -# For each function, add a header that includes the @@fn tag like below.
@verbatim
/*!***************************************************************
 * @fn example_io_entry(fbe_object_handle_t object_handle,
 *                                fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function is the entry point for
 *  functional transport operations to the object.
 *
 * @param object_handle - The object handle.
 * @param packet_p - The io packet that is arriving.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_GENERIC_FAILURE if transport is unknown,
 *                 otherwise we return the status of the transport handler.
 *
 ****************************************************************/
@endverbatim
   -# For each structure, add a header that includes the @@struct tag like below.
      Note that each field can be documented with a
@verbatim
      /*! */ comment before the field, 
      or 
      /*!< */ comment on the same line after the field.
@endverbatim
   @dontinclude fbe_template_header_file.h 
   @skip example_fbe_block_transport_identify_t
   @until example_fbe_block_transport_identify_t;
   -# For each enum, add a header that includes an @@enum tag like below.
      Note that each enumeration value can be documented with a
@verbatim
      /*! */ comment before the enumeration, 
      or 
      /*!< */ comment on the same line after the enumeration.
@endverbatim
   @dontinclude fbe_template_header_file.h 
   @skip example_fbe_payload_block_operation_opcode_t
   @until example_fbe_payload_block_operation_opcode_t;
   -# For each define, add a header with a @@def tag like below.
   @dontinclude fbe_template_header_file.h 
   @skip FBE_TEST
   @until #define FBE_TEST
   -# For each global variable, add a header with a @@var tag like below.
   @dontinclude fbe_template_source_file.c
   @skip fbe_test_object
   @until fbe_test_object;
*/

/*************************
 * end file fbe_doxygen_howto.h
 *************************/
