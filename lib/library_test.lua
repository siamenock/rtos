workspace "Test"

    -- Link test framework "Cmocka" 
    links { "cmocka" }

    -- [[ 1. Core library ]]
        -- [[ 1.1 Cache test ]]
        project "cache_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include" }
            files { "core/src/test/cache.c", "core/src/**.h" }
            -- Link testing target library
            linkoptions { "../../../libumpn.a" }
            postbuildcommands {
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} 2>/dev/null',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath}'
            }
            
        -- [[ 1.2. List test ]]
        project "list_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include" }
            files { "core/src/test/list.c", "core/src/**.h" }
            -- Link testing target library
            linkoptions { "../../../libumpn.a" }
            postbuildcommands {
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} 2>/dev/null',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath}'
            }

        -- [[ 1.3. Map test ]]
        project "map_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include" }
            files { "core/src/test/map.c", "core/src/**.h" }
            -- Link testing target library
            linkoptions { "../../../libumpn.a" }
            postbuildcommands {
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} 2>/dev/null',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath}'
            }
            
    -- Templete other library below
    -- [[ 2. Others ]] 
        -- [[ 2.1 ... ]] 
        --[[
           [project "jsmn_test"
           [    kind "ConsoleApp"
           [    -- Set the target directory for a generated target file 
           [    targetdir "test/jsmn"
           [    location "build/test/jsmn"
           [    includedirs { "jsmn/include" }
           [    files { "jsmn/src/test/cache.c", "jsmn/src/**.h" }
           [    -- Link testing target library
           [    linkoptions { "../../../jsmn.a" }
           ]]
