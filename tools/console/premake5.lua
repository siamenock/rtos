project "connect"
    includedirs { "../../lib/include", "include" }
    libdirs { "../../lib/ext/", "../../lib/tlsf/", "../../lib/hal/" }
    links { "ext", "tlsf", "hal" }
    location "build"
    kind "ConsoleApp"
    targetdir "bin"
    targetname "connect"
    files { "src/rpc.c" }
    files { "src/connect.c" }

project "create"
    includedirs { "../../lib/include", "include" }
    libdirs { "../../lib/ext/", "../../lib/tlsf/", "../../lib/hal/" }
    links { "ext", "tlsf", "hal" }
    location "build"
    kind "ConsoleApp"
    targetdir "bin"
    targetname "create"
    files { "src/rpc.c" }
    files { "src/create.c" }

project "destroy"
    includedirs { "../../lib/include", "include" }
    libdirs { "../../lib/ext/", "../../lib/tlsf/", "../../lib/hal/" }
    links { "ext", "tlsf", "hal" }
    location "build"
    kind "ConsoleApp"
    targetdir "bin"
    targetname "destroy"
    files { "src/rpc.c" }
    files { "src/destroy.c" }

project "list"
    includedirs { "../../lib/include", "include" }
    libdirs { "../../lib/ext/", "../../lib/tlsf/", "../../lib/hal/" }
    links { "ext", "tlsf", "hal" }
    location "build"
    kind "ConsoleApp"
    targetdir "bin"
    targetname "list"
    files { "src/rpc.c" }
    files { "src/list.c" }

project "status"
    includedirs { "../../lib/include", "include" }
    libdirs { "../../lib/ext/", "../../lib/tlsf/", "../../lib/hal/" }
    links { "ext", "tlsf", "hal" }
    location "build"
    kind "ConsoleApp"
    targetdir "bin"
    targetname "status"
    files { "src/rpc.c" }
    files { "src/status.c" }

project "start"
    includedirs { "../../lib/include", "include" }
    libdirs { "../../lib/ext/", "../../lib/tlsf/", "../../lib/hal/" }
    links { "ext", "tlsf", "hal" }
    location "build"
    kind "ConsoleApp"
    targetdir "bin"
    targetname "start"
    files { "src/rpc.c" }
    files { "src/start.c" }
    -- FIXME: Start, Pause, Resume, Stop commands are same. Need to be refactored

project "upload"
    includedirs { "../../lib/include", "include" }
    libdirs { "../../lib/ext/", "../../lib/tlsf/", "../../lib/hal/" }
    links { "ext", "tlsf", "hal" }
    location "build"
    kind "ConsoleApp"
    targetdir "bin"
    targetname "upload"
    files { "src/rpc.c" }
    files { "src/upload.c" }

project "download"
    includedirs { "../../lib/include", "include" }
    libdirs { "../../lib/ext/", "../../lib/tlsf/", "../../lib/hal/" }
    links { "ext", "tlsf", "hal" }
    location "build"
    kind "ConsoleApp"
    targetdir "bin"
    targetname "download"
    files { "src/rpc.c" }
    files { "src/download.c" }

project "monitor"
    includedirs { "../../lib/include", "include" }
    libdirs { "../../lib/ext/", "../../lib/tlsf/", "../../lib/hal/" }
    links { "ext", "tlsf", "hal" }
    location "build"
    kind "ConsoleApp"
    targetdir "bin"
    targetname "monitor"
    files { "src/rpc.c" }
    files { "src/monitor.c" }

project "stdin"
    includedirs { "../../lib/include", "include" }
    libdirs { "../../lib/ext/", "../../lib/tlsf/", "../../lib/hal/" }
    links { "ext", "tlsf", "hal" }
    location "build"
    kind "ConsoleApp"
    targetdir "bin"
    targetname "stdin"
    files { "src/rpc.c" }
    files { "src/stdin.c" }

project 'console'
    kind        'Makefile'
    location    '.'

    buildcommands {
        'make -C build -f connect.make',
        'make -C build -f create.make',
        'make -C build -f destroy.make',
        'make -C build -f list.make',
        'make -C build -f status.make',
        'make -C build -f start.make',
        'make -C build -f upload.make',
        'make -C build -f download.make',
        'make -C build -f monitor.make',
        'make -C build -f stdin.make',
    }

    postbuildcommands { 
        "cp bin/start bin/pause",
        "cp bin/start bin/resume",
        "cp bin/start bin/stop",
    }

    cleancommands {
        'make -C build clean -f connect.make clean',
        'make -C build clean -f create.make clean',
        'make -C build clean -f destroy.make clean',
        'make -C build clean -f list.make clean',
        'make -C build clean -f status.make clean',
        'make -C build clean -f start.make clean',
        'make -C build clean -f upload.make clean',
        'make -C build clean -f download.make clean',
        'make -C build clean -f monitor.make clean',
        'make -C build clean -f stdin.make clean',
	'rm -f bin/pause',
	'rm -f bin/resume',
	'rm -f bin/stop',
    }
