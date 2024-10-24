#!/bin/bash -e

cd /vdms/tests
chmod +x ./run_all_tests.py
chmod +x ./TestScript.py

# Check .coverage file doesn't exist
if [ -f './.coverage' ]; then
    echo ".coverage exists."
    rm ./.coverage
    echo ".coverage deleted."
fi

echo 'Running run_all_tests script for Python tests (-t=pt)'
python3 ./run_all_tests.py -t=pt

echo 'Running run_all_tests script for Remote Python tests (-t=rp)'
python3 ./run_all_tests.py -t=rp -u=${AWS_ACCESS_KEY_ID} -p=${AWS_SECRET_ACCESS_KEY}

echo 'Running the tests included in TestScript.py file'
# Append the coverage results to the ones obtained from running run_all_tests.py
python3 -m coverage run -a --omit="./run_all_tests.py,./TestScript.py" -m unittest discover --pattern=TestScript.py -v

python3 -m coverage report -m 2>&1 | tee /vdms/tests/coverage_report/python.new.coverage_report.txt
python3 -m coverage xml -o /vdms/tests/coverage_report/python.new.coverage_report.xml

echo "DONE"

cat /vdms/tests/coverage_report/python.new.coverage_report.xml | grep "coverage version" | grep -oP 'line-rate="([-+]?\d*\.\d+|\d+)"' | grep -oP "[-+]?\d*\.\d+|\d+"| awk '{print $1*100}' > /vdms/tests/coverage_report/python.new.coverage_value.txt
