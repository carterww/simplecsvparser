#ifndef SIMCSV_W_PARSER_H
#define SIMCSV_W_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <wctype.h>
#include <stdbool.h>

/* Wide options
 * I don't see a simple way to abstract these into one set of structs
 * Maybe there's a way with macros? I'll look into it later.
 */
typedef struct {
    wint_t delimiter;
    bool iltst; /* Ignore leading and trailing spaces and tabs */
    bool uniform_column_count;
    unsigned int data_start_line;
    unsigned int row_growth;
    unsigned int field_growth;
} w_parser_options;

typedef struct {
    wint_t **fields;
    unsigned int field_count;
} w_row;

typedef struct {
    w_row *rows;
    unsigned int row_count;
} w_table;

/* Sets the default parser options for the wide char parser.
 * The user can then modify them as needed or not use this at all.
 * This variant should be used when the user wants to use UTF-8
 * encoding with non-ASCII characters.
 */
int w_set_default_parser_options(w_parser_options *options);

#ifdef __cplusplus
}
#endif

#endif /* SIMCSV_W_PARSER_H */
