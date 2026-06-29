# EPI Guard — Plano de Implementação

> Sistema embarcado (Zephyr RTOS) que recebe de um **dashboard** quem está entrando e com quais EPIs, consulta uma **API + banco mockado** para decidir se está liberado, e aciona **LED + buzzer** conforme o resultado.

Documento para o time de 3 pessoas. Lê de cima para baixo: visão → decisões → arquitetura → contratos → firmware → API → dashboard → testes → requisitos → divisão de tarefas → cronograma → riscos → "pronto é quando".

---

## 1. O que mudou em relação ao projeto original

| Antes (RFID) | Agora (dashboard + API) |
|---|---|
| Leitor RC522 lê tags dos EPIs | Dashboard envia `{who, with[]}` para o firmware |
| Driver SPI feito na mão (sensor) | **WiFi/HTTP** vira a interface de comunicação obrigatória |
| Validação local com `settings` (mapa tag→EPI) | Validação **remota** numa API conectada a banco mockado |
| LED verde/vermelho | **LED + buzzer** (bip curto/longo, LED fixo/piscando) |

As 3 threads e os subsistemas (zbus, settings, shell, logging) **continuam sendo o esqueleto** — só muda a fonte do evento (dashboard em vez de RFID) e o destino da validação (API em vez de local).

---

## 2. Decisões assumidas (confirmem ou ajustem antes de codar)

Tomei estas decisões para o plano sair concreto. São pontos de virada — se mudarem, mudam pastas/threads/board.

1. **Placa: ESP32 (ESP32-DevKitC / WROOM-32).** Motivo: WiFi embutido (a mudança exige IP para chamar a API), barata e fácil de achar em Maceió, bom suporte no Zephyr, GPIO/PWM de sobra para LED+buzzer+botão. UART de debug sai pela ponte USB-serial da placa (atende o requisito). *Alternativa:* ESP32-S3 (tem USB nativo, útil se quiserem CDC ACM também).
2. **Interface de comunicação: WiFi (TCP/HTTP).** É a interface exigida pelo enunciado. Não usaremos mais o "sensor na mão".
3. **Caminho dos dados:** `Dashboard → Firmware → API → Banco → API → Firmware → atuadores → resposta ao Dashboard`. O **firmware é quem chama a API** (cliente HTTP), exatamente como o professor descreveu.
4. **Como o dashboard fala com o firmware:** o firmware sobe um **servidor HTTP** com a rota `POST /verify`. **Recomendado: o próprio firmware serve a página do dashboard** em `/` → mesma origem → **zero dor de cabeça com CORS**.
   - *Plano B (se o `http_server` do Zephyr estiver instável na versão de vocês):* firmware vira **só cliente HTTP** e o dashboard chega por **MQTT** (broker Mosquitto no notebook) ou por **USB serial (Web Serial API)**. Ver §13 (riscos).
5. **API + banco mockado:** serviço **Node.js + Express** rodando no notebook de um dos integrantes, na mesma rede WiFi. Banco = arquivo `db.json` (sem banco real — é PoC).
6. **Buzzer:** ativo (liga/desliga por GPIO) para simplicidade. *Alternativa:* passivo + PWM se quiserem tons diferentes.

> ⚠️ **Sobre versões:** nomes exatos de símbolos Kconfig, target da placa e maturidade do `http_server`/`http_client` mudam entre versões do Zephyr. Sempre confirmem contra a doc da **versão que vocês usam** (`west boards | grep esp32`, doc de cada subsistema). Os nomes aqui são os prováveis, não gravados em pedra.

---

## 3. Arquitetura geral

```
   ┌─────────────┐   ➀ POST /verify          ┌──────────────────────┐   ➁ POST /check        ┌──────────┐
   │  DASHBOARD  │   { who, with[] }          │       FIRMWARE       │   { who, with[] }       │   API    │
   │   (web)     │ ─────────────────────────► │   ESP32 · Zephyr     │ ──────────────────────► │ Express  │
   │             │ ◄───────────────────────── │                      │ ◄────────────────────── │          │
   └─────────────┘   ➄ resposta               └──────────┬───────────┘   ➃ { allowed, missing }└────┬─────┘
                     (liberado / negado)                  │ ➃ aciona                                  │ ➂ consulta
                                                          ▼                                           ▼
                                                  LED + BUZZER                                   ┌──────────┐
                                              (bip curto / longo,                                │ db.json  │
                                               LED fixo / piscando)                              │ (mock)   │
                                                                                                 └──────────┘
```

**Internamente, o firmware é dividido em 3 threads que conversam por zbus:**

```
            ┌──────────────────────────── FIRMWARE (Zephyr) ────────────────────────────┐
            │                                                                            │
 Dashboard  │   POST /verify     ┌────────────┐  pub   ┌─────────────────────┐  sub      │
   (web) ───┼──────────────────► │ net_thread │ ─────► │ chan_verify_request │ ────────┐ │
            │                     └─────┬──────┘        └─────────────────────┘         │ │
   resposta │                          ▲ sub                                            ▼ │
   ◄────────┼──────────────┐           │                                       ┌────────────────┐
            │              │           │            ┌─────────────────────┐    │ validator_     │
            │              └───────────┴──── pub ── │ chan_verify_result  │◄── │ thread         │
            │                                       └──────────┬──────────┘pub └───────┬────────┘
            │                                                  │ sub                   │ HTTP POST /check
            │                                                  ▼                       ▼
            │                                          ┌────────────────┐         (API + db.json)
            │                                          │ actuator_thread│
            │                                          └───────┬────────┘
            └──────────────────────────────────────────────────┼─────────────────────────────────┘
                                                                ▼
                                                    LED (verde/vermelho) + Buzzer
```

- **`chan_verify_request`** carrega o pedido (`who` + lista de EPIs).
- **`chan_verify_result`** carrega o veredito (`allowed` + EPIs faltando). Tem **dois observadores**: `actuator_thread` (aciona LED/buzzer) e `net_thread` (devolve a resposta HTTP ao dashboard). Isso é uma demonstração legítima de zbus com múltiplos consumidores.

---

## 4. Contratos de dados (definam isto primeiro — é o "idioma" entre os 4)

### 4.1 Dashboard → Firmware (`POST /verify`)
```json
{ "who": "Gabriel", "with": ["Capacete", "Óculos", "Cinto"] }
```

### 4.2 Firmware → API (`POST /check`)
```json
{ "who": "Gabriel", "with": ["Capacete", "Óculos", "Cinto"] }
```
(mesmo payload — o firmware repassa; em PoC não precisa transformar)

### 4.3 API → Firmware (resposta)
```json
{ "allowed": false, "required": ["Capacete", "Óculos", "Cinto", "Bota"], "missing": ["Bota"] }
```

### 4.4 Firmware → Dashboard (resposta de `/verify`)
```json
{ "status": "ok", "allowed": false, "missing": ["Bota"] }
```
`status` pode ser `"ok"` ou `"error"` (timeout/API fora do ar → o firmware faz padrão de erro nos atuadores).

> Regra de ouro: **EPIs como strings canônicas e fixas** (`"Capacete"`, `"Óculos"`, `"Luva"`, `"Cinto"`, `"Bota"`, `"Protetor auricular"`). Definam a lista mestre num lugar só (dashboard, API e firmware concordando). Comparação **case-sensitive** para simplificar.

---

## 5. Arquitetura do firmware

### 5.1 Threads

| Thread | Responsabilidade | Publica | Assina |
|---|---|---|---|
| `net_thread` | Conecta WiFi (creds do settings); sobe servidor HTTP; trata `POST /verify`; serve a página `/`; devolve a resposta ao dashboard | `chan_verify_request` | `chan_verify_result` |
| `validator_thread` | Recebe o pedido; chama a API (`POST /check`); interpreta a resposta; trata erro de rede | `chan_verify_result` | `chan_verify_request` |
| `actuator_thread` | Aciona LED + buzzer conforme o veredito | — | `chan_verify_result` |

**Sincronização da resposta HTTP:** para PoC, assumam **um pedido por vez** (single in-flight). O `net_thread` publica o request, bloqueia num `k_sem`/`k_msgq` até o resultado correspondente chegar em `chan_verify_result`, e então responde. Documentem essa limitação — é aceitável e mantém o código simples.

### 5.2 Canais zbus + structs de mensagem (`zbus_channels.h`)
```c
#define EPI_MAX_ITEMS      8
#define EPI_NAME_LEN       24
#define EPI_WHO_LEN        32

struct verify_request {
    uint32_t req_id;
    char     who[EPI_WHO_LEN];
    char     items[EPI_MAX_ITEMS][EPI_NAME_LEN];
    uint8_t  item_count;
};

enum verify_status { VERIFY_OK, VERIFY_ERROR };

struct verify_result {
    uint32_t           req_id;
    enum verify_status status;     /* OK ou erro de rede/API */
    bool               allowed;
    char               missing[EPI_MAX_ITEMS][EPI_NAME_LEN];
    uint8_t            missing_count;
};

ZBUS_CHAN_DECLARE(chan_verify_request);
ZBUS_CHAN_DECLARE(chan_verify_result);
```

### 5.3 Settings (chaves persistidas em NVS)

| Chave | Exemplo | Usada por |
|---|---|---|
| `wifi/ssid` | `"LabRedes"` | net_thread |
| `wifi/psk` | `"senha123"` | net_thread |
| `api/base_url` | `"http://192.168.0.10:3000"` | validator_thread |
| `device/name` | `"epi-guard-01"` | logging / dashboard |
| `policy/timeout_ms` | `3000` | validator_thread |

### 5.4 Shell (prefixo `epi`)

| Comando | O que faz |
|---|---|
| `epi verify <who> <epi1,epi2,...>` | **Injeta um pedido localmente** (publica em `chan_verify_request`) — testa toda a cadeia sem dashboard nem rede ainda |
| `epi wifi <ssid> <psk>` | Grava credenciais no settings |
| `epi api <url>` | Grava a URL base da API |
| `epi net` | Reconecta o WiFi |
| `epi status` | Mostra estado do WiFi, IP, URL da API, último resultado |
| `epi last` | Detalha a última verificação |

> O `epi verify` é o melhor amigo do time: permite validar threads + zbus + atuadores **antes** de WiFi, API e dashboard existirem.

### 5.5 Logging (módulos)
`net`, `validator`, `actuator`, `epi_settings`, `main`. Um `LOG_MODULE_REGISTER` por arquivo. Nível por módulo via Kconfig/`prj.conf`.

### 5.6 Periféricos e comportamentos

Devicetree overlay (`boards/esp32_devkitc_wroom.overlay`) com aliases:

| Alias | Periférico | Comportamento |
|---|---|---|
| `led-ok` | LED verde (GPIO) | **Liberado:** aceso fixo por ~3 s |
| `led-deny` | LED vermelho (GPIO) | **Negado:** piscando ~3 s |
| `buzzer` | Buzzer ativo (GPIO) | **Liberado:** bip curto (~120 ms) · **Negado:** bip longo (~900 ms) · **Erro:** 3 bips curtos |
| `sw0` | Botão | Repete a última verificação (re-publica o último `verify_request`) |

> O enunciado exige só 1 LED. Usar dois (verde/vermelho) deixa a demo mais clara — fica a critério de vocês.

### 5.7 `prj.conf` (esqueleto — confirmem nomes na sua versão)
```ini
# Núcleo
CONFIG_ZBUS=y
CONFIG_SETTINGS=y
CONFIG_NVS=y
CONFIG_FLASH=y
CONFIG_SHELL=y
CONFIG_LOG=y

# Rede / WiFi (ESP32)
CONFIG_WIFI=y
CONFIG_NETWORKING=y
CONFIG_NET_IPV4=y
CONFIG_NET_DHCPV4=y
CONFIG_NET_SOCKETS=y

# HTTP
CONFIG_HTTP_CLIENT=y
CONFIG_HTTP_SERVER=y     # confirmar maturidade na versão (ver §13)

# JSON
CONFIG_JSON_LIBRARY=y

# GPIO / PWM
CONFIG_GPIO=y
# CONFIG_PWM=y           # só se buzzer passivo

# Threads
CONFIG_MAIN_STACK_SIZE=2048
```

---

## 6. API + banco mockado

### 6.1 Lógica
1. Recebe `POST /check` com `{ who, with[] }`.
2. `required = db.people[who] ?? db.default_required`.
3. `missing = required.filter(r => !with.includes(r))`.
4. `allowed = missing.length === 0`.
5. Responde `{ allowed, required, missing }`.

### 6.2 `db.json` (exemplo)
```json
{
  "default_required": ["Capacete", "Óculos", "Cinto"],
  "people": {
    "Gabriel": ["Capacete", "Óculos", "Cinto", "Bota"],
    "Ana":     ["Capacete", "Luva"],
    "Joao":    ["Capacete", "Óculos", "Protetor auricular"]
  }
}
```

### 6.3 `server.js` (referência — ~30 linhas)
```js
const express = require("express");
const db = require("./db.json");
const app = express();
app.use(express.json());

app.post("/check", (req, res) => {
  const { who, with: items = [] } = req.body;
  const required = db.people[who] ?? db.default_required;
  const missing = required.filter(r => !items.includes(r));
  res.json({ allowed: missing.length === 0, required, missing });
});

app.listen(3000, "0.0.0.0", () => console.log("API on :3000"));
```
> Subam em `0.0.0.0` (não `localhost`) para o ESP32 alcançar pela rede. Anotem o IP do notebook (`ipconfig`/`ip addr`) — é o que vai em `epi api <url>`.

---

## 7. Dashboard (web)

Página simples: campo **who** + checkboxes dos EPIs + botão "Verificar" + área de resultado.

- **Recomendado:** servida **pelo próprio firmware** em `/` (mesma origem → sem CORS). HTML embutido como recurso estático no firmware.
- **Durante o desenvolvimento** (iterar sem reflashar): sirvam o dashboard pelo Express (`app.use(express.static(...))`) e, no `fetch` para o firmware, usem uma *requisição simples* (`Content-Type: text/plain`, sem headers customizados) para **evitar preflight**; basta o firmware responder com `Access-Control-Allow-Origin: *`.

`app.js` (essência):
```js
async function verificar() {
  const who = document.querySelector("#who").value;
  const items = [...document.querySelectorAll(".epi:checked")].map(c => c.value);
  const r = await fetch(`http://${FIRMWARE_IP}/verify`, {
    method: "POST",
    headers: { "Content-Type": "text/plain" },     // evita preflight
    body: JSON.stringify({ who, with: items })
  });
  const data = await r.json();
  mostrarResultado(data); // allowed? "LIBERADO" : "NEGADO — falta " + data.missing.join(", ")
}
```

---

## 8. Estrutura do repositório

```
epi-guard/
├─ firmware/
│  ├─ CMakeLists.txt
│  ├─ prj.conf
│  ├─ Kconfig
│  ├─ boards/esp32_devkitc_wroom.overlay      # aliases led-ok, led-deny, buzzer, sw0
│  ├─ src/
│  │  ├─ main.c              # init, carrega settings, sobe as 3 threads
│  │  ├─ net.c / net.h       # wifi + http_server + /verify + serve dashboard
│  │  ├─ validator.c / .h    # assina request → chama API → publica result
│  │  ├─ api_client.c / .h   # wrapper http_client  (INTERFACE MOCKÁVEL)
│  │  ├─ actuator.c / .h     # máquina de estados LED + buzzer
│  │  ├─ led.c/.h  buzzer.c/.h
│  │  ├─ zbus_channels.c/.h  # canais + structs
│  │  ├─ epi_settings.c/.h   # handlers do settings
│  │  ├─ epi_shell.c/.h      # comandos do shell
│  │  └─ json_codec.c/.h     # parse/serialize — PURO, sem I/O (testável)
│  └─ tests/
│     ├─ json_codec/         # ztest + testcase.yaml
│     ├─ validation/         # ztest + testcase.yaml
│     └─ zbus_flow/          # ztest + testcase.yaml
├─ api/
│  ├─ server.js
│  ├─ db.json
│  └─ package.json
├─ dashboard/
│  ├─ index.html
│  └─ app.js
└─ docs/
   └─ PLANO_EPI_Guard.md     # este arquivo
```

> **Decisão de design que vale ouro nos testes:** isole `json_codec` (lógica pura) e esconda a chamada de rede atrás de `api_client` com **interface mockável**:
> ```c
> int api_check(const struct verify_request *req, struct verify_result *out);
> ```
> Impl. real = `http_client`. Impl. de teste = retorna resultado canned. Assim o `validator_thread` é testável **sem rede**.

---

## 9. Estratégia de testes (ZTEST + Twister)

### Testes unitários (rodam em `native_sim`, sem hardware)
| Suíte | O que cobre |
|---|---|
| `json_codec` | Parse de `{who,with}` (válido, campo faltando, item demais, malformado); parse de `{allowed,missing}`; serialização da resposta |
| `validation` | Função pura "resultado → padrão de atuador" (liberado→bip curto/LED fixo, negado→bip longo/piscar, erro→3 bips) |
| `zbus_flow` | Publicar em `chan_verify_request` com `api_client` mockado → asserir que `chan_verify_result` é emitido com o veredito esperado |

### Twister
```bash
west twister -T firmware/tests -p native_sim
west twister -T firmware/tests -p esp32_devkitc_wroom   # smoke no alvo (opcional)
```
Cada suíte tem seu `testcase.yaml`. Meta: **twister verde no `native_sim`** como gate de CI.

---

## 10. Mapeamento dos requisitos (prova de aderência)

| Requisito | Como é atendido |
|---|---|
| UART de debug (USB) | Console do Zephyr pela ponte USB-serial da placa |
| LED (≥1) | `led-ok` verde (+ `led-deny` vermelho) via GPIO |
| Botão de função | `sw0` repete a última verificação |
| Interface de comunicação | **WiFi (TCP/HTTP)** — cliente para a API e servidor para o dashboard |
| C Code Guidelines | `clang-format` (config Zephyr) + `checkpatch` no CI |
| Testes (ZTEST + Twister) | 3 suítes unitárias + twister em `native_sim` |
| Logging | `LOG_MODULE_REGISTER` por módulo, nível configurável |
| Shell | Comandos `epi *` (incl. `epi verify` para teste ponta a ponta) |
| Settings | WiFi (ssid/psk), URL da API, nome do device, timeout — em NVS |
| zbus | `chan_verify_request` e `chan_verify_result` (este com 2 observadores) |
| Diferencial (criatividade) | Buzzer com padrões distintos, validação remota via API, dashboard servido pelo próprio firmware, `epi verify` como ferramenta de teste |

---

## 11. Divisão de trabalho (4 pessoas)

Cada dono escreve **também os testes do seu módulo**. Revisão cruzada obrigatória (PR + 1 review).

| Pessoa | Frente | Arquivos principais |
|---|---|---|
| **A — Núcleo & integração** | `main`, build, board overlay, `prj.conf`, `Kconfig`, `zbus_channels`, `epi_settings` | `main.c`, `zbus_channels.*`, `epi_settings.*`, overlay |
| **B — Conectividade** | WiFi, servidor HTTP `/verify`, servir dashboard, cliente da API | `net.*`, `api_client.*` |
| **C — Lógica & atuadores** | parsing/serialização, validador, LED, buzzer, shell | `json_codec.*`, `validator.*`, `actuator.*`, `led.*`, `buzzer.*`, `epi_shell.*` |
| **D — API, dashboard, QA** | serviço Express + `db.json`, dashboard web, harness de testes + Twister + CI, documentação | `api/*`, `dashboard/*`, `tests/` (infra), `docs/` |

Interfaces compartilhadas (combinem cedo): `json_codec` (B↔C), structs zbus (A↔todos), contrato da API (B↔D).

---

## 12. Cronograma sugerido (5 semanas)

| Marco | Entregável | Requisitos cobertos |
|---|---|---|
| **M0 — Setup** | Toolchain + `west` + "blinky" na placa; repo + estrutura; CI esqueleto | build system |
| **M1 — Esqueleto threads + zbus** | 3 threads + 2 canais; `epi verify` aciona a cadeia com `api_client` **falso** (resultado canned) → atuadores respondem. **Demo só por shell, sem rede.** | zbus, shell, logging |
| **M2 — Atuadores reais** | `led-ok`/`led-deny` + padrões de buzzer + botão repete | LED, botão, GPIO |
| **M3 — API + banco** | Express + `db.json`; `api_client` real (http_client) batendo na API; validador usa API de verdade | (interface comm — cliente) |
| **M4 — WiFi + settings** | Conecta WiFi pelas creds do settings; `epi wifi`/`epi api` persistem; reconexão | settings, WiFi |
| **M5 — Servidor HTTP + dashboard** | Rota `/verify`; dashboard; mesma origem (firmware serve a página); **demo completa dashboard→firmware→API→atuadores** | interface comm (servidor) |
| **M6 — Testes + acabamento** | Cobertura ZTEST, twister verde, `clang-format`/`checkpatch`, limpeza de logs, README + slides | testes, code guidelines |

> O **M1 já é demonstrável** (via `epi verify`). Isso garante que, mesmo se a rede atrasar, vocês têm algo funcionando para mostrar.

---

## 13. Riscos e mitigações

| Risco | Mitigação |
|---|---|
| `http_server` do Zephyr imaturo na sua versão | **Plano B:** firmware vira só cliente HTTP; dashboard chega por **MQTT** (Mosquitto no notebook) ou **USB serial (Web Serial)**. O caminho firmware→API (http_client) é o maduro e fica intacto |
| CORS browser→firmware | Firmware serve a própria página (mesma origem) **ou** requisição "simples" (`text/plain`) + header `Access-Control-Allow-Origin: *` |
| WiFi instável na demo | `epi verify` (shell) sempre funciona como caminho de fallback ao vivo |
| Memória do ESP32 com servidor HTTP | Página minúscula; ou servir o dashboard pelo notebook |
| Vários pedidos simultâneos | Restringir a **um por vez** (semáforo) e documentar |
| Disponibilidade da placa / gente bloqueada | Lógica desenvolvida em `native_sim` (não precisa de hardware para A, C e D avançarem) |
| Tempo apertado | Ordem dos marcos coloca o núcleo demonstrável no M1 |

---

## 14. "Pronto é quando" (Definition of Done)

- [ ] `west build` limpo para a placa **e** para `native_sim`
- [ ] `west twister -p native_sim` todo verde
- [ ] `clang-format` sem diffs; `checkpatch` sem erros
- [ ] Dashboard → firmware → API → atuadores respondendo certo nos 3 casos (liberado / negado / erro)
- [ ] Settings persiste após reboot (WiFi e URL da API)
- [ ] Todos os comandos `epi *` funcionando
- [ ] Logging em níveis apropriados, sem spam
- [ ] zbus é o mecanismo entre as threads (sem variáveis globais escondidas no lugar dele)
- [ ] README com setup + passos da demo + os slides atualizados

---

## 15. Quickstart (setup inicial)

```bash
# Zephyr + ESP32 (confirmar passos na doc da sua versão)
west init -m https://github.com/zephyrproject-rtos/zephyr --mr main zephyrproject
cd zephyrproject && west update
west zephyr-export
west blobs fetch hal_espressif          # blobs WiFi do ESP32

# Build / flash / monitor (verificar o target exato com: west boards | grep esp32)
west build -b esp32_devkitc_wroom/esp32/procpu firmware
west flash
west espressif monitor

# API
cd api && npm install && node server.js   # sobe em 0.0.0.0:3000
```

Fluxo de primeiro teste ponta a ponta (sem dashboard):
```
uart:~$ epi api http://192.168.0.10:3000
uart:~$ epi wifi LabRedes senha123
uart:~$ epi net
uart:~$ epi verify Gabriel Capacete,Óculos,Cinto
# -> deve faltar "Bota" (db de exemplo) -> bip longo + LED vermelho piscando
```
