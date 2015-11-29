/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
/*!**************************************************************************
 * @file fbe_api_doxygen_groups.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the definitions of the doxygen groups for FBE.
 *
 * @version
 *   10/2008:  Created. RPF.
 *
 ***************************************************************************/
//--------------------------------------------------------------------------
//  Need to add the define group here for each of the high level APIs that 
//  you want it to show up in the picture. 
//
//  NOTE:  Once you add the new define group here, you need to add
//         to the "subgraph cluster_0" drawing below.
//--------------------------------------------------------------------------
/*! @defgroup fbe_api_classes_usurper_interface FBE API Classes Usurper Interface
 *  This is the set of definitions that comprise the usurper 
 *  interfaces for all the FBE API classes.
 */ 

/*! @defgroup fbe_api_classes FBE API Classes
 *  This is the set of definitions for all FBE API classes.
 */ 

/*! @defgroup fbe_api_storage_extent_package_class FBE Storage Extent Package APIs
 *  @brief This is the set of definitions for FBE Storage Extent Package APIs.
 *  @ingroup fbe_api_storage_extent_package_interface
 */

/*! @defgroup fbe_physical_package_class FBE Physical Package APIs
 *  @brief This is the set of definitions for FBE Physical Package APIs.
 *  @ingroup fbe_api_physical_package_interface
 */ 

/*! @defgroup fbe_system_package_class FBE System APIs
 *  @brief This is the set of definitions for FBE System APIs.
 *  @ingroup fbe_api_system_package_interface
 */ 

/*! @defgroup fbe_api_neit_package_class FBE NEIT Package APIs 
 *  @brief This is the set of definitions for FBE NEIT Package APIs.
 *  @ingroup fbe_api_neit_package_interface
 */ 

/*! @defgroup fbe_api_esp_interface_class FBE ESP APIs 
 *  @brief This is the set of definitions for FBE ESP APIs.
 *  @ingroup fbe_api_esp_interface
 */ 

/*! @defgroup fbe_hash_table FBE Hash Table 
 *  @brief This is the set of definitions for FBE Hash Tables.
 */ 

/*! @defgroup fbe_database Database Service 
 *  @brief This is the set of definitions for the Database Service.
 */ 

/*************************************************** 
 * This is the information that shows up on the main Doxygen page. 
 * The dot defined below is a hierarchy of the fbe classes 
 * As we add classes we should update the diagram below. 
 ***************************************************/

/*! @mainpage FBE API Documentation
 *
 * @dot 
digraph fbe_classes_graph { 
     graph [color = "black"
label="FBE API Hierarchy - Click on the class to see info" 
center="true" bgcolor="white"]; 

node [ style="filled", fillcolor="blue", fontcolor="white"];

    subgraph cluster_0 {
        style=filled;
        color=lightgrey;
        node [style=filled,color=white];
        label = "FBE API";
        fbe_api_storage_extent_package_class[URL="\ref fbe_api_storage_extent_package_class", label="FBE API SEP Class", 
fontcolor="black", fillcolor="lightgoldenrod3"];
        fbe_system_package_class[URL="\ref fbe_system_package_class", label="FBE API System Class", 
fontcolor="black", fillcolor="lightgoldenrod3"];
        fbe_physical_package_class[URL="\ref fbe_physical_package_class", label="FBE API Physical Class", 
fontcolor="black", fillcolor="lightgoldenrod3"];
        fbe_api_neit_package_class[URL="\ref fbe_api_neit_package_class", label="FBE API NEIT Class", 
fontcolor="black", fillcolor="lightgoldenrod3"];
        fbe_api_esp_interface_class[URL="\ref fbe_api_esp_interface_class", label="FBE API ESP Class", 
fontcolor="black", fillcolor="lightgoldenrod3"];
    }
} 
* @enddot 
* 
* FBE API Classes - Are owned by the 
  <A HREF="mailto:USD_CE_DCummins@emc.com"> FBE Architecture Group</A>
* 
* Intermediate classes are classes that cannot be instantiated by themselves. 
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
* @section more_info More Information
* @ref fbe_api_classes contains documentation on each of the FBE API classes.
* 
* 
*/

/*************************
 * end file fbe_api_doxygen_groups.h
 *************************/
