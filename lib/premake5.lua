configurations { 'linux' }

include 'hal'
include 'tlsf'
include 'ext'
include 'lwip'
include 'collection'

project 'lib'
    kind        'Makefile'
    location    '.'

    buildcommands {
        'make -C hal',
        'make -C tlsf',
        'make -C ext',
        'make -C lwip',
        'make -C collection',

        'make -f extern.mk',
        'make -f archive.mk'
    }

    cleancommands {
        'make clean -C hal',
        'make clean -C tlsf',
        'make clean -C ext',
        'make clean -C lwip',
        'make clean -C collection',

        'make -f extern.mk clean',
        'make -f archive.mk clean',
    }
