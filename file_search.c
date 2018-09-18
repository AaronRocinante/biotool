//
// Created by Aaron on 9/16/18.
//

#include <Python.h>
#include <string.h>
#include <sys/stat.h>

#define ReturnPyNone() ({Py_INCREF(Py_None); return Py_None;})

static PyObject *SearchError;

typedef struct OptionalLongLong
{
    long long value;
    int valid;
} OptionalLongLong;

typedef struct StartEndPair
{
    ssize_t start;
    ssize_t end_exclusive;
    int valid;
} StartEndPair;

typedef struct SearchResult
{
    char *line;
    ssize_t low;
    int found;
} SearchResult;

inline int max_of_two(int a, int b)
{
    return a > b ? a : b;
}

ssize_t get_start_byte_offset(int const num_header_lines, FILE *file)
{
    size_t header_buffer_size = 256;
    char *header = (char *) malloc(sizeof(char) * header_buffer_size);
    ssize_t num_bytes_read, start_byte_offset = 0;

    for (int i = 0; i < num_header_lines; ++i)
    {
        num_bytes_read = getline(&header, &header_buffer_size, file);
        if (num_bytes_read == -1)
        {
            start_byte_offset = -1;
            break;
        }
        start_byte_offset += num_bytes_read;
    }
    free(header);
    return start_byte_offset;
}

/**
 * Converts a string into a long long and save it into result
 * @param string the string to be converted
 * @param result if the conversion is successfull, result will contain the converted long long.
 * Otherwise, the content of the result will remain unchanged.
 * @return 0 for success; -1 for failure.
 */
int strtoll_with_check(char const *string, long long *result)
{
    errno = 0;
    char *endptr;
    long long number = strtoll(string, &endptr, 10);
    if ((errno == ERANGE && (number == LLONG_MIN || number == LLONG_MAX)) || (errno != 0 && number == 0))
    {
        perror("strtoll_with_check strtoll");
        return -1;
    }
    if (endptr == string)
    {
        fprintf(stderr, "no digits were found in: %s\n", string);
        return -1;
    }
    *result = number;
    return 0;
}

OptionalLongLong get_long_long_from_line(char const *line, int const field_index, char const *sep)
{
    OptionalLongLong optional_location = {.valid=0};
    if (!line || strlen(line) == 0)
    {
        return optional_location;
    }

    char *copied_line = (char *) malloc(sizeof(char) * (1 + strlen(line)));
    strcpy(copied_line, line);
    errno = 0;
    int index = 0;
    char *token = strtok(copied_line, sep);
    long long location = 0;
    do
    {
        if (index == field_index)
        {
            if (strtoll_with_check(token, &location) == -1)
            {
                fprintf(stderr, "failed to get location from field %d (0-based) in line: %s\n", field_index, line);
                break;
            }
        }
        token = strtok(NULL, sep);
        ++index;
    } while (index <= field_index && token != NULL);

    optional_location.valid = index > field_index;
    optional_location.value = location;
    free(copied_line);
    return optional_location;
}

StartEndPair get_start_end_from_line(char const *line, int const start_index, int const end_index)
{
    StartEndPair pair = {.valid=0};
    if (!line || strlen(line) == 0)
    {
        return pair;
    }

    char *copied_line = (char *) malloc(sizeof(char) * (1 + strlen(line)));
    strcpy(copied_line, line);
    char const delim[2] = " \t";

    int index = 0;
    int larger_index = max_of_two(start_index, end_index);

    char *token;
    long long start, end_exclusive;

    token = strtok(copied_line, delim);

    do
    {
        if (index == start_index)
        {
            if (strtoll_with_check(token, &start) == -1)
            {
                fprintf(stderr, "failed to get start from field %d (0-based) in line: %s\n", start_index, line);
                break;
            }
            pair.start = start;
        }
        if (index == end_index)
        {
            if (strtoll_with_check(token, &end_exclusive) == -1)
            {
                fprintf(stderr, "failed to get end_exclusive from field %d (0-based) in line: %s\n", end_index, line);
                break;
            }
            pair.end_exclusive = end_exclusive;
        }
        token = strtok(NULL, delim);
        ++index;
    } while (index <= larger_index && token != NULL);

    free(copied_line);

    pair.valid = index > larger_index;
    return pair;
}

SearchResult not_found = {.line = NULL, .low = 0, .found=0};

SearchResult binary_search(ssize_t low, ssize_t high, long long const number, int const field_index, char const *sep,
                           FILE *file, char *line, size_t *buffer_size)
{
    if (low > high)
    {
        return not_found;
    }

    OptionalLongLong location;

    if (low == high)
    {
        fseek(file, low, SEEK_SET);
        getline(&line, buffer_size, file);
        location = get_long_long_from_line(line, field_index, sep);
        if (!location.valid)
        {
            return not_found;
        }
        if (location.value == number)
        {
            SearchResult result = {.line = line, .low = low, .found=1};
            return result;
        }
        else
        {
            return not_found;
        }
    }

    // There are at least two bytes to work with.
    ssize_t mid = low + (high - low) / 2;
    ssize_t original_mid = mid;
    fseek(file, mid, SEEK_SET);

    // Move right until the next new line character
    while ((char) fgetc(file) != '\n')
    {
        ++mid;
        // However, if we hit the upper bound before hitting a newline character,
        // we'd know that the line we're searching for could only be in the left half.
        if (mid == high)
        {
            return binary_search(low, original_mid, number, field_index, sep, file, line, buffer_size);
        }
    }
    // When we hit the newline character, the mid would not be incremented in the while loop body,
    // so we have to increment it here to keep it synchronized with the position of the file pointer.
    ++mid;

    getline(&line, buffer_size, file);
    location = get_long_long_from_line(line, field_index, sep);

    if (!location.valid)
    {
        return binary_search(low, original_mid, number, field_index, sep, file, line, buffer_size);
    }
    else if (location.value == number)
    {
        SearchResult result = {.line = line, .low = mid, .found=1};
        return result;
    }
    else if (location.value > number)
    {
        return binary_search(low, original_mid, number, field_index, sep, file, line, buffer_size);
    }
    else
    {
        return binary_search(mid, high, number, field_index, sep, file, line, buffer_size);
    }
}

static PyObject *search(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char *filename;
    long long number_to_search;
    int num_header_lines = 1, field_index = 0;
    char *sep = " \t";
    static char *keywords[] = {"filename", "number_to_search", "num_header_lines", "field_index", "sep", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sL|iis", keywords, &filename, &number_to_search, &num_header_lines,
                                     &field_index, &sep))
    {
        PyErr_SetString(SearchError, "failed to parse arguments");
        return NULL;
    }

    FILE *file;
    if (!(file = fopen(filename, "r")))
    {
        PyErr_SetString(SearchError, "failed to open file");
        return NULL;
    }

    struct stat file_stats;

    if (stat(filename, &file_stats) != 0)
    {
        PyErr_SetString(SearchError, "failed to get file stats");
        return NULL;
    }
    ssize_t file_byte_size = file_stats.st_size;
    ssize_t start_byte_offset = get_start_byte_offset(num_header_lines, file);

    size_t *buffer_size = (size_t *) malloc(sizeof(size_t));
    *buffer_size = 256;
    char *line = (char *) malloc(sizeof(char) * (*buffer_size));

    SearchResult result = binary_search(start_byte_offset, file_byte_size, number_to_search, field_index, sep,
                                        file, line, buffer_size);

    fclose(file);
    PyObject *val;
    if (result.found)
    {
        val = Py_BuildValue("sL", result.line, result.low);
    }
    else
    {
        Py_INCREF(Py_None);
        val = Py_None;
    }
    free(line);
    free(buffer_size);
    return val;
}

/**
 *
 * @param low 0-based low byte index (inclusive)
 * @param high 0-based high byte index (inclusive)
 * @param number the number to search for
 * @param file
 * @param line
 * @param buffer_size
 * @return
 */
SearchResult bed_binary_search(ssize_t low, ssize_t high, long long const number, FILE *file, char *line,
                               size_t *buffer_size)
{
    if (low > high)
    {
        return not_found;
    }

    StartEndPair pair;

    if (low == high)
    {
        fseek(file, low, SEEK_SET);
        getline(&line, buffer_size, file);
        pair = get_start_end_from_line(line, 1, 2);
        if (!pair.valid)
        {
            return not_found;
        }
        if (pair.start <= number && number < pair.end_exclusive)
        {
            SearchResult result = {.line = line, .low = low, .found=1};
            return result;
        }
        else
        {
            return not_found;
        }
    }

    // There are at least two bytes to work with.
    ssize_t mid = low + (high - low) / 2;
    ssize_t original_mid = mid;
    fseek(file, mid, SEEK_SET);

    // Move right until the next new line character
    while ((char) fgetc(file) != '\n')
    {
        ++mid;
        // However, if we hit the upper bound before hitting a newline character,
        // we'd know that the line we're searching for could only be in the left half.
        if (mid == high)
        {
            return bed_binary_search(low, original_mid, number, file, line, buffer_size);
        }
    }
    // When we hit the newline character, the mid would not be incremented in the while loop body,
    // so we have to increment it here to keep it synchronized with the position of the file pointer.
    ++mid;

    getline(&line, buffer_size, file);
    pair = get_start_end_from_line(line, 1, 2);

    if (!pair.valid)
    {
        return bed_binary_search(low, original_mid, number, file, line, buffer_size);
    }
    else if (pair.start <= number && number < pair.end_exclusive)
    {
        SearchResult result = {.line = line, .low = mid, .found=1};
        return result;
    }
    else if (number < pair.start)
    {
        return bed_binary_search(low, original_mid, number, file, line, buffer_size);
    }
    else
    {
        return bed_binary_search(mid, high, number, file, line, buffer_size);
    }
}

static PyObject *bed_search(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char *filename;
    long long number_to_search;
    int num_header_lines = 1;
    static char *keywords[] = {"filename", "number_to_search", "num_header_lines", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sL|i", keywords, &filename, &number_to_search, &num_header_lines))
    {
        PyErr_SetString(SearchError, "failed to parse arguments");
        return NULL;
    }

    FILE *file;
    if (!(file = fopen(filename, "r")))
    {
        PyErr_SetString(SearchError, "failed to open file");
        return NULL;
    }

    struct stat file_stats;

    if (stat(filename, &file_stats) != 0)
    {
        PyErr_SetString(SearchError, "failed to get file stats");
        return NULL;
    }

    ssize_t file_byte_size = file_stats.st_size;
    ssize_t start_byte_offset = get_start_byte_offset(num_header_lines, file);

    size_t *buffer_size = (size_t *) malloc(sizeof(size_t));
    *buffer_size = 256;
    char *line = (char *) malloc(sizeof(char) * (*buffer_size));

    SearchResult result = {.found=0};
    if (start_byte_offset != -1)
    {
        result = bed_binary_search(start_byte_offset, file_byte_size - 1, number_to_search, file, line, buffer_size);
    }

    fclose(file);
    PyObject *val;
    if (result.found)
    {
        val = Py_BuildValue("sL", result.line, result.low);
    }
    else
    {
        Py_INCREF(Py_None);
        val = Py_None;
    }

    free(line);
    free(buffer_size);
    return val;
}

static PyMethodDef module_methods[] = {
    {"search", (PyCFunction) search, METH_VARARGS | METH_KEYWORDS,
     "search(filename, number_to_search, num_header_lines=1, field_index=0, sep=' \\t')\n\n"
     "binary search for the line containing number_to_search in a sorted file "
     "each of whose lines contain a number\n"
     ":type filename: str\n"
     ":type number_to_search: int\n"
     ":param num_header_lines: number of header lines to skip\n"
     ":param field_index: 0-based index indicating which field in each line the number is\n"
     ":param sep: the field separator can be each of the characters in the sep string, default to whitespace and tabs\n"
     ":return: (line, byte_offset) or None"},

    {"bed_search", (PyCFunction) bed_search, METH_VARARGS | METH_KEYWORDS,
     "bed_search(filename, number_to_search, num_header_lines=1)\n\n"
     "binary search for the line containing number_to_search in a sorted BED file\n"
     ":type filename: str\n"
     ":type number_to_search: int\n"
     ":param num_header_lines: number of header lines to skip\n"
     ":return: (line, byte_offset) or None"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef file_binary_search_module = {
    PyModuleDef_HEAD_INIT,
    "file_binary_search",   /* name of module */
    "This module provides functions for searching lines in a sorted file", /* module documentation, may be NULL */
    -1,       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    module_methods
};

PyMODINIT_FUNC PyInit_file_binary_search(void)
{
    PyObject *module = PyModule_Create(&file_binary_search_module);
    if (module == NULL)
    {
        return NULL;
    }
    SearchError = PyErr_NewExceptionWithDoc("file_binary_search.FileSearchException", "FileSearchException", NULL,
                                            NULL);
    Py_INCREF(SearchError);
    PyModule_AddObject(module, "FileSearchException", SearchError);
    return module;
}
