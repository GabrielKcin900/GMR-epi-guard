# EPI Guard — roteiro de apresentacao

Roteiro curto para demo ao vivo (M5 validado + M6 testes).

## 1. Contexto (30 s)

- Controle de EPI em obra: operador informa quem e quais EPIs esta usando.
- ESP32 valida contra regras no notebook (API Express + `db.json`).
- Feedback local: LED + buzzer.

## 2. Arquitetura (1 min)

```
Browser (dashboard) --WiFi--> ESP32 (HTTP :80)
                                  |
                                  +-- zbus --> validator --> API :3000
                                  |
                                  +-- zbus --> actuator (LED/buzzer)
```

- 3 threads: `net`, `validator`, `actuator`
- 2 canais zbus: `chan_verify_request`, `chan_verify_result`

## 3. Preparacao antes da aula

1. Notebook na mesma WiFi que a placa.
2. `cd api && npm start` — API em `0.0.0.0:3000`.
3. Placa flashada com build M5; PuTTY COM3 115200.
4. `epi api http://<IP_NOTEBOOK>:3000` e `epi status` — anotar IP do ESP32.

## 4. Demo ao vivo (3–5 min)

| Passo | Acao | Resultado esperado |
|-------|------|-------------------|
| 1 | `curl http://<IP_ESP32>/ping` ou browser | `ok` |
| 2 | Abrir `http://<IP_ESP32>/` | Dashboard |
| 3 | Ana + Capacete + Luva | **LIBERADO** + bip curto |
| 4 | Gabriel sem Bota | **NEGADO** + falta Bota |
| 5 | Nome inexistente | **NAO CADASTRADO** |
| 6 | Terminal da API | Logs `POST /check who=...` |
| 7 | `epi verify Gabriel Capacete,Oculos,Cinto` no PuTTY | Mesma cadeia sem browser |

## 5. Testes automaticos (M6)

```bash
west twister -T epi-guard/firmware/tests -p native_sim --inline-logs
```

62 casos ZTEST em 6 suites: JSON (incl. casos de borda e -ENOSPC "cirurgicos"), feedback de atuador, fluxo zbus com mock, folding de nomes UTF-8, estado compartilhado e regras do mock da API. Cobertura: 98% de linhas / 90% de branches nos modulos de logica pura (relatorio HTML no artifact `coverage-report` do CI).

## 6. Fallback se WiFi falhar

- `epi verify` pelo shell sempre exercita validator + atuadores (API ainda precisa de rede).

## 7. Perguntas frequentes

- **Por que ASCII nos EPIs?** Monitor serial Windows nao envia acentos de forma confiavel.
- **Um pedido por vez?** PoC — semaforo no HTTP `/verify`.
- **Onde ficam as regras?** `api/db.json` — alterar sem reflashar o firmware.
