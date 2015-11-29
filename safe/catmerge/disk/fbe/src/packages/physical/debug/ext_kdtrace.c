/***************************************************************************
 * Copyright (C) EMC Corporation 1999-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*
 * kdtrace.c - k10 flare debug extension to display circular trace buffer
 *
 * Greg Schaffer 1999/08/12 - created
 * Jim Williams  2000/05/04 - alterations and additions
 * Greg Schaffer 2001/02/19 - phase2: Ktrace_info/cirbuf/altbuf enhancements
 * Greg Schaffer 2002/06/11 - multiple ring merge
 */
/*
 * CONFIGURATION #defines
 */
#define SYMBOL_LOOKUP "ktrace!"
#define DEFAULT_SLEEP 500
#define DEFAULT_COUNT 64
#define ST_BUF_CNT 4
#define EPSILON  0x1000

#define DBG_DPRINTF dbg_dprintf
#include "compare.h" 

/*
 * INCLUDE files
 */
#include <stdarg.h>
#include <stdio.h>

#include "dbgext.h"
#define KT_TABLE_S
#include "ktrace.h"
#include "ktrace_structs.h"
#include "strOpt.h"
#include "jfwstr.h"
#include "bool.h"
#include "regex-local.h"
#include "EmcPAL_Memory.h"
#include "user_redefs.h"

#if !defined(KTRACE_USER_RING_SIZE)
#define MAX_INT_32                  0xFFFFFFFF
#endif

#define CLEN        KT_STR_LEN

//#include <winbase.h> // conflicts - inline needed portions

typedef struct default_struct
{
    DWORD dwCount;
    DWORD dwPos;
    KTRACE_ring_id_T ring;
    bool do_4hex;
    bool do_all;
    bool do_caseless;
    bool do_debug;
    bool do_inverse;
    bool do_number;
    bool do_select;
    bool do_systime;
    bool do_thread;
    bool do_cpu;
    bool do_tstamp;
    bool do_all_rings;
    bool do_label;
    bool do_quiet;
    int count;
    int dreamTime;
} default_T;
/*
 * MACRO FUNCTIONS
 */

/*
 * addrsize (m_obj)
 *
 * DESCRIPTION: This macro takes as an argument the address of an item.
 *   it generates the address of the item followed by the size of the
 *   item separated by a ','.
 *
 * PARAMETER
 *   m_obj
 *
 * GLOBALS: none
 *
 * CALLS:
 *   sizeof()
 *
 * RETURNS
 *
 * ERRORS 
 */
#define addrsize(m_obj) \
  m_obj, sizeof (*(m_obj))


/*
 * FUNCTION PROTOTYPES
 */
void            bufBegin (void);
void            bufItem (char* formatPA, ...);
bool            bufSelectDisplay (void);
void            caseCheck(bool alwaysA);
bool            commonStrOpt (char *argPA,
                  const char *const optstrPA,
                  const char *const helpTextPA);
void            dbg_dprintf (char*format, ...);
bool            displayDefaults (void);
bool            displayRingHeader (const KTRACE_ring_id_T ringA);
bool            displayRingItem (const KTRACE_ring_id_T ringA,
                                 const unsigned int sloatA,
                 TRACE_RECORD* itemPA);
void            factoryDefaults (void);
bool            getRingItem (const KTRACE_ring_id_T ringA,
                 const int slotA, TRACE_RECORD* itemPA);
void            help_kdefault (void);
void            help_kselect (void);
void            help_ktail (void);
void            help_ktrace (void);
void            hex08x (ULONG64 x, char *s);
bool            findKtrace_info (void);
KTRACE_ring_id_T matchRing (const char *const namePA);
bool updateKtrace_info (void);

/*
 * GLOBAL DATA
 */

DWORD           dwCount;        // slots to process
ULONG           dwPos;
KTRACE_ring_id_T ring;
bool            do_4hex;        // -4 = show 4 hex args before string
bool            do_all;         // -a show all in ring buffer (less select)
bool            do_caseless;    // -i encountered
//bool          do_debug
bool            do_inverse;     // -v
bool            do_number;      // -n encountered
bool            do_select;      // -x encountered
bool            do_systime;     // -T encountered
bool            do_thread;      // -P NOT encountered
bool            do_cpu;         // -C encountered
bool            do_tstamp;      // -t encountered
bool            do_altbuf;      // -A (use alternate buffer)
bool            do_slot0;       // -0 start at slot0
bool            do_all_rings;   // -r all = show all ring buffers
bool            do_label;       // -l display ring label
bool            do_quiet;       // -q quiet mode (only timestamp and message)

//int count;
int dreamTime;

char            bufItemBuffer[CLEN*6] = EmcpalEmpty;
char           *bufItemP;
char            buf[CLEN] = EmcpalEmpty;       // command buffer
KTRC_info_T     _Ktrace_info;


ULONG           usec;            // microsecond var
double          rtc_usec;
double          rtc_round;
double          rtc_100nsec;
double          rtc_100nrnd;
TSTAMP          boot_tstamp;
EMCUTIL_SYSTEM_TIME boot_systime;
ULONG64         TARADDR_KTRACE_info;
TRACE_RECORD    currTraceRecord; // trace record
static TSTAMP   prev_ts;         // previous timestamp value
static ULONG    prev_wDay;       // previous day

char            fold[256];
char            pattern[256] = {'\000'};
regex_t         patBuf;
bool            patternOK = false;
bool            caselessAtCompile = false;

ULONG           infoOffset = 0;
csx_size_t infoSize = 0;
ULONG           ti_cirbuf_offset = 0;
ULONG           ti_lost_offset = 0;
ULONG           ti_flush_offset = 0;
CHAR            ti_name[KTRACE_RING_CNT][256] = EmcpalEmpty;
ULONG64         ti_cirbuf[KTRACE_RING_CNT] = EmcpalEmpty;
ULONG64         ti_altbuf[KTRACE_RING_CNT] = EmcpalEmpty;

#define TRC_RING_ENUM_MAX (TRC_K_MAX+1)

// Support for multiple ring merge
ULONG _kt_rings;
struct _kt_s 
{
    ULONG slot;
    ULONG valid;
    TSTAMP prev_tr_stamp;
    TRACE_RECORD record;
} _kt[KTRACE_RING_CNT];

#pragma data_seg ("EXT_HELP$4kdefault")
char            usageMsg_kdefault[] =    // command line summary
    "!kdefault -- set defaults for ktrace & ktail\n"
    " [-4 {on|off}] [-a {on|off}] [-i {on|off}] [-n {on|off}] [-t {on|off}]\n"
    " [-v {on|off}] [-x {on|off}] [-D {on|off}] [-I {on|off}] [-P {on|off}]\n"
    " [-C {on|off}] [-T {on|off}] [-h] [-F] [-H] [-V] [-c COUNT] [-s uSEC]\n"
    " [-L LAST] [-S SHOW] [-r{std|start|io|redir|user|traffic|eventlog|hcfg|hport|psm|envpoll|xors|xor|mlu|cbfs|peer|thrpri|all}]\n\n"
    "  -4 -- Abhaya's display format       -C -- display CPU number\n"
    "  -a -- show all in ring buffer       -A -- display alt buffer\n"
    "  -c -- ktail display old records     -D -- display internal debug\n"
    "  -h -- display this message          -F -- restore factory defaults\n"
    "  -i -- caseless regex compares       -H -- display all k* help\n"
    "  -n -- display entry numbers         -I -- cased regex compares\n"
    "  -r -- ring buffer                   -L -- ktrace LAST count\n"
    "  -s -- ktail sleep time              -P -- suppress process thread id\n"
    "  -t -- display timestamps            -S -- ktrace SHOW count\n"
    "  -v -- skip regex matches            -T -- display clock time\n"
    "  -x -- filter using regex\n          -V -- version\n"
    "  no options -- show defaults\n"
    "\n";

#pragma data_seg ("EXT_HELP$4kselect")
char            usageMsg_kselect[] =
    "!kselect -- set regular expression for ktrace/ktail output winnowing\n"
    " [-h] [-i] [-v] [-x] [-I] <regex>\n\n"
    "  -h -- display this message          -x -- filter using regex\n"
    "  -i -- caseless regex compares       -I -- cased regex compares\n"
    "  -v -- skip regex matches\n\n";

#pragma data_seg ("EXT_HELP$4ktail")
char        usageMsg_ktail[] =    // command line summary
    "!ktail [-0] [-4] [-a] [-h] [-i] [-n] [-t] [-v] [-x] [-I] [-P] [-C] [-T]\n"
    " [-c COUNT [-r{std|start|io|redir|user|traffic|eventlog|hcfg|hport|psm|envpoll|xors|xor|mlu|cbfs|peer|thrpri|all}] [-s uSEC]\n\n\n"
    "  -0 -- start with slot 0\n"
    "  -4 -- show 4 hex arguments before string\n"
    "  -a -- show all in ring buffer\n"
    "  -c -- number of existing records to display before new records\n"
    "  -h -- display this message\n"
    "  -i -- caseless regex compares\n"
    "  -I -- cased regex compares\n"
    "  -l -- display ring label\n"
    "  -n -- show slot numbers of entries\n"
    "  -P -- suppress display of process thread number\n"
    "  -C -- show CPU number\n"
    "  -q -- quiet mode (only timestamp and message)\n"
    "  -r -- ring to display, defaults to std\n"
    "  -s -- sleep time between retries in milliseconds\n"
    "  -t -- show 40-bits (10 hex bytes) of 64-bit timestamp\n"
    "  -T -- show clock time\n"
    "  -v -- skip entries that match regular expression\n"
    "  -x -- filter using regular expressions\n\n"
    "  Go back COUNT events in specified ring buffer, printing COUNT lines\n"
    "  then continue printing new events as they are posted.\n"
    "  When no new entries are found, wait uSEC before looking again\n"
    "  COUNT defaults to 64, uSEC to 500 milliseconds (1/2 second)\n\n";

#pragma data_seg ("EXT_HELP$4ktrace")
char            usageMsg_ktrace[] =    // command line help
    "!ktrace [-0] [-4] [-a] [-h] [-i] [-I] [-n] [-P] [-C] [-t] [-v] [-x]\n"
    " [-r{std|start|io|redir|user|traffic|eventlog|hcfg|hport|psm|envpoll|xors|xor|mlu|cbfs|peer|thrpri|all}] [LAST [SHOW]]\n\n"
    "  -0 -- start with slot 0\n"
    "  -4 -- show 4 hex arguments before string\n"
    "  -a -- show all in ring buffer\n"
    "  -A -- use altbuf instead of curbuf\n"
    "  -h -- display this message\n"
    "  -i -- caseless regex compares\n"
    "  -I -- cased regex compares\n"
    "  -l -- display ring label\n"
    "  -n -- show slot numbers of entries\n"
    "  -P -- suppress display of process thread number\n"
    "  -C -- show CPU number\n"
    "  -q -- quiet mode (only timestamp and message)\n"
    "  -r -- ring to display, defaults to std\n"
    "  -t -- show 40-bits (10 hex bytes) of 64-bit timestamp\n"
    "  -T -- show clock time\n"
    "  -v -- skip entries that match regular expression\n"
    "  -x -- filter using regular expressions\n\n"
    "  Go back LAST events in specified ring buffer, printing SHOW lines\n"
    "  SHOW defaults to LAST, which defaults to 64 lines\n"
    "\n";
#pragma data_seg ()


#pragma data_seg ("EXT_HELP$4ktstats")
char            usageMsg_ktstats[] =    // command line help
    "!ktstats\n"
    "Display various statistics on the SP ktrace resources.\n"
    "\n";
#pragma data_seg ()


/*
 * defaults
 *
 * DESCRIPTION: defaults for command line options
 *   Written to by this initialization and by !kdefaults and its
 *   callee's.  Individual data items read by !<*> commands
 *
 * NOTE: sync changes here with factoryDefaults() equivalent settings
 */
static default_T defaults =
{
    0,                // dwCount
    0,                // dwPos
    TRC_K_STD,        // ring
    false,            // do_4hex
    false,            // do_all
    false,            // do_caseless
    false,            // do_debug
    false,            // do_inverse
    false,            // do_number
    false,            // do_select
    true,             // do_systime
    true,             // do_thread
    false,            // do_cpu
    false,            // do_tstamp
    false,            // do_all_rings
    false,            // do_label
    false,            // do_quiet
    DEFAULT_COUNT,    // count
    DEFAULT_SLEEP,    // dreamTime
};

/*
 * CSX_DBG_EXT_DEFINE_CMD (kdefault)
 *
 * DESCRIPTION: This function sets and displays default values for data
 *   that is modified by input command lines.  It also supports some other
 *   module wide operations.
 *
 * PARAMETERS
 *   [args]        -- [R][*R] string containing command line
 *
 * GLOBALS
 *
 * CALLS
 *
 * RETURNS
 *
 * ERRORS
 *
 * HISTORY
 *   12-May-2000 jfw -- working version
 */
CSX_DBG_EXT_DEFINE_CMD (kdefault, "!kdefault\n" )
{
    int             tbool;    // temporary boolean flag
    KTRACE_ring_id_T tring;   // das ring number
    bool            something = false; // any options?
    char            optchar;  // command switch letter
    char           *arg = (char *) args;    // command line buffer pointer

    /*
     * process switches
     */
    (void) strOpt("", ""); // clear up stale state
    while ((optchar = strOpt (arg, "4:a:c:hi:n:r:s:t:v:x:C:D:FHI:L:P:S:T:V"))
           != strOpt_EOL)
    {
        switch (optchar)
        {
            case ('4'):     // -4 Abhaya's display format.
                if ((tbool = matchOnOff (stroptarg)) == -1)
                {
                    return;
                };
                defaults.do_4hex = tbool;
                something = true;
                break;

            case ('a'):     // -a -- show all ring buffer enties
                if ((tbool = matchOnOff (stroptarg)) == -1)
                {
                    return;
                };
                defaults.do_all = tbool;
                something = true;
                break;

            case ('c'):     // -c num -- count of old entries to display
                defaults.count = (int)GetArgument (stroptarg, 1);
                something = true;
                break;

            case ('h'):
                help_kdefault();
                return;

            case ('i'):     // -i (ignore case)
                if ((tbool = matchOnOff (stroptarg)) == -1)
                {
                    return;
                };
                defaults.do_caseless = tbool;
                caseCheck(false);
                something = true;
                break;

            case ('n'):     // -n (display entry numbers)
                if ((tbool = matchOnOff (stroptarg)) == -1)
                {
                    return;
                };
                defaults.do_number = tbool;
                something = true;
                break;

            case ('r'):     // -r ring -- ring to use
                if (!findKtrace_info ()) // get address of "Ktrace_info"
                {
                    return;     // failed, quit
                };
                if ((tring = matchRing (stroptarg)) == -1)
                {
                    return;
                };
                defaults.ring = tring;
                something = true;
                break;

            case ('s'):     // -s num -- uSecs to wait for new data
                defaults.dreamTime = (int)GetArgument (stroptarg, 1);
                something = true;
                break;

            case ('t'):     // -t (display timestamps)
                if ((tbool = matchOnOff (stroptarg)) == -1)
                {
                    return;
                };
                defaults.do_tstamp = tbool;
                something = true;
                break;

            case ('v'):     // -v (invert selection)
                if ((tbool = matchOnOff (stroptarg)) == -1)
                {
                    return;
                };
                defaults.do_inverse = tbool;
                defaults.do_select = tbool;
                something = true;
                break;

            case ('x'):     // -x (do regular expression selection)
                if ((tbool = matchOnOff (stroptarg)) == -1)
                {
                    return;
                };
                defaults.do_select = tbool;
                something = true;
                break;

            case ('C'):     // -C (display CPU number)
                if ((tbool = matchOnOff (stroptarg)) == -1)
                {
                    return;
                };
                defaults.do_cpu = tbool;
                something = true;
                break;

            case ('D'):     // -D (display internal debug messages)
                if ((tbool = matchOnOff (stroptarg)) == -1)
                {
                    return;
                };
                defaults.do_debug = tbool;
                something = true;
                break;

            case ('F'):     // -F restore to factory defaults
                factoryDefaults ();
                something = true;
                break;

            case ('H'):     // show all help info
                help_kdefault();
                help_kselect();
                help_ktail();
                help_ktrace();
                return;

            case ('I'):     // -I (match case)
                if ((tbool = matchOnOff (stroptarg)) == -1)
                {
                    return;
                };
                defaults.do_caseless = !tbool;
                caseCheck(false);
                something = true;
                break;

            case ('L'):     // -L ktrace LAST value
                defaults.dwPos = (DWORD)GetArgument (stroptarg, 1);
                something = true;
                break;

            case ('P'):     // -P (DO NOT display process thread)
                if ((tbool = matchOnOff (stroptarg)) == -1)
                {
                    return;
                };
                defaults.do_thread = !tbool;
                something = true;
                break;

            case ('S'):     // -S ktrace SHOW value
                defaults.dwCount = (DWORD)GetArgument (stroptarg, 1);
                something = true;
                break;

            case ('T'):     // -T (display clock time)
                if ((tbool = matchOnOff (stroptarg)) == -1)
                {
                    return;
                };
                defaults.do_systime = tbool;
                something = true;
                break;

#ifdef ALAMOSA_WINDOWS_ENV
            case ('V'):     // -V display version info
                csx_dbg_ext_print  (__FILE__ ": generated on "  __TIMESTAMP__
                         " using VC++ version %d\n", _MSC_VER);
                something = true;
                break;
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */

            case (':'):
                csx_dbg_ext_print  ("Missing value for switch -%c:\n", stroptopt);
                help_kdefault();
                return;

            case ('?'):
                csx_dbg_ext_print  ("Unknown switch: '-%c' (0%02o)\n", stroptopt, stroptopt);
                help_kdefault();
                return;

            default:
                csx_dbg_ext_print  ("Unexpected return value from strOpt() -- 0x%08X\n",
                         optchar);
                return;
        } /* switch */
    } /* while */

    /*
     * If we did not "do" anything (command line was "empty"
     * then display the current settings.
     */
    if (!something)
    {
        displayDefaults();
    };
} /* CSX_DBG_EXT_DEFINE_CMD (kdefault) */

/*
 * CSX_DBG_EXT_DEFINE_CMD (kselect)
 *
 */
CSX_DBG_EXT_DEFINE_CMD (kselect, "!kselect")
{
    char           *arg = (char *) args;    // command line buffer pointer
    size_t string_len;

    patternOK = false;

    /*
     * set defaults
     */
    do_caseless = defaults.do_caseless;
    do_inverse  = defaults.do_inverse;
    do_select   = defaults.do_select;

    if (!commonStrOpt (arg, "hIivx", usageMsg_kselect))
    {
        return;
    };

    /*
     * setting -x or -v here is equivalent of setting them on in kdefault
     */
    defaults.do_inverse = do_inverse;
    defaults.do_select  = do_select;

    stroptarg = skipWSP (stroptarg);
    trimWSP (stroptarg);

    if ((stroptarg[0] == '"')
        && (stroptarg[strlen(stroptarg) - 1] == '"'))
    {
        stroptarg++;
        stroptarg[strlen(stroptarg) - 1] = '\000';
    };

    string_len = MIN(sizeof(pattern)-1, strlen(stroptarg));
    strncpy (pattern, stroptarg, string_len);
    pattern[string_len] = '\0'; 
    caseCheck (true);

} /* CSX_DBG_EXT_DEFINE_CMD (kselect) */

//
//  Command:
//      ktrace count
//
//  Description:
//      Print trace records
//
//  Arguments:
//  IN HANDLE   hCurrentProcess
//  IN HANDLE   hCurrentThread
//  IN ULONG    dwCurrentPc
//  IN ULONG    dwProcessor
//  IN PCSTR    args
//
//  Return Value:
//      NONE
//
CSX_DBG_EXT_DEFINE_CMD (ktrace, "!ktrace [-0] [-4] [-a] [-h] [-i] [-I] [-n] [-P] [-C] [-t] [-v] [-x]\n  [-r{std|start|io|redir|user|traffic|eventlog|hcfg|hport|psm|envpoll|xors|xor|mlu|cbfs|thrpri|pe [LAST [SHOW]\n   -0 -- start with slot 0\n   -4 -- show 4 hex arguments before string\n   -a -- show all in ring buffer\n   -A -- use altbuf instead of curbuf\n   -h -- display this message\n   -i -- caseless regex compares\n   -I -- cased regex compares\n   -l -- display ring label\n   -n -- show slot numbers of entries\n   -P -- suppress display of process thread number\n   -C -- show CPU number\n   -q -- quiet mode (only timestamp and message)\n   -r -- ring to display, defaults to std\n   -t -- show 40-bits (10 hex bytes) of 64-bit timestamp\n   -T -- show clock time\n   -v -- skip entries that match regular expression\n   -x -- filter using regular expressions\n \n   Go back LAST events in specified ring buffer, printing SHOW linesi\n   SHOW defaults to LAST, which defaults to 64 lines\n ")
{
    DWORD           i;      // for index
    ULONG           slot;   // slot in ring buffer
    ULONG all_slots[KTRACE_RING_CNT];  // slot index holder for all record output
    char           *arg = (char *) args; // command line buffer pointer
    KTRACE_ring_id_T local_ring = TRC_RING_ENUM_MAX;
    LARGE_INTEGER   timestamp;  // buffer for part of timestamp
    TSTAMP shown_ts;

    globalCtrlC = FALSE;        // assume no ^C struck
    prev_ts = 0;
    if (!findKtrace_info ())    // get address of "Ktrace_info"
    {
        return;         // failed, quit
    };

    // process switches
  
    if (!commonStrOpt (arg, "04ahinlqr:tvxACIPT", usageMsg_ktrace))
    {
        return;
    };

    arg = stroptarg;        // remainder of the command line

    if ( do_all_rings )
    {
        for ( local_ring = 0; local_ring < TRC_RING_ENUM_MAX; local_ring++ )
        {
            _kt[local_ring].slot = 0;
            _kt_rings++;
        }
    }

    // multiple ring merge
    if (_kt_rings > 1)
    {
        KTRACE_ring_id_T mring;
        ULONGLONG msb;
        bool find_newest = false;

        msb = 1;
        msb <<= 63;

        //  Get count of entries to display
        if (do_all || do_slot0) // do_slot0 is not really valid for multi ring use
        {
            dwPos = MAX_INT_32;
            dwCount = MAX_INT_32;
        }
        else
        {
            dwPos = (ULONG)GetArgument (arg, 1);
            dwCount = (DWORD)GetArgument (arg, 2);
        }

        // Compute slot from relative start position
        if (dwPos == 0) // no value set to default
            dwPos = defaults.count;
        if (dwPos <= 0)  // greater than max size is handled later
            dwPos = MAX_INT_32;
        if ((dwCount <= 0) || (dwCount > dwPos))
            dwCount = dwPos;

        do_label = true;
        for(ring = 0; ring < KTRACE_RING_CNT; ring++)
        {
            all_slots[ring] = -1;
            if (_kt[ring].slot == -1)
            {
                continue;
            }
            // Compute slot from relative start position, read record.
            if (do_all)
            {
                /* this can be a race condition if the ti_slot increments before display - thats bad */
                slot = (_Ktrace_info.info[ring].ti_slot + 1) % _Ktrace_info.info[ring].ti_size;
            }
            else if (do_slot0)
            {
                slot = 0;
            }
            else
            {
                /* Check to see if the count is greater than the size of the buffer itself */
                if (dwCount >= _Ktrace_info.info[ring].ti_size)
                {
                    /* wrap to max for slot number */
                    slot = (_Ktrace_info.info[ring].ti_slot + 1) % _Ktrace_info.info[ring].ti_size;
                    /* this can be a race condition if the ti_slot increments before display thats bad */
                    all_slots[ring] = slot;
                } else {
                    /* count within buffer */
                    slot = (_Ktrace_info.info[ring].ti_slot - dwCount) % _Ktrace_info.info[ring].ti_size;
                    find_newest = true;                
                }                
            }
            // if wrapped to uninitialized record, start at slot 0.
            if (!getRingItem (ring, slot, &_kt[ring].record))
            {
                return;
            }
            /* if the count has pushed the slot passed a filled slot reset to zero to get all valid entries */
            if (_kt[ring].record.tr_id == 0)
            {
                slot = 0;
                if (!getRingItem (ring, slot, &_kt[ring].record))
                {
                    return;
                }
            }
            _kt[ring].slot = slot;
            _kt[ring].valid = TRUE;
            DBG_DPRINTF("\t***Starting ring Info*** ring=%d,, ti_slot=%d,ti_size=%d, slot=%d:\n", ring,_Ktrace_info.info[ring].ti_slot,_Ktrace_info.info[ring].ti_size,slot);
        }

        /* Find newest record in rings at the selected slot calculated from count, set to scan from there */
        if (find_newest)

        {
            mring = 0;
            shown_ts = 0; // set to no time selected
            for(ring = 0; ring < KTRACE_RING_CNT; ring++)
            {
                /* No records at all so don't test slot need this if you access _ktrace_info(9) fdb will crash */
                if(_kt[ring].slot == -1 || _Ktrace_info.info[ring].ti_slot == -1)
                {
                    /* Slot will equal current slot for this ring, move on to next ring */
                    continue;
                }
                /* need for test against all slots on a small buffer */
                if((_kt[ring].valid == TRUE) && (_kt[ring].slot != 0) && (_kt[ring].slot != all_slots[ring]))
                {    
                    timestamp.QuadPart=_kt[ring].record.tr_stamp;
                    DBG_DPRINTF("\t***Stamp Read*** ring=%d, %08x%08x:\n", ring, timestamp.HighPart,timestamp.LowPart);
                    if (_kt[mring].record.tr_stamp <= _kt[ring].record.tr_stamp)
                    {
                        mring = ring;
                        shown_ts = _kt[ring].record.tr_stamp; // newest
                    }
                }
            }
            if (shown_ts == 0)  //no ring was selected with largest time
            {
                find_newest = false;
            }
        }

        /* Adjust all the rings to the current display time stamp */
        if (find_newest)
        {
            timestamp.QuadPart=_kt[mring].record.tr_stamp;
            DBG_DPRINTF("\t*** Control TS is *** ring=%d, %08x%08x:\n", mring, timestamp.HighPart,timestamp.LowPart);                
            for(ring = 0; ring < KTRACE_RING_CNT; ring++)
            {
                /* Only adjust slot if not the ring with select time stamp */
                if(ring != mring)
                {                
                    /* No records at all so don't reset slot */
                    if(_kt[ring].slot == -1 || _Ktrace_info.info[ring].ti_slot == -1)
                    {
                        /* Slot will equal current slot for this ring, move on to next ring */
                        continue;
                    }
                    /* Invalidate each rings record ALL will be adjusted */
                    _kt[ring].valid = FALSE;
                    /* Set finding slot var to the current slot selected by user for the ring */
                    slot = _kt[ring].slot;
                    DBG_DPRINTF("\t***Starting Slot lookup *** ring=%d, slot=%d:\n", ring, slot);                
                    /* Loop incrementing up to ti_slot in the ring looking for the correct position 
                       relative to selected newest time stamp*/
                    while( slot != _Ktrace_info.info[ring].ti_slot)
                    {                        
                        /* Get ring record for this new slot */ 
                        if (!getRingItem (ring, slot, &_kt[ring].record))
                        {
                            return;
                        }
                        if (_kt[mring].record.tr_stamp < _kt[ring].record.tr_stamp)
                        {
                            /* a valid record was found, set valid/slot for this record and exit to next ring */
                            _kt[ring].slot = slot;
                            _kt[ring].valid = TRUE;
                            /* set slot to exit while loop */
                            slot = _Ktrace_info.info[ring].ti_slot;
                            timestamp.QuadPart=_kt[ring].record.tr_stamp;
                            DBG_DPRINTF("\t***Selected Relative TS*** ring=%d, %08x%08x:\n", ring, timestamp.HighPart,timestamp.LowPart);                
                            continue;
                        }
                        /* correct slot on wrap */
                        if (++slot >= _Ktrace_info.info[ring].ti_size)
                        {
                            slot = 0;
                        }
                    }
                    /* check if no valid slot was found and while ended - Next to reload it */
                    if(_kt[ring].valid != TRUE)
                    {
                        _kt[ring].slot = _Ktrace_info.info[ring].ti_slot;
                        if (!getRingItem (ring, _kt[ring].slot, &_kt[ring].record))
                        {
                            return;
                        }                        
                        _kt[ring].valid = TRUE;
                    }
                    DBG_DPRINTF("\t***TS relative Slot Selected *** ring=%d, slot=%d:\n", ring, _kt[ring].slot);
                }
            }
        }

        // Do the merge...
        while (!(CHECK_CONTROL_C  () || globalCtrlC))
        {
            // find oldest record, handling wrap
            mring = -1;
            for(ring = 0; ring < KTRACE_RING_CNT; ring++)
            {
                // if there are no items in the ring to begin with skip it.
                if ( _Ktrace_info.info[ring].ti_slot == -1 )
                {
                    _kt[ring].slot = -1;
                }

                if (_kt[ring].slot == -1)
                {
                    continue;
                }

                if ((mring == -1) ||
                    ((_kt[mring].record.tr_stamp > _kt[ring].record.tr_stamp) &&
                     (!(_kt[mring].record.tr_stamp & msb) ||
                      (_kt[ring].record.tr_stamp & msb))))
                {
                    mring = ring;
                }
            }
            if (mring == -1) 
            {
                break; // No more records to merge
            }
            ring = mring;
            // Display record and advance (check for end)
            timestamp.QuadPart=_kt[ring].record.tr_stamp;
            DBG_DPRINTF("\t\t\t*** Ring=%d, TS=%08x%08x:\n", ring, timestamp.HighPart,timestamp.LowPart);
            DBG_DPRINTF("%d,%05d: ", ring, _kt[ring].slot);
            displayRingItem(ring, _kt[ring].slot, &_kt[ring].record);
            if (_kt[ring].slot == _Ktrace_info.info[ring].ti_slot)
            {
                _kt[ring].slot = -1; // Done with this ring.
            }
            else
            {
                if (++_kt[ring].slot >= _Ktrace_info.info[ring].ti_size)
                {
                    _kt[ring].slot = 0;
                }
                if (!getRingItem (ring, _kt[ring].slot, &_kt[ring].record))
                {
                    return;
                }
            }
        }
        return;
    }

    // single ring display    
    //
    //  Get count of entries to display
    //
    if (do_all)
    {
        dwPos = _Ktrace_info.info[ring].ti_size;
        dwCount = _Ktrace_info.info[ring].ti_size;
    }
    else if (do_slot0)
    {
        dwPos = dwCount = _Ktrace_info.info[ring].ti_slot + 1;
    }
    else
    {
        dwPos = (ULONG)GetArgument (arg, 1);
        dwCount = (DWORD)GetArgument (arg, 2);
    }
    
    // Compute slot from relative start position
    if (dwPos == 0)
        dwPos = defaults.count;
    if (dwPos <= 0 || dwPos > _Ktrace_info.info[ring].ti_size)
        dwPos = _Ktrace_info.info[ring].ti_size;
    if ((dwCount <= 0) || (dwCount > dwPos))
        dwCount = dwPos;

    if ((ring == TRC_K_TRAFFIC) && do_all && !do_altbuf)  {
        ULONG64 addr = 0;
        ULONG lost = 0, flush = 0;
        addr = TARADDR_KTRACE_info + infoOffset + (infoSize*ring) + ti_lost_offset;
        csx_dbg_ext_read_in_len(addr, &lost, sizeof(ULONG));
        addr = TARADDR_KTRACE_info + infoOffset + (infoSize*ring) + ti_flush_offset;
        csx_dbg_ext_read_in_len(addr, &flush, sizeof(ULONG));
        csx_dbg_ext_print ("***** traffic lost %d entries and %d buffers\n",
                lost, ((flush > 1) ? (flush-1) : 0));
        if (!(flush > 1)) {
            // both ti_cirbuf and ti_altbuf
            dwPos *= 2;
            dwCount *= 2;
        } else {
            do_all = false;
            dwPos = dwCount = _Ktrace_info.info[ring].ti_slot + 1;
            csx_dbg_ext_print ("***** showing from slot0\n");
        }
    }

    if (!displayRingHeader (ring))
    {
        return;
    };

    slot = (_Ktrace_info.info[ring].ti_slot - dwPos) % _Ktrace_info.info[ring].ti_size;

    // if wrapped to uninitialized record, start at slot 0 and adjust count.
    if (!getRingItem (ring, slot, &currTraceRecord))
    {
        return;
    }
    if (currTraceRecord.tr_id == 0)
    {
        slot = (-1);
        dwCount = _Ktrace_info.info[ring].ti_slot + 1;
    }
    
    for (i = dwCount; i > 0; i--)
    {
        slot = (slot+1) % _Ktrace_info.info[ring].ti_size;
        if (CHECK_CONTROL_C  ()
            || globalCtrlC
            || !getRingItem (ring, slot, &currTraceRecord)
            || !displayRingItem (ring, slot, &currTraceRecord))
            return;
    }               /* for */
} /* CSX_DBG_EXT_DEFINE_CMD (ktrace) */

/*
 * CSX_DBG_EXT_DEFINE_CMD (ktail)
 *
 * DESCRIPTION: This function displays trace records as they are added to
 *   the specified ring buffer.  It is terminated by ctrl-C.
 *
 * PARAMETERS
 *   [args]        -- [R][*R]
 *
 * GLOBALS
 *
 * CALLS
 *
 * RETURNS
 *
 * ERRORS
 *
 * HISTORY
 *   12-May-2000 jfw -- working version
 *   30-Nov-2001 gss -- rewrite initial value of current, and waiting logic.
 */
CSX_DBG_EXT_DEFINE_CMD (ktail, "!ktail")
{
    ULONG  oldest;  // oldest slot number in selected ring
    ULONG  newest;  // newest slot number in selected ring
    ULONG  current; // current slot number in selected ring
    ULONG  ringSize; // number of slots in selected ring
    ULONG  count;   // number of old slots to display
    char   *arg = (char *) args;    // command line buffer pointer
    TSTAMP shown_ts;
    KTRACE_ring_id_T local_ring = TRC_RING_ENUM_MAX;
    LARGE_INTEGER   timestamp;  // buffer for part of timestamp
    ULONG ti_slot_offset = 0;

 restart:
    globalCtrlC = FALSE;        // assume no ^C struck
    prev_ts = 0;

    // calculate the ti_slot offset into TRACE_INFO structure now
    // since we'll be using this repeatedly.
    GETFIELDOFFSET(SYMBOL_LOOKUP "TRACE_INFO", "ti_slot", &ti_slot_offset);

    if (!findKtrace_info ())    // get address of "Ktrace_info"
    {
        return;         // failed, quit
    }

    // process command line
    if (!commonStrOpt (arg, "04ac:hinlqr:s:tvxCIPT", usageMsg_ktail))
    {
        return;
    }
    arg = stroptarg;        // remainder of the command line
    count = (ULONG)GetArgument (arg, 1);
    shown_ts = EPSILON;     // wait if ti_stamp == 0

    if ( do_all_rings )
    {
        for ( local_ring = 0; local_ring < TRC_RING_ENUM_MAX; local_ring++ )
        {
            _kt[local_ring].slot = 0;
            _kt_rings++;
        }
    }

    if (do_all)
    {
        count = _Ktrace_info.info[ring].ti_size - 1;
    }
    else if (do_slot0)
    {
        count = _Ktrace_info.info[ring].ti_slot + 1;
    }
    else if (!count)
    {
        count = defaults.count;
    }

    // multiple ring merge
    if (_kt_rings > 1)
    {
        KTRACE_ring_id_T mring;
        ULONG slot;
        ULONG all_slots[KTRACE_RING_CNT];  // slot index holder for all record output
        ULONGLONG msb;
        bool find_newest = false;

        msb = 1;
        msb <<= 63;

        do_label = true;
        for(ring = 0; ring < KTRACE_RING_CNT; ring++)
        {
            all_slots[ring] = -1;
            if (_kt[ring].slot == -1)
            {
                continue;
            }
            // Compute slot from relative start position, read record.
            if (do_all)
            {
                slot = (_Ktrace_info.info[ring].ti_slot + 1) % _Ktrace_info.info[ring].ti_size;
            }
            else if (do_slot0)
            {
                slot = 0;
            }
            else
            {
                /* Check to see if the count is greater than the size of the buffer itself */
                if (count >= _Ktrace_info.info[ring].ti_size)
                {
                    /* wrap to max for slot number */
                    /* could be a race condition here what if ti_slot increments as we print. */
                    slot = (_Ktrace_info.info[ring].ti_slot + 1) % _Ktrace_info.info[ring].ti_size;                
                    all_slots[ring] = slot;
                } else {
                    /* count within buffer */
                    slot = (_Ktrace_info.info[ring].ti_slot - count) % _Ktrace_info.info[ring].ti_size;
                    find_newest = true;                
                }                
            }
            // if wrapped to uninitialized record, start at slot 0.
            if (!getRingItem (ring, slot, &_kt[ring].record))
            {
                return;
            }
            if (_kt[ring].record.tr_id == 0)
            {
                slot = 0;
                if (!getRingItem (ring, slot, &_kt[ring].record))
                {
                    return;
                }
            }
            _kt[ring].slot = slot;
            _kt[ring].valid = TRUE;
        
            DBG_DPRINTF("\t***Starting ring Info*** ring=%d,, ti_slot=%d,ti_size=%d, slot=%d:\n", ring,_Ktrace_info.info[ring].ti_slot,_Ktrace_info.info[ring].ti_size,slot);                        
        }

        /* Find newest record in rings at the selected slot calculated from count, set to scan from there */
        if (find_newest)
        {
            mring = 0;
            shown_ts = 0; //set to no time selected
            for(ring = 0; ring < KTRACE_RING_CNT; ring++)
            {
                /* No records at all so don't test slot need this if you access _ktrace_info(9) fdb will crash */
                if(_kt[ring].slot == -1 || _Ktrace_info.info[ring].ti_slot == -1)
                {
                    /* Slot will equal current slot for this ring, move on to next ring */
                    continue;
                }
                /* need for test against all slots on a small buffer */
                if((_kt[ring].valid == TRUE) && (_kt[ring].slot != 0) && (_kt[ring].slot != all_slots[ring]))
                {    
                    timestamp.QuadPart=_kt[ring].record.tr_stamp;
                    DBG_DPRINTF("\t***Stamp Read*** ring=%d, %08x%08x:\n", ring, timestamp.HighPart,timestamp.LowPart);                
                    if (_kt[mring].record.tr_stamp <= _kt[ring].record.tr_stamp)
                    {
                        mring = ring;
                        shown_ts = _kt[ring].record.tr_stamp; // newest
                    }
                }
            }
            if (shown_ts == 0) // no ring was selected as largest time
            {
                find_newest = false;
            }
        }
        /* Adjust all the rings to the current display time stamp */
        if (find_newest)
        {
            timestamp.QuadPart=_kt[mring].record.tr_stamp;
            DBG_DPRINTF("\t*** Control TS is *** ring=%d, %08x%08x:\n", mring, timestamp.HighPart,timestamp.LowPart);                
            for(ring = 0; ring < KTRACE_RING_CNT; ring++)
            {
                /* Only adjust slot if not the ring with select time stamp */
                if(ring != mring)
                {                
                    /* No records at all so don't reset slot */
                    if(_kt[ring].slot == -1 || _Ktrace_info.info[ring].ti_slot == -1)
                    {
                        /* Slot will equal current slot for this ring, move on to next ring */
                        continue;
                    }
                    /* Invalidate each rings record ALL will be adjusted */
                    _kt[ring].valid = FALSE;
                    /* Set finding slot var to the current slot selected by user for the ring */
                    slot = _kt[ring].slot;
                    DBG_DPRINTF("\t***Starting Slot lookup *** ring=%d, slot=%d:\n", ring, slot);                
                    /* Loop incrementing up to ti_slot in the ring looking for the correct position 
                       relative to selected newest time stamp*/
                    while( slot != _Ktrace_info.info[ring].ti_slot)
                    {                        
                        /* Get ring record for this new slot */ 
                        if (!getRingItem (ring, slot, &_kt[ring].record))
                        {
                            return;
                        }                    
                        if (_kt[mring].record.tr_stamp < _kt[ring].record.tr_stamp)
                        {
                            /* a valid record was found, set valid/slot for this record and exit to next ring */
                            _kt[ring].slot = slot;
                            _kt[ring].valid = TRUE;
                            /* set slot to exit while loop */
                            slot = _Ktrace_info.info[ring].ti_slot;
                            timestamp.QuadPart=_kt[ring].record.tr_stamp;
                            DBG_DPRINTF("\t***Selected Relative TS*** ring=%d, %08x%08x:\n", ring, timestamp.HighPart,timestamp.LowPart);                
                            continue;
                        }
                        /* correct slot on wrap */
                        if (++slot >= _Ktrace_info.info[ring].ti_size)
                        {
                            slot = 0;
                        }
                    }
                    /* check if no valid slot was found and while ended - Next to reload it */
                    if(_kt[ring].valid != TRUE)
                    {
                        _kt[ring].slot = _Ktrace_info.info[ring].ti_slot;
                        if (!getRingItem (ring, _kt[ring].slot, &_kt[ring].record))
                        {
                            return;
                        }                        
                        _kt[ring].valid = TRUE;
                    }
                    DBG_DPRINTF("\t***TS relative Slot Selected *** ring=%d, slot=%d:\n", ring, _kt[ring].slot);                                
                }
            }
        }

        // Do the merge...
        while (!(CHECK_CONTROL_C  () || globalCtrlC)) // sleep loop
        {
            // find oldest record, handling wrap
            mring = -1;
            for(ring = 0; ring < KTRACE_RING_CNT; ring++)
            {
                if (_kt[ring].slot == -1)
                {
                    continue;
                }
                
                if (!_kt[ring].valid)
                {
                    ULONG64 info_p = 0;

                    // Get address location to read ti_slot from Ktrace_info structure 
                    info_p = TARADDR_KTRACE_info + infoOffset + (infoSize*ring) + ti_slot_offset;

                    if(csx_dbg_ext_read_in_len(info_p, &newest, sizeof(ULONG) ))
                    {
                        csx_dbg_ext_print ("Error in reading ti_slot field of TRACE_INFO at %llx @ ring %d \n", info_p, (int)ring);
                        return;
                    }

                    if ((newest == -1) ||
                        (_kt[ring].slot == ((newest+1) % _Ktrace_info.info[ring].ti_size)))
                    {
                        continue; // Empty or end of this ring
                    }
                    _Ktrace_info.info[ring].ti_slot = newest;
                    if (!getRingItem (ring, _kt[ring].slot, &_kt[ring].record))
                    {
                        goto restart;
                    }
                    // we've made the decision to print this record out
                    _kt[ring].valid = TRUE;
                }
                if ((mring == -1) ||
                    ((_kt[mring].record.tr_stamp > _kt[ring].record.tr_stamp) &&
                     (!(_kt[mring].record.tr_stamp & msb) ||
                      (_kt[ring].record.tr_stamp & msb))))
                {
                    mring = ring;
                }
            }
            if (mring == -1) 
            {
                csx_dbg_ext_rt_sleep_msecs (dreamTime);
                continue;
            }
            ring = mring;
            // Display record and advance (check for end)
            timestamp.QuadPart=_kt[ring].record.tr_stamp;
            DBG_DPRINTF("\t\t\t*** Ring=%d, TS=%08x%08x:\n", ring, timestamp.HighPart,timestamp.LowPart);
            DBG_DPRINTF("%d,%05d: ", ring, _kt[ring].slot);
            displayRingItem(ring, _kt[ring].slot, &_kt[ring].record);
            shown_ts = _kt[ring].record.tr_stamp;
            _kt[ring].valid = FALSE;

            // Advance slot
            current = _kt[ring].slot;
            if (++_kt[ring].slot >= _Ktrace_info.info[ring].ti_size)
            {
                _kt[ring].slot = 0;
            }
        }
        return;
    }

    // single ring !ktail

    // display header information about this ring
    if (!displayRingHeader (ring))
    {
        return;         // failed, quit.
    }

    // Find the newest trace record entry in the selected ring buffer.
    // Drop back by the value of count.  If not filled-in, start at 0.

    ringSize = _Ktrace_info.info[ring].ti_size;

    newest = _Ktrace_info.info[ring].ti_slot; // slot index of newest
    oldest = (ULONG) ((newest + 1) % ringSize);
    current = (ULONG) ((newest - count) % ringSize);
    if (!getRingItem (ring, current, &currTraceRecord))
    {
        return;
    }

    if (currTraceRecord.tr_id == 0)
    {
        current = 0;
    }
    DBG_DPRINTF ("ring %d, size %d, oldest %d, newest %d, current %d\n",
                 ring, ringSize, oldest, newest, current);
    // Loop until ^C is struck.
    while (!(CHECK_CONTROL_C  () || globalCtrlC)) // sleep loop
    {
        if (!getRingItem (ring, current, &currTraceRecord))
        {
            goto restart;
        }

        // if we've displayed newest record, wait for ti_slot to change
        if (currTraceRecord.tr_stamp < shown_ts)
        {
            ULONG64 info_p = 0;

            // Get address location to read ti_slot from Ktrace_info structure 
            info_p = TARADDR_KTRACE_info + infoOffset + (infoSize*ring) + ti_slot_offset;

            if(csx_dbg_ext_read_in_len(info_p, &newest, sizeof(ULONG) ))
            {
                csx_dbg_ext_print ("Error in reading ti_slot field of TRACE_INFO at %llx @ ring %d \n", info_p, (int)ring);
                return;
            }
            
            if (current == (ULONG) ((newest+1) % _Ktrace_info.info[ring].ti_size))
            {
                oldest = (ULONG) newest; // Wait for newest to change
                do
                {   
                    csx_dbg_ext_rt_sleep_msecs (dreamTime);  // no, take a nap

                    if(csx_dbg_ext_read_in_len(info_p, &newest, sizeof(ULONG) ))
                    {
                        csx_dbg_ext_print ("Error in reading ti_slot field of TRACE_INFO at %llx @ ring %d \n", info_p, (int)ring);
                        goto restart;
                    }

                    DBG_DPRINTF ("\t\t** sleep: current %d, newest %d **\n", current, newest);
                    if (CHECK_CONTROL_C ()) return;
                } while (newest == oldest);
            }
            // Current entry has changed, or stumbled on timestamp inversion
            if (!getRingItem (ring, current, &currTraceRecord))
            {
                goto restart;
            }
        }

        displayRingItem (ring, current, &currTraceRecord);
        shown_ts = currTraceRecord.tr_stamp;
        current = (ULONG) ((current+1) % ringSize);
    }
    csx_dbg_ext_print ("\n");
}               /* CSX_DBG_EXT_DEFINE_CMD (ktail) */

/*
 * void dbg_dprintf (char *formatA, ...)
 *
 * DESCRIPTION:  This function is used to optionally display debugging
 *   information about kdtrace.  If defaults.do_debug is false, nothing
 *   is displayed.  If it is true, displaying happens.
 *
 * PARAMETERS: variable argument list
 *
 * GLOBALS
 *   defaults.do_debug -- [R]
 *
 * CALLS
 *   csx_dbg_ext_print ()
 *   va_end()
 *   va_start()
 *   vsprintf()
 *
 * RETURNS: void
 *
 * ERRORS: none detected
 *
 * HISTORY
 *   12-May-2000 jfw -- first and operational version
 */
void
dbg_dprintf (char*formatA, ...)
{
    char buf[256];      // temp place for string
    va_list argptr;     // variable args data
  
    EmcpalZeroVariable(buf);
    if (!defaults.do_debug) // if we're not debugging, we're done
    {
        return;
    };
  
    va_start(argptr, formatA);  // start variable args processing
    vsprintf(buf, formatA, argptr); // use var args string formatted print
    va_end(argptr);     // end variable args processing
  
    csx_dbg_ext_print (buf);           // display the results
  
} /* dbg_dprintf () */

/*
 * bool displayDefaults (void)
 *
 * DESCRIPTION:
 *
 * PARAMETERS
 *
 * GLOBALS
 *
 * CALLS
 *
 * RETURNS
 *
 * ERRORS
 *
 */
bool
displayDefaults (void)
{

#define onoff(m_flag) \
   (m_flag) ? "<on>" : "<off>"

#define showif(m_flag, m_text) \
   (m_flag) ? m_text : ""

    if (!findKtrace_info ())
    {
        return (false);
    };
    
    /*if (csx_dbg_ext_read_in_len
        ((ULONG64) _Ktrace_info.info[defaults.ring].ti_name,
         buf, sizeof (buf), NULL))
    {
        csx_dbg_ext_print  ("Can't read string from 0x%x\n",
                 _Ktrace_info.info[defaults.ring].ti_name);
        return (false);
    };*/

    csx_dbg_ext_print  ("\n!kdefault [-4 %s] [-a %s] [-i %s] [-n %s] [-t %s] [-v %s]\n"
             "  [-x %s] [-D %s] [-I %s] [-P %s] [-C %s] [-T %s]\n"
             "  [-c %d] [-s %d] [-L %d] [-S %d] [-r %s]\n",
             onoff (defaults.do_4hex),
             onoff (defaults.do_all),
             onoff (defaults.do_caseless),
             onoff (defaults.do_number),
             onoff (defaults.do_tstamp),
             onoff (defaults.do_inverse),
             onoff (defaults.do_select),
             onoff (defaults.do_debug),
             onoff (!defaults.do_caseless),
             onoff (defaults.do_thread),
             onoff (defaults.do_cpu),
             onoff (defaults.do_systime),
             defaults.count,
             defaults.dreamTime,
             defaults.dwCount,
             defaults.dwPos,
             ti_name[defaults.ring]);

    csx_dbg_ext_print  ("\n!kselect %s%s%s%s\"%s\"\n",
             showif (defaults.do_caseless,  "-i "),
             showif (!defaults.do_caseless, "-I "),
             showif (defaults.do_inverse,   "-v "),
             showif (defaults.do_select,    "-x "),
             pattern);

    csx_dbg_ext_print  ("\n!ktail %s%s%s%s%s%s%s%s%s%s%s-s %d -r %s [COUNT = %d]\n",
             showif (defaults.do_4hex,      "-4 "),
             showif (defaults.do_all,       "-a "),
             showif (defaults.do_caseless,  "-i "),
             showif (!defaults.do_caseless, "-I "),
             showif (defaults.do_number,    "-n "),
             showif (defaults.do_thread,    "-P "),
             showif (defaults.do_cpu,       "-C "),
             showif (defaults.do_tstamp,    "-t "),
             showif (defaults.do_systime,   "-T "),
             showif (defaults.do_inverse,   "-v "),
             showif (defaults.do_select,    "-x "),
             defaults.dreamTime,
             ti_name[defaults.ring],
             defaults.count);

    csx_dbg_ext_print  ("!ktrace  %s%s%s%s%s%s%s%s%s%s%s-r %s [LAST = %d [SHOW = %d]]\n\n",
             showif (defaults.do_4hex,      "-4 "),
             showif (defaults.do_all,       "-a "),
             showif (defaults.do_caseless,  "-i "),
             showif (!defaults.do_caseless, "-I "),
             showif (defaults.do_number,    "-n "),
             showif (defaults.do_thread,    "-P "),
             showif (defaults.do_cpu,       "-C "),
             showif (defaults.do_tstamp,    "-t "),
             showif (defaults.do_systime,   "-T "),
             showif (defaults.do_inverse,   "-v "),
             showif (defaults.do_select,    "-x "),
             ti_name[defaults.ring],
             defaults.dwCount,
             defaults.dwPos);

    return (true);
} /* displayDefaults () */

/*
 * bool displayRingHeader (const KTRACE_ring_id_T ringA)
 *
 * DESCRIPTION:
 *
 * PARAMETERS
 *
 * GLOBALS
 *
 * CALLS
 *
 * RETURNS
 *
 * ERRORS
 *
 */
bool
displayRingHeader (const KTRACE_ring_id_T ringA)
{
    KTRC_info_T *Ktrace_info_v;     // virtual address calculations
    
    Ktrace_info_v = (KTRC_info_T *) (ULONG_PTR) TARADDR_KTRACE_info;

    csx_dbg_ext_print  ("0x%llX: flags (%04x); tflag (%04x)\n",
             (unsigned long long)TARADDR_KTRACE_info + infoOffset + ringA * infoSize,
             _Ktrace_info.info[ringA].ti_flags,
             _Ktrace_info.info[ringA].ti_tflag);
    csx_dbg_ext_print  ("rtc_freq %d.%06d MHz ",
             (ULONG)(_Ktrace_info.rtc_freq / 1000000),
             (ULONG)(_Ktrace_info.rtc_freq % 1000000));
    csx_dbg_ext_print  ("ti_slot %d; ti_size %d; ti_cirbuf 0x%llX; ti_altbuf 0x%llX\n",
             _Ktrace_info.info[ringA].ti_slot, _Ktrace_info.info[ringA].ti_size,
             (unsigned long long)ti_cirbuf[ringA], (unsigned long long)ti_altbuf[ringA]);
 
    if (do_altbuf)
    {
        csx_dbg_ext_print ("Using altbuf\n");
        ti_cirbuf[ringA] = ti_altbuf[ringA];
    }

    /*
     * Extract boot system time.
     */
    {
        emcutil_dbghelp_nt_time_fields_t SystemTime;
        LARGE_INTEGER stamp ;
        prev_wDay = 0;

        // Kernel SystemTime to user SystemTime
        emcutil_dbghelp_nt_time_to_nt_time_fields(&boot_systime, &SystemTime);

        csx_dbg_ext_print  ("Boot %4d/%02d/%02d %02d:%02d:%02d.%03d ",
                 SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay,
                 SystemTime.wHour, SystemTime.wMinute,
                 SystemTime.wSecond, SystemTime.wMilliseconds);
        stamp.QuadPart = boot_tstamp;
        csx_dbg_ext_print  ("stamp %02x%08x\n", (stamp.HighPart & 0xff), stamp.LowPart);
    }

    return (true);
}               /* displayRingHeader() */

/*
 * bool displayRingItem (const KTRACE_ring_id_T ringA)
 *
 * DESCRIPTION:
 *
 * PARAMETERS
 *
 * GLOBALS
 *
 * CALLS
 *
 * RETURNS
 *
 * ERRORS
 *
 */
bool
displayRingItem (const KTRACE_ring_id_T ringA, const unsigned int slotA, TRACE_RECORD* t)
{

    LARGE_INTEGER   stamp;  // buffer for part of timestamp
    char           *s = NULL;      // string pointer
    DWORD           j = 0;      // for index

    if (t->tr_id  == 0)     // empty? ignore it
        return (true);

    if (!prev_ts)
        prev_ts = t->tr_stamp;

    bufBegin();         // reset buffer to new item

    if (do_number)      // display slot number?
    {
        bufItem ("%05d: ", slotA);
    };

    // Show 10 hex digits of timestamp
    stamp.QuadPart = t->tr_stamp;
    usec = (ULONG) (((double) (t->tr_stamp - prev_ts) + rtc_round) / rtc_usec);

    if (do_tstamp)
        bufItem ("%02x%08x ", (stamp.HighPart & 0xff), stamp.LowPart);

    if (do_systime)
    {
        emcutil_dbghelp_nt_time_fields_t SystemTime = {0};
        emcutil_dbghelp_nt_time_t  FileTime;

        TSTAMP          elapsed;

        // Compute elapsed time in 100nanosec
        elapsed = (TSTAMP)
            (((double)(t->tr_stamp - boot_tstamp) + rtc_100nrnd) / rtc_100nsec);

        // Add elapsed time to boot_systime
        FileTime = boot_systime;
        *((TSTAMP*)&FileTime) += elapsed;

        // Kernel SystemTime to user SystemTime
        emcutil_dbghelp_nt_time_to_nt_time_fields (&FileTime, &SystemTime);

        // Check if different day
        if (prev_wDay != SystemTime.wDay)
        {
            csx_dbg_ext_print  ("DATE: %4d/%02d/%02d %02d:%02d:%02d.%03d\n",
                     SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay,
                     SystemTime.wHour, SystemTime.wMinute,
                     SystemTime.wSecond, SystemTime.wMilliseconds);
        }
        prev_wDay = SystemTime.wDay;

        bufItem ("%02d:%02d:%02d.%03d ",
                 SystemTime.wHour, SystemTime.wMinute,
                 SystemTime.wSecond, SystemTime.wMilliseconds);
    }

    // show rounded microsecs elapsed, thread-id
    if (!do_quiet)
        bufItem ("%10u ", usec);

    if (do_thread && !do_quiet)      // display thread id
        bufItem ("%16llX ", (unsigned long long)(t->tr_thread & ~KT_CPU_MASK));
    if (do_cpu && !do_quiet)         // display cpu
        bufItem ("%x ", (t->tr_thread & KT_CPU_MASK));

    if (do_4hex)
        bufItem ("%16llX, %16llX, %16llX, %16llX  %16llX:",
		 (unsigned long long)t->tr_a0, (unsigned long long)t->tr_a1,
		 (unsigned long long)t->tr_a2, (unsigned long long)t->tr_a3,
		 (unsigned long long)t->tr_id);

    if (do_all_rings || do_label)
    {
        switch ( ringA )
        {
            case TRC_K_STD:
                bufItem (" std:");
                break;
            case TRC_K_START:
                bufItem ("strt:");
                break;
            case TRC_K_IO:
                bufItem ("  io:");
                break;
            case TRC_K_USER:
                bufItem ("user:");
                break;
            case TRC_K_TRAFFIC:
                bufItem ("traf:");
                break;
            case TRC_K_CONFIG:
                bufItem ("hcfg:");
                break;
            case TRC_K_REDIR:
                bufItem ("rdir:");
                break;
            case TRC_K_EVT_LOG:
                bufItem ("elog:");
                break;
            case TRC_K_PSM:
                bufItem (" psm:");
                break;
            case TRC_K_ENV_POLL:
                bufItem ("poll:");
                break;
            case TRC_K_XOR_START:
                bufItem ("xors:");
                break;
            case TRC_K_XOR:
                bufItem (" xor:");
                break;
            case TRC_K_MLU:
                bufItem (" mlu:");
                break;
            case TRC_K_PEER:
                bufItem ("peer:");
                break;
            case TRC_K_CBFS:
                bufItem ("cbfs:");
                break;
            case TRC_K_RBAL:
                bufItem ("rbal:");
                break;
            case TRC_K_THRPRI:
                bufItem ("tpri:");
                break;
            case TRC_K_SADE:
                bufItem ("sade:");
                break;

                /* ADDING: please 4c: with leading space(s) */
            default:
                bufItem ("unkn:");
                break;
        }
    }
   
    if (t->tr_id > KT_LAST)
    {
        // constant string
        if (!csx_dbg_ext_read_in_len (t->tr_id, buf, CLEN ))
        {
            // Special care to buffer %s from debuggee
            char           *p, strbuf[ST_BUF_CNT][CLEN];
            ULONG64        *ap;
            char *newline = NULL;

            buf[CLEN - 1] = '\0';
            EmcpalZeroVariable(strbuf);
            if (s = strchr (buf, '\r'))
                *s = (*(s+1) == '\0') ? '\0' : ' ';
            if (s = strchr (buf, '\n'))
            {
                // truncate after newline if only trailing spaces
                newline = s;
                p = s+1;
                while(*p == ' ')
                {
                    p++;
                }
                if ((*p == '\0') && (p != s+1))
                {
                    *(s+1) = '\0';
                }
            }
            if ((t->tr_id >= ti_cirbuf[ringA]) &&
                (t->tr_id < (ti_cirbuf[ringA] + _Ktrace_info.info[ringA].ti_alloc_size)))
            {
                // KvTrace already formatted into associated string space;
                bufItem("%s", buf);
                if (!newline) bufItem("\n");
            }
            else if (strchr (buf, '%'))
            {
                // fdb may be run on 32-bit host, so expand %p to %llX
                // Process each format argument separately.
                char *f, fmt[32];

                // Check that %s addresses are valid
                p = buf;
                ap = (ULONG64 *) &t->tr_a0;

                for (j = 0; ; j++)
                {
                    BOOL wide = false;
                    BOOL ll = true; // Proteus
                    int l_count = 0;
                    f = fmt; // looking for new format

                retry:
                    while (*p && (*p != '%')) {
                        if (*p == '\n') newline = bufItemP;
                        if (*p != '\r') *bufItemP++ = *p;
                        p++;
                    }
                    if (*p != '%')
                        break;
                    if (*(p+1) == '%') { // escaped-%
                        *bufItemP++ = *p++;
                        *bufItemP++ = *p++;
                        goto retry;
                    }
                    if (j >= ST_BUF_CNT) {
                        // Too many arguments - truncate replacing % with #.
                        *bufItemP++ = '#';
                        break;
                    }

                    // decode % format
                    *f++ = *p++;
                    // flags [-+0 #]
                    while ((*p == '-') || (*p == '+') || (*p == '0')
                           || (*p == ' ') || (*p == '#'))
                        *f++ = *p++;

                    // width . precision
                    while (((*p) >= '0') && ((*p) <= '9')) // isdigit(*p)
                        *f++ = *p++;
                    if (*p == '.')
                    {
                        *f++ = *p++;
                        while (((*p) >= '0') && ((*p) <= '9')) // isdigit(*p)
                            *f++ = *p++;
                    }

                    // Microsoft: size qualifier [h l L I I32 ll]  // ??
                    if ((*p == 'h'))
                        *f++ = *p++;
                    while ((*p == 'w') || (*p == 'l') || (*p == 'L')) {
                        if (*p == 'l') l_count++;
                        *f++ = *p++;
                        wide = true;
                    }
                    if((*p == 'I')) {
                        if ((*(p+1) == '3') && (*(p+2) == '2')) {
                            p += 3;
                            ll  = false;
                        }
                        else if ((*(p+1) == '6') && (*(p+2) == '4')) {
                            p += 3;
                            ll = true;
                        }
                        else { // %I is pointer-sized. Proteus Array pointers are 64-bit
                            p++;
                            ll = true;
                        }
                    }
                    switch (*p)
                    {
                        case 'C':
                        case 'c':
                        case 'd':
                        case 'i':
                        case 'o':
                        case 'u':
                        case 'X': 
                        case 'x':
                            if (l_count == 2) {
                                f -= l_count;  // Assumes the 'l's are at the end.
                                ll = true;
                            }
                            if (ll) {
                                *f++ = 'I';
                                *f++ = '6';
                                *f++ = '4';
                            }
                            *f++ = *p++;
                            *f++ = '\0';
                            if (ll)
                                bufItem(fmt, (ULONG64) *ap);
                            else
                                bufItem(fmt, (ULONG32) *ap);
                            ap++;
                            break;
                        case 's':
                        case 'S':
                            *f++ = *p++;
                            *f++ = '\0';
                            if (csx_dbg_ext_read_in_len
                                ((ULONG64)*ap, strbuf[j], CLEN)) 
                            {
                                // hex08x ((ULONG64)*ap, strbuf[j]);
                                bufItem("%16llX", (unsigned long long)(*ap));
                                ap++;
                                break;
                            }
                            strbuf[j][CLEN-1] = '\0';
                            if (wide || *(p-1) == 'S') strbuf[j][CLEN-2] = '\0';
                            bufItem(fmt, strbuf[j]);
                            ap++;
                            break;

                        case 'p':
                            // %p for array is 64-bit pointer, perhaps run on 32-bit host
                            // so expand %p to %llX
                            p++; // Processed 'p'.
                            *f++ = 'I';
                            *f++ = '6';
                            *f++ = '4';
                            *f++ = 'X';
                            *f++ = '\0';
                            bufItem(fmt, (ULONG64)*ap);
                            ap++;
                            break;

                        case 'n':
                        default:
                            // unrecognized format string
                            if (*p == '\0')
                            {
                                bufItem ("[MAX %d]\n", KT_STR_LEN);
                            }
                            else
                            {
                                bufItem ("?%%%c:%16llX: %s\n",
					 *p, (unsigned long long)t->tr_id, buf);
                            }
                            goto itemSelect;
                    }   /* switch */
                }       /* for */

                if (!newline) *bufItemP++ = '\n';
                *bufItemP = '\0';

            }
            else if (newline)
            {   // Use buf as simple string, no other values
                bufItem ("%s", buf);
            }
            else if ((t->tr_a0 == 0xdeadbeef) && (t->tr_a1 == 0xdeadbeef) &&
                     (t->tr_a2 == 0xdeadbeef) && (t->tr_a3 == 0xdeadbeef))
            {   
                // No arguments. [KvTrace with truncated newline]
                bufItem ("%s\n", buf);
            }
            else
            {  
                // Use buf as simple string, 4 hex values
                bufItem ("%-22s %16llX, %16llX, %16llX, %16llX\n", buf,
			 (unsigned long long)t->tr_a0,
                         (unsigned long long)t->tr_a1,
			 (unsigned long long)t->tr_a2,
			 (unsigned long long)t->tr_a3);
            }
            goto itemSelect;
        }  // else fallthru
    }

    // decode KT_TIME stamp in localtime
    if (t->tr_id == KT_TIME)
    {
        emcutil_dbghelp_nt_time_fields_t        SystemTime = {0};
        emcutil_dbghelp_nt_time_t        FileTime;

        // Kernel SystemTime to user SystemTime
        FileTime = (EMCUTIL_SYSTEM_TIME) t->tr_a0 & 0x00000000FFFFFFFF;
        FileTime |= (EMCUTIL_SYSTEM_TIME) (t->tr_a1 & 0x00000000FFFFFFFF) << 32;
        emcutil_dbghelp_nt_time_to_nt_time_fields (&FileTime, &SystemTime);

        if (t->tr_a2 && !csx_dbg_ext_read_in_len (t->tr_a2, buf, CLEN ))
        {

            // a2:a3 is __FILE__:__LINE__
            buf[CLEN - 1] = '\0';
            bufItem ("%4d/%02d/%02d %02d:%02d:%02d.%03d  %s:%d\n",
                     SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay,
                     SystemTime.wHour, SystemTime.wMinute,
                     SystemTime.wSecond, SystemTime.wMilliseconds, buf,
                     t->tr_a3);
            goto itemSelect;
        }
        bufItem ("%4d/%02d/%02d %02d:%02d:%02d.%03d\n", SystemTime.wYear,
                 SystemTime.wMonth, SystemTime.wDay, SystemTime.wHour,
                 SystemTime.wMinute, SystemTime.wSecond,
                 SystemTime.wMilliseconds);
        goto itemSelect;
    }

    if ((t->tr_id >= KT_LAST) || (NULL == (s = KT_table_s[t->tr_id])))
    {
        // Missing Id string in table?
        bufItem ("slot=%d id=%16llx, %16llX, %16llX, %16llX, %16llX\n", 
                 slotA, t->tr_id,
                 (unsigned long long)t->tr_a0, (unsigned long long)t->tr_a1,
		 (unsigned long long)t->tr_a2, (unsigned long long)t->tr_a3);
        goto itemSelect;
    }

    // format string in table
    bufItem (s, t->tr_a0, t->tr_a1, t->tr_a2, t->tr_a3);
    bufItem ("\n");

 itemSelect:

    if (bufSelectDisplay ())
        prev_ts = t->tr_stamp;
    return (true);
}                               /* displayRingItem () */

/*
 *
 *
 * DESCRIPTION:
 *
 * PARAMETERS
 *
 * GLOBALS
 *
 * CALLS
 *
 * RETURNS
 *
 * ERRORS
 *
 * NOTE: sync changes here with initialization of defaults structure in
 *       data declaration area.
 */
void factoryDefaults (void)
{
    defaults.dwCount = 0;
    defaults.ring = TRC_K_STD;
    defaults.dwPos = 0;
    defaults.do_4hex = false;
    defaults.do_caseless = false;
    defaults.do_debug = false;
    defaults.do_inverse = false;
    defaults.do_number = false;
    defaults.do_select = false;
    defaults.do_thread = true;
    defaults.do_cpu = false;
    defaults.do_tstamp = false;
    defaults.count = DEFAULT_COUNT;
    defaults.dreamTime = DEFAULT_SLEEP;
} /* factoryDefaults () */

/*
 *
 *
 * DESCRIPTION:
 *
 * PARAMETERS
 *
 * GLOBALS
 *
 * CALLS
 *
 * RETURNS
 *
 * ERRORS
 *
 * NOTE: sync changes here with initialization of defaults structure in
 *       data declaration area.
 */
bool
findKtrace_info (void)
{
#if UMODE_ENV
    TARADDR_KTRACE_info = csx_dbg_ext_lookup_symbol_address ("ktrace", "Ktrace_info");
    if (!TARADDR_KTRACE_info)
    {
        csx_dbg_ext_print  ("No symbol USERSIM!Ktrace_info\n");
        TARADDR_KTRACE_info = csx_dbg_ext_lookup_symbol_address ("USERSIM", "KTRACE!Ktrace_info"); // try kernel version
        if (!TARADDR_KTRACE_info)
        {
            TARADDR_KTRACE_info = csx_dbg_ext_lookup_symbol_address ("USERSIM", "Ktrace_info"); // try global version
        }
    }
#else
    TARADDR_KTRACE_info = csx_dbg_ext_lookup_symbol_address ("ktrace","Ktrace_info");
#endif /* UMODE_ENV */
    if (!TARADDR_KTRACE_info)
    {
        csx_dbg_ext_print  ("Please type: !reload ktrace.sys  or check symbols path\n");
        return (false);
    }

    DBG_DPRINTF ("findKtrace_info: TARADDR_KTRACE_info=0x%llX, "
                 "&Ktrace_info='%s', addr=0x%x size=%d\n",
                 (unsigned long long)TARADDR_KTRACE_info,
                 "&KTRACE!Ktrace_info", addrsize (&_Ktrace_info));

    if(csx_dbg_ext_setup_read(TARADDR_KTRACE_info, SYMBOL_LOOKUP "KTRC_info_T") != 0)
    {
        csx_dbg_ext_print ("Error reading structure KTRC_info_T at location %llx\n", TARADDR_KTRACE_info);
        return (false);
    }

    _Ktrace_info.rtc_freq = (__int64)csx_dbg_ext_read_field("rtc_freq");
    _Ktrace_info.system_time = (__int64)csx_dbg_ext_read_field("system_time");
    _Ktrace_info.stamp = (__int64)csx_dbg_ext_read_field("stamp");
    _Ktrace_info.major = (char)csx_dbg_ext_read_field("major");
    _Ktrace_info.minor = (char)csx_dbg_ext_read_field("minor");

    // update info for each ring.
    if(!updateKtrace_info())
    {
        return (false);
    }

    /*
     * Compute frequency conversions for microsec and 100nanosec.
     */
    rtc_usec = (double)_Ktrace_info.rtc_freq / 1000000.0;
    rtc_round = (rtc_usec + 1.0) / 2.0;

    rtc_100nsec = (double) _Ktrace_info.rtc_freq / 10000000.0;
    rtc_100nrnd = (rtc_100nsec + 1.0) / 2.0;

    boot_tstamp = _Ktrace_info.stamp;
    memcpy((csx_pvoid_t)&boot_systime, (csx_pvoid_t)&_Ktrace_info.system_time, sizeof(boot_systime));;

   return (true);
} /* findKtrace_info () */


/*
 *
 *
 * DESCRIPTION:
 *
 * PARAMETERS
 *
 * GLOBALS
 *
 * CALLS
 *
 * RETURNS
 *
 * ERRORS
 *
 * NOTE: Initialize the info structure of _Ktrace_info in
 *       data declaration area.
 */

bool
updateKtrace_info
 (void)
{
    ULONG64 info_p;
    ULONG ring = 0;
    ULONG64 name;

    GETFIELDOFFSET_RT(SYMBOL_LOOKUP "KTRC_info_T", "info", &infoOffset);
    GETFIELDOFFSET_RT(SYMBOL_LOOKUP "TRACE_INFO", "ti_cirbuf", &ti_cirbuf_offset);
    GETFIELDOFFSET_RT(SYMBOL_LOOKUP "TRACE_INFO", "ti_lost",   &ti_lost_offset);
    GETFIELDOFFSET_RT(SYMBOL_LOOKUP "TRACE_INFO", "ti_flush",  &ti_flush_offset);

    infoSize = csx_dbg_ext_get_type_size(SYMBOL_LOOKUP "TRACE_INFO");
    info_p = TARADDR_KTRACE_info + infoOffset;

    for(ring = 0; ring < KTRACE_RING_CNT; ring++)
    {
        if(csx_dbg_ext_setup_read(info_p, SYMBOL_LOOKUP "TRACE_INFO")!=0)
        {
            csx_dbg_ext_print ("Reading of info structure of _KTRC_info_T at location 0x%llX failed\n",(unsigned long long)info_p);
            return (false);
        }
        ti_cirbuf[ring] = (ULONG64)csx_dbg_ext_read_field("ti_cirbuf");
        ti_altbuf[ring] = (ULONG64)csx_dbg_ext_read_field("ti_altbuf");
        _Ktrace_info.info[ring].ti_size = (ULONG) csx_dbg_ext_read_field("ti_size");
        _Ktrace_info.info[ring].ti_slot = (ULONG) csx_dbg_ext_read_field("ti_slot");
        _Ktrace_info.info[ring].ti_flags = (USHORT) csx_dbg_ext_read_field("ti_flags");
        _Ktrace_info.info[ring].ti_tflag = (ULONG) csx_dbg_ext_read_field("ti_tflag");
      
        name = csx_dbg_ext_read_field("ti_name");
        if (name != 0)
        {
            if (csx_dbg_ext_read_in_len(name, ti_name[ring], sizeof (ti_name[ring])))
            {
                csx_dbg_ext_print  ("Can't read string from 0x%llX\n",  (unsigned long long)name);
                return (false);
            }
        }

        info_p =  info_p + infoSize;
    }
    return (true);
}

/*
 *
 *
 * DESCRIPTION:
 *
 * PARAMETERS
 *
 * GLOBALS
 *
 * CALLS
 *
 * RETURNS
 *
 * ERRORS
 *
 */
//bool
//getRingItem (const KTRACE_ring_id_T ringA,
//         const int slotA,
//         TRACE_RECORD* itemPA)
//{
//    return (Read_In (_Ktrace_info.info[ringA].ti_cirbuf + slotA, "TRACE_RECORD",
//                     itemPA, sizeof (*itemPA)));
//} /* getRingItem () */
//

/*
 *
 *
 * DESCRIPTION:
 *
 * PARAMETERS
 *
 * GLOBALS
 *
 * CALLS
 *
 * RETURNS
 *
 * ERRORS
 *
 */
bool
getRingItem (const KTRACE_ring_id_T ringA,
         const int slotA,
         TRACE_RECORD* itemPA)
{
    ULONG64 ti_cirbuf1;

    // slot0 of traffic buffer, may have swapped ti_cirbuf and ti_altbuf
    if ((slotA == 0) && (ringA == TRC_K_TRAFFIC))
    {
        ULONG64 addr = TARADDR_KTRACE_info + infoOffset + (infoSize*ring) + ti_cirbuf_offset;
        if (do_all) { // oldest of cirbuf; all of altbuf; newest of cirbuf
            if (!do_altbuf) {
                addr += sizeof(ULONG64); // ti_altbuf_offset
                do_altbuf = true;
            } else {
                do_altbuf = false;
            }
        }
        if (csx_dbg_ext_read_in_len(addr, &ti_cirbuf[ringA], sizeof(ULONG64)))
        {
            csx_dbg_ext_print ("Error reading ti_cirbuf at %llx\n", addr);
            return false;
        }
    }

    ti_cirbuf1 = ti_cirbuf[ringA];
    ti_cirbuf1 = ti_cirbuf1 + (slotA * sizeof(TRACE_RECORD));

    itemPA->tr_id = 0;
    if(csx_dbg_ext_read_in_len(ti_cirbuf1,itemPA,sizeof(TRACE_RECORD)))
        return false;
    return(true);
} /* getRingItem () */


void
hex08x (ULONG64 x, char *s)
{
    static char     hex[] = "0123456789ABCDEF";
    int             i;
    // Extended to 16 characters in 64-bit case
    for (i = 0; i < 8; i++)
    {
        *s++ = hex[x >> 28];
        x <<= 4;
    }
    *s = '\0';
} /* hex08x () */

/*
 * KTRACE_ring_id_T matchRing (const char *const namePA)
 *
 * DESCRIPTION: This function searches for a name matching the argument
 *   string in the _Ktrace_info.info[n].ti_name array.  If a match is found, 
 *   the index into the array is returned, if it is not found -1 is returned.
 *
 * PARAMETERS
 *   namePA -- [R][*R] ring name to match
 *
 * GLOBALS
 *   buf    -- [*W] temporary memory buffer
 *   KTRACE_RING_CNT -- [R] number of array elements
 *
 * CALLS
 *   csx_dbg_ext_print ()
 *   csx_dbg_ext_read_in_len()
 *   strcmp()
 *
 * RETURNS
 *   array subscript of matching element
 *   -1 if no match found
 *
 * ERRORS
 *   error message on unable to read memory
 *   -1 returned on any error
 *
 * HISTORY
 *   4-May-2000 jfw -- initial version adapted from GS's version
 * 
 */
KTRACE_ring_id_T
matchRing (const char *const namePA)
{
    KTRACE_ring_id_T i;
    //int             dwBytesRead;

    if (strcmp (namePA, "all" ) == 0)
    {
        do_all_rings = TRUE;
        return TRC_K_STD;
    };

    for (i = 0; i < KTRACE_RING_CNT; i++)
    {
        if (ti_name[i] == '\0')
        {
            continue;
        }

        //if (csx_dbg_ext_read_in_len
        //    ((ULONG64) _Ktrace_info.info[i].ti_name, buf, sizeof (buf) ))
        //{
        //    csx_dbg_ext_print  ("Can't read string from 0x%llX\n", _Ktrace_info.info[i].ti_name);
        //    return ((KTRACE_ring_id_T) -1);
        //}

        //buf[sizeof (buf) - 1] = '\0';

        if (strcmp (namePA, ti_name[i]) == 0)
        {
            return (i);
        };
    }               /* for */

    csx_dbg_ext_print  ("Unknown ring buffer name\n");
    return ((KTRACE_ring_id_T) -1);
}               /* matchRing() */

void
bufBegin (void)
{
    bufItemP = bufItemBuffer;
} /* bufBegin () */

void
bufItem (char* formatPA, ...)
{
    csx_size_t added;
    va_list argptr;

    va_start (argptr, formatPA);

    added = csx_dbg_ext_vsprintf (bufItemP, formatPA, argptr);

    va_end (argptr);

    bufItemP += added;
} /* bufItem() */

bool
bufSelectDisplay (void)
{
    int length;
    int result;

    if (patternOK && do_select)
    {
        length = (int)strlen (bufItemBuffer);

        result = regexec(&patBuf,
                         bufItemBuffer,
                         0,
                         NULL,
                         0);

        if (do_inverse ^ result)
        {
            return false;
        };
    };

    csx_dbg_ext_print  ("%s", bufItemBuffer);
    return true;
} /* bufSelectDisplay() */


void
caseCheck (bool alwaysA)
{
    int status;
    int cflags = REG_EXTENDED;

    if ((caselessAtCompile == do_caseless) && !alwaysA)
    {
        return;
    };
        
    if(do_caseless)
    {
        cflags |= REG_ICASE;
    }

    status = regcomp(&patBuf, pattern, cflags);
    
    caselessAtCompile = do_caseless;

    if (!status)
    {
        patternOK = TRUE;
        fprintf (stderr, "Success for pattern  \"%s\"\n", pattern);
    }
    else
    {
        char errbuf[40];
        patternOK = FALSE;
        regerror(status, &patBuf, errbuf, sizeof(errbuf));
        fprintf (stderr, "Error: %s for pattern \"%s\"\n", errbuf, pattern);
    };
} /* caseCheck() */

void help_kdefault(void)
{
    csx_dbg_ext_print  ("%s\n", usageMsg_kdefault);
} /* help_kdefault() */

void help_kselect(void)
{
    csx_dbg_ext_print  ("%s\n", usageMsg_kselect);
} /* help_kselect() */

void help_ktail(void)
{
    csx_dbg_ext_print  ("%s\n", usageMsg_ktail);
} /* help_tail() */

void help_ktrace(void)
{
    csx_dbg_ext_print  ("%s\n", usageMsg_ktrace);
} /* help_kttrace() */

bool
commonStrOpt (char* argPA,
          const char *const optstrPA,
          const char *const helpTextPA)
{
    int optchar;

    // set defaults

    DBG_DPRINTF ("optstrPA= '%s'\n", optstrPA);

    _kt_rings  = 0;
    for(ring = 0; ring < KTRACE_RING_CNT; ring++)
    {
        _kt[ring].slot = _kt[ring].valid = -1;
    }
    do_4hex         = defaults.do_4hex;
    do_all          = defaults.do_all;
    do_number       = defaults.do_number;
    do_systime      = defaults.do_systime;
    do_thread       = defaults.do_thread;
    do_cpu          = defaults.do_cpu;
    do_tstamp       = defaults.do_tstamp;
    do_inverse      = defaults.do_inverse;
    do_select       = defaults.do_select;
    dwPos           = defaults.dwPos;
    dwCount         = defaults.dwCount;
    ring            = defaults.ring;
    dreamTime       = defaults.dreamTime;
    do_all_rings    = defaults.do_all_rings;
    do_label        = defaults.do_label;
    do_quiet        = defaults.do_quiet;
    do_altbuf       = false;
    do_slot0        = false;
    

    (void) strOpt("", ""); // clear up stale state
    while ((optchar = strOpt (argPA, optstrPA)) != strOpt_EOL)
    {
        DBG_DPRINTF ("optchar='%c' (%03o)\n", optchar, optchar);

        switch (optchar)
        {
            case '0':
                do_slot0 = true;
                break;

            case '4':       // -4 Abhaya's display format.
                do_4hex = true;
                break;

            case 'a':       // -a (look at all entries)
                do_all = true;
                break;

            case ('h'):
                csx_dbg_ext_print  ("%s\n", helpTextPA);
                return (false);

            case ('i'):     // -i (ignore case)
                do_caseless = true;
                caseCheck(false);
                break;

            case ('n'):     // -n (display entry numbers)
                do_number = true;
                break;

            case ('r'):     // -r ringname
                if ((ring = matchRing (stroptarg)) == -1)
                {
                    return (false); // name not found, quit
                };
                _kt[ring].slot = 0;
                _kt_rings++;
                break;

            case ('s'):     // -s num -- uSecs to wait for new data
                dreamTime = (int)GetArgument (stroptarg, 1);
                break;

            case ('t'):     // -t (display timestamps)
                do_tstamp = true;
                break;

            case ('v'):     // -v (invert selection process)
                do_inverse = true;
                do_select = true;
                break;

            case ('x'):     // -x (use selection)
                do_inverse = false;
                do_select = true;
                break;

            case ('I'):     // -I (match case)
                do_caseless = false;
                caseCheck(false);
                break;

            case ('A'):
                do_altbuf = true;
                break;

            case ('C'):
                do_cpu = true;
                break;

            case ('P'):     // -P (DO NOT display process thread)
                do_thread = false;
                break;

            case ('T'):     // -T (display clock time)
                do_systime = true;
                break;

            case ('l'):     // -l (display buffer label)
                do_label = true;
                break;

            case ('q'):     // -q (quiet mode)
                do_quiet = true;
                break;

            case (':'):
                csx_dbg_ext_print  ("Missing value for switch -%c:\n", stroptopt);
                csx_dbg_ext_print  ("%s\n", helpTextPA);
                return (false);

            case ('?'):
                csx_dbg_ext_print  ("Unknown switch: '-%c' (0%02o)\n", stroptopt, stroptopt);
                csx_dbg_ext_print  ("%s\n", helpTextPA);
                return (false);

            default:
                csx_dbg_ext_print  ("Unexpected return value from strOpt() -- 0x%08X\n",
                         optchar);
                return (false);
        } /* switch */
    } /* while */
    return (true);
} /* commonStrOpt() */

VOID
displayTimestamp(
    LARGE_INTEGER   Timestamp
    )
{
    emcutil_dbghelp_nt_time_fields_t SystemTime = {0};
    emcutil_dbghelp_nt_time_t startFileTime;

    EmcpalZeroVariable(SystemTime);
    EmcpalZeroVariable(startFileTime);
    startFileTime  = Timestamp.QuadPart;
    
    emcutil_dbghelp_nt_time_to_nt_time_fields( &startFileTime, &SystemTime );
    
        csx_dbg_ext_print  (" (%02d:%02d:%02d.%03d %02d/%02d/%4d)\n",
          SystemTime.wHour, SystemTime.wMinute,
          SystemTime.wSecond, SystemTime.wMilliseconds,
          SystemTime.wMonth, SystemTime.wDay, SystemTime.wYear );
}


CSX_DBG_EXT_DEFINE_CMD( timestamp, "!timestamp" )
{
#ifndef UMODE_ENV
    ULONG64                     addr                = csx_dbg_ext_lookup_symbol_address("ktrace", args );
    LARGE_INTEGER               Timestamp;

    EmcpalZeroVariable(Timestamp);
    if (!addr)
    {
        retrieve_global_system_time(&Timestamp);
    }
    else
    {
    if( csx_dbg_ext_read_in_len( (ULONG64) addr,
                     &Timestamp,
                     sizeof(LARGE_INTEGER) ) )
    {    


    csx_dbg_ext_print ( "Could not read the timestamp (0x%llX)! \n",
                 (unsigned long long)addr );
        return;
    }
    }

    csx_dbg_ext_print ("Timestamp 0x%llx value 0x%llx", (unsigned long long)addr, (unsigned long long)Timestamp.QuadPart);

    displayTimestamp( Timestamp );

    retrieve_global_tick_count(&Timestamp); 

    csx_dbg_ext_print ("TickCount 0x%llx value 0x%llx\n", (unsigned long long)addr, (unsigned long long)Timestamp.QuadPart);
#endif
    return;
}

PCHAR
ringToString(
    KTRACE_ring_id_T ringToConvert
    )
{
    switch ( ringToConvert )
    {
        case TRC_K_STD:
            return "std";
            break;
        case TRC_K_START:
            return "start";
            break;
        case TRC_K_IO:
            return "io";
            break;
        case TRC_K_USER:
            return "user";
            break;
        case TRC_K_TRAFFIC:
            return "traffic";
            break;
        case TRC_K_CONFIG:
            return "hcfg";
            break;
        case TRC_K_REDIR:
            return "redir";
            break;
        case TRC_K_EVT_LOG:
            return "eventlog";
            break;
        case TRC_K_PSM:
            return "psm";
            break;
        case TRC_K_ENV_POLL:
            return "envpoll";
            break;
        case TRC_K_XOR_START:
            return "xors";
            break;
        case TRC_K_XOR:
            return "xor";
            break;
        case TRC_K_MLU:
            return "mlu";
            break;
        case TRC_K_PEER:
            return "peer";
            break;
        case TRC_K_CBFS:
            return "cbfs";
            break;
        case TRC_K_RBAL:
            return "rbal";
            break;
        case TRC_K_THRPRI:
            return "thrpri";
            break;
        case TRC_K_SADE:
            return "sade";
        default:
            return "unknown";
            break;
    }
}

VOID
displayRecordTimestamp(
    TRACE_RECORD    *pRecord
    )
{
    emcutil_dbghelp_nt_time_fields_t SystemTime = {0};
    emcutil_dbghelp_nt_time_t        FileTime;
    TSTAMP          elapsed;

    // Compute elapsed time in 100nanosec
    elapsed = (TSTAMP)
        (((double)(pRecord->tr_stamp - boot_tstamp) + rtc_100nrnd) / rtc_100nsec);

    // Add elapsed time to boot_systime
    FileTime = boot_systime;
    *((TSTAMP*)&FileTime) += elapsed;

    // Kernel SystemTime to user SystemTime
    emcutil_dbghelp_nt_time_to_nt_time_fields (&FileTime, &SystemTime);

    csx_dbg_ext_print  ("%02d/%02d %02d:%02d:%02d",
             SystemTime.wMonth, SystemTime.wDay,
             SystemTime.wHour, SystemTime.wMinute,
             SystemTime.wSecond );
}

CSX_DBG_EXT_DEFINE_CMD( ktstats, "!ktstats" )
{
    TRACE_RECORD    newestTraceRecord;
    TRACE_RECORD    oldestTraceRecord;
    TSTAMP          elapsed;
    DWORD           hours, minutes, seconds;
    
    if ( !findKtrace_info() )
    {
        return;
    }
    // 80char-------------------------------------------------------------------------------
    //       xxxxxxxx xxxxx xxxxx xx/xx xx:xx:xx xx/xx xx:xx:xx xxx:xx:xx
    csx_dbg_ext_print ("Buffer   Slot  Size  Oldest         Newest         Elapsed\n" );
    csx_dbg_ext_print ("------------------------------------------------------------\n" );

    for( ring = 0; ring < TRC_K_MAX; ring++ )
    {
        if ( CHECK_CONTROL_C () ) return;

        if ( _Ktrace_info.info[ring].ti_slot == -1 )
        {
            csx_dbg_ext_print ("%8s no contents\n", ringToString( ring ) );
            continue;
        }

        csx_dbg_ext_print ("%8s %5d %5d ", ringToString( ring ),
                _Ktrace_info.info[ring].ti_slot,
                _Ktrace_info.info[ring].ti_size );

        do
        {
            if ( CHECK_CONTROL_C () ) return;

            if (!getRingItem( ring, _Ktrace_info.info[ring].ti_slot, &newestTraceRecord ))
            {
                return;
            }
            if (!getRingItem( ring, _Ktrace_info.info[ring].ti_slot + 1, &oldestTraceRecord ))
            {
                return;
            }

            // If we didn't really get the oldest and the newest, snooze
            // and try again later.

            if ( newestTraceRecord.tr_stamp <= oldestTraceRecord.tr_stamp )
            {
                csx_dbg_ext_rt_sleep_msecs (dreamTime);
                if ( !findKtrace_info() )
                {
                    return;
                }
            }
        } while ( newestTraceRecord.tr_stamp <= oldestTraceRecord.tr_stamp );

        if ( oldestTraceRecord.tr_stamp == 0 )
        {
            if (!getRingItem( ring, 0, &oldestTraceRecord ))
            {
                return;
            }
        }

        displayRecordTimestamp( &oldestTraceRecord );

        csx_dbg_ext_print (" ");

        displayRecordTimestamp( &newestTraceRecord );

        if ( oldestTraceRecord.tr_stamp != 0 )
        {
            csx_dbg_ext_print (" ");

            elapsed = (TSTAMP)
            (((double)( newestTraceRecord.tr_stamp - oldestTraceRecord.tr_stamp  ) +
                rtc_100nrnd) / rtc_100nsec);

            elapsed /= 10000000;

            hours = (DWORD)elapsed/3600;
            minutes = (DWORD)elapsed/60 - hours*60;
            seconds = (DWORD)elapsed - hours*3600 - minutes*60;

            csx_dbg_ext_print ("%3d:%02d:%02d", hours, minutes, seconds );
        }

        csx_dbg_ext_print ("\n");
    }
}

/* end of file kdtrace.c */
