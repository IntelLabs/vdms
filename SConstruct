# We need to add this dependecy.
# utils = SConscript(['utils/SConstruct'])
import os

# Set INTEL_PATH. First check arguments, then enviroment, then default
if ARGUMENTS.get("INTEL_PATH", '') != '':
  intel_path = ARGUMENTS.get("INTEL_PATH", '')
elif os.environ.get('INTEL_PATH', '') != '':
  intel_path = os.environ.get('INTEL_PATH', '')
else:
  intel_path = './'

client = SConscript(['client/SConstruct'])

env = Environment(CPPPATH= ['include', 'src',
                        '/usr/include/jsoncpp/',
                        'utils/include',
                        intel_path + 'jarvis/include',
                        intel_path + 'jarvis/util',
                        intel_path + 'vcl/include',
                        ],
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
                       'utils/', # for athena-utils
                       intel_path + 'jarvis/lib/',
                       intel_path + 'vcl/',
                       ]
            )


testenv = Environment(CPPPATH = [ 'include', 'src', 'utils/include',
                        intel_path + 'jarvis/include',
                        intel_path + 'jarvis/util', ],
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
                       'utils/', # for athena-utils
                       intel_path + 'jarvis/lib/' ]
                   )
