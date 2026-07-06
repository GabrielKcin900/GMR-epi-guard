/*
 * SPDX-License-Identifier: Apache-2.0
 * Canais zbus isolados para teste de fluxo (request -> mock -> result).
 */

#include "zbus_channels.h"

#include <zephyr/zbus/zbus.h>

ZBUS_SUBSCRIBER_DEFINE(validator_sub, 4);
ZBUS_SUBSCRIBER_DEFINE(result_sub, 4);

ZBUS_CHAN_DEFINE(chan_verify_request, struct verify_request, NULL, NULL,
		 ZBUS_OBSERVERS(validator_sub), ZBUS_MSG_INIT(.req_id = 0));

ZBUS_CHAN_DEFINE(chan_verify_result, struct verify_result, NULL, NULL,
		 ZBUS_OBSERVERS(result_sub),
		 ZBUS_MSG_INIT(.req_id = 0, .status = VERIFY_OK, .allowed = false));
