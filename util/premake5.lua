workspace "Utility"
    configurations { "Debug", "Release" }

    configurations "Debug"
    flags "symbols" 

    language "C"

    warnings "Extra"
    buildoptions { "-DLINUX -std=gnu99", "-Wno-unused", "-Wno-format", "-Wno-unused-result", "-Wno-unused-parameter" }

    targetdir "bin"
    includedirs { "$(HOME)/penguin/include", "$(HOME)/penguin/include/ipv4" }
    libdirs { "../" }
    links { "umpn" }
    files { "src/rpc.c" }

    project "connect"
        kind "ConsoleApp"
        location "build"
        targetname "connect"
        files { "src/connect.c" }

    project "create"
        kind "ConsoleApp"
        location "build"
        targetname "create"
        files { "src/create.c" }

    project "destroy"
        kind "ConsoleApp"
        location "build"
        targetname "destroy"
        files { "src/destroy.c" }

    project "list"
        kind "ConsoleApp"
        location "build"
        targetname "list"
        files { "src/list.c" }

    project "status"
        kind "ConsoleApp"
        location "build"
        targetname "status"
        files { "src/status.c" }

    project "start"
        kind "ConsoleApp"
        location "build"
        targetname "start"
        files { "src/start.c" }
        -- FIXME: Start, Pause, Resume, Stop commands are same. Need to be refactored
        postbuildcommands { 
            "cp ../bin/start ../bin/pause",
            "cp ../bin/start ../bin/resume",
            "cp ../bin/start ../bin/stop",
        }

    project "upload"
        kind "ConsoleApp"
        location "build"
        targetname "upload"
        files { "src/upload.c" }

    project "download"
        kind "ConsoleApp"
        location "build"
        targetname "download"
        files { "src/download.c" }

    project "monitor"
        kind "ConsoleApp"
        location "build"
        targetname "monitor"
        files { "src/monitor.c" }

    project "stdin"
        kind "ConsoleApp"
        location "build"
        targetname "stdin"
        files { "src/stdin.c" }
