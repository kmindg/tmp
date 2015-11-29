/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_log.cpp
 ***************************************************************************
 *
 * DESCRIPTION: MUT Framework logging 
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    09/18/2007   Buckley, Martin   initial version of logging interface implementation
 *    11/02/2007   Alexander Gorlov  create separate module for logging
 *    02/29/2008   Igor Bobrovnikov  new log format
 *    09/07/2010  Updated for C++ impl.
 *
 **************************************************************************/

/************************
 * INCLUDE FILES
 ************************/
#include "mut_log.h"
#include "mut_private.h"
#include "mut_cpp.h"
#include "mut_testsuite.h"
#include "mut_test_entry.h"
#include "mut_sdk.h"
#include "mut_test_control.h"
#include "mut_config_manager.h"
#include <vector>
#include "UserDefs.h"
using namespace std;

//#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
//#include <conio.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
//#include <limits.h>
#include <windows.h>

void mut_strip_terminators(char* buf);

/***************************************************************************
 * LOG TAGS DEFINITIONS
 **************************************************************************/

extern Mut_test_control *mut_control;

Mut_log_control::Mut_log_control() : section(NULL), text_file(NULL),xml_file(NULL) {
    text_log_file[0] = '\0';
    xml_log_file[0] = '\0';
    dump_file_name[0] = '\0';
#ifndef CSX_BV_ENVIRONMENT_WINDOWS
    setvbuf(stdout, NULL, _IOLBF, (size_t) 0);
    setvbuf(stderr, NULL, _IOLBF, (size_t) 0);
#endif                                     /* CSX_BV_ENVIRONMENT_WINDOWS */
}


Mut_log::Mut_log(): mListener(NULL){ }
Mut_log_statistics::Mut_log_statistics(): tests_passed(0), tests_failed(0), tests_notexecuted(0), 
    asserts_failed(0), suitestarted(0){};

void Mut_log::registerListener(Mut_abstract_log_listener *listener) {
    mListener = listener;
}

Mut_log_format_spec mut_log_format_entry[mut_formatitem_Size];

/***************************************************************************
 * LOW LEVEL OUTPUT ROUTINES BEGIN
 **************************************************************************/

void Mut_log::charcat(char *s, char c)
{
    size_t l = strlen(s);
    s[l] = c;
    s[l + 1] = 0;
}

unsigned Mut_log::mut_is_digit(char c)
{
    return c >= '0' && c <= '9';
}

void Mut_log::mut_log_format_prepare_prim(Mut_log_entry_coll *coll, const char *const fieldspec[], unsigned filedspec_size, const char *formatspec)
{
    unsigned mode = MUT_FALSE;
    char fieldname[MUT_STRING_LEN];
    size_t fieldwidth;
    mut_format_alignment_t alignment;
    mut_logging_t read_level = MUT_LOG_SUITE_STATUS;

    while (*formatspec)
    {
        Mut_log_format_entry *entry = coll->entry_coll + coll->coll_size;
        coll->coll_size++;
        entry->size = 0;

        if (*formatspec == mut_log_format_spec_char)
        {
            read_level = MUT_LOG_SUITE_STATUS;
            formatspec++;
            
            while (mut_is_digit(*formatspec))
            {
                read_level = (mut_logging_t) ((int)read_level * 10 + *formatspec - '0');
                formatspec++;
            }

            if (*formatspec == ':')
            {
                formatspec++;        
            }
            else
            {
                MUT_INTERNAL_ERROR(("Expected ':' after verbosity value in %s", formatspec));
            }
        }

        entry->level = read_level;

        while (*formatspec && *formatspec != mut_log_format_spec_char/*was crlf*/)
        {
            if (*formatspec == '%') 
            {   
                if (mode)
                {
                    char list_of_vars[MUT_STRING_LEN];
                    unsigned i;
                    
                    sprintf(list_of_vars, "%d:", filedspec_size);
                    
                    for (i = 0; i < filedspec_size; i++)
                    {
                        strcat(list_of_vars, " ");
                        if (strlen(list_of_vars) + strlen(fieldspec[i]) < sizeof(list_of_vars))
                        {
                            strcat(list_of_vars, fieldspec[i]);
                        }
                        else
                        {
                            MUT_INTERNAL_ERROR(("list of vars too long"));
                        }
                        if (!strcmp(fieldspec[i], fieldname))
                        {
                            break;
                        }
                    }

                    if (i < filedspec_size)
                    {
                        entry->fieldindex[entry->size] = i;
                        entry->fieldwidth[entry->size] = fieldwidth;
                        entry->alignment[entry->size] = alignment;
                        entry->size++;
                    }
                    else
                    {
                        MUT_INTERNAL_ERROR(("No such variable %s in format allowed (%s)", fieldname, list_of_vars));
                    }
                }
                else
                {
                    *fieldname = 0;
                    fieldwidth = 0;
                    alignment = mut_format_alignment_left;
                }

                mode = !mode;
            }
            else
            {
                if (mode)
                {
                    if (*formatspec == ':')
                    {
                        formatspec++;

                        switch (*formatspec)
                        {
                        case 'l':
                            formatspec++;
                            alignment = mut_format_alignment_left;
                            break;
                        case 'r':
                            formatspec++;
                            alignment = mut_format_alignment_right;
                            break;
                        default:;
                        }

                        while (*(formatspec) != '%')
                        {
                            fieldwidth = fieldwidth * 10 + *(formatspec++) - '0';
                        }
                        formatspec--;
                    }
                    else
                    {
                        charcat(fieldname, *formatspec);
                    }
                }
                else
                {
                    charcat(entry->innertext[entry->size], *formatspec);
                }
            }

            formatspec++;
        }
    }

    if (mode)
    {
         MUT_INTERNAL_ERROR(("No closing %% character in format specification"));
    }
}

void Mut_log::mut_log_format_prepare(mut_log_format_item_t format_ndx, const char *kind, const char *const fieldspec[], const char *const formatspec_txt, const char *const formatspec_xml)
{
    unsigned fieldspec_size = 0;
    Mut_log_format_spec *format = mut_log_format_entry + format_ndx;
    
    /* Calculate the number of fileds for the specific format */
    while (fieldspec[fieldspec_size++]) {};
    fieldspec_size--;

    format->arguments_size = fieldspec_size;
    format->kind = kind;

    mut_log_format_prepare_prim(&format->format[MUT_LOG_TEXT], fieldspec, fieldspec_size, formatspec_txt);
    mut_log_format_prepare_prim(&format->format[MUT_LOG_XML], fieldspec, fieldspec_size, formatspec_xml);    
}

/***************************************************************************
 * LOW LEVEL OUTPUT ROUTINES END
 **************************************************************************/

/***************************************************************************
 * FORMAT DEFINITION
 **************************************************************************/



const char* mut_format_class[mut_formatitem_Size] = {
    "Header",
    "TestStarted",
    "AssertFailed",
    "MutPrintf",
    "KTrace",
    "TestFinished",
    "Result",
    "TestTableFailed",
    "TestTableNotExecuted",
    "TestItemFailed",
    "TestItemNotExecuted",
    "Summary"
};

/* Part of the log identification */
static const char mut_xml_tag_mutlog[] =      "MUTLog";
static const char mut_xml_tag_mutheader[] =   "Header";
static const char mut_xml_tag_mutbody[] =     "Body";
static const char mut_xml_tag_result[] =      "Result";
static const char mut_xml_tag_summary[] =     "SuiteSummary";
static const char mut_xml_tag_version[] =     "Version";
static const char mut_xml_tag_logfile[] =     "LogFile";
static const char mut_xml_tag_testsuite[] =   "TestSuite";
static const char mut_xml_tag_fields[] =      "Fields";
static const char mut_xml_tag_field[] =       "Field";
static const char mut_xml_tag_class[] =       "Class";
static const char mut_xml_tag_duration[] =    "Duration";
static const char mut_xml_tag_status[] =      "Status";
static const char mut_xml_tag_failed[] =      "Failed";
static const char mut_xml_tag_passed[] =      "Passed";
static const char mut_xml_tag_notexecuted[] = "NotExecuted";
static const char mut_xml_tag_username[] =    "UserName";
static const char mut_xml_tag_compname[] =    "CompName";
static const char mut_xml_tag_viewpath[] =    "ViewPath";
static const char mut_xml_tag_configspec[] =  "ConfigSpec";
static const char mut_xml_tag_checkouts[] =   "Checkouts";

static const char mut_xml_tag_testsuitestarted[] =  "TestSuiteStarted";
static const char mut_xml_tag_testsuitefinished[] = "TestSuiteFinished";

static const char mut_xml_tag_time[] = "Time";
static const char mut_xml_tag_date[] = "Date";

static const char mut_log_version[] =   "1.0";

char console_suite_summary[MSG_BUFFER_SIZEOF*4];



static const char* const mut_format_fields[mut_formatitem_Size][mut_format_fields_max] = {
    {MUT_STD_FIELDS, "FormatName", "Version", "MutVersion", "SuiteName", "LogFile", "UserName", "ComputerName", "Iteration", "CmdLine", 0},
    {MUT_STD_FIELDS, "TestIndex", "TestName", "TestDescription", 0},
    {MUT_STD_FIELDS, "File", "Line", "Func", "Thread", "Message", 0},
    {MUT_STD_FIELDS, "Message", 0},
    {MUT_STD_FIELDS, "Message", 0},
    {MUT_STD_FIELDS, "TestIndex", "TestName",  "TestStatus", 0},
    {MUT_STD_FIELDS, "SuiteName", "Duration", "TestTableFailed", "TestTableNotExecuted", "SuiteStatus", "TestsFailedCount", "TestsPassedCount", "TestsNotExecutedCount", 0},
    {MUT_STD_FIELDS, "TestListFailed", 0},
    {MUT_STD_FIELDS, "TestListNotExecuted", 0},
    {MUT_STD_FIELDS, "TestName", "TestIndex", 0},
    {MUT_STD_FIELDS, "TestName", "TestIndex", 0},
    {MUT_STD_FIELDS, "SuiteName", "SuiteName", "TestTableFailed", "TestTableNotExecuted", "SuiteStatus", "TestsFailedCount", "TestsPassedCount", "TestsNotExecutedCount", 0}
};

char mut_format_spec_txt[mut_formatitem_Size][MUT_STRING_LEN] = {
    /* Header */
    MUT_FRM_VERB(1)MUT_TXT_STD_FILEDS" Iteration      %Iteration%\n"
    MUT_FRM_VERB(1)MUT_TXT_STD_FILEDS" %FormatName:14% Version %Version%\n"
    MUT_FRM_VERB(0)MUT_TXT_STD_FILEDS" TestSuite      %SuiteName%\n"
    MUT_FRM_VERB(0)MUT_TXT_STD_FILEDS" LogFile        %LogFile%\n"
    MUT_FRM_VERB(1)MUT_TXT_STD_FILEDS" ComputerName   %ComputerName%\n"
    MUT_FRM_VERB(1)MUT_TXT_STD_FILEDS" UserName       %UserName%\n"
    MUT_FRM_VERB(1)MUT_TXT_STD_FILEDS" MutVersion     %MutVersion%\n"
    MUT_FRM_VERB(1)MUT_TXT_STD_FILEDS" CmdLine        %CmdLine%\n"
    MUT_FRM_VERB(0)MUT_TXT_STD_FILEDS" SuiteStarted   %SuiteName%\n",

    /* TestStarted */
    MUT_FRM_VERB(1)MUT_TXT_STD_FILEDS" %Class:14% (%TestIndex:r2%) %TestName%: %TestDescription%\n",

    /* AssertFailed */
    MUT_FRM_VERB(0)MUT_TXT_STD_FILEDS" %Class:14% %Message%\n"
    MUT_FRM_VERB(1)MUT_TXT_STD_FILEDS" %Class:14% Function %Func%\n"
    MUT_FRM_VERB(1)MUT_TXT_STD_FILEDS" %Class:14% Thread %Thread%\n"
    MUT_FRM_VERB(1)MUT_TXT_STD_FILEDS" %Class:14% File %File%\n"
    MUT_FRM_VERB(1)MUT_TXT_STD_FILEDS" %Class:14% Line %Line%\n",
    
    /* MutPrintf */
    MUT_FRM_VERB(0)MUT_TXT_STD_FILEDS" %Class:14% %Message%\n",

    /* KTrace */
    MUT_FRM_VERB(16)MUT_TXT_STD_FILEDS" %Class:14% %Message%\n",

    /* TestFinished  */
    MUT_FRM_VERB(1)MUT_TXT_STD_FILEDS" %Class:14% (%TestIndex:r2%) %TestName% %TestStatus%\n",

    /* Result */
    MUT_FRM_VERB(1)MUT_TXT_STD_FILEDS" SuiteFinished  Duration: %Duration%s\n"
    MUT_FRM_VERB(1)"%TestTableFailed%"
    MUT_FRM_VERB(1)"%TestTableNotExecuted%"
    MUT_FRM_VERB(0)MUT_TXT_STD_FILEDS" SuiteSummary   Status: %SuiteStatus% Failed: %TestsFailedCount% Passed: %TestsPassedCount% NotExecuted: %TestsNotExecutedCount%\n",

    /* Result list of failed tests */
    "%TestListFailed%",

    /* Result list of not executed tests */
    "%TestListNotExecuted%",

    /* Result table of failed tests */
    MUT_TXT_STD_FILEDS" TestFailed     (%TestIndex:r2%) %TestName%\n",

    /* Result table of not executed tests */
    MUT_TXT_STD_FILEDS" TestNotRunned  (%TestIndex:r2%) %TestName%\n",
     /* Summary */
    MUT_FRM_VERB(1)MUT_TXT_STD_FILEDS" SuiteSummary   Suite: %SuiteName%\n"
    MUT_FRM_VERB(1)"%TestTableFailed%"
    MUT_FRM_VERB(1)"%TestTableNotExecuted%"
    MUT_FRM_VERB(0)MUT_TXT_STD_FILEDS" SuiteSummary   Status: %SuiteStatus% Failed: %TestsFailedCount% Passed: %TestsPassedCount% NotExecuted: %TestsNotExecutedCount%\n",
};



static char mut_format_spec_xml[mut_formatitem_Size][MUT_STRING_LEN] = {
    // Header 
    "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n"
    MUT_XML_TAG_OPEN(MUT_XML_TAB0, MUTLog)
    MUT_XML_TAG_OPEN(MUT_XML_TAB1, Header)
            MUT_XML_STD_FIELDS(MUT_XML_TAB2, 0)
            MUT_XML_FIELD(MUT_XML_TAB2, Version)
            MUT_XML_FIELD(MUT_XML_TAB2, SuiteName)
            MUT_XML_FIELD(MUT_XML_TAB2, LogFile)
            MUT_XML_FIELD(MUT_XML_TAB2, UserName)
            MUT_XML_FIELD(MUT_XML_TAB2, ComputerName)
    MUT_XML_TAG_CLOSE(MUT_XML_TAB1, Header)
    MUT_XML_TAG_OPEN(MUT_XML_TAB1, Body),

    // TestStarted 
    MUT_XML_TAG_OPEN(MUT_XML_TAB2, LogEntry)
        MUT_XML_STD_FIELDS(MUT_XML_TAB3, 1)
        MUT_XML_FIELD(MUT_XML_TAB3, TestIndex)
        MUT_XML_FIELD(MUT_XML_TAB3, TestName)
        MUT_XML_FIELD(MUT_XML_TAB3, TestDescription)
    MUT_XML_TAG_CLOSE(MUT_XML_TAB2, LogEntry),

    // AssertFailed 
    MUT_XML_TAG_OPEN(MUT_XML_TAB2, LogEntry)
        MUT_XML_STD_FIELDS(MUT_XML_TAB3, 0)
        MUT_XML_FIELD(MUT_XML_TAB3, File)
        MUT_XML_FIELD(MUT_XML_TAB3, Line)
        MUT_XML_FIELD(MUT_XML_TAB3, Func)
        MUT_XML_FIELD(MUT_XML_TAB3, Thread)
        MUT_XML_FIELD(MUT_XML_TAB3, Message)
    MUT_XML_TAG_CLOSE(MUT_XML_TAB2, LogEntry),

    // MutPrintf
    MUT_XML_TAG_OPEN(MUT_XML_TAB2, LogEntry)
        MUT_XML_STD_FIELDS(MUT_XML_TAB3, 0)
        MUT_XML_FIELD(MUT_XML_TAB3, Message)
    MUT_XML_TAG_CLOSE(MUT_XML_TAB2, LogEntry),

    // KTrace
    MUT_XML_TAG_OPEN(MUT_XML_TAB2, LogEntry)
        MUT_XML_STD_FIELDS(MUT_XML_TAB3, 16)
        MUT_XML_FIELD(MUT_XML_TAB3, Message)
    MUT_XML_TAG_CLOSE(MUT_XML_TAB2, LogEntry),

    // TestFinished
    MUT_XML_TAG_OPEN(MUT_XML_TAB2, LogEntry)
        MUT_XML_STD_FIELDS(MUT_XML_TAB3, 1)
        MUT_XML_FIELD(MUT_XML_TAB3, TestIndex)
        MUT_XML_FIELD(MUT_XML_TAB3, TestName)
        MUT_XML_FIELD(MUT_XML_TAB3, TestStatus)
    MUT_XML_TAG_CLOSE(MUT_XML_TAB2, LogEntry),

    // Result 
    MUT_XML_TAG_CLOSE(MUT_XML_TAB1, Body)
    MUT_XML_TAG_OPEN(MUT_XML_TAB1, Result)
            MUT_XML_STD_FIELDS(MUT_XML_TAB2, 0)
            MUT_XML_FIELD(MUT_XML_TAB2, SuiteName)
            MUT_XML_FIELD(MUT_XML_TAB2, Duration)
            "%TestTableFailed%"
            "%TestTableNotExecuted%"
            MUT_XML_FIELD(MUT_XML_TAB2, SuiteStatus)
            MUT_XML_FIELD(MUT_XML_TAB2, TestsFailedCount)
            MUT_XML_FIELD(MUT_XML_TAB2, TestsPassedCount)
            MUT_XML_FIELD(MUT_XML_TAB2, TestsNotExecutedCount)
    MUT_XML_TAG_CLOSE(MUT_XML_TAB1, Result)
    MUT_XML_TAG_CLOSE(MUT_XML_TAB0, MUTLog),

    // Result list of failed tests 
    MUT_XML_TAB2"<TestListFailed>\n"
    "%TestListFailed%"
    MUT_XML_TAB2"</TestListFailed>\n",

    // Result list of not executed tests 
    MUT_XML_TAB2"<TestListNotExecuted>\n"
    "%TestListNotExecuted%"
    MUT_XML_TAB2"</TestListNotExecuted>\n",

    // Result table of failed tests 
    MUT_XML_TAB3"<TestFailed>\n"
        MUT_XML_FIELD(MUT_XML_TAB4, TestIndex)
        MUT_XML_FIELD(MUT_XML_TAB4, TestName)
    MUT_XML_TAB3"</TestFailed>\n",

    // Result table of not executed tests 
    MUT_XML_TAB3"<TestNotExecuted>\n"
        MUT_XML_FIELD(MUT_XML_TAB4, TestIndex)
        MUT_XML_FIELD(MUT_XML_TAB4, TestName)
    MUT_XML_TAB3"</TestNotExecuted>\n",
    // TestSummary 
    MUT_XML_TAG_CLOSE(MUT_XML_TAB1, Body)
    MUT_XML_TAG_OPEN(MUT_XML_TAB1, Result)
            MUT_XML_STD_FIELDS(MUT_XML_TAB2, 0)
            MUT_XML_FIELD(MUT_XML_TAB2, SuiteName)
            MUT_XML_FIELD(MUT_XML_TAB2, SuiteName)
            "%TestTableFailed%"
            "%TestTableNotExecuted%"
            MUT_XML_FIELD(MUT_XML_TAB2, SuiteStatus)
            MUT_XML_FIELD(MUT_XML_TAB2, TestsFailedCount)
            MUT_XML_FIELD(MUT_XML_TAB2, TestsPassedCount)
            MUT_XML_FIELD(MUT_XML_TAB2, TestsNotExecutedCount)
    MUT_XML_TAG_CLOSE(MUT_XML_TAB1, Result)
    MUT_XML_TAG_CLOSE(MUT_XML_TAB0, MUTLog),
};







void Mut_log::mut_format_initialization(void)
{
   // mut_log_format_item_t f;
    int f;
    for (f = 0; f < mut_formatitem_Size; f++)
    {
        mut_log_format_prepare((mut_log_format_item_t)f, mut_format_class[f], mut_format_fields[f], mut_format_spec_txt[f], mut_format_spec_xml[f]);
    }
}

/***************************************************************************
 * FORMAT DEFINITION
 **************************************************************************/

/***************************************************************************
 * DEFAULT VALUES
 **************************************************************************/

/* Default format for creation of filename */
char mut_filename_format_default[] = "%testsuite%_%YYYYMMDD%_%hhmmss%.%ext%";
const char mut_textformat_filename_default[] = "";

enum {
    mut_class_id_width = 16,
};

/***************************************************************************
 * LOW LEVEL OUTPUT FUNCTIONS BEGIN
 **************************************************************************/

void Mut_log::time2str(char *buf)
{
    EMCUTIL_TIME_FIELDS  st;

    EmcutilGetSystemTimeFields(&st);
    EmcutilConvertTimeFieldsToLocalTime(&st);
    sprintf(buf, "%02u:%02u:%02u.%03u", st.Hour, st.Minute, st.Second, st.Milliseconds);
}

/** \fn static int mut_log_console(mut_logging_t level, const char* txt, const char* xml)
 *  \param txt - text representation of log entry.
 *  \param xml - XML representation of log entry.
 *  \details
 *  Provides output text representation to console (XML output TODO)
 */
void Mut_log::mut_log_console(const char* txt)
{
    puts(txt);
    return;
}

/** \fn static int mut_log_file(mut_logging_t level, const char* txt, const char* xml)
 *  \param txt - text representation of log entry.
 *  \param xml - XML representation of log entry.
 *  \details
 *  Provides output text or XML to a fileconsole according with loggong type
 */
void Mut_log::mut_log_file(const char* txt, const char* xml)
{
    /*
     * Some tests perform initialization after mut_init but before mut_run_testsuite
     * As such, the log file has not been initialized.  
     * Discard any log messages created, before mut_run_test has been executed
     */
    mut_log_text_file(txt);
    mut_log_xml_file(xml);

    return;
}

void Mut_log::mut_log_text_file(const char *buffer) {
    if(mListener != NULL) {
        mListener->send_text_msg(buffer);
    }
    if(log_control.text_file != NULL) {
        fputs(buffer, log_control.text_file);
        fflush(log_control.text_file);
    }
}

void Mut_log::mut_log_xml_file(const char *buffer) {
    if(mListener != NULL) {
        mListener->send_xml_msg(buffer);
    }
    if(log_control.xml_file != NULL) {
        fputs(buffer, log_control.xml_file);
        fflush(log_control.xml_file);
    }
}

/***************************************************************************
 * LOW LEVEL OUTPUT FUNCTIONS END
 **************************************************************************/

/***************************************************************************
 * XML SPECIFIC SUPPORT FUNCTIONS BEGIN
 **************************************************************************/

/** \fn inline inline const char * escape_char(char c)
 *  \param c - character to check
 *  \return null-terminated string representing special XML character c, or NULL
 *          if c is not special character in XML    
*/
const char * Mut_log::escape_char(char c)
{
    switch(c)
    {
    case '>':
        return "&gt;";
    case '<':
        return "&lt;";
    case '\"':
        return "&quot;";
    case '\'':
        return "&apos;";
    case '&':
        return "&amp;";
    default:
        return NULL;
    }
}

/** \fn void void handle_special_characters(const char *in, char *out)
 *  \param in - input character buffer
 *  \param size - size of out in characters
 *  \param out - output character buffer
 *  \details
 *  this function changes symbols that can not be present in xml in literal
 *  form to special symbol sequencies.
 */
 void Mut_log::handle_special_characters(char *out, int size, const char *in)
{
    int len = (int)strlen(in);
    int escaped_value_len = 0;
    int i;
    memset(out, 0, size);
    for(i = 0; i < len; i++)
    {
        const char * escape_seq = escape_char(in[i]);
        if(escape_seq)
        {
            escaped_value_len += (int)strlen(escape_seq);
            /* strcat must not overflow buffer */
            if(escaped_value_len < MUT_STRING_LEN) 
                strcat(out, escape_seq);
        }
        else
            if(escaped_value_len < MUT_STRING_LEN - 1) 
                out[escaped_value_len++] = in[i];
    }
}

/***************************************************************************
 * XML SPECIFIC SUPPORT FUNCTIONS END
 **************************************************************************/

/***************************************************************************
 * MUT OUTPUT HELPERS
 **************************************************************************/

enum { mut_spaces_len = 64 };
static const char mut_spaces_buffer[mut_spaces_len + 1] =
    "                                                                ";

/** \fn static const char* mut_spaces(int w)
 *  \param w - required width of the spaces string
 *  \return space string of specified width
 */
const char* Mut_log::mut_spaces(size_t actual, size_t desired)
{
    size_t w = 0;
    if (desired > actual)
    {
        w = desired - actual;
    }

    return mut_spaces_buffer + mut_spaces_len - w;
}

/** \fn void mut_log_flex_write_prim(char *buf, mut_logging_t forsed_level, const char **args, mut_log_entry_coll_t *coll)
 *  \param buf - storage location for output
 *  \param int filter_level - TRUE for use level for filtering the result
 *  \param int escape_characters - TRUE for escaping characters according to XML specification
 *  \param forsed_level - overides the verbosity level,
 *         MUT_LOG_NO_CHANGE to keep the level defined by the
 *         format
 *  \param args - string parameters for formatted output
 *  \param coll - reference to format specification
 */
void Mut_log::mut_log_flex_write_prim(char *buf, int filter_level, int escape_characters, mut_logging_t forsed_level, const char **args, Mut_log_entry_coll *coll)
{
    unsigned i;
    mut_logging_t level;
    char level_str[MUT_STRING_LEN];
    unsigned e;

    *buf = 0;

    for (e = 0; e < coll->coll_size; e++)
    {
        Mut_log_format_entry *format = coll->entry_coll + e;

        if (forsed_level >= 0)
        {
            level = forsed_level;
        }
        else
        {
            level = format->level;
        }

        if (filter_level && (level > log_level.get()))
        {
            continue;
        }

        sprintf(level_str, "%d", level);
        args[0] = level_str;

        strcat(buf, format->innertext[0]);
        for (i = 0; i < format->size;)
        {
            /* here we should made an escaping*/
            const char* value = args[format->fieldindex[i]];
            char escaped_value[MUT_STRING_LEN];
            if(escape_characters)
            {
                handle_special_characters(escaped_value, MUT_STRING_LEN, value);
                value = escaped_value;
            }
            if (format->alignment[i] == mut_format_alignment_left)
            {
                strcat(buf, value);
                strcat(buf, mut_spaces(strlen(value), format->fieldwidth[i]));
            }
            else
            {
                strcat(buf, mut_spaces(strlen(value), format->fieldwidth[i]));
                strcat(buf, value);
            }

            strcat(buf, format->innertext[++i]);
        }
    }

    return;
}


/** \fn void mut_log_flex_write(unsigned destinationMask, mut_logging_t level, mut_log_format_item_t format_ndx, ...)
 *  \param level - overides the verbosity level,
 *         MUT_LOG_NO_CHANGE to keep the level defined by the
 *         format
 *  \param format_ndx - format specification index
 *  \param ... - string parameters for formatted output
 */
void Mut_log::mut_log_flex_write(unsigned destinationMask, mut_logging_t level, mut_log_format_item_t format_ndx, ...)
{
    char con[MUT_STRING_LEN]; /* Buffer for console output */
    char txt[MUT_STRING_LEN]; /* Buffer for TXT file output */
    char xml[MUT_STRING_LEN]; /* Buffer for XML file output */
    va_list argptr;
    unsigned i;
    const char* args[100];
    char time[TIMESTAMP_SIZEOF]; 
    char date[DATESTAMP_SIZEOF];
    Mut_log_format_spec *format = mut_log_format_entry + format_ndx;

    time2str(time);
    _strdate(date);

    args[0] = 0;
    args[1] = format->kind;
    args[2] = date;
    args[3] = time;
    args[4] = log_control.section;

    va_start(argptr, format_ndx);
    for (i = 5; i < format->arguments_size; i++)
    {
        args[i] = va_arg(argptr, const char*);
    }
    va_end(argptr);

    if(destinationMask & MUT_CONSOLE_OUTPUT_ENABLED) {
        mut_log_flex_write_prim(con, MUT_TRUE, MUT_FALSE, level, args, &format->format[MUT_LOG_TEXT]);
        mut_strip_terminators(con);
    }
    if(destinationMask & MUT_TEXT_LOG_OUTPUT_ENABLED) {
        mut_log_flex_write_prim(txt, MUT_TRUE, MUT_FALSE, level, args, &format->format[MUT_LOG_TEXT]);
    }
    if(destinationMask & MUT_XML_LOG_OUTPUT_ENABLED) {
    mut_log_flex_write_prim(xml, MUT_TRUE, MUT_TRUE, level, args, &format->format[MUT_LOG_XML]);
        mut_log_flex_write_prim(txt, MUT_TRUE, MUT_FALSE, level, args, &format->format[MUT_LOG_TEXT]);
    }

    /* Transfer to the outputs */
    mut_log_transfer(destinationMask, con, txt, xml);

    return;
}

/** \fn void mut_log_flex_write_single(char *buffer, mut_log_format_item_t format_ndx, unsigned form_ndx, ...)
 *  \param level - overides the verbosity level, -1 to keep the level defined by the format
 *  \param format_ndx - format specification index
 *  \param ... - string parameters for formatted output
 */
void Mut_log::mut_log_flex_write_single(char *buffer, int filter_level, mut_log_format_item_t format_ndx, unsigned form_ndx, ...)
{
    va_list argptr;
    unsigned i;
    const char* args[100];
    char time[TIMESTAMP_SIZEOF];
    char date[DATESTAMP_SIZEOF];

    Mut_log_format_spec *format = mut_log_format_entry + format_ndx;

     time2str(time);
    _strdate(date);
 
    args[0] = 0;
    args[1] = format->kind;
    args[2] = date;
    args[3] = time;
    args[4] = log_control.section;
    
    va_start(argptr, form_ndx);
    for (i = 5; i < format->arguments_size; i++)
    {
        args[i] = va_arg(argptr, const char*);
    }
    va_end(argptr);

    mut_log_flex_write_prim(buffer, filter_level, MUT_FALSE, MUT_LOG_NO_CHANGE, args, &format->format[form_ndx]);
    
    return;
}

/* doxygen documentation for this function can be found in mut_log_private.h */
void Mut_log::mut_convert_to_xml(char *xml, const char *format_string, ...)
{
    char buff[MUT_STRING_LEN]; /* Buffer for XML file output */
    va_list argptr;
    const char* args[100];
    char time[TIMESTAMP_SIZEOF]; 
    char date[DATESTAMP_SIZEOF];
	Mut_log_format_spec *format = mut_log_format_entry + mut_formatitem_mutprintf;

    va_start(argptr, format_string);
    vsprintf(buff, format_string, argptr);
    va_end(argptr);

    time2str(time);
    _strdate(date);

    args[0] = 0;
    args[1] = format->kind;
    args[2] = date;
    args[3] = time;
    args[4] = log_control.section;
    args[5] = buff;

    mut_log_flex_write_prim(xml, MUT_FALSE, MUT_TRUE, MUT_LOG_TEST_STATUS, args, &format->format[MUT_LOG_XML]);

    return;
}

/***************************************************************************
 * GLOBAL FUNCTION DECLARATION
 * Doxygen documentation for these functions is located in .h files
 **************************************************************************/

/* doxygen documentation for this function can be found in mut.h */
void Mut_log::mut_report_test_started(Mut_testsuite *suite, Mut_test_entry *test)
{
    char testnam_str[256];
    char testnum_str[16];

    sprintf(testnum_str, "%d", test->get_test_index());
    sprintf(testnam_str, "%s.%s", suite->get_name(), test->get_test_id());
    
    
        mut_log_flex_write(MUT_ALL_OUTPUT_ENABLED, MUT_LOG_NO_CHANGE, mut_formatitem_teststarted, testnum_str, testnam_str, test->get_test_instance()->get_short_description());
    
    log_control.statistics.asserts_failed = 0;        
}

/* doxygen documentation for this function can be found in mut.h */
void Mut_log::mut_report_test_finished(Mut_testsuite *suite, Mut_test_entry *test, const char *value, bool updateLog)
{
    char testnam_str[256];
    char testnum_str[16];

    if (test->get_status() == MUT_TEST_PASSED)
    {
        log_control.statistics.tests_passed++;
    }
    else
    {
        log_control.statistics.tests_failed++;
    }

    if(updateLog) {
        sprintf(testnum_str, "%d", test->get_test_index());
        sprintf(testnam_str, "%s.%s", suite->get_name(), test->get_test_id());
  
        mut_log_flex_write(MUT_ALL_OUTPUT_ENABLED, MUT_LOG_NO_CHANGE, mut_formatitem_testfinished, testnum_str, testnam_str, value);
    }

    return;
}

/* doxygen documentation for this function can be found in mut.h */
void Mut_log::mut_report_test_timeout(Mut_testsuite *suite, Mut_test_entry *test, const char *value, bool updateLog)
{
    char testnam_str[256];
	char testnum_str[16];

    log_control.statistics.tests_failed++;
   
    if(updateLog) {
		sprintf(testnum_str, "%d", test->get_test_index());
        sprintf(testnam_str, "Timeout out occured in %s %s.%s", value, suite->get_name(), test->get_test_id());
        
        mut_log_flex_write(MUT_ALL_OUTPUT_ENABLED, MUT_LOG_NO_CHANGE, mut_formatitem_result_testitem_failed, testnam_str, testnum_str);
    }

    return;
}

/* doxygen documentation for this function can be found in mut.h */
void Mut_log::mut_report_test_notexecuted(Mut_testsuite *suite, Mut_test_entry *test)
{
    log_control.statistics.tests_notexecuted++;

    return;
}

void Mut_log::create_log_console(void)
{
    return;
}

void Mut_log::create_log_file(void)
{
    /*
     * user must specify either -txt or -xml to request that a log file be created
     */
    
    if (get_log_type() != MUT_LOG_NONE)
    {
        if(get_log_type() == MUT_LOG_BOTH || get_log_type() == MUT_LOG_TEXT)
        {
            log_control.text_file = fopen(log_control.text_log_file, "wc");
            if (log_control.text_file == NULL)
            {
                MUT_INTERNAL_ERROR(("Could not create text log file \"%s\"", log_control.text_log_file));
            }
        }

        if(get_log_type() == MUT_LOG_BOTH || get_log_type() == MUT_LOG_XML)
        {
            log_control.xml_file = fopen(log_control.xml_log_file, "wc");
            if (log_control.xml_file == NULL)
            {
                MUT_INTERNAL_ERROR(("Could not create xml log file \"%s\"", log_control.xml_log_file));
            }
        }
    }

    return;
}
/* doxygen documentation for this function can be found in mut_log.h */
void Mut_log::create_log(const char * /*suite*/name)
{
    mut_init_log_name(name);

    create_log_console();
    create_log_file();
   
}

void Mut_log::close_log_console(void)
{
    fflush(stdout);
    return;
}


void Mut_log::close_log_file(void)
{
    if (log_control.text_file != NULL)
    {
        fflush(log_control.text_file);
        fclose(log_control.text_file);
        log_control.text_file = NULL;
    }

    if (log_control.xml_file != NULL)
    {
        fflush(log_control.xml_file);
        fclose(log_control.xml_file);
        log_control.xml_file = NULL;
    }

    return;
}

/* doxygen documentation for this function can be found in mut_log.h */
void Mut_log::close_log()
{
    close_log_console();
    close_log_file();
 

    return;
}

void Mut_log::mut_log_transfer(unsigned destinationMask, const char *con, const char *txt, const char *xml)
{
    /* CG - added the call to csx_p_thr_os_blockable because these functions aren't async signal handler safe.  if you
     * want to remove them go ahead, but be prepared for deadlocks */ 
    if ((destinationMask & MUT_CONSOLE_OUTPUT_ENABLED && !disableConsole.get())) {
        if(strlen(con) != 0) {
            mut_log_console(con);
        }
    }

    if((destinationMask & MUT_TEXT_LOG_OUTPUT_ENABLED || destinationMask & MUT_XML_LOG_OUTPUT_ENABLED)) {
        mut_log_file(txt, xml);
    }
    return;
}

/* This function is reserved for KTrace integration */
static void ktrace_listener(char *msg)
{
    mut_ktrace(msg);

    return;
}

/* doxygen documentation for this function can be found in mut_log.h */
void Mut_log::mut_report_testsuite_starting(const char *name, int current_it, int total_it)
{
    char *compname = mut_get_host_name();
    char *username = mut_get_user_name();
    char mut_framework_version[MUT_STRING_LEN];
    char it_msg[64];
    char cmdLine[1024];

    cmdLine[0] = '\0';
    SAFE_STRNCAT(cmdLine, mut_get_program_name(), sizeof(cmdLine));
    SAFE_STRNCAT(cmdLine, " ", sizeof(" "));
    SAFE_STRNCAT(cmdLine, mut_get_cmdLine(), sizeof(cmdLine));


    // convert to 1 based for friendly display
    ++current_it;

    if (total_it == ITERATION_ENDLESS)
        sprintf_s(it_msg, sizeof(it_msg),"%d of infinite", current_it);
    else 
        sprintf_s(it_msg, sizeof(it_msg), "%d of total %d", current_it, total_it);

    sprintf(mut_framework_version, "%d.%d", mut_version_major, mut_version_minor);

    log_control.statistics.suitestarted = mut_tickcount_get();
    log_control.statistics.tests_failed = 0;
    log_control.statistics.tests_passed = 0;
    log_control.statistics.tests_notexecuted = 0;
    log_control.statistics.asserts_failed = 0;

    log_control.section = mut_xml_tag_mutheader;
    
    if (get_log_type() == MUT_LOG_NONE)
    {
        mut_log_flex_write(MUT_CONSOLE_OUTPUT_ENABLED,
                            MUT_LOG_NO_CHANGE,
                            mut_formatitem_header,
                            mut_xml_tag_mutlog,
                            mut_log_version,
                            mut_framework_version,
                            name,
                            "no log",
                            username,
                            compname, 
                            it_msg,
                            cmdLine);

    }
    else if(get_log_type() == MUT_LOG_TEXT)
    {
        mut_log_flex_write(MUT_CONSOLE_AND_TEXT_LOG_OUTPUT_ENABLED,
                            MUT_LOG_NO_CHANGE,
                            mut_formatitem_header,
                            mut_xml_tag_mutlog,
                            mut_log_version,
                            mut_framework_version,
                            name,
                            log_control.text_log_file,
                            username,
                            compname, 
                            it_msg,
                            cmdLine);
    }
    else if(get_log_type() == MUT_LOG_XML)
    {
        mut_log_flex_write(MUT_CONSOLE_OUTPUT_ENABLED,
                            MUT_LOG_NO_CHANGE,
                            mut_formatitem_header,
                            mut_xml_tag_mutlog,
                            mut_log_version,
                            mut_framework_version,
                            name,
                            log_control.xml_log_file,
                            username,
                            compname, 
                            it_msg,
                            cmdLine);

        mut_log_flex_write(MUT_XML_LOG_OUTPUT_ENABLED,
                            MUT_LOG_NO_CHANGE,
                            mut_formatitem_header,
                            mut_xml_tag_mutlog,
                            mut_log_version,
                            mut_framework_version,
                            name,
                            log_control.xml_log_file,
                            username,
                            compname,
                            cmdLine);
    }
    else if(get_log_type() == MUT_LOG_BOTH)
    {
        /*
         * process header twice, once to generate the text header, once to generate the XML header
         * Manipulate the FILE descriptors, so that the header will only be written to the appropriate file
         */
        mut_log_flex_write(MUT_CONSOLE_AND_TEXT_LOG_OUTPUT_ENABLED,
                            MUT_LOG_NO_CHANGE,
                            mut_formatitem_header,
                            mut_xml_tag_mutlog,
                            mut_log_version,
                            mut_framework_version,
                            name,
                            log_control.text_log_file,
                            username,
                            compname,
                            it_msg,
                            cmdLine);

        mut_log_flex_write(MUT_XML_LOG_OUTPUT_ENABLED,
                            MUT_LOG_NO_CHANGE,
                            mut_formatitem_header,
                            mut_xml_tag_mutlog,
                            mut_log_version,
                            mut_framework_version,
                            name,
                            log_control.xml_log_file,
                            username,
                            compname,
                            cmdLine);

    }

//    mut_printf(MUT_LOG_SUITE_STATUS, "CLI: %s", mut_get_cmdLine());

    log_control.section = mut_xml_tag_mutbody;

    free(compname);
    free(username);

    return;
}

/* doxygen documentation for this function can be found in mut_log.h */
void Mut_log::mut_report_testsuite_finished(const char *name, mut_testsuite_t *suite)
{
	mut_summary_t *pMut_summary = NULL;
	Mut_testsuite *current_suite = (Mut_testsuite *)suite;
	vector<Mut_test_entry *>::iterator current_test = current_suite->test_list->get_list_begin();

    // Allocate a summary structure
    pMut_summary = (mut_summary_t *)malloc(sizeof(mut_summary_t));

    if (!pMut_summary)
    {
        MUT_INTERNAL_ERROR(("Malloc failed in mut_report_testsuite finished"));
    }

    pMut_summary->suite_duration = mut_tickcount_get() - log_control.statistics.suitestarted;

    log_control.section = mut_xml_tag_result;

    *pMut_summary->testsfailed_txt_list = '\0';
    *pMut_summary->testsnotexecuted_txt_list = '\0';
    *pMut_summary->testsfailed_xml_list = '\0';
    *pMut_summary->testsnotexecuted_xml_list ='\0';
    while(current_test != current_suite->test_list->get_list_end())
    {
        sprintf(pMut_summary->test_index_str, "%d", (*current_test)->get_test_index());

        switch ((*current_test)->get_status())
        {
        case MUT_TEST_NOT_EXECUTED:
            {
            mut_log_flex_write_single(pMut_summary->testitem_txt, MUT_FALSE,
                mut_formatitem_result_testitem_notexecuted, 
                MUT_LOG_TEXT, (*current_test)->get_test_id(), pMut_summary->test_index_str);
            mut_log_flex_write_single(pMut_summary->testitem_xml, MUT_FALSE, 
                mut_formatitem_result_testitem_notexecuted, 
                MUT_LOG_XML, (*current_test)->get_test_id(), pMut_summary->test_index_str);

            strcat(pMut_summary->testsnotexecuted_txt_list, pMut_summary->testitem_txt);
            strcat(pMut_summary->testsnotexecuted_xml_list, pMut_summary->testitem_xml);
            break;
            }

        case MUT_TEST_PASSED:
            {
            break;
            }

        default:
            {
            mut_log_flex_write_single(pMut_summary->testitem_txt, MUT_FALSE,
                mut_formatitem_result_testitem_failed, 
                MUT_LOG_TEXT, (*current_test)->get_test_id(), pMut_summary->test_index_str);
            mut_log_flex_write_single(pMut_summary->testitem_xml, MUT_FALSE,
                mut_formatitem_result_testitem_failed, 
                MUT_LOG_XML, (*current_test)->get_test_id(), pMut_summary->test_index_str);

            strcat(pMut_summary->testsfailed_txt_list, pMut_summary->testitem_txt);
            strcat(pMut_summary->testsfailed_xml_list, pMut_summary->testitem_xml);
            break;
            }
        }

        current_test++;
    
    }
    
    sprintf(pMut_summary->s_dur, "%lu.%03lu", pMut_summary->suite_duration / 1000, pMut_summary->suite_duration % 1000);
    
    sprintf(pMut_summary->status, "%s%s",
        log_control.statistics.tests_passed > 0 && log_control.statistics.tests_failed == 0 ?
            "PASSED" : "FAILED",
        log_control.statistics.tests_notexecuted ?
            "-incomplete" : "");

    sprintf(pMut_summary->failed,      "%u", log_control.statistics.tests_failed);
    sprintf(pMut_summary->passed,      "%u", log_control.statistics.tests_passed);
    sprintf(pMut_summary->notexecuted, "%u", log_control.statistics.tests_notexecuted);

    if (log_control.statistics.tests_failed)
    {
        mut_log_flex_write_single(
            pMut_summary->testsfailed_txt, MUT_FALSE, 
            mut_formatitem_result_testtable_failed, 
            MUT_LOG_TEXT, pMut_summary->testsfailed_txt_list);

        mut_log_flex_write_single(
            pMut_summary->testsfailed_xml, MUT_FALSE, 
            mut_formatitem_result_testtable_failed, 
            MUT_LOG_XML, pMut_summary->testsfailed_xml_list);
    }
    else
    {
        *pMut_summary->testsfailed_txt = '\0';
        *pMut_summary->testsfailed_xml = '\0';
    }

    if (log_control.statistics.tests_notexecuted)
    {
        mut_log_flex_write_single(pMut_summary->testsnotexecuted_txt, MUT_FALSE,
            mut_formatitem_result_testtable_notexecuted, 
            MUT_LOG_TEXT, pMut_summary->testsnotexecuted_txt_list);
        mut_log_flex_write_single(pMut_summary->testsnotexecuted_xml, MUT_FALSE,
            mut_formatitem_result_testtable_notexecuted, 
            MUT_LOG_XML, pMut_summary->testsnotexecuted_xml_list);
    }
    else
    {
        *pMut_summary->testsnotexecuted_txt = '\0';
        *pMut_summary->testsnotexecuted_xml ='\0';
    }

    mut_log_flex_write_single(pMut_summary->buf_con, MUT_TRUE, mut_formatitem_result, MUT_LOG_TEXT,
        name, pMut_summary->s_dur, 
        pMut_summary->testsfailed_txt,
        pMut_summary->testsnotexecuted_txt,
        pMut_summary->status,
        pMut_summary->failed, pMut_summary->passed, pMut_summary->notexecuted
    );

    mut_log_flex_write_single(pMut_summary->buf_txt, MUT_FALSE, mut_formatitem_result, MUT_LOG_TEXT,
        name, pMut_summary->s_dur, 
        pMut_summary->testsfailed_txt,
        pMut_summary->testsnotexecuted_txt,
        pMut_summary->status,
        pMut_summary->failed, pMut_summary->passed, pMut_summary->notexecuted
    );

    mut_log_flex_write_single(pMut_summary->buf_xml, MUT_FALSE, mut_formatitem_result, MUT_LOG_XML,
        name, pMut_summary->s_dur, 
        pMut_summary->testsfailed_xml,
        pMut_summary->testsnotexecuted_xml,
        pMut_summary->status,
        pMut_summary->failed, pMut_summary->passed, pMut_summary->notexecuted
    );
    
    mut_strip_terminators(pMut_summary->buf_con);

    mut_log_transfer(MUT_ALL_OUTPUT_ENABLED, pMut_summary->buf_con, pMut_summary->buf_txt, pMut_summary->buf_xml);

    free(pMut_summary);
    return;
}

/* doxygen documentation for this function can be found in mut_log.h */
void mut_ktrace(const char *format, ...)
{
    char buffer[MSG_BUFFER_SIZEOF];
    va_list argptr;

    if(mut_control->mut_log->log_level.get() == MUT_LOG_KTRACE) {
        va_start(argptr, format);
        _vsnprintf(buffer, MSG_BUFFER_SIZEOF - 1, format, argptr);
        va_end(argptr);
        mut_control->mut_log->mut_log_flex_write(MUT_ALL_OUTPUT_ENABLED, MUT_LOG_KTRACE, mut_formatitem_ktrace, buffer);
    }
}


/* doxygen documentation for this function can be found in mut_log.h */
void mut_printf(const mut_logging_t level, const char *format, ...)
{
    char buffer[MSG_BUFFER_SIZEOF];
    va_list argptr;

    va_start(argptr, format);
    vsprintf(buffer, format, argptr);
    va_end(argptr);

    if(mut_control != NULL) {
        mut_control->mut_log->mut_log_flex_write(MUT_ALL_OUTPUT_ENABLED, level, mut_formatitem_mutprintf, buffer);
    }
    else {
        /*
         * Test code is calling mut_printf, before mut_init
         * Just display message on the console
         */
        printf("%s", buffer);
    }

    return;
}

/*!**************************************************************
 * mut_set_color()
 ****************************************************************
 * @brief
 *  change the output color of the font on the screen.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  3/11/2011 - Created. zphu
 *
 ****************************************************************/
void mut_set_color(mut_color_t color)
{
    if (!mut_control->mut_log->mut_is_color_output())
        return;

    if(color > MUT_COLOR_WHITE)
    {
        mut_printf(MUT_LOG_LOW, "%s: Can not set screen font color to %d.", __FUNCTION__, color);
        return;
    }

    // We need to get the correct csx way to do this.
    //SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

/** \fn static int mut_str_comp(const char *v, const char *p)
 *  \param v - string
 *  \param p - string
 *  \return 1 if v matches beginning of p
 *  \details
 *  check if string p match the beginning of v
 */
bool Mut_log:: mut_str_comp(const char *v, const char *p)
{
    while (*p && *(p++) == *(v++)) {};

    return !*p;
}

unsigned Mut_log::mut_file_format_token(char* buffer, const char** f, int solid, const char* pattern, const char* value_s, int value_i)
{
    if (solid)
    {
        if (mut_str_comp(*f, pattern))
        {
            strcat(buffer, value_s);
            *f += strlen(pattern);
             
            return MUT_TRUE;
        }
    }
    else
    {
        size_t count = 0;
        char v[16];
        size_t sl;

        while (**f == pattern[0])
        {
            (*f)++;
            count++;
        }

        if (!count) 
        {
            return MUT_FALSE;
        }
        
        sprintf(v, "%04u", value_i);
        sl = strlen(v);
        strcat(buffer, v + sl - min(sl, count));

        return MUT_TRUE;
    }

    return MUT_FALSE;
}

void Mut_log::mut_fill_format(char* buffer, const char* format, const char* suite, const char* extension, 
    int year, int month, int day, int hours, int minutes, int seconds)
{
    const char *ext = extension;
    unsigned mode = MUT_FALSE;

#ifdef ALAMOSA_WINDOWS_ENV
    sprintf(buffer, "%s\\", logdir.get());
#else
    sprintf(buffer, "%s/", logdir.get());
#endif /* ALAMOSA_WINDOWS_ENV - PATHS */
    if (format)
    {
        while (*format)
        {
            if (*format == '%')
            {
                format++;
                mode = !mode;
                continue;
            }
    
            if (mode)
            {
                if (mut_file_format_token(buffer, &format, 0, "Y", 0, year)) continue;
                if (mut_file_format_token(buffer, &format, 0, "M", 0, month)) continue;
                if (mut_file_format_token(buffer, &format, 0, "D", 0, day)) continue;
                if (mut_file_format_token(buffer, &format, 0, "h", 0, hours)) continue;
                if (mut_file_format_token(buffer, &format, 0, "m", 0, minutes)) continue;
                if (mut_file_format_token(buffer, &format, 0, "s", 0, seconds)) continue;
                if (mut_file_format_token(buffer, &format, 1, "testsuite", suite, 0)) continue;
                if (mut_file_format_token(buffer, &format, 1, "ext", ext, 0)) continue;
                MUT_INTERNAL_ERROR(("Unexpected format, %s", format))
            }
            else
            {
                char *b = buffer + strlen(buffer);
                *(b++) = *(format++);
                *b = 0;
            }
        }
    }

    return;
}

/* doxygen documentation for this function can be found in mut_log.h */
void Mut_log::mut_init_log_name(const char *name)
{
    struct tm *newtime;
    __time64_t long_time;

    _time64(&long_time);
    newtime = _localtime64(&long_time);

    /* construct date */
    newtime->tm_year += 1900;
    newtime->tm_mon += 1;

    // Create the text log file name
    mut_fill_format(log_control.text_log_file,
                        filename.get(), 
						name,
                        "txt",
                        newtime->tm_year,
                        newtime->tm_mon,
                        newtime->tm_mday,
                        newtime->tm_hour,
                        newtime->tm_min,
                        newtime->tm_sec);
        
	// Create the xml log file name
	mut_fill_format(log_control.xml_log_file,
                        filename.get(), 
						name,
                        "xml",
                        newtime->tm_year,
                        newtime->tm_mon,
                        newtime->tm_mday,
                        newtime->tm_hour,
                        newtime->tm_min,
                        newtime->tm_sec);
        
	// Create the dump file name
	mut_fill_format(log_control.dump_file_name,
                        filename.get(), 
						name,
                        "dmp",
                        newtime->tm_year,
                        newtime->tm_mon,
                        newtime->tm_mday,
                        newtime->tm_hour,
                        newtime->tm_min,
                        newtime->tm_sec);

    return;
}

void Mut_log::mut_remove_nl(char *s)
{
    size_t l = strlen(s);

    if (l < 1)
    {
        return;
    }

    if (s[l - 1] != mut_log_format_spec_crlf)
    {
        return;
    }

    s[l - 1] = 0;

    return;
}

void Mut_log::mut_format_load(void)
{
    FILE *f;
    char buf[MUT_STRING_LEN];
    char name[MUT_STRING_LEN];
    char accu[MUT_STRING_LEN];
    int i;
    unsigned stop;

    if (!formatfile.get())
    {
        return;
    }

    if (!(f = fopen(filename.get(), "r")))
    {
        MUT_INTERNAL_ERROR(("Unable to open text format file \"%s\"", formatfile.get()))
    }

    while(fgets(buf, MUT_STRING_LEN, f) != NULL)
    {
        mut_remove_nl(buf);

        if (buf[0] == mut_log_format_spec_char)
        {
            if (strlen(buf+1) < strlen(name)) 
            {
                strcpy(name, buf + 1);
            }
            else
            {
                MUT_INTERNAL_ERROR(("String too long"));
            }
        }
        else
        {
            MUT_INTERNAL_ERROR(("Expected %c", mut_log_format_spec_char));
        }

        *accu = 0;
        stop = MUT_FALSE;

        while(fgets(buf, MUT_STRING_LEN, f) != NULL)
        {
            if (strlen(buf) > 2 && buf[strlen(buf) - 2] == mut_log_format_spec_char && buf[strlen(buf) - 3] == mut_log_format_spec_char)
            {
                buf[strlen(buf) - 3] = 0;
                stop = MUT_TRUE;
            }

            strcat(accu, buf);

            if (stop)
            {
                break;
            }

        }

        for (i = 0; i < mut_formatitem_Size; i++)
        {
            if (!strcmp(mut_format_class[i], name))
            {
                strcpy(mut_format_spec_txt[i], accu);
                break;
            }
        }

        if (i >= mut_formatitem_Size)
        {
            MUT_INTERNAL_ERROR(("No such format item \"%s\"", name));
        }
    }

    fclose(f);

    return;
}

/* doxygen documentation for this function can be found in mut_log.h */
void Mut_log::mut_log_init(void)
{
    

    *log_control.text_log_file = 0;
    *log_control.xml_log_file = 0;
    log_control.text_file = NULL;
    log_control.xml_file = NULL;
    
    /*
     * all formatted messages require a section be specified.
     * Currently, issueing MUT_ASSERT or mut_print or received via mut_ktrace
     * will generate an exception, if section is NULL
     * 
     * Any test that performs initialization before 
     * MUT_RUN_TESTSUITE/mut_run_testsuite
     * will fault in mut_log_format_prepare_prim
     * if section is NULL.
     */
    log_control.section = "Unknown"; 

    return;
}

/* doxygen documentation for this function can be found in mut_log.h */
void Mut_log::mut_log_prepare(void)
{
   mut_format_load();
   mut_format_initialization();

   csx_ci_container_set_output_handlers_all(csx_ci_container_context_get(), CSX_NULL, Mut_log::mut_log_csx_output_handler);

   return;
}
/* doxygen documentation for this function can be found in mut.h */
void Mut_log::mut_log_assert(const char* file, unsigned line, const char* func,const char *fmt, ...)
{
    char time[TIMESTAMP_SIZEOF];
    char date[DATESTAMP_SIZEOF];
    char thread_str[16];
    char line_str[16];

    char reason[MUT_STRING_LEN];
    va_list argptr;

    csx_thread_id_t thread = csx_p_get_thread_id(); 

    time2str(time);
    _strdate(date);

    sprintf(thread_str, "0x%llX", (unsigned long long)thread);
    sprintf(line_str, "%u", line);

    va_start(argptr, fmt);
    csx_p_stdio_vsprintf(reason, fmt, argptr);
    va_end(argptr);

    log_control.statistics.asserts_failed++;
    mut_log_flex_write(MUT_ALL_OUTPUT_ENABLED, MUT_LOG_NO_CHANGE, mut_formatitem_assertfailed, file, line_str, func, thread_str, reason);

    mut_process_test_failure();

    return;
}

/** \fn LONG WINAPI ReportIncompleteTests(Mut_testsuite *suite, Mut_test_entry *test)
 *  \param suite - pointer to the suite
 *  \param suite - pointer to the current test
 *  \details
 *  This function is called to report test(s) that were not executed.
 */
void Mut_log::ReportIncompleteTests(Mut_testsuite *suite, Mut_test_entry *test)
{
    vector<Mut_test_entry *>::iterator current_test;

    current_test = suite->test_list->get_list_begin();
    while((*current_test++)->get_test_index() != test->get_test_index()) {}; // loop through tests up to the current one          

	/* Count how many tests were not executed by scanning the rest of test_list */
	while(current_test != suite->test_list->get_list_end())
	{
		mut_report_test_notexecuted(suite, *current_test); // need to move this function, or reference from log object?
		current_test++;
	}
}
/* doxygen documentation for this function can be found in mut.h */

int mut_has_any_assert_failed(){
    return mut_control->mut_log->mut_has_any_assert_failed() ? 1:0;
}

bool Mut_log::mut_has_any_assert_failed()
{
    return log_control.statistics.asserts_failed != 0;
}
/* doxygen documentation for this function can be found in mut_log.h */
void Mut_log::mut_report_test_summary(const char *name, mut_testsuite_t *suite, int iteration)
{
	mut_summary_t *pMut_summary = NULL;
	Mut_testsuite *current_suite = (Mut_testsuite *)suite;
	vector<Mut_test_entry *>::iterator current_test = current_suite->test_list->get_list_begin();

    // Allocate a summary structure
    pMut_summary = (mut_summary_t *)malloc(sizeof(mut_summary_t));
    if (!pMut_summary)
    {
        MUT_INTERNAL_ERROR(("Malloc failed in mut_report_testsuite finished"));
    }

    pMut_summary->suite_duration = mut_tickcount_get() - log_control.statistics.suitestarted;

    log_control.section = mut_xml_tag_result;

    *pMut_summary->testsfailed_txt_list = '\0';
    *pMut_summary->testsnotexecuted_txt_list = '\0';
    *pMut_summary->testsfailed_xml_list = '\0';
    *pMut_summary->testsnotexecuted_xml_list = '\0';
    while(current_test != current_suite->test_list->get_list_end() )
    {
        sprintf(pMut_summary->test_index_str, "%d", (*current_test)->get_test_index());

        switch ((*current_test)->get_status())
        {
        case MUT_TEST_NOT_EXECUTED:
            {
            mut_log_flex_write_single(pMut_summary->testitem_txt, MUT_FALSE,
                mut_formatitem_result_testitem_notexecuted, 
                MUT_LOG_TEXT, (*current_test)->get_test_id(), pMut_summary->test_index_str);
            mut_log_flex_write_single(pMut_summary->testitem_xml, MUT_FALSE, 
                mut_formatitem_result_testitem_notexecuted, 
                MUT_LOG_XML, (*current_test)->get_test_id(), pMut_summary->test_index_str);

            strcat(pMut_summary->testsnotexecuted_txt_list, pMut_summary->testitem_txt);
            strcat(pMut_summary->testsnotexecuted_xml_list, pMut_summary->testitem_xml);
            break;
            }

        case MUT_TEST_PASSED:
            {
            break;
            }

        default:
            {
            mut_log_flex_write_single(pMut_summary->testitem_txt, MUT_FALSE,
                mut_formatitem_result_testitem_failed, 
                MUT_LOG_TEXT, (*current_test)->get_test_id(), pMut_summary->test_index_str);
            mut_log_flex_write_single(pMut_summary->testitem_xml, MUT_FALSE,
                mut_formatitem_result_testitem_failed, 
                MUT_LOG_XML, (*current_test)->get_test_id(), pMut_summary->test_index_str);

            strcat(pMut_summary->testsfailed_txt_list, pMut_summary->testitem_txt);
            strcat(pMut_summary->testsfailed_xml_list, pMut_summary->testitem_xml);
            break;
            }
        }

        current_test++;
    }
    
    sprintf(pMut_summary->s_dur, "%lu.%03lu", pMut_summary->suite_duration / 1000, pMut_summary->suite_duration % 1000);
    
    sprintf(pMut_summary->status, "%s%s",
        log_control.statistics.tests_passed > 0 && log_control.statistics.tests_failed == 0 ?
            "PASSED" : "FAILED",
        log_control.statistics.tests_notexecuted ?
            "-incomplete" : "");

    sprintf(pMut_summary->failed,      "%u", log_control.statistics.tests_failed);
    sprintf(pMut_summary->passed,      "%u", log_control.statistics.tests_passed);
    sprintf(pMut_summary->notexecuted, "%u", log_control.statistics.tests_notexecuted);

    if (log_control.statistics.tests_failed)
    {
        mut_log_flex_write_single(
            pMut_summary->testsfailed_txt, MUT_FALSE, 
            mut_formatitem_result_testtable_failed, 
            MUT_LOG_TEXT, pMut_summary->testsfailed_txt_list);

        mut_log_flex_write_single(
            pMut_summary->testsfailed_xml, MUT_FALSE, 
            mut_formatitem_result_testtable_failed, 
            MUT_LOG_XML, pMut_summary->testsfailed_xml_list);
    }
    else
    {
        *pMut_summary->testsfailed_txt = '\0';
        *pMut_summary->testsfailed_xml = '\0';
    }

    if (log_control.statistics.tests_notexecuted)
    {
        mut_log_flex_write_single(pMut_summary->testsnotexecuted_txt, MUT_FALSE,
            mut_formatitem_result_testtable_notexecuted, 
            MUT_LOG_TEXT, pMut_summary->testsnotexecuted_txt_list);
        mut_log_flex_write_single(pMut_summary->testsnotexecuted_xml, MUT_FALSE,
            mut_formatitem_result_testtable_notexecuted, 
            MUT_LOG_XML, pMut_summary->testsnotexecuted_xml_list);
    }
    else
    {
        *pMut_summary->testsnotexecuted_txt = '\0';
        *pMut_summary->testsnotexecuted_xml = '\0';
    }

    mut_log_flex_write_single(pMut_summary->buf_con, MUT_TRUE, mut_formatitem_result_summary, MUT_LOG_TEXT,
        name, name, 
        pMut_summary->testsfailed_txt,
        pMut_summary->testsnotexecuted_txt,
        pMut_summary->status,
        pMut_summary->failed, pMut_summary->passed, pMut_summary->notexecuted
    );

    // Save the suite summary (only if you are doing 1 iteration)
    if(iteration == 1)
        strcat(console_suite_summary, pMut_summary->buf_con);
    free(pMut_summary);

    return;
}

MUT_API char * mut_get_logdir() {
	return mut_control->mut_log->mut_get_logdir();  // stored in an option
}

MUT_API char * mut_get_log_file_name() {
    return mut_control->mut_log->get_log_file_name(); // return the whole valid filename
}

void mut_strip_terminators(char* buf)
{
#ifndef ALAMOSA_WINDOWS_ENV
    size_t strSize = strlen(buf);
    if(strSize == 0) {
        return;
    }
    for(int index = strSize-1; index >= 0; index--) {
        if(buf[index] == '\n') {
           buf[index] = '\0';
        } else {
            break;
        }
    }
#else // ALAMOSA_WINDOWS_ENV
    xsize_t strSize = strlen(buf);
    if(strSize <= 2) {
        return;
    }
    xsize_t lastIndex = 0;
    for(xsize_t index = strSize-1; index >= 0; index--) {
        if(buf[index] == '\n') {
            buf[index] = '\0';
            lastIndex = index;
        } else {
            break;
        }
    }
    if(lastIndex) {
        buf[lastIndex] = '\n';
    }
#endif // ALAMOSA_WINDOWS_ENV
}

