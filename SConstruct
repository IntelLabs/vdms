# We need to add this dependecy.
import os


def buildServer(intel_path, lib_paths):
  env = Environment(CPPPATH= ['include', 'src',
                          '/usr/include/jsoncpp/',
                          'utils/include',
                          intel_path + 'jarvis/include',
                          intel_path + 'jarvis/util',
                          intel_path + 'vcl/include',
                          ],
                          CXXFLAGS="-std=c++11 -O3")

  athena_server_files = [
                  'src/QueryHandler.cc',
                  'src/SearchExpression.cc',
                  'src/PMGDQueryHandler.cc',
                  'src/CommandHandler.cc',
                  'src/athena.cc',
                  'src/Server.cc',
                  'src/CommunicationManager.cc',
                ]

  athena = env.Program('athena', athena_server_files,
              LIBS = [
                  # utils,
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
              LIBPATH = libs_paths
              )

  testenv = Environment(CPPPATH = [ 'include', 'src', 'utils/include',
                          intel_path + 'jarvis/include',
                          intel_path + 'vcl/include',
                          intel_path + 'jarvis/util', ],
                          CXXFLAGS="-std=c++11 -O3")

  test_sources = [ 'tests/main.cc',
                   'tests/pmgd_queries.cc',
                   'tests/json_query_test.cc'
                 ]

  query_tests = testenv.Program(
                  'tests/query_tests',
                  ['src/QueryHandler.o',
                  'src/SearchExpression.o',
                  'src/PMGDQueryHandler.o',
                  'src/CommandHandler.o',
                  test_sources ],
                  LIBS = ['jarvis', 'jarvis-util', 'jsoncpp',
                          'athena-utils', 'protobuf', 'vclimage',
                          'gtest', 'pthread' ],
                  LIBPATH = libs_paths
                 )

# Set INTEL_PATH. First check arguments, then enviroment, then default
if ARGUMENTS.get('INTEL_PATH', '') != '':
  intel_path = ARGUMENTS.get("INTEL_PATH", '')
elif os.environ.get('INTEL_PATH', '') != '':
  intel_path = os.environ.get('INTEL_PATH', '')
else:
  intel_path = './'

libs_paths = ['/usr/local/lib/',
               intel_path + 'athena/utils/',
               intel_path + 'vcl/',
               intel_path + 'jarvis/lib/' ]

utils  = SConscript(os.path.join('utils', 'SConscript'))
client = SConscript(os.path.join('client','SConscript'))

if not ARGUMENTS.get('BUILD_SERVER', False):
  buildServer(intel_path, libs_paths)



