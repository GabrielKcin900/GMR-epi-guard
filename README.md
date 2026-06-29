# EPI Guard

Projeto da disciplina de Sistemas Embarcados (Zephyr RTOS + ESP32).

## Documentação

- [Plano de implementação](docs/PLANO_EPI_Guard.md)

## Quickstart

### Firmware (M1)

```powershell
cd C:\Users\gabri\Documents\projeto_embarcados\zephyrproject
.\.venv\Scripts\Activate.ps1

west build -b doit_esp32_devkit_v1/esp32/procpu -p always epi-guard/firmware -d epi-guard/firmware/build
west flash -d epi-guard/firmware/build

# Monitor: entre em epi-guard/firmware/build antes (west espressif não usa -d como build dir)
cd epi-guard/firmware/build
west espressif monitor -p COM3
# Se ainda aparecer lixo, confira no flash se esptool mostra "Crystal frequency: 26MHz"
# (overlay boards/doit_esp32_26mhz.overlay corrige isso)
# Sair do monitor: Ctrl+[
cd ../../..
```

No shell serial:

```
uart:~$ epi verify Gabriel Capacete,Oculos,Cinto
uart:~$ epi last
```

Use **ASCII** nos comandos (`Oculos`, nao `Óculos`) — o monitor serial do Windows nao envia acentos.

### Monitor serial (Windows)

O `west espressif monitor` (idf_monitor) **nao suporta backspace** de forma confiavel com o shell Zephyr — use miniterm ou PuTTY.

**Recomendado** — miniterm (feche o idf_monitor antes):

```powershell
# Com overlay 26 MHz aplicado, baud = 115200
python -m serial.tools.miniterm COM3 115200
```

Se o backspace ainda falhar, tente PuTTY (COM3, 115200, 8N1, terminal VT100).

Sair do miniterm: `Ctrl+[`

Se o prompt `uart:~$` nao voltar apos um comando, pressione **Enter** — logs e feedback LED/buzzer competem pela UART.

### Buzzer (GPIO18 / D18)

Buzzer ativo de 2 pinos: **D18 → sinal**, **GND → GND**. Configurado no overlay `boards/doit_esp32_26mhz.overlay`. Volume baixo e normal em GPIO 3,3 V; para mais volume use buzzer com transistor ou alimentacao 5 V (com cuidado).

## Git

Repositorio do **projeto** (nao inclui o tree inteiro do Zephyr):

```powershell
cd epi-guard
git init
git add .
git status
git commit -m "feat: M1 — zbus, shell epi, mock API, LED e buzzer"
```

Publicar no GitHub (crie o repo vazio antes):

```powershell
git remote add origin https://github.com/SEU_USUARIO/epi-guard.git
git branch -M main
git push -u origin main
```

O firmware compila contra o `zephyrproject` local (west + Zephyr SDK). Cada integrante precisa do mesmo workspace Zephyr ou documentar a versao em `docs/`.

### Blinky (validação de placa)

```powershell
cd zephyr\samples\basic\blinky
west build -b doit_esp32_devkit_v1/esp32/procpu -p always
west flash
west espressif monitor
```