#!/usr/bin/env bash

set -e

ROOT_DIR=$(pwd)
BUILD_DIR="${ROOT_DIR}/build"
BIN_DIR="${BUILD_DIR}/bin"

build()
{
    mkdir -p "${BUILD_DIR}"

    cmake \
        -S "${ROOT_DIR}/benchmarks" \
        -B "${BUILD_DIR}" \
        -DCMAKE_BUILD_TYPE=Release

    cmake --build "${BUILD_DIR}" -j"$(nproc)"
}

run()
{
    echo
    echo "===================="
    echo "Running benchmarks..."
    echo "===================="
    echo

    for bench in "${BIN_DIR}"/castle_benchmark_*; do
        if [ -x "${bench}" ]; then
            echo "----------------------------------------"
            echo "Running $(basename "${bench}")"
            echo "----------------------------------------"

            "${bench}"

            echo
        fi
    done

    echo "All benchmarks completed."
}

run_json()
{
    mkdir -p "${BUILD_DIR}/benchmark_results"

    echo
    echo "===================="
    echo "Running benchmarks (JSON)..."
    echo "===================="
    echo

    for bench in "${BIN_DIR}"/castle_benchmark_*; do
        if [ -x "${bench}" ]; then

            name=$(basename "${bench}")
            output="${BUILD_DIR}/benchmark_results/${name}.json"

            echo "----------------------------------------"
            echo "Running ${name}"
            echo "Output : ${output}"
            echo "----------------------------------------"

            "${bench}" \
                --benchmark_out="${output}" \
                --benchmark_out_format=json

            echo
        fi
    done

    echo "JSON results saved to:"
    echo "${BUILD_DIR}/benchmark_results"
}

clean()
{
    echo "Removing ${BUILD_DIR}"
    rm -rf "${BUILD_DIR}"
}

COMMAND="${1:-default}"

case "${COMMAND}" in
    build)
        build
        ;;
    run)
        run
        ;;
    json)
        run_json
        ;;
    clean)
        clean
        ;;
    default)
        build
        run
        ;;
    *)
        echo "Usage:"
        echo "  $0               # build + run"
        echo "  $0 build         # build only"
        echo "  $0 run           # run only"
        echo "  $0 json          # run and save json"
        echo "  $0 clean         # remove build dir"
        exit 1
        ;;
esac
