cd /vdms/tests/python

./run_python_tests.sh
python -m coverage report -m 2>&1 | tee /vdms/tests/coverage_report/py_coverage_report.txt
python -m coverage xml -o /vdms/tests/coverage_report/py_coverage_report.xml

echo "DONE"