sh clean.sh

../../vdms -cfg config-tests.json > screen.log 2> log.log &
python3.5 -m unittest discover --pattern=*.py -v

sleep 1
pkill vdms
