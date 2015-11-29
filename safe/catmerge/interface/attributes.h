#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

#include "NDUXML.h"

class IprNode;

typedef IprNode * pIprNode;

class CSX_CLASS_EXPORT AttributeTable
{
public:
    AttributeTable();
    ~AttributeTable();
    void Set( char * Key, const char * Data );
    void Set( char * Key, int Data );
    char * Get( char * Key );
    int GetInt( char * Key );
    void ExportAttributesTo( AttributeTable & CombinedAttributes );
    char * Evaluate( char * Key, AttributeTable & GlobalAttributes );
    int EvaluateInt( char * Key, AttributeTable & GlobalAttributes );
    void Delete( char * Key );
    void DeleteAll( pIprNode pNode );
    void DeleteAll();
    int Save( char * FileName, char * TagName );
    int Load( char * FileName );
    void AddXml( NDUXmlOut & XmlFile, char * TagName );
    BOOL LoadXml( NDUXmlIn & XmlFile );

private:
    pIprNode AttributeTree;
};

#endif
