
//++
//
//  File Name:
//      ToC.h
//
//  Contents:
//      SuiteInfo class definition.
//
//  Revision History:
//
//      ??-???-??   Karl Owen   Implemented.
//      15-Nov-01   Karl Owen   Commented.
//
//--

#ifndef _ToC_H
#define _ToC_H

#include "NDUXML.h"
#include "SuiteInfo.h"
#include "K10TraceMsg.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

#define NDU_EMBEDDED_BUFFER_SIZE 256

//++
//
//  Class:
//      Entry
//
//  Description:
//      Container class representing an entry in the table of contents.
//
//  Revision History:
//      15-Nov-01   Karl Owen   Commented.
//
//--

class CSX_CLASS_EXPORT Entry
{
public:
                Entry( char *name = NULL,
                       char *path = NULL,
                       BOOL Installed = false,
                       BOOL Active = false,
                       BOOL Committed = false,
                       BOOL Prior = false );

                ~Entry( void );

                //  Append the entry chain pointed to by e to the chain
                //  starting with this.

    void        Append( Entry *e );

                //  Delete the entry chain pointed to by this.  If delInfo
                //  is true, delete the referenced SuiteInfo objects,
                //  otherwise do not.

    void        Delete( BOOL delInfo );

                //  Trace a message describing the entry, prefacing the
                //  message with header.

    void        DumpOne( char *header, K10TraceMsg *trace );

                //  Trace each entry in the chain pointed to by this,
                //  prefacing each message with header.

    void        Dump( char *header, K10TraceMsg *trace );

                // Get the value of different flags as required for a sync operation.

    void        GetSyncFlags( AttributeTable & GlobalAttributes, BOOL & RebootRequired, BOOL & QuiesceRequired, BOOL & CacheDisableRequired, BOOL & TerminateSelf, BOOL & RefreshConfiguration );

                // Add entries to an existing Xml file for this class instance.

    void        AddXml( NDUXmlOut & XmlFile );

                // Populate this class instance with data from an Xml file.

    BOOL        LoadXml( NDUXmlIn & XmlFile );

    char        *name;          //  Entry name
    char        *path;          //  Entry pathname, if installed
    BOOL        installed;      //  True if currently installed
    BOOL        active;         //  True if currently active
    BOOL        committed;      //  True if committed
    BOOL        prior;          //  True if previously active

    SuiteInfo   *info;
    Entry       *next;

    //  Used to detect memory leaks.

    static int  AllocCount;

private:
    int         bNotValid;      // set to nonzero when Delete is called bug 1905
};

//++
//
//  Class:
//      ToC
//
//  Description:
//      This class represents the NDU table of contents (ToC).
//
//  Revision History:
//      15-Nov-01   Karl Owen   Commented.
//
//--

class CSX_CLASS_EXPORT ToCImpl
{
public:
                ToCImpl( void );
                ~ToCImpl( void );

                //  Return the number of entries in the ToC.

    int         GetCount( void );

                //  Return a list of all SuiteInfo objects associated
                //  with ToC entries.

    SuiteInfo   *GetAllSuites( void );

                //  Return a list of all SuiteInfo objects associated
                //  with ToC entries which match name.

    SuiteInfo   *GetSuitesByName( char *name );

                //  Return a list of all SuiteInfo objects associated
                //  with ToC entries which match display name or the name.

    SuiteInfo   *GetSuitesByDisplaynameOrName( char *name );

                //  Return the active entry in the ToC matching name, if any.

    Entry       *GetActiveEntryByName( char *name );

                //  Return the prior entry in the ToC matching name, if any.

    Entry       *GetPriorEntryByName( char *name );

                //  Given a SuiteInfo object, return the ToC entry which
                //  refers to it, if any.

    Entry       *GetEntryFromSuite( SuiteInfo *s );

                //  Get a list of all entries in the ToC.

    Entry       *GetAllEntries( void );

                //  Add an entry to the ToC.  If flush is true, flush
                //  the ToC to persistant storage.

    void        Add( Entry *e, BOOL flush = true );

                //  Update an entry in the ToC and flush the ToC to
                //  persistant storage.

    void        Update( Entry *e );

                //  Remove any entry from the ToC and flush the ToC to
                //  persistant storage.

    void        Remove( Entry *e );

                //  Remove the entries for all inactive packages from the
                //  ToC and flush the ToC to persistant storage.

    BOOL        RemoveInactive( void );

                //  Trace the contents of the ToC.

    void        Dump( void );

                //  Return a list of packages installed on the SP boot
                //  disk without consulting the ToC.

    Entry       *Discover( void );

                //  Mirror UPFs between the SP boot disk and persistant
                //  storage.

    int         Mirror( void );

                // Update info points with the latest information.

    BOOL        RefreshInfo( BOOL NDUInProgress );

                // Find and add an embedded package.

    int         AddEmbedded( char * Package, char * Path );

                // Get the UPF Mirror Directory.

    char *      FindUPFDir( void );

                // Get the attributes of all active packages.

    void        ExportActiveAttributes( AttributeTable & ExportedAttributes );

                // Searches the attributes of active packages for a key and returns the corresponding value.

    const char * GetActiveAttribute( char * Key );

                // Add entries to an existing Xml file for this class instance.

    void        AddXml( NDUXmlOut & XmlFile );

                // Populate this class instance with data from an Xml file.

    BOOL        LoadXml( NDUXmlIn & XmlFile, BOOL NDUInProgress );

private:
	Entry       *entries;
    K10TraceMsg *trace;

    BOOL        ParseToC( BOOL NDUInProgress );

    void        MirrorSaved( char *upfdir, char *ext );
    void        Sync( void );

    void        Sanity( BOOL NDUInProgress );

    Entry       *FoundToC( char *path );
    Entry       *WalkTree( char *path, int level );
    static BOOL        printerrors;
    static BOOL        degraded;
};

//++
//
//  Class:
//      ToC
//
//  Description:
//      This class represents the NDU table of contents (ToC).
//
//  Revision History:
//      15-Nov-01   Karl Owen   Commented.
//
//--

class CSX_CLASS_EXPORT ToC
{
public:
                ToC( void );
                ~ToC( void );

                //  Return the number of entries in the ToC.

    int         GetCount( void );

                //  Return a list of all SuiteInfo objects associated
                //  with ToC entries.

    SuiteInfo   *GetAllSuites( void );

                //  Return a list of all SuiteInfo objects associated
                //  with ToC entries which match name.

    SuiteInfo   *GetSuitesByName( char *name );

                //  Return a list of all SuiteInfo objects associated
                //  with ToC entries which match display name or the name.

    SuiteInfo   *GetSuitesByDisplaynameOrName( char *name );

                //  Return the active entry in the ToC matching name, if any.

    Entry       *GetActiveEntryByName( char *name );

                //  Return the prior entry in the ToC matching name, if any.

    Entry       *GetPriorEntryByName( char *name );

                //  Given a SuiteInfo object, return the ToC entry which
                //  refers to it, if any.

    Entry       *GetEntryFromSuite( SuiteInfo *s );

                //  Get a list of all entries in the ToC.

    Entry       *GetAllEntries( void );

                //  Add an entry to the ToC.  If flush is true, flush
                //  the ToC to persistant storage.

    void        Add( Entry *e, BOOL flush = true );

                //  Update an entry in the ToC and flush the ToC to
                //  persistant storage.

    void        Update( Entry *e );

                //  Remove any entry from the ToC and flush the ToC to
                //  persistant storage.

    void        Remove( Entry *e );

                //  Remove the entries for all inactive packages from the
                //  ToC and flush the ToC to persistant storage.

    BOOL        RemoveInactive( void );

                //  Trace the contents of the ToC.

    void        Dump( void );

                //  Return a list of packages installed on the SP boot
                //  disk without consulting the ToC.

    Entry       *Discover( void );

                //  Mirror UPFs between the SP boot disk and persistant
                //  storage.

    int         Mirror( void );

                // Update info points with the latest information.

    BOOL        RefreshInfo( BOOL NDUInProgress );

                // Find and add an embedded package.

    int         AddEmbedded( char * Package, char * Path );

                // Get the UPF Mirror Directory.

    char *      FindUPFDir( void );

                // Get the attributes of all active packages.

    void        ExportActiveAttributes( AttributeTable & ExportedAttributes );

                // Searches the attributes of active packages for a key and returns the corresponding value.

    const char * GetActiveAttribute( char * Key );

                // Add entries to an existing Xml file for this class instance.

    void        AddXml( NDUXmlOut & XmlFile );

                // Populate this class instance with data from an Xml file.

    BOOL        LoadXml( NDUXmlIn & XmlFile, BOOL NDUInProgress );

private:
    //  Don't try to share the implementation class among all instances, as
    //  there are many complicated locking issues that will need to be handled
    //  first.  See AR 529832.
    ToCImpl *	pImpl;
};

#endif // _ToC_H

