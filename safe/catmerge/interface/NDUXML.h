
//++
//
//  File Name:
//      NDUXML.h
//
//  Contents:
//      NDUXmlIn and NDUXmlOut class definitions.
//
//  Revision History:
//
//      2010-Jul-11   Brandon Myers  Initial version.
//
//---

#ifndef NDU_XML_H
#define NDU_XML_H

#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

#define NDU_XML_TAG_SIZE 256
#define NDU_XML_DATA_SIZE 256
#define NDU_XML_ATTRIBUTE_SIZE 256
#define NDU_XML_VALUE_SIZE 256

#define NDU_XML_PREFIX_CLARIION "NDUSTATS"

#define NDU_XML_ATTRIBUTE_TYPE "type"

#define NDU_XML_VALUE_CATEGORY "Category"
#define NDU_XML_VALUE_CONTAINER "Container"
#define NDU_XML_VALUE_PROPERTY "Property"

#define NDU_XML_DEFAULT_INDENTION_STRING "  "

#define NDU_XML_TAG_NDU "NDU"

int GetListPosition( char * Data, char ** List, int ListSize );

class NDUXmlInImpl;
class NDUXmlOutImpl;

//  The NDUXMLIn class is a helper class used for reading in XML files that were
//  created using the NDUXmlOut class.  It may not be able to handle an
//  arbitrary XML file, as it isn't very robust.

class CSX_CLASS_EXPORT NDUXmlIn
{
public:
    NDUXmlIn( const char * FileName, const char * PrefixString = NULL );
    ~NDUXmlIn();
    bool IsGood();
    bool GetNextTag( char * Tag, char * Data, char * Attribute, char * Value );
    bool SkipSection( char * SkipTag );

private:
    NDUXmlInImpl * pImpl;
};

//  A begin tag marks the start of a section and contains no data.
//  An end tag marks the end of a section and contains no data.
//  A complete tag has a beginning tag, some data, and an end tag all in one
//  line.


enum NDUXmlTagFormat
{
    NDUXmlTagBegin,
    NDUXmlTagEnd,
    NDUXmlTagComplete
};

//  The NDUXMLOut class is a helper class used for creating XML files.

class CSX_CLASS_EXPORT NDUXmlOut
{
public:
    NDUXmlOut( const char * FileName, const char * PrefixString = NULL, bool DoShowAttributes = false, const char * IndentionString = NDU_XML_DEFAULT_INDENTION_STRING );
    ~NDUXmlOut();
    bool IsGood();
    void WriteHeader();
    void AddTag( const char * Tag, const char * Data, const char * Attribute, const char * Value, NDUXmlTagFormat Format );
    void AddTag( const char * Tag, int Data, const char * Attribute, const char * Value, NDUXmlTagFormat Format );
    void AddTag( const char * Tag, const char * Data, const char * Attribute, int Value, NDUXmlTagFormat Format );
    void AddTag( const char * Tag, int Data, const char * Attribute, int Value, NDUXmlTagFormat Format );
    void AddTag( const char * Tag, const char * Data, NDUXmlTagFormat Format );
    void AddTag( const char * Tag, int Data, NDUXmlTagFormat Format );
    void AddTag( const char * Tag, double Data, NDUXmlTagFormat Format );
    void AddTag( const char * Tag, NDUXmlTagFormat Format );
    void IncreaseIndention();
    void DecreaseIndention();

private:
    NDUXmlOutImpl * pImpl;
};

#endif

