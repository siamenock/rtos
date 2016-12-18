workspace "Utility"
    configurations { "Debug", "Release" }

    configurations "Debug"
    symbols "On" 

    language "C"

    warnings "Extra"
    buildoptions { "-DLINUX -std=gnu99", "-Wno-unused", "-Wno-format", "-Wno-unused-result", "-Wno-unused-parameter" }

    targetdir "."
    includedirs { "$(HOME)/penguin/include", "$(HOME)/penguin/include/ipv4" }
    libdirs { "../" }
    links { "umpn" }
    files { "src/connect.c" }

    project "create"
        kind "ConsoleApp"
        location "build"
        targetname "create"
        files { "src/create.c" }

    project "delete"
        kind "ConsoleApp"
        location "build"
        targetname "delete"
        files { "src/delete.c" }

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
            "cp ../start ../pause",
            "cp ../start ../resume",
            "cp ../start ../stop",
        }

