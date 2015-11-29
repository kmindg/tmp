#ifndef NDU_TOC_COMMON_H
#define NDU_TOC_COMMON_H

/* This header file is for values that need to be known by NDU when installing a new
 * package.  For example, it may hold commit levels or cache revision numbers that NDU
 * can use to compare to the currently installed package.  Using this header file will
 * avoid duplication between our code and the packaging, such as the TOC.txt file.
 */


/* Increment this number and reset the disruptive counter to 0 anytime a re-image is
required by a change */
#define BASE_IMAGE_FAMILY 2

/* Increment this number when a disruptive upgrade is required.  This number can be
reset to 0 if a re-image is required */
#define BASE_DISRUPTIVE_COUNTER 0

/* Cache message revision number.
 * This should be incremented after changing any peer to peer messages.
 */
#define CA_CACHE_REVISION_NUMBER (3)
#define DRAM_CACHE_REVISION_NUMBER (4)


/*----------------------------------------------------------------------
 * REV_NUM
 *
 * rev_num is intended to provide backward compatability between Flare 5
 * and Flare 4.  I am aware that there is a rev_no field in the GLUT_DB.
 * This can't be changed because Flare 4 explicitly looks to see if
 * we've gone backward in rev and will not use the database pages if so.
 * By defining rev_num in the glut, I'm leveraging the fact that the
 * unused portions of the db page in flare are zeroed.  That way I'll
 * know if I need to initialize the hot_fru[] list (as well as some
 * fru_tbl[] fields).  If Flare 4 later runs, as long as there are no
 * hot spares bound in the system, these later fields are useless and
 * can be ignored.  So we should be able to go back and forth between
 * revs.
 *----------------------------------------------------------------------*/

/*
 * Commit Levels
 * A new Commit Level is added 
 *  -   when a new functionality gets added that needs a commit to get enabled OR
 *  -   a db layout change occurs. DB-Layout occurs as a result fo some new functionality.
 * So a New functionality can be 
 *  -   Software ONLY or
 *  -   Software and DB-Layout changes
 * A commit Level, that requires changes to DB, is assigned a number multiple of 100.
 *      By convention, these have a _DB_ somewhere in the name for better readability.
 * A commit level, that is software-only, the existing commit level is bumped by 1.
 */
/* Release 19 and below - Only for Fish(XP) platforms */
#define FLARE_COMMIT_LEVEL_DB_INITIAL_XP_REV                  100

/* Release 22 - Hammer ONLY */
#define FLARE_COMMIT_LEVEL_DB_R22_HAMMER                      200

/* Release 24 - Hammer and Fish(XP) platforms */
#define FLARE_COMMIT_LEVEL_REBUILD_LOGGING                    201
#define FLARE_COMMIT_LEVEL_DB_R24_FASTBIND                    300
#define FLARE_COMMIT_LEVEL_PROACTIVE_SPARE                    301
#define FLARE_COMMIT_LEVEL_DB_R24_HS_REV_NUM                  400

/* Release 26 - Hammer and Fish(XP) platforms */
#define FLARE_COMMIT_LEVEL_DB_R26_RAID6_REV_NUM               401

/* Release 28 - Fleet platforms */
#define FLARE_COMMIT_LEVEL_LOGICAL_CHECKPOINT                 402
#define FLARE_COMMIT_LEVEL_PRIVATE_RGS_LUS                    403
/* Note -- will be raised to 500, see DIMS 175106 */

#define FLARE_COMMIT_LEVEL_DB_R28_REV_NUM                     2000

/* Libra (R28 patch) supports new 4 port FC 8GB SLIC aka Glacier */
#define FLARE_COMMIT_LEVEL_GLACIER_SLIC_SUPPORT               2001
/* Taurus (R29) supports both Glacier (4G FC) and Poseidon (10G ISCSI) */
#define FLARE_COMMIT_LEVEL_R29_POSEIDON_SLIC_SUPPORT          2002
#define FLARE_COMMIT_LEVEL_BVA                                2003

/* Release 29 - fleet platforms */
/*  The R29 branch NDU revisions
 *  FLARE_COMMIT_LEVEL_DB_R29_8K_SUPPORT_REV_NUM (REV:2100)
 *  FLARE_COMMIT_LEVEL_FAST_ASSIGN (REV:2101) and
 *  FLARE_COMMIT_LEVEL_DB_R29_SPINDOWN_REV_NUM (REV: 2200) are removed as 
 *  part of Taurus NDU clean-up and are now consider as R29 in general.*/
#define FLARE_COMMIT_LEVEL_DB_R29_REV_NUM                     2300

/* Release 30 - CFD (Critical Foreign Drive) related DB changes in RG table. */
#define FLARE_COMMIT_LEVEL_R30_CFD_REV_NUM                    2400

/* Release 30 - Parity Shed Position related DB changes in the GLUT Table */
#define FLARE_COMMIT_LEVEL_DB_R30_PARITY_SHED_POS_REV_NUM     2500

/* Release 30 - EFD Cache system lun support. */
#define FLARE_COMMIT_LEVEL_EFDC_SYSTEM_LUN_R30_REV_NUM        2501

#define FLARE_COMMIT_LEVEL_R30_HEATWAVE_SLIC_SUPPORT          2502

/* Release 31 - Scorpion release */
#define FLARE_COMMIT_LEVEL_R31_V1_REV_NUM                     3000

#define FLARE_COMMIT_LEVEL_R31_SYS_MEM_CFG_SUPPORT            3100

/* Release 32 - Inyo release */
#define FLARE_COMMIT_LEVEL_R32_LARGE_ELEMENT_REV_NUM          3200

/* Rockies  GA */
#define FLARE_COMMIT_LEVEL_ROCKIES                            3300

/* Rockies MR1 */
#define FLARE_COMMIT_LEVEL_ROCKIES_MR1_CBE_TEMP               3400
#define FLARE_COMMIT_LEVEL_ROCKIES_MR1_CBE                    3410

#define FLARE_COMMIT_LEVEL_ROCKIES_UNMAP                      3411

/* Rockies Salmon River 4K Support */
#define FLARE_COMMIT_LEVEL_ROCKIES_4K_DRIVE_SUPPORT           3500


/* Highest possible value of commit level for a version running Flare.  
 * Any versions containing Flare must have a commit level less than or equal to this value.
 * Any versions which do not contain Flare must have a commit level greater than this value. 
 */ 
#define MAX_COMMIT_LEVEL_FOR_FLARE                            9000

/*---------------------------------------------------------------------- 
 * MCR Package version 
 * - SEP Minor Version is needed for DB changes 
 *----------------------------------------------------------------------*/
#define SEP_PACKAGE_MINOR_VERSION      2

/*----------------------------------------------------------------------
 * SEP_PACKAGE_VERSION is the SEP revision number for all the DB. 
 * This replaces CURRENT_FLARE_DB_REV_NUM since FLARE and CM DB no longer 
 * exist. 
 *  
 * SEP_PACKAGE_VERSION is the Major Commit Level that defines above plus 
 * the SEP Minor Version. 
 *----------------------------------------------------------------------*/
#define SEP_PACKAGE_VERSION                (FLARE_COMMIT_LEVEL_ROCKIES_4K_DRIVE_SUPPORT + SEP_PACKAGE_MINOR_VERSION) 

/*----------------------------------------------------------------------
 * CURRENT_FLARE_COMMIT_LEVEL is the current commit level for Base package. 
 *----------------------------------------------------------------------*/
#define CURRENT_FLARE_COMMIT_LEVEL         SEP_PACKAGE_VERSION

/**********   Flare Software Feature Commit Level - End     **********/
/*********************************************************************/


//
// The following defines the current version of the PSM on-disk data
// structures.  The high order two bytes are intended as the major version,
// the low order two bytes are intended as the minor version.
//
//
// NOTE: Version numbers are restricted to only 16 bits (NDU limitation)
//

//++
// Name:
//      PSM_VERSION_1_0
//
// Description:
//      PSM Version 1.0 - the first PSM version.
//
// Revision History:
//      03-02-00    MWagner    Created
//
//--
#define PSM_VERSION_1_0                0x00010000
//.End PSM_VERSION_1_0

//++
// Name:
//      PSM_VERSION_1_1
//
// Description:
//      PSM Version 1.1 - PSM with extended DAD areas.  PSM 1.0 only
//      reserved 64 PSM extents (starting right after the superblock) for
//      DADs.  This version includes 1024 extents towards the end of
//      the PSM Container for DADs.
//
// Revision History:
//      03-02-00    MWagner    Created
//
//--
#define PSM_VERSION_1_1                0x00010001
//.End PSM_VERSION_1_1

//++
// Name:
//      PSM_VERSION_1_2
//
// Description:
//      PSM Version 1.2 - Enhanced PSM with DataArea Locking, COFW and
//      write-once data areas.
//
// Revision History:
//      03-02-00    MWagner    Created
//
//--
#define PSM_VERSION_1_2                0x00010002
//.End PSM_VERSION_1_2

//++
// Name:
//      PSM_VERSION_1_3
//
// Description:
//      PSM Version 1.3 - Enhanced PSM with Data Area expansion.
//
// Revision History:
//      03-02-00    MWagner    Created
//
//--
#define PSM_VERSION_1_3                0x00010003
//.End PSM_VERSION_1_3

//++
// Name:
//      PSM_VERSION_1_4
//
// Description:
//      PSM Version 1.4 - Zeus Leaf Size Fix
//
// Revision History:
//      03-02-00    MWagner    Created
//
//--
#define PSM_VERSION_1_4                0x00010004
//.End PSM_VERSION_1_4

//++
// Name:
//      PSM_VERSION_SCORPION_1_0
//
// Description:
//      Scorpion 1.0
//
// Revision History:
//      03-12-10    MWagner    Created
//
//--
#define PSM_VERSION_SCORPION_1_0                0x00010010
//.End PSM_VERSION_SCORPION_1_0

//++
// Name:
//      PSM_VERSION_SCORPION_1_1
//
// Description:
//      Scorpion 1.1 - which adds the space for Data Area Logs
//
// Revision History:
//      03-12-10    MWagner    Created
//
//--
#define PSM_VERSION_SCORPION_1_1                0x00010011
//.End PSM_VERSION_SCORPION_1_1

//++
// Name:
//      PSM_VERSION_SCORPION_1_2
//
// Description:
//      Scorpion 1.2 - has the fix for 379952
//
// Revision History:
//      08-20-10    MWagner    Created
//
//--
#define PSM_VERSION_SCORPION_1_2                0x00010012
//.End PSM_VERSION_SCORPION_1_2

//++
// Name:
//      PSM_VERSION_VNX2_0
//
// Description:
//      Initial Rockies Version
//
// Revision History:
//      05-01-12    MWagner    Created
//
//--
#define PSM_VERSION_VNX2_0              0x00010200
//.End PSM_VERSION_VNX2_0
//
//++
// Name:
//      PSM_CURRENT_VERSION
//
// Description:
//      Denotes the latest PSM version.
//
// Revision History:
//      03-02-00    MWagner    Created
//
//--
#define PSM_CURRENT_VERSION            PSM_VERSION_VNX2_0
//.End PSM_CURRENT_VERSION

//++
// Name:
//      PSM_MINOR_VERSION_MASK
//
// Description:
//      This mask is used to get the PSM minor version (least significant
//      16 bits).
//
// Revision History:
//      03-02-00    MWagner    Created
//
//--
#define PSM_MINOR_VERSION_MASK         0x0000FFFF
//.End PSM_MINOR_VERSION_MASK


//  The K10_PSM_ADMIN_COMMIT_LEVEL macro is used to calculate what is returned to NDU during a GET_COMPAT_MODE.

#define K10_PSM_ADMIN_COMMIT_LEVEL(x) ( ( x ) & PSM_MINOR_VERSION_MASK )


// Because previous versions didn't support the GetCompatMode operation, we
// need to return 0 for the old version instead of the version number value
// stored in the PSM file and in-memory structures.  This define sets the
// difference between the internal version numbers and the ones we report to
// NDU.
//
#define K10_SYSTEM_ADMIN_CONTROL_NDU_VERSION_OFFSET 1

#define K10_LUNFILTER_VERSION       2       // K10_LunFilterAttribs
#define K10_LUNEXT_ATTRIBS_VERSION  3       // K10_LunExtendedAttribs
#define K10_LUNEXT_ENTRY_VERSION    1       // K10_Lun_Extended_Entry

#define K10_LUNFILTER_VERSION_1         1       // K10_LunFilterAttribs
#define K10_LUNEXT_ATTRIBS_VERSION_2    2       // K10_LunExtendedAttribs
#define K10_LUNEXT_ATTRIBS_VERSION_1    1       // K10_LunExtendedAttribs

//  The K10_SYSTEM_ADMIN_COMMIT_LEVEL macro is used to calculate what is returned to NDU during a GET_COMPAT_MODE.

#define K10_SYSTEM_ADMIN_COMMIT_LEVEL(x) ( ( x ) - K10_SYSTEM_ADMIN_CONTROL_NDU_VERSION_OFFSET )

#define K10_AGG_ADMIN_COMMIT_R29_LEVEL                      10
#define K10_AGG_ADMIN_COMMIT_R33SP3_LEVEL                   11
#define CURRENT_AGGREGATE_COMMIT_LEVEL                      K10_AGG_ADMIN_COMMIT_R33SP3_LEVEL

typedef enum enum_AGG_DRIVER_COMPATIBLE_LEVEL
{
    AGG_DRIVER_BASE_LEVEL = 0,
    AGG_DRIVER_LUN_SHRINK_SUPPORT = K10_AGG_ADMIN_COMMIT_R29_LEVEL,
    AGG_DRIVER_R29_LEVEL  = K10_AGG_ADMIN_COMMIT_R29_LEVEL,                             //Add MetaLUN shrinking and support 8K FLU in R29
    AGG_DRIVER_CURRENT_LEVEL = CURRENT_AGGREGATE_COMMIT_LEVEL        
}AGG_DRIVER_COMPATIBLE_LEVEL; 

//
//  Name:
//
//      RM_COMPATIBILITY_LEVEL
//
//  Description:
//
//      Enumeration of the compatibility levels for MirrorView/S.
//
//--
typedef enum _RM_COMPATIBILITY_LEVEL
{
    // Value to indicate that the compatability level of a remote has not yet been
    // determined
    RM_COMPATABILITY_LEVEL_INVALID       = -1,

    //
    //  Value to indicate the initial compatibility level for MirrorView.
    //
    RM_COMPATIBILITY_LEVEL_ZERO          = 0,

    //
    //  Value to indicate the R18 compatibility level setting needed for
    //  the Consistency Group.
    //
    RM_COMPATIBILITY_LEVEL_ONE,

    //
    //  Value to indicate autostart support, appearing in R19 patch through R24
    //
    RM_COMPATIBILITY_LEVEL_TWO,

    //  Value to indicate the R24 compatibility level setting needed for
    //  persisting the WIL state.
    //
    RM_COMPATIBILITY_LEVEL_THREE,

    //
    //  R26 compatiblity level for eliminating proactive trespass on shutdown and
    //  peer CMI lost contact.
    //
    RM_COMPATIBILITY_LEVEL_FOUR,

    //
    // R29 compatibility level for improved granularity and introduction of thin support
    // 
    RM_COMPATIBILITY_LEVEL_FIVE,

    //
    // R30 compatibility level for KDBM support
    // 
    RM_COMPATIBILITY_LEVEL_SIX,

    //
    // R33 compatibility level for max 64 members per CG on VNX 
    // .
    RM_COMPATIBILITY_LEVEL_SEVEN,

    //
    // KH+ compatibility level for max 128 members per CG & max 256 CGs
    // This is for future use and just reserved the space in PSM.
    // 
    RM_COMPATIBILITY_LEVEL_EIGHT,

    //
    // Thunderbird compatibility level for max 512 mirrors,524288 WIL BitmapBits on Oberon3/4
    //
 
    RM_COMPATIBILITY_LEVEL_NINE,

    //
    //  The max value of the MirrorView's compatibility level.
    RM_COMPATIBILITY_CURRENT = RM_COMPATIBILITY_LEVEL_NINE

} RM_COMPATIBILITY_LEVEL, *PRM_COMPATIBILITY_LEVEL;


//++
//
//  Name:
//
//      SC_COMPATIBILITY_LEVEL
//
//  Description:
//
//      Enumeration of the compatibility levels for SnapView.
//
//--
typedef enum
{
    //
    //  Value to indicate the initial compatibility level for SnapCopy.
    //    
    SC_COMPATIBILITY_LEVEL_INITIAL          = 0,

    //
    //  Value to indicate the R18 compatibility level setting needed for
    //  the Consistent Start functionality.
    //    
    SC_COMPAT_LEVEL_CONSISTENT_START,

    //
    //  Value to indicate the R18 compatibility level setting needed for
    //  the change in default granualarity
    //    
    SC_COMPAT_LEVEL_2K_GRANULARITY_CHANGE,

    // 
    // Value to indicate the R24 compatibility level setting needed for
    // the change in the way limits are retrieved from registry, for the 
    // PSM Expand operation and for Global Reserved LU Pool functionality.
    //
    SC_COMPAT_LEVEL_MARS_RELEASE,

    //
    // Value to indicate the R29 compatibility level setting needed for
    // increase the Snap source LUNs and consistent operation limits.
    //
    SC_COMPAT_LEVEL_TAURUS_RELEASE,

    //
    //  The max value of the SnapView's compatability level.
    //    
    SC_COMPATIBILITY_CURRENT = SC_COMPAT_LEVEL_TAURUS_RELEASE

} SC_COMPATIBILITY_LEVEL, *PSC_COMPATIBILITY_LEVEL;


//  Version information for the PSM data areas managed by Clones.

//  LEVEL 1 represents the database version for all releases prior to R19.
#define CLONES_DB_LEVEL_1     0x0001

//  LEVEL 2 represents the database version for release 19
#define CLONES_DB_LEVEL_2     0x0002

//  LEVEL 3 represents the database version for release 24
#define CLONES_DB_LEVEL_3     0x0003

//  LEVEL 4 represents the database version for release 28.
//  This level increased from "3" to "4" for R28 support.
#define CLONES_DB_LEVEL_4     0x0004

//  LEVEL 5 represents the database version for release 28++.
//  This level increased from "4" to "5" for R28++ support.
#define CLONES_DB_LEVEL_5     0x0005

//  LEVEL 6 represents the database version for release 32.
//  This level increased from "5" to "6" for R32 support.
#define CLONES_DB_LEVEL_6     0x0006

//  A single version is used for all data areas.
#define CL_CURRENT_DATABASE_VERSION     CLONES_DB_LEVEL_6

//  Similar to K10SystemAdmin, the version number starts at 1, but what is
//  returned to NDU is reduced by 1 for backwards compatibility with TOC.txt
//  files that did not have an entry (and therefore 0 was assumed).

#define K10_CLONES_ADMIN_NDU_VERSION_OFFSET 1
#define K10_CLONES_ADMIN_COMMIT_LEVEL(x) ( ( x ) - K10_CLONES_ADMIN_NDU_VERSION_OFFSET )

                    
//++
//
//  Name:
//
//      FAR_COMMIT_LEVEL
//
//  Description:
//
//      Enumeration of the compatibility levels for MirrorView/A.
//
//--
typedef enum _FAR_COMMIT_LEVEL
{
    FAR_COMMIT_LEVEL_MIN = 0,
    FAR_COMMIT_LEVEL_ORIG  = FAR_COMMIT_LEVEL_MIN,  // 0
    FAR_COMMIT_LEVEL_BDR,                           // 1
    FAR_COMMIT_LEVEL_THIN,                          // 2
    FAR_COMMIT_LEVEL_MAX = FAR_COMMIT_LEVEL_THIN

} FAR_COMMIT_LEVEL, *PFAR_COMMIT_LEVEL;

//
// To require a commit increment the commit level.
//
#define FAR_CURRENT_COMMIT_VERSION_LEVEL FAR_COMMIT_LEVEL_THIN



//++
//
//  Name:
//
//      MLU_COMPATIBILITY_LEVEL
//
//  Description:
//
//      Enumeration of the compatibility levels used by the Mapped LUN driver.
//
//--
typedef enum
{
    MLU_COMPAT_LEVEL_MIRA                   = 0,

    MLU_COMPAT_LEVEL_TAURUS                 = 2,
        
    MLU_COMPAT_LEVEL_ZEUS                   = 3,

    MLU_COMPAT_LEVEL_ZEUS_PATCH9            = 301,       //aka SP 9

    MLU_COMPAT_LEVEL_SCORPION               = 4,

    MLU_COMPAT_LEVEL_ELIAS                  = 5,

    MLU_COMPAT_LEVEL_ELIAS_PATCH2           = 311,    

    MLU_COMPAT_LEVEL_ORION                  = 6,        //This is old compat level for Inyo which has been deprecated.

    MLU_COMPAT_LEVEL_INYO                   = 320,

    MLU_COMPAT_LEVEL_INYO_MR                = 321,

    MLU_COMPAT_LEVEL_ROCKIES                = 330,

    MLU_COMPAT_LEVEL_ROCKIES_SP2            = 331,

    MLU_COMPAT_LEVEL_KITTY_HAWK             = 340,

    MLU_COMPAT_LEVEL_CHAPEL_HILL            = 345,

    MLU_COMPAT_LEVEL_KITTY_HAWK_PLUS        = 350,

    //
    //  Starting with PSI 5, the compat level value for PSI n will be (1000 + (10 * n)).
    //  We leave a gap of 10 between each PSI in order to accommodate later insertion of
    //  intermediate levels by patches or MRs that require changing persistent metadata
    //  or inter-SP message formats. This scheme allows up to 9 such intermediate levels
    //  to be inserted between any pair of PSIs, which should be more than adequate.
    // 

    MLU_COMPAT_LEVEL_PSI_05                 = 1050,

    MLU_COMPAT_LEVEL_PSI_06                 = 1060,

    MLU_COMPAT_LEVEL_PSI_07                 = 1070,

    MLU_COMPAT_LEVEL_PSI_08                 = 1080,

    MLU_COMPAT_LEVEL_PSI_09                 = 1090,

    MLU_COMPAT_LEVEL_PSI_10                 = 1100,

    MLU_COMPAT_LEVEL_PSI_11                 = 1110,

    MLU_COMPAT_LEVEL_PSI_12                 = 1120,

    MLU_COMPAT_LEVEL_PSI_13                 = 1130,

    MLU_COMPAT_LEVEL_PSI_14                 = 1140,

    MLU_COMPAT_LEVEL_PSI_15                 = 1150,

    MLU_COMPAT_LEVEL_PSI_16                 = 1160,

    MLU_COMPAT_LEVEL_PSI_17                 = 1170,

    MLU_COMPAT_LEVEL_PSI_18                 = 1180,

    MLU_COMPAT_LEVEL_PSI_19                 = 1190,

    MLU_COMPAT_LEVEL_PSI_20                 = 1200,

    MLU_COMPATIBILITY_CURRENT               = MLU_COMPAT_LEVEL_PSI_09

} MLU_COMPATIBILITY_LEVEL, *PMLU_COMPATIBILITY_LEVEL;


//++
//
//  Name:
//
//      PE_COMPATIBILITY_LEVEL
//
//  Description:
//
//      Enumeration of the compatibility levels for the Auto-Tiering Policy Engine.
//
//--
typedef enum
{
    PE_COMPAT_LEVEL_MIN           = 0,

    PE_COMPAT_LEVEL_ZEUS          = 1,

    PE_COMPAT_LEVEL_INYO          = 0x3200,

    PE_COMPATIBILITY_CURRENT = PE_COMPAT_LEVEL_INYO

} PE_COMPATIBILITY_LEVEL, *PPE_COMPATIBILITY_LEVEL;


//++
//
//  Name:
//
//      DD_COMPATIBILITY_LEVEL
//
//  Description:
//
//      Enumeration of the compatibility levels for Deduplication.
//
//--
typedef enum
{
    DD_COMPAT_LEVEL_MIN           = 0,

    DD_COMPAT_LEVEL_TITAN         = 1,

    DD_COMPATIBILITY_CURRENT = DD_COMPAT_LEVEL_TITAN

} DD_COMPATIBILITY_LEVEL, *PDD_COMPATIBILITY_LEVEL;


//++
//
//  Name:
//
//    VFLARE_REVISION
//
//  Description:
//
//    Enumeration for the revision levels for the vFlare driver.
//
//    Moving forward, every new original release that requires a new vFlare revision
//    will be assigned a numerical value that is ten more than the previous revision.
//    Any patches that require a new revision will be assigned revision values that
//    start at one greater than the revision used in the base release and incrementing by
//    one thereafter (e.g. 101 for the first such Northbridge patch, 102 for the second,
//    etc.). So room for nine new revisions for patches to a given release should be
//    more than adequate.
//--
typedef enum
{
    VFLARE_REVISION_NORTHBRIDGE             = 100,

    VFLARE_REVISION_CURRENT                 = VFLARE_REVISION_NORTHBRIDGE

} VFLARE_REVISION;


/**************************************************************************************/
/*****************      iSCSI Zone database version information      ******************/
//
// Version of the iSCSIZone Data Base. The version number was started at 0x0101 
// to aid in its uniqueness. The higher order byte is used as a major version and the 
// lower order byte is used for minor version.
//
#define ISCSI_ZONE_DB_VERSION_1_2        0x0102

//
// On Release 29 the Database size was increased, hence the minor version was 
// reset to 1 and the major version was increased to 2. 
// 
#define ISCSI_ZONE_DB_VERSION_2_1       0x0201

//
// This version number is used as a sanity check to insure that the ZML is using 
// the correct definition of the data base structures. It is also used as a sanity 
// check to insure that the ZML is attempting to parse a valid Zone data base.
//
#define ISCSI_ZONE_DB_VERSION            ISCSI_ZONE_DB_VERSION_2_1

/**************************************************************************************/

/**************************************************************************************/
/*********************      CPM database version information      *********************/

#define RELEASE_11_DM_VERSION        1
#define RELEASE_13_DM_VERSION        2
#define RELEASE_14_DM_VERSION        3
#define RELEASE_24_DM_VERSION        4
#define RELEASE_26_DM_VERSION        5
#define RELEASE_28_DM_VERSION        6
#define RELEASE_29_DM_VERSION        7
#define CURRENT_DM_VERSION           RELEASE_29_DM_VERSION

/**************************************************************************************/

#endif
