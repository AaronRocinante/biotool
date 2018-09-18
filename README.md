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
        :type filename: str
        :type number_to_search: int
        :param num_header_lines: number of header lines to skip
        :return: (line, byte_offset) or None
    
    search(...)
        search(filename, number_to_search, num_header_lines=1, field_index=0, sep=' \t')
        
        binary search for the line containing number_to_search in a sorted file each of whose lines contain a number
        :type filename: str
        :type number_to_search: int
        :param num_header_lines: number of header lines to skip
        :param field_index: 0-based index indicating which field in each line the number is
        :param sep: the field separator can be each of the characters in the sep string, default to whitespace and tabs
        :return: (line, byte_offset) or None
```


License
-------

Copyright 2018 Aaron Zhou

Licensed under the Apache License, Version 2.0 (the "License");

you may not use this file except in compliance with the License.

You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
