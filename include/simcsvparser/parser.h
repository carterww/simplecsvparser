#ifndef PARSER_H
#define PARSER_H

#include <wctype.h>
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#define ENCODING_STRING_LENGTH 10

typedef struct {
    char delimiter;
    char encoding[ENCODING_STRING_LENGTH];
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
    char encoding[ENCODING_STRING_LENGTH];
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

/* Sets the default parser options for the wide char parser.
 * The user can then modify them as needed or not use this at all.
 * This variant should be used when the user wants to use UTF-8
 * encoding with non-ASCII characters.
 */
int w_set_default_parser_options(w_parser_options *options);

/* Frees the memory allocated for a table, but not the table itself */
void free_table(table *table);
/* Parser for CSV files
 * Returns 0 on success, some other value on failure
 * @param csv_path: The path to the CSV file.
 * @param options: The options for the parser.
 * @param result: A pointer to a table struct that will be filled with the
 * parsed CSV data.
 */
int csv_parse(const char *csv_path, parser_options options, table *result);

#ifdef __cplusplus
}
#endif

#endif /* PARSER_H */
