/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EPI_API_CLIENT_H_
#define EPI_API_CLIENT_H_

#include "zbus_channels.h"

/**
 * Consulta regras de EPI via mock local ou HTTP (api/db.json + POST /check).
 *
 * @return 0 em sucesso, código negativo em falha de rede/parse.
 */
int api_check(const struct verify_request *req, struct verify_result *out);

/** Teste TCP ate a URL configurada (epi api). */
int api_ping(void);

#endif /* EPI_API_CLIENT_H_ */
