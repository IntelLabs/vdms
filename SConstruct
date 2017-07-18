# We need to add this dependecy.
# utils = SConscript(['utils/SConstruct'])

intel_root='/opt/intel/'

env = Environment(CPPPATH= ['include', 'src',
                        'utils/include',
                        '/usr/include/jsoncpp/',
                        intel_root + 'jarvis/include',
                        intel_root + 'jarvis/util',
                        intel_root + 'vcl/include',
                        intel_root + 'utils/include',],
                        CXXFLAGS="-std=c++11 -O3")

athena_common_files = [
                'src/QueryHandler.cc',
                'src/SearchExpression.cc',
                'src/PMGDQueryHandler.cc',
                'src/CommandHandler.cc',
                ]

athena_server_files = [ 'src/athena.cc',
                        'src/Server.cc',
                        'src/CommunicationManager.cc',
                      ]

athena = env.Program('athena', [ athena_common_files, athena_server_files ] ,
            LIBS = [
                'jarvis', 'jarvis-util',
                'jsoncpp',
                'athena-utils', 'protobuf',
                'vclimage', 'pthread',
                'opencv_core', 'tiledb',
                'z' , 'crypto' ,
                'lz4' , 'zstd' ,
                'blosc', 'mpi',
                'opencv_imgcodecs',
                'opencv_highgui',
                'opencv_imgproc'
                ],
            LIBPATH = ['/usr/local/lib/',
                       intel_root + 'jarvis/lib/',
                       intel_root + 'vcl/Image/',
                       intel_root + 'utils/', # for athena-utils
                       ]
            )


testenv = Environment(CPPPATH = [ 'include', 'src', 'utils/include',
                        intel_root + 'jarvis/include',
                        intel_root + 'jarvis/util', ],
                        CXXFLAGS="-std=c++11 -O3")

test_sources = [ 'tests/main.cc',
                 'tests/pmgd_queries.cc',
                 'tests/json_query_test.cc'
               ]

query_tests = testenv.Program( 'tests/query_tests',
                                   [ athena_common_files, test_sources ],
                    LIBS = ['jarvis', 'jarvis-util', 'jsoncpp',
                            'athena-utils', 'protobuf', 'gtest', 'pthread' ],
                    LIBPATH = ['/usr/local/lib/',
                       intel_root + 'utils/', # for athena-utils
                       intel_root + 'jarvis/lib/' ]
                   )
