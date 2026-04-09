#!/bin/bash
set -e

BRANCHES=("qcom-msm8974-6.12.y")
ARCH="arm"
CROSS_COMPILE="arm-none-linux-gnueabihf-"
PKG_VERSION="1.0-2"
CONFIG_LOCALVERSION="-citronics-lemon"

ROOT_DIR=$(pwd)

KERNEL_SRC_DIR="${ROOT_DIR}/linux"

cd "$KERNEL_SRC_DIR"
git fetch --all
cd - > /dev/null

CONFIGS_DIR="$ROOT_DIR/configs"
OUTPUT_BASE="$ROOT_DIR/output"
BUILD_BASE="$ROOT_DIR/build"

for BRANCH in "${BRANCHES[@]}"; do
    VERSION="${BRANCH#qcom-msm8974-}"
    KERNEL_NAME="msm8974-${VERSION}"
    CONFIG_FILE="/home/dlh/ECAM/Cours/Driver_C/Citronics/LKM/board.config"
    OUTPUT_DIR="${OUTPUT_BASE}/${VERSION}"
    BUILD_DIR="${BUILD_BASE}/${VERSION}"
    MOD_SYMVERS="/home/dlh/ECAM/Cours/Driver_C/Citronics/LKM/Module.symvers"

    echo "🔁 Building branch: $BRANCH"
    echo "⚙️  Using config: $CONFIG_FILE"

    # Remove worktree from Git if it exists
    cd "$KERNEL_SRC_DIR"
    if git worktree list | grep -q "$BUILD_DIR"; then
        git worktree remove --force "$BUILD_DIR"
    fi

    # Ensure folder is gone
    rm -rf "$BUILD_DIR"
    git worktree prune

    # Add clean worktree
    git fetch origin "$BRANCH"
    git worktree add "$BUILD_DIR" "origin/$BRANCH"

    cd "$BUILD_DIR"

    # Setup build environment
    export ARCH="$ARCH"
    export CROSS_COMPILE="$CROSS_COMPILE"

    # Copy config and build
    cp "$CONFIG_FILE" .config
    make olddefconfig

    cp "$MOD_SYMVERS" Module.symvers

    echo "🚧 Making modules_prepare for $KERNEL_NAME with local version $CONFIG_LOCALVERSION"
    make -j$(nproc) \
        LOCALVERSION=$CONFIG_LOCALVERSION \
        modules

    echo "✅ Done: $OUTPUT_DIR"
    cd - > /dev/null
done

echo "🎉 All builds completed successfully."
