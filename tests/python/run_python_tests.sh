rm log.log screen.log dump.log
rm -r db
mkdir db
mkdir db/test-graph
mkdir db/images
mkdir db/images/pngs
mkdir db/images/jpgs
mkdir db/descriptors

../../athena -cfg config-tests.json > screen.log &
python main.py -v

sleep 1
pkill athena

dumpg test-graph > dump.log
