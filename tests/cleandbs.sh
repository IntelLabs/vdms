rm -r jsongraph qhgraph simpleAdd_db simpleAddx10_db simpleUpdate_db
rm -r entitycheck_db datatypecheck_db db_backup test_db_1
rm tests_log.log tests_screen.log tests_remote_screen.log tests_remote_log.log tests_udf_screen.log tests_udf_log.log

rm -rf tdb/
rm -r db dbs test_db_client
rm -r temp
rm -r videos_tests
rm -r vdms
rm test_images/tdb_to_jpg.jpg
rm test_images/tdb_to_png.png
rm test_images/test_image.jpg
rm remote_function_test/tmpfile*
rm -r backups
echo 'Removing temporary files'
rm -rf ../minio_files
rm -rf ../test_db
rm -rf ../test_db_aws