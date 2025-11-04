# VDMS Test Suite
After you are ready for commit, run the tests from the tests folder.
Make sure all the tests passes, if not, please fix the code, or the failing tests, or contact the person who wrote the test for help.
Make sure that when running the tests, nothing else is printed on the screen other than test information.

## Running the VDMS tests

There are two ways for running the current tests found in the tests directory of the VDMS repo:

1. Running the Python script called `tests/run_all_tests.py`. This script file can run 4 of the 5 types of existing tests:
      * Remote C++ tests
      * Local C++ tests
      * Remote Python tests
      * Local Python tests
2. Running the specific Shell script file for each type of tests:
      * Remote C++ tests: `tests/run_aws_tests.sh`
      * Local C++ tests: `tests/run_tests.sh`
      * Remote Python tests: `tests/python/run_python_aws_tests.sh`
      * Local Python tests: `tests/python/run_python_tests.sh`
      * Neo4j tests: `tests/run_neo4j_tests.sh`
<br>


## Run Tests Using run_all_tests.py

### Description
This script provides the commands for running and managing various types of tests, including the local C++ unit tests, remote C++ unit tests, local Python tests, remote Python tests, and Neo4j tests.
Details on the script are available in [VDMS Test Script](VDMS-Test-Script.md).
The script file uses the argparse library to handle command-line arguments and supports configuration through JSON files.

### Requirements
* Python 3.10+
* Dependencies (e.g., argparse, json, os, subprocess)

### Command line
To run the tests, use the following command-line arguments:
```bash
python run_all_tests.py [options]
```

The following options are available:

| Flag                                                                | Description                                                                                          |
| ------------------------------------------------------------------- | :--------------------------------------------------------------------------------------------------- |
| -a, --minio_port<br>(type: int)                                     | The port number to connect to the MinIO server.<br><br>*Example:* `python3 run_all_tests.py -a 9000` |
| -b, --run<br>(action: store_false, default: True)                   | If False then it validates the arguments, run the UDF scripts (if needed) but it doesn't run the tests.<br><br>*Example:* `python run_all_tests.py -b` |
| -c, --config_files_for_vdms<br>(type: str, action: append)          | Create a list of config files to be used by VDMS instances. <br><br>*Example:* `python3 run_all_tests.py -c config1.json -c config2.json` <br><br>**Note:** If you need to run several instances of VDMS servers, then please add the corresponding config file per each instance. |
| -d, --tmp_tests_dir<br>(type: str)                                  | Temporary dir for the files/dirs created by the tests.<br><br>*Example:* `python3 run_all_tests.py -d /tmp/tests`<br><br>**Note:** By default, the dir path will be `/tmp/tests_output_dir` |
| -e, --stderr_filename<br>(type: str)                                | Name of the file where the stderr messages are going to be written to.<br><br>*Example:* `python3 run_all_tests.py -e error.log`<br><br>**Note:** This file will be created inside of the `tmp_tests_dir` directory. |
| -g, --googletest_path<br>(type: str)                                | Path to the compiled binary of the tests used by googletest.<br><br>*Example:* `python run_all_tests.py -g /path/to/unittest`|
| -j, --json<br>(type: str)                                           | Path to the JSON config file where all the argument values can be found.<br><br>*Example:* `python3 run_all_tests.py -j config.json`<br><br>**Note:** If you choose to use the config.json instead of adding the arguments directly to the command line, please take into account that you don't need to specify to the command line those arguments already found in the JSON config file. In case that by mistake you add an argument to the command line and it is already included in the JSON config file, the duplicated argument found in the command line will have a higher precedence. You can find a template of the JSON config file in the `tests/default-config-testing.json` file. The valid values are the following:<br><pre><code>{<br>// Any relative path must be used by considering the root of the repo as starting point<br>    "test_name": "TestSuite.*",<br>    "type_of_test": "ut",  //["ut", "pt", "rp", "ru", "neo"]<br>    "keep_tmp_tests_dir": false,  //[false, true]<br>    "tmp_tests_dir": "/tmp/tests_output_dir",<br>    "neo4j_port": 7687,<br>    "neo4j_username": "",<br>    "neo4j_password": "",<br>    "vdms_app_path": "build/vdms",<br>    "config_files_for_vdms": ["tests/unit_tests/VDMS1_CONFIG_FILE.json", "tests/unit_tests/VDMS2_CONFIG_FILE.json"],<br>    "googletest_path": "build/tests/unit_tests",<br>    "stop_tests_on_fai<br>    "minio_app_path": "minio",<br>    "minio_port": 9000,<br>    "minio_console_port": 9001,<br>    "stderr_filename": "stderr_log.log",  // by default, the location is in "/tmp/tests_output_dir" dir<br>    "stdout_filename": "stdout_log.log",  // by default, the location is in "/tmp/tests_output_dir" dir<br>    "run": true//[false, true]<br>} </code></pre>|
| -k, --keep_tmp_tests_dir<br>(action: store_true, default: False)    | If True then it does not delete the temporary directory created for the tests.<br><br>*Example:* `python run_all_tests.py -k` |
| -m, --minio_app_path<br>(type: str)                                 | Path to the MinIO server app.<br><br>*Example:* `python run_all_tests.py -m /path/to/minio`          |
| -n, --test_name<br>(type: str)                                      | The name of the test or the pattern of the test names.<br><br>*Example for Python tests:* `python run_all_tests.py -n TestBoundingBox.TestBoundingBox.test_addBoundingBox`<br>*Example for C++ tests:* `python run_all_tests.py -n ImageTest.DefaultConstructor`<br><br>**Note:** This argument accepts the patterns used by Googletest (for C++ tests) and unittest (for Python tests) |
| -o, --stdout_filename<br>(type: str)                                | Name of the file where the stdout messages are going to be written to.<br><br>*Example:* `python run_all_tests.py -o output.log`<br><br>**Note:** This file will be created inside of the `tmp_tests_dir` directory. |
| -p, --minio_password<br>(type: str)                                 | The password used to connect to the MinIO server.<br><br>*Example:* `python run_all_tests.py -p password` |
| -r, --neo4j_port<br>(type: int)                                     | Port for Neo4j container.<br><br>*Example:* `python run_all_tests.py -r 7687`                        |
| -s, --stop_tests_on_failure<br>(action: store_true, default: False) | If True then, the execution of the rest of the tests will be skipped when one test fails (available only for googletest).<br><br>*Example:* `python run_all_tests.py -s` |
| -t, --type_of_test<br>(type: str)                                   | The type of the test: `ut` for local unit tests, `ru` for remote unit tests, `pt` for local Python tests, `rp` for remote Python tests, and `neo` for Neo4j tests.<br><br>*Example:* `python run_all_tests.py -t ut` |
| -u, --minio_username<br>(type: str)                                 | The username used to connect to the MinIO server.<br><br>*Example:* `python run_all_tests.py -u username` |
| -v, --vdms_app_path<br>(type: str)                                  | The path to the VDMS app.<br><br>*Example:* `python run_all_tests.py -v /path/to/vdms`               |
| -w, --neo4j_password<br>(type: str)                                 | This argument specifies the password required to connect to the Neo4j container<br><br>*Example:* `python run_all_tests.py -w mypassword` |
| -x, --neo4j_username<br>(type: str)                                 | This argument specifies the username required to connect to the Neo4j container.<br><br>*Example:* `python run_all_tests.py -x myusername` |
| -y, --minio_console_port<br>(type: int)                             | Console Port for Minio server.<br><br>*Example:* `python run_all_tests.py -y 9001`                   |
| -z, --neo4j_endpoint<br>(type: str)                                 | This argument specifies the endpoint URL for the Neo4j container.<br><br>*Example:* `python run_all_tests.py -z http://localhost:7474` |
<br>


## Run Tests Using the Shell script files
In case you are interested in running the specific Shell script file for a specific type of test suite

### Local Storage Tests
There are some Shell scripts used to run the C++, Neo4j, and Python tests for local storage.

#### Local C++ Tests

Run `sh run_tests.sh` from the `tests` directory for the C++ tests.
There are two special flags that you may use when calling `sh run_tests.sh`:

| Flag | Description |
| ---- | ----------- |
| -n   | This flag let you specify a pattern of the test names to be tested. <br> For example:<br><ul><li>`sh run_tests.sh -n "BLOB.find_Blob"`  The script runs the test called "BLOB.find_Blob" only</li><li>`sh run_tests.sh -n "ImageTest.*"`   The script runs all the tests which Test suite is called "ImageTest"</li><li>`sh run_tests.sh -n "*"`  The script runs everything, due to the single match-everything * value.</li><li>`sh run_tests.sh -n "*Null*:*Constructor*"`  The script runs any test whose full name contains either "Null" or "Constructor" .</li><li>`sh run_tests.sh -n "-*DeathTest.*"`  The script runs all non-death tests.</li><li>`sh run_tests.sh -n "FooTest.*-FooTest.Bar"`  The script runs everything in test suite FooTest except FooTest.Bar.</li><li>`sh run_tests.sh -n "FooTest.*:BarTest.*-FooTest.Bar:BarTest.Foo"`  The script runs everything in test suite FooTest except FooTest.Bar and everything in test suite BarTest except BarTest.Foo.</li></ul> |
| -s   | This flag let the script stop and skip the execution of the remaining tests in case any of the tests fails.<br>For example:<br><ul><li>`sh run_tests.sh -s`</li><ul> |

<br>

You should see on screen something like:
```
Running C++ tests...
[==========] Running 214 tests from 18 test suites.
[----------] Global test environment set-up.
[----------] 1 test from AutoReplicate
[ RUN      ] AutoReplicate.default_replicate
[       OK ] AutoReplicate.default_replicate (1414 ms)
[----------] 1 test from AutoReplicate (1414 ms total)

[----------] 1 test from ExampleHandler
[ RUN      ] ExampleHandler.simplePing
[       OK ] ExampleHandler.simplePing (86 ms)
[----------] 1 test from ExampleHandler (86 ms total)

[----------] 2 tests from AddImage
[ RUN      ] AddImage.simpleAdd
[       OK ] AddImage.simpleAdd (419 ms)
[ RUN      ] AddImage.simpleAddx10
[       OK ] AddImage.simpleAddx10 (570 ms)
[----------] 2 tests from AddImage (990 ms total)

[----------] 1 test from UpdateEntity
[ RUN      ] UpdateEntity.simpleAddUpdate
[       OK ] UpdateEntity.simpleAddUpdate (448 ms)
[----------] 1 test from UpdateEntity (448 ms total)

...

[----------] 10 tests from CLIENT_CPP_CSV
[ RUN      ] CLIENT_CPP_CSV.parse_csv_entity
[       OK ] CLIENT_CPP_CSV.parse_csv_entity (240 ms)
[ RUN      ] CLIENT_CPP_CSV.parse_csv_connection
[       OK ] CLIENT_CPP_CSV.parse_csv_connection (12 ms)
[ RUN      ] CLIENT_CPP_CSV.parse_csv_images
[       OK ] CLIENT_CPP_CSV.parse_csv_images (1662 ms)
[ RUN      ] CLIENT_CPP_CSV.parse_csv_descriptor_set
[       OK ] CLIENT_CPP_CSV.parse_csv_descriptor_set (27967 ms)
[ RUN      ] CLIENT_CPP_CSV.parse_csv_descriptor
[       OK ] CLIENT_CPP_CSV.parse_csv_descriptor (10 ms)
[ RUN      ] CLIENT_CPP_CSV.parse_csv_bb
[       OK ] CLIENT_CPP_CSV.parse_csv_bb (81 ms)
[ RUN      ] CLIENT_CPP_CSV.parse_csv_video
[       OK ] CLIENT_CPP_CSV.parse_csv_video (1495 ms)
[ RUN      ] CLIENT_CPP_CSV.parse_csv_invalid_entity
[       OK ] CLIENT_CPP_CSV.parse_csv_invalid_entity (1 ms)
[ RUN      ] CLIENT_CPP_CSV.parse_csv_invalid_image
Error: Failed to open file.
[       OK ] CLIENT_CPP_CSV.parse_csv_invalid_image (3 ms)
[ RUN      ] CLIENT_CPP_CSV.parse_csv_invalid_video
Failed to open file: ../tests/test_videos/Megamind_invalid.avi
[       OK ] CLIENT_CPP_CSV.parse_csv_invalid_video (2 ms)
[----------] 10 tests from CLIENT_CPP_CSV (31476 ms total)

[----------] 1 test from CLIENT_CPP_Video
[ RUN      ] CLIENT_CPP_Video.add_single_video
[       OK ] CLIENT_CPP_Video.add_single_video (1061 ms)
[----------] 1 test from CLIENT_CPP_Video (1061 ms total)

[----------] 3 tests from BLOB
[ RUN      ] BLOB.add_Blob
[       OK ] BLOB.add_Blob (40 ms)
[ RUN      ] BLOB.update_Blob
[       OK ] BLOB.update_Blob (8 ms)
[ RUN      ] BLOB.find_Blob
[       OK ] BLOB.find_Blob (3 ms)
[----------] 3 tests from BLOB (52 ms total)

[----------] Global test environment tear-down
[==========] 214 tests from 18 test suites ran. (896859 ms total)
[  PASSED  ] 214 tests.
```

<br>

#### Local Python Tests
Run `sh run_python_tests.sh` from the `tests/python` directory for the Python tests.
There is one special flag that you may use when calling `sh run_python_tests.sh`:

| Flag | Description |
| ---- | ----------- |
| -n   | This flag let you specify a pattern of the test names to be tested. It can be modules, classes or even individual test methods.<br>For example:<br><ul><li>`sh run_python_tests.sh -n "TestVideos.TestVideos.test_updateVideo"`  The script runs the test called `TestVideos.TestVideos.test_updateVideo` only.</li><li>`sh run_python_tests.sh -n "test_module1 test_module2"`  The script runs all the tests which belong to `test_module1` and `test_module2` modules.</li><li>`sh run_python_tests.sh -n "test_module.TestClass"`  The script runs all the tests which belong to `test_module.TestClass` class.</li><li>`sh run_python_tests.sh -n "test_module.TestClass.test_method"`  The script runs the test called `test_module.TestClass.test_method` only.</li><li>`sh run_python_tests.sh -n "TestBoundingBox.py"`  The script runs all the tests found in the file called `TestBoundingBox.py`.</li></ul> |

<br>

You should see on screen is something like:
```
Running Python tests...
test_addBoundingBox (TestBoundingBox.TestBoundingBox) ... ok
test_addBoundingBoxWithImage (TestBoundingBox.TestBoundingBox) ... ok
test_findBoundingBox (TestBoundingBox.TestBoundingBox) ... ok
test_findBoundingBoxBlob (TestBoundingBox.TestBoundingBox) ... ok
test_findBoundingBoxBlobComplex (TestBoundingBox.TestBoundingBox) ... ok
test_findBoundingBoxByCoordinates (TestBoundingBox.TestBoundingBox) ... ok
test_findBoundingBoxCoordinates (TestBoundingBox.TestBoundingBox) ... ok
test_findBoundingBoxesInImage (TestBoundingBox.TestBoundingBox) ... ok
test_updateBoundingBox (TestBoundingBox.TestBoundingBox) ... ok
test_updateBoundingBoxCoords (TestBoundingBox.TestBoundingBox) ... ok
test_FindEntity_link_constraints_float (TestConnections.TestConnections) ... ok
test_FindEntity_link_constraints_string (TestConnections.TestConnections) ... ok
test_addDescriptorsx1000 (TestDescriptors.TestDescriptors) ... ok
test_addSet (TestDescriptors.TestDescriptors) ... ok
test_addSetAndDescriptors (TestDescriptors.TestDescriptors) ... ok
test_addSetAndDescriptorsDimMismatch (TestDescriptors.TestDescriptors) ... ok
test_classifyDescriptor (TestDescriptors.TestDescriptors) ... ok
test_addDescriptorsx1000FaissIVFFlat (TestEngineDescriptors.TestDescriptors) ... ok
test_addDescriptorsx1000TileDBDense (TestEngineDescriptors.TestDescriptors) ... ok
test_addDescriptorsx1000TileDBSparse (TestEngineDescriptors.TestDescriptors) ... ok
test_addDifferentSets (TestEngineDescriptors.TestDescriptors) ... ok
test_FindWithSortBlock (TestEntities.TestEntities) ... ok
test_FindWithSortKey (TestEntities.TestEntities) ... ok
test_addEntityWithLink (TestEntities.TestEntities) ... ok
test_addFindEntity (TestEntities.TestEntities) ... ok
test_addfindEntityWrongConstraints (TestEntities.TestEntities) ... ok
test_runMultipleAdds (TestEntities.TestEntities) ... ok
test_addEntityWithBlob (TestEntitiesBlobs.TestEntitiesBlob) ... ok
test_addEntityWithBlobAndFind (TestEntitiesBlobs.TestEntitiesBlob) ... ok
test_addEntityWithBlobNoBlob (TestEntitiesBlobs.TestEntitiesBlob) ... ok
test_findDescByBlob (TestFindDescriptors.TestFindDescriptors) ... ok
test_findDescByBlobAndConstraints (TestFindDescriptors.TestFindDescriptors) ... ok
test_findDescByBlobNoLabels (TestFindDescriptors.TestFindDescriptors) ... ok
test_findDescByBlobNoResults (TestFindDescriptors.TestFindDescriptors) ... ok
test_findDescByBlobUnusedRef (TestFindDescriptors.TestFindDescriptors) ... ok
test_findDescByBlobWithLink (TestFindDescriptors.TestFindDescriptors) ... ok
test_findDescByConst_blobTrue (TestFindDescriptors.TestFindDescriptors) ... ok
test_findDescByConst_get_id (TestFindDescriptors.TestFindDescriptors) ... ok
test_findDescByConst_multiple_blobTrue (TestFindDescriptors.TestFindDescriptors) ... ok
test_findDescByConstraints (TestFindDescriptors.TestFindDescriptors) ... ok
test_findDescUnusedRef (TestFindDescriptors.TestFindDescriptors) ... ok
test_addImage (TestImages.TestImages) ... ok
test_addImageWithLink (TestImages.TestImages) ... ok
test_findEntityImage (TestImages.TestImages) ... ok
test_findImage (TestImages.TestImages) ... ok
test_findImageNoBlob (TestImages.TestImages) ... ok
test_findImageRefNoBlobNoPropsResults (TestImages.TestImages) ... ok
test_findImageResults (TestImages.TestImages) ... ok
test_findImage_multiple_results (TestImages.TestImages) ... ok
test_updateImage (TestImages.TestImages) ... ok
test_concurrent (TestRetail.TestEntities) ... skipped 'Skipping class until fixed'
test_create_connection (TestTestCommand.TestTestCommand) ... ok
test_disconnect (TestTestCommand.TestTestCommand) ... ok
test_vdms_existing_connection (TestVDMSClient.TestVDMSClient) ... ok
test_vdms_get_last_response (TestVDMSClient.TestVDMSClient) ... ok
test_vdms_get_last_response_str (TestVDMSClient.TestVDMSClient) ... ok
test_vdms_non_existing_connection (TestVDMSClient.TestVDMSClient) ... ok
test_vdms_non_json_query (TestVDMSClient.TestVDMSClient) ... ok
test_vdms_print_last_response (TestVDMSClient.TestVDMSClient) ... ok
test_vdms_query_disconnected (TestVDMSClient.TestVDMSClient) ... ok
test_FindFramesByFrames (TestVideos.TestVideos) ... ok
test_FindFramesByInterval (TestVideos.TestVideos) ... ok
test_FindFramesInvalidParameters (TestVideos.TestVideos) ... ok
test_FindFramesMissingParameters (TestVideos.TestVideos) ... ok
test_addVideo (TestVideos.TestVideos) ... ok
test_addVideoFromLocalFile_file_not_found (TestVideos.TestVideos) ... ok
test_addVideoFromLocalFile_invalid_command (TestVideos.TestVideos) ... ok
test_addVideoFromLocalFile_success (TestVideos.TestVideos) ... skipped 'Skipping class until fixed'
test_addVideoWithLink (TestVideos.TestVideos) ... ok
test_extractKeyFrames (TestVideos.TestVideos) ... ok
test_findVid_multiple_results (TestVideos.TestVideos) ... ok
test_findVideo (TestVideos.TestVideos) ... ok
test_findVideoNoBlob (TestVideos.TestVideos) ... ok
test_findVideoResults (TestVideos.TestVideos) ... ok
test_updateVideo (TestVideos.TestVideos) ... ok

----------------------------------------------------------------------
Ran 75 tests in 58.802s
```

<br>


#### C++ Neo4j Tests
Run `sh run_neo4j_tests.sh` from the `tests` directory for the C++ Neo4j tests.
There are a few flags available to run `sh run_neo4j_tests.sh`:

| Flag                     | Description |
| ------------------------ | ----------- |
| -h, --help               | Print this help message |
| -a, --minio_port         | API Port for S3/Minio server. Default is 9000 |
| -c, --minio_console_port | Console Port for S3/Minio server. Default is 9001 |
| -p, --minio_password     | Password for S3/Minio server |
| -u, --minio_username     | Username for S3/Minio server |
| -e, --neo4j_endpoint     | Neo4j endpoint |
| -v, --neo4j_port         | Port for Neo4j container. Default is 7687 |
| -w, --neo4j_password     | Password for Neo4j container |
| -n, --neo4j_username     | Username for Neo4j container |
| -t, --test_name          | Name of test to run [`OpsIOCoordinatorTest`, `Neo4JE2ETest`, `Neo4jBackendTest`, `NeoHandlerTest`] |

<br>

You should see results similar to:
```
Starting OpsIOCoordinatorTest...
Note: Google Test filter = OpsIOCoordinatorTest.*
[==========] Running 4 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 4 tests from OpsIOCoordinatorTest
[ RUN      ] OpsIOCoordinatorTest.PutObjTest
Warning: Using default endpoint_override
Instantiating global S3 Connection...
Global S3 Connection Started!
Added object 'test_obj' to bucket: minio-bucket
[       OK ] OpsIOCoordinatorTest.PutObjTest (6352 ms)
[ RUN      ] OpsIOCoordinatorTest.GetObjTest
Instantiating global S3 Connection...
Global S3 Connection Started!
Successfully retrieved 'test_obj' from 'minio-bucket'.
[       OK ] OpsIOCoordinatorTest.GetObjTest (6138 ms)
[ RUN      ] OpsIOCoordinatorTest.GetConnTest
Instantiating global S3 Connection...
Global S3 Connection Started!
[       OK ] OpsIOCoordinatorTest.GetConnTest (6040 ms)
[ RUN      ] OpsIOCoordinatorTest.DoOpsTest
Instantiating global S3 Connection...
Global S3 Connection Started!
14617
[       OK ] OpsIOCoordinatorTest.DoOpsTest (6103 ms)
[----------] 4 tests from OpsIOCoordinatorTest (24636 ms total)

[----------] Global test environment tear-down
[==========] 4 tests from 1 test suite ran. (24636 ms total)
[  PASSED  ] 4 tests.

Starting Neo4JE2ETest...
Note: Google Test filter = Neo4JE2ETest.*
[==========] Running 2 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 2 tests from Neo4JE2ETest
[ RUN      ] Neo4JE2ETest.E2E_Neo4j_Add_Find_Img
Warning: Using default endpoint_override
[       OK ] Neo4JE2ETest.E2E_Neo4j_Add_Find_Img (1677 ms)
[ RUN      ] Neo4JE2ETest.E2E_Neo4j_Add_Find_Metadata
Warning: Using default endpoint_override
[       OK ] Neo4JE2ETest.E2E_Neo4j_Add_Find_Metadata (210 ms)
[----------] 2 tests from Neo4JE2ETest (1887 ms total)

[----------] Global test environment tear-down
[==========] 2 tests from 1 test suite ran. (1888 ms total)
[  PASSED  ] 2 tests.
```

<br>

### Remote (S3 Storage) Tests
There are also scripts to run the C++ and Python tests for S3 storage.
For testing the S3 storage, we use MinIO and MinIO Client.
Prior to running this test, when using MinIO, it is necessary to install the MinIO components.
Assuming the codebase is located in `/vdms`, run the following:
```bash
curl -L -o /vdms/minio https://dl.min.io/server/minio/release/linux-amd64/minio
chmod +x /vdms/minio
mkdir -p /vdms/minio_files/minio-bucket


# Install the MinIO Client mc command line tool used by scripts for creating buckets
curl -o /usr/local/bin/mc https://dl.min.io/client/mc/release/linux-amd64/mc
chmod +x /usr/local/bin/mc
```
<br>

To properly run the testing scripts, we require the username for your S3/MinIO storage.
It is okay to use the default username (`AWS_ACCESS_KEY_ID`) and password (`AWS_SECRET_ACCESS_KEY`) for MinIO but we need these values for the test.

#### Remote C++ Tests
Run `sh run_aws_tests.sh -u ${AWS_ACCESS_KEY_ID} -p ${AWS_SECRET_ACCESS_KEY}` from the `tests` directory for the C++ tests.
There are a few special flags that you may use when calling `sh run_aws_tests.sh`:

| Flag | Description |
| ---- | ----------- |
| -a   | This flag specifies the api port for your S3/MinIO storage. The default is 9000 |
| -n   | This flag let you specify a pattern of the test names to be tested.<br>For example:<br><ul><li>`sh run_aws_tests.sh -u ${AWS_ACCESS_KEY_ID} -p ${AWS_SECRET_ACCESS_KEY} -n "RemoteConnectionTest.RemoteDisconnectedReadVideoFilename"`  The script runs the test called `RemoteConnectionTest.RemoteDisconnectedReadVideoFilename` only</li><li>`sh run_aws_tests.sh -u ${AWS_ACCESS_KEY_ID} -p ${AWS_SECRET_ACCESS_KEY} -n "RemoteConnectionTest.*"`   The script runs all the tests which Test suite is called `RemoteConnectionTest`</li><li>`sh run_aws_tests.sh -u ${AWS_ACCESS_KEY_ID} -p ${AWS_SECRET_ACCESS_KEY} -n "*"`  The script runs everything, due to the single match-everything value (`*`).</li><li>`sh run_aws_tests.sh -u ${AWS_ACCESS_KEY_ID} -p ${AWS_SECRET_ACCESS_KEY} -n "*Null*:*Constructor*"`  The script runs any test whose full name contains either `Null` or `Constructor` .</li><li>`sh run_aws_tests.sh -u ${AWS_ACCESS_KEY_ID} -p ${AWS_SECRET_ACCESS_KEY} -n "-*DeathTest.*"`  The script runs all non-death tests.</li><li>`sh run_aws_tests.sh -u ${AWS_ACCESS_KEY_ID} -p ${AWS_SECRET_ACCESS_KEY} -n "FooTest.*-FooTest.Bar"`  The script runs everything in test suite `FooTest` except `FooTest.Bar`.</li><li>`sh run_aws_tests.sh -u ${AWS_ACCESS_KEY_ID} -p ${AWS_SECRET_ACCESS_KEY} -n "FooTest.*:BarTest.*-FooTest.Bar:BarTest.Foo"`  The script runs everything in test suite `FooTest` except `FooTest.Bar` and everything in test suite `BarTest` except `BarTest.Foo`.</li></ul> |
| -p   | This flag specifies the password for your S3/MinIO storage |
| -s   | This flag let the script stop and skip the execution of the remaining tests in case any of the tests fails.<br>For example:<br><ul><li>`sh run_aws_tests.sh -s`</li></ul> |
| -u   | This flag specifies the username for your S3/MinIO storage |

<br>

You should see on screen is something like:
```
Running C++ tests...
Note: Google Test filter = RemoteConnectionTest.*
[==========] Running 19 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 19 tests from RemoteConnectionTest
[ RUN      ] RemoteConnectionTest.RemoteWriteFilename
Added object 'test_images/large1.jpg' to bucket: minio-bucket
[       OK ] RemoteConnectionTest.RemoteWriteFilename (6263 ms)
[ RUN      ] RemoteConnectionTest.RemoteReadWriteBuffer
Successfully retrieved 'test_images/large1.jpg' from 'minio-bucket'.
Added object 'test_images/large1.jpg' to bucket: minio-bucket
[       OK ] RemoteConnectionTest.RemoteReadWriteBuffer (6141 ms)
[ RUN      ] RemoteConnectionTest.RemoteListRetrieveFile
Successfully retrieved 'test_images/large1.jpg' from 'minio-bucket'.
[       OK ] RemoteConnectionTest.RemoteListRetrieveFile (6061 ms)
[ RUN      ] RemoteConnectionTest.RemoteWriteVideoFilename
Added object 'test_videos/Megamind.avi' to bucket: minio-bucket
[       OK ] RemoteConnectionTest.RemoteWriteVideoFilename (6172 ms)
[ RUN      ] RemoteConnectionTest.RemoteReadVideoFilename
Successfully retrieved 'test_videos/Megamind.avi' from 'minio-bucket'.
[       OK ] RemoteConnectionTest.RemoteReadVideoFilename (6071 ms)
[ RUN      ] RemoteConnectionTest.ImageRemoteWritePNG
Added object 'pngs/test_image.png' to bucket: minio-bucket
[       OK ] RemoteConnectionTest.ImageRemoteWritePNG (6228 ms)
[ RUN      ] RemoteConnectionTest.ImageRemoteReadPNG
Successfully retrieved 'pngs/test_image.png' from 'minio-bucket'.
[       OK ] RemoteConnectionTest.ImageRemoteReadPNG (6213 ms)
[ RUN      ] RemoteConnectionTest.ImageRemoteRemovePNG
[       OK ] RemoteConnectionTest.ImageRemoteRemovePNG (6050 ms)
[ RUN      ] RemoteConnectionTest.ImageRemoteWriteJPG
Added object 'jpgs/large1.jpg' to bucket: minio-bucket
[       OK ] RemoteConnectionTest.ImageRemoteWriteJPG (6124 ms)
[ RUN      ] RemoteConnectionTest.ImageRemoteReadJPG
Successfully retrieved 'jpgs/large1.jpg' from 'minio-bucket'.
[       OK ] RemoteConnectionTest.ImageRemoteReadJPG (6186 ms)
[ RUN      ] RemoteConnectionTest.ImageRemoteRemoveJPG
[       OK ] RemoteConnectionTest.ImageRemoteRemoveJPG (6052 ms)
[ RUN      ] RemoteConnectionTest.TDBImageWriteS3
[       OK ] RemoteConnectionTest.TDBImageWriteS3 (6219 ms)
[ RUN      ] RemoteConnectionTest.RemoteDisconnectedWriteFilename
WRITE: The RemoteConnection has not been started
[       OK ] RemoteConnectionTest.RemoteDisconnectedWriteFilename (6056 ms)
[ RUN      ] RemoteConnectionTest.RemoteDisconnectedReadBuffer
READ: The RemoteConnection has not been started
Added object 'test_images/large1.jpg' to bucket: minio-bucket
[       OK ] RemoteConnectionTest.RemoteDisconnectedReadBuffer (6064 ms)
[ RUN      ] RemoteConnectionTest.RemoteDisconnectedWriteBuffer
WRITE: The RemoteConnection has not been started
Successfully retrieved 'test_images/large1.jpg' from 'minio-bucket'.
[       OK ] RemoteConnectionTest.RemoteDisconnectedWriteBuffer (6059 ms)
[ RUN      ] RemoteConnectionTest.RemoteDisconnectedListFiles
WRITE: The RemoteConnection has not been started
[       OK ] RemoteConnectionTest.RemoteDisconnectedListFiles (6055 ms)
[ RUN      ] RemoteConnectionTest.RemoteDisconnectedRetrieveFile
WRITE: The RemoteConnection has not been started
[       OK ] RemoteConnectionTest.RemoteDisconnectedRetrieveFile (6058 ms)
[ RUN      ] RemoteConnectionTest.RemoteDisconnectedWriteVideoFilename
WRITE: The RemoteConnection has not been started
[       OK ] RemoteConnectionTest.RemoteDisconnectedWriteVideoFilename (6057 ms)
[ RUN      ] RemoteConnectionTest.RemoteDisconnectedReadVideoFilename
READ_Video: The RemoteConnection has not been started
[       OK ] RemoteConnectionTest.RemoteDisconnectedReadVideoFilename (6054 ms)
[----------] 19 tests from RemoteConnectionTest (116191 ms total)

[----------] Global test environment tear-down
[==========] 19 tests from 1 test suite ran. (116192 ms total)
[  PASSED  ] 19 tests.
```

<br>

#### Remote Python Tests
Run `sh run_python_aws_tests.sh -u ${AWS_ACCESS_KEY_ID} -p ${AWS_SECRET_ACCESS_KEY}` from the `tests/python` directory for the Python tests.
There is a few special flag that you may use when calling `sh run_python_aws_tests.sh`:

| Flag | Description |
| ---- | ----------- |
| -a   | This flag specifies the api port for your S3/MinIO storage. The default is 9000 |
| -n   | This flag let you specify a pattern of the test names to be tested. It can be modules, classes or even individual test methods.<br>For example:<br><ul><li>`sh run_python_aws_tests.sh -u ${AWS_ACCESS_KEY_ID} -p ${AWS_SECRET_ACCESS_KEY} -n "TestVideos.TestVideos.test_updateVideo"`  The script runs the test called `TestVideos.TestVideos.test_updateVideo` only.</li><li>`sh run_python_aws_tests.sh -u ${AWS_ACCESS_KEY_ID} -p ${AWS_SECRET_ACCESS_KEY} -n "test_module1 test_module2"`  The script runs all the tests which belong to `test_module1` and `test_module2` modules.</li><li>`sh run_python_aws_tests.sh -u ${AWS_ACCESS_KEY_ID} -p ${AWS_SECRET_ACCESS_KEY} -n "test_module.TestClass"`  The script runs all the tests which belong to `test_module.TestClass` class.</li><li>`sh run_python_aws_tests.sh -u ${AWS_ACCESS_KEY_ID} -p ${AWS_SECRET_ACCESS_KEY} -n "test_module.TestClass.test_method"`  The script runs the test called `test_module.TestClass.test_method` only.</li><li>`sh run_python_aws_tests.sh -u ${AWS_ACCESS_KEY_ID} -p ${AWS_SECRET_ACCESS_KEY} -n "TestBoundingBox.py"`  The script runs all the tests found in the file called `TestBoundingBox.py`.</li> |
| -p   | This flag specifies the password for your S3/MinIO storage |
| -u   | This flag specifies the username for your S3/MinIO storage |

<br>

You should see on screen is something like:
```
Running Python AWS S3 tests...
test_addBoundingBox (TestBoundingBox.TestBoundingBox) ... ok
test_addBoundingBoxWithImage (TestBoundingBox.TestBoundingBox) ... ok
test_findBoundingBox (TestBoundingBox.TestBoundingBox) ... ok
test_findBoundingBoxBlob (TestBoundingBox.TestBoundingBox) ... ok
test_findBoundingBoxBlobComplex (TestBoundingBox.TestBoundingBox) ... ok
test_findBoundingBoxByCoordinates (TestBoundingBox.TestBoundingBox) ... ok
test_findBoundingBoxCoordinates (TestBoundingBox.TestBoundingBox) ... ok
test_findBoundingBoxesInImage (TestBoundingBox.TestBoundingBox) ... ok
test_updateBoundingBox (TestBoundingBox.TestBoundingBox) ... ok
test_updateBoundingBoxCoords (TestBoundingBox.TestBoundingBox) ... ok
test_FindEntity_link_constraints_float (TestConnections.TestConnections) ... ok
test_FindEntity_link_constraints_string (TestConnections.TestConnections) ... ok
test_addDescriptorsx1000 (TestDescriptors.TestDescriptors) ... ok
test_addSet (TestDescriptors.TestDescriptors) ... ok
test_addSetAndDescriptors (TestDescriptors.TestDescriptors) ... ok
test_addSetAndDescriptorsDimMismatch (TestDescriptors.TestDescriptors) ... ok
test_classifyDescriptor (TestDescriptors.TestDescriptors) ... ok
test_addDescriptorsx1000FaissIVFFlat (TestEngineDescriptors.TestDescriptors) ... ok
test_addDescriptorsx1000TileDBDense (TestEngineDescriptors.TestDescriptors) ... ok
test_addDescriptorsx1000TileDBSparse (TestEngineDescriptors.TestDescriptors) ... ok
test_addDifferentSets (TestEngineDescriptors.TestDescriptors) ... ok
test_FindWithSortBlock (TestEntities.TestEntities) ... ok
test_FindWithSortKey (TestEntities.TestEntities) ... ok
test_addEntityWithLink (TestEntities.TestEntities) ... ok
test_addFindEntity (TestEntities.TestEntities) ... ok
test_addfindEntityWrongConstraints (TestEntities.TestEntities) ... ok
test_runMultipleAdds (TestEntities.TestEntities) ... ok
test_addEntityWithBlob (TestEntitiesBlobs.TestEntitiesBlob) ... ok
test_addEntityWithBlobAndFind (TestEntitiesBlobs.TestEntitiesBlob) ... ok
test_addEntityWithBlobNoBlob (TestEntitiesBlobs.TestEntitiesBlob) ... ok
test_findDescByBlob (TestFindDescriptors.TestFindDescriptors) ... ok
test_findDescByBlobAndConstraints (TestFindDescriptors.TestFindDescriptors) ... ok
test_findDescByBlobNoLabels (TestFindDescriptors.TestFindDescriptors) ... ok
test_findDescByBlobNoResults (TestFindDescriptors.TestFindDescriptors) ... ok
test_findDescByBlobUnusedRef (TestFindDescriptors.TestFindDescriptors) ... ok
test_findDescByBlobWithLink (TestFindDescriptors.TestFindDescriptors) ... ok
test_findDescByConst_blobTrue (TestFindDescriptors.TestFindDescriptors) ... ok
test_findDescByConst_get_id (TestFindDescriptors.TestFindDescriptors) ... ok
test_findDescByConst_multiple_blobTrue (TestFindDescriptors.TestFindDescriptors) ... ok
test_findDescByConstraints (TestFindDescriptors.TestFindDescriptors) ... ok
test_findDescUnusedRef (TestFindDescriptors.TestFindDescriptors) ... ok
test_addImage (TestImages.TestImages) ... ok
test_addImageWithLink (TestImages.TestImages) ... ok
test_findEntityImage (TestImages.TestImages) ... ok
test_findImage (TestImages.TestImages) ... ok
test_findImageNoBlob (TestImages.TestImages) ... ok
test_findImageRefNoBlobNoPropsResults (TestImages.TestImages) ... ok
test_findImageResults (TestImages.TestImages) ... ok
test_findImage_multiple_results (TestImages.TestImages) ... ok
test_updateImage (TestImages.TestImages) ... ok
test_concurrent (TestRetail.TestEntities) ... skipped 'Skipping class until fixed'
test_create_connection (TestTestCommand.TestTestCommand) ... ok
test_disconnect (TestTestCommand.TestTestCommand) ... ok
test_vdms_existing_connection (TestVDMSClient.TestVDMSClient) ... ok
test_vdms_get_last_response (TestVDMSClient.TestVDMSClient) ... ok
test_vdms_get_last_response_str (TestVDMSClient.TestVDMSClient) ... ok
test_vdms_non_existing_connection (TestVDMSClient.TestVDMSClient) ... ok
test_vdms_non_json_query (TestVDMSClient.TestVDMSClient) ... ok
test_vdms_print_last_response (TestVDMSClient.TestVDMSClient) ... ok
test_vdms_query_disconnected (TestVDMSClient.TestVDMSClient) ... ok
test_FindFramesByFrames (TestVideos.TestVideos) ... ok
test_FindFramesByInterval (TestVideos.TestVideos) ... ok
test_FindFramesInvalidParameters (TestVideos.TestVideos) ... ok
test_FindFramesMissingParameters (TestVideos.TestVideos) ... ok
test_addVideo (TestVideos.TestVideos) ... ok
test_addVideoFromLocalFile_file_not_found (TestVideos.TestVideos) ... ok
test_addVideoFromLocalFile_invalid_command (TestVideos.TestVideos) ... ok
test_addVideoFromLocalFile_success (TestVideos.TestVideos) ... skipped 'Skipping class until fixed'
test_addVideoWithLink (TestVideos.TestVideos) ... ok
test_extractKeyFrames (TestVideos.TestVideos) ... ok
test_findVid_multiple_results (TestVideos.TestVideos) ... ok
test_findVideo (TestVideos.TestVideos) ... ok
test_findVideoNoBlob (TestVideos.TestVideos) ... ok
test_findVideoResults (TestVideos.TestVideos) ... ok
test_updateVideo (TestVideos.TestVideos) ... ok

----------------------------------------------------------------------
Ran 75 tests in 320.375s
```
