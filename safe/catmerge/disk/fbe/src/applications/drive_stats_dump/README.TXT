
**********************************************************************
* Copyright (C) EMC Corporation 2012
* All rights reserved.
* Licensed material -- property of EMC Corporation
**********************************************************************
*
* @author
*  1/20/2012 Gerry Fredette - Created
*  2/16/2012 Update for parser version 1.3
*  3/4/2013  Update for parser version 1.4 - Adds Christina's changes for BMS parsing
*  4/16/2013 Armando Vega - Version 1.5 Update to make code compile on both Windows and Linux.
*            Added logic to skip garbage characters in between headers (LINUX artifact).
*            This tool can now parse drives collected on both Linux and Windows using 
*            Either Windows or Linux version of the executable.
*  5/29/2013 Christina Chiang - Version 1.6 Added BMS_DATA1 to handle the BMS with the Total unique BMS 03/xx or 01/xx entries.
*  2/12/2014 Christina Chiang - Version 1.7 Added DCS2 to handle the old format and new format of log page 0x31 for Samsung RDX.
*
* DESCRIPTION
 * DESCRIPTION:
 * The drive_stats_dump tool is useful for parsing the
 * drive_flash_stats.txt file generated on Clariion Arrays running
 * more recent versions of software.
 *
 * Previously this generated file was used to collect just drive fuel
 * gauge data from flash drives.  The resulting ASCII data accumulated
 * as the tool ran every 8 days.  The resulting file in:
 *     c:\dumps\drive_flash_stats.txt
 * would be retrieved as part of an SPCollect. 
 * 
 * The method and schedule is still the same, but the contents of the
 * file has evolved to store binary data and now includes multiple
 * types of data in addition to the fuel gauge data.
 * 
 * The details of the format is defined in the document:
 *     SMART Device Attribute Data Collector.doc
 * 
 * This parsing tool can be used to dump the contents of the new
 * binary file.  It also recognizes the old fuel gauge data format
 * which may be present in the same file.
 *
 * Version 1.3 also recognizes an early version of the file format 
 * which had very limitted distribution.  In this case the output is
 * almost identical. But the headers show magic numbers of DCS0, FRU0 and DAT0 
 * instead of DCS1, FRU1 and DATA.
 * 
 * SETUP:
 * To run this tool, you need to copy the console application
 * drive_stats_dump.exe into a folder that is in your PATH and
 * copy the drive_flash_stats.txt file onto your machine. 
 * Alternatively you can copy the tool onto the SP and execute it
 * against the file in the dumps folder.
 *
 *
 * USAGE:
 * drive_stats_dump [override_file_name [-m printmask]]
 *
 *  Where:
 *
 *   input_file_name - name of file containing the binary drive stats. 
 *   printmask - is a bit mask to enable various output. You pass a value in decimal
 *               which corresponds to the bits you want to output:
 *         1 - PRT_SESSION  print session headers
 *         2 - PRT_FRUS     print fru headers
 *         4 - PRT_DATAHEAD print the data header 
 *         8 - PRT_FORMATTED_DATA  print using formatted SMART data.
 *        16 - PRT_RAW_DATA print using raw output
 *       256 - PRT_DEBUG_FLAG print with additional Debug output
 *
 *   default input_file_name is "drive_flash_stats.txt"
 *   default printmask is 15 (i.e. PRT_SESSION | PRT_FRUS | PRT_DATAHEAD | PRT_FORMATTED_DATA)
 *
 * note: FORMATTED (8) and RAW (16) are mutually exclusive.  If both are
 *   specified, you will get RAW output.


*
* examples:
*   To print just the headers without the detailed data:
*       drive_stats_dump drive_flash_stats.txt 7
*
*   To print just the Session and Fru headers from the file in the dumps folder:
*       drive_stats_dump c:\dumps\drive_flash_stats.txt 3 
*
*   To print the standard headers, but output the data in raw format:
*       drive_stats_dump drive_flash_stats.txt 23
*
*   To print just the raw data from a file:
*       drive_stats_dump  my_backup_file 16
*
*   To print just the default output for a file on a remote share:
*       drive_stats_dump  "\\mrfile\AE_VOL\FBE Physical\Drive Control\SMART data\drive_flash_stats_parser\example_drive_flash_stats.txt"
* 
*
**********************************************************************
