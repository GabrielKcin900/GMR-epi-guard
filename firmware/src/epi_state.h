/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EPI_STATE_H_
#define EPI_STATE_H_

#include "zbus_channels.h"

void epi_state_store_request(const struct verify_request *req);
void epi_state_store_result(const struct verify_result *res);

void epi_state_get_last_request(struct verify_request *req);
void epi_state_get_last_result(struct verify_result *res);

bool epi_state_has_result(void);

#endif /* EPI_STATE_H_ */
