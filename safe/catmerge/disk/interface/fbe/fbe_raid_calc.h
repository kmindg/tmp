/*******************************************************************
 * Copyright (C) EMC Corporation 1997 - 2002
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *******************************************************************/
#ifndef _RAID_CALC_DLL_H
#define _RAID_CALC_DLL_H
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

#include "core_config.h"
// The below is used for all LBA calculations
// Currently only the aggregate functions support this.

# ifdef __cplusplus
#define DiskSectorCount ULONGLONG
#endif
/*************************************************************
 *
 * Raid Calculator DLL Interface file
 *
 * The Raid Calculator DLL can be used to get raid calculations
 * for a variety of unit types.
 *
 * The use of this library is as follows:
 *  1) Fill out the RAID_CALC_STRUCT 
 *  2) Call one of the below Raid Calculator interfaces
 *  3) The "output" fields will be filled in
 *
 *
 *************************************************************/

/*!***********************************************************
 * @def RAID_CALC_LBA_MAX define 64 bit Max LBA value 
 * Leave room for invalid input to be one more than this
 *************************************************************/
#define RAID_CALC_LBA_MAX 0xFFFFffffFFFFfffeULL

/*************************************************************
 * See the Raid Calculator and the Raid DLL Tester for
 * examples of how to use this dll.
 *
 *************************************************************/

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the RAIDDLL_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// RAIDDLL_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.

#ifdef RAIDDLL_EXPORTS
#define RAIDDLL_API CSX_MOD_EXPORT
#define RAIDDLL_API_CLASS CSX_CLASS_EXPORT
#else
#define RAIDDLL_API CSX_MOD_IMPORT
#define RAIDDLL_API_CLASS CSX_CLASS_IMPORT
#endif

//
// This is the definition of a raid type used in the 
// Raid Calculator DLL interface.
//
typedef enum _RAID_TYPE
{
    RAID0,
    RAID1,
    RAID10,
    RAID3,
    RAID5,
    HI5,
    IND_DISK,
    RAID6,
    AGGREGATE,
    RAID_MAX_UNIT_TYPE,
	/* If raid type does not match any of the above, 
     * then return INVALID_RAID_TYPE.
     */
    INVALID_RAID_TYPE = 0xFFFF
} RAID_TYPE;

#define RAID_MAX_DISK_ARRAY_WIDTH 16
#define RAID_DEFAULT_ADDRESS_OFFSET 0x8E000

#define RAID_DEFAULT_SECTORS_PER_ELEMENT 128
#define RAID_DEFAULT_RAID3_SECTORS_PER_ELEMENT 16
#define RAID_DEFAULT_HI5_SECTORS_PER_ELEMENT 130

// Piranha has a larger default parity stripe size.
#define RAID_DEFAULT_SECTORS_PER_ELEMENT_PIRANHA (RAID_DEFAULT_ELEMENTS_PER_PARITY_STRIPE_PIRANHA *\
                                                  RAID_DEFAULT_SECTORS_PER_ELEMENT)

#define RAID_DEFAULT_ARRAY_WIDTH 5

// The default sectors per element is just the CX default of 128.
#define RAID_DEFAULT_ELEMENTS_PER_PARITY_STRIPE RAID_DEFAULT_ELEMENTS_PER_PARITY_STRIPE_CX
#define RAID_DEFAULT_ELEMENTS_PER_PARITY_STRIPE_CX 1

// Piranha and hammer share the same default elements per parity stripe.
// If this ever changes, then we'll need to add a new elements per parity stripe.
//
#define RAID_DEFAULT_ELEMENTS_PER_PARITY_STRIPE_PIRANHA 8
#define RAID_DEFAULT_ELEMENTS_PER_PARITY_STRIPE_HAMMER 8

#define RAID_WIDTH_VALID(m_raidType,m_ArrayWidth) \
 ( ( m_ArrayWidth <= RAID_MAX_WIDTH[m_raidType] ) && \
   ( m_ArrayWidth >= RAID_MIN_WIDTH[m_raidType] ))

const unsigned int RAID_DEFAULT_WIDTH[RAID_MAX_UNIT_TYPE] = {
    5, // RAID0
    2, // RAID1
    4, // RAID10
    5, // RAID3
    5, // RAID5
    5, // HI5
    1, // ID
    10, // RAID6
    2, // AGGREGATE
};

const unsigned int RAID_MIN_WIDTH[RAID_MAX_UNIT_TYPE] = {
    3, // RAID0
    2, // RAID1
    2, // RAID10
    4, // RAID3
    3, // RAID5
    3, // HI5
    1, // ID
    3, // RAID6
    1  // AGGREGATE
};

const unsigned int RAID_MAX_WIDTH[RAID_MAX_UNIT_TYPE] = {
    16, // RAID0
    2, // RAID1
    16, // RAID10
    9, // RAID3
    16, // RAID5
    16, // HI5
    1,  // ID
    16, // RAID6
    16  // AGGREGATE
};

typedef enum _R5_DATA_LAYOUT
{
    RAID_LEFT_SYMMETRIC,
    RAID_RIGHT_SYMMETRIC,
    RAID_LEFT_ASYMMETRIC,
    RAID_RIGHT_ASYMMETRIC
} R5_DATA_LAYOUT;

typedef enum _RAID_PRODUCT_TYPE
{
    RAID_LONGBOW_PHASE2,
    RAID_CX_FIBRE,
    RAID_CX_ATA,
    RAID_CUSTOM,

    RAID_PARTITIONED_DENALI,
    RAID_K1,
    RAID_ALPINE,
    RAID_LONGBOW,
    RAID_CATAPULT,
    RAID_MAX_PRODUCT_TYPE
} RAID_PRODUCT_TYPE;

const UINT_32 RAID_ADDRESS_OFFSET_CONST[RAID_MAX_PRODUCT_TYPE] = {
#if 0
    0x2d000,  /*  Partitioned denali */
    0x2d000,  /* K1 */
    0x8f000,  /* Alpine */
    0x8d000,  /* Longbow */
    0x8d000,  /* Catapult */
#endif
    0x8d000,   /* Longbow Phase 2 */
    0x11048,  /* CX Fibre */
    0x11080   /* CX ATA */
};

#define RAID_DEFAULT_RAID_TYPE RAID5

#define RAID_DEFAULT_RAID_LEVEL RAID_PARITY


typedef enum _RAID_OPCODE
{
    READ_OP = 1,
    WRITE_OP = 2,
    REBUILD_OP = 8,
    VERIFY_OP = 9
} RAID_OPCODE;


//**************************************************
// RAID_GEN_REQ_STRUCT
//**************************************************
// This structure is the interface to
// fetch generate information on a RAID Driver sub Request Basis.
//**************************************************

typedef struct RAID_GEN_REQ_STRUCT {

    ULONGLONG Lba;
    unsigned int Blocks;

    /* Max blocks per drive.
     * Parity is the last one.
     */ 
    unsigned int ReadBlocksPerDrive[MAX_DISK_ARRAY_WIDTH];

    /* Max blocks per drive.
     * Parity is the last one.
     */ 
    unsigned int WriteBlocksPerDrive[MAX_DISK_ARRAY_WIDTH];
    
    /* Has max blocks over all drives.
     */
    unsigned int MaxDriveBlocks;
    
} RAID_GEN_REQ_STRUCT, *PRAID_GEN_REQ_STRUCT;

//**************************************************
// RAID_GEN_STRUCT
//**************************************************
// This structure is the interface to
// fetch generate information for an entire host request.
//**************************************************

typedef struct RAID_GEN_STRUCT {

    /* Treated as input param, total blocks to generate.
     */
    unsigned int TotalBlocks;

    /* Treated as output param,
     * total sub requests in this overall request.
     * How many request the raid breaks this into.
     */
    unsigned int TotalRequests;

    /* Input request, start at this initial request.
     * For example, a 1 means start at the first request,
     * and a 2 means start at the 2nd request.
     */
    unsigned int StartingRequest;

    /* Input only.  The kind of operation to generate.
     */
    RAID_OPCODE  OpCode;

    /* Input also.  Type of operation.  TRUE for host op FALSE for cache op.
     */
    BOOL bHostOp;

    /* Input also.  Since the cache page size effects generate size, we need this.
     */
    unsigned int CachePageSize;

    /* Input also.  Allows calculation of parity stripe size.
     */
    unsigned int default_sectors_per_element;
    
    /* This is input also.
     * Typically we fill in how many entries have been allocated below.
     */
    unsigned int GenArraySize;

    /* This must be the last entry.  
     * This is an array sized to GenArraySize.
     */
    RAID_GEN_REQ_STRUCT GenArray[1];
} RAID_GEN_STRUCT, *PRAID_GEN_STRUCT;


# ifdef __cplusplus
// This class is exported from the RaidDll.dll
class RAIDDLL_API_CLASS CRaidDll {
public:
    CRaidDll(void);
};

struct RAID_CALC_STRUCT;

class CRaid {


public:

    CRaid();
    void InitDefaults();
    
    void calcLba();
    void calcRaid0Logical();    
    void calcRaid0Physical();
    void calcRaid10Physical();
    void calcRaid10Logical();
    void calcRaid5Logical( const bool fixedParity = false);
    void calcRaid5Physical( const bool fixedParity = false);
    void calcHi5( const bool fixedParity = false);
    bool SetArrayWidth( const unsigned int m_nArrayWidth);
    void SetElementSize( const unsigned int m_nElementSize);
    unsigned int GetDataDisks();
    ULONG GetDataStripeSize( void );
    ULONG GetParityStripeSize( void );
    ULONG GetElementsPerParityStripe( void );

    const CRaid &operator=(const RAID_CALC_STRUCT &CalcStruct);

    ULONGLONG addressOffset[RAID_MAX_DISK_ARRAY_WIDTH];
    unsigned int fruNumbers[RAID_MAX_DISK_ARRAY_WIDTH];
    unsigned int sectorsPerElement;
    unsigned int arrayWidth;
    unsigned int elementsPerParityStripe;
    RAID_TYPE raidType;
    R5_DATA_LAYOUT raidLayout;
    RAID_PRODUCT_TYPE raidProduct;
    unsigned int dataLogToPhys[RAID_MAX_DISK_ARRAY_WIDTH];
    ULONGLONG physicalParityStart[RAID_MAX_DISK_ARRAY_WIDTH];
    unsigned int dataPos;
    unsigned int logDataPos; /* Logical start data position */
    unsigned int parityPos;
    unsigned int parityPos2; /* For R6, this is the 2nd parity */
    ULONGLONG pba;
    ULONGLONG lba;
    unsigned int twoGB;
    unsigned int logicalMode;
    unsigned int calcSegDb;
    unsigned int segmentOffset;
    ULONGLONG segDbLba;  /* For log segment database pba */

    /* If we're on Piranha, where the morley element size
     * is different, then this is set differently.
     */
    unsigned int defaultSectorsPerElement;
    
    /* This is the offset from the beginning of the LUN.
     * We add this value to every input Lba.
     * This is calculated using the AlignmentLba value.
     */
    ULONGLONG bindOffset;
    bool Initialized;
};

/**************************************************
 * RAID_CALC_PLATFORM
 *
 * Depending on how this is set, we will
 * calculate Raid 5 geometry a different way.
 **************************************************/
enum RAID_CALC_PLATFORM
{
    /* Use LONGBOW For everything before Chameleon 2 */
    RAID_CALC_LONGBOW,

    /* Use CX For everything including and after Cham 2 */
    RAID_CALC_CX,

    /* Use this for Piranha */
    RAID_CALC_PIRANHA,
};


/************************************************************************
*
 * This is the definition of the Raid Calculator
 * Dll interface.
 * The RAID_CALC_STRUCTURE must be filled out appropriately
 * before being passed in or the values returned will be indeterminate.
 *
 * RAID_TYPE raidType - Valid Types include     
 *                      RAID0, RAID1, RAID10, RAID3, RAID5, HI5, IND_DISK, RAID6
 *
 * sectorsPerElement - Element Size of the Unit
 *
 * AddressOffset     - Offset of the beginning of the drive where
 *                     this unit starts.
 *
 * ArrayWidth        - number of drives in the unit.
 *                     Valid values are:
 *                      Raid 5  - 3 to 16
 *                      Raid 3  - 5 or 9
 *                      Raid 1  - 2
 *                      Raid 10 - 4, 6, 8, 10, 12, 14, 16
 *                      Raid 0  - 3 to 16
 *                      ID      - 1
 *                      Raid 6  - 3 to 16
 * 
 * AlignmentLba     - This is the LBA, which should end up at the 
 *                    beginning of a stripe.  This is exactly the same
 *                    as the Alignment Offset value that the user
 *                    enters when they do a bind.
 *
 * Lba              - Logical Block Address.  Also referred to
 *                      as Host Address. Valid values are 0 to 0xFFFFFFFF
 *                      This views the address space from the host as
 *                      opposed to the drive level.
 *
 * Pba              - Physical Block Address.  This is a disk address.
 *                      This is the location on the disk where the host
 *                      data resides.
 *
 * Hi5SegmentOffset -   These are used for Hi5 Calculations
 * Hi5SegmentDb     -   Setting these is harmless if
 *                      Your unit type is not Hi5.
 *
 * Platform         -   Longbow, Alpine, K1 (Denali), SCSI products
 *                      uses Left Symmetric Layout.
 *
 *                      All others use Right Symmetric Layout.
 *
 * dataPos          -   This is the position where data resides.
 *
 * parityPos        -   This is the position where parity resides.
 * parityPos2       -   This is the position where Raid 6 Diagonal parity resides.
 *
 ************************************************************************
 */
#define RAID_OBJ_RESERVED_SIZE sizeof(CRaid)
typedef struct RAID_CALC_STRUCT {

    RAID_TYPE raidType; 
    unsigned int sectorsPerElement;
    ULONGLONG AddressOffset;
    unsigned int ArrayWidth;
    ULONGLONG AlignmentLba;  // This is the LBA we would like to align to a stripe
    ULONGLONG Lba;
    ULONGLONG Pba;
    unsigned int elementsPerParityStripe;
    unsigned int Hi5SegmentOffset;
    unsigned int Hi5SegmentDb;

    // Type of platform to be used for this request.
    RAID_CALC_PLATFORM platform;
    
    unsigned int dataPos;
    unsigned int parityPos;
    unsigned int parityPos2; /* R6 diagonal parity. */
    char Reserved[RAID_OBJ_RESERVED_SIZE];
    CRaid *raidObj;
} RAID_CALC_STRUCT, *PRAID_CALC_STRUCT;

/*************************************************************
 * RaidCalcLogicalToPhysical(),
 * RaidCalcPhysicalToLogical()
 *************************************************************
 * The following values from the RAID_CALC_STRUCT
 * must be filled out *prior* to calling the
 * below interface function.
 *
 * INPUT:
 *
 *  raidType
 *  sectorsPerElement
 *  Platform
 *  AddressOffset
 *  ArrayWidth
 *  AlignmentLba - 0 if no bind offset, nonzero otherwise.
 *
 *  For Logical to Physical:
 *      Lba
 *
 *  For Physical to Logical:
 *      Pba
 *      dataPos 
 *
 *  Additionally, for Hi5 Luns:
 *      Hi5SegmentOffset
 *      Hi5SegmentDb
 *
 * OUTPUT:
 *
 *  For a Logical to Physical Conversion 
 *  we return the following:
 *      Pba
 *      dataPos
 *      parityPos
 *      parityPos2 (if this is R6)
 *
 *  For a Physical to Logical Conversion
 *  we return the following:
 *      Lba
 *      parityPos
 *      parityPos2 (if this is R6)
 *     
 * 
 *************************************************************/

extern RAIDDLL_API bool RaidCalcLogicalToPhysical(RAID_CALC_STRUCT &CalcStruct);

extern RAIDDLL_API bool RaidCalcPhysicalToLogical(RAID_CALC_STRUCT &CalcStruct);

// Use this function to set the width.
extern RAIDDLL_API bool RaidCalcSetWidth(RAID_CALC_STRUCT &CalcStruct,
                                         unsigned int width);


// Use this function to set the element size.
extern RAIDDLL_API bool RaidCalcSetElementSize(RAID_CALC_STRUCT &CalcStruct,
                                                unsigned int ElementSize);

extern RAIDDLL_API unsigned int RaidCalcGetDataDisks(RAID_CALC_STRUCT &CalcStruct);



// This is used to initialize the RaidCalcStruct
// If this is not initialized before calling, 
// then we will fail on later interface calls

extern RAIDDLL_API bool RaidCalcInit(RAID_CALC_STRUCT &CalcStruct);

extern RAIDDLL_API bool RaidCalcGenerate(RAID_CALC_STRUCT &CalcStruct,
                                         RAID_GEN_STRUCT &RaidGenStruct);



#endif // __cplusplus
#endif // _RAID_CALC_DLL_H
