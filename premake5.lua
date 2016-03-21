-- [[ Global configuration for every workspaces ]]

    -- Default language is C
    language "C"
    -- Maximum warning level
    warnings "Extra"
    -- We follows GNU99 standard. Do not use standard libary
    buildoptions { "-std=gnu99 -ffreestanding" }
    -- Do not link standard libary
    linkoptions { "-nostdlib" }

-- [[ Workspace 1. Build ]]
workspace "Build"
   -- Build workspace configuration 
   configurations { "Debug", "Release" }
    
   project "Build"
        kind "Makefile"

        buildcommands {
            "make -f Loader.make",
            "make -f Kernel.make",

            "make -C tools",

            'bin/smap kernel/build/kernel.elf kernel.smap',
            'bin/pnkc kernel/build/kernel.elf kernel.smap kernel.bin',
            'sudo tools/mkinitrd initrd.img 1 drivers/*.ko firmware/*',
            'tools/mkimage system.img 64 3 12 fat32 fat32 ext2 loader/build/loader.bin kernel.bin initrd.img',
        }

        cleancommands {
            "make -C tools",

            "make clean -f Loader.make",
            "make clean -f Kernel.make"
        }

-- [[ Workspace 2. Loader ]]
workspace "Loader"
    -- Loader workspace configurations 
    configurations { "Debug", "Release" }
    -- Loader is 32bit architecture
    architecture "x86"
    -- Define 32bit mode flag
    defines { "__MODE_32_BIT__" }
    -- Designate linker script. Beginning exection starts with 'start'
    linkoptions { "-T linker.ld -e start" }

    include "loader/loader.lua"

-- [[ Workspace 3. Kernel ]]
workspace "Kernel"
    -- Kernel workspace configurations 
    configurations { "Debug", "Release" }
    -- Kenrel is 64bit architecture
    architecture "x86_64"
    -- Define kernel flag
    defines { "_KERNEL_" }
    -- Memory model is for kernel
    buildoptions { "-mcmodel=kernel" }

    -- Library
    include "lib/library.lua"
    -- Kernel 
    include "kernel/kernel.lua"
    -- Drivers
    include "drivers/drivers.lua"
