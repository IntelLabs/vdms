# We need to add this dependecy.
import os

def buildServer(intel_path, env):

  athena_env = env.Clone()
  athena_env.Append(CPPPATH= ['include', 'src',
                '/usr/include/jsoncpp/',
                'utils/include',
                intel_path + 'jarvis/include',
                intel_path + 'jarvis/util',
                intel_path + 'vcl/include',
                intel_path + 'vcl/src',
                ])

  libs_paths = ['/usr/local/lib/', 'utils/',
               intel_path + 'utils/',
               intel_path + 'vcl/',
               intel_path + 'jarvis/lib/'
               ]

  athena_server_files = [
                  'src/AthenaConfig.cc',
                  'src/QueryHandler.cc',
                  'src/SearchExpression.cc',
                  'src/PMGDQueryHandler.cc',
                  'src/RSCommand.cc',
                  'src/CommandHandler.cc',
                  'src/athena.cc',
                  'src/Server.cc',
                  'src/CommunicationManager.cc',
                ]

  athena = athena_env.Program('athena', athena_server_files,
              LIBS = [
                  'jarvis', 'jarvis-util',
                  'jsoncpp',
                  'athena-utils', 'protobuf',
                  'vcl', 'pthread',
                  ],
              LIBPATH = libs_paths
              )

  testenv = Environment(CPPPATH = [ 'include', 'src', 'utils/include',
                          intel_path + 'jarvis/include',
                          intel_path + 'vcl/include',
                          intel_path + 'vcl/src',
                          intel_path + 'jarvis/util', ],
                          CXXFLAGS="-std=c++11 -g -O3")

  test_sources = ['tests/main.cc',
                  'tests/pmgd_queries.cc',
                  'tests/add_image.cc',
                  'tests/json_query_test.cc'
                 ]

  query_tests = testenv.Program(
                  'tests/query_tests',
                  ['src/QueryHandler.o',
                  'src/SearchExpression.o',
                  'src/AthenaConfig.o',
                  'src/RSCommand.o',
                  'src/PMGDQueryHandler.o',
                  'src/CommandHandler.o',
                  test_sources ],
                  LIBS = ['jarvis', 'jarvis-util', 'jsoncpp',
                          'athena-utils', 'protobuf', 'vcl',
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

# Enviroment use by all the builds
env = Environment(CXXFLAGS="-std=c++11 -O3")

SConscript(os.path.join('utils', 'SConscript'), exports=['env'])
SConscript(os.path.join('client','SConscript'), exports=['env'])

if not ARGUMENTS.get('BUILD_SERVER', False):
  buildServer(intel_path, env)
