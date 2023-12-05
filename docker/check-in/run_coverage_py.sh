#!/bin/bash -e

cd /vdms/tests/python

./run_python_tests.sh
./run_python_aws_tests.sh -u ${AWS_ACCESS_KEY_ID} -p ${AWS_SECRET_ACCESS_KEY}
python -m coverage report -m 2>&1 | tee /vdms/tests/coverage_report/py_coverage_report.txt
python -m coverage xml -o /vdms/tests/coverage_report/py_coverage_report.xml

echo "DONE"
