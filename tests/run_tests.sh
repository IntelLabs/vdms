sh cleandbs.sh

mkdir dbs  # necessary for Descriptors
mkdir temp # necessary for Videos
mkdir videos_tests

echo 'not the vdms application - this file is needed for shared key' > vdms

# Gets coverage for files in ../src and ../include
echo 'Running C++ tests...'
./../build/tests/unit_test \
    --gtest_filter=-ImageTest.CreateNameTDB:ImageTest.NoMetadata:VideoTest.CreateUnique:Descriptors_Add.add_1by1_and_search_1k

#OMIT Descriptors_Add.add_1by1_and_search_1k due to duration

gcovr -k -p --root /vdms/ \
    -f /vdms/src \
    -f /vdms/include \
    -e /vdms/src/pmgd \
    -e "/vdms/.*\.h" \
    --exclude-unreachable-branches --txt \
    --xml-pretty --xml=c_coverage_report.xml

# echo 'Running Python tests...'
# cd python
# sh run_python_tests.sh
