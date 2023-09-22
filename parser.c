#include "include/simcsvparser/parser.h" /* I will fix this later */
#include "include/simcsvparser/options.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 128

#define INSIDE_LITERAL 0x1
#define OUTSIDE_LITERAL 0x0

#define REACHED_QUOTE_IN_LITERAL 0x2

/* Private function declarations */

/* Responsible for flushing the buffer into carry and resetting the buffer */
static void flush_buffer(char *buff, unsigned int *bufflen, char **carry);
/* Responsible for parsing a literal and returning the string */
static char *handle_literal(FILE *csv, char *buff, char *after_literal);
/* Allocation helpers */


/* Responsible for allocating memory for the initial table */
static void alloc_table(table *table);
/* We use field count here because when uniform column count is true,
 * we can know how many fields we need to allocate for each row.
 * This allows us to only realloc first row then next rows can use it's size
 */
static void alloc_row(row *row, unsigned int field_count);
/* Responsible for reallocating memory for the table by adding one row.
 * I may change the number of rows we allocate for later to be more efficient
 * but only after I've done some tests on it.
 */
static void realloc_table(table *table);
/* Responsible for reallocating memory for the row by adding one field.
 * This one I will probably change to be more efficient later. These
 * memcpys are going to be expensive.
 */
static void relloc_row(row *row);

int set_default_parser_options(parser_options *options) {
    options->delimiter = DELIMITER_COMMA;
    strcpy(options->encoding, ENCODING_ASCII);
    options->iltst = true;
    options->uniform_column_count = true;
    options->data_start_line = 0;
    return 0;
}

int w_set_default_parser_options(w_parser_options *options) {
    options->delimiter = W_DELIMITER_COMMA;
    strcpy(options->encoding, ENCODING_UTF8);
    options->iltst = true;
    options->uniform_column_count = true;
    options->data_start_line = 0;
    return 0;
}

void free_table(table *table) {
    for (int i = 0; i < table->row_count; i++) {
        for (int j = 0; j < table->rows[i].field_count; j++) {
            free(table->rows[i].fields[j]);
        }
        free(table->rows[i].fields);
    }
    free(table->rows);
}

int csv_parse(const char *csv_path, parser_options options, table *result) {
    unsigned int curr_row_num = 0;
    unsigned int bufflen = 0;
    char buffer[BUFFER_SIZE];
    char *carry = NULL; /* Temporary string used before writing to the struct */
    char c = 0;

    memset(buffer, 0, BUFFER_SIZE * sizeof(char));

    FILE *csv = fopen(csv_path, "r");
    if (csv == NULL)
        return errno;
    
    /* Loop to get to the data */
    while(curr_row_num < options.data_start_line) {
        c = fgetc(csv);
        if (feof(csv))
            goto cleanup;
        /* Data hasn't started yet, so keep going */
        if (c == '\"')
            if (handle_literal(csv, buffer, &c) == NULL)
                goto error;
        if (c == '\n')
            curr_row_num++;
    }

    /* We ate char on data row last loop, so go back one */
    if (c != 0)
        fseek(csv, -1L, SEEK_CUR);

    alloc_table(result);
    alloc_row(&(result->rows[0]), 1);

    unsigned int curr_field_count = 0;
    curr_row_num = 0;
    bool reached_data = false;
    while (1) {
main_loop:
        c = fgetc(csv);
        if (feof(csv))
            goto cleanup;
        
        if (c == '\"') {
            carry = handle_literal(csv, buffer, &c);
            if (carry == NULL)
                goto error;
            result->rows[curr_row_num].fields[curr_field_count++] = carry;
            carry = NULL;

            if (curr_row_num == 0 || !(options.uniform_column_count))
                relloc_row(&(result->rows[curr_row_num]));
            goto main_loop;
        }

        if (c == '\n') {
            if (!reached_data)
                goto main_loop;
            flush_buffer(buffer, &bufflen, &carry);

            /* Don't increment curr_field_count because we started with one more than we actually had */
            result->rows[curr_row_num].fields[curr_field_count++] = carry;
            carry = NULL;
            result->rows[curr_row_num].field_count = curr_field_count;

            realloc_table(result);
            if (options.uniform_column_count)
                alloc_row(&(result->rows[++curr_row_num]), result->rows[0].field_count);
            else {
                alloc_row(&(result->rows[++curr_row_num]), 1);
            }
            curr_field_count = 0;
            reached_data = false;
            goto main_loop;
        }
        
        if (bufflen == BUFFER_SIZE) {
            flush_buffer(buffer, &bufflen, &carry);
        }

        if (c == options.delimiter) {
            flush_buffer(buffer, &bufflen, &carry);
            result->rows[curr_row_num].fields[curr_field_count++] = carry;
            carry = NULL;
            if (curr_row_num == 0 || !(options.uniform_column_count))
                relloc_row(&(result->rows[curr_row_num]));
            reached_data = false;
        } else if (options.iltst && !reached_data && (c == ' ' || c == '\t')) {
            goto main_loop;
        } else {
            reached_data = true;
            buffer[bufflen++] = c;
        }

    }

error:
    fclose(csv);
    return -1;
cleanup:
    fclose(csv);
    result->row_count = curr_row_num;
    return 0;
}

static char *handle_literal(FILE *csv, char *buff, char *after_literal) {
    bool reached_quote_in_literal = false;
    char c = '\"';
    char *carry = NULL;
    unsigned int bufflen = 0;

    while (1) {
        c = fgetc(csv);
        if (feof(csv))
            return NULL;
        if (bufflen == BUFFER_SIZE) {
            flush_buffer(buff, &bufflen, &carry);
        }
        if (c == '\"') {
            if (!reached_quote_in_literal) {
                reached_quote_in_literal = true;
            } else {
                reached_quote_in_literal = false;
                buff[bufflen++] = c;
            }
        } else {
            if (reached_quote_in_literal) {
                flush_buffer(buff, &bufflen, &carry);
                *after_literal = c; /* We char after literal, so main function will need it */
                return carry;
            }
            buff[bufflen++] = c;
        }

    }
}

static void flush_buffer(char *buff, unsigned int *bufflen, char **carry) {
    /* If NULL, then this is the first time we are flushing the buffer */
    if (*carry == NULL) {
        *carry = calloc(*bufflen + 1, sizeof(char));
        memcpy(*carry, buff, *bufflen * sizeof(char));
    /* We have flushed the buffer into carry before, so we need to realloc
     * by adding bufflen to the current size of carry */
    } else {
        *carry = realloc(*carry, (strlen(*carry) + *bufflen + 1) * sizeof(char));
        strcat(*carry, buff);
    }
    memset(buff, 0, *bufflen * sizeof(char));
    *bufflen = 0;
}

static void alloc_table(table *table) {
    table->row_count = 1; /* We start with one row */
    table->rows = calloc(1, sizeof(row));
}

static void alloc_row(row *row, unsigned int field_count) {
    row->field_count = field_count; /* We start with one field */
    row->fields = calloc(field_count, sizeof(char *));
}

static void realloc_table(table *table) {
    table->row_count++;
    table->rows = realloc(table->rows, table->row_count * sizeof(row));
}

static void relloc_row(row *row) {
    row->field_count++;
    row->fields = realloc(row->fields, row->field_count * sizeof(char *));
}
