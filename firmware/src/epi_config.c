/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "epi_config.h"

#include <string.h>

#include <zephyr/kernel.h>

#if IS_ENABLED(CONFIG_SETTINGS)
#include "epi_settings.h"
#endif

static char api_url[EPI_API_URL_MAX];
static struct k_mutex config_lock;

static int epi_config_init_fn(void)
{
	k_mutex_init(&config_lock);
	api_url[0] = '\0';
	return 0;
}

SYS_INIT(epi_config_init_fn, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

void epi_config_set_api_url(const char *url)
{
	if (url == NULL) {
		return;
	}

	k_mutex_lock(&config_lock, K_FOREVER);
	strncpy(api_url, url, sizeof(api_url) - 1);
	api_url[sizeof(api_url) - 1] = '\0';
	k_mutex_unlock(&config_lock);

#if IS_ENABLED(CONFIG_SETTINGS)
	(void)epi_settings_set_api_url(url);
#endif
}

void epi_config_sync_from_settings(void)
{
#if IS_ENABLED(CONFIG_SETTINGS)
	char buf[EPI_API_URL_MAX];

	if (epi_settings_get_api_url(buf, sizeof(buf)) == 0 && buf[0] != '\0') {
		k_mutex_lock(&config_lock, K_FOREVER);
		strncpy(api_url, buf, sizeof(api_url) - 1);
		api_url[sizeof(api_url) - 1] = '\0';
		k_mutex_unlock(&config_lock);
	}
#else
	ARG_UNUSED(0);
#endif
}

const char *epi_config_get_api_url(void)
{
	return api_url;
}

bool epi_config_api_ready(void)
{
	return api_url[0] != '\0';
}
