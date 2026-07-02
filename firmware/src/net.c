/*
 * SPDX-License-Identifier: Apache-2.0
 * net_thread — WiFi (M4) + observador zbus (dashboard no M5).
 */

#include "net.h"
#include "zbus_channels.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#if IS_ENABLED(CONFIG_WIFI)
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>
#endif

#if IS_ENABLED(CONFIG_SETTINGS)
#include "epi_settings.h"
#endif

LOG_MODULE_REGISTER(net, LOG_LEVEL_INF);

extern struct zbus_observer net_sub;

#if IS_ENABLED(CONFIG_WIFI)

static enum epi_wifi_state wifi_state = EPI_WIFI_DISCONNECTED;
static struct net_mgmt_event_callback wifi_mgmt_cb;
static struct k_mutex net_lock;

static void set_wifi_state(enum epi_wifi_state state)
{
	k_mutex_lock(&net_lock, K_FOREVER);
	wifi_state = state;
	k_mutex_unlock(&net_lock);
}

enum epi_wifi_state epi_net_wifi_state(void)
{
	enum epi_wifi_state state;

	k_mutex_lock(&net_lock, K_FOREVER);
	state = wifi_state;
	k_mutex_unlock(&net_lock);

	return state;
}

static void wifi_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
			       struct net_if *iface)
{
	ARG_UNUSED(iface);

	if (mgmt_event == NET_EVENT_WIFI_CONNECT_RESULT) {
		const struct wifi_status *status = cb->info;

		if (status != NULL && status->status != 0) {
			LOG_ERR("WiFi connect failed (%d)", status->status);
			set_wifi_state(EPI_WIFI_DISCONNECTED);
		} else {
			LOG_INF("WiFi associado — aguardando DHCP");
		}
		return;
	}

	if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
		LOG_INF("IPv4 obtido (DHCP)");
		set_wifi_state(EPI_WIFI_CONNECTED);
	}
}

static int wifi_do_connect(const char *ssid, const char *psk)
{
	struct net_if *iface = net_if_get_default();
	struct wifi_connect_req_params params;
	int retries = 10;
	int ret;

	if (iface == NULL) {
		return -ENODEV;
	}

	memset(&params, 0, sizeof(params));
	params.ssid = ssid;
	params.ssid_length = strlen(ssid);
	params.psk = psk;
	params.psk_length = strlen(psk);
	params.security = WIFI_SECURITY_TYPE_PSK;

	set_wifi_state(EPI_WIFI_CONNECTING);
	LOG_INF("conectando WiFi: %s", ssid);

	while (retries-- > 0) {
		ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &params, sizeof(params));
		if (ret == 0) {
			return 0;
		}
		k_msleep(500);
	}

	set_wifi_state(EPI_WIFI_DISCONNECTED);
	return ret;
}

int epi_net_connect(void)
{
#if IS_ENABLED(CONFIG_SETTINGS)
	char ssid[EPI_WIFI_SSID_MAX + 1];
	char psk[EPI_WIFI_PSK_MAX + 1];
	int ret;

	if (!epi_settings_has_wifi()) {
		LOG_WRN("WiFi nao configurado (epi wifi <ssid> <psk>)");
		return -ENOENT;
	}

	epi_settings_get_wifi_ssid(ssid, sizeof(ssid));
	epi_settings_get_wifi_psk(psk, sizeof(psk));

	ret = wifi_do_connect(ssid, psk);
	if (ret < 0) {
		LOG_ERR("falha ao conectar WiFi: %d", ret);
	}

	return ret;
#else
	return -ENOTSUP;
#endif
}

int epi_net_reconnect(void)
{
	struct net_if *iface = net_if_get_default();
	int ret;

	if (iface == NULL) {
		return -ENODEV;
	}

	(void)net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0);
	k_msleep(500);
	set_wifi_state(EPI_WIFI_DISCONNECTED);

	ret = epi_net_connect();
	return ret;
}

int epi_net_get_ipv4_str(char *buf, size_t len)
{
	struct net_if *iface;
	struct net_in_addr *addr;

	if (buf == NULL || len == 0) {
		return -EINVAL;
	}

	buf[0] = '\0';

	iface = net_if_get_default();
	if (iface == NULL) {
		return -ENODEV;
	}

	addr = net_if_ipv4_get_global_addr(iface, NET_ADDR_PREFERRED);
	if (addr == NULL) {
		return -ENOENT;
	}

	if (net_addr_ntop(AF_INET, addr, buf, len) == NULL) {
		return -EIO;
	}

	return 0;
}

static void net_wifi_init(void)
{
	k_mutex_init(&net_lock);
	net_mgmt_init_event_callback(&wifi_mgmt_cb, wifi_event_handler,
				     NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&wifi_mgmt_cb);
}

#else /* !CONFIG_WIFI */

enum epi_wifi_state epi_net_wifi_state(void)
{
	return EPI_WIFI_DISCONNECTED;
}

int epi_net_connect(void)
{
	return -ENOTSUP;
}

int epi_net_reconnect(void)
{
	return -ENOTSUP;
}

int epi_net_get_ipv4_str(char *buf, size_t len)
{
	if (buf != NULL && len > 0) {
		buf[0] = '\0';
	}
	return -ENOTSUP;
}

#endif /* CONFIG_WIFI */

static void net_thread(void)
{
	const struct zbus_channel *chan;

#if IS_ENABLED(CONFIG_WIFI)
	net_wifi_init();

#if IS_ENABLED(CONFIG_SETTINGS)
	if (epi_settings_has_wifi()) {
		(void)epi_net_connect();
	}
#endif
	LOG_INF("net thread started (WiFi + zbus)");
#else
	LOG_INF("net thread started (zbus — sem WiFi)");
#endif

	while (!zbus_sub_wait(&net_sub, &chan, K_FOREVER)) {
		struct verify_result res;

		if (chan != &chan_verify_result) {
			continue;
		}

		if (zbus_chan_read(&chan_verify_result, &res, K_MSEC(500)) < 0) {
			continue;
		}

		if (res.status != VERIFY_OK) {
			LOG_INF("[net] responderia ao dashboard: status=error req_id=%u", res.req_id);
			continue;
		}

		if (res.unknown_person) {
			LOG_INF("[net] responderia ao dashboard: unknown_person req_id=%u",
				res.req_id);
		} else {
			LOG_INF("[net] responderia ao dashboard: allowed=%d req_id=%u missing=%u",
				res.allowed, res.req_id, res.missing_count);
		}
	}
}

K_THREAD_DEFINE(net_thread_id, 3072, net_thread, NULL, NULL, NULL, 6, 0, 0);
