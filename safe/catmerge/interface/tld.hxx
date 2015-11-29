//////////////////////////////////////////////////////////////////////
// tld.hxx -- declaration of TLD (Tag-Length-Data) class
// 
// Copyright 1999 CLARiiON Storage Subsystems, Inc. All Rights Reserved
// 
// Author          : Brian Campbell
// Created On      : May 13 1999
// Last Modified By: T. David Hudson
// Last Modified On: Tue Oct 26 10:17:04 2004
// Update Count    : 71
// PURPOSE
//      This file declares the TLD functionality. Refer to TLD protocol
//      specification for definition of TLD and the TLD design specification
//      for discusion of how this code works
//
// Change History:
//   10-18-1999 Brian Campbell
//              Initial offering
//
//   12-16-1999 Brian Campbell
//              Add GetRevision method for compatibility checks
//
//   08-30-2002 Lynn Bryant
//              Add 64-bit integer support
//
//   09/17/2002 Lynn Bryant
//              Modified the 64-bit integer support method so that it
//              would not require ASE or Layered drivers to modify
//              their code when dealing with 32-bit integers
//
//   09-27-2002 Lynn Bryant
//              Added an include file that Admin needed
//
//   02-19-2003 David Evans
//              Added memory tracking functionality
//
//////////////////////////////////////////////////////////////////////

#ifndef _TLD_HXX
#define _TLD_HXX
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

#if ! defined(NT) && (defined(_WINDOWS) || defined(WIN32) || defined(_WIN32_WINNT))
#undef NT
#define NT 1
#endif

#if defined(DLL_BUILD)
#define DLL_EXPORT       CSX_MOD_EXPORT
#define DLL_EXPORT_CLASS CSX_CLASS_EXPORT
// use shared memory as the default for dll builds
#define TLD_OPTION_DEFAULT_SHARED_MEMORY TLD::TLD_OPTION_USE_SHARED_MEMORY
#else
#define DLL_EXPORT
#define DLL_EXPORT_CLASS
// no shared memory as the default for NON dll builds
#define TLD_OPTION_DEFAULT_SHARED_MEMORY 0
#endif

#include <stdlib.h>

#ifndef NO_IOSTREAM
#ifndef OLD_IOSTREAM 
#include <iostream>
using namespace std;
#else
#include <iostream.h>
#endif
#endif

typedef unsigned char TLDdata;
typedef unsigned int TLDnumber;
typedef unsigned int TLDlength;
typedef unsigned int TLDtag;
typedef unsigned int TLDbool;
// LAB 8/30/02 - add support for 64-bit unsigned integers
#ifdef NT
  typedef unsigned _int64 TLDBigNumber;
#else
  typedef unsigned long long TLDBigNumber;
#endif

#ifdef NT
#define TLD_USING_LOOKASIDE_LIST
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

#if defined(___ENABLE_EV_MEMORY_TRACKING___)
//#include <windows.h>
static void *(*_newOperator)(size_t size) = &operator new;
static void (*_deleteOperator)(void *ptr) = &operator delete;
DLL_EXPORT void SetTldNewOperator(void *(*newOperator)(size_t size));
DLL_EXPORT void SetTldDeleteOperator(void (*deleteOperator)(void *ptr));
DLL_EXPORT void EnterTldCriticalSection();
DLL_EXPORT void LeaveTldCriticalSection();
#endif // defined(___ENABLE_EV_MEMORY_TRACKING___)

#define TLD_MAX_REQUEST 206240

class DLL_EXPORT_CLASS TLD
{
  public:
#ifndef NO_IOSTREAM
#if defined(OUTPUT64BIT_NO_EV_OR_RW) && defined(___ENABLE_EV_MEMORY_TRACKING___)
    friend static void
    _output64Bit(
                 TLDnumber high32Bits,
                 TLDnumber low32Bits,
                 TLDnumber octOrDecOrHexBase,
                 size_t fillWidth,
                 int negIfLeft0IntPosRight,
                 char fillChar,
                 char * & newBuffer
                );
    friend static ostream&
    operator<<(ostream& os, TLDBigNumber i);
#endif // defined(OUTPUT64BIT_NO_EV_OR_RW) && defined(___ENABLE_EV_MEMORY_TRACKING___)
#endif

    
    enum TLD_CONSTANTS
    {
      TLD_CONSTANT_CONTROLBYTESIZE = sizeof(TLDdata),

      // control + taglength plus lengthlength
      TLD_MINIMUM_SIZE = TLD_CONSTANT_CONTROLBYTESIZE + 2,

      TLD_MAXIMUM_HEADERSIZE = TLD_CONSTANT_CONTROLBYTESIZE + sizeof(TLDtag)
      + sizeof (TLDlength),
      
      // constant indicating how many occurrences of a tag to find
      TLD_ALLOCCURRENCES = ~0,

      // tag used for applications that do not care about the value
      TLD_TAG_UNUSED = 0
    };

#define TLD_BIT(position) ((char)1 << position)

    // both local and global options are listed here. Certain options make 
    // sense in certain contexts. This can mean that known ones are ignored
    // sometimes. See individual methods for behavoir

    enum TLD_OPTIONS
    {
      TLD_OPTION_REQUIRED_TAG = TLD_BIT(0),
      TLD_OPTION_COPYDATA = TLD_BIT(1),
      TLD_OPTION_THROW = TLD_BIT(2),
      TLD_OPTION_ENDIAN_LITTLE = TLD_BIT(3), // off means big
      TLD_OPTION_USE_SHARED_MEMORY = TLD_BIT(4), // off means "new"
      TLD_OPTION_DECODE_ONLY_TOP_LEVEL = TLD_BIT(5),
      TLD_OPTION_MODE_ASCII = TLD_BIT(6),
      TLD_OPTION_ALLOW_MULTIPLE_TAGS = TLD_BIT(7),
      TLD_OPTION_SEARCH_PEERS_ONLY = TLD_BIT(8),
      TLD_OPTION_SEARCH_EMBEDDED_ONLY = TLD_BIT(9),
      // used inconjunction with other bits to override defaults
      TLD_OPTION_OVERRIDE = TLD_BIT(10),
      TLD_OPTION_MAX
    };

#undef TLD_BIT
    
    enum TLD_ERROR
    {
      TLD_ERROR_SUCCESS = 0,
      TLD_ERROR_UNSUPPORTED_MODE,
      TLD_ERROR_UNSUPPORTED_LENGTH,

      // when ascii mode is supported
//      TLD_ERROR_MIXED_MODES,
//      TLD_ERROR_BAD_ASCII_LENGTH,

      TLD_ERROR_MULTIPLE_TAGS_FOUND,
      TLD_ERROR_TAG_NOT_FOUND,
      TLD_ERROR_LENGTH_OVERFLOW,
      TLD_ERROR_LENGTH_UNDERFLOW,
      TLD_ERROR_MEMORY_ALLOCATION_FAILURE,
      TLD_ERROR_NULL_DATA_POINTER,
      TLD_ERROR_BAD_DATA_POINTER,
      TLD_ERROR_INVALID_INDEX,
      TLD_ERROR_INVALID_OPERATION_ON_EMBEDDED,
      TLD_ERROR_MAX
    };
    
    // Make "empty" tld
    TLD(TLDtag tag = (TLDtag)TLD_TAG_UNUSED);
    
    // binary mode constructor

    TLD(TLDtag tag, TLDlength length, TLDdata *data,
        TLDbool copy = TRUE);
    
    // these 2 are for creating shortcuts for data
    TLD(TLDtag tag, TLDnumber numberData);
    TLD(TLDtag tag, TLDBigNumber bigNumberData, TLDbool is64BitNumber);

    // some compilers distinguish unsigned, so we need both
    TLD(TLDtag tag, TLDdata *stringData, TLDbool copy = TRUE);
    TLD(TLDtag tag, const char *stringData, TLDbool copy = TRUE);

    //  Copy constructor.
    TLD(const TLD &copyfrom, TLDbool copydata = TRUE);

    // destructor
    virtual ~TLD();

    // assignment operator
    TLD &operator=(const TLD &rhs);

    // comparison operator
    int operator==(const TLD &rhs);

    TLDbool GetLocalOption(TLD_OPTIONS option) const;
    TLDnumber GetLocalOptions() const;
    void SetLocalOption(TLD_OPTIONS options, TLDbool on);
    void SetLocalOptions(TLDnumber options);
    
    // get the tag
    TLDtag GetTag() const;
    
    // get the length
    TLDlength GetLength() const;
    
    // Get the pointer to the data
    TLDdata *GetData() const;

    // set the length
    void SetLength(TLDlength length);    

    // set the Tag
    void SetTag(TLDtag tag);

    // set the pointer to the data
    void SetData(TLDdata *data);
        
    void
    CopyData(TLDdata *newData, TLDlength length, TLDlength allocationLength=0);

    // Get Number of Embedded TLDs, 0 means none. Immediate level only
    TLDnumber GetEmbeddedCount() const;

    TLD *GetEmbedded() const;
    
    TLD &GetEmbeddedAt(const TLDnumber index) const;

    const TLD *
    Find(TLDtag tag, TLDnumber options=0) const;

    TLD *
    FindReference(TLDtag tag,
                  TLDnumber maxoccurrences = (TLDnumber)TLD_ALLOCCURRENCES,
                  TLDnumber options = 0) const;
    
    TLDnumber
    GetNumber(TLDtag tag, TLDnumber options = 0) const;

    TLDBigNumber
    GetBigNumber(TLDtag tag, TLDnumber options = 0) const;

    const TLDdata *
    GetString(TLDtag tag, TLDnumber options = 0);

    // clear a tld and delete the embedded tld's
    void Clear();
    
    // copy a tld recursively, delete current contents
    void Copy(const TLD &copyfrom, TLDbool copydata = TRUE);

    // decode a hidden tag
    TLD_ERROR DecodeHiddenTag(TLDtag tag, TLDnumber options = 0);

    // Encode a hidden tag
    TLD_ERROR EncodeHiddenTag(TLDtag tag, TLDnumber options = 0);

#ifndef NO_IOSTREAM
#ifndef OLD_IOSTREAM
	// dump contents
    void
    Dump(TLDnumber maxwidth = (TLDnumber)TLD_ALLOCCURRENCES,
         TLDnumber maxdepth = (TLDnumber)TLD_ALLOCCURRENCES,
		 TLDbool showIndentation = TRUE, std::ostream &outs = std::cout) const;

	void
		Dump(std::ostream &outs) const;

#else
    // dump contents
    void
    Dump(TLDnumber maxwidth = (TLDnumber)TLD_ALLOCCURRENCES,
         TLDnumber maxdepth = (TLDnumber)TLD_ALLOCCURRENCES,
         TLDbool showIndentation = TRUE, ostream &outs = cout) const;

	void
    Dump(ostream &outs) const;

#endif
#endif

    
    
    void AddEmbedded(TLD &tld, TLDbool addToCurrentLength = TRUE);
    void AddPeer(TLD &tld);

    TLD *RemovePeer();

    TLD *RemoveEmbedded();
    
    TLD *GetPeer() const;

    TLD &GetPeerAt(const TLDnumber index) const;
    
    TLDnumber GetPeerCount(TLD **lastPeer = (TLD **)0) const;

    TLDlength GetHeaderLength() const;
    
    TLDlength GetTLDAndPeerLength() const;

    TLDlength GetTLDLength() const;
    
    TLD &GetTLDFromData(TLDnumber index = 0) const;
    
    TLDbool IsAsciiMode() const;
    
    // These are static per language definition
    static void *operator new(size_t size);

    static void operator delete(void *data
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#else
				, size_t size
#endif
				);
    
    // calculate the minimum size for given number
    static TLDnumber CalculateMinimumSizeToStoreNumber(TLDBigNumber number);

    static TLD_ERROR
    Decode(TLDdata *decodeFromHere, TLD **decodeToHere, 
           TLDlength totalLengthOfStream, TLDnumber options = 0);
    
    static TLD_ERROR 
    Encode(TLD &tld, TLDdata **encodeHereP, TLDlength &bytesEncoded,
           TLDnumber options = 0);

    static TLDBigNumber
    GetEndianNumber(TLDdata *sourceAddress, 
                    TLDlength bytesEncoded,
                    TLDbool isSourceEndianLittle = IsNativeEndianLittle());
    
    static TLDbool GetGlobalOption(TLD_OPTIONS option);
    static TLDnumber GetGlobalOptions();

    static const TLDnumber GetRevision();
    
    static TLDbool IsNativeEndianLittle();

    static TLDnumber
    SetEndianNumber(TLDdata *targetAddress, TLDBigNumber usingThisNumber,
                    TLDbool isTargetEndianLittle = IsNativeEndianLittle());
    
    static void SetGlobalOption(TLD_OPTIONS options, TLDbool on);
    static void SetGlobalOptions(TLDnumber options);

#ifdef TLD_USING_LOOKASIDE_LIST
    static void SetMultiThreaded(bool isMultiThreaded);
    static bool GetMultiThreaded();
#endif

  protected:

    // control enums
    
    enum TLD_CONTROL
    {
      // area operations
      TLD_CONTROL_MIN = -1,
      TLD_CONTROL_MODE,
      TLD_CONTROL_RESERVED,
      TLD_CONTROL_LENGTH,
      TLD_CONTROL_EMBEDDED,
      TLD_CONTROL_TAG,
      TLD_CONTROL_MAX
    };

    static void
    _controlAdmin(TLD_CONTROL area, TLDdata &control, TLDdata &data, 
                  TLDbool set);

    static TLD &
    _throw(const TLD_ERROR error, const char *function=0, 
           const TLDnumber line=0, const char *file=0);

  private:

    //
    // Private Data Members
    // 

    TLDdata _control;
    
    // the tag
    TLDtag _tag;
    
    // the length 
    TLDlength _length;
    
    // the data
    TLDdata *_data;

#ifdef TLD_USING_LOOKASIDE_LIST
    TLDdata _scratchPad[32];

    bool _usingScratch;
#endif

    // embedded tld's (or children). Pointer TLD list to other TLD's
    TLD *_embedded;
    
    // the peer
    TLD *_peer;
    
    TLDnumber _localOptions;

    static TLDnumber _globalOptions;

#ifdef TLD_USING_LOOKASIDE_LIST
    static bool _isMultiThreaded;
#endif
    //
    // functions:
    //

    static void _delete(TLDdata *data, TLDbool isArray=TRUE);
    static TLDdata *_new(TLDlength size);
    
    
#ifndef NO_IOSTREAM
#ifndef OLD_IOSTREAM
	static void
	_dump(const TLD *tld, 
          TLDnumber maxwidth, 
          TLDnumber depth,
          TLDnumber maxdepth,
          TLDbool showIndentation, 
          TLDnumber embeddedLevel, 
		  std::ostream &outs);
#else
    static void
    _dump(const TLD *tld, 
          TLDnumber maxwidth, 
          TLDnumber depth,
          TLDnumber maxdepth,
          TLDbool showIndentation, 
          TLDnumber embeddedLevel, 
          ostream &outs);
#endif
#endif

    
    static TLD_ERROR
    _encode(TLD *tld, TLDdata **encodeHereP, TLDlength &bytesEncodedSoFar,
            const TLDlength bytesNeededForEncoding, TLDnumber options);

    void
    _findReference(const TLD &searchTLD, TLD &foundTLD, TLDtag tag,
                   TLDnumber options, TLDnumber maxoccurrences, 
                   TLDnumber &foundoccurrences) const;

    const TLD * 
    _findValue(const TLD *searchTLD, 
               TLDtag tag, 
               TLDnumber options) const;

    // Get the tag length field in the control byte.
    TLDdata _getTagLength() const;

    // Get the length length field in the control byte.
    TLDdata _getLengthLength() const;

    // set whether this is embeded or not
    TLDdata _getEmbedded() const;

    void
    _initialize(TLDtag tag, TLDlength _length, TLDdata *data,
                TLDbool copy);

    // set whether this is embeded or not
    void _setEmbedded(TLDbool isEmbeddedTrueNotEmbededFalse);

    // Set the tag length field in the control byte.
    void _setTagLength(TLDtag tag);

    // Set the length length field in the control byte.
    void _setLengthLength(TLDlength length);
    
};

// some compilers cannot handle public types when called outside of
// base/derived class scope

#if !defined(SUN)  || defined(_CLASS_SCOPE_DECL_NOT_GLOBAL)
typedef TLD::TLD_ERROR TLD_ERROR;
#endif

#endif // #ifndef TLD_HXX
