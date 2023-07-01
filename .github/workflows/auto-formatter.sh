#!/bin/bash -e

check_package(){
    PACKAGE_TYPE=$1
    PACKAGE_NAME=$2

    if [ $PACKAGE_TYPE == "apt" ]; then
        if hash $PACKAGE_NAME 2>/dev/null; then
            echo "$PACKAGE_NAME exists!"
        else
            echo "Installing $PACKAGE_NAME"
            sudo apt-get install $PACKAGE_NAME
        fi
    elif [ $PACKAGE_TYPE == "python" ]; then
        if python3 -c "import $PACKAGE_NAME" 2>/dev/null; then
            echo "$PACKAGE_NAME exists!"
        else
            echo "Installing $PACKAGE_NAME"
            python3 -m pip install --upgrade --no-cache-dir '$PACKAGE_NAME'
        fi
    else
        echo "UNKNOWN Package type ($PACKAGE_TYPE). Choose apt or python"
        exit 1;
    fi
}

REPO_DIR=$(dirname "$(dirname "$(dirname "$(readlink -f "$0")")")")
echo "SCAN DIR: ${REPO_DIR}"

# Run Clang-Format on C++ Code (Google C++ Style)
check_package apt clang-format
find "${REPO_DIR}" -type f -not -path "${REPO_DIR}/src/pmgd/*" \
    -not -path "${REPO_DIR}/build/*" \
    -regex '.*\.\(cc\|cpp\|h\|hpp\)' | xargs clang-format -i

# Run Linter on Python Code
check_package python 'black>=23.1.0'
black ${REPO_DIR}/
