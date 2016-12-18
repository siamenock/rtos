workspace "Utility"
    configurations { "Debug", "Release" }

    configurations "Debug"
    flags { "Symbols" }

    configurations "Release"
    flags { "Optimize" }

    language "C"
    warnings "Extra"
    buildoptions { "-DLINUX -std=gnu99", "-Wno-unused", "-Wno-format", "-Wno-unused-result" }

    project "create"

        kind "ConsoleApp"
        location "build"
        targetname "create"
        targetdir "."
        files { "src/connect.c", "src/create.c" }
        includedirs { "$(HOME)/penguin/include", "$(HOME)/penguin/include/ipv4" }
        libdirs { "../" }
        links { "umpn" }
