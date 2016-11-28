-- [[ Premake script for kernel workspace ]] 
workspace "Kernel"
    -- [[ 1. Linux compability layer ]]
    project "linux"
        kind "ConsoleApp"
        location "linux/build"
        targetname "linux.ko"
        targetdir "."
        -- O0 optimziation
        optimize "On"
        includedirs { "../kernel/src", "../lib/core/include", "./linux" }
        linkoptions { "-r" }
        files { "linux/**.h", "linux/**.c" }
        removefiles { "linux/packetngin/**.h", "linux/packetngin/**.c" }

    -- [[ 2. VirtI/O ]]
    project "virtio"
        kind "ConsoleApp"
        location "virtio/build"
        targetname "virtio.ko"
        targetdir "."
        -- O0 optimziation
        optimize "On"
        includedirs { "../kernel/src", "../kernel/src/driver", "../lib/core/include" }
        files { "virtio/**.h", "virtio/**.c" }
        linkoptions { "-r ../../linux.ko" }

    -- [[ 3. FAT ]]
    project "fat"
        kind "ConsoleApp"
        location "fat/build"
        targetname "fat.ko"
        targetdir "."
        -- O0 optimziation
        optimize "On"
        includedirs { "../kernel/src", "../kernel/src/driver", "../lib/core/include" }
        files { "virtio/**.h", "virtio/**.c" }
        files { "fat/**.h", "fat/**.c" }
        linkoptions { "-r ../../linux.ko" }
