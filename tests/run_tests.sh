sh cleandbs.sh

mkdir dbs  # necessary for Descriptors
mkdir temp # necessary for Videos
mkdir videos_tests

./unit_test
echo 'Running python tests...'
cd python
sh run_python_tests.sh
