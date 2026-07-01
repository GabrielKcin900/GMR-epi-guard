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

### Monitor serial com PuTTY (recomendado no Windows)

O `west espressif monitor` **nao suporta backspace** com o shell Zephyr. O **PuTTY** costuma funcionar melhor.

**Antes de abrir o PuTTY:** feche miniterm, idf_monitor e qualquer outro programa usando COM3.

#### 1. Instalar

Baixe em: https://www.putty.org/ (ou `winget install PuTTY.PuTTY`)

#### 2. Session (conexao serial)

| Campo | Valor |
|-------|-------|
| Connection type | **Serial** |
| Serial line | `COM3` (veja no Gerenciador de Dispositivos se for outra) |
| Speed | **115200** |

#### 3. Terminal

| Opcao | Valor |
|-------|-------|
| Terminal-type string | `vt100` |
| Local echo | **Force off** |
| Local line editing | **Force off** |

O Zephyr shell faz o echo e a edicao de linha — se ligar echo local, as letras duplicam.

#### 4. Keyboard

| Opcao | Valor |
|-------|-------|
| Backspace key | **Control-? (127)** |

Se backspace nao apagar, troque para **Control-H** e teste de novo.

#### 5. Connection → Serial (conferir)

| Opcao | Valor |
|-------|-------|
| Data bits | 8 |
| Stop bits | 1 |
| Parity | None |
| Flow control | None |

#### 6. Salvar e conectar

1. Volte em **Session**, digite um nome em **Saved Sessions** (ex.: `ESP32-EPI-Guard`)
2. Clique **Save**
3. Clique **Open**
4. Pressione **Enter** algumas vezes ate aparecer `uart:~$`

#### 7. Fluxo flash + PuTTY

```powershell
# 1) Flash (feche o PuTTY antes!)
west flash -d epi-guard/firmware/build

# 2) Abra o PuTTY e conecte em COM3 115200
```

Reiniciar a placa: botao **RESET** na ESP32 ou reconecte o USB.

Se o prompt sumir apos um comando, pressione **Enter** e aguarde o feedback LED/buzzer (~3 s).

#### Alternativa: miniterm

```powershell
python -m serial.tools.miniterm COM3 115200
```

Sair do miniterm: `Ctrl+]`

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