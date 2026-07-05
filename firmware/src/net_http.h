/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EPI_NET_HTTP_H_
#define EPI_NET_HTTP_H_

#include "zbus_channels.h"

#if IS_ENABLED(CONFIG_EPI_HTTP_SERVER)

void epi_http_init(void);
int epi_http_server_try_start(void);
bool epi_http_is_running(void);
void epi_http_on_verify_result(const struct verify_result *res);

#else

static inline void epi_http_on_verify_result(const struct verify_result *res)
{
	ARG_UNUSED(res);
}

static inline bool epi_http_is_running(void)
{
	return false;
}

#endif /* CONFIG_EPI_HTTP_SERVER */

#endif /* EPI_NET_HTTP_H_ */
