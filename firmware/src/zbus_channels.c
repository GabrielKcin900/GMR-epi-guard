/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zbus_channels.h"

#include <zephyr/zbus/zbus.h>


// This macro defines a channel.
// ZBUS_SUBSCRIBER_DEFINE
// Parameters
// _name	The channel's name.
// _type	The Message type. It must be a struct or union.
// _validator	The validator function.
// _user_data	A pointer to the user data.
// _observers	The observers list. The order defines observer priority, with the first observer having the highest priority.
// _init_val	The message initialization.



ZBUS_SUBSCRIBER_DEFINE(validator_sub, 4);
ZBUS_SUBSCRIBER_DEFINE(actuator_sub, 4);
ZBUS_SUBSCRIBER_DEFINE(net_sub, 4);

ZBUS_CHAN_DEFINE(chan_verify_request, struct verify_request, NULL, NULL,
ZBUS_OBSERVERS(validator_sub), ZBUS_MSG_INIT(.req_id = 0));

ZBUS_CHAN_DEFINE(chan_verify_result, struct verify_result, NULL, NULL,
ZBUS_OBSERVERS(actuator_sub, net_sub),
ZBUS_MSG_INIT(.req_id = 0, .status = VERIFY_OK, .allowed = false));
