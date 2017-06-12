project "Manager"
    kind "ConsoleApp"
    targetname "manager"

    buildoptions {
        "-idirafter ../../kernel/src",
        "-std=gnu99",
        "-mcmodel=large",
        "-Wno-unused",
        "-Wno-format",
        "-Wno-unused-result",
        "-fno-builtin"
    }
    linkoptions { "-nostartfiles", "-Wl,-Ttext-segment=0xff00000000" }
    includedirs { "../../lib/include", "." }
    libdirs {
        "../../lib/ext",
        "../../lib/vnic",
        "../../lib/tlsf"
    }
    links { "ext", "rt", "vnic", "tlsf" }
    build.compileProperty('x86_64')

    -- Make version header
    prebuildcommands {
        './mkver.sh > src/version.h'
    }
