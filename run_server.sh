rm log.log
rm -r db

mkdir db
mkdir db/images
mkdir db/images/png
mkdir db/images/jpg
mkdir db/images/tiledb
mkdir db/blobs
mkdir db/descriptors

./vdms 2> log.log
