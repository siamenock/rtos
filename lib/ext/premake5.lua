project 'ext'
    kind 'StaticLib'

    build.compileProperty('x86_64')
    build.linkingProperty { 'hal', 'tlsf', 'vnic' }
    build.targetPath('..')

    buildoptions { "-fpic", "-msse4.1" }

    filter { 'configurations:linux' }
        defines     { 'LINUX' }
        removefiles { 'src/malloc.c' }
    filter {}

    postbuildcommands { '{COPY} -L include/* ../include' }

-- TODO: revive below
--include 'doc'
--include 'tests'

