#!/bin/bash
# Read in the file of environment settings
. /build/env.sh
# Then run the CMD
# exec "$@"
echo "$MAVEN_OPTS" > /tmp/testfile
MAVEN_OPTS="$MAVEN_OPTS"