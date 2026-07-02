/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#if IS_ENABLED(CONFIG_SETTINGS)
#include "epi_config.h"
#include "epi_settings.h"
#endif

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
#if IS_ENABLED(CONFIG_SETTINGS)
	int rc;

	rc = epi_settings_init();
	if (rc < 0) {
		LOG_ERR("settings init failed: %d", rc);
	} else {
		rc = epi_settings_load();
		if (rc < 0) {
			LOG_WRN("settings load failed: %d", rc);
		}
		epi_config_sync_from_settings();
	}
#endif

	LOG_INF("EPI Guard firmware — marco M4");
#if IS_ENABLED(CONFIG_EPI_API_USE_MOCK)
	LOG_INF("API: mock local (epi verify sem rede)");
#else
	LOG_INF("API: HTTP client (epi api http://<IP>:3000)");
#endif
#if IS_ENABLED(CONFIG_WIFI)
	LOG_INF("WiFi: epi wifi <ssid> <psk> | epi net");
#endif
	LOG_INF("Teste: epi verify Gabriel Capacete,Oculos,Cinto,Bota");

	return 0;
}
