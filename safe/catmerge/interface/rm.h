// FileHeader
//
//   File:
//      rm.h
//
//   Description:
//      This file contains the definitions that specify the exported interface to the
//      Remote Mirroring driver.  This interface is exported, but it is expected that
//      there is only ONE consumer of this interface, the Remote Mirror Administration
//      DLL.  Because of this fact, this interface is still considered private to
//      Remote Mirroring and should not be utilized by any piece of software outside
//      of our own domain.  Should anyone want to talk with the Remote Mirror driver,
//      it should utilize the Administration DLL interface, for this is the one true
//      interface that Remote Mirrors will support.
//
//      The single consumer of the services provided by the Remote Mirror driver should
//      find everything they need in this file.  If we need something more, let's make
//      sure it is in this file, please do not go poking around source code and duplicate
//      values and interfaces for use.  Thankfully, that's kind of hard to do with
//      a driver called from user space :-)
//
//   Author(s):
//      Paul A. Cooke
//
//   Change Log:
//      1.15    06/04   CC      Added Domino property.
//      1.14    09/00   ALT     Defined the secondary read status modification IOCTL.
//      1.13    07/00   ALT     Added image locality enum and data item.
//      1.12    06/00   ALT     Added fields to GetGeneric out buffer.
//      1.11    03/00   ALT     Added default Max Memory definition.
//      1.10    03/00   WPH     Added shutdown and quiesce buffers
//      1.09    02/00   WPH     Added destroyOrphanImageFlag
//      1.08    02/99   ALT     Added some debug options for a slave.
//      1.07    01/00   WPH     Added in enum QUIESCE/SHUTDOWN/REASSIGN
//      1.06    12/99   WPH     Changed default image recovery policy to auto
//                              Added define for MinimumWriteIntentSize
//                              Added PromoteSlave return structure
//      1.05    09/99   ALT     Added print databases and print statistics to
//                              list of options for "Modify Debug Parameters"
//                              IOCTL.
//      1.04    08/99   WPH     Changed name of attribute operations to property
//                              operations and converted MaximumMissingImages to
//                              Synch_Transition_Time
//      1.03    08/99   WPH     Moved Admin Enumeration to RemoteMirrorAdminExport.h
//      1.02    06/99   ALT     Fixing depenency on "CmiUpperInterface.h".
//                              Adding IOCTL for modifying RmDebugMask and
//                              causing a break point "on the fly".
//      1.01    05/99   GAR     Add fracture/sync support
//      1.00    01/99   PAC     Initial creation...
//
//  Copyright (c) 1999 - 2010, EMC Corporation.  All rights reserved.
//  Licensed Material -- Property of EMC Corporation.
//
//  *** NOTE : tabs in this file are set to every 4 spaces. ***
//
#ifndef  _K10_RM_H_
#define  _K10_RM_H_

//
//  OS Include files...
//
#include "EmcPAL.h"
//#include <ctype.h>
#include "csx_ext.h"

//
//  K10 include files...
//
#include "k10defs.h"
#include "K10RemoteMirrorAdminExport.h"
#include "K10DiskDriverAdminExport.h"

#include "ndu_toc_common.h"

// DEFINEs
///////////////////////////////////////////////////////////////////////////////////////////////
//
//  The following are all of the external defines that one needs to know.
//
///////////////////////////////////////////////////////////////////////////////////////////////

//
//  The following defines the user space accessible name of the RM device.  Both Unicode
//  and a plain old ASCII name are supplied depending upon how one constructs their
//  user space application.
//
#define     RM_DEVICE_NAME_WCHAR            L"\\\\.\\CLARiiONrm"
#define     RM_DEVICE_NAME_CHAR             "\\\\.\\CLARiiONrm"

//
//  The maximum number of logical units we can handle.  This is an order of
//  magnitude larger than the number of LUs that FLARE will support for the K10 at
//  FCS, so it should be sufficient for a while (for some definition of 'a while').
//
#define RM_MAX_LUS                          2048

//
//  The minimum limits we will impose if we cannot exhume the actual Feature Limits
//  from the Registry.  These numbers are based on the limits used by the smallest 
//  array model (CX3-10) supported in the Taurus release.
//
#define RM_MIN_LIMIT_ON_MIRRORS             50
#define RM_MIN_LIMIT_ON_MIRRORS_USING_WIL   25
#define RM_MIN_LIMIT_ON_CGS                 8
#define RM_MIN_LIMIT_ON_CG_MEMBERS          8

//
//  The maximum number of characters in the name of a Remote Mirror instantiation.
//
#define RM_MIRROR_NAME_MAX                 256 


//
//  The maximum number of characters in the description of a Remote Mirror instantiation.
//
#define RM_MIRROR_DESC_MAX                  512


//
//  The maximum number of slaves that Remote Mirrors will support.
//
#define RM_MAX_SLAVES                       2

//
//  The maximum number of slaves that User is able to add.
//  quick test to see if this may be a problem
//
#define RM_MAX_SLAVES_EXTERNAL_INFO         2

//
//  The maximum number of FLARE Logical Units that can be grouped together to form
//  a Remote Mirrors Logical Unit Array Set.
//
#define RM_MAX_LUS_PER_ARRAY_SET            8

//
//  The following defines the maximum value for the amount of memory the driver
//  can allocate out of NonPagedPool.  This value is currently set at 10MB.
//
#define RM_DEFAULT_MAX_MEMORY_IN_BYTES      (10*1024*1024)

//
//  The following defines the maximum number of attempts to open PSM during
//  the admin init operation.
//
#define RM_MAX_ADMIN_INIT_ATTEMPTS      10

//
//  The following defines the maximum number in the sync order operation
//  Once this number is reached it puts the image at the head
//  of the list to be synchronized.  Currently an arbitrary number
//  until find a reason to set higher or lower
//
#define MAX_SYNCH_ORDER                 100

//
//  The following defines the maximum number of LUs to be
//  used in a single mirror.
//
#define MAX_LUS_IN_MIRROR               1

//
//  Definitions for default mirror property values.
//
#define RM_DEFAULT_MIRROR_MISSING_IMAGES            CSX_U16_MAX
#define RM_DEFAULT_SYNCH_TRANSITION_TIME            60
#define RM_DEFAULT_MIRROR_IMAGES_REQUIRED           0
#define RM_DEFAULT_MIRROR_SYNC_PRIORITY             0
#define RM_DEFAULT_MIRROR_EXTENT_SIZE               1024
#define RM_DEFAULT_MINIMUM_WRITE_INTENT_SIZE        250000 //  128 Meg = 250000 blocks * 512 Byte/block


//
//  Define the current version of our API.
//

#define RM_CURRENT_API_REVISION                         5 // set to 5 for CG


//
// Default Delta Token - when used will trigger a full poll response from the driver.
// 
#define RM_POLL_DEFAULT_TOKEN                       0L



#define RM_MAKE_ADMIN_ERROR( ErrorNum ) \
    ( ErrorNum | K10_ERROR_BASE_RM )


#define RM_MAKE_KERNEL_ERROR( ErrorNum ) \
    ( ( ErrorNum & 0x0000FFFF ) | K10_ERROR_BASE_RM )


// TYPEDEFs
///////////////////////////////////////////////////////////////////////////////////////////////
//
//  The following are all of the external structure definitions that one needs to know.
//
///////////////////////////////////////////////////////////////////////////////////////////////



//
//  The Token used to demarcate the boundaries of a Delta Poll. 
//
typedef EMCPAL_LARGE_INTEGER       RM_POLL_DELTA_TOKEN, *PRM_POLL_DELTA_TOKEN;


//  The Remote Mirror Cookie structure is used to track the state of a Mirror instantiation.
//  It's primary purpose is to track Mirror membership changes.  Mirror membership events
//  include adding a slave, deleting a slave, fracturing an image, completion of a slave
//  image synchronization and any other event that could cause consistency problems in the
//  event of a failure of the mirror master and/or one or more slaves.
//
//  K10ArrayId
//      The unique WWN for the K10 that is the master of the mirror.
//
//  AdministrativeActionCount
//      The number of times that an administrative action has taken place on this
//      K10.  This count is stored persistently.  This count is used instead of
//      a time stamp to protect against time skew between two SPs.
//
typedef struct  _RM_COOKIE
{
    K10_ARRAY_ID        K10ArrayId;

    EMCPAL_LARGE_INTEGER       AdministrativeActionCount;

}   RM_COOKIE, *PRM_COOKIE;


//
//  The Remote Mirror Mirror ID is used to uniquely identify a Mirror.  The ID is a
//  WWID constructed when the mirror is created and is thus gauranteed to be unique
//  accross the universe (in theory anyway).
//
typedef K10_WWID        RM_MIRROR_ID, *PRM_MIRROR_ID;


//
//  The Remote Mirror Mirror Name is used to provide a human choosable monicer for
//  the mirror's ID (Strings are easier to remember than gibberish).  The name is
//  intended for display by an administrative program and is treated in a complete
//  opaque manor by Remote Mirrors.
//
//  e.g.  "Oracle DB 1"
//
typedef CHAR            RM_MIRROR_NAME[ RM_MIRROR_NAME_MAX ];


//
//  The Remote Mirror Mirror Description is used to provide a human choosable description
//  of the mirror's name.  The description is intended for display by an administrative
//  program to provide additional clarification to the name and is treated in a complete
//  opaque manor by Remote Mirrors.
//
//  e.g.  "The Oracle Database for the Accounting Department."
//
typedef CHAR            RM_MIRROR_DESC[ RM_MIRROR_DESC_MAX ];



#define RM_DEFAULT_MIRROR_STATE                 RM_MIRROR_STATE_ACTIVE



#define RM_DEFAULT_IMAGE_STATE                  RM_IMAGE_STATE_OUT_OF_SYNC;

//
//  The Remote Mirror Write Policy controls when write requests are forwarded to slave
//  images in relation to the acknowledgement to the host.  In all cases, the host is
//  never acknowledged until a write request is written to stable storage on the master.
//
//  RM_WRITE_POLICY_SYNCHRONOUS
//      This policy specifies that writes are written to slave images before host
//      acknowledgement.
//
//  RM_WRITE_POLICY_ASYNCHRONOUS
//      This policy specifies that writes are acknowledged to the host before they are
//      written to slave images.
//
typedef enum _RM_WRITE_POLICY
{
    RM_WRITE_POLICY_INVALID = -1,
    RM_WRITE_POLICY_SYNCHRONOUS,
    RM_WRITE_POLICY_ASYNCHRONOUS,

    //  Add new values here...

    RM_WRITE_POLICY_MAX

}   RM_WRITE_POLICY, *PRM_WRITE_POLICY;

#define RM_DEFAULT_MIRROR_WRITE_POLICY          RM_WRITE_POLICY_SYNCHRONOUS


//
//  The Remote Mirror Activation Policy controls in what state a mirror will activate to
//  upon the startup of the K10s that comprise a Mirror.
//
//  RM_ACTIVATION_POLICY_AUTO_ACTIVATE
//      This value indicates that a mirror will transition to RM_MIRROR_STATE_ACTIVE upon
//      startup if all conditions required for an active mirror are satisfied.
//      If all conditions are not satisfied, the Mirror will transition to
//      RM_MIRROR_STATE_ATTENTION.
//
//  RM_ACTIVATION_POLICY_ADMIN_ACTIVATE
//      This value indicates that a mirror will not transition to the
//      RM_MIRROR_STATE_ACTIVE upon startup if all conditions required for an active
//      mirror are satisfied.  The user must initiate the transition to the active state.
//
typedef enum _RM_ACTIVATION_POLICY
{
    RM_ACTIVATION_POLICY_INVALID = -1,
    RM_ACTIVATION_POLICY_AUTO_ACTIVATE,
    RM_ACTIVATION_POLICY_ADMIN_ACTIVATE,

    //  Add new values here...

    RM_ACTIVATION_POLICY_MAX

}   RM_ACTIVATION_POLICY, *PRM_ACTIVATION_POLICY;

#define RM_DEFAULT_MIRROR_ACTIVATION_POLICY     RM_ACTIVATION_POLICY_AUTO_ACTIVATE


//
//  The Remote Mirror Image Role indicates what role an image is playing within a mirror
//  instantiation.
//
//  RM_IMAGE_ROLE_MASTER
//      This value indicates that an image is a master image and is afforded all rights
//      and privileges associated with a master image.
//
//  RM_IMAGE_ROLE_SLAVE
//      This value indicates that an image is a slave image and should act accordingly.
//
typedef enum _RM_IMAGE_ROLE
{
    RM_IMAGE_ROLE_INVALID = -1,
    RM_IMAGE_ROLE_MASTER,
    RM_IMAGE_ROLE_SLAVE,

    //  Add new values here...

    RM_IMAGE_ROLE_MAX

}   RM_IMAGE_ROLE, *PRM_IMAGE_ROLE;


//
//  The Remote Mirror Image Recovery Policy is used to determine how a slave image is
//  recovered after a failure of the slave.
//
//  RM_IMAGE_RECOVERY_POLICY_NONE
//      This value indicates that recovery will not take place without administrative action.
//
//  RM_IMAGE_RECOVERY_POLICY_AUTO,
//      This value indicates that recovery is automatic once a slave is determined to be
//      once again reachable.
//
typedef enum _RM_IMAGE_RECOVERY_POLICY
{
    RM_IMAGE_RECOVERY_POLICY_INVALID = -1,
    RM_IMAGE_RECOVERY_POLICY_NONE,
    RM_IMAGE_RECOVERY_POLICY_AUTO,

    //  Add new values here...

    RM_IMAGE_RECOVERY_POLICY_MAX

}   RM_IMAGE_RECOVERY_POLICY, *PRM_IMAGE_RECOVERY_POLICY;

#define RM_DEFAULT_IMAGE_RECOVERY_POLICY            RM_IMAGE_RECOVERY_POLICY_AUTO


//
//  The Remote Mirror Logical Unit Utilization Status is used to indicate on a per LU
//  basis the results of a utilization request.  Such requests occur when a mirror is
//  created and when a secondary is added.  If each LU has a SUCCESS status, then the create
//  or add request will succeed.  If any LU has another status, then the create or add
//  request will fail and a LU status will be returned to indicate which LU caused
//  the problem.
//
typedef enum _RM_LU_UTILIZATION_STATUS
{
    RM_LU_UTILIZATION_STATUS_INVALID = -1,
    RM_LU_UTILIZATION_STATUS_SUCCESS,
    RM_LU_UTILIZATION_STATUS_NOT_BOUND,
    RM_LU_UTILIZATION_STATUS_USED,

    //  Add new values here...

    RM_LU_UTILIZATION_STATUS_MAX

}   RM_LU_UTILIZATION_STATUS, *PRM_LU_UTILIZATION_STATUS;


//
//  Each Mirror instantiation has a set of user settable properties.  The Remote Mirror
//  Mirror Properties Mask structure is used so that one or more arbitrary properties can
//  be set in a single call.  For each bit in the bit-mask that is set, the corresponding
//  entry in the Mirror Properties structure will be updated.  This implies that two
//  structures need to be set in unison to update the properties of a mirror instantiation.
//
//  For explanation of the variables that get set by each of the bits defined, see the
//  RM_MIRROR_PROPERTIES structure defined below.
//
typedef struct _RM_MIRROR_PROPERTIES_MASK
{
    UCHAR                   Name : 1;

    UCHAR                   Description : 1;

    UCHAR                   MirrorState : 1;

    UCHAR                   WritePolicy : 1;

    UCHAR                   ActivationPolicy : 1;

    UCHAR                   Synch_Transition_Time : 1;

    UCHAR                   MinimumImagesRequired : 1;

    UCHAR                   SynchronizationPriority : 1;

    UCHAR                   ExtentSize : 1;

    UCHAR                   UsingWriteIntent : 1;

    UCHAR                   Domino : 1;

    UCHAR                   BackdoorRead : 1;

}   RM_MIRROR_PROPERTIES_MASK, *PRM_MIRROR_PROPERTIES_MASK;


//
//  The Remote Mirror Mirror Properties structure is used to hold the set of user
//  modifiable properties for a mirror instantiation.  When used in conjunction with the
//  RM_MIRROR_PROPERTIES_MASK structure, it allows the setting of one or more arbitrary
//  properties.
// 
//  IMPORTANT NOTE:  If you add any members to this structure, you need to update the change
//  map computation routine: adminCreateChangeMapsExtended.  This routine computes the differences
//  between two of these structures - in addition, it determines if a full "mirror" update
//  is needed in KDBM or just a smaller image info table update.
//
//  Name
//      The human choosable monicer for the mirror's ID (remember, strings are easier to
//      remember than gibberish).  The name is intended for display by an administrative
//      program and is treated in a complete opaque manner by Remote Mirrors.
//
//  Description
//      The choosable description of the mirror's name.  The description is intended for
//      display by an administrative program to provide additional clarification to the
//      name and is treated in a complete opaque manor by Remote Mirrors.
//
//  MirrorState
//     This property reflects the state of a mirror instantiation.  While it is a property
//     that can be set by the user (i.e. they can explicitly deactivate a mirror), it
//     is also just as likely that this property can be changed by the Remote Mirrors
//     software explicitly (i.e. we loose too many secondaries to continue operation so we
//     set the mirror to the attention state).
//
//  WritePolicy
//      This property controls the write policy of the mirror instantiation and can be
//      one of two values : RM_WRITE_POLICY_SYNCHRONOUS -or- RM_WRITE_POLICY_ASYNCHRONOUS.
//      For the initial version of Remote Mirrors, only the Synchronous write policy
//      will be honored.
//
//  ActivationPolicy
//      This property controls the behavior of the mirror instantiation during startup.
//      See the policy definitions above for more information.
//
//  Synch_Transition_Time
//      This property controls the time period in seconds from which a mirror will
//      transition to the synchronized state.  When the time between I/O exceeds
//      this value the mirror will change to from a consistent to synchronized.
//      A value of -1 indicates that this property will be ignored in
//      mirror operations.
//
//  MinimumImagesRequired
//      This property controls the minimum number of slave images that a mirror
//      instantiation must have functional in order to be active.  It is analogous
//      to the MaximumMissingImages property but from the opposite prospective.  A
//      value of 0xFFFF (-1) indicates that all images are required for the mirror
//      instantiation to remain active.  A value of zero would allow all slave images
//      to be off-line and the mirror master would still function.  This is the default
//      value.  A mirror instantiation with 2 slaves that has only a single slave
//      functioning will remain active if this value is set to a value of 0 or 1.
//      But if the value is set to -1 or 2, the mirror instantiation will transition
//      to the inactive state and the master will stop accepting I/O from the hosts.
//      This property must be set to a value that remains in harmony with the
//      MaximumMissingImages property (if you get the rational now, ask).
//
//  SynchronizationPriority
//      This property represents the synchronization priority for the mirror. When multiple
//      mirror instantiations must be synchronized, the synchronization priority is used to
//      schedule each mirror's synchronization to minimize the amount of mirror interconnect
//      bandwidth devoted to synchronization.
//
//  ExtentSize
//      This property controls the extent size for tracking the fracturing of a slave image.
//      The value of this property is expressed in terms of 512 byte disk blocks.  An LU
//      is logically divided into chunks of the expressed extent size.  During a slave image
//      fracture, each write into an 'extent' is flagged in a bit-mask representation of clean
//      and dirty extents.  When the slave image is synchronized, only extents that have their
//      bit turned on need to be forwarded to the slave.  An extent size of 1 would have a
//      bit for every single block on the disk.  This will provide the quickest synchronization
//      but at the expense of memory.  An extent size equal to the number of blocks
//      within an LU would cause the whole LU to be marked dirty on a single write anywhere
//      on the LU.  This will conserve the most memory but provide for the slowest
//      synchronization.  The setting of this property should be a balance of the two extremes.
//
//  Domino
//      This property controls whether or not I/O will be refused when a mirror enters
//      the attention state.
//
//  Reserved
//      A semi-arbitrary number of bytes to give us room to grow when we include asyncronous
//      mirrors.  Right now it looks like 16 bytes or less will be all that is necessary,
//      but we might as well be greedy and give us room to grow into since this structure
//      is stored on stable storage (PSM).  WriteBacklog would go in here...
//
#define RM_MIRROR_PROPERTIES_RESERVED       \
    ( 1024 - \
      sizeof( RM_MIRROR_NAME ) - \
      sizeof( RM_MIRROR_DESC ) - \
      sizeof( RM_MIRROR_STATE ) - \
      sizeof( RM_WRITE_POLICY ) - \
      sizeof( RM_ACTIVATION_POLICY ) - \
      sizeof( ULONG ) - \
      sizeof( USHORT ) - \
      sizeof( ULONG ) - \
      sizeof( ULONG ) - \
      sizeof( ULONG ) - \
      sizeof( ULONG ) - \
      sizeof( ULONG ) )

//
//  Since we depend on a certain alignment for the arithmetic above, make sure
//  that the members of the structure are on 1-byte boundaries.
//

//
// NOTE: The varialbles "Name" and "Description" in the following mirror properties structure
// may not contain a null terminated string (see dims 73386). To obtain a null terminated string,
// use the function GetMirrorNameString() (defined in ...RemoteMirrors\src\krnl\mirror\mirror.c)
// in the driver code and the functions GetMirrorName() and GetMirrorDescString() (defined
// in ...\RemoteMirrors\ext\mirror.c) in the mirror view debug dll code.
//

#pragma pack( push, RM_MIRROR_PROPERTIES, 1 )

typedef struct _RM_MIRROR_PROPERTIES
{
    RM_MIRROR_NAME          Name;

    RM_MIRROR_DESC          Description;

    RM_MIRROR_STATE         MirrorState;

    RM_WRITE_POLICY         WritePolicy;

    RM_ACTIVATION_POLICY    ActivationPolicy;

    ULONG                   Synch_Transition_Time;

    csx_s16_t                   MinimumImagesRequired;

    ULONG                   SynchronizationPriority;

    ULONG                   ExtentSize;         // Is this large enough?

    ULONG                   UsingWriteIntent;

    ULONG                   Domino;

    ULONG                   BackdoorRead;

    UCHAR                   Reserved[ RM_MIRROR_PROPERTIES_RESERVED ];

}   RM_MIRROR_PROPERTIES, *PRM_MIRROR_PROPERTIES;

#pragma pack( pop, RM_MIRROR_PROPERTIES )

#define RM_DEFAULT_USING_WRITE_INTENT          0    // means not using write intent log
#define RM_DEFAULT_DOMINO                      0    // means Domino is off
#define RM_DEFAULT_MIRROR_BACKDOOR_READ_ACCESS 0    // Backdoor read access is disabled by default

//
//  The Remote Mirror Mirror Properties structure is used to hold the set of Mirror
//  information for a mirror that will need to be utilized outside the RM device driver.
//
//  MirrorId
//      This is the ID for the mirror.
//
//  Name
//      The human choosable monicer for the mirror's ID (remember, strings are easier to
//      remember than gibberish).
//
//  Description
//      The choosable description of the mirror's name.
//
//  MirrorState
//     This property reflects the state of a mirror instantiation.
//
//  ImageRole
//      The role that the image plays within it's mirror instantiation.
//
typedef struct _RM_MIRROR_ENUM_INFO
{
    RM_MIRROR_ID            MirrorId;

    RM_MIRROR_NAME          Name;

    RM_MIRROR_DESC          Description;

    RM_MIRROR_STATE         MirrorState;

    RM_IMAGE_ROLE           ImageRole;

}   RM_MIRROR_ENUM_INFO, *PRM_MIRROR_ENUM_INFO;

//   Image Locality Enumeration
//      Enum of whether the image is local, remote, or invalid indicator
//
//      Local - image resides on this K10
//
//      Remote - image resides on the remote K10
//
//      Invalid - indicates no data stored or invalid data
//
typedef enum _RM_IMAGE_LOCALITY
{
    IMAGE_LOCALITY_INVALID   = -1,

    IMAGE_LOCALITY_LOCAL,

    IMAGE_LOCALITY_REMOTE

} RM_IMAGE_LOCALITY, *PRM_IMAGE_LOCALITY;


//
//  Each Mirror instantiation has a set of user settable properties.  The Remote Mirror
//  Mirror Properties Mask structure is used so that one or more arbitrary properties can
//  be set in a single call.  For each bit in the bit-mask that is set, the corresponding
//  entry in the Mirror Properties structure will be updated.  This implies that two
//  structures need to be set in unison to update the properties of a mirror instantiation.
//
//  For explanation of the variables that get set by each of the bits defined, see the
//  RM_MIRROR_PROPERTIES structure defined below.
//
typedef struct _RM_IMAGE_PROPERTIES_MASK
{
    UCHAR                   Role : 1;

    UCHAR                   State : 1;

    UCHAR                   RecoveryPolicy : 1;

    UCHAR                   ImageCondition : 1;

    UCHAR                   SyncDelay : 1;

    UCHAR                   PreferredSP : 1;

    UCHAR                   Locality : 1;

}   RM_IMAGE_PROPERTIES_MASK, *PRM_IMAGE_PROPERTIES_MASK;


//
//  The Remote Mirror Image Properties structure is used to hold the user configurable
//  properties for a single Mirror image.  These values are stored for both master images
//  and slave images, although their usage is !heavily! weighted toward slave images.
//
//  Role
//      This value specifies the role that the image plays within a mirror instantiation.
//      While we only forsee the roles 'master' and 'slave' for the immediate future,
//      other roles can be specified if the need arises.
//
//  State
//      The state of the image.  This is used, in conjunction with the Mirror cookie,
//      to track the state of an image in relation to the other images that are members
//      of the Mirror.
//
//  RecoveryPolicy
//      This value is used to determine how a slave image is recovered after a failure
//      of the slave and it is now reachable.
//
//  ImageCondition
//      Additional information on the state of the image in respect to the master
//      such as if admin fractured etc.
//
//  SyncRate
//      This value serves simply as an indicator for the relative speed of a
//      synchronization.  This value relates to two other values for specifying
//      the number of I/O's and delay for this particular rate.  These values are
//      stored in the global SyncSettings structure (GlobalMirrorData.SyncSettings).
//      For each rate (high, medium, low), there are definitions for the number of
//      I/O's, amount of delay, and cost.
//
//  PreferredSP
//      This value selects a storage processor that is the preferred recipient of
//      administration and data propogation messages.
//
//  SynchProgress
//      Indicator of the progress of the synchronization of a slave image, a
//      checkpoint parameter if you will.  This value is only meaningful during
//      a full resynchronization and will allow us to continue such a
//      synchronization to a slave from the last checkpoint in the event that
//      the master image cycles.  Note that we have to store the sync progress
//      for each image piece since the sync operations are performed in
//      parallel.
//

//
//  Since we depend on a certain alignment for on-disk structures, make sure
//  that the members of the structure are on 1-byte boundaries.
//

#pragma pack( push, RM_IMAGE_PROPERTIES, 1 )

typedef struct _RM_IMAGE_PROPERTIES
{
    RM_IMAGE_ROLE               Role;

    RM_IMAGE_STATE              State;

    RM_IMAGE_RECOVERY_POLICY    RecoveryPolicy;

    RM_CONFIG_IMAGE_CONDITION   ImageCondition;

    ULONG                       SyncRate;

    ULONG                       PreferredSP;

    EMCPAL_LARGE_INTEGER               SynchProgress[ RM_MAX_LUS_PER_ARRAY_SET ];

    RM_IMAGE_LOCALITY           Locality;

}   RM_IMAGE_PROPERTIES, *PRM_IMAGE_PROPERTIES;

#pragma pack( pop, RM_IMAGE_PROPERTIES )

#define RM_DEFAULT_IMAGE_TIMEOUT_VALUE                  5000
#define RM_DEFAULT_IMAGE_CONDITION                      RmConfigUnknownImageCondition
#define RM_DEFAULT_IMAGE_SYNC_RATE                      100
#define RM_DEFAULT_IMAGE_PREFERRED_SP                   0
#define RM_DEFAULT_IMAGE_LOCALITY                       IMAGE_LOCALITY_INVALID

// IOCTLValues
///////////////////////////////////////////////////////////////////////////////////////////////
//
//  The IOCTL values that the Remote Mirror driver will respond to.
//
///////////////////////////////////////////////////////////////////////////////////////////////

//
//  Define the device type value.  Note that values used by Microsoft
//  Corporation are in the range 0-32767, and 32768-65535 are reserved for use
//  by customers.
//
#define     FILE_DEVICE_RM             42000


//
//  Define IOCTL index values.  Note that function codes 0-2047 are reserved
//  for Microsoft Corporation, and 2048-4095 are reserved for customers.
//
#define     IOCTL_INDEX_RM_BASE        2400

//
//  The Remote Mirrors device driver IOCTLs and their helper macro.
//

#define     RM_BUILD_IOCTL( _x_ )      EMCPAL_IOCTL_CTL_CODE( (ULONG) FILE_DEVICE_RM,  \
                                                 (IOCTL_INDEX_RM_BASE + _x_),  \
                                                 EMCPAL_IOCTL_METHOD_BUFFERED,  \
                                                 EMCPAL_IOCTL_FILE_ANY_ACCESS )

#define     RM_BUILD_IOCTL_OUT_DIRECT( _x_ )      EMCPAL_IOCTL_CTL_CODE( (ULONG) FILE_DEVICE_RM,  \
                                                 (IOCTL_INDEX_RM_BASE + _x_),  \
                                                 EMCPAL_IOCTL_METHOD_OUT_DIRECT,  \
                                                 EMCPAL_IOCTL_FILE_ANY_ACCESS )

#define IOCTL_K10_BIND                          RM_BUILD_IOCTL( K10_BIND )
#define IOCTL_K10_UNBIND                        RM_BUILD_IOCTL( K10_UNBIND )
#define IOCTL_K10_ENUM_OBJECTS                  RM_BUILD_IOCTL( K10_ENUMERATE_OBJECTS )
#define IOCTL_K10_GET_GENERIC_PROPS             RM_BUILD_IOCTL( K10_GET_GENERIC_PROPS )
#define IOCTL_K10_PUT_GENERIC_PROPS             RM_BUILD_IOCTL( K10_PUT_GENERIC_PROPS )
#define IOCTL_K10_GET_SPECIFIC_PROPS            RM_BUILD_IOCTL( K10_GET_SPECIFIC_PROPS )
#define IOCTL_K10_PUT_SPECIFIC_PROPS            RM_BUILD_IOCTL( K10_PUT_SPECIFIC_PROPS )
#define IOCTL_K10_GET_COMPAT_MODE               RM_BUILD_IOCTL( K10_GET_COMPAT_MODE )
#define IOCTL_K10_PUT_COMPAT_MODE               RM_BUILD_IOCTL( K10_PUT_COMPAT_MODE )
#define IOCTL_RM_CREATE_MIRROR                  RM_BUILD_IOCTL( RM_CREATE_MIRROR )
#define IOCTL_RM_DESTROY_MIRROR                 RM_BUILD_IOCTL( RM_DESTROY_MIRROR )
#define IOCTL_RM_ENUM_MIRRORS                   RM_BUILD_IOCTL( RM_ENUM_MIRRORS )
#define IOCTL_RM_ADD_SLAVE                      RM_BUILD_IOCTL( RM_ADD_SLAVE )
#define IOCTL_RM_REMOVE_SLAVE                   RM_BUILD_IOCTL( RM_REMOVE_SLAVE )
#define IOCTL_RM_GET_MIRROR_PROPERTIES          RM_BUILD_IOCTL( RM_GET_MIRROR_PROPERTIES )
#define IOCTL_RM_SET_MIRROR_PROPERTIES          RM_BUILD_IOCTL( RM_SET_MIRROR_PROPERTIES )
#define IOCTL_RM_GET_IMAGE_PROPERTIES           RM_BUILD_IOCTL( RM_GET_IMAGE_PROPERTIES )
#define IOCTL_RM_SET_IMAGE_PROPERTIES           RM_BUILD_IOCTL( RM_SET_IMAGE_PROPERTIES )
#define IOCTL_RM_ENUM_UNUSED_OBJECTS            RM_BUILD_IOCTL( RM_ENUM_UNUSED_OBJECTS )
#define IOCTL_RM_PROMOTE_SLAVE                  RM_BUILD_IOCTL( RM_PROMOTE_SLAVE )
#define IOCTL_RM_FRACTURE_SLAVE                 RM_BUILD_IOCTL( RM_FRACTURE_SLAVE )
#define IOCTL_RM_SYNCHRONIZE_SLAVE              RM_BUILD_IOCTL( RM_SYNCHRONIZE_SLAVE )
#define IOCTL_RM_SET_DRIVER_PROPERTIES          RM_BUILD_IOCTL( RM_SET_DRIVER_PROPERTIES )
#define IOCTL_RM_GET_DRIVER_PROPERTIES          RM_BUILD_IOCTL( RM_GET_DRIVER_PROPERTIES )
#define IOCTL_RM_GET_MIRROR_TRESPASS_LICENSE    RM_BUILD_IOCTL( RM_GET_MIRROR_TRESPASS_LICENSE )
#define IOCTL_K10_QUIESCE                       RM_BUILD_IOCTL( RM_K10_QUIESCE )
#define IOCTL_K10_SHUTDOWN                      RM_BUILD_IOCTL( RM_K10_SHUTDOWN )
#define IOCTL_K10_REASSIGN                      RM_BUILD_IOCTL( RM_K10_REASSIGN )
#define IOCTL_K10_DISK_ADMIN_OPC_STATS          RM_BUILD_IOCTL( K10_DISK_ADMIN_OPC_STATS )
#define IOCTL_RM_RESET_IOSTATS                  RM_BUILD_IOCTL( RM_RESET_IOSTATS )

//
// Add test IOCTLs inside the conditional compilation.  Only use the test
// IOCTLs in the checked (debug) build.
//

#if DBG // Checked (debug) build.

    #define IOCTL_RM_TEST_FAKE_LOST_CONTACT         RM_BUILD_IOCTL( RM_TEST_FAKE_LOST_CONTACT )
    #define IOCTL_RM_TEST_MODIFY_DEBUG_PARAMETERS   RM_BUILD_IOCTL( RM_TEST_MODIFY_DEBUG_PARAMETERS )

#endif  // DBG

#define IOCTL_RM_GET_RAW_PERFORMANCE_DATA       RM_BUILD_IOCTL( RM_GET_RAW_PERFORMANCE_DATA )

//
// If allowed, define the secondary read IOCTL.  This allows read operations
// on the secondary.  It is turned off by default and must be turned
// on after boot.
//

#if RM_SECONDARY_READS_ALLOWED

    #define IOCTL_RM_MODIFY_SECONDARY_READ_STATUS   RM_BUILD_IOCTL( RM_MODIFY_SECONDARY_READ_STATUS )

#endif  // RM_SECONDARY_READS_ALLOWED


#define IOCTL_RM_GET_IMAGE_LU_PROPERTIES			RM_BUILD_IOCTL( RM_GET_IMAGE_LU_PROPERTIES )

//
// New IOCTLs for Delta Polling
// 
#define IOCTL_RM_POLL_GET_CONTAINER_SIZE            RM_BUILD_IOCTL( RM_POLL_GET_CONTAINER_SIZE )
#define IOCTL_RM_POLL_RETRIEVE_DATA                 RM_BUILD_IOCTL_OUT_DIRECT( RM_POLL_RETRIEVE_DATA )

// IOCTLBuffers
///////////////////////////////////////////////////////////////////////////////////////////////
//
//  The IOCTL input and output buffers that are used to interface with Remote
//  Mirrors control device.
//
///////////////////////////////////////////////////////////////////////////////////////////////

//
//  The following defines the current version of the RM IOCTL interface structures.
//  It could have gone above with the rest of the defines, but makes better sense here.
//
#define     RM_CURRENT_API_VERSION      0x0002


//
//  The Remote Mirrors Bind Input Buffer is used in conjunction with IOCTL_K10_BIND
//  to add the Remote Mirrors driver into a device stack.
//
//  RevisionId
//      Revision ID of the API. Users should always set this to RM_CURRENT_API_VERSION.
//
//  IsRebind
//      Flag indicating whether this is a 'rebind' of a previouly 'unbound' device
//      object that occured with its 'WillRebind' flag set.
//
//  ConsumedDevice
//      The full object name for the device that is 'consumed' by the bind operation.
//
//  LuId
//      The World Wide ID for the actual Flare LU at the bottom of the device stack.
//
typedef struct _RM_BIND_IN_BUFFER
{
    ULONG                       RevisionId;

    BOOLEAN                     IsRebind;

    K10_DVR_OBJECT_NAME         ConsumedDevice;

    K10_LU_ID                   LuId;

}   RM_BIND_IN_BUFFER, *PRM_BIND_IN_BUFFER;


//
//  The Remote Mirrors Bind Output Buffer is used return the name of the new device object
//  that is exported in response to a bind request.
//
//  ExportedDevice
//      The full object name for the device that is 'exported' due to the bind operation.
//
typedef struct _RM_BIND_OUT_BUFFER
{
    K10_DVR_OBJECT_NAME         ExportedDevice;

}   RM_BIND_OUT_BUFFER, *PRM_BIND_OUT_BUFFER;


//
//  The Remote Mirrors Unbind Input Buffer is used in conjunction with IOCTL_K10_UNBIND
//  to remove the Remote Mirrors driver from a device stack.
//
//  RevisionId
//      Revision ID of the API. Users should always set this to RM_CURRENT_API_VERSION.
//
//  WillRebind
//      Flag indicating whether or not the device will be 'rebound' with a subsequent
//      'bind' operation.  This is used to allow a restack of the layered drivers on
//      a stack by stack basis.
//
//  ExportedDevice
//      The full exported object name for the device that is to be removed from the stack.
//
typedef struct _RM_UNBIND_IN_BUFFER
{
    ULONG                       RevisionId;

    BOOLEAN                     WillRebind;

    K10_DVR_OBJECT_NAME         ExportedDevice;

}   RM_UNBIND_IN_BUFFER, *PRM_UNBIND_IN_BUFFER;


//
//  The Remote Mirrors Enumerate Objects Input Buffer is used in conjunction with
//  IOCTL_K10_ENUM_OBJECTS to request the listing of device objects that the driver
//  consumes and exports.
//
//  RevisionId
//      Revision ID of the API. Users should always set this to RM_CURRENT_API_VERSION.
//
typedef struct _RM_ENUM_OBJECTS_IN_BUFFER
{
    ULONG                       RevisionId;

}   RM_ENUM_OBJECTS_IN_BUFFER, *PRM_ENUM_OBJECTS_IN_BUFFER;


//
//  The Remote Mirrors Enumerate Objects Output Buffer is used to return the listing
//  of the device objects that the driver consumes and exports.
//
//  NodesRequired
//      The number of entries required to retrieve the complete enumeration of all the
//      device objects
//
//  NodesReturned
//      The actual number of entries returned from the enumeration request.
//
//  Contents
//      A place holder to access the returned device object names.
//
typedef struct _RM_ENUM_OBJECTS_OUT_BUFFER
{
    ULONG                       Status;

    ULONG                       NodesRequired;

    ULONG                       NodesReturned;

    K10_DVR_OBJECT_NAME         Contents[ 1 ];

}   RM_ENUM_OBJECTS_OUT_BUFFER, *PRM_ENUM_OBJECTS_OUT_BUFFER;


//
//  The Remote Mirrors Get Generic Properties Input Buffer is used in conjunction with
//  IOCTL_K10_GET_GENERIC_PROPS to retrieve the generic properties of a device object.
//
//  RevisionId
//      Revision ID of the API. Users should always set this to RM_CURRENT_API_VERSION.
//
//  DeviceName
//      Device name of the object to retrieve information about.  The device name should
//      be a value as returned by using IOCTL_K10_ENUM_OBJECTS.
//
typedef struct _RM_GET_GENERIC_PROPERTIES_IN_BUFFER
{
    ULONG                       RevisionId;

    K10_DVR_OBJECT_NAME         DeviceName;

}   RM_GET_GENERIC_PROPERTIES_IN_BUFFER, *PRM_GET_GENERIC_PROPERTIES_IN_BUFFER;


//
//  The Remote Mirrors Get Generic Properties Output Buffer is used to return the generic
//  properties for the requested device object.
//
//  LuId
//      The world wide ID associated with the 'device stack' for the supplied device name.
//
//  ConsumedDeviceCount
//      Count of the devices consumed by the suppled device name.  The value will always
//      be zero if the supplied name is a consumed device.  The value will always be
//      one if the supplied name is an exported device.
//
//  ConsumedDeviceName
//      Device name of the object (if any) that the supplied device name consumes.
//
//  NoExtent
//      Indicates don't want anyone else to layer over this LU. In RM only applies to the
//      the write intent logs.
//
//  Consumed
//      Indicates the LU is consumed and no one else should have access. In RM only applies
//      to the the write intent logs.
//
typedef struct _RM_GET_GENERIC_PROPERTIES_OUT_BUFFER
{
    ULONG                       Status;

    K10_LU_ID                   LuId;

    ULONG                       ConsumedDeviceCount;

    K10_DVR_OBJECT_NAME         ConsumedDeviceName;

    UCHAR                       NoExtent;

    UCHAR                       Consumed;

}   RM_GET_GENERIC_PROPERTIES_OUT_BUFFER, *PRM_GET_GENERIC_PROPERTIES_OUT_BUFFER;


//
//  The Remote Mirrors Create Mirror Input Buffer is used in conjunction with
//  IOCTL_RM_CREATE_MIRROR to create a new mirror instantiation.
//
//  RevisionId
//      Revision ID of the API. Users should always set this to RM_CURRENT_API_VERSION.
//
//  PropertiesMask
//      Mask that specifies the 'Properties' that need to be set to values during the
//      mirror creation process.
//
//  Properties
//      Property values that are used to initialize the new mirror.
//
//  ArraySetCount
//      The number of logical units that comprise the array set for the master image.
//
//  ArraySet
//      The IDs for the actual logical units that comprise the array set for the master image.
//
//  CreateMirrorNumberOverrideFlag
//      If RM driver has to limit number mirrors created on system then also
//      will need a way to disable the limit for testing purposes.  This flag
//      allows to exceed number of mirrors on system.
//      1 ignores number of mirrors / 0 runs normal safety checks.
//
typedef struct _RM_CREATE_MIRROR_IN_BUFFER
{
    ULONG                       RevisionId;

    RM_MIRROR_PROPERTIES_MASK   PropertiesMask;

    RM_MIRROR_PROPERTIES        Properties;

    ULONG                       ArraySetCount;

    K10_LU_ID                   ArraySet[ RM_MAX_LUS_PER_ARRAY_SET ];

    ULONG                       CreateMirrorNumberOverrideFlag;

}   RM_CREATE_MIRROR_IN_BUFFER, *PRM_CREATE_MIRROR_IN_BUFFER;


//
//  The Remote Mirrors Create Mirror Output Buffer is used to return the ID for the new
//  mirror instantiation.
//
//  Status
//      The actual completion status of the requested operation.
//
//  MirrorId
//      The ID for the newly created mirror instantiation if it was successfully created.
//
//  LuStatus
//      The utilization satus of each LU that was specified in the input array set,
//      in the same order as the input set.  If the operation was successful, each
//      status will also indicate SUCCESS; otherwise, the LuStatus field will have
//      status codes to reflect whether the LU could have been utilized or an error
//      code indicating the reason for the LU not being useable.
//
typedef struct _RM_CREATE_MIRROR_OUT_BUFFER
{
    ULONG                       Status;

    RM_MIRROR_ID                MirrorId;

    RM_LU_UTILIZATION_STATUS    LuStatus[ RM_MAX_LUS_PER_ARRAY_SET ];

}   RM_CREATE_MIRROR_OUT_BUFFER, *PRM_CREATE_MIRROR_OUT_BUFFER;


//
//  The Remote Mirrors Destroy Mirror Input Buffer is used in conjunction with
//  IOCTL_RM_DESTROY_MIRROR to destroy a mirror instantiation.
//
//  RevisionId
//      Revision ID of the API. Users should always set this to RM_CURRENT_API_VERSION.
//
//  MirrorId
//      The ID for the mirror that should be destroyed.
//
typedef struct _RM_DESTROY_MIRROR_IN_BUFFER
{
    ULONG                       RevisionId;

    RM_MIRROR_ID                MirrorId;

    ULONG                       DestroyOrphanImageFlag;

}   RM_DESTROY_MIRROR_IN_BUFFER, *PRM_DESTROY_MIRROR_IN_BUFFER;


//
//  The Remote Mirrors Enumerate Mirrors Input Buffer is used in conjunction with
//  IOCTL_RM_ENUM_MIRRORS to request the listing of mirror instantiations that the
//  driver manages (master -or- slave).
//
//  RevisionId
//      Revision ID of the API. Users should always set this to RM_CURRENT_API_VERSION.
//
typedef struct _RM_ENUM_MIRRORS_IN_BUFFER
{
    ULONG                       RevisionId;

}   RM_ENUM_MIRRORS_IN_BUFFER, *PRM_ENUM_MIRRORS_IN_BUFFER;


//
//  The Remote Mirrors Enumerate Mirrors Output Buffer to return the listing of the
//  mirror instantiations that the driver manages (master -or- slave).
//
//  Status
//      Use to return custom status codes out of NT internals.
//
//  NodesRequired
//      The number of entries required to retrieve the complete enumeration of all the
//      mirror instantiations.
//
//  NodesReturned
//      The actual number of entries returned from the enumeration request.
//
//  Contents
//      A place holder to access the returned mirror enumeration structures.
//
typedef struct _RM_ENUM_MIRRORS_OUT_BUFFER
{
    ULONG                       Status;

    ULONG                       NodesRequired;

    ULONG                       NodesReturned;

    RM_MIRROR_ENUM_INFO         Contents[ 1 ];

}   RM_ENUM_MIRRORS_OUT_BUFFER, *PRM_ENUM_MIRRORS_OUT_BUFFER;


//
//  The Remote Mirrors Add Slave Input Buffer is used in conjunction with
//  IOCTL_RM_ADD_SLAVE to add a slave image to the given mirror instantiation.
//
//  RevisionId
//      Revision ID of the API. Users should always set this to RM_CURRENT_API_VERSION.
//
//  MirrorId
//      This is the ID for the mirror to be updated.
//
//  SlaveK10Id
//      Identifier for the K10 that will host the new slave image for the mirror
//      instantiation represented by the supplied MirrorId.
//
//  PropertiesMask
//      Mask for any image properties that are to be set.
//
//  Properties
//      Container for any image properties that are to be set.
//
//  ArraySetCount
//      The number of logical units that comprise the array set for the new slave image.
//
//  ArraySet
//      The IDs for the actual logical units that comprise the array set for the slave image.
//
//  CreateSecondaryNumberOverrideFlag
//          A flag to indicate whether to test if we exceed the max number of
//          mirrors on the system or not.
//          If flag == 1 then override and can create more mirrors on system
//          if flag == 0 then run normal safety checks and won't exceed
//          max mirrors.
//
typedef struct _RM_ADD_SLAVE_IN_BUFFER
{
    ULONG                       RevisionId;

    RM_MIRROR_ID                MirrorId;

    K10_ARRAY_ID                SlaveK10Id;

    RM_IMAGE_PROPERTIES_MASK    PropertiesMask;

    RM_IMAGE_PROPERTIES         Properties;

    ULONG                       ArraySetCount;

    K10_LU_ID                   ArraySet[ RM_MAX_LUS_PER_ARRAY_SET ];

    ULONG                       CreateSecondaryNumberOverrideFlag;

}   RM_ADD_SLAVE_IN_BUFFER, *PRM_ADD_SLAVE_IN_BUFFER;


//
//  The Remote Mirrors Add Slave Output Buffer is used to return the completion status
//  of the operation and the utilization status of each LU of the slave image.
//
//  Status
//      The actual completion status of the requested operation.
//
//  LuStatus
//      The utilization satus of each LU that was specified in the input array set,
//      in the same order as the input set.  If the operation was successful, each
//      status will also indicate SUCCESS; otherwise, the LuStatus field will have
//      status codes to reflect whether the LU could have been utilized or an error
//      code indicating the reason for the LU not being useable.
//
typedef struct _RM_ADD_SLAVE_OUT_BUFFER
{
    ULONG                       Status;

    RM_LU_UTILIZATION_STATUS    LuStatus[ RM_MAX_LUS_PER_ARRAY_SET ];

}   RM_ADD_SLAVE_OUT_BUFFER, *PRM_ADD_SLAVE_OUT_BUFFER;


//
//  The Remote Mirrors Remove Slave Input Buffer is used in conjunction with
//  IOCTL_RM_REMVE_SLAVE to remove a slave image from the given mirror instantiation.
//
//  RevisionId
//      Revision ID of the API. Users should always set this to RM_CURRENT_API_VERSION.
//
//  MirrorId
//      This is the ID for the mirror to be updated.
//
//  SlaveK10Id
//      This is the World Wide Id for the K10 that will no longer host a slave image.
//
//  OverrideFlag
//      Used to force removal of a slave image from the mirrors stored data when the slave
//      image can't be reached and told to remove from the remote K10.  Any positive value
//      will cause the slave image to be removed from the mirror.  0 should be used for normal
//      operation.
//
//
typedef struct _RM_REMOVE_SLAVE_IN_BUFFER
{
    ULONG                       RevisionId;

    RM_MIRROR_ID                MirrorId;

    K10_ARRAY_ID                SlaveK10Id;

    ULONG                       OverrideFlag;

}   RM_REMOVE_SLAVE_IN_BUFFER, *PRM_REMOVE_SLAVE_IN_BUFFER;

//
//  The Remote Mirrors Fracture Slave Input Buffer is used in conjunction with
//  IOCTL_RM_FRACTURE_SLAVE to fracture a slave image from the given mirror instantiation.
//
//  RevisionId
//      Revision ID of the API. Users should always set this to RM_CURRENT_API_VERSION.
//
//  MirrorId
//      This is the ID for the mirror to be updated.
//
//  SlaveK10Id
//      This is the World Wide Id for the K10 that is hosting the image to be
//      fractured
//
typedef struct _RM_FRACTURE_SLAVE_IN_BUFFER
{
    ULONG                       RevisionId;

    RM_MIRROR_ID                MirrorId;

    K10_ARRAY_ID                SlaveK10Id;

}   RM_FRACTURE_SLAVE_IN_BUFFER, *PRM_FRACTURE_SLAVE_IN_BUFFER;

//
//  The Remote Mirrors Synchronize Slave Input Buffer is used in conjunction with
//  IOCTL_RM_SYNCHRONIZE_SLAVE to synchronize a slave image that had previously
//  been fractured.
//
//  RevisionId
//      Revision ID of the API. Users should always set this to RM_CURRENT_API_VERSION.
//
//  MirrorId
//      This is the ID for the mirror to be synchronized.
//
//  SlaveK10Id
//      This is the World Wide Id for the K10 that is hosting the image to be
//      synchronized
//
typedef struct _RM_SYNCHRONIZE_SLAVE_IN_BUFFER
{
    ULONG                       RevisionId;

    RM_MIRROR_ID                MirrorId;

    K10_ARRAY_ID                SlaveK10Id;

}   RM_SYNCHRONIZE_SLAVE_IN_BUFFER, *PRM_SYNCHRONIZE_SLAVE_IN_BUFFER;

//
//  The Remote Mirrors Get Mirror Properties Input Buffer is used in conjunction with
//  IOCTL_RM_GET_MIRROR_PROPERTIES to request the properties of a specific mirror.
//
//  RevisionId
//      Revision ID of the API. Users should always set this to RM_CURRENT_API_VERSION.
//
//  MirrorId
//      The ID for the mirror instantiation to retrive information about.
//
typedef struct _RM_GET_MIRROR_PROPERTIES_IN_BUFFER
{
    ULONG                       RevisionId;

    RM_MIRROR_ID                MirrorId;

}   RM_GET_MIRROR_PROPERTIES_IN_BUFFER, *PRM_GET_MIRROR_PROPERTIES_IN_BUFFER;



/*
 * NAVI wants a CGId in Get Mirror Property, however, The clutch_ID is defined in
 * clutch_common_if.h, to make it easier, we define a same ID here to use
 */

/*
 * RM_CGID defines the type of CGID , it is the same as CLUTCH_ID .
 */

typedef struct RM_CGID
{
    K10_ARRAY_ID    arrayId;
    ULONG           clutchNumber;
} RM_CGID, *PRM_CGID;

//
//  The Remote Mirrors Get Mirror Properties Output Buffer returns the requested properties
//  of the specified mirror instantiation.
//
//  Status
//      Return NT status codes.
//
//  MirrorId
//      This is the ID for the mirror.  The Mirror ID must be unique accross the universe
//      of the ID space.  It is used as the primary identifier of a mirror instantiation.
//
//  Cookie
//      This is the current 'cookie' for the mirror instantiation.  It is used to track
//      mirror membership changes to ensure the integrity of the mirror.
//
//  Size
//      This is the total size of the mirror instantiation in 512 byte disk blocks.  It is
//      computed from the size of each of the LU array set members and is stored here as a
//      way to double check that all of the sizes have remained the same.
//
//  Properties
//      Container for the mirror properties.  These are the properties of a Mirror that
//      are (may) be exposed to an administrator for tweaking.  If it is not in this
//      structure, odds are it is not configurable.
//
//  ImageCount
//      This is the number of configured images in use by the Mirror.
//
//  Images
//      Array of containers for the K10 WWNs for each K10 hosted image, master and slaves.
//      These values can be used in conjunction with the Mirror ID to retrieve per image
//      property information.
//
//  ImageRoles
//      Array of containers for the roles that each image plays.
//
//  UsableLUType
//      This field will be populated with the possible secondary images that can be added
//      to this mirror. This field will be populated only when Navi sends the 
//      RM_GET_MIRROR_PROPERTIES query.

//
//  UNUSED
//      Name tells everything
//
typedef struct _RM_GET_MIRROR_PROPERTIES_OUT_BUFFER
{
    ULONG                       Status;

    RM_MIRROR_ID                MirrorId;

    RM_COOKIE                   Cookie;

    EMCPAL_LARGE_INTEGER               Size;

    RM_MIRROR_PROPERTIES        Properties;

    ULONG                       ImageCount;

    K10_ARRAY_ID                Images[ RM_MAX_SLAVES + 1];

    RM_IMAGE_ROLE               ImageRoles[ RM_MAX_SLAVES + 1];

    RM_CGID                     CGId;

    ULONG                       UsableLUType;

    ULONG                       UNUSED;

}   RM_GET_MIRROR_PROPERTIES_OUT_BUFFER, *PRM_GET_MIRROR_PROPERTIES_OUT_BUFFER;


//
//  The Remote Mirrors Set Mirror Properties Input Buffer is used in conjunction with
//  IOCTL_RM_SET_MIRROR_PROPERTIES to modify some of the specified mirrors properties.
//
//  RevisionId
//      Revision ID of the API. Users should always set this to RM_CURRENT_API_VERSION.
//
//  MirrorId
//      This is the ID for the mirror to be updated.
//
//  PropertiesMask
//      Mask for the mirror properties that are to be set.
//
//  NewProperties
//      Container for the mirror properties that are to be set.
//
typedef struct _RM_SET_MIRROR_PROPERTIES_IN_BUFFER
{
    ULONG                       RevisionId;

    RM_MIRROR_ID                MirrorId;

    RM_MIRROR_PROPERTIES_MASK   PropertiesMask;

    RM_MIRROR_PROPERTIES        NewProperties;

}   RM_SET_MIRROR_PROPERTIES_IN_BUFFER, *PRM_SET_MIRROR_PROPERTIES_IN_BUFFER;


//
//  The Remote Mirrors Get Image Properties Input Buffer is used in conjunction with
//  IOCTL_RM_GET_IMAGE_PROPERTIES to request the properties for a specific mirror image.
//
//  RevisionId
//      Revision ID of the API. Users should always set this to RM_CURRENT_API_VERSION.
//
//  MirrorId
//      The ID for the mirror to be queried.
//
//  ImageK10Id
//      The World Wide Id for the K10 that hosts the image in question.
//
typedef struct _RM_GET_IMAGE_PROPERTIES_IN_BUFFER
{
    ULONG                       RevisionId;

    RM_MIRROR_ID                MirrorId;

    K10_ARRAY_ID                ImageK10Id;

}   RM_GET_IMAGE_PROPERTIES_IN_BUFFER, *PRM_GET_IMAGE_PROPERTIES_IN_BUFFER;

//
//  The Remote Mirrors Get Image Properties Output Buffer returns the image properites
//  for the requested mirror image.
//
//  Status
//      Return operation status codes
//
//  ImageK10Id
//      The World Wide Id for the K10 that hosts the image in question.  Darn well better
//      be the same as the one passed in.  Returned just in case someone up above wants to
//      do some sort of sanity check on the response.
//
//  Properties
//      Container for the image properties.  These are the properties of an image that
//      are (may) be exposed to an administrator for tweaking.
//
//  Cookie
//      A copy of the last Mirror Cookie that this image received and updated.
//
//  State
//      The state of the image.  This is used, in conjunction with the Mirror cookie,
//      to track the state of an image in relation to the other images that are members
//      of the Mirror.
//
//  SyncProgress
//      Indicator of the progress of the synchronization of a slave image.
//      This is a percentage of complete roughly and is not the same value that is stored
//      in the image properties.  This is calculated from the image properties value.
//
//  ArraySetCount
//      The number of LUs in the array set that comprise this image.
//
//  ArraySet
//      The World Wide IDs for the LUs in the array set that comprise this image.  This
//      allows us to use the information in the consumables database to map between WWIDs
//      and actual NT device object names.
//
//	ExportedLuSizeInBlocks
//		The image size in 512-byte blocks.
//
//	ConsumedLuSizeInBlocks
//		The image size in 512-byte blocks.
//
//	ExpansionCompletionPercentage
//		The completion percentage of an expansion.
//
//	IsExpansionBlocked
//		TRUE if an expansion is currently blocked
//		FALSE otherwise
//
//	ExpansionBlockageReason
//		status explaining the expansion blockage
//
typedef struct _RM_GET_IMAGE_PROPERTIES_OUT_BUFFER
{
    ULONG                       Status;

    K10_ARRAY_ID                ImageK10Id;

    RM_IMAGE_PROPERTIES         Properties;

    RM_COOKIE                   Cookie;

    ULONG                       SyncProgress;

    ULONG                       ArraySetCount;

    K10_LU_ID                   ArraySet[ RM_MAX_LUS_PER_ARRAY_SET ];

    ULONGLONG					ExportedLuSizeInBlocks;

    ULONGLONG					ConsumedLuSizeInBlocks;

    ULONG						ExpansionCompletionPercentage;

    BOOLEAN						IsExpansionBlocked;

    ULONG				  		ExpansionBlockageReason;

	ULONG						ExpansionState;
	
}   RM_GET_IMAGE_PROPERTIES_OUT_BUFFER, *PRM_GET_IMAGE_PROPERTIES_OUT_BUFFER;


//
//  The Remote Mirrors Set Image Properties Input Buffer is used in conjunction with
//  IOCTL_RM_SET_IMAGE_PROPERTIES to modify some of the specified image properties.
//
//  RevisionId
//      Revision ID of the API. Users should always set this to RM_CURRENT_API_VERSION.
//
//  MirrorId
//      This is the ID for the mirror to be updated.
//
//  ImageK10Id
//      The World Wide Id for the K10 that hosts the image to be updated.
//
//  PropertiesMask
//      Mask for the image properties that are to be set.
//
//  NewProperties
//      Container for the image properties that are to be set.
//
typedef struct _RM_SET_IMAGE_PROPERTIES_IN_BUFFER
{
    ULONG                       RevisionId;

    RM_MIRROR_ID                MirrorId;

    K10_ARRAY_ID                ImageK10Id;

    RM_IMAGE_PROPERTIES_MASK    PropertiesMask;

    RM_IMAGE_PROPERTIES         NewProperties;

}   RM_SET_IMAGE_PROPERTIES_IN_BUFFER, *PRM_SET_IMAGE_PROPERTIES_IN_BUFFER;


//
//  The Remote Mirrors Get Unused Objects Input Buffer is used in conjunction with
//  IOCTL_RM_ENUM_UNUSED_OBJECTS to retrieve the list of device objects that are
//  Remote Mirrorable (i.e. bound in the stack) but are not part of a mirror
//  instantiation.
//
//  RevisionId
//      Revision ID of the API. Users should always set this to RM_CURRENT_API_VERSION.
//
typedef struct _RM_ENUM_UNUSED_OBJECTS_IN_BUFFER
{
    ULONG                       RevisionId;

}   RM_ENUM_UNUSED_OBJECTS_IN_BUFFER, *PRM_ENUM_UNUSED_OBJECTS_IN_BUFFER;


//
//  The Remote Mirrors Get Unused Objects Output Buffer is used to return the list
//  of available device objects for Remote Mirrors.
//
//  NodesRequired
//      The number of entries required to retrieve the complete enumeration of all the
//      device objects.
//
//  NodesReturned
//      The actual number of entries returned from the enumeration request.
//
//  Contents
//      A place holder to access the returned device object names.
//
typedef struct _RM_ENUM_UNUSED_OBJECTS_OUT_BUFFER
{

    ULONG                       Status;

    ULONG                       NodesRequired;

    ULONG                       NodesReturned;

    K10_DVR_OBJECT_NAME         Contents[ 1 ];

}   RM_ENUM_UNUSED_OBJECTS_OUT_BUFFER, *PRM_ENUM_UNUSED_OBJECTS_OUT_BUFFER;

//
//  The Remote Mirrors Promote Slave Input Buffer is used in conjunction with
//  IOCTL_RM_PROMOTE_SLAVE to promote a slave image into the role of the mirror
//  master.  It must be sent to a slave image requesting that it
//  take over as master image.
//
//  RevisionId
//      Revision ID of the API.  Users should always set this to RM_CURRENT_API_VERSION.
//
//  MirrorId
//      This is the ID for the mirror to be updated.
//
//  NewMasterK10Id
//      The World Wide Name for the K10 that is to be the new master.
//
//
typedef struct _RM_PROMOTE_SLAVE_IN_BUFFER
{
    ULONG                       RevisionId;

    RM_MIRROR_ID                MirrorId;

    K10_ARRAY_ID                NewMasterK10Id;

    ULONG                       UseWriteIntent;

}   RM_PROMOTE_SLAVE_IN_BUFFER, *PRM_PROMOTE_SLAVE_IN_BUFFER;

//
//  The Remote Mirrors Promote Slave Output Buffer is used to return the success of the
//  promotion process and the MirrorId of the new mirror master.
//
//  RevisionId
//      Revision ID of the API.  Users should always set this to RM_CURRENT_API_VERSION.
//
//  MirrorId
//      This is the ID for the mirror to be updated.
//
//  NewMasterK10Id
//      The World Wide Name for the K10 that is to be the new master.
//
//
typedef struct _RM_PROMOTE_SLAVE_OUT_BUFFER
{
    ULONG                       Status;

    RM_MIRROR_ID                NewMirrorId;

    K10_ARRAY_ID                NewMasterK10Id;

}   RM_PROMOTE_SLAVE_OUT_BUFFER, *PRM_PROMOTE_SLAVE_OUT_BUFFER;

//
//  The Remote Mirrors Set Driver Properties Input Buffer is used in conjunction with
//  IOCTL_RM_SET_DRIVER_PROPERTIES to modify some of the specified mirroring properties.
//
//  RevisionId
//      Revision ID of the API. Users should always set this to RM_CURRENT_API_VERSION.
//
//  SPAMapPartition
//      This is a STRING of the SPAMapPartition location to be used in RM
//
//  SPALUID
//      This is the LUID of the map partition
//
//  SPBMapPartition
//      This is a STRING of the SPBMapPartition location to be used in RM
//
//  SPBLUID
//      This is the LUID of the map partition
//
//  AllocateWriteIntent
//      This flag is used to specify if the map partition should be allocated or deallocated
//      If this value is set to 0 the write intent logs will be deallocated if allocated.
//      If the write intent logs are to be set then this value must be set to 1.
//
typedef struct _RM_SET_DRIVER_PROPERTIES_IN_BUFFER
{
    ULONG                       RevisionId;

    CHAR                        SPAMapPartition[ K10_DEVICE_ID_LEN ];

    K10_LU_ID                   SPALuId;

    CHAR                        SPBMapPartition[ K10_DEVICE_ID_LEN ];

    K10_LU_ID                   SPBLuId;

    ULONG                       AllocateWriteIntent;

}   RM_SET_DRIVER_PROPERTIES_IN_BUFFER, *PRM_SET_DRIVER_PROPERTIES_IN_BUFFER;


//
//  The Remote Mirrors Set Driver Properties Output Buffer is used in conjunction with
//  IOCTL_RM_SET_DRIVER_PROPERTIES to return a status.
//
//  RevisionId
//      Revision ID of the API. Users should always set this to RM_CURRENT_API_VERSION.
//
//  Status
//      This is a NTSTATUS of the operation
//
typedef struct _RM_SET_DRIVER_PROPERTIES_OUT_BUFFER
{
    ULONG                       RevisionId;

    ULONG                       Status;

}   RM_SET_DRIVER_PROPERTIES_OUT_BUFFER, *PRM_SET_DRIVER_PROPERTIES_OUT_BUFFER;



//
//  The Remote Mirrors Get Driver Properties Input Buffer is used in conjunction with
//  IOCTL_RM_GET_DRIVER_PROPERTIES to return the map partition information.
//  May be modified to return other data in the future
//
//  RevisionId
//      Revision ID of the API. Users should always set this to RM_CURRENT_API_VERSION.
//
//
typedef struct _RM_GET_DRIVER_PROPERTIES_IN_BUFFER
{
    ULONG                       RevisionId;

}   RM_GET_DRIVER_PROPERTIES_IN_BUFFER, *PRM_GET_DRIVER_PROPERTIES_IN_BUFFER;

//
//  The Remote Mirrors Get Driver Properties Output Buffer is used in conjunction with
//  IOCTL_RM_GET_DRIVER_PROPERTIES to return some of the specified mirroring properties.
//      This structure may be modified in the future to return additional information.
//
//  RevisionId
//      Revision ID of the API. Should always be set to RM_CURRENT_API_VERSION.
//
//  SPAMapPartition
//      This is a STRING of the SPAMapPartition location to be used in RM
//
//  SPALuId
//      LuId of the Map Partition drive
//
//  SPBMapPartition
//      This is a STRING of the SPBMapPartition location to be used in RM
//
//  SPBLuId
//      LuId of the Map Partition drive
//
//  MinimumWriteIntentSize
//      Use to return the defined value to NAVI for future use
//
//  MaxMemoryAllowed
//      The following defines the maximum value for the amount of memory the driver
//      can allocate out of NonPagedPool.  This value is currently defined at 10MB.
//      Here for future situations when needs to be changed.
//
//  MaxMirrors
//      The following defines the maximum number of mirrors the driver
//      will allow on the system.
//
//  MaxMirrorsWithWriteIntent
//      The following defines the maximum number of mirrors that
//      may use the write intent log at any one time.  This is dependent
//      upon the MaxMemory Allowed to the RM driver.
//
//  MaxLUsInMirror
//      The following returns the maximum number of LUs that may make up
//      a single mirror.
//
//  MaxNumberSecondaryImages
//      The following defines the maximum number of secondary images the
//      Mirroring driver supports.
//
//  IsThinCapable
//      This flag indicates whether primary/secondary images can be created
//      on Thin LUs or not. This is set based on the current compat level of
//      MV/S driver.
//
typedef struct _RM_GET_DRIVER_PROPERTIES_OUT_BUFFER
{
    ULONG                       Status;

    CHAR                        SPAMapPartition[ K10_DEVICE_ID_LEN ];

    K10_LU_ID                   SPALuId;

    CHAR                        SPBMapPartition[ K10_DEVICE_ID_LEN ];

    K10_LU_ID                   SPBLuId;

    ULONG                       RMUsingNTDisks;

    ULONG                       MinimumWriteIntentSize;

    ULONG                       MaxMemoryAllowed;

    ULONG                       MaxMirrorsWithWriteIntent;

    ULONG                       MaxMirrors;

    ULONG                       MaxLUsInMirror;

    ULONG                       MaxNumberSecondaryImages;

    ULONG                       MaxNumberCGsPerArray;

    ULONG                       MaxNumberMirrorsPerCG;

    ULONG                       BackdoorReadSupport;

    ULONG                       IsThinCapable;

}   RM_GET_DRIVER_PROPERTIES_OUT_BUFFER, *PRM_GET_DRIVER_PROPERTIES_OUT_BUFFER;

//
//  The Remote Mirrors quiesce is used to stop originiating I/O from an LU
//
//  RevisionId
//      Revision ID of the API. Users should always set this to RM_CURRENT_API_VERSION.
//
//  DoAll
//      If nonzero ignore object name and quiesce all LU's
//
//  QuiesceType
//      Nonzero a request to quiesce, zero is to unquiesce
//
//  ObjectName
//      Name of object to be quiesced/unquiesced
//
//
typedef struct _RM_QUIESCE_IN_BUFFER
{
    ULONG                       RevisionId;

    BOOLEAN                     DoAll;

    BOOLEAN                     QuiesceType;

    K10_DVR_OBJECT_NAME         ObjectName;

}   RM_QUIESCE_IN_BUFFER, *PRM_QUIESCE_IN_BUFFER;


//
//  The Remote Mirrors shudown operation is preparation to shutdown RM operations
//
//  RevisionId
//      Revision ID of the API. Users should always set this to RM_CURRENT_API_VERSION.
//
//  ShutdownType
//      K10_SHUTDOWN_OPERATION_WARNING (0)- the Admin Lib must return immediately from this OPC,
//      so you may take action to signal the driver but do not block on actions completed by
//      the driver.
//      K10_SHUTDOWN_OPERATION_SHUTDOWN (1)- the Admin Lib must block until all work
//      required by the driver for clean shutdown is complete. IO must be stopped.
//      Device objects must be closed, if not already closed due to an MJ_CLOSE received
//      from a device higher in the stack. Optionally, the component can save state to PSM.
//
typedef struct _RM_SHUTDOWN_IN_BUFFER
{
    ULONG                       RevisionId;

    ULONG                       ShutdownType;

}   RM_SHUTDOWN_IN_BUFFER, *PRM_SHUTDOWN_IN_BUFFER;

//
//  The Remote Mirrors Set Driver Properties Input Buffer is used in conjunction with
//  IOCTL_RM_SET_DRIVER_PROPERTIES to modify some of the specified mirroring properties.
//
//  RevisionId
//      Revision ID of the API. Users should always set this to RM_CURRENT_API_VERSION.
//
typedef struct _RM_GET_COMPAT_MODE_IN_BUFFER
{
    ULONG                       RevisionId;

}   RM_GET_COMPAT_MODE_IN_BUFFER, *PRM_GET_COMPAT_MODE_IN_BUFFER;


//
//  The Remote Mirrors Get Compatability Mode is used to return the mode of the
//  RM driver.
//
//  RevisionId
//      Revision ID of the API. Users should always set this to RM_CURRENT_API_VERSION.
//
//  Status
//      This is a NTSTATUS of the operation
//
//  CompatabilityMode
//      Mode of the RM driver at the moment; i.e. Latest, Old.
//
typedef struct _RM_GET_COMPAT_MODE_OUT_BUFFER
{
    ULONG                           RevisionId;

    ULONG                           Status;

    K10_COMPATIBILITY_MODE_WRAP     K10CompatibilityMode;

}   RM_GET_COMPAT_MODE_OUT_BUFFER, *PRM_GET_COMPAT_MODE_OUT_BUFFER;

//
//  The Remote Mirrors Set Compatability Mode is used to tell the RM driver
//  to commit and start using new features or revert to old features.
//
//  RevisionId
//      Revision ID of the API. Users should always set this to RM_CURRENT_API_VERSION.
//
//  CompatabilityMode
//      Mode of the RM driver to go to; i.e. Latest, Old.
//
typedef struct _RM_PUT_COMPAT_MODE_IN_BUFFER
{
    ULONG                           RevisionId;

    K10_COMPATIBILITY_MODE_WRAP     K10CompatibilityMode;

}   RM_PUT_COMPAT_MODE_IN_BUFFER, *PRM_PUT_COMPAT_MODE_IN_BUFFER;

//
//  The Remote Mirror Meida Failure enumeration indicates what type of slave
//  media failure (or lack thereof) to simulate.  Note this is an aide and no
//  substitute for just pulling the disks out!
//
//  RM_SLAVE_INVALID_MEDIA_FAILURE
//      An invalid media failure type.
//
//  RM_SLAVE_NO_MEDIA_FAILURE
//      There is no simulated media failure on the slave.
//
//  RM_SLAVE_IMMEDIATE_MEDIA_FAILURE
//      This type of media failure causes I/O on the slave to fail before
//      calling Flare.
//
//  RM_SLAVE_CALLBACK_MEDIA_FAILURE
//      This type of media failure causes I/O on the slave to fail on the
//      Flare callback path.  That is, the I/O actually succeed, but we are
//      faking it and pretending that it failed for testing purposes.
//
typedef enum _RM_SLAVE_MEDIA_FAILURE
{
    RM_SLAVE_INVALID_MEDIA_FAILURE  = -1,

    RM_SLAVE_NO_MEDIA_FAILURE,

    RM_SLAVE_IMMEDIATE_MEDIA_FAILURE,

    RM_SLAVE_CALLBACK_MEDIA_FAILURE

}   RM_SLAVE_MEDIA_FAILURE, *PRM_SLAVE_MEDIA_FAILURE;


// RM_COMPATIBILITY_MODE moved to "ndu_toc_common.h"

// We will not be using KDBM at this time, so define the required compat level to be too high
#define RM_COMPATIBILITY_LEVEL_KDBM RM_COMPATIBILITY_LEVEL_SIX

// Macro for easy determination of whether or not the committed Compat level indicates
// that KDBM is in use.  Note that we *must* coexist with existing PSM persistence until
// a level >= RM_COMPATIBILITY_LEVEL_KDBM is committed.
#define RM_IS_KDBM_IN_USE() (MirroringDriver.CompatibilityLevel >= RM_COMPATIBILITY_LEVEL_KDBM)

// Define that the compat level where thin support was introduced as level 5
#define RM_COMPATIBILITY_LEVEL_THIN RM_COMPATIBILITY_LEVEL_FIVE

// Macro for easy determination of whether or not the committed compat level indicates
// that thin is supported.
#define RM_IS_LOCAL_THIN_COMPAT() (MirroringDriver.CompatibilityLevel >= RM_COMPATIBILITY_LEVEL_THIN)

//
//  The values below have been chosen to match the existing values currently in use
//  for the various sync rates.  This assures backward compatability with values that
//  have been persisted.  They no longer map to specific delays - they are simply an
//  indication of the relative speed of the rates: fast, medium, slow = high, medium, low
//
typedef enum  {
    RM_SYNC_RATE_FAST = 0,
    RM_SYNC_RATE_MEDIUM = 100,
    RM_SYNC_RATE_SLOW = 1000000
} RM_SYNC_SPEED, *PRM_SYNC_SPEED;


//
// Add test IOCTLs inside the conditional compilation.  Only use the test
// IOCTLs in the checked (debug) build.
//

#if DBG // Checked (debug) build.

//
//  The Remote Mirrors Fake Lost Contact Input Buffer is used in conjunction
//  with IOCTL_RM_TEST_FAKE_LOST_CONTACT to simulate lost contact with a local
//  or remote SP.
//
//  RevisionId
//      Revision ID of the API.  Users should always set this to
//      RM_CURRENT_API_VERSION.
//
//  Node
//      The WWN of the array to simulate lost contact with.
//
//  Engine
//      The Engine (0 or 1) of the SP to simulate lost contact with.
//
//  MarkAlive
//      An indicator that specifies whether to mark the SP as alive (TRUE), or
//      to  simulate the CMI callback routine (FALSE).
//
typedef struct _RM_TEST_FAKE_LOST_CONTACT_IN_BUFFER
{
    ULONG                       RevisionId;

    K10_ARRAY_ID                Node;

    ULONG                       Engine;

    BOOLEAN                     MarkAlive;

}   RM_TEST_FAKE_LOST_CONTACT_IN_BUFFER, *PRM_TEST_FAKE_LOST_CONTACT_IN_BUFFER;

//
//  The Remote Mirrors Modify Debug Parameters Input Buffer is used in
//  conjunction with IOCTL_RM_TEST_MODIFY_DEBUG_PARAMETERS to change the
//  debugging parameters or the Remote Mirroring driver "on the fly".
//
//  RevisionId
//      Revision ID of the API.  Users should always set this to
//      RM_CURRENT_API_VERSION.
//
//  BreakPoint
//      A boolean value that indicates whether or not we want to cause a break
//      point in the Remote Mirroring driver.  This is helpful in debugging
//      because you do not have to set the break point through the debugger.
//
//  ModifyDebugMask
//      A boolean value that indicates whether or not the debug mask should be
//      altered with the value included in this IOCTL packet.
//
//  DebugMask
//      A new debug mask for the Remote Mirroring driver to use.  Note that this
//      IOCTL does not change the entry in the registry.  It only modifies the
//      in core version of the debug mask.  If the Remote Mirroring driver is
//      unloaded and then reloaded, the debug mask in the registry will be
//      resident.
//
//  PrintDatabases
//      A boolean value indicating whether or not to print out the internal
//      Remote Mirroring databases.
//
//  PrintStatistics
//      A boolean value indicating whether or not to print out the internal
//      Remote Mirroring statistics.
//
//  ModifySlaveMediaFailure
//      A boolean indicating whether or not to change the status of the slave
//      media failure.  When the slave has a media failure, the master will
//      Administratively Fracture the image.
//
//  MediaFailureType
//      The new media failure type for the slave.  Note that this IOCTL should
//      be issued on the slave that you want to change.  Changing this value
//      will cause ALL I/Os for ALL mirrors to Flare to fail (or succeed).
//      Note this is an aide and no substitute for just pulling the disks out!
//
//  DropAllIo
//      A boolean indicating whether or not the secondary image should drop
//      all I/O without sending a response to the primary image.  This is
//      useful in testing cancelled I/O.
//
typedef struct _RM_TEST_MODIFY_DEBUG_PARAMETERS_IN_BUFFER
{
    ULONG                       RevisionId;

    BOOLEAN                     BreakPoint;

    BOOLEAN                     ModifyDebugMask;

    ULONG                       DebugMask;

    BOOLEAN                     PrintDatabases;

    BOOLEAN                     PrintStatistics;

    BOOLEAN                     ModifySlaveMediaFailure;

    RM_SLAVE_MEDIA_FAILURE      MediaFailureType;

    BOOLEAN                     DropAllIo;

}   RM_TEST_MODIFY_DEBUG_PARAMETERS_IN_BUFFER, *PRM_TEST_MODIFY_DEBUG_PARAMETERS_IN_BUFFER;

#endif  // DBG

//
//  The Remote Mirrors Get Raw Performance Data Input Buffer is used in
//  conjunction with IOCTL_RM_GET_RAW_PERFORMANCE_DATA to support calls from
//  the Remote Mirror Performance Monitor DLL.
//
//  RevisionId
//      Revision ID of the API.  Users should always set this to
//      RM_CURRENT_API_VERSION.
//
typedef struct _RM_GET_RAW_PERFORMANCE_DATA_IN_BUFFER
{
    ULONG                       RevisionId;

}   RM_GET_RAW_PERFORMANCE_DATA_IN_BUFFER, *PRM_GET_RAW_PERFORMANCE_DATA_IN_BUFFER;

//
//  The Remote Mirrors Get Raw Performance Data Output Buffer is used in
//  conjunction with IOCTL_RM_GET_RAW_PERFORMANCE_DATA to answer requests from
//  the Remote Mirror Performance Monitor DLL.
//
//  NumberOfCmiTransmissions
//      The total number of CMI transmissions since the driver was started.
//      Note that this is for all mirrors.
//
//  NumberOfOutstandingCmiTransmissions
//      This value is a snapshot at the total number of outstanding CMI
//      transmissions at this moment in time.
//
typedef struct _RM_GET_RAW_PERFORMANCE_DATA_OUT_BUFFER
{
    EMCPAL_LARGE_INTEGER               NumberOfCmiTransmissions;

    LONG                        NumberOfOutstandingCmiTransmissions;

}   RM_GET_RAW_PERFORMANCE_DATA_OUT_BUFFER, *PRM_GET_RAW_PERFORMANCE_DATA_OUT_BUFFER;


#if RM_SECONDARY_READS_ALLOWED

//
//  The Remote Mirrors Get Raw Performance Data Input Buffer is used in
//  conjunction with IOCTL_RM_MODIFY_SECONDARY_READ_STATUS to support calls
//  to modify the ability to read on the secondary.
//
//  RevisionId
//      Revision ID of the API.  Users should always set this to
//      RM_CURRENT_API_VERSION.
//
//  SecondaryReadStatus
//      A boolean indicating whether reads are allowed on secondary images.  A
//      value of TRUE indicates that reads are allowed to secondary images.
//      A value of FALSE indicates that read operations fail on the secondary.
//
typedef struct _RM_MODIFY_SECONDARY_READ_STATUS_IN_BUFFER
{
    ULONG                       RevisionId;

    BOOLEAN                     SecondaryReadStatus;

}   RM_MODIFY_SECONDARY_READ_STATUS_IN_BUFFER,
        *PRM_MODIFY_SECONDARY_READ_STATUS_IN_BUFFER;

#endif // RM_SECONDARY_READS_ALLOWED

//
// Clone of a Mirror Related:
//
// Defines for InProgress flags
//
//      MVCLONES_IN_PROGRESS_NOT_SET -- the flag/enum has not been set
//
//      MVCLONES_SYNC_IN_PROGRESS    -- mirror synchronization is in-progress
//
//      MVCLONES_PROMOTE_IN_PROGRESS -- regular mirror promote is in-progress
//
//		MVCLONES_PROMOTE_ON_REMOTE_IN_PROGRESS - regular mirror promote in progress on remote
//
//      MVCLONES_CREATE_MIRROR_IN_PROGRESS -- mirror creation is in-Progress
//
//      MVCLONES_ADD_SLAVE_IN_PROGRESS  -- add slave is in-progress
//
//  Note: CG related in-Progress flags are not defined here. They are defined in the CG operation already.

typedef enum _MVCLONES_IN_PROGRESS
{
    MVCLONES_IN_PROGRESS_NOT_SET,
    MVCLONES_SYNC_IN_PROGRESS,                 		
    MVCLONES_PROMOTE_IN_PROGRESS,
	MVCLONES_PROMOTE_ON_REMOTE_IN_PROGRESS,
    MVCLONES_CREATE_MIRROR_IN_PROGRESS,
    MVCLONES_ADD_SLAVE_IN_PROGRESS,

    //  Add new values here...

    MVCLONES_IN_PROGRESS_MAX

} MVCLONES_IN_PROGRESS, *PMVCLONES_IN_PROGRESS;

//
// New Data Structures for Delta Polling
// 

//
// In Buffer for IOCTL_RM_POLL_GET_CONTAINER_SIZE
// 
// RevisionId
//      Revision ID of the API.  Users should always set this to
//      RM_CURRENT_API_VERSION.
// 
typedef struct _RM_POLL_GET_CONTAINER_SIZE_IN_BUFFER
{
    ULONG                       RevisionId;

}RM_POLL_GET_CONTAINER_SIZE_IN_BUFFER, *PRM_POLL_GET_CONTAINER_SIZE_IN_BUFFER;

//
// Out Buffer for IOCTL_RM_POLL_GET_CONTAINER_SIZE
// 
// Status
//      This is a NTSTATUS of the operation
// 
// MaxMirrorCount
//      The maximum number of mirrors that can be changed or deleted on the array.
//      This is also the feature limit.
// 
// MaxCGCount
//      The maximum number of CGs that can be changed or deleted on the array. 
//      This is also the feature limit.
//
// MembersPerCG
//      The maximum number of mirrors that can belong to one CG. 
//      This is also the feature limit.
// 
typedef struct _RM_POLL_GET_CONTAINER_SIZE_OUT_BUFFER
{
    ULONG                       Status;

    ULONG                       MaxMirrorCount;

    ULONG                       MaxCGCount;

    ULONG                       MembersPerCG;

}RM_POLL_GET_CONTAINER_SIZE_OUT_BUFFER, *PRM_POLL_GET_CONTAINER_SIZE_OUT_BUFFER;

//
// In Buffer for IOCTL_RM_POLL_RETRIEVE_DATA
// 
// RevisionId
//      Revision ID of the API. Users should always set this to 
//      RM_CURRENT_API_VERSION.
// 
// DeltaToken
//      The delta token passed in by the polling client. 
// 
typedef struct _RM_POLL_RETRIEVE_DATA_IN_BUFFER
{
    ULONG                       RevisionId;

    RM_POLL_DELTA_TOKEN         DeltaToken;

}RM_POLL_RETRIEVE_DATA_IN_BUFFER, *PRM_POLL_RETRIEVE_DATA_IN_BUFFER;

//
// This structure describes how many mirror and CG obects are contained in the
// poll data returned by the driver to the DLL, and whether the driver properties
// have changed.
// 
// HaveDriverPropertiesChanged
//      Boolean indicates if the driver properties changed. If FALSE on a delta 
//      poll, DriverProperties field of the PollResults data structure is undefined.
// 
// NumDeletedCGs
//      The number of deleted CGs.
// 
// NumDeletedMirrors
//      The number of deleted mirrors.
// 
// NumNewAndChangedCGs
//      The number of new or changed CGs.
// 
// NumNewAndChangedMirrors
//      The number of new or changed mirrors.
// 
typedef struct _RM_POLL_TABLE_OF_CONTENTS
{
    BOOLEAN                     HaveDriverPropertiesChanged;

    ULONG                       NumDeletedCGs;

    ULONG                       NumDeletedMirrors;

    ULONG                       NumNewAndChangedCGs;

    ULONG                       NumNewAndChangedMirrors;

}RM_POLL_TABLE_OF_CONTENTS, *PRM_POLL_TABLE_OF_CONTENTS;

//
// This structure combines both mirror and image properties (for all images) of a mirror 
// into one single structure.
// 
// MirrorData
//      Mirror Properties
// 
// ImageData
//      Image Properties
// 
typedef struct _RM_POLL_MIRROR_DATA
{
    RM_GET_MIRROR_PROPERTIES_OUT_BUFFER         MirrorData;

    RM_GET_IMAGE_PROPERTIES_OUT_BUFFER          ImageData[RM_MAX_SLAVES + 1];       

}RM_POLL_MIRROR_DATA, *PRM_POLL_MIRROR_DATA;

//
// Poll Data describing MV/S objects
// 
// TableOfContents
//      Describes how many mirror and CG objects are contained in the rest of the
//      structure, and whether the driver properties have changed.
// 
// DriverProperties
//      The driver properties. This data is not defined unless the TableOfContents 
//      indicates that the driver properties have changed.
// 
// VariablePortion
//      The start of the variable section in the Poll results. The objects that would 
//      be DENSELY packed in this portion are
//      -Array of CG IDs of Deleted CGs.
//      -Array of Mirror IDs of Deleted Mirrors.
//      -Array of new or changed CGs.
//      -Array of new or changed mirrors.
//      Pointer arithmetic will be used to allocate, pack and access this portion.
// 
typedef struct _RM_POLL_RESULTS
{
    RM_POLL_TABLE_OF_CONTENTS   TableOfContents;

    RM_GET_DRIVER_PROPERTIES_OUT_BUFFER     DriverProperties;
    
    CHAR                        VariablePortion[1];

}RM_POLL_RESULTS, *PRM_POLL_RESULTS;

//
// Out Buffer for IOCTL_RM_POLL_RETRIEVE_DATA
// 
// Status
//      The NTSTATUS of the overall operation
// 
// IsDeltaResponse
//      This Boolean indicates whether the response is a full or delta.
// 
// DeltaToken
//      The new delta token value returned by the driver. 
// 
// PollResults
//      The results of the poll - data describing all MV/S objects in case of a full
//      response, or just the changes in case of a delta reponse.
// 
typedef struct _RM_POLL_RETRIEVE_DATA_OUT_BUFFER
{
    ULONG                       Status;

    BOOLEAN                     IsDeltaResponse;

    RM_POLL_DELTA_TOKEN         DeltaToken;

    RM_POLL_RESULTS             PollResults;

}RM_POLL_RETRIEVE_DATA_OUT_BUFFER, *PRM_POLL_RETRIEVE_DATA_OUT_BUFFER;

#endif  // _K10_RM_H_

