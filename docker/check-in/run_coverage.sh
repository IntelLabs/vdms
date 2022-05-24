cd /vdms/tests

chmod +x run_tests.sh
./run_tests.sh

gcovr --root /vdms \
    -e /vdms/src/pmgd -e /vdms/build/CMakeFiles \
    -f "/vdms/client/.*\.cc" -f "/vdms/ext/.*\.cc" -f "/vdms/src/.*\.cc" \
    -f src/SearchExpression.cc \
    --exclude-unreachable-branches \
    --txt=/vdms/tests/coverage_report/c_coverage_report.txt \
    --xml-pretty --xml=/vdms/tests/coverage_report/c_coverage_report.xml

echo "DONE"

cat /vdms/tests/coverage_report/c_coverage_report.txt
