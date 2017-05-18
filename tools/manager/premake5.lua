workspace "Penguin"
    configurations { "Debug", "Release" }

    configurations "Debug"
        flags { "Symbols" }

    configurations "Release"
        flags { "Optimize" }

    language "C"
    warnings "Extra"
    buildoptions { "-idirafter ../../../kernel/src", "-DLINUX -std=gnu99", "-mcmodel=large", "-Wno-unused", "-Wno-format", "-Wno-unused-result", "-fno-builtin" }
    linkoptions { "-nostartfiles", "-Wl,-Ttext-segment=0xff00000000" }

    filter 'files:src/**.asm'
        buildmessage 'Compiling %{file.relpath}'
        buildcommands {
            'nasm -f elf64 -o "%{cfg.objdir}/%{file.basename}.o" "%{file.relpath}"'
        }
        buildoutputs { '%{cfg.objdir}/%{file.basename}.o' }

    project "Manager"

        kind "ConsoleApp"
        location "build"
        targetname "manager"
        targetdir "."
        files { "src/**.h", "src/**.c", "src/**.asm" }
        includedirs { "../../include", "../../include/ipv4" }
        libdirs { "." }
        links { "core_linux", "rt", "vnic", "tlsf" }

        -- Make version header
        prebuildcommands {
            '../mkver.sh > ../src/version.h'
        }

