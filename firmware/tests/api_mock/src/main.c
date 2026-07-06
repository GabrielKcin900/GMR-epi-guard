/*
 * SPDX-License-Identifier: Apache-2.0
 * ZTEST — api_client_mock (regras de EPI por pessoa, casos de borda).
 */

#include "api_client_internal.h"

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

static void fill_request(struct verify_request *req, const char *who,
			 const char *const *items, uint8_t count)
{
	memset(req, 0, sizeof(*req));
	req->req_id = 1;
	strncpy(req->who, who, sizeof(req->who) - 1);

	for (uint8_t i = 0; i < count && i < EPI_MAX_ITEMS; i++) {
		strncpy(req->items[i], items[i], EPI_NAME_LEN - 1);
		req->item_count++;
	}
}

ZTEST(api_mock, test_null_args)
{
	struct verify_request req;
	struct verify_result res;

	zassert_equal(-EINVAL, api_check_mock(NULL, &res));
	zassert_equal(-EINVAL, api_check_mock(&req, NULL));
	zassert_equal(-EINVAL, api_check_mock(NULL, NULL));
}

ZTEST(api_mock, test_ana_allowed)
{
	struct verify_request req;
	struct verify_result res;
	const char *items[] = { "Capacete", "Luva" };

	fill_request(&req, "Ana", items, 2);
	zassert_ok(api_check_mock(&req, &res));
	zassert_equal(VERIFY_OK, res.status);
	zassert_true(res.allowed);
	zassert_equal(0, res.missing_count);
	zassert_false(res.unknown_person);
}

ZTEST(api_mock, test_order_independent)
{
	struct verify_request req;
	struct verify_result res;
	const char *items[] = { "Luva", "Capacete" };

	fill_request(&req, "Ana", items, 2);
	zassert_ok(api_check_mock(&req, &res));
	zassert_true(res.allowed);
}

ZTEST(api_mock, test_extra_items_still_allowed)
{
	struct verify_request req;
	struct verify_result res;
	const char *items[] = { "Capacete", "Luva", "Bota", "Cinto" };

	fill_request(&req, "Ana", items, 4);
	zassert_ok(api_check_mock(&req, &res));
	zassert_true(res.allowed);
	zassert_equal(0, res.missing_count);
}

ZTEST(api_mock, test_joao_missing_two)
{
	struct verify_request req;
	struct verify_result res;
	const char *items[] = { "Capacete" };

	fill_request(&req, "Joao", items, 1);
	zassert_ok(api_check_mock(&req, &res));
	zassert_false(res.allowed);
	zassert_equal(2, res.missing_count);
	zassert_true(strcmp(res.missing[0], "Oculos") == 0);
	zassert_true(strcmp(res.missing[1], "Protetor auricular") == 0);
}

ZTEST(api_mock, test_accented_item_matches)
{
	struct verify_request req;
	struct verify_result res;
	/* "Ó" "culos" em UTF-8 deve casar com o requisito "Oculos" */
	const char *items[] = { "Capacete", "\xC3\x93" "culos", "Cinto", "Bota" };

	fill_request(&req, "Gabriel", items, 4);
	zassert_ok(api_check_mock(&req, &res));
	zassert_true(res.allowed);
}

ZTEST(api_mock, test_unknown_person)
{
	struct verify_request req;
	struct verify_result res;
	const char *items[] = { "Capacete" };

	fill_request(&req, "Intruso", items, 1);
	zassert_ok(api_check_mock(&req, &res));
	zassert_true(res.unknown_person);
	zassert_false(res.allowed);
	zassert_equal(0, res.missing_count);
}

ZTEST(api_mock, test_empty_items_all_missing)
{
	struct verify_request req;
	struct verify_result res;

	fill_request(&req, "Gabriel", NULL, 0);
	zassert_ok(api_check_mock(&req, &res));
	zassert_false(res.allowed);
	zassert_equal(4, res.missing_count);
}

ZTEST_SUITE(api_mock, NULL, NULL, NULL, NULL, NULL);
