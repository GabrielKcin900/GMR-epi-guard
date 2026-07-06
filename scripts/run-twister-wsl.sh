#!/usr/bin/env bash
# Roda os testes ZTEST (M6) no WSL.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

if [[ ! -f .venv-wsl/bin/activate ]]; then
	echo "Execute primeiro: bash epi-guard/scripts/setup-wsl-tests.sh"
	exit 1
fi

# shellcheck disable=SC1091
source .venv-wsl/bin/activate
# shellcheck disable=SC1091
source zephyr/zephyr-env.sh

exec west twister -T epi-guard/firmware/tests -p native_sim --inline-logs "$@"
