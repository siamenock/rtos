-- [[ Premake script for kernel workspace ]] 
workspace "Kernel"

    -- Assembly compiler build commands --
    -- Since there are no implicit compile commands for assembly files. We have to designate it
    filter 'files:src/**.asm'
        buildmessage 'Compiling %{file.relpath}'
        buildcommands {
            'nasm -f elf64 -o "%{cfg.objdir}/%{file.basename}.o" "%{file.relpath}"'
        }
        buildoutputs { '%{cfg.objdir}/%{file.basename}.o' }

    -- [[ Kernel project ]]
    project "kernel"
        -- Generate binary object for console application
        kind "ConsoleApp"
        -- Set the destination directory for a generated file related to build
        location "build"
        -- Set the target directory for a generated target file 
        targetdir "build"
        -- Name of genrated file is 
        targetname "kernel.elf"
        -- O0 optimziation
        optimize "Off"
        -- Force to make symbol table
        flags { "Symbols" }

        -- Designate linker script. Beginning exection starts with 'main'
        linkoptions { "-T elf_x86_64.ld -e main" }
        -- We cares about files below
        files { "src/**.asm", "src/**.h", "src/**.c" }
        -- Startup file is linked by linker script
        removefiles { "src/entry.asm" }
        -- Find headers in there 
        includedirs { "../lib/core/include", "../lib/TLSF/src", "../lib/lwip/src/**" }
        -- Link directory
        libdirs { "../lib" }
        -- Link external libaries named 
        links { "core", "tlsf", "lwip" }

        -- Make version header 
        prebuildcommands {
            './mkver.sh > ../src/version.h',
            "nasm -f elf64 -o obj/entry.o ../src/entry.asm"
        }
