/*
 * SPDX-License-Identifier: Apache-2.0
 * ZTEST — resultado -> padrao de feedback (atuadores).
 */

#include "actuator_feedback.h"

#include <string.h>

#include <zephyr/ztest.h>

static struct verify_result make_result(enum verify_status status, bool allowed,
				      bool unknown)
{
	struct verify_result res;

	memset(&res, 0, sizeof(res));
	res.status = status;
	res.allowed = allowed;
	res.unknown_person = unknown;
	return res;
}

ZTEST(validation, test_feedback_allowed)
{
	struct verify_result res = make_result(VERIFY_OK, true, false);

	zassert_equal(EPI_FEEDBACK_ALLOWED, epi_feedback_from_result(&res));
}

ZTEST(validation, test_feedback_denied)
{
	struct verify_result res = make_result(VERIFY_OK, false, false);

	strncpy(res.missing[0], "Bota", EPI_NAME_LEN - 1);
	res.missing_count = 1;

	zassert_equal(EPI_FEEDBACK_DENIED, epi_feedback_from_result(&res));
}

ZTEST(validation, test_feedback_unknown)
{
	struct verify_result res = make_result(VERIFY_OK, false, true);

	zassert_equal(EPI_FEEDBACK_UNKNOWN, epi_feedback_from_result(&res));
}

ZTEST(validation, test_feedback_error_status)
{
	struct verify_result res = make_result(VERIFY_ERROR, false, false);

	zassert_equal(EPI_FEEDBACK_ERROR, epi_feedback_from_result(&res));
}

ZTEST(validation, test_feedback_null)
{
	zassert_equal(EPI_FEEDBACK_ERROR, epi_feedback_from_result(NULL));
}

ZTEST_SUITE(validation, NULL, NULL, NULL, NULL, NULL);
