/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
/*!**************************************************************************
 * @file fbe_doxygen_groups.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the definitions of the doxygen groups for FBE.
 *
 * @version
 *   10/2008:  Created. RPF.
 *
 ***************************************************************************/

/*! @defgroup fbe_classes_usurper_interface FBE Classes Usurper Interface
 *  This is the set of definitions that comprise the usurper 
 *  interfaces for all the FBE classes.
 */ 
/*! @defgroup fbe_classes FBE Classes
 *  This is the set of definitions for all FBE classes.
 */ 

/*! @defgroup fbe_base_object FBE Base Class
 *  The base object is the foundation for all FBE objects.
 *  All FBE objects "inherit" from this object.
 */ 
 /*! @defgroup fbe_base_discovered FBE Base Discovered Class
  *  Base discovered class is inherited by all classes that are discovered by
  *  another object.
  *  @ingroup fbe_base_object
  */
   
 /*! @defgroup fbe_base_discovering FBE Base Discovering Class
  *  Base discovering class is inherited by all classes that discover other
  *  objects.
  *  @ingroup fbe_base_discovered
  */


/*! @defgroup fbe_base_object FBE Base Class
 *  The base object is the foundation for all FBE objects.
 *  All FBE objects "inherit" from this object.
 */ 

/*! @defgroup fbe_interfaces FBE Infrastructure Interfaces
  *   This is the set of definitions that make up the FBE Infrastructure APIs. 
  */

/*! @defgroup fbe_transport_interfaces FBE Transport Interfaces
  *   This is the set of definitions that make up the FBE Transport Infrastructure APIs.
  * @ingroup fbe_interfaces
  */

/*************************************************** 
 * This is the information that shows up on the main Doxygen page. 
 * The dot defined below is a hierarchy of the fbe classes 
 * As we add classes we should update the diagram below. 
 ***************************************************/

/*! @mainpage FBE Documentation
*
* @dot 
digraph fbe_classes_graph { 
     graph [color = "black"
label="FBE Class Hierarchy - Click on the class to see info on the class" 
center="true" bgcolor="white"]; 

node [ style="filled", fillcolor="blue", fontcolor="white"];

	fbe_base_discovering -> fbe_base_drive; 
	fbe_base_discovering -> fbe_base_board;
	fbe_base_discovering -> fbe_base_enclosure;
	fbe_base_discovering -> fbe_base_port;
	fbe_base_object [URL="\ref fbe_base_object", fontcolor="black", fillcolor="lightgoldenrod3"];
	fbe_base_object -> fbe_base_discovered;
	fbe_base_discovered [URL="\ref fbe_base_discovered", fontcolor="black", fillcolor="lightgoldenrod3"];
	fbe_base_discovering [URL="\ref fbe_base_discovering", fontcolor="black", fillcolor="lightgoldenrod3"];
	fbe_base_discovered -> fbe_base_discovering;
	fbe_base_board [fontcolor="black", fillcolor="lightgoldenrod3"];
	fbe_base_port [fontcolor="black", fillcolor="lightgoldenrod3"];
	fbe_base_enclosure [fontcolor="black", fillcolor="lightgoldenrod3"];
	fbe_base_drive [fontcolor="black", fillcolor="lightgoldenrod3"];
	fbe_logical_drive [URL="\ref logical_drive_class"];
	fbe_base_discovered -> fbe_logical_drive;

	fbe_base_drive -> fbe_sas_physical_drive;
	fbe_base_drive -> fbe_sata_physical_drive;
	fbe_sas_enclosure[fillcolor="gray",fontcolor="black"];
	fbe_base_enclosure [URL="\ref fbe_enclosure_class", fontcolor="black", fillcolor="lightgoldenrod3"];
	fbe_base_board -> fbe_fleet_board;
    fbe_base_board ->fbe_magnum_board;
	fbe_sas_viper_enclosure [URL="\ref fbe_enclosure_class"];
	fbe_base_enclosure -> fbe_sas_enclosure;
	fbe_sas_enclosure -> fbe_sas_viper_enclosure;
	
	fbe_base_port -> fbe_sas_port; 

      subgraph cluster_0 {
		style=filled;
		color=lightgrey;
		node [style=filled,color=white];
		label = "Key";
		Infrastructure_Class[fontcolor="black", fillcolor="lightgoldenrod3" label="FBE Infrastructure Class"];
		Intermediate_Class[fillcolor="gray", fontcolor="black", label="Intermediate Class"];
		Leaf_Class[label = "Leaf Class"];
		Infrastructure_Class -> Intermediate_Class;
		Intermediate_Class -> Leaf_Class;
	}
} 
* @enddot 
* 
* FBE Infrastructure Classes - Are owned by the 
  <A HREF="mailto:fbearchitecture@emc.com"> FBE Architecture Group</A>
* 
* Intermediate classes are classes that cannot be instantiated by themselves. 
* 
* Leaf classes are owned by individual component groups in 
 <A HREF="mailto:fbephysical@emc.com">FBE Physical</A> or
 <A HREF="mailto:fbelogical@emc.com">FBE Logical</A>.
*
* <hr>
* @section start Getting Started
*
* Click on the class in the hierachy above to view documentation on that object.
*
* The <A href ="modules.html">Modules</A> tab ontains a list of the
* documentation sections.
* 
* The <A href ="globals.html">Globals</A> tab has a list of all entities.
* 
* <hr> 
* @section using_doxygen Using Doxygen 
* 
* <A href ="http://opseroom01.corp.emc.com/eRoomReq/Files/SPOmidrangesysdiv/EMCSPOMidFlreBacEnd/0_124741/Doxygen%20Training.ppt"> 
* Doxygen Training Presentation</A> is located in the eRoom and explains more
* about Doxygen and how to use it. 
* 
* @ref doxygen_groups_training 
*  This section contains shows how to add Doxygen documentation on 
   your source code.
* - @ref doxygen_training_detail_sec - Explains how to add Doxygen documentation 
  to your code.  We go through how to document files, functions, structures,
  enumerations, and more.
* - @ref doxygen_source_file_templates_page - Contains templates that you can 
  use to get started with Doxygen right away. For each template there is a link
  showing the Doxygen documentation that gets generated for the template.  These
  templates could be used by folks as a starting point when creating new files
  or can be used as a guide when adding documentation to existing files.
* - @ref doxygen_groups_training_intro_sec - Explains how to create Doxygen 
  Groups to organize your code documentation into groups that are easy for
  people to find code and navigate the code.  All "groups" show up on the
  "modules" tab in Doxygen.
* 
* <hr>
* @section more_info More Information
* @ref fbe_classes contains documentation on each of the FBE classes.
* 
* @ref fbe_interfaces has the documentation on all the FBE Infrastructure
* Interfaces.
* 
*/

/*************************
 * end file fbe_doxygen_groups.h
 *************************/
