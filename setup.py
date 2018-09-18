from distutils.core import setup, Extension

file_binary_search_module = Extension('file_binary_search', sources=['file_search.c'])

setup(name='biotool',
      version='0.1.6',
      license='Apache License, Version 2.0',
      description='BED file binary search, skips the first line of the file',
      ext_package='biotool',
      ext_modules=[file_binary_search_module])
