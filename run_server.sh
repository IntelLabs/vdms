scons -j123
rm log.log screen.log dump.log
rm -r db
mkdir db
mkdir db/native_format
mkdir db/native_format/pngs
mkdir db/native_format/jpgs
mkdir db/native_format/descriptors
./vdms
