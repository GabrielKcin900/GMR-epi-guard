/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "json_codec.h"
#include "epi_names.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/data/json.h>
#include <zephyr/sys/util.h>

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
	size_t pos = 0;
	int ret;

	if (req == NULL || buf == NULL || buflen == 0) {
		return -EINVAL;
	}

	ret = snprintk(buf, buflen, "{\"who\":\"%s\",\"with\":[", req->who);
	if (ret < 0 || (size_t)ret >= buflen) {
		return -ENOSPC;
	}
	pos = (size_t)ret;

	for (uint8_t i = 0; i < req->item_count; i++) {
		if (i > 0) {
			ret = snprintk(buf + pos, buflen - pos, ",");
			if (ret < 0 || (size_t)ret >= buflen - pos) {
				return -ENOSPC;
			}
			pos += (size_t)ret;
		}

		ret = snprintk(buf + pos, buflen - pos, "\"%s\"", req->items[i]);
		if (ret < 0 || (size_t)ret >= buflen - pos) {
			return -ENOSPC;
		}
		pos += (size_t)ret;
	}

	ret = snprintk(buf + pos, buflen - pos, "]}");
	if (ret < 0 || (size_t)ret >= buflen - pos) {
		return -ENOSPC;
	}

	return 0;
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

int json_encode_verify_response(const struct verify_result *res, char *buf, size_t buflen)
{
	size_t pos = 0;
	int ret;
	const char *status_str;

	if (res == NULL || buf == NULL || buflen == 0) {
		return -EINVAL;
	}

	status_str = (res->status == VERIFY_OK) ? "ok" : "error";

	ret = snprintk(buf, buflen, "{\"status\":\"%s\",\"allowed\":%s,\"missing\":[",
		       status_str, res->allowed ? "true" : "false");
	if (ret < 0 || (size_t)ret >= buflen) {
		return -ENOSPC;
	}
	pos = (size_t)ret;

	for (uint8_t i = 0; i < res->missing_count; i++) {
		if (i > 0) {
			ret = snprintk(buf + pos, buflen - pos, ",");
			if (ret < 0 || (size_t)ret >= buflen - pos) {
				return -ENOSPC;
			}
			pos += (size_t)ret;
		}

		ret = snprintk(buf + pos, buflen - pos, "\"%s\"", res->missing[i]);
		if (ret < 0 || (size_t)ret >= buflen - pos) {
			return -ENOSPC;
		}
		pos += (size_t)ret;
	}

	ret = snprintk(buf + pos, buflen - pos, "]");
	if (ret < 0 || (size_t)ret >= buflen - pos) {
		return -ENOSPC;
	}
	pos += (size_t)ret;

	if (res->unknown_person) {
		ret = snprintk(buf + pos, buflen - pos, ",\"unknown_person\":true");
		if (ret < 0 || (size_t)ret >= buflen - pos) {
			return -ENOSPC;
		}
		pos += (size_t)ret;
	}

	ret = snprintk(buf + pos, buflen - pos, "}");
	if (ret < 0 || (size_t)ret >= buflen - pos) {
		return -ENOSPC;
	}

	return 0;
}
