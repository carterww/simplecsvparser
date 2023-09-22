# What constites a CSV file?
1. A CSV file is generally a file format for representing plain text data in a sequence of rows. The fields in each row are separated by a delimiter (usually a comma).
    - Other delimiters include tab, space, semi-colon, really whatever you want.
2. A rule is if a field contains the delimiter, it must be enclosed in quotation marks.
3. Usually each record has the same number of fields.
    - My note: I think this rule is probably broken a lot, so a parser should be able to handle csv files where the data is not so strict. Ex). A csv file full of integer arrays for testing.

# Any agreed format?
Not really. There is a standard called RFC 4180, but wikipedia says it's usually used pretty loosely.

# Rules I'll Use to make this
1. CSV will have an optional header row(s).
    - In the case a header is present, the user must identify which line the data starts on.
2. Fields will be separated by a delimiter, by default a comma.
    - The user can change this to whatever character they would like.
3. Leading and trailing spaces and tabs will be ignored by default.
    - The user will be able to change this. The only options, however, will be ignoring the spaces and tabs or keeping them.
    - If a tab or space is a delimeter, this option will automatically be set to not ignore them.
4. If the delimeter is present in the field, the field must be enclosed in quotation marks. In this case, everything in the quotation marks will be interpretted literally.
    - If a quotation mark is present in the field, it must be represented as double quotes. Ex. "Hello, ""World""",
5. All rows will have the same number of fields in the same order.
    - There will be an option for rows of variable length for dealing with unstructured data.
6. ASCII will be the default encoding for the file. If the file is UTF-8, it must be specified by the user.
    - I will probably not support UTF-16 and some other things because who would actually use it.
7. A record ends with a line terminator (\n).
8. Everything in the CSV will be interpretted as strings.
    - I may add support for reading in numbers directly.
