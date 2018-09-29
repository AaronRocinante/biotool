from distutils.core import setup, Extension

file_binary_search_module = Extension('file_binary_search', sources=['file_search.c'])

setup(name='biotool',
      version='0.1.8',
      license='Apache License, Version 2.0',
      description='Utilities such as binary search on the lines of a file',
      ext_package='biotool',
      ext_modules=[file_binary_search_module])
