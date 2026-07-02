/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "json_codec.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/data/json.h>
#include <zephyr/sys/util.h>

struct check_response_json {
	bool     allowed;
	bool     unknown;
	char     missing[EPI_MAX_ITEMS][EPI_NAME_LEN];
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

int json_parse_check_response(const char *json, size_t len, struct verify_result *out)
{
	struct check_response_json parsed;
	char *mutable;
	int64_t ret;

	if (json == NULL || out == NULL) {
		return -EINVAL;
	}

	memset(&parsed, 0, sizeof(parsed));

	mutable = (char *)json;
	ret = json_obj_parse(mutable, len, check_resp_descr, ARRAY_SIZE(check_resp_descr),
			     &parsed);
	if (ret < 0) {
		return (int)ret;
	}

	out->status = VERIFY_OK;
	out->allowed = parsed.allowed;
	out->unknown_person = parsed.unknown;
	out->missing_count = 0;

	for (size_t i = 0; i < parsed.missing_len && i < EPI_MAX_ITEMS; i++) {
		strncpy(out->missing[out->missing_count], parsed.missing[i], EPI_NAME_LEN - 1);
		out->missing[out->missing_count][EPI_NAME_LEN - 1] = '\0';
		out->missing_count++;
	}

	return 0;
}
