project 'collection'
    kind 'StaticLib'

    build.compileProperty('x86_64')
    build.linkingProperty()
    build.targetPath('..')

    buildoptions    '-fms-extensions'

