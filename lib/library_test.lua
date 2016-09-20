workspace "Test"

    -- Link test framework "Cmocka" 
    links { "cmocka" }

    -- Assembly compiler build commands --
    filter 'files:**.asm'
        buildmessage 'Compiling %{file.relpath}'
        buildcommands {
            'nasm -f elf64 -o "%{cfg.objdir}/%{file.basename}.o" "%{file.relpath}"'
        }
        buildoutputs { '%{cfg.objdir}/%{file.basename}.o' }

    -- [[ 1. Core library ]]
        -- [[ 1.1 Cache test ]]
        project "cache_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include", "TLSF/src" }
            files { "core/src/cache.c", "core/src/test/cache.c", "core/src/asm.asm", "core/src/lock.c", "core/src/**.h" , "core/src/_malloc.c", "TLSF/src/**.h", "core/src/map.c", "core/src/list.c" }
            -- Link testing target library
            linkoptions { "../../../libtlsf.a" }
            postbuildcommands {
                -- Remove old result
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                -- Genrate XML format result . ||:(or true) is for error suppressing
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} ||:',
                -- Genrate stdout format result 
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
            }
            
        -- [[ 1.2. List test ]]
        project "list_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include", "TLSF/src" }
            files { "core/src/list.c", "core/src/test/list.c", "core/src/asm.asm", "core/src/lock.c", "core/src/**.h", "core/src/_malloc.c", "TLSF/src/**.h",}
            -- Link testing target library
            linkoptions { "../../../libtlsf.a" }
            postbuildcommands {
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} ||:',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
            }

        -- [[ 1.3. Map test ]]
        project "map_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include", "TLSF/src"}
            files { "core/src/test/map.c", "core/src/map.c", "core/src/list.c", "core/src/asm.asm", "core/src/lock.c", "core/src/**.h", "core/src/_malloc.c", "TLSF/src/**.h",}
            -- Link testing target library
            linkoptions { "../../../libtlsf.a" }
            postbuildcommands {
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} 2>/dev/null ||:',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
            }

        -- [[ 1.4. Malloc test ]]
        project "malloc_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include", "TLSF/src" }
            files { "core/src/asm.asm", "core/src/lock.c", "core/src/_malloc.c", "core/src/test/_malloc.c", "core/src/**.h", "TLSF/src/**.h" }
            -- Link testing target library
            linkoptions { "../../../libtlsf.a"}
            postbuildcommands {
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} ||:',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
            }

        -- [[ 1.5. String test ]]
        project "string_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include" }
            files { "core/src/_string.c", "core/src/test/_string.c", "core/src/**.h" }
            -- Link testing target library
            buildoptions { "-msse4.1" }
            postbuildcommands {
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} ||:',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
            }

        -- [[ 1.6. Gmalloc test ]]
        project "gmalloc_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include", "TLSF/src" }
            files { "core/src/asm.asm", "core/src/lock.c", "core/src/_malloc.c", "core/src/gmalloc.c", "core/src/test/gmalloc.c", "core/src/**.h", "TLSF/src/**.h" }
            -- Link testing target library
            linkoptions { "../../../libtlsf.a"}
            postbuildcommands {
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} ||:',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
            }

        -- [[ 1.7. Timer test ]]
        project "timer_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include" }
            files { "core/src/timer.c", "core/src/test/timer.c", "core/src/**.h" }
            -- Link testing target library
            -- buildoptions { "-D LINUX" }
            postbuildcommands {
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} ||:',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
            }

        -- [[ 1.8. Thread test ]]
        project "thread_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include" }
            files { "core/src/asm.asm", "core/src/lock.c", "core/src/thread.c", "core/src/test/thread.c", "core/src/**.h" }
            -- Link testing target library
            buildoptions { "-msse4.1"  }
            linkoptions { "-lpthread" }
            postbuildcommands {
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} ||:',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
            }

        -- [[ 1.9. Shared test ]]
        project "shared_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include" }
            files { "core/src/shared.c", "core/src/test/shared.c", "core/src/**.h" }
            -- Link testing target library
            buildoptions { "-msse4.1" }
            postbuildcommands {
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} ||:',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
            }

        -- [[ 1.10. Lock test ]]
        project "lock_test"
            kind "ConsoleApp"
			linkoptions { "-lpthread" }
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include" }
            files { "core/src/asm.asm", "core/src/lock.c", "core/src/lock.c", "core/src/test/lock.c", "core/src/**.h" }
            -- Link testing target library
            postbuildcommands {
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} ||:',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
            }
           

        -- [[ 1.11. File test ]]
        project "file_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target lock 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include" , "TLSF/src" }
            files { "core/src/asm.asm", "core/src/map.c", "core/src/lock.c", "core/src/lock.c", "core/src/_malloc.c", "core/src/list.c", 
                    "core/src/event.c", "core/src/fifo.c", "core/src/timer.c", "core/src/fio.c", "core/src/file.c", "core/src/test/file.c", "core/src/**.h" }
            -- Link testing target library
            buildoptions { "-msse4.1" }
            linkoptions { "../../../libtlsf.a", "-lpthread" }
            postbuildcommands {
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} ||:',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
            }

        -- [[ 1.12. Fio test ]]
        project "fio_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target lock 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include" , "TLSF/src" }
            files { "core/src/asm.asm", "core/src/lock.c", "core/src/lock.c", "core/src/_malloc.c", 
                    "core/src/fifo.c", "core/src/fio.c", "core/src/test/fio.c", "core/src/**.h" }
            -- Link testing target library
            buildoptions { "-msse4.1" }
            linkoptions { "../../../libtlsf.a", "-lpthread" }
            postbuildcommands {
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} ||:',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
            }


----
--        -- [[ 1.13. RPC test ]]
--        project "rpc_test"
--            kind "ConsoleApp"
--            -- Set the target directory for a generated target file 
--            targetdir "test/core"
--            location "build/test/core"
--            includedirs { "core/include" , "core/include/control"}
--            files { "core/src/**.h" , "core/include/control/**.h", "core/src/rpc.c", "core/src/test/rpc.c" }
--            -- Link testing target library
--            buildoptions { "-D LINUX" }
--            linkoptions { "-lpthread" }
--            postbuildcommands {
--                '{DELETE} %{cfg.buildtarget.abspath}.xml',
--                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} ||:',
--                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
--            }

            
    -- Templete other library below
    -- [[ 2. Others ]] 
        -- [[ 2.1 Cmd test ]]
        project "cmd_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include", "TLSF/src" }
            files { "core/src/cmd.c", "core/src/test/cmd.c", "core/include/util/**.h", "core/src/asm.asm", "core/src/lock.c", "core/src/**.h" , "core/src/_malloc.c", "TLSF/src/**.h", "core/src/map.c", "core/src/list.c" }
            -- Link testing target library
            linkoptions { "../../../libtlsf.a" }
            postbuildcommands {
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} ||:',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
            }

 
        -- [[ 2.2 Types test ]]
        project "types_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include" }
            files { "core/src/types.c", "core/src/test/types.c", "core/src/_string.c", "core/src/**.h" }
            -- Link testing target library
         --   linkoptions { "../../../libtlsf.a" }
            buildoptions { "-msse4.1" }
            postbuildcommands {
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} ||:',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
            }


        -- [[ 2.3 Json test ]]
        project "json_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include", "jsmn" }
            files { "core/src/json.c", "core/src/test/json.c", "jsmn/jsmn.c", "jsmn/jsmn.h", "core/include/util/**.h" }
            -- Link testing target library
            linkoptions { "../../../libtlsf.a" }
            postbuildcommands {
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} ||:',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
            }


        -- [[ 2.4 Ring test ]]
        project "ring_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include" }
            files { "core/src/ring.c", "core/src/test/ring.c", "core/include/util/**.h" }
            -- Link testing target library
            linkoptions { "../../../libtlsf.a", "-lpthread" }
            postbuildcommands {
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} ||:',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
            }


        -- [[ 2.5. Set test ]]
        project "set_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include", "TLSF/src"}
            files { "core/src/test/set.c", "core/src/set.c", "core/src/list.c", "core/src/asm.asm", "core/src/lock.c", "core/src/**.h", "core/src/_malloc.c", "TLSF/src/**.h",}
            -- Link testing target library
            linkoptions { "../../../libtlsf.a" }
            postbuildcommands {
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} 2>/dev/null ||:',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:' 
            }

        -- [[ 2.7 Vector test ]]
        project "vector_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include", "TLSF/src"}
            files { "core/src/test/vector.c", "core/src/vector.c", "core/src/asm.asm", "core/src/**.h", "core/src/lock.c", "core/src/_malloc.c", "TLSF/src/**.h",}
            -- Link testing target library
            linkoptions { "../../../libtlsf.a" }
            postbuildcommands {
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} 2>/dev/null ||:',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
            }

			
        -- [[ 2.8 Event test ]]
        project "event_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include", "TLSF/src" }
            files { "core/src/event.c", "core/src/test/event.c", "core/include/util/**.h", "core/src/asm.asm", "core/src/lock.c", "core/src/**.h", "core/src/_malloc.c", "TLSF/src/**.h", "core/src/map.c", "core/src/list.c", "core/src/timer.c" }
            -- Link testing target library
            linkoptions { "../../../libtlsf.a" }
            postbuildcommands {
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} ||:',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
            }


        -- [[ 2.9 Fifo test ]]
        project "fifo_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include", "TLSF/src"}
            files { "core/src/test/fifo.c", "core/src/fifo.c","core/src/asm.asm", "core/src/**.h", "core/src/lock.c", "core/src/_malloc.c", "TLSF/src/**.h",}
            -- Link testing target library
            linkoptions { "../../../libtlsf.a" }
            postbuildcommands {
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} 2>/dev/null ||:',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
            }

    -- [[ 3. net ]] 
        -- [[ 3.1 Crc test ]]
        project "crc_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include" }
            files { "core/src/crc.c", "core/src/test/crc.c", "core/include/util/**.h" }
            -- Link testing target library
            linkoptions { "../../../libtlsf.a" }
            postbuildcommands {
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} ||:',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
            }


        -- [[ 3.2 Checksum test ]]
        project "checksum_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include" }
            files { "core/src/checksum.c", "core/src/test/checksum.c", "core/include/util/**.h" }
            -- Link testing target library
            linkoptions { "../../../libtlsf.a" }
            postbuildcommands {
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} ||:',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
            }


        -- [[ 3.3 Interface test ]]
        project "interface_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include", "TLSF/src" }
            files { "core/src/interface.c", "core/src/test/interface.c", "core/include/util/**.h", "core/src/asm.asm", "core/src/lock.c", "core/src/**.h" , "core/src/_malloc.c", "TLSF/src/**.h", "core/src/set.c", "core/src/list.c" }
            -- Link testing target library
            linkoptions { "../../../libtlsf.a" }
            postbuildcommands {
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} ||:',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
            }


        -- [[ 3.4 Ether test ]]
        project "ether_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include" }
            files { "core/src/ether.c", "core/src/test/ether.c", "core/include/util/**.h" }
            -- Link testing target library
            linkoptions { "../../../libtlsf.a" }
            postbuildcommands {
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} ||:',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
            }


        -- [[ 3.5 Packet test ]]
        project "packet_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include" }
            files { "core/src/packet.c", "core/src/test/packet.c", "core/include/util/**.h" }
            -- Link testing target library
            linkoptions { "../../../libtlsf.a" }
            postbuildcommands {
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} ||:',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
			} 


        -- [[ 3.6 Md5 test ]]
        project "md5_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include" }
            files { "core/src/md5.c", "core/src/test/md5.c", "core/src/**.h", "core/src/md5_asm.S" }
            -- Link testing target library
            linkoptions { "../../../libtlsf.a" }
            postbuildcommands {
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} ||:',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
            }


        -- [[ 3.7 Ip test ]]
        project "ip_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include", "TLSF/src" }
            files { "core/src/ip.c", "core/src/test/ip.c", "core/src/icmp.c", "core/src/nic.c", "core/src/interface.c", "core/include/util/**.h", "core/src/asm.asm", "core/src/lock.c", "core/src/**.h" , "core/src/_malloc.c", "TLSF/src/**.h", "core/src/fifo.c", "core/src/map.c", "core/src/list.c", "core/src/set.c", "core/src/checksum.c" }
            -- Link testing target library
            linkoptions { "../../../libtlsf.a" }
            postbuildcommands {
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} ||:',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
            }


        -- [[ 3.8 Nic test ]]
        project "nic_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include", "TLSF/src" }
            files { "core/src/nic.c", "core/src/test/nic.c", "core/src/interface.c", "core/include/util/**.h", "core/src/asm.asm", "core/src/lock.c", "core/src/**.h" , "core/src/_malloc.c", "TLSF/src/**.h", "core/src/fifo.c", "core/src/map.c", "core/src/list.c", "core/src/set.c" }
            -- Link testing target library
            linkoptions { "../../../libtlsf.a", "-lpthread" }
            postbuildcommands {
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} ||:',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
            }


        -- [[ 3.9 Icmp test ]]
        project "icmp_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include", "TLSF/src" }
            files { "core/src/icmp.c", "core/src/nic.c", "core/src/test/icmp.c", "core/src/interface.c", "core/include/util/**.h", "core/src/asm.asm", "core/src/lock.c", "core/src/**.h" , "core/src/_malloc.c", "TLSF/src/**.h", "core/src/fifo.c", "core/src/map.c", "core/src/list.c", "core/src/set.c", "core/src/checksum.c" }
            -- Link testing target library
            linkoptions { "../../../libtlsf.a" }
            postbuildcommands {
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} ||:',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
            }


        -- [[ 3.10 Arp test ]]
        project "arp_test"
            kind "ConsoleApp"
            -- Set the target directory for a generated target file 
            targetdir "test/core"
            location "build/test/core"
            includedirs { "core/include", "TLSF/src" }
            files { "core/src/arp.c", "core/src/icmp.c", "core/src/timer.c", "core/src/nic.c", "core/src/test/arp.c", "core/src/interface.c", "core/include/util/**.h", "core/src/asm.asm", "core/src/lock.c", "core/src/**.h" , "core/src/_malloc.c", "TLSF/src/**.h", "core/src/fifo.c", "core/src/map.c", "core/src/list.c", "core/src/set.c", "core/src/checksum.c" }
            -- Link testing target library
            linkoptions { "../../../libtlsf.a" }
            postbuildcommands {
                '{DELETE} %{cfg.buildtarget.abspath}.xml',
                '@export CMOCKA_XML_FILE=\'%{cfg.buildtarget.abspath}.xml\'; export CMOCKA_MESSAGE_OUTPUT=xml; %{cfg.buildtarget.abspath} ||:',
                '@export CMOCKA_MESSAGE_OUTPUT=stdout; %{cfg.buildtarget.abspath} ||:'
            }


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
          --]] 
