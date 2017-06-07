# We need to add this dependecy.
utils = SConscript(['utils/SConstruct'])

intel_root='/opt/intel/'

env = Environment(CPPPATH= ['include', 'src',
                        'utils/include',
                        intel_root + 'vcl/Image/include',
                        intel_root + 'jarvis/include',
                        intel_root + 'jarvis/util',
                        intel_root + 'utils/include',],
                        CXXFLAGS="-std=c++11 -O3")


source_files = ['src/AthenaDemoHLS.cc',
                'src/athenaServer.cc',
                'src/CommandHandler.cc',
                'src/CommunicationManager.cc',
                # 'src/QueryEngine.cc',
                'src/QueryHandler.cc',
                'src/Server.cc',
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
