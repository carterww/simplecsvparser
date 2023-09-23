#include "parser.h"
#include "options.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Private function declarations */

/* Responsible for flushing the buffer into carry and resetting the buffer */
static void flush_buffer(char *buff, unsigned int *bufflen, char **carry);
/* Responsible for parsing a literal and returning the string */
static char *handle_literal(FILE *csv, char *buff, char *after_literal);

/* Allocation helpers */

/* Responsible for allocating memory for the initial table */
static int alloc_table(table *table);
/* We use field count here because when uniform column count is true,
 * we can know how many fields we need to allocate for each row.
 * This allows us to only realloc first row then next rows can use it's size
 */
static int alloc_row(row *row, unsigned int field_count);
/* Responsible for reallocating memory for the table by adding one row.
 * I may change the number of rows we allocate for later to be more efficient
 * but only after I've done some tests on it.
 */
static int realloc_table(table *table, unsigned int grow);
/* Responsible for reallocating memory for the row by adding one field.
 * This one I will probably change to be more efficient later. These
 * memcpys are going to be expensive.
 */
static int relloc_row(row *row, unsigned int grow);
/* We may overallocate memory for the table, so this function will
 * reallocate the memory to fit the final table.
 */
static int fit_table(table *table, unsigned int row_count);
/* We may overallocate memory for the row, so this function will
 * reallocate the memory to fit the row to the final size.
 */
static int fit_fields(row *row, unsigned int field_count);

int set_default_parser_options(parser_options *options) {
    if (options == NULL)
        return -1;
    options->delimiter = DELIMITER_COMMA;
    options->iltst = true;
    options->uniform_column_count = true;
    options->data_start_line = 0;
    options->row_growth = ROW_GROWTH;
    options->field_growth = FIELD_GROWTH;
    return 0;
}

void free_table(table *table) {
    for (int i = 0; i < table->row_count; i++) {
        for (int j = 0; j < table->rows[i].field_count; j++) {
            if (table->rows[i].fields[j] != NULL)
                free(table->rows[i].fields[j]);
        }
        if (table->rows[i].fields != NULL)
            free(table->rows[i].fields);
    }
    if (table->rows != NULL)
        free(table->rows);
}

int csv_parse(const char *csv_path, parser_options options, table *result) {
    if (csv_path == NULL || result == NULL)
        return -1;
    FILE *csv = fopen(csv_path, "r");
    if (csv == NULL)
        return errno;
    
    unsigned int curr_row_num = 0;
    unsigned int bufflen = 0;
    char buffer[BUFFER_SIZE];
    char c = 0;
    memset(buffer, 0, BUFFER_SIZE * sizeof(char));
    /* Loop to get to the data */
    while(curr_row_num < options.data_start_line) {
        c = fgetc(csv);
        if (feof(csv))
            goto cleanup;
        /* Data hasn't started yet, so keep going */
        if (c == '\"') {
            char *carry = handle_literal(csv, buffer, &c);
            if (carry == NULL)
                goto error;
            free(carry);
        }
        if (c == '\n')
            curr_row_num++;
    }

    /* We ate char on data row last loop, so go back one */
    if (c != 0)
        fseek(csv, -1L, SEEK_CUR);

    if (alloc_table(result) != 0)
        goto error;
    if (alloc_row(&(result->rows[0]), 1) != 0)
        goto error;

    unsigned int curr_field_count = 0;
    char *carry = NULL; /* Temporary string used before writing to the struct */
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

            if (curr_field_count >= result->rows[curr_row_num].field_count &&
                    (curr_row_num == 0 || !(options.uniform_column_count)))
                if (relloc_row(&(result->rows[curr_row_num]), options.field_growth) != 0)
                    goto error;
            goto main_loop;
        }
        if (c == '\r') /* Handle windows line endings */
            continue;
        if (c == '\n') {
            if (!reached_data)
                goto main_loop;
            flush_buffer(buffer, &bufflen, &carry);

            result->rows[curr_row_num].fields[curr_field_count++] = carry;
            carry = NULL;
            if (fit_fields(&(result->rows[curr_row_num]), curr_field_count) != 0)
                goto error;
            result->rows[curr_row_num].field_count = curr_field_count;

            if (curr_row_num + 1 >= result->row_count) {
                if (realloc_table(result, options.row_growth) != 0)
                    goto error;
            }
            if (options.uniform_column_count) {
                if (alloc_row(&(result->rows[++curr_row_num]), result->rows[0].field_count) != 0)
                    goto error;
            } else {
                if (alloc_row(&(result->rows[++curr_row_num]), 1) != 0)
                    goto error;
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
            if (curr_field_count >= result->rows[curr_row_num].field_count &&
                    (curr_row_num == 0 || !(options.uniform_column_count))) {
                if (relloc_row(&(result->rows[curr_row_num]), options.field_growth) != 0)
                    goto error;
            }
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
    free_table(result);
    return -1;
cleanup:
    fclose(csv);
    if (result->rows[curr_row_num].fields != NULL)
        free(result->rows[curr_row_num].fields); /* This was causing a leak */
    fit_table(result, curr_row_num);
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
            continue;
        } else if (reached_quote_in_literal) {
            flush_buffer(buff, &bufflen, &carry);
            *after_literal = c; /* We char after literal, so main function will need it */
            return carry;
        }
        buff[bufflen++] = c;

    }
}

static void flush_buffer(char *buff, unsigned int *bufflen, char **carry) {
    /* If NULL, then this is the first time we are flushing the buffer */
    if (*carry == NULL) {
        *carry = calloc(*bufflen + 1, sizeof(char));
        memcpy(*carry, buff, *bufflen);
    /* We have flushed the buffer into carry before, so we need to realloc
     * by adding bufflen to the current size of carry */
    } else {
        *carry = realloc(*carry, strlen(*carry) + *bufflen + 1);
        strcat(*carry, buff);
    }
    memset(buff, 0, *bufflen);
    *bufflen = 0;
}

static int alloc_table(table *table) {
    row *ptr = calloc(1, sizeof(row));
    if (ptr != NULL) {
        table->rows = ptr;
        table->row_count = 1;
        return 0;
    }
    return -1;
}

static int alloc_row(row *row, unsigned int field_count) {
    char **ptr = calloc(field_count, sizeof(char *));
    if (ptr != NULL) {
        row->fields = ptr;
        row->field_count = field_count;
        return 0;
    }
    return -1;
}

static int realloc_table(table *table, unsigned int grow) {
    row *ptr = realloc(table->rows, (table->row_count + grow) * sizeof(row));
    if (ptr != NULL) {
        table->rows = ptr;
        table->row_count += grow;
        return 0;
    }
    return -1;
}

static int relloc_row(row *row, unsigned int grow) {
    char **ptr = realloc(row->fields, (row->field_count + grow) * sizeof(char *));
    if (ptr != NULL) {
        row->field_count += grow;
        row->fields = ptr;
        return 0;
    }
    return -1;
}

static int fit_fields(row *row, unsigned int field_count) {
    char **ptr = realloc(row->fields, field_count * sizeof(char *));
    if (ptr != NULL) {
        row->fields = ptr;
        return 0;
    }
    return -1;
}
 static int fit_table(table *table, unsigned int row_count) {
    row *ptr = realloc(table->rows, row_count * sizeof(row));
    if (ptr != NULL) {
        table->rows = ptr;
        return 0;
    }
    return -1;
 }
