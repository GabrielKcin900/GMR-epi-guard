/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EPI_API_CLIENT_INTERNAL_H_
#define EPI_API_CLIENT_INTERNAL_H_

#include "zbus_channels.h"

int api_check_mock(const struct verify_request *req, struct verify_result *out);
int api_check_http(const struct verify_request *req, struct verify_result *out);
int api_ping_http(void);

#endif /* EPI_API_CLIENT_INTERNAL_H_ */
