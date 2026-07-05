# EPI Guard
- Plano: docs/PLANO_EPI_Guard.md
- Placa: doit_esp32_devkit_v1/esp32/procpu
- Marco atual: **M5** (HTTP server + dashboard embutido); proximo: M6 (testes + acabamento)
- Firmware: firmware/
- Dashboard: dashboard/ (embutido no firmware via `firmware/scripts/embed_dashboard.py`)
- Build M5: `west build ... -- "-DCONF_FILE=prj.conf;prj_m3.conf;prj_m4.conf;prj_m5.conf"`
