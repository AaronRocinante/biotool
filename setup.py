from distutils.core import setup, Extension

module1 = Extension('bed_binary_search', sources=['bed_binary_search.c'])

setup(name='bed_binary_search',
      version='0.1.1',
      license='Apache License, Version 2.0',
      description='BED file binary search',
      ext_modules=[module1])
