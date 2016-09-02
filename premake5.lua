workspace "Penguin"
    configurations { "Debug", "Release" }

    language "C"
    warnings "Extra"
    buildoptions { "-std=c99", "-mcmodel=large", "-Wno-unused-parameter" }
    linkoptions { "-nostartfiles", "-Wl,-Ttext-segment=0xff00000000" }

    project "Manager"

        kind "ConsoleApp"
        location "build"
        targetname "manager"
        targetdir "."
        files { "src/**.h", "src/**.c" }
        includedirs { "../rtos/include", "../rtos/include/ipv4" }
        libdirs { "." }
        links { "umpn", "rt" }

