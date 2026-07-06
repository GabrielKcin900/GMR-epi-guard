/*
 * SPDX-License-Identifier: Apache-2.0
 * Parse e serializacao 100% via subsistema data/json do Zephyr
 * (descritores + json_obj_parse/json_obj_encode_buf).
 */

#include "json_codec.h"
#include "epi_names.h"

#include <errno.h>
#include <string.h>

#include <zephyr/data/json.h>
#include <zephyr/sys/util.h>

struct check_request_json {
	const char *who;
	const char *with[EPI_MAX_ITEMS];
	size_t      with_len;
};

static const struct json_obj_descr check_req_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct check_request_json, who, JSON_TOK_STRING),
	JSON_OBJ_DESCR_ARRAY(struct check_request_json, with, EPI_MAX_ITEMS, with_len,
			     JSON_TOK_STRING),
};

struct check_response_json {
	bool     allowed;
	bool     unknown;
	/* JSON_TOK_STRING em array grava ponteiros (char*), nao buffers fixos */
	char    *missing[EPI_MAX_ITEMS];
	size_t   missing_len;
};

static const struct json_obj_descr check_resp_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct check_response_json, allowed, JSON_TOK_TRUE),
	JSON_OBJ_DESCR_PRIM(struct check_response_json, unknown, JSON_TOK_TRUE),
	JSON_OBJ_DESCR_ARRAY(struct check_response_json, missing, EPI_MAX_ITEMS,
			     missing_len, JSON_TOK_STRING),
};

int json_encode_check_request(const struct verify_request *req, char *buf, size_t buflen)
{
	struct check_request_json obj;
	int ret;

	if (req == NULL || buf == NULL || buflen == 0) {
		return -EINVAL;
	}

	obj.who = req->who;
	obj.with_len = MIN(req->item_count, EPI_MAX_ITEMS);
	for (size_t i = 0; i < obj.with_len; i++) {
		obj.with[i] = req->items[i];
	}

	ret = json_obj_encode_buf(check_req_descr, ARRAY_SIZE(check_req_descr), &obj,
				  buf, buflen);

	return (ret == -ENOMEM) ? -ENOSPC : ret;
}

int json_parse_check_response(char *json, size_t len, struct verify_result *out)
{
	struct check_response_json parsed;
	int64_t ret;

	if (json == NULL || out == NULL) {
		return -EINVAL;
	}

	memset(&parsed, 0, sizeof(parsed));

	ret = json_obj_parse(json, len, check_resp_descr, ARRAY_SIZE(check_resp_descr), &parsed);
	if (ret < 0) {
		return (int)ret;
	}

	out->status = VERIFY_OK;
	out->allowed = parsed.allowed;
	out->unknown_person = parsed.unknown;
	out->missing_count = 0;

	for (size_t i = 0; i < parsed.missing_len && i < EPI_MAX_ITEMS; i++) {
		if (parsed.missing[i] == NULL) {
			continue;
		}
		epi_name_to_ascii(parsed.missing[i], out->missing[out->missing_count],
				  EPI_NAME_LEN);
		out->missing_count++;
	}

	return 0;
}

struct verify_request_json {
	char    *who;
	char    *with[EPI_MAX_ITEMS];
	size_t   with_len;
};

static const struct json_obj_descr verify_req_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct verify_request_json, who, JSON_TOK_STRING),
	JSON_OBJ_DESCR_ARRAY(struct verify_request_json, with, EPI_MAX_ITEMS, with_len,
			     JSON_TOK_STRING),
};

int json_parse_verify_request(char *json, size_t len, struct verify_request *out)
{
	struct verify_request_json parsed;
	int64_t ret;

	if (json == NULL || out == NULL) {
		return -EINVAL;
	}

	memset(&parsed, 0, sizeof(parsed));
	memset(out, 0, sizeof(*out));

	ret = json_obj_parse(json, len, verify_req_descr, ARRAY_SIZE(verify_req_descr), &parsed);
	if (ret < 0 || parsed.who == NULL) {
		return -EINVAL;
	}

	strncpy(out->who, parsed.who, EPI_WHO_LEN - 1);
	epi_name_to_ascii(out->who, out->who, EPI_WHO_LEN);

	for (size_t i = 0; i < parsed.with_len && out->item_count < EPI_MAX_ITEMS; i++) {
		if (parsed.with[i] == NULL) {
			continue;
		}
		epi_name_to_ascii(parsed.with[i], out->items[out->item_count], EPI_NAME_LEN);
		out->item_count++;
	}

	return 0;
}

struct verify_response_json {
	const char *status;
	bool        allowed;
	const char *missing[EPI_MAX_ITEMS];
	size_t      missing_len;
	bool        unknown_person;
};

static const struct json_obj_descr verify_resp_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct verify_response_json, status, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct verify_response_json, allowed, JSON_TOK_TRUE),
	JSON_OBJ_DESCR_ARRAY(struct verify_response_json, missing, EPI_MAX_ITEMS,
			     missing_len, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct verify_response_json, unknown_person, JSON_TOK_TRUE),
};

int json_encode_verify_response(const struct verify_result *res, char *buf, size_t buflen)
{
	struct verify_response_json obj;
	int ret;

	if (res == NULL || buf == NULL || buflen == 0) {
		return -EINVAL;
	}

	obj.status = (res->status == VERIFY_OK) ? "ok" : "error";
	obj.allowed = res->allowed;
	obj.unknown_person = res->unknown_person;
	obj.missing_len = MIN(res->missing_count, EPI_MAX_ITEMS);
	for (size_t i = 0; i < obj.missing_len; i++) {
		obj.missing[i] = res->missing[i];
	}

	ret = json_obj_encode_buf(verify_resp_descr, ARRAY_SIZE(verify_resp_descr), &obj,
				  buf, buflen);

	return (ret == -ENOMEM) ? -ENOSPC : ret;
}
