rm log.log screen.log dump.log
rm -r db
mkdir db
mkdir db/test-graph
mkdir db/images
mkdir db/images/pngs
mkdir db/images/jpgs
mkdir db/descriptors

../../vdms -cfg config-tests.json > screen.log &
python main.py -v

sleep 1
pkill vdms

dumpg test-graph > dump.log
