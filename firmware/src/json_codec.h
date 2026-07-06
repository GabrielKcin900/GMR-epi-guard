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
 * @param json buffer mutavel (parser do Zephyr escreve '\\0' nas strings).
 */
int json_parse_check_response(char *json, size_t len, struct verify_result *out);

/**
 * Interpreta pedido do dashboard POST /verify: { who, with[] }.
 * @param json buffer mutavel.
 */
int json_parse_verify_request(char *json, size_t len, struct verify_request *out);

/**
 * Serializa resposta ao dashboard: { status, allowed, missing[], unknown_person }.
 */
int json_encode_verify_response(const struct verify_result *res, char *buf, size_t buflen);

#endif /* EPI_JSON_CODEC_H_ */
