#include "w_parser.h"
#include "options.h"

#include <string.h>

int w_set_default_parser_options(w_parser_options *options) {
    if (options == NULL)
        return -1;
    options->delimiter = W_DELIMITER_COMMA;
    options->iltst = true;
    options->uniform_column_count = true;
    options->data_start_line = 0;
    options->row_growth = ROW_GROWTH;
    options->field_growth = FIELD_GROWTH;
    return 0;
}

