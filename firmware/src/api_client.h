/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EPI_API_CLIENT_H_
#define EPI_API_CLIENT_H_

#include "zbus_channels.h"

/**
 * Consulta regras de EPI (M1: mock local espelhando db.json do plano).
 * M3 substituirá por HTTP client real.
 *
 * @return 0 em sucesso, código negativo em falha.
 */
int api_check(const struct verify_request *req, struct verify_result *out);

#endif /* EPI_API_CLIENT_H_ */
