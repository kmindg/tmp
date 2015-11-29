// Copyright (C) 1999	Data General Corporation
//
//	Revision History
//	----------------
//	23 Sep 99	H. Weiner	Add HBA & LU_EXTENDED tags
//	23 Nov 99	D. Zeryck	Mods for Navi review
//  08 Jan 00   B. Yang		Added FrontBus & CMI
//	19 Jan 00	D. Zeryck	Add Frontbus usage param, creator info tags
//	26 Jan 00	D. Zeryck	Add error mask tag
//  06 Feb 00   K. Owen     Add NDU tags.
//  14 Feb 00	B. Yang		Added TAG_K10_FRONTBUS_PLID_SET
//	22 Mar 00	B. Yang		Added tags for K10_SYSTEM_ADMIN_DB_NETWORK
//	 6 APr 00	H. Weiner	Add MAX_TLD_NUMBER_SIZE
//	26 Apr 00	D. Zeryck	Add TAG_K10_NETID_IP_SETUP
//   9 Jun 00   K. Owen     Add degraded mode NDU tags.
//  19 Jun 00   K. Owen     Add NDU skip reboot tag.
//	11 Aug 00	D. Zeryck	Bug 946 - implement drive FW download
//  06 Sep 00   K. Owen     Task 1307 - NDU threading.
//  22 Sep 00   K. Owen     Task 1942 - Support disruptive upgrades.
//   5 Jan 01   H. Weiner   Phase 2: Add TAG_K10_BIOS_PROM
//  16 Mar 01   B. Myers    Defects 11248, 11323, 11328, 11329: Add TAG_K10_NDU_SUITE_INSTALL_INFO
//  19 Apr 01   B. Yang     Gang Trespass - Add TAG_K10_LU_EXTENDED_ATTRIBUTES_GANG_WWN	
//   2 May 01   H. Weiner   Phase 3: Adds for setting port speed to FRONTBUS section
//  16 May 01   H. Weiner   Reserve for clone DLL
//  ************
//	30 Aug 01	C. Hopkins  Moved all tags to file "global_ctl_tags.h", consolidating all
//							our various core tags.  This file (k10Tags.h) remains only so 
//							I don't have to go find all the files that include it.


#include "global_ctl_tags.h"


