#!/usr/bin/env bash
set -euo pipefail

mode="${1:---all}"
repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

usage() {
    echo "Usage: $0 [--all|--static|--firmware]" >&2
}

run_static_checks() {
    local actionlint_bin
    local test_dir

    python3 tools/check_repo.py

    actionlint_bin="${ACTIONLINT_BIN:-}"
    if [[ -z "${actionlint_bin}" ]]; then
        actionlint_bin="$(command -v actionlint || true)"
    fi
    if [[ -z "${actionlint_bin}" || ! -x "${actionlint_bin}" ]]; then
        actionlint_bin="$(./tools/install-actionlint.sh)"
    fi
    "${actionlint_bin}" -color .github/workflows/*.yml

    test_dir="$(mktemp -d /tmp/ai-passport-host-tests.XXXXXX)"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Imain \
        tests/test_ui_pixel_math.c main/ui_pixel_math.c \
        -o "${test_dir}/test_ui_pixel_math"
    "${test_dir}/test_ui_pixel_math"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Imain \
        tests/test_niuma_model.c main/niuma_model.c \
        -o "${test_dir}/test_niuma_model"
    "${test_dir}/test_niuma_model"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Imain \
        tests/test_niuma_journey.c main/niuma_model.c main/niuma_save.c \
        -o "${test_dir}/test_niuma_journey"
    "${test_dir}/test_niuma_journey"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Imain \
        tests/test_niuma_game.c main/niuma_game.c main/niuma_model.c \
        -o "${test_dir}/test_niuma_game"
    "${test_dir}/test_niuma_game"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Imain \
        tests/test_niuma_art.c main/niuma_art.c main/niuma_game.c main/niuma_model.c \
        -o "${test_dir}/test_niuma_art"
    "${test_dir}/test_niuma_art"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Imain \
        tests/test_niuma_save.c main/niuma_save.c main/niuma_model.c \
        -o "${test_dir}/test_niuma_save"
    "${test_dir}/test_niuma_save"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror \
        -Itests/niuma_storage/stubs -Imain tests/niuma_storage/test_worker.c \
        main/niuma_model.c main/niuma_save.c -o "${test_dir}/test_niuma_storage"
    "${test_dir}/test_niuma_storage"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Imain \
        tests/test_niuma_events.c main/niuma_events.c main/niuma_model.c \
        -o "${test_dir}/test_niuma_events"
    "${test_dir}/test_niuma_events"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Imain \
        tests/test_niuma_sound.c main/niuma_synth.c \
        -o "${test_dir}/test_niuma_sound"
    "${test_dir}/test_niuma_sound"
    python3 tests/test_verify_firmware.py
    rm -rf "${test_dir}"
    echo "Host tests: PASS"
}

run_firmware_checks() (
    local validation_build_dir

    if ! command -v idf.py >/dev/null 2>&1; then
        echo "ERROR: idf.py is not available; activate ESP-IDF 5.5.3 first." >&2
        return 1
    fi

    validation_build_dir="$(mktemp -d /tmp/ai-passport-firmware.XXXXXX)"
    trap 'case "${validation_build_dir}" in /tmp/ai-passport-firmware.*) rm -rf -- "${validation_build_dir}" ;; esac' EXIT

    SDKCONFIG_DEFAULTS="${repo_root}/sdkconfig.defaults" \
        idf.py -B "${validation_build_dir}" \
        -D "SDKCONFIG=${validation_build_dir}/sdkconfig" build
    idf.py -B "${validation_build_dir}" merge-bin \
        -o "${validation_build_dir}/FoloToy-AI-Passport-full.bin"
    python3 tools/verify_firmware.py "${validation_build_dir}"
    mkdir -p "${repo_root}/build"
    install -m 0644 \
        "${validation_build_dir}/FoloToy-AI-Passport-full.bin" \
        "${repo_root}/build/FoloToy-AI-Passport-full.bin"
    echo "Firmware build: PASS"
)

run_ui_checks() (
    local ui_build_dir
    ui_build_dir="$(mktemp -d /tmp/ai-passport-ui-tests.XXXXXX)"
    trap 'case "${ui_build_dir}" in /tmp/ai-passport-ui-tests.*) rm -rf -- "${ui_build_dir}" ;; esac' EXIT
    # The firmware step resolves the pinned LVGL dependency first. This build
    # uses host compilers and exercises the production controller and renderer.
    cmake -S tests/niuma_render -B "${ui_build_dir}" \
        -DCMAKE_C_COMPILER="${CC:-cc}" -DCMAKE_CXX_COMPILER="${CXX:-c++}"
    cmake --build "${ui_build_dir}" --parallel 4
    "${ui_build_dir}/niuma_render"
    echo "Production UI tests: PASS"
)

cd "${repo_root}"
case "${mode}" in
    --all)
        run_static_checks
        run_firmware_checks
        run_ui_checks
        ;;
    --static)
        run_static_checks
        ;;
    --firmware)
        run_firmware_checks
        ;;
    *)
        usage
        exit 2
        ;;
esac
