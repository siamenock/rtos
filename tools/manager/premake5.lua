project "Manager"
    kind "ConsoleApp"
    location "build"
    targetname "manager"
    targetdir "."

    buildoptions {
        "-idirafter ../../../kernel/src",
        "-std=gnu99",
        "-mcmodel=large",
        "-Wno-unused",
        "-Wno-format",
        "-Wno-unused-result",
        "-fno-builtin"
    }
    linkoptions { "-nostartfiles", "-Wl,-Ttext-segment=0xff00000000" }
    files { "src/**.h", "src/**.c", "src/**.asm" }
    includedirs { "../../lib/include", "." }
    libdirs {
        "../../../lib/ext",
        "../../../lib/vnic",
        "../../../lib/tlsf"
    }
    links { "ext", "rt", "vnic", "tlsf" }

    -- Make version header
    prebuildcommands {
        '../mkver.sh > ../src/version.h'
    }
