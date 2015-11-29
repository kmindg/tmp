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
 *  9/23/2015 Dave Peterson    - Version 1.7.1 Fixed compile warnings, made command-args TRiiAGE-compliant.
 *  10/13/2015 Wayne Garrett   - Version 1.8 Added log pages 2,3,5,11,2f and 37.
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


typedef struct fast_cache_over_provisioning_head_s
{
    standard_head_t stdhead;    
    timestamp_t collection_timestamp;  /* output from perl localtime*/    
} fast_cache_over_provisioning_head_t;


typedef struct log_page_parser_s
{
    unsigned char * log_page_buffer;
    int index;
    int bytes_remaining;    
} log_page_parser_t;

typedef struct log_2f_s
{
    unsigned long long nand_writes;
    unsigned long long host_writes;
} log_2f_t;

typedef struct log_30_s
{
    unsigned long long nand_writes;
    unsigned long long host_writes;
} log_30_t;

typedef struct log_31_s
{
    unsigned long long nand_writes;
    unsigned long long host_writes;
} log_31_t;

typedef struct log_37_s
{
    unsigned long long nand_writes;
    unsigned long long host_writes;
} log_37_t;

typedef struct drive_info_s
{
    fru_head_t fru;
    log_2f_t log_2f;
    log_30_t log_30;
    log_31_t log_31;
    log_37_t log_37;
} drive_info_t;

#define TLA_SIZE 12
typedef struct tla_table_entry_s
{
   char tla[TLA_SIZE+1];    /* +1 for EOL */
} tla_table_entry_t;

/* Note,  these are TLAs as of 10/23/2016.   This will need to be
   maintained. */

/*Hitachi Sunset Cove Plus*/
const tla_table_entry_t g_tla_table_scp[] =
{
    "005051126",
    "005051127",
    "005051128",
    "005051129",
    "005051130",
    "005051131",
    "005051132",
    "005051133",
    "005051134",
    "005051135",
    "005051136",
    "005051137",
    "005051138",
    "005051139",
    "005051140",
    "005051141",
    "005051158",
    "005051159",
    "005051160",
    "005051161",
    "005051162",
    "005051163",
    "005051164",
    "005051165",
    "005051169",
    "005051170",
    "005051171",
    "005051172",
    "005051175",
    "005051176",
    "005051187",
    "005051188",
    "005051189",
    "005051190",
    "005051191",
    "005051192",
    "005051193",
    "005051194",
    "005051195",
    "005051196",
    "005051197",
    "005051198",
    "005051199",
    "005051200",
    "005051201",
    "005051202",
    "005051227",
    "005051228",
    "005051229",
    "005051230",
    "005051231",
    "005051232",
    "005051233",
    "005051234",
    "005051379",
    "005051380",
    "005051387",
    "005051388",
    "005051391",
    "005051392",
    "005051395",
    "005051396",
    "005051541",
    "005051542",
    "005051543",
    "005051544",
    "005051545",
    "005051546",
    "005051547",
    "005051548",
    "005051549",
    "005051550",
    "005051571",
    "005051572",
    "005051573",
    "005051574",
    "005051577",
    "005051578",
    "005051579",
    "005051580",
    "005051581",
    "005051582",
    "005051583",
    "005051584",
    "005051585",
    "005051586",
    "005051587",
    "005051588",
    "005051589",
    "005051590",
    "005051591",
    "005051592",
    "005051801",
    "005051802",
    "005051803",
    "005051804",
    "005051805",
    "005051806",
    "005051807",
    "005051808",
};

/* Micron Buckhorn*/
const tla_table_entry_t g_tla_table_buckhorn[] =
{
    "005050112",
    "005050113",
    "005050114",
    "005050115",
    "005050116",
    "005050117",
    "005050424",
    "005050425",
    "005050598",
    "005050599",
    "005050600",
};

/* Samsung RDX MLC and SLC*/
const tla_table_entry_t g_tla_table_rdx[] =
{
    "005050496",
    "005050497",
    "005050498",
    "005050499",
    "005050500",
    "005050501",
    "005050502",
    "005050503",
    "005050504",
    "005050505",
    "005050506",
    "005050507",
    "005050523",
    "005050524",
    "005050525",
    "005050526",
    "005050527",
    "005050528",
    "005050529",
    "005050530",
    "005050537",
    "005050538",
    "005050540",
};


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
#define FCOP_HEADER_TYPE        10  /* Fast Cache Over Provisioning header */


/* The following must match the production code values
 * Specifically these must match CM_DRIVE_STATS_DATA_VARIANT_ID which is
 * defined in cm_local.h or fbe_drive_mgmt_private.h.
 */
#define VARIANT_ID_UNKNOWN                  0
#define VARIANT_ID_SMART_DATA               1  /* for SATA flash */
#define VARIANT_ID_FUEL_GAUGE_DATA          2  /* for SAS flash */  
#define VARIANT_ID_BMS_DATA                 3  /* log page 0x15 */  
#define VARIANT_ID_BMS_DATA1                4  /* New BMS format with total unique 03 and 02 entries.*/
#define VARIANT_ID_WRITE_ERROR_COUNTERS     5  /* log page 0x2 */
#define VARIANT_ID_READ_ERROR_COUNTERS      6  /* log page 0x3 */    
#define VARIANT_ID_VERIFY_ERROR_COUNTERS    7  /* log page 0x5 */
#define VARIANT_ID_SSD_MEDIA                8  /* log page 0x11 */
#define VARIANT_ID_INFO_EXCEPTIONS          9  /* log page 0x2f */
#define VARIANT_ID_VENDOR_SPECIFIC_SMART    10 /* log page 0x37*/

/* Max Length of log page is unfortunately hardcoded in the Fuel Gauge collection.  This dump utility will
   need to account for that.  The code which defines the length is
   #define DMO_FUEL_GAUGE_WRITE_BUFFER_LENGTH (DMO_FUEL_GAUGE_MSG_HEADER_SIZE + DMO_FUEL_GAUGE_WRITE_BUFFER_TRANSFTER_COUNT) */  
#define MAX_DATA_VARIANT_LEN  1111

#define INVALID_NUM  -1

/* String names for printing Name/Value pairs.  Each string must be unique.
   Other scripts will parse on this value, so do not change unless those scripts 
   have been tested. 
*/
#define POWER_ON_HOURS_STR          "POWER_ON_HOURS"
#define NAND_BYTES_READ_STR         "NAND_BYTES_READ"
#define NAND_BYTES_WRITTEN_STR      "NAND_BYTES_WRITTEN"
#define MAX_DRIVE_TEMP_STR          "MAX_DRIVE_TEMP"
#define TOTAL_READ_COMMANDS_STR     "TOTAL_READ_COMMANDS"
#define TOTAL_WRITE_COMMANDS_STR    "TOTAL_WRITE_COMMANDS"
#define TOTAL_HOST_WRITE_COUNT_STR  "TOTAL_HOST_WRITE_COUNT"
#define TOTAL_NAND_WRITE_COUNT_STR  "TOTAL_NAND_WRITE_COUNT"
#define HOST_WRITES_32MB_STR        "HOST_WRITES_32MB"
#define NAND_WRITES_32MB_STR        "NAND_WRITES_32MB"


#define MIN(a,b) (((a)<(b))?(a):(b))


void initialize_log_2f(log_2f_t *log_2f_p);
void initialize_log_30(log_30_t *log_30_p);
void initialize_log_31(log_31_t *log_31_p);
void initialize_log_37(log_37_t *log_37_p);
void initialize_drive_info(drive_info_t *drive_info_p, fru_head_t *fru_p);
int get_header_type(FILE *fp, session_head_t *session, fru_head_t *fru, data_head_t *data, unsigned char *raw_buffer, int *ret_count, int printmask);
void dump_session_header(session_head_t *session);
char *format_stdhead(standard_head_t *head);
char *format_timestamp(timestamp_t *ts);
char *format_perl_localtime(timestamp_t *ts);
void dump_fru_header(fru_head_t *fru);
void dump_data_header(data_head_t *data);
void dump_fcop_header(fast_cache_over_provisioning_head_t *fcop_head_p);
const char *cvt_varient_id(int variant_id);
unsigned short cvt_to_short(unsigned char *num, int swap);
unsigned long cvt_to_long(unsigned char *num, int swap);
unsigned long long cvt_value_to_ll(unsigned char *num, int swap, int numbytes);
int get_and_dump_varient(drive_info_t *drive_info_p, FILE *fp, data_head_t *data, int printmask, int dcs_header_type);
int dump_fast_cache_over_provisioning(FILE *fp, fast_cache_over_provisioning_head_t *fcop_head_p, int printmask);
int get_data(FILE *fp, unsigned char *data_buffer, int length);
void raw_dump(unsigned char *data_buffer, int length, const char *indent);
void page30_formatted_dump(drive_info_t *drive_info_p, unsigned char *data_buffer, int length, int printmask);
void parse_log_page_30(unsigned char * const data_buffer_p, log_30_t *log_page_30_p);
void page31_formatted_dump(drive_info_t *drive_info_p, unsigned char *data_buffer, int length, int dcs_header_type, int printmask);
void parse_log_page_31(unsigned char * const data_buffer_p, int length, log_31_t *log_page_31_p);
/*/unsigned long long cvt_value_to_longl(unsigned char *num, int swap, int numbytes);*/
unsigned char *format_attribute(unsigned char *out_buffer, unsigned char *in_buffer);
void dump_old_fuel_gauge(unsigned char *data_buffer, int length);
char *format_bms_data_page_header(unsigned char *data_buffer);
void page15_formatted_dump(unsigned char *data_buffer, int length, int printmask);
void page15_formatted_dump1(unsigned char *data_buffer, int length, int printmask);
void log_page_formatted_dump(unsigned char *data_buffer, int length, int printmask);
void page02_formatted_dump(unsigned char *data_buffer, int length, int printmask);
void page03_formatted_dump(unsigned char *data_buffer, int length, int printmask);
void page05_formatted_dump(unsigned char *data_buffer, int length, int printmask);
void page11_formatted_dump(unsigned char *data_buffer, int length, int printmask);
void page2f_formatted_dump(unsigned char *data_buffer, int length, int printmask);
void page37_formatted_dump(unsigned char *data_buffer, int length, int printmask);

void log_page_initialize_parser(log_page_parser_t *parser_p, unsigned char *log_page_buffer, int length);
bool log_page_get_next_parameter(log_page_parser_t * parser_p, unsigned char **log_parameter_pp);
void log_page_get_parameter(unsigned char *log_parameter_p, unsigned int *param_code_p, unsigned int *param_len_p, unsigned long long *value_p, bool print_warn);

bool is_tla_in_table(unsigned char *const tla, const tla_table_entry_t *const table_p, int num_entries);
bool is_scp_drive(unsigned char *const tla);
bool is_buckhorn_drive(unsigned char *const tla);
bool is_rdx_drive(unsigned char *const tla);

#define BIT(b) (1 << (b))

#define PRT_SESSION         BIT(0)  /* (1) */
#define PRT_FRUS            BIT(1)  /* (2) */
#define PRT_DATAHEAD        BIT(2)  /* (4) */
#define PRT_FORMATTED_DATA  BIT(3)  /* (8) FORMATTED and RAW data output is mutually exclusive*/
#define PRT_RAW_DATA        BIT(4)  /* (16) */
#define PRT_WA_DATA         BIT(5)  /* (32) Print Write Amplifcation info. (Mutually excusive w/ formatted and raw data).  */
#define PRT_FCOP            BIT(6)  /* (64) */
#define PRT_DEBUG_FLAG      BIT(8)  /* (256)*/ 

#define PRT_ALL     (PRT_SESSION | PRT_FRUS | PRT_DATAHEAD | PRT_FCOP | PRT_FORMATTED_DATA)  /* (79) */
#define PRT_ALL_RAW (PRT_SESSION | PRT_FRUS | PRT_DATAHEAD | PRT_FCOP | PRT_RAW_DATA)        /* (87) */
#define PRT_ALL_WA  (PRT_SESSION | PRT_FRUS | PRT_DATAHEAD | PRT_WA_DATA)   /* (39) */

#define DEFAULT_INPUT_FILE_NAME  (const char *) "c:\\drive_flash_stats.txt"

#define RAW_DATA_INDENT "      "
#define SWAP        TRUE
#define DONTSWAP    FALSE

#define DATA_BUFFER_SIZE  2048
#define DATA_BUFFER_SIZE_DCS1 516

static unsigned char my_data_buffer[DATA_BUFFER_SIZE];

#define PROGRAM "fbe_drive_stats_dump"
#define DRIVE_STATS_DUMP_VERSION  "1.8.0"

void do_version(void)
{
  printf("[ %s %s ]\n", PROGRAM, DRIVE_STATS_DUMP_VERSION);
}

void do_help(void)
{
  printf("Usage: %s [args] file\n", PROGRAM);
  printf("    -h:       Print this message.\n");
  printf("    -m <dec>: Set printmask to <dec>.\n");
  printf("    -r:       Prints all fields as raw hex.\n");
  printf("    -v:       Display version information\n");
  printf("Examples:\n");
  printf("    Dump standard output\n");
  printf("        %s <file>\n", PROGRAM);
  printf("    Dump all fields raw hex\n");
  printf("        %s <file> -r \n", PROGRAM);
  printf("    Dump Write Amplifcation info\n");
  printf("        %s <file> -m 39\n", PROGRAM);
}

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
 *  0 on success, 1 on failure
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
    int i;
    session_head_t session;
    fru_head_t fru;
    data_head_t data;    
    char infile[256];
    drive_info_t drive_info;

	DCS1_header_type = 0;

    /* default input file */    
    strncpy (infile, DEFAULT_INPUT_FILE_NAME, sizeof(infile));

    printmask = PRT_ALL;  /* Print everything */

    /* Process commandline arguments */

    for (i=1; i<argc; i++) {
      if      (!strcmp(argv[i], "-h"))	{ do_help(); return 0; }
      else if (!strcmp(argv[i], "-v"))	{ do_version(); return 0; }
      else if (!strcmp(argv[i], "-m"))	{ if (i+1 < argc) { printmask = atoi(argv[++i]); } }
      else if (!strcmp(argv[i], "-r"))  { printmask = PRT_ALL_RAW; }
      else				{ strncpy(infile, argv[i], sizeof(infile)); }
    }

    if ((fp = fopen(infile, "rb")) == NULL)
    {
        printf("ERROR: Failed to open input file: %s\n", infile);
        return 1;
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
            return 1;
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
                initialize_drive_info(&drive_info, &fru);
            break;

            case DCS0_DATA_HEADER_TYPE:
            case DATA_HEADER_TYPE:
                if (printmask & PRT_DATAHEAD)
                {
                    dump_data_header(&data);
                }
                if (get_and_dump_varient(&drive_info, fp, &data, printmask, DCS1_header_type))
                {
                    printf("ERROR: Data variant corrupt\n");
                    return 1;
                }
            break;

        case FCOP_HEADER_TYPE:
                if (printmask & PRT_FCOP) 
                {                    
                    dump_fcop_header((fast_cache_over_provisioning_head_t*)my_data_buffer);
                }
              
                if(dump_fast_cache_over_provisioning(fp, (fast_cache_over_provisioning_head_t*)my_data_buffer, printmask))
                {
                    printf("ERROR: FastCache Over Provisioning corrupt\n");
                    return 1;
                }                
                break;

            case END_OF_FILE_TYPE:
                /* nothing to do except exit cleanly.*/
                printf("hit eof\n");
            break;

            default:
                printf("ERROR: Bad header type. File is corrupt\n");
                return 1;
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
        (header_char == 'O') ||
        (header_char == 'P') ||
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
    else if (header[0] == 'F' && header[1] == 'C' && header[2] == 'O' && header[3] == 'P')
    {
        /* this is a fru header */
        retheader = FCOP_HEADER_TYPE;
        buf = raw_buffer;
        length =  sizeof(fast_cache_over_provisioning_head_t);
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
                if (printmask & PRT_DEBUG_FLAG)
                {
                    printf("chr %d is (0x%x)\n", i, *buf);
                } 
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
 * @fn      format_perl_localtime()
 *
 * @brief
 *  This function takes a timestamp that was generated form perl localtime and
 *  converts it to the format yyyy/mm/dd-hh:mm:ss
 *
 * @param ts - pointer to timestamp bytes.
 *
 * @return
 *  pointer to static buffer containing output time stamp string.
 *
 * HISTORY
 *  10/11/15 :  Wayne Garrett -- Created.
 *
 *****************************************************************************/
char *format_perl_localtime(timestamp_t *ts)
{
    static char ts_buffer[32];

    /* 2012/12/31-12:12:59*/
    sprintf((char *)&ts_buffer, "%04d/%02d/%02d-%02d:%02d:%02d", 
        ts->year+1900,
        ts->month+1,
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


void dump_fcop_header(fast_cache_over_provisioning_head_t *fcop_head_p)
{
    printf("  FCOP: %s length:%llu\n",
           format_perl_localtime(&fcop_head_p->collection_timestamp),
           cvt_value_to_ll(fcop_head_p->stdhead.length, DONTSWAP, 2)           
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
const char *cvt_varient_id(int variant_id)
{
    if (variant_id == VARIANT_ID_SMART_DATA)
        return "SMART_DATA";
    else if (variant_id == VARIANT_ID_BMS_DATA)
        return "BMS_DATA";
	else if (variant_id == VARIANT_ID_BMS_DATA1)
        return "BMS_DATA1";
    else if (variant_id == VARIANT_ID_FUEL_GAUGE_DATA)
        return "FUEL_GAUGE";
    else if (variant_id == VARIANT_ID_WRITE_ERROR_COUNTERS)
        return "WRITE_ERROR_COUNTERS";
    else if (variant_id == VARIANT_ID_READ_ERROR_COUNTERS)
        return "READ_ERROR_COUNTERS";
    else if (variant_id == VARIANT_ID_VERIFY_ERROR_COUNTERS)
        return "VERIFY_ERROR_COUNTERS";
    else if (variant_id == VARIANT_ID_SSD_MEDIA)
        return "SSD_MEDIA";
    else if (variant_id == VARIANT_ID_INFO_EXCEPTIONS)
        return "INFO_EXCEPTIONS";
    else if (variant_id == VARIANT_ID_VENDOR_SPECIFIC_SMART)
        return "VENDOR_SPECIFIC_SMART";
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
int get_and_dump_varient(drive_info_t *drive_info_p, FILE *fp, data_head_t *data, int printmask, int dcs_header_type)
{
    int size;
    int length = cvt_to_short(data->stdhead.length, SWAP);

    
#if 0
    printf("Length from the header is: %d\n", length);
#endif

    /* Fuel Gauge has a max len it will write for any Variant.  The length obtained inside the header 
       was derived from what the drive returned, not what was actually written to the file.  Adjust the length
       in order to parse the file correctly. */
    if (length > MAX_DATA_VARIANT_LEN)
    {
        printf("WARN: data variant greater than max.  Adjusting. %d > %d\n", length, MAX_DATA_VARIANT_LEN);
        length = MAX_DATA_VARIANT_LEN;
    }

    length -= sizeof(data_head_t); /* The size includes the extra bytes for the data header itself.*/
    
	if ((data->variant_id == VARIANT_ID_FUEL_GAUGE_DATA) && (dcs_header_type == 1) && (length > 200))
	{
		/* Special handle for the Fuel gauge (0x31) was truncated for 12 bytes.*/
		length -= 12;
	}

    if (length > DATA_BUFFER_SIZE)
    {
        printf("ERROR: File is corrupt or has too large a data variant. %d > %d\n", length, DATA_BUFFER_SIZE);
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
    else
    {
        switch (data->variant_id)
        {
            case VARIANT_ID_SMART_DATA:
                page30_formatted_dump(drive_info_p, my_data_buffer, length, printmask);
            break;

            case VARIANT_ID_FUEL_GAUGE_DATA:
                page31_formatted_dump(drive_info_p, my_data_buffer, length, dcs_header_type, printmask);
            break;

            case VARIANT_ID_BMS_DATA:
                page15_formatted_dump(my_data_buffer, length, printmask);
            break;

			case VARIANT_ID_BMS_DATA1:
                page15_formatted_dump1(my_data_buffer, length, printmask);
            break;

            case VARIANT_ID_WRITE_ERROR_COUNTERS:
                page02_formatted_dump(my_data_buffer, length, printmask);
                break;

            case VARIANT_ID_READ_ERROR_COUNTERS:
                page03_formatted_dump(my_data_buffer, length, printmask);
                break;

            case VARIANT_ID_VERIFY_ERROR_COUNTERS:
                page05_formatted_dump(my_data_buffer, length, printmask);
                break;

            case VARIANT_ID_SSD_MEDIA:
                page11_formatted_dump(my_data_buffer, length, printmask);
                break;

            case VARIANT_ID_INFO_EXCEPTIONS:
                page2f_formatted_dump(my_data_buffer, length, printmask);
                break;

            case VARIANT_ID_VENDOR_SPECIFIC_SMART:
                page37_formatted_dump(my_data_buffer, length, printmask);
                break;

            case VARIANT_ID_UNKNOWN:
            default:
                printf("WARN: Unknown format for data block.\n");
            break;
        }

    }
    return FALSE;
}


/*!****************************************************************************
 * @fn      dump_fast_cache_over_provisioning()
 *
 * @brief
 *  This function dumps the FAST Cache over provisioning info, which is
 *  just the text output from the on-array FctCli.exe
 *
 * @author
 *  10/13/15 :  Wayne Garrett -- Created.
 *
 *****************************************************************************/
int dump_fast_cache_over_provisioning(FILE *fp, fast_cache_over_provisioning_head_t *fcop_head_p, int printmask)
{
    unsigned char data_buffer[DATA_BUFFER_SIZE+1]; /* +1 for EOL*/
    int buff_size = DATA_BUFFER_SIZE;
    int bytes_read;

    long long bytes_remaining = cvt_value_to_ll(fcop_head_p->stdhead.length, DONTSWAP, 2);    

    printf("      %s\n", "<FCOP_OUTPUT>");

    while (bytes_remaining > 0)
    {    
        memset(data_buffer, 0, sizeof(data_buffer));
        bytes_read = get_data(fp, data_buffer, MIN((int)bytes_remaining, buff_size));
        bytes_remaining -= bytes_read;
    
        if (printmask & PRT_RAW_DATA)
        {
            raw_dump(data_buffer, bytes_read, RAW_DATA_INDENT);
        }
        else if (printmask & PRT_FORMATTED_DATA)
        {            
            printf("%s",data_buffer);            
        }
    }

    printf("\n      %s\n", "</FCOP_OUTPUT>");


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
void raw_dump(unsigned char *data_buffer, int length, const char *indent)
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
 * @fn      format_log_page_data_page_header(unsigned char *data_buffer)
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
 *  10/13/15 :  Wayne Garrett -- Modified from format_smart_data_page_header to
 *                               prevent breaking legacy parsers.
 *
 *****************************************************************************/
char *format_log_page_data_page_header(unsigned char *data_buffer)
{
    static char smart_data_output_buffer[64];

    sprintf(smart_data_output_buffer, "    page:%02x subpage: %02x length:%04d", 
        data_buffer[0],
        data_buffer[1],
        cvt_to_short(&data_buffer[2], SWAP));
    return smart_data_output_buffer;
}


char *format_log_page_parameter_code_string(unsigned int param_code, unsigned long long value, char *description)
{
    static char output_buffer[256];

    sprintf(output_buffer, "Param: 0x%04x, Value: 0x%012llx(%15llu), %s", 
        param_code, value, value, description);

    return output_buffer;
}

char *format_log_page_parameter_name_value_string(char *name, unsigned long long value)
{
    static char output_buffer[256];

    sprintf(output_buffer, "%s: 0x%012llx(%15llu)", 
        name, value, value);

    return output_buffer;
}

char *print_log_page_37_for_hitachi(unsigned char *data_buffer_p, unsigned char *log_parameter_p, int length, const char *indent)
{
    static char output_buffer[256];
    unsigned long long value;    
    int page_len = (int)cvt_value_to_ll(&data_buffer_p[2], DONTSWAP, 2);    

    /* Make sure param length is greater than what we expect for Hitachi SSD.  If not,
       this this is probably not an SCP.  Post processing scripts will be responsible
       to verify data is from correct drive family. */
    
    if (page_len < 0x40)  /* this matches SCP*/
    {
        printf("%sWARN: Not a valid param for Hitachi SSD. page len:0x%x\n", indent, page_len);
        return output_buffer;
    }

    value = cvt_value_to_ll(&data_buffer_p[8], DONTSWAP, 4);
    printf("%s%-30s: 0x%012llx (%15lld)\n", indent, POWER_ON_HOURS_STR, value, value);

    value = cvt_value_to_ll(&data_buffer_p[12], DONTSWAP, 8);
    printf("%s%-30s: 0x%012llx (%15lld)\n", indent, NAND_BYTES_READ_STR, value, value);

    value = cvt_value_to_ll(&data_buffer_p[20], DONTSWAP, 8);
    printf("%s%-30s: 0x%012llx (%15lld)\n", indent, NAND_BYTES_WRITTEN_STR, value, value);

    value = (unsigned long long)data_buffer_p[28];
    printf("%s%-30s: 0x%012llx (%15lld)\n", indent, MAX_DRIVE_TEMP_STR, value, value);

    value = cvt_value_to_ll(&data_buffer_p[33], DONTSWAP, 8);
    printf("%s%-30s: 0x%012llx (%15lld)\n", indent, TOTAL_READ_COMMANDS_STR, value, value);

    value = cvt_value_to_ll(&data_buffer_p[41], DONTSWAP, 8);
    printf("%s%-30s: 0x%012llx (%15lld)\n", indent, TOTAL_WRITE_COMMANDS_STR, value, value);

    value = cvt_value_to_ll(&data_buffer_p[52], DONTSWAP, 8);
    printf("%s%-30s: 0x%012llx (%15lld)\n", indent, TOTAL_HOST_WRITE_COUNT_STR, value, value);

    value = cvt_value_to_ll(&data_buffer_p[60], DONTSWAP, 8);
    printf("%s%-30s: 0x%012llx (%15lld)\n", indent, TOTAL_NAND_WRITE_COUNT_STR, value, value);

    return output_buffer;
}



char *format_raw_data(unsigned char *data_buffer, int length, const char *indent)
{
    static char output_buffer[256];
    int i;


    for (i=0; i<length; i++)
    {
        if (i%16  == 0)
        {
            sprintf(output_buffer, "%s", indent);
        }

        sprintf(output_buffer, "%02x", data_buffer[i]);

        if (i+1 < length)
        {
            sprintf(output_buffer, " ");
        }
    }
    return output_buffer;
}

char *format_log_page_parameter_code_string_as_raw_data(unsigned int param_code, unsigned char *log_parameter_p)
{
    static char output_buffer[256];
    int length = log_parameter_p[3];
    unsigned char *data = &log_parameter_p[4];

    sprintf(output_buffer, "Param: 0x%04x, Raw Data:\n%s\n", 
            param_code, format_raw_data(data, length, "      "));

    return output_buffer;
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
 * @note
    Raw Read Error Count (0x01), 1,	Count of all errors detected over the drive's life while reading data from Flash.
    Reallocated Flash Block Count (0x05), 5, Count of reallocated Flash blocks. Specifically, Flash blocks reallocated during run-time (grown bad blocks).
    Power On Hours (0x09), 9, Number of hours elapsed in the Power-On state.
    Power Cycle (0x0C), 12, Number of Power-On events in the life of the drive.
    Device Capacity  (0x0E), 14, Raw Flash Capacity  
    User Capacity (0x0F), 15, Total User Capacity
    Spare Blocks Available (0x10), 16, Total Spare Flash Blocks at time 0
    Remaining Spare Blocks (0x11), 17, Remaining Spare Flash Blocks at current time
    Total Erase Count 	, 100, Total Erase count across all Flash Blocks on the drive
    Total block erase failure (0xAC), 172, Total Flash block erase failure count
    Per Block Max Erase Count (0xAD), 173, Highest erase count of any Flash block on any chip on the drive.
    Unexpected Power Loss count, 174, Number of Surprise Power Removal events
    Average Erase Count, 175, Average erase count of all Flash blocks on the drive
    Total block program failure (0xB5), 181, Total block program failure count
    Reported Uncorrectable Errors (0xBB), 187, The number of uncorrectable errors reported at the interface.
    Temperature (0xC2), 194, Temperature of drive case.
    Current Pending Block Count (0xC5), 197, Count of Flash blocks that are marked for reallocation but have not been fully reallocated yet.
    Offline Surface Scan (0xC6), 198, If applicable this attribute will report the uncorrectable error count during SMART off-line scanning.  If not applicable a value of zero must be returned. 
    SATA FIS CRC Errors, 199, Number of SATA FIS CRC Errors
    UECC Rebuild Event Counter, 200, Counter for the number of times UECC Recovery was used to rebuild due to a failure mode (Section 4.5.11) 
    UECC Recovery Event Notification, 201, See Note below for detail
    Percentage of Drive Life Used, 202, Percent of life used on the SSD (a value from 0 to 0x64)
    Total Bytes Sectors Read, 234, Total number of Bytes Sectors Read from Flash over the drive life.
    Tobtal Host Bytes Sectors Written, 235, Total number ofByte Sectors Written to the drive by a Host over the drive life.
 *
 * 
 * HISTORY
 *  01/20/12 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
void page30_formatted_dump(drive_info_t *drive_info_p, unsigned char *data_buffer, int length, int printmask)
{
    unsigned char buffer[64];
    int i;
    i = 0;

    initialize_log_30(&drive_info_p->log_30);
    parse_log_page_30(data_buffer, &drive_info_p->log_30);
    
    if (printmask & PRT_FORMATTED_DATA)
    {
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
    else if (printmask & PRT_WA_DATA)
    {
        if (is_buckhorn_drive(drive_info_p->fru.tla_number))
        {
            if (drive_info_p->log_30.host_writes != INVALID_NUM){            
                unsigned long long host_writes = (drive_info_p->log_30.host_writes*512)/(32*1024*1024); /* convert to 32MiB*/                
                printf("      %s\n", format_log_page_parameter_name_value_string(HOST_WRITES_32MB_STR, host_writes));
            }
            else{
                printf("      WARN: host writes not found");
            }

            if (drive_info_p->log_30.nand_writes != INVALID_NUM){
                unsigned long long nand_writes = (drive_info_p->log_30.nand_writes*8192)/(32*1024*1024); /* convert to 32MiB*/
                printf("      %s\n", format_log_page_parameter_name_value_string(NAND_WRITES_32MB_STR, nand_writes));
            }
            else{
                printf("      WARN: nand writes not found");
            }
        }
    }
}

/*!****************************************************************************
 * @fn      parse_log_page_30()
 *
 * @brief
 * This function parses log page 30 and stores the params in the passed in
 * data structure.
 *
 * @param data_buffer - pointer to attribute data buffer.
 * @param log_page_30_p - log 30 data structure
 *
 * @return
 *  None
 * 
 * @author
 *  10/13/15 :  Wayne Garrett -- Created.
 *
 *****************************************************************************/
void parse_log_page_30(unsigned char * const data_buffer_p, log_30_t *log_page_30_p)
{
    unsigned char *buff_p = data_buffer_p;
    int i;

    buff_p += 6;
    for (i = 0; i < 30; i++)
    {
        int attr = buff_p[0];

        switch (attr)
        {
        case 247:
            log_page_30_p->host_writes = cvt_value_to_ll(&buff_p[5], SWAP, 6); 
            break;

        case 248:
            log_page_30_p->nand_writes = cvt_value_to_ll(&buff_p[5], SWAP, 6);
            break;
        }
        buff_p += 12;
    }
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
void page31_formatted_dump(drive_info_t *drive_info_p, unsigned char *data_buffer, int length, int dcs_header_type, int printmask)
{
    int data_length;
	unsigned int param_code;

    initialize_log_31(&drive_info_p->log_31);
    parse_log_page_31(data_buffer, length, &drive_info_p->log_31);

    if (printmask & PRT_FORMATTED_DATA)
    {
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
    else if (printmask & PRT_WA_DATA)
    {
        if (drive_info_p->log_31.host_writes != INVALID_NUM)
        {
            printf("      %s\n", format_log_page_parameter_name_value_string(HOST_WRITES_32MB_STR, drive_info_p->log_31.host_writes));
        }

        if (drive_info_p->log_31.nand_writes != INVALID_NUM)
        {
            printf("      %s\n", format_log_page_parameter_name_value_string(NAND_WRITES_32MB_STR, drive_info_p->log_31.nand_writes));
        }      
    }
}

/*!****************************************************************************
 * @fn      parse_log_page_31()
 *
 * @brief
 * This function parses log page 31 and stores the params in the passed in
 * data structure.
 *
 * @param data_buffer - pointer to attribute data buffer.
 * @param log_page_31_p - log 31 data structure
 *
 * @return
 *  None
 * 
 * @author
 *  10/13/15 :  Wayne Garrett -- Created.
 *
 *****************************************************************************/
void parse_log_page_31(unsigned char * const data_buffer_p, int length, log_31_t *log_page_31_p)
{
    log_page_parser_t parser;
    unsigned char *log_parameter_p = NULL;
    unsigned int param_code;
    unsigned int param_len;
    unsigned long long value;

    
    log_page_initialize_parser(&parser, data_buffer_p, length);

    while (log_page_get_next_parameter(&parser, &log_parameter_p))
    {     
        log_page_get_parameter(log_parameter_p, &param_code, &param_len, &value, true);        

        switch(param_code)
        {
            /*TODO: add power-on, p/e cycle, and max erase */
        case 0x8FF8: 
            log_page_31_p->host_writes = value;
            break;
        case 0x8FF9:
            log_page_31_p->nand_writes = value;            
            break;            
        }        
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
void page15_formatted_dump(unsigned char *data_buffer, int length, int printmask)
{
    bms_data_t bms;
   
    if (printmask & PRT_FORMATTED_DATA)       
    {
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
void page15_formatted_dump1(unsigned char *data_buffer, int length, int printmask)
{
    bms_data1_t bms;
   
    if (printmask & PRT_FORMATTED_DATA)       
    {
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
}

void log_page_get_parameter(unsigned char *log_parameter_p, unsigned int *param_code_p, unsigned int *param_len_p, unsigned long long *value_p, bool print_warn)
{
    *param_code_p =  cvt_to_short(&log_parameter_p[0], SWAP);
    *param_len_p = log_parameter_p[3];

    if (*param_len_p == 4)
    {
        *value_p = cvt_to_long(&log_parameter_p[4], SWAP);
    }
    else if (*param_len_p == 8)
    {
        *value_p = cvt_value_to_ll(&log_parameter_p[4], DONTSWAP, 8);
    }
    else
    {
        if (print_warn)
        {
            printf("WARN: Unsupported Param Value length. Parameter code:%04xh len:%02xh\n", 
                   *param_code_p, *param_len_p);
        }
        *value_p = 0;
    }
}


void log_page_initialize_parser(log_page_parser_t *parser_p, unsigned char *log_page_buffer, int length)
{
    parser_p->log_page_buffer = log_page_buffer;
    parser_p->index = 0;
    parser_p->bytes_remaining = length;
}

bool log_page_get_next_parameter(log_page_parser_t * parser_p, unsigned char **log_parameter_pp)
{
    if (parser_p->bytes_remaining <= 0)
    {
        return false;
    }

    if (parser_p->index == 0)  
    {
        /*find first parameter*/
        parser_p->bytes_remaining -= 4;    
        parser_p->index = 4;

        if (parser_p->bytes_remaining <= 0)  /* no more parameters */
        {
            return false;
        }

        *log_parameter_pp = &parser_p->log_page_buffer[parser_p->index];
    }
    else
    {
        /* get next parameter*/
        int data_length;
        data_length = parser_p->log_page_buffer[parser_p->index+3] +4;
        parser_p->bytes_remaining -= data_length;
        parser_p->index += data_length;

        if (parser_p->bytes_remaining <= 0)    /* no more parameters */
        {
            return false;
        }     
           
        *log_parameter_pp = &parser_p->log_page_buffer[parser_p->index];
    }

    return true;
}


/*!****************************************************************************
 * @fn      log_page_formatted_dump()
 *
 * @brief
 * This function prints out the parameter code and value for a given log page
 *
 * @param data_buffer - pointer to attribute data buffer.
 * @param length - length of the attribute data buffer.
 *
 * @return
 *  None
 *
 * HISTORY
 *  10/13/15 :  Wayne Garrett -- Created.
 *
 *****************************************************************************/
void log_page_formatted_dump(unsigned char *data_buffer, int length, int printmask)
{
    log_page_parser_t parser;
    unsigned char *log_parameter_p;
    unsigned int param_code;
    unsigned int param_len;
    unsigned long long value;

    if (printmask & PRT_FORMATTED_DATA)       
    {    
        printf("%s\n", format_log_page_data_page_header(data_buffer));
    
        log_page_initialize_parser(&parser, data_buffer, length);
    
        while (log_page_get_next_parameter(&parser, &log_parameter_p))
        {
            log_page_get_parameter(log_parameter_p, &param_code, &param_len, &value, true);
    
            char *description = "";
            printf("      %s\n", format_log_page_parameter_code_string(param_code,value,description));
        }
    }
}

/*!****************************************************************************
 * @fn      page02_formatted_dump()
 *
 * @brief
 * This function prints out the Write Error Counters log page in a pretty
 * formatted output.
 *
 * @param data_buffer - pointer to attribute data buffer.
 * @param length - length of the attribute data buffer.
 *
 * @return
 *  None
 *
 * HISTORY
 *  10/13/15 :  Wayne Garrett -- Created.
 *
 *****************************************************************************/
void page02_formatted_dump(unsigned char *data_buffer, int length, int printmask)
{
    log_page_parser_t parser;
    unsigned char *log_parameter_p;
    unsigned int param_code;
    unsigned int param_len;
    unsigned long long value;

    if (printmask & PRT_FORMATTED_DATA)
    {
        printf("%s\n", format_log_page_data_page_header(data_buffer));
    
        log_page_initialize_parser(&parser, data_buffer, length);
    
        while (log_page_get_next_parameter(&parser, &log_parameter_p))
        {
            log_page_get_parameter(log_parameter_p, &param_code, &param_len, &value, true);
            char *description;
    
            switch(param_code)
            {
            case 0x0000:
                description = "Errors corrected w/out substantial delay";
                break;
            case 0x0001:
                description = "Errors corrected w/ possible delay";
                break;
            case 0x0002:
                description = "Total re-writes";
                break;
            case 0x0003:
                description = "Total errors corrected";
                break;
            case 0x0004:
                description = "Total times correction algorithm processed";
                break;
            case 0x0005:
                description = "Total bytes processed";
                break;
            case 0x0006:
                description = "Total uncorrected errors";
                break;
            default:
                description = "";
                break;
            }
            
            printf("      %s\n", format_log_page_parameter_code_string(param_code,value,description));
        }
    }
}

/*!****************************************************************************
 * @fn      page03_formatted_dump()
 *
 * @brief
 * This function prints out the Read Error Counters log page in a pretty
 * formatted output.
 *
 * @param data_buffer - pointer to attribute data buffer.
 * @param length - length of the attribute data buffer.
 *
 * @return
 *  None
 *
 * HISTORY
 *  10/13/15 :  Wayne Garrett -- Created.
 *
 *****************************************************************************/
void page03_formatted_dump(unsigned char *data_buffer, int length, int printmask)
{
    log_page_parser_t parser;
    unsigned char *log_parameter_p;
    unsigned int param_code;
    unsigned int param_len;
    unsigned long long value;

    if (printmask & PRT_FORMATTED_DATA)
    {
        printf("%s\n", format_log_page_data_page_header(data_buffer));
    
        log_page_initialize_parser(&parser, data_buffer, length);
    
        while (log_page_get_next_parameter(&parser, &log_parameter_p))
        {
            log_page_get_parameter(log_parameter_p, &param_code, &param_len, &value, true);
            char *description;
    
            switch(param_code)
            {
            case 0x0000:
                description = "Errors corrected w/out substantial delay";
                break;
            case 0x0001:
                description = "Errors corrected w/ possible delay";
                break;
            case 0x0002:
                description = "Total re-reads";
                break;
            case 0x0003:
                description = "Total errors corrected";
                break;
            case 0x0004:
                description = "Total times correction algorithm processed";
                break;
            case 0x0005:
                description = "Total bytes processed";
                break;
            case 0x0006:
                description = "Total uncorrected errors";
                break;
            default:
                description = "";
                break;
            }
    
            printf("      %s\n", format_log_page_parameter_code_string(param_code,value,description));
        }
    }
}

/*!****************************************************************************
 * @fn      page05_formatted_dump()
 *
 * @brief
 * This function prints out the Verify Error Counters log page in a pretty
 * formatted output.
 *
 * @param data_buffer - pointer to attribute data buffer.
 * @param length - length of the attribute data buffer.
 *
 * @return
 *  None
 *
 * HISTORY
 *  10/13/15 :  Wayne Garrett -- Created.
 *
 *****************************************************************************/
void page05_formatted_dump(unsigned char *data_buffer, int length, int printmask)
{
    log_page_parser_t parser;
    unsigned char *log_parameter_p;
    unsigned int param_code;
    unsigned int param_len;
    unsigned long long value;

    if (printmask & PRT_FORMATTED_DATA)
    {
        printf("%s\n", format_log_page_data_page_header(data_buffer));    
    
        log_page_initialize_parser(&parser, data_buffer, length);
    
        while (log_page_get_next_parameter(&parser, &log_parameter_p))
        {
            log_page_get_parameter(log_parameter_p, &param_code, &param_len, &value, true);
            char *description;
    
            switch(param_code)
            {
            case 0x0000:
                description = "Errors corrected w/out substantial delay";
                break;
            case 0x0001:
                description = "Errors corrected w/ possible delay";
                break;
            case 0x0002:
                description = "Total re-verifies";
                break;
            case 0x0003:
                description = "Total errors corrected";
                break;
            case 0x0004:
                description = "Total times correction algorithm processed";
                break;
            case 0x0005:
                description = "Total bytes processed";
                break;
            case 0x0006:
                description = "Total uncorrected errors";
                break;
            default:
                description = "";
                break;
            }
    
            printf("      %s\n", format_log_page_parameter_code_string(param_code,value,description));
        }
    }
}


/*!****************************************************************************
 * @fn      page11_formatted_dump()
 *
 * @brief
 * This function prints out the Verify Error Counters log page in a pretty
 * formatted output.
 *
 * @param data_buffer - pointer to attribute data buffer.
 * @param length - length of the attribute data buffer.
 *
 * @return
 *  None
 *
 * HISTORY
 *  10/13/15 :  Wayne Garrett -- Created.
 *
 *****************************************************************************/
void page11_formatted_dump(unsigned char *data_buffer, int length, int printmask)
{
    log_page_parser_t parser;
    unsigned char *log_parameter_p;
    unsigned int param_code;
    unsigned int param_len;
    unsigned long long value;

    if (printmask & PRT_FORMATTED_DATA)
    {
        printf("%s\n", format_log_page_data_page_header(data_buffer));
    
        log_page_initialize_parser(&parser, data_buffer, length);
    
        while (log_page_get_next_parameter(&parser, &log_parameter_p))
        {
            char *description;        
    
            log_page_get_parameter(log_parameter_p, &param_code, &param_len, &value, true);        
    
            switch(param_code)
            {
            case 0x0001:
                description = "Percentage Used Endurance Indicator";
                break;
            default:
                description = "";
                break;
            }
    
            printf("      %s\n", format_log_page_parameter_code_string(param_code,value,description));
        }
    }
}


/*!****************************************************************************
 * @fn      page2f_formatted_dump()
 *
 * @brief
 * This function prints out the Informational Exceptions log page in a pretty
 * formatted output.
 *
 * @param data_buffer - pointer to attribute data buffer.
 * @param length - length of the attribute data buffer.
 *
 * @return
 *  None
 *
 * HISTORY
 *  10/13/15 :  Wayne Garrett -- Created.
 *
 *****************************************************************************/
void page2f_formatted_dump(unsigned char *data_buffer, int length, int printmask)
{
    log_page_parser_t parser;
    unsigned char *log_parameter_p;
    unsigned int param_code;
    unsigned int param_len;
    unsigned long long value;

    if (printmask & PRT_FORMATTED_DATA)
    {
        printf("%s\n", format_log_page_data_page_header(data_buffer));
    
        log_page_initialize_parser(&parser, data_buffer, length);
    
        while (log_page_get_next_parameter(&parser, &log_parameter_p))
        {
            char *description;        
    
            log_page_get_parameter(log_parameter_p, &param_code, &param_len, &value, true);        
    
            switch(param_code)
            {
            case 0x0000:   /*TODO: this param is a struct of fields.   It should be displayed per T10 spec*/
                description = "Informational Exceptions General";
                break;
    
                /* NOTE,  The following code only applies to Samsung RDX, REX, and forward.   If another vendor dumps these they probably won't mean the same thing. */
            case 0x0001:
                description = "";
                break;
    
            case 0x0005:
                description = "";
                break;
    
            case 0x0009:
                description = "";
                break;
    
            case 0x000c:
                description = "";
                break;
    
            case 0x000e:
                description = "";
                break;
    
            case 0x000f:
                description = "";
                break;
    
            case 0x0010:
                description = "";
                break;
    
            case 0x0011:
                description = "";
                break;
    
            case 0x00ad:
                description = "";
                break;
    
            case 0x00b2:
                description = "";
                break;
    
            case 0x00b4:
                description = "";
                break;
    
            case 0x00b5:
                description = "";
                break;
    
            case 0x00b6:
                description = "";
                break;
    
            case 0x00bb:
                description = "";
                break;
    
            case 0x00be:
                description = "";
                break;
    
            case 0x00c2: /*194*/
                description = "Samsung - Drive Temp";
                break;
    
            case 0x00c3:
                description = "";
                break;
    
            case 0x00c6:
                description = "";
                break;
    
            case 0x00c7:
                description = "";
                break;
    
            case 0x00c9:
                description = "";
                break;
    
            case 0x00ca:
                description = "";
                break;
    
            case 0x00e9: /*233*/
                description = "Samsung - Host Sector Writes";
                break;
    
            case 0x00ea: /*234*/
                description = "Samsung - Host Sector Reads";
                break;
    
            case 0x00eb: /*235*/
                description = "Samsung - NAND Writes";
                break;
    
            case 0x00f0:
                description = "";
                break;
    
            default:
                description = "";
                break;
            }
    
            printf("      %s\n", format_log_page_parameter_code_string(param_code,value,description));
        }
    }
}


/*!****************************************************************************
 * @fn      page37_formatted_dump()
 *
 * @brief
 * This function prints out a vendor specific SMART log page in a pretty
 * formatted output.
 *
 * @param data_buffer - pointer to attribute data buffer.
 * @param lenght - length of the attribute data buffer.
 *
 * @return
 *  None
 *
 * HISTORY
 *  10/13/15 :  Wayne Garrett -- Created.
 *
 *****************************************************************************/
void page37_formatted_dump(unsigned char *data_buffer, int length, int printmask)
{
    log_page_parser_t parser;
    unsigned char *log_parameter_p;
    unsigned int param_code;
    unsigned int param_len;
    unsigned long long value;

    if (printmask & (PRT_FORMATTED_DATA|PRT_WA_DATA))
    {
        printf("%s\n", format_log_page_data_page_header(data_buffer));
    
        log_page_initialize_parser(&parser, data_buffer, length);
    
        while (log_page_get_next_parameter(&parser, &log_parameter_p))
        {
            char *description;        
    
            log_page_get_parameter(log_parameter_p, &param_code, &param_len, &value, false);
           
            switch(param_code)
            {
            case 0x0000:     
                /* As of today, only Hitachi SCP supports this page & param.  To keep this simply, we will
                   always dump this info if this page is found.  It's up to post processing parser to verify
                   it is indeed from a Hitachi SCP drive.*/
                description = "Hitachi SCP - vendor specific";
                printf("      %s\n", format_log_page_parameter_code_string(param_code,value,description));       
                print_log_page_37_for_hitachi(data_buffer, log_parameter_p, param_len, "         ");
                break;
            default:
                break;
            }
        }
    }
}


/*!****************************************************************************
 * @fn      initialize_log_2f()
 *
 * @brief
 * This function initalizes a log page data structure
 *
 * @param log_page_2f_p - log page data structure
 *
 * @return
 *  None
 * 
 * @author
 *  10/13/15 :  Wayne Garrett -- Created.
 *
 *****************************************************************************/
void initialize_log_2f(log_2f_t *log_2f_p)
{
    memset((char *)log_2f_p, 0xff, sizeof(log_2f_t));
    log_2f_p->host_writes = INVALID_NUM;
    log_2f_p->nand_writes = INVALID_NUM;
}

/*!****************************************************************************
 * @fn      initialize_log_30()
 *
 * @brief
 * This function initalizes a log page data structure
 *
 * @param log_page_30_p - log page data structure
 *
 * @return
 *  None
 * 
 * @author
 *  10/13/15 :  Wayne Garrett -- Created.
 *
 *****************************************************************************/
void initialize_log_30(log_30_t *log_30_p)
{
    memset((char *)log_30_p, 0xff, sizeof(log_30_t));
    log_30_p->host_writes = INVALID_NUM;
    log_30_p->nand_writes = INVALID_NUM;
}

/*!****************************************************************************
 * @fn      initialize_log_31()
 *
 * @brief
 * This function initalizes a log page data structure
 *
 * @param log_page_31_p - log page data structure
 *
 * @return
 *  None
 * 
 * @author
 *  10/13/15 :  Wayne Garrett -- Created.
 *
 *****************************************************************************/
void initialize_log_31(log_31_t *log_31_p)
{
    memset((char *)log_31_p, 0xff, sizeof(log_31_t));
    log_31_p->host_writes = INVALID_NUM;
    log_31_p->nand_writes = INVALID_NUM;
}

/*!****************************************************************************
 * @fn      initialize_log_37()
 *
 * @brief
 * This function initalizes a log page data structure
 *
 * @param log_page_37_p - log page data structure
 *
 * @return
 *  None
 * 
 * @author
 *  10/13/15 :  Wayne Garrett -- Created.
 *
 *****************************************************************************/
void initialize_log_37(log_37_t *log_37_p)
{
    memset((char *)log_37_p, 0xff, sizeof(log_37_t));
    log_37_p->host_writes = INVALID_NUM;
    log_37_p->nand_writes = INVALID_NUM;
}

/*!****************************************************************************
 * @fn      initialize_drive_info()
 *
 * @brief
 * This function initalizes drive info data structure
 *
 * @param drive_info_p - structure to be initialized.
 * @param fru_p - ptr to fru header info.
 *
 * @return
 *  None
 * 
 * @author
 *  10/13/15 :  Wayne Garrett -- Created.
 *
 *****************************************************************************/
void initialize_drive_info(drive_info_t *drive_info_p, fru_head_t *fru_p)
{
    memset((char *)drive_info_p, 0xff, sizeof(drive_info_t));
    drive_info_p->fru = *fru_p;
    initialize_log_2f(&drive_info_p->log_2f);
    initialize_log_30(&drive_info_p->log_30);
    initialize_log_31(&drive_info_p->log_31);
    initialize_log_37(&drive_info_p->log_37);
}


/*!****************************************************************************
 * @fn      is_tla_in_table()
 *
 * @brief
 * This function checks if tla is found in the given table.
 *
 * @author
 *  10/13/15 :  Wayne Garrett -- Created.
 *
 *****************************************************************************/
bool is_tla_in_table(unsigned char *const tla, const tla_table_entry_t *const table_p, int num_entries)
{
    int i=0;
    int last_entry = num_entries-1;

    for (i=0; i<last_entry; i++)
    {
        if (strncmp((const char *)tla, (const char *)table_p[i].tla, 9)==0)
        {
            return true;
        }
    }
    return false;
}

/*!****************************************************************************
 * @fn      is_scp_drive()
 *
 * @brief
 * This function checks drive is a Hitachi Sunset Cove Plus
 *
 * @author
 *  10/13/15 :  Wayne Garrett -- Created.
 *
 *****************************************************************************/
bool is_scp_drive(unsigned char *const tla)
{
    int num_entries = (int)(sizeof(g_tla_table_scp) / sizeof(g_tla_table_scp[0]));

    return is_tla_in_table(tla, g_tla_table_scp, num_entries);
}

/*!****************************************************************************
 * @fn      is_buckhorn_drive()
 *
 * @brief
 * This function checks drive is a Micron Buckhorn drive
 *
 * @author
 *  10/13/15 :  Wayne Garrett -- Created.
 *
 *****************************************************************************/
bool is_buckhorn_drive(unsigned char *const tla)
{
    int num_entries = (int)(sizeof(g_tla_table_buckhorn) / sizeof(g_tla_table_buckhorn[0]));

    return is_tla_in_table(tla, g_tla_table_buckhorn, num_entries);
}

/*!****************************************************************************
 * @fn      is_rdx_drive()
 *
 * @brief
 * This function checks drive is a Samsung RDX drive
 *
 * @author
 *  10/13/15 :  Wayne Garrett -- Created.
 *
 *****************************************************************************/
bool is_rdx_drive(unsigned char *const tla)
{
    int num_entries = (int)(sizeof(g_tla_table_rdx) / sizeof(g_tla_table_rdx[0]));

    return is_tla_in_table(tla, g_tla_table_rdx, num_entries);
}
