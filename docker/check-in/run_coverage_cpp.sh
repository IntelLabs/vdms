#!/bin/bash -e

cd /vdms/tests

chmod +x run_tests.sh
echo 'Running run_tests.sh script'
./run_tests.sh
echo 'Checking for the available disk space due MinIO requires at least 1gb free'
df -h
chmod +x run_aws_tests.sh
echo 'Running run_aws_tests.sh script'
./run_aws_tests.sh -u ${AWS_ACCESS_KEY_ID} -p ${AWS_SECRET_ACCESS_KEY}

gcovr --root /vdms \
    -e /vdms/src/pmgd -e /vdms/build -e /vdms/distributed -e /vdms/tests \
    --gcov-ignore-parse-errors=negative_hits.warn_once_per_file \
    --gcov-ignore-errors=no_working_dir_found \
    -f "/vdms/.*/.*\.cc" -f "/vdms/.*/.*\.cpp" \
    --exclude-unreachable-branches \
    --txt=/vdms/tests/coverage_report/c_coverage_report.txt \
    --xml-pretty --xml=/vdms/tests/coverage_report/c_coverage_report.xml

echo "DONE"

cat /vdms/tests/coverage_report/c_coverage_report.txt
