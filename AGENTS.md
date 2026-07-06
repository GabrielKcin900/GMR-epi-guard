# EPI Guard
- Plano: docs/PLANO_EPI_Guard.md
- Placa: doit_esp32_devkit_v1/esp32/procpu
- Marco atual: **M6** (ZTEST + twister + acabamento)
- Firmware: firmware/
- Testes: `west twister -T firmware/tests -p native_sim` (Linux/WSL; requer gcc host)
- Build M5: `west build ... -- "-DCONF_FILE=prj.conf;prj_m3.conf;prj_m4.conf;prj_m5.conf"`
