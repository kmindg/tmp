/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
/*!**************************************************************************
 * @file emcpal_api_doxygen_groups.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the definitions of the doxygen groups for EmcPAL.
 *
 * @version
 *   3/2011:  Created. JJY
 *
 ***************************************************************************/
//--------------------------------------------------------------------------
//  Need to add the define group here for each of the high level APIs that 
//  you want it to show up in the picture. 
//
//  NOTE:  Once you add the new define group here, you need to add
//         to the "subgraph cluster_0" drawing below.
//--------------------------------------------------------------------------

/*! @defgroup emcpal_client EmcPAL Client Facilities
 *  @brief This describes the EmcPAL interfaces for Client support
 */ 

/*! @defgroup emcpal_concurrent EmcPAL Concurrent Execution Facilities
 *  This group defines the facilities provided by Emcpal for Process and
 *  Thread management.
 */ 

/*! @defgroup emcpal_process EmcPAL Process Management Facilities
 *  This group defines the various APIs provided by Emcpal for the management
 *  of processes.
 *  @ingroup emcpal_concurrent
 */

/*! @defgroup emcpal_thread EmcPAL Thread Management Facilities
 *  @brief Various APIs provided by Emcpal for the management of threads.
 *  @ingroup emcpal_concurrent
 */

/*! @defgroup emcpal_configuration EmcPAL Configuration
 *  @brief Describes the methods and techniques by which a software component is
 *  enabled for EmcPAL operation by setting it up as a client.
 */ 

/*! @defgroup emcpal_core_structures EmcPAL Core Structures
 *  @brief Describes major data EmcPAL structures.
 */ 

/*! @defgroup emcpal_driver_utilities EmcPAL Driver Utilities
 *  @brief Functions for obtaining an EmcPAL Client object pointer (required as an input for many EmcPAL APIs),
 *  EmcPAL driver name, registry path, etc.
 */ 

/*! @defgroup emcpal_irps EmcPAL IRP Abstraction
 *  @brief This describes the EmcPAL IRP abstractions and related items
 */ 

/*! @defgroup emcpal_memory EmcPAL Memory Management APIs
 *  @brief This is the set of definitions for EmcPAL provided Memory Management facilties.
 */

/*! @defgroup emcpal_memory_utils EmcPAL Memory Utilities
 *  @brief This is the set of definitions for EmcPAL provided Memory comparison and manipulation facilties.
 */

/*! @defgroup emcpal_misc EmcPAL Miscellaneous
 *  @brief This is a place holder for documentation that doesn't
 *  seem to fit elsewhere.  This will be reorganized in the future
 */ 

/*! @defgroup emcpal_portio EmcPAL Port IO
 *  @brief EmcPAL Port I/O APIs
 */ 

/*! @defgroup emcpal_registry EmcPAL Registry APIs
 *  @brief This is the set of definitions for EmcPAL provided Registry facilties.
 */

/*! @defgroup emcpal_status_codes EmcPAL Status Codes
 *  @brief This describes the set of Status Codes defined for EmcPAL.
 */ 

/*! @defgroup emcpal_synch_coord EmcPAL Synchronization and Coordination Facilities
 *  @brief This is the set of definitions for the various methods of of synchronization and
 *  coordinated access provided by EmcPAL.  This includes methods for providing mutual
 *  exclusion of protected resources
 */ 

/*! @defgroup emcpal_callback_events EmcPAL Callback Events
 *  @brief This is the set of definitions for the management of Callback type
 *  events (aka DPCs in Windows)as provided by EmcPAL
 *  @ingroup emcpal_synch_coord
 */ 

/*! @defgroup emcpal_interlocked EmcPAL Interlocked Operations Facilities
 *  @brief This is the set of definitions for the management of Interlocked
 *  operations as provided by EmcPAL
 *  @ingroup emcpal_synch_coord
 */ 

/*! @defgroup emcpal_mutex EmcPAL Mutual Exclusion Facilities
 *  This is the set of definitions for Mutexes as 
 *  provided by EmcPAL
 *  @ingroup emcpal_synch_coord
 */ 

/*! @defgroup emcpal_recmutex EmcPAL Recursive Mutual Exclusion Facilities
 *  This is the set of definitions for Recursive Mutexes as 
 *  provided by EmcPAL
 *  @ingroup emcpal_synch_coord
 */ 

/*! @defgroup emcpal_rendezvous__events EmcPAL Rendezvous Events
 *  @brief This is the set of definitions for the management of Rendezvous as  
 *  provided by EmcPAL
 *  @ingroup emcpal_synch_coord
 */ 

/*! @defgroup emcpal_semaphore EmcPAL Semaphore Facilities
 *  @brief This is the set of definitions for the management of Semaphores as  
 *  provided by EmcPAL
 *  @ingroup emcpal_synch_coord
 */ 

/*! @defgroup emcpal_spinlock EmcPAL Spinlock Facilities
 *  This is the set of definitions for Spinlock facilities as 
 *  provided by EmcPAL
 *  @ingroup emcpal_synch_coord
 */ 

/*! @defgroup emcpal_sync_events EmcPAL Synchronization Events
 *  @brief This is the set of definitions for the management of Synchronization Events as  
 *  provided by EmcPAL
 *  @ingroup emcpal_synch_coord
 */ 

/*! @defgroup emcpal_time EmcPAL Time Facilities
 *  @brief This is the set of definitions for all Time related features of EmcPAL. Note that the EmcUTIL Time APIs are more flexible.
 */ 

/*! @defgroup emcpal_timer EmcPAL Timer Facilities
 *  @brief This is the set of definitions for all Timer related features of EmcPAL
 */ 

/*! @defgroup emcutil_csxshell EmcUTIL CSX Shell
 *  @brief The set of definitions for EmcUTIL CSX shell.
 */ 

/*! @defgroup emcutil_devioctl EmcUTIL Device Ioctl APIs
 *  @brief EmcUTIL Device IOCTL APIs.
 */ 

/*! @defgroup emcutil_eventlog EmcUTIL Event Logging
 *  @brief This is the set of definitions for event logging
 */

/*! @defgroup emcutil_misc EmcUTIL Miscellaneous
 *  @brief EmcUTIL Miscellaneous Items and APIs.
 */ 

/*! @defgroup emcutil_status EmcUTIL Status Codes and related APIs
 *  @brief EmcUTIL Status Codes returned by EmcUTIL APIs
 *  @ingroup emcutil_misc
 */

/*! @defgroup emcutil_memory EmcUTIL Memory related APIs
 *  @brief EmcUTIL Memory related APIs.
 *  @ingroup emcutil_misc
 */

/*! @defgroup emcutil_string EmcUTIL String Conversion APIs
 *  @brief The set of definitions for EmcUTIL String Conversion APIs
 *  @ingroup emcutil_misc
 */

/*! @defgroup emcutil_registry EmcUTIL Registry
 *  @brief This is the set of definitions for EmcUTIL provided Registry facilties.
 */

/*! @defgroup emcutil_registry_pla EmcUTIL Replacement Registry API
 *  @brief The set of definitions for EmcUTIL provided replacement API for EmcPAL Registry API
 *  @ingroup emcutil_registry
 */

/*! @defgroup emcutil_registry_abs EmcUTIL Abstracted Registry API
 *  @brief The set of definitions for EmcUTIL provided abstracted registry API.
 *  @ingroup emcutil_registry
 */

/*! @defgroup emcutil_time EmcUTIL Time APIs
 *  @brief Set of definitions for the EmcUTIL time APIs.
 */ 

/*************************
 * end file emcpal_api_doxygen_groups.h
 *************************/
