/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api_client.h"
#include "api_client_internal.h"

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(api_client, LOG_LEVEL_INF);

int api_check(const struct verify_request *req, struct verify_result *out)
{
	if (req == NULL || out == NULL) {
		return -EINVAL;
	}

#if IS_ENABLED(CONFIG_EPI_API_USE_MOCK)
	return api_check_mock(req, out);
#else
	return api_check_http(req, out);
#endif
}

int api_ping(void)
{
#if IS_ENABLED(CONFIG_EPI_API_USE_MOCK)
	return 0;
#else
	return api_ping_http();
#endif
}
