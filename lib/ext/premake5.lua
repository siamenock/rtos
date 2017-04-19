project 'ext'
    kind 'StaticLib'

    build.compileProperty('x86_64')
    build.linkingProperty { 'hal', 'tlsf' }
    build.targetPath('..')

    filter { 'configurations:linux' }
        defines     { 'LINUX' }
        removefiles { 'src/malloc.c' }
    filter {}

    postbuildcommands { '{COPY} -L include/* ../include' }

-- TODO: revive below
--include 'doc'
--include 'tests'

