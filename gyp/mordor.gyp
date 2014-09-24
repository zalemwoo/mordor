{
  'includes': ['common.gypi'],
  'make_global_settings': [
#    ['CXX','/usr/bin/clang++'],
#    ['LINK','/usr/bin/clang++'],
  ],
  'target_defaults': {
    'msvs_settings': {
#     'msvs_precompiled_header': '../mordor/pch.h',
#     'msvs_precompiled_source': '../mordor/pch.cpp',
      'VCCLCompilerTool': {
        'WarningLevel': '4', # /W4
      },
    },
    'xcode_settings': {
      'GCC_VERSION': 'com.apple.compilers.llvm.clang.1_0',
      'GCC_PREFIX_HEADER': '../mordor/pch.h',
      'GCC_PRECOMPILE_PREFIX_HEADER': 'YES',
      'CLANG_CXX_LANGUAGE_STANDARD': 'c++0x',
      'MACOSX_DEPLOYMENT_TARGET': '10.8', # OS X Deployment Target: 10.8
      'CLANG_CXX_LIBRARY': 'libc++', # libc++ requires OS X 10.7 or later
    },
	'conditions': [
      ['OS != "win"',{
        "link_settings": {
        "libraries": [
            '-L /usr/local/lib',
            '-lssl',
            '-lcrypto',
            '-lboost_thread',
            '-lboost_system',
            '-llzma',
            '-lz',
            '-lpthread',
          ],
        },
        'cflags': ['-include ../mordor/pch.h'],
        'include_dirs': [
          '..',
        ],
      }],
    ],
  },
  'targets': [
    {
      'target_name': 'mordor_pch',
      'product_name': 'pch.h.gch',
      'type': 'none',
      'cflags': ['-x c++-header'],
      'sources': [
        '../mordor/pch.h',
        '../mordor/pch.cpp',
      ]
    },
    {
      'target_name': 'mordor_lib',
      'product_name': 'mordor',
      'type': 'static_library',
      'dependencies': [
        'mordor_pch',
      ],
      'sources': [
        '../mordor/assert.cpp',
        '../mordor/string.cpp',
        '../mordor/statistics.cpp',
        '../mordor/sleep.cpp',
        '../mordor/fiber.cpp',
#       '../mordor/yaml.cpp',
        '../mordor/thread.cpp',
        '../mordor/openssl_lock.cpp',
        '../mordor/workerpool.cpp',
        '../mordor/cxa_exception.cpp',
        '../mordor/main.cpp',
        '../mordor/exception.cpp',
        '../mordor/timer.cpp',
        '../mordor/semaphore.cpp',
        '../mordor/assert.cpp',
        '../mordor/fibersynchronization.cpp',
        '../mordor/zip.cpp',
        '../mordor/ragel.cpp',
        '../mordor/date_time.cpp',
        '../mordor/protobuf.cpp',
        '../mordor/iomanager_epoll.cpp',
        '../mordor/scheduler.cpp',
        '../mordor/config.cpp',
        '../mordor/util.cpp',
        '../mordor/socks.cpp',
        '../mordor/parallel.cpp',
        '../mordor/log.cpp',
        '../mordor/daemon.cpp',
        '../mordor/socket.cpp',
        '../mordor/streams/counter.cpp',
        '../mordor/streams/buffer.cpp',
        '../mordor/streams/hash.cpp',
        '../mordor/streams/memory.cpp',
        '../mordor/streams/lzma2.cpp',
        '../mordor/streams/temp.cpp',
        '../mordor/streams/timeout.cpp',
        '../mordor/streams/zero.cpp',
        '../mordor/streams/fd.cpp',
        '../mordor/streams/limited.cpp',
        '../mordor/streams/http.cpp',
        '../mordor/streams/singleplex.cpp',
        '../mordor/streams/random.cpp',
        '../mordor/streams/buffered.cpp',
        '../mordor/streams/cat.cpp',
        '../mordor/streams/filter.cpp',
        '../mordor/streams/null.cpp',
        '../mordor/streams/pipe.cpp',
        '../mordor/streams/crypto.cpp',
        '../mordor/streams/file.cpp',
        '../mordor/streams/transfer.cpp',
        '../mordor/streams/stream.cpp',
        '../mordor/streams/throttle.cpp',
        '../mordor/streams/zlib.cpp',
        '../mordor/streams/ssl.cpp',
        '../mordor/streams/std.cpp',
        '../mordor/streams/test.cpp',
        '../mordor/streams/socket.cpp',
        '../mordor/http/basic.cpp',
        '../mordor/http/chunked.cpp',
        '../mordor/http/client.cpp',
        '../mordor/http/oauth2.cpp',
        '../mordor/http/http.cpp',
        '../mordor/http/oauth.cpp',
        '../mordor/http/servlet.cpp',
        '../mordor/http/auth.cpp',
        '../mordor/http/server.cpp',
        '../mordor/http/broker.cpp',
        '../mordor/http/digest.cpp',
        '../mordor/http/connection.cpp',
        '../mordor/http/servlets/config.cpp',
        '../mordor/http/multipart.cpp',
        '../mordor/http/proxy.cpp',
        '../mordor/xml/dom_parser.cpp',
      ],
      'conditions': [
        ['OS == "win"', {
          'sources':[
            '../mordor/eventloop.cpp',
            '../mordor/runtime_linking.cpp',
            '../mordor/iomanager_iocp.cpp',
            '../mordor/streams/handle.cpp',
            '../mordor/streams/efs.cpp',
            '../mordor/streams/namedpipe.cpp',
            '../mordor/http/negotiate.cpp',
          ]
        }],
        ['OS == "mac"', {
          'sources':[
            '../mordor/iomanager_kqueue.cpp',
          ]
        }],
      ],
      'xcode_settings': {
        'GCC_INLINES_ARE_PRIVATE_EXTERN': 'YES', # '-fvisibility-inlines-hidden'
        'GCC_SYMBOLS_PRIVATE_EXTERN': 'YES', # '-fvisibility=hidden'
      },
    },
    {
      'target_name': 'mordor_test',
      'product_name': 'mordortest',
      'type': 'static_library',
      'dependencies': [
        'mordor_lib',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        '../mordor/test/stdoutlistener.cpp',
        '../mordor/test/antxmllistener.cpp',
        '../mordor/test/test.cpp',
        '../mordor/test/compoundlistener.cpp',
      ],
      'xcode_settings': {
      },
    }, # mordor_test
    {
      'target_name': 'tests',
      'product_name': 'tests',
      'type': 'executable',
      'dependencies': [
        'mordor_lib',
        'mordor_test',
      ],
      'sources': [
        '../mordor/tests/atomic.cpp',
        '../mordor/tests/coroutine.cpp',
        '../mordor/tests/fibers.cpp',
        '../mordor/tests/fibersync.cpp',
        '../mordor/tests/log.cpp',
        '../mordor/tests/stream.cpp',
        '../mordor/tests/socket.cpp',
        '../mordor/tests/thread.cpp',
        '../mordor/tests/zlib.cpp',
        '../mordor/tests/run_tests.cpp',
      ],
    }, # tests
    {
      'target_name': 'cat',
      'product_name': 'cat',
      'type': 'executable',
      'dependencies': [
        'mordor_lib',
      ],
      'sources': [
        '../mordor/examples/cat.cpp',
      ],
    }, # cat
  ] # targets
}
