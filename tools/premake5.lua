include 'console'
include 'manager'

project 'tools'
    kind        'Makefile'
    location    '.'

    buildcommands {
        'test -f grub/Makefile || { cd grub; ./autogen.sh; ./configure; }',
        'make -C grub',
        'make -C console',
        'make -C manager',
        'make -C pnkc',
        'make -C smap',
    }

    cleancommands {
        'make clean -C grub',
        'make clean -C console',
        'make clean -C manager',
        'make clean -C pnkc',
        'make clean -C smap',
    }
