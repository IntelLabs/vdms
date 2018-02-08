# We need to add this dependecy.
import os
AddOption('--no-server', action="store_false", dest="no-server",
                      default = True,
                      help='Only build client libraries')
AddOption('--timing', action='append_const', dest='cflags',
                      const='-DCHRONO_TIMING',
                      help= 'Build server with chronos')

def buildServer(env):

  env.Append(
    CPPPATH= ['src', 'utils/include',
              '/usr/include/jsoncpp/',
              os.path.join(env['INTEL_PATH'], 'pmgd/include'),
              os.path.join(env['INTEL_PATH'], 'pmgd/util'),
              os.path.join(env['INTEL_PATH'], 'vcl/include'),
              os.path.join(env['INTEL_PATH'], 'vcl/src'),
             ],
    LIBS = [ 'pmgd', 'pmgd-util',
             'jsoncpp', 'protobuf',
             'vdms-utils', 'vcl', 'pthread',
           ],
    LIBPATH = ['/usr/local/lib/', 'utils/',
               os.path.join(env['INTEL_PATH'], 'utils/'),
               os.path.join(env['INTEL_PATH'], 'vcl/'),
               os.path.join(env['INTEL_PATH'], 'pmgd/lib/')
               ]
  )

  vdms_server_files = [
                  'src/vdms.cc',
                  'src/Server.cc',
                  'src/VDMSConfig.cc',
                  'src/QueryHandler.cc',
                  'src/QueryMessage.cc',
                  'src/CommunicationManager.cc',
                  'src/PMGDQuery.cc',
                  'src/SearchExpression.cc',
                  'src/PMGDIterators.cc',
                  'src/PMGDQueryHandler.cc',
                  'src/RSCommand.cc',
                  'src/ImageCommand.cc',
                  'src/ExceptionsCommand.cc',
                  'src/DescriptorsManager.cc',
                  'src/DescriptorsCommand.cc',
                ]

  vdms = env.Program('vdms', vdms_server_files)

# Set INTEL_PATH. First check arguments, then enviroment, then default
if ARGUMENTS.get('INTEL_PATH', '') != '':
  intel_path = ARGUMENTS.get("INTEL_PATH", '')
elif os.environ.get('INTEL_PATH', '') != '':
  intel_path = os.environ.get('INTEL_PATH', '')
else:
  intel_path = os.getcwd()

# Enviroment use by all the builds
env = Environment(CXXFLAGS="-std=c++11 -O3")
env.Append(INTEL_PATH= intel_path)
env.MergeFlags(GetOption('cflags'))

SConscript(os.path.join('utils', 'SConscript'), exports=['env'])
SConscript(os.path.join('client/cpp','SConscript'), exports=['env'])

if GetOption('no-server'):
  buildServer(env)
  # Build tests only if server is built
  SConscript(os.path.join('tests', 'SConscript'), exports=['env'])
