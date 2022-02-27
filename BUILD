cc_library(
    name = 'array_orderbook',
    srcs = [
        'orderbook.cpp',
    ],
    deps = [
        '#pthread',
        '#rt',
        '#dl'
    ],
    defs = [
        'LINUX',
        '_PTHREADS',
        '_NEW_LIC',
        '_GNU_SOURCE',
        '_REENTRANT',
        '__USING_TCMALLOC__',
    ],
    optimize = [
        #'O2',
    ],
    extra_cppflags = [
        '-Wall',
        '-pipe',
        '-fPIC',
        '-Wno-deprecated',
        '-g',
        '-std=c++11',
    ],
    incs = [
        '',
    ],
    export_incs = ['.'],
)

cc_binary(
    name = 'array_orderbook_test',
    srcs = [
        'orderbook_test.cpp',
    ],
    deps = [
        ':array_orderbook',
        '//comm/util:commutil',
        '//comm/kit:kit',
    ],
    defs = [
        'LINUX',
        '_PTHREADS',
        '_NEW_LIC',
        '_GNU_SOURCE',
        '_REENTRANT',
    ],
    optimize = [
        #'O2',
    ],
    extra_cppflags = [
        '-Wall',
        '-pipe',
        '-fPIC',
        '-Wno-deprecated',
        '-g',
        '-std=c++11',
    ],
    incs = [
        '',
    ],
)

