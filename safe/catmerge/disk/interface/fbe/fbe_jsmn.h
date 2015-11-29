#ifndef FBE_JSMN_H
#define FBE_JSMN_H

/*!**************************************************************************
 * @file fbe_jsmn.h
 ***************************************************************************
 *
 * @brief
 *  This file is header file for paring the json file into tokens.
 *  It is from the open source code jsmn in C from http://www.json.org/
 *  jsmn is a minimalistic library for parsing JSON data format.
 * 
 * @revision
 *   12-Sep-2014 PHE - Created. 
 *
 ***************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  JSON type identifier. Basic types are:
 * 	o Object
 * 	o Array
 * 	o String
 * 	o Other primitive: number, boolean (true/false) or null
 */
typedef enum {
    JSMN_PRIMITIVE = 0,
    JSMN_OBJECT = 1,
    JSMN_ARRAY = 2,
    JSMN_STRING = 3
} jsmn_type_t;

typedef enum {
    /* no error */
    JSMN_ERROR_NONE = 0,
    /* Not enough tokens were provided */
    JSMN_ERROR_NOMEM,
    /* Invalid character inside JSON string */
    JSMN_ERROR_INVAL,
    /* The string is not a full JSON packet, more bytes expected */
    JSMN_ERROR_PART,
} jsmn_err_t;


/**
 * JSON token description.
 * @param		type	type (object, array, string etc.)
 * @param		start	start position in JSON data string
 * @param		end		end position in JSON data string
 * @param       size    the number of child (nested) tokens
 */

typedef struct {
    jsmn_type_t type;
    int start;
    int end;
    int size;
#ifdef JSMN_PARENT_LINKS
    int parent;
#endif
} jsmn_tok_t;

/**
 * JSON parser. Contains an array of token blocks available. Also stores
 * the string being parsed now and current position in that string
 */
typedef struct {
    unsigned int pos; /* offset in the JSON string */
    unsigned int toknext; /* next token to allocate */
    int toksuper; /* superior token node, e.g parent object or array */
} jsmn_parser_t;

/**
 * Create JSON parser over an array of tokens
 */
void jsmn_init(jsmn_parser_t *parser);

/**
 * Run JSON parser. It parses a JSON data string into and array of tokens, each describing
 * a single JSON object.
 */
jsmn_err_t jsmn_parse(jsmn_parser_t *parser, 
                      const char *js, 
                      size_t len,
                      jsmn_tok_t *tokens, 
                      unsigned int num_tokens, 
                      unsigned int * token_count);

#ifdef __cplusplus
}
#endif

#endif /* FBE_JSMN_H */


