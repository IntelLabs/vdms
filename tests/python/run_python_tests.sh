
rm -r test-graph
rm -r native_format
mkdir native_format
mkdir native_format/pngs
mkdir native_format/jpgs

../../athena -cfg config-tests.json > log.log &
python addImage.py
pkill athena