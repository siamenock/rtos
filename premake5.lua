-- [[ Global configuration for every workspaces ]]

    -- Default language is C
    language "C"
    -- Maximum warning level
    warnings "Extra"
    -- We follows GNU99 standard. 
    buildoptions { "-std=gnu99 -Wno-unused-parameter" }

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
            -- NOTE: assume that initrd.img < 4MB
            'sudo bin/mkinitrd initrd.img 4 drivers/*.ko firmware/*',
            'dd if=/dev/zero of=system.img bs=512 count=131072',	-- 64MB
            -- NOTE: assume that initrd.img + grub files < 5MB
            'bin/mkimage system.img 5 12 fat32 fat32 ext2 loader/build/loader.bin kernel.bin initrd.img',
        }

        cleancommands {
            "make clean -C tools",

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
    -- Do not link standard library. Designate linker script. Beginning exection starts with 'start'
    linkoptions { "-nostdlib -T linker.ld -e start" }
    -- Do not use standard libary
    buildoptions { "-ffreestanding" }

    include "loader/loader.lua"

-- [[ Workspace 3. Kernel ]]
workspace "Kernel"
    -- Kernel workspace configurations 
    configurations { "Debug", "Release" }
    -- Kenrel is 64bit architecture
    architecture "x86_64"
    -- Define kernel flag
    defines { "_KERNEL_" }
    -- Do not use standard libary. Memory model is for kernel
    buildoptions { "-ffreestanding -mcmodel=kernel" }
    -- Do not link standard library.
    linkoptions { "-nostdlib" }

    -- Library
    include "lib/library.lua"
    -- Kernel 
    include "kernel/kernel.lua"
    -- Drivers
    include "drivers/drivers.lua"

-- [[ Workspace 4. Unit Test ]]
workspace "Test"
    -- Test workspace configurations 
    configurations { "Debug" }
    -- Test is for 64bit architecture source
    architecture "x86_64"

    -- Library test
    include "lib/library_test.lua"

