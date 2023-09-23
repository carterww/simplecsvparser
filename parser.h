#ifndef SIMCSV_PARSER_H
#define SIMCSV_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef struct {
    char delimiter;
    bool iltst; /* Ignore leading and trailing spaces and tabs */
    bool uniform_column_count;
    unsigned int data_start_line;
    unsigned int row_growth;
    unsigned int field_growth;
} parser_options;

typedef struct {
    char **fields;
    unsigned int field_count;
} row;

typedef struct {
    row *rows;
    unsigned int row_count;
} table;

/* Sets the default parser options for the parser.
 * The user can then modify them as needed or not use this at all
 */
int set_default_parser_options(parser_options *options);

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

#endif /* SIMCSV_PARSER_H */
