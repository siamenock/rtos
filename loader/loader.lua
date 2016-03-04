-- [[ Premake script for loader workspace ]] 
workspace "Loader"
    -- We cares about files below
    files { "src/**.h", "src/**.c" }

    -- [[ Temporary loader project ]]
    project "loader.tmp"
        -- Generate binary object for console application
        kind "ConsoleApp"
        -- Set the target direcotory for a generated target file 
        flags { "Symbols" }
        -- Set the destination direcotory for a generated file related to build
        location "build"
        -- Set the target direcotory for a generated target file 
        targetdir "build"

        prebuildcommands {
            "./set_tmp",
            "nasm -f elf32 -o obj/entry.o ../src/entry_tmp.asm"
        }

        postbuildcommands {
            "./set_value"
        }

    -- [[ Loader project ]]
    project "loader.bin"
        -- Generate binary object for console application
        kind "ConsoleApp"
        -- Set the destination direcotory for a generated file related to build
        location "build"
        -- Set the target direcotory for a generated target file 
        targetdir "build"

        prebuildcommands {
            "nasm -f elf32 -o obj/entry.o ../src/entry_tmp.asm"
        }

        postbuildcommands {
            "{DELETE} ../src/entry_tmp.asm"
        }
