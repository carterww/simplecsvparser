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
static void flush_buffer(char *buff, int bufflen, char **carry);
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
    int curr_row_num = 0;
    int bufflen = 0;
    char buffer[BUFFER_SIZE];
    char *carry; /* Temporary string used before writing to the struct */
    char c = 0;

    memset(buffer, 0, BUFFER_SIZE * sizeof(char));

    FILE *csv = fopen(csv_path, "r");
    if (csv == NULL)
        return errno;
    
    printf("START: Parsing %s\n", csv_path);
    printf("Going to %d, before first while loop\n", options.data_start_line);
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
    printf("At %d, after first while loop\n", curr_row_num);

    /* We ate char on data row last loop, so go back one */
    if (c != 0)
        fseek(csv, -1L, SEEK_CUR);
    carry = NULL;
    curr_row_num = 0;

    printf("About to allocate table and first row\n");
    alloc_table(result);
    alloc_row(&(result->rows[0]), 1);
    printf("Allocated table and first row\n");

    int curr_field_count = 0;
    while (1) {
main_loop:
        c = fgetc(csv);
        if (feof(csv))
            goto cleanup;
        
        switch (c) {
            case '\"':
                carry = handle_literal(csv, buffer, &c);
                if (carry == NULL)
                    goto error;
                result->rows[curr_row_num].fields[curr_field_count++] = carry;
                /* If first row, then we need to realloc the row
                 * If not uniform column count, then we need to realloc the row
                 */
                if (curr_row_num == 0 || !(options.uniform_column_count))
                    relloc_row(&(result->rows[curr_row_num]));
                fseek(csv, -1L, SEEK_CUR);
                goto main_loop;
            case '\n':
                /* If we are at the end of the line, then we need to realloc the table
                 * and reset the field count
                 */
                flush_buffer(buffer, bufflen, &carry);
                /* Don't increment curr_field_count because we started with one more than we actually had */
                result->rows[curr_row_num].fields[curr_field_count++] = carry;
                result->rows[curr_row_num].field_count = curr_field_count;
                carry = NULL;
                bufflen = 0;
                realloc_table(result);
                if (options.uniform_column_count)
                    alloc_row(&(result->rows[curr_row_num + 1]), result->rows[0].field_count);
                else 
                    alloc_row(&(result->rows[curr_row_num + 1]), 1);
                printf("In newline: Current row: %d, Current field: %d\n", curr_row_num, curr_field_count);
                curr_field_count = 0;
                curr_row_num++;
                break;
        }
        if (bufflen == BUFFER_SIZE) {
            flush_buffer(buffer, bufflen, &carry);
            bufflen = 0;
        }
        if (c == options.delimiter) {
            printf("%d: In delimiter: %s\n", curr_field_count, buffer);
            flush_buffer(buffer, bufflen, &carry);
            bufflen = 0;
            result->rows[curr_row_num].fields[curr_field_count++] = carry;
            printf("%d: In delimiter: %s\n", curr_field_count, carry);
            carry = NULL;
            if (curr_row_num == 0 || !(options.uniform_column_count))
                relloc_row(&(result->rows[curr_row_num]));
        } else {
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
    bool state = INSIDE_LITERAL;
    char c = '\"';
    char *carry = NULL;
    unsigned int bufflen = 0;

    while (1) {
        c = fgetc(csv);
        if (feof(csv))
            return NULL;
        if (bufflen == BUFFER_SIZE) {
            flush_buffer(buff, bufflen, &carry);
            bufflen = 0;
        }
        if (c == '\"') {
            if (!(state & REACHED_QUOTE_IN_LITERAL)) {
                state |= REACHED_QUOTE_IN_LITERAL;
            } else if (state & REACHED_QUOTE_IN_LITERAL) {
                buff[bufflen++] = c;
                state &= ~REACHED_QUOTE_IN_LITERAL;
            }
        } else {
            if (state & REACHED_QUOTE_IN_LITERAL) {
                flush_buffer(buff, bufflen, &carry);
                *after_literal = c; /* We char after literal, so main function will need it */
                return carry;
            }
            buff[bufflen++] = c;
        }

    }
}

static void flush_buffer(char *buff, int bufflen, char **carry) {
    /* If NULL, then this is the first time we are flushing the buffer */
    if (*carry == NULL) {
        *carry = malloc(bufflen * sizeof(char));
        memcpy(*carry, buff, bufflen * sizeof(char));
    /* We have flushed the buffer into carry before, so we need to realloc
     * by adding bufflen to the current size of carry */
    } else {
        *carry = realloc(*carry, (strlen(*carry) + bufflen) * sizeof(char));
        strcat(*carry, buff);
    }
    memset(buff, 0, bufflen * sizeof(char));
}

static void alloc_table(table *table) {
    table->row_count = 1; /* We start with one row */
    table->rows = calloc(1, sizeof(row));
}

static void alloc_row(row *row, unsigned int field_count) {
    row->field_count = 1; /* We start with one field */
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
