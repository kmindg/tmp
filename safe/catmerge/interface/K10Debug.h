#ifndef  _K10_DEBUG_H_
#define  _K10_DEBUG_H_

//***************************************************************************
// Copyright (C) Data General Corporation 1989-2000
// All rights reserved.
// Licensed material -- property of Data General Corporation
//**************************************************************************/

//++
// File Name:
//      K10Debug.h
//
// Contents:
//      Various useful debug utulities
//
// Macros:
//     K10DebugPrintEntry()
//     K10DebugPrintExit()
//     K10DebugPrintError()
//     K10DebugPrintInfo()
//     K10DebugPrintDiagnostic()
//     K10DebugPrintOnSuccess()
//     K10DebugPrintOnError()
//
//     K10_ASSERT()
//     K10_ASSERTMSG()
//
//
// Revision History:
//      16-Dec-98   MWagner  Created.
//
//      10-Dec-99   MWagner  added ASSERT macros
//
//--

#include "ktrace.h"
#include "generic_types.h"

//++
// Debug Print facilities.
//
// These facilites allow slightly more granularity than
// the NT Debug Print facilities.  These facilities are
// conditional on the message type (Error, Entry, Info,
// Exit, and Diagnostic), and message class (used for
// module names or other sub-subsystem classifications).
// The intent is that the DEBUG_LEVELS can be defined on
// a per module basis, as can a debug filter.
//
// #define             DLS_BUTLER_LOCAL_CLASS         K10_DEBUG_CLASS_ONE
// #define             DLS_BUTLER_PEER_CLASS          K10_DEBUG_CLASS_TWO
// #define             DLS_FOOTMAN_CLASS              K10_DEBUG_CLASS_THREE
//
// ULONG    K10DebugMask            =  (
//                                     K10_DEBUG_LEVEL_ENTRY   |
//                                     K10_DEBUG_LEVEL_EXIT    |
//                                     DLS_BUTLER_CABAL_CLASS  |
//                                     0x0);
//
// This filter would print only entry and exit messages from the
// Dls Butler local class.
//--

#if DBG

extern ULONG            K10DebugMask;
extern CHAR             K10DebugErrorStr[];
extern CHAR             K10DebugInfoStr[];
extern CHAR             K10DebugEntryStr[];
extern CHAR             K10DebugExitStr[];
extern CHAR             K10DebugDiagStr[];


#define K10_DEBUG_LEVEL_ERROR            0x00010000
#define K10_DEBUG_LEVEL_INFO             0x00020000
#define K10_DEBUG_LEVEL_ENTRY            0x00040000
#define K10_DEBUG_LEVEL_EXIT             0x00080000
#define K10_DEBUG_LEVEL_DIAGNOSTIC       0x00100000

#define K10_DEBUG_CLASS_ONE              0x00000001
#define K10_DEBUG_CLASS_TWO              0x00000002
#define K10_DEBUG_CLASS_THREE            0x00000004
#define K10_DEBUG_CLASS_FOUR             0x00000008
#define K10_DEBUG_CLASS_FIVE             0x00000010
#define K10_DEBUG_CLASS_SIX              0x00000020
#define K10_DEBUG_CLASS_SEVEN            0x00000040
#define K10_DEBUG_CLASS_EIGHT            0x00000080
#define K10_DEBUG_CLASS_NINE             0x00000100
#define K10_DEBUG_CLASS_TEN              0x00000200
#define K10_DEBUG_CLASS_ELEVEN           0x00000400
#define K10_DEBUG_CLASS_TWELVE           0x00000800
#define K10_DEBUG_CLASS_THIRTEEN         0x00001000
#define K10_DEBUG_CLASS_FOURTEEN         0x00002000
#define K10_DEBUG_CLASS_FIFTEEN          0x00004000
#define K10_DEBUG_CLASS_SIXTEEN          0x00008000

#define K10_DEBUG_CLASS_ALL              0x0000ffff

#define DEBUG_MASK_DEFAULT               (K10_DEBUG_LEVEL_ERROR |        \
                                              K10_DEBUG_CLASS_ALL)

#define K10DebugPrint( _Mask_, _DebugLevelStr_,  _DebugMessage_ )        \
                if ( (_Mask_ & K10DebugMask & 0xffff) &&                 \
                     (_Mask_ & K10DebugMask & 0xffff0000) )              \
                {                                                        \
                    KdPrint( (_DebugLevelStr_) );                        \
                    KdPrint( _DebugMessage_ );                           \
                    KdPrint( ("\n") );                                   \
                }

#define K10DebugPrintError( _Class_, _DebugMessage_ )                    \
               K10DebugPrint( (K10_DEBUG_LEVEL_ERROR | _Class_),         \
                               K10DebugErrorStr,                         \
                               _DebugMessage_)

#define K10DebugPrintInfo( _Class_, _DebugMessage_ )                     \
                K10DebugPrint( (K10_DEBUG_LEVEL_INFO | _Class_),         \
                               K10DebugInfoStr,                          \
                               _DebugMessage_ )

#define K10DebugPrintEntry( _Class_, _DebugMessage_ )                    \
                K10DebugPrint( (K10_DEBUG_LEVEL_ENTRY | _Class_),        \
                               K10DebugEntryStr,                         \
                               _DebugMessage_ )

#define K10DebugPrintExit( _Class_, _DebugMessage_ )                     \
                K10DebugPrint( (K10_DEBUG_LEVEL_EXIT  | _Class_),        \
                               K10DebugExitStr,                          \
                               _DebugMessage_ )                          \

#define K10DebugPrintDiagnostic( _Class_, _DebugMessage_ )               \
                K10DebugPrint( (K10_DEBUG_LEVEL_DIAGNOSTIC | _Class_),   \
                                K10DebugDiagStr,                         \
                                _DebugMessage_ )

#define K10DebugPrintOnSuccess( _Status_, _Class_, _DebugMessage_ )      \
                if ( EMCPAL_IS_SUCCESS( _Status_ ) )                            \
                {                                                        \
                    K10DebugPrintInfo( _Class_, _DebugMessage_ )         \
                }

#define K10DebugPrintOnError( _Status_, _Class_, _DebugMessage_ )        \
                if ( !EMCPAL_IS_SUCCESS( _Status_ ) )                           \
                {                                                        \
                    K10DebugPrintError( _Class_, _DebugMessage_ )        \
                }
#else

#define K10DebugSetFunctionName( _Name_ )
#define K10DebugPrint( _Mask_, _DebugLevelStr_, _DebugMessage_ )
#define K10DebugPrintError( _Class_, _DebugMessage_ )
#define K10DebugPrintInfo( _Class_, _DebugMessage_ )
#define K10DebugPrintEntry( _Class_, _DebugMessage_ )
#define K10DebugPrintExit( _Class_, _DebugMessage_ )
#define K10DebugPrintDiagnostic( _Class_, _DebugMessage_ )
#define K10DebugPrintOnSuccess( _Status_, _Class_, _DebugMessage_ )
#define K10DebugPrintOnError( _Status_, _Class_, _DebugMessage_ )

#endif

//++
//
// Debug ASSERT facilities.
// According to OSR, the ASSERT() and ASSERT_MSG() macros defined
// by Microsoft only work in Checked Builds of the OS (i.e., they
// don't work in Free Builds of the OS, even when your code is
// Checked).
//
// Here's an alternative.  For extra credit, how many subsystems
// have redefined the system ASSERT() to get around this problem?
// Personally, I think redefining system macros is a really great
// idea, (I once worked with code where someone had #define'd
// fopen()), but this is for people who don't like that kind of
// activity.
//
//--
#if DBG
#define K10_ASSERT( _expression_ )                                      \
    if (!( _expression_ ))                                              \
    {                                                                   \
        KvPrint(  "Assert Failed line %d in file %s.\n",                \
                __LINE__, catmergeFile(__FILE__) );                                   \
        EmcpalDebugBreakPoint();                                                \
    }

#define K10_ASSERTMSG( _message_ , _expression_ )                       \
    if (!( _expression_ ))                                              \
    {                                                                   \
        KvPrint("Assert Failed line %d in file %s.\n",                  \
                __LINE__, catmergeFile(__FILE__) );                                   \
        KvPrint( _message_ );                                           \
        EmcpalDebugBreakPoint();                                                \
    }

#else

#define K10_ASSERT( _expression_ )
#define K10_ASSERTMSG( _message_ , _expression )

#endif

#endif  // _K10_DEBUG_H_

