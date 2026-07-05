/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api_client.h"
#include "epi_state.h"
#include "zbus_channels.h"

#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(validator, LOG_LEVEL_INF);

extern struct zbus_observer validator_sub;

static void validator_thread(void)
{
	const struct zbus_channel *chan;

	LOG_INF("validator thread started");

	while (!zbus_sub_wait(&validator_sub, &chan, K_FOREVER)) {
		struct verify_request req;
		struct verify_result res;
		int err;

		if (chan != &chan_verify_request) {
			continue;
		}

		err = zbus_chan_read(&chan_verify_request, &req, K_MSEC(500));
		if (err < 0) {
			LOG_ERR("read request failed: %d", err);
			continue;
		}

		epi_state_store_request(&req);

		memset(&res, 0, sizeof(res));
		res.req_id = req.req_id;

		err = api_check(&req, &res);
		res.req_id = req.req_id;
		if (err < 0) {
			res.status = VERIFY_ERROR;
			res.allowed = false;
		}

		err = zbus_chan_pub(&chan_verify_result, &res, K_SECONDS(2));
		if (err < 0) {
			LOG_ERR("publish result failed: %d", err);
		}
	}
}

K_THREAD_DEFINE(validator_thread_id, 4096, validator_thread, NULL, NULL, NULL, 5, 0, 0);
