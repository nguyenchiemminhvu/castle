#!/bin/bash
# ---------------------------------------------------------------------------
# build_test.sh
# ---------------------------------------------------------------------------
# Configure, build and run the CASTLE unit-test suite, then produce an lcov
# HTML coverage report under build/coverage/.
# ---------------------------------------------------------------------------
set -euo pipefail

cur_pwd=$(pwd)
build_dir="$cur_pwd/build"
cov_dir="$build_dir/coverage"

mkdir -p "$build_dir"

# ---------------------------------------------------------------------------
# Configure & build
# ---------------------------------------------------------------------------
cmake -S "$cur_pwd" -B "$build_dir" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCASTLE_BUILD_TESTING=ON \
    -DCASTLE_FETCH_GTEST=ON \
    -DCASTLE_ENABLE_COVERAGE=ON

cmake --build "$build_dir" --target castle_tests -j$(nproc)

# ---------------------------------------------------------------------------
# Run tests
# ---------------------------------------------------------------------------
ctest --test-dir "$build_dir" --output-on-failure -j$(nproc) || true

# ---------------------------------------------------------------------------
# Coverage (lcov)
# ---------------------------------------------------------------------------
if command -v lcov >/dev/null 2>&1; then
    mkdir -p "$cov_dir"

    lcov --directory "$build_dir" --capture \
        --output-file "$build_dir/coverage.raw.info" \
        --rc lcov_branch_coverage=1

    # Keep only the CASTLE public headers in the report.
    lcov --extract "$build_dir/coverage.raw.info" \
        "*/include/castle/*" \
        --output-file "$build_dir/castle.info" \
        --rc lcov_branch_coverage=1

    lcov --list "$build_dir/castle.info" --rc lcov_branch_coverage=1

    if command -v genhtml >/dev/null 2>&1; then
        genhtml "$build_dir/castle.info" \
            --output-directory "$cov_dir" \
            --branch-coverage \
            --legend
        echo "Coverage report: $cov_dir/index.html"
    fi
else
    echo "lcov not found: skipping coverage report generation." >&2
fi

cd "$cur_pwd"