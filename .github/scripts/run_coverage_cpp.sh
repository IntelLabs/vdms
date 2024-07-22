#!/bin/bash -e

cd /vdms/tests

# Run Local C++ PMGD Based Tests
chmod +x run_tests.sh
echo 'Running run_tests.sh script'
./run_tests.sh

# Run S3 C++ PMGD Based Tests
echo 'Checking for the available disk space due MinIO requires at least 1gb free'
df -h
chmod +x run_aws_tests.sh
echo 'Running run_aws_tests.sh script'
./run_aws_tests.sh -u ${AWS_ACCESS_KEY_ID} -p ${AWS_SECRET_ACCESS_KEY}

# Obtain Coverage
gcovr --root /vdms \
    -e /vdms/src/pmgd -e /vdms/build -e /vdms/distributed -e /vdms/tests \
    --gcov-ignore-parse-errors=negative_hits.warn_once_per_file \
    --gcov-ignore-errors=no_working_dir_found \
    -f "/vdms/.*/.*\.cc" -f "/vdms/.*/.*\.cpp" \
    --exclude-unreachable-branches \
    --exclude-noncode-lines \
    --txt=/vdms/tests/coverage_report/cpp.new.coverage_report.txt \
    --xml-pretty --xml=/vdms/tests/coverage_report/cpp.new.coverage_report.xml

echo "DONE"

cat /vdms/tests/coverage_report/cpp.new.coverage_report.xml | grep -oP 'coverage line-rate="([-+]?\d*\.\d+|\d+)"' | grep -oP "[-+]?\d*\.\d+|\d+" | awk '{print $1*100}' > /vdms/tests/coverage_report/cpp.new.coverage_value.txt
cat /vdms/tests/coverage_report/cpp.new.coverage_report.txt
