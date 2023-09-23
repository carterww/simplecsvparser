# SimCSVParser
SimCSVParser is a simple parser written in C that I wrote to use for a project. The goal was to crate a simple interface for reading csv files with some options I like. Simplicity and portability are the main characteristics of the parser. It currently only supports 1 byte characters, but my plan is to add a w_char variant for reading UTF-8 beyond ASCII.

# Implementation
There are two variants of the parser. Each have a very similar interface and characteristics, one variant supports wide characters. I will use the parser.c variants throughout the doc.
    1. **parser.c**
    2. **w_parser.c**
        - Note: Not started yet.
There exists three main structures:
1. **table**: A structure to represent a collection of rows. It holds the pointer to the first row and the number of rows.
2. **row**: A structure to represent a row in a csv file. It holds the pointer to the first string and the number of strings.
3. parser_options: A structure that changes the behavior of the parser at runtime. This will be explained in it's [own section.](#parser-options)

There are three main functions. These will be explained in [another section.](#interface)
1. set_default_parser_options()
2. free_table()
3. csv_parse()

# Parser Options
Represented as a C struct:
```
typedef struct {
    char delimiter;
    bool iltst;
    bool uniform_column_count;
    unsigned int data_start_line;
    unsigned int row_growth;
    unsigned int field_growth;
} parser_options;
```
- `char delimeter = ',';`
    - Delimeter splitting the values in the csv file. 
- `bool iltst = true;`
    - Ignore leading and traling spaces and tabs.
- `bool uniform_column_count = true;`
    - Each row will have the same number of values. If true, memory allocation is more efficient.
- `unsigned int data_start_line = 0;`
    - Line the data starts on. 0 indexed.
- `unsigned int row_growth = ROW_GROWTH;`
    - Set the rate at which the parser allocates new rows when full. For example, ROW_GROWTH is set to 8 in options.h, so the table will allocate 8 new rows each time it fills. Tweaking this number can save many calls to realloc.
- `unsigned int field_growth = FIELD_GROWTH;`
    - Similar to row_growth except it indicates the rate at which the parser allocates new strings to the row. For example, FIELD_GROWTH is set to 4 in options.h, so the row will allocate 4 new char * when it fills. Tweaking this number can save many calls to realloc.
Setting row_growth and field_growth to good values will greatly increase performance. If uniform_column_count is true, field growth be ignored after the first row is read.

# Interface
- `int set_default_parser_options(parser_options *options);`
    - Initializes parser_options struct to the default values [indicated here.](#parser-options)
- `void free_table(table *table);`
    - Free the rows and fields allocated to the table.
- `int csv_parse(const char *csv_path, parser_options options, table *result);`
    - Parsing function that returns 0 on success and something else on error. It fills the table passed in as it reads from the csv. This function is probably thread safe (although I haven't tested it). By that I mean you could spawn multiple threads that call this function simultaneously to read different csvs.

# Notes
This has not been tested thoroughly. I just got it functional for my purposes and will update it/test it later. I'd like to add some options like allowing custom row implementations and such.
CSVs are not a standardized format, so I made some assumptions while writing this:
1. Rows are terminated with either '\n' or "\r\n".
   - '\r' is ignored in the parse.
2. If the delimiter appears in a field, the field must be enclosed in quotation marks. All characters in the quotes will be interpretted literally except quotes.
   - To use quotes inside the literal, use double quotes. Ex) "hello, ""world""" gets parsed as hello, "world"
   - The end of the quote is assumed to be the end of the field.
3. If your CSV file contains characters ranging outside of ASCII, you must use the w_char variant.

