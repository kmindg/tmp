#if !defined(STROPT_H)
#define STROPT_H
/*
 * strOpt.h
 *
 * DESCRIPTION: This is the header file that describes the interface
 *   to the strOpt() function.
 *
 * HISTORY
 *   4-May-2000   JFW -- initial version
 *  29-Jun-2000CE JFW -- added ability to have '--' as an option
 *
 */

/*
 * strOpt_ret_T
 *
 * DESCRIPTION:  This enum defines the "error" return values from
 *   the strOpt() function.
 *
 * ERRORS:
 *  strOpt_EOL -- found no more options when scanning the command string
 *  strOpt_UNK -- option found in command line that does not appear in the
 *                option string.
 *  strOpt_VAL -- option found in the command line that is not followed by
 *                a value string, but the option string indicated that the
 *                option requires a value.
 *
 * NOTE:  This implementation does NOT permute the command line looking
 *        for options.  It just scans the command line beginning to end
 *        and stops at the first non-option token found.
 */
/* INCLUDES */
#include "cdecl.h"
#include "csx_ext.h"

typedef enum strOpt_ret_enum
{
  strOpt_EOL = -1,
  strOpt_UNK = '?',
  strOpt_VAL = ':',
}
strOpt_ret_T;

/*
 * NULL -- no value associated with returned option
 * else -- if strOpt_EOL not returned, value associated with returned option
 *      -- if strOpt_EOL is returned, remainder of the command string
 */
CSX_EXTERN char    *stroptarg;

/*
 * if strOpt_EOL not returned, the option letter found in the command string
 * if strOpt_EOL is returned, NUL ('\000') character.
 *
 */
CSX_EXTERN_C char stroptopt;

/*
 * int strOpt (const char *const argPA, const char *const optstringPA)
 *
 * DESCRIPTION: This function provides getopt() functionality for parsing
 *   a command line supplied as a single string rather than the argc, argv
 *   form.  It uses different global names so that it can be used in the
 *   same program that also uses the getopt() function.
 *
 *   It allows the following characters in the optstringPA argument:
 *
 *   optstringPA[0] '-' unsupported, ignored
 *   optstringPA[0] '+' unsupported, ignored
 *   optstringPA[n] 'x' option letter (any printable character other than
 *                                     ':', '?', space, HT)
 *   optstringPA[n+1] ':' [optstringPA[n+2] ':']
 *   An option letter can be followed by 1 or 2 colons.  If it is followed
 *   by 1 colon, then the option takes a required value.  If is is followed
 *   by 2 colons, then the option takes an optional value.
 *
 *   option string syntax:
 *
 *   <OPTION_STRING> :== [ <permute> ] <opt_tokens>
 *   <permute>       :== '+'
 *                   |   '-'
 *   <opt_tokens>    :== <opt_token> [ <opt_tokens> ]
 *   <opt_token>     :== <simple_opt>
 *                   |   <req_val_opt>
 *                   |   <opt_val_opt>
 *   <simple_opt>    :== <opt_char>
 *   <req_val_opt>   :== <opt_char> ':'
 *   <opt_val_opt>   :== <opt_char> ':' ':'
 *   <opt_char>      :== printable except '\040', '\t', and ':'
 *
 *   command line syntax:
 *
 *   <COMMAND_LINE>  :== [ <WSP> ] [ <cmd_items> ] [ <WSP> ] <EOS>
 *   <WSP>           :== <wsp_char> [ <WSP> ]
 *   <cmd_items>     :== <cmd_item> [ <WSP> ] [ <cmd_items> ]
 *   <cmd_item>      :== <option>
 *                   |   <option> [ <WSP> ]  <value>
 *                   |   <non_option>
 *   <option>        :== '-' <opt_char>
 *   <value>         :== <opt_chars>
 *   <non_option>    :== <opt_chars>
 *   <opt_chars>     :== <opt_char> [ <opt_chars> ]
 *
 *   <EOS>           :== '\000'
 *   <wsp_chars>     :== '\040'
 *                   |   '\t'
 *   <opt_char>      :== printable except '\040', '\t', and ':'
 *

 * PARAMETERS
 *   argPA       -- [R][*R] command string to scan
 *   optstringPA -- [R][*R] option string that drives parsing
 *
 * GLOBALS
 *   stroptopt   -- [W] option letter.  If last char in command line is
 *                      '-', stroptopt is set to EOS and '?' is returned.
 *                      if '?' is in the optstring and "-?" is found
 *                      in the command string, stroptopt is set to '?' and
 *                      '?' is returned.
 *   stroptarg   -- [W] address of option value or NULL.  A non-NULL value
 *                      is the address of an internal static buffer that will
 *                      have undefined contents at the next call to strOpt().
 *                      if strOpt_EOL is returned, address of remainder of
 *                      the command line.
 *
 * CALLS: nothing.
 *
 * RETURNS
 *   option letter
 *   strOpt_UNK '?' if unknown option letter
 *   strOpt_VAL ':' if option with required, but missing value
 *   strOpt_EOL -1  if no more options found.
 *
 * ERRORS: as described above
 *
 * NOTES: standalone '-' and '--' functionality is not implemented.
 *        '--' can be processed by adding '-' to any but the first
 *        character of stroptstr and processing the '-' option.
 *
 */
CSX_EXTERN_C int   CDECL_    strOpt (const char *const argPA,
			const char *const optstringPA);

#endif /* !defined(STROPT_H) */
