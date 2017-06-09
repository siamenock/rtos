configurations { 'linux' }

include 'console'
include 'manager'

project 'lib'
    kind        'Makefile'
    location    '.'

    buildcommands {
        'make -C grub',
        'make -C console',
        'make -C manager',
        'make -C pnkc',
        'make -C smap',

        'make -f extern.mk',
        'make -f archive.mk'
    }

    cleancommands {
        'make clean -C grub',
        'make clean -C aceunit',
        'make clean -C console',
        'make clean -C manager',
        'make clean -C pnkc',
        'make clean -C smap',

        'make -f extern.mk clean',
        'make -f archive.mk clean',
    }