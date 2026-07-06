/*
 * SPDX-License-Identifier: Apache-2.0
 * ZTEST — fluxo zbus: request -> api_check_mock -> result.
 */

#include "api_client_internal.h"
#include "zbus_channels.h"

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/ztest.h>

extern struct zbus_observer validator_sub;
extern struct zbus_observer result_sub;

static struct verify_result last_result;

static void fill_request(struct verify_request *req, uint32_t id, const char *who,
			 const char *const *items, uint8_t count)
{
	memset(req, 0, sizeof(*req));
	req->req_id = id;
	strncpy(req->who, who, sizeof(req->who) - 1);

	for (uint8_t i = 0; i < count && i < EPI_MAX_ITEMS; i++) {
		strncpy(req->items[i], items[i], EPI_NAME_LEN - 1);
		req->item_count++;
	}
}

static void validator_thread(void)
{
	const struct zbus_channel *chan;

	while (!zbus_sub_wait(&validator_sub, &chan, K_FOREVER)) {
		struct verify_request req;
		struct verify_result res;
		int err;

		if (chan != &chan_verify_request) {
			continue;
		}

		if (zbus_chan_read(&chan_verify_request, &req, K_MSEC(500)) < 0) {
			continue;
		}

		memset(&res, 0, sizeof(res));
		res.req_id = req.req_id;

		err = api_check_mock(&req, &res);
		res.req_id = req.req_id;
		if (err < 0) {
			res.status = VERIFY_ERROR;
			res.allowed = false;
		}

		(void)zbus_chan_pub(&chan_verify_result, &res, K_SECONDS(1));
	}
}

K_THREAD_DEFINE(validator_tid, 2048, validator_thread, NULL, NULL, NULL, 5, 0, 0);

static int wait_for_result(uint32_t req_id, k_timeout_t timeout)
{
	const struct zbus_channel *chan;

	if (zbus_sub_wait(&result_sub, &chan, timeout) != 0) {
		return -ETIMEDOUT;
	}

	if (chan != &chan_verify_result) {
		return -ENOENT;
	}

	if (zbus_chan_read(&chan_verify_result, &last_result, K_MSEC(500)) < 0) {
		return -EIO;
	}

	if (last_result.req_id != req_id) {
		return -EINVAL;
	}

	return 0;
}

static void *zbus_flow_setup(void)
{
	k_sleep(K_MSEC(50));
	return NULL;
}

ZTEST(zbus_flow, test_gabriel_allowed)
{
	struct verify_request req;
	const char *items[] = { "Capacete", "Oculos", "Cinto", "Bota" };

	fill_request(&req, 1, "Gabriel", items, 4);
	zassert_ok(zbus_chan_pub(&chan_verify_request, &req, K_SECONDS(1)));
	zassert_ok(wait_for_result(1, K_SECONDS(2)));
	zassert_equal(VERIFY_OK, last_result.status);
	zassert_true(last_result.allowed);
	zassert_false(last_result.unknown_person);
}

ZTEST(zbus_flow, test_gabriel_missing_bota)
{
	struct verify_request req;
	const char *items[] = { "Capacete", "Oculos", "Cinto" };

	fill_request(&req, 2, "Gabriel", items, 3);
	zassert_ok(zbus_chan_pub(&chan_verify_request, &req, K_SECONDS(1)));
	zassert_ok(wait_for_result(2, K_SECONDS(2)));
	zassert_equal(VERIFY_OK, last_result.status);
	zassert_false(last_result.allowed);
	zassert_equal(1, last_result.missing_count);
	zassert_mem_equal("Bota", last_result.missing[0], 5);
}

ZTEST(zbus_flow, test_unknown_person)
{
	struct verify_request req;
	const char *items[] = { "Capacete" };

	fill_request(&req, 3, "Desconhecido", items, 1);
	zassert_ok(zbus_chan_pub(&chan_verify_request, &req, K_SECONDS(1)));
	zassert_ok(wait_for_result(3, K_SECONDS(2)));
	zassert_equal(VERIFY_OK, last_result.status);
	zassert_true(last_result.unknown_person);
	zassert_false(last_result.allowed);
}

ZTEST_SUITE(zbus_flow, NULL, zbus_flow_setup, NULL, NULL, NULL);
