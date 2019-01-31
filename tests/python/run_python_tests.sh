rm log.log screen.log
rm -r test_db

../../vdms -cfg config-tests.json > screen.log 2> log.log &
python -m unittest discover --pattern=*.py -v

sleep 1
pkill vdms
