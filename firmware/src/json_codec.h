/*
 * SPDX-License-Identifier: Apache-2.0
 * Parse/serialize JSON — logica pura, sem I/O (plano §8).
 */

#ifndef EPI_JSON_CODEC_H_
#define EPI_JSON_CODEC_H_

#include "zbus_channels.h"

/**
 * Serializa pedido de verificacao: {"who":"...","with":[...]}.
 *
 * @return 0 em sucesso, -ENOSPC se buffer pequeno, outro codigo negativo em erro.
 */
int json_encode_check_request(const struct verify_request *req, char *buf, size_t buflen);

/**
 * Interpreta resposta da API: { allowed, unknown?, missing[] }.
 *
 * @return 0 em sucesso, codigo negativo se JSON invalido.
 */
int json_parse_check_response(const char *json, size_t len, struct verify_result *out);

#endif /* EPI_JSON_CODEC_H_ */
