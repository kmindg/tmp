//
//	This file is for internal use only!  Don't mess with it unless you
//	understand what you are doing.
//
//	This file is used to construct a fragment of a table of all tag name
//	strings and tag values for use by tools such as DAQ, TLDtest, etc.
//	The client that #includes this file is responsible for declaring the
//	table entry type, the name of the header, and the terminator of the
//	table.  The only requirement is that the table entries must consist
//	of a string field (for the tag name) followed by an integer field
//	(for the tag value).  For example:
//
//		// First, import the definitions of the tag name enumeration literals.
//		#include "global_ctl_tags.h"
//
//		typedef struct
//		{
//			char *	TagName;
//			int		TagVal;
//		} TagTableEntry;
//
//		TagTableEntry MyTableOfTags [] =
//		{
//		#include "_allTagsTable.h"
//		};
//
//
//Revision History:
//
//	20-Dec-2006	Goudreau	Created.
//



//
//	Undefine and redefine the tag definition macros and then re-include
//	the underlying tag values file, this time to produce table entries.
//
#undef TAGVdefine
#define TAGVdefine(TagName, TagValue) { #TagName, TagName },
#undef TAGdefine
#define TAGdefine(TagName) { #TagName, TagName },

#define _TAGS_PART1               1
#define _TAGS_PART2               1
#include "_Tags.h"
#undef _TAGS_PART2
#undef _TAGS_PART1