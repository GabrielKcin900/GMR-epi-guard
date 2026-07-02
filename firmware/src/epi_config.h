/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EPI_CONFIG_H_
#define EPI_CONFIG_H_

#include <stdbool.h>

#define EPI_API_URL_MAX 96

/** Define URL base da API (ex.: http://192.168.0.10:3000). */
void epi_config_set_api_url(const char *url);

/** Retorna URL configurada ou string vazia. */
const char *epi_config_get_api_url(void);

/** true se modo HTTP ativo e URL nao vazia. */
bool epi_config_api_ready(void);

/** Sincroniza URL em RAM a partir do settings (apos epi_settings_load). */
void epi_config_sync_from_settings(void);

#endif /* EPI_CONFIG_H_ */
