sh cleandbs.sh

mkdir dbs  # necessary for Descriptors
mkdir temp # necessary for Videos
mkdir videos_tests
mkdir backups


echo 'not the vdms application - this file is needed for shared key' > vdms

# Gets coverage for files in ../src and ../include
# OMIT Descriptors_Add.add_1by1_and_search_1k due to duration
echo 'Running C++ tests...'
./../build/tests/unit_test \
    --gtest_filter=-ImageTest.CreateNameTDB:ImageTest.NoMetadata:VideoTest.CreateUnique:Descriptors_Add.add_1by1_and_search_1k

# echo 'Running Python tests...'
# cd python
# sh run_python_tests.sh
