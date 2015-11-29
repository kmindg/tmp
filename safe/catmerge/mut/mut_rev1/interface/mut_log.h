/***************************************************************************
 * Copyright (C) EMC Corporation 2010-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_log.h
 ***************************************************************************
 *
 * DESCRIPTION: several mut log related classes and definitions
 *
 * Classes:
 *      Mut_log_statistics
 *      Mut_log_format_entry
 *      Mut_log_entry_coll
 *      Mut_log_format_spec
 *      Mut_log_format_spec
 *      Mut_log
 * NOTES:
 *
 * HISTORY:
 *    10/2010 Gutted, ready for new cpp implementation JD
 **************************************************************************/


#ifndef MUT_LOG_H
#define MUT_LOG_H
#include "EmcPAL.h"
#include "EmcUTIL.h"
#include "EmcUTIL_Time.h"
#include "mut_log_private.h"
#include "mut_private.h"
#include "mut_options.h"
#include "mut_abstract_log_listener.h"
#include "ktrace.h"

#define mut_log_format_fields_reserve 64
#define MUT_TXT_STD_FILEDS "%Date% %Time% %Verbosity:r2% %Section:6%"

#define MUT_STD_FIELDS "Verbosity", "Class", "Date", "Time", "Section"

#define MUT_FRM_VERB(v) "\\"#v":"
#define MUT_XML_TAB0
#define MUT_XML_TAB1 "    "
#define MUT_XML_TAB2 MUT_XML_TAB1 MUT_XML_TAB1
#define MUT_XML_TAB3 MUT_XML_TAB2 MUT_XML_TAB1
#define MUT_XML_TAB4 MUT_XML_TAB2 MUT_XML_TAB2

#define MUT_XML_TAG_OPEN(tab, id) tab "<"#id">\n"
#define MUT_XML_TAG_CLOSE(tab, id) tab "</"#id">\n"

#define MUT_XML_FIELD(tab, id) tab "<"#id">%"#id"%</"#id">\n"

#define MUT_XML_STD_FIELDS(tab, verb) \
    MUT_XML_FIELD(tab, Class) \
    MUT_FRM_VERB(verb)MUT_XML_FIELD(tab, Verbosity) \
    MUT_XML_FIELD(tab, Date) \
    MUT_XML_FIELD(tab, Time)

/** /class Mut_log_statistics
 *  /details Holds data items for statistics
 *   
 */
class Mut_log_statistics {
public:
    unsigned long suitestarted;
    unsigned tests_passed;       /* The number of tests passed during test suite execution */
    unsigned tests_failed;       /* The number of tests failed during test suite execution */
    unsigned tests_notexecuted;  /* The number of tests not executed due early abort */
    unsigned asserts_failed;     /* The number of asserts have been failed furing test execution */

    Mut_log_statistics();
};

/***************************************************************************
 * LOG TAGS DEFINITIONS
 **************************************************************************/


typedef enum mut_log_format_item_s {
    mut_formatitem_header = 0,
    mut_formatitem_teststarted = 1,
    mut_formatitem_assertfailed = 2,
    mut_formatitem_mutprintf = 3,
    mut_formatitem_ktrace = 4,
    mut_formatitem_testfinished = 5,
    mut_formatitem_result = 6,
    mut_formatitem_result_testtable_failed = 7,
    mut_formatitem_result_testtable_notexecuted = 8,
    mut_formatitem_result_testitem_failed = 9,
    mut_formatitem_result_testitem_notexecuted = 10,
	mut_formatitem_result_summary = 11,

    mut_formatitem_Size = 12,
} mut_log_format_item_t;

enum { 
    mut_log_format_spec_crlf = 0x0A, 
    mut_log_format_spec_char = '\\', 
};

enum {mut_format_fields_max = 32};


typedef enum mut_format_alignment_e {
    mut_format_alignment_left = 0,
    mut_format_alignment_right = 1,
} mut_format_alignment_t;

/* Log format entry definition */
class Mut_log_format_entry {
public:
    mut_logging_t level;
    unsigned size;       /* Number of format fileds */
    char innertext[mut_log_format_fields_reserve][MUT_STRING_LEN]; /* Inner text array */ 
    unsigned fieldindex[mut_log_format_fields_reserve];
    size_t fieldwidth[mut_log_format_fields_reserve]; 
    unsigned alignment[mut_log_format_fields_reserve]; 
};

class Mut_log_entry_coll {
public:
    unsigned coll_size; /* Number of entry arguments */
    Mut_log_format_entry entry_coll[20];    
};

class Mut_log_format_spec {
public:
    const char* kind;    /* Class definition */
    unsigned arguments_size; /* Number of entry arguments */
    Mut_log_entry_coll format[2];    
};



typedef struct mut_summary_s {
	char s_dur[MUT_STRING_LEN];

    char buf_con[MUT_STRING_LEN];

    char buf_txt[MUT_STRING_LEN];
    char testitem_txt[MUT_STRING_LEN];
    char testsfailed_txt[MUT_STRING_LEN];
    char testsfailed_txt_list[MUT_STRING_LEN];
    char testsnotexecuted_txt[MUT_STRING_LEN];
    char testsnotexecuted_txt_list[MUT_STRING_LEN];

    char buf_xml[MUT_STRING_LEN];
    char testitem_xml[MUT_STRING_LEN];
    char testsfailed_xml[MUT_STRING_LEN];
    char testsfailed_xml_list[MUT_STRING_LEN];
    char testsnotexecuted_xml[MUT_STRING_LEN];
    char testsnotexecuted_xml_list[MUT_STRING_LEN];
    
    unsigned long suite_duration;

    char status[MUT_SHORT_STRING_LEN];
    char failed[MUT_SHORT_STRING_LEN];
    char passed[MUT_SHORT_STRING_LEN];
    char notexecuted[MUT_SHORT_STRING_LEN];
    char test_index_str[MUT_SHORT_STRING_LEN];
} mut_summary_t;



// statics from mut_log.cpp
extern const char* mut_format_class[];
extern char console_suite_summary[];
extern  char mut_format_spec_txt[mut_formatitem_Size][MUT_STRING_LEN];
extern  char mut_filename_format_default[];
extern const char mut_textformat_filename_default[];



class Mut_log_control {
public:
    char text_log_file[EMCUTIL_MAX_PATH];	/**< Log file name */
    char  xml_log_file[EMCUTIL_MAX_PATH];	/**< Log file name */
	char  dump_file_name[EMCUTIL_MAX_PATH];	/**< dump file name */
    FILE *text_file;				/**< Text log file descriptor */
    FILE *xml_file;					/**< XML log file descriptor */
    const char *section;
    Mut_log_statistics statistics;

    Mut_log_control();
   
};


class Mut_log {
public:
    Mut_log();

    void registerListener(Mut_abstract_log_listener *listener);

private:

    Mut_abstract_log_listener *mListener;

    void close_log_console(void);
    void close_log_file(void);
    void create_log_console(void);
    void create_log_file(void);
    void mut_init_log_name(const char *name);
    void mut_log_transfer(unsigned destinationMask, const char *con, const char *txt, const char *xml);
    const char* mut_spaces(size_t actual, size_t desired);

    /** \fn const char * escape_char(char c)
     *  \param c - character to check
     *   \return null-terminated string representing special XML
     *       character c, or NULL if c is not special character in
     *       XML
    **/ 
    const char * escape_char(char c);
    /** \fn void void handle_special_characters(const char *in, char *out)
     *  \param in - input character buffer
     *  \param size - size of out in characters
     *  \param out - output character buffer
     *  \details
     *  this function changes symbols that can not be present in xml in literal
     *  form to special symbol sequencies.
     */
    void handle_special_characters(char *out, int size, const char *in);
    void charcat(char *s, char c);
    unsigned mut_is_digit(char c);
    void mut_log_format_prepare_prim(Mut_log_entry_coll *coll, const char *const fieldspec[], unsigned fieldspec_size, const char *formatspec);
    void mut_log_format_prepare(mut_log_format_item_t format_ndx, const char *kind, const char *const fieldspec[], const char *const formatspec_txt, const char *const formatspec_xml);

    void mut_log_flex_write_prim(char *buf, int filter_level, int escape_characters, mut_logging_t forsed_level, const char **args, Mut_log_entry_coll *coll);
    
    /** \fn void mut_log_flex_write_single(char *buffer, mut_log_format_item_t format_ndx, unsigned form_ndx, ...)
     *  \param level - overides the verbosity level, -1 to keep the level defined by the format
     *  \param format_ndx - format specification index
     *  \param ... - string parameters for formatted output
     */ 
    void mut_log_flex_write_single(char *buffer, int filter_level, mut_log_format_item_t format_ndx, unsigned form_ndx, ...);
    void mut_format_initialization(void);
    
    void mut_format_load(void);
    void mut_remove_nl(char *s);

    /** \fn int mut_log_file(mut_logging_t level, const char* txt,
     *      const char* xml)
     *  \param txt - text representation of log entry.
     *  \param xml - XML representation of log entry.
     *  \details
     *  Provides output text or XML to a fileconsole according with loggong type
     */
    void mut_log_file(const char* txt, const char* xml);

    static void mut_log_csx_output_handler(
            csx_ci_output_context_t output_context,
            csx_ci_output_type_t output_type,
            csx_ci_output_level_t output_level,
            csx_cstring_t header,
            csx_cstring_t format,
            va_list ap)
    {
        CSX_CALL_CONTEXT;
        csx_string_t output_buffer;
        csx_bool_t elevated;
        csx_nsint_t buffer_idx;
        csx_size_t buffer_size;

        CSX_CALL_ENTER(); // all kinds of exotic low-level stuff can flow to here - so lets make sure we are a CSX thread

        CSX_UNREFERENCED_PARAMETER(output_context);

        if (CSX_IS_FALSE(csx_p_thr_os_blockable())) {
            elevated = CSX_TRUE;
            csx_rt_io_temp_print_buffer_build(&buffer_idx, &output_buffer, &buffer_size, CSX_NULL, format, ap);
        } else {
            elevated = CSX_FALSE;
            output_buffer = csx_rt_mx_strbuildv(format, ap);
        }

        if (output_buffer != NULL)
        {
            switch (output_type) {
                case CSX_CI_OUTPUT_TYPE_ASSERT:
                case CSX_CI_OUTPUT_TYPE_ODDITY:
                    /* if we're panicing this is hard assert output so make sure it is treated as standard mut assert output */
                    if (csx_rt_assert_get_is_panic_in_progress()) {
                        mut_printf(MUT_LOG_NO_CHANGE, "%s%s", header, output_buffer);
                    } else {
                        mut_printf(MUT_LOG_LOW, "%s%s", header, output_buffer);
                    }
                    /* fall through */
                case CSX_CI_OUTPUT_TYPE_NATIVE:
                case CSX_CI_OUTPUT_TYPE_STD:
                case CSX_CI_OUTPUT_TYPE_DEBUG:
                case CSX_CI_OUTPUT_TYPE_TRACE:
                default:
                    KvTrace("%s:%s", header, output_buffer);
                    break;
            }
            if (CSX_IS_TRUE(elevated)) {
                csx_rt_io_temp_print_buffer_free(buffer_idx);
            } else {
                csx_rt_mx_strfree(output_buffer);
            }
        }
        CSX_CALL_LEAVE();
    }

    void time2str(char *buf);
    
    void mut_fill_format(char* buffer, const char* format, const char* suite, const char* extension, 
                         int year, int month, int day, int hours, int minutes, int seconds);
    unsigned mut_file_format_token(char* buffer, const char** f, int solid, const char* pattern, const char* value_s, int value_i);
    bool mut_str_comp(const char *v, const char *p);
public:
    
    Mut_log_control log_control;
   //options
    mut_log_level_option log_level;
    mut_logdir_option logdir;
    mut_filename_option filename;
    mut_text_option text;
    mut_xml_option xml;
    mut_disableConsole_option disableConsole;
    mut_formatfile_option formatfile;
    mut_cli_init_formatfile_option cli_init_formatfile;
    mut_color_output_option color_output;

    /** \fn static int mut_log_console(mut_logging_t level, const char* txt, const char* xml)
     *  \param txt - text representation of log entry.
     *  \details
     *  Provides output text representation to console (XML output TODO)
     */
    void mut_log_console(const char* txt);

    void mut_log_text_file(const char *buffer);
    void mut_log_xml_file(const char *buffer);

    enum mut_log_type_e get_log_type();

    void mut_log_assert(const char* file, unsigned line, const char* func, const char *fmt, ...) __attribute__((format(__printf_func__,5,6)));   
    bool mut_has_any_assert_failed();

    void mut_log_flex_write(unsigned destinationMask, mut_logging_t level, mut_log_format_item_t format_ndx, ...);
    /** \fn void mut_report_test_started(Mut_testsuite *suite, Mut_test_entry *test)
     *  \param suite - suite test belongs to
     *  \param test - pointer to teh test_entry_t structure
     *  \param value - string to be used as test status
     *  \details
     *  This function is used to report status of test execution.
     */
    void mut_report_test_started(Mut_testsuite *suite, Mut_test_entry *test);
    
    /** \fn void mut_report_test_finished(Mut_testsuite *suite, Mut_test_entry *test, char *value)
     *  \param suite - suite test belongs to
     *  \param test - pointer to teh test_entry_t structure
     *  \param value - string to be used as test status
     *  \details
     *  This function is used to report status of test execution.
     */
    void mut_report_test_finished(Mut_testsuite *suite, Mut_test_entry *test, const char *value, bool updateLog = TRUE);
    
	/** \fn void mut_report_test_timeout(Mut_testsuite *suite, Mut_test_entry *test, char *value)
     *  \param suite - suite test belongs to
     *  \param test - pointer to teh test_entry_t structure
     *  \param value - string to be used as test status
     *  \details
     *  This function is used to report timeout occurance in test.
     */
    void mut_report_test_timeout(Mut_testsuite *suite, Mut_test_entry *test, const char *value, bool updateLog = TRUE);
    
    void mut_report_test_notexecuted(Mut_testsuite *suite, Mut_test_entry *test);
    
    /** \fn void mut_report_testsuite_starting(const char *name)
     *  \param name - testsuite name
     *  \details
     *  This function prints testsuite starting string to the log.
     */
    void mut_report_testsuite_starting(const char *name, int current_iteration, int total_iteration);
    
    /** \fn void mut_report_testsuite_finished(const char *name, mut_testsuite_t *suite)
     *  \param name - testsuite name
     *  \details
     *  This function prints testsuite finished string to the log.
     */
    void mut_report_testsuite_finished(const char *name, mut_testsuite_t *suite);
    
    /** \fn void mut_convert_to_xml(char *xml, const char *format_string, ...)
     *  \param xml - holds the conversion result
     *  \param format_string - format string like in printf
     *  \param ... - optional orguments like in printf
     *  \details
     *  This function is used to convert log message to xml format.
     */
    void mut_convert_to_xml(char *xml, const char *format_string, ...) __attribute__((format(__printf_func__,3,4)));
    
    void ReportIncompleteTests(Mut_testsuite *suite, Mut_test_entry *test);
    /** \fn void create_log(const char *name)
     *  \param name - log file name
     *  \details
     *  This function starts log with given name.
     */
    void create_log(const char *name);
    
    /** \fn void close_log()
     *  \details
     *  Closes log file.
     */
    void close_log();
    
    /** \fn void print_log_level_options(void)
     *  \details
     *  This function prints log level specific help.
     */
    void print_log_level_options(void);
    
    /** \fn void mut_log_init(void)
     *  \details logging subsystem initialization
     */
    void mut_log_init(void);
    
    /** \fn void mut_log_prepare(void)
     *  \details logging subsystem initialization
     */
    void mut_log_prepare(void);
    
    
    /** \fn void mut_internal_critical_section_begin(void)
     *  \details MUT internal critical section begin
     */
    void mut_internal_critical_section_begin(void);
    
    /** \fn void mut_internal_critical_section_end(void)
    *  \details MUT internal critical section end
     */
    void mut_internal_critical_section_end(void);
    
    /** \fn void mut_report_test_summary(const char *name, mut_testsuite_t *suite, int iteration)
     *  \param name - testsuite name
     *  \param suite - pointer to the mut_testsuite_t structure 
     *  \param iteration - number of iterations being done
     *  \details
     *  This function prints testsuite finished string to the log.
     */
    void mut_report_test_summary(const char *name, mut_testsuite_t *suite, int iteration);
    
    /** \fn char *get_log_file_name()
     *  \details
     *  Return the log file name MUT is writing to.
     */

    char *get_log_file_name() {
        return (get_log_type() == MUT_LOG_XML)
                ? log_control.xml_log_file
                : log_control.text_log_file;
    }

    char *get_dump_file_name() {
        return log_control.dump_file_name;
    }

    char *mut_get_logdir() {
        return logdir.get();  // stored in an option
    }

    bool mut_is_color_output() {
        return color_output.get();
    }
};



#endif
