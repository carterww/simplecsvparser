#ifndef PARSER_H
#define PARSER_H

#include <wctype.h>
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef struct {
    char delimiter;
    char *encoding;
    bool iltst; /* Ignore leading and trailing spaces and tabs */
    bool uniform_column_count;
    unsigned int data_start_line;
} parser_options;

typedef struct {
    char **fields;
    unsigned int field_count;
} row;

typedef struct {
    row *rows;
    unsigned int row_count;
} table;

/* Wide options
 * I don't see a simple way to abstract these into one set of structs
 * Maybe there's a way with macros? I'll look into it later.
 */
typedef struct {
    wint_t delimiter;
    char *encoding;
    bool iltst; /* Ignore leading and trailing spaces and tabs */
    bool uniform_column_count;
    unsigned int data_start_line;
} w_parser_options;

typedef struct {
    wint_t **fields;
    unsigned int field_count;
} w_row;

typedef struct {
    w_row *rows;
    unsigned int row_count;
} w_table;

/* Sets the default parser options for the parser.
 * The user can then modify them as needed or not use this at all
 */
int set_default_parser_options(parser_options *options);
int w_set_default_parser_options(w_parser_options *options);



#ifdef __cplusplus
}
#endif

#endif /* PARSER_H */
