/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EPI_SETTINGS_H_
#define EPI_SETTINGS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define EPI_WIFI_SSID_MAX 32
#define EPI_WIFI_PSK_MAX  64
#define EPI_API_URL_MAX   96
#define EPI_DEVICE_NAME_MAX 32

int epi_settings_init(void);
int epi_settings_load(void);

bool epi_settings_has_wifi(void);

int epi_settings_get_wifi_ssid(char *buf, size_t len);
int epi_settings_get_wifi_psk(char *buf, size_t len);
int epi_settings_set_wifi(const char *ssid, const char *psk);

int epi_settings_get_api_url(char *buf, size_t len);
int epi_settings_set_api_url(const char *url);

int epi_settings_get_device_name(char *buf, size_t len);
int epi_settings_set_device_name(const char *name);

uint32_t epi_settings_get_timeout_ms(void);
int epi_settings_set_timeout_ms(uint32_t ms);

#endif /* EPI_SETTINGS_H_ */
