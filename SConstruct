# We need to add this dependecy.
import os
AddOption('--no-server', action="store_false", dest="no-server",
                      default = True,
                      help='Only build client libraries')
AddOption('--timing', action='append_const', dest='cflags',
                      const='-DCHRONO_TIMING',
                      help= 'Build server with chronos')

def buildServer(intel_path, env):

  env.Append(
    CPPPATH= ['src', 'utils/include',
              '/usr/include/jsoncpp/',
              intel_path + 'jarvis/include',
              intel_path + 'jarvis/util',
              intel_path + 'vcl/include',
              intel_path + 'vcl/src',
             ],
    LIBS = [ 'jarvis', 'jarvis-util',
             'jsoncpp', 'protobuf',
             'athena-utils', 'vcl', 'pthread',
           ],
    LIBPATH = ['/usr/local/lib/', 'utils/',
               intel_path + 'utils/',
               intel_path + 'vcl/',
               intel_path + 'jarvis/lib/'
               ]
  )

  athena_server_files = [
                  'src/athena.cc',
                  'src/Server.cc',
                  'src/AthenaConfig.cc',
                  'src/QueryHandler.cc',
                  'src/QueryMessage.cc',
                  'src/CommunicationManager.cc',
                  'src/PMGDQuery.cc',
                  'src/SearchExpression.cc',
                  'src/PMGDQueryHandler.cc',
                  'src/RSCommand.cc',
                  'src/ImageCommand.cc',
                  'src/ExceptionsCommand.cc',
                ]

  athena = env.Program('athena', athena_server_files)

# Set INTEL_PATH. First check arguments, then enviroment, then default
if ARGUMENTS.get('INTEL_PATH', '') != '':
  intel_path = ARGUMENTS.get("INTEL_PATH", '')
elif os.environ.get('INTEL_PATH', '') != '':
  intel_path = os.environ.get('INTEL_PATH', '')
else:
  intel_path = './'

# Enviroment use by all the builds
env = Environment(CXXFLAGS="-std=c++11 -O3")
env.MergeFlags(GetOption('cflags'))

SConscript(os.path.join('utils', 'SConscript'), exports=['env'])
SConscript(os.path.join('client','SConscript'), exports=['env'])

if GetOption('no-server'):
  buildServer(intel_path, env)
  # Build tests only if server is built
  SConscript(os.path.join('tests', 'SConscript'), exports=['env'])
