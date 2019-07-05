# We need to add this dependecy.
import os
AddOption('--no-server', action="store_false", dest="no-server",
                      default = True,
                      help='Only build client libraries')

AddOption('--timing', action='append_const', dest='cflags',
                      const='-DCHRONO_TIMING',
                      help= 'Build server with chronos')

AddOption('--prefix', dest='prefix',
                      type='string',
                      default='/usr/local/',
                      nargs=1,
                      action='store',
                      metavar='DIR',
                      help='installation prefix')

def buildServer(env):

  env.Append(
    CPPPATH= ['src', 'include', 'src/vcl', 'utils/include',
              '/usr/include/jsoncpp/',
              os.path.join(env['INTEL_PATH'], 'pmgd/include'),
              os.path.join(env['INTEL_PATH'], 'pmgd/util'),
             ],
    LIBS = [ 'pmgd', 'pmgd-util',
             'jsoncpp', 'protobuf', 'tbb',
             'vdms-utils', 'pthread',
             'tiledb',
             'opencv_core',
             'opencv_imgproc',
             'opencv_imgcodecs',
             'opencv_videoio',
             'opencv_highgui',
             'gomp',
             'faiss',
             'avcodec',
             'avformat',
             'avutil',
           ],

    LIBPATH = ['utils/',
               os.path.join(env['INTEL_PATH'], 'utils/'),
               os.path.join(env['INTEL_PATH'], 'pmgd/lib/'),
               ]
  )

  vdms_server_files = [
                  'src/vdms.cc',
                  'src/Server.cc',
                  'src/VDMSConfig.cc',
                  'src/QueryHandler.cc',
                  'src/QueryMessage.cc',
                  'src/CommunicationManager.cc',
                  'src/ExceptionsCommand.cc',
                  'src/PMGDQuery.cc',
                  'src/SearchExpression.cc',
                  'src/PMGDIterators.cc',
                  'src/PMGDQueryHandler.cc',
                  'src/RSCommand.cc',
                  'src/ImageCommand.cc',
                  'src/DescriptorsManager.cc',
                  'src/DescriptorsCommand.cc',
                  'src/BoundingBoxCommand.cc',
                  'src/VideoCommand.cc',
                  'src/vcl/utils.cc',
                  'src/vcl/Exception.cc',
                  'src/vcl/TDBObject.cc',
                  'src/vcl/Image.cc',
                  'src/vcl/TDBImage.cc',
                  'src/vcl/Video.cc',
                  'src/vcl/KeyFrameParser.cc',
                  'src/vcl/DescriptorSet.cc',
                  'src/vcl/DescriptorSetData.cc',
                  'src/vcl/FaissDescriptorSet.cc',
                  'src/vcl/TDBDescriptorSet.cc',
                  'src/vcl/TDBDenseDescriptorSet.cc',
                  'src/vcl/TDBSparseDescriptorSet.cc',
                ]

  env.Program('vdms', vdms_server_files)


# Set INTEL_PATH. First check arguments, then enviroment, then default
if ARGUMENTS.get('INTEL_PATH', '') != '':
    intel_path = ARGUMENTS.get("INTEL_PATH", '')
elif os.environ.get('INTEL_PATH', '') != '':
    intel_path = os.environ.get('INTEL_PATH', '')
else:
    intel_path = os.getcwd()

# Enviroment use by all the builds
env = Environment(CXXFLAGS="-std=c++11 -O3 -fopenmp")
env.Append(INTEL_PATH= intel_path)
env.MergeFlags(GetOption('cflags'))

prefix = str(GetOption('prefix'))

env.Alias('install-bin', env.Install(os.path.join(prefix, "bin"),
      source="vdms"))
env.Alias('install-client', env.Install(os.path.join(prefix, "lib"),
      source="client/cpp/libvdms-client.so"))
env.Alias('install-utils', env.Install(os.path.join(prefix, "lib"),
      source="utils/libvdms-utils.so"))
env.Alias('install', ['install-bin', 'install-client', 'install-utils'])

SConscript(os.path.join('utils', 'SConscript'), exports=['env'])
SConscript(os.path.join('client/cpp','SConscript'), exports=['env'])

if GetOption('no-server'):
    buildServer(env)
    # Build tests only if server is built
    SConscript(os.path.join('tests', 'SConscript'), exports=['env'])
