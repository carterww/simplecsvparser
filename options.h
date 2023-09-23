#ifndef OPTION_H
#define OPTION_H

#ifdef __cplusplus
extern "C" {
#endif

/* Supported delimiters when using widechar */
#define W_DELIMITER_SPACE L' '
#define W_DELIMITER_TAB L'\t'
#define W_DELIMITER_COMMA L','
#define W_DELIMITER_SEMICOLON L';'
#define W_DELIMITER_PIPE L'|'

/* Supported delimiters when using char */
#define DELIMITER_SPACE ' '
#define DELIMITER_TAB '\t'
#define DELIMITER_COMMA ','
#define DELIMITER_SEMICOLON ';'
#define DELIMITER_PIPE '|'

/* Default options for the parser */
#define BUFFER_SIZE 128
#define ROW_GROWTH 8
#define FIELD_GROWTH 4

#ifdef __cplusplus
}
#endif

#endif /* OPTION_H */
