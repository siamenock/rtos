project 'hal'
    kind 'StaticLib'

    build.compileProperty('x86_64')
    build.linkingProperty()
    build.targetPath('..')

    postbuildcommands { '{COPY} -L include/* ../include' }
