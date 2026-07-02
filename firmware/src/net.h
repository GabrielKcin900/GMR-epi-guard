/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EPI_NET_H_
#define EPI_NET_H_

#include <stddef.h>

enum epi_wifi_state {
	EPI_WIFI_DISCONNECTED = 0,
	EPI_WIFI_CONNECTING,
	EPI_WIFI_CONNECTED,
};

enum epi_wifi_state epi_net_wifi_state(void);

/** Conecta WiFi com credenciais do settings. */
int epi_net_connect(void);

/** Desconecta e reconecta. */
int epi_net_reconnect(void);

/** Escreve IPv4 global (DHCP) ou string vazia. */
int epi_net_get_ipv4_str(char *buf, size_t len);

#endif /* EPI_NET_H_ */
