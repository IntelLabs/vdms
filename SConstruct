# We need to add this dependecy.
# utils = SConscript(['utils/SConstruct'])

intel_root='/opt/intel/'

env = Environment(CPPPATH= ['include', 'src',
                        'utils/include',
                        intel_root + 'vcl/Image/include',
                        intel_root + 'jarvis/include',
                        intel_root + 'jarvis/util',
                        intel_root + 'utils/include',],
                        CXXFLAGS="-std=c++11 -O3")


source_files = ['src/athena.cc',
                'src/Server.cc',
                'src/CommandHandler.cc',
                'src/CommunicationManager.cc',
                # 'src/QueryEngine.cc',
                'src/QueryHandler.cc',
                # 'src/AthenaDemoHLS.cc',
                # 'src/athenaServer.cc',
                ]

athena = env.Program('athena', source_files,
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

test_sources = ['tests/main.cc',
                'src/PMGDQueryHandler.cc',
                'tests/pmgd_queries.cc' ]

pmgd_query_test = testenv.Program( 'tests/pmgd_query_test',
                                    test_sources,
                    LIBS = ['jarvis', 'jarvis-util', 'jsoncpp',
                            'athena-utils', 'protobuf', 'gtest', 'pthread' ],
                    LIBPATH = ['/usr/local/lib/',
                       intel_root + 'utils/', # for athena-utils
                       intel_root + 'jarvis/lib/' ]
                   )
