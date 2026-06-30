#!/bin/sh
# This file is in the public domain.

# Die on failures, including in particular zzuf finding a crash.
set -eu

# Our settings, passed into the driver(s) via environment variables.
# We need to force the port so that zzuf knows what to grab.
export MHD_TEST_FORCE_SERVER_PORT=${MHD_TEST_FORCE_SERVER_PORT:-4010}
export MHD_TEST_FORCE_CLIENT_PORT=${MHD_TEST_FORCE_CLIENT_PORT:-4010}

# This sets the number of iterations of the regular tests we run with
# the fuzzer. Using a higher value than 16 could theoretically yield
# better coverage.
export MHD_TEST_FUZZING=${MHD_TEST_FUZZING:-16}

# List of tests to fuzz. Note that TLS-based tests do not fuzz well...
TESTS="test_client_server test_authentication test_postparser"

if ! command -v "${ZZUF:-zzuf}" > /dev/null 2>&1 ;
then
  echo "zzuf command missing" 1>&2
  exit 77
fi

for TEST in $TESTS
do
    make $TEST
    ${ZZUF:-zzuf} \
        --ratio=0.001:0.4 \
        --autoinc \
        --verbose \
        --signal \
        --ports=${MHD_TEST_FORCE_SERVER_PORT} \
        --max-usertime=${MAX_RUNTIME_SEC:-1800} \
        --network \
        --exclude=. \
        --jobs=1 \
        ${ZZUF_FLAGS:-} \
        ./.libs/$TEST
done
echo ""
echo "****************"
echo "ALL TESTS PASSED"
echo "****************"
exit 0
