#!/bin/bash -e

REPO_DIR=$(dirname "$(dirname "$(dirname "$(readlink -f "$0")")")")
echo "${REPO_DIR}"

# Run Clang-Format on C++ Code (Google C++ Style)
# TODO: Formatting src/vcl/DescriptorSet.cc causes build error
# sudo apt-get install clang-format
find "${REPO_DIR}" -type f -not -path "${REPO_DIR}/src/pmgd/*" \
    -not -path "${REPO_DIR}/build/*" \
    -not -path "${REPO_DIR}/src/vcl/DescriptorSet.cc" \
    -regex '.*\.\(cc\|cpp\|h\|hpp\)' | xargs clang-format -i

# Run Linter on Python Code
# python3 -m pip install --upgrade --no-cache-dir 'black>=23.1.0'
black ${REPO_DIR}/
