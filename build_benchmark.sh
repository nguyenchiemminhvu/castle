#!/usr/bin/env bash

set -euo pipefail

# =============================================================================
# CASTLE Benchmark Build & Run
#
# Usage:
#
#   ./benchmarks/build.sh
#   ./benchmarks/build.sh build
#   ./benchmarks/build.sh run
#   ./benchmarks/build.sh clean
#   ./benchmarks/build.sh clean-build
#
# Optional environment variables:
#
#   BUILD_TYPE=Debug ./benchmarks/build.sh
#   BUILD_TYPE=Release BUILD_JOBS=8 ./benchmarks/build.sh
#
# =============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CASTLE_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

BUILD_DIR="${SCRIPT_DIR}/build"
RESULTS_DIR="${BUILD_DIR}/results"

BUILD_TYPE="${BUILD_TYPE:-Release}"

if [[ -z "${BUILD_JOBS:-}" ]]; then
    if command -v nproc >/dev/null 2>&1; then
        BUILD_JOBS="$(nproc)"
    else
        BUILD_JOBS=4
    fi
fi

ACTION="${1:-all}"

# =============================================================================
# Helpers
# =============================================================================

print_header()
{
    echo
    echo "================================================================"
    echo " CASTLE Benchmarks"
    echo "================================================================"
    echo
}

print_config()
{
    echo "Castle root : ${CASTLE_ROOT}"
    echo "Source      : ${SCRIPT_DIR}"
    echo "Build       : ${BUILD_DIR}"
    echo "Results     : ${RESULTS_DIR}"
    echo "Build type  : ${BUILD_TYPE}"
    echo "Jobs        : ${BUILD_JOBS}"
    echo
}

# =============================================================================
# Configure
# =============================================================================

configure()
{
    echo
    echo "==> Configuring benchmarks"
    echo

    cmake \
        -S "${SCRIPT_DIR}" \
        -B "${BUILD_DIR}" \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
        -DCASTLE_BENCHMARK_ENABLE_STL=ON \
        -DCASTLE_BENCHMARK_ENABLE_WARNINGS=ON

    echo
}

# =============================================================================
# Build
# =============================================================================

build()
{
    echo
    echo "==> Building benchmarks"
    echo

    cmake \
        --build "${BUILD_DIR}" \
        --parallel "${BUILD_JOBS}"

    echo
}

# =============================================================================
# Find benchmark executables
# =============================================================================

find_benchmarks()
{
    local bin_dir="${BUILD_DIR}/bin"

    if [[ ! -d "${bin_dir}" ]]; then
        echo "ERROR: Benchmark binary directory does not exist:"
        echo "  ${bin_dir}"
        return 1
    fi

    find "${bin_dir}" \
        -maxdepth 1 \
        -type f \
        -executable \
        -name 'castle_benchmark_*' \
        -print \
        | sort
}

# =============================================================================
# Run one benchmark
# =============================================================================

run_one()
{
    local executable="$1"
    local name
    local output

    name="$(basename "${executable}")"
    output="${RESULTS_DIR}/${name}.json"

    echo
    echo "----------------------------------------------------------------"
    echo " ${name}"
    echo "----------------------------------------------------------------"
    echo

    "${executable}" \
        --benchmark_color=true \
        --benchmark_min_time=0.5 \
        --benchmark_repetitions=5 \
        --benchmark_report_aggregates_only=true \
        --benchmark_out="${output}" \
        --benchmark_out_format=json

    echo
    echo "Result:"
    echo "  ${output}"
}

# =============================================================================
# Run all benchmarks
# =============================================================================

run()
{
    echo
    echo "==> Running benchmarks"
    echo

    mkdir -p "${RESULTS_DIR}"

    local benchmarks=()

    while IFS= read -r executable; do
        benchmarks+=("${executable}")
    done < <(find_benchmarks)

    if [[ "${#benchmarks[@]}" -eq 0 ]]; then
        echo
        echo "ERROR: No CASTLE benchmark executables found."
        echo
        echo "Expected executables such as:"
        echo "  castle_benchmark_containers"
        echo "  castle_benchmark_utility"
        echo "  castle_benchmark_callbacks"
        echo "  castle_benchmark_events"
        echo "  castle_benchmark_chrono"
        echo
        echo "Build the benchmarks first:"
        echo
        echo "  $0 build"
        echo
        return 1
    fi

    for executable in "${benchmarks[@]}"; do
        run_one "${executable}"
    done

    echo
    echo "==> Benchmark results"
    echo

    find "${RESULTS_DIR}" \
        -maxdepth 1 \
        -type f \
        -name '*.json' \
        -print \
        | sort

    echo
}

# =============================================================================
# Clean
# =============================================================================

clean()
{
    echo
    echo "==> Cleaning benchmark build"
    echo

    if [[ -d "${BUILD_DIR}" ]]; then
        rm -rf "${BUILD_DIR}"
        echo "Removed:"
        echo "  ${BUILD_DIR}"
    else
        echo "Nothing to clean."
    fi

    echo
}

# =============================================================================
# Main
# =============================================================================

print_header

case "${ACTION}" in

    all)
        print_config
        configure
        build
        run
        ;;

    configure)
        print_config
        configure
        ;;

    build)
        print_config
        configure
        build
        ;;

    run)
        print_config
        run
        ;;

    clean)
        clean
        ;;

    clean-build)
        clean
        print_config
        configure
        build
        run
        ;;

    *)
        echo "Usage:"
        echo
        echo "  $0                 Configure, build and run"
        echo "  $0 configure       Configure only"
        echo "  $0 build           Configure and build"
        echo "  $0 run             Run existing benchmarks"
        echo "  $0 clean           Remove benchmark build"
        echo "  $0 clean-build     Clean, configure, build and run"
        echo
        echo "Environment:"
        echo
        echo "  BUILD_TYPE=Release"
        echo "  BUILD_JOBS=${BUILD_JOBS}"
        echo
        echo "Examples:"
        echo
        echo "  $0"
        echo "  BUILD_TYPE=Debug $0"
        echo "  BUILD_JOBS=8 $0"
        echo "  $0 clean-build"
        echo
        exit 1
        ;;

esac

echo
echo "================================================================"
echo " Done"
echo "================================================================"
echo
