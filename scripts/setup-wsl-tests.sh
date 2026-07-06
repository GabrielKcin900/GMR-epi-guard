#!/usr/bin/env bash
# Setup one-time para rodar Twister (native_sim) no WSL.
# Nao use "sudo apt install west" — o west do apt nao traz deps do Zephyr (natsort, etc.).

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

pick_python() {
	for cmd in python3.12 python3.11 python3; do
		if command -v "$cmd" >/dev/null 2>&1; then
			echo "$cmd"
			return 0
		fi
	done
	return 1
}

PY="$(pick_python)" || {
	echo "Erro: python3 nao encontrado"
	exit 1
}

echo "==> Python: $($PY --version)"
if "$PY" -c 'import sys; exit(0 if sys.version_info < (3, 14) else 1)'; then
	:
else
	echo "Aviso: Python 3.14+ pode falhar com alguns pacotes; prefira: sudo apt install python3.12"
fi

echo "==> Dependencias do sistema (gcc + python venv)"
sudo apt update
sudo apt install -y "$PY"-venv python3-pip build-essential gcc g++ ninja-build

if command -v west >/dev/null && dpkg -s west >/dev/null 2>&1; then
	echo "==> Removendo west do apt (conflita com o west do venv Zephyr)"
	sudo apt remove -y west || true
fi

echo "==> venv Linux (.venv-wsl)"
rm -rf .venv-wsl
"$PY" -m venv .venv-wsl
# shellcheck disable=SC1091
source .venv-wsl/bin/activate
pip install --upgrade pip
pip install -r epi-guard/scripts/requirements-twister.txt

echo ""
echo "Pronto. Em cada sessao WSL:"
echo "  cd $ROOT"
echo "  source .venv-wsl/bin/activate"
echo "  source zephyr/zephyr-env.sh"
echo "  west twister -T epi-guard/firmware/tests -p native_sim --inline-logs"
