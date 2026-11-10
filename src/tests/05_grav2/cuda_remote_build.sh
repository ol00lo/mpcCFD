#!/usr/bin/env bash
set -euo pipefail

# ==== Settings ====
REMOTE_HOST=**.**.***.**
REMOTE_PORT=****
REMOTE_DIR= name                 # folder name inside /home/user/
LOCAL_OUTPUT_DIR="./build/bin/remote_output"   # куда класть локально

REMOTE_USER=user
BUILD_DIR=build                      # build subfolder
BINARY=./build/bin/grav2             # binary path relative to REMOTE_DIR
BINARY_ARGS="$*"
CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=Release -DWITH_CUDA=ON"
OUTPUT_DIR="./build/bin/output"                # путь на удалённом сервере, относительно REMOTE_PATH

PREFIX="[cuda_remote_build.sh] --- " # log prefix

REMOTE_HOME="/home/$REMOTE_USER"
REMOTE_PATH="$REMOTE_HOME/$REMOTE_DIR"
LOCAL_DIR="$(pwd)"
SSH_OPTS="-p $REMOTE_PORT -o StrictHostKeyChecking=accept-new"

echo "${PREFIX}Local project: $LOCAL_DIR"
echo "${PREFIX}Remote host:   $REMOTE_USER@$REMOTE_HOST:$REMOTE_PORT"
echo "${PREFIX}Remote folder: $REMOTE_PATH"

# ==== 1) Create the remote folder if it does not exist ====
# Password is asked here; later ssh reuses the connection (ControlMaster)
echo "${PREFIX}[1/5] Creating remote folder..."
ssh $SSH_OPTS -o ControlMaster=auto -o ControlPath="$HOME/.ssh/cm-%r@%h:%p" \
    -o ControlPersist=10m \
    "$REMOTE_USER@$REMOTE_HOST" "mkdir -p '$REMOTE_PATH'"

# ==== 2) Sync files ====
echo "${PREFIX}[2/5] Syncing files..."
rsync -az --delete \
    --exclude ".git" \
    --exclude "$BUILD_DIR" \
    --exclude ".idea" \
    --exclude ".vscode" \
    --no-owner --no-group \
    -e "ssh $SSH_OPTS -o ControlMaster=auto -o ControlPath=$HOME/.ssh/cm-%r@%h:%p -o ControlPersist=10m" \
    "$LOCAL_DIR/" "$REMOTE_USER@$REMOTE_HOST:$REMOTE_PATH/"

# ==== 3) Build with CMake ====
echo "${PREFIX}[3/5] Building with CMake..."
ssh $SSH_OPTS -o ControlMaster=auto -o ControlPath="$HOME/.ssh/cm-%r@%h:%p" \
    -o ControlPersist=10m \
    "$REMOTE_USER@$REMOTE_HOST" bash -s <<EOF
set -euo pipefail
cd '$REMOTE_PATH'
cmake -S . -B '$BUILD_DIR' $CMAKE_FLAGS
cmake --build '$BUILD_DIR' -j "\$(nproc 2>/dev/null || echo 2)"
EOF

# ==== 4) Run the binary ====
echo "${PREFIX}[4/5] Running: $BINARY $BINARY_ARGS"
ssh -t $SSH_OPTS -o ControlMaster=auto -o ControlPath="$HOME/.ssh/cm-%r@%h:%p" \
    -o ControlPersist=10m \
    "$REMOTE_USER@$REMOTE_HOST" bash -s <<EOF
set -euo pipefail
cd '$REMOTE_PATH'
exec '$BINARY' $BINARY_ARGS
EOF

# ==== 5) Sync output files from remote to local ====
echo "${PREFIX}[5/5] Syncing output files back to local..."
mkdir -p "$LOCAL_OUTPUT_DIR"
rsync -az --delete \
    --no-owner --no-group \
    -e "ssh $SSH_OPTS -o ControlMaster=auto -o ControlPath=$HOME/.ssh/cm-%r@%h:%p -o ControlPersist=10m" \
    "$REMOTE_USER@$REMOTE_HOST:$REMOTE_PATH/$OUTPUT_DIR/" \
    "$LOCAL_OUTPUT_DIR/"

echo "${PREFIX}DONE"
