# biotool

To install: 

`pip3 install biotool`


To import in python3: 

`import biotool.file_binary_search as bs`


Then call `bs.bed_search(...)` or `bs.search(...)` as per the docstring below.

```
FUNCTIONS
    bed_search(...)
        bed_search(filename, number_to_search, num_header_lines=1)
        
        binary search for the line containing number_to_search in a sorted BED file
        :param num_header_lines: number of header lines to skip
        :return: (line, byte_offset) or None
    
    search(...)
        search(filename, number_to_search, num_header_lines=1, field_index=0)
        
        binary search for the line containing number_to_search in a sorted file each of whose lines contain a number
        :param num_header_lines: number of header lines to skip
        :param field_index: 0-based index indicating which field in each line the number is
        :return: (line, byte_offset) or None
```
