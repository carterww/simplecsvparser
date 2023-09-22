#include "include/simcsvparser/parser.h" /* I will fix this later */
#include "include/simcsvparser/options.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 128

int set_default_parser_options(parser_options *options) {
    options->delimiter = DELIMITER_COMMA;
    options->encoding = ENCODING_ASCII;
    options->iltst = true;
    options->uniform_column_count = true;
    options->data_start_line = 0;
    return 0;
}

int w_set_default_parser_options(w_parser_options *options) {
    options->delimiter = W_DELIMITER_COMMA;
    options->encoding = ENCODING_UTF8;
    options->iltst = true;
    options->uniform_column_count = true;
    options->data_start_line = 0;
    return 0;
}

/* For reference
   typedef struct {
   char delimiter;
   char *encoding;
   bool iltst;
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
   */
int csv_parse(const char *csv_path, parser_options options, table *result) {
    int curr_row_num = 0;
    int bufflen = 0;
    char buffer[BUFFER_SIZE];
    char *carry; /* Temporary string used before writing to the struct */
    char c = 0;
    bool inside_literal = 0;

    memset(buffer, 0, BUFFER_SIZE * sizeof(char));

    FILE *csv = fopen(csv_path, "r");
    if (csv == NULL)
        return errno;

    while(1) {
        c = fgetc(csv);
        if (feof(csv))
            goto cleanup;
        
        switch (c) {
            case '\n':
                curr_row_num++;
                //flush_buff(); this shoudl zero the buff's memory
                bufflen = 0;
                break;
        }
    }

error:
    fclose(csv);
    return -1;
cleanup:
    fclose(csv);
    return 0;
}
