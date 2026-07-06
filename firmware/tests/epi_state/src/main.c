/*
 * SPDX-License-Identifier: Apache-2.0
 * ZTEST — epi_state (estado compartilhado: ultimo pedido/resultado, req_id).
 *
 * O estado e global; o teste de ciclo de vida cobre o antes/depois num caso
 * unico para nao depender da ordem de execucao das demais funcoes.
 */

#include "epi_state.h"

#include <string.h>

#include <zephyr/ztest.h>

ZTEST(epi_state, test_result_lifecycle)
{
	struct verify_result in;
	struct verify_result out;

	zassert_false(epi_state_has_result());

	memset(&in, 0, sizeof(in));
	in.req_id = 7;
	in.status = VERIFY_OK;
	in.allowed = false;
	in.missing_count = 1;
	strncpy(in.missing[0], "Bota", EPI_NAME_LEN - 1);

	epi_state_store_result(&in);
	zassert_true(epi_state_has_result());

	memset(&out, 0xFF, sizeof(out));
	epi_state_get_last_result(&out);
	zassert_equal(7, out.req_id);
	zassert_equal(VERIFY_OK, out.status);
	zassert_false(out.allowed);
	zassert_equal(1, out.missing_count);
	zassert_mem_equal("Bota", out.missing[0], 5);
}

ZTEST(epi_state, test_request_roundtrip)
{
	struct verify_request in;
	struct verify_request out;

	memset(&in, 0, sizeof(in));
	in.req_id = 99;
	strncpy(in.who, "Ana", EPI_WHO_LEN - 1);
	strncpy(in.items[0], "Capacete", EPI_NAME_LEN - 1);
	in.item_count = 1;

	epi_state_store_request(&in);

	memset(&out, 0, sizeof(out));
	epi_state_get_last_request(&out);
	zassert_equal(99, out.req_id);
	zassert_mem_equal("Ana", out.who, 4);
	zassert_equal(1, out.item_count);
}

ZTEST(epi_state, test_null_does_not_overwrite)
{
	struct verify_request in;
	struct verify_request out;

	memset(&in, 0, sizeof(in));
	in.req_id = 123;
	epi_state_store_request(&in);

	epi_state_store_request(NULL);
	epi_state_store_result(NULL);
	epi_state_get_last_request(NULL);
	epi_state_get_last_result(NULL);

	epi_state_get_last_request(&out);
	zassert_equal(123, out.req_id);
}

ZTEST(epi_state, test_alloc_req_id_monotonic)
{
	uint32_t first = epi_state_alloc_req_id();
	uint32_t second = epi_state_alloc_req_id();
	uint32_t third = epi_state_alloc_req_id();

	zassert_equal(first + 1, second);
	zassert_equal(second + 1, third);
}

ZTEST_SUITE(epi_state, NULL, NULL, NULL, NULL, NULL);
