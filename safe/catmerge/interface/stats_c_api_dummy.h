/*-----------------------------------------------------------------------------
 * Copyright (C) 1991--2013, All Rights Reserved, by
 * EMC Corporation, Hopkinton, Mass.
 *
 * This software is furnished under a license and may be used and copied
 * only  in  accordance  with  the  terms  of such  license and with the
 * inclusion of the above copyright notice. This software or  any  other
 * copies thereof may not be provided or otherwise made available to any
 * other person. No title to and ownership of  the  software  is  hereby
 * transferred.
 *
 * The information in this software is subject to change without  notice
 * and  should  not be  construed  as  a commitment by EMC Corporation.
 *
 * EMC assumes no responsibility for the use or  reliability  of its
 * software on equipment which is not supplied by EMC.
 *

 *
 *  FACILITY:
 *
 *      Stats Infrastructure
 *
 *  ABSTRACT:
 *  
 *      Stats Infrastructure related definitions for C
 *
 *  AUTHORS:
 *  
 *      Observability Producer Team
 *
 *  CREATION DATE:
 *
 *      November  2012
 *
 *  MODIFICATION HISTORY:
 *
 *  RCS Log Removed.  To view log, use: rlog <file>
 *-----------------------------------------------------------------------------
 */

#ifndef __STATS_C_API_H_
#define __STATS_C_API_H_

#if defined(__cplusplus)
extern "C" {
#endif

/* 
*  Macros and other definitions.
*/
#define RETURN_IF_NULL(var, errorcode)    if(NULL == (var)) { return   (errorcode); }

// Only gcc, no warnings
//#define STAT_OFFSETOF(TYPE, MEMBER) 	  ({ TYPE obj; (unsigned long)&(obj.MEMBER) - (unsigned long)&obj; })

// Warning if no '-Wno-invalid-offsetof' specified, please specify it for compile on C++
// error: invalid access to non-static data member 'type::member' of NULL object
// error: (perhaps the 'offsetof' macro was used incorrectly)
#define STAT_OFFSETOF(TYPE, MEMBER) 	  ((unsigned long)&((TYPE *)0)->MEMBER)

typedef enum {
  METER_64   = 0,  // 64 bit meter type
  METER_32   = 1,  // 32 bit meter type
  U_METER_64 = 2,  // unsigned 64 bit meter type
  U_METER_32 = 3,  // unsigned 32 bit meter type
} STATS_METER_TYPE;

/*
* Base Stat type that is used by rest of the 
* types. 
*/
typedef struct {
  void  *pStat;
  unsigned int signature;
  STATS_METER_TYPE meter_type;
  void  *pMeta;
} STAT;


#ifndef __cplusplus
#ifndef BOOL_H
typedef enum {
    false = 0,
    true 
} bool;
#endif
#endif

/*
* Error return codes
*/
typedef signed char ps_i8_t;
typedef unsigned char ps_ui8_t;
typedef signed short ps_i16_t;
typedef unsigned short ps_ui16_t;
typedef signed int ps_i32_t;
typedef unsigned int ps_ui32_t;
typedef signed long long ps_i64_t;
typedef unsigned long long ps_ui64_t;
typedef STAT FAMILY;
typedef STAT STYPE;
typedef STAT FIELD;
typedef STAT COMPUTEDFIELD;
typedef STAT COMPUTED2FIELD;
typedef STAT COUNTER;
typedef STAT FACT;
typedef STAT COMPUTED2;
typedef STAT COMPUTED;
typedef STAT COMPOUND;
typedef STAT SET;
typedef void META;

typedef ps_i64_t (*returnFcnPtrType)( void* );
typedef ps_i32_t (*returnFcnPtrType32)( void* );
typedef ps_ui64_t (*returnFcnPtrTypeU)( void* );
typedef ps_ui32_t (*returnFcnPtrTypeU32)( void* );


typedef bool (* enumerateAndCallFcnPtr) ( void *, char *, void *);
typedef void (* EnumerateFcnPtr) (void * params);
/**
 * Type of function pointer, to be defined by user using macro STAT_ATOMIC_FETCH_C_DEF_START
 * and STAT_ATOMIC_FETCH_C_DEF_END.
 */
typedef void (*CFetchFcnPtr) (void * ri);
#if	!defined(WIN32)
#ifndef _SIZE_T_
typedef unsigned long size_t;
#endif
#endif

/*
* Error return codes
*/
#define STATS_SUCCESS               0x00 /* success */
#define STATS_BAD_ARGUMENT          0x01 /* a bad argument is specified */
#define STATS_BAD_VISIBILITY        0x02 /* invalid visibility specified */
#define STATS_OP_NOT_ALLOWED        0x03 /* operation is not permitted */
#define STATS_NULL_PARENT           0x04 /* parent argument is NULL */
#define STATS_NULL_REFERENCE        0x05 /* reference argument is NULL */
#define STATS_NULL_COUNTER          0x06 /* counter argument is NULL */
#define STATS_NULL_FACT             0x06 /* fact argument is NULL */
#define STATS_NULL_RETURNFCNPTR     0x07 /* returnFcnPtr agrument is NULL */
#define STATS_NULL_STYPE            0x08 /* stype argument is NULL */
#define STATS_NULL_NAME             0x09 /* name argument is NULL */
#define STATS_NULL_TITLE            0x0A /* title argument is NULL */
#define STATS_NULL_SUMMARY          0x0B /* summary argument is NULL */
#define STATS_NULL_ELEMENT_SUMMARY  0x0C /* element summary argument is NULL */
#define STATS_NULL_OPERAND          0x0D /* operand argument is NULL */
#define STATS_BAD_OPERATOR          0x0E /* operator argument is invalid */
#define STATS_BAD_BEHAVIOR          0x0F /* behavior argument is invalid */
#define STATS_NULL_STATPATHLIST     0x10 /* statpathlist argument is NULL */
#define STATS_NULL_FAMILY           0x11 /* family argument is NULL */
#define STATS_NULL_META             0x12 /* meta argument is NULL */
#define STATS_NULL_COMPUTED         0x13 /* computed or computed2 argument is NULL */
#define STATS_INVALID_COUNT         0x14 /* count is 0 or out of range */
#define STATS_INVALID_METER_TYPE    0x15 /* fact type is invalid */
#define STATS_BAD_PARENT            0x16 /* parent is invalid */
#define STATS_BAD_STYPE             0x17 /* stype is invalid */
#define STATS_BAD_FACT              0x18 /* fact is invalid */
#define STATS_BAD_COUNTER           0x19 /* counter is invalid */
#define STATS_NOT_SUPPORTED         0x20 /* The function is not supported yet */

const char* getCAPIErrorMsg(int errorCode);

#define MAX_STAT_PATH_LIST_COUNT    1024

//! Defines the types of visibility a node has.
/** The Tree variants become the default for all children.
 * product - intended to be seen by end users
 * support - intended for support personel
 * eng     - intended for developer community
 */
typedef enum {
  PRODUCTTREE = 0,
  PRODUCT     = 1,
  SUPPORTTREE = 2,
  SUPPORT     = 3,
  ENG         = 4,
  INTERNAL    = 5
} VISIBILITY;

/*
* Field Types, can be used with compound or compoundR 
*/
typedef enum {
  U_FACT32_FIELD    = 0,
  FACT32_FIELD      = 1,
  U_FACT64_FIELD    = 2,
  FACT64_FIELD    = 3,
  U_COUNTER32_FIELD = 4,
  U_COUNTER64_FIELD = 5
} FIELD_KIND;

/*
* Resolver Hints,used while creating the Set
*/
typedef enum {
  FILESYSTEM_HINT         = 'F',
  METAVOLUME_HINT         = 'M',
  DISKVOLUMES_HINT        = 'D',
  QTREES_HINT             = 'Q',
  IP_HINT                 = 'I',
  UID_HINT                = 'U',
  GID_HINT                = 'G',
  INODE_HINT              = 'O',
  FILESYSTEMANDMOUNT_HINT = 'L',
  VDM_HINT                = 'V',
  NORESOLVER              = 'N'
} RESOLVER_HINT;

/*
* Computed Operators
*/
typedef enum {
  SUM  = 0,
  PDCT = 1,
  AVG  = 2,
} COMPUTEDOPTYPE;

/*
* Operators for computed2 
*/
typedef enum {
  DIV  = 0,
  MULT = 1,
  ADD  = 2,
  SUB  = 3,
  PCT  = 4
} COMPUTED2OPTYPE;


/*
* Behaviour enumerations
*/
typedef enum {
  COUNTER_BEHAVIOR        = 0,
  FACT_BEHAVIOR           = 1,
  ERROR_COUNTER_BEHAVIOR  = 2,
} BEHAVIOR;

/*
* KScale, specify the display behavior type
*/
typedef enum {
  KSCALE_UNO   = 0,
  KSCALE_KILO  = 1,
  KSCALE_MEGA  = 2,
  KSCALE_GIGA  = 3,

  KSCALE_UNBI  = 4,
  KSCALE_KIBI  = 5,
  KSCALE_MIBI  = 6,
  KSCALE_GIBI  = 7,
  KSCALE_UNDEF = 8,
} STATS_KSCALE;

/*
* Wrap,specifies the wrap policy of a stat
*/
typedef enum {
  WRAP_UNK        = 0,    // Unknown Wrap type
  WRAP_W16        = 1,    // Wraps at 16 bits
  WRAP_W32        = 2,    // Wraps at 32 bits
  WRAP_W64        = 3,    // Wraps at 64 bits
  WRAP_W32_OR_W64 = 4,  // Wrap inconsistently at either 32 or 64 bits
  WRAP_E32        = 5,
  WRAP_E64        = 6,
} STATS_WRAP;
/*
*  Order here is intentional, refer to statstypes.cxx for more details
*/
typedef struct {
  enumerateAndCallFcnPtr enumerateAndCall;
} EnumParams;

/**
 * struct for definition of SType fields
 */
typedef struct  _StatField {
    FIELD*       field;
    const char*  name;
    const char*  summary;
    VISIBILITY   vis;
    const char*  title;
    long         offset;
    FIELD_KIND   kind;
    char*        metaUnits;
    STATS_KSCALE metaKScale;
    short        metaWidth;
    STATS_WRAP   metaWrap;
} StatField;

//! Define the fetch function of type 'CFetchFcnPtr' for atomic struct
/** - FCNNAME: name of the fetch function which should be passed to createAtomicSType 
 *  - DESTTYPE: the user defined struct to hold the result
 *  - SOURCETYPE: the user defined struct 
 *  between these 2 macros, result is pointer to DESTTYPE, source is pointer to SOURCETYPE
 */
#define STAT_ATOMIC_FETCH_C_DEF_START( FCNNAME, DESTTYPE, SOURCETYPE )     \
void FCNNAME( void* ri ){                                              \
  void *_result;                                                       \
  void *_source;                                                       \
  DESTTYPE *result;                                                    \
  SOURCETYPE *source;                                                  \
  getSourceAndResultAddr(ri, &_source, &_result);                      \
  result = ( DESTTYPE *)_result;                                       \
  source = ( SOURCETYPE *)_source;

#define STAT_ATOMIC_FETCH_C_DEF_END }



//! Creates a stats family node
/** 'createFamily' allows a developer to create a stats family that can be used
 *  parent for other stats
 *
 * Arguments:
 * - family: This is an out argument If this function returns STATS_SUCCESS,
 *           this agrument will point to the stat family created by this
 *           function.
 * - parent: Pointer to the parnet family. The new family will be created
 *           as a child under this family. If this agrument is NULL the
 *           family will will be created as a child of monitorRoot which is
 *           root node for stats tree.
 * - name: This is the name of family being created.
 * - summary: Short description about this family stat.
 * - visibility: The visibility of this family stat.
 *
 * Return values:
 * - STATS_SUCCESS        on success
 * - STATS_NULL_NAME      if name argument is NULL
 * - STATS_NULL_SUMMARY   if summary argument is NULL
 * - STATS_BAD_VISIBILITY if an invalid visibility is specified
 * - STATS_BAD_ARGUMENT   if parent is not pointing to a valid family created
 *                        using a call to createFamily
 *
 * The caller of this function is responsible for freeing the memory allocated
 * for the family by calling deleteFamily.
 * DEMO for Wadah and TEam
 */
int createFamily(FAMILY **family, FAMILY *parent, const char *name,
                 const char *summary, VISIBILITY visibility);


//! Deletes a family node created using createFamily
/** 'deleteFamily' allows a developer to create a stats family that can be used
 *  parent for other stats
 *
 * Arguments:
 * - family: This is an in argument If this function returns STATS_SUCCESS,
 *           this pointer pointed to the FAMILY is deleted and memory associated
 *           with this stat is freed.

 *
 * Return values:
 * - STATS_SUCCESS        on success
 * - STATS_BAD_ARGUMENT   if the arguement is not pointing to a valid family created
 *                        using a call to createFamily
 *
 */
int deleteFamily(FAMILY *family);


/*
// Counter API Starts here
*/


//! Creates a native counter stat
/** 'createNativeCounter' allows a developer to create a native counter stats
 *
 * Arguments:
 * - counter: This is an out argument If this function returns STATS_SUCCESS,
 *            this agrument will point to the native counter created by this
 *            function.Only unsigned 32bit and 64bit counter are supported.
 * - parent: Pointer to the parnet family. The native counter stat will be
 *           created as a child under this family. This agrument should
 *           not be NULL.
 * - name:   This is the name of native counter being created.
 * - summary: Short description about this native counter stat.
 * - visibility: The visibility of this native counter stat.
 * - title: Title of this native counter.
 * - meter_type: This is the bitwidth hint of the counter
 *
 * Return values:
 * - STATS_SUCCESS        on success
 * - STATS_NULL_NAME      if name argument is NULL
 * - STATS_NULL_SUMMARY   if summary argument is NULL
 * - STATS_NULL_TITLE     if title argument is NULL
 * - STATS_BAD_VISIBILITY if an invalid visibility is specified
 * - STATS_NULL_PARENT    if parent argument is NULL
 * - STATS_BAD_ARGUMENT   if parent is not pointing to a valid family created
 *                        using a call to createFamily
 *
 * The caller of this function is responsible for freeing the memory allocated
 * for the native counter by calling deleteCounter.
 */
int createCounter(COUNTER **counter, FAMILY *parent, const char *name,
                        const char *summary, VISIBILITY visibility,
                        const char *title,META *meta, STATS_METER_TYPE meter_type);

//! Deletes a counter  statistic created using createCouneter
/** 'deleteCounter' allows a developer to delete any flavor of Counter stat which is 
 *  created using one of 'createFact', 'createReferenceCounter' or 'createVirtualCounter' 
 *  methods.
 *
 * Arguments:
 * - counterStat: This pointer pointed to the Counter Statistic to be deleted.
 *
 * Return values:
 * - STATS_SUCCESS        on success
 * - STATS_BAD_ARGUMENT   if the arguement is not pointing to a valid Fact pointer created
 *                        byusing a call to create*Counter
 *
 */
int deleteCounter(COUNTER *counter);

//! Increments native counter stat by one
/** 'incCounter' allows a developer to increment a native counter
 * stats by one 
 *
 * Arguments:
 * - counter: Pointer to a native counter created using a call to
 *            createNativeCounter. If this function returns STATS_SUCCESS,
 *            the couter value will be incremented by one. This should
 *            should not be NULL.
 *
 * Return values:
 * - STATS_SUCCESS      on success
 * - STATS_BAD_ARGUMENT if counter is not pointing to a valid native counter
 *                      created using a call to createNativeCounter.
 * - STATS_NULL_COUNTER if counter agrument is NULL.
 */
int incCounter(COUNTER *counter);


//! Increments native counter stat by the specified value
/** 'addNativeCounter' allows a developer to increment a native counter
 * stats by value specified as add argument 
 *
 * Arguments:
 * - counter: Pointer to a native counter created using a call to
 *            createNativeCounter. If this function returns STATS_SUCCESS,
 *            the couter value will be incremented by the value specified by
 *            add argument. This should not be NULL.
 * - add: The value this native counter will be increment by
 *
 * Return values:
 * - STATS_SUCCESS      on success
 * - STATS_BAD_ARGUMENT if counter is not pointing to a valid native counter
 *                      created using a call to createNativeCounter.
 * - STATS_NULL_COUNTER if counter agrument is NULL.
 */
int addCounter(COUNTER *counter, ps_ui64_t add);


//! Increments native counter stat by one
/** 'incCounter' allows a developer to increment a native counter
 * stats by one  while maintaining the atomicity of the add operation
 *
 * Arguments:
 * - counter: Pointer to a native counter created using a call to
 *            createNativeCounter. If this function returns STATS_SUCCESS,
 *            the couter value will be incremented by one. This should
 *            should not be NULL.
 *
 * Return values:
 * - STATS_SUCCESS      on success
 * - STATS_BAD_ARGUMENT if counter is not pointing to a valid native counter
 *                      created using a call to createNativeCounter.
 * - STATS_NULL_COUNTER if counter agrument is NULL.
 */
int atomicIncCounter(COUNTER *counter);


//! Increments native counter stat by the specified value
/** 'atomicAddCounter' allows a developer to increment a native counter
 * stats by value specified as add argument and garuntees atomicity of 
 * the add operation.
 *
 * Arguments:
 * - counter: Pointer to a native counter created using a call to
 *            createNativeCounter. If this function returns STATS_SUCCESS,
 *            the couter value will be incremented by the value specified by
 *            add argument. This should not be NULL.
 * - add: The value this native counter will be increment by
 *
 * Return values:
 * - STATS_SUCCESS      on success
 * - STATS_BAD_ARGUMENT if counter is not pointing to a valid native counter
 *                      created using a call to createNativeCounter.
 * - STATS_NULL_COUNTER if counter agrument is NULL.
 */
int atomicAddCounter(COUNTER *counter, ps_ui64_t add);


//! Creates a reference counter stat
/** 'createReferenceCounter' allows a developer to create a reference counter stats
 *
 * Arguments:
 * - counter: This is an out argument If this function returns STATS_SUCCESS,
 *            this agrument will point to the native counter created by this#endif
 *            function.Only unsigned 32bit and 64bit counter are supported.
 * - parent: Pointer to the parnet family. The native counter stat will be
 *           created as a child under this family. This agrument should
 *           not be NULL.
 * - name:   This is the name of native counter being created.
 * - summary: Short description about this native counter stat.
 * - reference: This is address of the actual counter. The actual counter should be a 64bit unsigned integer. 
 *                 This should not be NULL.
 * - visibility: The visibility of this native counter stat.
 * - title: Title of this native counter.
 * - meter_type: This is the bitwidth hint of the counter
 *
 * Return values:
 * - STATS_SUCCESS        on success
 * - STATS_NULL_NAME      if name argument is NULL
 * - STATS_NULL_SUMMARY   if summary argument is NULL
 * - STATS_NULL_TITLE     if title argument is NULL
 * - STATS_BAD_VISIBILITY if an invalid visibility is specified
 * - STATS_NULL_PARENT    if parent argument is NULL
 * - STATS_BAD_ARGUMENT   if parent is not pointing to a valid family created
 *                        using a call to createFamily
 * - STATS_NULL_RETURNFCNPTR if returnFcnPtr argument is NULL
 * - STATS_NULL_META      if meta argument is NULL
 *
 * The caller of this function is responsible for freeing the memory allocated
 * for the native counter by calling deleteCounter.
 */
int createReferenceCounter(COUNTER **_counter, FAMILY *_parent, const char *_name,
            const char *_summary, void *reference, VISIBILITY _vis,
            const char *_title, META *_meta, STATS_METER_TYPE meter_type);

//! Creates a virtual counter stat
/** 'createVirtualCounter' allows a developer to create a virtual counter stats
 *
 * Arguments:
 * - counter: This is an out argument If this function returns STATS_SUCCESS,
 *            this agrument will point to the native counter created by this
 *            function.Only unsigned 32bit and 64bit counter are supported.
 * - parent: Pointer to the parnet family. The native counter stat will be
 *           created as a child under this family. This agrument should
 *           not be NULL.
 * - name:   This is the name of native counter being created.
 * - summary: Short description about this native counter stat.
 * - returnFcnPtr: This is a pointer to a function which will return
 *                 the value of counter. This function should return
 *                 64bit unsigned integer. This should not be NULL.
 * - visibility: The visibility of this native counter stat.
 * - title: Title of this native counter.
 * - meter_type: This is the bitwidth hint of the counter
 *
 * Return values:
 * - STATS_SUCCESS        on success
 * - STATS_NULL_NAME      if name argument is NULL
 * - STATS_NULL_SUMMARY   if summary argument is NULL
 * - STATS_NULL_TITLE     if title argument is NULL
 * - STATS_BAD_VISIBILITY if an invalid visibility is specified
 * - STATS_NULL_PARENT    if parent argument is NULL
 * - STATS_BAD_ARGUMENT   if parent is not pointing to a valid family created
 *                        using a call to createFamily
 * - STATS_NULL_RETURNFCNPTR if returnFcnPtr argument is NULL
 * - STATS_NULL_META      if meta argument is NULL
 *
 * The caller of this function is responsible for freeing the memory allocated
 * for the native counter by calling deleteCounter.
 */
int createVirtualCounter(COUNTER **_counter, FAMILY *_parent, const char *_name,
            const char *_summary, void *_returnFcnPtr, VISIBILITY _vis,
            const char *_title, META *_meta, STATS_METER_TYPE meter_type);
            
/*
// FACT API Starts here
*/
            
//! Creates a native fact stat
/** 'createFact' allows a developer to create a native fact stat
 *
 * Arguments:
 * - fact: This is an out argument If this function returns STATS_SUCCESS,
 *         this agrument will point to the native fact created by this
 *         function.
 * - parent: Pointer to the parnet family. The native fact stat will be
 *           created as a child under this family. This agrument should
 *           not be NULL.
 * - name: This is the name of native fact being created.
 * - summary: Short description about this native fact stat.
 * - visibility: The visibility of this native fact stat.
 * - title: Title of this native fact.
 * - meter_type: This is the type of fact (signed or unsigned, 64 or 32 bit).
 *   <TABLE>
 *    <TR><TD>meter_type </TD>       <TD> function type expected (parameter five)</TD></TR>
 *    <TR><TD>METER_64   </TD>       <TD> ps_i64_t (*returnFcnPtrType)( void* );</TD></TR>
 *    <TR><TD>METER_32   </TD>       <TD> ps_i32_t (*returnFcnPtrType)( void* );</TD></TR>
 *    <TR><TD>U_METER_64 </TD>       <TD> ps_ui64_t (*returnFcnPtrType)( void* );</TD></TR>
 *    <TR><TD>U_METER_32 </TD>       <TD> ps_ui32_t (*returnFcnPtrType)( void* );</TD></TR>
 *   </TABLE>
 *
 * Return values:
 * - STATS_SUCCESS            on success
 * - STATS_NULL_NAME          if name argument is NULL
 * - STATS_NULL_SUMMARY       if summary argument is NULL
 * - STATS_NULL_TITLE         if title argument is NULL
 * - STATS_BAD_VISIBILITY     if an invalid visibility is specified
 * - STATS_NULL_PARENT        if parent argument is NULL
 * - STATS_BAD_ARGUMENT       if parent is not pointing to a valid family created
 *                            using a call to createFamily
 * - STATS_INVALID_METER_TYPE if invalid meter_type is specified
 *
 * The caller of this function is responsible for freeing the memory allocated
 * for the native fact by calling deleteFact. For creation Reference fact and 
 * virtual fact check 'createReferenceFact' and 'createVirtualFact' respectively.
 */

int createFact(FACT **fact, FAMILY *parent, const char *name,
                     const char *summary, VISIBILITY visibility,
                     const char *title, META *_meta, STATS_METER_TYPE meter_type);

//! Creates a reference fact statistic
/** 'createReferenceFact' allows a developer to create a reference fact statistic
 *
 * Arguments:
 * - fact: This is an out argument If this function returns STATS_SUCCESS,
 *         this agrument will point to the reference fact created by this
 *         function.
 * - parent: Pointer to the parent family. The reference fact stat will be
 *           created as a child under this family. This agrument should
 *           not be NULL.
 * - name: This is the name of reference fact being created.
 * - summary: Short description about this reference fact statistic.
 * - reference: This is a pointer to the actual fact object that this 
 *   statistic will use. It's type should match the meter_type passed in.
 *   See 'meter_type' below for more details.
 * - visibility: The visibility of this reference fact statistic.
 * - title: Title of this reference fact.
 * - meta: Metadata for this statistic.
 * - meter_type: This is the type of fact (signed or unsigned, 64 or 32 bit).
 *   <TABLE>
 *    <TR><TD>meter_type </TD>       <TD> function type expected (parameter five)</TD></TR>
 *    <TR><TD>METER_64   </TD>       <TD> ps_i64_t (*returnFcnPtrType)( void* );</TD></TR>
 *    <TR><TD>METER_32   </TD>       <TD> ps_i32_t (*returnFcnPtrType)( void* );</TD></TR>
 *    <TR><TD>U_METER_64 </TD>       <TD> ps_ui64_t (*returnFcnPtrType)( void* );</TD></TR>
 *    <TR><TD>U_METER_32 </TD>       <TD> ps_ui32_t (*returnFcnPtrType)( void* );</TD></TR>
 *   </TABLE>
 *
 * Return values:
 * - STATS_SUCCESS        on success
 * - STATS_NULL_NAME      if name argument is NULL
 * - STATS_NULL_SUMMARY   if summary argument is NULL
 * - STATS_NULL_TITLE     if title argument is NULL
 * - STATS_NULL_REFERENCE if reference argument is NULL
 * - STATS_BAD_VISIBILITY if an invalid visibility is specified
 * - STATS_NULL_PARENT    if parent argument is NULL
 * - STATS_BAD_ARGUMENT   if parent is not pointing to a valid family created
 *                        using a call to createFamily
 * - STATS_INVALID_METER_TYPE if invalid meter_type is specified
 *
 * The caller of this function is responsible for freeing the memory allocated
 * for the reference fact by calling deleteFact. For creation of native fact and
 * virtual fact check 'createFact' and 'createVirtualFact' respectively.
 */

int createReferenceFact(FACT **fact, FAMILY *parent, const char *name,
                        const char *summary, void *reference,
                        VISIBILITY visibility, const char *title, META *meta,
                        STATS_METER_TYPE meter_type);

//! Creates a virtual fact statistic
/** 'createVirtualFact' allows a developer to create a virtual fact statistic
 *
 * Arguments:
 * - fact: This is an out argument If this function returns STATS_SUCCESS,
 *         this agrument will point to the reference fact created by this
 *         function.
 * - parent: Pointer to the parent family. The virtual fact stat will be
 *           created as a child under this family. This agrument should
 *           not be NULL.
 * - name: This is the name of virtual fact being created.
 * - summary: Short description about this virtual fact statistic.
 * - returnFcnPtr: This is a pointer to the fact function that this statistic 
 *   will use. The function return type should change based on the stats meter type 
 *   (the last argument).  The function pointer parameter needs to be cast as a
 *   (void *). Please refer to the 'meter_type' argument for more details.
 * - visibility: The visibility of this virtual fact statistic.
 * - title: Title of this virtual fact.
 * - meta: Metadata for this statistic.
 * - meter_type: This is the type of fact (signed or unsigned, 64 or 32 bit).
 *   <TABLE>
 *    <TR><TD>meter_type </TD>       <TD> function type expected (parameter five)</TD></TR>
 *    <TR><TD>METER_64   </TD>       <TD> ps_i64_t (*returnFcnPtrType)( void* );</TD></TR>
 *    <TR><TD>METER_32   </TD>       <TD> ps_i32_t (*returnFcnPtrType)( void* );</TD></TR>
 *    <TR><TD>U_METER_64 </TD>       <TD> ps_ui64_t (*returnFcnPtrType)( void* );</TD></TR>
 *    <TR><TD>U_METER_32 </TD>       <TD> ps_ui32_t (*returnFcnPtrType)( void* );</TD></TR>
 *   </TABLE>
 *
 * Return values:
 * - STATS_SUCCESS            on success
 * - STATS_NULL_NAME          if name argument is NULL
 * - STATS_NULL_SUMMARY       if summary argument is NULL
 * - STATS_NULL_TITLE         if title argument is NULL
 * - STATS_NULL_RETURNFCNPTR  if returnFcnPtr argument is NULL
 * - STATS_BAD_VISIBILITY     if an invalid visibility is specified
 * - STATS_NULL_PARENT        if parent argument is NULL
 * - STATS_BAD_ARGUMENT       if parent is not pointing to a valid family 
 *                            created using a call to createFamily
 * - STATS_INVALID_METER_TYPE if invalid meter_type is specified
 *
 * The caller of this function is responsible for freeing the memory allocated
 * for the virtual fact by calling deleteFact. For creation of native fact and
 * reference fact check 'createFact' and 'createReferenceFact' respectively.
 */

int createVirtualFact(FACT **fact, FAMILY *parent, const char *name,
                        const char *summary, void *returnFcnPtr,
                        VISIBILITY visibility, const char *title, META *meta,
                        STATS_METER_TYPE meter_type);

//! Creates a virtual nonnmeric fact statistic
/** 'createVirtualNonnumericFact' allows a developer to create a virtual nonnmeric fact statistic
 *
 * Arguments:
 * - fact: This is an out argument If this function returns STATS_SUCCESS,
 *         this agrument will point to the reference nonnmeric fact created by this
 *         function.
 * - parent: Pointer to the parent family. The virtual nonnmeric fact stat will be
 *           created as a child under this family. This agrument should
 *           not be NULL.
 * - name: This is the name of virtual nonnmeric fact being created.
 * - summary: Short description about this virtual nonnmeric fact statistic.
 * - returnFcnPtr: This is a pointer to the nonnmeric fact function that this statistic 
 *   will use. The function return type should change based on the stats meter type 
 *   (the last argument).  The function pointer parameter needs to be cast as a
 *   (void *). Please refer to the 'meter_type' argument for more details.
 * - visibility: The visibility of this virtual nonnmeric fact statistic.
 * - title: Title of this virtual nonnmeric fact.
 * - len: The width of meta
 *
 * Return values:
 * - STATS_SUCCESS            on success
 * - STATS_NULL_NAME          if name argument is NULL
 * - STATS_NULL_SUMMARY       if summary argument is NULL
 * - STATS_NULL_TITLE         if title argument is NULL
 * - STATS_NULL_RETURNFCNPTR  if returnFcnPtr argument is NULL
 * - STATS_BAD_VISIBILITY     if an invalid visibility is specified
 * - STATS_NULL_PARENT        if parent argument is NULL
 * - STATS_BAD_ARGUMENT       if parent is not pointing to a valid family 
 *                            created using a call to createFamily
 * - STATS_INVALID_METER_TYPE if invalid meter_type is specified
 *
 * The caller of this function is responsible for freeing the memory allocated
 * for the virtual nonnmeric fact by calling deleteFact. For creation of native nonnmeric fact and
 * reference nonnmeric fact check 'createFact' and 'createReferenceFact' respectively.
 */
int createVirtualNonnumericFact(FACT **fact, FAMILY *parent, const char *name,
                     const char *summary, void* returnFcnPtr,
                     VISIBILITY visibility, const char *title, int len);

//! Deletes a Fact statistic created using createFact
/** 'deleteFact' allows a developer to delete any flavor of Fact stat which is 
 *  created using one of 'createFact', 'createReferenceFact',
 *  'createVirtualFact', 'createReferenceFact32' or 'createVirtualFact32' 
 *  methods.
 *
 * Arguments:
 * - factStat: This pointer pointed to the Fact Statistic to be deleted.
 *
 * Return values:
 * - STATS_SUCCESS        on success
 * - STATS_BAD_ARGUMENT   if the arguement is not pointing to a valid Fact pointer created
 *                        byusing a call to create*Fact
 *
 */
int deleteFact(FACT *factStat);


//! Increments native fact stat by one
/** 'incFact' allows a developer to increment a native fact
 * stats by one 
 *
 * Arguments:
 * - fact: Pointer to a native fact created using a call to
 *         createFact. If this function returns STATS_SUCCESS,
 *        the couter value will be incremented by one. This should
 *        should not be NULL.
 *
 * Return values:
 * - STATS_SUCCESS      on success
 * - STATS_BAD_ARGUMENT if fact is not pointing to a valid native fact
 *                      created using a call to createFact.
 * - STATS_NULL_FACT    if fact agrument is NULL.
 */
int incFact(FACT *fact);

//! Increments native fact stat by one and garuntees atomicity
/** 'atomicIncFact' allows a developer to increment a native fact
 * stats by one and this function garuntees atomicity while incrementing
 * the value of the Fact.
 *
 * Arguments:
 * - fact: Pointer to a native fact created using a call to
 *         createFact. If this function returns STATS_SUCCESS,
 *        the couter value will be incremented by one. This should
 *        should not be NULL.
 *
 * Return values:
 * - STATS_SUCCESS      on success
 * - STATS_BAD_ARGUMENT if fact is not pointing to a valid native fact
 *                      created using a call to createFact.
 * - STATS_NULL_FACT    if fact agrument is NULL.
 */
int atomicIncFact(FACT *fact);

//! Decrements native fact stat by one
/** 'decFact' allows a developer to decrement a native fact
 * stats by one 
 *
 * Arguments:
 * - fact: Pointer to a native fact created using a call to
 *         createFact. If this function returns STATS_SUCCESS,
 *        the couter value will be incremented by one. This should
 *        should not be NULL.
 *
 * Return values:
 * - STATS_SUCCESS      on success
 * - STATS_BAD_ARGUMENT if fact is not pointing to a valid native fact
 *                      created using a call to createFact.
 * - STATS_NULL_FACT    if fact agrument is NULL.
 */
int decFact(FACT *fact);

//! Decrements native fact stat by one and garuntees atomicity
/** 'atomicDecFact' allows a developer to decrement a native fact
 * stats by one and this function garuntees atomicity while decrementing
 * the value of the Fact.
 *
 * Arguments:
 * - fact: Pointer to a native fact created using a call to
 *         createFact. If this function returns STATS_SUCCESS,
 *        the couter value will be incremented by one. This should
 *        should not be NULL.
 *
 * Return values:
 * - STATS_SUCCESS      on success
 * - STATS_BAD_ARGUMENT if fact is not pointing to a valid native fact
 *                      created using a call to createFact.
 * - STATS_NULL_FACT    if fact agrument is NULL.
 */
int atomicDecFact(FACT *fact);



//! Increments native fact stat by the specified value
/** 'addFact' allows a developer to increment a native fact
 * stats by value specified as add argument 
 *
 * Arguments:
 * - fact: Pointer to a native fact created using a call to
 *         createFact. If this function returns STATS_SUCCESS,
 *         the couter value will be incremented by the value specified by
 *         add argument. This should not be NULL.
 * - add: The value this native fact will be increment by
 *
 * Return values:
 * - STATS_SUCCESS      on succes
 * - STATS_BAD_ARGUMENT if fact is not pointing to a valid native fact
 *                      created using a call to createFact.
 * - STATS_NULL_FACT    if fact agrument is NULL.
 */
int addFact(FACT *fact, ps_ui64_t add);

//! Increments native fact stat by the specified value
/** 'atomicAddFact' allows a developer to increment a native fact
 * stats by value specified as add argument  and garuntees atomocity of the
 * operation
 *
 * Arguments:
 * - fact: Pointer to a native fact created using a call to
 *         createFact. If this function returns STATS_SUCCESS,
 *         the couter value will be incremented by the value specified by
 *         add argument. This should not be NULL.
 * - add: The value this native fact will be increment by
 *
 * Return values:
 * - STATS_SUCCESS      on succes
 * - STATS_BAD_ARGUMENT if fact is not pointing to a valid native fact
 *                      created using a call to createFact.
 * - STATS_NULL_FACT    if fact agrument is NULL.
 */
int atomicAddFact(FACT *fact, ps_ui64_t add);

//! Subtract native fact stat by the specified value
/** 'subFact' allows a developer to increment a native fact
 * stats by value specified as sub argument 
 *
 * Arguments:
 * - fact: Pointer to a native fact created using a call to
 *         createFact. If this function returns STATS_SUCCESS,
 *         the couter value will be subtracted by the value specified by
 *         sub argument. This should not be NULL.
 * - sub: The value this native fact will be subtracted by
 *
 * Return values:
 * - STATS_SUCCESS      on succes
 * - STATS_BAD_ARGUMENT if fact is not pointing to a valid native fact
 *                      created using a call to createFact.
 * - STATS_NULL_FACT    if fact agrument is NULL.
 */
int subFact(FACT *fact, ps_ui64_t sub);

//! Subtract native fact stat by the specified value
/** 'atomicSubFact' allows a developer to increment a native fact
 * stats by value specified as sub argument and garuntees atomocity of the
 * operation
 *
 * Arguments:
 * - fact: Pointer to a native fact created using a call to
 *         createFact. If this function returns STATS_SUCCESS,
 *         the couter value will be subtracted by the value specified by
 *         sub argument. This should not be NULL.
 * - sub: The value this native fact will be subtracted by
 *
 * Return values:
 * - STATS_SUCCESS      on succes
 * - STATS_BAD_ARGUMENT if fact is not pointing to a valid native fact
 *                      created using a call to createFact.
 * - STATS_NULL_FACT    if fact agrument is NULL.
 */
int atomicSubFact(FACT *fact, ps_ui64_t sub);


/*
//  Computed and Computed2
*/ 

//! Creates a Computed2 statistic
/** 'createComputed2' allows a developer to create a computed2 statistic
 *
 * Arguments:
 * - computed2: This is an out argument If this function returns STATS_SUCCESS, 
 *   this argument will point to the Computed2 statistic created by this function.
 * - parent: Pointer to the parent family. The Computed2 statistic will be created 
 *   as a child under this family. This argument should not be NULL.
 * - name: the short name this statistic will have in the stats namespace.
 * - summary: description of this stat.
 * - op1: Statpath name for the first operand
 * - op2: Statpath name for the second operand
 * - optype: Operation that is to be applied to above two statpaths
 * - visibility: The visibility of this stat.
 * - title: Title of this Computed2 stat
 * - meta: Pointer to meta returned by createMeta function 
 * - behavior: How will this Computed2 behave counter, fact or error-counter
 *
 * Return values:
 * - STATS_SUCCESS on success
 * - STATS_NULL_PARENT if parent argument is NULL
 * - STATS_NULL_NAME if name argument is NULL
 * - STATS_NULL_SUMMARY if summary argument is NULL
 * - STATS_NULL_OPERAND if either op1 or op2 is NULL
 * - STATS_BAD_OPERATOR if an invalid operator is specified
 * - STATS_NULL_TITLE if title argument is NULL
 * - STATS_NULL_META if meta argument is NULL
 * - STATS_BAD_VISIBILITY if an invalid visibility is specified
 * - STATS_BAD_ARGUMENT if parent is not pointing to a valid family created using a call to 
 *   createFamily or stype is not to a valid STYPE created using a call to createSType or if meta is not 
 *   pointing to a valid META returned by createMeta
 * - STATS_BAD_BEHAVIOR if an invalid behavior is specified
 *
 * The caller of this function is responsible for freeing the memory allocated 
 * for the Computed2 statistic by calling deleteComputed.
 */
                        
int createComputed2(COMPUTED2 **computed2, FAMILY *parent, const char *name,
                    const char *summary, const char *op1, const char *op2,
                    COMPUTED2OPTYPE optype, VISIBILITY visibility,
                    const char *title, META *meta, BEHAVIOR behavior);

//! Creates a Computed statistic
/** 'createComputed' allows a developer to create a computed statistic
 *
 * Arguments:
 * - computed:      This is an out argument If this function returns STATS_SUCCESS, this
 *                  argument will point to the Computed statistic created by this function.
 *                  It should be the pointer to another variable.
 * - parent:        Pointer to the parent family. The Computed statistic will be created
 *                  as a child under this family. This argument should not be NULL.
 * - name:          the short name this statistic will have in the stats namespace.
 * - summary:       description of this stat.
 * - statpathlist:  char array holding the statpath names as operands
 * - count:         Number of statpaths in statpathlist
 * - optype:        Operation that is to be applied to above two or more statpaths
 * - visibility:    The visibility of this stat.
 * - title:         Title of this Computed stat
 * - meta:          Pointer to meta returned by createMeta function 
 * - behavior:      How will this Computed behave counter, fact or error-counter
 *
 * Return values:
 * - STATS_SUCCESS           on success
 * - STATS_BAD_ARGUMENT      if computed argument is NULL, or parent argument is not
 *                           pointing to a valid family created by createFamily
 * - STATS_NULL_PARENT       if parent argument is NULL
 * - STATS_NULL_NAME         if name argument is NULL
 * - STATS_NULL_SUMMARY      if summary argument is NULL
 * - STATS_NULL_STATPATHLIST if statpathlist argument is NULL
 * - STATS_INVALID_COUNT     if count is 0 or out of range
 * - STATS_OP_NOT_ALLOWED    if an invalid operator is specified
 * - STATS_BAD_VISIBILITY    if an invalid visibility is specified
 * - STATS_NULL_TITLE        if title argument is NULL
 * - STATS_NULL_META         if meta argument is NULL
 * - STATS_BAD_BEHAVIOR      if an invalid behavior is specified
 *
 * The caller of this function is responsible for freeing the memory allocated 
 * for the Computed statistic by calling deleteComputed.
 */
                        
int createComputed(COMPUTED **computed, FAMILY *parent, const char *name,
                    const char *summary, const char **statpathlist,
                    const unsigned int count, COMPUTEDOPTYPE optype,
                    VISIBILITY visibility,const char *title, 
                    META *meta, BEHAVIOR behavior);
                    
//! Deletes a computed statistic created using using createComputed function.
/** 'deleteComputed ' allows a developer to delete a Computed stat which is 
 *  created using 'createComputed'  methods
 *
 * Arguments:
 * - factStat: This pointer pointed to the Computed Statistic to be deleted.
 *
 * Return values:
 * - STATS_SUCCESS        on success
 * - STATS_BAD_ARGUMENT   if the arguement is not pointing to a valid Computed pointer
 *                        created by using a call to createComputed
 */
int deleteComputed (COMPUTED *computed);

/*
//  STypes
*/ 

//! Creates an SType
/** 'createSType' allows a developer to create a SType node which can be used as parent
 *  while creating fields in a Compound and Set stat.
 *
 * Arguments:
 * - stype: This is an out argument If this function returns STATS_SUCCESS, this argument 
 *   will point to the STYPE statistic created by this function.
 * - name:   This is the name of STYPE being created.
 * - size: This is the size of the structure on which this STYPE is based. 
 *   This structure will hold the Compound or Set statistics
 * - fields: This is an array containing definition of fields of STYPE
 * - numberOfField: This is the number of fields
 *
 * Return values:
 * - STATS_SUCCESS        on success
 * - STATS_NULL_NAME      if name argument is NULL
 *
 * The caller of this function is responsible for freeing the memory allocated
 * for the STYPE by calling deleteSType.
 */
int createSType(STYPE **stype, const char *name, size_t size,
                StatField fields[], size_t numberOfField);

//! Deletes anSType
/** 'deleteSType' allows a developer to a developer to delete a SType 
 *  which is created using createSType function.
 *
 * Arguments:
 * - pStype: Pointer to STYPE that is to be deleted.
 *
 * Return values:
 * - STATS_SUCCESS        on success
 * - STATS_BAD_ARGUMENT   if the arguement is not pointing to a valid STYPE pointer created
 *                        byusing a call to createSType
 * - STATS_OP_NOT_ALLOWED if the operation is not permitted.
 *
 */
int deleteSType (STYPE *stype);


/**
 * Helper function to be used in fetch functions.
 * Instead of calling it directly, user just uses macro STAT_ATOMIC_FETCH_C_DEF_START and 
 * STAT_ATOMIC_FETCH_C_DEF_END to define the fetch functions.
 */
void getSourceAndResultAddr(void *ri, void **source, void **result);

//! Creates an Atomic SType
/** 'createAtomicSType' allows a developer to create an Atomic SType node which can be used 
 *   as parent while creating fields in a Compound and Set stat.
 *
 * Arguments:
 * - stype: This is an out argument If this function returns STATS_SUCCESS, this argument 
 *   will point to the STYPE statistic created by this function.
 * - name:   This is the name of STYPE being created.
 * - size: This is the size of the structure on which this STYPE is based. 
 *   This structure will hold the Compound or Set statistics
 * - fields: This is an array containing definition of fields of STYPE
 * - numberOfField: This is the number of fields
 * - fetchFuncPtr: This is the pointer to user defined function by which the statistics result is 
 *   fetched and filled into the destination structure.
 *
 * Return values:
 * - STATS_SUCCESS        on success
 * - STATS_NULL_NAME      if name argument is NULL
 *
 * The caller of this function is responsible for freeing the memory allocated
 * for the STYPE by calling deleteSType.
 */
int createAtomicSType(STYPE **stype, const char *name, size_t size, 
                      StatField fields[], size_t numberOfField,
                      CFetchFcnPtr fetchFuncPtr);

/*
// Compound and CompoundR
*/ 

//! Creates a CompoundR stat
/** 'createCompoundR' allows a developer to create a CompoundR stat
 *
 * Arguments:
 * - compoundR: This is an out argument If this function returns
 *              STATS_SUCCESS, this agrument will point to the CompoundR stat
 *              created by this function.
 * - parent: Pointer to the parnet family. The CompoundR stat will
 *           be created as a child under this family. This agrument should
 *           not be NULL.
 * - name: This is the name of CompoundR stat being created
 * - summary: Short description about this CompoundR stat
 * - addr: Address of the structure holding stats
 * - visibility: The visibility of this CompoundR stat
 * - title: Title of this CompoundR stat
 * - stype: Pointer to the STYPE node created using createSType function. This
 *          not be NULL
 *
 * Return values:
 * - STATS_SUCCESS      on success
 * - STATS_NULL_PARENT    if parent argument is NULL
 * - STATS_NULL_NAME      if name argument is NULL
 * - STATS_NULL_SUMMARY   if summary argument is NULL
 * - STATS_NULL_REFERENCE if addr argument is NULL
 * - STATS_NULL_TITLE     if title argument is NULL
 * - STATS_NULL_STYPE   if stype argument is NULL
 * - STATS_BAD_VISIBILITY if an invalid visibility is specified
 * - STATS_BAD_ARGUMENT if parent is not pointing to a valid family created
 *                      using a call to createFamily or stype is not
 *                      to a valid STYPE created using a call to createSType
 *
 * The caller of this function is responsible for freeing the memory allocated
 * for the CompoundR stat by calling deleteStat.
 */
int createCompoundR(COMPOUND **compoundR, FAMILY *parent, const char *name,
                    const char *summary, void * addr, VISIBILITY visibility,
                    const char *title, STYPE *stype);

//! Deletes compoundR Statistic
/** 'deleteCompoundR' allows a developer to a developer to delete a compoundR 
 *  which is created using createCompoundR function.
 *
 * Arguments:
 * - pStype: Pointer to STYPE that is to be deleted.
 *
 * Return values:
 * - STATS_SUCCESS        on success
 * - STATS_BAD_ARGUMENT   if the arguement is not pointing to a valid STYPE pointer created
 *                        byusing a call to createSType
 * - STATS_OP_NOT_ALLOWED if the operation is not permitted.
 *
 */
int deleteCompoundR (COMPOUND* compoundRStat);

//! create a Fact or Counter field stat
/** 'createFieldStat' allows a developer to create a Fact or Counter field
 * stat under a CompoundR stat.
 *
 * Arguments:
 * - field: This is an out argument If this function returns
 *          STATS_SUCCESS, this agrument will point to the Fact or Counter stat
 *          created by this function.
 * - parent: Pointer to the parnet STYPE node. The Fact or Counter field stat
 *           will be created as a child under this STYPE node. This agrument
 *           should not be NULL
 * - name: This is the name of Fact or Counter field stat being created
 * - summary: Short description about this Fact or Counter field stat
 * - offset: The offset of this field in the Structure containing stats
 * - kind: The type of field FACT_FIELD or COUNTER_FIELD
 * - meta: Pointer to meta returned by createMeta function 
 *
 * Return values:
 * - STATS_SUCCESS        on success
 * - STATS_NULL_NAME      if name argument is NULL
 * - STATS_NULL_SUMMARY   if summary argument is NULL
 * - STATS_NULL_TITLE     if title argument is NULL
 * - STATS_NULL_PARENT    if parent argument is NULL
 * - STATS_BAD_VISIBILITY if an invalid visibility is specified
 * - STATS_BAD_ARGUMENT   if parent is not pointing to a valid STYPE created
 *                        using a call to createSType
 *
 * The caller of this function is responsible for freeing the memory allocated
 * for the Fact or Counter field stat by calling deleteStat. Field stats should
 * be deleted before deleting their parent STYPE. Since this function takes
 * offset of the corresponding field in the underlying stat structure as an
 * argument, develover are encouraged to use the macros STAT_COUNTER_FIELD_DEF
 * and STAT_FACT_FIELD_DEF to avoid errors while calculating offset.
 */
int createFieldStat(FIELD **field, STYPE *parent, const char *name,
                    const char *summary, VISIBILITY visibility,
                    const char *title, long offset, FIELD_KIND kind,
                    META* meta);

//! create a computed2 filed Stat
/** 'createComputed2FieldStat' allows a developer to create a Fact or Counter field
 * stat under a CompoundR stat.
 *
 * Arguments:
 * - field: This is an out argument If this function returns
 *          STATS_SUCCESS, this agrument will point to the Fact or Counter stat
 *          created by this function.
 * - parent: Pointer to the parnet STYPE node. 
 * - name: This is the name of computed2 field stat being created
 * - summary: Short description about this Fact or Counter field stat
 * - op1: Statpath name for the first operand
 * - op2: Statpath name for the second operand
 * - optype: Operation that is to be applied to above two statpaths
 * - visibility: The visibility of this stat.
 * - title: Title of this Computed2 stat
 * - meta: Pointer to meta returned by createMeta function 
 * - bahaviour: Should this type be a FACT_FIELD or COUNTER_FIELD
 *
 * Return values:
 * - STATS_SUCCESS        on success
 * - STATS_NULL_NAME      if name argument is NULL
 * - STATS_NULL_SUMMARY   if summary argument is NULL
 * - STATS_NULL_TITLE     if title argument is NULL
 * - STATS_NULL_PARENT    if parent argument is NULL
 * - STATS_BAD_VISIBILITY if an invalid visibility is specified
 * - STATS_BAD_ARGUMENT   if parent is not pointing to a valid STYPE created
 *                        using a call to createSType
 *
 * The caller of this function is responsible for freeing the memory allocated
 * for the computed2  field stat by calling deleteComputed2FieldStat. Field stats should
 * be deleted before deleting their parent STYPE. 
 */
int createComputed2FieldStat(COMPUTED2FIELD **field, STYPE *parent, const char *name,
                    const char *summary, const char * op1, const char *op2, COMPUTED2OPTYPE optype, 
                    VISIBILITY visibility, const char *title, META *meta, BEHAVIOR behavior);

//! Creates a Computed statistic
/** 'createComputedFieldStat' allows a developer to create a computed statistic
 *
 * Arguments:
 * - computed: This is an out argument If this function returns STATS_SUCCESS, 
 *   this argument will point to the Computed field statistic created by this function.
 * - parent: Pointer to the stype. The Computed statistic will be created 
 *   as a child under this family. This argument should not be NULL.
 * - name: the short name this statistic will have in the stats namespace.
 * - summary: description of this stat.
 * - optype: Operation that is to be applied to above two statpaths
 * - visibility: The visibility of this stat.
 * - title: Title of this Computed stat
 * - meta: Pointer to meta returned by createMeta function 
 * - behavior: How will this Computed behave counter, fact or error-counter
 *
 * Return values:
 * - STATS_SUCCESS on success
 * - STATS_NULL_PARENT if parent argument is NULL
 * - STATS_NULL_NAME if name argument is NULL
 * - STATS_NULL_SUMMARY if summary argument is NULL
 * - STATS_NULL_OPERAND if either op1 or op2 is NULL
 * - STATS_BAD_OPERATOR if an invalid operator is specified
 * - STATS_NULL_TITLE if title argument is NULL
 * - STATS_NULL_META if meta argument is NULL
 * - STATS_BAD_VISIBILITY if an invalid visibility is specified
 * - STATS_BAD_ARGUMENT if parent is not pointing to a valid family created using a call to 
 *   createFamily or stype is not to a valid STYPE created using a call to createSType or if meta is not 
 *   pointing to a valid META returned by createMeta
 * - STATS_BAD_BEHAVIOR if an invalid behavior is specified
 *
 * The caller of this function is responsible for freeing the memory allocated 
 * for the Computed2 statistic by calling deleteComputedFieldStat.Field stats should
 * be deleted before deleting their parent STYPE. 
 */
int createComputedFieldStat(COMPUTEDFIELD **computed, STYPE *parent, const char *name,
                    const char *summary, const char **statpathlist,
                    const unsigned int count, COMPUTEDOPTYPE optype,
                    VISIBILITY visibility,const char *title, 
                    META *meta, BEHAVIOR behavior);

//! Deletes FIELD  Statistic
/** 'deleteFieldStat' deleteFieldStat allows a developer to delete a 
 * FIELD statistic which is created using createField function..
 *
 * Arguments:
 * - fieldStat: Pointer to FIELD  stat that is to be deleted.
 *
 * Return values:
 * - STATS_SUCCESS        on success
 * - STATS_BAD_ARGUMENT   if the arguement is not pointing to a valid STYPE pointer created
 *                        byusing a call to createFieldStat
 * - STATS_OP_NOT_ALLOWED if the operation is not permitted.
 *
 */
int deleteFieldStat (FIELD* fieldStat);

//! Deletes COMPUTED2FIELD  Statistic
/** 'deleteComputed2FieldStat' deleteFieldStat allows a developer to delete a 
 * FIELD statistic which is created using createComputed2Field function..
 *
 * Arguments:
 * - fieldStat: Pointer to COMPUTED2FIELD  stat that is to be deleted.
 *
 * Return values:
 * - STATS_SUCCESS        on success
 * - STATS_BAD_ARGUMENT   if the arguement is not pointing to a valid STYPE pointer created
 *                        byusing a call to createFieldStat
 * - STATS_OP_NOT_ALLOWED if the operation is not permitted.
 *
 */
int deleteComputed2FieldStat (COMPUTED2FIELD* fieldStat);


//! Deletes COMPUTEDFIELD  Statistic
/** 'deleteComputedFieldStat' deleteFieldStat allows a developer to delete a 
 * COMPUTEDFIELD statistic which is created using createComputedField function..
 *
 * Arguments:
 * - fieldStat: Pointer to COMPUTEDFIELD  stat that is to be deleted.
 *
 * Return values:
 * - STATS_SUCCESS        on success
 * - STATS_BAD_ARGUMENT   if the arguement is not pointing to a valid STYPE pointer created
 *                        byusing a call to createFieldStat
 * - STATS_OP_NOT_ALLOWED if the operation is not permitted.
 *
 */
int deleteComputedFieldStat (COMPUTEDFIELD* fieldStat);

//! Creates a Set stat
/** 'createSet' allows a developer to create a Set stat
 *
 * Arguments:
 * - set: This is an out argument If this function returns STATS_SUCCESS, this
 *        agrument will point to the Set stat created by this function.
 * - parent: Pointer to the parnet family. The Set stat will be created as a
 *           child under this family. This agrument should not be NULL.
 * - name: This is the name of Set stat being created
 * - summary: Short description about this Set stat
 * - visibility: The visibility of this Set stat
 * - title: Title of this Set stat
 * - width: The display width for the elements of this Set stat
 * - enumeratefcnptr: Pointer to a function which will enumerate through the
 *                    elements of this Set stat
 * - elementSummary: Summary for the elements of this Set stat
 * - stype: Pointer to SType created around the structure containing statistics
 * - resolverHint: Hint for the client application to resolve element names
 *
 * Return values:
 * - STATS_SUCCESS              on success
 * - STATS_NULL_PARENT          if parent argument is NULL
 * - STATS_NULL_NAME            if name argument is NULL
 * - STATS_NULL_SUMMARY         if summary argument is NULL
 * - STATS_NULL_TITLE           if title argument is NULL
 * - STATS_NULL_ELEMENT_SUMMARY if element summary argument is NULL
 * - STATS_NULL_STYPE           if stype argument is NULL
 * - STATS_BAD_ARGUMENT         if parent is not pointing to a valid family created
 *                              using a call to createFamily or stype is not
 *                              to a valid STYPE created using a call to createSType
 * - STATS_BAD_VISIBILITY       if an invalid visibility is specified
 *
 * The caller of this function is responsible for freeing the memory allocated
 * for the Set stat by calling deleteStat. Developers are responsible for
 * implementing enumerate function which will enumerate through the elements
 * of Set stat.
 */

int createSet(SET **set, FAMILY *parent, const char *name, const char* summary,
              const VISIBILITY visibility, const char *title, const short width,
              EnumerateFcnPtr enumeratefcnptr, const char *elementSummary,
              STYPE *stype, RESOLVER_HINT resolverHint);

//! Deletes a Set  Statistic
/** 'deleteSet'  allows a developer to delete a 
 * SET statistic which is created using createSet function.
 *
 * Arguments:
 * - fieldStat: Pointer to SET  stat that is to be deleted.
 * * Return values:
 * - STATS_SUCCESS        on success
 * - STATS_BAD_ARGUMENT   if the arguement is not pointing to a valid SET pointer created
 *                        byusing a call to createSet
 * - STATS_OP_NOT_ALLOWED if the operation is not permitted.
 *
 */
int deleteSet (SET *set);

//! Enumerate through the elements of Set
/** 'enumerateAndCallFunction'  allows a developer to delete a 
 * * Return values:
 * - STATS_SUCCESS        on success
 * - Non Zero value on Failure
 */
int enumerateAndCallFunction(void * me, char *name, void *object);


//! Creates a Meta Object
/** 'createMeta' allows a developer to create a meta data object which can be passed to create* functions
 *
 * Arguments:
 * - unit: measurement unit string to describe statistic value
 * - kscale: Scaling factor to be applied to the meter value at presentation time
 * - width: Minimum column width, specific to the CLI, for displaying the meter value
 * - wrap: defines how to treat Counter wraparounds
 *
 * Return values:
 * - Pointer to META created by the function              on success
 * - If any of the argument is NULL or invalid, 
 *     createMeta will create a META with default values of unit (""), 
 *   kscale (STATS_KSCALE_UNDEF), width (-1), and wrap (STATS_WRAP_UNK)
 */

META * createMeta(const char * unit, STATS_KSCALE kscale, short width, STATS_WRAP wrap);
              
//! Deletes an META object 
/** 'deleteMeta'  allows a developer to delete a 
 * SET statistic which is created using createSet function.
 *
 * Arguments:
 * - meta: Pointer to META  object that is to be deleted.
 *
 * Return values:
 * - STATS_SUCCESS        on success
 * - STATS_BAD_ARGUMENT   if the arguement is not pointing to a valid SET pointer created
 *                        byusing a call to createSet
 * - STATS_OP_NOT_ALLOWED if the operation is not permitted.
 *
 */
int deleteMeta (META *meta);
              
//! Returns the string after till first . in the input string
/** 'popNameDot' allows a developer to retrieve string till first occurance of
 * . character in the input string
 *
 * Arguments:
 * - inbuf: The input string. This function will modify the input string as
 *          explained in the notes below. This should not be NULL
 * - outbuf: The out string containing characters from begining till first
 *           occurance of . in input string
 * - size: Size of outbuf argument
 *
 * Return values:
 * - 0        on success or if either inbuf or outbuf is NULL.
 * - size     size of outbuf if the buffer supplied is not large enough to
 *            hold output string
 
 *
 * The caller of this function is responsible for allocating memory for outbuf.
 * If either inbuf or outbuf is NULL this functions will perform no operation
 * and return 0 (0 is also returned in case of success)
 * Here are few examples to understand what this function does with inbuf and
 * oubuf
 * |--------|---------------------------|-------------------------------|
 * |        |         BEFORE            |          AFTER                |
 * |--------|---------------------------|-------------------------------|
 * | inbuf  |         "abc"             |          ""                   |
 * |--------|---------------------------|-------------------------------|
 * | outbuf |         ""                |          "abc"                |
 * |--------|---------------------------|-------------------------------|
 * | inbuf  |         "abc.xyz"         |          "xyz"                |
 * |--------|---------------------------|-------------------------------|
 * | outbuf |         ""                |          "abc"                |
 * |--------|---------------------------|-------------------------------|
 */
size_t popNameDot(char * inbuf, char * outbuf, size_t size);

//! Enumerate through the elements of Set
/** 'getElementName'  allows a developer to delete a
 * Arguments:
 * - enumParams: This is the same arguement supplied by the call back function
 * - name: This is an in/out parameter. If there is an elemet or field in the
 *         statpath, the same is filled into this.
 * - length: The max length of the input "name".
 * - level: the number of element name that skips.
 * * Return values:
 * - Length of the string, if statpath has elemnt or field name
 * - 0, if statpath does not contain elemnt or field name
 */
int getElementName(void * enumParams,char *name,const unsigned int length, const unsigned int level);

//! Check whether it is already inside the enumeration context of a Set
/** 'isCallFromEnum' used in nested Set situlation to avoid recursive lock
 *  Return values:
 * - 0: if it is not inside the enumeration context
 * - others: if it is inside the enumeration context
 */
int isCallFromEnum(void* me);

//! Get root families by family name, such as "nfs", "net"
FAMILY* getRootFamily(const char* rootFamilyName);

#if defined(__cplusplus)
}
#endif

#endif //end of #ifndef __STATS_C_API_H_
