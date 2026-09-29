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

    # Extract CASTLE
    lcov --extract "$build_dir/coverage.raw.info" \
        "*/include/castle/*" \
        --output-file "$build_dir/castle.info" \
        --rc lcov_branch_coverage=1

    # Extract CASTLE_EXT
    lcov --extract "$build_dir/coverage.raw.info" \
        "*/include/castle_ext/*" \
        --output-file "$build_dir/castle_ext.info" \
        --rc lcov_branch_coverage=1

    # Merge both
    lcov -a "$build_dir/castle.info" \
        -a "$build_dir/castle_ext.info" \
        -o "$build_dir/all.info" \
        --rc lcov_branch_coverage=1

    lcov --list "$build_dir/castle.info" --rc lcov_branch_coverage=1
    lcov --list "$build_dir/castle_ext.info" --rc lcov_branch_coverage=1
    lcov --list "$build_dir/all.info" --rc lcov_branch_coverage=1

    if command -v genhtml >/dev/null 2>&1; then
        genhtml "$build_dir/all.info" \
            --output-directory "$cov_dir/all" \
            --branch-coverage \
            --legend \
            --prefix "$cur_pwd"

            genhtml "$build_dir/castle.info" \
            --output-directory "$cov_dir/castle" \
            --branch-coverage \
            --legend \
            --prefix "$cur_pwd"

            genhtml "$build_dir/castle_ext.info" \
            --output-directory "$cov_dir/castle_ext" \
            --branch-coverage \
            --legend \
            --prefix "$cur_pwd"
        echo "Coverage report: $cov_dir/index.html"
    fi
else
    echo "lcov not found: skipping coverage report generation." >&2
fi

cd "$cur_pwd"