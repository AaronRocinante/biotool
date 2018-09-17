//
// Created by Aaron on 9/16/18.
//

#include <Python.h>
#include <string.h>
#include <sys/stat.h>

#define ReturnPyNone() ({Py_INCREF(Py_None); return Py_None;})

static PyObject *SearchError;

long long get_location_from_line(char const *line)
{
    if (!line || strlen(line) == 0)
    {
        return -1;
    }

    char *copied_line = (char *) malloc(sizeof(char) * (1 + strlen(line)));
    strcpy(copied_line, line);
    char const sep[2] = ",";
    char *endptr;
    errno = 0;
    char *token = strtok(copied_line, sep);

    long long location = strtoll(token, &endptr, 10);
    free(copied_line);

    if ((errno == ERANGE && (location == LLONG_MIN || location == LLONG_MAX)) || (errno != 0 && location == 0))
    {
        fprintf(stderr, "failed to get location from the first field in line: %s\n", line);
        perror("get_location_from_line");
        exit(EXIT_FAILURE);
    }
    if (endptr == token)
    {
        fprintf(stderr, "no digits were found in the first field in line: %s\n", line);
        exit(EXIT_FAILURE);
    }
    return location;
}

typedef struct SearchResult
{
    char *line;
    ssize_t low;
    int found;
} SearchResult;

SearchResult not_found = {.line = NULL, .low = 0, .found=0};

SearchResult binary_search(ssize_t low, ssize_t high, long long number, FILE *file, char *line, size_t *buffer_size)
{
    if (low > high)
    {
        return not_found;
    }

    long long location;

    if (low == high)
    {
        fseek(file, low, SEEK_SET);
        getline(&line, buffer_size, file);
        location = get_location_from_line(line);
        if (location < 0)
        {
            return not_found;
        }
        if (location == number)
        {
            SearchResult result = {.line = line, .low = 0, .found=1};
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
            return binary_search(low, original_mid, number, file, line, buffer_size);
        }
    }
    // When we hit the newline character, the mid would not be incremented in the while loop body,
    // so we have to increment it here to keep it synchronized with the position of the file pointer.
    ++mid;

    getline(&line, buffer_size, file);
    location = get_location_from_line(line);

    if (location < 0)
    {
        return binary_search(low, original_mid, number, file, line, buffer_size);
    }
    else if (location == number)
    {
        SearchResult result = {.line = line, .low = mid, .found=1};
        return result;
    }
    else if (location > number)
    {
        return binary_search(low, original_mid, number, file, line, buffer_size);
    }
    else
    {
        return binary_search(mid, high, number, file, line, buffer_size);
    }
}

static PyObject *search(PyObject *self, PyObject *args)
{
    char *filename;
    long long number_to_search;
    if (!PyArg_ParseTuple(args, "sL", &filename, &number_to_search))
    {
        PyErr_SetString(SearchError, "failed to parse arguments");
        ReturnPyNone();
    }

    FILE *file;
    if (!(file = fopen(filename, "r")))
    {
        PyErr_SetString(SearchError, "failed to open file");
        ReturnPyNone();
    }

    struct stat file_stats;

    if (stat(filename, &file_stats) != 0)
    {
        PyErr_SetString(SearchError, "failed to get file stats");
        ReturnPyNone();
    }
    ssize_t file_byte_size = file_stats.st_size;

    size_t header_buffer_size = 1024;
    char *header = (char *) malloc(sizeof(char) * header_buffer_size);
    getline(&header, &header_buffer_size, file);
    free(header);

    size_t *buffer_size = (size_t *) malloc(sizeof(size_t));
    *buffer_size = 1024;
    char *line = (char *) malloc(sizeof(char) * (*buffer_size));

    SearchResult result = binary_search(strlen(header), file_byte_size, number_to_search, file, line, buffer_size);

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
{"search", search, METH_VARARGS, "Binary search on the lines of a bed file"},
{NULL, NULL, 0, NULL}
};

static struct PyModuleDef bed_binary_search_module = {
PyModuleDef_HEAD_INIT,
"bed_binary_search",   /* name of module */
"This module provides an interface for solving bed binary search.", /* module documentation, may be NULL */
-1,       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
module_methods
};

PyMODINIT_FUNC PyInit_bed_binary_search(void)
{
    PyObject *module = PyModule_Create(&bed_binary_search_module);
    if (module == NULL)
    {
        return NULL;
    }
    SearchError = PyErr_NewException("bed_binary_search.error", NULL, NULL);
    Py_INCREF(SearchError);
    PyModule_AddObject(module, "error", SearchError);
    return module;
}
