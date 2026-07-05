# EPI Guard

Projeto da disciplina de Sistemas Embarcados (Zephyr RTOS + ESP32).

## Documentação

- [Plano de implementação](docs/PLANO_EPI_Guard.md)

## Quickstart

### Firmware (M3 — mock local, padrao)

```powershell
cd C:\Users\gabri\Documents\projeto_embarcados\zephyrproject
.\.venv\Scripts\Activate.ps1

west build -b doit_esp32_devkit_v1/esp32/procpu -p always epi-guard/firmware -d epi-guard/firmware/build
west flash -d epi-guard/firmware/build
```

Build padrao usa **mock local** (`CONFIG_EPI_API_USE_MOCK=y`) — funciona sem rede.

### Firmware (M4 — WiFi + settings + HTTP)

Requer blobs Espressif (uma vez):

```powershell
west blobs fetch hal_espressif
```

Build completo:

```powershell
west build -b doit_esp32_devkit_v1/esp32/procpu -p always epi-guard/firmware -d epi-guard/firmware/build-m4 -- "-DCONF_FILE=prj.conf;prj_m3.conf;prj_m4.conf"
west flash -d epi-guard/firmware/build-m4
```

No PuTTY (substitua SSID, senha e IP do notebook):

```
uart:~$ epi wifi MinhaRede minha_senha
uart:~$ epi api http://192.168.0.10:3000
uart:~$ epi status
uart:~$ epi verify Gabriel Capacete,Oculos,Cinto,Bota
```

Credenciais e URL **persistem apos reboot** (NVS). `epi net` reconecta o WiFi.

### Firmware (M5 — servidor HTTP + dashboard)

Build completo (WiFi + API + dashboard embutido no firmware):

```powershell
west build -b doit_esp32_devkit_v1/esp32/procpu -p always epi-guard/firmware -d epi-guard/firmware/build-m5 -- "-DCONF_FILE=prj.conf;prj_m3.conf;prj_m4.conf;prj_m5.conf"
west flash -d epi-guard/firmware/build-m5
```

**Demo M5 (browser → firmware → API → atuadores):**

1. Suba a API no notebook (`npm start` em `epi-guard/api`).
2. Configure a placa (PuTTY COM3 115200):
   ```
   uart:~$ epi wifi MinhaRede minha_senha
   uart:~$ epi api http://192.168.0.10:3000
   uart:~$ epi status
   ```
   Anote o IP da placa (ex.: `192.168.0.42`). `epi status` mostra `Dashboard: http://<IP>/`.
3. Abra no browser: `http://<IP_ESP32>/`
4. Preencha **who** + EPIs (ASCII) e clique **Verificar**.

Casos de teste (`api/db.json`):

| Quem | EPIs marcados | Resultado esperado |
|------|---------------|-------------------|
| Gabriel | Capacete, Oculos, Cinto, Bota | LIBERADO (bip curto + LED) |
| Gabriel | Capacete, Oculos, Cinto | NEGADO — falta Bota |
| Desconhecido | qualquer | NAO CADASTRADO (alarme ate BOOT) |

**Limitacao PoC:** um pedido HTTP por vez (503 se enviar outro antes do anterior terminar).

Para editar o dashboard e reflashar:

```powershell
python epi-guard/firmware/scripts/embed_dashboard.py
west build -b doit_esp32_devkit_v1/esp32/procpu epi-guard/firmware -d epi-guard/firmware/build-m5 -- "-DCONF_FILE=prj.conf;prj_m3.conf;prj_m4.conf;prj_m5.conf"
```

Fontes: `dashboard/index.html`, `dashboard/app.js`.

### Firmware (M3 — cliente HTTP, sem WiFi)

```powershell
west build -b doit_esp32_devkit_v1/esp32/procpu -p always epi-guard/firmware -d epi-guard/firmware/build-m3 -- "-DCONF_FILE=prj.conf;prj_m3.conf"
west flash -d epi-guard/firmware/build-m3
```

Somente compila o cliente HTTP; teste na placa ainda requer o build M4 acima.

### API Express (notebook)

```powershell
cd epi-guard/api
npm install
npm start
```

Teste com curl:

```powershell
curl -X POST http://localhost:3000/check -H "Content-Type: application/json" -d "{\"who\":\"Gabriel\",\"with\":[\"Capacete\",\"Oculos\",\"Cinto\",\"Bota\"]}"
```

Pessoa nao cadastrada retorna `"unknown": true`.

```powershell
# Monitor (opcional)
cd epi-guard/firmware/build
west espressif monitor -p COM3
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