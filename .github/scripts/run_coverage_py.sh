#!/bin/bash -e

cd /vdms/tests/python

./run_python_tests.sh
./run_python_aws_tests.sh -u ${AWS_ACCESS_KEY_ID} -p ${AWS_SECRET_ACCESS_KEY}
python -m coverage report -m 2>&1 | tee /vdms/tests/coverage_report/python.new.coverage_report.txt
python -m coverage xml -o /vdms/tests/coverage_report/python.new.coverage_report.xml

echo "DONE"

cat /vdms/tests/coverage_report/python.new.coverage_report.xml | grep "coverage version" | grep -oP 'line-rate="([-+]?\d*\.\d+|\d+)"' | grep -oP "[-+]?\d*\.\d+|\d+"| awk '{print $1*100}' > /vdms/tests/coverage_report/python.new.coverage_value.txt
