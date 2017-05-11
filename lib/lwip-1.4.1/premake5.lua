project "lwip"
    kind "StaticLib"

    build.compileProperty('x86_64')
    build.linkingProperty()
    build.targetPath('..')
    build.exportPath('lwip', { 'include/ipv4' })

    removefiles         { 'src/core/ipv6/**.c' }
    includedirs         { '../ext/include' }

    postbuildcommands   { '{COPY} -L include/* ../include' }
