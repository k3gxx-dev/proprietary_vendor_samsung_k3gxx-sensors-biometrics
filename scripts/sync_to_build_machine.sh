#!/bin/bash
set -euo pipefail

REMOTE_HOST="${REMOTE_HOST:-ct102}"
REMOTE_PATH="${REMOTE_PATH:-/zRAID/builds/k3gxx-lineage-18.1/vendor/samsung/k3gxx/sensors-biometrics/}"
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "=== Sincronizando repositório local com ${REMOTE_HOST}:${REMOTE_PATH} ==="
rsync -avz --delete \
    --exclude='.git' \
    --exclude='tmp' \
    --exclude='.remember' \
    "${REPO_ROOT}/" "${REMOTE_HOST}:${REMOTE_PATH}"

echo "=== Sincronização concluída com sucesso ==="
