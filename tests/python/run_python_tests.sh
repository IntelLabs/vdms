sh clean.sh

../../vdms -cfg config-tests.json > screen.log 2> log.log &
python main.py -v

sleep 1
pkill vdms
