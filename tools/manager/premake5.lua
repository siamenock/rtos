workspace "Penguin"
    configurations { "Debug", "Release" }

    configurations "Debug"
        flags { "Symbols" }

    configurations "Release"
        flags { "Optimize" }

    language "C"
    warnings "Extra"
    buildoptions { "-DLINUX -std=gnu99", "-mcmodel=large", "-Wno-unused", "-Wno-format", "-Wno-unused-result" }
    linkoptions { "-nostartfiles", "-Wl,-Ttext-segment=0xff00000000" }

    project "Manager"

        kind "ConsoleApp"
        location "build"
        targetname "manager"
        targetdir "."
        files { "src/**.h", "src/**.c" }
        includedirs { "../penguin/lib/nic/include", "../penguin/include", "../penguin/include/ipv4" }
        libdirs { "." }
        links { "umpn", "rt", "nic" }

        -- Make version header
        prebuildcommands {
            '../mkver.sh > ../src/version.h'
        }

