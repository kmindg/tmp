// win32_drive_stats_dump.cpp : Defines the entry point for the console application.
//


/**********************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 **********************************************************************/

/*!*********************************************************************
 * @file win32_drive_stats_dump.c
 **********************************************************************
 *
 * @brief This utility is used to dump out the binary data collected 
 *        using the drive_flash_stats logic in Clariion array software.
 *        This is a 32-bit windows console application. 
 *        Details of the file format this utility parses is found in the
 *        document: "SMART Device Attribute Data Collector.doc"
 *
 * DESCRIPTION:
 * The win32_drive_stats_dump tool is useful for parsing the
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
 * SETUP:
 * To run this tool, you need to copy the Windows console application
 * win32_drive_stats_dump.exe into a folder that is in your PATH and
 * copy the drive_flash_stats.txt file onto your machine. 
 * Alternatively you can copy the tool onto the SP and execute it
 * against the file in the dumps folder.
 *
 *
 * USAGE:
 * win32_drive_stats_dump [override_file_name [printmask]]
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
 *   specified, you will get FORMATTED output.
 *
 * examples:
 *   To print just the headers without the detailed data:
 *       win32_drive_stats_dump drive_flash_stats.txt 7
 *
 *   To print just the Session and Fru headers from the file in the dumps folder:
 *       win32_drive_stats_dump c:\dumps\drive_flash_stats.txt 3 
 *
 *   To print the standard headers, but output the data in raw format:
 *       win32_drive_stats_dump drive_flash_stats.txt 23
 *
 *   To print just the raw data from a file:
 *       win32_drive_stats_dump  my_backup_file 16
 *
 *   To print just the default output for a file on a remote share:
 *       win32_drive_stats_dump  "\\mrfile\AE_VOL\FBE Physical\Drive Control\SMART data\drive_flash_stats_parser\example_drive_flash_stats.txt"
 *
 *
 * @author
 *  1/20/2012 Gerry Fredette -   Created
 *  2/10/2012 Gerry Fredette -   Version 1.2 fixes to various issues.
 *  2/16/2012 Gerry Fredette -   Version 1.3 adds support for "DCS" variation.
 *  3/2/2013  Christina Chiang - Version 1.4 add support for BMS normalized data.
 *  4/16/2013 Armando Vega -     Version 1.5 Updated to compile on Win/Linux. Updated parser to skip garbage data between headers (for LINUX).
 *  5/29/2013 Christina Chiang - Version 1.6 Added BMS_DATA1 to handle the BMS with the Total unique BMS 03/xx or 01/xx entries.
 *  2/12/2014 Christina Chiang - Version 1.7 Added DCS2 to handle the old format and new format of log page 0x31 for Samsung RDX.
 *
 *************************************************************************************************************************************/


//#include "stdafx.h"
#include "targetver.h"

#include <stdio.h>


#include <string.h>
#include <ctype.h>
#include <stdlib.h>

typedef struct magicnum_s {
    unsigned char number[4];
} magicnum_t;

typedef struct standard_head_s {
    magicnum_t  magic;
    unsigned char length[2];
} standard_head_t;

typedef struct revison_s {
    unsigned char major;
    unsigned char minor;
} revison_t;

typedef struct timestamp_s {
    unsigned char year;
    unsigned char month;
    unsigned char day;
    unsigned char hour;
    unsigned char min;
    unsigned char sec;
} timestamp_t;

typedef struct session_head_s {
    standard_head_t stdhead;
    unsigned char tool_id;
    revison_t file_format_version;
    revison_t tool_version;
    timestamp_t creation_time_stamp;
} session_head_t;

typedef struct fru_head_s {
    standard_head_t stdhead;
    unsigned char fru_number[4];
    unsigned char b_e_d[3];
    unsigned char serial_number[21];
    unsigned char tla_number[13];
    unsigned char fw_rev[10];
    timestamp_t creation_time_stamp;
} fru_head_t;

typedef struct data_head_s {
    standard_head_t stdhead;
    unsigned char variant_id;
} data_head_t;

/* physical drive BMS Scan Summary */
typedef struct bms_data_s
{
    timestamp_t    time_stamp;
    unsigned char  num_bg_scans_performed[2];
    unsigned char  total_entries[2];
	unsigned char  total_iraw;
    unsigned char  worst_head;
    unsigned char  worst_head_entries[2];
    unsigned char  total_03_errors[2];
    unsigned char  total_01_errors[2];
    unsigned char  time_range[4];
    unsigned char  drive_power_on_minutes[4]; 
} bms_data_t;

typedef struct bms_data1_s
{
    timestamp_t    time_stamp;
    unsigned char  num_bg_scans_performed[2];
    unsigned char  total_entries[2];
	unsigned char  total_iraw;
    unsigned char  worst_head;
    unsigned char  worst_head_entries[2];
    unsigned char  total_03_errors[2];
    unsigned char  total_01_errors[2];
    unsigned char  time_range[4];
    unsigned char  drive_power_on_minutes[4];
	unsigned char  total_unique_03_errors[2];
    unsigned char  total_unique_01_errors[2];
} bms_data1_t;
#define FALSE 0
#define TRUE !FALSE

#define END_OF_FILE_TYPE        0
#define UNKNOWN_HEADER_TYPE     1
#define SESSION_HEADER_TYPE     2
#define FRU_HEADER_TYPE         3
#define OLD_FUEL_GAUGE_TYPE     4
#define DATA_HEADER_TYPE        5
#define DCS0_SESSION_HEADER_TYPE 6  /* This is an exception header */
#define DCS0_FRU_HEADER_TYPE     7  /* This is an exception header */
#define DCS0_DATA_HEADER_TYPE    8  /* This is an exception header */
#define DCS1_SESSION_HEADER_TYPE 9  /* Write buffer = 516. */


/* The following must match the production code values
 * Specifically these must match CM_DRIVE_STATS_DATA_VARIANT_ID which is
 * defined in cm_local.h.
 */
#define VARIANT_ID_UNKNOWN          0
#define VARIANT_ID_SMART_DATA       1
#define VARIANT_ID_FUEL_GAUGE_DATA  2
#define VARIANT_ID_BMS_DATA         3 
#define VARIANT_ID_BMS_DATA1        4 /* New BMS format with total unique 03 and 02 entries.*/

int get_header_type(FILE *fp, session_head_t *session, fru_head_t *fru, data_head_t *data, unsigned char *raw_buffer, int *ret_count, int printmask);
void dump_session_header(session_head_t *session);
char *format_stdhead(standard_head_t *head);
char *format_timestamp(timestamp_t *ts);
void dump_fru_header(fru_head_t *fru);
void dump_data_header(data_head_t *data);
char *cvt_varient_id(int variant_id);
unsigned short cvt_to_short(unsigned char *num, int swap);
unsigned long cvt_to_long(unsigned char *num, int swap);
unsigned long long cvt_value_to_ll(unsigned char *num, int swap, int numbytes);
int get_and_dump_varient(FILE *fp, data_head_t *data, int printmask, int dcs_header_type);
int get_data(FILE *fp, unsigned char *data_buffer, int length);
void raw_dump(unsigned char *data_buffer, int length, char *indent);
void page30_formatted_dump(unsigned char *data_buffer, int length);
void page31_formatted_dump(unsigned char *data_buffer, int length, int dcs_header_type);
/*/unsigned long long cvt_value_to_longl(unsigned char *num, int swap, int numbytes);*/
unsigned char *format_attribute(unsigned char *out_buffer, unsigned char *in_buffer);
void dump_old_fuel_gauge(unsigned char *data_buffer, int length);
char *format_bms_data_page_header(unsigned char *data_buffer);
void page15_formatted_dump(unsigned char *data_buffer, int length);
void page15_formatted_dump1(unsigned char *data_buffer, int length);

#define BIT(b) (1 << (b))

#define PRT_SESSION         BIT(0)
#define PRT_FRUS            BIT(1)
#define PRT_DATAHEAD        BIT(2)
#define PRT_FORMATTED_DATA  BIT(3)  /* FORMATTED and RAW data output is mutually exclusive*/
#define PRT_RAW_DATA        BIT(4)
#define PRT_DEBUG_FLAG      BIT(8)   

#define PRT_ALL     (PRT_SESSION | PRT_FRUS | PRT_DATAHEAD | PRT_FORMATTED_DATA)

#define DEFAULT_INPUT_FILE_NAME  (const char *) "c:\\drive_flash_stats.txt"

#define RAW_DATA_INDENT "      "
#define SWAP        TRUE
#define DONTSWAP    FALSE

#define DATA_BUFFER_SIZE  1024
#define DATA_BUFFER_SIZE_DCS1 516

static unsigned char my_data_buffer[DATA_BUFFER_SIZE];

#define DRIVE_STATS_DUMP_VERSION  "1.7"



/*!****************************************************************************
 * @fn      main()
 *
 * @brief
 *  This main function implements the main driver of the drive_stats_dump utility.
 *
 * @param argc - standard argument convention for number of arguments
 * @param argv - standard argument convention for argument strings
 *
 * @return
 *  None
 *
 * HISTORY
 *  01/20/12 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/

int main(int argc, char **argv)
{
    FILE *fp;
    int header_type;
	int DCS1_header_type;
    int printmask;
    int length;
    session_head_t session;
    fru_head_t fru;
    data_head_t data;
    char infile[256];
	//int argcnt;
	//char *ptr;

	DCS1_header_type = 0;

    /* default input file */    
    strncpy (infile, DEFAULT_INPUT_FILE_NAME, sizeof(infile));

    printmask = PRT_ALL;  /* Print everything */

    printf("drive_stats_dump version %s\n", DRIVE_STATS_DUMP_VERSION);

    if (argc > 1)
    {
        argv++;        
        strncpy (&infile[0], (const char *) (*argv++), sizeof(infile));
        printf("Override on input file %s\n", infile);
    }

    if (argc > 2)
    {
        printmask = atoi((const char *) (*argv++));
        printf("Override on printmask %d\n", printmask);
    }


    if ((fp = fopen(infile, "rb")) == NULL)
    {
        printf("ERROR: Failed to open input file: %s\n", infile);
        return -1;
    }

    while(!feof(fp))
    {
		memset(my_data_buffer, 0, sizeof(my_data_buffer));
		memset((char *) &session, 0, sizeof(session));
		memset((char *) &fru, 0, sizeof(fru));
		memset((char *) &data, 0, sizeof(data));
        if ((header_type = get_header_type(fp, &session, &fru, &data, my_data_buffer, &length, printmask)) < 0)
        {
            printf("ERROR: failed to parse session_header from file\n");
            return -1;
        }
        switch(header_type) {
            case OLD_FUEL_GAUGE_TYPE:
                if (printmask & PRT_SESSION)
                {
                    dump_old_fuel_gauge(my_data_buffer, length);
                }
                break;
            case DCS0_SESSION_HEADER_TYPE:
            case SESSION_HEADER_TYPE:
			case DCS1_SESSION_HEADER_TYPE:
				/* DCS1_header_type = 1 for the old format, DCS1_header_type = 0 for the new format. */
				DCS1_header_type = (header_type == DCS1_SESSION_HEADER_TYPE)? 1: 0;
                if (printmask & PRT_SESSION)
                {
                    dump_session_header(&session);
                }
            break;
            
            case DCS0_FRU_HEADER_TYPE:
            case FRU_HEADER_TYPE:
                if (printmask & PRT_FRUS)
                {
                    dump_fru_header(&fru);
                }
            break;

            case DCS0_DATA_HEADER_TYPE:
            case DATA_HEADER_TYPE:
                if (printmask & PRT_DATAHEAD)
                {
                    dump_data_header(&data);
                }
                if (get_and_dump_varient(fp, &data, printmask, DCS1_header_type))
                {
                    printf("ERROR: Data variant corrupt\n");
                    return -1;
                }
            break;

            case END_OF_FILE_TYPE:
                /* nothing to do except exit cleanly.*/
                printf("hit eof\n");
            break;


            default:
                printf("ERROR: Bad header type. File is corrupt\n");
                return -1;
            break;
        }
    }

    fclose(fp);
	return 0;
}

/*!****************************************************************************
 * @fn      is_header_char()
 *
 * @brief
 * Helper function to identify if a character is part of a valid header.
 * This method need to be updated if new header tags are added.
 *
 * @param header_char character to check against valid header char.
 *
 * @return
 *  1 if is valid, 0 otherwise.
 *
 * HISTORY
 *  04/16/13 :  Armando Vega -- Created.
 *
 *****************************************************************************/
int is_header_char(char header_char)
{
    int is_valid = 0;
    if ((header_char == '0') || 
        (header_char == '1') || 
		(header_char == '2') || 
        (header_char == 'A') || 
        (header_char == 'C') || 
        (header_char == 'D') || 
        (header_char == 'E') || 
        (header_char == 'F') || 
        (header_char == 'I') || 
        (header_char == 'M') || 
        (header_char == 'R') || 
        (header_char == 'S') || 
        (header_char == 'T') || 
        (header_char == 'U'))
    {
        is_valid = 1;
    }
    return is_valid;
}

/*!****************************************************************************
 * @fn      get_header_type()
 *
 * @brief
 * This function reads the first few bytes of the file and determines
 * the type of header.  reads in the remainder to a return buffer and
 * returns a header type to the caller.
 *
 * @param fp - file to read.
 * @param session - output buffer if the header is a session header.
 * @param fru - output buffer if the header is a fru header
 * @param data - output buffer if the header is a data header
 * @param raw_buffer - if the header is an old fuel gauge entry
 * @param ret_count - number of bytes returned. Only used for old fuel gauge
 * @param printmask - bit mask indicating output masking
 *
 * @return
 *  header type found by parsing the input file.
 *
 * HISTORY
 *  01/20/12 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
int get_header_type(FILE *fp, session_head_t *session, fru_head_t *fru, data_head_t *data, unsigned char *raw_buffer, int *ret_count, int printmask)
{
    unsigned char *buf;
    unsigned char header[4];
    unsigned char dcs0_fru[2];
    unsigned char dcs0_data[4];
    unsigned char tool_id;
    int i, retheader, length;
    int find_chrs, done, headerFound;
    unsigned char chr_to_find;    
    buf = header;
    headerFound = 0;
    
    for (i = 0; i < sizeof(header); i++)
    {
        if (!feof(fp))
        {
            *buf = fgetc(fp);

            // Skip any invalid header characters on file (occurs on LINUX)            
            while (!is_header_char(*buf) && !feof(fp))
            {
                *buf = fgetc(fp);                
            }

            if (feof(fp) && i == 0)
            {
                return END_OF_FILE_TYPE;
            }

            if (printmask & PRT_DEBUG_FLAG)
            {
                printf("chr %d is (0x%x)\n", i, *buf);
            }            
            buf++;            
            
        }        

        if (i == 2 && header[0] == 01 && header[1] == 02 && header[2] == 00)
        {
            /* We have a special case when dealing with the old style data buffer
             * We only want to read in 3 bytes since the rest is the disk log page.
             */
            break;
        }
    }

    if (feof(fp))
    {
        printf("ERROR: File is too short\n");
        return UNKNOWN_HEADER_TYPE;
    }

    find_chrs = FALSE;
    /* This first two entries are to handle an intrim version of the tool which used
     * a different format.  This code should never be in the field, but ended up
     * on some lab machines.  Therefore it is handled as an exception.
     */
    if (header[0] == 01 && header[1] == 02 && header[2] == 00)
    {
        /* We have a valid old style DATA header with only 3 bytes in header */
        retheader = DCS0_DATA_HEADER_TYPE;
        dcs0_data[0] = header[0];
        dcs0_data[1] = header[1];
        dcs0_data[2] = header[2];
        header[0] = 'D';
        header[1] = 'A';
        header[2] = 'T';
        header[3] = '0'; /* Indicate old style header */
        buf = (unsigned char *) data;
        length =  0;
    }
    else if (header[0] == 'D' && header[1] == 'C' && header[2] == 'S' && header[3] == 01)
    {
        /* this is a session header */
        retheader = DCS0_SESSION_HEADER_TYPE;
        buf = (unsigned char *) session;
        length =  sizeof(session_head_t);
        headerFound = 1;
    }
    else if (header[0] == 0 && header[1] == 62)
    {
        /* We have a valid old style FRU header */
        retheader = DCS0_FRU_HEADER_TYPE;
        dcs0_fru[0] = header[2];
        dcs0_fru[1] = header[3];
        header[0] = 'F';
        header[1] = 'R';
        header[2] = 'U';
        header[3] = '0'; /* Indicate old style header */
        buf = (unsigned char *) fru;
        length =  sizeof(fru_head_t);
        headerFound = 1;
    }
    else if (header[0] == 'D' && header[1] == 'C' && header[2] == 'S' && header[3] == '1')
    {
        /* this is a session header for the write buffer with the old format */
        retheader = DCS1_SESSION_HEADER_TYPE;
        buf = (unsigned char *) session;
        length =  sizeof(session_head_t);
        headerFound = 1;
    }
	else if (header[0] == 'D' && header[1] == 'C' && header[2] == 'S' && header[3] == '2')
    {
        /* this is a session header with the New format  */
        retheader = SESSION_HEADER_TYPE;
        buf = (unsigned char *) session;
        length =  sizeof(session_head_t);
        headerFound = 1;
    }
    else if (header[0] == 'F' && header[1] == 'R' && header[2] == 'U' && header[3] == '1')
    {        
        /* this is a fru header */
        retheader = FRU_HEADER_TYPE;
        buf = (unsigned char *) fru;
        length =  sizeof(fru_head_t);
        headerFound = 1;
    }
    else if (header[0] == 'D' && header[1] == 'A' && header[2] == 'T' && header[3] == 'A')
    {
        /* this is a fru header */
        retheader = DATA_HEADER_TYPE;
        buf = (unsigned char *) data;
        length =  sizeof(data_head_t);
        headerFound = 1;
    }
    else if (header[0] == 'T' && header[1] == 'I' && header[2] == 'M' && header[3] == 'E')
    {
        /* this is an old style Fuel Gauge header */
        retheader = OLD_FUEL_GAUGE_TYPE;
        buf = raw_buffer;
        find_chrs = TRUE;
        length =  3; /* How many characters to look for */
        chr_to_find = 0x0a;
        headerFound = 1;
    }
    else
    {
        printf("ERROR: Header bytes do not match valid one, found %c,%c,%c\n",header[0],header[1],header[2]);
        return UNKNOWN_HEADER_TYPE;
    }    

    *buf++ = header[0];
    *buf++ = header[1];
    *buf++ = header[2];
    if (retheader == DCS0_SESSION_HEADER_TYPE)
    {
        *buf++ = '0'; /* Use this to indicate the alternate (older) DCS header */
        
        tool_id = header[3]; /* get tool_id */
        *buf++ = fgetc(fp);  /* get length high */
        *buf++ = fgetc(fp);  /* get length low */
        *buf++ = tool_id;
        length -= 3 ; /*  adjust length so we can get the rest below since find_chrs is FALSE.*/
    }
    else
    {
        *buf++ = header[3];  /* This is the 4th byte of the standard header.*/
    }

    if (retheader == DCS0_FRU_HEADER_TYPE)
    {
        *buf++ = 0;
        *buf++ = 62;
        *buf++ = dcs0_fru[0];
        *buf++ = dcs0_fru[1];
        length -= 4;
        /* adjust the length so we can get the rest below since find_chrs is FALSE */
    } else if (retheader == DCS0_DATA_HEADER_TYPE)
    {
        *buf++ = dcs0_data[1];
        *buf++ = dcs0_data[2] + 0xb; /* adjust the length that we ask the dumper to dump */
        *buf++ = dcs0_data[0];
    }
    if (find_chrs)
    {
        i = 4;  /* How many characters we already processed */
        done = FALSE;
        while(i < DATA_BUFFER_SIZE && !done)
        {
            if (!feof(fp))
            {
                *buf = fgetc(fp);
                i++;
                if (*buf == chr_to_find)
                {
                    if (--length == 0)
                    {
                        done = TRUE;
                    }
                }
#if 0
                printf("chr %d is (0x%x)\n", i, *buf); 
#endif
                buf++;
            }
            else
            {
                break;
            }
        }
        if (!done)
        {
            printf("ERROR: Unexpected end of file found\n");
            return UNKNOWN_HEADER_TYPE;
        }
        *ret_count = i;
    }
    else
    {
        for (i = 4; i < length; i++)
        {
            if (!feof(fp))
            {
                *buf = fgetc(fp);
#if 0
                printf("chr %d is (0x%x)\n", i, *buf); 
#endif
                buf++;
            }
        }
    }
    if (!feof(fp))
    {
        return retheader;
    }
    else
    {
        printf("ERROR: File end found\n");
        return UNKNOWN_HEADER_TYPE;
    }
}




/*!****************************************************************************
 * @fn      dump_session_header()
 *
 * @brief
 * This function prints out a standard session header for
 * file_format_version 01:00.  
 *
 * @param session - pointer to session header structure.
 *
 * @return
 *  None
 *
 * HISTORY
 *  01/20/12 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
void dump_session_header(session_head_t *session)
{
          /*SESSION: DCS1 0017 01 01:00 01:00 2012:01:19-12:32 */
    printf("SESSION: %s %02d %02d:%02d %02d:%02d %s\n", 
        format_stdhead(&session->stdhead),
        session->tool_id,

        session->file_format_version.major,
        session->file_format_version.minor,
        session->tool_version.major,
        session->tool_version.minor,

        format_timestamp(&session->creation_time_stamp));
}


/*!****************************************************************************
 * @fn      format_stdhead()
 *
 * @brief
 *  This function dumps a standard data header.
 *
 * @param head points to the data header
 *
 * @return
 *  pointer to static print buffer containing output data header string.
 *
 * HISTORY
 *  01/24/12 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
char *format_stdhead(standard_head_t *head)
{
    static char standard_header_prtbuffer[16];

    sprintf(standard_header_prtbuffer, "%c%c%c%c %05d", 
        head->magic.number[0],
        head->magic.number[1],
        head->magic.number[2],
        head->magic.number[3],
        cvt_to_short(head->length, SWAP)
    );
    return standard_header_prtbuffer;
}



/*!****************************************************************************
 * @fn      format_timestamp()
 *
 * @brief
 *  This function formats the timestamp into a human readable format.
 *
 * @param ts - pointer to timestamp bytes.
 *
 * @return
 *  pointer to static buffer containing output time stamp string.
 *
 * @note
 *  This function is NOT re-entrant.  Only one call to this function should be
 *  made for each call to printf. Otherwise caller should copy the output data
 *  to a private buffer.
 *
 * HISTORY
 *  01/20/12 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
char *format_timestamp(timestamp_t *ts)
{
    static char ts_buffer[32];

    /* 2012/12/31-12:12:59*/
    sprintf((char *)&ts_buffer, "%04d/%02d/%02d-%02d:%02d:%02d", 
        ts->year+2000,
        ts->month,
        ts->day,
        ts->hour,
        ts->min,
        ts->sec);
    return ts_buffer;
}


/*!****************************************************************************
 * @fn      dump_fru_header()
 *
 * @brief
 * This function prints out a standard fru header for
 * file_format_version 01:00.  
 *
 * @param session - pointer to fru header structure.
 *
 * @param None
 *
 * @return
 *  None
 *
 * HISTORY
 *  01/20/12 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
void dump_fru_header(fru_head_t *fru)
{
    printf("  FRU: %s %4lu %1d_%02d_%03d %21s %13s %10s %s\n",
        format_stdhead(&fru->stdhead),
        cvt_to_long(fru->fru_number, SWAP),
        fru->b_e_d[0], fru->b_e_d[1], fru->b_e_d[2],
        fru->serial_number,
        fru->tla_number,
        fru->fw_rev,
        format_timestamp(&fru->creation_time_stamp)
    );
}



/*!****************************************************************************
 * @fn      void dump_data_header()
 *
 * @brief
 *  This function prints a data buffer
 *
 * @param None
 *
 * @return
 *  None
 *
 * HISTORY
 *  01/24/12 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
void dump_data_header(data_head_t *data)
{
    printf("    DATA: %s %s\n",
        format_stdhead(&data->stdhead),
        cvt_varient_id(data->variant_id)
    );
}


/* This function needs to map to a string based on the values in the
 * production code.*/



/*!****************************************************************************
 * @fn      cvt_varient_id()
 *
 * @brief
 *  This function maps a variant identifier into a printable string.
 *
 * @param variant_id - identifier as defined by the file format document. This
 * code needs to match the supported codes.
 *
 * @return
 *  varian_id string.
 *
 * HISTORY
 *  01/20/12 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
char *cvt_varient_id(int variant_id)
{
    if (variant_id == VARIANT_ID_SMART_DATA)
        return "SMART_DATA";
    else if (variant_id == VARIANT_ID_BMS_DATA)
        return "BMS_DATA";
	else if (variant_id == VARIANT_ID_BMS_DATA1)
        return "BMS_DATA1";
    else if (variant_id == VARIANT_ID_FUEL_GAUGE_DATA)
        return "FUEL_GAUGE";
    else 
        return "UNKNOWN";
}



/*!****************************************************************************
 * @fn      cvt_to_short()
 *
 * @brief
 *  This function converts a pair of bytes into an unsigned short integer.
 * This assumes the bytes are in Big endian order.
 *
 * @param num - pointer to first of the two sequential bytes.
 *
 * @return
 *  number represented by the two bytes.
 *
 * HISTORY
 *  01/20/12 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
unsigned short cvt_to_short(unsigned char *num, int swap)
{
    if (swap)
    {
        return (unsigned short) (num[0] << 8) | 
            num[1];
    }
    else
    {
        return (unsigned short) (num[1] << 8) | 
            num[0];
    }

}



/*!****************************************************************************
 * @fn      cvt_to_long()
 *
 * @brief
 *  This function converts a sequenc of four bytes into an unsigned long integer.
 * This assumes the bytes are in Big endian order.
 *
 * @param num - pointer to first of the four sequential bytes.
 *
 * @return
 *  number represented by the four bytes.
 *
 * HISTORY
 *  01/20/12 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
unsigned long cvt_to_long(unsigned char *num, int swap)
{
    if (swap)
    {
        return (unsigned long) 
            ((num[0] << 24) | 
             (num[1] << 16) |
             (num[2] << 8) |
              num[3]);
    }
    else
    {
        return (unsigned long) 
            ((num[3] << 24) | 
             (num[2] << 16) |
             (num[1] << 8) |
              num[0]);
    }
}




/*!****************************************************************************
 * @fn      get_and_dump_varient()
 *
 * @brief
 *  This function reads in the varient part of the data block. 
 *
 * @param fp - file to read.
 * @param fru - pointer to fru header filled in by caller.
 * @param printmask - bit mask indicating output masking
 * @param dcs_header_type - 0 (New format_, 1 (Old Format)
 *
 * @get_and_dump_varient
 *  TRUE - there was an error.
 *  FALSE - if not error.
 *
 * HISTORY
 *  01/20/12 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
int get_and_dump_varient(FILE *fp, data_head_t *data, int printmask, int dcs_header_type)
{
    int size;
    int length = cvt_to_short(data->stdhead.length, SWAP);

#if 0
    printf("Length from the header is: %d\n", length);
#endif

    length -= sizeof(data_head_t); /* The size includes the extra bytes for the data header itself.*/
    
	if ((data->variant_id == VARIANT_ID_FUEL_GAUGE_DATA) && (dcs_header_type == 1) && (length > 200))
	{
		/* Special handle for the Fuel gauge (0x31) was truncated for 12 bytes.*/
		length -= 12;
	}

    if (length > DATA_BUFFER_SIZE)
    {
        printf("ERROR: File is corrupt or has too large a data variant\n");
        return TRUE;
    }

	memset(my_data_buffer, 0, sizeof(my_data_buffer));
    size = get_data(fp, my_data_buffer, length);
    if (size != length)
    {
		printf("ERROR: unexpected size mismatch. Exp: %d, Rcv: %d\n", length, size);
    }

    if (printmask & PRT_RAW_DATA)
    {
        raw_dump(my_data_buffer, length, RAW_DATA_INDENT);
    }
    else if (printmask & PRT_FORMATTED_DATA)
    {
        switch (data->variant_id)
        {
            case VARIANT_ID_SMART_DATA:
                page30_formatted_dump(my_data_buffer, length);
            break;

            case VARIANT_ID_FUEL_GAUGE_DATA:
                page31_formatted_dump(my_data_buffer, length, dcs_header_type);
            break;

            case VARIANT_ID_BMS_DATA:
                page15_formatted_dump(my_data_buffer, length);
            break;

			case VARIANT_ID_BMS_DATA1:
                page15_formatted_dump1(my_data_buffer, length);
            break;

            case VARIANT_ID_UNKNOWN:
            default:
                printf("ERROR: Unknown format for data block\n");
            break;
        }

    }
    return FALSE;
}


/*!****************************************************************************
 * @fn      get_data()
 *
 * @brief
 *  This function reads in length bytes of data from the file. 
 *
 * @param fp - file to read.
 * @param data_buffer - pointer buffer to store the read in data.
 * @param lenght - how many bytes to read
 *
 * @return
 *  -1 if the file ends before the expected length bytes are read.
 *  otherwise the number of bytes read in (i.e. length)
 *
 * HISTORY
 *  01/20/12 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
int get_data(FILE *fp, unsigned char *data_buffer, int length)
{
    int i = 0;

    while(!feof(fp) && i < length)
    {
        *data_buffer++ = fgetc(fp);
        i++;
    }

    if (feof(fp) && i != (length - 1))
    {
        return i;
    }
    else
    {
        return i;
    }
}


/*!****************************************************************************
 * @fn      raw_dump()
 *
 * @brief
 *  This function prints a block of data in hex and ASCII.  
 *
 * @param data_buffer - pointer to buffer to print.
 * @param length - number of bytes to print.
 *
 * @return
 *  None
 *
 * HISTORY
 *  01/20/12 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
void raw_dump(unsigned char *data_buffer, int length, char *indent)
{
    unsigned char buffer[16];
    unsigned char chr;
    int i,j,k;

    i = 0;
    j = 0;

    printf("%s", indent);
    while(i < length)
    {
        chr = *data_buffer++;
        printf("%02x ", chr);       /* Print the hex value of the data */
        i++; /* total count */
        buffer[j++] = chr;
        if (j == 16)
        {   /* Print the ASCII of the data */
            printf("  ");
            for (k = 0; k < 16; k++)
            {
                if (isprint(buffer[k]))
                {
                    printf("%c ", buffer[k]);
                }
                else
                {
                    printf(". ");
                }
            }
            j = 0;
            printf("\n");
            printf("%s", indent);
        }
    }
    if (j != 0)
    {
        printf("  ");
        for (k = j; k < 16; k++)
        {
            printf("   ");
        }
        for (k = 0; k < j; k++)
        {
            if (isprint(buffer[k]))
            {
                printf("%c ", buffer[k]);
            }
            else
            {
                printf(". ");
            }
        }
        printf("\n");
    }


    printf("\n");
}


/*!****************************************************************************
 * @fn      format_smart_data_page_header(unsigned char *data_buffer)
 *
 * @brief
 *  This function formats the SMART data header.
 *
 * @param None
 *
 * @return
 *  None
 *
 * HISTORY
 *  01/24/12 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
char *format_smart_data_page_header(unsigned char *data_buffer)
{
    static char smart_data_output_buffer[64];

    sprintf(smart_data_output_buffer, "    page:%02x subpage: %02x length:%04dd", 
        data_buffer[0],
        data_buffer[1],
        cvt_to_short(&data_buffer[2], SWAP));
    return smart_data_output_buffer;
}


/*!****************************************************************************
 * @fn      page30_formatted_dump()
 *
 * @brief
 * This function prints out the FUEL GAUGE attribute data in a pretty
 * formatted output.
 *
 * @param data_buffer - pointer to attribute data buffer.
 * @param lenght - length of the attribute data buffer.
 *
 * @return
 *  None
 *
 * HISTORY
 *  01/20/12 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
void page30_formatted_dump(unsigned char *data_buffer, int length)
{
    unsigned char buffer[64];
    int i;

    i = 0;

    printf("%s rev:%02x,%02x\n", 
        format_smart_data_page_header(data_buffer),
        data_buffer[4], data_buffer[5]);
    data_buffer += 6;
    for (i = 0; i < 30; i++)
    {
        printf("      %s\n", format_attribute(buffer, data_buffer));
        data_buffer += 12;
    }
    printf("      offl:%02x selfexe:%02x time:%05dd, vend:%02x cap:%02x,%02x%02x errlog:%02x selfchkpt:%02x %dmin %dmin chksum %02x\n",
        data_buffer[0], /* off-line data collection status */
        data_buffer[1], /* Self-test execution status */
        cvt_to_short(&data_buffer[2], SWAP), /* Total time in seconds to complete off-line data collectoin activity*/
        data_buffer[4], /*Vendor Specific*/
        data_buffer[5], /*Off-line data collectoin capability*/
        data_buffer[6], data_buffer[7],/*SMART capability */
        data_buffer[8], /*Error logging capability 7-1 Reserved 0 1 =Device error logging supported*/
        data_buffer[9], /*Self-test failure checkpoint*/
        data_buffer[10],/*Short Self-test routine recommended polling time (in minutes)*/
        data_buffer[11], /*Extended self-test routine recommended polling time (in minutes)*/
        data_buffer[149]
    );
}



/*!****************************************************************************
 * @fn      page31_formatted_dump()
 *
 * @brief
 * This function prints out the SMART data attribute data in a pretty
 * formatted output.
 *
 * @param data_buffer - pointer to attribute data buffer.
 * @param lenght - length of the attribute data buffer.
 * @param dcs_header_type - 0 (New format_, 1 (Old Format)
 *
 * @return
 *  None
 *
 * HISTORY
 *  01/24/12 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
void page31_formatted_dump(unsigned char *data_buffer, int length, int dcs_header_type)
{
    int data_length;
	unsigned int param_code;
	printf("%s\n", 
        format_smart_data_page_header(data_buffer));

    length -= 4;

    while (length > 0)
    {
        param_code =  cvt_to_short(&data_buffer[4], SWAP);
		data_length = data_buffer[7] +4;

		if ((param_code == 0x8ffe) && (dcs_header_type == 1))
		{
			/* The max write buffer size was set 516 in DCS1. Log page 0x31 will over 516 for some drive type (like Samsung RDX).
	           12 bytes was truncated. */
				printf("      Parameter code:%04xh %02xh %02x Data Truncated.\n", 
                param_code, 
                data_buffer[6], data_buffer[7]
              );

			  length -= data_length;
              data_buffer += data_length;
			  break;
		}
		else if (data_buffer[7] == 4)
		{
			/* New format for parameter code 0x8ffe and 0x8ff. */
			printf("      Parameter code:%04xh %02xh %02x Value: 0x%012lxh (%15lu)\n", 
                param_code, 
                data_buffer[6], data_buffer[7],
                cvt_to_long(&data_buffer[8], SWAP),
                cvt_to_long(&data_buffer[8], SWAP)
             );
		}
		else /* parameter code size is 8. */
		{
			printf("      Parameter code:%04xh %02xh %02x Value: 0x%012llxh (%15lld)\n", 
                param_code, 
                data_buffer[6], data_buffer[7],
                cvt_value_to_ll(&data_buffer[8], DONTSWAP, 8),
                cvt_value_to_ll(&data_buffer[8], DONTSWAP, 8)
             );
		}
	
        length -= data_length;
        data_buffer += data_length;
    }
}


/*!****************************************************************************
 * @fn      cvt_value_to_ll()
 *
 * @brief
 *  This function converts a 6 byte attribute field into a long long. This
 * assumes the data is in little endian format and so it swaps the bytes around.
 *
 * @param num - pointer to 1st of the 6 bytes to be converted.
 *
 * @return
 *  resulting number as a long long.
 *
 * HISTORY
 *  01/20/12 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
unsigned long long cvt_value_to_ll(unsigned char *num, int swap, int numbytes)
{
    unsigned long long retvalue=0;
    int shiftvalue = 8 * (numbytes - 1);
    int i;
    
    if (swap)
    {
        for (i = numbytes-1; i >= 0; i--, shiftvalue -= 8)
        {
            retvalue |= (((unsigned long long )num[i]) << shiftvalue); 
#if 0
            printf("byte is (0x%02x) %d %d, 0x%llxh\n", num[i], i, shiftvalue, retvalue);
#endif
        }
    }
    else
    {
        for (i = 0; i < numbytes; i++, shiftvalue -= 8)
        {
            retvalue |= (((unsigned long long )num[i]) << shiftvalue);
        }
    }
    return retvalue;
}



/*!****************************************************************************
 * @fn      format_attribute()
 *
 * @brief
 *  This function formats the attribute line output into the out_buffer.
 *
 * @param out_buffer - for resulting formatted output.
 * @param in_buffer - contains the raw 12 byte attribute data.
 *
 * @return
 *  pointer to start of out_buffer.
 *
 * HISTORY
 *  01/20/12 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
unsigned char *format_attribute(unsigned char *out_buffer, unsigned char *in_buffer)
{
    sprintf((char *)out_buffer, "attr# %02x (%3d)  %04x %02x %02x 0x%012llx (%15lld)",
        in_buffer[0],
        in_buffer[0],
        cvt_to_short(&in_buffer[2], SWAP),
        in_buffer[3],
        in_buffer[4],
        (unsigned long long)cvt_value_to_ll(&in_buffer[5], SWAP, 6),
        (unsigned long long)cvt_value_to_ll(&in_buffer[5], SWAP, 6)
    );
    return out_buffer;
}




/*!****************************************************************************
 * @fn      dump_old_fuel_gauge()
 *
 * @brief
 *  This function formats and prints an old style fuel gauge entry.
 * It is assumed that this input data buffer is made up of 3 fields. The first 
 * is terminated with a NUL byte, while the second and third are terminated with 
 * a new line (i.e. ASCII NL or 0xa).
 *
 * @param data_buffer - pointer to raw data
 * @param length - length in bytes of raw data buffer
 *
 * @return
 *  None
 *
 * HISTORY
 *  01/23/12 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
void dump_old_fuel_gauge(unsigned char *data_buffer, int length)
{
    char *buf, *p2, *p3;

    buf = strchr((char *)data_buffer, (int)'\0');
    p2 = ++buf;
    buf = strchr(buf, 0xa);
    *buf++ = '\0';
    p3 = buf;
    buf = strchr(buf, 0xa);
    *buf++ = '\0';

    printf("OLD_FUEL_GAUGE entry %s %s %s\n", data_buffer, p2, p3);
}

/*!****************************************************************************
 * @fn      format_bms_data_page_header(unsigned char *data_buffer)
 *
 * @brief
 *  This function formats the BMS data header.
 *
 * @param None
 *
 * @return
 *  None
 *
 * HISTORY
 *  11/27/12 :  Christina Chiang -- Created.
 *
 *****************************************************************************/
char *format_bms_data_page_header(unsigned char *data_buffer)
{
    static char bms_data_output_buffer[64];

    sprintf(bms_data_output_buffer, "    page:%02x subpage: %02x length:%04lud", 
        0x15, 0x0, sizeof (bms_data_t));

    return bms_data_output_buffer;
}

/*!****************************************************************************
 * @fn      page15_formatted_dump()
 *
 * @brief
 * This function prints out the BMS data (page 0x15) in a pretty
 * formatted output.
 *
 * @param data_buffer - pointer to BMS data buffer.
 * @param lenght - length of the BMS data buffer.
 *
 * @return
 *  None
 *
 * HISTORY
 *  11/28/12 :  Christina Chiang -- Created.
 *
 *****************************************************************************/
void page15_formatted_dump(unsigned char *data_buffer, int length)
{
    bms_data_t bms;
   
    printf("%s \n", format_bms_data_page_header(data_buffer));
   
	memcpy (&bms, data_buffer, sizeof (bms_data_t));
    
	printf("        Collect time:            %s \n", format_timestamp(&bms.time_stamp));
	printf("        Number of Scans:         0x%08x (%10d) \n", cvt_to_short(&bms.num_bg_scans_performed[0],DONTSWAP),cvt_to_short(&bms.num_bg_scans_performed[0],  DONTSWAP));
	printf("        Total BMS Entries:       0x%08x (%10d) \n", cvt_to_short(&bms.total_entries[0], DONTSWAP),cvt_to_short(&bms.total_entries[0], DONTSWAP));
	printf("        Total IRAW Entries:      0x%08x (%10d) \n", bms.total_iraw, bms.total_iraw);
	printf("        Worst Head:              0x%08x (%10d) \n", bms.worst_head, bms.worst_head);
	printf("        Worst Head Entries:      0x%08x (%10d) \n", cvt_to_short(&bms.worst_head_entries[0], DONTSWAP), cvt_to_short(&bms.worst_head_entries[0], DONTSWAP));
	printf("        Total BMS 03/xx Entries: 0x%08x (%10d) \n", cvt_to_short(&bms.total_03_errors[0], DONTSWAP), cvt_to_short(&bms.total_03_errors[0], DONTSWAP));
	printf("        Total BMS 01/xx Entries: 0x%08x (%10d) \n", cvt_to_short(&bms.total_01_errors[0], DONTSWAP), cvt_to_short(&bms.total_01_errors[0], DONTSWAP));
	printf("        BMS Entry Range:         0x%08lx (%10lu) days\n", cvt_to_long(&bms.time_range[0], DONTSWAP)/(60*24), cvt_to_long(&bms.time_range[0], DONTSWAP)/(60*24));
	printf("        Drive Power-On:          0x%08lx (%10lu) days\n", cvt_to_long(&bms.drive_power_on_minutes[0], DONTSWAP)/(60*24), cvt_to_long(&bms.drive_power_on_minutes[0], DONTSWAP)/(60*24));
}

/*!****************************************************************************
 * @fn      page15_formatted_dump1()
 *
 * @brief
 * This function prints out the BMS data (page 0x15) in a pretty
 * formatted output with two extra values as page15_formatted_dump.
 *
 * @param data_buffer - pointer to BMS data buffer.
 * @param lenght - length of the BMS data buffer.
 *
 * @return
 *  None
 *
 * HISTORY
 *  05/27/13 :  Christina Chiang -- Created.
 *
 *****************************************************************************/
void page15_formatted_dump1(unsigned char *data_buffer, int length)
{
    bms_data1_t bms;
   
    printf("%s \n", format_bms_data_page_header(data_buffer));
   
	memcpy (&bms, data_buffer, sizeof (bms_data1_t));
    
	printf("        Collect time:                   %s \n", format_timestamp(&bms.time_stamp));
	printf("        Number of Scans:                0x%08x (%10d) \n", cvt_to_short(&bms.num_bg_scans_performed[0],DONTSWAP),cvt_to_short(&bms.num_bg_scans_performed[0],  DONTSWAP));
	printf("        Total BMS Entries:              0x%08x (%10d) \n", cvt_to_short(&bms.total_entries[0], DONTSWAP),cvt_to_short(&bms.total_entries[0], DONTSWAP));
	printf("        Total IRAW Entries:             0x%08x (%10d) \n", bms.total_iraw, bms.total_iraw);
	printf("        Worst Head:                     0x%08x (%10d) \n", bms.worst_head, bms.worst_head);
	printf("        Worst Head Entries:             0x%08x (%10d) \n", cvt_to_short(&bms.worst_head_entries[0], DONTSWAP), cvt_to_short(&bms.worst_head_entries[0], DONTSWAP));
	printf("        Total BMS 03/xx Entries:        0x%08x (%10d) \n", cvt_to_short(&bms.total_03_errors[0], DONTSWAP), cvt_to_short(&bms.total_03_errors[0], DONTSWAP));
	printf("        Total BMS 01/xx Entries:        0x%08x (%10d) \n", cvt_to_short(&bms.total_01_errors[0], DONTSWAP), cvt_to_short(&bms.total_01_errors[0], DONTSWAP));
	printf("        Total unique BMS 03/xx Entries: 0x%08x (%10d) \n", cvt_to_short(&bms.total_unique_03_errors[0], DONTSWAP), cvt_to_short(&bms.total_unique_03_errors[0], DONTSWAP));
	printf("        Total unique BMS 01/xx Entries: 0x%08x (%10d) \n", cvt_to_short(&bms.total_unique_01_errors[0], DONTSWAP), cvt_to_short(&bms.total_unique_01_errors[0], DONTSWAP));
	printf("        BMS Entry Range:                0x%08lx (%10lu) days\n", cvt_to_long(&bms.time_range[0], DONTSWAP)/(60*24), cvt_to_long(&bms.time_range[0], DONTSWAP)/(60*24));
	printf("        Drive Power-On:                 0x%08lx (%10lu) days\n", cvt_to_long(&bms.drive_power_on_minutes[0], DONTSWAP)/(60*24), cvt_to_long(&bms.drive_power_on_minutes[0], DONTSWAP)/(60*24));
}

