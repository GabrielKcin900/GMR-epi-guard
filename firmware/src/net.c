/*
 * SPDX-License-Identifier: Apache-2.0
 * M1: stub da net_thread — observa resultados (futuro: resposta HTTP ao dashboard).
 */

#include "zbus_channels.h"

#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(net, LOG_LEVEL_INF);

extern struct zbus_observer net_sub;

static void net_thread(void)
{
	const struct zbus_channel *chan;

	LOG_INF("net thread started (M1 stub — WiFi/HTTP no M4/M5)");

	while (!zbus_sub_wait(&net_sub, &chan, K_FOREVER)) {
		struct verify_result res;

		if (chan != &chan_verify_result) {
			continue;
		}

		if (zbus_chan_read(&chan_verify_result, &res, K_MSEC(500)) < 0) {
			continue;
		}

		if (res.status != VERIFY_OK) {
			LOG_INF("[net stub] responderia ao dashboard: status=error req_id=%u",
				res.req_id);
			continue;
		}

		LOG_INF("[net stub] responderia ao dashboard: allowed=%d req_id=%u missing=%u",
			res.allowed, res.req_id, res.missing_count);
	}
}

K_THREAD_DEFINE(net_thread_id, 1536, net_thread, NULL, NULL, NULL, 6, 0, 0);
