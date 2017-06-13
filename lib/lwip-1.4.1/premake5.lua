project "lwip"
    kind "StaticLib"

    buildoptions { '-fPIC' }
    build.compileProperty('x86_64')
    build.linkingProperty()
    build.targetPath('..')
    build.exportPath('lwip', { 'include/ipv4' })

    removefiles         { 'src/core/ipv6/**.c' }
    includedirs         { '../ext/include', '../vnic/include' }

    postbuildcommands   { '{COPY} -L include/* ../include' }
