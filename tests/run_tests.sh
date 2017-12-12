cd .. && scons -j123 && cd tests
sh cleandbs.sh
./query_tests
echo 'Running python tests...'
cd python
sh run_python_tests.sh
