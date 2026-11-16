#!/bin/bash
set -e

export PSPDEV="${PSPDEV:-/usr/local/pspdev}"
export PATH="$PSPDEV/bin:$PATH"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="${SCRIPT_DIR}"

# If running under WSL with sources on a Windows drive (/mnt/c/...),
# building in native Linux storage (/tmp or ~/.cache) avoids 9P filesystem latency
# and speeds up compilation by up to 5x-10x!
if [[ "${SRC_DIR}" == /mnt/* ]] && [ -z "$BUILD_DIR" ]; then
    BUILD_DIR="/tmp/opticraft_psp_build"
else
    BUILD_DIR="${BUILD_DIR:-${SRC_DIR}/build/psp}"
fi

echo "=== OptiCraft Heritage Edition: Building for PSP ==="
echo "Source directory: ${SRC_DIR}"
echo "Build directory:  ${BUILD_DIR}"
echo "Build PRX:        ${BUILD_PRX:-ON}"

# Configure ccache if present
if command -v ccache >/dev/null 2>&1; then
    echo "=== ccache detected: configuring cache settings ==="
    export CCACHE_SLOPPINESS=time_macros,include_file_mtime
    export CCACHE_COMPRESS=1
    export CCACHE_COMPRESSLEVEL=6
    export CCACHE_MAXSIZE=5G
    export CCACHE_BASEDIR="${SRC_DIR}"
    export CCACHE_NOHASHDIR=1
    ccache -s 2>/dev/null || true
fi

# Select CMake generator: prefer Ninja when available
CMAKE_GEN=()
if [ -f "${BUILD_DIR}/CMakeCache.txt" ]; then
    echo "=== Existing CMake build detected: preserving its generator ==="
elif command -v ninja >/dev/null 2>&1; then
    echo "=== Ninja build generator detected ==="
    CMAKE_GEN=("-G" "Ninja")
fi

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

cmake "${SRC_DIR}" \
    "${CMAKE_GEN[@]}" \
    -DCMAKE_TOOLCHAIN_FILE="${PSPDEV}/psp/share/pspdev.cmake" \
    -DPLATFORM=PSP \
    -DCMAKE_BUILD_TYPE="${PSP_BUILD_TYPE:-Release}" \
    -DMC_LOG_LEVEL="${MC_LOG_LEVEL:-0}" \
    -DBUILD_PRX="${BUILD_PRX:-ON}" \
    -DPSP_REMOTE_DEBUG="${PSP_REMOTE_DEBUG:-OFF}"

if command -v nproc >/dev/null 2>&1; then
    BUILD_JOBS="$(nproc)"
elif command -v sysctl >/dev/null 2>&1; then
    BUILD_JOBS="$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
else
    BUILD_JOBS=4
fi

cmake --build . --parallel "${BUILD_JOBS}"

if command -v ccache >/dev/null 2>&1; then
    echo "=== ccache statistics after build ==="
    ccache -s || true
fi

echo "=== Copying EBOOT.PBP to project root and bin/psp ==="
mkdir -p "${SRC_DIR}/bin/psp"
cp -f "${BUILD_DIR}/bin/psp/EBOOT.PBP" "${SRC_DIR}/bin/psp/EBOOT.PBP" 2>/dev/null || cp -f "${BUILD_DIR}/EBOOT.PBP" "${SRC_DIR}/bin/psp/EBOOT.PBP" 2>/dev/null || true
cp -f "${SRC_DIR}/bin/psp/EBOOT.PBP" "${SRC_DIR}/EBOOT.PBP" 2>/dev/null || true

if [[ "${BUILD_PRX:-ON}" == "ON" ]]; then
    ELF_PATH="${BUILD_DIR}/OptiCraft.elf"
    if [[ -f "${ELF_PATH}" ]]; then
        cp -f "${ELF_PATH}" "${SRC_DIR}/OptiCraft.elf"
    fi

    PRX_PATH="${BUILD_DIR}/OptiCraft.elf.prx"
    if [[ ! -f "${PRX_PATH}" ]]; then
        PRX_PATH="${BUILD_DIR}/bin/psp/OptiCraft.elf.prx"
    fi
    if [[ -f "${PRX_PATH}" ]]; then
        cp -f "${PRX_PATH}" "${SRC_DIR}/OptiCraft.prx"
        echo "=== Copied PSPLINK PRX to ${SRC_DIR}/OptiCraft.prx ==="
    else
        echo "ERROR: BUILD_PRX=ON but OptiCraft.prx was not generated"
        exit 1
    fi
fi

# Deploy to PPSSPP if user folder exists
PPSSPP_DIR="/mnt/c/Users/user/Documents/PPSSPP/PSP/GAME/OptiCraft"
if [ -d "$PPSSPP_DIR" ]; then
    echo "=== Deploying EBOOT.PBP to PPSSPP ($PPSSPP_DIR) ==="
    cp -f "${SRC_DIR}/EBOOT.PBP" "$PPSSPP_DIR/EBOOT.PBP" 2>/dev/null || true
    if [ -f "${SRC_DIR}/assets.pak" ]; then
        if [ ! -f "$PPSSPP_DIR/assets.pak" ] || [ $(stat -c%s "${SRC_DIR}/assets.pak") -ne $(stat -c%s "$PPSSPP_DIR/assets.pak" 2>/dev/null || echo 0) ]; then
            echo "=== Syncing updated assets.pak to PPSSPP ==="
            cp -f "${SRC_DIR}/assets.pak" "$PPSSPP_DIR/assets.pak" 2>/dev/null || true
        fi
    fi
fi

echo "=== Build Complete! ==="
ls -lh "${SRC_DIR}/EBOOT.PBP"
