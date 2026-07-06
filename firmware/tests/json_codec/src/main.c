/*
 * SPDX-License-Identifier: Apache-2.0
 * ZTEST — json_codec (parse/serialize puro).
 */

#include "json_codec.h"

#include <string.h>

#include <zephyr/ztest.h>

static void fill_request(struct verify_request *req, const char *who,
			 const char *const *items, uint8_t count)
{
	memset(req, 0, sizeof(*req));
	req->req_id = 42;
	strncpy(req->who, who, sizeof(req->who) - 1);

	for (uint8_t i = 0; i < count && i < EPI_MAX_ITEMS; i++) {
		strncpy(req->items[i], items[i], EPI_NAME_LEN - 1);
		req->item_count++;
	}
}

ZTEST(json_codec, test_encode_check_request)
{
	struct verify_request req;
	const char *items[] = { "Capacete", "Oculos" };
	char buf[128];

	fill_request(&req, "Gabriel", items, 2);

	zassert_ok(json_encode_check_request(&req, buf, sizeof(buf)));
	zassert_true(strstr(buf, "\"who\":\"Gabriel\"") != NULL);
	zassert_true(strstr(buf, "\"Capacete\"") != NULL);
	zassert_true(strstr(buf, "\"Oculos\"") != NULL);
}

ZTEST(json_codec, test_encode_check_request_buffer_small)
{
	struct verify_request req;
	const char *items[] = { "Capacete" };
	char buf[8];

	fill_request(&req, "Gabriel", items, 1);
	zassert_equal(-ENOSPC, json_encode_check_request(&req, buf, sizeof(buf)));
}

ZTEST(json_codec, test_parse_verify_request_valid)
{
	char json[] = "{\"who\":\"Ana\",\"with\":[\"Capacete\",\"Luva\"]}";
	struct verify_request req;

	zassert_ok(json_parse_verify_request(json, strlen(json), &req));
	zassert_mem_equal("Ana", req.who, 4);
	zassert_equal(2, req.item_count);
	zassert_true(strcmp(req.items[0], "Capacete") == 0);
	zassert_true(strcmp(req.items[1], "Luva") == 0);
}

ZTEST(json_codec, test_parse_verify_request_missing_who)
{
	char json[] = "{\"with\":[\"Capacete\"]}";
	struct verify_request req;

	zassert_equal(-EINVAL, json_parse_verify_request(json, strlen(json), &req));
}

ZTEST(json_codec, test_parse_verify_request_malformed)
{
	char json[] = "{not json";
	struct verify_request req;

	zassert_true(json_parse_verify_request(json, strlen(json), &req) < 0);
}

ZTEST(json_codec, test_parse_check_response_allowed)
{
	char json[] = "{\"allowed\":true,\"unknown\":false,\"missing\":[]}";
	struct verify_result res;

	memset(&res, 0, sizeof(res));
	zassert_ok(json_parse_check_response(json, strlen(json), &res));
	zassert_equal(VERIFY_OK, res.status);
	zassert_true(res.allowed);
	zassert_false(res.unknown_person);
	zassert_equal(0, res.missing_count);
}

ZTEST(json_codec, test_parse_check_response_missing_items)
{
	char json[] = "{\"allowed\":false,\"unknown\":false,\"missing\":[\"Bota\",\"Luva\"]}";
	struct verify_result res;

	memset(&res, 0, sizeof(res));
	zassert_ok(json_parse_check_response(json, strlen(json), &res));
	zassert_false(res.allowed);
	zassert_equal(2, res.missing_count);
	zassert_mem_equal("Bota", res.missing[0], 5);
	zassert_mem_equal("Luva", res.missing[1], 5);
}

ZTEST(json_codec, test_parse_check_response_unknown_person)
{
	char json[] = "{\"allowed\":false,\"unknown\":true,\"missing\":[]}";
	struct verify_result res;

	memset(&res, 0, sizeof(res));
	zassert_ok(json_parse_check_response(json, strlen(json), &res));
	zassert_true(res.unknown_person);
	zassert_false(res.allowed);
}

ZTEST(json_codec, test_encode_verify_response_ok)
{
	struct verify_result res;
	char buf[192];

	memset(&res, 0, sizeof(res));
	res.status = VERIFY_OK;
	res.allowed = true;
	res.missing_count = 0;

	zassert_ok(json_encode_verify_response(&res, buf, sizeof(buf)));
	zassert_true(strstr(buf, "\"status\":\"ok\"") != NULL);
	zassert_true(strstr(buf, "\"allowed\":true") != NULL);
}

ZTEST(json_codec, test_encode_verify_response_error_with_missing)
{
	struct verify_result res;
	char buf[256];

	memset(&res, 0, sizeof(res));
	res.status = VERIFY_ERROR;
	res.allowed = false;
	strncpy(res.missing[0], "Bota", EPI_NAME_LEN - 1);
	res.missing_count = 1;

	zassert_ok(json_encode_verify_response(&res, buf, sizeof(buf)));
	zassert_true(strstr(buf, "\"status\":\"error\"") != NULL);
	zassert_true(strstr(buf, "\"Bota\"") != NULL);
}

ZTEST(json_codec, test_encode_verify_response_unknown_flag)
{
	struct verify_result res;
	char buf[256];

	memset(&res, 0, sizeof(res));
	res.status = VERIFY_OK;
	res.unknown_person = true;

	zassert_ok(json_encode_verify_response(&res, buf, sizeof(buf)));
	zassert_true(strstr(buf, "\"unknown_person\":true") != NULL);
}

ZTEST(json_codec, test_encode_check_request_null_args)
{
	struct verify_request req;
	char buf[64];

	memset(&req, 0, sizeof(req));
	zassert_equal(-EINVAL, json_encode_check_request(NULL, buf, sizeof(buf)));
	zassert_equal(-EINVAL, json_encode_check_request(&req, NULL, sizeof(buf)));
	zassert_equal(-EINVAL, json_encode_check_request(&req, buf, 0));
}

ZTEST(json_codec, test_encode_check_request_empty_items)
{
	struct verify_request req;
	char buf[64];

	fill_request(&req, "Ana", NULL, 0);
	zassert_ok(json_encode_check_request(&req, buf, sizeof(buf)));
	zassert_true(strstr(buf, "\"with\":[]") != NULL);
}

ZTEST(json_codec, test_parse_check_response_null_args)
{
	char json[] = "{}";
	struct verify_result res;

	zassert_equal(-EINVAL, json_parse_check_response(NULL, 0, &res));
	zassert_equal(-EINVAL, json_parse_check_response(json, strlen(json), NULL));
}

ZTEST(json_codec, test_parse_check_response_absent_fields)
{
	/* campos ausentes sao opcionais: default false/vazio */
	char json[] = "{\"allowed\":true}";
	struct verify_result res;

	memset(&res, 0xFF, sizeof(res));
	zassert_ok(json_parse_check_response(json, strlen(json), &res));
	zassert_true(res.allowed);
	zassert_false(res.unknown_person);
	zassert_equal(0, res.missing_count);
}

ZTEST(json_codec, test_parse_verify_request_folds_accents)
{
	/* "João" + "Ó" "culos" em UTF-8 viram ASCII apos o parse */
	char json[] = "{\"who\":\"Jo\xC3\xA3o\",\"with\":[\"\xC3\x93" "culos\"]}";
	struct verify_request req;

	zassert_ok(json_parse_verify_request(json, strlen(json), &req));
	zassert_true(strcmp(req.who, "Joao") == 0);
	zassert_equal(1, req.item_count);
	zassert_true(strcmp(req.items[0], "Oculos") == 0);
}

ZTEST(json_codec, test_encode_verify_response_null_args)
{
	struct verify_result res;
	char buf[64];

	memset(&res, 0, sizeof(res));
	zassert_equal(-EINVAL, json_encode_verify_response(NULL, buf, sizeof(buf)));
	zassert_equal(-EINVAL, json_encode_verify_response(&res, NULL, sizeof(buf)));
	zassert_equal(-EINVAL, json_encode_verify_response(&res, buf, 0));
}

ZTEST(json_codec, test_encode_verify_response_buffer_small)
{
	struct verify_result res;
	char buf[16];

	memset(&res, 0, sizeof(res));
	res.status = VERIFY_OK;
	res.allowed = true;

	zassert_equal(-ENOSPC, json_encode_verify_response(&res, buf, sizeof(buf)));
}

ZTEST_SUITE(json_codec, NULL, NULL, NULL, NULL, NULL);
