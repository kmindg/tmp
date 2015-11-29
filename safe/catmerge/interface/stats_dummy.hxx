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
 *  Stats Infrastructure
 *
 *  ABSTRACT:
 *  Dummy stats Infrastructure related definitions for C++ for Windows
 *
 *  AUTHORS:
 *  Observability Team
 *  
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
#ifndef __STATS_HXX
#define __STATS_HXX

#include <stdio.h>
#define STD Stat

namespace Stat
{
#define ET_MIN_TOP_LEN       1
#define ET_MAX_TOP_LEN     256
#define ET_DEFAULT_TOP_LEN  20
class string
{
public:
      // Default Constructor
      string (){};

      // Create a string from the specified null
      // terminated character array. A null string
      // is created if the specified character array
      // is either NULL or the first character is '\0'.
      string (const char *pStr){};

      // Create a string from the specified character array and length.
      // A null string is created if the length is 0.
      // NOTE: No check for NULL characters in the middle of the input string
      string (const char *pStr, size_t len){};

      // Copy constructor. Create a string by
      // copying the specified string.
      string (const string& str){};

      ~string (){};

};

class ElementHint
{
public:
    enum {
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
    };
};

class ParentType
{
public:
    enum Enum {Family = 0, Set = 1, ESet = 2};
};

enum {standardRI = 0, dummyRI = 1, ignoredDummyRI = 2};

class Computed2Operator
{
public:
    enum Enum { div, mult, add, sub, pct,unk };

    static STD::string getOpStr(Enum e) {
        switch ( e ) {
        case div:
            return "div";
            break;
        case mult:
            return "mult";
            break;
        case add:
            return "add";
            break;
        case sub:
            return "sub";
            break;
        case pct:
            return "pct";
            break;
        default:
            return "unk";
        }
    }
};

class ComputedOperator
{
public:
    enum Enum { sum, pdct, avg ,unk};

    static STD::string getOpStr(Enum e) {
        switch ( e ) {
        case sum:
            return "sum";
            break;
        case pdct:
            return "pdct";
            break;
        case avg:
            return "avg";
            break;
        default:
            return "unk";
        }
    }
};

class Kind
{
public:
    enum Enum { family, counter, fact, nonNumericFact, constant, compound, set, implicitSet, eset, cset,type, computed, unk, _last};
    static bool isScalar( Enum e) {
        return (e >= counter) && (e <= constant);
    }
};

class computed2Behavior
{
public:
    enum Enum { counter, fact, errorCounter ,unk};
};

class computedBehavior
{
public:
    enum Enum { counter, fact, errorCounter ,unk};
};

class SortOrder
{
public:
    enum Enum { asc, desc};
};

class TopParams
{
public:
    short           lines;
    SortOrder::Enum order;
    int             key;
    TopParams() : lines(ET_DEFAULT_TOP_LEN), order(SortOrder::desc), key(1) {}
};

class SessionKind
{
public:
    enum Enum { list, info, rest};
};

class Version
{
public:
    enum Enum { oldest=5, v5=5, v6=6, v7=7, latest=7};
};


class Visibility
{
public:
    enum Enum { unused, deflt, productTree, product, supportTree, support, engTree, eng, internal, _last };
};

class KScale
{
public:
    enum Enum { u,     k,    m,   g,  ui, ki, mi, gi, undefined };
};


class Wrap
{
public:
    enum Enum { unk, w16, w32, w64, w32or64, e64  };
};

class Meta
{
public:
    Meta( const short colWidth )
        : units( "" ), kscale(KScale::undefined), colWidth(colWidth),
          wrap(Wrap::unk) { }

    Meta( const Wrap::Enum wrap )
        : units( "" ), kscale(KScale::undefined), colWidth(-1), wrap(wrap) {}

    Meta( const STD::string& units,
          const KScale::Enum kscale,
          const short colWidth = -1,
          const Wrap::Enum wrap = Wrap::unk )
        : units(units), kscale(kscale), colWidth(colWidth), wrap(wrap) {}

    Meta( const KScale::Enum kscale,
          const short colWidth = -1,
          const Wrap::Enum wrap = Wrap::unk )
        : units( "" ), kscale(kscale), colWidth(colWidth), wrap(wrap) {}

    Meta()
        : units( "" ), kscale(KScale::undefined), colWidth(-1), wrap(Wrap::unk) {}

    STD::string units;   //unit of meter i.e. days, min, bytes, user etc
    KScale::Enum kscale;
    short colWidth;
    Wrap::Enum wrap;     //wrap for meter value to decide its range
};

class ReqInstance
{
public:
};

class AtomicReqInstanceBase : public ReqInstance
{
public:
};

class Addr
{
public:
};
class SetLookupData
{
public:
};

class StructMeterBase
{
public:
    Addr registerSelf(SetLookupData&) {
        return Addr();
    }
};

class SType
{
};


class BaseSet
{
public:
    class EnumParams
    {
    public:
        bool enumerateAndCallFunction(const char *, StructMeterBase *) const {
            return true;
        }
    };
};

class Family
{
public:
    Family() {
        printf("Creating Family ...\n");
    }
    Family(Family&, const char *, const char *, int) {
        printf("Creating Family ...\n");
    }
};
extern	Family monitorRoot;

class CtxtBase
{
public:
};
template<class T> class RefCountedPtr {};
typedef RefCountedPtr<CtxtBase> CtxtBasePtr;

typedef Addr (*lookupfunc_t)(STD::string, SetLookupData, STD::string&);
typedef void (*enumfunc_t)(const BaseSet::EnumParams&);
typedef void (* EnumerateFcnPtr)( const BaseSet::EnumParams& params );
typedef bool (* EnableFcnPtr)   ( const BaseSet::EnumParams& params );
typedef void (* DisableFcnPtr)  ( const BaseSet::EnumParams& params );
typedef void (* CleanupFcnPtr)  ( void*, Family*);
typedef Addr (* ObjectLookupFcnPtr)( STD::string name, SetLookupData info, STD::string& errorMsg );
typedef bool (* enumerateAndCallFcnPtr) ( void *, char * , void * );
typedef size_t (* nameFcnPtr) (void *, char *, size_t );
typedef bool (* isCallFromEnumFcnPtr) ( void * );
typedef void (*FetchFcnPtr)( ReqInstance* ri );
typedef AtomicReqInstanceBase* (*NewARIFcnPtr)( Family* const base, CtxtBasePtr& context, const Addr& sourceAddr, FetchFcnPtr fetchFcnPtr );
//typedef long off_t;

extern	Meta fixMetaWrap( Meta meta, size_t size) ;

#define STAT_STRUCT( STAT_TYPE_NAME, METER_STRUCT )\
        STAT_STRUCT_DECL( STAT_TYPE_NAME, METER_STRUCT ) \
        STAT_STRUCT_DEF(  STAT_TYPE_NAME, METER_STRUCT )


#define STAT_STRUCT_DECL( STAT_TYPE_NAME, METER_STRUCT )	typedef int STAT_TYPE_NAME
#define STAT_STRUCT_DEF( STAT_TYPE_NAME, METER_STRUCT )

#define STAT_ATOMIC_STRUCT( STAT_TYPE_NAME, DEST_METER_STRUCT, SRC_METER_STRUCT, ATOMIC_FETCH_FCN ) \
    STAT_ATOMIC_STRUCT_DECL( STAT_TYPE_NAME, DEST_METER_STRUCT, SRC_METER_STRUCT ) \
    STAT_ATOMIC_STRUCT_DEF(  STAT_TYPE_NAME, DEST_METER_STRUCT, ATOMIC_FETCH_FCN )

#define STAT_ATOMIC_STRUCT_DECL( STAT_TYPE_NAME, DEST_METER_STRUCT, SRC_METER_STRUCT ) typedef int STAT_TYPE_NAME
#define STAT_ATOMIC_STRUCT_DEF( STAT_TYPE_NAME, DEST_METER_STRUCT, ATOMIC_FETCH_FCN )

#define STAT_FIELD_DECL( NAME, STAT_TYPE_NAME, FIELD)
#define STAT_FIELD_DEF(NAME, SUMMARY, VIS, TITLE, META, STAT_TYPE_NAME, FIELD, FIELD_TYPE, KIND)
#define STAT_FIELD_DEL( NAME, STAT_TYPE_NAME, FIELD)

#define STAT_FACT_FIELD_DECL( NAME, STAT_TYPE_NAME, FIELD ) STAT_FIELD_DECL( NAME, STAT_TYPE_NAME, FIELD)
#define STAT_FACT_FIELD_DEF( NAME, SUMMARY, VIS, TITLE, META, STAT_TYPE_NAME, FIELD, FIELD_TYPE ) \
  STAT_FIELD_DEF( NAME, SUMMARY, VIS, TITLE, META, STAT_TYPE_NAME, FIELD, FIELD_TYPE, Stat::Kind::fact )

#define STAT_FACT_FIELD_DEL( NAME, STAT_TYPE_NAME, FIELD ) STAT_FIELD_DEL( NAME, STAT_TYPE_NAME, FIELD)

#define STAT_COUNTER_FIELD_DECL( NAME, STAT_TYPE_NAME, FIELD ) STAT_FIELD_DECL( NAME, STAT_TYPE_NAME, FIELD)
#define STAT_COUNTER_FIELD_DEF( NAME, SUMMARY, VIS, TITLE, META, STAT_TYPE_NAME, FIELD, FIELD_TYPE) \
  STAT_FIELD_DEF( NAME, SUMMARY, VIS, TITLE, META, STAT_TYPE_NAME, FIELD, FIELD_TYPE, Stat::Kind::counter )
#define STAT_COUNTER_FIELD_DEL( NAME, STAT_TYPE_NAME, FIELD ) STAT_FIELD_DEL( NAME, STAT_TYPE_NAME, FIELD)

#define STAT_COUNTER_SET_DEF( NAME, SUMMARY, VIS, TITLE, META, STAT_TYPE_NAME, FIELD_TYPE )
#define STAT_FACT_SET_DEF( NAME, SUMMARY, VIS, TITLE, META, STAT_TYPE_NAME, FIELD_TYPE )

#define STAT_COMPUTED2_FIELD_DECL(NAME, STAT_TYPE_NAME, FIELD )
#define STAT_COMPUTED2_FIELD_DEF( NAME, SUMMARY, VIS, TITLE, META, STAT_TYPE_NAME, FIELD, OP1, OP2, OPERATOR, BEHAVIOR)
#define STAT_COMPUTED2_FIELD_DEL( NAME, STAT_TYPE_NAME, FIELD )

#define STAT_COMPUTED_FIELD_DECL( NAME, STAT_TYPE_NAME, FIELD)
#define STAT_COMPUTED_FIELD_DEF( NAME, SUMMARY, VIS, TITLE, META, STAT_TYPE_NAME, FIELD, STATLIST, COUNT, OPERATOR, BEHAVIOR)
#define STAT_COMPUTED_FIELD_DEL(NAME, STAT_TYPE_NAME, FIELD)

#define STAT_SET_FIELD_DECL( NAME, PARENT_STAT_TYPE_NAME, STAT_TYPE_NAME )
#define STAT_SET_FIELD_DEF( NAME, SUMMARY, VIS, TITLE, WIDTH, PARENT_STAT_TYPE_NAME,\
                            STAT_TYPE_NAME, LOOKUP_FUNCTION, ENUMERTE_FUNCTION,     \
                             ELEMENT_SUMMARY)
#define STAT_SET_FIELD_DEL( NAME, PARENT_STAT_TYPE_NAME, STAT_TYPE_NAME )

#define STAT_SET_FIELD_IDENTIFIER_DECL( NAME, PARENT_STAT_TYPE_NAME, STAT_TYPE_NAME )
#define STAT_SET_FIELD_IDENTIFIER_DEF( NAME, SUMMARY, VIS, TITLE, WIDTH, \
 PARENT_STAT_TYPE_NAME, STAT_TYPE_NAME, LOOKUP_FUNCTION, ENUMERTE_FUNCTION, \
 ELEMENT_SUMMARY, ELEMENTIDRESOLVER )
#define STAT_SET_FIELD_IDENTIFIER_DEL( NAME, PARENT_STAT_TYPE_NAME, STAT_TYPE_NAME )

#define STAT_ESET_FIELD_DECL( NAME, PARENT_STAT_TYPE_NAME, STAT_TYPE_NAME )
#define STAT_ESET_FIELD_DEF( CONTAINER, NAME, SUMMARY, VIS, TITLE, WIDTH,      \
                             PARENT_STAT_TYPE_NAME, STAT_TYPE_NAME,            \
                             ENUMERATE_FUNCTION, SOURCE_METER_POINTER,         \
                             ENABLE_FUNCTION_POINTER, DISABLE_FUNCTION_POINTER,\
                             CLEANUP_FUNCTION_POINTER, ELEMENT_SUMMARY )
#define STAT_ESET_FIELD_DEL( NAME, PARENT_STAT_TYPE_NAME, STAT_TYPE_NAME )


#define STAT_ESET_FIELD_IDENTIFIER_DECL( NAME, PARENT_STAT_TYPE_NAME, STAT_TYPE_NAME )
#define STAT_ESET_FIELD_IDENTIFIER_DEF( NAME, SUMMARY, VIS, TITLE, WIDTH,            \
 PARENT_STAT_TYPE_NAME, STAT_TYPE_NAME, ENUMERATE_FUNCTION, SOURCE_METER_POINTER,\
 ENABLE_FUNCTION_POINTER, DISABLE_FUNCTION_POINTER, CLEANUP_FUNCTION_POINTER,    \
 ELEMENT_SUMMARY, ELEMENTIDRESOLVER)
#define STAT_ESET_FIELD_IDENTIFIER_DEL( NAME, PARENT_STAT_TYPE_NAME, STAT_TYPE_NAME )

class Base
{
public:
};
class BaseStruct : public SType
{
public:
    BaseStruct( const STD::string& name,
                const STD::string& summary,
                const size_t size,
                BaseStruct* derrivedFrom,
                FetchFcnPtr atomicFetchFcn,
                NewARIFcnPtr newARIFcnPtr );
};
typedef STD::string (*StringFromVoidFcnPtr)(void*);
template<class CTYPE>
class ScalarType : public SType {};

ReqInstance* compoundFind( const STD::string& pathIn,
                           CtxtBasePtr& context,
                           Base* object,
                           BaseStruct* type,
                           void* addr );

// Create compound stat object which stores structure type stat object
// and structure is used to get meter value
template< class STAT_TYPE >
class Compound : public Base
{
public:
    Compound( Family& parent,
              const STD::string name,
              const STD::string summary,
              const Stat::Visibility::Enum vis = Stat::Visibility::deflt,
              const STD::string title = "",
              const Meta& meta = Meta() );
};


template< class STAT_TYPE >
Compound<STAT_TYPE>::Compound( Family& parent,
                               const STD::string name,
                               const STD::string summary,
                               const Stat::Visibility::Enum vis,
                               const STD::string title,
                               const Meta& meta )
{
    printf("Creating Compound ...\n");
};

// Create compound stat object which stores struct type by reference
template< class STAT_TYPE >
class CompoundR : public Base
{
public:
    CompoundR( Family& parent,
               const STD::string name,
               const STD::string summary,
               typename STAT_TYPE::SrcType* addr,
               const Stat::Visibility::Enum vis = Stat::Visibility::deflt,
               const STD::string title = "",
               const Meta& meta = Meta() );
private:
    void* addr;  // Pointer to Reference of structure which has meter
};


template< class STAT_TYPE >
CompoundR<STAT_TYPE>::CompoundR( Family& parent,
                                 const STD::string name,
                                 const STD::string summary,
                                 typename STAT_TYPE::SrcType* addr,
                                 const Stat::Visibility::Enum vis,
                                 const STD::string title,
                                 const Meta& meta )
    : Base( parent, name, summary, vis, Kind::compound, title,
            &STAT_TYPE::sType, meta, (StringFromVoidFcnPtr)0 ),
    addr( addr )
{
    printf("Creating CompoundR ...\n");
}

// Create compound stat object used from the c-api
// this is the non templatized version
class BasicCompoundR : public Base
{
public:
    BasicCompoundR( Family& parent,
                    const STD::string name,
                    const STD::string summary,
                    void* addr,
                    BaseStruct* sType,
                    const Stat::Visibility::Enum vis = Stat::Visibility::deflt,
                    const STD::string title = "",
                    const Meta& meta = Meta() )
    {}
private:
    void* addr;  // Pointer to Reference of structure which has meter
};

// Files drivers/fa2/386/fore_atm_user.h and drivers/fa2x/386/user.h
// define a macro called Set which interfers with the following.

// Class to create "set" type of stat object of given stat type(STYPE)
// It also used to create Dynamic object & it is not added in to stats tree
template< class STYPE >
class Set : public BaseSet
{
public:
    typedef void (* EnumerateFcnPtr)( const EnumParams& params );
    Set( Family& parent,
         const STD::string& name,
         const STD::string& summary,
         const Visibility::Enum vis,
         const STD::string title,
         const short width,
         ObjectLookupFcnPtr objectLookupFcnPtr,
         EnumerateFcnPtr applyPartsFcnPtr,
         STD::string elementSummary,
         const char elementIdResolver = ElementHint::NORESOLVER ) // Even for V5, developer can specify an identifier but we wont send in Meta data
	{
        printf("Creating Set ...\n");
    }
    ~Set() {}
};

class ApplyPartActionParms {};

// This class is same as BaseSet class, however for Implicit Set stat design
// Object lookup and enumerate&all functions are internal to the stat class
class ImplicitSet : public Family
{
public:
    typedef bool (* ApplyPartActionPtr)( const ApplyPartActionParms& params );
    virtual Addr ObjectLookupFcn( STD::string name,
                                  SetLookupData info,
                                  STD::string& errorMsg ) {
        return Stat::Addr();
    }

    class EnumParams
    {
    public:
        EnumParams( ApplyPartActionPtr apap,
                    Visibility::Enum vis,
                    CtxtBasePtr& context,
                    void* state  )
            : _apap(apap), _vis(vis), _context(context), _state(state)
        {}
        bool enumerateAndCallFunction( const STD::string& name,
                                       void* object ) const;
        ApplyPartActionPtr apap() const {
            return _apap;
        }
        Visibility::Enum vis() const {
            return _vis;
        }
        CtxtBasePtr context() const {
            return _context;
        }
        void* state() const {
            return _state;
        }
    private:
        ApplyPartActionPtr _apap;
        Visibility::Enum _vis;
        CtxtBasePtr _context;
        void* _state; // stores obj addr which can use for describe values
    };

    virtual void EnumerateFcn ( const EnumParams& params ) {}
    ImplicitSet( Family& parent,
                 const STD::string& name,
                 const STD::string& summary,
                 const Visibility::Enum vis,
                 const STD::string title,
                 SType* membersType,
                 const STD::string& eleSummary,
                 const Meta& meta )
    {}

    ~ImplicitSet() {}
    void applyParts( ApplyPartActionPtr fcn,   // Refer Family class function
                     void* state,
                     CtxtBasePtr& context );
    ReqInstance* find( const STD::string& pathIn,
                       CtxtBasePtr& context );
    void seize() {
        inUse = true;
    }
    void Release() {
        inUse = false;
    }
    const STD::string elementSummary;
    bool inUse;
};

// This class is same as Set class and follows the ImplicitSet class rule
// This is special Set stat for scalar data type array objects
template< class STAT_TYPE >
class ScalarSet : public ImplicitSet
{
public:
    ScalarSet(Stat::Family& parent,
              const STD::string name,
              const STD::string& summary,
              const Visibility::Enum vis,
              const STD::string title,
              const short width,
              ScalarType <STAT_TYPE> &stype,
              void* instanceAddr,
              unsigned int count,
              const char** names=NULL,
              const STD::string& elementSummary = "")
    {}

    virtual void EnumerateFcn ( const Stat::ImplicitSet::EnumParams& params ) {
        return;
    }
    virtual Stat::Addr ObjectLookupFcn( STD::string name,
                                        Stat::SetLookupData info,
                                        STD::string& errorMsg ) {
        return Stat::Addr();
    }

    ~ScalarSet() { }
protected:
    unsigned int count;
    const char** nameList;
    STAT_TYPE *instance; // stat meter object
};

class BaseESet : public BaseSet
{
public:

    BaseESet( void*                  container,
              Family&                parent,
              const STD::string&     name,
              const STD::string&     summary,
              const Visibility::Enum vis,
              const STD::string      title,
              const short            width,
              ObjectLookupFcnPtr     objectLookupFcnPtr,
              EnumerateFcnPtr        applyPartsFcnPtr,
              void*                  srcMtrPtr,
              EnableFcnPtr           enableFcnPtr,
              DisableFcnPtr          disableFcnPtr,
              CleanupFcnPtr          cleanupFcnPtr,
              STD::string            elementSummary,
              BaseStruct*            membersType,
              const Stat::ParentType::Enum parentType,
              const char             elementIdResolver ) {
    }

private:
    void*         container;
public:
    void*                  srcMtrPtr;
    EnableFcnPtr           enableFcnPtr;
    DisableFcnPtr          disableFcnPtr;
    CleanupFcnPtr          cleanupFcnPtr;
    Stat::ParentType::Enum _parentType;

    ~BaseESet() {}
};


/* Class to create "ESet" (Etrace Set) type of stat object of given stat
 * type (STYPE)
 */
class ESet
{
public:
    ESet( void*                  container,
          Family&                parent,
          const STD::string&     name,
          const STD::string&     summary,
          const Visibility::Enum vis,
          const STD::string      title,
          const short            width,
          EnumerateFcnPtr        applyPartsFcnPtr,
          void*                  srcMtrPtr,
          EnableFcnPtr           enableFcnPtr,
          DisableFcnPtr          disableFcnPtr,
          CleanupFcnPtr          cleanupFcnPtr,
          STD::string            elementSummary,
          BaseStruct*            sType,
          const ParentType::Enum parentType = ParentType::Family,
          /* Even for V5, developer can specify an identifier but we wont
               * send in Meta data
               */
          const char             elementIdResolver =
              ElementHint::NORESOLVER ) {
        printf("Creating ESet ...\n");
    }

    ~ESet() {}
};

/* Base type to CSet
 * type (STYPE)
 */
class BaseCSet : public BaseSet
{
public:
    typedef void (* EnumerateFcnPtr)( void *params );

    BaseCSet( Family&                parent,
              const STD::string&     name,
              const STD::string&     summary,
              const Visibility::Enum vis,
              const STD::string      title,
              const short            width,
              EnumerateFcnPtr        applyPartsFcnPtr,
              STD::string            elementSummary,
              BaseStruct*            membersType,
              const char             elementIdResolver ) {
    }

    EnumerateFcnPtr applyPartsFcnPtr;

    ~BaseCSet() {}
};

/* Class to create "CSet" (Observability Set - used with Block components) type of stat object of given stat
 * type (STYPE)
 */
class CSet
{
public:

    CSet( Family&             parent,
          const STD::string&     name,
          const STD::string&     summary,
          const Visibility::Enum vis,
          const STD::string      title,
          const short            width,
          EnumerateFcnPtr        applyPartsFcnPtr,
          STD::string            elementSummary,
          BaseStruct*            sType,
          const char             elementIdResolver =
              ElementHint::NORESOLVER ) {
        printf("Creating CSet ...\n");
    }

    ~CSet() {}
};

template< class CTYPE >
class NonnumericFact : public Base
{
public:
    NonnumericFact( Family& parent,
                    const STD::string name,
                    const STD::string summary,
                    const Stat::Visibility::Enum vis = Stat::Visibility::deflt,
                    const STD::string title = "",
                    const Meta& meta = Meta(),
                    const char elementIdResolver = ElementHint::NORESOLVER ) {
        printf("Creating NonnumericFact...\n");
    }

};


// Create Nonnumeric Fact stat object whose meter is reference pointer
template< class CTYPE >
class NonnumericFactR
{
public:
    NonnumericFactR( Family& parent,
                     const STD::string name,
                     const STD::string summary,
                     CTYPE* reference,
                     const Stat::Visibility::Enum vis = Stat::Visibility::deflt,
                     const STD::string title = "",
                     const Meta& meta = Meta(),
                     void* userDataPtr = (void*)0,
                     const char elementIdResolver = ElementHint::NORESOLVER ) {
        printf("Creating NonnumericFactR...\n");
    }
};

// Create Nonnumeric Fact stat object whose meter value gets from function
class NonnumericFactV : public Base
{
public:
    NonnumericFactV( Family& parent,
                     const STD::string name,
                     const STD::string summary,
                     StringFromVoidFcnPtr sfvfp,
                     const Visibility::Enum vis = Visibility::deflt,
                     const STD::string title = "",
                     const Meta& meta = Meta(),
                     void* userDataPtr = (void*)0,
                     const char elementIdResolver = ElementHint::NORESOLVER ) {
        printf("Creating NonnumericFactV...\n");
    }
};

// Create Fact stat object of given fact class meter type
template< class FACT_METER_TYPE>
class Fact : public Base
{
public:
    Fact( Family& parent,
          const STD::string name,
          const STD::string summary,
          const Stat::Visibility::Enum vis = Stat::Visibility::deflt,
          const STD::string title = "",
          const Meta& meta = Meta() ) {

        printf("Creating Fact...\n");
    }
};

// Create Fact stats object whose meter is reference type
template< class CTYPE >
class FactR
{
public:
    FactR( Family& parent,
           const STD::string name,
           const STD::string summary,
           CTYPE* reference,
           const Stat::Visibility::Enum vis = Stat::Visibility::deflt,
           const STD::string title = "",
           const Meta& meta = Meta() ) {
        printf("Creating FactR...\n");
    }
};

// Create Fact stat object of virtual meter type i.e. meter value is
// retrieved from function (here function pointer "ReturnFcnPtrType")
template< class CTYPE >
class FactV : public Base
{
public:
    // Function pointer to return Meter value
    typedef CTYPE (*ReturnFcnPtrType)( void* );

    FactV( Family& parent,
           const STD::string name,
           const STD::string summary,
           ReturnFcnPtrType returnFcnPtr,
           const Stat::Visibility::Enum vis = Stat::Visibility::deflt,
           const STD::string title = "",
           const Meta& meta = Meta(),
           void* userDataPtr = (void*)0,
           bool dynamic = false ) {
        printf("Creating FactV...\n");
    }
};

// Create stat object using other two stat object. It get meter value by
// performing operation(+,-,/,%) on two stat object as per client request
class Computed2 : public Base
{
public:
    Computed2( Family& parent,
               const STD::string name,
               const STD::string summary,
               const STD::string op1,
               const STD::string op2,
               const Stat::Computed2Operator::Enum opType,
               const Stat::Visibility::Enum vis = Stat::Visibility::deflt,
               const STD::string title = "",
               const Meta& meta = Meta(),
               const computed2Behavior::Enum behavior = computed2Behavior::fact,
               bool  isField = false,
               int   sortKey = 0,
               bool  resetBehavior = false ) {
        printf("Creating Computed2...\n");
    }
};

class Computed : public Base
{
public:
    Computed( Family& parent,
              const STD::string name,
              const STD::string summary,
              const char** statpathList,
              const unsigned int cnt,
              const Stat::ComputedOperator::Enum opType,
              const Stat::Visibility::Enum vis = Stat::Visibility::deflt,
              const STD::string title = "",
              const Meta& meta = Meta(),
              const computedBehavior::Enum behavior = computedBehavior::fact,
              bool  isField = false,
              int   sortKey = 0,
              bool  resetBehavior = false ) {
        printf("Creating Computed...\n");
    }
};

// Create counter type stat object
template< class COUNTER_METER_TYPE >
class Counter : public Base
{
public:
    Counter( Family& parent,
             const STD::string name,
             const STD::string summary,
             const Stat::Visibility::Enum vis = Stat::Visibility::deflt,
             const STD::string title = "",
             const Meta& meta = Meta(),
             bool  resetBehavior = false ) {
        printf("Creating Counter...\n");
    }
};

// Create counter stat object whose meter is reference type
template< class CTYPE >
class CounterR
{
public:
    CounterR( Family& parent,
              const STD::string name,
              const STD::string summary,
              CTYPE* reference,
              const Stat::Visibility::Enum vis = Stat::Visibility::deflt,
              const STD::string title = "",
              const Meta& meta = Meta(),
              bool  resetBehavior = false ) {
        printf("Creating CounterR...\n");
    }
};

// Create counter stat object whose meter is virtual type i.e. meter
// value is retrieved from function (here "ReturnFcnPtrType")
template< class CTYPE >
class CounterV : public Base
{
public:
    typedef CTYPE (*ReturnFcnPtrType)( void* );

    CounterV( Family& parent,
              const STD::string name,
              const STD::string summary,
              ReturnFcnPtrType returnFcnPtr,
              const Stat::Visibility::Enum vis = Stat::Visibility::deflt,
              const STD::string title = "",
              const Meta& meta = Meta(),
              bool  resetBehavior = false,
              void* userDataPtr = (void*)0,
              bool dynamic = false ) {
        printf("Creating CounterV...\n");
    }
};

class CG {
  public:
    enum Enum{ _default, global, globalOff, _last, };

    static bool isEnabled( const Stat::CG::Enum grp ){return true;}
    static bool isdefaultCGState( const Stat::CG::Enum grp ){return true;}
    static bool isadminEnabled( const Stat::CG::Enum grp ){return true;}
    static void enable( const Stat::CG::Enum grp ){}
    static void disable( const Stat::CG::Enum grp ){}
    static void adminenable( const Stat::CG::Enum grp ){}
    static void admindisable( const Stat::CG::Enum grp ){}
    static void setup(){}

};
typedef int CounterMeter16Global;
typedef int CounterMeterGlobal;
typedef int CounterMeterGlobalOff;
typedef int CounterMeter32Global;
typedef int CounterMeter32GlobalOff;

typedef int FactMeterGlobal;
typedef int FactMeter32Global;
typedef int UFactMeterGlobal;
typedef int UFactMeter32Global;

};

extern Stat::Family netFacilityRoot;
extern Stat::Family tftpFacilityRoot;
extern Stat::Family ipFacilityRoot;
extern Stat::Family v4FacilityRoot;
extern Stat::Family udpFacilityRoot;
extern Stat::Family tcpFacilityRoot;
extern Stat::Family icmpFacilityRoot;
extern Stat::Family v6FacilityRoot;
extern Stat::Family icmp6FacilityRoot;

extern Stat::Family storeFacilityRoot;
extern Stat::Family nfsFacilityRoot;
extern Stat::Family rpcFacilityRoot;
extern Stat::Family cifsFacilityRoot;
extern Stat::Family ntpFacilityRoot;
extern Stat::Family fsFacilityRoot;
extern Stat::Family ndmpFacilityRoot;
extern Stat::Family snapFacilityRoot;
extern Stat::Family sysFacilityRoot;
extern Stat::Family kernelFacilityRoot;
extern Stat::Family memoryRoot;
extern Stat::Family cpuRoot;
extern Stat::Family servicesRoot;
extern Stat::Family pageLevelAllocatorRoot;
extern Stat::Family translationsRoot;
extern Stat::Family bufferCacheRoot;
extern Stat::Family uncachedBuffersRoot;
extern Stat::Family mallocRoot;
extern Stat::Family slabPoolsRoot;
extern Stat::Family locksRoot;
extern Stat::Family utilizationRoot;
extern Stat::Family cbfsFacilityRoot;
extern Stat::Family chipsetFacilityRoot;
extern Stat::Family pentiumFacilityRoot;
extern Stat::Family ftpFacilityRoot;
extern Stat::Family httpFacilityRoot;
extern Stat::Family lockdFacilityRoot;
extern Stat::Family vcFacilityRoot;
extern Stat::Family ufsFacilityRoot;
extern Stat::Family repFacilityRoot;
extern Stat::Family repV2FacilityRoot;
extern Stat::Family ufs64FacilityRoot;
extern Stat::Family iscsiFacilityRoot;
#if !defined(__VNXE__)
extern Stat::Family mpfsFacilityRoot;
extern Stat::Family dpFacilityRoot;
extern Stat::Family replFacilityRoot;
#endif
#if defined(ETRACE)
extern Stat::Family etraceFacilityRoot;
extern Stat::Family etraceFoundationFacilityRoot;
extern Stat::Family etraceDynamicFacilityRoot;
extern Stat::Family journalFacilityRoot;
extern Stat::Family fileResolveRoot;
#endif
extern Stat::Family sshFacilityRoot;
extern Stat::Family storageFacilityRoot;
extern Stat::Family physicalFacilityRoot;
extern Stat::Family fibreChannelFacilityRoot;

using namespace Stat;

#endif
